/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001  Sven Eberhardt
 * Copyright (c) 2005, 2009  Günther Brammer
 * Copyright (c) 2006  Armin Burgmeier
 * Copyright (c) 2010  Mortimer
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

/* Handles engine execution in developer mode */

#ifndef INC_C4Console
#define INC_C4Console

#include "C4ConsoleGUI.h"
#include "C4PropertyDlg.h"
#include "C4ToolsDlg.h"
#include "C4ObjectListDlg.h"
#include "C4EditCursor.h"

#include <StdWindow.h>

const int C4CNS_ModePlay = 0,
                           C4CNS_ModeEdit = 1,
                                            C4CNS_ModeDraw = 2;

#define IDM_NET_CLIENT1   10000
#define IDM_NET_CLIENT2   10100
#define IDM_PLAYER_QUIT1  10200
#define IDM_PLAYER_QUIT2  10300
#define IDM_VIEWPORT_NEW1 10400
#define IDM_VIEWPORT_NEW2 10500

#ifdef WITH_DEVELOPER_MODE
#include <StdGtkWindow.h>
typedef CStdGtkWindow C4ConsoleBase;
#else
typedef CStdWindow C4ConsoleBase;
#endif

class C4Console: public C4ConsoleGUI
{
public:
	C4Console();
	virtual ~C4Console();
	void Default();
	virtual void Clear();
	virtual void Close();
	using C4ConsoleBase::Init;
	virtual CStdWindow * Init(CStdApp * app);
	void Execute();
	void ClearPointers(C4Object *pObj);
	bool Message(const char *szMessage, bool fQuery=false);
	bool In(const char *szText);
	void DoPlay();
	void DoHalt();	
	void UpdateInputCtrl();
	void UpdateMenus();
	bool OpenGame();
	bool TogglePause(); // key callpack: pause
public:
	bool CloseGame();
	bool UpdatePlayerMenu();
	bool UpdateViewportMenu();
	void UpdateStatusBars();
	// Menu	
	void ClearViewportMenu();
	void UpdateNetMenu();
	void ClearNetMenu();
	void PlayerJoin();
	void EditObjects();
	void EditInfo();
	void EditScript();
	void EditTitle();
	void ViewportNew();
	void HelpAbout();
	bool FileSelect(char *sFilename, int iSize, const char *szFilter, DWORD dwFlags, bool fSave=false);
	bool SaveGame(bool fSaveGame);
	bool FileSaveAs(bool fSaveGame);
	bool FileSave(bool fSaveGame);
	bool FileOpen();
	bool FileOpenWPlrs();
	bool FileCommand();
	bool FileClose();
	bool FileQuit();
	bool FileRecord();
	void SetCaptionToFilename(const char* szFilename);
public:
	C4PropertyDlg   PropertyDlg;
	C4ToolsDlg      ToolsDlg;
	C4ObjectListDlg ObjectListDlg;
	C4EditCursor    EditCursor;

	int ScriptCounter;
	int FrameCounter;
	int Time,FPS;
#if defined(USE_X11) && !defined(WITH_DEVELOPER_MODE)
	virtual void HandleMessage (XEvent &);
#endif
};

#define C4ConsoleWindowClassname "C4Console"

extern C4Console      Console;

#endif
