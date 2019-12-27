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
// game saving functionality

#include "C4Include.h"
#include "control/C4GameSave.h"

#include "C4Version.h"
#include "c4group/C4Components.h"
#include "control/C4GameParameters.h"
#include "control/C4Record.h"
#include "control/C4RoundResults.h"
#include "landscape/C4Landscape.h"
#include "landscape/C4MassMover.h"
#include "landscape/C4PXS.h"
#include "player/C4PlayerList.h"
#include "network/C4Network2.h"
#include "script/C4Value.h"

// *** C4GameSave main class

bool C4GameSave::SaveCreateGroup(const char *szFilename, C4Group &hUseGroup)
{
	// erase any previous item (2do: work in C4Groups?)
	EraseItem(szFilename);
	// copy from previous group?
	if (GetCopyScenario())
		if (!ItemIdentical(Game.ScenarioFilename, szFilename))
			if (!C4Group_CopyItem(Game.ScenarioFilename, szFilename))
				{ LogF(LoadResStr("IDS_CNS_SAVEASERROR"), szFilename); return false; }
	// open it
	if (!hUseGroup.Open(szFilename, !GetCopyScenario()))
	{
		EraseItem(szFilename);
		LogF(LoadResStr("IDS_CNS_SAVEASERROR"), szFilename);
		return false;
	}
	// done, success
	return true;
}

bool C4GameSave::SaveCore()
{
	// base on original, current core
	rC4S = Game.C4S;
	// Always mark current engine version
	rC4S.Head.C4XVer[0]=C4XVER1; rC4S.Head.C4XVer[1]=C4XVER2;
	// Some flags are not to be set for initial settings:
	//  They depend on whether specific runtime data is present, which may simply not be stored into initial
	//  saves, because they rely on any data present and up-to-date within the scenario!
	if (!fInitial)
	{
		// NoInitialize: Marks whether object data is contained and not to be created from core
		rC4S.Head.NoInitialize = true;
		// the SaveGame-value, despite it's name, marks whether exact runtime data is contained
		// the flag must not be altered for pure
		rC4S.Head.SaveGame = GetSaveRuntimeData() && IsExact();
	}
	// some values relevant for synced saves only
	if (IsExact())
	{
		rC4S.Head.RandomSeed=Game.RandomSeed;
	}
	// reset some network flags
	rC4S.Head.NetworkGame=false;
	// Title in language game was started in (not: save scenarios and net references)
	if (!GetKeepTitle())
	{
		rC4S.Head.Title = Game.ScenarioTitle.getData();
	}
	// some adjustments for everything but saved scenarios
	if (IsExact())
	{
		// Store used definitions
		rC4S.Definitions.SetModules(Game.DefinitionFilenames,Config.General.SystemDataPath, Config.General.UserDataPath);
		// Save game parameters
		if (!Game.Parameters.Save(*pSaveGroup, &Game.C4S)) return false;
	}
	// clear MissionAccess in save games and records (sulai)
	rC4S.Head.MissionAccess.clear();
	// store origin
	if (GetSaveOrigin())
	{
		// keep if assigned already (e.g., when doing a record of a savegame)
		if (!rC4S.Head.Origin.getLength())
		{
			rC4S.Head.Origin.Copy(Game.ScenarioFilename);
			Config.ForceRelativePath(&rC4S.Head.Origin);
		}
	}
	else if (GetClearOrigin())
		rC4S.Head.Origin.Clear();
	// adjust specific values (virtual call)
	AdjustCore(rC4S);
	// Save scenario core
	return !!rC4S.Save(*pSaveGroup);
}

bool C4GameSave::SaveScenarioSections()
{
	// any scenario sections?
	if (!Game.pScenarioSections) return true;
	// prepare section filename
	int iWildcardPos = SCharPos('*', C4CFN_ScenarioSections);
	char fn[_MAX_FNAME_LEN];
	// save all modified sections
	for (C4ScenarioSection *pSect = Game.pScenarioSections; pSect; pSect = pSect->pNext)
	{
		// compose section filename
		SCopy(C4CFN_ScenarioSections, fn);
		SDelete(fn, 1, iWildcardPos); SInsert(fn, pSect->name.getData(), iWildcardPos);
		// do not save self, because that is implied in CurrentScenarioSection and the main landscape/object data
		if (pSect == Game.pCurrentScenarioSection)
			pSaveGroup->DeleteEntry(fn);
		else if (pSect->fModified)
		{
			// modified section: delete current
			pSaveGroup->DeleteEntry(fn);
			// replace by new
			pSaveGroup->Add(pSect->temp_filename.getData(), fn);
		}
	}
	// done, success
	return true;
}

