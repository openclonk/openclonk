/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2004-2005, 2007-2008  Matthes Bender
 * Copyright (c) 2004-2008  Sven Eberhardt
 * Copyright (c) 2005-2006, 2009  Peter Wortmann
 * Copyright (c) 2005-2006, 2008-2009  Günther Brammer
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

/* Main class to initialize configuration and execute the game */

#include <C4Include.h>
#include <C4Application.h>
#include <C4Version.h>
#ifdef _WIN32
#include <StdRegistry.h>
#include <C4UpdateDlg.h>
#endif

#include "C4Game.h"
#include "C4GraphicsSystem.h"
#include "C4GraphicsResource.h"
#include "C4MessageInput.h"
#include <C4FileClasses.h>
#include <C4FullScreen.h>
#include <C4Language.h>
#include <C4Console.h>
#include <C4Startup.h>
#include <C4Log.h>
#include <C4GamePadCon.h>
#include <C4GameLobby.h>

#include <StdRegistry.h> // For DDraw emulation warning


C4Application::C4Application():
	isFullScreen(true),
	UseStartupDialog(true),
	CheckForUpdates(false),
	NoSplash(false),
	launchEditor(false),
	restartAtEnd(false),
	pGamePadControl(NULL),
	DDraw(NULL), AppState(C4AS_None)
	{
	}

C4Application::~C4Application()
	{
	// clear gamepad
	if (pGamePadControl) delete pGamePadControl;
	// Close log
	CloseLog();
	// Launch editor
	if (launchEditor)
		{
#ifdef _WIN32
		char strCommandLine[_MAX_PATH + 1]; SCopy(Config.AtExePath(C4CFN_Editor), strCommandLine);
		STARTUPINFO StartupInfo; ZeroMemory(&StartupInfo, sizeof StartupInfo);
		StartupInfo.cb = sizeof StartupInfo;
		PROCESS_INFORMATION ProcessInfo; ZeroMemory(&ProcessInfo, sizeof ProcessInfo);
		CreateProcess(NULL, strCommandLine, NULL, NULL, true, 0, NULL, NULL, &StartupInfo, &ProcessInfo);
#endif
		}
	}

