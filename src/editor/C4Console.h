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

/* Handles engine execution in developer mode */

#ifndef INC_C4Console
#define INC_C4Console

#include "editor/C4ConsoleGUI.h"
#include "editor/C4ToolsDlg.h"
#include "editor/C4ObjectListDlg.h"
#include "editor/C4EditCursor.h"

#include "platform/C4Window.h"

const int C4CNS_ModePlay = 0,
          C4CNS_ModeEdit = 1,
          C4CNS_ModeCreateObject = 2,
          C4CNS_ModeDraw = 3;

#define IDM_NET_CLIENT1   10000
#define IDM_NET_CLIENT2   10100
#define IDM_PLAYER_QUIT1  10200
#define IDM_PLAYER_QUIT2  10300
#define IDM_VIEWPORT_NEW1 10400
#define IDM_VIEWPORT_NEW2 10500

class C4Console: public C4ConsoleGUI
{
public:
	C4Console();
	virtual ~C4Console();
	void Default();
	virtual void Clear();
	virtual void Close();
	using C4Window::Init;
	virtual C4Window * Init(C4AbstractApp * app);
	void Execute();
	void ClearPointers(C4Object *pObj);
	bool Message(const char *szMessage, bool fQuery=false);
	bool In(const char *szText);
	void DoPlay();
	void DoHalt();	
	void UpdateInputCtrl();
	void UpdateMenus();
	void InitGame();
	bool TogglePause(); // key callpack: pause
public:
	void CloseGame();
	bool UpdatePlayerMenu();
	bool UpdateViewportMenu();
	void UpdateStatusBars();
	// Menu	
	void ClearViewportMenu();
	void UpdateNetMenu();
	void ClearNetMenu();
	void PlayerJoin();
	void ViewportNew();
	void HelpAbout();
	bool FileSelect(StdStrBuf *sFilename, const char *szFilter, DWORD dwFlags, bool fSave=false);
	bool SaveGame(const char * path);
	bool SaveScenario(const char * path, bool export_packed=false);
	bool FileSaveAs(bool fSaveGame, bool export_packed=false);
	bool FileSave();
	bool FileNew();
	bool FileOpen(const char *filename=nullptr, bool host_in_network=false);
	bool FileOpenWPlrs();
	bool FileCommand();
	bool FileClose();
	bool FileQuit();
	bool FileRecord();
	void SetCaptionToFilename(const char* szFilename);
public:
	C4ToolsDlg      ToolsDlg;
	C4ObjectListDlg ObjectListDlg;
	C4EditCursor    EditCursor;

	int FrameCounter;
	int Time,FPS;

	// Script MRU: Keep track of recent script executions in global and local windows
	enum RecentScriptInputLists
	{
		MRU_Scenario = 0,
		MRU_Object = 1
	};
private:
	std::list<StdCopyStrBuf> recent_script_input[2];
public:
	std::list<const char *> GetScriptSuggestions(class C4PropList *target, RecentScriptInputLists section) const;
	void RegisterRecentInput(const char *input, RecentScriptInputLists section);
};

extern C4Console      Console;

#endif
