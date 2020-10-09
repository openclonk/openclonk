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

/* Game configuration as stored in registry */

#ifndef INC_C4Config
#define INC_C4Config

#include "config/C4Constants.h"
#include "lib/C4InputValidation.h"
#include "control/C4PlayerControl.h"

#define C4DEFAULT_FONT_NAME "Endeavour"
enum { CFG_MaxString  = 1024, CFG_MaxEditorMRU = 8 };


class C4ConfigGeneral
{
public:
	enum { ConfigResetSafetyVal = 42 };

	char Name[CFG_MaxString + 1];
	char Language[CFG_MaxString + 1]; // entered by user in frontend options (may contain comma separated list or long language descriptions)
	char LanguageEx[CFG_MaxString + 1]; // full fallback list composed by frontend options (condensed comma separated list)
	char Participants[CFG_MaxString + 1];
	int32_t  AlwaysDebug; // if set: turns on debugmode whenever engine is started
	int32_t  OpenScenarioInGameMode; // When the program arguments include a scenario path, open the game regularly
	char RXFontName[CFG_MaxString + 1];
	int32_t  RXFontSize;
	char ConfigUserPath[CFG_MaxString + 1];
	StdStrBuf ScreenshotFolder;
	StdStrBuf ModsFolder;
	char MissionAccess[CFG_MaxString+1];
	int32_t FPS;
	int32_t DefRec;
	int32_t MMTimer;  // use multimedia-timers
	int32_t ScrollSmooth; // view movement smoothing
	int32_t ConfigResetSafety; // safety value: If this value is screwed, the config got corrupted and must be reset
	// Determined at run-time
	StdCopyStrBuf ExePath;
	StdCopyStrBuf TempPath;
	char UserDataPath[CFG_MaxString + 1];
	char ModsDataPath[CFG_MaxString + 1];
	char SystemDataPath[CFG_MaxString + 1];
	char ScreenshotPath[CFG_MaxString + 1];
	char TempUpdatePath[CFG_MaxString + 1];
	bool GamepadEnabled;
	bool FirstStart;
	int32_t DebugRec;      // if defined, the external file is used for debugrec writing. Otherwise read/check
	int32_t DebugRecWrite; // if defined, an external file is used for debugrec writing (replays only)
	char DebugRecExternalFile[_MAX_PATH_LEN];

public:
	static int GetLanguageSequence(const char *source, char *target);
	void DefaultLanguage();
	bool CreateSaveFolder(const char *directory, const char *language_title);
	void DeterminePaths();
	void CompileFunc(StdCompiler *compiler);

private:
	struct
	{
		char PlayerPath[CFG_MaxString + 1];
	} Adopt;
};

class C4ConfigDeveloper
{
public:
	int32_t AutoFileReload;
	char TodoFilename[CFG_MaxString + 1];
	char AltTodoFilename[CFG_MaxString + 1];
	int32_t MaxScriptMRU; // maximum number of remembered elements in recently used scripts
	int32_t DebugShapeTextures; // if nonzero, show messages about loaded shape textures
	bool ShowHelp; // show help buttons and descriptions in editor
	char RecentlyEditedSzenarios[CFG_MaxEditorMRU][CFG_MaxString + 1];
	void CompileFunc(StdCompiler *compiler);
	void AddRecentlyEditedScenario(const char *filename);
};

class C4ConfigGraphics
{
public:
	int32_t SplitscreenDividers;
	int32_t ShowStartupMessages;
	int32_t VerboseObjectLoading;
	int32_t MenuTransparency;
	int32_t UpperBoard;
	int32_t ShowClock;
	int32_t ResX,ResY;
	int32_t WindowX,WindowY;
	int32_t RefreshRate;	// monitor vertical refresh rate
	int32_t Windowed; // 0: fullscreen, 1: windowed, 2: fullscreen in game, windowed in menu
	int32_t ShowCrewNames; // show player name above clonks?
	int32_t ShowCrewCNames; // show clonk names above clonks?
	int32_t PXSGfx;     // show PXS-graphics (instead of sole pixels)
	int32_t Gamma; // gamma value
	int32_t Monitor;    // monitor index to play on
	int32_t MaxRefreshDelay; // minimum time after which graphics should be refreshed (ms)
	int32_t NoOffscreenBlits; // if set, all blits to non-primary-surfaces are emulated
	int32_t MultiSampling; // multisampling samples
	int32_t AutoFrameSkip; // if true, gfx frames are skipped when they would slow down the game
	int32_t DebugOpenGL; // if true, enables OpenGL debugging
	int32_t MouseCursorSize; // size in pixels

	void CompileFunc(StdCompiler *compiler);
};

class C4ConfigSound
{
public:
	int32_t RXSound;
	int32_t RXMusic;
	int32_t FEMusic;
	int32_t FESamples;
	int32_t Verbose;  // show music files names
	int32_t MusicVolume;
	int32_t SoundVolume;
	void CompileFunc(StdCompiler *compiler);
};

