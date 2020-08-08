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

/* Main class to initialize configuration and execute the game */

#include "C4Include.h"
#include "C4ForbidLibraryCompilation.h"
#include "game/C4Application.h"

#include "C4Version.h"
#include "editor/C4Console.h"
#include "game/C4FullScreen.h"
#include "game/C4GraphicsSystem.h"
#include "graphics/C4Draw.h"
#include "graphics/C4GraphicsResource.h"
#include "graphics/StdPNG.h"
#include "gui/C4GameLobby.h"
#include "gui/C4GfxErrorDlg.h"
#include "gui/C4MessageInput.h"
#ifdef _WIN32
#include "gui/C4UpdateDlg.h"
#endif
#include "gui/C4Startup.h"
#include "landscape/C4Particles.h"
#include "network/C4Network2.h"
#include "network/C4Network2IRC.h"
#include "platform/C4GamePadCon.h"
#include "C4Licenses.h"

#include <getopt.h>

static C4Network2IRCClient ApplicationIRCClient;
const std::string C4Application::Revision{ C4REVISION };

C4Application::C4Application():
		IRCClient(ApplicationIRCClient)
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
	Config.Load(sConfigFilename.getData());
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
	// Open log
	OpenLog();

	// Engine header message
	Log(C4ENGINECAPTION);
	LogF("Version: %s %s (%s - %s)", C4VERSION, C4_OS, GetRevision(), C4REVISION_TS);
	LogF(R"(ExePath: "%s")", Config.General.ExePath.getData());
	LogF(R"(SystemDataPath: "%s")", Config.General.SystemDataPath);
	LogF(R"(UserDataPath: "%s")", Config.General.UserDataPath);

	// Init C4Group
	C4Group_SetProcessCallback(&ProcessCallback);
	C4Group_SetTempPath(Config.General.TempPath.getData());
	C4Group_SetSortList(C4CFN_FLS);

	// Cleanup temp folders left behind
	Config.CleanupTempUpdateFolder();

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

	// Open additional logs that depend on command line
	OpenExtraLogs();

	// Init external language packs
	Languages.Init();
	// Load language string table
	if (!Languages.LoadLanguage(Config.General.LanguageEx))
		// No language table was loaded - bad luck...
		if (!Languages.HasStringTable())
			Log("WARNING: No language string table loaded!");

#if defined(WIN32) && defined(WITH_AUTOMATIC_UPDATE)
	// Windows: handle incoming updates directly, even before starting up the gui
	//          because updates will be applied in the console anyway.
	if (!Application.IncomingUpdate.empty())
		if (C4UpdateDlg::ApplyUpdate(Application.IncomingUpdate.c_str(), false, nullptr))
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
			{ Clear(); ShowGfxErrorDialog(); return false; }
	}
	else
	{
		if (!(pWindow = Console.Init(this)))
			{ Clear(); return false; }
	}

	// init timers (needs window)
	Add(pGameTimer = new C4ApplicationGameTimer());

	// Initialize OpenGL
	bool success = DDrawInit(this, GetConfigWidth(), GetConfigHeight(), Config.Graphics.Monitor);
	if (!success) { LogFatal(LoadResStr("IDS_ERR_DDRAW")); Clear(); ShowGfxErrorDialog(); return false; }

	if (!isEditor)
	{
		if (!SetVideoMode(Application.GetConfigWidth(), Application.GetConfigHeight(), Config.Graphics.RefreshRate, Config.Graphics.Monitor, !Config.Graphics.Windowed))
			pWindow->SetSize(Config.Graphics.WindowX, Config.Graphics.WindowY);
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
}

