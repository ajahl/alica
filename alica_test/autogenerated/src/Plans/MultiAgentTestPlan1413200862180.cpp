#include "Plans/MultiAgentTestPlan1413200862180.h"
using namespace alica;
/*PROTECTED REGION ID(eph1413200862180) ENABLED START*/ //Add additional using directives here
#include "TestWorldModel.h"
/*PROTECTED REGION END*/
namespace alicaAutogenerated
{
//Plan:MultiAgentTestPlan

/* generated comment
 
 Task: AttackTask  -> EntryPoint-ID: 1413200877337

 Task: DefaultTask  -> EntryPoint-ID: 1413200890537

 Task: DefaultTask  -> EntryPoint-ID: 1413807260446

 */
shared_ptr<UtilityFunction> UtilityFunction1413200862180::getUtilityFunction(Plan* plan)
{
  /*PROTECTED REGION ID(1413200862180) ENABLED START*/

  shared_ptr < UtilityFunction > defaultFunction = make_shared < DefaultUtilityFunction > (plan);
  return defaultFunction;

  /*PROTECTED REGION END*/
}

//State: OtherState in Plan: MultiAgentTestPlan

/*
 *		
 * Transition:
 *   - Name: , ConditionString: , Comment :  
 *
 * Plans in State: 				
 *   - Plan - (Name): AttackDefault, (PlanID): 1402488866727 
 *
 * Tasks: 
 *   - AttackTask (1407153522080) (Entrypoint: 1413200877337)
 *   - DefaultTask (1225112227903) (Entrypoint: 1413200890537)
 *   - DefaultTask (1225112227903) (Entrypoint: 1413807260446)
 *
 * States:
 *   - OtherState (1413200877336)
 *   - State1 (1413200910490)
 *   - State2 (1413201030936)
 *   - NewSuccessState1 (1413201164999)
 *   - NewSuccessState2 (1413552736921)
 *   - Idle (1413807264574)
 *
 * Vars:
 */
bool TransitionCondition1413201370590::evaluate(shared_ptr<RunningPlan> rp)
{
  /*PROTECTED REGION ID(1413201368286) ENABLED START*/
	if(rp->getOwnID() == 8)
	{
		return alicaTests::TestWorldModel::getOne()->isTransitionCondition1413201370590();
	}
	else
	{
		return alicaTests::TestWorldModel::getTwo()->isTransitionCondition1413201370590();
	}
  /*PROTECTED REGION END*/

}

//State: State1 in Plan: MultiAgentTestPlan

/*
 *		
 * Transition:
 *   - Name: , ConditionString: , Comment :  
 *
 * Plans in State: 				
 *   - Plan - (Name): AttackDefault, (PlanID): 1402488866727 
 *
 * Tasks: 
 *   - AttackTask (1407153522080) (Entrypoint: 1413200877337)
 *   - DefaultTask (1225112227903) (Entrypoint: 1413200890537)
 *   - DefaultTask (1225112227903) (Entrypoint: 1413807260446)
 *
 * States:
 *   - OtherState (1413200877336)
 *   - State1 (1413200910490)
 *   - State2 (1413201030936)
 *   - NewSuccessState1 (1413201164999)
 *   - NewSuccessState2 (1413552736921)
 *   - Idle (1413807264574)
 *
 * Vars:
 */
bool TransitionCondition1413201052549::evaluate(shared_ptr<RunningPlan> rp)
{
  /*PROTECTED REGION ID(1413201050743) ENABLED START*/
	if(rp->getOwnID() == 8)
	{
		return alicaTests::TestWorldModel::getOne()->isTransitionCondition1413201052549();
	}
	else
	{
		return alicaTests::TestWorldModel::getTwo()->isTransitionCondition1413201052549();
	}
  /*PROTECTED REGION END*/

}

//State: State2 in Plan: MultiAgentTestPlan

/*
 *		
 * Transition:
 *   - Name: , ConditionString: , Comment :  
 *
 * Plans in State: 				
 *   - Plan - (Name): AttackDefault, (PlanID): 1402488866727 
 *
 * Tasks: 
 *   - AttackTask (1407153522080) (Entrypoint: 1413200877337)
 *   - DefaultTask (1225112227903) (Entrypoint: 1413200890537)
 *   - DefaultTask (1225112227903) (Entrypoint: 1413807260446)
 *
 * States:
 *   - OtherState (1413200877336)
 *   - State1 (1413200910490)
 *   - State2 (1413201030936)
 *   - NewSuccessState1 (1413201164999)
 *   - NewSuccessState2 (1413552736921)
 *   - Idle (1413807264574)
 *
 * Vars:
 */
bool TransitionCondition1413201367990::evaluate(shared_ptr<RunningPlan> rp)
{
  /*PROTECTED REGION ID(1413201367062) ENABLED START*/
	if(rp->getOwnID() == 8)
	{
		return alicaTests::TestWorldModel::getOne()->isTransitionCondition1413201367990();
	}
	else
	{
		return alicaTests::TestWorldModel::getTwo()->isTransitionCondition1413201367990();
	}
  /*PROTECTED REGION END*/

}

//State: NewSuccessState1 in Plan: MultiAgentTestPlan

//State: NewSuccessState2 in Plan: MultiAgentTestPlan

//State: Idle in Plan: MultiAgentTestPlan

}