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

#ifndef INC_C4Application
#define INC_C4Application

#include "c4group/C4Group.h"
#include "platform/C4MusicSystem.h"
#include "platform/C4SoundSystem.h"
#include "c4group/C4Components.h"
#include "network/C4InteractiveThread.h"
#include "platform/C4App.h"

class C4ApplicationGameTimer;

/* Main class to initialize configuration and execute the game */

class C4Application: public C4AbstractApp
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
	// clear app
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
	virtual void OnKeyboardLayoutChanged();
	bool SetGameFont(const char *szFontFace, int32_t iFontSize);
	void NextTick();

	virtual void Quit();
	void OpenGame(const char * scenario = 0); // start game in the next main loop round
	void QuitGame(); // quit game, and application if in fullscreen without startup
	void Activate(); // activate app to gain full focus in OS
	void SetNextMission(const char *szMissionFilename);
	virtual void OnCommand(const char *szCmd);

	bool IsQuittingGame() const { return AppState >= C4AS_AfterGame; }

	const char *GetRevision() const { return Revision.c_str(); }

	// set by ParseCommandLine
	int isEditor;
	// set by ParseCommandLine, for manually applying downloaded update packs
	std::string IncomingUpdate;
	// set by ParseCommandLine, for manually invoking an update check by command line or url
	int CheckForUpdates;

	bool FullScreenMode();
	int GetConfigWidth()  { return (!FullScreenMode()) ? Config.Graphics.WindowX : Config.Graphics.ResX; }
	int GetConfigHeight() { return (!FullScreenMode()) ? Config.Graphics.WindowY : Config.Graphics.ResY; }
	
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
	std::string IncomingKeyfile;
private:
	// if set, this mission will be launched next
	std::string NextMission;
	// version information strings
	static const std::string Revision;
};

extern C4Application  Application;

class C4ApplicationGameTimer : public CStdMultimediaTimerProc
{
public:
	C4ApplicationGameTimer();
private:
	C4TimeMilliseconds tLastGameTick;
	unsigned int iGameTickDelay;
	unsigned int iExtraGameTickDelay;
public:
	void SetGameTickDelay(uint32_t iDelay);

	virtual bool Execute(int iTimeout, pollfd *);
	virtual bool IsLowPriority();
};

#endif