void C4Application::ParseCommandLine(int argc, char * argv[])
{
	argv0 = argv[0];
	StdStrBuf CmdLine("Command line:");
	for(int i = 0; i < argc; ++i) {
		CmdLine.Append(" ");
		CmdLine.Append(argv[i]);
	}
	Log(CmdLine.getData());

	ClearCommandLine();
	Game.NetworkActive = false;
	isEditor = 2;
	int c;
	while (true)
	{

		static struct option long_options[] =
		{
			// option, w/ argument?, set directly, set to...
			{"editor", no_argument, &isEditor, 1},
			{"fullscreen", no_argument, &isEditor, 0},
			{"debugwait", no_argument, &Game.DebugWait, 1},
			{"update", no_argument, &CheckForUpdates, 1},
			{"noruntimejoin", no_argument, &Config.Network.NoRuntimeJoin, 1},
			{"runtimejoin", no_argument, &Config.Network.NoRuntimeJoin, 0},
			{"noleague", no_argument, &Config.Network.LeagueServerSignUp, 0},
			{"league", no_argument, &Config.Network.LeagueServerSignUp, 1},
			{"nosignup", no_argument, &Config.Network.MasterServerSignUp, 0},
			{"signup", no_argument, &Config.Network.MasterServerSignUp, 1},
			
			{"debugrecread", required_argument, nullptr, 'K'},
			{"debugrecwrite", required_argument, nullptr, 'w'},

			{"client", required_argument, nullptr, 'c'},
			{"host", no_argument, nullptr, 'h'},
			{"debughost", required_argument, nullptr, 'H'},
			{"debugpass", required_argument, nullptr, 'P'},
			{"debug", required_argument, nullptr, 'D'},
			{"data", required_argument, nullptr, 'd'},
			{"startup", required_argument, nullptr, 's'},
			{"stream", required_argument, nullptr, 'e'},
			{"recdump", required_argument, nullptr, 'R'},
			{"comment", required_argument, nullptr, 'm'},
			{"pass", required_argument, nullptr, 'p'},
			{"udpport", required_argument, nullptr, 'u'},
			{"tcpport", required_argument, nullptr, 't'},
			{"join", required_argument, nullptr, 'j'},
			{"language", required_argument, nullptr, 'L'},
			{"scenpar", required_argument, nullptr, 'S'},

			{"observe", no_argument, nullptr, 'o'},
			{"nonetwork", no_argument, nullptr, 'N'},
			{"network", no_argument, nullptr, 'n'},
			{"record", no_argument, nullptr, 'r'},

			{"lobby", optional_argument, nullptr, 'l'},

			{"debug-opengl", no_argument, &Config.Graphics.DebugOpenGL, 1},
			{"config", required_argument, nullptr, 0},
			{"show-licenses", no_argument, nullptr, 0},
			{nullptr, 0, nullptr, 0}
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
			// Legal stuff
			if (SEqualNoCase(long_options[option_index].name, "show-licenses"))
			{
				std::string sep{"\n=================================\n"};
				for (const auto& license : OCLicenses)
					Log((sep + license.path + ": " + license.name + sep + license.content + "\n").c_str());
				Quit();
			}
			// Config: Already handled earlier.
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
		case 'K':
			if (optarg && optarg[0])
			{
				LogF("Reading from DebugRec file '%s'", optarg);
				SCopy(optarg, Config.General.DebugRecExternalFile, _MAX_PATH);
			}
			else
				Log("Reading DebugRec from CtrlRec file in scenario record");
			Config.General.DebugRec = 1;
			Config.General.DebugRecWrite = 0;
			break;
		case 'w':
			if (optarg && optarg[0])
			{
				LogF("Writing to DebugRec file '%s'", optarg);
				SCopy(optarg, Config.General.DebugRecExternalFile, _MAX_PATH);
			}
			else
				Log("Writing DebugRec to CtrlRec file in scenario record");
			Config.General.DebugRec = 1;
			Config.General.DebugRecWrite = 1;
			break;
		case 'r': Game.Record = true; break;
		case 'n': Game.NetworkActive = true; break;
		case 'N': Game.NetworkActive = false; break;
		// Language override by parameter
		case 'L': SCopy(optarg, Config.General.LanguageEx, CFG_MaxString); break;
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
		// set custom scenario parameter by command line
		case 'S':
			{
			StdStrBuf sopt, soptval; sopt.Copy(optarg);
			int32_t val=1;
			if (sopt.SplitAtChar('=', &soptval)) val=atoi(soptval.getData());
			Game.StartupScenarioParameters.SetValue(sopt.getData(), val, false);
			}
			break;
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
			Application.IncomingKeyfile = szParameter;
			continue;
		}
		// Update file
		if (SEqualNoCase(GetExtension(szParameter),"ocu"))
		{
			Application.IncomingUpdate = szParameter;
			continue;
		}
		// record stream
		if (SEqualNoCase(GetExtension(szParameter),"c4r"))
		{
			Game.RecordStream.Copy(szParameter);
		}
		// Direct join by URL
		if (SEqual2NoCase(szParameter, "clonk:") || SEqual2NoCase(szParameter, "openclonk:"))
		{
			// Store address
			SCopy(SAdvancePast(szParameter, ':'), Game.DirectJoinAddress, _MAX_PATH);
			SClearFrontBack(Game.DirectJoinAddress, '/');
			// Special case: if the target address is "update" then this is used for update initiation by url
			if (SEqualNoCase(Game.DirectJoinAddress, "update"))
			{
				Application.CheckForUpdates = true;
				Game.DirectJoinAddress[0] = 0;
				continue;
			}
			// Special case: start the mod dialog and initiate installation of a mod.
			const char* install_mod_command = "installmod";
			const auto install_mod_command_length = strlen(install_mod_command);
			if (SEqualNoCase(Game.DirectJoinAddress, install_mod_command, install_mod_command_length))
			{
				// Advance the string to the parameter after the command.
				const char *id = Game.DirectJoinAddress + install_mod_command_length;
				if (SLen(id) > 1)
				{
					++id; // Remove slash.
					C4Startup::SetStartScreen("mods", id);
				}
				Game.DirectJoinAddress[0] = 0;
				continue;
			}
			// Self-enable network
			Game.NetworkActive = true;
			continue;
		}
	}

#ifdef _WIN32
	// Clean up some forward/backward slach confusion since many internal OC file functions cannot handle both
	SReplaceChar(Game.ScenarioFilename, AltDirectorySeparator, DirectorySeparator);
	SReplaceChar(Game.PlayerFilenames, AltDirectorySeparator, DirectorySeparator);
	SReplaceChar(Game.DefinitionFilenames, AltDirectorySeparator, DirectorySeparator);
	std::replace(begin(IncomingKeyfile), end(IncomingKeyfile), AltDirectorySeparator, DirectorySeparator);
	std::replace(begin(IncomingUpdate), end(IncomingUpdate), AltDirectorySeparator, DirectorySeparator);
	Game.RecordStream.ReplaceChar(AltDirectorySeparator, DirectorySeparator);
#endif

	// Default to editor if scenario given, player mode otherwise
	if (isEditor == 2)
		isEditor = !!*Game.ScenarioFilename && !Config.General.OpenScenarioInGameMode;

	// record?
	Game.Record = Game.Record || (Config.Network.LeagueServerSignUp && Game.NetworkActive);

	// startup dialog required?
	QuitAfterGame = !isEditor && Game.HasScenario();
}

