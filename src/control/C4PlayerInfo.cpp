/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2004-2009, RedWolf Design GmbH, http://www.clonk.de/
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
// permanent player information management
// see header for some additional information

#include "C4Include.h"
#include "control/C4PlayerInfo.h"

#include "c4group/C4Components.h"
#include "control/C4GameControl.h"
#include "game/C4FullScreen.h"
#include "player/C4Player.h"
#include "player/C4PlayerList.h"

// *** C4PlayerInfo

void C4PlayerInfo::Clear()
{
	// del temp file
	DeleteTempFile();
	// clear fields
	sName.Clear(); szFilename.Clear();
	pRes = nullptr;
	ResCore.Clear();
	// default fields
	dwColor = dwOriginalColor = 0xffffffff;
	dwAlternateColor = 0;
	dwFlags = 0;
	iID = idSavegamePlayer = idTeam = 0;
	iInGameNumber = iInGameJoinFrame = iInGamePartFrame = -1;
	sLeagueAccount = ""; iLeagueScore=iLeagueRank=0;
	iLeagueProjectedGain = -1;
	eType = C4PT_User;
	idExtraData = C4ID::None;
	iLeaguePerformance = 0;
	sLeagueProgressData.Clear();
}

void C4PlayerInfo::DeleteTempFile()
{
	// is temp file?
	if (!! szFilename && (dwFlags & PIF_TempFile))
	{
		// erase it
		EraseItem(szFilename.getData());
		// reset flag and filename to prevent double deletion
		dwFlags &= ~PIF_TempFile;
		szFilename.Clear();
	}
}

bool C4PlayerInfo::LoadFromLocalFile(const char *szFilename)
{
	// players should not be added in replay mode
	assert(!Game.C4S.Head.Replay);
	// clear previous
	Clear();
	// open player file group
	C4Group Grp;
	if (!Reloc.Open(Grp, szFilename)) return false;

	// read core
	C4PlayerInfoCore C4P;
	if (!C4P.Load(Grp)) return false;
	// set values
	eType = C4PT_User;
	sName = C4P.PrefName;
	this->szFilename = szFilename;
	dwColor = dwOriginalColor = 0xff000000 | (C4P.PrefColorDw & 0xffffff); // ignore alpha
	dwAlternateColor = 0xff000000 | (C4P.PrefColor2Dw & 0xffffff); // ignore alpha
	// network: resource (not for replays, because everyone has the player files there...)
	if (::Network.isEnabled() && !Game.C4S.Head.Replay)
	{
		// add resource
		// 2do: rejoining players need to update their resource version when saving the player
		// otherwise, player file versions may differ
		pRes = ::Network.ResList.getRefRes(szFilename, true);
		// not found? add
		if (!pRes) pRes = ::Network.ResList.AddByGroup(&Grp, false, NRT_Player, -1, szFilename);
		if (!pRes) return false;
		// set core and flag
		ResCore = pRes->getCore();
		dwFlags |= PIF_HasRes;
		// filename is no longer needed in network mode, because it's stored in the res-core
	}
	// done, success
	return true;
}

bool C4PlayerInfo::SetAsScriptPlayer(const char *szName, uint32_t dwColor, uint32_t dwFlags, C4ID idExtra)
{
	// clear previous
	Clear();
	// set parameters
	eType = C4PT_Script;
	this->dwColor = dwOriginalColor = 0xff000000 | (dwColor & 0xffffff); // ignore alpha
	dwAlternateColor = 0;
	this->sName.CopyValidated(szName);
	idExtraData = idExtra;
	this->dwFlags |= dwFlags;
	// done, success
	return true;
}

const char *C4PlayerInfo::GetLocalJoinFilename() const
{
	// preferred: by resource
	if (pRes) return pRes->getFile();
	// if no resource is known (replay or non-net), return filename
	return szFilename.getData();
}

uint32_t C4PlayerInfo::GetLobbyColor() const
{
	// special case if random teams and team colors are enabled in lobby:
	// Unjoined players do not show their team! Instead, they just display their original color
	if (Game.Teams.GetTeamDist() == C4TeamList::TEAMDIST_RandomInv)
		if (Game.Teams.IsTeamColors())
			if (Game.Teams.GetTeamByID(GetTeam()))
				if (!HasJoined() && !GetAssociatedSavegamePlayerID())
					return GetOriginalColor();
	// otherwise, just show the normal player color
	return GetColor();
}

StdStrBuf C4PlayerInfo::GetLobbyName() const
{
	// return player name including colored clan/team tag if known
	StdStrBuf sResult;
	if (sLeagueAccount.getLength())
	{
		if (sClanTag.getLength())
		{
			// gray team tag color used in lobby and game evaluation dialog!
			sResult.Format("<c afafaf>%s</c> %s", sClanTag.getData(), sLeagueAccount.getData());
		}
		else
			sResult.Ref(sLeagueAccount);
	}
	else
	{
		// fallback to regular player name
		sResult.Ref(sForcedName.getLength() ? static_cast<const StdStrBuf &>(sForcedName) : static_cast<const StdStrBuf &>(sName));
	}
	return sResult;
}

bool C4PlayerInfo::HasTeamWon() const
{
	// team win/solo win
	C4Team *pTeam;
	if (idTeam && (pTeam = Game.Teams.GetTeamByID(idTeam)))
		return pTeam->HasWon();
	else
		return HasWon();
}

void C4PlayerInfo::CompileFunc(StdCompiler *pComp)
{
	// Names
	pComp->Value(mkNamingAdapt(sName, "Name", ""));
	pComp->Value(mkNamingAdapt(sForcedName, "ForcedName", ""));
	pComp->Value(mkNamingAdapt(szFilename, "Filename", ""));

	// Flags
	const StdBitfieldEntry<uint16_t> Entries[] =
	{
		{ "Joined", PIF_Joined },
		{ "Removed", PIF_Removed },
		{ "HasResource", PIF_HasRes },
		{ "JoinIssued", PIF_JoinIssued },
		{ "SavegameJoin", PIF_JoinedForSavegameOnly },
		{ "Disconnected", PIF_Disconnected },
		{ "VotedOut", PIF_VotedOut },
		{ "Won", PIF_Won },
		{ "AttributesFixed", PIF_AttributesFixed },
		{ "NoScenarioInit", PIF_NoScenarioInit },
		{ "NoScenarioSave", PIF_NoScenarioSave },
		{ "NoEliminationCheck", PIF_NoEliminationCheck },
		{ "Invisible", PIF_Invisible},
		{ nullptr, 0 },
	};
	uint16_t dwSyncFlags = dwFlags & PIF_SyncFlags; // do not store local flags!
	pComp->Value(mkNamingAdapt(mkBitfieldAdapt(dwSyncFlags, Entries), "Flags", 0u));
	if (pComp->isDeserializer()) dwFlags = dwSyncFlags;
	pComp->Value(mkNamingAdapt(iID, "ID", 0));

	// type
	StdEnumEntry<C4PlayerType> PlayerTypes[] =
	{
		{ "User",   C4PT_User },
		{ "Script", C4PT_Script },

		{ nullptr,  C4PT_User },
	};
	pComp->Value(mkNamingAdapt(mkEnumAdaptT<uint8_t>(eType, PlayerTypes), "Type", C4PT_User));

	// safety: Do not allow invisible regular players
	if (pComp->isDeserializer())
	{
		if (eType != C4PT_Script) dwFlags &= ~PIF_Invisible;
	}

	// load colors
	pComp->Value(mkNamingAdapt(dwColor, "Color", 0u));
	pComp->Value(mkNamingAdapt(dwOriginalColor, "OriginalColor", dwColor));
	// load savegame ID
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(idSavegamePlayer), "SavgamePlayer", 0));
	// load team ID
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(idTeam), "Team", 0));
	// load authentication ID
	pComp->Value(mkNamingAdapt(szAuthID, "AUID", ""));

	// InGame info
	if (dwFlags & PIF_Joined)
	{
		pComp->Value(mkNamingAdapt(iInGameNumber,    "GameNumber", -1));
		pComp->Value(mkNamingAdapt(iInGameJoinFrame, "GameJoinFrame", -1));
	}
	else
		iInGameNumber = iInGameJoinFrame = -1;

	if (dwFlags & PIF_Removed)
		pComp->Value(mkNamingAdapt(iInGamePartFrame, "GamePartFrame", -1));
	else
		iInGamePartFrame = -1;

	// script player extra data
	pComp->Value(mkNamingAdapt(idExtraData, "ExtraData", C4ID::None));

	// load league info
	pComp->Value(mkNamingAdapt(sLeagueAccount, "LeagueAccount", ""));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iLeagueScore), "LeagueScore", 0));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iLeagueRank), "LeagueRank", 0));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iLeagueRankSymbol), "LeagueRankSymbol", 0));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iLeagueProjectedGain), "ProjectedGain", -1));
	pComp->Value(mkNamingAdapt(mkParAdapt(sClanTag, StdCompiler::RCT_All), "ClanTag", ""));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iLeaguePerformance), "LeaguePerformance", 0));
	pComp->Value(mkNamingAdapt(sLeagueProgressData, "LeagueProgressData", ""));

	// file resource
	if (pComp->isDeserializer() && Game.C4S.Head.Replay)
	{
		// Replays don't have player resources, drop the flag
		dwFlags &= ~PIF_HasRes;
	}
	if (dwFlags & PIF_HasRes)
	{
		// ResCore
		if (pComp->isSerializer() && pRes)
		{
			// ensure ResCore is up-to-date
			ResCore = pRes->getCore();
		}
		pComp->Value(mkNamingAdapt(ResCore, "ResCore"));
	}

}

