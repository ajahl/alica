/*
 * AlicaEngine.cpp
 *
 *  Created on: Mar 3, 2014
 *      Author: Stephan Opfer
 */
#define AE_DEBUG

#include "engine/AlicaEngine.h"
#include "engine/PlanRepository.h"
#include "engine/parser/PlanParser.h"
#include "engine/behaviourpool/BehaviourPool.h"
#include "engine/model/RoleSet.h"
#include "engine/ISyncModul.h"
#include "engine/IRoleAssignment.h"
#include "engine/allocationauthority/AuthorityManager.h"
#include "engine/IEngineModule.h"
#include "engine/planselector/PlanSelector.h"
#include "engine/PlanBase.h"
#include "engine/teamobserver/TeamObserver.h"
#include "engine/logging/Logger.h"
#include "engine/roleassignment/RoleAssignment.h"
#include "engine/staticroleassignment/StaticRoleAssignment.h"
#include "engine/UtilityFunction.h"
#include "engine/model/Plan.h"
#include "engine/syncmodul/SyncModul.h"
#include "engine/IConditionCreator.h"
#include "engine/planselector/PartialAssignmentPool.h"
#include "engine/expressionhandler/ExpressionHandler.h"
#include <engine/constraintmodul/ISolver.h>
#include "engine/constraintmodul/VariableSyncModule.h"
#include "engine/collections/AssignmentCollection.h"

using namespace std;
namespace alica
{
	/**
	 * The main class.
	 */
	AlicaEngine::AlicaEngine()
	{
		this->stepCalled = false;
		this->planBase = nullptr;
		this->planner = nullptr;
		this->planSelector = nullptr;
		this->communicator = nullptr;
		this->alicaClock = nullptr;
		this->syncModul = nullptr;
		this->sc = supplementary::SystemConfig::getInstance();
		this->terminating = false;
		this->teamObserver = nullptr;
		this->roleAssignment = nullptr;
		this->behaviourPool = nullptr;
		this->syncModul = nullptr;
		this->roleSet = nullptr;
		this->expressionHandler = nullptr;
		this->masterPlan = nullptr;
		this->planParser = nullptr;
		this->log = nullptr;
		this->planRepository = nullptr;
		this->auth = nullptr;
		this->roleSet = nullptr;
		this->stepEngine = false;
		this->maySendMessages = false;
		this->pap = nullptr;
		this->variableSyncModule = nullptr;
		this->useStaticRoles = false;

#ifdef AE_DEBUG
		cout << "AE: Constructor finished!" << endl;
#endif
	}

	AlicaEngine::~AlicaEngine()
	{
	}

	/**
	 * Intialise the engine
	 * @param bc A behaviourcreator
	 * @param roleSetName A string, the roleset to be used. If empty, a default roleset is looked for
	 * @param masterPlanName A string, the top-level plan to be used
	 * @param roleSetDir A string, the directory in which to search for roleSets. If empty, the base role path will be used.
	 * @param stepEngine A bool, whether or not the engine should start in stepped mode
	 * @return bool true if everything worked false otherwise
	 */
	bool AlicaEngine::init(IBehaviourCreator* bc, IConditionCreator* cc, IUtilityCreator* uc, IConstraintCreator* crc,
							string roleSetName, string masterPlanName, string roleSetDir, bool stepEngine)
	{
		this->maySendMessages = !(*sc)["Alica"]->get<bool>("Alica.SilentStart", NULL);
		this->useStaticRoles = (*sc)["Alica"]->get<bool>("Alica.UseStaticRoles", NULL);
		AssignmentCollection::maxEpsCount = (*this->sc)["Alica"]->get<short>("Alica.MaxEpsPerPlan", NULL);
		AssignmentCollection::allowIdling = (*this->sc)["Alica"]->get<bool>("Alica.AllowIdling", NULL);
		this->terminating = false;
		this->stepEngine = stepEngine;
		if (this->planRepository == nullptr)
		{
			this->planRepository = new PlanRepository();
		}
		if (this->planParser == nullptr)
		{
			this->planParser = new PlanParser(this, this->planRepository);
		}
		if (this->masterPlan == nullptr)
		{
			this->masterPlan = this->planParser->parsePlanTree(masterPlanName);
		}
		if (this->roleSet == nullptr)
		{
			this->roleSet = this->planParser->parseRoleSet(roleSetName, roleSetDir);
		}
		if (this->behaviourPool == nullptr)
		{
			this->behaviourPool = new BehaviourPool(this);
		}
		if (this->teamObserver == nullptr)
		{
			this->teamObserver = new TeamObserver(this);
		}
		if (this->roleAssignment == nullptr)
		{
			if (this->useStaticRoles)
			{
				this->roleAssignment = new StaticRoleAssignment(this);
			}
			else
			{
				this->roleAssignment = new RoleAssignment(this);
			}
			// the communicator is expected to be set before init() is called
			this->roleAssignment->setCommunication(communicator);
		}
		if (this->syncModul == nullptr)
		{
			this->syncModul = new SyncModul(this);
		}
		if (this->expressionHandler == nullptr)
		{
			this->expressionHandler = new ExpressionHandler(this, cc, uc, crc);
		}
		this->stepCalled = false;
		bool everythingWorked = true;
		everythingWorked &= this->behaviourPool->init(bc);
		this->auth = new AuthorityManager(this);
		this->log = new Logger(this);
		this->teamObserver->init();
		this->roleAssignment->init();
		if (this->pap == nullptr)
		{
			pap = new PartialAssignmentPool();
		}
		if (planSelector == nullptr)
		{
			this->planSelector = new PlanSelector(this, pap);
		}
		this->auth->init();
		this->planBase = new PlanBase(this, this->masterPlan);
		this->expressionHandler->attachAll();
		UtilityFunction::initDataStructures(this);
		this->syncModul->init();
		if (this->variableSyncModule == nullptr)
		{
			this->variableSyncModule = new VariableSyncModule(this);
		}
		if (this->getCommunicator() != nullptr)
		{
			this->getCommunicator()->startCommunication();
		}
		if (this->variableSyncModule != nullptr)
		{
			this->variableSyncModule->init();
		}
		return everythingWorked;
	}

