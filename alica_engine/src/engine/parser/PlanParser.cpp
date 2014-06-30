/*
 * PlanParser.cpp
 *
 *  Created on: Mar 27, 2014
 *      Author: Stephan Opfer
 */
//#define PP_DEBUG

#include "engine/parser/PlanParser.h"
#include "engine/parser/ModelFactory.h"
#include "engine/AlicaEngine.h"

namespace alica
{

	PlanParser::PlanParser(PlanRepository* rep)
	{
		using namespace supplementary;

		this->masterPlan = nullptr;
		this->rep = rep;
		this->mf = shared_ptr<ModelFactory>(new ModelFactory(this, rep));
		this->sc = SystemConfig::getInstance();
		this->domainConfigFolder = this->sc->getConfigPath();

		this->planDir = (*this->sc)["Alica"]->get<string>("Alica.PlanDir", NULL);
		this->roleDir = (*this->sc)["Alica"]->get<string>("Alica.RoleDir", NULL);

		if (domainConfigFolder.find_last_of(FileSystem::PATH_SEPARATOR) != domainConfigFolder.length() - 1)
		{
			domainConfigFolder = domainConfigFolder + FileSystem::PATH_SEPARATOR;
		}
		if (planDir.find_last_of(FileSystem::PATH_SEPARATOR) != planDir.length() - 1)
		{
			planDir = planDir + FileSystem::PATH_SEPARATOR;
		}
		if (roleDir.find_last_of(FileSystem::PATH_SEPARATOR) != roleDir.length() - 1)
		{
			roleDir = roleDir + FileSystem::PATH_SEPARATOR;
		}
		if (!(supplementary::FileSystem::isPathRooted(this->planDir)))
		{
			basePlanPath = domainConfigFolder + planDir;
		}
		else
		{
			basePlanPath = planDir;
		}
		if (!(supplementary::FileSystem::isPathRooted(this->roleDir)))
		{
			baseRolePath = domainConfigFolder + roleDir;
		}
		else
		{
			baseRolePath = roleDir;
		}
#ifdef PP_DEBUG
		cout << "PP: basePlanPath: " << basePlanPath << endl;
		cout << "PP: baseRolePath: " << baseRolePath << endl;
#endif
		if (!(supplementary::FileSystem::fileExists(basePlanPath)))
		{
			AlicaEngine::getInstance()->abort("PP: BasePlanPath does not exists " + basePlanPath);
		}
		if (!(supplementary::FileSystem::fileExists(baseRolePath)))
		{
			AlicaEngine::getInstance()->abort("PP: BaseRolePath does not exists " + baseRolePath);
		}
	}

	PlanParser::~PlanParser()
	{
	}

