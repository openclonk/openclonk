/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2013, The OpenClonk Team and contributors
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

/* Main class to run the game */

#ifndef INC_C4Game
#define INC_C4Game

#include <C4Scenario.h>
#include <C4PathFinder.h>
#include <C4Extra.h>
#include "C4Scoreboard.h"
#include <C4PlayerControl.h>

class C4Game
{
private:
	// used as StdCompiler-parameter
	struct CompileSettings
	{
		bool fScenarioSection;
		bool fPlayers;
		bool fExact;
		bool fSync;

		CompileSettings(bool fScenarioSection, bool fPlayers, bool fExact, bool fSync)
				: fScenarioSection(fScenarioSection), fPlayers(fPlayers), fExact(fExact), fSync(fSync) { }
	};

	// struct of keyboard set and indexed control key
	struct C4KeySetCtrl
	{
		int32_t iKeySet, iCtrl;

		C4KeySetCtrl(int32_t iKeySet, int32_t iCtrl) : iKeySet(iKeySet), iCtrl(iCtrl) {}
	};

public:
	C4Game();
	~C4Game();

	C4GameParameters   &Parameters;
	class C4ScenarioParameters &StartupScenarioParameters; // parameters given on command line or during startup UI
	C4ClientList       &Clients; // Shortcut
	C4TeamList         &Teams; // Shortcut
	C4PlayerInfoList   &PlayerInfos; // Shortcut
	C4PlayerInfoList   &RestorePlayerInfos; // Shortcut
	C4RoundResults      &RoundResults;
	C4Scenario          C4S;
	class C4ScenarioParameterDefs &ScenarioParameterDefs;
	C4ComponentHost     Info;
	C4ComponentHost     Title;
	C4ComponentHost     Names;
	C4ComponentHost     GameText;
	C4LangStringTable   MainSysLangStringTable, ScenarioLangStringTable;
	StdStrBuf           PlayerNames;
	C4Control          &Input; // shortcut

	C4PathFinder        PathFinder;
	C4TransferZones     TransferZones;
	C4Group             ScenarioFile;
	C4GroupSet          GroupSet;
	C4Group             *pParentGroup;
	C4Extra             Extra;
	class C4ScenarioObjectsScriptHost *pScenarioObjectsScript;
	C4ScenarioSection   *pScenarioSections, *pCurrentScenarioSection;
	C4Effect            *pGlobalEffects;
	C4PlayerControlDefs PlayerControlDefs;
	C4PlayerControlAssignmentSets PlayerControlUserAssignmentSets, PlayerControlDefaultAssignmentSets;
	C4Scoreboard        Scoreboard;
	C4Network2Stats *pNetworkStatistics; // may be NULL if no statistics are recorded
	C4KeyboardInput &KeyboardInput;
	C4FileMonitor *pFileMonitor;
	C4GameSec1Timer *pSec1Timer;

	char CurrentScenarioSection[C4MaxName+1];
	char ScenarioFilename[_MAX_PATH+1];
	StdCopyStrBuf ScenarioTitle;
	char PlayerFilenames[20*_MAX_PATH+1];
	char DefinitionFilenames[20*_MAX_PATH+1];
	char DirectJoinAddress[_MAX_PATH+1];
	C4Network2Reference *pJoinReference;
	int32_t StartupPlayerCount;
	int32_t FPS,cFPS;
	int32_t HaltCount;
	bool GameOver;
	bool Evaluated;
	bool GameOverDlgShown;
	bool fScriptCreatedObjects;
	bool fLobby;
	int32_t iLobbyTimeout;
	bool fObserve;
	bool fReferenceDefinitionOverride;
	bool NetworkActive;
	bool Record;
	StdStrBuf RecordDumpFile;
	StdStrBuf RecordStream;
	bool TempScenarioFile;
	bool fPreinited; // set after PreInit has been called; unset by Clear and Default
	int32_t FrameCounter;
	int32_t iTick2,iTick3,iTick5,iTick10,iTick35,iTick255,iTick1000;
	bool TimeGo;
	int32_t Time;
	int32_t StartTime;
	int32_t InitProgress; int32_t LastInitProgress; int32_t LastInitProgressShowTime;
	int32_t RandomSeed;
	int32_t Rules;
	bool GameGo;
	bool FullSpeed;
	int32_t FrameSkip; bool DoSkipFrame;
	uint32_t FoWColor;        // FoW-color; may contain transparency
	bool fResortAnyObject; // if set, object list will be checked for unsorted objects next frame
	bool IsRunning;        // (NoSave) if set, the game is running; if not, just the startup message board is painted
	bool PointersDenumerated; // (NoSave) set after object pointers have been denumerated
	size_t StartupLogPos, QuitLogPos; // current log positions when game was last started and cleared
	bool fQuitWithError; // if set, game shut down irregularly
	int32_t iMusicLevel;      // scenario-defined music level
	// current play list
	StdCopyStrBuf PlayList;
	bool DebugMode;
	// next mission to be played after this one
	StdCopyStrBuf NextMission, NextMissionText, NextMissionDesc;
	// debug settings
	uint16_t DebugPort; StdStrBuf DebugPassword, DebugHost; int DebugWait;

