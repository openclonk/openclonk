/*
 * OpenClonk, http://www.openclonk.org
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

#include "C4Include.h"
#include "C4ForbidLibraryCompilation.h"
#include "config/C4Config.h"

#include "C4Version.h"
#include "c4group/C4Components.h"
#include "network/C4Network2.h"

#include "platform/C4Window.h"
#include "platform/StdRegistry.h"

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_LOCALE_H
#include <clocale>
#endif

#ifdef USE_CONSOLE
#define DONCOFF 0
#else
#define DONCOFF 1
#endif

#include "game/C4Application.h"

void C4ConfigGeneral::CompileFunc(StdCompiler *pComp)
{
	// For those without the ability to intuitively guess what the falses and trues mean:
	// its mkNamingAdapt(field, name, default, fPrefillDefault, fStoreDefault)
	// where fStoreDefault writes out the value to the config even if it's the same as the default.
#define s mkStringAdaptM
	pComp->Value(mkNamingAdapt(s(Name),             "Name",               ""             ));
	pComp->Value(mkNamingAdapt(s(Language),         "Language",           "", false, true));
	pComp->Value(mkNamingAdapt(s(LanguageEx),       "LanguageEx",         "", false, true));
	pComp->Value(mkNamingAdapt(s(Participants),     "Participants",       ""             ));

	// deliberately not grandfathering UserPath setting, since it was written to config by default
	pComp->Value(mkNamingAdapt(s(ConfigUserPath),   "UserDataPath",       "", false, true));
	// assimilate old data
	pComp->Value(mkNamingAdapt(s(Adopt.PlayerPath), "PlayerPath",       ""));

	// temporary path only set during updates
	pComp->Value(mkNamingAdapt(s(TempUpdatePath),   "TempUpdatePath",     ""));

	pComp->Value(mkNamingAdapt(s(MissionAccess),    "MissionAccess",      "", false, true));
	pComp->Value(mkNamingAdapt(FPS,                 "FPS",                0              ));
	pComp->Value(mkNamingAdapt(DefRec,              "DefRec",             0              ));
	pComp->Value(mkNamingAdapt(ScreenshotFolder,    "ScreenshotFolder",   "Screenshots",  false, true));
	pComp->Value(mkNamingAdapt(ScrollSmooth,        "ScrollSmooth",       4              ));
	pComp->Value(mkNamingAdapt(AlwaysDebug,         "DebugMode",          0              ));
	pComp->Value(mkNamingAdapt(OpenScenarioInGameMode, "OpenScenarioInGameMode", 0   )); 
#ifdef _WIN32
	pComp->Value(mkNamingAdapt(MMTimer,             "MMTimer",            1              ));
#endif
	pComp->Value(mkNamingAdapt(s(RXFontName),       "FontName",           C4DEFAULT_FONT_NAME,   false, true));
	pComp->Value(mkNamingAdapt(RXFontSize,          "FontSize",           14,            false, true));
	pComp->Value(mkNamingAdapt(GamepadEnabled,      "GamepadEnabled",     true           ));
	pComp->Value(mkNamingAdapt(FirstStart,          "FirstStart",         true           ));
	pComp->Value(mkNamingAdapt(ConfigResetSafety,   "ConfigResetSafety",  static_cast<int32_t>(ConfigResetSafetyVal) ));
}

void C4ConfigDeveloper::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(AutoFileReload,      "AutoFileReload",     1                    , false, true));
	pComp->Value(mkNamingAdapt(s(TodoFilename),     "TodoFilename",       "{SCENARIO}/TODO.txt", false, true));
	pComp->Value(mkNamingAdapt(s(AltTodoFilename),  "AltTodoFilename2",   "{USERPATH}/TODO.txt", false, true));
	pComp->Value(mkNamingAdapt(MaxScriptMRU,        "MaxScriptMRU",       30                   , false, false));
	pComp->Value(mkNamingAdapt(DebugShapeTextures,  "DebugShapeTextures", 0                    , false, true));
	pComp->Value(mkNamingAdapt(ShowHelp,            "ShowHelp",           true                 , false, false));
	for (int32_t i = 0; i < CFG_MaxEditorMRU; ++i)
		pComp->Value(mkNamingAdapt(s(RecentlyEditedSzenarios[i]), FormatString("EditorMRU%02d", (int)i).getData(), "", false, false));
}

void C4ConfigDeveloper::AddRecentlyEditedScenario(const char *fn)
{
	if (!fn || !*fn) return;
	// Put given scenario first in list by moving all other scenarios down
	// Check how many scenarios to move down the list. Stop moving down when the given scenario is in the list
	int32_t move_down_num;
	for (move_down_num = 0; move_down_num < CFG_MaxEditorMRU - 1; ++move_down_num)
		if (!strncmp(fn, RecentlyEditedSzenarios[move_down_num], CFG_MaxString))
			break;
	// Move them down
	for (int32_t i = move_down_num; i > 0; --i)
		strcpy(RecentlyEditedSzenarios[i], RecentlyEditedSzenarios[i - 1]);
	// Put current scenario in
	strncpy(RecentlyEditedSzenarios[0], fn, CFG_MaxString);
}

void C4ConfigGraphics::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(ResX,                  "ResolutionX",         -1             ,false, true));
	pComp->Value(mkNamingAdapt(ResY,                  "ResolutionY",         -1             ,false, true));
	pComp->Value(mkNamingAdapt(WindowX,               "WindowX",              800           ,false, true));
	pComp->Value(mkNamingAdapt(WindowY,               "WindowY",              600           ,false, true));
	pComp->Value(mkNamingAdapt(RefreshRate,           "RefreshRate",          0             ));
	pComp->Value(mkNamingAdapt(SplitscreenDividers,   "SplitscreenDividers",  1             ));
	pComp->Value(mkNamingAdapt(ShowStartupMessages,   "ShowStartupMessages",  1             ,false, true));
	pComp->Value(mkNamingAdapt(VerboseObjectLoading,  "VerboseObjectLoading", 0             ));
	pComp->Value(mkNamingAdapt(MenuTransparency,      "MenuTransparency",     1             ,false, true));
	pComp->Value(mkNamingAdapt(UpperBoard,            "UpperBoard",           1             ,false, true));
	pComp->Value(mkNamingAdapt(ShowClock,             "ShowClock",            0             ,false, true));
	pComp->Value(mkNamingAdapt(ShowCrewNames,         "ShowCrewNames",        1             ,false, true));
	pComp->Value(mkNamingAdapt(ShowCrewCNames,        "ShowCrewCNames",       0             ,false, true));
	pComp->Value(mkNamingAdapt(Windowed,              "Windowed",             0             ,false, true));
	pComp->Value(mkNamingAdapt(PXSGfx,                "PXSGfx"  ,             1             ));
	pComp->Value(mkNamingAdapt(Gamma,                 "Gamma"  ,              100           ));
	pComp->Value(mkNamingAdapt(Currency,              "Currency"  ,           0             ));
	pComp->Value(mkNamingAdapt(Monitor,               "Monitor",              0             )); // 0 = D3DADAPTER_DEFAULT
	pComp->Value(mkNamingAdapt(MaxRefreshDelay,       "MaxRefreshDelay",      30            ));
	pComp->Value(mkNamingAdapt(NoOffscreenBlits,      "NoOffscreenBlits",     1             ));
	pComp->Value(mkNamingAdapt(MultiSampling,         "MultiSampling",        4             ));
	pComp->Value(mkNamingAdapt(AutoFrameSkip,         "AutoFrameSkip",        1          ));
	pComp->Value(mkNamingAdapt(MouseCursorSize,       "MouseCursorSize",      50            ));
}

void C4ConfigSound::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(RXSound,               "Sound",                DONCOFF       ,false, true));
	pComp->Value(mkNamingAdapt(RXMusic,               "Music",                DONCOFF       ,false, true));
	pComp->Value(mkNamingAdapt(FEMusic,               "MenuMusic",            DONCOFF       ,false, true));
	pComp->Value(mkNamingAdapt(FESamples,             "MenuSound",            DONCOFF       ,false, true));
	pComp->Value(mkNamingAdapt(Verbose,               "Verbose",              0             ));
	pComp->Value(mkNamingAdapt(MusicVolume,           "MusicVolume2",         40            ,false, true));
	pComp->Value(mkNamingAdapt(SoundVolume,           "SoundVolume",          100           ,false, true));
}

void C4ConfigNetwork::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(ControlRate,             "ControlRate",          3            ,false, true));
	pComp->Value(mkNamingAdapt(ControlPreSend,          "ControlPreSend",       -1            ));
	pComp->Value(mkNamingAdapt(s(WorkPath),             "WorkPath",             "Network"     ,false, true));
	pComp->Value(mkNamingAdapt(Lobby,                   "Lobby",                0             ));
	pComp->Value(mkNamingAdapt(NoRuntimeJoin,           "NoRuntimeJoin",        1             ,false, true));
	pComp->Value(mkNamingAdapt(NoReferenceRequest,      "NoReferenceRequest",   0             ));
	pComp->Value(mkNamingAdapt(MaxResSearchRecursion,   "MaxResSearchRecursion",1             ,false, true));
	pComp->Value(mkNamingAdapt(Comment,                 "Comment",              ""            ,false, true));
	pComp->Value(mkNamingAdapt(PortTCP,                 "PortTCP",              C4NetStdPortTCP       ,false, true));
	pComp->Value(mkNamingAdapt(PortUDP,                 "PortUDP",              C4NetStdPortUDP       ,false, true));
	pComp->Value(mkNamingAdapt(EnableUPnP,              "EnableUPnP",           1             , false, true));
	pComp->Value(mkNamingAdapt(PortDiscovery,           "PortDiscovery",        C4NetStdPortDiscovery ,false, true));
	pComp->Value(mkNamingAdapt(PortRefServer,           "PortRefServer",        C4NetStdPortRefServer ,false, true));
	pComp->Value(mkNamingAdapt(ControlMode,             "ControlMode",          0             ));
	pComp->Value(mkNamingAdapt(Nick,                    "Nick",                 ""            ,false, true));
	pComp->Value(mkNamingAdapt(MaxLoadFileSize,         "MaxLoadFileSize",      5*1024*1024   ,false, true));

	pComp->Value(mkNamingAdapt(MasterServerSignUp,      "MasterServerSignUp",   1             ));
	pComp->Value(mkNamingAdapt(MasterServerActive,      "MasterServerActive",   0             ));
	pComp->Value(mkNamingAdapt(MasterKeepPeriod,        "MasterKeepPeriod",     60            ));
	pComp->Value(mkNamingAdapt(MasterReferencePeriod,   "MasterReferencePeriod",120           ));
	pComp->Value(mkNamingAdapt(LeagueServerSignUp,      "LeagueServerSignUp",   0             ));
	pComp->Value(mkNamingAdapt(UseAlternateServer,      "UseAlternateServer",   0             ));
	pComp->Value(mkNamingAdapt(s(AlternateServerAddress),"AlternateServerAddress", "league.openclonk.org:80/league.php"));
	pComp->Value(mkNamingAdapt(s(LastPassword),         "LastPassword",         "Wipf"        ));
#ifdef WITH_AUTOMATIC_UPDATE
	pComp->Value(mkNamingAdapt(s(UpdateServerAddress),  "UpdateServerAddress",     "www.openclonk.org:80/update/"));
	pComp->Value(mkNamingAdapt(AutomaticUpdate,         "AutomaticUpdate",      0             ,false, true));
	pComp->Value(mkNamingAdapt(LastUpdateTime,          "LastUpdateTime",       0             ));
#endif
	pComp->Value(mkNamingAdapt(AsyncMaxWait,            "AsyncMaxWait",         2             ));
	pComp->Value(mkNamingAdapt(PacketLogging,           "PacketLogging",        0             ));
	

	pComp->Value(mkNamingAdapt(s(PuncherAddress),       "PuncherAddress",       "netpuncher.openclonk.org:11115"));
	pComp->Value(mkNamingAdapt(mkParAdapt(LastLeagueServer, StdCompiler::RCT_All),     "LastLeagueServer",     ""            ));
	pComp->Value(mkNamingAdapt(mkParAdapt(LastLeaguePlayerName, StdCompiler::RCT_All), "LastLeaguePlayerName", ""            ));
	pComp->Value(mkNamingAdapt(mkParAdapt(LastLeagueAccount, StdCompiler::RCT_All),    "LastLeagueAccount",    ""            ));
	pComp->Value(mkNamingAdapt(mkParAdapt(LastLeagueLoginToken, StdCompiler::RCT_All), "LastLeagueLoginToken", ""            ));
}

void C4ConfigLobby::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(AllowPlayerSave,         "AllowPlayerSave",      0             ,false, false));
	pComp->Value(mkNamingAdapt(CountdownTime,           "CountdownTime",        5             ,false, false));
}

void C4ConfigIRC::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(s(Server),               "Server",               "irc.euirc.net", false, true));
	pComp->Value(mkNamingAdapt(s(Nick),                 "Nick",                 ""                    , false, true));
	pComp->Value(mkNamingAdapt(s(RealName),             "RealName",             ""                    , false, true));
	pComp->Value(mkNamingAdapt(s(Channel),              "Channel",              "#openclonk"    , false, true));
	pComp->Value(mkNamingAdapt(AllowAllChannels,        "AllowAllChannels",     0                     , false, true));
}

void C4ConfigSecurity::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(WasRegistered,           "WasRegistered",        0                   ));
#ifdef _WIN32
	pComp->Value(mkNamingAdapt(s(KeyPath),              "KeyPath",              R"(%APPDATA%\)" C4ENGINENAME, false, true));
#elif defined(__linux__)
	pComp->Value(mkNamingAdapt(s(KeyPath),              "KeyPath",              "$HOME/.clonk/" C4ENGINENICK, false, true));
#elif defined(__APPLE__)
	pComp->Value(mkNamingAdapt(s(KeyPath),              "KeyPath",              "$HOME/Library/Application Support/" C4ENGINENAME, false, true));
#endif
}

void C4ConfigGamepad::CompileFunc(StdCompiler *pComp, bool fButtonsOnly)
{
	/* The defaults here are for a Logitech Dual Action under Linux-SDL. Better than nothing, I guess. */
	if (!fButtonsOnly)
	{
		for (int i=0; i<6; ++i)
		{
			pComp->Value(mkNamingAdapt(AxisMin[i],          FormatString("Axis%dMin", i).getData(),     0u));
			pComp->Value(mkNamingAdapt(AxisMax[i],          FormatString("Axis%dMax", i).getData(),     0u));
			pComp->Value(mkNamingAdapt(AxisCalibrated[i],   FormatString("Axis%dCalibrated", i).getData(), false));
		}
	}
	pComp->Value(mkNamingAdapt(Button[0],               "Button1",              -1          ));
	pComp->Value(mkNamingAdapt(Button[1],               "Button2",              -1          ));
	pComp->Value(mkNamingAdapt(Button[2],               "Button3",              -1          ));
	pComp->Value(mkNamingAdapt(Button[3],               "Button4",              -1          ));
	pComp->Value(mkNamingAdapt(Button[4],               "Button5",              -1          ));
	pComp->Value(mkNamingAdapt(Button[5],               "Button6",              -1          ));
	pComp->Value(mkNamingAdapt(Button[6],               "Button7",              -1          ));
	pComp->Value(mkNamingAdapt(Button[7],               "Button8",              -1          ));
	pComp->Value(mkNamingAdapt(Button[8],               "Button9",              -1          ));
	pComp->Value(mkNamingAdapt(Button[9],               "Button10",             -1          ));
	pComp->Value(mkNamingAdapt(Button[10],              "Button11",             -1          ));
	pComp->Value(mkNamingAdapt(Button[11],              "Button12",             -1          ));
}

