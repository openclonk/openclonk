/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2009, RedWolf Design GmbH, http://www.clonk.de/
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
// player team management for teamwork melees

#include "C4Include.h"
#include "control/C4Teams.h"

#include "c4group/C4Components.h"
#include "control/C4GameControl.h"
#include "lib/C4Random.h"
#include "player/C4Player.h"
#include "player/C4PlayerList.h"

// ---------------------------------------------------------------
// C4Team

C4Team::C4Team(const C4Team &rCopy)
		: piPlayers(new int32_t[rCopy.GetPlayerCount()]),
		iPlayerCount(rCopy.GetPlayerCount()),
		iPlayerCapacity(rCopy.GetPlayerCount()),
		iID(rCopy.GetID()), iPlrStartIndex(rCopy.iPlrStartIndex), dwClr(rCopy.dwClr),
		sIconSpec(rCopy.GetIconSpec()), iMaxPlayer(rCopy.iMaxPlayer)
{
	// copy name
	SCopy(rCopy.GetName(), Name, C4MaxName);
	// copy players
	for (int32_t i = 0; i < iPlayerCount; i++)
		piPlayers[i] = rCopy.GetIndexedPlayer(i);
}

void C4Team::Clear()
{
	delete [] piPlayers; piPlayers = nullptr;
	iPlayerCount = iPlayerCapacity = iMaxPlayer = 0;
	iID = 0; *Name=0;
	sIconSpec.Clear();
}

void C4Team::AddPlayer(C4PlayerInfo &rInfo, bool fAdjustPlayer)
{
	// must not happen!
	assert(rInfo.GetID());
	if (!rInfo.GetID()) return;
	// add player; grow vector if necessary
	if (iPlayerCount >= iPlayerCapacity)
	{
		int32_t *piNewPlayers = new int32_t[iPlayerCapacity = (iPlayerCount+4)&~3];
		if (iPlayerCount) memcpy(piNewPlayers, piPlayers, iPlayerCount*sizeof(int32_t));
		delete [] piPlayers; piPlayers = piNewPlayers;
	}
	// store new player
	piPlayers[iPlayerCount++] = rInfo.GetID();
	if (!fAdjustPlayer) return;
	// set values in info
	rInfo.SetTeam(GetID());
	if (Game.Teams.IsTeamColors()) rInfo.SetColor(GetColor());
	// and in actual player, if it is joined already
	if (rInfo.IsJoined())
	{
		C4Player *pJoinedPlr = ::Players.GetByInfoID(rInfo.GetID());
		assert(pJoinedPlr || (rInfo.GetType() == C4PT_Script));
		if (pJoinedPlr)
		{
			pJoinedPlr->Team = GetID();
			if (Game.Teams.IsTeamColors()) pJoinedPlr->SetPlayerColor(GetColor());
		}
	}
}

void C4Team::RemoveIndexedPlayer(int32_t iIndex)
{
	// safety
	assert(Inside<int32_t>(iIndex, 0, iPlayerCount-1));
	if (!Inside<int32_t>(iIndex, 0, iPlayerCount-1)) return;
	// move other players done
	for (int32_t i = iIndex+1; i < iPlayerCount; ++i)
		piPlayers[i-1] = piPlayers[i];
	--iPlayerCount;
}

void C4Team::RemovePlayerByID(int32_t iID)
{
	// get index
	int32_t i;
	for (i=0; i < iPlayerCount; ++i)
		if (piPlayers[i] == iID) break;
	if (i == iPlayerCount) { assert(false); return; } // ID not found
	// remove it
	RemoveIndexedPlayer(i);
}

bool C4Team::IsPlayerIDInTeam(int32_t iID)
{
	int32_t i=iPlayerCount, *piPlr = piPlayers;
	while (i--) if (*piPlr++ == iID) return true;
	return false;
}

int32_t C4Team::GetFirstUnjoinedPlayerID() const
{
	// search for a player that does not have the join-flag set
	int32_t i=iPlayerCount, idPlr, *piPlr = piPlayers;
	C4PlayerInfo *pInfo;
	while (i--)
		if ((pInfo = Game.PlayerInfos.GetPlayerInfoByID(idPlr = *piPlr++)))
			if (!pInfo->HasJoinIssued())
				return idPlr;
	// none found
	return 0;
}

