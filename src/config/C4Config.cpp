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

#ifdef USE_CONSOLE
#define DONCOFF 0
#else
#define DONCOFF 1
#endif

#include "game/C4Application.h"

void C4ConfigGeneral::CompileFunc(StdCompiler *compiler)
{
	// For those without the ability to intuitively guess what the falses and trues mean:
	// its mkNamingAdapt(field, name, default, fPrefillDefault, fStoreDefault)
	// where fStoreDefault writes out the value to the config even if it's the same as the default.
#define s mkStringAdaptM
	compiler->Value(mkNamingAdapt(s(Name),             "Name",               ""             ));
	compiler->Value(mkNamingAdapt(s(Language),         "Language",           "", false, true));
	compiler->Value(mkNamingAdapt(s(LanguageEx),       "LanguageEx",         "", false, true));
	compiler->Value(mkNamingAdapt(s(Participants),     "Participants",       ""             ));

	// deliberately not grandfathering UserPath setting, since it was written to config by default
	compiler->Value(mkNamingAdapt(s(ConfigUserPath),   "UserDataPath",       "", false, true));
	compiler->Value(mkNamingAdapt(s(ModsDataPath),     "ModsDataPath",       "", false, true));
	// assimilate old data
	compiler->Value(mkNamingAdapt(s(Adopt.PlayerPath), "PlayerPath",       ""));

	// temporary path only set during updates
	compiler->Value(mkNamingAdapt(s(TempUpdatePath),   "TempUpdatePath",     ""));

	compiler->Value(mkNamingAdapt(s(MissionAccess),    "MissionAccess",      "", false, true));
	compiler->Value(mkNamingAdapt(FPS,                 "FPS",                0              ));
	compiler->Value(mkNamingAdapt(DefRec,              "DefRec",             0              ));
	compiler->Value(mkNamingAdapt(ScreenshotFolder,    "ScreenshotFolder",   "Screenshots",  false, true));
	compiler->Value(mkNamingAdapt(ModsFolder,          "ModsFolder",         "mods",  false, true));
	compiler->Value(mkNamingAdapt(ScrollSmooth,        "ScrollSmooth",       4              ));
	compiler->Value(mkNamingAdapt(AlwaysDebug,         "DebugMode",          0              ));
	compiler->Value(mkNamingAdapt(OpenScenarioInGameMode, "OpenScenarioInGameMode", 0   )); 
#ifdef _WIN32
	compiler->Value(mkNamingAdapt(MMTimer,             "MMTimer",            1              ));
#endif
	compiler->Value(mkNamingAdapt(s(RXFontName),       "FontName",           C4DEFAULT_FONT_NAME,   false, true));
	compiler->Value(mkNamingAdapt(RXFontSize,          "FontSize",           14,            false, true));
	compiler->Value(mkNamingAdapt(GamepadEnabled,      "GamepadEnabled",     true           ));
	compiler->Value(mkNamingAdapt(FirstStart,          "FirstStart",         true           ));
	compiler->Value(mkNamingAdapt(ConfigResetSafety,   "ConfigResetSafety",  static_cast<int32_t>(ConfigResetSafetyVal) ));
}

void C4ConfigDeveloper::CompileFunc(StdCompiler *compiler)
{
	compiler->Value(mkNamingAdapt(AutoFileReload,      "AutoFileReload",     1                    , false, true));
	compiler->Value(mkNamingAdapt(s(TodoFilename),     "TodoFilename",       "{SCENARIO}/TODO.txt", false, true));
	compiler->Value(mkNamingAdapt(s(AltTodoFilename),  "AltTodoFilename2",   "{USERPATH}/TODO.txt", false, true));
	compiler->Value(mkNamingAdapt(MaxScriptMRU,        "MaxScriptMRU",       30                   , false, false));
	compiler->Value(mkNamingAdapt(DebugShapeTextures,  "DebugShapeTextures", 0                    , false, true));
	compiler->Value(mkNamingAdapt(ShowHelp,            "ShowHelp",           true                 , false, false));
	for (int32_t i = 0; i < CFG_MaxEditorMRU; ++i)
	{
		compiler->Value(mkNamingAdapt(s(RecentlyEditedSzenarios[i]), FormatString("EditorMRU%02d", (int)i).getData(), "", false, false));
	}
}

