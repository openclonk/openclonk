/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2004-2005, 2007-2008  Matthes Bender
 * Copyright (c) 2004-2008  Sven Eberhardt
 * Copyright (c) 2005-2006, 2008-2011  Günther Brammer
 * Copyright (c) 2005-2006, 2009  Peter Wortmann
 * Copyright (c) 2009, 2011  Nicolas Hake
 * Copyright (c) 2010  Benjamin Herr
 * Copyright (c) 2011  Armin Burgmeier
 * Copyright (c) 2011  Julius Michaelis
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
#include <C4FullScreen.h>
#include <C4Language.h>
#include <C4Console.h>
#include <C4Startup.h>
#include <C4Log.h>
#include <C4GamePadCon.h>
#include <C4GameLobby.h>
#include <C4Fonts.h>
#include <C4Network2.h>
#include <C4Network2IRC.h>

#include <StdRegistry.h> // For DDraw emulation warning

#include <getopt.h>

static C4Network2IRCClient ApplicationIRCClient;

C4Application::C4Application():
		isEditor(false),
		IRCClient(ApplicationIRCClient),
		QuitAfterGame(false),
		CheckForUpdates(false),
		NoSplash(false),
		restartAtEnd(false),
		pGamePadControl(NULL),
		AppState(C4AS_None)
{
}

C4Application::~C4Application()
{
	// clear gamepad
	if (pGamePadControl) delete pGamePadControl;
	// Close log
	CloseLog();
}

bool C4Application::DoInit(int argc, char * argv[])
{
	assert(AppState == C4AS_None);
	// Config overwrite by parameter
	StdStrBuf sConfigFilename;
	for (int32_t iPar=0; iPar < argc; iPar++)
		if (SEqual2NoCase(argv[iPar], "--config="))
			sConfigFilename.Copy(argv[iPar] + 9);
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
			Log("ERROR: Custom configuration corrupted - program abort!\n");
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
	C4Group_SetProcessCallback(&ProcessCallback);
	C4Group_SetTempPath(Config.General.TempPath.getData());
	C4Group_SetSortList(C4CFN_FLS);

	// Open log
	OpenLog();

	Revision.Ref(C4REVISION);

	// Initialize game data paths
	Reloc.Init();

	// init system group
	if (!Reloc.Open(SystemGroup, C4CFN_System))
	{
		// Error opening system group - no LogFatal, because it needs language table.
		// This will *not* use the FatalErrors stack, but this will cause the game
		// to instantly halt, anyway.
		const char *szMessage = "Error opening system group file (System.ocg)!";
		Log(szMessage);
		// Fatal error, game cannot start - have player notice
		MessageDialog(szMessage);
		return false;
	}
	// Parse command line
	ParseCommandLine(argc, argv);

	// Init external language packs
	Languages.Init();
	// Load language string table
	if (!Languages.LoadLanguage(Config.General.LanguageEx))
		// No language table was loaded - bad luck...
		if (!IsResStrTableLoaded())
			Log("WARNING: No language string table loaded!");

#ifdef WIN32
	// Windows: handle incoming updates directly, even before starting up the gui
	//          because updates will be applied in the console anyway.
	if (Application.IncomingUpdate)
		if (C4UpdateDlg::ApplyUpdate(Application.IncomingUpdate.getData(), false, NULL))
			return true;
#endif

	// Fixup resolution
	if (!Config.Graphics.Windowed)
		ApplyResolutionConstraints();

	// activate
	Active=true;

	// Init carrier window
	if (!isEditor)
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
	LogF("Version: %s %s (%s)", C4VERSION, C4_OS, Revision.getData());

	// Initialize D3D/OpenGL
	bool success = DDrawInit(this, isEditor, false, Config.Graphics.ResX, Config.Graphics.ResY, Config.Graphics.BitDepth, Config.Graphics.Engine, Config.Graphics.Monitor);
	if (!success) { LogFatal(LoadResStr("IDS_ERR_DDRAW")); Clear(); return false; }

	if (!isEditor)
	{
		if (!SetVideoMode(Config.Graphics.ResX, Config.Graphics.ResY, Config.Graphics.BitDepth, Config.Graphics.RefreshRate, Config.Graphics.Monitor, !Config.Graphics.Windowed))
			pWindow->SetSize(Config.Graphics.ResX, Config.Graphics.ResY);
	}

	// Initialize gamepad
	if (!pGamePadControl && Config.General.GamepadEnabled)
		pGamePadControl = new C4GamePadControl();

	AppState = C4AS_PreInit;

	return true;
}