	/**
	 * Closes the engine for good.
	 */
	void AlicaEngine::shutdown()
	{
		if (this->getCommunicator() != nullptr)
		{
			this->getCommunicator()->stopCommunication();
		}
		this->terminating = true;
		this->maySendMessages = false;

		if (this->behaviourPool != nullptr)
		{
			this->behaviourPool->stopAll();
			delete this->behaviourPool;
			this->behaviourPool = nullptr;
		}

		if (this->planBase != nullptr)
		{
			this->planBase->stop();
			delete this->planBase;
			this->planBase = nullptr;
		}

		if (this->auth != nullptr)
		{
			this->auth->close();
			delete this->auth;
			this->auth = nullptr;
		}

		if (this->syncModul != nullptr)
		{
			this->syncModul->close();
			delete this->syncModul;
			this->syncModul = nullptr;
		}

		if (this->teamObserver != nullptr)
		{
			this->teamObserver->close();
			delete this->teamObserver;
			this->teamObserver = nullptr;
		}

		if (this->log != nullptr)
		{
			this->log->close();
			delete this->log;
			this->log = nullptr;
		}

		if (this->planRepository != nullptr)
		{
			delete this->planRepository;
			this->planRepository = nullptr;
		}

		if (this->planParser != nullptr)
		{
			delete this->planParser;
			this->planParser = nullptr;
		}

		delete planSelector;
		planSelector = nullptr;

		if (this->pap != nullptr)
		{
			delete this->pap;
			this->pap = nullptr;
		}

		this->roleSet = nullptr;
		this->masterPlan = nullptr;

		if (this->expressionHandler != nullptr)
		{
			delete this->expressionHandler;
			this->expressionHandler = nullptr;
		}

		if (this->variableSyncModule != nullptr)
		{
			delete this->variableSyncModule;
			this->variableSyncModule = nullptr;
		}
		if (this->roleAssignment != nullptr)
		{
			delete this->roleAssignment;
			this->roleAssignment = nullptr;
		}

	}

	/**
	 * Register with this EngineTrigger to be called after an engine iteration is complete.
	 */
	void AlicaEngine::iterationComplete()
	{
		//TODO: implement the trigger function for iteration complete
	}

	/**
	 * Starts the engine.
	 */
	void AlicaEngine::start()
	{
		this->planBase->start();
		cout << "AE: Engine started" << endl;
	}
	void AlicaEngine::setStepCalled(bool stepCalled)
	{
		this->stepCalled = stepCalled;
	}
	bool AlicaEngine::getStepCalled() const
	{
		return this->stepCalled;
	}

	bool AlicaEngine::getStepEngine()
	{
		return this->stepEngine;
	}
	/**
	 * Returns the plan repository, which holds the static ALICA program.
	 */
	PlanRepository * AlicaEngine::getPlanRepository()
	{
		return this->planRepository;
	}

	/**
	 * Returns the planselector
	 */
	IPlanSelector* AlicaEngine::getPlanSelector()
	{
		return this->planSelector;
	}
	/**
	 * Returns the Alica Clock interface
	 */
	IAlicaClock* AlicaEngine::getIAlicaClock()
	{
		return this->alicaClock;
	}
	void AlicaEngine::setIAlicaClock(IAlicaClock* clock)
	{
		this->alicaClock = clock;
	}