void C4Application::ApplyResolutionConstraints()
{
	// Not changing the resolution always works anyway
	if (Config.Graphics.ResX == -1 && Config.Graphics.ResY == -1)
		return;
	// Enumerate display modes
	int32_t idx = -1, iXRes, iYRes, iBitDepth, iRefreshRate;
	int32_t best_match = -1;
	uint32_t best_delta = ~0;
	while (GetIndexedDisplayMode(++idx, &iXRes, &iYRes, &iBitDepth, &iRefreshRate, Config.Graphics.Monitor))
	{
		if (iBitDepth != C4Draw::COLOR_DEPTH) continue;
		uint32_t delta = std::abs(Config.Graphics.ResX*Config.Graphics.ResY - iXRes*iYRes);
		if (!delta && iBitDepth == C4Draw::COLOR_DEPTH && iRefreshRate == Config.Graphics.RefreshRate)
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
		Config.Graphics.RefreshRate = iRefreshRate;
	}
}

bool C4Application::PreInit()
{
	// startup dialog: Only use if no next mission has been provided
	bool fUseStartupDialog = !Game.HasScenario();

	// Load graphics early, before we draw anything, since we need shaders
	// loaded to draw.
	Game.SetInitProgress(0.0f);
	Log(LoadResStr("IDS_PRC_GFXRES"));
	if (!GraphicsResource.Init()) return false;
	Game.SetInitProgress(fUseStartupDialog ? 10.0f : 1.0f);

	// Startup message board
	if (!isEditor)
		if (Config.Graphics.ShowStartupMessages || Game.NetworkActive)
		{
			C4Facet cgo; cgo.Set(FullScreen.pSurface,0,0,C4GUI::GetScreenWdt(), C4GUI::GetScreenHgt());
			GraphicsSystem.MessageBoard->Init(cgo,true);
		}

	// init loader: Black screen for first start if a video is to be shown; otherwise default spec
	if (fUseStartupDialog && !isEditor)
	{
		if (!::GraphicsSystem.InitLoaderScreen(C4CFN_StartupBackgroundMain))
			{ LogFatal(LoadResStr("IDS_PRC_ERRLOADER")); return false; }
	}
	Game.SetInitProgress(fUseStartupDialog ? 20.0f : 2.0f);

	if (!Game.PreInit()) return false;

	// Music
	if (!MusicSystem.Init("frontend"))
		Log(LoadResStr("IDS_PRC_NOMUSIC"));

	Game.SetInitProgress(fUseStartupDialog ? 34.0f : 2.0f);

	// Sound
	if (!SoundSystem.Init())
		Log(LoadResStr("IDS_PRC_NOSND"));

	// Play some music! - after sound init because sound system might be needed by music system
	if (fUseStartupDialog && !isEditor && Config.Sound.FEMusic)
		MusicSystem.Play();

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
	NextMission.clear();
	// stop timer
	if (pGameTimer)
	{
		Remove(pGameTimer);
		delete pGameTimer; pGameTimer = nullptr;
	}
	// quit irc
	IRCClient.Close();
	// close system group (System.ocg)
	SystemGroup.Close();
	// Log
	if (::Languages.HasStringTable()) // Avoid (double and undefined) message on (second?) shutdown...
		Log(LoadResStr("IDS_PRC_DEINIT"));
	// Clear external language packs and string table
	Languages.Clear();
	Languages.ClearLanguage();
	// gamepad clear
	if (pGamePadControl) { delete pGamePadControl; pGamePadControl=nullptr; }
	// music system clear
	MusicSystem.Clear();
	SoundSystem.Clear();
	RestoreVideoMode();
	// clear editcursor holding graphics before clearing draw
	::Console.EditCursor.Clear();
	// Clear direct draw (late, because it's needed for e.g. Log)
	if (pDraw) { delete pDraw; pDraw=nullptr; }
	// Close window
	FullScreen.Clear();
	Console.Clear();
	// There might be pending saves - do them after the fullscreen windows got closed
	// so the app just remains as a lingering process until saving is done
	CPNGFile::WaitForSaves();
	// The very final stuff
	C4AbstractApp::Clear();
}