void C4PlayerInfo::SetFilename(const char *szToFilename)
{
	szFilename = szToFilename;
}

void C4PlayerInfo::SetToScenarioFilename(const char *szScenFilename)
{
	// kill res
	DiscardResource();
	// set new filename
	SetFilename(szScenFilename);
	// flag scenario filename
	dwFlags |= PIF_InScenarioFile;
}

void C4PlayerInfo::LoadResource()
{
	// only if any resource present and not yet assigned
	if (IsRemoved() || !(dwFlags & PIF_HasRes) || pRes) return;
	// Ignore res if a local file is to be used
	// the PIF_InScenarioFile is not set for startup players in initial replays,
	// because resources are used for player joins but emulated in playback control
	// if there will ever be resources in replay mode, this special case can be removed
	if (Game.C4S.Head.Replay || (dwFlags & PIF_InScenarioFile))
		dwFlags &= ~PIF_HasRes;
	else
		// create resource (will check if resource already exists)
		if (!(pRes = ::Network.ResList.AddByCore(ResCore)))
		{
			dwFlags &= ~PIF_HasRes;
			// add failed? invalid resource??! -- TODO: may be too large to load
			LogF("Error: Could not add resource %d for player %s! Player file too large to load?", (int) ResCore.getID(), (const char *) GetFilename());
		}
}

void C4PlayerInfo::DiscardResource()
{
	// del any file resource
	if (pRes)
	{
		assert(dwFlags & PIF_HasRes);
		pRes = nullptr;
		dwFlags &= ~PIF_HasRes;
	}
	else assert(~dwFlags & PIF_HasRes);
	ResCore.Clear();
}

bool C4PlayerInfo::SetSavegameResume(C4PlayerInfo *pSavegameInfo)
{
	// copy some data fields; but not the file fields, because the join method is determined by this player
	if (!pSavegameInfo) return false;
	iID = pSavegameInfo->GetID();
	dwFlags = (dwFlags & ~PIF_SavegameTakeoverFlags) | (pSavegameInfo->GetFlags() & PIF_SavegameTakeoverFlags);
	dwColor = pSavegameInfo->GetColor(); // redundant; should be done by host already
	idTeam = pSavegameInfo->GetTeam();
	return true;
}

void C4PlayerInfo::SetJoined(int32_t iNumber)
{
	// mark as joined in current frame
	iInGameNumber = iNumber;
	iInGameJoinFrame = Game.FrameCounter;
	dwFlags |= PIF_Joined;
}

void C4PlayerInfo::SetRemoved()
{
	// mark as removed - always marks as previously joined, too
	dwFlags |= PIF_Joined | PIF_Removed;
	// remember removal frame
	iInGamePartFrame = Game.FrameCounter;
}

bool C4PlayerInfo::LoadBigIcon(C4FacetSurface &fctTarget)
{
	bool fSuccess = false;
	// load BigIcon.png of player into target facet; return false if no bigicon present or player file not yet loaded
	C4Group Plr;
	C4Network2Res *pRes = nullptr;
	bool fIncompleteRes = false;
	if ((pRes = GetRes()))
		if (!pRes->isComplete())
			fIncompleteRes = true;
	size_t iBigIconSize=0;
	if (!fIncompleteRes)
		if (Plr.Open(pRes ? pRes->getFile() : GetFilename()))
			if (Plr.AccessEntry(C4CFN_BigIcon, &iBigIconSize))
				if (iBigIconSize<=C4NetResMaxBigicon*1024)
					if (fctTarget.Load(Plr, C4CFN_BigIcon, C4FCT_Full, C4FCT_Full, false, 0))
						fSuccess = true;
	return fSuccess;
}


// *** C4ClientPlayerInfos

C4ClientPlayerInfos::C4ClientPlayerInfos(const char *szJoinFilenames, bool fAdd, C4PlayerInfo *pAddInfo)
{
	// init for local client?
	if (szJoinFilenames || pAddInfo)
	{
		// set developer flag for developer hosts
		if (SSearch(Config.GetRegistrationData("Type"), "Developer"))
			dwFlags |= CIF_Developer;
		// set local ID
		iClientID = ::Control.ClientID();
		// maybe control is not preinitialized
		if (!::Control.isNetwork() && iClientID < 0) iClientID = 0;
		// join packet or initial packet?
		if (fAdd)
			// packet is to be added to other players
			dwFlags |= CIF_AddPlayers;
		else
			// set initial flag for first-time join packet
			dwFlags |= CIF_Initial;
		// join all players in list
		if ((iPlayerCapacity = (szJoinFilenames ? SModuleCount(szJoinFilenames) : 0) + !!pAddInfo))
		{
			ppPlayers = new C4PlayerInfo *[iPlayerCapacity];
			if (szJoinFilenames)
			{
				char szPlrFile[_MAX_PATH_LEN];
				for (int32_t i=0; i<iPlayerCapacity; ++i)
					if (SGetModule(szJoinFilenames, i, szPlrFile, _MAX_PATH))
					{
						C4PlayerInfo *pNewInfo = new C4PlayerInfo();
						if (pNewInfo->LoadFromLocalFile(szPlrFile))
						{
							// player def loaded; register and count it
							ppPlayers[iPlayerCount++] = pNewInfo;
						}
						else
						{
							// loading failure; clear info class
							delete pNewInfo;
							// 
							Log(FormatString(LoadResStr("IDS_ERR_LOAD_PLAYER"), szPlrFile).getData());
						}
					}
			}
			if (pAddInfo)
				ppPlayers[iPlayerCount++] = pAddInfo;
		}
	}
}

C4ClientPlayerInfos::C4ClientPlayerInfos(const C4ClientPlayerInfos &rCopy)
{
	// copy fields
	iClientID = rCopy.iClientID;
	if ((iPlayerCount = rCopy.iPlayerCount))
	{
		// copy player infos
		ppPlayers = new C4PlayerInfo *[iPlayerCapacity = rCopy.iPlayerCapacity];
		int32_t i = iPlayerCount;
		C4PlayerInfo **ppCurrPlrInfo = ppPlayers, **ppSrcPlrInfo = rCopy.ppPlayers;
		while (i--) *ppCurrPlrInfo++ = new C4PlayerInfo(**ppSrcPlrInfo++);
	}
	// no players
	else
	{
		ppPlayers = nullptr;
		iPlayerCapacity = 0;
	}
	// misc fields
	dwFlags = rCopy.dwFlags;
}

C4ClientPlayerInfos &C4ClientPlayerInfos::operator = (const C4ClientPlayerInfos &rCopy)
{
	Clear();
	// copy fields
	iClientID = rCopy.iClientID;
	if ((iPlayerCount = rCopy.iPlayerCount))
	{
		// copy player infos
		ppPlayers = new C4PlayerInfo *[iPlayerCapacity = rCopy.iPlayerCapacity];
		int32_t i = iPlayerCount;
		C4PlayerInfo **ppCurrPlrInfo = ppPlayers, **ppSrcPlrInfo = rCopy.ppPlayers;
		while (i--) *ppCurrPlrInfo++ = new C4PlayerInfo(**ppSrcPlrInfo++);
	}
	// no players
	else
	{
		ppPlayers = nullptr;
		iPlayerCapacity = 0;
	}
	// misc fields
	dwFlags = rCopy.dwFlags;
	return *this;
}


void C4ClientPlayerInfos::Clear()
{
	// del player infos
	int32_t i = iPlayerCount; C4PlayerInfo **ppCurrPlrInfo = ppPlayers;
	while (i--) delete *ppCurrPlrInfo++;
	// del player info vector
	delete [] ppPlayers; ppPlayers = nullptr;
	// reset other fields
	iPlayerCount = iPlayerCapacity = 0;
	iClientID=-1;
	dwFlags = 0;
}

