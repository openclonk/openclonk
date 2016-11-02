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
// merely controls what to save when how - actual saving procedures reside in the subclasses
//
// Game saving is done on the following occasions:
// -scenario saving (C4GameSaveScenario) [SyncStateScenario; KeepFiles]
// -savegames (C4GameSaveSavegame) [SyncStateSavegame; KeepFiles]
// -records (C4GameSaveRecord) [SyncStateSynced; KeepFiles] - initially and while game is running
// -network synchronizations (C4GameSaveNetwork) [SyncStateSynced] - in lobby and runtime mode
// -network references (C4GameSaveNetReference) [SyncStateScenario]

#ifndef INC_C4GameSave
#define INC_C4GameSave

#include "landscape/C4Scenario.h"
#include "c4group/C4Components.h"

class C4GameSave
{
private:
	C4Scenario rC4S; // local scenario core copy

protected:
	C4Group *pSaveGroup; // group file written to
	bool fOwnGroup;      // whether group file is owned

	// if set, the game is saved at initial (pre-frame0) state
	// (lobby-dynamics, initial records and network references)
	// no runtime data will be saved for initial-state-saves
	// SafeGame/NoInitialize-core-settings will be kept in its current state for initial saves
	bool fInitial;

	// sync state describes what to save
	enum SyncState
	{
		SyncNONE = 0,
		SyncScenario = 1, // save (eventually static) landscape and objects only to play as a separate scenario later
		SyncSavegame = 2, // save all runtime data, so the scenario can be continued at a future date
		SyncSynchronized = 3 // save exact runtime data to be network- or replay-save
	} Sync; // sync is set by ctor

	// query functions
	virtual bool GetSaveRuntimeData() { return !fInitial; }               // save exact landscape, players, etc.
	virtual bool GetKeepTitle() { return !IsExact(); }            // whether original, localized title with image and icon shall be deleted
	virtual bool GetSaveDesc() { return true; }                   // should WriteDescData be executed in Save()-call?
	virtual bool GetCopyScenario() { return true; }               // return whether the savegame depends on the game scenario file
	virtual const char *GetSortOrder() { return C4FLS_Scenario; } // return nullptr to prevent sorting
	virtual bool GetCreateSmallFile() { return false; }           // return whether file size should be minimized
	virtual bool GetForceExactLandscape() { return GetSaveRuntimeData() && IsExact(); } // whether exact landscape shall be saved
	virtual bool GetSaveOrigin() { return false; }                // return whether C4S.Head.Origin shall be set
	virtual bool GetClearOrigin() { return !GetSaveOrigin(); }    // return whether C4S.Head.Origin shall be cleared if it's set
	virtual bool GetSaveUserPlayers() { return IsExact(); }       // return whether joined user players shall be saved into SavePlayerInfos
	virtual bool GetSaveScriptPlayers() { return IsExact(); }       // return whether joined script players shall be saved into SavePlayerInfos
	virtual bool GetSaveUserPlayerFiles() { return IsExact(); }       // return whether .ocp files of joined user players shall be put into the scenario
	virtual bool GetSaveScriptPlayerFiles() { return IsExact(); }       // return whether .ocp files of joined script players shall be put into the scenario

	// savegame specializations
	virtual void AdjustCore(C4Scenario &rC4S) {}         // set specific C4S values
	virtual bool WriteDesc(StdStrBuf &sBuf) { return true; } // write desc (contents only)
	virtual bool SaveComponents() { return true; }       // save (or remove) custom components for specialization
	virtual bool OnSaving() { return true; } // callback for special actions to be performed when saving (like, add sync)

	// query sync level
	bool IsExact() { return Sync>=SyncSavegame; }      // exact save (players, always exact landscape, etc.)
	bool IsSynced() { return Sync>=SyncSynchronized; } // synchronized

	// protected constructor
	C4GameSave(bool fAInitial, SyncState ASync) : pSaveGroup(nullptr), fOwnGroup(false), fInitial(fAInitial), Sync(ASync) { }
protected:
	// some desc writing helpers
	void WriteDescLineFeed(StdStrBuf &sBuf); // append a line break to desc
	void WriteDescDate(StdStrBuf &sBuf, bool fRecord = false);     // append current date to desc buffer
	void WriteDescGameTime(StdStrBuf &sBuf); // append game time to desc buffer, if it's >0
	void WriteDescDefinitions(StdStrBuf &sBuf);    // append used definition filenames to desc buffer
	void WriteDescNetworkClients(StdStrBuf &sBuf); // append current network client list to desc buffer
	void WriteDescPlayers(StdStrBuf &sBuf, bool fByTeam, int32_t idTeam); // helper func used by WriteDescPlayers: Write all players matching team
	void WriteDescPlayers(StdStrBuf &sBuf);        // append currently participating players to desc buffer
	void WriteDescLeague(StdStrBuf &sBuf, bool fLeague, const char *strLeagueName); // append league status
	void WriteDescEngine(StdStrBuf &sBuf);         // append engine build
private:
	// saving subcalls
	bool SaveCreateGroup(const char *szFilename, C4Group &hUseGroup); // create/copy group at target filename
	bool SaveCore();             // save C4S core
	bool SaveScenarioSections(); // save scenario sections
	bool SaveLandscape();        // save current landscape
	bool SaveRuntimeData(); // save any runtime data
public:
	virtual ~C4GameSave() { Close(); } // dtor: close group

