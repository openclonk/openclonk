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

/* Main class to run the game */

#ifndef INC_C4Game
#define INC_C4Game

#include "c4group/C4Extra.h"
#include "control/C4PlayerControl.h"
#include "gui/C4Scoreboard.h"
#include "landscape/C4PathFinder.h"
#include "landscape/C4Scenario.h"
#include "landscape/C4TransferZone.h"

class C4ScriptGuiWindow;

class C4Game
{
public:
	// Initialization mode: Regular game start, section load or editor reload
	enum InitMode
	{
		IM_Normal = 0,
		IM_Section = 1,
		IM_ReInit = 2
	};
private:
	// used as StdCompiler-parameter
	struct CompileSettings
	{
		InitMode init_mode;
		bool fPlayers;
		bool fExact;
		bool fSync;

		CompileSettings(InitMode init_mode, bool fPlayers, bool fExact, bool fSync)
				: init_mode(init_mode), fPlayers(fPlayers), fExact(fExact), fSync(fSync) { }
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
	C4PlayerControlDefs PlayerControlDefs;
	C4PlayerControlAssignmentSets PlayerControlUserAssignmentSets, PlayerControlDefaultAssignmentSets;
	C4Scoreboard        Scoreboard;
	std::unique_ptr<C4Network2Stats> pNetworkStatistics; // may be nullptr if no statistics are recorded
	C4KeyboardInput &KeyboardInput;
	std::unique_ptr<C4FileMonitor> pFileMonitor;
	std::unique_ptr<C4GameSec1Timer> pSec1Timer;
	C4Value            &GlobalSoundModifier; // contains proplist for sound modifier to be applied to all new sounds played