void C4ClientPlayerInfos::GrabMergeFrom(C4ClientPlayerInfos &rFrom)
{
	// anything to grab?
	if (!rFrom.iPlayerCount) return;
	// any previous players to copy?
	if (iPlayerCount)
	{
		// buffer sufficient?
		if (iPlayerCount + rFrom.iPlayerCount > iPlayerCapacity)
			GrowList(rFrom.iPlayerCount);
		// merge into new buffer
		memcpy(ppPlayers + iPlayerCount, rFrom.ppPlayers, rFrom.iPlayerCount * sizeof(C4PlayerInfo *));
		iPlayerCount += rFrom.iPlayerCount;
		rFrom.iPlayerCount = rFrom.iPlayerCapacity = 0;
		delete [] rFrom.ppPlayers; rFrom.ppPlayers = nullptr;
	}
	else
	{
		// no own players: take over buffer of pFrom
		if (ppPlayers) delete [] ppPlayers;
		ppPlayers = rFrom.ppPlayers; rFrom.ppPlayers = nullptr;
		iPlayerCount = rFrom.iPlayerCount; rFrom.iPlayerCount = 0;
		iPlayerCapacity = rFrom.iPlayerCapacity; rFrom.iPlayerCapacity = 0;
	}
}

void C4ClientPlayerInfos::AddInfo(C4PlayerInfo *pAddInfo)
{
	// grow list if necessary
	if (iPlayerCount == iPlayerCapacity) GrowList(4);
	// add info
	ppPlayers[iPlayerCount++] = pAddInfo;
}

void C4ClientPlayerInfos::RemoveIndexedInfo(int32_t iAtIndex)
{
	// bounds check
	if (iAtIndex<0 || iAtIndex>=iPlayerCount) return;
	// del player info at index
	delete ppPlayers[iAtIndex];
	// move down last index (may self-assign a ptr)
	ppPlayers[iAtIndex] = ppPlayers[--iPlayerCount];
}

void C4ClientPlayerInfos::RemoveInfo(int32_t idPlr)
{
	// check all infos; remove the one that matches
	int32_t i = 0; C4PlayerInfo **ppCurrPlrInfo = ppPlayers;
	while (i < iPlayerCount)
	{
		if ((*ppCurrPlrInfo)->GetID() == idPlr)
		{
			RemoveIndexedInfo(i);
			return;
		}
		++ppCurrPlrInfo; ++i;
	}
	// none matched
	return;
}

void C4ClientPlayerInfos::GrowList(size_t iByVal)
{
	// create new list (out of mem: simply returns here; info list remains in a valid state)
	C4PlayerInfo **ppNewInfo = new C4PlayerInfo *[iPlayerCapacity += iByVal];
	// move existing
	if (ppPlayers)
	{
		memcpy(ppNewInfo, ppPlayers, iPlayerCount * sizeof(C4PlayerInfo *));
		delete [] ppPlayers;
	}
	// assign new
	ppPlayers = ppNewInfo;
}

int32_t C4ClientPlayerInfos::GetFlaggedPlayerCount(DWORD dwFlag) const
{
	// check all players
	int32_t iCount = 0;
	int32_t i = iPlayerCount; C4PlayerInfo **ppCurrPlrInfo = ppPlayers;
	while (i--) if ((*ppCurrPlrInfo++)->GetFlags() | dwFlag)
			++iCount;
	// return number of matching infos
	return iCount;
}

C4PlayerInfo *C4ClientPlayerInfos::GetPlayerInfo(int32_t iIndex) const
{
	// check range
	if (iIndex<0 || iIndex>=iPlayerCount) return nullptr;
	// return indexed info
	return ppPlayers[iIndex];
}

C4PlayerInfo *C4ClientPlayerInfos::GetPlayerInfo(int32_t iIndex, C4PlayerType eType) const
{
	// get indexed matching info
	for (int32_t iCheck=0; iCheck<iPlayerCount; ++iCheck)
	{
		C4PlayerInfo *pNfo = ppPlayers[iCheck];
		if (pNfo->GetType() == eType)
			if (!iIndex--)
				return pNfo;
	}
	// nothing found
	return nullptr;
}

C4PlayerInfo *C4ClientPlayerInfos::GetPlayerInfoByID(int32_t id) const
{
	// check all infos
	int32_t i = iPlayerCount; C4PlayerInfo **ppCurrPlrInfo = ppPlayers;
	while (i--)
	{
		if ((*ppCurrPlrInfo)->GetID() == id) return *ppCurrPlrInfo;
		++ppCurrPlrInfo;
	}
	// none matched
	return nullptr;
}

C4PlayerInfo *C4ClientPlayerInfos::GetPlayerInfoByRes(int32_t idResID) const
{
	int32_t i = iPlayerCount; C4PlayerInfo **ppCurrPlrInfo = ppPlayers;
	C4Network2Res *pRes;
	while (i--)
	{
		if ((pRes = (*ppCurrPlrInfo)->GetRes()))
			if (pRes->getResID() == idResID)
				// only if the player is actually using the resource
				if ((*ppCurrPlrInfo)->IsUsingPlayerFile())
					return *ppCurrPlrInfo;
		++ppCurrPlrInfo;
	}
	return nullptr;
}

bool C4ClientPlayerInfos::HasUnjoinedPlayers() const
{
	// check all players
	int32_t i = iPlayerCount; C4PlayerInfo **ppCurrPlrInfo = ppPlayers;
	while (i--) if (!(*ppCurrPlrInfo++)->HasJoined()) return true;
	// all joined
	return false;
}

int32_t C4ClientPlayerInfos::GetJoinedPlayerCount() const
{
	// count all players with IsJoined()
	int32_t i = iPlayerCount; int32_t cnt=0; C4PlayerInfo **ppCurrPlrInfo = ppPlayers;
	while (i--) if ((*ppCurrPlrInfo++)->IsJoined()) ++cnt;
	return cnt;
}

void C4ClientPlayerInfos::CompileFunc(StdCompiler *pComp)
{
	bool deserializing = pComp->isDeserializer();
	if (deserializing) Clear();
	pComp->Value(mkNamingAdapt(iClientID, "ID", C4ClientIDUnknown));

	// Flags
	StdBitfieldEntry<uint32_t> Entries[] =
	{
		{ "AddPlayers", CIF_AddPlayers },
		{ "Updated", CIF_Updated },
		{ "Initial", CIF_Initial },
		{ "Developer", CIF_Developer },

		{ nullptr, 0 }
	};
	pComp->Value(mkNamingAdapt(mkBitfieldAdapt(dwFlags, Entries), "Flags", 0u));

	pComp->Value(mkNamingCountAdapt<int32_t>(iPlayerCount, "Player"));
	if (iPlayerCount < 0 || iPlayerCount > C4MaxPlayer)
		{ pComp->excCorrupt("player count out of range"); return; }
	// Grow list, if necessary
	if (deserializing && iPlayerCount > iPlayerCapacity)
	{
		GrowList(iPlayerCount - iPlayerCapacity);
		ZeroMem(ppPlayers, sizeof(*ppPlayers) * iPlayerCount);
	}
	// Compile
	pComp->Value(mkNamingAdapt(mkArrayAdaptMap(ppPlayers, iPlayerCount, mkPtrAdaptNoNull<C4PlayerInfo>), "Player"));
	// Force specialization
	mkPtrAdaptNoNull<C4PlayerInfo>(*ppPlayers);
}

void C4ClientPlayerInfos::LoadResources()
{
	// load for all players
	int32_t i = iPlayerCount; C4PlayerInfo **ppCurrPlrInfo = ppPlayers;
	while (i--) (*ppCurrPlrInfo++)->LoadResource();
}

// *** C4PlayerInfoList

C4PlayerInfoList::C4PlayerInfoList() = default;
	// ctor: no need to alloc mem yet

C4PlayerInfoList::C4PlayerInfoList(const C4PlayerInfoList &rCpy) : iClientCount(rCpy.iClientCount), iClientCapacity(rCpy.iClientCapacity),
		ppClients(nullptr), iLastPlayerID(rCpy.iLastPlayerID)
{
	// copy client info vector
	if (rCpy.ppClients)
	{
		ppClients = new C4ClientPlayerInfos*[iClientCapacity];
		C4ClientPlayerInfos **ppInfo = ppClients, **ppCpy = rCpy.ppClients;
		int32_t i = iClientCount;
		while (i--) *ppInfo++ = new C4ClientPlayerInfos(**ppCpy++);
	}
}

