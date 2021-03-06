/*
 * PlanSelector.cpp
 *
 *  Created on: Jul 4, 2014
 *      Author: Stefan Jakob
 */

#include <engine/planselector/PlanSelector.h>
#include <engine/planselector/PartialAssignmentPool.h>
#include "engine/AlicaEngine.h"
#include "engine/teamobserver/TeamObserver.h"
#include "engine/planselector/PartialAssignment.h"
#include "engine/model/Plan.h"
#include "engine/RunningPlan.h"
#include "engine/Assignment.h"
#include "engine/model/PlanType.h"
#include "engine/model/AbstractPlan.h"
#include "engine/model/BehaviourConfiguration.h"
#include "engine/model/PlanningProblem.h"
#include "engine/model/Behaviour.h"
#include "engine/IPlanner.h"
#include "engine/planselector/TaskAssignment.h"
#include "engine/model/EntryPoint.h"
#include "engine/collections/RobotProperties.h"
#include "engine/model/State.h"

#include "engine/RunningPlan.h"
#include "engine/model/AbstractPlan.h"

#include "engine/model/Task.h"


namespace alica
{

	PlanSelector::PlanSelector(AlicaEngine* ae, PartialAssignmentPool* pap)
	{
		this->ae = ae;
		this->to = ae->getTeamObserver();
		this->pap = pap;
	}

	PlanSelector::~PlanSelector()
	{
	}

	/**
	 * Edits data from the old running plan to call the method CreateRunningPlan appropriatly.
	 */
	shared_ptr<RunningPlan> PlanSelector::getBestSimilarAssignment(shared_ptr<RunningPlan> rp)
	{
		// Reset set index of the partial assignment multiton
		PartialAssignment::reset(pap);
		// CREATE NEW PLAN LIST
		list<Plan*> newPlanList;
		if (rp->getPlanType() == nullptr)
		{
			newPlanList = list<Plan*>();
			newPlanList.push_back(dynamic_cast<Plan*>(rp->getPlan()));
		}
		else
		{
			newPlanList = rp->getPlanType()->getPlans();
		}
		// GET ROBOTS TO ASSIGN
		auto robots = rp->getAssignment()->getAllRobots();
		return this->createRunningPlan(rp->getParent(), newPlanList, robots, rp, rp->getPlanType());
	}

	/**
	 * Edits data from the old running plan to call the method CreateRunningPlan appropriately.
	 */
	shared_ptr<RunningPlan> PlanSelector::getBestSimilarAssignment(shared_ptr<RunningPlan> rp, shared_ptr<vector<int> > robots)
	{
		// Reset set index of the partial assignment object pool
		PartialAssignment::reset(pap);
		// CREATE NEW PLAN LIST
		list<Plan*> newPlanList;
		if (rp->getPlanType() == nullptr)
		{
			newPlanList = list<Plan*>();
			newPlanList.push_back(dynamic_cast<Plan*>(rp->getPlan()));
		}
		else
		{
			newPlanList = rp->getPlanType()->getPlans();
		}
		return this->createRunningPlan(rp->getParent(), newPlanList, robots, rp, rp->getPlanType());
	}

	/**
	 * Solves the task allocation problem for a given state.
	 * @param parent The RunningPlan in which task allocation is due for the RunningPlan.ActiveState.
	 * @param plans The list of children of the state for which to allocate, a list<AbstractPlan>.
	 * @param robotIDs The set of robots or agents, which are available in a shared_ptr<vector<int> >
	 * @return A shared_ptr<list<shared_ptr<RunningPlan>>>, encoding the solution.
	 */
	shared_ptr<list<shared_ptr<RunningPlan>>> PlanSelector::getPlansForState(shared_ptr<RunningPlan> planningParent,
	                                                               list<alica::AbstractPlan*>* plans,
																	shared_ptr<vector<int> > robotIDs)
	{
		PartialAssignment::reset(pap);
		shared_ptr<list<shared_ptr<RunningPlan>> > ll = this->getPlansForStateInternal(planningParent, plans, robotIDs);
		return ll;

	}

