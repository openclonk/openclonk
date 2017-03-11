/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2008-2009, RedWolf Design GmbH, http://www.clonk.de/
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
// Round result information to be displayed in game over dialog

#include "C4Include.h"
#include "control/C4RoundResults.h"

#include "player/C4Player.h"
#include "game/C4Game.h"
#include "object/C4Def.h"
#include "object/C4Object.h"
#include "player/C4PlayerList.h"
#include "object/C4GameObjects.h"
#include "object/C4DefList.h"

// *** C4RoundResultsPlayer

void C4RoundResultsPlayer::CompileFunc(StdCompiler *pComp)
{
	// remember to adjust operator = and == when adding values here!
	pComp->Value(mkNamingAdapt(id,    "ID",   0));
	// pComp->Value(mkNamingAdapt(fctBigIcon,    "Icon",      C4TargetFacet())); - not possible
	pComp->Value(mkNamingAdapt(iTotalPlayingTime,  "TotalPlayingTime",    0u));
	pComp->Value(mkNamingAdapt(iScoreOld,  "SettlementScoreOld",    -1));
	pComp->Value(mkNamingAdapt(iScoreNew,  "SettlementScoreNew",    -1));
	pComp->Value(mkNamingAdapt(iLeagueScoreNew,  "Score",    -1));          // name used in league reply!
	pComp->Value(mkNamingAdapt(iLeagueScoreGain,  "GameScore",    -1));     // name used in league reply!
	pComp->Value(mkNamingAdapt(iLeagueRankNew,  "Rank",    0));             // name used in league reply!
	pComp->Value(mkNamingAdapt(iLeagueRankSymbolNew,  "RankSymbol",    0)); // name used in league reply!
	pComp->Value(mkNamingAdapt(sLeagueProgressData,  "LeagueProgressData",    StdCopyStrBuf()));
	StdEnumEntry<LeagueStatus> LeagueStatusEntries[] =
	{
		{ "",     RRPLS_Unknown },
		{ "Lost", RRPLS_Lost    },
		{ "Won",  RRPLS_Won     },
	};
	pComp->Value(mkNamingAdapt(mkEnumAdaptT<uint8_t>(eLeagueStatus, LeagueStatusEntries),  "Status",    RRPLS_Unknown));            // name used in league reply!
}

void C4RoundResultsPlayer::EvaluatePlayer(C4Player *pPlr)
{
	assert(pPlr);
	// set fields by player
	iTotalPlayingTime = pPlr->TotalPlayingTime;
	if (pPlr->Evaluated)
	{
		iScoreNew = pPlr->TotalScore;
		iScoreOld = iScoreNew - pPlr->LastRound.FinalScore;
	}
	else
	{
		// player not evaluated (e.g., removed by disconnect): Old score known only
		iScoreOld = pPlr->TotalScore;
	}
	// load icon from player
	fctBigIcon.Clear();
	if (pPlr->BigIcon.Surface)
	{
		fctBigIcon.Create(pPlr->BigIcon.Wdt, pPlr->BigIcon.Hgt);
		pPlr->BigIcon.Draw(fctBigIcon);
	}
	// progress data by player
	C4PlayerInfo *pInfo = pPlr->GetInfo();
	if (pInfo)
		{
		sLeagueProgressData.Copy(pInfo->GetLeagueProgressData());
		}
}

void C4RoundResultsPlayer::EvaluateLeague(C4RoundResultsPlayer *pLeaguePlayerInfo)
{
	assert(pLeaguePlayerInfo);
	//
	// copy league info
	iLeagueScoreNew = pLeaguePlayerInfo->iLeagueScoreNew;
	iLeagueScoreGain = pLeaguePlayerInfo->iLeagueScoreGain;
	iLeagueRankNew = pLeaguePlayerInfo->iLeagueRankNew;
	iLeagueRankSymbolNew = pLeaguePlayerInfo->iLeagueRankSymbolNew;
	sLeagueProgressData =pLeaguePlayerInfo->sLeagueProgressData;
}

void C4RoundResultsPlayer::AddCustomEvaluationString(const char *szCustomString)
{
	if (sCustomEvaluationStrings.getLength()) sCustomEvaluationStrings.Append("   ");
	sCustomEvaluationStrings.Append(szCustomString);
}

