/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2006-2007  Peter Wortmann
 * Copyright (c) 2006  Günther Brammer
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
/* Game parameters - game data that is valid before the game is started */

#ifndef C4GAMEPARAMETERS_H
#define C4GAMEPARAMETERS_H

#include "C4IDList.h"
#include "C4PlayerInfo.h"
#include "C4LangStringTable.h"
#include "C4Teams.h"

class C4GameRes
{
	friend class C4GameResList;
public:
	C4GameRes();
	C4GameRes(const C4GameRes &Res);
	~C4GameRes();

	C4GameRes &operator = (const C4GameRes &Res);

private:
	C4Network2ResType eType;
	StdCopyStrBuf File;
	const C4Network2ResCore *pResCore;
	C4Network2Res::Ref pNetRes;

public:
	C4Network2ResType  getType() const { return eType; }
	const char        *getFile() const { return File.getData(); }
	bool               isPresent() const { return !! File; }
	const C4Network2ResCore *getResCore() const { return pResCore; }
	C4Network2Res::Ref getNetRes() const { return pNetRes; }

	void SetFile(C4Network2ResType eType, const char *szFile);
	void SetResCore(C4Network2ResCore *pResCore);
	void SetNetRes(C4Network2Res::Ref pRes);

	bool Publish(C4Network2ResList *pResList);
	bool Load(C4Network2ResList *pResList);
	bool InitNetwork(C4Network2ResList *pResList);

	void CalcHash();

	void Clear();

	void CompileFunc(StdCompiler *pComp);
};

class C4GameResList
{
private:
	C4GameRes **pResList;
	int32_t iResCount, iResCapacity;

public:
	C4GameResList() : pResList(NULL), iResCount(0), iResCapacity(0) {}
	~C4GameResList() { Clear(); }

	C4GameResList &operator = (const C4GameResList &List);

	int32_t getResCount() const { return iResCount; }

	C4GameRes *iterRes(C4GameRes *pLast, C4Network2ResType eType = NRT_Null);

	void Clear();
	bool Load(const char *szDefinitionFilenames); // host: create res cores by definition filenames

	C4GameRes *CreateByFile(C4Network2ResType eType, const char *szFile);
	C4GameRes *CreateByNetRes(C4Network2Res::Ref pNetRes);
	bool InitNetwork(C4Network2ResList *pNetResList);

	void CalcHashes();

	bool RetrieveFiles(); // client: make sure all definition files are loaded

	void CompileFunc(StdCompiler *pComp);

protected:
	void Add(C4GameRes *pRes);
};

class C4GameParameters
{
public:
	C4GameParameters();
	~C4GameParameters();

	// League (empty if it's not a league game)
	StdCopyStrBuf League;
	StdCopyStrBuf LeagueAddress;
	StdCopyStrBuf StreamAddress;

	// Maximum player count allowed
	int32_t MaxPlayers;

	// Original network game? Also set in replays of network games for sync safety
	bool IsNetworkGame;

	// Control rate
	int32_t ControlRate;

	// Allow debug mode?
	bool AllowDebug;

	// Active rules and goals
	C4IDList Rules;
	C4IDList Goals;

	// Game resources
	C4GameRes Scenario;
	C4GameResList GameRes;

	// Clients
	C4ClientList Clients;

	// Players & Teams
	C4PlayerInfoList PlayerInfos;
	C4PlayerInfoList RestorePlayerInfos;
	C4TeamList Teams;

	bool isLeague() const { return !!LeagueAddress.getLength(); }
	bool doStreaming() const { return !!StreamAddress.getLength(); }
	const char* getLeague() { return League.getData(); }
	StdStrBuf GetGameGoalString();
	void EnforceLeagueRules(class C4Scenario *pScenario);

	void Clear();
	bool Load(C4Group &hGroup, C4Scenario *pDefault, const char *szGameText, C4LangStringTable *pLang, const char *DefinitionFilenames);
	bool InitNetwork(C4Network2ResList *pResList);
	bool Save(C4Group &hGroup, C4Scenario *pDefault);

	void CompileFunc(StdCompiler *pComp, C4Scenario *pScenario = NULL);
};

#endif // C4GAMEPARAMETERS_H
