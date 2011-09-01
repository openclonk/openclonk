/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2004, 2006-2007, 2009  Sven Eberhardt
 * Copyright (c) 2005-2007  Peter Wortmann
 * Copyright (c) 2006  Florian Groß
 * Copyright (c) 2006, 2009, 2011  Günther Brammer
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de
 *
 * Portions might be copyrighted by other authors who have contributed
 * to OpenClonk.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * See isc_license.txt for full license and disclaimer.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 * See clonk_trademark_license.txt for full license.
 */
#include "C4Include.h"
#include "C4GameParameters.h"

#include "C4Log.h"
#include "C4Components.h"
#include "C4Def.h"
#include <C4DefList.h>
#include <C4Game.h>
#include <C4GameObjects.h>
#include <C4Network2.h>

// *** C4GameRes


C4GameRes::C4GameRes()
		: eType(NRT_Null), pResCore(NULL), pNetRes(NULL)
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
	pResCore = NULL;
	pNetRes = NULL;
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
	bool fCompiler = pComp->isCompiler();
	// Clear previous data for compiling
	if (fCompiler) Clear();
	// Core is needed to decompile something meaningful
	if (!fCompiler) assert(pResCore);
	// De-/Compile core
	pComp->Value(mkPtrAdaptNoNull(const_cast<C4Network2ResCore * &>(pResCore)));
	// Compile: Set type accordingly
	if (fCompiler)
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
	C4Network2Res::Ref pNetRes = pNetResList->AddByFile(File.getData(), false, eType, -1, NULL, fAllowUnloadable);
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
			pLast = NULL;
	return NULL;
}

void C4GameResList::Clear()
{
	// clear them
	for (int32_t i = 0; i < iResCount; i++)
		delete pResList[i];
	delete [] pResList;
	pResList = NULL;
	iResCount = iResCapacity = 0;
}

void C4GameResList::LoadFoldersWithLocalDefs(const char *szPath)
{
	// Scan path for folder names
	int32_t cnt,iBackslash;
	char szFoldername[_MAX_PATH+1];
	C4Group hGroup;
	for (cnt=0; (iBackslash=SCharPos(DirectorySeparator,szPath,cnt)) > -1; cnt++)
	{
		// Get folder name
		SCopy(szPath,szFoldername,iBackslash);
		// Open folder
		if (SEqualNoCase(GetExtension(szFoldername),"ocf"))
			if (hGroup.Open(szFoldername))
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
	C4Group *pMatParentGrp = NULL;
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
	bool fCompiler = pComp->isCompiler();
	// Clear previous data
	if (fCompiler) Clear();
	// Compile resource count
	pComp->Value(mkNamingCountAdapt(iResCount, "Resource"));
	// Create list
	if (fCompiler)
	{
		pResList = new C4GameRes *[iResCapacity = iResCount];
		ZeroMem(pResList, sizeof(*pResList) * iResCount);
	}
	// Compile list
	pComp->Value(
	  mkNamingAdapt(
	    mkArrayAdaptMap(pResList, iResCount, /*(C4GameRes *)NULL, */ mkPtrAdaptNoNull<C4GameRes>),
	    "Resource"));
	mkPtrAdaptNoNull<C4GameRes>(*pResList);
}

// *** C4GameParameters

C4GameParameters::C4GameParameters()
{

}

C4GameParameters::~C4GameParameters()
{

}

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
}

bool C4GameParameters::Load(C4Group &hGroup, C4Scenario *pScenario, const char *szGameText, C4LangStringTable *pLang, const char *DefinitionFilenames)
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
	pComp->Value(mkNamingAdapt(ControlRate,       "ControlRate",      -1));
	pComp->Value(mkNamingAdapt(Rules,             "Rules",            !pScenario ? C4IDList() : pScenario->Game.Rules));
	pComp->Value(mkNamingAdapt(Goals,             "Goals",            !pScenario ? C4IDList() : pScenario->Game.Goals));
	pComp->Value(mkNamingAdapt(League,          "League",             StdStrBuf()));

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

}

StdStrBuf C4GameParameters::GetGameGoalString()
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