void C4ConfigDeveloper::AddRecentlyEditedScenario(const char *filename)
{
	if (!filename || !*filename)
	{
		return;
	}
	// Put given scenario first in list by moving all other scenarios down
	// Check how many scenarios to move down the list. Stop moving down when the given scenario is in the list
	int32_t move_down_num;
	for (move_down_num = 0; move_down_num < CFG_MaxEditorMRU - 1; ++move_down_num)
	{
		if (!strncmp(filename, RecentlyEditedSzenarios[move_down_num], CFG_MaxString))
		{
			break;
		}
	}
	// Move them down
	for (int32_t i = move_down_num; i > 0; --i)
	{
		strcpy(RecentlyEditedSzenarios[i], RecentlyEditedSzenarios[i - 1]);
	}
	// Put current scenario in
	strncpy(RecentlyEditedSzenarios[0], filename, CFG_MaxString);
}

void C4ConfigGraphics::CompileFunc(StdCompiler *compiler)
{
	compiler->Value(mkNamingAdapt(ResX,                  "ResolutionX",         -1             ,false, true));
	compiler->Value(mkNamingAdapt(ResY,                  "ResolutionY",         -1             ,false, true));
	compiler->Value(mkNamingAdapt(WindowX,               "WindowX",              800           ,false, true));
	compiler->Value(mkNamingAdapt(WindowY,               "WindowY",              600           ,false, true));
	compiler->Value(mkNamingAdapt(RefreshRate,           "RefreshRate",          0             ));
	compiler->Value(mkNamingAdapt(SplitscreenDividers,   "SplitscreenDividers",  1             ));
	compiler->Value(mkNamingAdapt(ShowStartupMessages,   "ShowStartupMessages",  1             ,false, true));
	compiler->Value(mkNamingAdapt(VerboseObjectLoading,  "VerboseObjectLoading", 0             ));
	compiler->Value(mkNamingAdapt(MenuTransparency,      "MenuTransparency",     1             ,false, true));
	compiler->Value(mkNamingAdapt(UpperBoard,            "UpperBoard",           1             ,false, true));
	compiler->Value(mkNamingAdapt(ShowClock,             "ShowClock",            0             ,false, true));
	compiler->Value(mkNamingAdapt(ShowCrewNames,         "ShowCrewNames",        1             ,false, true));
	compiler->Value(mkNamingAdapt(ShowCrewCNames,        "ShowCrewCNames",       0             ,false, true));
	compiler->Value(mkNamingAdapt(Windowed,              "Windowed",             0             ,false, true));
	compiler->Value(mkNamingAdapt(PXSGfx,                "PXSGfx"  ,             1             ));
	compiler->Value(mkNamingAdapt(Gamma,                 "Gamma"  ,              100           ));
	compiler->Value(mkNamingAdapt(Monitor,               "Monitor",              0             )); // 0 = D3DADAPTER_DEFAULT
	compiler->Value(mkNamingAdapt(MaxRefreshDelay,       "MaxRefreshDelay",      30            ));
	compiler->Value(mkNamingAdapt(NoOffscreenBlits,      "NoOffscreenBlits",     1             ));
	compiler->Value(mkNamingAdapt(MultiSampling,         "MultiSampling",        4             ));
	compiler->Value(mkNamingAdapt(AutoFrameSkip,         "AutoFrameSkip",        1          ));
	compiler->Value(mkNamingAdapt(MouseCursorSize,       "MouseCursorSize",      50            ));
}

void C4ConfigSound::CompileFunc(StdCompiler *compiler)
{
	compiler->Value(mkNamingAdapt(RXSound,               "Sound",                DONCOFF       ,false, true));
	compiler->Value(mkNamingAdapt(RXMusic,               "Music",                DONCOFF       ,false, true));
	compiler->Value(mkNamingAdapt(FEMusic,               "MenuMusic",            DONCOFF       ,false, true));
	compiler->Value(mkNamingAdapt(FESamples,             "MenuSound",            DONCOFF       ,false, true));
	compiler->Value(mkNamingAdapt(Verbose,               "Verbose",              0             ));
	compiler->Value(mkNamingAdapt(MusicVolume,           "MusicVolume2",         40            ,false, true));
	compiler->Value(mkNamingAdapt(SoundVolume,           "SoundVolume",          100           ,false, true));
}