void C4ConfigGamepad::ResetButtons()
{
	// loads an empty config for the buttons
	StdCompilerNull Comp; Comp.Compile(mkParAdapt(*this, true));
}

void C4ConfigGamepad::Reset()
{
	// loads an empty config for the gamepad config
	StdCompilerNull Comp; Comp.Compile(mkParAdapt(*this, false));
}

void C4ConfigControls::CompileFunc(StdCompiler *pComp)
{
#ifndef USE_CONSOLE
	if (pComp->isSerializer())
	{
		// The registry compiler is broken with arrays. It doesn't delete extra items if the config got shorter
		// Solve it by defaulting the array before writing to it.
		pComp->Default("UserSets");
	}
	pComp->Value(mkNamingAdapt(UserSets, "UserSets",    C4PlayerControlAssignmentSets()));
	pComp->Value(mkNamingAdapt(MouseAutoScroll,      "MouseAutoScroll",      0 /* change default 33 to enable */ ));
	pComp->Value(mkNamingAdapt(GamepadGuiControl, "GamepadGuiControl",    0,     false, true));
#endif
}

#undef s

C4Config::C4Config()
{
	Default();
}

C4Config::~C4Config()
{
	fConfigLoaded = false;
}

void C4Config::Default()
{
	// force default values
	StdCompilerNull Comp; Comp.Compile(*this);
	fConfigLoaded=false;
}