	RoleSet* PlanParser::parseRoleSet(string roleSetName, string roleSetDir)
	{
		using namespace supplementary;

		if (roleSetName.empty())
		{
			roleSetName = findDefaultRoleSet(roleSetDir);
		}
		else
		{
			if (roleSetDir.find_last_of(FileSystem::PATH_SEPARATOR) != roleSetDir.length() - 1 && roleSetDir.length() > 0)
			{
				roleSetDir = roleSetDir + FileSystem::PATH_SEPARATOR;
			}
			if (!FileSystem::isPathRooted(roleSetDir))
			{
				roleSetName = FileSystem::combinePaths(FileSystem::combinePaths(baseRolePath, roleSetDir), roleSetName);
			}
			else
			{
				roleSetName = FileSystem::combinePaths(roleSetDir, roleSetName);
			}
		}

		if (!supplementary::FileSystem::endsWith(roleSetName, ".rset"))
		{
			roleSetName = roleSetName + ".rset";
		}
		if (!supplementary::FileSystem::fileExists(roleSetName))
		{
			AlicaEngine::getInstance()->abort("PP: Cannot find roleset: " + roleSetName);
		}

#ifdef PP_DEBUG
		cout << "PP: Parsing RoleSet " << roleSetName << endl;
#endif

		this->currentDirectory = supplementary::FileSystem::getParent(roleSetName);

		tinyxml2::XMLDocument doc;
		doc.LoadFile(roleSetName.c_str());
		if (doc.ErrorID() != tinyxml2::XML_NO_ERROR)
		{
			cout << "PP: doc.ErrorCode: " << tinyxml2::XMLErrorStr[doc.ErrorID()] << endl;
			throw new exception();
		}

		RoleSet* r = this->mf->createRoleSet(&doc, this->masterPlan);

		filesParsed.push_back(roleSetName);

		while (this->filesToParse.size() > 0)
		{
			string fileToParse = this->filesToParse.front();
			this->filesToParse.pop_front();
			this->currentDirectory = supplementary::FileSystem::getParent(fileToParse);
			this->currentFile = fileToParse;

			if (!supplementary::FileSystem::fileExists(fileToParse))
			{
				AlicaEngine::getInstance()->abort("PP: Cannot Find referenced file ", fileToParse);
			}
			if (supplementary::FileSystem::endsWith(fileToParse, ".rdefset"))
			{
				parseRoleDefFile(fileToParse);
			}
			else if (supplementary::FileSystem::endsWith(fileToParse, ".cdefset"))
			{
				parseCapabilityDefFile(fileToParse);
			}
			else
			{
				AlicaEngine::getInstance()->abort("PP: Cannot Parse file " + fileToParse);
			}
			filesParsed.push_back(fileToParse);

		}

		this->mf->attachRoleReferences();
		this->mf->attachCharacteristicReferences();
		return r;

	}
	void PlanParser::parseRoleDefFile(string currentFile)
	{
#ifdef PP_DEBUG
		cout << "PP: parsing RoleDef file: " << currentFile << endl;
#endif
		tinyxml2::XMLDocument doc;
		doc.LoadFile(currentFile.c_str());
		if (doc.ErrorID() != tinyxml2::XML_NO_ERROR)
		{
			cout << "PP: doc.ErrorCode: " << tinyxml2::XMLErrorStr[doc.ErrorID()] << endl;
			throw new exception();
		}
		this->mf->createRoleDefinitionSet(&doc);
	}
	void PlanParser::parseCapabilityDefFile(string currentFile)
	{
#ifdef PP_DEBUG
		cout << "PP: parsing RoleDef file: " << currentFile << endl;
#endif
		tinyxml2::XMLDocument doc;
		doc.LoadFile(currentFile.c_str());
		if (doc.ErrorID() != tinyxml2::XML_NO_ERROR)
		{
			cout << "PP: doc.ErrorCode: " << tinyxml2::XMLErrorStr[doc.ErrorID()] << endl;
			throw new exception();
		}
		this->mf->createCapabilityDefinitionSet(&doc);

	}
	string PlanParser::findDefaultRoleSet(string dir)
	{

		if (!supplementary::FileSystem::isPathRooted(dir))
		{
			dir = supplementary::FileSystem::combinePaths(this->baseRolePath, dir);
		}
		if (!supplementary::FileSystem::isDirectory(dir))
		{
			AlicaEngine::getInstance()->abort("PP: RoleSet directory does not exist: " + dir);
		}

		vector<string> files = supplementary::FileSystem::findAllFiles(dir, ".rset");

		for (string s : files)
		{
			tinyxml2::XMLDocument doc;
			doc.LoadFile(s.c_str());
			if (doc.ErrorID() != tinyxml2::XML_NO_ERROR)
			{
				cout << "PP: doc.ErrorCode: " << tinyxml2::XMLErrorStr[doc.ErrorID()] << endl;
				throw new exception();
			}

			tinyxml2::XMLElement* element = doc.FirstChildElement();
			const char* attr = element->Attribute("default");
			if (attr)
			{
				string attrString = attr;
				if (attrString.compare("true") == 0)
				{
					return s;
				}
			}
		}
		if (files.size() == 1)
		{
			return files[0];
		}
		AlicaEngine::getInstance()->abort("PP: Cannot find a default roleset in directory: " + dir);
		return "";
	}