void C4ConfigNetwork::CompileFunc(StdCompiler *compiler)
{
	compiler->Value(mkNamingAdapt(ControlRate,             "ControlRate",          3            ,false, true));
	compiler->Value(mkNamingAdapt(ControlPreSend,          "ControlPreSend",       -1            ));
	compiler->Value(mkNamingAdapt(s(WorkPath),             "WorkPath",             "Network"     ,false, true));
	compiler->Value(mkNamingAdapt(Lobby,                   "Lobby",                0             ));
	compiler->Value(mkNamingAdapt(NoRuntimeJoin,           "NoRuntimeJoin",        1             ,false, true));
	compiler->Value(mkNamingAdapt(NoReferenceRequest,      "NoReferenceRequest",   0             ));
	compiler->Value(mkNamingAdapt(MaxResSearchRecursion,   "MaxResSearchRecursion",1             ,false, true));
	compiler->Value(mkNamingAdapt(Comment,                 "Comment",              ""            ,false, true));
	compiler->Value(mkNamingAdapt(PortTCP,                 "PortTCP",              C4NetStdPortTCP       ,false, true));
	compiler->Value(mkNamingAdapt(PortUDP,                 "PortUDP",              C4NetStdPortUDP       ,false, true));
	compiler->Value(mkNamingAdapt(EnableUPnP,              "EnableUPnP",           1             , false, true));
	compiler->Value(mkNamingAdapt(PortDiscovery,           "PortDiscovery",        C4NetStdPortDiscovery ,false, true));
	compiler->Value(mkNamingAdapt(PortRefServer,           "PortRefServer",        C4NetStdPortRefServer ,false, true));
	compiler->Value(mkNamingAdapt(ControlMode,             "ControlMode",          0             ));
	compiler->Value(mkNamingAdapt(Nick,                    "Nick",                 ""            ,false, true));
	compiler->Value(mkNamingAdapt(MaxLoadFileSize,         "MaxLoadFileSize",      5*1024*1024   ,false, true));

	compiler->Value(mkNamingAdapt(MasterServerSignUp,      "MasterServerSignUp",   1             ));
	compiler->Value(mkNamingAdapt(MasterServerActive,      "MasterServerActive",   0             ));
	compiler->Value(mkNamingAdapt(MasterKeepPeriod,        "MasterKeepPeriod",     60            ));
	compiler->Value(mkNamingAdapt(MasterReferencePeriod,   "MasterReferencePeriod",120           ));
	compiler->Value(mkNamingAdapt(LeagueServerSignUp,      "LeagueServerSignUp",   0             ));
	compiler->Value(mkNamingAdapt(UseAlternateServer,      "UseAlternateServer",   0             ));
	compiler->Value(mkNamingAdapt(s(AlternateServerAddress),"AlternateServerAddress", "league.openclonk.org:80/league.php"));
	compiler->Value(mkNamingAdapt(UseAlternateModDatabaseServer, "UseAlternateModDatabaseServer", 0));
	compiler->Value(mkNamingAdapt(s(AlternateModDatabaseServerAddress), "AlternateModDatabaseServerAddress", "mods.openclonk.org/api/"));
	compiler->Value(mkNamingAdapt(s(LastPassword),         "LastPassword",         "Wipf"        ));
#ifdef WITH_AUTOMATIC_UPDATE
	compiler->Value(mkNamingAdapt(s(UpdateServerAddress),  "UpdateServerAddress",     "www.openclonk.org:80/update/"));
	compiler->Value(mkNamingAdapt(AutomaticUpdate,         "AutomaticUpdate",      0             ,false, true));
	compiler->Value(mkNamingAdapt(LastUpdateTime,          "LastUpdateTime",       0             ));
#endif
	compiler->Value(mkNamingAdapt(AsyncMaxWait,            "AsyncMaxWait",         2             ));
	compiler->Value(mkNamingAdapt(PacketLogging,           "PacketLogging",        0             ));
	

	compiler->Value(mkNamingAdapt(s(PuncherAddress),       "PuncherAddress",       "netpuncher.openclonk.org:11115"));
	compiler->Value(mkNamingAdapt(mkParAdapt(LastLeagueServer, StdCompiler::RCT_All),     "LastLeagueServer",     ""            ));
	compiler->Value(mkNamingAdapt(mkParAdapt(LastLeaguePlayerName, StdCompiler::RCT_All), "LastLeaguePlayerName", ""            ));
	compiler->Value(mkNamingAdapt(mkParAdapt(LastLeagueAccount, StdCompiler::RCT_All),    "LastLeagueAccount",    ""            ));
	compiler->Value(mkNamingAdapt(mkParAdapt(LastLeagueLoginToken, StdCompiler::RCT_All), "LastLeagueLoginToken", ""            ));
}