bool C4Application::DoInit()
	{
	assert(AppState == C4AS_None);
	// Config overwrite by parameter
	StdStrBuf sConfigFilename;
  char szParameter[_MAX_PATH+1];
	for (int32_t iPar=0; SGetParameter(GetCommandLine(), iPar, szParameter, _MAX_PATH); iPar++)
		if (SEqual2NoCase(szParameter, "/config:"))
			sConfigFilename.Copy(szParameter + 8);
	// Config check
	Config.Init();
	Config.Load(true, sConfigFilename.getData());
	Config.Save();
	// sometimes, the configuration can become corrupted due to loading errors or w/e
	// check this and reset defaults if necessary
	if (Config.IsCorrupted())
		{
		if (sConfigFilename)
			{
			// custom config corrupted: Fail
			Log("Warning: Custom configuration corrupted - program abort!\n");
			return false;
			}
		else
			{
			// default config corrupted: Restore default
			Log("Warning: Configuration corrupted - restoring default!\n");
			Config.Default();
			Config.Save();
			Config.Load();
			}
		}
	// Init C4Group
	C4Group_SetMaker(Config.General.Name);
	C4Group_SetProcessCallback(&ProcessCallback);
	C4Group_SetTempPath(Config.General.TempPath);
	C4Group_SetSortList(C4CFN_FLS);

	// Open log
	OpenLog();

	// init system group
	if (!SystemGroup.Open(C4CFN_System))
		{
		// Error opening system group - no LogFatal, because it needs language table.
		// This will *not* use the FatalErrors stack, but this will cause the game
		// to instantly halt, anyway.
		const char *szMessage = "Error opening system group file (System.c4g)!";
		Log(szMessage);
		// Fatal error, game cannot start - have player notice
		MessageDialog(szMessage);
		return false;
		}

	// Language override by parameter
	const char *pLanguage;
	if ((pLanguage = SSearchNoCase(GetCommandLine(), "/Language:")))
		SCopyUntil(pLanguage, Config.General.LanguageEx, ' ', CFG_MaxString);

	// Init external language packs
	Languages.Init();
	// Load language string table
	if (!Languages.LoadLanguage(Config.General.LanguageEx))
		// No language table was loaded - bad luck...
		if (!IsResStrTableLoaded())
			Log("WARNING: No language string table loaded!");

	// Set unregistered user name
	if (!Config.Registered())
		C4Group_SetMaker(LoadResStr("IDS_PRC_UNREGUSER"));

	// Parse command line
	Game.ParseCommandLine(GetCommandLine());

#ifdef WIN32
	// Windows: handle incoming updates directly, even before starting up the gui
	//          because updates will be applied in the console anyway.
	if (Application.IncomingUpdate)
		if (C4UpdateDlg::ApplyUpdate(Application.IncomingUpdate.getData(), false, NULL))
			return true;
#endif

	DDrawCfg.Shader = !!Config.Graphics.EnableShaders;

	// Fixup resolution
	ApplyResolutionConstraints();

	// activate
	Active=true;

	// Init carrier window
	if (isFullScreen)
		{
		if (!(pWindow = FullScreen.Init(this)))
			{ Clear(); return false; }
		}
	else
		{
		if (!(pWindow = Console.Init(this)))
			{ Clear(); return false; }
		}

	// init timers (needs window)
	Add(pGameTimer = new C4ApplicationGameTimer());

	// Engine header message
	Log(C4ENGINEINFOLONG);
	LogF("Version: %s %s", C4VERSION, C4_OS);

#if defined(USE_DIRECTX) && defined(_WIN32)
	// DDraw emulation warning
	DWORD DDrawEmulationState;
	if (GetRegistryDWord(HKEY_LOCAL_MACHINE,"Software\\Microsoft\\DirectDraw","EmulationOnly",&DDrawEmulationState))
		if (DDrawEmulationState)
			Log("WARNING: DDraw Software emulation is activated!");
#endif
	// Initialize D3D/OpenGL
	DDraw = DDrawInit(this, isFullScreen, false, Config.Graphics.ResX, Config.Graphics.ResY, Config.Graphics.BitDepth, Config.Graphics.Engine, Config.Graphics.Monitor);
	if (!DDraw) { LogFatal(LoadResStr("IDS_ERR_DDRAW")); Clear(); return false; }
	
	if (isFullScreen)
		{
		if (!SetVideoMode(Config.Graphics.ResX, Config.Graphics.ResY, Config.Graphics.BitDepth, Config.Graphics.Monitor, !Config.Graphics.Windowed))
			pWindow->SetSize(Config.Graphics.ResX, Config.Graphics.ResY);
		}

#if defined(_WIN32) && !defined(USE_CONSOLE)
	// Register clonk file classes - notice: under Vista this will only work if we have administrator rights
	char szModule[_MAX_PATH+1]; GetModuleFileName(NULL, szModule, _MAX_PATH);
	SetC4FileClasses(szModule);
#endif

	// Initialize gamepad
	if (!pGamePadControl && Config.General.GamepadEnabled)
		pGamePadControl = new C4GamePadControl();

	AppState = C4AS_PreInit;

	return true;
	}