bool C4GameSave::SaveLandscape()
{
	// exact?
	if (::Landscape.GetMode() ==  LandscapeMode::Exact || GetForceExactLandscape())
	{
		C4DebugRecOff DBGRECOFF;
		// Landscape
		bool fSuccess;
		if (::Landscape.GetMode() ==  LandscapeMode::Exact)
			fSuccess = !!::Landscape.Save(*pSaveGroup);
		else
			fSuccess = !!::Landscape.SaveDiff(*pSaveGroup, !IsSynced());
		if (!fSuccess) return false;
		DBGRECOFF.Clear();
		// PXS
		if (!::PXS.Save(*pSaveGroup)) return false;
		// MassMover (create copy, may not modify running data)
		C4MassMoverSet MassMoverSet;
		MassMoverSet.Copy(::MassMover);
		if (!MassMoverSet.Save(*pSaveGroup)) return false;
		// Material enumeration
		if (!::MaterialMap.SaveEnumeration(*pSaveGroup)) return false;
	}
	// static / dynamic
	if (::Landscape.GetMode() ==  LandscapeMode::Static)
	{
		// static map
		// remove old-style landscape.bmp
		pSaveGroup->DeleteEntry(C4CFN_Landscape);
		// save materials if not already done
		if (!GetForceExactLandscape())
		{
			// save map
			if (!::Landscape.SaveMap(*pSaveGroup)) return false;
			// save textures (if changed)
			if (!::Landscape.SaveTextures(*pSaveGroup)) return false;
		}
	}
	else if (::Landscape.GetMode() !=  LandscapeMode::Exact)
	{
		// dynamic map by landscape.txt or scenario core: nothing to save
		// in fact, it doesn't even make much sense to save the Objects.txt
		// but the user pressed save after all...
	}
	return true;
}

bool C4GameSave::SaveRuntimeData()
{
	// Game.txt data (general runtime data and objects)
	C4ValueNumbers numbers;
	if (!Game.SaveData(*pSaveGroup, false, IsExact(), IsSynced(), &numbers))
		{ Log(LoadResStr("IDS_ERR_SAVE_RUNTIMEDATA")); return false; }
	// scenario sections (exact only)
	if (IsExact()) if (!SaveScenarioSections())
			{ Log(LoadResStr("IDS_ERR_SAVE_SCENSECTIONS")); return false; }
	// landscape
	if (!SaveLandscape()) { Log(LoadResStr("IDS_ERR_SAVE_LANDSCAPE")); return false; }
	// Round results
	if (GetSaveUserPlayers()) if (!Game.RoundResults.Save(*pSaveGroup))
			{ Log(LoadResStr("IDS_ERR_ERRORSAVINGROUNDRESULTS")); return false; }
	// Teams
	if (!Game.Teams.Save(*pSaveGroup))
		{ Log(LoadResStr(LoadResStr("IDS_ERR_ERRORSAVINGTEAMS"))); return false; }
	if (GetSaveUserPlayers() || GetSaveScriptPlayers())
	{
		// player infos
		// the stored player info filenames will point into the scenario file, and no resource information
		// will be saved. PlayerInfo must be saved first, because those will generate the storage filenames to be used by
		// C4PlayerList
		C4PlayerInfoList RestoreInfos;
		RestoreInfos.SetAsRestoreInfos(Game.PlayerInfos, GetSaveUserPlayers(), GetSaveScriptPlayers(), GetSaveUserPlayerFiles(), GetSaveScriptPlayerFiles());
		if (!RestoreInfos.Save(*pSaveGroup, C4CFN_SavePlayerInfos))
			{ Log(LoadResStr("IDS_ERR_SAVE_RESTOREPLAYERINFOS")); return false; }
		// Players
		// this will save the player files to the savegame scenario group only
		// synchronization to the original player files will be done in global game
		// synchronization (via control queue)
		if (GetSaveUserPlayerFiles() || GetSaveScriptPlayerFiles())
		{
			if (!::Players.Save((*pSaveGroup), GetCreateSmallFile(), RestoreInfos))
				{ Log(LoadResStr("IDS_ERR_SAVE_PLAYERS")); return false; }
		}
	}
	else
	{
		// non-exact runtime data: remove any exact files
		// No player files
		pSaveGroup->Delete(C4CFN_PlayerInfos);
		pSaveGroup->Delete(C4CFN_SavePlayerInfos);
	}
	// done, success
	return true;
}