void C4Config::GetConfigFileName(StdStrBuf &filename, const char *szConfigFile)
{
	if (szConfigFile)
	{
		// Config filename is specified
		filename.Ref(szConfigFile);
	}
	else
	{
		// Config filename from home
		StdStrBuf home(getenv("HOME"));
		if (home) { home += "/"; }
		filename.Copy(home);
#ifdef __APPLE__
		filename += "Library/Preferences/" C4ENGINEID ".config";
#else
		filename += ".clonk/" C4ENGINENICK "/config";
#endif
	}
}

bool C4Config::Load(const char *szConfigFile)
{
	try
	{
#ifdef _WIN32
		// Windows: Default load from registry, if no explicit config file is specified
		if (!szConfigFile)
		{
			StdCompilerConfigRead CfgRead(HKEY_CURRENT_USER, "Software\\" C4CFG_Company "\\" C4ENGINENAME);
			CfgRead.Compile(*this);
		}
		else
#endif
		{
			// Nonwindows or explicit config file: Determine filename to load config from
			StdStrBuf filename;
			GetConfigFileName(filename, szConfigFile);

			// Load config file into buf
			StdStrBuf buf;
			buf.LoadFromFile(filename.getData());

			if (buf.isNull())
			{
				// Config file not present?
#ifdef __linux__
				if (!szConfigFile)
				{
					StdStrBuf filename(getenv("HOME"));
					if (filename) { filename += "/"; }
					filename += ".clonk/" C4ENGINENICK;
					CreatePath(filename.getData());
				}
#endif
				// Buggy StdCompiler crashes when compiling a Null-StdStrBuf
				buf.Ref(" ");
			}

			// Read config from buffer
			StdCompilerINIRead IniRead;
			IniRead.setInput(buf);
			IniRead.Compile(*this);
		}
	}
	catch (StdCompiler::Exception *pExc)
	{
		// Configuration file syntax error?
		LogF("Error loading configuration: %s"/*LoadResStr("IDS_ERR_CONFREAD") - restbl not yet loaded*/, pExc->Msg.getData());
		delete pExc;
		return false;
	}

	// Config postinit
	General.DeterminePaths();
#ifdef HAVE_WINSOCK
	// Setup WS manually, so c4group doesn't depend on C4NetIO
	WSADATA wsadata;
	bool fWinSock = !WSAStartup(WINSOCK_VERSION, &wsadata);
#endif
	if (SEqual(Network.Nick.getData(), "Unknown"))
	{
		char LocalName[25+1]; *LocalName = 0;
		gethostname(LocalName, 25);
		if (*LocalName) Network.Nick.Copy(LocalName);
	}
#ifdef HAVE_WINSOCK
	if (fWinSock) WSACleanup();
#endif
	General.DefaultLanguage();
	// Warning against invalid ports
	if (Config.Network.PortTCP>0 && Config.Network.PortTCP == Config.Network.PortRefServer)
	{
		Log("Warning: Network TCP port and reference server port both set to same value - increasing reference server port!");
		++Config.Network.PortRefServer;
		if (Config.Network.PortRefServer>=65536) Config.Network.PortRefServer = C4NetStdPortRefServer;
	}
	if (Config.Network.PortUDP>0 && Config.Network.PortUDP == Config.Network.PortDiscovery)
	{
		Log("Warning: Network UDP port and LAN game discovery port both set to same value - increasing discovery port!");
		++Config.Network.PortDiscovery;
		if (Config.Network.PortDiscovery>=65536) Config.Network.PortDiscovery = C4NetStdPortDiscovery;
	}
	// Empty nick already defaults to GetRegistrationData("Nick") or
	// Network.LocalName at relevant places.
	fConfigLoaded = true;
	if (szConfigFile) ConfigFilename.Copy(szConfigFile); else ConfigFilename.Clear();
	return true;
}