	// Init and execution
	void Clear();
	void Abort(bool fApproved = false); // hard-quit on Esc+Y (/J/O)
	void Evaluate();
	void ShowGameOverDlg();
	bool DoKeyboardInput(C4KeyCode vk_code, C4KeyEventType eEventType, bool fAlt, bool fCtrl, bool fShift, bool fRepeated, class C4GUI::Dialog *pForDialog=NULL, bool fPlrCtrlOnly=false, int32_t iStrength=-1);
	void DrawCursors(C4TargetFacet &cgo, int32_t iPlayer);
	void FixRandom(int32_t iSeed);
	bool Init();
	bool PreInit();
	void SetScenarioFilename(const char*);
	bool HasScenario() { return *DirectJoinAddress || *ScenarioFilename || RecordStream.getSize(); }
	bool Execute();
	C4Player *JoinPlayer(const char *szFilename, int32_t iAtClient, const char *szAtClientName, C4PlayerInfo *pInfo);
	bool DoGameOver();
	bool CanQuickSave();
	bool QuickSave(const char *strFilename, const char *strTitle, bool fForceSave=false);
	void SetInitProgress(float fToProgress);
	void OnResolutionChanged(unsigned int iXRes, unsigned int iYRes); // update anything that's dependant on screen resolution
	void OnKeyboardLayoutChanged();
	void InitFullscreenComponents(bool fRunning);
	bool ToggleChat();
	// Pause
	bool TogglePause();
	bool Pause();
	bool Unpause();
	bool IsPaused();
	// Network
	void Synchronize(bool fSavePlayerFiles);
	void SyncClearance();
	bool ReSync();
	void SyncCheckFiles(); // check if files are in sync
	// Editing
	bool DropFile(const char *szFilename, float iX, float iY);
	bool DropDef(C4ID id, float iX, float iY);
	bool LoadDef(const char *szFilename);
	bool ReloadFile(const char *szPath);
	bool ReloadDef(C4ID id);
	bool ReloadParticle(const char *szName);
	// Object functions
	void ClearPointers(C4Object *cobj);
	C4Object *CreateObject(C4PropList * type, C4Object *pCreator, int32_t owner=NO_OWNER,
	                       int32_t x=50, int32_t y=50, int32_t r=0,
	                       C4Real xdir=Fix0, C4Real ydir=Fix0, C4Real rdir=Fix0, int32_t iController=NO_OWNER);
	C4Object *CreateObject(C4ID type, C4Object *pCreator, int32_t owner=NO_OWNER,
	                       int32_t x=50, int32_t y=50, int32_t r=0,
	                       C4Real xdir=Fix0, C4Real ydir=Fix0, C4Real rdir=Fix0, int32_t iController=NO_OWNER);
	C4Object *CreateObjectConstruction(C4PropList * type,
	                                   C4Object *pCreator,
	                                   int32_t owner,
	                                   int32_t ctx=0, int32_t bty=0,
	                                   int32_t con=1, bool terrain=false);
	C4Object *CreateInfoObject(C4ObjectInfo *cinf, int32_t owner,
	                           int32_t tx=50, int32_t ty=50);
	C4Object *OverlapObject(int32_t tx, int32_t ty, int32_t wdt, int32_t hgt, int32_t Plane);
	C4Object *FindObject(C4Def * pDef,
	                     int32_t iX=0, int32_t iY=0, int32_t iWdt=0, int32_t iHgt=0,
	                     DWORD ocf=OCF_All,
	                     C4Object *pFindNext=NULL);
	C4Object *FindVisObject( // find object in view at pos, regarding parallaxity and visibility (but not distance)
	  float tx, float ty, int32_t iPlr, const C4Facet &fctViewportGame, const C4Facet &fctViewportGUI,
	  float iX, float iY,
	  DWORD category,
	  float gui_x, float gui_y);
	/* int32_t ObjectCount(C4ID id,
	     int32_t x=0, int32_t y=0, int32_t wdt=0, int32_t hgt=0,
	     DWORD ocf=OCF_All,
	     const char *szAction=NULL, C4Object *pActionTarget=NULL,
	     C4Object *pExclude=NULL,
	     C4Object *pContainer=NULL,
	                 int32_t iOwner=ANY_OWNER);*/
	int32_t ObjectCount(C4ID id);
	void CastObjects(C4ID id, C4Object *pCreator, int32_t num, int32_t level, int32_t tx, int32_t ty, int32_t iOwner=NO_OWNER, int32_t iController=NO_OWNER, C4ValueArray *out_objects=NULL);
	C4Object *PlaceVegetation(C4PropList *def, int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, int32_t iGrowth);
	C4Object *PlaceAnimal(C4PropList *def);