void C4Team::CompileFunc(StdCompiler *pComp)
{
	if (pComp->isDeserializer()) Clear();
	pComp->Value(mkNamingAdapt(iID,                  "id",             0));
	pComp->Value(mkNamingAdapt(mkStringAdaptMA(Name), "Name",          ""));
	pComp->Value(mkNamingAdapt(iPlrStartIndex,       "PlrStartIndex",  0));
	pComp->Value(mkNamingAdapt(iPlayerCount,         "PlayerCount", 0));
	if (pComp->isDeserializer()) { delete [] piPlayers; piPlayers = new int32_t [iPlayerCapacity = iPlayerCount]; ZeroMem(piPlayers, sizeof(*piPlayers) * iPlayerCount); }
	pComp->Value(mkNamingAdapt(mkArrayAdapt(piPlayers, iPlayerCount, -1), "Players"));
	pComp->Value(mkNamingAdapt(dwClr,                "Color",        0u));
	pComp->Value(mkNamingAdapt(mkParAdapt(sIconSpec, StdCompiler::RCT_All),
	                                                 "IconSpec",     StdCopyStrBuf()));
	pComp->Value(mkNamingAdapt(iMaxPlayer,           "MaxPlayer",    0));
}

void C4Team::RecheckPlayers()
{
	// check all players within the team
	for (int32_t i=0; i<iPlayerCount; ++i)
	{
		bool fIsValid = false; int32_t id; C4PlayerInfo *pInfo;
		if ((id = piPlayers[i]))
			if ((pInfo = Game.PlayerInfos.GetPlayerInfoByID(id)))
				if (pInfo->GetTeam() == GetID())
					if (pInfo->IsUsingTeam())
						fIsValid = true;
		// removal will decrease iPlayerCount, which will abort the loop earlier
		if (!fIsValid) RemoveIndexedPlayer(i--);
	}
	// now check for any new players in the team
	int32_t id = 0; C4PlayerInfo *pInfo;
	while ((pInfo = Game.PlayerInfos.GetNextPlayerInfoByID(id)))
	{
		id = pInfo->GetID();
		if (pInfo->GetTeam() == GetID())
			if (pInfo->IsUsingTeam())
				if (!IsPlayerIDInTeam(id))
					AddPlayer(*pInfo, false);
	}
}

DWORD GenerateRandomPlayerColor(int32_t iTry); // C4PlayerInfo.cpp
bool IsColorConflict(DWORD dwClr1, DWORD dwClr2); // C4PlayerInfo.cpp

void C4Team::RecheckColor(C4TeamList &rForList)
{
	// number of times trying new player colors
	const int32_t C4MaxTeamColorChangeTries = 100;
	if (!dwClr)
	{
		const int defTeamColorCount = 10;
		DWORD defTeamColorRGB[defTeamColorCount] = { 0xF40000, 0x00C800, 0xFCF41C, 0x2020FF, // red, green, yellow, blue,
		    0xC48444, 0xFFFFFF, 0x848484, 0xFF00EF, // brown, white, grey, pink,
		    0x00FFFF, 0x784830
		                                           }; // cyan, dk brown
		// no color assigned yet: Generate by team ID
		if (iID >=1 && iID <=defTeamColorCount)
		{
			// default colors
			dwClr = defTeamColorRGB[iID-1] | 0xff000000;
		}
		else
		{
			// find a new, unused color
			for (int32_t iTry=1; iTry<C4MaxTeamColorChangeTries; ++iTry)
			{
				dwClr = GenerateRandomPlayerColor(iTry);
				int32_t iIdx=0; C4Team *pTeam; bool fOK=true;
				while ((pTeam = rForList.GetTeamByIndex(iIdx++)))
					if (pTeam != this)
						if (IsColorConflict(pTeam->GetColor(), dwClr))
						{
							fOK=false;
							break;
						}
				// color is fine?
				if (fOK) return;
				// it's not; try next color
			}
			// Giving up: Use last generated color
		}
	}
}

StdStrBuf C4Team::GetNameWithParticipants() const
{
	// compose team name like "Team 1 (boni, GhostBear, Clonko)"
	// or just "Team 1" for empty team
	StdStrBuf sTeamName;
	sTeamName.Copy(GetName());
	if (GetPlayerCount())
	{
		sTeamName.Append(" (");
		int32_t iTeamPlrCount=0;
		for (int32_t j=0; j<GetPlayerCount(); ++j)
		{
			int32_t iPlr = GetIndexedPlayer(j);
			C4PlayerInfo *pPlrInfo;
			if (iPlr) if  ((pPlrInfo = Game.PlayerInfos.GetPlayerInfoByID(iPlr)))
				{
					if (iTeamPlrCount++) sTeamName.Append(", ");
					sTeamName.Append(pPlrInfo->GetName());
				}
		}
		sTeamName.AppendChar(')');
	}
	return sTeamName;
}

