/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2007  Matthes Bender
 * Copyright (c) 2001, 2004, 2006-2007  Sven Eberhardt
 * Copyright (c) 2005  Peter Wortmann
 * Copyright (c) 2006  Günther Brammer
 * Copyright (c) 2009  Nicolas Hake
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

/* Game configuration as stored in registry */

#ifndef INC_C4Config
#define INC_C4Config

#include "StdConfig.h"
#include "C4Constants.h"
#include "C4InputValidation.h"
#include <list>

const char *CfgAtTempPath(const char *szFilename);

class C4ConfigGeneral
	{
	public:
		enum { ConfigResetSafetyVal = 42 };

		char Name[CFG_MaxString+1];
		char Language[CFG_MaxString+1]; // entered by user in frontend options (may contain comma separated list or long language descriptions)
		char LanguageEx[CFG_MaxString+1]; // full fallback list composed by frontend options (condensed comma separated list)
		char Definitions[CFG_MaxString+1];
		char Participants[CFG_MaxString+1];
		int32_t  AlwaysDebug; // if set: turns on debugmode whenever engine is started
		char RXFontName[CFG_MaxString+1];
		int32_t	 RXFontSize;
		char ConfigUserPath[CFG_MaxString + 1];
		StdStrBuf SaveGameFolder;
		StdStrBuf SaveDemoFolder;
		StdStrBuf ScreenshotFolder;
		char MissionAccess[CFG_MaxString+1];
		char UpdateEngine[CFG_MaxString+1];
		char UpdateObjects[CFG_MaxString+1];
		char UpdateMajor[CFG_MaxString+1];
		int32_t FPS;
		int32_t Record;
		int32_t DefRec;
		int32_t MMTimer;	// use multimedia-timers
		int32_t FairCrew;   // don't use permanent crew physicals
		int32_t FairCrewStrength, MaxFairCrewStrength; // strength of clonks in fair crew mode
		int32_t ScrollSmooth; // view movement smoothing
		int32_t ConfigResetSafety; // safety value: If this value is screwed, the config got currupted and must be reset
		// Determined at run-time
		char ExePath[CFG_MaxString+1];
		char TempPath[CFG_MaxString+1];
		char UserDataPath[CFG_MaxString+1];
		char SystemDataPath[CFG_MaxString+1];
		char ScreenshotPath[CFG_MaxString+1];
		char BetaCode[CFG_MaxString+1];
		bool GamepadEnabled;
		bool FirstStart;
		bool UserPortraitsWritten; // set when default portraits have been copied to the UserPath (this is only done once)

		// Additional paths to read data from (prioritized lower than UserDataPath/SystemDataPath)
		typedef std::list<const char *> PathList;
		PathList AdditionalDataPaths;
	public:
		static int GetLanguageSequence(const char *strSource, char *strTarget);
		void DefaultLanguage();
		bool CreateSaveFolder(const char *strDirectory, const char *strLanguageTitle);
		void AdoptOldSettings();
		void DeterminePaths(bool forceWorkingDirectory);
		void CompileFunc(StdCompiler *pComp);
		void AddAdditionalDataPath(const char *szPath);
		void ClearAdditionalDataPaths();
		~C4ConfigGeneral() { ClearAdditionalDataPaths(); }

	private:
		struct
			{
			char PlayerPath[CFG_MaxString+1];
			} Adopt;
	};

class C4ConfigDeveloper
	{
	public:
		int32_t AutoFileReload;
#ifdef _WIN32
		int32_t AutoEditScan;
#endif
		int32_t AllErrorsFatal;
		void CompileFunc(StdCompiler *pComp);
	};