	bool LoadScenarioSection(const char *szSection, DWORD dwFlags);
	bool SaveDesc(C4Group &hGroup, bool fSaveGame=false, bool fReference=false, bool fLobby=false, bool fUnregistered=false, bool fRecord=false);

	bool DrawTextSpecImage(C4Facet& fctTarget, const char *szSpec, class C4DrawTransform* pTransform, uint32_t dwClr=0xff);
	float GetTextSpecImageAspect(const char* szSpec);
	bool DrawPropListSpecImage(C4Facet& fctTarget, C4PropList *pSpec);
	bool SpeedUp();
	bool SlowDown();
	bool InitKeyboard(); // register main keyboard input functions
	void UpdateLanguage();
	bool InitPlayerControlSettings();
	bool InitPlayerControlUserSettings(); // merge player control default settings and config overloads into user setting
	void SetDefaultGamma();

protected:
	void Default();
	void InitInEarth();
	void InitVegetation();
	void InitAnimals();
	void InitGoals();
	void InitRules();
	void InitValueOverloads();
	void InitEnvironment();
	void UpdateRules();
	void CloseScenario();
	void DeleteObjects(bool fDeleteInactive);
	void ExecObjects();
	void Ticks();
	bool CheckObjectEnumeration();
	bool LoadScenarioComponents();
public:
	bool LoadAdditionalSystemGroup(class C4Group &parent_group);
	bool SaveGameTitle(C4Group &hGroup);
protected:
	bool InitGame(C4Group &hGroup, bool fLoadSection, bool fLoadSky, C4ValueNumbers *);
	bool InitGameFinal();
	bool InitNetworkFromAddress(const char *szAddress);
	bool InitNetworkFromReference(const C4Network2Reference &Reference);
	bool InitNetworkHost();
	bool DoLobby();
	bool PreInitControl();
	bool InitControl();
	bool InitScriptEngine();
	bool LinkScriptEngine();
	bool InitPlayers(C4ValueNumbers *);
	bool InitRecord();
	bool OpenScenario();
	bool InitDefs();
	bool InitMaterialTexture();
	bool GameOverCheck();
	bool PlaceInEarth(C4ID id);
public:
	void CompileFunc(StdCompiler *pComp, CompileSettings comp, C4ValueNumbers *);
	bool SaveData(C4Group &hGroup, bool fSaveSection, bool fSaveExact, bool fSaveSync, C4ValueNumbers *);
protected:
	bool CompileRuntimeData(C4Group &hGroup, bool fLoadSection, bool exact, bool sync, C4ValueNumbers *);
	bool StoreParticipantPlayers();
	bool RecreatePlayerFiles();

	// Object function internals
	C4Object *NewObject( C4PropList *ndef, C4Object *pCreator,
	                     int32_t owner, C4ObjectInfo *info,
	                     int32_t tx, int32_t ty, int32_t tr,
	                     C4Real xdir, C4Real ydir, C4Real rdir,
	                     int32_t con, int32_t iController);
	void ClearObjectPtrs(C4Object *tptr);
	void ObjectRemovalCheck();

	bool ToggleDebugMode(); // dbg modeon/off if allowed
	bool ActivateMenu(const char *szCommand); // exec given menu command for first local player

public:
	bool ToggleChart(); // chart dlg on/off
	void SetMusicLevel(int32_t iToLvl); // change game music volume; multiplied by config volume for real volume
};


const int32_t
	C4RULE_ConstructionNeedsMaterial = 1;

extern C4Game         Game;

// a global wrapper
inline StdStrBuf GetKeyboardInputName(const char *szKeyName, bool fShort = false, int32_t iIndex = 0)
{
	return Game.KeyboardInput.GetKeyCodeNameByKeyName(szKeyName, fShort, iIndex);
}


#endif
