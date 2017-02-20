/*
 * OpenClonk, http://www.openclonk.org
 *
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
//
// A separate list of all local and remote player infos is held here,
// independantely of the global C4PlayerList.
// This list is used for:
// -player information to be known before actual join
//  (player count for landscape width, team, color, etc.)
// -player file resource association (network mode)
// -league information to be stored for each player; even after elimination
// *-startup loader screen information; e.g. for replays
//
// * = 2do
//
// Please note that any fields added to C4PlayerInfo will be transferred to the masterserver and back.
// C4RoundResults is responsible for collecting information after player elimination.

#ifndef INC_C4PlayerInfo
#define INC_C4PlayerInfo

#include "network/C4PacketBase.h"
#include "network/C4Network2Res.h"
#include "config/C4Constants.h"
#include "lib/C4InputValidation.h"
#include "object/C4Id.h"

// information about one player at a client
class C4PlayerInfo
{
public:
	// player flags
	enum Flags
	{
		PIF_Joined = 1 << 0,   // player has joined the game
		PIF_Removed = 1 << 2,   // player has been removed
		PIF_HasRes = 1 << 3,   // pRes is set
		PIF_JoinIssued = 1 << 4,   // flag for host to mark a player for which the join is issued
		PIF_TempFile = 1 << 5,   // player file is temporary and to be deleted after join recreation
		PIF_InScenarioFile = 1 << 6,   // player file is present within the scenario; res is not to be used
		PIF_JoinedForSavegameOnly = 1 << 7, // player file has been specified to take over a savegame player; do not join as normal player if association fails
		PIF_Disconnected = 1 << 8,   // the player was removed because his client disconnected
		PIF_Won = 1 << 9,   // player survived until game end (for game evaluation only)
		PIF_VotedOut = 1 << 10,  // player was removed from the round after a successful voting
		PIF_AttributesFixed = 1 << 11, // player color and name aren't changed on collision
		PIF_NoScenarioInit = 1 << 12, // do not call ScenariInit for this player
		PIF_NoEliminationCheck = 1 << 13, // do not eliminate player if crew is empty
		PIF_Invisible = 1 << 14,  // do not show in lobby and menus
		PIF_NoScenarioSave = 1 << 15,  // not saved in SavePlayerInfos.txt if "save as scenario" is performed

		// flags to be synchronized via network and saved into player info
		PIF_SyncFlags = PIF_Joined | PIF_Removed | PIF_HasRes | PIF_InScenarioFile | PIF_JoinedForSavegameOnly | PIF_Disconnected | PIF_Won | PIF_VotedOut | PIF_AttributesFixed | PIF_NoScenarioInit | PIF_NoEliminationCheck | PIF_Invisible | PIF_NoScenarioSave,

		// flags to be copied from savegame-player for takeover
		PIF_SavegameTakeoverFlags = PIF_Joined | PIF_Removed | PIF_JoinIssued | PIF_AttributesFixed | PIF_NoScenarioInit | PIF_NoEliminationCheck | PIF_Invisible | PIF_NoScenarioSave,
	};

	// player attributes used in attribute conflict resolver
	enum Attribute { PLRATT_Color=0, PLRATT_Name=1, PLRATT_Last=2 };
	enum AttributeLevel { PLRAL_Current, PLRAL_Original, PLRAL_Alternate };
private:
	uint32_t dwFlags; // DWORD-mask of C4PlayerInfoFlags-constants
	C4PlayerType eType;         // user or script player

	ValidatedStdCopyStrBuf<C4InVal::VAL_NameNoEmpty> sName;     // player name
	ValidatedStdCopyStrBuf<C4InVal::VAL_NameAllowEmpty> sForcedName; // player name if a new name is forced e.g. because the current name appeared twice
	int32_t iID;          // unique ID set by host
	C4Network2Res::Ref pRes;    // player resource to load from
	C4Network2ResCore ResCore;  // core of resource to load from
	StdCopyStrBuf szFilename;   // source filename for local players
	uint32_t dwColor;           // player color
	uint32_t dwOriginalColor, dwAlternateColor; // original player color wish
	int32_t idSavegamePlayer;   // ID of associated savegame player
	int32_t idTeam;             // team ID
	StdCopyStrBuf szAuthID;     // authentication ID (for league server, will be cleared on successful join)
	int32_t iInGameNumber, iInGameJoinFrame, iInGamePartFrame; // information about player in game
	C4ID idExtraData;           // extra data for script players

	ValidatedStdCopyStrBuf<C4InVal::VAL_NameAllowEmpty> sLeagueAccount; // account name on league server
	int32_t iLeagueScore;       // score on league server at join time
	int32_t iLeagueRank;        // rank on league server at join time
	int32_t iLeagueRankSymbol;  // symbolization of the player's rank
	int32_t iLeagueScoreProjected;// score on league server in case of win
	int32_t iLeagueProjectedGain; // projected league score increase if game is won - -1 for unknown; valid values always positive
	ValidatedStdCopyStrBuf<C4InVal::VAL_NameAllowEmpty> sClanTag; // clan ("team") tag
	int32_t iLeaguePerformance; // script-set league performance value, only set temporarily for masterserver end reference
	StdCopyStrBuf sLeagueProgressData; // level progress data as reported by league

public:
	C4PlayerInfo()                           // construct empty
			:  dwFlags(0), eType(C4PT_User), iID(0), pRes(0), szFilename(), dwColor(0xffffffff),
			dwOriginalColor(0xffffffff), dwAlternateColor(0), idSavegamePlayer(0), idTeam(0), iInGameNumber(-1),
			iInGameJoinFrame(-1), iInGamePartFrame(-1), idExtraData(C4ID::None), sLeagueAccount(""),
			iLeagueScore(0), iLeagueRank(0), iLeagueRankSymbol(0), iLeagueProjectedGain(-1), iLeaguePerformance(0) { }

	void Clear();                            // clear fields

	bool LoadFromLocalFile(const char *szFilename); // load data from local file
	bool SetAsScriptPlayer(const char *szName, uint32_t dwColor, uint32_t dwFlags, C4ID idExtra); // set as a script (AI) player

	void SetJoined(int32_t iNumber); // mark as joined in current game frame
	void SetJoinIssued() { dwFlags |= PIF_JoinIssued; }        // mark as joined
	void SetRemoved();              // mark as removed in current game frame - always marks as previously joined, too
	void SetID(int32_t iToID) { iID = iToID; }              // set player info ID
	void SetColor(DWORD dwUseClr) { dwColor = dwUseClr; } // set color to be used
	void SetOriginalColor(DWORD dwUseClr) { dwOriginalColor = dwUseClr; } // set color the player wishes to have
	void SetFilename(const char *szToFilename);           // set new player filename
	void SetToScenarioFilename(const char *szScenFilename); // set to file within scenario; discard resource
	void SetTempFile() { assert(!!szFilename); dwFlags |= PIF_TempFile; } // mark filename as temp, so it is deleted in dtor or after join
	void SetTeam(int32_t idToTeam) { idTeam = idToTeam; }
	void DeleteTempFile();                                // delete filename if temp
	void LoadResource();                                  // network: Load resource if present and not being loaded yet
	void DiscardResource();                               // delete any source resource for network player infos
	void SetAssociatedSavegamePlayer(int32_t aidSavegamePlayer)   // link with savegame player from restore list
	{ idSavegamePlayer=aidSavegamePlayer; }
	int32_t GetAssociatedSavegamePlayerID() const
	{ return idSavegamePlayer; }
	void SetJoinForSavegameOnly()                          // flag to be deleted if savegame association fails
	{ dwFlags |= PIF_JoinedForSavegameOnly; }
	bool IsJoinForSavegameOnly()                           // flag to be deleted if savegame association fails
	{ return !!(dwFlags & PIF_JoinedForSavegameOnly); }
	bool SetSavegameResume(C4PlayerInfo *pSavegameInfo);   // take over savegame player data to do resume
	void SetAuthID(const char *sznAuthID)
	{ szAuthID = sznAuthID; }
	void SetLeagueData(const char *szAccount, const char *szNewClanTag, int32_t iScore, int32_t iRank, int32_t iRankSymbol, const char *szProgressData)
	{ sLeagueAccount.CopyValidated(szAccount); sClanTag.CopyValidated(szNewClanTag); iLeagueScore = iScore; iLeagueRank = iRank; iLeagueRankSymbol = iRankSymbol; sLeagueProgressData.Copy(szProgressData); }
	void SetLeaguePerformance(int32_t iNewPerf)
		{ iLeaguePerformance = iNewPerf; }
	void SetLeagueProgressData(const char *szNewProgressData)
		{ if (szNewProgressData) sLeagueProgressData.Copy(szNewProgressData); else sLeagueProgressData.Clear(); }
	void SetVotedOut()
	{ dwFlags |= PIF_VotedOut; }
	void SetLeagueProjectedGain(int32_t iProjectedGain)
	{ assert(iProjectedGain>=0); iLeagueProjectedGain = iProjectedGain; }
	void ResetLeagueProjectedGain()
	{ iLeagueProjectedGain = -1; }
	void SetForcedName(const char *szNewName)
	{ if (szNewName) sForcedName.CopyValidated(szNewName); else sForcedName.Clear(); }

	void CompileFunc(StdCompiler *pComp);

	C4PlayerType GetType() const { return eType; }
	uint32_t GetColor() const { return dwColor; }     // get player color
	uint32_t GetLobbyColor() const;
	uint32_t GetOriginalColor() const { return dwOriginalColor; }     // get original player color
	uint32_t GetAlternateColor() const { return dwAlternateColor; }     // get secondary original player color
	const char *GetName() const { return sLeagueAccount.getLength() ? sLeagueAccount.getData() : sForcedName.getLength() ? sForcedName.getData() : sName.getData(); } // get player name
	const char *GetOriginalName() const { return sName.getData(); }
	const char *GetForcedName() const { return sForcedName.getData(); }
	StdStrBuf GetLobbyName() const; // return player name including clan/team tag if known; fallback to regular player name
	const char *GetFilename() const { return szFilename.getData(); } // get filename for local games
	const char *GetLocalJoinFilename() const;              // get name of file to join the player from
	C4Network2Res *GetRes() const { return pRes; }         // get player resource for network games
	bool IsRemoved() const { return !!(dwFlags & PIF_Removed); }
	bool HasJoined() const { return !!(dwFlags & PIF_Joined); }    // return whether player has joined
	bool IsJoined() const { return HasJoined() && !(dwFlags & PIF_Removed); } // return whether player is currently in the game
	bool HasJoinIssued() const { return !!(dwFlags & (PIF_Joined | PIF_JoinIssued)); } // return whether player join is in the queue already (or performed long ago, even)
	bool HasJoinPending() const { return !(dwFlags & (PIF_Joined | PIF_Removed)); } // return whether player join should be done but has not been performed yet
	bool IsUsingColor() const { return !IsRemoved() && !idSavegamePlayer; } //return whether the player is actually using the player color
	bool IsUsingName() const { return !IsRemoved() && !sLeagueAccount.getLength(); } //return whether the player is actually using the player name (e.g. not if league name is used)
	bool IsUsingAttribute(Attribute eAttr) const { if (eAttr == PLRATT_Color) return IsUsingColor(); else return IsUsingName(); }
	bool IsUsingPlayerFile() const { return !IsRemoved(); } //return whether the player is using the file (i.e., isn't dead yet)
	bool IsUsingTeam() const { return !IsRemoved(); }       // whether player should be in the team list
	bool IsAttributesFixed() const { return !!(dwFlags & PIF_AttributesFixed); }
	bool IsInvisible() const { return !!(dwFlags & PIF_Invisible); }
	bool IsScenarioInitDesired() const { return !(dwFlags & PIF_NoScenarioInit); }
	bool IsScenarioSaveDesired() const { return !(dwFlags & PIF_NoScenarioSave); }
	C4ID GetScriptPlayerExtraID() const { return idExtraData; }
	bool IsNoEliminationCheck() const { return !!(dwFlags & PIF_NoEliminationCheck); }
	bool HasAutoGeneratedColor() { return dwColor != dwOriginalColor; } // whether the player got a new color assigned due to color conflict
	bool HasWon() const { return !!(dwFlags & PIF_Won); }
	bool HasTeamWon() const;
	const char *getAuthID() const { return szAuthID.getData(); } // returns authentication ID for this player [league]
	const char *getLeagueAccount() const { return sLeagueAccount.getData(); } // returns account name on league server
	int32_t getLeagueScore() const { return iLeagueScore; } // returns score number on league server (0 for not assigned)
	int32_t getLeagueRank() const { return iLeagueRank; } // returns rank on league server (0 for not assigned)
	int32_t getLeagueRankSymbol() const { return iLeagueRankSymbol; } // returns rank symbol on league server (0 for not assigned)
	int32_t getLeagueScoreProjected() const { return iLeagueScoreProjected; } // returns score on league server in case of win (0 for not assigned)
	int32_t GetInGameNumber() const { return iInGameNumber; } // returns player number the player had in the game
	bool IsLeagueProjectedGainValid() const { return iLeagueProjectedGain>=0; }
	int32_t GetLeagueProjectedGain() const { return iLeagueProjectedGain; } // get score gain in primary league if this player's team wins
	const char *GetLeagueProgressData() const { return sLeagueProgressData.getData(); } 

	int32_t GetID() const { return iID; }                    // get unique ID, if assigned
	int32_t GetTeam() const { return idTeam; }
	bool IsTempFile() const { return !!(dwFlags & PIF_TempFile); } // return whether filename points to temp folder

	DWORD GetFlags() { return dwFlags; } // for dbg print only

	void SetDisconnected() { dwFlags |= PIF_Disconnected; }
	void SetWinner() { dwFlags |= PIF_Won; }

	bool LoadBigIcon(C4FacetSurface &fctTarget); // load BigIcon.png of player into target facet; return false if no bigicon present or player file not yet loaded
};

// player infos for one client
// merely a list of player infos
class C4ClientPlayerInfos
{
private:
	// std::vector...
	int32_t iPlayerCount;               // number of clients registered into the list
	int32_t iPlayerCapacity;            // size of pClients-array
	C4PlayerInfo **ppPlayers;       // array of registered client information
	void GrowList(size_t iByVal); // increase list capacity

	int32_t iClientID;          // ID of client described by this packet

	// flags for this packet
	enum Flags
	{
		CIF_AddPlayers  = 1<<0, // if set, the players are to be added to the current list (otherwise overwrite)
		CIF_Updated     = 1<<1, // set temporarily if changed and not transmissioned to clients (valid for host only)
		CIF_Initial     = 1<<2, // set for first-time player info packets
		CIF_Developer   = 1<<3, // set for developer hosts (by regkey); client side check only!
		CIF_Removed     = 1<<4 // client was removed
	};
	uint32_t dwFlags; // bit mask of the above flags

public:
	C4ClientPlayerInfos(const char *szJoinFilenames=nullptr, bool fAdd=false, C4PlayerInfo *pAddInfo=nullptr); // ctor; sets local data (or makes an add-player-packet if filename is given) if par is true
	C4ClientPlayerInfos(const C4ClientPlayerInfos &rCopy); // copy ctor
	~C4ClientPlayerInfos() { Clear(); }                   // dtor

	C4ClientPlayerInfos &operator = (const C4ClientPlayerInfos &rCopy);

	void Clear();                                     // del all players
	void GrabMergeFrom(C4ClientPlayerInfos &rFrom);   // merge existing player info packed into this one - empties pFrom!
	void AddInfo(C4PlayerInfo *pAddInfo);             // add info to list
	void RemoveIndexedInfo(int32_t iAtIndex);             // remove info from list (delete it)
	void RemoveInfo(int32_t idPlr);                       // remove info from list (delete it)

	// update-flag
	void SetUpdated() { dwFlags |= CIF_Updated; }
	bool IsUpdated() { return !!(dwFlags & CIF_Updated); }
	void ResetUpdated() { dwFlags &= ~CIF_Updated; }
	void SetAdd() { dwFlags |= CIF_AddPlayers; }
	void ResetAdd() { dwFlags &= ~CIF_AddPlayers; }

	// query functions
	int32_t GetPlayerCount() const { return iPlayerCount; } // get number of player infos available
	int32_t GetFlaggedPlayerCount(DWORD dwFlag) const;      // get number of player infos with any of the given flags set
	C4PlayerInfo *GetPlayerInfo(int32_t iIndex) const;      // get indexed player info
	C4PlayerInfo *GetPlayerInfo(int32_t iIndex, C4PlayerType eType) const;      // get indexed player info of given type
	C4PlayerInfo *GetPlayerInfoByID(int32_t id) const;      // get player info by unique player ID
	C4PlayerInfo *GetPlayerInfoByRes(int32_t idResID) const; // get player info by resource ID
	int32_t GetClientID() const { return iClientID; }       // get target client ID
	bool HasUnjoinedPlayers() const;                          // check all players and return whether one of them didn't join
	int32_t GetJoinedPlayerCount() const;                   // return number of players that are IsJoined()
	bool IsAddPacket() const { return !!(dwFlags & CIF_AddPlayers); }  // return whether players are to be added to the current list (otherwise overwrite)
	bool IsInitialPacket() const { return !!(dwFlags & CIF_Initial); } // returns whether this packet was sent as the first local-join packet
	bool IsDeveloperPacket() const { return !!(dwFlags & CIF_Developer); } // returns whether packet was created by a developer host - client side check only!

	// network: Load all resources connected with the players that are not being loaded yet
	void LoadResources();

	// pack/unpack functions
	void CompileFunc(StdCompiler *pComp);
};

// * PID_PlayerInfoUpdRequest
// packet containing information about one or more joined players at a client
// or about lobby player-info updates
class C4PacketPlayerInfoUpdRequest : public C4PacketBase
{
public:
	C4ClientPlayerInfos Info; // info for clients to be joined

	C4PacketPlayerInfoUpdRequest() : Info() { } // std ctor
	C4PacketPlayerInfoUpdRequest(const char *szFilenames, bool fAdd) // ctor
			: Info(szFilenames, fAdd) { };

	C4PacketPlayerInfoUpdRequest(const C4ClientPlayerInfos &rInfo) : Info(rInfo) {} // ctor

	virtual void CompileFunc(StdCompiler *pComp);
};

// * PID_PlayerInfoUpd
// packet containing information about one or more (updated) players at a client
class C4PacketPlayerInfo : public C4PacketBase
{
public:
	C4ClientPlayerInfos Info; // info for clients to be updated
	bool fIsRecreationInfo;   // if set, this info packet describes savegame recreation players

	C4PacketPlayerInfo() : Info(), fIsRecreationInfo(false) { } // std ctor
	C4PacketPlayerInfo(const C4ClientPlayerInfos &rCopyInfos, bool fRecreationPlayers) // ctor
			: Info(rCopyInfos), fIsRecreationInfo(fRecreationPlayers) { };

	virtual void CompileFunc(StdCompiler *pComp);
};

// player info list
// contains player info packets for all known clients and self
class C4PlayerInfoList
{
private:
	// std::vector...
	int32_t iClientCount;               // number of clients registered into the list
	int32_t iClientCapacity;            // size of pClients-array
	C4ClientPlayerInfos **ppClients; // array of registered client information
	void GrowList(size_t iByVal); // increase list capacity

	int32_t iLastPlayerID;              // last ID given to a player

	enum MatchingLevel { PML_PlrFileName=0, PML_PlrName, PML_PrefColor, PML_Any };

public:
	C4PlayerInfoList();               // ctor
	C4PlayerInfoList(const C4PlayerInfoList &rCpy);
	~C4PlayerInfoList() { Clear(); }  // dtor
	C4PlayerInfoList &operator = (const C4PlayerInfoList &rCpy);
	void Clear();                     // clear list

	// forwards player info update request to the appropriate handler
	bool DoPlayerInfoUpdate(C4ClientPlayerInfos *pUpdate);

	// performs a local player join for the given player file(s)
	bool DoLocalNonNetworkPlayerJoin(const char *szPlayerFile);
	bool DoLocalNonNetworkPlayerInfoUpdate(C4ClientPlayerInfos *pUpdate);

	// sets any unset IDs (host/standalone only); also removes players that would exceed the maximum player limit
	// returns whether any players remain
	bool AssignPlayerIDs(C4ClientPlayerInfos *pNewClientInfo);

	// assign any unset teams (host/standalone only) - fByHost determines whether packet was sent by host
	void AssignTeams(C4ClientPlayerInfos *pNewClientInfo, bool fByHost);

	// generate teams used by the player info list if they do not exist and auto generated teams are enabled
	// used for replays
	void RecheckAutoGeneratedTeams();

	// add info for client; overwriting or appending to existing info if necessary
	// this takes over the pNewClientInfo ptr, and may invalidate (delete) it!
	// the pointer to the info structure as it is valid in the list is returned
	// when infos are added, unset IDs will automatically be assigned (should happen for host only!)
	C4ClientPlayerInfos *AddInfo(C4ClientPlayerInfos *pNewClientInfo);

	// resolve any color conflicts in self AND given (optional) packet. Sets Updated-flags.
	void ResolvePlayerAttributeConflicts(C4ClientPlayerInfos *pSecPacket);

	// do color updates: Savegame color assignment; team colors; duplicate attribute check
	void UpdatePlayerAttributes(C4ClientPlayerInfos *pForInfo, bool fResolveConflicts);
	void UpdatePlayerAttributes();

	// query functions
	int32_t GetInfoCount() const { return iClientCount; }    // get number of registered client infos
	C4ClientPlayerInfos *GetIndexedInfo(int32_t iIndex) const // get client player infos by indexed
	{ return (ppClients && Inside<int32_t>(iIndex, 0, iClientCount-1)) ? ppClients[iIndex] : nullptr; }
	C4ClientPlayerInfos **GetInfoPtrByClientID(int32_t iClientID) const; // get info for a specific client ID
	C4ClientPlayerInfos *GetInfoByClientID(int32_t iClientID) const
	{ C4ClientPlayerInfos **ppNfo = GetInfoPtrByClientID(iClientID); return ppNfo ? *ppNfo : nullptr; }
	C4PlayerInfo *GetPlayerInfoByIndex(int32_t index) const;  // get player info by index (for running through all players regardless of clients or ids)
	C4PlayerInfo *GetPlayerInfoByID(int32_t id) const;        // get player info by unique player ID
	C4PlayerInfo *GetPlayerInfoByID(int32_t id, int32_t *pidClient) const;  // get player info by unique player ID, and assign associated client
	C4ClientPlayerInfos *GetClientInfoByPlayerID(int32_t id) const; // get client info that contains a specific player
	C4PlayerInfo *GetPlayerInfoBySavegameID(int32_t id) const;// get player info by savegame association ID
	C4PlayerInfo *GetNextPlayerInfoByID(int32_t id) const;    // get player info with smallest ID > given id
	C4PlayerInfo *GetActivePlayerInfoByName(const char *szName);    // find info by name (case insensitive)
	int32_t GetPlayerCount() const;                           // get number of players on all clients
	int32_t GetJoinIssuedPlayerCount() const;                 // get number of players with PIF_JoinIssued-flag set
	int32_t GetJoinPendingPlayerCount() const;                 // get number of players with PIF_JoinIssued-flag but not joined or removed flag set
	int32_t GetActivePlayerCount(bool fCountInvisible) const;                     // get number of players that have not been removed
	StdStrBuf GetActivePlayerNames(bool fCountInvisible, int32_t iAtClientID=-1) const;                   // get a comma-separated list of players that have not been removed yet
	int32_t GetActiveScriptPlayerCount(bool fCountSavegameResumes, bool fCountInvisible) const;               // get number of script players that have not been removed
	C4PlayerInfo *GetPrimaryInfoByClientID(int32_t iClientID) const
	{
		C4ClientPlayerInfos *pInfoPkt = GetInfoByClientID(iClientID);
		if (!pInfoPkt) return nullptr;
		return pInfoPkt->GetPlayerInfo(0);
	}
	C4PlayerInfo *FindSavegameResumePlayerInfo(const C4PlayerInfo *pMatchInfo, MatchingLevel mlMatchStart, MatchingLevel mlMatchEnd) const; // automatic savegame player association: Associate by name (or prefcolor, if none matches)
	bool HasSameTeamPlayers(int32_t iClient1, int32_t iClient2) const; // check all active players; return true if two of them on different clients are in the same team
	C4PlayerInfo *FindUnassociatedRestoreInfo(const C4PlayerInfoList &rRestoreInfoList); // find a player in the given list that has not been associated by a player in this list

	void RemoveInfo(C4ClientPlayerInfos **ppRemoveInfo) // remove client info given by direct ptr into list
	{ *ppRemoveInfo = ppClients[--iClientCount]; /* maybe redundant self-assignment; no vector shrink */ }