class C4ConfigGraphics
	{
	public:
		int32_t SplitscreenDividers;
		int32_t AddNewCrewPortraits;
		int32_t SaveDefaultPortraits;
		int32_t ShowStartupMessages;
		int32_t VerboseObjectLoading;
		int32_t ColorAnimation;
		int32_t HighResLandscape;
		int32_t SmokeLevel;
		int32_t VideoModule;
		int32_t MenuTransparency;
		int32_t UpperBoard;
		int32_t ShowClock;
		int32_t ResX,ResY;
		int32_t GuiResX,GuiResY;
		int32_t Windowed;
		int32_t ShowAllResolutions;
		int32_t ShowCrewNames; // show player name above clonks?
		int32_t ShowCrewCNames; // show clonk names above clonks?
		int32_t BitDepth; // used bit depth for newgfx
		int32_t MsgBoard;
		int32_t MsgBrdFadeDelay;
		int32_t PXSGfx;			// show PXS-graphics (instead of sole pixels)
		int32_t Engine;			// 0: D3D; 1: OpenGL;
		int32_t Gamma1, Gamma2, Gamma3; // gamma ramps
		int32_t Currency;		// default wealth symbolseb
		int32_t RenderInactiveEM; // draw vieports even if inactive in CPEM
		int32_t DisableGamma;
		int32_t Monitor;    // monitor index to play on
		int32_t FireParticles; // draw extended fire particles if enabled (default on)
		int32_t MaxRefreshDelay; // minimum time after which graphics should be refreshed (ms)
		int32_t EnableShaders; // enable pixel shaders on engines that support them
		void CompileFunc(StdCompiler *pComp);
		void ApplyResolutionConstraints();
	};

class C4ConfigSound
	{
	public:
    int32_t RXSound;
		int32_t RXMusic;
		int32_t FEMusic;
		int32_t FESamples;
		int32_t FMMode;
		int32_t Verbose;  // show music files names
		int32_t MusicVolume;
		int32_t SoundVolume;
		void CompileFunc(StdCompiler *pComp);
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
		int32_t SendPortraits;
		int32_t PortTCP,PortUDP,PortDiscovery,PortRefServer;
		int32_t ControlMode;
		ValidatedStdCopyStrBuf<C4InVal::VAL_NameNoEmpty> LocalName;
		ValidatedStdCopyStrBuf<C4InVal::VAL_NameAllowEmpty> Nick;
		int32_t MaxLoadFileSize;
		char LastPassword[CFG_MaxString+1];
		char AlternateServerAddress[CFG_MaxString+1];
		char UpdateServerAddress[CFG_MaxString+1];
		char PuncherAddress[CFG_MaxString+1];
		int32_t AutomaticUpdate;
		int32_t LastUpdateTime;
		int32_t AsyncMaxWait;
	public:
		void CompileFunc(StdCompiler *pComp);
		const char *GetLeagueServerAddress();
		void CheckPortsForCollisions();
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
		int32_t NoSplash;
		int32_t AlphabeticalSorting; // if set, Folder.txt-sorting is ignored in scenario selection
		int32_t LastPortraitFolderIdx;
		void CompileFunc(StdCompiler *pComp);
	};

class C4ConfigLobby
	{
	public:
		int32_t CountdownTime;
		int32_t AllowPlayerSave; // whether save-to-disk function is enabled for player ressources
		void CompileFunc(StdCompiler *pComp);
	};

class C4ConfigIRC
	{
	public:
		char Server[CFG_MaxString+1];
		char Nick[CFG_MaxString+1];
		char RealName[CFG_MaxString+1];
		char Channel[CFG_MaxString+1];
		void CompileFunc(StdCompiler *pComp);
		int32_t AllowAllChannels;
	};

class C4ConfigExplorer
	{
	public:
		int32_t Mode;
		int32_t Run;
    char Definitions[CFG_MaxString+1];
		char Engines[CFG_MaxString+1];
    char EditorBitmap[CFG_MaxString+1];
    char EditorPNG[CFG_MaxString+1];
    char EditorMusic[CFG_MaxString+1];
    char EditorRichText[CFG_MaxString+1];
    char EditorScript[CFG_MaxString+1];
    char EditorText[CFG_MaxString+1];
    char EditorSound[CFG_MaxString+1];
    char EditorZip[CFG_MaxString+1];
    char EditorDefinition[CFG_MaxString+1];
    char EditorHtml[CFG_MaxString+1];
		char CommandLine[CFG_MaxString+1];
		int32_t EditorUseShell;
		int32_t Kindersicherung;
		int32_t DontShowLanguageDisclaimer;
		int32_t IDListSortColumn;
		int32_t IDListViewMode;
		int32_t LicenseAccepted;
		char Reload[CFG_MaxString+1];
	public:
		void CompileFunc(StdCompiler *pComp);
	};