bool C4Config::Save()
{
	try
	{
#ifdef _WIN32
		if (!ConfigFilename.getLength())
		{
			// Windows: Default save to registry, if it wasn't loaded from file
			StdCompilerConfigWrite CfgWrite(HKEY_CURRENT_USER, "Software\\" C4CFG_Company "\\" C4ENGINENAME);
			CfgWrite.Decompile(*this);
		}
		else
#endif
		{
			StdStrBuf filename;
			GetConfigFileName(filename, ConfigFilename.getLength() ? ConfigFilename.getData() : nullptr);
			StdCompilerINIWrite IniWrite;
			IniWrite.Decompile(*this);
			IniWrite.getOutput().SaveToFile(filename.getData());
		}
	}
	catch (StdCompiler::Exception *pExc)
	{
		LogF(LoadResStr("IDS_ERR_CONFSAVE"), pExc->Msg.getData());
		delete pExc;
		return false;
	}
	return true;
}

void C4ConfigGeneral::DeterminePaths()
{
#ifdef _WIN32
	// Exe path
	wchar_t apath[CFG_MaxString];
	if (GetModuleFileNameW(nullptr,apath,CFG_MaxString))
	{
		ExePath = StdStrBuf(apath);
		TruncatePath(ExePath.getMData());
		ExePath.SetLength(SLen(ExePath.getMData()));
		ExePath.AppendBackslash();
	}
	// Temp path
	GetTempPathW(CFG_MaxString,apath);
	TempPath = StdStrBuf(apath);
	if (TempPath[0]) TempPath.AppendBackslash();
#elif defined(PROC_SELF_EXE)
	ExePath.SetLength(1024);
	ssize_t l = readlink(PROC_SELF_EXE, ExePath.getMData(), 1024);
	if (l < -1)
	{
		ExePath.Ref(".");
	}
	else
	{
		ExePath.SetLength(l);
		GetParentPath(ExePath.getData(), &ExePath);
		ExePath.AppendBackslash();
	}
	const char * t = getenv("TMPDIR");
	if (t)
	{
		TempPath = t;
		TempPath.AppendBackslash();
	}
	else
		TempPath = "/tmp/";
#else
	// Mac: Just use the working directory as ExePath.
	ExePath = GetWorkingDirectory();
	ExePath.AppendBackslash();
	TempPath = "/tmp/";
#endif

	// Find system-wide data path
#if defined(_WIN32)
	// Use ExePath: on windows, everything is installed to one directory
	SCopy(ExePath.getMData(),SystemDataPath);
#elif defined(__APPLE__)
	SCopy(::Application.GetGameDataPath().c_str(),SystemDataPath);
#elif defined(WITH_APPDIR_INSTALLATION)
	// AppDir: layout like normal unix installation, but relative to executable.
	auto str = FormatString("%s%s", ExePath.getMData(), OC_SYSTEM_DATA_DIR);
	SCopy(str.getMData(), SystemDataPath);
#elif defined(WITH_AUTOMATIC_UPDATE)
	// WITH_AUTOMATIC_UPDATE builds are our tarball releases and
	// development snapshots, i.e. where the game data is at the
	// same location as the executable.
	SCopy(ExePath.getMData(),SystemDataPath);
#elif defined(OC_SYSTEM_DATA_DIR)
	SCopy(OC_SYSTEM_DATA_DIR, SystemDataPath);
#else
#error Please define OC_SYSTEM_DATA_DIR!
#endif
	AppendBackslash(SystemDataPath);

	// Find user-specific data path
	if (ConfigUserPath[0])
		SCopy(ConfigUserPath, UserDataPath);
	else
#if defined(_WIN32)
		SCopy(R"(%APPDATA%\)" C4ENGINENAME, UserDataPath);
#elif defined(__APPLE__)
		SCopy("$HOME/Library/Application Support/" C4ENGINENAME, UserDataPath);
#else
		SCopy("$HOME/.clonk/" C4ENGINENICK, UserDataPath);
#endif
	C4Config::ExpandEnvironmentVariables(UserDataPath, CFG_MaxString);
	AppendBackslash(UserDataPath);

	// Screenshot path
	SCopy(UserDataPath, ScreenshotPath, CFG_MaxString-1);
	if (ScreenshotFolder.getLength()+std::strlen(ScreenshotPath)+1<=CFG_MaxString)
	{
		SAppend(ScreenshotFolder.getData(), ScreenshotPath);
		AppendBackslash(ScreenshotPath);
	}
	// Create user path if it doesn't already exist
	CreatePath(UserDataPath);
}