void C4Application::ClearCommandLine()
{
	*Game.PlayerFilenames = 0;
	Game.StartupPlayerCount = 0;
}

void C4Application::ParseCommandLine(int argc, char * argv[])
{

	StdStrBuf CmdLine("Command line:");
	for(int i = 0; i < argc; ++i) {
		CmdLine.Append(" ");
		CmdLine.Append(argv[i]);
	}
	Log(CmdLine.getData());

	ClearCommandLine();
	Game.NetworkActive = false;
	Reloc.Init(); // re-init from config
	isEditor = 2;
	int c;
	while (1)
	{

		static struct option long_options[] =
		{
			// option, w/ argument?, set directly, set to...
			{"editor", no_argument, &isEditor, 1},
			{"fullscreen", no_argument, &isEditor, 0},
			{"debugwait", no_argument, &Game.DebugWait, 1},
			{"nosplash", no_argument, &NoSplash, 1},
			{"update", no_argument, &CheckForUpdates, 1},
			{"noruntimejoin", no_argument, &Config.Network.NoRuntimeJoin, 1},
			{"runtimejoin", no_argument, &Config.Network.NoRuntimeJoin, 0},
			{"noleague", no_argument, &Config.Network.LeagueServerSignUp, 0},
			{"league", no_argument, &Config.Network.LeagueServerSignUp, 1},
			{"nosignup", no_argument, &Config.Network.MasterServerSignUp, 0},
			{"signup", no_argument, &Config.Network.MasterServerSignUp, 1},

			{"client", required_argument, 0, 'c'},
			{"host", no_argument, 0, 'h'},
			{"debughost", required_argument, 0, 'H'},
			{"debugpass", required_argument, 0, 'P'},
			{"debug", required_argument, 0, 'D'},
			{"data", required_argument, 0, 'd'},
			{"startup", required_argument, 0, 's'},
			{"stream", required_argument, 0, 'e'},
			{"recdump", required_argument, 0, 'R'},
			{"comment", required_argument, 0, 'm'},
			{"pass", required_argument, 0, 'p'},
			{"udpport", required_argument, 0, 'u'},
			{"tcpport", required_argument, 0, 't'},
			{"join", required_argument, 0, 'j'},
			{"language", required_argument, 0, 'L'},

			{"observe", no_argument, 0, 'o'},
			{"nonetwork", no_argument, 0, 'N'},
			{"network", no_argument, 0, 'n'},
			{"record", no_argument, 0, 'r'},

			{"lobby", required_argument, 0, 'l'},
			{0, 0, 0, 0}
		};
		int option_index = 0;
		c = getopt_long (argc, argv, "abc:d:f:",
			long_options, &option_index);
     		// no more options
		if (c == -1)
			break;
		switch (c)
		{
		case 0:
			// Signup
			if (SEqualNoCase(long_options[option_index].name, "signup"))
			{
				Game.NetworkActive = true;
			}
			// League
			if (SEqualNoCase(long_options[option_index].name, "league"))
			{
				Game.NetworkActive = true;
				Config.Network.MasterServerSignUp = true;
			}
			break;
		// Lobby
		case 'l':
			Game.fLobby = true;
			// lobby timeout specified? (e.g. --lobby=120)
			if (optarg)
			{
				Game.iLobbyTimeout = atoi(optarg);
				if (Game.iLobbyTimeout < 0) Game.iLobbyTimeout = 0;
			}
			break;
		case 'o': Game.fObserve = true; break;
		// Direct join
		case 'j':
			Game.NetworkActive = true;
			SCopy(optarg, Game.DirectJoinAddress, _MAX_PATH);
			break;
		case 'r': Game.Record = true; break;
		case 'n': Game.NetworkActive = true; break;
		case 'N': Game.NetworkActive = false; break;
		// Language override by parameter
		case 'L': SCopy(optarg, Config.General.LanguageEx, CFG_MaxString);
		// port overrides
		case 't': Config.Network.PortTCP = atoi(optarg); break;
		case 'u': Config.Network.PortUDP = atoi(optarg); break;
		// network game password
		case 'p': Network.SetPassword(optarg); break;
		// network game comment
		case 'm': Config.Network.Comment.CopyValidated(optarg); break;
		// record dump
		case 'R': Game.RecordDumpFile.Copy(optarg); break;
		// record stream
		case 'e': Game.RecordStream.Copy(optarg); break;
		// startup start screen
		case 's': C4Startup::SetStartScreen(optarg); break;
		// additional read-only data path
		case 'd': Reloc.AddPath(optarg); break;
		// debug options
		case 'D': Game.DebugPort = atoi(optarg); break;
		case 'P': Game.DebugPassword = optarg; break;
		case 'H': Game.DebugHost = optarg; break;
		// debug configs
		case 'h':
			Game.NetworkActive = true;
			Game.fLobby = true;
			Config.Network.PortTCP = 11112;
			Config.Network.PortUDP = 11113;
			Config.Network.MasterServerSignUp = Config.Network.LeagueServerSignUp = false;
			break;
		case 'c':
			Game.NetworkActive = true;
			SCopy("localhost", Game.DirectJoinAddress, _MAX_PATH);
			Game.fLobby = true;
			Config.Network.PortTCP = 11112 + 2*(atoi(optarg)+1);
			Config.Network.PortUDP = 11113 + 2*(atoi(optarg)+1);
			break;
		case '?': /* getopt_long already printed an error message. */ break;
		default: assert(!"unexpected getopt_long return value");
		}
	}
	if (!Config.Network.MasterServerSignUp)
		Config.Network.LeagueServerSignUp = false;
	if (Game.fObserve || Game.fLobby)
		Game.NetworkActive = true;

	while (optind < argc)
	{
		char * szParameter = argv[optind++];
		{ // Strip trailing / that result from tab-completing unpacked c4groups
			int iLen = SLen(szParameter);
			if (iLen > 5 && szParameter[iLen-1] == '/' && szParameter[iLen-5] == '.' && szParameter[iLen-4] == 'o' && szParameter[iLen-3] == 'c')
			{
				szParameter[iLen-1] = '\0';
			}
		}
		// Scenario file
		if (SEqualNoCase(GetExtension(szParameter),"ocs"))
		{
			if(IsGlobalPath(szParameter))
				Game.SetScenarioFilename(szParameter);
			else
				Game.SetScenarioFilename((std::string(GetWorkingDirectory()) + DirSep + szParameter).c_str());

			continue;
		}
		if (SEqualNoCase(GetFilename(szParameter),"scenario.txt"))
		{
			Game.SetScenarioFilename(szParameter);
			continue;
		}
		// Player file
		if (SEqualNoCase(GetExtension(szParameter),"ocp"))
		{
			if(IsGlobalPath(szParameter))
				SAddModule(Game.PlayerFilenames, szParameter);
			else
				SAddModule(Game.PlayerFilenames, (std::string(GetWorkingDirectory()) + DirSep + szParameter).c_str());

			continue;
		}
		// Definition file
		if (SEqualNoCase(GetExtension(szParameter),"ocd"))
		{
			SAddModule(Game.DefinitionFilenames,szParameter);
			continue;
		}
		// Key file
		if (SEqualNoCase(GetExtension(szParameter),"c4k"))
		{
			Application.IncomingKeyfile.Copy(szParameter);
			continue;
		}
		// Update file
		if (SEqualNoCase(GetExtension(szParameter),"ocu"))
		{
			Application.IncomingUpdate.Copy(szParameter);
			continue;
		}
		// record stream
		if (SEqualNoCase(GetExtension(szParameter),"c4r"))
		{
			Game.RecordStream.Copy(szParameter);
		}
		// Direct join by URL
		if (SEqual2NoCase(szParameter, "clonk:"))
		{
			// Store address
			SCopy(szParameter + 6, Game.DirectJoinAddress, _MAX_PATH);
			SClearFrontBack(Game.DirectJoinAddress, '/');
			// Special case: if the target address is "update" then this is used for update initiation by url
			if (SEqualNoCase(Game.DirectJoinAddress, "update"))
			{
				Application.CheckForUpdates = true;
				Game.DirectJoinAddress[0] = 0;
				continue;
			}
			// Self-enable network
			Game.NetworkActive = true;
			continue;
		}
	}

	// Default to editor if scenario given, player mode otherwise
	if (isEditor == 2)
		isEditor = !!*Game.ScenarioFilename;

	// Determine startup player count
	Game.StartupPlayerCount = SModuleCount(Game.PlayerFilenames);

	// record?
	Game.Record = Game.Record || (Config.Network.LeagueServerSignUp && Game.NetworkActive);

	// startup dialog required?
	QuitAfterGame = !isEditor && Game.HasScenario();
}

