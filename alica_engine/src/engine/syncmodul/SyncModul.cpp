/*
 * SyncModul.cpp
 *
 *  Created on: Aug 27, 2014
 *      Author: Paul Panin
 */

#include "engine/syncmodul/SyncModul.h"
#include "engine/AlicaEngine.h"
#include "engine/teamobserver/TeamObserver.h"
#include "engine/PlanRepository.h"
#include "engine/syncmodul/Synchronisation.h"
#include "engine/model/Transition.h"
#include "engine/containers/SyncData.h"
#include "engine/containers/SyncReady.h"
#include "engine/containers/SyncTalk.h"
#include "engine/IAlicaCommunication.h"
#include "engine/model/SyncTransition.h"

namespace alica
{

	SyncModul::SyncModul(AlicaEngine* ae)
	{
		this->ae = ae;
		this->myId = 0;
		this->pr = nullptr;
		this->running = false;
		this->ticks = 0;
		this->communicator = nullptr;

	}

	SyncModul::~SyncModul()
	{
		for (auto iter : this->synchSet)
		{
			delete iter.second;
		}
	}
	void SyncModul::init()
	{
		this->ticks = 0;
		this->running = true;
		this->myId = ae->getTeamObserver()->getOwnId();
		this->pr = this->ae->getPlanRepository();
		this->communicator = this->ae->getCommunicator();
	}
	void SyncModul::close()
	{
		this->running = false;
		cout << "SynchModul: Closed SynchModul" << endl;
	}
	void SyncModul::tick()
	{
		list<Synchronisation*> failedSyncs;
		lock_guard<mutex> lock(lomutex);
		for (auto iter : this->synchSet)
		{
			if (!iter.second->isValid(ticks))
			{
				failedSyncs.push_back(iter.second);
			}
			ticks++;
			for (Synchronisation* s : failedSyncs)
			{
				delete this->synchSet[s->getSyncTransition()];
				this->synchSet.erase(s->getSyncTransition());
			}
		}

	}
	void SyncModul::setSynchronisation(Transition* trans, bool holds)
	{
		Synchronisation* s;
		map<SyncTransition*, Synchronisation*>::iterator i = this->synchSet.find(trans->getSyncTransition());
		if (i != this->synchSet.end())
		{
			i->second->setTick(this->ticks);
			i->second->changeOwnData(trans->getId(), holds);
		}
		else
		{
			s = new Synchronisation(ae, myId, trans->getSyncTransition(), this);
			s->setTick(this->ticks);
			s->changeOwnData(trans->getId(), holds);
			lock_guard<mutex> lock(this->lomutex);
			synchSet.insert(pair<SyncTransition*, Synchronisation*>(trans->getSyncTransition(), s));
		}
	}
	void SyncModul::sendSyncTalk(SyncTalk& st)
	{
		if (!this->ae->isMaySendMessages())
			return;
		st.senderID = this->myId;
		this->communicator->sendSyncTalk(st);

	}
	void SyncModul::sendSyncReady(SyncReady& sr)
	{
		if (!this->ae->isMaySendMessages())
			return;
		sr.senderID = this->myId;
		communicator->sendSyncReady(sr);
	}
	void SyncModul::sendAcks(vector<SyncData*> syncDataList)
	{
		if (!this->ae->isMaySendMessages())
			return;
		SyncTalk st;
		st.senderID = this->myId;
		st.syncData = syncDataList;
		this->communicator->sendSyncTalk(st);
	}
	void SyncModul::synchronisationDone(SyncTransition* st)
	{
#ifdef SM_SUCCES
		cout << "SyncDONE in SYNCMODUL for synctransID: " << st->getId() << endl;
#endif

#ifdef SM_SUCCES
			cout << "Remove synchronisation object for syntransID: " << st->getId() << endl;
#endif
		delete this->synchSet[st];
		this->synchSet.erase(st);
		this->synchedTransitions.push_back(st);
#ifdef SM_SUCCES
		cout << "SM: SYNC TRIGGER TIME:" << this->ae->getIAlicaClock()->now()/1000000UL << endl;
#endif
	}