void C4ConfigLobby::CompileFunc(StdCompiler *compiler)
{
	compiler->Value(mkNamingAdapt(AllowPlayerSave,         "AllowPlayerSave",      0             ,false, false));
	compiler->Value(mkNamingAdapt(CountdownTime,           "CountdownTime",        5             ,false, false));
}

void C4ConfigIRC::CompileFunc(StdCompiler *compiler)
{
	compiler->Value(mkNamingAdapt(s(Server),               "Server",               "irc.euirc.net", false, true));
	compiler->Value(mkNamingAdapt(s(Nick),                 "Nick",                 ""                    , false, true));
	compiler->Value(mkNamingAdapt(s(RealName),             "RealName",             ""                    , false, true));
	compiler->Value(mkNamingAdapt(s(Channel),              "Channel",              "#openclonk"    , false, true));
	compiler->Value(mkNamingAdapt(AllowAllChannels,        "AllowAllChannels",     0                     , false, true));
}

void C4ConfigSecurity::CompileFunc(StdCompiler *compiler)
{
	compiler->Value(mkNamingAdapt(WasRegistered,           "WasRegistered",        0                   ));
#ifdef _WIN32
	compiler->Value(mkNamingAdapt(s(KeyPath),              "KeyPath",              R"(%APPDATA%\)" C4ENGINENAME, false, true));
#elif defined(__linux__)
	compiler->Value(mkNamingAdapt(s(KeyPath),              "KeyPath",              "$HOME/.clonk/" C4ENGINENICK, false, true));
#elif defined(__APPLE__)
	compiler->Value(mkNamingAdapt(s(KeyPath),              "KeyPath",              "$HOME/Library/Application Support/" C4ENGINENAME, false, true));
#endif
}