C4PlayerInfoList &C4PlayerInfoList::operator = (const C4PlayerInfoList &rCpy)
{
	Clear();
	iClientCount = rCpy.iClientCount;
	iClientCapacity = rCpy.iClientCapacity;
	iLastPlayerID = rCpy.iLastPlayerID;
	if (rCpy.ppClients)
	{
		ppClients = new C4ClientPlayerInfos*[iClientCapacity];
		C4ClientPlayerInfos **ppInfo = ppClients, **ppCpy = rCpy.ppClients;
		int32_t i = iClientCount;
		while (i--) *ppInfo++ = new C4ClientPlayerInfos(**ppCpy++);
	}
	else
		ppClients = nullptr;
	return *this;
}

void C4PlayerInfoList::Clear()
{
	// delete client infos
	C4ClientPlayerInfos **ppInfo = ppClients; int32_t i=iClientCount;
	while (i--) delete *ppInfo++;
	// clear client infos
	delete [] ppClients; ppClients = nullptr;
	iClientCount = iClientCapacity = 0;
	// reset player ID counter
	iLastPlayerID = 0;
}

void C4PlayerInfoList::GrowList(size_t iByVal)
{
	// create new list (out of mem: simply returns here; info list remains in a valid state)
	C4ClientPlayerInfos **ppNewInfo = new C4ClientPlayerInfos *[iClientCapacity += iByVal];
	// move existing
	if (ppClients)
	{
		memcpy(ppNewInfo, ppClients, iClientCount * sizeof(C4ClientPlayerInfos *));
		delete [] ppClients;
	}
	// assign new
	ppClients = ppNewInfo;
}

bool C4PlayerInfoList::DoLocalNonNetworkPlayerJoin(const char *szPlayerFile)
{
	// construct joining information
	C4ClientPlayerInfos *pNewJoin = new C4ClientPlayerInfos(szPlayerFile, true);
	// handle it
	bool fSuccess = DoLocalNonNetworkPlayerInfoUpdate(pNewJoin);
	// done
	delete pNewJoin;
	return fSuccess;
}

bool C4PlayerInfoList::DoPlayerInfoUpdate(C4ClientPlayerInfos *pUpdate)
{
	// never done by clients or in replay - update will be handled via queue
	if (!::Control.isCtrlHost()) return false;
	// in network game, process by host. In offline game, just create control
	bool fSucc = true;
	if (::Control.isNetwork())
		::Network.Players.RequestPlayerInfoUpdate(*pUpdate);
	else
		fSucc = DoLocalNonNetworkPlayerInfoUpdate(pUpdate);
	return fSucc;
}

bool C4PlayerInfoList::DoLocalNonNetworkPlayerInfoUpdate(C4ClientPlayerInfos *pUpdate)
{
	// assign IDs first: Must be done early, so AssignTeams works
	if (!AssignPlayerIDs(pUpdate))
	{
		return false;
	}
	// set standard teams
	AssignTeams(pUpdate, true);
	// color/name change by team or savegame assignment
	UpdatePlayerAttributes(pUpdate, true);
	// add through queue: This will add directly, do the record and put player joins into the queue
	// in running mode, this call will also put the actual player joins into the queue
	::Control.DoInput(CID_PlrInfo, new C4ControlPlayerInfo(*pUpdate), Game.IsRunning ? CDT_Queue : CDT_Direct);
	// done, success
	return true;
}

void C4PlayerInfoList::UpdatePlayerAttributes(C4ClientPlayerInfos *pForInfo, bool fResolveConflicts)
{
	assert(pForInfo);
	// update colors of all players of this packet
	C4PlayerInfo *pInfo, *pInfo2; int32_t i=0;
	while ((pInfo = pForInfo->GetPlayerInfo(i++)))
		if (!pInfo->HasJoined())
		{
			// assign savegame colors
			int32_t idSavegameID; bool fHasForcedColor = false; DWORD dwForceClr;
			if ((idSavegameID = pInfo->GetAssociatedSavegamePlayerID()))
				if ((pInfo2 = Game.RestorePlayerInfos.GetPlayerInfoByID(idSavegameID)))
				{
					dwForceClr = pInfo2->GetColor();
					fHasForcedColor = true;
				}
			// assign team colors
			if (!fHasForcedColor && Game.Teams.IsTeamColors())
			{
				C4Team *pPlrTeam = Game.Teams.GetTeamByID(pInfo->GetTeam());
				if (pPlrTeam)
				{
					dwForceClr = pPlrTeam->GetColor();
					fHasForcedColor = true;
				}
			}
			// do color change
			if (fHasForcedColor && (dwForceClr != pInfo->GetColor()))
			{
				pInfo->SetColor(dwForceClr);
				pForInfo->SetUpdated();
			}
			// make sure colors have correct alpha (modified engines might send malformed packages of transparent colors)
			if ((pInfo->GetColor() & 0xff000000u) != 0xff000000u)
			{
				pInfo->SetColor(pInfo->GetColor() | 0xff000000u);
				pForInfo->SetUpdated();
			}
		}
	if (fResolveConflicts) ResolvePlayerAttributeConflicts(pForInfo);
}

void C4PlayerInfoList::UpdatePlayerAttributes()
{
	// update attributes of all packets
	int32_t iIdx=0;
	C4ClientPlayerInfos *pForInfo;
	while ((pForInfo = GetIndexedInfo(iIdx++))) UpdatePlayerAttributes(pForInfo, false);
	// now resole all conflicts
	ResolvePlayerAttributeConflicts(nullptr);
}

bool C4PlayerInfoList::AssignPlayerIDs(C4ClientPlayerInfos *pNewClientInfo)
{
	// assign player IDs to those player infos without
	C4PlayerInfo *pPlrInfo; int32_t i=0;
	while ((pPlrInfo = pNewClientInfo->GetPlayerInfo(i++)))
		if (!pPlrInfo->GetID())
		{
			// are there still any player slots free?
			if (GetFreePlayerSlotCount()<1)
			{
				// nope - then deny this join!
				Log(FormatString(LoadResStr("IDS_MSG_TOOMANYPLAYERS"), (int)Game.Parameters.MaxPlayers).getData());
				pNewClientInfo->RemoveIndexedInfo(--i);
				continue;
			}
			// Join OK; grant an ID
			pPlrInfo->SetID(++iLastPlayerID);
		}
	// return whether any join remains
	return !!pNewClientInfo->GetPlayerCount();
}

int32_t C4PlayerInfoList::GetFreePlayerSlotCount()
{
	// number of free slots depends on max player setting
	return std::max<int32_t>(Game.Parameters.MaxPlayers - GetStartupCount(), 0);
}

void C4PlayerInfoList::AssignTeams(C4ClientPlayerInfos *pNewClientInfo, bool fByHost)
{
	if (!Game.Teams.IsMultiTeams()) return;
	// assign any unset teams (host/standalone only - fByHost determines whether the packet came from the host)
	C4PlayerInfo *pPlrInfo; int32_t i=0;
	while ((pPlrInfo = pNewClientInfo->GetPlayerInfo(i++)))
		Game.Teams.RecheckPlayerInfoTeams(*pPlrInfo, fByHost);
}

void C4PlayerInfoList::RecheckAutoGeneratedTeams()
{
	// ensure all teams specified in the list exist
	C4ClientPlayerInfos *pPlrInfos; int32_t j=0;
	while ((pPlrInfos = GetIndexedInfo(j++)))
	{
		C4PlayerInfo *pPlrInfo; int32_t i=0;
		while ((pPlrInfo = pPlrInfos->GetPlayerInfo(i++)))
		{
			int32_t idTeam = pPlrInfo->GetTeam();
			if (idTeam) Game.Teams.GetGenerateTeamByID(idTeam);
		}
	}
}

C4ClientPlayerInfos *C4PlayerInfoList::AddInfo(C4ClientPlayerInfos *pNewClientInfo)
{
	assert(pNewClientInfo);
	// caution: also called for RestorePlayerInfos-list
	// host: reserve new IDs for all players
	// client: all IDs should be assigned already by host
	if (::Network.isHost() || !::Network.isEnabled())
	{
		if (!AssignPlayerIDs(pNewClientInfo) && pNewClientInfo->IsAddPacket())
		{
			// no players could be added (probably MaxPlayer)
			delete pNewClientInfo;
			return nullptr;
		}
	}
	// ensure all teams specified in the list exist (this should be done for savegame teams as well)
	C4PlayerInfo *pInfo; int32_t i=0;
	while ((pInfo = pNewClientInfo->GetPlayerInfo(i++)))
	{
		int32_t idTeam = pInfo->GetTeam();
		if (idTeam) Game.Teams.GetGenerateTeamByID(idTeam);
	}
	// add info for client; overwriting or appending to existing info if necessary
	// try to find existing data of same client
	C4ClientPlayerInfos **ppExistingInfo = GetInfoPtrByClientID(pNewClientInfo->GetClientID());
	if (ppExistingInfo)
	{
		// info exists: append to it?
		if (pNewClientInfo->IsAddPacket())
		{
			(*ppExistingInfo)->GrabMergeFrom(*pNewClientInfo);
			// info added: remove unused class
			delete pNewClientInfo;
			// assign existing info for further usage in this fn
			return pNewClientInfo = *ppExistingInfo;
		}
		// no add packet: overwrite current info
		delete *ppExistingInfo;
		return *ppExistingInfo = pNewClientInfo;
	}
	// no existing info: add it directly
	pNewClientInfo->ResetAdd();
	// may need to grow list (vector) for that
	if (iClientCount >= iClientCapacity) GrowList(4);
	ppClients[iClientCount++] = pNewClientInfo;
	// done; return actual info
	return pNewClientInfo;
}

