/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/*
 * OpenClonk, http://www.openclonk.org
 * Copyright (c) 1998-2000, 2003-2004, 2007-2008  Matthes Bender
 * Copyright (c) 2002, 2006-2008  Sven Eberhardt
 * Copyright (c) 2003, 2005-2007  Peter Wortmann
 * Copyright (c) 2005-2009  Günther Brammer
 * Copyright (c) 2006  Alex
 * Copyright (c) 2006-2007  Julian Raschke
 * Copyright (c) 2008  Armin Burgmeier
 * Copyright (c) 2009  Nicolas Hake
 *
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

#include <C4Include.h>
#include <C4Config.h>
#include <C4Version.h>

#include <C4Log.h>
#include <C4Components.h>
#include <C4Network2.h>
#include <C4Language.h>

#include <utility>
#include <StdFile.h>
#include <StdWindow.h>
#include <StdRegistry.h>
#include <StdWindow.h>

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

C4Config *pConfig;

void C4ConfigGeneral::CompileFunc(StdCompiler *pComp)
{
	// For those without the ability to intuitively guess what the falses and trues mean:
	// its mkNamingAdapt(field, name, default, fPrefillDefault, fStoreDefault)
	// where fStoreDefault writes out the value to the config even if it's the same as the default.
#define s mkStringAdaptM
	pComp->Value(mkNamingAdapt(s(Name),             "Name",               ""             ));
	pComp->Value(mkNamingAdapt(s(Language),         "Language",           "", false, true));
	pComp->Value(mkNamingAdapt(s(LanguageEx),       "LanguageEx",         "", false, true));
	pComp->Value(mkNamingAdapt(s(Definitions),      "Definitions",        ""             ));
	pComp->Value(mkNamingAdapt(s(Participants),     "Participants",       ""             ));

	// deliberately not grandfathering UserPath setting, since it was written to config by default
	pComp->Value(mkNamingAdapt(s(ConfigUserPath),   "UserDataPath",       "", false, true));
	// assimilate old data
	pComp->Value(mkNamingAdapt(s(Adopt.PlayerPath), "PlayerPath",       ""));

	pComp->Value(mkNamingAdapt(SaveGameFolder,      "SaveGameFolder",     "Savegames.c4f", false, true));
	pComp->Value(mkNamingAdapt(SaveDemoFolder,      "SaveDemoFolder",     "Records.c4f",   false, true  ));
	pComp->Value(mkNamingAdapt(s(MissionAccess),    "MissionAccess",      "", false, true));
	pComp->Value(mkNamingAdapt(FPS,                 "FPS",                0              ));
	pComp->Value(mkNamingAdapt(Record,              "Record",             0              ));
	pComp->Value(mkNamingAdapt(DefRec,              "DefRec",             0              ));
	pComp->Value(mkNamingAdapt(ScreenshotFolder,    "ScreenshotFolder",   "Screenshots",  false, true));
	pComp->Value(mkNamingAdapt(ScrollSmooth,        "ScrollSmooth",       4              ));
	pComp->Value(mkNamingAdapt(AlwaysDebug,         "DebugMode",          0              ));
#ifdef _WIN32
	pComp->Value(mkNamingAdapt(MMTimer,             "MMTimer",            1              ));
#endif
	pComp->Value(mkNamingAdapt(s(RXFontName),       "FontName",           C4DEFAULT_FONT_NAME,   false, true));
	pComp->Value(mkNamingAdapt(RXFontSize,          "FontSize",           14,            false, true));
	pComp->Value(mkNamingAdapt(GamepadEnabled,      "GamepadEnabled",     true           ));
	pComp->Value(mkNamingAdapt(FirstStart,          "FirstStart",         true           ));
	pComp->Value(mkNamingAdapt(UserPortraitsWritten,"UserPortraitsWritten",false         ));
	pComp->Value(mkNamingAdapt(ConfigResetSafety,   "ConfigResetSafety",  static_cast<int32_t>(ConfigResetSafetyVal) ));
}

void C4ConfigDeveloper::CompileFunc(StdCompiler *pComp)
{
#ifdef _WIN32
	pComp->Value(mkNamingAdapt(AutoEditScan,        "AutoEditScan",       1              ));
#endif
	pComp->Value(mkNamingAdapt(AutoFileReload,      "AutoFileReload",     1              ));
	pComp->Value(mkNamingAdapt(AllErrorsFatal,      "AllErrorsFatal",     0              ));
}

void C4ConfigGraphics::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(ResX,                  "ResolutionX",          800           ,false, true));
	pComp->Value(mkNamingAdapt(ResY,                  "ResolutionY",          600           ,false, true));
	pComp->Value(mkNamingAdapt(GuiResX,                 "GuiResolutionX",       800           ,false, true));
	pComp->Value(mkNamingAdapt(GuiResY,                 "GuiResolutionY",       600           ,false, true));
	pComp->Value(mkNamingAdapt(ShowAllResolutions,    "ShowAllResolutions",   0             ,false, true));
	pComp->Value(mkNamingAdapt(SplitscreenDividers,   "SplitscreenDividers",  1             ));
	pComp->Value(mkNamingAdapt(AddNewCrewPortraits,   "AddNewCrewPortraits",  0             ,false, true));
	pComp->Value(mkNamingAdapt(SaveDefaultPortraits,  "SaveDefaultPortraits", 0             ,false, true));
	pComp->Value(mkNamingAdapt(ShowStartupMessages,   "ShowStartupMessages",  1             ,false, true));
	pComp->Value(mkNamingAdapt(ColorAnimation,        "ColorAnimation",       0             ,false, true));
	pComp->Value(mkNamingAdapt(HighResLandscape,      "HighResLandscape",     0             ,false, true));
	pComp->Value(mkNamingAdapt(SmokeLevel,            "SmokeLevel",           200           ,false, true));
	pComp->Value(mkNamingAdapt(VerboseObjectLoading,  "VerboseObjectLoading", 0             ));
	pComp->Value(mkNamingAdapt(VideoModule,           "VideoModule",          0             ,false, true));
	pComp->Value(mkNamingAdapt(MenuTransparency,      "MenuTransparency",     1             ,false, true));
	pComp->Value(mkNamingAdapt(UpperBoard,            "UpperBoard",           1             ,false, true));
	pComp->Value(mkNamingAdapt(ShowClock,             "ShowClock",            0             ,false, true));
	pComp->Value(mkNamingAdapt(ShowCrewNames,         "ShowCrewNames",        1             ,false, true));
	pComp->Value(mkNamingAdapt(ShowCrewCNames,        "ShowCrewCNames",       1             ,false, true));
	pComp->Value(mkNamingAdapt(BitDepth,              "BitDepth",             32            ,false, true));
	pComp->Value(mkNamingAdapt(Windowed,              "Windowed",             0             ,false, true));
	pComp->Value(mkNamingAdapt(PXSGfx,                "PXSGfx"  ,             1             ));
	pComp->Value(mkNamingAdapt(Engine,                "Engine"  ,             1             ,false, true));
	pComp->Value(mkNamingAdapt(Gamma1,                "Gamma1"  ,             0             ));
	pComp->Value(mkNamingAdapt(Gamma2,                "Gamma2"  ,             0x808080      ));
	pComp->Value(mkNamingAdapt(Gamma3,                "Gamma3"  ,             0xffffff      ));
	pComp->Value(mkNamingAdapt(Currency,              "Currency"  ,           0             ));
	pComp->Value(mkNamingAdapt(RenderInactiveEM,      "RenderInactiveEM",     1             ));
	pComp->Value(mkNamingAdapt(DisableGamma,          "DisableGamma",         0             ,false, true));
	pComp->Value(mkNamingAdapt(Monitor,               "Monitor",              0             )); // 0 = D3DADAPTER_DEFAULT
	pComp->Value(mkNamingAdapt(FireParticles,         "FireParticles",        1         ));
	pComp->Value(mkNamingAdapt(MaxRefreshDelay,       "MaxRefreshDelay",      30            ));
	pComp->Value(mkNamingAdapt(EnableShaders,         "Shader",               0             ,false, true));
	pComp->Value(mkNamingAdapt(NoOffscreenBlits,      "NoOffscreenBlits",     1             ));
	pComp->Value(mkNamingAdapt(ClipManuallyE,         "ClipManuallyE",        1             ));
}

void C4ConfigSound::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(RXSound,               "Sound",                1             ,false, true));
	pComp->Value(mkNamingAdapt(RXMusic,               "Music",                1             ,false, true));
	pComp->Value(mkNamingAdapt(FEMusic,               "MenuMusic",            1             ,false, true));
	pComp->Value(mkNamingAdapt(FESamples,             "MenuSound",            1             ,false, true));
	pComp->Value(mkNamingAdapt(FMMode,                "FMMode",               1             ));
	pComp->Value(mkNamingAdapt(Verbose,               "Verbose",              0             ));
	pComp->Value(mkNamingAdapt(MusicVolume,           "MusicVolume",          100           ,false, true));
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
	pComp->Value(mkNamingAdapt(PortDiscovery,           "PortDiscovery",        C4NetStdPortDiscovery ,false, true));
	pComp->Value(mkNamingAdapt(PortRefServer,           "PortRefServer",        C4NetStdPortRefServer ,false, true));
	pComp->Value(mkNamingAdapt(ControlMode,             "ControlMode",          0             ));
	pComp->Value(mkNamingAdapt(SendPortraits,           "SendPortraits",        0             ,false, true));
	pComp->Value(mkNamingAdapt(Nick,                    "Nick",                 ""            ,false, true));
	pComp->Value(mkNamingAdapt(MaxLoadFileSize,         "MaxLoadFileSize",      5*1024*1024   ,false, true));

	pComp->Value(mkNamingAdapt(MasterServerSignUp,      "MasterServerSignUp",   1             ));
	pComp->Value(mkNamingAdapt(MasterServerActive,      "MasterServerActive",   0             ));
	pComp->Value(mkNamingAdapt(MasterKeepPeriod,        "MasterKeepPeriod",     60            ));
	pComp->Value(mkNamingAdapt(MasterReferencePeriod,   "MasterReferencePeriod",120           ));
	pComp->Value(mkNamingAdapt(LeagueServerSignUp,      "LeagueServerSignUp",   0             ));
	pComp->Value(mkNamingAdapt(UseAlternateServer,      "UseAlternateServer",   0             ));
	pComp->Value(mkNamingAdapt(s(AlternateServerAddress),"AlternateServerAddress", "boom.openclonk.org:80/server/"));
	pComp->Value(mkNamingAdapt(s(LastPassword),         "LastPassword",         "Wipf"        ));
#ifdef WITH_AUTOMATIC_UPDATE
	pComp->Value(mkNamingAdapt(s(UpdateServerAddress),  "UpdateServerAddress",     "boom.openclonk.org:80/server/"));
	pComp->Value(mkNamingAdapt(AutomaticUpdate,         "AutomaticUpdate",      0             ,false, true));
	pComp->Value(mkNamingAdapt(LastUpdateTime,          "LastUpdateTime",       0             ));
#endif
	pComp->Value(mkNamingAdapt(AsyncMaxWait,            "AsyncMaxWait",         2             ));

	pComp->Value(mkNamingAdapt(s(PuncherAddress),       "PuncherAddress",       "clonk.de:11115")); // maybe store default for this one?

}

void C4ConfigLobby::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(AllowPlayerSave,         "AllowPlayerSave",      0             ,false, false));
	pComp->Value(mkNamingAdapt(CountdownTime,           "CountdownTime",        5             ,false, false));
}

void C4ConfigIRC::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(s(Server),               "Server",               "irc.ham.de.euirc.net", false, true));
	pComp->Value(mkNamingAdapt(s(Nick),                 "Nick",                 ""                    , false, true));
	pComp->Value(mkNamingAdapt(s(RealName),             "RealName",             ""                    , false, true));
	pComp->Value(mkNamingAdapt(s(Channel),              "Channel",              "#openclonk"    , false, true));
	pComp->Value(mkNamingAdapt(AllowAllChannels,        "AllowAllChannels",     0                     , false, true));
}

void C4ConfigSecurity::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(WasRegistered,           "WasRegistered",        0                   ));
#ifdef _WIN32
	pComp->Value(mkNamingAdapt(s(KeyPath),              "KeyPath",              "%APPDATA%\\" C4ENGINENAME, false, true));
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

void C4ConfigControls::CompileFunc(StdCompiler *pComp, bool fKeysOnly)
{
#ifndef USE_CONSOLE
	if (fKeysOnly) return;

	pComp->Value(mkNamingAdapt(MouseAScroll,      "MouseAutoScroll",      0));
	pComp->Value(mkNamingAdapt(GamepadGuiControl, "GamepadGuiControl",    0,     false, true));
#endif
}

const char *CfgAtTempPath(const char *szFilename)
{
	// safety
	if (!pConfig) return NULL;
	// get at temp path
	return pConfig->AtTempPath(szFilename);
}


C4Config::C4Config()
{
	pConfig=this;
	Default();
}

C4Config::~C4Config()
{
	fConfigLoaded = false;
	pConfig=NULL;
}

void C4Config::Default()
{
	// force default values
	StdCompilerNull Comp; Comp.Compile(*this);
	fConfigLoaded=false;
}

void C4Config::GetConfigFileName(StdStrBuf &filename, bool forceWorkingDirectory, const char *szConfigFile)
{
	if (szConfigFile)
	{
		// Config filename is specified
		filename.Ref(szConfigFile);
		// make sure we're at the correct path to load it
		if (forceWorkingDirectory) General.DeterminePaths(true);
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

bool C4Config::Load(bool forceWorkingDirectory, const char *szConfigFile)
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
			GetConfigFileName(filename, forceWorkingDirectory, szConfigFile);

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
	General.DeterminePaths(forceWorkingDirectory);
	General.AdoptOldSettings();
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
#if defined USE_GL && !defined USE_DIRECTX
	if (Graphics.Engine == GFXENGN_DIRECTX || Graphics.Engine == GFXENGN_DIRECTXS)
		Graphics.Engine = GFXENGN_OPENGL;
#endif
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
	/*if (!Network.Nick.getLength())
	  Network.Nick.Copy(Network.LocalName); // assuming that LocalName will always contain some useful value*/
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
			GetConfigFileName(filename, false, ConfigFilename.getLength() ? ConfigFilename.getData() : NULL);
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

