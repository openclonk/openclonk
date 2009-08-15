/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001  Sven Eberhardt
 * Copyright (c) 2005  Günther Brammer
 * Copyright (c) 2006  Armin Burgmeier
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

#include "C4PropertyDlg.h"
#include "C4ToolsDlg.h"
#include "C4ObjectListDlg.h"
#include "C4EditCursor.h"

#include <StdWindow.h>

#ifdef WITH_DEVELOPER_MODE
# include <gtk/gtkwidget.h>
#endif

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

class C4Console: public C4ConsoleBase
	{
	public:
		C4Console();
		virtual ~C4Console();
		bool Editing;
		C4PropertyDlg	PropertyDlg;
		C4ToolsDlg		ToolsDlg;
		C4ObjectListDlg	ObjectListDlg;
		C4EditCursor	EditCursor;
		void Default();
		virtual void Clear();
		virtual void Close();
		virtual CStdWindow * Init(CStdApp * app);
		void Execute();
		void ClearPointers(C4Object *pObj);
		bool Message(const char *szMessage, bool fQuery=false);
		void SetCaption(const char *szCaption);
		bool In(const char *szText);
		bool Out(const char *szText);
		bool ClearLog(); // empty log text
		void DoPlay();
		void DoHalt();
		bool UpdateCursorBar(const char *szCursor);
		bool UpdateHaltCtrls(bool fHalt);
		bool UpdateModeCtrls(int iMode);
		void UpdateInputCtrl();
		void UpdateMenus();
		bool OpenGame(const char * szCmdLine = 0);
		bool TogglePause(); // key callpack: pause
	protected:
		bool CloseGame();
		bool fGameOpen;
		void EnableControls(bool fEnable);
		bool UpdatePlayerMenu();
		bool UpdateViewportMenu();
		bool UpdateStatusBars();
		// Menu
		void ClearPlayerMenu();
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

		int ScriptCounter;
		int FrameCounter;
		int Time,FPS;
		int MenuIndexFile;
		int MenuIndexComponents;
		int MenuIndexPlayer;
		int MenuIndexViewport;
		int MenuIndexNet;
		int MenuIndexHelp;
#ifdef _WIN32
		void UpdateMenuText(HMENU hMenu);
		BOOL RegisterConsoleWindowClass(HINSTANCE hInst);
		bool AddMenuItem(HMENU hMenu, DWORD dwID, const char *szString, bool fEnabled=true);
		virtual bool Win32DialogMessageHandling(MSG * msg)
			{
			return (hWindow && IsDialogMessage(hWindow,msg)) || (PropertyDlg.hDialog && IsDialogMessage(PropertyDlg.hDialog,msg));
			};
		HBITMAP hbmMouse;
		HBITMAP hbmMouse2;
		HBITMAP hbmCursor;
		HBITMAP hbmCursor2;
		HBITMAP hbmBrush;
		HBITMAP hbmBrush2;
		HBITMAP hbmPlay;
		HBITMAP hbmPlay2;
		HBITMAP hbmHalt;
		HBITMAP hbmHalt2;
	friend BOOL CALLBACK ConsoleDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
#elif defined(WITH_DEVELOPER_MODE)
		virtual GtkWidget* InitGUI();

		GdkCursor* cursorDefault;
		GdkCursor* cursorWait;

		GtkWidget* txtLog;
		GtkWidget* txtScript;
		GtkWidget* btnPlay;
		GtkWidget* btnHalt;
		GtkWidget* btnModePlay;
		GtkWidget* btnModeEdit;
		GtkWidget* btnModeDraw;

		GtkWidget* menuBar;
		GtkWidget* itemNet;

		GtkWidget* menuViewport;
		GtkWidget* menuPlayer;

		GtkWidget* fileOpen;
		GtkWidget* fileOpenWithPlayers;
		GtkWidget* fileSave;
		GtkWidget* fileSaveAs;
		GtkWidget* fileSaveGame;
		GtkWidget* fileSaveGameAs;
		GtkWidget* fileRecord;
		GtkWidget* fileClose;
		GtkWidget* fileQuit;

		GtkWidget* compScript;
		GtkWidget* compTitle;
		GtkWidget* compInfo;
		GtkWidget* compObjects;

		GtkWidget* plrJoin;

		GtkWidget* viewNew;

		GtkWidget* helpAbout;

		GtkWidget* lblCursor;
		GtkWidget* lblFrame;
		GtkWidget* lblScript;
		GtkWidget* lblTime;

		gulong handlerPlay;
		gulong handlerHalt;
		gulong handlerModePlay;
		gulong handlerModeEdit;
		gulong handlerModeDraw;

		static void OnScriptEntry(GtkWidget* entry, gpointer data);
		static void OnPlay(GtkWidget* button, gpointer data);
		static void OnHalt(GtkWidget* button, gpointer data);
		static void OnModePlay(GtkWidget* button, gpointer data);
		static void OnModeEdit(GtkWidget* button, gpointer data);
		static void OnModeDraw(GtkWidget* button, gpointer data);

		static void OnFileOpen(GtkWidget* item, gpointer data);
		static void OnFileOpenWPlrs(GtkWidget* item, gpointer data);
		static void OnFileSave(GtkWidget* item, gpointer data);
		static void OnFileSaveAs(GtkWidget* item, gpointer data);
		static void OnFileSaveGame(GtkWidget* item, gpointer data);
		static void OnFileSaveGameAs(GtkWidget* item, gpointer data);
		static void OnFileRecord(GtkWidget* item, gpointer data);
		static void OnFileClose(GtkWidget* item, gpointer data);
		static void OnFileQuit(GtkWidget* item, gpointer data);

		static void OnCompObjects(GtkWidget* item, gpointer data);
		static void OnCompScript(GtkWidget* item, gpointer data);
		static void OnCompTitle(GtkWidget* item, gpointer data);
		static void OnCompInfo(GtkWidget* item, gpointer data);

		static void OnPlrJoin(GtkWidget* item, gpointer data);
		static void OnPlrQuit(GtkWidget* item, gpointer data);
		static void OnViewNew(GtkWidget* item, gpointer data);
		static void OnViewNewPlr(GtkWidget* item, gpointer data);
		static void OnHelpAbout(GtkWidget* item, gpointer data);

		static void OnNetClient(GtkWidget* item, gpointer data);
#elif defined(USE_X11) && !defined(WITH_DEVELOPER_MODE)
		virtual void HandleMessage (XEvent &);
#endif
	};

#define C4ConsoleWindowClassname "C4Console"

extern C4Console			Console;

#endif