void C4Application::ApplyResolutionConstraints()
{
	// Enumerate display modes
	int32_t idx = -1, iXRes, iYRes, iBitDepth, iRefreshRate;
	int32_t best_match = -1;
	uint32_t best_delta = ~0;
	while (GetIndexedDisplayMode(++idx, &iXRes, &iYRes, &iBitDepth, &iRefreshRate, Config.Graphics.Monitor))
	{
		if (iBitDepth != Config.Graphics.BitDepth) continue;
		uint32_t delta = std::abs(Config.Graphics.ResX*Config.Graphics.ResY - iXRes*iYRes);
		if (!delta && iBitDepth == Config.Graphics.BitDepth && iRefreshRate == Config.Graphics.RefreshRate)
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
		GetIndexedDisplayMode(best_match, &iXRes, &iYRes, &iBitDepth, &iRefreshRate, Config.Graphics.Monitor);
		if (iXRes != Config.Graphics.ResX || iYRes != Config.Graphics.ResY)
			// Don't warn if only bit depth changes
			// Also, lang table not loaded yet
			LogF("Warning: The selected resolution %dx%d is not available and has been changed to %dx%d.", Config.Graphics.ResX, Config.Graphics.ResY, iXRes, iYRes);
		Config.Graphics.ResX = iXRes; Config.Graphics.ResY = iYRes;
		Config.Graphics.BitDepth = iBitDepth;
		Config.Graphics.RefreshRate = iRefreshRate;
	}
}