C4ClientPlayerInfos **C4PlayerInfoList::GetInfoPtrByClientID(int32_t iClientID) const
{
	// search list
	for (int32_t i=0; i<iClientCount; ++i) if (ppClients[i]->GetClientID() == iClientID) return ppClients+i;
	// nothing found
	return nullptr;
}

int32_t C4PlayerInfoList::GetPlayerCount() const
{
	// count players of all clients
	int32_t iCount=0;
	for (int32_t i=0; i<iClientCount; ++i)
		iCount += ppClients[i]->GetPlayerCount();
	// return it
	return iCount;
}

int32_t C4PlayerInfoList::GetJoinIssuedPlayerCount() const
{
	// count players of all clients
	int32_t iCount=0;
	for (int32_t i=0; i<iClientCount; ++i)
	{
		C4ClientPlayerInfos *pClient = ppClients[i];
		for (int32_t j=0; j<pClient->GetPlayerCount(); ++j)
			if (pClient->GetPlayerInfo(j)->HasJoinIssued())
				++iCount;
	}
	// return it
	return iCount;
}

int32_t C4PlayerInfoList::GetJoinPendingPlayerCount() const
{
	// count players of all clients
	int32_t iCount = 0;
	for (int32_t i = 0; i<iClientCount; ++i)
	{
		C4ClientPlayerInfos *pClient = ppClients[i];
		for (int32_t j = 0; j<pClient->GetPlayerCount(); ++j)
			if (pClient->GetPlayerInfo(j)->HasJoinPending())
				++iCount;
	}
	// return it
	return iCount;
}

int32_t C4PlayerInfoList::GetActivePlayerCount(bool fCountInvisible) const
{
	// count players of all clients
	int32_t iCount=0;
	for (int32_t i=0; i<iClientCount; ++i)
	{
		C4ClientPlayerInfos *pClient = ppClients[i];
		for (int32_t j=0; j<pClient->GetPlayerCount(); ++j)
		{
			C4PlayerInfo *pInfo = pClient->GetPlayerInfo(j);
			if (!pInfo->IsRemoved())
				if (fCountInvisible || !pInfo->IsInvisible())
					++iCount;
		}
	}
	// return it
	return iCount;
}

int32_t C4PlayerInfoList::GetActiveScriptPlayerCount(bool fCountSavegameResumes, bool fCountInvisible) const
{
	// count players of all clients
	int32_t iCount=0;
	for (int32_t i=0; i<iClientCount; ++i)
	{
		C4ClientPlayerInfos *pClient = ppClients[i];
		for (int32_t j=0; j<pClient->GetPlayerCount(); ++j)
		{
			C4PlayerInfo *pNfo = pClient->GetPlayerInfo(j);
			if (!pNfo->IsRemoved())
				if (pNfo->GetType() == C4PT_Script)
					if (fCountSavegameResumes || !pNfo->GetAssociatedSavegamePlayerID())
						if (fCountInvisible || !pNfo->IsInvisible())
							++iCount;
		}
	}
	// return it
	return iCount;
}

StdStrBuf C4PlayerInfoList::GetActivePlayerNames(bool fCountInvisible, int32_t iAtClientID) const
{
	// add up players of all clients
	StdStrBuf sPlr;
	int32_t iCount=0;
	for (int32_t i=0; i<iClientCount; ++i)
	{
		C4ClientPlayerInfos *pClient = ppClients[i];
		if (iAtClientID != -1 && pClient->GetClientID() != iAtClientID) continue;
		for (int32_t j=0; j<pClient->GetPlayerCount(); ++j)
		{
			C4PlayerInfo *pInfo = pClient->GetPlayerInfo(j);
			if (!pInfo->IsRemoved()) if (fCountInvisible || !pInfo->IsInvisible())
				{
					if (iCount++)
					{
						// not first name: Add separator
						sPlr.Append(", ");
					}
					sPlr.Append(pInfo->GetName());
				}
		}
	}
	// return it
	return sPlr;
}

C4PlayerInfo *C4PlayerInfoList::GetPlayerInfoByIndex(int32_t index) const
{
	// check all packets for a player
	for (int32_t i=0; i<iClientCount; ++i)
	{
		int32_t j=0; C4PlayerInfo *pInfo;
		while ((pInfo = ppClients[i]->GetPlayerInfo(j++)))
			if (index-- <= 0)
				return pInfo;
	}
	// nothing found
	return nullptr;
}

C4PlayerInfo *C4PlayerInfoList::GetPlayerInfoByID(int32_t id) const
{
	// must be a valid ID
	assert(id);
	// check all packets for a player
	for (int32_t i=0; i<iClientCount; ++i)
	{
		int32_t j=0; C4PlayerInfo *pInfo;
		while ((pInfo = ppClients[i]->GetPlayerInfo(j++)))
			if (pInfo->GetID() == id) return pInfo;
	}
	// nothing found
	return nullptr;
}

C4ClientPlayerInfos *C4PlayerInfoList::GetClientInfoByPlayerID(int32_t id) const
{
	// get client info that contains a specific player
	assert(id);
	for (int32_t i=0; i<iClientCount; ++i)
	{
		int32_t j=0; C4PlayerInfo *pInfo;
		while ((pInfo = ppClients[i]->GetPlayerInfo(j++)))
			if (pInfo->GetID() == id) return ppClients[i];
	}
	// nothing found
	return nullptr;
}

C4PlayerInfo *C4PlayerInfoList::GetPlayerInfoByID(int32_t id, int32_t *pidClient) const
{
	// must be a valid ID
	assert(id); assert(pidClient);
	// check all packets for a player
	for (int32_t i=0; i<iClientCount; ++i)
	{
		int32_t j=0; C4PlayerInfo *pInfo;
		while ((pInfo = ppClients[i]->GetPlayerInfo(j++)))
			if (pInfo->GetID() == id)
			{
				*pidClient = ppClients[i]->GetClientID();
				return pInfo;
			}
	}
	// nothing found
	return nullptr;
}

C4PlayerInfo *C4PlayerInfoList::GetPlayerInfoBySavegameID(int32_t id) const
{
	// must be a valid ID
	assert(id);
	// check all packets for a player
	for (int32_t i=0; i<iClientCount; ++i)
	{
		int32_t j=0; C4PlayerInfo *pInfo;
		while ((pInfo = ppClients[i]->GetPlayerInfo(j++)))
			if (pInfo->GetAssociatedSavegamePlayerID() == id) return pInfo;
	}
	// nothing found
	return nullptr;
}

C4PlayerInfo *C4PlayerInfoList::GetNextPlayerInfoByID(int32_t id) const
{
	// check all packets for players
	C4PlayerInfo *pSmallest=nullptr;
	for (int32_t i=0; i<iClientCount; ++i)
	{
		int32_t j=0; C4PlayerInfo *pInfo;
		while ((pInfo = ppClients[i]->GetPlayerInfo(j++)))
			if (pInfo->GetID() > id)
				if (!pSmallest || pSmallest->GetID()>pInfo->GetID())
					pSmallest = pInfo;
	}
	// return best found
	return pSmallest;
}

C4PlayerInfo *C4PlayerInfoList::GetActivePlayerInfoByName(const char *szName)
{
	// check all packets for matching players
	for (int32_t i=0; i<iClientCount; ++i)
	{
		int32_t j=0; C4PlayerInfo *pInfo;
		while ((pInfo = ppClients[i]->GetPlayerInfo(j++)))
			if (!pInfo->IsRemoved())
				if (SEqualNoCase(szName, pInfo->GetName()))
					return pInfo;
	}
	// nothing found
	return nullptr;
}