bool C4RoundResultsPlayer::operator ==(const C4RoundResultsPlayer &cmp)
{
	// cmp all xcept icon
	if (id != cmp.id) return false;
	if (iTotalPlayingTime != cmp.iTotalPlayingTime) return false;
	if (iScoreOld != cmp.iScoreOld) return false;
	if (iScoreNew != cmp.iScoreNew) return false;
	if (sCustomEvaluationStrings != cmp.sCustomEvaluationStrings) return false;
	if (iLeagueScoreNew != cmp.iLeagueScoreNew) return false;
	if (iLeagueScoreGain != cmp.iLeagueScoreGain) return false;
	if (iLeagueRankNew != cmp.iLeagueRankNew) return false;
	if (iLeagueRankSymbolNew != cmp.iLeagueRankSymbolNew) return false;
	if (eLeagueStatus != cmp.eLeagueStatus) return false;
	if (sLeagueProgressData != cmp.sLeagueProgressData) return false;
	return true;
}

C4RoundResultsPlayer &C4RoundResultsPlayer::operator =(const C4RoundResultsPlayer &cpy)
{
	if (this == &cpy) return *this;
	// assign all xcept icon
	id = cpy.id;
	iTotalPlayingTime = cpy.iTotalPlayingTime;
	iScoreOld = cpy.iScoreOld;
	iScoreNew = cpy.iScoreNew;
	sCustomEvaluationStrings = cpy.sCustomEvaluationStrings;
	iLeagueScoreNew = cpy.iLeagueScoreNew;
	iLeagueScoreGain = cpy.iLeagueScoreGain;
	iLeagueRankNew = cpy.iLeagueRankNew;
	iLeagueRankSymbolNew = cpy.iLeagueRankSymbolNew;
	eLeagueStatus = cpy.eLeagueStatus;
	sLeagueProgressData = cpy.sLeagueProgressData;
	return *this;
}


// *** C4RoundResultsPlayers

void C4RoundResultsPlayers::Clear()
{
	while (iPlayerCount) delete ppPlayers[--iPlayerCount];
	delete [] ppPlayers;
	ppPlayers = nullptr;
	iPlayerCapacity = 0;
}

void C4RoundResultsPlayers::CompileFunc(StdCompiler *pComp)
{
	bool deserializing = pComp->isDeserializer();
	if (deserializing) Clear();
	int32_t iTemp = iPlayerCount;
	pComp->Value(mkNamingCountAdapt<int32_t>(iTemp, "Player"));
	if (iTemp < 0 || iTemp > C4MaxPlayer)
		{ pComp->excCorrupt("player count out of range"); return; }
	// Grow list, if necessary
	if (deserializing && iTemp > iPlayerCapacity)
	{
		GrowList(iTemp - iPlayerCapacity);
		iPlayerCount = iTemp;
		ZeroMem(ppPlayers, sizeof(*ppPlayers) * iPlayerCount);
	}
	// Compile
	pComp->Value(mkNamingAdapt(mkArrayAdaptMap(ppPlayers, iPlayerCount, mkPtrAdaptNoNull<C4RoundResultsPlayer>), "Player"));
	// Force specialization
	mkPtrAdaptNoNull<C4RoundResultsPlayer>(*ppPlayers);
}

C4RoundResultsPlayer *C4RoundResultsPlayers::GetByIndex(int32_t idx) const
{
	if (idx>=0 && idx<iPlayerCount)
		return ppPlayers[idx];
	else
		return nullptr;
}

C4RoundResultsPlayer *C4RoundResultsPlayers::GetByID(int32_t id) const
{
	for (int32_t idx=0; idx<iPlayerCount; ++idx)
		if (ppPlayers[idx]->GetID() == id)
			return ppPlayers[idx];
	return nullptr;
}

void C4RoundResultsPlayers::GrowList(size_t iByVal)
{
	// create new list (out of mem: simply returns here; info list remains in a valid state)
	C4RoundResultsPlayer **ppNew = new C4RoundResultsPlayer *[iPlayerCapacity += iByVal];
	// move existing
	if (ppPlayers)
	{
		memcpy(ppNew, ppPlayers, iPlayerCount * sizeof(C4RoundResultsPlayer *));
		delete [] ppPlayers;
	}
	// assign new
	ppPlayers = ppNew;
}

void C4RoundResultsPlayers::Add(C4RoundResultsPlayer *pNewPlayer)
{
	assert(pNewPlayer);
	if (iPlayerCount == iPlayerCapacity) GrowList(4);
	ppPlayers[iPlayerCount++] = pNewPlayer;
}

C4RoundResultsPlayer *C4RoundResultsPlayers::GetCreateByID(int32_t id)
{
	assert(id);
	// find existing
	C4RoundResultsPlayer *pPlr = GetByID(id);
	// not found: Add new
	if (!pPlr)
	{
		pPlr = new C4RoundResultsPlayer();
		pPlr->SetID(id);
		Add(pPlr);
	}
	return pPlr;
}

