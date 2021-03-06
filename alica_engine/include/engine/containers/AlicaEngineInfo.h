/*
 * AlicaEngineInfo.h
 *
 *  Created on: Jun 30, 2014
 *      Author: Paul Panin
 */

#ifndef ALICAENGINEINFO_H_
#define ALICAENGINEINFO_H_

#include <vector>
#include <string>
#include <tuple>

using namespace std;

namespace alica
{
	typedef tuple<int, string, string, string, string, string, vector<int>> stdAlicaEngineInfo;
	struct AlicaEngineInfo
	{
		AlicaEngineInfo()
		{
		}
		int senderID;
		string masterPlan;
		string currentPlan;
		string currentState;
		string currentRole;
		string currentTask;
		vector<int> robotIDsWithMe;

		AlicaEngineInfo(stdAlicaEngineInfo &s)
		{
			senderID = get<0>(s);
			masterPlan = move(get<1>(s));
			currentPlan = move(get<2>(s));
			currentState = move(get<3>(s));
			currentRole = move(get<4>(s));
			currentTask = move(get<5>(s));
			robotIDsWithMe = move(get<6>(s));
		}

		stdAlicaEngineInfo toStandard()
		{
			return move(
					make_tuple(senderID, masterPlan, currentPlan, currentState, currentRole, currentTask,
								robotIDsWithMe));
		}
	};
}

#endif /* ALICAENGINEINFO_H_ */