class C4ConfigNetwork
{
public:
	int32_t ControlRate;
	int32_t ControlPreSend;
	int32_t Lobby;
	int32_t NoRuntimeJoin;
	int32_t NoReferenceRequest;
	int32_t MaxResSearchRecursion;
	char WorkPath[CFG_MaxString+1];
	ValidatedStdCopyStrBuf<C4InVal::VAL_Comment> Comment;
	int32_t MasterServerSignUp;
	int32_t MasterServerActive;
	int32_t MasterKeepPeriod;
	int32_t MasterReferencePeriod;
	int32_t LeagueServerSignUp;
	int32_t UseAlternateServer;
	int32_t PortTCP,PortUDP,PortDiscovery,PortRefServer;
	int32_t EnableUPnP;
	int32_t ControlMode;
	ValidatedStdCopyStrBuf<C4InVal::VAL_NameAllowEmpty> Nick;
	int32_t MaxLoadFileSize;
	char LastPassword[CFG_MaxString+1];
	char AlternateServerAddress[CFG_MaxString+1];
	char PuncherAddress[CFG_MaxString+1];
	StdCopyStrBuf LastLeagueServer, LastLeaguePlayerName, LastLeagueAccount, LastLeagueLoginToken;
	// For the mod database.
	int32_t UseAlternateModDatabaseServer;
	char AlternateModDatabaseServerAddress[CFG_MaxString + 1];
#ifdef WITH_AUTOMATIC_UPDATE
	char UpdateServerAddress[CFG_MaxString+1];
	int32_t AutomaticUpdate;
	int32_t LastUpdateTime;
#endif
	int32_t AsyncMaxWait;
	int32_t PacketLogging;
public:
	void CompileFunc(StdCompiler *compiler);
	const char *GetLeagueServerAddress();
	const char *GetModDatabaseServerAddress();
	void CheckPortsForCollisions();
	void SetLeagueLoginData(const char *server_name, const char *player_name, const char *account, const char *login_token);
	bool GetLeagueLoginData(const char *server_name, const char *player_name, StdStrBuf *account, StdStrBuf *login_token) const;
};

class C4ConfigStartup
{
public:
	// config for do-not-show-this-msg-again-messages
	int32_t HideMsgGfxEngineChange;
	int32_t HideMsgGfxBitDepthChange;
	int32_t HideMsgMMTimerChange;
	int32_t HideMsgStartDedicated;
	int32_t HideMsgPlrTakeOver;
	int32_t HideMsgPlrNoTakeOver;
	int32_t HideMsgNoOfficialLeague;
	int32_t HideMsgIRCDangerous;
	int32_t AlphabeticalSorting; // if set, Folder.txt-sorting is ignored in scenario selection
	int32_t LastPortraitFolderIdx;
	void CompileFunc(StdCompiler *compiler);
};

class C4ConfigLobby
{
public:
	int32_t CountdownTime;
	int32_t AllowPlayerSave; // whether save-to-disk function is enabled for player resources
	void CompileFunc(StdCompiler *compiler);
};

class C4ConfigIRC
{
public:
	char Server[CFG_MaxString + 1];
	char Nick[CFG_MaxString + 1];
	char RealName[CFG_MaxString + 1];
	char Channel[CFG_MaxString + 1];
	void CompileFunc(StdCompiler *compiler);
	int32_t AllowAllChannels;
};

const int C4ConfigMaxGamepads = 4;

class C4ConfigGamepad
{
public:
	int32_t Button[C4MaxKey];
	uint32_t AxisMin[6], AxisMax[6];
	bool AxisCalibrated[6];
	void CompileFunc(StdCompiler *compiler, bool buttons_only = false);
	void ResetButtons(); // reset all buttons to default
	void Reset(); // reset all buttons and axis calibration to default
};

class C4ConfigControls
{
public:
	int32_t GamepadGuiControl;
	int32_t MouseAutoScroll; // auto scroll strength
	C4PlayerControlAssignmentSets UserSets;

	void CompileFunc(StdCompiler *compiler);
	void ResetKeys(); // reset all keys to default
};

class C4ConfigSecurity
{
public:
	char KeyPath[CFG_MaxString + 1]; // absolute path; environment variables are stored and only expanded upon evaluation
	int32_t WasRegistered;
	void CompileFunc(StdCompiler *compiler);
};

class C4Config
{
public:
	C4Config();
	~C4Config();
public:
	C4ConfigGeneral   General;
	C4ConfigDeveloper Developer;
	C4ConfigGraphics  Graphics;
	C4ConfigSound     Sound;
	C4ConfigNetwork   Network;
	C4ConfigLobby     Lobby;
	C4ConfigIRC       IRC;
	C4ConfigGamepad   Gamepads[C4ConfigMaxGamepads];
	C4ConfigControls  Controls;
	C4ConfigStartup   Startup;
	C4ConfigSecurity  Security;
	bool ConfigLoaded; // true if config has been successfully loaded
	StdStrBuf ConfigFilename; // set for configs loaded from a nondefault config file
public:
	const char* GetSubkeyPath(const char *subkey);
	void Default();
	bool Save();
	bool Load(const char *config_file = nullptr);
	bool Init();
	bool Registered();
	const char *AtExePath(const char *filename);
	const char *AtTempPath(const char *filename);
	const char *AtTempUpdatePath(const char *filename);
	const char *AtNetworkPath(const char *filename);
	const char *AtScreenshotPath(const char *filename);
	const char *AtUserDataPath(const char *filename);
	const char *AtUserDataRelativePath(const char *filename);
	const char *AtSystemDataPath(const char *filename);
	const char *AtSystemDataRelativePath(const char *filename);
	const char *AtRelativePath(const char *filename); // Returns ASDRP or AUDRP depending on location
	const char *GetRegistrationData(const char* field) { return ""; }
	void ForceRelativePath(StdStrBuf *filename); // try AtRelativePath; force GetC4Filename if not possible
	void CompileFunc(StdCompiler *compiler);
	bool IsCorrupted() { return (General.ConfigResetSafety != C4ConfigGeneral::ConfigResetSafetyVal) || !Graphics.ResX; }
	bool RemoveModule(const char *path, char *modules);
	bool IsModule(const char *path, char *modules);
	bool AddModule(const char *path, char *modules);
	void GetConfigFileName(StdStrBuf &filename, const char *config_file);
	void CleanupTempUpdateFolder();
	const char *MakeTempUpdateFolder();

	static void ExpandEnvironmentVariables(char *path, size_t max_length);
};

extern C4Config Config;

#endif // INC_C4Config