static char AtPathFilename[_MAX_PATH+1];

const char* C4Config::AtExePath(const char *szFilename)
{
	SCopy(General.ExePath.getData(),AtPathFilename,_MAX_PATH);
	SAppend(szFilename,AtPathFilename,_MAX_PATH);
	return AtPathFilename;
}

const char* C4Config::AtUserDataPath(const char *szFilename)
{
	SCopy(General.UserDataPath, AtPathFilename, _MAX_PATH);
	SAppend(szFilename, AtPathFilename, _MAX_PATH);
	return AtPathFilename;
}

const char* C4Config::AtSystemDataPath(const char *szFilename)
{
	SCopy(General.SystemDataPath, AtPathFilename, _MAX_PATH);
	SAppend(szFilename, AtPathFilename, _MAX_PATH);
	return AtPathFilename;
}

const char* C4Config::AtTempPath(const char *szFilename)
{
	SCopy(General.TempPath.getData(),AtPathFilename,_MAX_PATH);
	SAppend(szFilename,AtPathFilename,_MAX_PATH);
	return AtPathFilename;
}

const char* C4Config::AtNetworkPath(const char *szFilename)
{
	SCopy(General.UserDataPath,AtPathFilename,_MAX_PATH);
	SAppend(Network.WorkPath,AtPathFilename,_MAX_PATH);
	SAppend(szFilename,AtPathFilename,_MAX_PATH);
	return AtPathFilename;
}