void C4Application::ApplyResolutionConstraints()
{
	// Enumerate display modes
	int32_t idx = 0, iXRes, iYRes, iBitDepth;
	int32_t best_match = -1;
	uint32_t best_delta = ~0;
	int32_t ResX = Config.Graphics.ResX, ResY = Config.Graphics.ResY, BitDepth = Config.Graphics.BitDepth;
	while (GetIndexedDisplayMode(idx++, &iXRes, &iYRes, &iBitDepth, Config.Graphics.Monitor))
	{
		uint32_t delta = std::abs(ResX*ResY - iXRes*iYRes);
		if (!delta && iBitDepth == BitDepth)
			return; // Exactly the expected mode
		if (delta < best_delta)
		{
			// Better match than before
			best_match = idx;
			best_delta = delta;
		}
	}
	if (best_match != -1)
	{
		// Apply next-best mode
		GetIndexedDisplayMode(best_match, &iXRes, &iYRes, &iBitDepth, Config.Graphics.Monitor);
		if (iXRes != ResX || iYRes != ResY)
			// Don't warn if only bit depth changes
			// Also, lang table not loaded yet
			LogF("Warning: The selected resolution %dx%d is not available and has been changed to %dx%d.", ResX, ResY, iXRes, iYRes);
		ResX = iXRes; ResY = iYRes;
	}
}

bool C4Application::PreInit()
	{
	if (!Game.PreInit()) return false;

	// startup dialog: Only use if no next mission has been provided
	bool fDoUseStartupDialog = UseStartupDialog && !*Game.ScenarioFilename;

	// init loader: Black screen for first start if a video is to be shown; otherwise default spec
	if (fDoUseStartupDialog)
		{
		//Log(LoadResStr("IDS_PRC_INITLOADER"));
		bool fUseBlackScreenLoader = UseStartupDialog && !C4Startup::WasFirstRun() && !Config.Startup.NoSplash && !NoSplash && FileExists(C4CFN_Splash);
		if (!::GraphicsSystem.InitLoaderScreen(C4CFN_StartupBackgroundMain, fUseBlackScreenLoader))
			{ LogFatal(LoadResStr("IDS_PRC_ERRLOADER")); return false; }
		}

	Game.SetInitProgress(fDoUseStartupDialog ? 10.0f : 1.0f);

	// Music
	if (!MusicSystem.Init("Frontend.*"))
		Log(LoadResStr("IDS_PRC_NOMUSIC"));

	Game.SetInitProgress(fDoUseStartupDialog ? 20.0f : 2.0f);

	// Sound
	if (!SoundSystem.Init())
		Log(LoadResStr("IDS_PRC_NOSND"));

	Game.SetInitProgress(fDoUseStartupDialog ? 30.0f : 3.0f);

	AppState = fDoUseStartupDialog ? C4AS_Startup : C4AS_StartGame;

	return true;
	}

bool C4Application::ProcessCallback(const char *szMessage, int iProcess)
	{
	Console.Out(szMessage);
	return true;
	}

void C4Application::Clear()
	{
	Game.Clear();
	NextMission.Clear();
	// stop timer
	Remove(pGameTimer);
	delete pGameTimer; pGameTimer = NULL;
	// close system group (System.c4g)
	SystemGroup.Close();
	// Log
	if (IsResStrTableLoaded()) // Avoid (double and undefined) message on (second?) shutdown...
		Log(LoadResStr("IDS_PRC_DEINIT"));
	// Clear external language packs and string table
	Languages.Clear();
	Languages.ClearLanguage();
	// gamepad clear
	if (pGamePadControl) { delete pGamePadControl; pGamePadControl=NULL; }
	// music system clear
	MusicSystem.Clear();
	SoundSystem.Clear();
	RestoreVideoMode();
	// Clear direct draw (late, because it's needed for e.g. Log)
	if (DDraw) { delete DDraw; DDraw=NULL; }
	// Close window
	FullScreen.Clear();
	Console.Clear();
	// The very final stuff
	CStdApp::Clear();
	}

bool C4Application::OpenGame()
	{
	if (isFullScreen)
		{
		// Open game
		return Game.Init();
		}
	else
		{
		// Execute command line
		if (Game.ScenarioFilename[0] || Game.DirectJoinAddress[0])
			return Console.OpenGame(szCmdLine);
		}
	// done; success
	return true;
	}