bool C4Application::PreInit()
{
	// startup dialog: Only use if no next mission has been provided
	bool fUseStartupDialog = !Game.HasScenario();

	// Startup message board
	if (!isEditor)
		if (Config.Graphics.ShowStartupMessages || Game.NetworkActive)
		{
			C4Facet cgo; cgo.Set(FullScreen.pSurface,0,0,C4GUI::GetScreenWdt(), C4GUI::GetScreenHgt());
			GraphicsSystem.MessageBoard.Init(cgo,true);
		}
	Game.SetInitProgress(0.0f);

	// init loader: Black screen for first start if a video is to be shown; otherwise default spec
	if (fUseStartupDialog && !isEditor)
	{
		//Log(LoadResStr("IDS_PRC_INITLOADER"));
		bool fUseBlackScreenLoader = !C4Startup::WasFirstRun() && !Config.Startup.NoSplash && !NoSplash && FileExists(C4CFN_Splash);
		if (!::GraphicsSystem.InitLoaderScreen(C4CFN_StartupBackgroundMain, fUseBlackScreenLoader))
			{ LogFatal(LoadResStr("IDS_PRC_ERRLOADER")); return false; }
	}
	Game.SetInitProgress(fUseStartupDialog ? 10.0f : 1.0f);

	if (!Game.PreInit()) return false;

	// Music
	if (!MusicSystem.Init("Frontend.*"))
		Log(LoadResStr("IDS_PRC_NOMUSIC"));

	// Play some music!
	if (fUseStartupDialog && !isEditor && Config.Sound.FEMusic)
		MusicSystem.Play();

	Game.SetInitProgress(fUseStartupDialog ? 34.0f : 2.0f);

	// Sound
	if (!SoundSystem.Init())
		Log(LoadResStr("IDS_PRC_NOSND"));

	Game.SetInitProgress(fUseStartupDialog ? 35.0f : 3.0f);

	if (fUseStartupDialog)
	{
		AppState = C4AS_Startup;
		// default record?
		Game.Record = Game.Record || Config.General.DefRec;
		// if no scenario or direct join has been specified, get game startup parameters by startup dialog
		if (!isEditor)
			C4Startup::InitStartup();
	}
	// directly launch scenario / network game
	else
	{
		AppState = C4AS_StartGame;
	}

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
	// quit irc
	IRCClient.Close();
	// close system group (System.ocg)
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
	if (lpDDraw) { delete lpDDraw; lpDDraw=NULL; }
	// Close window
	FullScreen.Clear();
	Console.Clear();
	// The very final stuff
	CStdApp::Clear();
}