bool C4Team::HasWon() const
{
	// return true if any member player of the team has won
	bool fHasWon = false;
	for (int32_t i=0; i<iPlayerCount; ++i)
	{
		int32_t id; C4PlayerInfo *pInfo;
		if ((id = piPlayers[i]))
			if ((pInfo = Game.PlayerInfos.GetPlayerInfoByID(id)))
				if (pInfo->HasWon())
				{
					fHasWon = true;
					break;
				}
	}
	return fHasWon;
}

// ---------------------------------------------------------------
// C4TeamList

void C4TeamList::Clear()
{
	// del all teams
	ClearTeams();
	// del player team vector
	delete [] ppList; ppList = nullptr;
	iTeamCapacity = 0;
	fAllowHostilityChange = true;
	fAllowTeamSwitch = false;
	fCustom = false;
	fActive = true;
	fTeamColors = false;
	eTeamDist = TEAMDIST_Free;
	fAutoGenerateTeams = false;
	iMaxScriptPlayers = 0;
	sScriptPlayerNames.Clear();
}

C4TeamList &C4TeamList::operator =(const C4TeamList &rCopy)
{
	Clear();
	if ((iTeamCount = iTeamCapacity = rCopy.iTeamCount))
		ppList = new C4Team *[iTeamCapacity];
	for (int i = 0; i < iTeamCount; i++)
		ppList[i] = new C4Team(*rCopy.ppList[i]);
	iLastTeamID = rCopy.iLastTeamID;
	fAllowHostilityChange = rCopy.fAllowHostilityChange;
	fAllowTeamSwitch = rCopy.fAllowTeamSwitch;
	fCustom = rCopy.fCustom;
	fActive = rCopy.fActive;
	eTeamDist = rCopy.eTeamDist;
	fTeamColors = rCopy.fTeamColors;
	fAutoGenerateTeams = rCopy.fAutoGenerateTeams;
	sScriptPlayerNames = rCopy.sScriptPlayerNames;
	return *this;
}

bool C4TeamList::CanLocalChooseTeam() const
{
	// only if there are any teams
	if (!fActive) return false;
	// check by mode
	switch (eTeamDist)
	{
	case TEAMDIST_Free: return true;
	case TEAMDIST_Host: return ::Control.isCtrlHost();
	case TEAMDIST_None:
	case TEAMDIST_Random:
	case TEAMDIST_RandomInv:
		return false;
	default: assert(false); return false;
	}
}

bool C4TeamList::CanLocalChooseTeam(int32_t idPlayer) const
{
	// must be possible at all
	if (!CanLocalChooseTeam()) return false;
	// there must be space in a target team
	// always possible if teams are generated on the fly
	if (IsAutoGenerateTeams()) return true;
	// also possible if one of the teams that's not the player's is not full
	C4Team *pCurrentTeam = nullptr, *pCheck;
	if (idPlayer) pCurrentTeam = GetTeamByPlayerID(idPlayer);
	int32_t iCheckTeam=0;
	while ((pCheck = GetTeamByIndex(iCheckTeam++)))
		if (pCheck != pCurrentTeam)
			if (!pCheck->IsFull())
				break;
	return !!pCheck;
}

bool C4TeamList::CanLocalSeeTeam() const
{
	if (!fActive) return false;
	// invisible teams aren't revealed before game start
	if (eTeamDist != TEAMDIST_RandomInv) return true;
	return !!Game.IsRunning;
}

void C4TeamList::AddTeam(C4Team *pNewTeam)
{
	// add team; grow vector if necessary
	if (iTeamCount >= iTeamCapacity)
	{
		// grow up to the nearest multiple of 4 elements
		// (TODO: Replace the whole thing e.g. with a simple std::vector<C4Team>)
		C4Team **ppNewTeams = new C4Team*[(iTeamCapacity = ((iTeamCount+4)&~3))];
		if (iTeamCount) memcpy(ppNewTeams, ppList, iTeamCount*sizeof(C4Team *));
		delete [] ppList; ppList = ppNewTeams;
	}
	// store new team
	ppList[iTeamCount++] = pNewTeam;
	// adjust ID
	iLastTeamID = std::max(pNewTeam->iID, iLastTeamID);
}

void C4TeamList::ClearTeams()
{
	// delete all teams
	C4Team **ppTeam=ppList;
	if (iTeamCount) { while (iTeamCount--) delete *(ppTeam++); iTeamCount = 0; }
	iLastTeamID = 0;
}

C4Team *C4TeamList::CreateTeam(const char *szName)
{
	// custom team
	C4Team *pNewTeam = new C4Team();
	pNewTeam->iID = iLastTeamID + 1;
	SCopy(szName, pNewTeam->Name, C4MaxName);
	AddTeam(pNewTeam);
	pNewTeam->RecheckColor(*this);
	return pNewTeam;
}

