/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2016, The OpenClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */
#include "C4Include.h"
#include "control/C4GameParameters.h"

#include "c4group/C4Components.h"
#include "object/C4Def.h"
#include "object/C4DefList.h"
#include "game/C4Application.h"
#include "network/C4Network2.h"

// *** C4GameRes


C4GameRes::C4GameRes()
		: pNetRes(nullptr)
{

}

C4GameRes::C4GameRes(const C4GameRes &Res)
		: eType(Res.getType()), File(Res.getFile()), pResCore(Res.getResCore()), pNetRes(Res.getNetRes())
{
	if (pResCore && !pNetRes)
		pResCore = new C4Network2ResCore(*pResCore);
}


C4GameRes::~C4GameRes()
{
	Clear();
}

C4GameRes &C4GameRes::operator = (const C4GameRes &Res)
{
	Clear();
	eType = Res.getType();
	File = Res.getFile();
	pResCore = Res.getResCore();
	pNetRes = Res.getNetRes();
	if (pResCore && !pNetRes)
		pResCore = new C4Network2ResCore(*pResCore);
	return *this;
}

void C4GameRes::Clear()
{
	eType = NRT_Null;
	File.Clear();
	if (pResCore && !pNetRes)
		delete pResCore;
	pResCore = nullptr;
	pNetRes = nullptr;
}

void C4GameRes::SetFile(C4Network2ResType enType, const char *sznFile)
{
	assert(!pNetRes && !pResCore);
	eType = enType;
	File = sznFile;
}

void C4GameRes::SetResCore(C4Network2ResCore *pnResCore)
{
	assert(!pNetRes);
	pResCore = pnResCore;
	eType = pResCore->getType();
}

void C4GameRes::SetNetRes(C4Network2Res::Ref pnNetRes)
{
	Clear();
	pNetRes = pnNetRes;
	eType = pNetRes->getType();
	File = pNetRes->getFile();
	pResCore = &pNetRes->getCore();
}

void C4GameRes::CompileFunc(StdCompiler *pComp)
{
	bool deserializing = pComp->isDeserializer();
	// Clear previous data for compiling
	if (deserializing) Clear();
	// Core is needed to decompile something meaningful
	if (!deserializing) assert(pResCore);
	// De-/Compile core
	pComp->Value(mkPtrAdaptNoNull(const_cast<C4Network2ResCore * &>(pResCore)));
	// Compile: Set type accordingly
	if (deserializing)
		eType = pResCore->getType();
}

bool C4GameRes::Publish(C4Network2ResList *pNetResList)
{
	assert(isPresent());
	// Already present?
	if (pNetRes) return true;
	// determine whether it's loadable
	bool fAllowUnloadable = false;
	if (eType == NRT_Definitions) fAllowUnloadable = true;
	// Add to network resource list
	C4Network2Res::Ref pNetRes = pNetResList->AddByFile(File.getData(), false, eType, -1, nullptr, fAllowUnloadable);
	if (!pNetRes) return false;
	// Set resource
	SetNetRes(pNetRes);
	return true;
}

bool C4GameRes::Load(C4Network2ResList *pNetResList)
{
	assert(pResCore);
	// Already present?
	if (pNetRes) return true;
	// Add to network resource list
	C4Network2Res::Ref pNetRes = pNetResList->AddByCore(*pResCore);
	if (!pNetRes) return false;
	// Set resource
	SetNetRes(pNetRes);
	return true;
}

bool C4GameRes::InitNetwork(C4Network2ResList *pNetResList)
{
	// Already initialized?
	if (getNetRes())
		return true;
	// Present? [Host]
	if (isPresent())
	{
		// Publish on network
		if (!Publish(pNetResList))
		{
			LogFatal(FormatString(LoadResStr("IDS_NET_NOFILEPUBLISH"), getFile()).getData());
			return false;
		}
	}
	// Got a core? [Client]
	else if (pResCore)
	{
		// Search/Load it
		if (!Load(pNetResList))
		{
			// Give some hints to why this might happen.
			const char *szFilename = pResCore->getFileName();
			if (!pResCore->isLoadable())
				if (pResCore->getType() == NRT_System)
					LogFatal(FormatString(LoadResStr("IDS_NET_NOSAMESYSTEM"), szFilename).getData());
				else
					LogFatal(FormatString(LoadResStr("IDS_NET_NOSAMEANDTOOLARGE"), szFilename).getData());
			// Should not happen
			else
				LogFatal(FormatString(LoadResStr("IDS_NET_NOVALIDCORE"), szFilename).getData());
			return false;
		}
	}
	// Okay
	return true;
}

void C4GameRes::CalcHash()
{
	if (!pNetRes) return;
	pNetRes->CalculateSHA();
}

// *** C4GameResList