void C4Application::Quit()
{
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

void C4Application::OpenGame(const char * scenario)
{
	if (AppState == C4AS_Startup)
	{
		if (scenario) Game.SetScenarioFilename(scenario);
		AppState = C4AS_StartGame;
	}
	else
	{
		SetNextMission(scenario);
		AppState = C4AS_AfterGame;
	}

}

void C4Application::QuitGame()
{
	// reinit desired? Do restart
	if (!QuitAfterGame || NextMission)
	{
		AppState = C4AS_AfterGame;
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
		// wait for the user to start a game
		break;
	case C4AS_StartGame:
		// immediate progress to next state; OpenGame will enter HandleMessage-loops in startup and lobby!
		C4Startup::CloseStartup();
		AppState = C4AS_Game;
		// first-time game initialization
		if (!Game.Init())
		{
			// set error flag (unless this was a lobby user abort)
			if (!C4GameLobby::UserAbort)
				Game.fQuitWithError = true;
			// no start: Regular QuitGame; this may reset the engine to startup mode if desired
			QuitGame();
			break;
		}
		break;
	case C4AS_AfterGame:
		// stop game
		Game.Clear();
		AppState = C4AS_PreInit;
		// if a next mission is desired, set to start it
		if (NextMission)
		{
			Game.SetScenarioFilename(NextMission.getData());
			Game.fLobby = Game.NetworkActive;
			Game.fObserve = false;
			NextMission.Clear();
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
	if (!Game.DoSkipFrame)
	{
		// Fullscreen mode
		if (!isEditor)
			FullScreen.Execute();
		// Console mode
		else
			Console.Execute();
	}
}

void C4Application::SetGameTickDelay(int iDelay)
{
	if (!pGameTimer) return;
	pGameTimer->SetGameTickDelay(iDelay);
}

void C4Application::OnResolutionChanged(unsigned int iXRes, unsigned int iYRes)
{
	// notify game
	if (lpDDraw)
	{
		Game.OnResolutionChanged(iXRes, iYRes);
		lpDDraw->OnResolutionChanged(iXRes, iYRes);
	}
	if (pWindow && pWindow->pSurface)
		pWindow->pSurface->UpdateSize(iXRes, iYRes);
}

bool C4Application::SetGameFont(const char *szFontFace, int32_t iFontSize)
{
#ifndef USE_CONSOLE
	// safety
	if (!szFontFace || !*szFontFace || iFontSize<1 || SLen(szFontFace)>=static_cast<int>(sizeof Config.General.RXFontName)) return false;
	// first, check if the selected font can be created at all
	// check regular font only - there's no reason why the other fonts couldn't be created
	CStdFont TestFont;
	if (!::FontLoader.InitFont(TestFont, szFontFace, C4FontLoader::C4FT_Main, iFontSize, &::GraphicsResource.Files))
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
	if (AppState == C4AS_Game)
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
	{
		NextMission.Copy(szMissionFilename);
		// scenarios tend to use the wrong slash
		SReplaceChar(NextMission.getMData(), AltDirectorySeparator, DirectorySeparator);
	}
	else
		NextMission.Clear();
}

void C4Application::NextTick()
{
	if (!pGameTimer) return;
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
	unsigned int Now = GetTime();
	// Execute
	if (Now >= iLastGameTick + iGameTickDelay || Game.GameGo)
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

bool  C4ApplicationGameTimer::IsLowPriority() { return true; }