const char *C4Config::AtScreenshotPath(const char *szFilename)
{
	int len;
	SCopy(General.ScreenshotPath,AtPathFilename,_MAX_PATH);
	if ((len = SLen(AtPathFilename)))
		if (AtPathFilename[len-1] == DirectorySeparator)
			AtPathFilename[len-1] = '\0';
	if (!CreatePath(AtPathFilename))
	{
		SCopy(General.UserDataPath,AtPathFilename,_MAX_PATH);
	}
	AppendBackslash(AtPathFilename);
	SAppend(szFilename,AtPathFilename,_MAX_PATH);
	return AtPathFilename;
}


bool C4ConfigGeneral::CreateSaveFolder(const char *strDirectory, const char *strLanguageTitle)
{
	// Create directory if needed
	if (!CreatePath(strDirectory))
		return false;
	// Create title component if needed
	char lang[3]; SCopy(Config.General.Language, lang, 2);
	StdStrBuf strTitleFile; strTitleFile.Format("%s%c%s", strDirectory, DirectorySeparator, C4CFN_WriteTitle);
	StdStrBuf strTitleData; strTitleData.Format("%s:%s", lang, strLanguageTitle);
	CStdFile hFile;
	if (!FileExists(strTitleFile.getData()))
		if (!hFile.Create(strTitleFile.getData()) || !hFile.WriteString(strTitleData.getData()) || !hFile.Close())
			return false;
	// Save folder seems okay
	return true;
}


