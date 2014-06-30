/*
 * IBehaviourCreator.h
 *
 *  Created on: Jun 18, 2014
 *      Author: emmeda
 */

#ifndef IBEHAVIOURCREATOR_H_
#define IBEHAVIOURCREATOR_H_

using namespace std;

#include <string>
#include <memory>

namespace alica
{

	class BasicBehaviour;

	class IBehaviourCreator
	{
	public:
		virtual ~IBehaviourCreator() {}
		virtual unique_ptr<BasicBehaviour> createBehaviour(string behaviourName) = 0;
	};

} /* namespace alica */

#endif /* IBEHAVIOURCREATOR_H_ */