C4PlayerInfo *C4PlayerInfoList::FindSavegameResumePlayerInfo(const C4PlayerInfo *pMatchInfo, MatchingLevel mlMatchStart, MatchingLevel mlMatchEnd) const
{
	assert(pMatchInfo);
	// try different matching levels using the infamous for-case-paradigm
	for (int iMatchLvl = mlMatchStart; iMatchLvl <= mlMatchEnd; ++iMatchLvl)
	{
		for (int32_t i=0; i<iClientCount; ++i)
		{
			int32_t j=0; C4PlayerInfo *pInfo;
			while ((pInfo = ppClients[i]->GetPlayerInfo(j++)))
				if (!Game.PlayerInfos.GetPlayerInfoByID(pInfo->GetID()) && !Game.PlayerInfos.GetPlayerInfoBySavegameID(pInfo->GetID()))  // only unassigned player infos
					switch (iMatchLvl)
					{
					case PML_PlrFileName: // file name and player name must match
						if (!pMatchInfo->GetFilename() || !pInfo->GetFilename()) break;
						if (!SEqualNoCase(GetFilename(pMatchInfo->GetFilename()), GetFilename(pInfo->GetFilename()))) break;
						// nobreak: Check player name as well
					case PML_PlrName: // match player name
						if (SEqualNoCase(pMatchInfo->GetName(), pInfo->GetName()))
							return pInfo;
						break;
					case PML_PrefColor: // match player color
						if (pMatchInfo->GetOriginalColor() == pInfo->GetOriginalColor())
							return pInfo;
						break;
					case PML_Any: // match anything
						return pInfo;
					}
		}
	}
	// no match
	return nullptr;
}

C4PlayerInfo *C4PlayerInfoList::FindUnassociatedRestoreInfo(const C4PlayerInfoList &rRestoreInfoList)
{
	// search given list for a player that's not associated locally
	C4ClientPlayerInfos *pRestoreClient; int32_t iClient=0;
	while ((pRestoreClient = rRestoreInfoList.GetIndexedInfo(iClient++)))
	{
		C4PlayerInfo *pRestoreInfo; int32_t iInfo=0;
		while ((pRestoreInfo = pRestoreClient->GetPlayerInfo(iInfo++)))
			if (pRestoreInfo->IsJoined())
				// match association either by savegame ID (before C4Game::InitPlayers) or real ID (after C4Game::InitPlayers)
				if (!GetPlayerInfoBySavegameID(pRestoreInfo->GetID()) && !GetPlayerInfoByID(pRestoreInfo->GetID()))
					return pRestoreInfo;
	}
	// no unassociated info found
	return nullptr;
}

bool C4PlayerInfoList::HasSameTeamPlayers(int32_t iClient1, int32_t iClient2) const
{
	// compare all player teams of clients
	const C4ClientPlayerInfos *pCnfo1 = GetInfoByClientID(iClient1);
	const C4ClientPlayerInfos *pCnfo2 = GetInfoByClientID(iClient2);
	if (!pCnfo1 || !pCnfo2) return false;
	int32_t i=0,j; const C4PlayerInfo *pNfo1, *pNfo2;
	while ((pNfo1 = pCnfo1->GetPlayerInfo(i++)))
	{
		if (!pNfo1->IsUsingTeam()) continue;
		j=0;
		while ((pNfo2 = pCnfo2->GetPlayerInfo(j++)))
		{
			if (!pNfo2->IsUsingTeam()) continue;
			if (pNfo2->GetTeam() == pNfo1->GetTeam())
				// match found!
				return true;
		}
	}
	// no match
	return false;
}

bool C4PlayerInfoList::Load(C4Group &hGroup, const char *szFromFile, C4LangStringTable *pLang)
{
	// clear previous
	Clear();
	// load file contents
	StdStrBuf Buf;
	if (!hGroup.LoadEntryString(szFromFile, &Buf))
		// no file is OK; means no player infos
		return true;
	// replace strings
	if (pLang) pLang->ReplaceStrings(Buf);
	// (try to) compile
	if (!CompileFromBuf_LogWarn<StdCompilerINIRead>(
	      mkNamingAdapt(*this, "PlayerInfoList"),
	      Buf, szFromFile))
		return false;
	// done, success
	return true;
}

bool C4PlayerInfoList::Save(C4Group &hGroup, const char *szToFile)
{
	// remove previous entry from group
	hGroup.DeleteEntry(szToFile);
	// anything to save?
	if (!iClientCount) return true;
	// save it
	try
	{
		// decompile
		StdStrBuf Buf = DecompileToBuf<StdCompilerINIWrite>(
		                  mkNamingAdapt(*this, "PlayerInfoList"));
		// save buffer to group
		hGroup.Add(szToFile, Buf, false, true);
	}
	catch (StdCompiler::Exception *)
		{ return false; }
	// done, success
	return true;
}

bool C4PlayerInfoList::InitLocal()
{
	// not in replay
	if (Game.C4S.Head.Replay) return true;
	// no double init
	assert(!GetInfoCount());
	// no network
	assert(!::Network.isEnabled());
	// create player info for local player joins
	C4ClientPlayerInfos *pLocalInfo = new C4ClientPlayerInfos(Game.PlayerFilenames);
	// register local info immediately
	pLocalInfo = AddInfo(pLocalInfo);
	// Script players in restore infos need to be associated with matching script players in main info list
	CreateRestoreInfosForJoinedScriptPlayers(Game.RestorePlayerInfos);
	// and assign teams
	if (Game.Teams.IsMultiTeams() && pLocalInfo)
		AssignTeams(pLocalInfo, true);
	// done, success
	return true;
}

bool C4PlayerInfoList::LocalJoinUnjoinedPlayersInQueue()
{
	// local call only - in network, C4Network2Players joins players!
	assert(!::Network.isEnabled());
	// get local players
	C4ClientPlayerInfos **ppkLocal = GetInfoPtrByClientID(::Control.ClientID()), *pkLocal;
	if (!ppkLocal) return false;
	pkLocal = *ppkLocal;
	// check all players
	int32_t i=0; C4PlayerInfo *pInfo;
	while ((pInfo = pkLocal->GetPlayerInfo(i++)))
		// not yet joined?
		if (!pInfo->HasJoinIssued())
		{
			// join will be marked when queue is executed (C4Player::Join)
			// but better mark join now already to prevent permanent sending overkill
			pInfo->SetJoinIssued();
			// join by filename if possible. Script players may not have a filename assigned though
			const char *szFilename = pInfo->GetFilename();
			if (!szFilename && (pInfo->GetType() != C4PT_Script))
			{
				// failure for user players
				const char *szPlrName = pInfo->GetName(); if (!szPlrName) szPlrName="???";
				LogF(LoadResStr("IDS_ERR_JOINQUEUEPLRS"), szPlrName);
				continue;
			}
			Game.Input.Add(CID_JoinPlr,
			               new C4ControlJoinPlayer(szFilename, ::Control.ClientID(), pInfo->GetID()));
		}
	// done, success
	return true;
}

void C4PlayerInfoList::CreateRestoreInfosForJoinedScriptPlayers(C4PlayerInfoList &rSavegamePlayers)
{
	// create matching script player joins for all script playeers in restore info
	// Just copy their infos to the first client
	int32_t i;
	C4ClientPlayerInfos *pHostInfo = GetIndexedInfo(0);
	for (i=0; i<rSavegamePlayers.GetInfoCount(); ++i)
	{
		C4ClientPlayerInfos *pkInfo = rSavegamePlayers.GetIndexedInfo(i);
		int32_t j=0; C4PlayerInfo *pInfo;
		while ((pInfo = pkInfo->GetPlayerInfo(j++)))
			if (pInfo->GetType() == C4PT_Script)
			{
				// safety
				C4PlayerInfo *pRejoinInfo;
				if ((pRejoinInfo = GetPlayerInfoBySavegameID(pInfo->GetID())))
				{
					LogF("Warning: User player %s takes over script player %s!", pRejoinInfo->GetName(), pInfo->GetName());
					continue;
				}
				if (!pHostInfo)
				{
					LogF("Error restoring savegame script players: No host player infos to add to!");
					continue;
				}
				// generate takeover info
				pRejoinInfo = new C4PlayerInfo(*pInfo);
				pRejoinInfo->SetAssociatedSavegamePlayer(pInfo->GetID());
				pHostInfo->AddInfo(pRejoinInfo);
			}
	}
	// teams must recognize the change
	Game.Teams.RecheckPlayers();
}