	bool SyncModul::followSyncTransition(Transition* trans)
	{
		list<SyncTransition*>::iterator it = find(this->synchedTransitions.begin(), this->synchedTransitions.end(),
													trans->getSyncTransition());
		if (it != this->synchedTransitions.end())
		{
			this->synchedTransitions.remove(trans->getSyncTransition());
			return true;
		}
		return false;
	}
	void SyncModul::onSyncTalk(shared_ptr<SyncTalk> st)
	{
		if (!this->running || st->senderID == this->myId)
			return;
		if (this->ae->getTeamObserver()->isRobotIgnored(st->senderID))
			return;

#ifdef SM_SUCCES
		cout << "SyncModul:Handle Synctalk" << endl;
#endif

		vector<SyncData*> toAck;
		for (SyncData* sd : st->syncData)
		{
#ifdef SM_SUCCES
			cout << "SyncModul: TransID" << sd->transitionID << endl;
			cout << "SyncModul: RobotID" << sd->robotID << endl;
			cout << "SyncModul: Condition" << sd->conditionHolds << endl;
			cout << "SyncModul: ACK" << sd->ack << endl;
#endif

			Transition* trans = nullptr;
			SyncTransition* syncTrans = nullptr;

			map<long, Transition*>::iterator iter = this->pr->getTransitions().find(sd->transitionID);
			if (iter != this->pr->getTransitions().end())
			{
				trans = iter->second;
				if (trans->getSyncTransition() != nullptr)
				{
					syncTrans = trans->getSyncTransition();
				}
				else
				{
					cerr << "SyncModul: Transition " << trans->getId() << " is not connected to a SyncTransition"
							<< endl;
					return;
				}
			}
			else
			{
				cerr << "SyncModul: Could not find Element for Transition with ID: " << sd->transitionID << endl;
				return;
			}

			Synchronisation* sync = nullptr;
			bool doAck = true;
			{
				lock_guard<mutex> lock(lomutex);
				map<SyncTransition*, Synchronisation*>::iterator i = this->synchSet.find(syncTrans);

				if (i != this->synchSet.end())
				{
					sync = i->second;
					sync->integrateSyncTalk(st, this->ticks);
				}
				else
				{
					sync = new Synchronisation(ae, this->myId, syncTrans, this);
					synchSet.insert(pair<SyncTransition*, Synchronisation*>(syncTrans, sync));
					doAck = sync->integrateSyncTalk(st, this->ticks);
				}

			}
			if (!sd->ack && st->senderID == sd->robotID && doAck)
			{
				toAck.push_back(sd);
			}

		}
		for (SyncData* sd : toAck)
		{
			sd->ack = true;
		}
		if (toAck.size() > 0)
		{
			sendAcks(toAck);
		}

	}
	void SyncModul::onSyncReady(shared_ptr<SyncReady> sr)
	{
		if (!this->running || sr->senderID == this->myId)
			return;
		if (this->ae->getTeamObserver()->isRobotIgnored(sr->senderID))
			return;
		SyncTransition* syncTrans = nullptr;
		map<long, SyncTransition*>::iterator iter = this->pr->getSyncTransitions().find(sr->syncTransitionID);
		if(iter == this->pr->getSyncTransitions().end())
		{
			cout << "SyncModul: Unable to find synchronisation " << sr->syncTransitionID << " send by " << sr->senderID << endl;
			return;
 		}
		else
		{
			syncTrans = iter->second;
		}

		{
			lock_guard<mutex> lock(lomutex);
			map<SyncTransition*, Synchronisation*>::iterator i = this->synchSet.find(syncTrans);
			if(i != this->synchSet.end())
			{
				i->second->integrateSyncReady(sr);
			}
		}

	}

} /* namespace supplementary */