C4GameResList &C4GameResList::operator = (const C4GameResList &List)
{
	Clear();
	// Copy the list
	iResCount = iResCapacity = List.iResCount;
	pResList = new C4GameRes *[iResCapacity];
	for (int i = 0; i < iResCount; i++)
		pResList[i] = new C4GameRes(*List.pResList[i]);
	return *this;
}

C4GameRes *C4GameResList::iterRes(C4GameRes *pLast, C4Network2ResType eType)
{
	for (int i = 0; i < iResCount; i++)
		if (!pLast)
		{
			if (eType == NRT_Null || pResList[i]->getType() == eType)
				return pResList[i];
		}
		else if (pLast == pResList[i])
			pLast = nullptr;
	return nullptr;
}

void C4GameResList::Clear()
{
	// clear them
	for (int32_t i = 0; i < iResCount; i++)
		delete pResList[i];
	delete [] pResList;
	pResList = nullptr;
	iResCount = iResCapacity = 0;
}

void C4GameResList::LoadFoldersWithLocalDefs(const char *szPath)
{
	// Scan path for folder names
	int32_t iBackslash;
	char szFoldername[_MAX_PATH+1];
	C4Group hGroup;
#ifdef _WIN32
	// Allow both backward and forward slashes when searching because the path
	// may be given with forward slashes. We would skip loading some definitions
	// if we didn't handle this properly and the user would have no clue what was
	// going on. See also http://forum.openclonk.org/topic_show.pl?tid=905.
	char control[3] = { DirectorySeparator, AltDirectorySeparator, '\0' };
	const int32_t len = (int32_t)strlen(szPath);
	for (int32_t iPrev=0; (iBackslash = strcspn(szPath+iPrev, control) + iPrev) < len; iPrev = iBackslash + 1)
#else
	for (int32_t cnt=0; (iBackslash=SCharPos(DirectorySeparator,szPath,cnt)) > -1; cnt++)
#endif
	{
		// Get folder name
		SCopy(szPath,szFoldername,iBackslash);
		// Open folder
		if (SEqualNoCase(GetExtension(szFoldername),"ocf"))
		{
			if (Reloc.Open(hGroup, szFoldername))
			{
				// Check for contained defs
				// do not, however, add them to the group set:
				//   parent folders are added by OpenScenario already!
				int32_t iContents;
				if ((iContents = Game.GroupSet.CheckGroupContents(hGroup, C4GSCnt_Definitions)))
				{
					// Add folder to list
					CreateByFile(NRT_Definitions, szFoldername);
				}
				// Close folder
				hGroup.Close();
			}
			else
			{
				LogF("Internal WARNING: Could not inspect folder %s for definitions.", szFoldername);
			}
		}
	}
}

bool C4GameResList::Load(C4Group &hGroup, C4Scenario *pScenario, const char * szDefinitionFilenames)
{
	// clear any prev
	Clear();
	// no defs to be added? that's OK (LocalOnly)
	if (szDefinitionFilenames && *szDefinitionFilenames)
	{
		// add them
		char szSegment[_MAX_PATH+1];
		for (int32_t cseg=0; SCopySegment(szDefinitionFilenames,cseg,szSegment,';',_MAX_PATH); ++cseg)
			if (*szSegment)
				CreateByFile(NRT_Definitions, szSegment);
	}

	LoadFoldersWithLocalDefs(pScenario->Head.Origin ? pScenario->Head.Origin.getData() : hGroup.GetFullName().getData());

	// add System.ocg
	CreateByFile(NRT_System, C4CFN_System);
	// add all instances of Material.ocg, except those inside the scenario file
	C4Group *pMatParentGrp = nullptr;
	while ((pMatParentGrp = Game.GroupSet.FindGroup(C4GSCnt_Material, pMatParentGrp)))
		if (pMatParentGrp != &Game.ScenarioFile)
		{
			StdStrBuf MaterialPath = pMatParentGrp->GetFullName() + DirSep C4CFN_Material;
			CreateByFile(NRT_Material, (pMatParentGrp->GetFullName() + DirSep C4CFN_Material).getData());
		}
	// add global Material.ocg
	CreateByFile(NRT_Material, C4CFN_Material);
	// done; success
	return true;
}

C4GameRes *C4GameResList::CreateByFile(C4Network2ResType eType, const char *szFile)
{
	// Create & set
	C4GameRes *pRes = new C4GameRes();
	pRes->SetFile(eType, szFile);
	// Add to list
	Add(pRes);
	return pRes;
}

C4GameRes *C4GameResList::CreateByNetRes(C4Network2Res::Ref pNetRes)
{
	// Create & set
	C4GameRes *pRes = new C4GameRes();
	pRes->SetNetRes(pNetRes);
	// Add to list
	Add(pRes);
	return pRes;
}