bool C4RoundResultsPlayers::operator ==(const C4RoundResultsPlayers &cmp)
{
	if (iPlayerCount != cmp.iPlayerCount) return false;
	for (int32_t i = 0; i < iPlayerCount; ++i)
		if (!(*ppPlayers[i] == *cmp.ppPlayers[i]))
			return false;
	// equal
	return true;
}

C4RoundResultsPlayers &C4RoundResultsPlayers::operator =(const C4RoundResultsPlayers &cpy)
{
	Clear();
	C4RoundResultsPlayer *pPlr; int32_t i=0;
	while ((pPlr = cpy.GetByIndex(i++)))
		Add(new C4RoundResultsPlayer(*pPlr));
	return *this;
}


// *** C4RoundResults

void C4RoundResults::Clear()
{
	Players.Clear();
	Goals.Clear();
	iPlayingTime = 0;
	sCustomEvaluationStrings.Clear();
	iLeaguePerformance = 0;
	sNetResult.Clear();
	eNetResult = NR_None;
	fHideSettlementScore=false;
	Statistics.Clear();
}

void C4RoundResults::Init()
{
	if (Game.C4S.Game.IsMelee())
		fHideSettlementScore=true;
	else fHideSettlementScore=false;
}

void C4RoundResults::CompileFunc(StdCompiler *pComp)
{
	bool deserializing = pComp->isDeserializer();
	if (deserializing) Clear();
	pComp->Value(mkNamingAdapt(Goals, "Goals", C4IDList()));
	pComp->Value(mkNamingAdapt(iPlayingTime, "PlayingTime", 0u));
	pComp->Value(mkNamingAdapt(fHideSettlementScore, "HideSettlementScore", Game.C4S.Game.IsMelee()));
	pComp->Value(mkNamingAdapt(sCustomEvaluationStrings, "CustomEvaluationStrings", StdCopyStrBuf()));
	pComp->Value(mkNamingAdapt(iLeaguePerformance, "LeaguePerformance", 0));
	pComp->Value(mkNamingAdapt(Players, "PlayerInfos", C4RoundResultsPlayers()));
	pComp->Value(mkNamingAdapt(sNetResult, "NetResult", StdCopyStrBuf()));
	StdEnumEntry<NetResult> NetResultEntries[] =
	{
		{ "",            NR_None       },
		{ "LeagueOK",    NR_LeagueOK   },
		{ "LeagueError", NR_LeagueError},
		{ "NetError",    NR_NetError   },
	};
	pComp->Value(mkNamingAdapt(mkEnumAdaptT<uint8_t>(eNetResult, NetResultEntries), "NetResult", NR_None));
}

void C4RoundResults::EvaluateGoals(C4IDList &GoalList, C4IDList &FulfilledGoalList, int32_t iPlayerNumber)
{
	// clear prev
	GoalList.Clear(); FulfilledGoalList.Clear();
	// Items
	int32_t cnt; C4ID idGoal;
	for (cnt=0; (idGoal=::Objects.GetListID(C4D_Goal,cnt)); cnt++)
	{
		// determine if the goal is fulfilled - do the calls even if the menu is not to be opened to ensure synchronization
		bool fFulfilled = false;;
		C4Object *pObj = C4Id2Def(idGoal) ? ::Objects.Find(::Definitions.ID2Def(idGoal)) : nullptr;
		if (pObj)
		{
			// Check fulfilled per player, this enables the possibility of rivalry.
			C4AulParSet pars(iPlayerNumber);
			fFulfilled = !!pObj->Call(PSF_IsFulfilled, &pars);
		}
		GoalList.SetIDCount(idGoal, cnt, true);
		if (fFulfilled) FulfilledGoalList.SetIDCount(idGoal, 1, true);
	}
}

void C4RoundResults::EvaluateGame()
{
	// set game data
	C4Player *pFirstLocalPlayer = ::Players.GetLocalByIndex(0);
	int32_t iFirstLocalPlayer = pFirstLocalPlayer ? pFirstLocalPlayer->Number : NO_OWNER;
	EvaluateGoals(Goals, FulfilledGoals, iFirstLocalPlayer);
	iPlayingTime = Game.Time;

	// collect statistics
	try
	{
		C4AulParSet Pars;
		auto stats = ::ScriptEngine.GetPropList()->Call(PSF_CollectStatistics, &Pars);
		if (stats != C4VNull)
			Statistics = stats.ToJSON();
	}
	catch (C4JSONSerializationError& e)
	{
		DebugLogF("ERROR: cannot serialize CollectStatistics() result: %s", e.what());
	}
}