public:
	bool Load(C4Group &hGroup, const char *szFromFile, class C4LangStringTable *pLang=nullptr); // clear self and load from group file
	bool Save(C4Group &hGroup, const char *szToFile); // save to group file

	// external ID counter manipulation used by C4Game
	void SetIDCounter(int32_t idNewCounter) { iLastPlayerID = idNewCounter; }
	int32_t GetIDCounter() { return iLastPlayerID; }
	void FixIDCounter(); // make sure ID counter is same as largest info

	// game interaction
	bool InitLocal();                       // put locally joining players into list (non-network)
	bool LocalJoinUnjoinedPlayersInQueue(); // join all unjoined players to local input queue
	int32_t GetStartupCount();              // get number of players already joined and to be joined
	void CreateRestoreInfosForJoinedScriptPlayers(C4PlayerInfoList &rSavegamePlayers); // create matching script player joins for all script playeers in restore info
	bool RecreatePlayers(C4ValueNumbers *); // directly join all players whose join-flag is set
	bool RecreatePlayerFiles();             // update player source files
	bool RestoreSavegameInfos(C4PlayerInfoList &rSavegamePlayers); // recreate this list from rSavegamePlayers for host/single games; just merge associated infos
	bool SetAsRestoreInfos(C4PlayerInfoList &rFromPlayers, bool fSaveUserPlrs, bool fSaveScriptPlrs, bool fSetUserPlrRefToLocalGroup, bool fSetScriptPlrRefToLocalGroup); // copy all joined players from player list
	void RemoveUnassociatedPlayers(C4PlayerInfoList &rSavegamePlayers); // remove all savegame players that are not associated to this list from the game
	int32_t GetFreePlayerSlotCount();       // get number of players that may still join
	void ResetLeagueProjectedGain(bool fSetUpdated); // reset known projected gains for all players (to be updated by league again)

	// network: Load all resources connected with the players that are not being loaded yet
	void LoadResources();

	// compiler
	void CompileFunc(StdCompiler *pComp);
};

#endif // INC_C4PlayerInfo