bool C4GameSave::SaveDesc(C4Group &hToGroup)
{
	// Unfortunately, there's no way to prealloc the buffer in an appropriate size
	StdStrBuf sBuffer;

	// Scenario title
	sBuffer.Append(Game.ScenarioTitle.getData());
	sBuffer.Append("\n\n");

	// OK; each specializations has its own desc format
	WriteDesc(sBuffer);

	// Generate Filename
	StdStrBuf sFilename; char szLang[3];
	SCopyUntil(Config.General.Language, szLang, ',', 2);
	sFilename.Format(C4CFN_ScenarioDesc,szLang);

	// Save to file
	return !!hToGroup.Add(sFilename.getData(),sBuffer,false,true);
}

void C4GameSave::WriteDescLineFeed(StdStrBuf &sBuf)
{
	// paragraph end + cosmetics
	sBuf.Append("\n\n");
}

void C4GameSave::WriteDescDate(StdStrBuf &sBuf, bool fRecord)
{
	// write local time/date
	time_t tTime; time(&tTime);
	struct tm *pLocalTime;
	pLocalTime=localtime(&tTime);
	sBuf.AppendFormat(LoadResStr(fRecord ? "IDS_DESC_DATEREC" : (::Network.isEnabled() ? "IDS_DESC_DATENET" : "IDS_DESC_DATE")),
	                  pLocalTime->tm_mday,
	                  pLocalTime->tm_mon+1,
	                  pLocalTime->tm_year+1900,
	                  pLocalTime->tm_hour,
	                  pLocalTime->tm_min);
	WriteDescLineFeed(sBuf);
}

void C4GameSave::WriteDescGameTime(StdStrBuf &sBuf)
{
	// Write game duration
	if (Game.Time)
	{
		sBuf.AppendFormat(LoadResStr("IDS_DESC_DURATION"),
		                  Game.Time/3600,(Game.Time%3600)/60,Game.Time%60);
		WriteDescLineFeed(sBuf);
	}
}

void C4GameSave::WriteDescEngine(StdStrBuf &sBuf)
{
	char ver[32]; sprintf(ver, "%d.%d", (int)C4XVER1, (int)C4XVER2);
	sBuf.AppendFormat(LoadResStr("IDS_DESC_VERSION"), ver);
	WriteDescLineFeed(sBuf);
}

void C4GameSave::WriteDescLeague(StdStrBuf &sBuf, bool fLeague, const char *strLeagueName)
{
	if (fLeague)
	{
		sBuf.AppendFormat(LoadResStr("IDS_PRC_LEAGUE"), strLeagueName);
		WriteDescLineFeed(sBuf);
	}
}

void C4GameSave::WriteDescDefinitions(StdStrBuf &sBuf)
{
	// Definition specs
	if (Game.DefinitionFilenames[0])
	{
		char szDef[_MAX_PATH_LEN];
		// Desc
		sBuf.Append(LoadResStr("IDS_DESC_DEFSPECS"));
		// Get definition modules
		for (int cnt=0; SGetModule(Game.DefinitionFilenames,cnt,szDef); cnt++)
		{
			// Get exe relative path
			StdStrBuf sDefFilename;
			sDefFilename.Copy(Config.AtRelativePath(szDef));
			// Append comma
			if (cnt>0) sBuf.Append(", ");
			// Apend to desc
			sBuf.Append(sDefFilename);
		}
		// End of line
		WriteDescLineFeed(sBuf);
	}
}