	shared_ptr<RunningPlan> PlanSelector::createRunningPlan(weak_ptr<RunningPlan> planningParent, list<Plan*> plans,
													shared_ptr<vector<int> > robotIDs, shared_ptr<RunningPlan> oldRp,
													PlanType* relevantPlanType)
	{
		list<Plan*> newPlanList;
		// REMOVE EVERY PLAN WITH TOO GREAT MIN CARDINALITY
		for (Plan* plan : plans)
		{
			// CHECK: number of robots < minimum cardinality of this plan
			if (plan->getMinCardinality() >
					(robotIDs->size() + to->successesInPlan(plan) ))
			{
#ifdef PSDEBUG
				stringstream ss;
				ss << "PS: RobotIds: ";
				for (int robot : (*robotIDs))
				{
					ss << robot << ", ";
				}
				ss << "= " << robotIDs->size() << " IDs are not enough for the plan " << plan->getName() << "!" << endl;
				//this.baseModule.Mon.Error(1000, msg);

				cout << ss.str();
#endif
			}
			else
			{
				// this plan was ok according to its cardinalities, so we can add it
				newPlanList.push_back(plan);
			}
		}
		// WE HAVE NOT ENOUGH ROBOTS TO EXECUTE ANY PLAN
		if (newPlanList.size() == 0)
		{
			return nullptr;
		}
		// TASKASSIGNMENT
		TaskAssignment* ta = nullptr;
		shared_ptr<Assignment> oldAss = nullptr;
		shared_ptr<RunningPlan> rp;
		if (oldRp == nullptr)
		{
			// preassign other robots, because we dont need a similar assignment
			rp = make_shared<RunningPlan>(ae, relevantPlanType);
			ta = new TaskAssignment(this->ae->getPartialAssignmentPool(), this->ae->getTeamObserver(), newPlanList, robotIDs, true);
		}
		else
		{
			// dont preassign other robots, because we need a similar assignment (not the same)
			rp = make_shared<RunningPlan>(ae, oldRp->getPlanType());
			ta = new TaskAssignment(this->ae->getPartialAssignmentPool(), this->ae->getTeamObserver(), newPlanList, robotIDs, false);
			oldAss = oldRp->getAssignment();
		}


		// some variables for the do while loop
		EntryPoint* ep = nullptr;
		shared_ptr<RobotProperties> ownRobProb = to->getOwnRobotProperties();
		// PLANNINGPARENT
		rp->setParent(planningParent);
		shared_ptr<list<shared_ptr<RunningPlan>>> rpChildren = nullptr;
		do
		{
			// ASSIGNMENT
			rp->setAssignment(ta->getNextBestAssignment(&*oldAss));
			if (rp->getAssignment() == nullptr)
			{
#ifdef PSDEBUG
				cout << "PS: rp.Assignment is NULL" << endl;
#endif
				return nullptr;
			}

			// PLAN (needed for Conditionchecks)
			rp->setPlan(rp->getAssignment()->getPlan());
#ifdef PSDEBUG
			cout << "PS: rp.Assignment of Plan " << rp->getPlan()->getName() << " from " << ownRobProb->getId() << " is: " << rp->getAssignment()->toString();
#endif
			// CONDITIONCHECK
			if (!rp->evalPreCondition())
			{
				continue;
			}
			if (!rp->evalRuntimeCondition())
			{
				continue;
			}

			// OWN ENTRYPOINT
			ep = rp->getAssignment()->getEntryPointOfRobot(ownRobProb->getId());

			if (ep == nullptr)
			{
#ifdef PSDEBUG
				cout << "PS: The robot " << ownRobProb->getName() << "(Id: " << ownRobProb->getId()
						<< ") is not assigned to enter the plan " << rp->getPlan()->getName() << " and will IDLE!"
						<< endl;
#endif
				rp->setActiveState(nullptr);
				rp->setOwnEntryPoint(nullptr);
				return rp; // If we return here, this robot will idle (no ep at rp)
			}
			else
			{
				// assign found EntryPoint (this robot dont idle)
				rp->setOwnEntryPoint(ep);
			}
			// ACTIVE STATE set by RunningPlan
			if(oldRp == nullptr)
			{
				// RECURSIVE PLANSELECTING FOR NEW STATE
				rpChildren = this->getPlansForStateInternal(rp, &rp->getActiveState()->getPlans(), rp->getAssignment()->getRobotsWorking(ep));
			}
			else
			{
#ifdef PSDEBUG
					cout << "PS: no recursion due to utilitycheck" << endl;
#endif
					// Don't calculate children, because we have an
					// oldRp -> we just replace the oldRp
					// (not its children -> this will happen in an extra call)
					break;
			}
		} while (rpChildren == nullptr);
		// WHEN WE GOT HERE, THIS ROBOT WONT IDLE AND WE HAVE A
		// VALID ASSIGNMENT, WHICH PASSED ALL RUNTIME CONDITIONS
		if(rpChildren != nullptr && rpChildren->size() != 0) // c# rpChildren != null
		{
#ifdef PSDEBUG
				cout << "PS: Set child -> father reference" << endl;
#endif
				rp->addChildren(rpChildren);
		}
#ifdef PSDEBUG
		cout << "PS: Created RunningPlan: \n" << rp->toString() << endl;
#endif
		delete ta;
		return rp; // If we return here, this robot is normal assigned
	}