	/**
	 * Returns the behaviourpool
	 */
	IBehaviourPool * AlicaEngine::getBehaviourPool()
	{
		return this->behaviourPool;
	}

	/**
	 * Returns the TeamObserver, which handles most communication tasks.
	 */
	ITeamObserver * AlicaEngine::getTeamObserver()
	{
		return this->teamObserver;
	}
	void AlicaEngine::setTeamObserver(ITeamObserver* teamObserver)
	{
		this->teamObserver = teamObserver;
	}

	/**
	 * Gets the SyncModul, which enables synchronized transitions.
	 */
	ISyncModul * AlicaEngine::getSyncModul()
	{
		return syncModul;
	}

	void AlicaEngine::setSyncModul(ISyncModul* syncModul)
	{
		this->syncModul = syncModul;
	}

	/**
	 * Gets AuthorityManager, which detects and resolvs conflicts in task allocation.
	 */
	AuthorityManager * AlicaEngine::getAuth()
	{
		return auth;
	}
	void AlicaEngine::setAuth(AuthorityManager* auth)
	{
		this->auth = auth;
	}

	/**
	 * Gets the RoleAssignment, responsible for allocating roles to robots.
	 */
	IRoleAssignment * AlicaEngine::getRoleAssignment()
	{
		return roleAssignment;
	}

	void AlicaEngine::setRoleAssignment(IRoleAssignment* roleAssignment)
	{
		this->roleAssignment = roleAssignment;
	}

	/**
	 * Returns the parser which reads ALICAs XML representation
	 */
	IPlanParser * AlicaEngine::getPlanParser()
	{
		return planParser;
	}

	/**
	 * Returns the RoleSet in use.
	 */
	RoleSet * AlicaEngine::getRoleSet()
	{
		return roleSet;
	}

	void AlicaEngine::setStepEngine(bool stepEngine)
	{
		this->stepEngine = stepEngine;
	}

	/**
	 * Abort execution with a message, called if initialization fails.
	 * @param msg A string
	 */
	void AlicaEngine::abort(string msg)
	{
		cerr << "ABORT: " << msg << endl;
		exit(EXIT_FAILURE);
	}

	/**
	 * Gets the robot name, either by access the environment variable "ROBOT", or if that isn't set, the hostname.
	 * @return The robot name under which the engine operates, a string
	 */
	string AlicaEngine::getRobotName() const
	{
		return sc->getHostname();
	}

	/**
	 * Gets the Logger
	 */
	Logger * AlicaEngine::getLog()
	{
		return log;
	}

	void AlicaEngine::setLog(Logger* log)
	{
		this->log = log;
	}

	bool AlicaEngine::isTerminating() const
	{
		return terminating;
	}
	bool AlicaEngine::isMaySendMessages() const
	{
		return maySendMessages;
	}
	void AlicaEngine::setMaySendMessages(bool maySendMessages)
	{
		this->maySendMessages = maySendMessages;
	}

	void AlicaEngine::setTerminating(bool terminating)
	{
		this->terminating = terminating;
	}

	IAlicaCommunication * AlicaEngine::getCommunicator()
	{
		return communicator;
	}

	void AlicaEngine::setCommunicator(IAlicaCommunication * communicator)
	{
		this->communicator = communicator;
	}

	/**
	 * Returns the problem planner
	 */
	IPlanner* AlicaEngine::getPlanner()
	{
		return planner;
	}

	/**
	 * Returns Alica Main clase that manages the current alica state
	 */
	PlanBase* AlicaEngine::getPlanBase()
	{
		return planBase;
	}

	void AlicaEngine::addSolver(int identifier, ISolver* solver)
	{
		this->solver[identifier] = solver;
	}

	ISolver* AlicaEngine::getSolver(int identifier)
	{
		return this->solver[identifier];
	}

	IVariableSyncModule* AlicaEngine::getResultStore()
	{
		return this->variableSyncModule;
	}

	void AlicaEngine::setResultStore(IVariableSyncModule* resultStore)
	{
		this->variableSyncModule = resultStore;
	}

	PartialAssignmentPool* AlicaEngine::getPartialAssignmentPool()
	{
		return this->pap;
	}
	/**
	 * Triggers the engine to run one iteration.
	 * Attention: This method call is asynchronous to the triggered iteration.
	 * So please wait long enough to let the engine do their stuff of its iteration,
	 * before you read values, which will be changed by this iteration.
	 */
	void AlicaEngine::stepNotify()
	{
		this->setStepCalled(true);
		this->getPlanBase()->getStepModeCV()->notify_all();
	}

} /* namespace Alica */