void C4GameSave::WriteDescNetworkClients(StdStrBuf &sBuf)
{
	// Desc
	sBuf.Append(LoadResStr("IDS_DESC_CLIENTS"));
	// Client names
	for (C4Network2Client *pClient=::Network.Clients.GetNextClient(nullptr); pClient; pClient=::Network.Clients.GetNextClient(pClient))
		{ sBuf.Append(", ");  sBuf.Append(pClient->getName()); }
	// End of line
	WriteDescLineFeed(sBuf);
}

void C4GameSave::WriteDescPlayers(StdStrBuf &sBuf, bool fByTeam, int32_t idTeam)
{
	// write out all players; only if they match the given team if specified
	C4PlayerInfo *pPlr; bool fAnyPlrWritten = false;
	for (int i = 0; (pPlr = Game.PlayerInfos.GetPlayerInfoByIndex(i)); i++)
		if (pPlr->HasJoined() && !pPlr->IsRemoved() && !pPlr->IsInvisible())
		{
			if (fByTeam)
			{
				if (idTeam)
				{
					// match team
					if (pPlr->GetTeam() != idTeam) continue;
				}
				else
				{
					// must be in no known team
					if (Game.Teams.GetTeamByID(pPlr->GetTeam())) continue;
				}
			}
			if (fAnyPlrWritten)
				sBuf.Append(", ");
			else if (fByTeam && idTeam)
			{
				C4Team *pTeam = Game.Teams.GetTeamByID(idTeam);
				if (pTeam) sBuf.AppendFormat("%s: ", pTeam->GetName());
			}
			sBuf.Append(pPlr->GetName());
			fAnyPlrWritten = true;
		}
	if (fAnyPlrWritten) WriteDescLineFeed(sBuf);
}

void C4GameSave::WriteDescPlayers(StdStrBuf &sBuf)
{
	// New style using Game.PlayerInfos
	if (Game.PlayerInfos.GetPlayerCount())
	{
		sBuf.Append(LoadResStr("IDS_DESC_PLRS"));
		if (Game.Teams.IsMultiTeams() && !Game.Teams.IsAutoGenerateTeams())
		{
			// Teams defined: Print players sorted by teams
			WriteDescLineFeed(sBuf);
			C4Team *pTeam; int32_t i=0;
			while ((pTeam = Game.Teams.GetTeamByIndex(i++)))
			{
				WriteDescPlayers(sBuf, true, pTeam->GetID());
			}
			// Finally, print out players outside known teams (those can only be achieved by script using SetPlayerTeam)
			WriteDescPlayers(sBuf, true, 0);
		}
		else
		{
			// No teams defined: Print all players that have ever joined
			WriteDescPlayers(sBuf, false, 0);
		}
	}
}

bool C4GameSave::Save(const char *szFilename)
{
	// close any previous
	Close();
	// create group
	C4Group *pLSaveGroup = new C4Group();
	if (!SaveCreateGroup(szFilename, *pLSaveGroup))
	{
		LogF(LoadResStr("IDS_ERR_SAVE_TARGETGRP"), szFilename ? szFilename : "nullptr!");
		delete pLSaveGroup;
		return false;
	}
	// save to it
	return Save(*pLSaveGroup, true);
}

bool C4GameSave::Save(C4Group &hToGroup, bool fKeepGroup)
{
	// close any previous
	Close();
	// set group
	pSaveGroup = &hToGroup; fOwnGroup = fKeepGroup;
	// PreSave-actions (virtual call)
	if (!OnSaving()) return false;
	// always save core
	if (!SaveCore()) { Log(LoadResStr("IDS_ERR_SAVE_CORE")); return false; }
	// cleanup group
	pSaveGroup->Delete(C4CFN_PlayerFiles);
	// remove: Title text, image and icon if specified
	if (!GetKeepTitle())
	{
		pSaveGroup->Delete(FormatString("%s.*",C4CFN_ScenarioTitle).getData());
		pSaveGroup->Delete(C4CFN_ScenarioIcon);
		pSaveGroup->Delete(FormatString(C4CFN_ScenarioDesc,"*").getData());
		pSaveGroup->Delete(C4CFN_Titles);
		pSaveGroup->Delete(C4CFN_Info);
	}
	// save additional runtime data
	if (GetSaveRuntimeData()) if (!SaveRuntimeData()) return false;
	// Desc
	if (GetSaveDesc())
		if (!SaveDesc(*pSaveGroup))
			Log(LoadResStr("IDS_ERR_SAVE_DESC"));  /* nofail */
	// save specialized components (virtual call)
	if (!SaveComponents()) return false;
	// done, success
	return true;
}