void C4Application::Quit()
	{
	// Clear definitions passed by frontend for this round
	Config.General.Definitions[0] = 0;
	// Participants should not be cleared for usual startup dialog
	//Config.General.Participants[0] = 0;
	// Save config if there was no loading error
	if (Config.fConfigLoaded) Config.Save();
	// make sure startup data is unloaded
	C4Startup::Unload();
	// fonts are loaded at start and never unloaded
	::GraphicsResource.ClearFonts();
	// quit app
	CStdApp::Quit();
	AppState = C4AS_Quit;
	}

void C4Application::QuitGame()
	{
	// reinit desired? Do restart
	if (UseStartupDialog || NextMission)
		{
		// backup last start params
		bool fWasNetworkActive = Game.NetworkActive;
		// stop game
		Game.Clear();
		Game.Default();
		AppState = C4AS_PreInit;
		// if a next mission is desired, set to start it
		if (NextMission)
			{
			SCopy(NextMission.getData(), Game.ScenarioFilename, _MAX_PATH);
			SReplaceChar(Game.ScenarioFilename, '\\', DirSep[0]); // linux/mac: make sure we are using forward slashes
			Game.fLobby = Game.NetworkActive = fWasNetworkActive;
			Game.fObserve = false;
			Game.Record = !!Config.General.Record;
			NextMission.Clear();
			}
		}
	else
		{
		Quit();
		}
	}

void C4Application::GameTick()
	{
	// Exec depending on game state
	switch (AppState)
		{
		case C4AS_None:
			assert(AppState != C4AS_None);
			break;
		case C4AS_Quit:
			// Do nothing, the main loop will exit soon
			break;
		case C4AS_PreInit:
			if (!PreInit()) Quit();
			break;
		case C4AS_Startup:
			AppState = C4AS_Game;
			// if no scenario or direct join has been specified, get game startup parameters by startup dialog
			Game.ScenarioTitle.Copy(LoadResStr("IDS_PRC_INITIALIZE"));
			if (!C4Startup::Execute()) { Quit(); return; }
			AppState = C4AS_StartGame;
			break;
		case C4AS_StartGame:
			// immediate progress to next state; OpenGame will enter HandleMessage-loops in startup and lobby!
			AppState = C4AS_Game;
			// first-time game initialization
			if (!OpenGame())
				{
				// set error flag (unless this was a lobby user abort)
				if (!C4GameLobby::UserAbort)
					Game.fQuitWithError = true;
				// no start: Regular QuitGame; this may reset the engine to startup mode if desired
				QuitGame();
				}
			break;
		case C4AS_Game:
			// Game
			if (Game.IsRunning)
				Game.Execute();
			Game.DoSkipFrame = false;
			// Sound
			SoundSystem.Execute();
			// Gamepad
			if (pGamePadControl) pGamePadControl->Execute();
			break;
		}
	}

void C4Application::Draw()
	{
	// Graphics
	if(!Game.DoSkipFrame)
		{
		// Fullscreen mode
		if (isFullScreen)
			FullScreen.Execute();
		// Console mode
		else
			Console.Execute();
		}
	}

void C4Application::SetGameTickDelay(int iDelay)
	{
	if(!pGameTimer) return;
	pGameTimer->SetGameTickDelay(iDelay);
	}

void C4Application::OnResolutionChanged(unsigned int iXRes, unsigned int iYRes)
	{
	// notify game
	if (DDraw)
		{
		Game.OnResolutionChanged(iXRes, iYRes);
		DDraw->OnResolutionChanged(iXRes, iYRes);
		}
	}