const char* C4ConfigNetwork::GetLeagueServerAddress()
{
	// Alternate (configurable) league server
	if (UseAlternateServer)
		return AlternateServerAddress;
	// Standard (hardcoded) official league server
	else
		return "league.openclonk.org:80/league.php";
}

void C4ConfigNetwork::CheckPortsForCollisions()
{
	// check for port collisions
	if (PortTCP != -1 && PortTCP == PortRefServer)
	{
		LogSilentF("Network: TCP Port collision, setting defaults");
		PortTCP = C4NetStdPortTCP;
		PortRefServer = C4NetStdPortRefServer;
	}
	if (PortUDP != -1 && PortUDP == PortDiscovery)
	{
		LogSilentF("Network: UDP Port collision, setting defaults");
		PortUDP = C4NetStdPortUDP;
		PortDiscovery = C4NetStdPortDiscovery;
	}
}

void C4ConfigNetwork::SetLeagueLoginData(const char *szServer, const char *szPlayerName, const char *szAccount, const char *szLoginToken)
{
	// ideally, there would be a list to store multiple logins
	// however, we don't really support multiplayer at one computer at the moment anyway
	LastLeagueServer.Copy(szServer);
	LastLeaguePlayerName.Copy(szPlayerName);
	LastLeagueAccount.Copy(szAccount);
	LastLeagueLoginToken.Copy(szLoginToken);
}

bool C4ConfigNetwork::GetLeagueLoginData(const char *szServer, const char *szPlayerName, StdStrBuf *pAccount, StdStrBuf *pLoginToken) const
{
	// check if last login matches and store if desired
	if (LastLeagueServer == szServer && LastLeaguePlayerName == szPlayerName)
	{
		pAccount->Copy(LastLeagueAccount);
		pLoginToken->Copy(LastLeagueLoginToken);
		return true;
	}
	return false;
}

void C4ConfigControls::ResetKeys()
{
	UserSets.Clear();
}

const char* C4Config::AtUserDataRelativePath(const char *szFilename)
{
	// Specified file is located in UserDataPath: return relative path
	return GetRelativePathS(szFilename, General.UserDataPath);
}

const char* C4Config::AtSystemDataRelativePath(const char *szFilename)
{
	// Specified file is located in SystemDataPath: return relative path
	return GetRelativePathS(szFilename, General.SystemDataPath);
}

const char* C4Config::AtRelativePath(const char *szFilename)
{
	const char *szPath = GetRelativePathS(szFilename, General.UserDataPath);
	if (szPath == szFilename)
		return GetRelativePathS(szFilename, General.SystemDataPath);
	return szPath;
}

void C4Config::ForceRelativePath(StdStrBuf *sFilename)
{
	assert(sFilename);
	// Specified file is located in SystemDataPath?
	const char *szRelative = GetRelativePathS(sFilename->getData(), General.SystemDataPath);
	if (szRelative != sFilename->getData())
	{
		// return relative path
		StdStrBuf sTemp; sTemp.Copy(szRelative);
		sFilename->Take(std::move(sTemp));
	}
	else
	{
		// not in ExePath: Is it a global path?
		if (IsGlobalPath(sFilename->getData()))
		{
			// then shorten it (e.g. C:\Temp\Missions.ocf\Goldmine.ocs to Missions.ocf\Goldmine.ocs)
			StdStrBuf sTemp; sTemp.Copy(GetC4Filename(sFilename->getData()));
			sFilename->Take(std::move(sTemp));
		}
	}
}

void C4ConfigGeneral::DefaultLanguage()
{
	// No language defined: default to German or English by system language
	if (!Language[0])
	{
		if (IsGermanSystem())
			SCopy("DE - Deutsch", Language);
		else
			SCopy("US - English", Language);
	}
	// No fallback sequence defined: use primary language list
	if (!LanguageEx[0])
		GetLanguageSequence(Language, LanguageEx);
}

bool C4Config::Registered()
{
	// Dummy function: to be overloaded in C4Config
	return true;
}

bool C4Config::Init()
{
	return true;
}

const char* C4Config::GetSubkeyPath(const char *strSubkey)
{
	static char key[1024 + 1];
#ifdef _WIN32
	sprintf(key, R"(Software\%s\%s\%s)", C4CFG_Company, C4ENGINENAME, strSubkey);
#else
	sprintf(key, "%s", strSubkey);
#endif
	return key;
}