const int C4ConfigMaxGamepads = 4;

class C4ConfigGamepad
	{
	public:
		int32_t Button[C4MaxKey];
		uint32_t AxisMin[6], AxisMax[6];
		bool AxisCalibrated[6];
		void CompileFunc(StdCompiler *pComp, bool fButtonsOnly=false);
		void ResetButtons(); // reset all buttons to default
		void Reset(); // reset all buttons and axis calibration to default
	};

class C4ConfigControls
	{
	public:
		int32_t GamepadGuiControl;
		int32_t MouseAScroll; // auto scroll strength
    int32_t Keyboard[C4MaxKeyboardSet][C4MaxKey];
		void CompileFunc(StdCompiler *pComp, bool fKeysOnly=false);
		void ResetKeys(); // reset all keys to default
	};

class C4ConfigSecurity
{
	public:
		char KeyPath[CFG_MaxString + 1]; // absolute path; environment variables are stored and only expanded upon evaluation
		int32_t WasRegistered;
		void CompileFunc(StdCompiler *pComp);
};

class C4Config: protected CStdConfig
	{
	public:
		C4Config();
		~C4Config();
	public:
		C4ConfigGeneral		General;
		C4ConfigDeveloper	Developer;
		C4ConfigGraphics	Graphics;
		C4ConfigSound			Sound;
		C4ConfigNetwork		Network;
		C4ConfigLobby     Lobby;
		C4ConfigIRC       IRC;
		C4ConfigExplorer	Explorer;
		C4ConfigGamepad		Gamepads[C4ConfigMaxGamepads];
		C4ConfigControls	Controls;
		C4ConfigStartup   Startup;
		C4ConfigSecurity  Security;
		bool fConfigLoaded; // true if config has been successfully loaded
		StdStrBuf ConfigFilename; // set for configs loaded from a nondefault config file
	public:
		const char* GetSubkeyPath(const char *strSubkey);
		void Default();
		bool Save();
		bool Load(bool forceWorkingDirectory = true, const char *szConfigFile = NULL);
		bool Init();
		bool Registered();
		const char *AtExePath(const char *szFilename);
		const char *AtTempPath(const char *szFilename);
		const char *AtNetworkPath(const char *szFilename);
		const char *AtScreenshotPath(const char *szFilename);
		const char *AtUserDataPath(const char *szFilename);
		const char *AtUserDataRelativePath(const char *szFilename);
		const char *AtSystemDataPath(const char *szFilename);
		const char *AtSystemDataRelativePath(const char *szFilename);
		const char *AtRelativePath(const char *szFilename); // Returns ASDRP or AUDRP depending on location
		const char *AtDataReadPath(const char *szFilename, bool fPreferWorkdir = false);
		const char *GetRegistrationData(const char* strField) { return ""; }
		void ForceRelativePath(StdStrBuf *sFilename); // try AtRelativePath; force GetC4Filename if not possible
		void CompileFunc(StdCompiler *pComp);
		bool IsCorrupted() { return (General.ConfigResetSafety != C4ConfigGeneral::ConfigResetSafetyVal) || !Graphics.ResX; }
		bool RemoveModule(const char *szPath, char *szModules);
		bool IsModule(const char *szPath, char *szModules);
		bool AddModule(const char *szPath, char *szModules);

		static void ExpandEnvironmentVariables(char *strPath, size_t iMaxLen);
	private:
		const char *AtDataReadPathCore(const char *szFilename, bool fPreferWorkdir = false);
	};

#include <C4ConfigShareware.h>

extern C4Config *pConfig; // Some cheap temporary hack to get to config in Engine + Frontend...
// This is so good - we can use it everywear!!!!
#endif // INC_C4Config