void C4Application::Quit()
{
	// Participants should not be cleared for usual startup dialog

	// Save config if there was no loading error
	if (Config.ConfigLoaded) Config.Save();
	// make sure startup data is unloaded
	C4Startup::Unload();
	// fonts are loaded at start and never unloaded
	::GraphicsResource.ClearFonts();
	// quit app
	C4AbstractApp::Quit();
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
	if (!QuitAfterGame || !NextMission.empty())
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
		SoundSystem.Execute();
		MusicSystem.Execute();
		if (pGamePadControl) pGamePadControl->Execute();
		// wait for the user to start a game
		break;
	case C4AS_StartGame:
		// immediate progress to next state; OpenGame will enter HandleMessage-loops in startup and lobby!
		C4Startup::CloseStartup();
		AppState = C4AS_Game;
#ifdef WITH_QT_EDITOR
		// Notify console
		if (isEditor) ::Console.OnStartGame();
#endif
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
		if(Config.Graphics.Windowed == 2 && FullScreenMode())
			Application.SetVideoMode(GetConfigWidth(), GetConfigHeight(), Config.Graphics.RefreshRate, Config.Graphics.Monitor, true);
		if (!isEditor)
			pWindow->GrabMouse(true);
		// Gamepad events have to be polled here so that the controller
		// connection state is always up-to-date before players are
		// joining.
		if (pGamePadControl) pGamePadControl->Execute();
		break;
	case C4AS_AfterGame:
		// stop game
		Game.Clear();
		if(Config.Graphics.Windowed == 2 && NextMission.empty() && !isEditor)
			Application.SetVideoMode(GetConfigWidth(), GetConfigHeight(), Config.Graphics.RefreshRate, Config.Graphics.Monitor, false);
		if (!isEditor)
			pWindow->GrabMouse(false);
		AppState = C4AS_PreInit;
		// if a next mission is desired, set to start it
		if (!NextMission.empty())
		{
			Game.SetScenarioFilename(NextMission.c_str());
			Game.fLobby = Game.NetworkActive;
			Game.fObserve = false;
			NextMission.clear();
		}
		break;
	case C4AS_Game:
		// Game
		if (Game.IsRunning)
			Game.Execute();
		// Sound
		SoundSystem.Execute();
		MusicSystem.Execute();
		// Gamepad
		if (pGamePadControl) pGamePadControl->Execute();
		break;
	}
}

void C4Application::Draw()
{
	// Graphics

	// Fullscreen mode
	if (!isEditor)
		FullScreen.Execute();
	// Console mode
	else
		Console.Execute();
}

void C4Application::SetGameTickDelay(int iDelay)
{
	if (!pGameTimer) return;
	pGameTimer->SetGameTickDelay(iDelay);
}

