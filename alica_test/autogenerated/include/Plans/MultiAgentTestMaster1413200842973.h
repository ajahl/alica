#ifndef MultiAgentTestMaster_H_
#define MultiAgentTestMaster_H_

#include "DomainCondition.h"
#include "engine/BasicUtilityFunction.h"
#include "engine/UtilityFunction.h"
#include "engine/DefaultUtilityFunction.h"
/*PROTECTED REGION ID(incl1413200842973) ENABLED START*/
//Add inlcudes here
/*PROTECTED REGION END*/
using namespace alica;

namespace alicaAutogenerated
{
/*PROTECTED REGION ID(meth1413200842973) ENABLED START*/
//Add other things here
/*PROTECTED REGION END*/
class UtilityFunction1413200842973 : public BasicUtilityFunction
{
  shared_ptr<UtilityFunction> getUtilityFunction(Plan* plan);
};

class TransitionCondition1413201227586 : public DomainCondition
{
  bool evaluate(shared_ptr<RunningPlan> rp);
};

class TransitionCondition1413201389955 : public DomainCondition
{
  bool evaluate(shared_ptr<RunningPlan> rp);
};

} /* namespace alica */

#endif