int C4ConfigGeneral::GetLanguageSequence(const char *strSource, char *strTarget)
{
	// Copy a condensed list of language codes from the source list to the target string,
	// skipping any whitespace or long language descriptions. Language sequences are
	// comma separated.
	int iCount = 0;
	char strLang[2 + 1];
	for (int i = 0; SCopySegment(strSource, i, strLang, ',', 2, true); i++)
		if (strLang[0])
		{
			if (strTarget[0]) SAppendChar(',', strTarget);
			SAppend(strLang, strTarget);
			iCount++;
		}
	return iCount;
}

void C4ConfigStartup::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(HideMsgGfxEngineChange,      "HideMsgGfxEngineChange",     0));
	pComp->Value(mkNamingAdapt(HideMsgGfxBitDepthChange,    "HideMsgGfxBitDepthChange",   0));
	pComp->Value(mkNamingAdapt(HideMsgMMTimerChange,        "HideMsgMMTimerChange",       0));
	pComp->Value(mkNamingAdapt(HideMsgStartDedicated,       "HideMsgStartDedicated",      0));
	pComp->Value(mkNamingAdapt(HideMsgPlrTakeOver,          "HideMsgPlrTakeOver",         0));
	pComp->Value(mkNamingAdapt(HideMsgPlrNoTakeOver,        "HideMsgPlrNoTakeOver",       0));
	pComp->Value(mkNamingAdapt(HideMsgNoOfficialLeague,     "HideMsgNoOfficialLeague",    0));
	pComp->Value(mkNamingAdapt(HideMsgIRCDangerous,         "HideMsgIRCDangerous",        0));
	pComp->Value(mkNamingAdapt(AlphabeticalSorting,         "AlphabeticalSorting",        0));
	pComp->Value(mkNamingAdapt(LastPortraitFolderIdx,       "LastPortraitFolderIdx",      0));
}

void C4Config::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(General,     "General"     ));
	pComp->Value(mkNamingAdapt(Controls,    "Controls"    ));
	for (int i=0; i<C4ConfigMaxGamepads; ++i)
		pComp->Value(mkNamingAdapt(Gamepads[i],     FormatString("Gamepad%d", i).getData()));
	pComp->Value(mkNamingAdapt(Graphics,    "Graphics"    ));
	pComp->Value(mkNamingAdapt(Sound,       "Sound"       ));
	pComp->Value(mkNamingAdapt(Network,     "Network"     ));
	pComp->Value(mkNamingAdapt(Lobby,       "Lobby"       ));
	pComp->Value(mkNamingAdapt(IRC,         "IRC"         ));
	pComp->Value(mkNamingAdapt(Developer,   "Developer"   ));
	pComp->Value(mkNamingAdapt(Startup,     "Startup"     ));
	pComp->Value(mkNamingAdapt(Security,    "Security"    ));
}

bool C4Config::AddModule(const char *szPath, char *szModules)
{
	return SAddModule(szModules,szPath);
}

bool C4Config::IsModule(const char *szPath, char *szModules)
{
	return SIsModule(szModules,szPath);
}

bool C4Config::RemoveModule(const char *szPath, char *szModules)
{
	return SRemoveModule(szModules,szPath);
}

void C4Config::ExpandEnvironmentVariables(char *strPath, size_t iMaxLen)
{
#ifdef _WIN32
	wchar_t buf[_MAX_PATH + 1];
	ExpandEnvironmentStringsW(GetWideChar(strPath), buf, _MAX_PATH);
	SCopy(StdStrBuf(buf).getData(), strPath, iMaxLen);
#else // __linux__ or __APPLE___
	StdStrBuf home(getenv("HOME"));
	char* rest;
	if (home && (rest = const_cast<char *>(SSearch(strPath, "$HOME"))) && (std::strlen(strPath) - 5 + home.getLength() <= iMaxLen))
	{
		// String replace... there might be a more elegant way to do this.
		memmove(rest + home.getLength() - SLen("$HOME"), rest, SLen(rest) + 1);
		strncpy(rest - SLen("$HOME"), home.getData(), home.getLength());
	}
#endif
}

void C4Config::CleanupTempUpdateFolder()
{
	// Get rid of update path present from before update
	if (*General.TempUpdatePath)
	{
		EraseItem(General.TempUpdatePath);
		*General.TempUpdatePath = '\0';
	}
}

const char *C4Config::MakeTempUpdateFolder()
{
	// just pick a temp name
	StdStrBuf sTempName;
	sTempName.Copy(AtTempPath("update"));
	MakeTempFilename(&sTempName);
	SCopy(sTempName.getData(), General.TempUpdatePath);
	CreatePath(General.TempUpdatePath);
	return General.TempUpdatePath;
}

const char *C4Config::AtTempUpdatePath(const char *szFilename)
{
	SCopy(General.TempUpdatePath,AtPathFilename,_MAX_PATH-1);
	AppendBackslash(AtPathFilename);
	SAppend(szFilename,AtPathFilename,_MAX_PATH);
	return AtPathFilename;
}

C4Config Config;