bool C4GameSave::Close()
{
	bool fSuccess = true;
	// any group open?
	if (pSaveGroup)
	{
		// sort group
		const char *szSortOrder = GetSortOrder();
		if (szSortOrder) pSaveGroup->Sort(szSortOrder);
		// close if owned group
		if (fOwnGroup)
		{
			fSuccess = !!pSaveGroup->Close();
			delete pSaveGroup;
			fOwnGroup = false;
		}
		pSaveGroup = nullptr;
	}
	return fSuccess;
}


// *** C4GameSaveSavegame

bool C4GameSaveSavegame::OnSaving()
{
	if (!Game.IsRunning) return true;
	// synchronization to sync player files on all clients
	// this resets playing times and stores them in the players?
	// but doing so would be too late when the queue is executed!
	// TODO: remove it? (-> PeterW ;))
	if (::Network.isEnabled())
		Game.Input.Add(CID_Synchronize, new C4ControlSynchronize(true));
	else
		::Players.SynchronizeLocalFiles();
	// OK; save now
	return true;
}

void C4GameSaveSavegame::AdjustCore(C4Scenario &rC4S)
{
	// Determine save game index from trailing number in group file name
	int iSaveGameIndex = GetTrailingNumber(GetFilenameOnly(pSaveGroup->GetFullName().getData()));
	// Looks like a decent index: set numbered icon
	if (Inside(iSaveGameIndex, 1, 10))
		rC4S.Head.Icon = 2 + (iSaveGameIndex - 1);
	// Else: set normal script icon
	else
		rC4S.Head.Icon = 29;
}

bool C4GameSaveSavegame::SaveComponents()
{
	// special for savegames: save a screenshot
	if (!Game.SaveGameTitle((*pSaveGroup)))
		Log(LoadResStr("IDS_ERR_SAVE_GAMETITLE")); /* nofail */
	// done, success
	return true;
}

bool C4GameSaveSavegame::WriteDesc(StdStrBuf &sBuf)
{
	// compose savegame desc
	WriteDescDate(sBuf);
	WriteDescGameTime(sBuf);
	WriteDescDefinitions(sBuf);
	if (::Network.isEnabled()) WriteDescNetworkClients(sBuf);
	WriteDescPlayers(sBuf);
	// done, success
	return true;
}


// *** C4GameSaveRecord

void C4GameSaveRecord::AdjustCore(C4Scenario &rC4S)
{
	// specific recording flags
	rC4S.Head.Replay=true;
	if (!rC4S.Head.Film) rC4S.Head.Film=C4SFilm_Normal; /* default to film */
	rC4S.Head.Icon=29;
	// default record title
	char buf[1024 + 1];
	sprintf(buf, "%03i %s [%d.%d]", iNum, Game.ScenarioTitle.getData(), (int)C4XVER1, (int)C4XVER2);
	rC4S.Head.Title = buf;
}

bool C4GameSaveRecord::SaveComponents()
{
	// special: records need player infos even if done initially
	if (fInitial) Game.PlayerInfos.Save((*pSaveGroup), C4CFN_PlayerInfos);
	// for !fInitial, player infos will be saved as regular runtime data
	// done, success
	return true;
}

bool C4GameSaveRecord::WriteDesc(StdStrBuf &sBuf)
{
	// compose record desc
	WriteDescDate(sBuf, true);
	WriteDescGameTime(sBuf);
	WriteDescEngine(sBuf);
	WriteDescDefinitions(sBuf);
	WriteDescLeague(sBuf, fLeague, Game.Parameters.League.getData());
	if (::Network.isEnabled()) WriteDescNetworkClients(sBuf);
	WriteDescPlayers(sBuf);
	// done, success
	return true;
}


// *** C4GameSaveNetwork

void C4GameSaveNetwork::AdjustCore(C4Scenario &rC4S)
{
	// specific dynamic flags
	rC4S.Head.NetworkGame=true;
	rC4S.Head.NetworkRuntimeJoin = !fInitial;
}