	char CurrentScenarioSection[C4MaxName+1];
	char ScenarioFilename[_MAX_PATH_LEN];
	StdCopyStrBuf ScenarioTitle;
	char PlayerFilenames[20*_MAX_PATH_LEN];
	char DefinitionFilenames[20*_MAX_PATH_LEN];
	char DirectJoinAddress[_MAX_PATH_LEN];
	char DirectJoinTempFilename[_MAX_PATH_LEN];
	std::unique_ptr<C4Network2Reference> pJoinReference;
	int32_t StartupPlayerCount;
	int32_t StartupTeamCount;
	int32_t FPS,cFPS;
	int32_t HaltCount;
	bool InitialPlayersJoined; // true after the InitializeFinal callback has been made
	bool GameOver;
	bool EvaluateOnAbort; // set in Scenario.txt, copied here because of sections
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
	StdStrBuf TempScenarioFile;
	bool fPreinited{false}; // set after PreInit has been called; unset by Clear and Default
	int32_t FrameCounter;
	int32_t iTick2, iTick3, iTick5, iTick10, iTick35, iTick255, iTick1000;
	bool TimeGo;
	int32_t Time;
	int32_t StartTime;
	int32_t InitProgress; int32_t LastInitProgress; int32_t LastInitProgressShowTime;
	int32_t RandomSeed;
	bool GameGo;
	bool FullSpeed;
	int32_t FrameSkip; bool DoSkipFrame;
	bool fResortAnyObject; // if set, object list will be checked for unsorted objects next frame
	bool IsRunning;        // (NoSave) if set, the game is running; if not, just the startup message board is painted
	bool PointersDenumerated; // (NoSave) set after object pointers have been denumerated
	size_t StartupLogPos{0}, QuitLogPos{0}; // current log positions when game was last started and cleared
	bool fQuitWithError{false}; // if set, game shut down irregularly
	// Show errors and allow debug commands?
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
	bool DoKeyboardInput(C4KeyCode vk_code, C4KeyEventType event_type, bool alt, bool ctrl, bool shift, bool repeated, class C4GUI::Dialog *for_dialog = nullptr, bool fPlrCtrlOnly = false, int32_t strength = -1);
	bool DoKeyboardInput(C4KeyCodeEx key, C4KeyEventType event_type, class C4GUI::Dialog *for_dialog = nullptr, bool fPlrCtrlOnly = false, int32_t strength = -1);
	void DrawCrewOverheadText(C4TargetFacet &cgo, int32_t player_nr);
	void FixRandom(uint64_t seed);
	bool Init();
	bool PreInit();
	void SetScenarioFilename(const char*);
	bool HasScenario() { return *DirectJoinAddress || *ScenarioFilename || RecordStream.getSize(); }
	bool Execute();
	C4Player *JoinPlayer(const char *filename, int32_t at_client, const char *at_client_name, C4PlayerInfo *info);
	void OnPlayerJoinFinished();
	bool DoGameOver();
	bool CanQuickSave();
	bool QuickSave(const char *filename, const char *title, bool force_save = false);
	void SetInitProgress(float to_progress);
	void OnResolutionChanged(unsigned int res_x, unsigned int res_y); // update anything that's dependent on screen resolution
	void OnKeyboardLayoutChanged();
	void InitFullscreenComponents(bool is_running);
	bool ToggleChat();
	// Pause
	bool TogglePause();
	bool Pause();
	bool Unpause();
	bool IsPaused();
	// Network
	void Synchronize(bool save_player_files);
	void SyncClearance();
	// Editing
	bool DropFile(const char *filename, float x, float y);
	bool DropDef(C4ID id, float x, float y);
	bool ReloadFile(const char *filepath);
	bool ReloadDef(C4ID id);
	bool ReloadParticle(const char *name);
	// Object functions
	void ClearPointers(C4Object *obj);
	C4Object *CreateObject(C4PropList * type, C4Object *creator, int32_t owner = NO_OWNER,
	                       int32_t x = 50, int32_t y = 50, int32_t r = 0, bool grow_from_center = false,
	                       C4Real xdir = Fix0, C4Real ydir = Fix0, C4Real rdir = Fix0, int32_t controller = NO_OWNER);
	C4Object *CreateObject(C4ID type, C4Object *creator, int32_t owner = NO_OWNER,
	                       int32_t x = 50, int32_t y = 50, int32_t r = 0, bool grow_from_center = false,
	                       C4Real xdir = Fix0, C4Real ydir = Fix0, C4Real rdir = Fix0, int32_t controller = NO_OWNER);
	C4Object *CreateObjectConstruction(C4PropList * type,
	                                   C4Object *creator,
	                                   int32_t owner,
	                                   int32_t center_x = 0, int32_t bottom_y = 0,
	                                   int32_t con = 1, bool adjust_terrain = false);
	C4Object *CreateInfoObject(C4ObjectInfo *info, int32_t owner,
	                           int32_t x = 50, int32_t y = 50);
	C4Object *FindConstuctionSiteBlock(int32_t x, int32_t y, int32_t wdt, int32_t hgt);
	C4Object *FindObject(C4Def * def,
	                     int32_t x = 0, int32_t y = 0, int32_t wdt = 0, int32_t hgt = 0,
	                     DWORD ocf = OCF_All,
	                     C4Object *find_next = nullptr);
	C4Object *FindVisObject( // find object in view at pos, regarding parallaxity and visibility (but not distance)
	  float x, float y, int32_t player_nr, const C4Facet &viewport_game, const C4Facet &viewport_gui,
	  float game_x, float game_y,
	  DWORD category,
	  float gui_x, float gui_y);
	int32_t ObjectCount(C4ID id);
	void CastObjects(C4ID id, C4Object *creator, int32_t num, int32_t level, int32_t x, int32_t y, int32_t owner = NO_OWNER, int32_t controller = NO_OWNER, C4ValueArray *out_objects = nullptr);
	C4Object *PlaceVegetation(C4PropList *def, int32_t x, int32_t y, int32_t wdt, int32_t hgt, int32_t growth, C4PropList *shape_proplist, C4PropList * out_pos_proplist);
	C4Object *PlaceAnimal(C4PropList *def);
	C4Value GRBroadcast(const char *function, C4AulParSet *pars = nullptr, bool pass_error = false, bool reject_test = false);  // call function in scenario script and all goals/rules/environment objects