	Plan* PlanParser::parsePlanTree(string masterplan)
	{
		string masterPlanPath;
		bool found = supplementary::FileSystem::findFile(this->basePlanPath, masterplan + ".pml", masterPlanPath);
#ifdef PP_DEBUG
		cout << "PP: masterPlanPath: " << masterPlanPath << endl;
#endif
		if (!found)
		{
			AlicaEngine::getInstance()->abort("PP: Cannot find MasterPlan '" + masterplan);
		}
		this->currentFile = masterPlanPath;
		this->currentDirectory = supplementary::FileSystem::getParent(masterPlanPath);
#ifdef PP_DEBUG
		cout << "PP: CurFile: " << this->currentFile << " CurDir: " << this->currentDirectory << endl;
#endif

		this->masterPlan = parsePlanFile(masterPlanPath);
		parseFileLoop();

		this->mf->computeReachabilities();
		return this->masterPlan;
	}
	void PlanParser::parseFileLoop()
	{
		while (this->filesToParse.size() > 0)
		{
			string fileToParse = this->filesToParse.front();
			this->filesToParse.pop_front();
			this->currentDirectory = supplementary::FileSystem::getParent(fileToParse);
			this->currentFile = fileToParse;

			if (!supplementary::FileSystem::fileExists(fileToParse))
			{
				AlicaEngine::getInstance()->abort("PP: Cannot Find referenced file ", fileToParse);
			}
			if (supplementary::FileSystem::endsWith(fileToParse, ".pml"))
			{
				parsePlanFile(fileToParse);
			}
			else if (supplementary::FileSystem::endsWith(fileToParse, ".tsk"))
			{
				parseTaskFile(fileToParse);
			}
			else if (supplementary::FileSystem::endsWith(fileToParse, ".beh"))
			{
				parseBehaviourFile(fileToParse);
			}
			else if (supplementary::FileSystem::endsWith(fileToParse, ".pty"))
			{
				parsePlanTypeFile(fileToParse);
			}
			else
			{
				AlicaEngine::getInstance()->abort("PP: Cannot Parse file", fileToParse);
			}
			filesParsed.push_back(fileToParse);
		}
		this->mf->attachPlanReferences();

	}
	void PlanParser::parsePlanTypeFile(string currentFile)
	{
#ifdef PP_DEBUG
		cout << "PP: parsing PlanType file: " << currentFile << endl;
#endif
		tinyxml2::XMLDocument doc;
		doc.LoadFile(currentFile.c_str());
		if (doc.ErrorID() != tinyxml2::XML_NO_ERROR)
		{
			cout << "PP: doc.ErrorCode: " << tinyxml2::XMLErrorStr[doc.ErrorID()] << endl;
			throw new exception();
		}
		this->mf->createPlanType(&doc);
	}
	void PlanParser::parseBehaviourFile(string currentFile)
	{
#ifdef PP_DEBUG
		cout << "PP: parsing Behaviour file: " << currentFile << endl;
#endif
		tinyxml2::XMLDocument doc;
		doc.LoadFile(currentFile.c_str());
		if (doc.ErrorID() != tinyxml2::XML_NO_ERROR)
		{
			cout << "PP: doc.ErrorCode: " << tinyxml2::XMLErrorStr[doc.ErrorID()] << endl;
			throw new exception();
		}
		this->mf->createBehaviour(&doc);

	}

	void PlanParser::parseTaskFile(string currentFile)
	{
#ifdef PP_DEBUG
		cout << "PP: parsing Task file: " << currentFile << endl;
#endif
		tinyxml2::XMLDocument doc;
		doc.LoadFile(currentFile.c_str());
#ifdef PP_DEBUG
		cout << "TASKREPO " << currentFile << endl;
#endif
		if (doc.ErrorID() != tinyxml2::XML_NO_ERROR)
		{
			cout << "PP: doc.ErrorCode: " << tinyxml2::XMLErrorStr[doc.ErrorID()] << endl;
			throw new exception();
		}
		this->mf->createTasks(&doc);

	}

	Plan* PlanParser::parsePlanFile(string& planFile)
	{
#ifdef PP_DEBUG
		cout << "PP: parsing Plan file: " << planFile << endl;
#endif

		tinyxml2::XMLDocument doc;
		doc.LoadFile(planFile.c_str());
		if (doc.ErrorID() != tinyxml2::XML_NO_ERROR)
		{
			cout << "PP: doc.ErrorCode: " << tinyxml2::XMLErrorStr[doc.ErrorID()] << endl;
			throw new exception();
		}
		Plan* p = this->mf->createPlan(&doc);
		return p;
	}