bool C4GameResList::InitNetwork(C4Network2ResList *pNetResList)
{
	// Check all resources without attached network resource object
	for (int i = 0; i < iResCount; i++)
		if (!pResList[i]->InitNetwork(pNetResList))
			return false;
	// Success
	return true;
}

void C4GameResList::CalcHashes()
{
	for (int32_t i = 0; i < iResCount; i++)
		pResList[i]->CalcHash();
}

bool C4GameResList::RetrieveFiles()
{
	// wait for all resources
	for (int32_t i = 0; i < iResCount; i++)
	{
		const C4Network2ResCore &Core = *pResList[i]->getResCore();
		StdStrBuf ResNameBuf = FormatString("%s: %s", LoadResStr("IDS_DLG_DEFINITION"), GetFilename(Core.getFileName()));
		if (!::Network.RetrieveRes(Core, C4NetResRetrieveTimeout, ResNameBuf.getData()))
			return false;
	}
	return true;
}

void C4GameResList::Add(C4GameRes *pRes)
{
	// Enlarge
	if (iResCount >= iResCapacity)
	{
		iResCapacity += 10;
		C4GameRes **pnResList = new C4GameRes *[iResCapacity];
		for (int i = 0; i < iResCount; i++)
			pnResList[i] = pResList[i];
		pResList = pnResList;
	}
	// Add
	pResList[iResCount++] = pRes;
}

void C4GameResList::CompileFunc(StdCompiler *pComp)
{
	bool deserializing = pComp->isDeserializer();
	// Clear previous data
	if (deserializing) Clear();
	// Compile resource count
	pComp->Value(mkNamingCountAdapt(iResCount, "Resource"));
	// Create list
	if (deserializing)
	{
		pResList = new C4GameRes *[iResCapacity = iResCount];
		ZeroMem(pResList, sizeof(*pResList) * iResCount);
	}
	// Compile list
	pComp->Value(
	  mkNamingAdapt(
	    mkArrayAdaptMap(pResList, iResCount, mkPtrAdaptNoNull<C4GameRes>),
	    "Resource"));
	mkPtrAdaptNoNull<C4GameRes>(*pResList);
}



// *** C4GameParameters

C4GameParameters::C4GameParameters() = default;

C4GameParameters::~C4GameParameters() = default;

void C4GameParameters::Clear()
{
	League.Clear();
	LeagueAddress.Clear();
	Rules.Clear();
	Goals.Clear();
	Scenario.Clear();
	GameRes.Clear();
	Clients.Clear();
	PlayerInfos.Clear();
	RestorePlayerInfos.Clear();
	Teams.Clear();
	ScenarioParameters.Clear();
}

bool C4GameParameters::Load(C4Group &hGroup, C4Scenario *pScenario, const char *szGameText, C4LangStringTable *pLang, const char *DefinitionFilenames, C4ScenarioParameters *pStartupScenarioParameters)
{
	// Clear previous data
	Clear();

	// Scenario
	Scenario.SetFile(NRT_Scenario, hGroup.GetFullName().getData());

	// Additional game resources
	if (!GameRes.Load(hGroup, pScenario, DefinitionFilenames))
		return false;

	// Player infos (replays only)
	if (pScenario->Head.Replay)
		if (hGroup.FindEntry(C4CFN_PlayerInfos))
			PlayerInfos.Load(hGroup, C4CFN_PlayerInfos);

	// Savegame restore infos: Used for savegames to rejoin joined players
	if (hGroup.FindEntry(C4CFN_SavePlayerInfos))
	{
		// load to savegame info list
		RestorePlayerInfos.Load(hGroup, C4CFN_SavePlayerInfos,  pLang);
		// transfer counter to allow for additional player joins in savegame resumes
		PlayerInfos.SetIDCounter(RestorePlayerInfos.GetIDCounter());
		// in network mode, savegame players may be reassigned in the lobby
		// in any mode, the final player restoration will be done in InitPlayers()
		// dropping any players that could not be restored
	}

	// Load teams
	if (!Teams.Load(hGroup, pScenario, pLang))
		{ LogFatal(LoadResStr("IDS_PRC_ERRORLOADINGTEAMS")); return false; }

	// Compile data
	StdStrBuf Buf;
	if (hGroup.LoadEntryString(C4CFN_Parameters, &Buf))
	{
		if (!CompileFromBuf_LogWarn<StdCompilerINIRead>(
		      mkNamingAdapt(mkParAdapt(*this, pScenario), "Parameters"),
		      Buf,
		      C4CFN_Parameters))
			return false;
	}
	else
	{

		// Set default values
		StdCompilerNull DefaultCompiler;
		DefaultCompiler.Compile(mkParAdapt(*this, pScenario));

		// Set control rate default
		if (ControlRate < 0)
			ControlRate = Config.Network.ControlRate;

		// network game?
		IsNetworkGame = Game.NetworkActive;

		// Auto frame skip by options
		AutoFrameSkip = !!::Config.Graphics.AutoFrameSkip;

		// custom parameters from startup
		if (pStartupScenarioParameters) ScenarioParameters = *pStartupScenarioParameters;
	}


	// enforce league settings
	if (isLeague()) EnforceLeagueRules(pScenario);

	// Done
	return true;
}