	bool LoadScenarioSection(const char *section_name, DWORD flags);
	bool CreateSectionFromTempFile(const char *section_name, const char *temp_filename);

	bool DrawTextSpecImage(C4Facet& target, const char *spec, class C4DrawTransform* transform, uint32_t color = 0xff);
	float GetTextSpecImageAspect(const char* spec);
	bool DrawPropListSpecImage(C4Facet& target, C4PropList *spec);
	bool SpeedUp();
	bool SlowDown();
	bool InitKeyboard(); // register main keyboard input functions
	void UpdateLanguage();
	bool InitPlayerControlSettings();
	bool InitPlayerControlUserSettings(); // merge player control default settings and config overloads into user setting
	void SetDefaultGamma();

	std::unique_ptr<C4ScriptGuiWindow> ScriptGuiRoot;
protected:
	void Default();
	void InitInEarth();
	void InitVegetation();
	void InitAnimals();
	void InitGoals();
	void InitRules();
	void InitValueOverloads();
	void InitEnvironment();
	void CloseScenario();
	void DeleteObjects(bool delete_inactive);
	void ExecObjects();
	void Ticks();
	bool CheckObjectEnumeration();
	bool LoadScenarioComponents();
public:
	bool LoadAdditionalSystemGroup(class C4Group &parent_group);
	bool SaveGameTitle(C4Group &hGroup);
protected:
	bool InitGame(C4Group &group, InitMode init_mode, bool load_sky, C4ValueNumbers *);
	bool InitGameFinal(InitMode init_mode);
	bool InitNetworkFromAddress(const char *address);
	bool InitNetworkFromReferenceFile(const char *temp_filename);
	bool InitNetworkFromReference(const C4Network2Reference &Reference);
	bool InitNetworkHost();
	bool InitControl();
	bool InitScriptEngine();
	bool LinkScriptEngine();
	bool ReLinkScriptEngine();
	bool InitPlayers(C4ValueNumbers *);
	bool OpenScenario();
	bool InitDefs();
	bool InitMaterialTexture();
	bool GameOverCheck();
	bool PlaceInEarth(C4ID id);
public:
	void CompileFunc(StdCompiler *compiler, CompileSettings settings, C4ValueNumbers *);
	bool SaveData(C4Group &group, bool save_section, bool save_exact, bool save_sync, C4ValueNumbers *);
protected:
	bool CompileRuntimeData(C4Group &group, InitMode init_mode, bool exact, bool sync, C4ValueNumbers *);

	// Object function internals
	C4Object *NewObject( C4PropList *def, C4Object *creator,
	                     int32_t owner, C4ObjectInfo *info,
	                     int32_t x, int32_t y, int32_t r,
	                     C4Real xdir, C4Real ydir, C4Real rdir,
						 int32_t con, int32_t controller, bool grow_from_center);
	void ClearObjectPtrs(C4Object *to_obj);
	void ObjectRemovalCheck();

	bool ToggleDebugMode(); // dbg modeon/off if allowed
	bool ActivateMenu(const char *command); // exec given menu command for first local player

public:
	bool ToggleChart(); // chart dlg on/off
	void SetGlobalSoundModifier(C4PropList *modifier_props);

	// Localized strings in editor props
	C4String *GetTranslatedString(const class C4Value &input_string, C4Value *selected_language, bool fail_silently) const;
	C4PropList *AllocateTranslatedString();

	static constexpr const char * DirectJoinFilePrefix = "file:";
};

extern C4Game         Game;

// a global wrapper
inline StdStrBuf GetKeyboardInputName(const char *key_name, bool abbreviated = false, int32_t index = 0)
{
	return Game.KeyboardInput.GetKeyCodeNameByKeyName(key_name, abbreviated, index);
}


#endif