bool C4TeamList::GenerateDefaultTeams(int32_t iUpToID)
{
	// generate until last team ID matches given
	while (iLastTeamID < iUpToID)
	{
		char TeamName[C4MaxName+1];
		sprintf(TeamName, LoadResStr("IDS_MSG_TEAM"), iLastTeamID+1);
		if (!CreateTeam(TeamName)) return false;
	}
	return true;
}

C4Team *C4TeamList::GetTeamByID(int32_t iID) const
{
	C4Team **ppCheck=ppList; int32_t iCnt=iTeamCount;
	for (; iCnt--; ++ppCheck) if ((*ppCheck)->GetID() == iID) return *ppCheck;
	return nullptr;
}

C4Team *C4TeamList::GetGenerateTeamByID(int32_t iID)
{
	// only if enabled
	if (!IsMultiTeams()) return nullptr;
	// new team?
	if (iID == TEAMID_New) iID = GetLargestTeamID()+1;
	// find in list
	C4Team *pTeam = GetTeamByID(iID);
	if (pTeam) return pTeam;
	// not found: Generate
	GenerateDefaultTeams(iID);
	return GetTeamByID(iID);
}

C4Team *C4TeamList::GetTeamByIndex(int32_t iIndex) const
{
	// safety
	if (!Inside<int32_t>(iIndex, 0, iTeamCount-1)) return nullptr;
	// direct list access
	return ppList[iIndex];
}

C4Team *C4TeamList::GetTeamByName(const char *szName) const
{
	assert(szName);
	C4Team **ppCheck=ppList; int32_t iCnt=iTeamCount;
	for (; iCnt--; ++ppCheck) if (SEqual((*ppCheck)->GetName(), szName)) return *ppCheck;
	return nullptr;
}

C4Team *C4TeamList::GetTeamByPlayerID(int32_t iID) const
{
	C4Team **ppCheck=ppList; int32_t iCnt=iTeamCount;
	for (; iCnt--; ++ppCheck) if ((*ppCheck)->IsPlayerIDInTeam(iID)) return *ppCheck;
	return nullptr;
}

int32_t C4TeamList::GetLargestTeamID() const
{
	int32_t iLargest = 0;
	C4Team **ppCheck=ppList; int32_t iCnt=iTeamCount;
	for (; iCnt--; ++ppCheck) iLargest = std::max((*ppCheck)->GetID(), iLargest);
	return iLargest;
}

C4Team *C4TeamList::GetRandomSmallestTeam() const
{
	C4Team *pLowestTeam = nullptr; int iLowestTeamCount = 0;
	C4Team **ppCheck=ppList; int32_t iCnt=iTeamCount;
	for (; iCnt--; ++ppCheck)
	{
		if ((*ppCheck)->IsFull()) continue; // do not join into full teams
		if (!pLowestTeam || pLowestTeam->GetPlayerCount() > (*ppCheck)->GetPlayerCount())
		{
			pLowestTeam = *ppCheck;
			iLowestTeamCount = 1;
		}
		else if (pLowestTeam->GetPlayerCount() == (*ppCheck)->GetPlayerCount())
			if (!UnsyncedRandom(++iLowestTeamCount))
				pLowestTeam = *ppCheck;
	}
	return pLowestTeam;
}

bool C4TeamList::IsTeamVisible() const
{
	// teams invisible during lobby time if random surprise teams
	if (eTeamDist == TEAMDIST_RandomInv)
		if (::Network.isLobbyActive())
			return false;
	return true;
}