	bool Save(const char *szFilename); // create group at filename and do actual saving; group is kept open until dtor or Close()-call!
	bool Save(C4Group &hToGroup, bool fKeepGroup);      // save game directly to target group
	bool SaveDesc(C4Group &hToGroup);                   // save scenario desc to file
	bool Close();                      // close scenario group

	C4Group *GetGroup() { return pSaveGroup; } // get scenario saving group; only open between calls to Save() and Close()
};

class C4GameSaveScenario : public C4GameSave
{
public:
	C4GameSaveScenario(bool fForceExactLandscape, bool fSaveOrigin) : C4GameSave(false, SyncScenario), fForceExactLandscape(fForceExactLandscape), fSaveOrigin(fSaveOrigin) {} // ctor

protected:
	bool fForceExactLandscape;
	bool fSaveOrigin;
	virtual bool GetSaveOrigin() { return fSaveOrigin; }
	virtual bool GetClearOrigin() { return false; } // always keep existing origin
	virtual bool GetSaveDesc() { return false; }    // should WriteDescData be executed in Save()-call?
	virtual bool GetForceExactLandscape() { return C4GameSave::GetForceExactLandscape() || fForceExactLandscape; }
	virtual bool GetSaveScriptPlayers() { return true; } // script players are also saved; but user players aren't!
	virtual bool GetSaveScriptPlayerFiles() { return true; } // script players are also saved; but user players aren't!
};

class C4GameSaveSavegame : public C4GameSave
{
public:
	C4GameSaveSavegame() : C4GameSave(false, SyncSavegame) {}

protected:
	// savegame specializations
	virtual bool GetSaveOrigin() { return true; }   // origin must be saved in savegames
	virtual bool GetSaveUserPlayerFiles() { return false; } // user player files are not needed in savegames, because they will be replaced by player files of resuming playerss
	virtual void AdjustCore(C4Scenario &rC4S);      // set specific C4S values
	virtual bool WriteDesc(StdStrBuf &sBuf);        // write savegame desc (contents only)
	virtual bool SaveComponents();                  // custom savegame components (title)
	virtual bool OnSaving();                        // add sync when saving

};

class C4GameSaveRecord : public C4GameSave
{
private:
	int iNum; // record number
	bool fLeague; // recording of a league game?
	bool fCopyScenario; // copy scenario?

public:
	C4GameSaveRecord(bool fAInitial, int iANum, bool fLeague, bool fCopyScenario = true)
			: C4GameSave(fAInitial, SyncSynchronized), iNum(iANum), fLeague(fLeague), fCopyScenario(fCopyScenario)
	{}

protected:
	// query functions
	virtual bool GetSaveDesc() { return false; }     // desc is saved by external call when the record is finished
	virtual bool GetCreateSmallFile() { return true; } // no need to save players complete with portraits
	virtual bool GetSaveOrigin() { return true; }    // origin must be saved to trace language packs, folder local material, etc. for records

	virtual bool GetCopyScenario() { return fCopyScenario; } // records without copied scenario are a lot smaller can be reconstructed later (used for streaming)

	// savegame specializations
	virtual void AdjustCore(C4Scenario &rC4S);       // set specific C4S values
	virtual bool WriteDesc(StdStrBuf &sBuf);         // write desc (contents only) - using old-style unchecked string buffers here...
	virtual bool SaveComponents();                   // custom components: PlayerInfos even if fInitial

};

class C4GameSaveNetwork : public C4GameSave
{
public:
	C4GameSaveNetwork(bool fAInitial) : C4GameSave(fAInitial, SyncSynchronized) {}

protected:
	// query functions
	virtual bool GetSaveOrigin() { return true; }     // clients must know where to get music and localization
	virtual bool GetKeepTitle() { return false; }     // always delete title files (not used in dynamics)
	virtual bool GetSaveDesc() { return false; }      // no desc in dynamics
	virtual bool GetCreateSmallFile() { return true; }// return whether file size should be minimized

	virtual bool GetCopyScenario() { return false; }    // network dynamics do not base on normal scenario
	// savegame specializations
	virtual void AdjustCore(C4Scenario &rC4S);           // set specific C4S values
};

#endif // INC_C4GameSave