void C4Application::OnResolutionChanged(unsigned int iXRes, unsigned int iYRes)
{
	// notify game
	if (pDraw)
	{
		Game.OnResolutionChanged(iXRes, iYRes);
		pDraw->OnResolutionChanged(iXRes, iYRes);
	}
	if (pWindow)
	{
		if (pWindow->pSurface)
			pWindow->pSurface->UpdateSize(iXRes, iYRes);
		if (!FullScreenMode())
		{
			C4Rect r;
			pWindow->GetSize(&r);
			Config.Graphics.WindowX = r.Wdt;
			Config.Graphics.WindowY = r.Hgt;
		}
	}
}

void C4Application::OnKeyboardLayoutChanged()
{
	// re-resolve all keys
	Game.OnKeyboardLayoutChanged();
	if (AppState == C4AS_Startup) C4Startup::Get()->OnKeyboardLayoutChanged();
}

bool C4Application::SetGameFont(const char *szFontFace, int32_t iFontSize)
{
#ifndef USE_CONSOLE
	// safety
	if (!szFontFace || !*szFontFace || iFontSize<1 || SLen(szFontFace)>=static_cast<int>(sizeof Config.General.RXFontName)) return false;
	// first, check if the selected font can be created at all
	// check regular font only - there's no reason why the other fonts couldn't be created
	CStdFont TestFont;
	if (!::FontLoader.InitFont(&TestFont, szFontFace, C4FontLoader::C4FT_Main, iFontSize, &::GraphicsResource.Files))
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
	if (AppState == C4AS_Game)
		::MessageInput.ProcessInput(szCmd);
	else if (AppState == C4AS_Startup)
	{
		AppState = C4AS_PreInit;
		Game.SetScenarioFilename(szCmd);
	}
}

void C4Application::Activate()
{
#ifdef USE_WIN32_WINDOWS
	BringWindowToTop(FullScreen.hWindow);
	ShowWindow(FullScreen.hWindow, SW_SHOW);
#endif
}

void C4Application::SetNextMission(const char *szMissionFilename)
{
	// set next mission if any is desired
	if (szMissionFilename)
	{
		NextMission = szMissionFilename;
		// scenarios tend to use the wrong slash
		std::replace(begin(NextMission), end(NextMission), AltDirectorySeparator, DirectorySeparator);
	}
	else
		NextMission.clear();
}

void C4Application::NextTick()
{
	if (!pGameTimer) return;
	pGameTimer->Set();
}

bool C4Application::FullScreenMode()
{
	if(isEditor)
		return false;
	if(!Config.Graphics.Windowed)
		return true;
	if(Config.Graphics.Windowed == 2 && Game.IsRunning)
		return true;
	return false;
}

// *** C4ApplicationGameTimer

C4ApplicationGameTimer::C4ApplicationGameTimer()
		: CStdMultimediaTimerProc(26),
		tLastGameTick(C4TimeMilliseconds::NegativeInfinity)
{
}

void C4ApplicationGameTimer::SetGameTickDelay(uint32_t iDelay)
{
	// Remember delay
	iGameTickDelay = iDelay;
	// Smaller than minimum refresh delay?
	if (iDelay < uint32_t(Config.Graphics.MaxRefreshDelay))
	{
		// Set critical timer
		SetDelay(iDelay);
		// No additional breaking needed
		iExtraGameTickDelay = 0;
	}
	else
	{
		// Set critical timer
		SetDelay(Config.Graphics.MaxRefreshDelay);
		// Slow down game tick
		iExtraGameTickDelay = iDelay;
	}
}

bool C4ApplicationGameTimer::Execute(int iTimeout, pollfd *)
{
	// Check timer and reset
	if (!CheckAndReset()) return true;
	C4TimeMilliseconds tNow = C4TimeMilliseconds::Now();
	// Execute
	if (tNow >= tLastGameTick + iExtraGameTickDelay || Game.GameGo)
	{
		if (iGameTickDelay)
			tLastGameTick += iGameTickDelay;
		else
			tLastGameTick = tNow;

		// Compensate if things get too slow
		if (tNow > tLastGameTick + iGameTickDelay)
			tLastGameTick += (tNow - tLastGameTick) / 2;

		Application.GameTick();
	}
	// Draw
	if (!Game.DoSkipFrame)
	{
		C4TimeMilliseconds tPreGfxTime = C4TimeMilliseconds::Now();

		Application.Draw();

		// Automatic frame skip if graphics are slowing down the game (skip max. every 2nd frame)
		Game.DoSkipFrame = Game.Parameters.AutoFrameSkip && (tPreGfxTime + iGameTickDelay < C4TimeMilliseconds::Now());
	} else {
		Game.DoSkipFrame=false;
	}
	return true;
}

bool  C4ApplicationGameTimer::IsLowPriority() { return true; }
