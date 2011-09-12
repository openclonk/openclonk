/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2007  Matthes Bender
 * Copyright (c) 2001, 2004-2005, 2008  Sven Eberhardt
 * Copyright (c) 2005-2006, 2010  Günther Brammer
 * Copyright (c) 2007, 2009  Peter Wortmann
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

#ifndef INC_C4Application
#define INC_C4Application

#include <C4ConfigShareware.h>
#include <C4Group.h>
#include <C4MusicSystem.h>
#include <C4SoundSystem.h>
#include <C4Components.h>
#include <C4InteractiveThread.h>
#include <StdApp.h>

class CStdDDraw;
class C4ApplicationGameTimer;
class C4GamePadControl;

/* Main class to initialize configuration and execute the game */

class C4Application: public CStdApp
{
public:
	C4Application();
	~C4Application();
	// Flag for restarting the engine at the end
	bool restartAtEnd;
	// main System.ocg in working folder
	C4Group SystemGroup;
	C4MusicSystem MusicSystem;
	C4SoundSystem SoundSystem;
	C4GamePadControl * pGamePadControl;
	// Thread for interactive processes (automatically starts as needed)
	C4InteractiveThread InteractiveThread;
	// IRC client for global chat
	C4Network2IRCClient &IRCClient;
	void Clear();
	void ClearCommandLine();
	// Tick timing
	void GameTick();
	void Draw();
	// System.ocg helper funcs
	bool OpenSystemGroup() { return SystemGroup.IsOpen() || SystemGroup.Open(C4CFN_System); }
	void CloseSystemGroup() { SystemGroup.Close(); }
	void SetGameTickDelay(int iDelay);
	virtual void OnResolutionChanged(unsigned int iXRes, unsigned int iYRes);
	bool SetGameFont(const char *szFontFace, int32_t iFontSize);
	void NextTick();

	virtual void Quit();
	void OpenGame(const char * scenario = 0); // start game in the next main loop round
	void QuitGame(); // quit game, and application if in fullscreen without startup
	void Activate(); // activate app to gain full focus in OS
	void SetNextMission(const char *szMissionFilename);
	virtual void OnCommand(const char *szCmd);

	const char *GetRevision() const { return Revision.getData(); }

	// set by ParseCommandLine
	int isEditor;
	// set by ParseCommandLine, only pertains to this program start - independent of Config.Startup.NoSplash
	int NoSplash;
	// set by ParseCommandLine, for manually applying downloaded update packs
	StdStrBuf IncomingUpdate;
	// set by ParseCommandLine, for manually invoking an update check by command line or url
	int CheckForUpdates;	
protected:
	enum State { C4AS_None, C4AS_PreInit, C4AS_Startup, C4AS_StartGame, C4AS_Game, C4AS_AfterGame, C4AS_Quit } AppState;
	C4ApplicationGameTimer *pGameTimer;

	virtual bool DoInit(int argc, char * argv[]);
	void ParseCommandLine(int argc, char * argv[]);
	bool PreInit();
	static bool ProcessCallback(const char *szMessage, int iProcess);
	void ApplyResolutionConstraints();

	// set by ParseCommandLine, if neither editor, scenario nor direct join adress has been specified
	int QuitAfterGame;
	// set by ParseCommandLine, for installing registration keys
	StdStrBuf IncomingKeyfile;
private:
	// if set, this mission will be launched next
	StdCopyStrBuf NextMission;
	// version information strings
	StdCopyStrBuf Revision;
};

extern C4Application  Application;

class C4ApplicationGameTimer : public CStdMultimediaTimerProc
{
public:
	C4ApplicationGameTimer();
private:
	unsigned int iLastGameTick, iGameTickDelay;
	bool fRecursing;
public:
	void SetGameTickDelay(uint32_t iDelay);

	virtual bool Execute(int iTimeout, pollfd *);
	virtual bool IsLowPriority();
};

class C4ApplicationSec1Timer : protected CStdTimerProc
{
public:
	C4ApplicationSec1Timer() : CStdTimerProc(1000) { }
	virtual void OnSec1Timer() = 0;
protected:
	virtual bool Execute(int, pollfd *)
	{
		if (CheckAndReset())
			OnSec1Timer();
		return true;
	}
};

#endif