bool C4TeamList::RecheckPlayerInfoTeams(C4PlayerInfo &rNewJoin, bool fByHost)
{
	// only if enabled
	assert(IsMultiTeams());
	if (!IsMultiTeams()) return false;
	// check whether a new team is to be assigned first
	C4Team *pCurrentTeam = GetTeamByPlayerID(rNewJoin.GetID());
	int32_t idCurrentTeam = pCurrentTeam ? pCurrentTeam->GetID() : 0;
	if (rNewJoin.GetTeam())
	{
		// was that team a change to the current team?
		// no change anyway: OK, skip this info
		if (idCurrentTeam == rNewJoin.GetTeam()) return true;
		// the player had a different team assigned: Check if changes are allowed at all
		if (eTeamDist == TEAMDIST_Free || (eTeamDist == TEAMDIST_Host && fByHost))
			// also make sure that selecting this team is allowed, e.g. doesn't break the team limit
			// this also checks whether the team number is a valid team - but it would accept TEAMID_New, which shouldn't be used in player infos!
			if (rNewJoin.GetTeam() != TEAMID_New && IsJoin2TeamAllowed(rNewJoin.GetTeam(), rNewJoin.GetType()))
				// okay; accept change
				return true;
		// Reject change by reassigning the current team
		rNewJoin.SetTeam(idCurrentTeam);
		// and determine a new team, if none has been assigned yet
		if (idCurrentTeam) return true;
	}
	// new team assignment
	// teams are always needed in the lobby, so there's a team preset to change
	// for runtime joins, teams are needed if specified by teams.txt or if any teams have been created before (to avoid mixed team-noteam-scenarios)
	// but only assign teams in runtime join if the player won't pick it himself
	bool fWillHaveLobby = ::Network.isEnabled() && !::Network.Status.isPastLobby() && Game.fLobby;
	bool fHasOrWillHaveLobby = ::Network.isLobbyActive() || fWillHaveLobby;
	bool fCanPickTeamAtRuntime = !IsRandomTeam() && (rNewJoin.GetType() == C4PT_User) && IsRuntimeJoinTeamChoice();
	bool fIsTeamNeeded = IsRuntimeJoinTeamChoice() || GetTeamCount();
	if (!fHasOrWillHaveLobby && (!fIsTeamNeeded || fCanPickTeamAtRuntime)) return false;
	// get least-used team
	C4Team *pAssignTeam=nullptr;
	C4Team *pLowestTeam = GetRandomSmallestTeam();
	// melee mode
	if (IsAutoGenerateTeams() && !IsRandomTeam())
	{
		// reuse old team only if it's empty
		if (pLowestTeam && !pLowestTeam->GetPlayerCount())
			pAssignTeam = pLowestTeam;
		else
		{
			// no empty team: generate new
			GenerateDefaultTeams(iLastTeamID+1);
			pAssignTeam = GetTeamByID(iLastTeamID);
		}
	}
	else
	{
		if (!pLowestTeam)
		{
			// not enough teams defined in teamwork mode?
			// then create two teams as default
			if (!GetTeamByIndex(1))
				GenerateDefaultTeams(2);
			else
				// otherwise, all defined teams are full. This is a scenario error, because MaxPlayer should have been adjusted
				return false;
			pLowestTeam = GetTeamByIndex(0);
		}
		pAssignTeam = pLowestTeam;
	}
	// assign it
	if (!pAssignTeam) return false;
	pAssignTeam->AddPlayer(rNewJoin, true);
	return true;
}

bool C4TeamList::IsJoin2TeamAllowed(int32_t idTeam, C4PlayerType plrType)
{
	// join to new team: Only if new teams can be created
	if (idTeam == TEAMID_New) return IsAutoGenerateTeams();
	// team number must be valid
	C4Team *pTeam = GetTeamByID(idTeam);
	if (!pTeam) return false;
	// team player count must not exceed the limit, unless it is a script player
	return !pTeam->IsFull() || plrType == C4PT_Script;
}

void C4TeamList::CompileFunc(StdCompiler *pComp)
{
	// if (pComp->isDeserializer()) Clear(); - do not clear, because this would corrupt  the fCustom-flag
	pComp->Value(mkNamingAdapt(fActive,               "Active",               true));
	pComp->Value(mkNamingAdapt(fCustom,               "Custom",               true));
	pComp->Value(mkNamingAdapt(fAllowHostilityChange, "AllowHostilityChange", false));
	pComp->Value(mkNamingAdapt(fAllowTeamSwitch,      "AllowTeamSwitch",      false));
	pComp->Value(mkNamingAdapt(fAutoGenerateTeams,    "AutoGenerateTeams",    false));
	pComp->Value(mkNamingAdapt(iLastTeamID, "LastTeamID", 0));

	StdEnumEntry<TeamDist> TeamDistEntries[] =
	{
		{ "Free", TEAMDIST_Free },
		{ "Host", TEAMDIST_Host },
		{ "None", TEAMDIST_None },
		{ "Random", TEAMDIST_Random },
		{ "RandomInv", TEAMDIST_RandomInv },
	};
	pComp->Value(mkNamingAdapt(mkEnumAdaptT<uint8_t>(eTeamDist, TeamDistEntries), "TeamDistribution", TEAMDIST_Free));

	pComp->Value(mkNamingAdapt(fTeamColors, "TeamColors", false));
	pComp->Value(mkNamingAdapt(iMaxScriptPlayers, "MaxScriptPlayers", 0));
	pComp->Value(mkNamingAdapt(mkParAdapt(sScriptPlayerNames, StdCompiler::RCT_All), "ScriptPlayerNames", StdStrBuf()));

	int32_t iOldTeamCount = iTeamCount;
	pComp->Value(mkNamingCountAdapt(iTeamCount,  "Team"));

	if (pComp->isDeserializer())
	{
		while (iOldTeamCount--) delete ppList[iOldTeamCount];
		delete [] ppList;
		if ((iTeamCapacity = iTeamCount))
		{
			ppList = new C4Team *[iTeamCapacity];
			memset(ppList, 0, sizeof(C4Team *)*iTeamCapacity);
		}
		else
			ppList = nullptr;
	}

	if (iTeamCount)
	{
		// Force compiler to spezialize
		mkPtrAdaptNoNull(*ppList);
		// Save team list, using map-function.
		pComp->Value(mkNamingAdapt(
		               mkArrayAdaptMap(ppList, iTeamCount, mkPtrAdaptNoNull<C4Team>),
		               "Team"));
	}

	if (pComp->isDeserializer())
	{
		// adjust last team ID, which may not be set properly for player-generated team files
		iLastTeamID = std::max(GetLargestTeamID(), iLastTeamID);
		// force automatic generation of teams if none are defined
		if (!iTeamCount) fAutoGenerateTeams = true;
	}
}

