/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
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

/* Structures for object and player info components */

#ifndef INC_C4InfoCore
#define INC_C4InfoCore

#include "lib/C4InputValidation.h"
#include "object/C4Id.h"
#include "player/C4ScenarioParameters.h"
#include "script/C4ValueMap.h"

const int32_t C4MaxPhysical = 100000,
                              C4MaxDeathMsg = 75;

class C4ObjectInfoCore
{
public:
	C4ObjectInfoCore();
public:
	C4ID id;
	char Name[C4MaxName+1];
	int32_t  Participation;
	int32_t  Rank;
	StdStrBuf sRankName;
	StdStrBuf sNextRankName;
	int32_t NextRankExp; // EXP_NoPromotion for no more promotion; 0 if standard rank system is used
	int32_t  Experience,Rounds;
	int32_t  DeathCount;
	char TypeName[C4MaxName+1+1];
	int32_t  Birthday,TotalPlayingTime;
	int32_t  Age;
	char DeathMessage[C4MaxDeathMsg+1];
	C4ValueMapData ExtraData;
public:
	bool Save(C4Group &hGroup, class C4DefList *pDefs);
	bool Load(C4Group &hGroup);
	void Default(C4ID n_id=C4ID::None, class C4DefList *pDefs=nullptr, const char *cpNames=nullptr);
	void Promote(int32_t iRank, C4RankSystem &rRanks, bool fForceRankName);
	bool GetNextRankInfo(C4RankSystem &rDefaultRanks, int32_t *piNextRankExp, StdStrBuf *psNextRankName);
	void CompileFunc(StdCompiler *pComp);
protected:
	bool Compile(const char *szSource);
	bool Decompile(StdStrBuf &Buf);

	void UpdateCustomRanks(C4DefList *pDefs); // sets NextRankName and NextRankExp
};

class C4RoundResult
{
public:
	StdCopyStrBuf Title;
	uint32_t Date = 0;
	int32_t Duration = 0;
	int32_t Won = 0;
	int32_t Score = 0, FinalScore = 0, TotalScore = 0;
	int32_t Bonus = 0;
	int32_t Level = 0;
public:
	void Default();
	void CompileFunc(StdCompiler *pComp);
};

class C4PlayerInfoCore
{
public:
	C4PlayerInfoCore();
public:
	// Player Info
	char PrefName[C4MaxName+1];
	char Comment[C4MaxComment+1];
	int32_t  Rank;
	char RankName[C4MaxName+1];
	int32_t  TotalScore;
	int32_t  Rounds,RoundsWon,RoundsLost;
	int32_t  TotalPlayingTime;
	C4RoundResult LastRound;
	C4ValueMapData ExtraData;
	char LeagueName[C4MaxName+1];
	// Preferences
	StdCopyStrBuf PrefControl; // name of control set from definition file
	int32_t PrefMouse;
	int32_t PrefColor;
	uint32_t PrefColorDw, PrefColor2Dw;
	int32_t PrefClonkSkin;
	// Old control method - loaded for backwards compatilibity if PrefControl is unassigned
	// and stored back so you can use the same player file for CR and OC
	int32_t OldPrefControl;
	int32_t OldPrefControlStyle;
	int32_t OldPrefAutoContextMenu;

	// achievements indexed by achievement name and scenario
	C4ScenarioParameters Achievements;
public:
	void Default(C4RankSystem *pRanks=nullptr);
	void Promote(int32_t iRank, C4RankSystem &rRanks);
	bool Load(C4Group &hGroup);
	bool Save(C4Group &hGroup);
	bool CheckPromotion(C4RankSystem &rRanks);
	static DWORD GetPrefColorValue(int32_t iPrefColor);
	void CompileFunc(StdCompiler *pComp);
};

#endif