void C4GameParameters::EnforceLeagueRules(C4Scenario *pScenario)
{
	Scenario.CalcHash();
	GameRes.CalcHashes();
	Teams.EnforceLeagueRules();
	AllowDebug = false;
	if (pScenario) MaxPlayers = pScenario->Head.MaxPlayerLeague;
	// forced league values in custom scenario parameters
	size_t idx=0; const C4ScenarioParameterDef *pdef; int32_t val;
	while ((pdef = ::Game.ScenarioParameterDefs.GetParameterDefByIndex(idx++)))
		if ((val = pdef->GetLeagueValue()))
			ScenarioParameters.SetValue(pdef->GetID(), val, false);
}

bool C4GameParameters::Save(C4Group &hGroup, C4Scenario *pScenario)
{

	// Write Parameters.txt
	StdStrBuf ParData = DecompileToBuf<StdCompilerINIWrite>(
	                      mkNamingAdapt(mkParAdapt(*this, pScenario), "Parameters"));
	if (!hGroup.Add(C4CFN_Parameters, ParData, false, true))
		return false;

	// Done
	return true;
}

bool C4GameParameters::InitNetwork(C4Network2ResList *pResList)
{

	// Scenario & material resource
	if (!Scenario.InitNetwork(pResList))
		return false;

	// Other game resources
	if (!GameRes.InitNetwork(pResList))
		return false;

	// Done
	return true;
}

void C4GameParameters::CompileFunc(StdCompiler *pComp, C4Scenario *pScenario)
{
	pComp->Value(mkNamingAdapt(MaxPlayers,        "MaxPlayers",       !pScenario ? 0 : pScenario->Head.MaxPlayer));
	pComp->Value(mkNamingAdapt(AllowDebug,        "AllowDebug",       true));
	pComp->Value(mkNamingAdapt(IsNetworkGame,     "IsNetworkGame",    false));
	pComp->Value(mkNamingAdapt(IsEditor,          "IsEditor",         !!::Application.isEditor));
	pComp->Value(mkNamingAdapt(ControlRate,       "ControlRate",      -1));
	pComp->Value(mkNamingAdapt(AutoFrameSkip,     "AutoFrameSkip",    false));
	pComp->Value(mkNamingAdapt(Rules,             "Rules",            !pScenario ? C4IDList() : pScenario->Game.Rules));
	pComp->Value(mkNamingAdapt(Goals,             "Goals",            !pScenario ? C4IDList() : pScenario->Game.Goals));
	pComp->Value(mkNamingAdapt(League,            "League",           StdStrBuf()));

	// These values are either stored separately (see Load/Save) or
	// don't make sense for savegames.
	if (!pScenario)
	{
		pComp->Value(mkNamingAdapt(LeagueAddress,   "LeagueAddress",    ""));

		pComp->Value(mkNamingAdapt(Scenario,        "Scenario"          ));
		pComp->Value(GameRes);

		pComp->Value(mkNamingAdapt(PlayerInfos,     "PlayerInfos"       ));
		pComp->Value(mkNamingAdapt(RestorePlayerInfos,"RestorePlayerInfos"));
		pComp->Value(mkNamingAdapt(Teams,           "Teams"             ));
	}

	pComp->Value(Clients);

	pComp->Value(mkNamingAdapt(ScenarioParameters, "ScenarioParameters"));

}

StdStrBuf C4GameParameters::GetGameGoalString() const
{
	// getting game goals from the ID list
	// unfortunately, names cannot be deduced before object definitions are loaded
	StdStrBuf sResult;
	C4ID idGoal;
	for (int32_t i=0; i<Goals.GetNumberOfIDs(); ++i)
		if ((idGoal = Goals.GetID(i))) if (idGoal != C4ID::None)
			{
				if (Game.IsRunning)
				{
					C4Def *pDef = C4Id2Def(idGoal);
					if (pDef)
					{
						if (sResult.getLength()) sResult.Append(", ");
						sResult.Append(pDef->GetName());
					}
				}
				else
				{
					if (sResult.getLength()) sResult.Append(", ");
					sResult.Append(idGoal.ToString());
				}
			}
	// Max length safety
	if (sResult.getLength() > C4MaxTitle) sResult.SetLength(C4MaxTitle);
	// Compose desc string
	if (sResult.getLength())
		return FormatString("%s: %s", LoadResStr("IDS_MENU_CPGOALS"), sResult.getData());
	else
		return StdCopyStrBuf(LoadResStr("IDS_CTL_NOGOAL"), true);
}