bool C4TeamList::Load(C4Group &hGroup, class C4Scenario *pInitDefault, class C4LangStringTable *pLang)
{
	// clear previous
	Clear();
	// load file contents
	StdStrBuf Buf;
	if (!hGroup.LoadEntryString(C4CFN_Teams, &Buf))
	{
		// no teams: Try default init
		if (!pInitDefault) return false;
		// no teams defined: Activate default melee teams if a melee rule is found
		// default: FFA for anything that looks like melee
		if ( pInitDefault->Game.IsMelee())
		{
			fAllowHostilityChange = true;
			fActive = true;
			fAutoGenerateTeams = true;
		}
		else
		{
			// No goals/rules whatsoever: They could be present in the objects.txt, but parsing that would be a bit of
			//  overkill
			// So just keep the old behaviour here, and disallow teams
			fAllowHostilityChange = true;
			fActive = false;
		}
		fCustom = false;
	}
	else
	{
		// team definition file may be localized
		if (pLang) pLang->ReplaceStrings(Buf);
		// compile
		if (!CompileFromBuf_LogWarn<StdCompilerINIRead>(mkNamingAdapt(*this, "Teams"), Buf, C4CFN_Teams)) return false;
	}
	// post-initialization: Generate default team colors
	int32_t iTeam=0; C4Team *pTeam;
	while ((pTeam = GetTeamByIndex(iTeam++)))
		pTeam->RecheckColor(*this);
	return true;
}

bool C4TeamList::Save(C4Group &hGroup)
{
	// remove previous entry from group
	hGroup.DeleteEntry(C4CFN_Teams);
	// decompile
	try
	{
		StdStrBuf Buf = DecompileToBuf<StdCompilerINIWrite>(mkNamingAdapt(*this, "Teams"));
		// save it
		hGroup.Add(C4CFN_Teams, Buf, false, true);
	}
	catch (StdCompiler::Exception *)
		{ return false; }
	// done, success
	return true;
}

void C4TeamList::RecheckPlayers()
{
	C4Team **ppCheck=ppList; int32_t iCnt=iTeamCount;
	for (; iCnt--; ++ppCheck) (*ppCheck)->RecheckPlayers();
}

void C4TeamList::RecheckTeams()
{
	// automatic team distributions only
	if (!IsRandomTeam()) return;
	// host decides random teams
	if (!::Control.isCtrlHost())
	{
		// Still make sure that we have the right number of teams on the clients as well so that
		// GetTeamCount() does not report inconsistent values.
		EnsureTeamCount();
		return;
	}
	// random teams in auto generate mode? Make sure there are exactly two teams
	if (IsAutoGenerateTeams() && GetTeamCount() != 2)
		{
		ReassignAllTeams();
		return;
		}
	// redistribute players of largest team that has relocatable players left towards smaller teams
	for (;;)
	{
		C4Team *pLowestTeam = GetRandomSmallestTeam();
		if (!pLowestTeam) break; // no teams: Nothing to re-distribute.
		// get largest team that has relocateable players
		C4Team *pLargestTeam = nullptr;
		C4Team **ppCheck=ppList; int32_t iCnt=iTeamCount;
		for (; iCnt--; ++ppCheck) if (!pLargestTeam || pLargestTeam->GetPlayerCount() > (*ppCheck)->GetPlayerCount())
				if ((*ppCheck)->GetFirstUnjoinedPlayerID())
					pLargestTeam = *ppCheck;
		// no team can redistribute?
		if (!pLargestTeam) break;
		// redistribution won't help much?
		if (pLargestTeam->GetPlayerCount() - pLowestTeam->GetPlayerCount() <= 1) break;
		// okay; redistribute one player!
		int32_t idRedistPlayer = pLargestTeam->GetFirstUnjoinedPlayerID();
		C4PlayerInfo *pInfo = Game.PlayerInfos.GetPlayerInfoByID(idRedistPlayer);
		assert(pInfo);
		if (!pInfo) break; // umn...serious problems
		pLargestTeam->RemovePlayerByID(idRedistPlayer);
		pLowestTeam->AddPlayer(*pInfo, true);
		C4ClientPlayerInfos *pClrInfo = Game.PlayerInfos.GetClientInfoByPlayerID(idRedistPlayer);
		assert(pClrInfo);
		// player info change: mark updated to remote clients get information
		if (pClrInfo)
		{
			pClrInfo->SetUpdated();
		}
	}
}