void C4ConfigGamepad::CompileFunc(StdCompiler *compiler, bool buttons_only)
{
	/* The defaults here are for a Logitech Dual Action under Linux-SDL. Better than nothing, I guess. */
	if (!buttons_only)
	{
		for (int i = 0; i < 6; ++i)
		{
			compiler->Value(mkNamingAdapt(AxisMin[i],          FormatString("Axis%dMin", i).getData(),     0u));
			compiler->Value(mkNamingAdapt(AxisMax[i],          FormatString("Axis%dMax", i).getData(),     0u));
			compiler->Value(mkNamingAdapt(AxisCalibrated[i],   FormatString("Axis%dCalibrated", i).getData(), false));
		}
	}
	compiler->Value(mkNamingAdapt(Button[0],               "Button1",              -1          ));
	compiler->Value(mkNamingAdapt(Button[1],               "Button2",              -1          ));
	compiler->Value(mkNamingAdapt(Button[2],               "Button3",              -1          ));
	compiler->Value(mkNamingAdapt(Button[3],               "Button4",              -1          ));
	compiler->Value(mkNamingAdapt(Button[4],               "Button5",              -1          ));
	compiler->Value(mkNamingAdapt(Button[5],               "Button6",              -1          ));
	compiler->Value(mkNamingAdapt(Button[6],               "Button7",              -1          ));
	compiler->Value(mkNamingAdapt(Button[7],               "Button8",              -1          ));
	compiler->Value(mkNamingAdapt(Button[8],               "Button9",              -1          ));
	compiler->Value(mkNamingAdapt(Button[9],               "Button10",             -1          ));
	compiler->Value(mkNamingAdapt(Button[10],              "Button11",             -1          ));
	compiler->Value(mkNamingAdapt(Button[11],              "Button12",             -1          ));
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

void C4ConfigControls::CompileFunc(StdCompiler *compiler)
{
#ifndef USE_CONSOLE
	if (compiler->isSerializer())
	{
		// The registry compiler is broken with arrays. It doesn't delete extra items if the config got shorter
		// Solve it by defaulting the array before writing to it.
		compiler->Default("UserSets");
	}
	compiler->Value(mkNamingAdapt(UserSets, "UserSets",    C4PlayerControlAssignmentSets()));
	compiler->Value(mkNamingAdapt(MouseAutoScroll,      "MouseAutoScroll",      0 /* change default 33 to enable */ ));
	compiler->Value(mkNamingAdapt(GamepadGuiControl, "GamepadGuiControl",    0,     false, true));
#endif
}

#undef s

C4Config::C4Config()
{
	Default();
}

C4Config::~C4Config()
{
	ConfigLoaded = false;
}

void C4Config::Default()
{
	// force default values
	StdCompilerNull Comp; Comp.Compile(*this);
	ConfigLoaded = false;
}

void C4Config::GetConfigFileName(StdStrBuf &filename, const char *config_file)
{
	if (config_file)
	{
		// Config filename is specified
		filename.Ref(config_file);
	}
	else
	{
		// Config filename from home
		StdStrBuf home(getenv("HOME"));
		if (home)
		{
			home += "/";
		}
		filename.Copy(home);
#ifdef __APPLE__
		filename += "Library/Preferences/" C4ENGINEID ".config";
#else
		filename += ".clonk/" C4ENGINENICK "/config";
#endif
	}
}

bool C4Config::Load(const char *config_file)
{
	try
	{
#ifdef _WIN32
		// Windows: Default load from registry, if no explicit config file is specified
		if (!config_file)
		{
			StdCompilerConfigRead CfgRead(HKEY_CURRENT_USER, "Software\\" C4CFG_Company "\\" C4ENGINENAME);
			CfgRead.Compile(*this);
		}
		else
#endif
		{
			// Nonwindows or explicit config file: Determine filename to load config from
			StdStrBuf filename;
			GetConfigFileName(filename, config_file);

			// Load config file into buf
			StdStrBuf buf;
			buf.LoadFromFile(filename.getData());

			if (buf.isNull())
			{
				// Config file not present?
#ifdef __linux__
				if (!config_file)
				{
					StdStrBuf filename(getenv("HOME"));
					if (filename)
					{
						filename += "/";
					}
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
		char LocalName[25+1];
		*LocalName = 0;
		gethostname(LocalName, 25);
		if (*LocalName)
		{
			Network.Nick.Copy(LocalName);
		}
	}
#ifdef HAVE_WINSOCK
	if (fWinSock)
	{
		WSACleanup();
	}
#endif
	General.DefaultLanguage();
	// Warning against invalid ports
	if (Config.Network.PortTCP > 0 && Config.Network.PortTCP == Config.Network.PortRefServer)
	{
		Log("Warning: Network TCP port and reference server port both set to same value - increasing reference server port!");
		++Config.Network.PortRefServer;
		if (Config.Network.PortRefServer >= 65536)
		{
			Config.Network.PortRefServer = C4NetStdPortRefServer;
		}
	}
	if (Config.Network.PortUDP > 0 && Config.Network.PortUDP == Config.Network.PortDiscovery)
	{
		Log("Warning: Network UDP port and LAN game discovery port both set to same value - increasing discovery port!");
		++Config.Network.PortDiscovery;
		if (Config.Network.PortDiscovery >= 65536)
		{
			Config.Network.PortDiscovery = C4NetStdPortDiscovery;
		}
	}
	// Empty nick already defaults to GetRegistrationData("Nick") or
	// Network.LocalName at relevant places.
	ConfigLoaded = true;
	if (config_file)
	{
		ConfigFilename.Copy(config_file);
	}
	else
	{
		ConfigFilename.Clear();
	}
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
	catch (StdCompiler::Exception *exception)
	{
		LogF(LoadResStr("IDS_ERR_CONFSAVE"), exception->Msg.getData());
		delete exception;
		return false;
	}
	return true;
}

void C4ConfigGeneral::DeterminePaths()
{
#ifdef _WIN32
	// Exe path
	wchar_t apath[CFG_MaxString];
	if (GetModuleFileNameW(nullptr, apath, CFG_MaxString))
	{
		ExePath = StdStrBuf(apath);
		TruncatePath(ExePath.getMData());
		ExePath.SetLength(SLen(ExePath.getMData()));
		ExePath.AppendBackslash();
	}

	// Temp path
	GetTempPathW(CFG_MaxString,apath);
	TempPath = StdStrBuf(apath);
	if (TempPath[0])
	{
		TempPath.AppendBackslash();
	}
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
	{
		TempPath = "/tmp/";
	}
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
	SCopy(::Application.GetGameDataPath().c_str(), SystemDataPath);
#elif defined(WITH_AUTOMATIC_UPDATE) && defined(WITH_APPDIR_INSTALLATION)
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
	{
		SCopy(ConfigUserPath, UserDataPath);
	}
	else
	{
#if defined(_WIN32)
		SCopy(R"(%APPDATA%\)" C4ENGINENAME, UserDataPath);
#elif defined(__APPLE__)
		SCopy("$HOME/Library/Application Support/" C4ENGINENAME, UserDataPath);
#else
		SCopy("$HOME/.clonk/" C4ENGINENICK, UserDataPath);
#endif
	}
	C4Config::ExpandEnvironmentVariables(UserDataPath, CFG_MaxString);
	AppendBackslash(UserDataPath);

	// Screenshot path
	SCopy(UserDataPath, ScreenshotPath, CFG_MaxString - 1);
	if (ScreenshotFolder.getLength() + std::strlen(ScreenshotPath) + 1 <= CFG_MaxString)
	{
		SAppend(ScreenshotFolder.getData(), ScreenshotPath);
		AppendBackslash(ScreenshotPath);
	}
	// Mods path
	SCopy(UserDataPath, ModsDataPath, CFG_MaxString - 1);
	if (ModsFolder.getLength() + std::strlen(ModsDataPath) + 1 <= CFG_MaxString)
	{
		SAppend(ModsFolder.getData(), ModsDataPath);
		AppendBackslash(ModsDataPath);
	}
	// Create user path if it doesn't already exist
	CreatePath(UserDataPath);
}

static char AtPathFilename[_MAX_PATH_LEN];

const char* C4Config::AtExePath(const char *filename)
{
	SCopy(General.ExePath.getData(), AtPathFilename, _MAX_PATH);
	SAppend(filename, AtPathFilename, _MAX_PATH);
	return AtPathFilename;
}

const char* C4Config::AtUserDataPath(const char *filename)
{
	SCopy(General.UserDataPath, AtPathFilename, _MAX_PATH);
	SAppend(filename, AtPathFilename, _MAX_PATH);
	return AtPathFilename;
}

const char* C4Config::AtSystemDataPath(const char *filename)
{
	SCopy(General.SystemDataPath, AtPathFilename, _MAX_PATH);
	SAppend(filename, AtPathFilename, _MAX_PATH);
	return AtPathFilename;
}

const char* C4Config::AtTempPath(const char *filename)
{
	SCopy(General.TempPath.getData(),AtPathFilename,_MAX_PATH);
	SAppend(filename,AtPathFilename,_MAX_PATH);
	return AtPathFilename;
}

const char* C4Config::AtNetworkPath(const char *filename)
{
	SCopy(General.UserDataPath, AtPathFilename, _MAX_PATH);
	SAppend(Network.WorkPath, AtPathFilename, _MAX_PATH);
	SAppend(filename, AtPathFilename, _MAX_PATH);
	return AtPathFilename;
}

const char *C4Config::AtScreenshotPath(const char *filename)
{
	int len;
	SCopy(General.ScreenshotPath, AtPathFilename, _MAX_PATH);
	if ((len = SLen(AtPathFilename)))
	{
		if (AtPathFilename[len-1] == DirectorySeparator)
		{
			AtPathFilename[len-1] = '\0';
		}
	}
	if (!CreatePath(AtPathFilename))
	{
		SCopy(General.UserDataPath,AtPathFilename,_MAX_PATH);
	}
	AppendBackslash(AtPathFilename);
	SAppend(filename, AtPathFilename, _MAX_PATH);
	return AtPathFilename;
}


bool C4ConfigGeneral::CreateSaveFolder(const char *directory, const char *language_title)
{
	// Create directory if needed
	if (!CreatePath(directory))
	{
		return false;
	}
	// Create title component if needed
	char language[3]; SCopy(Config.General.Language, language, 2);
	StdStrBuf title_file; title_file.Format("%s%c%s", directory, DirectorySeparator, C4CFN_WriteTitle);
	StdStrBuf title_data; title_data.Format("%s:%s", language, language_title);

	CStdFile file;
	if (!FileExists(title_file.getData()))
	{
		if (!file.Create(title_file.getData()) || !file.WriteString(title_data.getData()) || !file.Close())
		{
			return false;
		}
	}
	// Save folder seems okay
	return true;
}


const char* C4ConfigNetwork::GetLeagueServerAddress()
{
	// Alternate (configurable) league server
	if (UseAlternateServer)
	{
		return AlternateServerAddress;
	}
	// Standard (hardcoded) official league server
	else
	{
		return "league.openclonk.org:80/league.php";
	}
}

const char* C4ConfigNetwork::GetModDatabaseServerAddress()
{
	// Alternate (configurable) mod database server
	if (UseAlternateModDatabaseServer)
		return AlternateModDatabaseServerAddress;
	// Standard (hardcoded) official mod database server
	else
		return "mods.openclonk.org/api/";
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

void C4ConfigNetwork::SetLeagueLoginData(const char *server_name, const char *player_name, const char *account, const char *login_token)
{
	// ideally, there would be a list to store multiple logins
	// however, we don't really support multiplayer at one computer at the moment anyway
	LastLeagueServer.Copy(server_name);
	LastLeaguePlayerName.Copy(player_name);
	LastLeagueAccount.Copy(account);
	LastLeagueLoginToken.Copy(login_token);
}

bool C4ConfigNetwork::GetLeagueLoginData(const char *server_name, const char *player_name, StdStrBuf *account, StdStrBuf *login_token) const
{
	// check if last login matches and store if desired
	if (LastLeagueServer == server_name && LastLeaguePlayerName == player_name)
	{
		account->Copy(LastLeagueAccount);
		login_token->Copy(LastLeagueLoginToken);
		return true;
	}
	return false;
}

void C4ConfigControls::ResetKeys()
{
	UserSets.Clear();
}

const char* C4Config::AtUserDataRelativePath(const char *filename)
{
	// Specified file is located in UserDataPath: return relative path
	return GetRelativePathS(filename, General.UserDataPath);
}

const char* C4Config::AtSystemDataRelativePath(const char *filename)
{
	// Specified file is located in SystemDataPath: return relative path
	return GetRelativePathS(filename, General.SystemDataPath);
}

const char* C4Config::AtRelativePath(const char *filename)
{
	const char *path = GetRelativePathS(filename, General.UserDataPath);
	if (path == filename)
	{
		return GetRelativePathS(filename, General.SystemDataPath);
	}
	return path;
}

void C4Config::ForceRelativePath(StdStrBuf *filename)
{
	assert(filename);
	// Specified file is located in SystemDataPath?
	const char *relative = GetRelativePathS(filename->getData(), General.SystemDataPath);
	if (relative != filename->getData())
	{
		// return relative path
		StdStrBuf temp; temp.Copy(relative);
		filename->Take(std::move(temp));
	}
	else
	{
		// not in ExePath: Is it a global path?
		if (IsGlobalPath(filename->getData()))
		{
			// then shorten it (e.g. C:\Temp\Missions.ocf\Goldmine.ocs to Missions.ocf\Goldmine.ocs)
			StdStrBuf temp; temp.Copy(GetC4Filename(filename->getData()));
			filename->Take(std::move(temp));
		}
	}
}

void C4ConfigGeneral::DefaultLanguage()
{
	// No language defined: default to German or English by system language
	if (!Language[0])
	{
		if (IsGermanSystem())
		{
			SCopy("DE - Deutsch", Language);
		}
		else
		{
			SCopy("US - English", Language);
		}
	}
	// No fallback sequence defined: use primary language list
	if (!LanguageEx[0])
	{
		GetLanguageSequence(Language, LanguageEx);
	}
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

const char* C4Config::GetSubkeyPath(const char *subkey)
{
	static char key[1024 + 1];
#ifdef _WIN32
	sprintf(key, R"(Software\%s\%s\%s)", C4CFG_Company, C4ENGINENAME, subkey);
#else
	sprintf(key, "%s", subkey);
#endif
	return key;
}

int C4ConfigGeneral::GetLanguageSequence(const char *source, char *target)
{
	// Copy a condensed list of language codes from the source list to the target string,
	// skipping any whitespace or long language descriptions. Language sequences are
	// comma separated.
	int count = 0;
	char language[2 + 1];
	for (int i = 0; SCopySegment(source, i, language, ',', 2, true); i++)
	{
		if (language[0])
		{
			if (target[0]) SAppendChar(',', target);
			SAppend(language, target);
			count++;
		}
	}
	return count;
}

void C4ConfigStartup::CompileFunc(StdCompiler *compiler)
{
	compiler->Value(mkNamingAdapt(HideMsgGfxEngineChange,      "HideMsgGfxEngineChange",     0));
	compiler->Value(mkNamingAdapt(HideMsgGfxBitDepthChange,    "HideMsgGfxBitDepthChange",   0));
	compiler->Value(mkNamingAdapt(HideMsgMMTimerChange,        "HideMsgMMTimerChange",       0));
	compiler->Value(mkNamingAdapt(HideMsgStartDedicated,       "HideMsgStartDedicated",      0));
	compiler->Value(mkNamingAdapt(HideMsgPlrTakeOver,          "HideMsgPlrTakeOver",         0));
	compiler->Value(mkNamingAdapt(HideMsgPlrNoTakeOver,        "HideMsgPlrNoTakeOver",       0));
	compiler->Value(mkNamingAdapt(HideMsgNoOfficialLeague,     "HideMsgNoOfficialLeague",    0));
	compiler->Value(mkNamingAdapt(HideMsgIRCDangerous,         "HideMsgIRCDangerous",        0));
	compiler->Value(mkNamingAdapt(AlphabeticalSorting,         "AlphabeticalSorting",        0));
	compiler->Value(mkNamingAdapt(LastPortraitFolderIdx,       "LastPortraitFolderIdx",      0));
}

void C4Config::CompileFunc(StdCompiler *compiler)
{
	compiler->Value(mkNamingAdapt(General,     "General"     ));
	compiler->Value(mkNamingAdapt(Controls,    "Controls"    ));
	for (int i=0; i<C4ConfigMaxGamepads; ++i)
	{
		compiler->Value(mkNamingAdapt(Gamepads[i],     FormatString("Gamepad%d", i).getData()));
	}
	compiler->Value(mkNamingAdapt(Graphics,    "Graphics"    ));
	compiler->Value(mkNamingAdapt(Sound,       "Sound"       ));
	compiler->Value(mkNamingAdapt(Network,     "Network"     ));
	compiler->Value(mkNamingAdapt(Lobby,       "Lobby"       ));
	compiler->Value(mkNamingAdapt(IRC,         "IRC"         ));
	compiler->Value(mkNamingAdapt(Developer,   "Developer"   ));
	compiler->Value(mkNamingAdapt(Startup,     "Startup"     ));
	compiler->Value(mkNamingAdapt(Security,    "Security"    ));
}

bool C4Config::AddModule(const char *path, char *modules)
{
	return SAddModule(modules,path);
}

bool C4Config::IsModule(const char *path, char *modules)
{
	return SIsModule(modules,path);
}

bool C4Config::RemoveModule(const char *path, char *modules)
{
	return SRemoveModule(modules,path);
}

void C4Config::ExpandEnvironmentVariables(char *path, size_t max_length)
{
#ifdef _WIN32
	wchar_t buf[_MAX_PATH_LEN];
	ExpandEnvironmentStringsW(GetWideChar(path), buf, _MAX_PATH);
	SCopy(StdStrBuf(buf).getData(), path, max_length);
#else // __linux__ or __APPLE___
	StdStrBuf home(getenv("HOME"));
	char* rest;
	if (home && (rest = const_cast<char *>(SSearch(path, "$HOME"))) && (std::strlen(path) - 5 + home.getLength() <= max_length))
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
	StdStrBuf temp_name;
	temp_name.Copy(AtTempPath("update"));
	MakeTempFilename(&temp_name);
	SCopy(temp_name.getData(), General.TempUpdatePath);
	CreatePath(General.TempUpdatePath);
	return General.TempUpdatePath;
}

const char *C4Config::AtTempUpdatePath(const char *filename)
{
	SCopy(General.TempUpdatePath,AtPathFilename,_MAX_PATH-1);
	AppendBackslash(AtPathFilename);
	SAppend(filename,AtPathFilename,_MAX_PATH);
	return AtPathFilename;
}

C4Config Config;