	shared_ptr<list<shared_ptr<RunningPlan>> > PlanSelector::getPlansForStateInternal(shared_ptr<RunningPlan> planningParent,
	                                                                      list<alica::AbstractPlan*>* plans,
																			shared_ptr<vector<int> > robotIDs)
	{
		shared_ptr<list<shared_ptr<RunningPlan>> > rps = make_shared<list<shared_ptr<RunningPlan>> >();
#ifdef PSDEBUG
		cout << "<######PS: GetPlansForState: Parent:"
						<< (planningParent != nullptr ? planningParent->getPlan()->getName() : "null") << " plan count: "
						<< plans->size() << " robot count: "
						<< robotIDs->size() << " ######>" << endl;
#endif
		shared_ptr<RunningPlan> rp;
		list<Plan*> planList;
		BehaviourConfiguration* bc;
		Plan* p;
		PlanType* pt;
		PlanningProblem* pp;
		for (AbstractPlan* ap : *plans)
		{
			// BEHAVIOUR CONFIGURATION
			bc = dynamic_cast<BehaviourConfiguration*>(ap);
			if (bc != nullptr)
			{
				rp = make_shared<RunningPlan>(ae, bc);
				// A BehaviourConfiguration is a Plan too (in this context)
				rp->setPlan(bc);
				rps->push_back(rp);
				rp->setParent(planningParent);
#ifdef PSDEBUG
				cout << "PS: Added Behaviour " << bc->getBehaviour()->getName() << endl;
#endif
			}
			else
			{
				// PLAN
				p = dynamic_cast<Plan*>(ap);
				if (p != nullptr)
				{
					planList = list<Plan*>();
					planList.push_back(p);
					rp = this->createRunningPlan(planningParent, planList, robotIDs, nullptr, nullptr);
					if (rp == nullptr)
					{
#ifdef PSDEBUG
						cout << "PS: It was not possible to create a RunningPlan for the Plan " << p->getName() << "!"
								<< endl;
#endif
						return nullptr;
					}
					rps->push_back(rp);
				}
				else
				{
					// PLANTYPE
					pt = dynamic_cast<PlanType*>(ap);
					if (pt != nullptr)
					{
						rp = this->createRunningPlan(planningParent, pt->getPlans(), robotIDs, nullptr, pt);
						if (rp == nullptr)
						{
//#ifdef PSDEBUG
							cout << "PS: It was not possible to create a RunningPlan for the Plan " << pt->getName()
									<< "!" << endl;
//#endif
							return nullptr;
						}
						rps->push_back(rp);
					}
					else
					{
						pp = dynamic_cast<PlanningProblem*>(ap);
						if (pp == nullptr)
						{
							cerr
									<< "PS: WTF? An AbstractPlan wasnt a BehaviourConfiguration, a Plan, a PlanType nor a PlannigProblem: "
									<< ap->getId() << endl;
							throw new exception();
						}
						//TODO implement method in planner
						Plan* myP = ae->getPlanner()->requestPlan(pp);
						planList = list<Plan*>();
						planList.push_back(myP);
						rp = this->createRunningPlan(planningParent, planList, robotIDs, nullptr, nullptr);
						if (rp == nullptr)
						{
#ifdef PSDEBUG
							cout << "PS: Unable to execute planning result" << endl;
#endif
							return nullptr;
						}
						rps->push_back(rp);
					}
				}// else Plan
			}// else BehaviourConfiguration
		}// foreach AbstractPlan
		return rps;
	}

} /* namespace alica */