void C4TeamList::EnsureTeamCount()
{
	// in random autogenerate mode, there must be exactly two teams
	if (IsRandomTeam())
	{
		if (IsAutoGenerateTeams() && GetTeamCount() != 2)
		{
			ClearTeams();
			GenerateDefaultTeams(2);
		}
	}
}

void C4TeamList::ReassignAllTeams()
{
	assert(::Control.isCtrlHost());
	if (!::Control.isCtrlHost()) return;
	// go through all player infos; reset team in them
	int32_t idStart = -1; C4PlayerInfo *pNfo;
	while ((pNfo = Game.PlayerInfos.GetNextPlayerInfoByID(idStart)))
	{
		idStart = pNfo->GetID();
		if (pNfo->HasJoinIssued()) continue;
		pNfo->SetTeam(0);
		// mark changed info as updated
		C4ClientPlayerInfos *pCltInfo = Game.PlayerInfos.GetClientInfoByPlayerID(idStart);
		assert(pCltInfo);
		if (pCltInfo)
		{
			pCltInfo->SetUpdated();
		}
	}
	// clear players from team lists
	RecheckPlayers();
	EnsureTeamCount();
	// reassign them
	idStart = -1;
	while ((pNfo = Game.PlayerInfos.GetNextPlayerInfoByID(idStart)))
	{
		idStart = pNfo->GetID();
		if (pNfo->HasJoinIssued()) continue;
		assert(!pNfo->GetTeam());
		RecheckPlayerInfoTeams(*pNfo, true);
	}
}

StdStrBuf C4TeamList::GetTeamDistName(TeamDist eTeamDist) const
{
	switch (eTeamDist)
	{
	case TEAMDIST_Free:      return(StdStrBuf(LoadResStr("IDS_MSG_TEAMDIST_FREE"), true));
	case TEAMDIST_Host:      return(StdStrBuf(LoadResStr("IDS_MSG_TEAMDIST_HOST"), true));
	case TEAMDIST_None:      return(StdStrBuf(LoadResStr("IDS_MSG_TEAMDIST_NONE"), true));
	case TEAMDIST_Random:    return(StdStrBuf(LoadResStr("IDS_MSG_TEAMDIST_RND"), true));
	case TEAMDIST_RandomInv: return(StdStrBuf(LoadResStr("IDS_MSG_TEAMDIST_RNDINV"), true));
	default: return(FormatString("TEAMDIST_undefined(%d)", (int) eTeamDist));
	}
}

void C4TeamList::FillTeamDistOptions(C4GUI::ComboBox_FillCB *pFiller) const
{
	// no teams if disabled
	if (!fActive) return;
	// team distribution options
	pFiller->AddEntry(GetTeamDistName(TEAMDIST_Free).getData(), TEAMDIST_Free);
	pFiller->AddEntry(GetTeamDistName(TEAMDIST_Host).getData(), TEAMDIST_Host);
	if (IsAutoGenerateTeams()) pFiller->AddEntry(GetTeamDistName(TEAMDIST_None).getData(), TEAMDIST_None); // no teams: only for regular melees
	pFiller->AddEntry(GetTeamDistName(TEAMDIST_Random).getData(), TEAMDIST_Random);
	pFiller->AddEntry(GetTeamDistName(TEAMDIST_RandomInv).getData(), TEAMDIST_RandomInv);
	}

void C4TeamList::SendSetTeamDist(TeamDist eNewTeamDist)
{
	assert(::Control.isCtrlHost());
	// set it for all clients
	::Control.DoInput(CID_Set, new C4ControlSet(C4CVT_TeamDistribution, eNewTeamDist), CDT_Sync);
}

StdStrBuf C4TeamList::GetTeamDistString() const
{
	// return name of current team distribution setting
	return GetTeamDistName(eTeamDist);
}

bool C4TeamList::HasTeamDistOptions() const
{
	// team distribution can be changed if teams are enabled
	return fActive;
}