void C4RoundResults::EvaluateNetwork(C4RoundResults::NetResult eNetResult, const char *szResultMsg)
{
	// take result only if there was no previous result (the previous one is usually more specific)
	if (!HasNetResult())
	{
		this->eNetResult = eNetResult;
		if (szResultMsg) sNetResult.Copy(szResultMsg); else sNetResult.Clear();
	}
}

void C4RoundResults::EvaluateLeague(const char *szResultMsg, bool fSuccess, const C4RoundResultsPlayers &rLeagueInfo)
{
	// League evaluation imples network evaluation
	Game.RoundResults.EvaluateNetwork(fSuccess ? C4RoundResults::NR_LeagueOK : C4RoundResults::NR_LeagueError, szResultMsg);
	// Evaluation called by league: Sets new league scores and ranks
	C4RoundResultsPlayer *pPlr, *pOwnPlr; int32_t i = 0;
	while ((pPlr = rLeagueInfo.GetByIndex(i++)))
	{
		pOwnPlr = Players.GetCreateByID(pPlr->GetID());
		pOwnPlr->EvaluateLeague(pPlr);
	}
}

void C4RoundResults::EvaluatePlayer(C4Player *pPlr)
{
	// Evaluation called by player when it's evaluated
	assert(pPlr);
	C4RoundResultsPlayer *pOwnPlr = Players.GetCreateByID(pPlr->ID);
	pOwnPlr->EvaluatePlayer(pPlr);
}

void C4RoundResults::AddCustomEvaluationString(const char *szCustomString, int32_t idPlayer)
{
	// Set custom string to be shown in game over dialog
	// idPlayer==0 for global strings
	if (!idPlayer)
	{
		if (sCustomEvaluationStrings.getLength()) sCustomEvaluationStrings.AppendChar('|');
		sCustomEvaluationStrings.Append(szCustomString);
	}
	else
	{
		C4RoundResultsPlayer *pOwnPlr = Players.GetCreateByID(idPlayer);
		pOwnPlr->AddCustomEvaluationString(szCustomString);
	}
}

void C4RoundResults::HideSettlementScore(bool fHide)
{
	fHideSettlementScore=fHide;
}

bool C4RoundResults::SettlementScoreIsHidden()
{
	return fHideSettlementScore;
}

void C4RoundResults::SetLeaguePerformance(int32_t iNewPerf, int32_t idPlayer)
{
	// Store to be sent later. idPlayer == 0 means global performance.
	if(!idPlayer)
		{
		iLeaguePerformance = iNewPerf;
		}
	else
		{
		C4RoundResultsPlayer *pOwnPlr = Players.GetCreateByID(idPlayer);
		pOwnPlr->SetLeaguePerformance(iNewPerf);
		}
	}

int32_t C4RoundResults::GetLeaguePerformance(int32_t idPlayer) const
{
	if(!idPlayer)
		return iLeaguePerformance;
	else
		if(C4RoundResultsPlayer *pPlr = Players.GetByID(idPlayer))
			return pPlr->GetLeaguePerformance();
	return 0;
}

bool C4RoundResults::Load(C4Group &hGroup, const char *szFilename)
{
	// clear previous
	Clear();
	// load file contents
	StdStrBuf Buf;
	if (!hGroup.LoadEntryString(szFilename, &Buf)) return false;
	// compile
	if (!CompileFromBuf_LogWarn<StdCompilerINIRead>(mkNamingAdapt(*this, "RoundResults"), Buf, szFilename)) return false;
	// done, success
	return true;
}

bool C4RoundResults::Save(C4Group &hGroup, const char *szFilename)
{
	// remove previous entry from group
	hGroup.DeleteEntry(szFilename);
	// decompile
	try
	{
		StdStrBuf Buf = DecompileToBuf<StdCompilerINIWrite>(mkNamingAdapt(*this, "RoundResults"));
		// save it, if not empty
		if (Buf.getLength())
			if (!hGroup.Add(szFilename, Buf, false, true))
				return false;
	}
	catch (StdCompiler::Exception *)
		{ return false; }
	// done, success
	return true;
}


// *** C4PacketLeagueRoundResults

void C4PacketLeagueRoundResults::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(fSuccess, "Success", false));
	pComp->Value(mkNamingAdapt(sResultsString, "ResultString", StdCopyStrBuf()));
	pComp->Value(Players);
}