void C4ConfigGeneral::DeterminePaths(bool forceWorkingDirectory)
{
#ifdef _WIN32
	// Exe path
	if (GetModuleFileName(NULL,ExePath,CFG_MaxString))
		{ TruncatePath(ExePath); AppendBackslash(ExePath); }
	// Temp path
	GetTempPath(CFG_MaxString,TempPath);
	if (TempPath[0]) AppendBackslash(TempPath);
#elif defined(__linux__)
	GetParentPath(Application.Location, ExePath);
	AppendBackslash(ExePath);
	const char * t = getenv("TMPDIR");
	if (t)
	{
		SCopy(t, TempPath, sizeof(TempPath)-2);
		AppendBackslash(TempPath);
	}
	else
		SCopy("/tmp/", TempPath);
#else
	// Mac: Just use the working directory as ExePath.
	SCopy(GetWorkingDirectory(), ExePath);
	AppendBackslash(ExePath);
	SCopy("/tmp/", TempPath);
#endif
	// Force working directory to exe path if desired

#ifndef _DEBUG
	if (forceWorkingDirectory)
		SetWorkingDirectory(ExePath);
#endif

	// Find system-wide data path
#if defined(_WIN32) || defined(__APPLE__)
	// Use workdir; in release builds, this is the exe dir
	SCopy(GetWorkingDirectory(),SystemDataPath);
	AppendBackslash(SystemDataPath);
#elif defined(__linux__)
	// FIXME: Where to put this?
	SCopy(ExePath,SystemDataPath);
#endif

	// Find user-specific data path
	if (ConfigUserPath[0])
		SCopy(ConfigUserPath, UserDataPath);
	else
#if defined(_WIN32)
		SCopy("%APPDATA%\\" C4ENGINENAME, UserDataPath);
#elif defined(__linux__)
		SCopy("$HOME/.clonk/" C4ENGINENICK, UserDataPath);
#elif defined(__APPLE__)
		SCopy("$HOME/Library/Application Support/" C4ENGINENAME, UserDataPath);
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

static bool GrabOldPlayerFile(const char *fn)
{
	if (FileExists(Config.AtUserDataPath(GetFilename(fn))))
	{
		// player already exists in user dir, skip
		return true;
	}
	if (!MoveItem(fn, Config.AtUserDataPath(GetFilename(fn))))
	{
		// possibly unpacked
		if (DirectoryExists(fn))
		{
			if (!C4Group_PackDirectoryTo(fn, Config.AtUserDataPath(GetFilename(fn))))
				return true; // ignore errors
		}
	}
	return true;
}
void C4ConfigGeneral::AdoptOldSettings()
{
	// Copy over old player data
	if (*Adopt.PlayerPath)
	{
		ForEachFile(FormatString("%s/%s", Config.AtExePath(Adopt.PlayerPath), C4CFN_PlayerFiles).getData(), &GrabOldPlayerFile);
		*Adopt.PlayerPath = '\0';
	}
	else if (!ItemIdentical(Config.General.ExePath, Config.General.UserDataPath))
	{
		ForEachFile(Config.AtExePath(C4CFN_PlayerFiles), &GrabOldPlayerFile);
	}
}

void C4ConfigGeneral::ClearAdditionalDataPaths()
{
	for (PathList::iterator it = AdditionalDataPaths.begin(); it != AdditionalDataPaths.end(); ++it)
		delete[] *it;
	AdditionalDataPaths.clear();
}

void C4ConfigGeneral::AddAdditionalDataPath(const char *szPath)
{
	char *clone = new char[SLen(szPath)+1];
	SCopy(szPath,clone);
	AdditionalDataPaths.push_back(clone);
}

char AtPathFilename[_MAX_PATH+1];

const char* C4Config::AtExePath(const char *szFilename)
{
	SCopy(General.ExePath,AtPathFilename,_MAX_PATH);
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
	SCopy(General.TempPath,AtPathFilename,_MAX_PATH);
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
		SCopy(General.ExePath, General.ScreenshotPath, CFG_MaxString-1);
		SCopy(General.ScreenshotPath,AtPathFilename,_MAX_PATH);
	}
	else
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
		return "boom.openclonk.org:80/server/";
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
void C4ConfigControls::ResetKeys()
{
	StdCompilerNull Comp; Comp.Compile(mkParAdapt(*this, true));
}

const char* C4Config::AtDataReadPath(const char *szFilename, bool fPreferWorkdir)
{
	if (IsGlobalPath(szFilename)) return szFilename;
	StdCopyStrBuf sfn(szFilename);
	do
	{
		const char *path = AtDataReadPathCore(sfn.getData(), fPreferWorkdir);
		if (path)
		{
			if (path != AtPathFilename) SCopy(path,AtPathFilename);
			AtPathFilename[_MAX_PATH] = '\0';
			AppendBackslash(AtPathFilename);
			SAppend(szFilename,AtPathFilename,_MAX_PATH);
			return AtPathFilename;
		}
	}
	while (TruncatePath(sfn.getMData()));
	return szFilename;
}

const char* C4Config::AtDataReadPathCore(const char *szFilename, bool fPreferWorkdir)
{
	if (fPreferWorkdir && FileExists(szFilename))
	{
		SCopy(GetWorkingDirectory(),AtPathFilename,_MAX_PATH-1);
		return AtPathFilename;
	}
	// Check extra data paths
	for (C4ConfigGeneral::PathList::iterator it = General.AdditionalDataPaths.begin();
	     it != General.AdditionalDataPaths.end();
	     ++it)
	{
		SCopy(*it, AtPathFilename, _MAX_PATH-1);
		AppendBackslash(AtPathFilename);
		SAppend(szFilename,AtPathFilename,_MAX_PATH);
		if (FileExists(AtPathFilename))
			return *it;
	}
	if (FileExists(AtUserDataPath(szFilename))) return General.UserDataPath;
	if (FileExists(AtSystemDataPath(szFilename))) return General.SystemDataPath;
	if (!fPreferWorkdir && FileExists(szFilename))
	{
		SCopy(GetWorkingDirectory(),AtPathFilename,_MAX_PATH-1);
		return AtPathFilename;
	}
	return NULL;
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
			// then shorten it (e.g. C:\Temp\Missions.c4f\Goldmine.c4s to Missions.c4f\Goldmine.c4s)
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
	sprintf(key, "Software\\%s\\%s\\%s", C4CFG_Company, C4ENGINENAME, strSubkey);
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
	pComp->Value(mkNamingAdapt(NoSplash,                    "NoSplash",                   1));
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
	char buf[_MAX_PATH + 1];
	ExpandEnvironmentStrings(strPath, buf, _MAX_PATH);
	SCopy(buf, strPath, iMaxLen);
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