void C4TeamList::SetTeamDistribution(TeamDist eToVal)
{
	if (!Inside(eToVal, TEAMDIST_First, TEAMDIST_Last)) { assert(false); return; }
	eTeamDist = eToVal;
	// team distribution mode changed: Host may beed to redistribute
	if (::Control.isCtrlHost())
	{
		// if a random team mode was set, reassign all teams so it's really random.
		// Also reassign in no-team-mode so enough teams for all players exist
		if (IsRandomTeam() || eTeamDist==TEAMDIST_None)
			ReassignAllTeams();
		else
		{
			// otherwise, it's sufficient to just reassign any teams that are incorrect for the current mode
			RecheckTeams();
		}
		// send updates to other clients and reset flags
		if (::Network.isEnabled())
		{
			::Network.Players.SendUpdatedPlayers();
		}
	}
}

void C4TeamList::SendSetTeamColors(bool fEnabled)
{
	// set it for all clients
	::Control.DoInput(CID_Set, new C4ControlSet(C4CVT_TeamColors, fEnabled), CDT_Sync);
}

void C4TeamList::SetTeamColors(bool fEnabled)
{
	// change only
	if (fEnabled == fTeamColors) return;
	// reflect change
	fTeamColors = fEnabled;
	// update colors of all players
	if (!::Control.isCtrlHost()) return;
	// go through all player infos; reset color in them
	Game.PlayerInfos.UpdatePlayerAttributes(); // sets team and savegame colors
	if (::Network.isEnabled())
	{
		// sends color updates to all clients
		::Network.Players.SendUpdatedPlayers();
	}
}

void C4TeamList::EnforceLeagueRules()
{
	// enforce some league settings
	// allow temp hostility switching; often used e.g. to unstick friendly Clonks, but:
	fAllowTeamSwitch = false; // switching teams in league games? Yeah, sure...
}

int32_t C4TeamList::GetForcedTeamSelection(int32_t idForPlayer) const
{
	// if there's only one team for the player to join, return that team ID
	C4Team *pOKTeam = nullptr, *pCheck;
	if (idForPlayer) pOKTeam = GetTeamByPlayerID(idForPlayer); // curent team is always possible, even if full
	int32_t iCheckTeam=0;
	while ((pCheck = GetTeamByIndex(iCheckTeam++)))
		if (!pCheck->IsFull())
		{
			// this team could be joined
			if (pOKTeam && pOKTeam != pCheck)
			{
				// there already was a team that could be joined
				// two alternatives -> team selection is not forced
				return 0;
			}
			pOKTeam = pCheck;
		}
	// was there a team that could be joined?
	if (pOKTeam)
	{
		// if teams are generated on the fly, there would always be the possibility of creating a new team    }
		if (IsAutoGenerateTeams()) return 0;
		// otherwise, this team is forced!
		return pOKTeam->GetID();
	}
	// no team could be joined: Teams auto generated?
	if (IsAutoGenerateTeams())
	{
		// then the only possible way is to join a new team
		return TEAMID_New;
	}
	// otherwise, nothing can be done...
	return 0;
}

StdStrBuf C4TeamList::GetScriptPlayerName() const
{
	// get a name to assign to a new script player. Try to avoid name conflicts
	if (!sScriptPlayerNames.getLength()) return StdStrBuf(LoadResStr("IDS_TEXT_COMPUTER")); // default name
	// test available script names
	int32_t iNameIdx = 0; StdStrBuf sOut;
	while (sScriptPlayerNames.GetSection(iNameIdx++, &sOut, '|'))
		if (!Game.PlayerInfos.GetActivePlayerInfoByName(sOut.getData()))
			return sOut;
	// none are available: Return a random name
	sScriptPlayerNames.GetSection(UnsyncedRandom(iNameIdx-1), &sOut, '|');
	return sOut;
}

int32_t C4TeamList::GetStartupTeamCount(int32_t startup_player_count)
{
	// Count non-empty teams
	int32_t i_team = 0; C4Team *team;
	int32_t team_count = 0;
	while ((team = GetTeamByIndex(i_team++)))
	{
		if (team->GetPlayerCount() > 0) ++team_count;
	}
	// No populated teams found? This can happen in non-network mode when no players are assigned
	if (!team_count)
	{
		// Teams have not been selected yet, but the map script may want to have an estimate
		// in this case, calculate prospective teams from startup player count
		if (IsCustom() && !IsAutoGenerateTeams())
		{
			// Teams are pre-defined. Assume players will try to distribute broadly on these teams
			team_count = std::min<int32_t>(startup_player_count, GetTeamCount());
		}
		else if (IsRandomTeam())
		{
			// Randomized teams: Players will be put into two teams.
			team_count = std::min<int32_t>(startup_player_count, 2);
		}
		else
		{
			// Teams are auto-added -> fallback to player count
			team_count = startup_player_count;
		}
	}

	return team_count;
}