bool C4PlayerInfoList::RestoreSavegameInfos(C4PlayerInfoList &rSavegamePlayers)
{
	// any un-associated players?
	if (rSavegamePlayers.GetPlayerCount())
	{
		// for runtime network joins, this should never happen!
		assert(!Game.C4S.Head.NetworkRuntimeJoin);

		// do savegame player association of real players
		// for non-lobby games do automatic association first
		int32_t iNumGrabbed = 0, i;
		if (!::Network.isEnabled() && Game.C4S.Head.SaveGame)
		{
			// do several passes: First passes using regular player matching; following passes matching anything but with a warning message
			for (int eMatchingLevel = 0; eMatchingLevel <= PML_Any; ++eMatchingLevel)
				for (int32_t i=0; i<iClientCount; ++i)
				{
					C4ClientPlayerInfos *pkInfo = GetIndexedInfo(i);
					int32_t j=0, id; C4PlayerInfo *pInfo, *pSavegameInfo;
					while ((pInfo = pkInfo->GetPlayerInfo(j++)))
						if (!(id = pInfo->GetAssociatedSavegamePlayerID()))
							if ((pSavegameInfo = rSavegamePlayers.FindSavegameResumePlayerInfo(pInfo, (MatchingLevel)eMatchingLevel, (MatchingLevel)eMatchingLevel)))
							{
								pInfo->SetAssociatedSavegamePlayer(pSavegameInfo->GetID());
								if (eMatchingLevel > PML_PlrName)
								{
									// this is a "wild" match: Warn the player (but not in replays)
									StdStrBuf sMsg; sMsg.Format(LoadResStr("IDS_MSG_PLAYERASSIGNMENT"), pInfo->GetName(), pSavegameInfo->GetName());
									Log(sMsg.getData());
									if (::pGUI && FullScreen.Active && !Game.C4S.Head.Replay)
										::pGUI->ShowMessageModal(sMsg.getData(), LoadResStr("IDS_MSG_FREESAVEGAMEPLRS"), C4GUI::MessageDialog::btnOK, C4GUI::Ico_Notify, &Config.Startup.HideMsgPlrTakeOver);
								}
							}
				}
		}
		// association complete: evaluate it
		for (i=0; i<iClientCount; ++i)
		{
			C4ClientPlayerInfos *pkInfo = GetIndexedInfo(i);
			int32_t j=0, id; C4PlayerInfo *pInfo, *pSavegameInfo;
			while ((pInfo = pkInfo->GetPlayerInfo(j++)))
				if ((id = pInfo->GetAssociatedSavegamePlayerID()))
				{
					if ((pSavegameInfo = rSavegamePlayers.GetPlayerInfoByID(id)))
					{
						// pInfo continues for pSavegameInfo
						pInfo->SetSavegameResume(pSavegameInfo);
						++iNumGrabbed;
					}
					else
					{
						// shouldn't happen
						assert(!"Invalid savegame association");
					}
				}
				else
				{
					// no association for this info: Joins as new player
					// in savegames, this is unusual. For regular script player restore, it's not
					if (Game.C4S.Head.SaveGame) LogF(LoadResStr("IDS_PRC_RESUMENOPLRASSOCIATION"), (const char *)pInfo->GetName());
				}
		}
		// otherwise any remaining players
		int32_t iCountRemaining = rSavegamePlayers.GetPlayerCount() - iNumGrabbed;
		if (iCountRemaining)
		{
			// in replay mode, if there are no regular player joins, it must have been a runtime record
			// i.e., a record that was started during the game
			// in this case, the savegame player infos equal the real player infos to be used
			if (::Control.isReplay() && !GetInfoCount())
			{
				*this = rSavegamePlayers;
			}
			else
			{
				// in regular mode, these players must be removed
				LogF(LoadResStr("IDS_PRC_RESUMEREMOVEPLRS"), iCountRemaining);
				// remove them directly from the game
				RemoveUnassociatedPlayers(rSavegamePlayers);
			}
		}
	}
	// now that players are restored, restore teams
	Game.Teams.RecheckPlayers();
	// done, success
	return true;
}

bool C4PlayerInfoList::RecreatePlayerFiles()
{
	// Note that this method will be called on the main list for savegame resumes (even in network) or regular games with RecreateInfos,
	//  and on RestorePlayerInfos for runtime network joins
	// check all player files that need to be recreated
	for (int32_t i=0; i<iClientCount; ++i)
	{
		C4ClientPlayerInfos *pkInfo = ppClients[i];
		int32_t j=0; C4PlayerInfo *pInfo;
		while ((pInfo = pkInfo->GetPlayerInfo(j++)))
			if (pInfo->IsJoined())
			{
				// all players in replays and runtime joins; script players even in savegames need to be restored from the scenario goup
				if (Game.C4S.Head.Replay || Game.C4S.Head.NetworkRuntimeJoin || pInfo->GetType() == C4PT_Script)
				{
					// in this case, a filename must have been assigned while saving
					// and mark a file inside the scenario file
					// get filename of joined player - this should always be valid!
					const char *szCurrPlrFile;
					StdStrBuf sFilenameInRecord;
					if (Game.C4S.Head.Replay)
					{
						// replay of resumed savegame: RecreatePlayers saves used player files into the record group in this manner
						sFilenameInRecord.Format("Recreate-%d.ocp", pInfo->GetID());
						szCurrPlrFile = sFilenameInRecord.getData();
					}
					else
						szCurrPlrFile = pInfo->GetFilename();
					const char *szPlrName = pInfo->GetName(); if (!szPlrName) szPlrName = "???";
					if (!szCurrPlrFile || !*szCurrPlrFile)
					{
						// that's okay for script players, because those may join w/o recreation files
						if (pInfo->GetType() != C4PT_Script)
						{
							LogF(LoadResStr("IDS_ERR_LOAD_RECR_NOFILE"), szPlrName);
						}
						continue;
					}
					// join from temp file
					StdCopyStrBuf szJoinPath;
					szJoinPath = Config.AtTempPath(GetFilename(szCurrPlrFile));
					// extract player there
					if (!Game.ScenarioFile.FindEntry(GetFilename(szCurrPlrFile)) || !Game.ScenarioFile.Extract(GetFilename(szCurrPlrFile), szJoinPath.getData()))
					{
						// that's okay for script players, because those may join w/o recreation files
						if (pInfo->GetType() != C4PT_Script)
						{
							LogF(LoadResStr("IDS_ERR_LOAD_RECR_NOEXTRACT"), szPlrName, GetFilename(szCurrPlrFile));
						}
						continue;
					}
					// set join source
					pInfo->SetFilename(szJoinPath.getData());
					pInfo->DiscardResource();
					// setting a temp file here will cause the player file to be deleted directly after recreation
					// if recreation fails (e.g. the game gets aborted due to invalid files), the info dtor will delete the file
					pInfo->SetTempFile();
				}
				else
				{
					// regular player in savegame being resumed in network or normal mode:
					// the filenames and/or resources should have been assigned
					// a) either in lobby mode during player re-acquisition
					// b) or when players from rSavegamePlayers were taken over
				}
			}
			else if (!pInfo->HasJoinIssued())
			{
				// new players to be joined into the game:
				// regular control queue join can be done; no special handling needed
			}
	}
	// done, success
	return true;
}

bool C4PlayerInfoList::RecreatePlayers(C4ValueNumbers * numbers)
{
	// check all player infos
	for (int32_t i=0; i<iClientCount; ++i)
	{
		C4ClientPlayerInfos *pkInfo = ppClients[i];
		// skip clients without joined players
		if (!pkInfo->GetJoinedPlayerCount()) continue;
		// determine client ID and name
		// client IDs must be set correctly even in replays,
		// so client-removal packets are executed correctly
		int32_t idAtClient = pkInfo->GetClientID();
		const char *szAtClientName;
		if (Game.C4S.Head.Replay)
			// the client name can currently not really be retrieved in replays
			// but it's not used anyway
			szAtClientName = "Replay";
		else
			// local non-network non-replay games set local name
			if (!::Network.isEnabled())
			{
				assert(idAtClient == ::Control.ClientID());
				szAtClientName = "Local";
			}
			else
			{
				// network non-replay games: find client and set name by it
				const C4Client *pClient = Game.Clients.getClientByID(idAtClient);
				if (pClient)
					szAtClientName = pClient->getName();
				else
				{
					// this shouldn't happen - remove the player info
					LogF(LoadResStr("IDS_PRC_RESUMENOCLIENT"), idAtClient, pkInfo->GetPlayerCount());
					continue;
				}
			}
		// rejoin all joined players of that client
		int32_t j=0; C4PlayerInfo *pInfo;
		while ((pInfo = pkInfo->GetPlayerInfo(j++)))
			if (pInfo->IsJoined())
			{
				// get filename to join from
				const char *szFilename = pInfo->GetLocalJoinFilename();
				// ensure resource is loaded, if joining from resource
				// this may display a waiting dialog and block the thread for a while
				C4Network2Res *pJoinRes = pInfo->GetRes();
				if (szFilename && pJoinRes && pJoinRes->isLoading())
				{
					const char *szName = pInfo->GetName();
					if (!::Network.RetrieveRes(pJoinRes->getCore(), C4NetResRetrieveTimeout,
					                           FormatString(LoadResStr("IDS_NET_RES_PLRFILE"), szName).getData()))
						szFilename=nullptr;
				}
				// file present?
				if (!szFilename || !*szFilename)
				{
					if (pInfo->GetType() == C4PT_User)
					{
						// for user players, this could happen only if the user cancelled the resource
						const char *szPlrName = pInfo->GetName(); if (!szPlrName) szPlrName = "???";
						LogF(LoadResStr("IDS_ERR_LOAD_RECR_NOFILEFROMNET"), szPlrName);
						continue;
					}
					else
					{
						// for script players: Recreation without filename OK
						szFilename = nullptr;
					}
				}
				// record file handling: Save to the record file in the manner it's expected by C4PlayerInfoList::RecreatePlayers
				if (::Control.isRecord() && szFilename)
				{
					StdStrBuf sFilenameInRecord;
					sFilenameInRecord.Format("Recreate-%d.ocp", pInfo->GetID());
					::Control.RecAddFile(szFilename, sFilenameInRecord.getData());
				}
				// recreate join directly
				::Players.Join(szFilename, false, idAtClient, szAtClientName, pInfo, numbers);
				// delete temporary files immediately
				if (pInfo->IsTempFile()) pInfo->DeleteTempFile();
			}
	}
	// done!
	return true;
}

