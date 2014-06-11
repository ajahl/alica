/*
 * State.h
 *
 *  Created on: Mar 5, 2014
 *      Author: Stephan Opfer
 */

#ifndef STATE_H_
#define STATE_H_

using namespace std;

#include <stdio.h>
#include <list>

#include "AlicaElement.h"

namespace alica
{
	class Plan;
	class Transition;
	class AbstractPlan;
	class Parametrisation;

	class State : public AlicaElement
	{
	public:
		State();
		State(long id);
		virtual ~State();
		virtual string toString() const;
		bool isFailureState() const;
		void setFailureState(bool failureState);
		const Plan* getInPlan() const;
		void setInPlan( Plan* inPlan);
		const list<Transition*>& getInTransitions() const;
		void setInTransitions(const list<Transition*>& inTransitions);
		const list<Transition*>& getOutTransitions() const;
		void setOutTransitions(const list<Transition*>& outTransition);
		 list<Parametrisation*>& getParametrisation();
		void setParametrisation(const list<Parametrisation*>& parametrisation);
		const list<AbstractPlan*>& getPlans() const;
		void setPlans(const list<AbstractPlan*>& plans);bool isSuccessState() const;
		void setSuccessState(bool successState);bool isTerminal() const;
		void setTerminal(bool terminal);

	protected:
		list<AbstractPlan*> plans;
		list<Transition*> inTransitions;
		list<Transition*> outTransitions;
		list<Parametrisation*> parametrisation;
		Plan* inPlan;
		bool terminal;
		bool failureState;
		bool successState;

	};

} /* namespace Alica */

#endif /* STATE_H_ */