	shared_ptr<map<long, alica::AlicaElement> > PlanParser::getParsedElements()
	{

		shared_ptr<map<long, alica::AlicaElement> > map;
		return map;
	}

	void PlanParser::ignoreMasterPlanId(bool val)
	{
		this->mf->setIgnoreMasterPlanId(val);
	}

	string PlanParser::getCurrentFile()
	{
		return this->currentFile;
	}

	void PlanParser::setCurrentFile(string currentFile)
	{
		if (currentFile.compare(0, basePlanPath.size(), basePlanPath))
		{
			this->currentFile = currentFile.substr(basePlanPath.size());
		}
		else if (currentFile.compare(0, baseRolePath.size(), baseRolePath))
		{
			this->currentFile = currentFile.substr(baseRolePath.size());
		}
		else
		{
			this->currentFile = currentFile;
		}
	}

	/**
	 * parse id of the plan
	 * @param node the given xml node
	 * @return id: returns the id from the plan
	 */
	long PlanParser::parserId(tinyxml2::XMLElement* node)
	{
		long id = -1;
		string idString = "";
		const char* idChar = node->Attribute("id");
		if (idChar)
			idString = idChar;
		if (idString.compare("") != 0)
		{
			try
			{
				id = stol(idString);
			}
			catch (exception &e)
			{
				AlicaEngine::getInstance()->abort("PP: Cannot convert ID to long: " + idString + " WHAT?? " + e.what());
			}
			return id;
		}
		else
		{
			string idString = "";
			const char* idChar = node->Attribute("href");
			if (idChar)
				idString = idChar;
			if (idString.compare("") != 0)
			{
				id = fetchId(idString, id);
				return id;
			}
			else
			{
				const tinyxml2::XMLNode* currNode = node->FirstChild();
				while (currNode)
				{
					const tinyxml2::XMLText* textNode = currNode->ToText();
					if (textNode)
					{
						id = fetchId(textNode->Value(), id);
						return id;
					}

					currNode = currNode->NextSibling();
				}
			}
		}

		cerr << "Cannot resolve remote reference!\nAttributes of node in question are:" << endl;
		const tinyxml2::XMLAttribute* curAttribute = node->FirstAttribute();
		while (curAttribute)
		{
			cout << curAttribute->Name() << " : " << curAttribute->Value() << endl;
			curAttribute = curAttribute->Next();
		}
		AlicaEngine::getInstance()->abort("PP: Couldn't resolve remote reference: " + string(node->Name()));
		return -1;
	}

	/**
	 * Helper
	 * @param idString is a String that have to be converted in a long
	 * @param id the id
	 */
	long PlanParser::fetchId(const string& idString, long id)
	{
		int hashPos = idString.find_first_of("#");
		string locator = idString.substr(0, hashPos);
		if (!locator.empty())
		{
			if (!supplementary::FileSystem::endsWith(this->currentDirectory, "/"))
			{
				this->currentDirectory = this->currentDirectory + "/";
			}
			string path = this->currentDirectory + locator;
			list<string>::iterator findIterParsed = find(filesParsed.begin(), filesParsed.end(), path);
			list<string>::iterator findIterToParse = find(filesToParse.begin(), filesToParse.end(), path);
			if (findIterParsed == filesParsed.end() && findIterToParse == filesToParse.end())
			{
#ifdef PP_DEBUG
				cout << "PP: Adding " + path + " to parse queue " << endl;
#endif
				filesToParse.push_back(path);
			}
		}
		string tokenId = idString.substr(hashPos + 1, idString.length() - hashPos);
		try
		{
			id = stol(tokenId);
		}
		catch (exception& e)
		{
			AlicaEngine::getInstance()->abort("PP: Cannot convert ID to long: " + tokenId + " WHAT?? " + e.what());
		}
		return id;
	}
}
/* namespace Alica */