void C4PlayerInfoList::RemoveUnassociatedPlayers(C4PlayerInfoList &rSavegamePlayers)
{
	// check all joined infos
	C4ClientPlayerInfos *pClient; int iClient=0;
	while ((pClient = rSavegamePlayers.GetIndexedInfo(iClient++)))
	{
		C4PlayerInfo *pInfo; int iInfo = 0;
		while ((pInfo = pClient->GetPlayerInfo(iInfo++)))
		{
			// remove players that were in the game but are not associated
			if (pInfo->IsJoined() && !GetPlayerInfoBySavegameID(pInfo->GetID()))
			{
				if (::Players.RemoveUnjoined(pInfo->GetInGameNumber()))
				{
					LogF(LoadResStr("IDS_PRC_REMOVEPLR"), pInfo->GetName());
				}
			}
			pInfo->SetRemoved();
		}
	}
}

bool C4PlayerInfoList::SetAsRestoreInfos(C4PlayerInfoList &rFromPlayers, bool fSaveUserPlrs, bool fSaveScriptPlrs, bool fSetUserPlrRefToLocalGroup, bool fSetScriptPlrRefToLocalGroup)
{
	// copy everything
	*this = rFromPlayers;
	// then remove everything that's no longer joined and update the rest
	C4ClientPlayerInfos *pClient; int iClient=0;
	while ((pClient = GetIndexedInfo(iClient++)))
	{
		// update all players for this client
		C4PlayerInfo *pInfo; int iInfo = 0;
		while ((pInfo = pClient->GetPlayerInfo(iInfo++)))
		{
			bool fKeepInfo = false;
			// remove players that are not in the game
			if (pInfo->IsJoined())
			{
				// pre-reset filename
				pInfo->SetFilename(nullptr);
				if (pInfo->GetType() == C4PT_User)
				{
					fKeepInfo = fSaveUserPlrs;
					if (fSetUserPlrRefToLocalGroup)
					{
						// in the game: Set filename for inside savegame file
						StdStrBuf sNewName;
						if (::Network.isEnabled())
						{
							C4Client *pGameClient = Game.Clients.getClientByID(pClient->GetClientID());
							const char *szName = pGameClient ? pGameClient->getName() : "Unknown";
							sNewName.Format("%s-%s", szName, (const char *) GetFilename(pInfo->GetLocalJoinFilename()));
						}
						else
							sNewName.Copy(GetFilename(pInfo->GetFilename()));

						// O(n) is fast.
						// If not, blame whoever wrote Replace! ;)
						sNewName.Replace("%", "%25", 0);
						for (int ch = 128; ch < 256; ++ch)
						{
							const char* hexChars = "0123456789abcdef";
							char old[] = { char(ch), 0 };
							char safe[] = { '%', 'x', 'x', 0 };
							safe[1] = hexChars[ch / 16];
							safe[2] = hexChars[ch % 16];
							sNewName.Replace(old, safe, 0);
						}

						pInfo->SetFilename(sNewName.getData());
					}
				}
				else if (pInfo->GetType() == C4PT_Script)
				{
					// Save only if either all players should be saved (fSaveScriptPlrs && fSaveUserPlrs)
					//  or if script players are saved and general scenario saving for this script player is desired
					fKeepInfo = fSaveScriptPlrs && (fSaveUserPlrs || pInfo->IsScenarioSaveDesired());
					if (fSetScriptPlrRefToLocalGroup)
					{
						// just compose a unique filename for script player
						pInfo->SetFilename(FormatString("ScriptPlr-%d.ocp", (int)pInfo->GetID()).getData());
					}
				}
			}
			if (!fKeepInfo)
			{
				pClient->RemoveIndexedInfo(--iInfo);
			}
			else
			{
				pInfo->DiscardResource();
			}
		}
		// remove empty clients
		if (!pClient->GetPlayerCount())
		{
			RemoveInfo(GetInfoPtrByClientID(pClient->GetClientID()));
			delete pClient;
			--iClient;
		}
	}
	// done
	return true;
}

void C4PlayerInfoList::ResetLeagueProjectedGain(bool fSetUpdated)
{
	C4ClientPlayerInfos *pClient; int iClient=0;
	while ((pClient = GetIndexedInfo(iClient++)))
	{
		C4PlayerInfo *pInfo; int iInfo = 0;
		while ((pInfo = pClient->GetPlayerInfo(iInfo++)))
			if (pInfo->IsLeagueProjectedGainValid())
			{
				pInfo->ResetLeagueProjectedGain();
				if (fSetUpdated)
					pClient->SetUpdated();
			}
	}
}

void C4PlayerInfoList::CompileFunc(StdCompiler *pComp)
{
	bool deserializing = pComp->isDeserializer();
	if (deserializing) Clear();
	// skip compiling if there is nothing to compile (cosmentics)
	if (!deserializing && pComp->hasNaming() && iLastPlayerID == 0 && iClientCount == 0)
		return;
	// header
	pComp->Value(mkNamingAdapt(iLastPlayerID, "LastPlayerID", 0));
	// client count
	int32_t iTemp = iClientCount;
	pComp->Value(mkNamingCountAdapt<int32_t>(iTemp, "Client"));
	if (iTemp < 0 || iTemp > C4MaxClient)
		{ pComp->excCorrupt("client count out of range"); return; }
	// grow list
	if (deserializing)
	{
		if (iTemp > iClientCapacity) GrowList(iTemp - iClientCapacity);
		iClientCount = iTemp;
		ZeroMem(ppClients, sizeof(*ppClients) * iClientCount);
	}
	// client packets
	pComp->Value(
	  mkNamingAdapt(
	    mkArrayAdaptMap(ppClients, iClientCount, mkPtrAdaptNoNull<C4ClientPlayerInfos>),
	    "Client"));
	// force compiler to specialize
	mkPtrAdaptNoNull<C4ClientPlayerInfos>(*ppClients);
}

int32_t C4PlayerInfoList::GetStartupCount()
{
	// count all joined and to-be-joined
	int32_t iCnt=0;
	for (int32_t i=0; i<iClientCount; ++i)
	{
		int32_t j=0; C4PlayerInfo *pInfo;
		while ((pInfo = ppClients[i]->GetPlayerInfo(j++)))
			if (!pInfo->IsRemoved()) ++iCnt;
	}
	return iCnt;
}

void C4PlayerInfoList::LoadResources()
{
	// load for all players
	int32_t i = iClientCount; C4ClientPlayerInfos **ppClient=ppClients;
	while (i--) (*ppClient++)->LoadResources();
}

void C4PlayerInfoList::FixIDCounter()
{
	// make sure ID counter is same as largest info
	for (int32_t i=0; i<iClientCount; ++i)
	{
		int32_t j=0; C4PlayerInfo *pInfo;
		while ((pInfo = ppClients[i]->GetPlayerInfo(j++)))
		{
			iLastPlayerID = std::max<int32_t>(pInfo->GetID(), iLastPlayerID);
		}
	}
}


/* -- Player info packets -- */

void C4PacketPlayerInfoUpdRequest::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(Info);
}

void C4PacketPlayerInfo::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(fIsRecreationInfo, "Recreation", false));
	pComp->Value(mkNamingAdapt(Info, "Info"));
}