bool C4Application::SetGameFont(const char *szFontFace, int32_t iFontSize)
	{
#ifndef USE_CONSOLE
	// safety
	if (!szFontFace || !*szFontFace || iFontSize<1 || SLen(szFontFace)>=static_cast<int>(sizeof Config.General.RXFontName)) return false;
	// first, check if the selected font can be created at all
	// check regular font only - there's no reason why the other fonts couldn't be created
	CStdFont TestFont;
	if (!Game.FontLoader.InitFont(TestFont, szFontFace, C4FontLoader::C4FT_Main, iFontSize, &::GraphicsResource.Files))
		return false;
	// OK; reinit all fonts
	StdStrBuf sOldFont; sOldFont.Copy(Config.General.RXFontName);
	int32_t iOldFontSize = Config.General.RXFontSize;
	SCopy(szFontFace, Config.General.RXFontName);
	Config.General.RXFontSize = iFontSize;
	if (!::GraphicsResource.InitFonts() || !C4Startup::Get()->Graphics.InitFonts())
		{
		// failed :o
		// shouldn't happen. Better restore config.
		SCopy(sOldFont.getData(), Config.General.RXFontName);
		Config.General.RXFontSize = iOldFontSize;
		return false;
		}
#endif
	// save changes
	return true;
	}

void C4Application::OnCommand(const char *szCmd)
	{
	// reroute to whatever seems to take commands at the moment
	if(AppState == C4AS_Game)
		::MessageInput.ProcessInput(szCmd);
	}

void C4Application::Activate()
	{
#ifdef WIN32
	// Activate the application to regain focus if it has been lost during loading.
	// As this is officially not possible any more in new versions of Windows
	// (BringWindowTopTop alone won't have any effect if the calling process is
	// not in the foreground itself), we are using an ugly OS hack.
  DWORD nForeThread = GetWindowThreadProcessId(GetForegroundWindow(), 0);
  DWORD nAppThread = GetCurrentThreadId();
	if (nForeThread != nAppThread)
		{
    AttachThreadInput(nForeThread, nAppThread, true);
    BringWindowToTop(FullScreen.hWindow);
    ShowWindow(FullScreen.hWindow, SW_SHOW);
    AttachThreadInput(nForeThread, nAppThread, false);
		}
  else
		{
    BringWindowToTop(FullScreen.hWindow);
    ShowWindow(FullScreen.hWindow, SW_SHOW);
		}
#endif
	}

void C4Application::SetNextMission(const char *szMissionFilename)
	{
	// set next mission if any is desired
	if (szMissionFilename)
		NextMission.Copy(szMissionFilename);
	else
		NextMission.Clear();
	}

void C4Application::NextTick()
	{
	if(!pGameTimer) return;
	pGameTimer->Set();
	}

// *** C4ApplicationGameTimer

C4ApplicationGameTimer::C4ApplicationGameTimer()
	: CStdMultimediaTimerProc(26),
		iLastGameTick(0), iGameTickDelay(0)
	{
	}

void C4ApplicationGameTimer::SetGameTickDelay(uint32_t iDelay)
	{
	// Smaller than minimum refresh delay?
	if (iDelay < uint32_t(Config.Graphics.MaxRefreshDelay))
		{
		// Set critical timer
		SetDelay(iDelay);
		// No additional breaking needed
		iGameTickDelay = 0;
		}
	else
		{
		// Do some magic to get as near as possible to the requested delay
		int iGraphDelay = Max<uint32_t>(1, iDelay);
		iGraphDelay /= (iGraphDelay + Config.Graphics.MaxRefreshDelay - 1) / Config.Graphics.MaxRefreshDelay;
		// Set critical timer
		SetDelay(iGraphDelay);
		// Slow down game tick
		iGameTickDelay = iDelay - iGraphDelay / 2;
		}
	}

bool C4ApplicationGameTimer::Execute(int iTimeout, pollfd *)
	{
	// Check timer and reset
	if (!CheckAndReset()) return true;
	unsigned int Now = timeGetTime();
	// Execute
	if(Now >= iLastGameTick + iGameTickDelay || Game.GameGo)
		{
		iLastGameTick += iGameTickDelay;
		// Compensate if things get too slow
		if (Now >= iLastGameTick + iGameTickDelay)
			iLastGameTick += (Now - iLastGameTick) / 2;
		Application.GameTick();
		}
	// Draw always
	Application.Draw();
	return true;
	}

