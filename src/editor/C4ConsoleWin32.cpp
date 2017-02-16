/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2003, Matthes Bender
 * Copyright (c) 2004, Peter Wortmann
 * Copyright (c) 2005-2007, Günther Brammer
 * Copyright (c) 2005, 2007, Sven Eberhardt
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

#include "C4Include.h"
#include "editor/C4Console.h"

#include "platform/C4AppWin32Impl.h"
#include "editor/C4ConsoleGUI.h"
#include "graphics/C4DrawGL.h"
#include "landscape/C4Landscape.h"
#include "object/C4Object.h"
#include "player/C4PlayerList.h"
#include "landscape/C4Texture.h"
#include "C4Version.h"
#include "game/C4Viewport.h"
#include "platform/StdRegistry.h"
#include "lib/StdColors.h"
#include "landscape/C4Sky.h"

#include "platform/C4windowswrapper.h"
#include <mmsystem.h>
#include <commdlg.h>
#include "res/resource.h"
#define GetWideLPARAM(c) reinterpret_cast<LPARAM>(static_cast<wchar_t*>(GetWideChar(c)))

inline StdStrBuf::wchar_t_holder LoadResStrW(const char *id) { return GetWideChar(LoadResStr(id)); }

bool SetMenuItemText(HMENU hMenu, WORD id, const char *szText);

bool AddMenuItem(C4ConsoleGUI *console, HMENU hMenu, DWORD dwID, const char *szString, bool fEnabled)
{
	if (!console->Active) return false;
	MENUITEMINFOW minfo;
	ZeroMem(&minfo,sizeof(minfo));
	minfo.cbSize = sizeof(minfo);
	minfo.fMask = MIIM_ID | MIIM_TYPE | MIIM_DATA | MIIM_STATE;
	minfo.fType = MFT_STRING;
	minfo.wID = dwID;
	StdBuf td = GetWideCharBuf(szString);
	minfo.dwTypeData = getMBufPtr<wchar_t>(td);
	minfo.cch = wcslen(minfo.dwTypeData);
	if (!fEnabled) minfo.fState|=MFS_GRAYED;
	return !!InsertMenuItemW(hMenu,0,false,&minfo);
}

class C4ConsoleGUI::State
{
public:
	BOOL RegisterConsoleWindowClass(HINSTANCE hInst);
	bool AddMenuItem(HMENU hMenu, DWORD dwID, const char *szString, bool fEnabled=true);
	HWND hPropertyDlg;
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
	int MenuIndexFile;
	int MenuIndexPlayer;
	int MenuIndexViewport;
	int MenuIndexNet;
	int MenuIndexHelp;
	int property_dlg_inputarea_height;
	int property_dlg_margin;
	int property_dlg_okbutton_width;
	HWND console_handle;
	int console_default_width, console_default_height; // default (and minimum) console window size
	int console_margin; // margins between controls and from window borders
	int console_wide_margin; // larger margins around some console buttons
	int console_button_height; // height of buttons and the three control rows in the console
	int console_ok_button_width; // width of OK button to enter script commands (everyone just presses enter anyway...)
	int console_status_width; // width of frame counter and time/FPS display status boxes

	State(C4ConsoleGUI *console)
	{
		hbmMouse=nullptr;
		hbmMouse2=nullptr;
		hbmCursor=nullptr;
		hbmCursor2=nullptr;
		hbmBrush=nullptr;
		hbmBrush2=nullptr;
		hbmPlay=nullptr;
		hbmPlay2=nullptr;
		hbmHalt=nullptr;
		hbmHalt2=nullptr;
		hPropertyDlg=nullptr;
		MenuIndexFile       =  0;
		MenuIndexPlayer     =  1;
		MenuIndexViewport   =  2;
		MenuIndexNet        = -1;
		MenuIndexHelp       =  3;
		property_dlg_inputarea_height = 0;
		property_dlg_margin = 0;
		property_dlg_okbutton_width = 0;
		console_handle = nullptr;
		console_default_width = 0;
		console_default_height = 0;
		console_margin = 0;
		console_wide_margin = 0;
		console_button_height = 0;
		console_ok_button_width = 0;
		console_status_width = 0;
	}

	~State()
	{
		if (hbmMouse) DeleteObject(hbmMouse);
		if (hbmMouse2) DeleteObject(hbmMouse2);
		if (hbmCursor) DeleteObject(hbmCursor);
		if (hbmCursor2) DeleteObject(hbmCursor2);
		if (hbmBrush) DeleteObject(hbmBrush);
		if (hbmBrush2) DeleteObject(hbmBrush2);
		if (hbmPlay) DeleteObject(hbmPlay);
		if (hbmPlay2) DeleteObject(hbmPlay2);
		if (hbmHalt) DeleteObject(hbmHalt);
		if (hbmHalt2) DeleteObject(hbmHalt2);
	}

	void CreateBitmaps(C4AbstractApp *application)
	{
		HINSTANCE instance = application->GetInstance();
		hbmMouse=(HBITMAP)LoadBitmapW(instance,MAKEINTRESOURCEW(IDB_MOUSE));
		hbmMouse2=(HBITMAP)LoadBitmapW(instance,MAKEINTRESOURCEW(IDB_MOUSE2));
		hbmCursor=(HBITMAP)LoadBitmapW(instance,MAKEINTRESOURCEW(IDB_CURSOR));
		hbmCursor2=(HBITMAP)LoadBitmapW(instance,MAKEINTRESOURCEW(IDB_CURSOR2));
		hbmBrush=(HBITMAP)LoadBitmapW(instance,MAKEINTRESOURCEW(IDB_BRUSH));
		hbmBrush2=(HBITMAP)LoadBitmapW(instance,MAKEINTRESOURCEW(IDB_BRUSH2));
		hbmPlay=(HBITMAP)LoadBitmapW(instance,MAKEINTRESOURCEW(IDB_PLAY));
		hbmPlay2=(HBITMAP)LoadBitmapW(instance,MAKEINTRESOURCEW(IDB_PLAY2));
		hbmHalt=(HBITMAP)LoadBitmapW(instance,MAKEINTRESOURCEW(IDB_HALT));
		hbmHalt2=(HBITMAP)LoadBitmapW(instance,MAKEINTRESOURCEW(IDB_HALT2));
	}

	void UpdateMenuText(C4ConsoleGUI &console, HMENU hMenu)
	{
		HMENU hSubMenu;
		if (!console.Active) return;
		// File
		ModifyMenuW(hMenu,MenuIndexFile,MF_BYPOSITION | MF_STRING,0,LoadResStrW("IDS_MNU_FILE"));
		hSubMenu = GetSubMenu(hMenu,MenuIndexFile);
		SetMenuItemText(hSubMenu,IDM_FILE_OPEN,LoadResStr("IDS_MNU_OPEN"));
		SetMenuItemText(hSubMenu,IDM_FILE_OPENWPLRS,LoadResStr("IDS_MNU_OPENWPLRS"));
		SetMenuItemText(hSubMenu,IDM_FILE_RECORD,LoadResStr("IDS_MNU_RECORD"));
		SetMenuItemText(hSubMenu,IDM_FILE_SAVE,LoadResStr("IDS_MNU_SAVESCENARIO"));
		SetMenuItemText(hSubMenu,IDM_FILE_SAVEAS,LoadResStr("IDS_MNU_SAVESCENARIOAS"));
		SetMenuItemText(hSubMenu,IDM_FILE_SAVEGAMEAS,LoadResStr("IDS_MNU_SAVEGAMEAS"));
		SetMenuItemText(hSubMenu,IDM_FILE_CLOSE,LoadResStr("IDS_MNU_CLOSE"));
		SetMenuItemText(hSubMenu,IDM_FILE_QUIT,LoadResStr("IDS_MNU_QUIT"));
		// Player
		ModifyMenuW(hMenu,MenuIndexPlayer,MF_BYPOSITION | MF_STRING,0,LoadResStrW("IDS_MNU_PLAYER"));
		hSubMenu = GetSubMenu(hMenu,MenuIndexPlayer);
		SetMenuItemText(hSubMenu,IDM_PLAYER_JOIN,LoadResStr("IDS_MNU_JOIN"));
		// Viewport
		ModifyMenuW(hMenu,MenuIndexViewport,MF_BYPOSITION | MF_STRING,0,LoadResStrW("IDS_MNU_VIEWPORT"));
		hSubMenu = GetSubMenu(hMenu,MenuIndexViewport);
		SetMenuItemText(hSubMenu,IDM_VIEWPORT_NEW,LoadResStr("IDS_MNU_NEW"));
		// Help
		hSubMenu = GetSubMenu(hMenu,MenuIndexHelp);
		SetMenuItemText(hSubMenu,IDM_HELP_ABOUT,LoadResStr("IDS_MENU_ABOUT"));
	}

	void PropertyDlgInitLayout()
	{
		// Find out desired sizes and margins of elements used in property dialogue.
		// Just remember initial layout.
		// This is easier than getting all values from Windows metrics definitions.
		RECT client_rc = { 0,0,252,101 }, button_rc = { 207,182,254,202 };
		::GetClientRect(hPropertyDlg, &client_rc);
		HWND button = ::GetDlgItem(hPropertyDlg, IDOK);
		::GetWindowRect(button, &button_rc);
		property_dlg_inputarea_height = button_rc.bottom - button_rc.top;
		property_dlg_margin = 1; // hardcoded. The elements are actually placed quite poorly in the .rc, cannot derive from it
		property_dlg_okbutton_width = button_rc.right - button_rc.left;
	}

	void PropertyDlgUpdateSize()
	{
		// Positions unknown?
		if (!property_dlg_inputarea_height) return;
		// Reposition all child elements after size of property dialogue has changed
		RECT rc = { 0,0,0,0 };
		if (!::GetClientRect(hPropertyDlg, &rc)) return;
		int y0 = rc.bottom - property_dlg_margin - property_dlg_inputarea_height;
		// Output text box
		::SetWindowPos(::GetDlgItem(hPropertyDlg, IDC_EDITOUTPUT), nullptr,
			property_dlg_margin,
			property_dlg_margin,
			rc.right - 2* property_dlg_margin,
			y0 - 2* property_dlg_margin,
			SWP_NOOWNERZORDER | SWP_NOZORDER);
		// Input ComboBox
		::SetWindowPos(::GetDlgItem(hPropertyDlg, IDC_COMBOINPUT), nullptr,
			property_dlg_margin,
			y0,
			rc.right - property_dlg_okbutton_width - 3*property_dlg_margin,
			property_dlg_inputarea_height,
			SWP_NOOWNERZORDER | SWP_NOZORDER);
		// OK button
		::SetWindowPos(::GetDlgItem(hPropertyDlg, IDOK), nullptr,
			rc.right - property_dlg_margin - property_dlg_okbutton_width,
			y0,
			property_dlg_okbutton_width,
			property_dlg_inputarea_height,
			SWP_NOOWNERZORDER | SWP_NOZORDER);
	}

	void ConsoleInitLayout()
	{
		// Find out desired sizes and margins of elements used in console dialogue.
		// Just remember initial layout.
		// This is easier than getting all values from Windows metrics definitions.
		RECT console_rc = { 0,0,356,252 };
		::GetWindowRect(console_handle, &console_rc);
		console_default_width = console_rc.right - console_rc.left;
		console_default_height = console_rc.bottom - console_rc.top;
		console_margin = 1; // hardcoded margins
		console_wide_margin = 3;
		RECT button_rc = { 288,180,350,200 };
		::GetWindowRect(::GetDlgItem(console_handle, IDOK), &button_rc);
		console_button_height = button_rc.bottom - button_rc.top;
		console_ok_button_width = button_rc.right - button_rc.left;
		RECT status_rc = { 222,205,350,223 };
		::GetWindowRect(::GetDlgItem(console_handle, IDC_STATICTIME), &status_rc);
		console_status_width = status_rc.right - status_rc.left;
	}

	void ConsoleUpdateSize()
	{
		// Positions unknown?
		if (!console_default_width) return;
		// Reposition all child elements after size of console dialogue has changed
		RECT rc = { 0,0,0,0 };
		if (!::GetClientRect(console_handle, &rc)) return;
		int y0 = rc.bottom - console_margin * 3 - console_button_height * 3;
		int y1 = rc.bottom - console_margin * 2 - console_button_height * 2;
		int y2 = rc.bottom - console_margin * 1 - console_button_height * 1;
		int x0 = rc.right - console_margin - console_button_height;
		// Output text box
		::SetWindowPos(::GetDlgItem(console_handle, IDC_EDITOUTPUT), nullptr,
			console_margin,
			0,
			x0 - console_margin - console_wide_margin,
			y0 - console_margin,
			SWP_NOOWNERZORDER | SWP_NOZORDER);
		// Input ComboBox
		::SetWindowPos(::GetDlgItem(console_handle, IDC_COMBOINPUT), nullptr,
			console_margin,
			y0,
			rc.right - console_ok_button_width - console_margin * 3,
			console_button_height,
			SWP_NOOWNERZORDER | SWP_NOZORDER);
		// Input OK button
		::SetWindowPos(::GetDlgItem(console_handle, IDOK), nullptr,
			rc.right - console_margin - console_ok_button_width,
			y0,
			console_ok_button_width,
			console_button_height,
			SWP_NOOWNERZORDER | SWP_NOZORDER);
		// Frame status bar
		::SetWindowPos(::GetDlgItem(console_handle, IDC_STATICFRAME), nullptr,
			console_margin,
			y1,
			console_status_width,
			console_button_height,
			SWP_NOOWNERZORDER | SWP_NOZORDER);
		// Play button
		::SetWindowPos(::GetDlgItem(console_handle, IDC_BUTTONPLAY), nullptr,
			console_margin + console_status_width + console_wide_margin,
			y1,
			console_button_height,
			console_button_height,
			SWP_NOOWNERZORDER | SWP_NOZORDER);
		// Halt button
		::SetWindowPos(::GetDlgItem(console_handle, IDC_BUTTONHALT), nullptr,
			console_margin + console_status_width + console_wide_margin * 2 + console_button_height,
			y1,
			console_button_height,
			console_button_height,
			SWP_NOOWNERZORDER | SWP_NOZORDER);
		// Time/FPS status bar
		::SetWindowPos(::GetDlgItem(console_handle, IDC_STATICTIME), nullptr,
			rc.right - console_margin - console_status_width,
			y1,
			console_status_width,
			console_button_height,
			SWP_NOOWNERZORDER | SWP_NOZORDER);
		// Main status bar
		::SetWindowPos(::GetDlgItem(console_handle, IDC_STATICCURSOR), nullptr,
			console_margin,
			y2,
			rc.right - 2* console_margin,
			console_button_height,
			SWP_NOOWNERZORDER | SWP_NOZORDER);
		// Tool buttons
		::SetWindowPos(::GetDlgItem(console_handle, IDC_BUTTONMODEPLAY), nullptr,
			x0,
			console_margin,
			console_button_height,
			console_button_height,
			SWP_NOOWNERZORDER | SWP_NOZORDER);
		::SetWindowPos(::GetDlgItem(console_handle, IDC_BUTTONMODEEDIT), nullptr,
			x0,
			console_margin * 2 + console_button_height,
			console_button_height,
			console_button_height,
			SWP_NOOWNERZORDER | SWP_NOZORDER);
		::SetWindowPos(::GetDlgItem(console_handle, IDC_BUTTONMODEDRAW), nullptr,
			x0,
			console_margin * 3 + console_button_height * 2,
			console_button_height,
			console_button_height,
			SWP_NOOWNERZORDER | SWP_NOZORDER);
	}
};

void C4ConsoleGUI::UpdateMenuText(HMENU hMenu) { state->UpdateMenuText(*this, hMenu); }

static void ClearDlg(HWND &handle)
{
	if (handle)
		DestroyWindow(handle);
	handle = nullptr;
}

INT_PTR CALLBACK ConsoleDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
		//------------------------------------------------------------------------------------------------------------
	case WM_ACTIVATEAPP:
		Application.Active = wParam != 0;
		return true;
		//------------------------------------------------------------------------------------------------------------
	case WM_DESTROY:
		StoreWindowPosition(hDlg, "Main", Config.GetSubkeyPath("Console"), true);
		Application.Quit();
		return true;
		//------------------------------------------------------------------------------------------------------------
	case WM_CLOSE:
		Console.Close();
		return true;
		//------------------------------------------------------------------------------------------------------------
	case MM_MCINOTIFY:
		if (wParam == MCI_NOTIFY_SUCCESSFUL)
			Application.MusicSystem.NotifySuccess();
		return true;
		//------------------------------------------------------------------------------------------------------------
	case WM_INITDIALOG:
		Console.Active = true;
		SendMessage(hDlg, DM_SETDEFID, (WPARAM)IDOK, (LPARAM)0);
		Console.UpdateMenuText(GetMenu(hDlg));
		return true;
		//------------------------------------------------------------------------------------------------------------
	case WM_COMMAND:
		// Evaluate command
		switch (LOWORD(wParam))
		{
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDOK:
			// IDC_COMBOINPUT to Console.In()
			wchar_t buffer[16000];
			GetDlgItemTextW(hDlg, IDC_COMBOINPUT, buffer, 16000);
			if (buffer[0])
			{
				StdStrBuf in_char(buffer);
				::Console.RegisterRecentInput(in_char.getData(), C4Console::MRU_Scenario);
				::Console.In(in_char.getData());
				::Console.UpdateInputCtrl();
			}
			return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_BUTTONHALT:
			Console.DoHalt();
			return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_BUTTONPLAY:
			Console.DoPlay();
			return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_BUTTONMODEPLAY:
			Console.EditCursor.SetMode(C4CNS_ModePlay);
			return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_BUTTONMODEEDIT:
			Console.EditCursor.SetMode(C4CNS_ModeEdit);
			return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_BUTTONMODEDRAW:
			Console.EditCursor.SetMode(C4CNS_ModeDraw);
			return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDM_FILE_QUIT: Console.FileQuit(); return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDM_FILE_SAVEAS: Console.FileSaveAs(false);  return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDM_FILE_SAVE: Console.FileSave();  return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDM_FILE_SAVEGAMEAS: Console.FileSaveAs(true); return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDM_FILE_OPEN: Console.FileOpen(); return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDM_FILE_RECORD: Console.FileRecord(); return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDM_FILE_OPENWPLRS:  Console.FileOpenWPlrs(); return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDM_FILE_CLOSE: Console.FileClose(); return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDM_HELP_ABOUT: Console.HelpAbout(); return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDM_PLAYER_JOIN: Console.PlayerJoin(); return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDM_VIEWPORT_NEW: Console.ViewportNew(); return true;
		}
		// New player viewport
		if (Inside((int)LOWORD(wParam), IDM_VIEWPORT_NEW1, IDM_VIEWPORT_NEW2))
		{
			::Viewports.CreateViewport(LOWORD(wParam) - IDM_VIEWPORT_NEW1);
			return true;
		}
		// Remove player
		if (Inside((int)LOWORD(wParam), IDM_PLAYER_QUIT1, IDM_PLAYER_QUIT2))
		{
			C4Player *plr = ::Players.Get(LOWORD(wParam) - IDM_PLAYER_QUIT1);
			if (!plr) return true;
			::Control.Input.Add(CID_PlrAction, C4ControlPlayerAction::Eliminate(plr));
			return true;
		}
		// Remove client
		if (Inside((int)LOWORD(wParam), IDM_NET_CLIENT1, IDM_NET_CLIENT2))
		{
			if (!::Control.isCtrlHost()) return false;
			Game.Clients.CtrlRemove(Game.Clients.getClientByID(LOWORD(wParam) - IDM_NET_CLIENT1), LoadResStr("IDS_MSG_KICKBYMENU"));
			return true;
		}
		return false;
		//------------------------------------------------------------------------------------------------------------
	case WM_USER_LOG:
		if (SEqual2((const char *)lParam, "IDS_"))
			Log(LoadResStr((const char *)lParam));
		else
			Log((const char *)lParam);
		return false;
		//------------------------------------------------------------------------------------------------------------
	case WM_COPYDATA:
	{
		COPYDATASTRUCT* pcds = reinterpret_cast<COPYDATASTRUCT *>(lParam);
		if (pcds->dwData == WM_USER_RELOADFILE)
		{
			// get path, ensure proper termination
			const char *szPath = reinterpret_cast<const char *>(pcds->lpData);
			if (szPath[pcds->cbData - 1]) break;
			// reload
			Game.ReloadFile(szPath);
		}
		return false;
	}
	//------------------------------------------------------------------------------------------------------------
	case WM_INPUTLANGCHANGE:
		::Application.OnKeyboardLayoutChanged();
		break;
	//------------------------------------------------------------------------------------------------------------
	// Resizing
	case WM_GETMINMAXINFO:
		// Window may not become smaller than initial size
		if (Console.state && Console.state->console_default_width)
		{
			MINMAXINFO *info = reinterpret_cast<MINMAXINFO *>(lParam);
			info->ptMinTrackSize.x = Console.state->console_default_width;
			info->ptMinTrackSize.y = Console.state->console_default_height;
		}
		return 0;
	case WM_SIZING: Console.state->ConsoleUpdateSize(); break;
	case WM_WINDOWPOSCHANGED:
	{
		const WINDOWPOS *data = reinterpret_cast<const WINDOWPOS *>(lParam);
		if (data && !(data->flags & SWP_NOSIZE)) Console.state->ConsoleUpdateSize();
		break;
	}
	}
	return false;
}

class C4ToolsDlg::State: public C4ConsoleGUI::InternalState<class C4ToolsDlg>
{
public:
	HWND hDialog;
	C4Window *pPreviewWindow;
	friend INT_PTR CALLBACK ToolsDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
	HBITMAP hbmBrush,hbmBrush2;
	HBITMAP hbmLine,hbmLine2;
	HBITMAP hbmRect,hbmRect2;
	HBITMAP hbmFill,hbmFill2;
	HBITMAP hbmPicker,hbmPicker2;
	HBITMAP hbmDynamic;
	HBITMAP hbmStatic;
	HBITMAP hbmExact;
	
	State(C4ToolsDlg *toolsDlg): C4ConsoleGUI::InternalState<class C4ToolsDlg>(toolsDlg), hDialog(0),
		hbmBrush(0), hbmBrush2(0),
		hbmLine(0), hbmLine2(0),
		hbmRect(0), hbmRect2(0),
		hbmFill(0), hbmFill2(0),
		hbmPicker(0), hbmPicker2(0),
		hbmDynamic(0),
		hbmStatic(0),
		hbmExact(0)
	{
		pPreviewWindow = nullptr;
	}
	
	void LoadBitmaps(HINSTANCE instance)
	{
		if (!hbmBrush) hbmBrush=(HBITMAP)LoadBitmapW(instance,MAKEINTRESOURCEW(IDB_BRUSH));
		if (!hbmLine) hbmLine=(HBITMAP)LoadBitmapW(instance,MAKEINTRESOURCEW(IDB_LINE));
		if (!hbmRect) hbmRect=(HBITMAP)LoadBitmapW(instance,MAKEINTRESOURCEW(IDB_RECT));
		if (!hbmFill) hbmFill=(HBITMAP)LoadBitmapW(instance,MAKEINTRESOURCEW(IDB_FILL));
		if (!hbmPicker) hbmPicker=(HBITMAP)LoadBitmapW(instance,MAKEINTRESOURCEW(IDB_PICKER));
		if (!hbmBrush2) hbmBrush2=(HBITMAP)LoadBitmapW(instance,MAKEINTRESOURCEW(IDB_BRUSH2));
		if (!hbmLine2) hbmLine2=(HBITMAP)LoadBitmapW(instance,MAKEINTRESOURCEW(IDB_LINE2));
		if (!hbmRect2) hbmRect2=(HBITMAP)LoadBitmapW(instance,MAKEINTRESOURCEW(IDB_RECT2));
		if (!hbmFill2) hbmFill2=(HBITMAP)LoadBitmapW(instance,MAKEINTRESOURCEW(IDB_FILL2));
		if (!hbmPicker2) hbmPicker2=(HBITMAP)LoadBitmapW(instance,MAKEINTRESOURCEW(IDB_PICKER2));
		if (!hbmDynamic) hbmDynamic=(HBITMAP)LoadBitmapW(instance,MAKEINTRESOURCEW(IDB_DYNAMIC));
		if (!hbmStatic) hbmStatic=(HBITMAP)LoadBitmapW(instance,MAKEINTRESOURCEW(IDB_STATIC));
		if (!hbmExact) hbmExact=(HBITMAP)LoadBitmapW(instance,MAKEINTRESOURCEW(IDB_EXACT));
	}

	~State()
	{
		Clear();
	}

	void Clear()
	{
		// Unload bitmaps
		if (hbmBrush) { DeleteObject(hbmBrush); hbmBrush = 0; }
		if (hbmLine) { DeleteObject(hbmLine); hbmLine = 0; }
		if (hbmRect) { DeleteObject(hbmRect); hbmRect = 0; }
		if (hbmFill) { DeleteObject(hbmFill); hbmFill = 0; }
		if (hbmPicker) { DeleteObject(hbmPicker); hbmPicker = 0; }
		if (hbmBrush2) { DeleteObject(hbmBrush2); hbmBrush2 = 0; }
		if (hbmLine2) { DeleteObject(hbmLine2); hbmLine2 = 0; }
		if (hbmRect2) { DeleteObject(hbmRect2); hbmRect2 = 0; }
		if (hbmFill2) { DeleteObject(hbmFill2); hbmFill2 = 0; }
		if (hbmPicker2) { DeleteObject(hbmPicker2); hbmPicker2 = 0; }
		if (hbmDynamic) { DeleteObject(hbmDynamic); hbmDynamic = 0; }
		if (hbmStatic) { DeleteObject(hbmStatic); hbmStatic = 0; }
		if (hbmExact) { DeleteObject(hbmExact); hbmExact = 0; }
		if (pPreviewWindow)
		{
			delete pPreviewWindow;
			pPreviewWindow = nullptr;
		}
		if (hDialog) DestroyWindow(hDialog); hDialog=nullptr;
	}

	void Default()
	{
		GetOwner()->ModeBack = true;
	}

};

#include <commctrl.h>
INT_PTR CALLBACK ToolsDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	int32_t iValue;
	switch (Msg)
	{
		//----------------------------------------------------------------------------------------------
	case WM_CLOSE:
		Console.ToolsDlg.Clear();
		break;
		//----------------------------------------------------------------------------------------------
	case WM_DESTROY:
		StoreWindowPosition(hDlg, "Tools", Config.GetSubkeyPath("Console"), false);
		break;
		//----------------------------------------------------------------------------------------------
	case WM_INITDIALOG:
		return true;
		//----------------------------------------------------------------------------------------------
	case WM_PAINT:
		PostMessage(hDlg,WM_USER,0,0); // For user paint
		return false;
		//----------------------------------------------------------------------------------------------
	case WM_USER:
		Console.ToolsDlg.NeedPreviewUpdate();
		return true;
		//----------------------------------------------------------------------------------------------
	case WM_VSCROLL:
		switch (LOWORD(wParam))
		{
		case SB_THUMBTRACK: case SB_THUMBPOSITION:
			iValue=HIWORD(wParam);
			Console.ToolsDlg.SetGrade(C4TLS_GradeMax-iValue);
			break;
		case SB_PAGEUP: case SB_PAGEDOWN:
		case SB_LINEUP: case SB_LINEDOWN:
			iValue=SendDlgItemMessage(hDlg,IDC_SLIDERGRADE,TBM_GETPOS,0,0);
			Console.ToolsDlg.SetGrade(C4TLS_GradeMax-iValue);
			break;
		}
		return true;
		//----------------------------------------------------------------------------------------------
	case WM_COMMAND:
		// Evaluate command
		switch (LOWORD(wParam))
		{
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDOK:
			return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_BUTTONMODEDYNAMIC:
			Console.ToolsDlg.SetLandscapeMode(LandscapeMode::Dynamic, false);
			return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_BUTTONMODESTATIC:
			Console.ToolsDlg.SetLandscapeMode(LandscapeMode::Static, false);
			return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_BUTTONMODEEXACT:
			Console.ToolsDlg.SetLandscapeMode(LandscapeMode::Exact, false);
			return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_BUTTONBRUSH:
			Console.ToolsDlg.SetTool(C4TLS_Brush, false);
			return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_BUTTONLINE:
			Console.ToolsDlg.SetTool(C4TLS_Line, false);
			return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_BUTTONRECT:
			Console.ToolsDlg.SetTool(C4TLS_Rect, false);
			return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_BUTTONFILL:
			Console.ToolsDlg.SetTool(C4TLS_Fill, false);
			return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_BUTTONPICKER:
			Console.ToolsDlg.SetTool(C4TLS_Picker, false);
			return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_COMBOFGMATERIAL: case IDC_COMBOBGMATERIAL:
		case IDC_COMBOFGTEXTURE: case IDC_COMBOBGTEXTURE:
			switch (HIWORD(wParam))
			{
			case CBN_SELCHANGE:
			{
				// New material or texture selected. Get selection string
				wchar_t str[100];
				WORD idCombo = LOWORD(wParam);
				int32_t cursel = SendDlgItemMessage(hDlg, idCombo, CB_GETCURSEL, 0, 0);
				SendDlgItemMessage(hDlg, idCombo, CB_GETLBTEXT, cursel, (LPARAM)str);
				// Convert to ascii
				StdStrBuf str_buf(str);
				const char *astr = str_buf.getData();
				// Update appropriate setting in drawing tool
				switch (idCombo)
				{
				case IDC_COMBOFGMATERIAL: Console.ToolsDlg.SetMaterial(astr); break;
				case IDC_COMBOFGTEXTURE: Console.ToolsDlg.SetTexture(astr); break;
				case IDC_COMBOBGMATERIAL: Console.ToolsDlg.SetBackMaterial(astr); break;
				case IDC_COMBOBGTEXTURE: Console.ToolsDlg.SetBackTexture(astr); break;
				}
			}
			return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		}
		}
		return false;
		//----------------------------------------------------------------------------------------
	}
	return false;
}

INT_PTR CALLBACK PropertyDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
		//------------------------------------------------------------------------------------------------
	case WM_CLOSE:
		Console.PropertyDlgClose();
		break;
		//------------------------------------------------------------------------------------------------
	case WM_DESTROY:
		StoreWindowPosition(hDlg, "Property", Config.GetSubkeyPath("Console"), true);
		break;
		//------------------------------------------------------------------------------------------------
	case WM_INITDIALOG:
		SendMessage(hDlg,DM_SETDEFID,(WPARAM)IDOK,(LPARAM)0);
		return true;
	//------------------------------------------------------------------------------------------------
	// Callbacks during/after window resizing
	case WM_SIZING: Console.state->PropertyDlgUpdateSize(); break;
	case WM_WINDOWPOSCHANGED:
	{
		const WINDOWPOS *data = reinterpret_cast<const WINDOWPOS *>(lParam);
		if (data && !(data->flags & SWP_NOSIZE)) Console.state->PropertyDlgUpdateSize();
		break;
	}
	//------------------------------------------------------------------------------------------------
	case WM_COMMAND:
		// Evaluate command
		switch (LOWORD(wParam))
		{
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDOK:
			// IDC_COMBOINPUT to Console.EditCursor.In()
			wchar_t buffer[16000];
			GetDlgItemTextW(hDlg,IDC_COMBOINPUT,buffer,16000);
			if (buffer[0])
				Console.EditCursor.In(StdStrBuf(buffer).getData());
			return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_BUTTONRELOADDEF:
			{
			C4Object * pObj = Console.EditCursor.GetSelection().GetObject();
			if (pObj)
				Game.ReloadDef(pObj->id);
			return true;
			}
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		}
		return false;
		//-----------------------------------------------------------------------------------------------
	}
	return false;
}

void C4ConsoleGUI::Win32KeepDialogsFloating(HWND hwnd)
{
	if (!hwnd)
		hwnd = hWindow;
	if (Console.state->hPropertyDlg)
		SetWindowLongPtr(Console.state->hPropertyDlg, GWLP_HWNDPARENT, reinterpret_cast<LONG_PTR>(hwnd));
	if (Console.ToolsDlg.state->hDialog)
		SetWindowLongPtr(Console.ToolsDlg.state->hDialog, GWLP_HWNDPARENT, reinterpret_cast<LONG_PTR>(hwnd));
}

bool C4ConsoleGUI::Win32DialogMessageHandling(MSG *msg)
{
	return (hWindow && IsDialogMessage(hWindow,msg)) || (Console.state->hPropertyDlg && IsDialogMessage(Console.state->hPropertyDlg,msg));
}

void C4ConsoleGUI::SetCursor(Cursor cursor)
{
	::SetCursor(LoadCursor(0,IDC_WAIT));
}

bool C4ConsoleGUI::UpdateModeCtrls(int iMode)
{
	if (!Active)
		return false;

	SendDlgItemMessage(hWindow,IDC_BUTTONMODEPLAY,BM_SETSTATE,(iMode==C4CNS_ModePlay),0);
	UpdateWindow(GetDlgItem(hWindow,IDC_BUTTONMODEPLAY));
	SendDlgItemMessage(hWindow,IDC_BUTTONMODEEDIT,BM_SETSTATE,(iMode==C4CNS_ModeEdit),0);
	UpdateWindow(GetDlgItem(hWindow,IDC_BUTTONMODEEDIT));
	SendDlgItemMessage(hWindow,IDC_BUTTONMODEDRAW,BM_SETSTATE,(iMode==C4CNS_ModeDraw),0);
	UpdateWindow(GetDlgItem(hWindow,IDC_BUTTONMODEDRAW));
	return true;
}

bool C4ConsoleGUI::CreateConsoleWindow(C4AbstractApp *application)
{
	hWindow = CreateDialog(application->GetInstance(), MAKEINTRESOURCE(IDD_CONSOLE), nullptr, ConsoleDlgProc);
	if (!hWindow)
	{
		wchar_t * lpMsgBuf;
		FormatMessage(
		  FORMAT_MESSAGE_ALLOCATE_BUFFER |
		  FORMAT_MESSAGE_FROM_SYSTEM |
		  FORMAT_MESSAGE_IGNORE_INSERTS,
		  nullptr,
		  GetLastError(),
		  0,
		  (wchar_t *)&lpMsgBuf, // really.
		  0,
		  nullptr);
		Log(FormatString("Error creating dialog window: %s", StdStrBuf(lpMsgBuf).getData()).getData());
		// Free the buffer.
		LocalFree(lpMsgBuf);
		return false;
	}
	// Remember metrics
	state->console_handle = hWindow;
	state->ConsoleInitLayout();
	// Restore window position
	RestoreWindowPosition(hWindow, "Main", Config.GetSubkeyPath("Console"));
	// Set icon
	SendMessage(hWindow,WM_SETICON,ICON_BIG,(LPARAM)LoadIcon(application->GetInstance(),MAKEINTRESOURCE(IDI_00_C4X)));
	SendMessage(hWindow,WM_SETICON,ICON_SMALL,(LPARAM)LoadIcon(application->GetInstance(),MAKEINTRESOURCE(IDI_00_C4X)));
	// Set text
	SetTitle(LoadResStr("IDS_CNS_CONSOLE"));
	// Load bitmaps
	state->CreateBitmaps(application);
	// Enable controls
	UpdateHaltCtrls(true);
	EnableControls(fGameOpen);
	ClearViewportMenu();
	// Show window and set focus
	ShowWindow(hWindow,SW_SHOWNORMAL);
	UpdateWindow(hWindow);
	SetFocus(hWindow);
	ShowCursor(true);
	renderwnd = hWindow;
	// Success
	return true;
}

void C4ConsoleGUI::DeleteConsoleWindow()
{
}

void C4ConsoleGUI::DoEnableControls(bool fEnable)
{
	// Set button images (edit modes & halt controls)
	SendDlgItemMessage(hWindow,IDC_BUTTONMODEPLAY,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)(fEnable ? state->hbmMouse : state->hbmMouse2));
	SendDlgItemMessage(hWindow,IDC_BUTTONMODEEDIT,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)((fEnable && Editing) ? state->hbmCursor : state->hbmCursor2));
	SendDlgItemMessage(hWindow,IDC_BUTTONMODEDRAW,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)((fEnable && Editing) ? state->hbmBrush : state->hbmBrush2));
	SendDlgItemMessage(hWindow,IDC_BUTTONPLAY,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)(::Network.isLobbyActive() || fEnable ? state->hbmPlay : state->hbmPlay2));
	SendDlgItemMessage(hWindow,IDC_BUTTONHALT,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)(::Network.isLobbyActive() || fEnable ? state->hbmHalt : state->hbmHalt2));

	// OK
	EnableWindow( GetDlgItem(hWindow,IDOK), fEnable);

	// Halt controls
	EnableWindow(GetDlgItem(hWindow,IDC_BUTTONPLAY), ::Network.isLobbyActive() || fEnable);
	EnableWindow(GetDlgItem(hWindow,IDC_BUTTONHALT), ::Network.isLobbyActive() || fEnable);

	// Edit modes
	EnableWindow(GetDlgItem(hWindow,IDC_BUTTONMODEPLAY),(fEnable));
	EnableWindow(GetDlgItem(hWindow,IDC_BUTTONMODEEDIT),(fEnable && Editing));
	EnableWindow(GetDlgItem(hWindow,IDC_BUTTONMODEDRAW),(fEnable && Editing));

	// Console input
	EnableWindow(GetDlgItem(hWindow,IDC_COMBOINPUT), fEnable);

	// File menu
	// C4Network2 will have to handle that cases somehow (TODO: test)
	EnableMenuItem(GetMenu(hWindow),IDM_FILE_OPEN, MF_BYCOMMAND | MF_ENABLED );
	EnableMenuItem(GetMenu(hWindow),IDM_FILE_OPENWPLRS, MF_BYCOMMAND | MF_ENABLED );
	EnableMenuItem(GetMenu(hWindow),IDM_FILE_RECORD, MF_BYCOMMAND | ((Game.IsRunning && ::Control.IsRuntimeRecordPossible()) ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(GetMenu(hWindow),IDM_FILE_SAVEGAMEAS, MF_BYCOMMAND | ((fEnable && ::Players.GetCount()) ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(GetMenu(hWindow),IDM_FILE_SAVE, MF_BYCOMMAND | (fEnable ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(GetMenu(hWindow),IDM_FILE_SAVEAS, MF_BYCOMMAND | (fEnable ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(GetMenu(hWindow),IDM_FILE_CLOSE, MF_BYCOMMAND | (fEnable ? MF_ENABLED : MF_GRAYED));

	// Player & viewport menu
	EnableMenuItem(GetMenu(hWindow),IDM_PLAYER_JOIN, MF_BYCOMMAND | ((fEnable && Editing) ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(GetMenu(hWindow),IDM_VIEWPORT_NEW, MF_BYCOMMAND | (fEnable ? MF_ENABLED : MF_GRAYED));
}

bool C4ConsoleGUI::DoUpdateHaltCtrls(bool fHalt)
{
	SendDlgItemMessage(hWindow,IDC_BUTTONPLAY,BM_SETSTATE,!fHalt,0);
	UpdateWindow(GetDlgItem(hWindow,IDC_BUTTONPLAY));
	SendDlgItemMessage(hWindow,IDC_BUTTONHALT,BM_SETSTATE,fHalt,0);
	UpdateWindow(GetDlgItem(hWindow,IDC_BUTTONHALT));
	return true;
}

void C4ConsoleGUI::Out(const char* message)
{
	if (!Active) return;
	if (!message || !*message) return;
	int len,len2,lines; wchar_t *buffer, *buffer2;
	len = 65000;//SendDlgItemMessage(hWindow,IDC_EDITOUTPUT,EM_LINELENGTH,(WPARAM)0,(LPARAM)0);
	StdBuf messageW = GetWideCharBuf(message);
	len2 = len+std::min<int32_t>(messageW.getSize()/sizeof(wchar_t)+2, 5000);
	buffer = new wchar_t [len2];
	buffer[0]=0;
	GetDlgItemTextW(hWindow,IDC_EDITOUTPUT,buffer,len);
	if (buffer[0]) wcscat(buffer, L"\r\n");
	wcsncat(buffer,getBufPtr<wchar_t>(messageW),len2-wcslen(buffer)-1);
	if (wcslen(buffer) > 60000) buffer2 = buffer + wcslen(buffer) - 60000; else buffer2 = buffer; // max log length: Otherwise, discard beginning
	SetDlgItemTextW(hWindow,IDC_EDITOUTPUT,buffer2);
	delete [] buffer;
	lines = SendDlgItemMessage(hWindow,IDC_EDITOUTPUT,EM_GETLINECOUNT,(WPARAM)0,(LPARAM)0);
	SendDlgItemMessage(hWindow,IDC_EDITOUTPUT,EM_LINESCROLL,(WPARAM)0,(LPARAM)lines);
	UpdateWindow(hWindow);
}

bool C4ConsoleGUI::ClearLog()
{
	SetDlgItemTextW(hWindow,IDC_EDITOUTPUT,L"");
	SendDlgItemMessage(hWindow,IDC_EDITOUTPUT,EM_LINESCROLL,(WPARAM)0,0);
	UpdateWindow(hWindow);
	return true;
}

void C4ConsoleGUI::SetCaptionToFileName(const char* file_name)
{
}

void C4ConsoleGUI::DisplayInfoText(C4ConsoleGUI::InfoTextType type, StdStrBuf& text)
{
	if (!Active)
		return;
	int dialog_item;
	switch (type)
	{
	case CONSOLE_Cursor:
		dialog_item = IDC_STATICCURSOR;
		break;
	case CONSOLE_FrameCounter:
		dialog_item = IDC_STATICFRAME;
		break;
	case CONSOLE_TimeFPS:
		dialog_item = IDC_STATICTIME;
		break;
	default:
		assert(false);
		return;
	}
	SetDlgItemTextW(hWindow,dialog_item,text.GetWideChar());
	UpdateWindow(GetDlgItem(hWindow,dialog_item));
}

bool C4ConsoleGUI::Message(const char *message, bool query)
{
	return (IDOK==MessageBoxW(hWindow,GetWideChar(message),ADDL(C4ENGINECAPTION),query ? (MB_OKCANCEL | MB_ICONEXCLAMATION) : MB_ICONEXCLAMATION));
}

void C4ConsoleGUI::RecordingEnabled()
{
	EnableMenuItem(GetMenu(hWindow),IDM_FILE_RECORD, MF_BYCOMMAND | MF_GRAYED);
}

void C4ConsoleGUI::ShowAboutWithCopyright(StdStrBuf &copyright)
{
	StdStrBuf strMessage; strMessage.Format("%s %s\n\n%s", C4ENGINECAPTION, C4VERSION, copyright.getData());
	MessageBoxW(nullptr, strMessage.GetWideChar(), ADDL(C4ENGINECAPTION), MB_ICONINFORMATION | MB_TASKMODAL);
}

bool C4ConsoleGUI::FileSelect(StdStrBuf *sFilename, const char * szFilter, DWORD dwFlags, bool fSave)
{
	enum { ArbitraryMaximumLength = 4096 };
	wchar_t buffer[ArbitraryMaximumLength];
	sFilename->ReplaceChar('/', '\\'); // GetSaveFileNameW has trouble with forward slashes
	wcsncpy(buffer, sFilename->GetWideChar(), ArbitraryMaximumLength - 1);
	buffer[ArbitraryMaximumLength - 1] = 0;
	OPENFILENAMEW ofn;
	ZeroMem(&ofn,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hwndOwner=hWindow;
	const char * s = szFilter;
	while (*s) s = s + strlen(s) + 1;
	s++;
	int n = s - szFilter;
	int len = MultiByteToWideChar(CP_UTF8, 0, szFilter, n, nullptr, 0);
	StdBuf filt;
	filt.SetSize(len * sizeof(wchar_t));
	MultiByteToWideChar(CP_UTF8, 0, szFilter, n, getMBufPtr<wchar_t>(filt), len );
	ofn.lpstrFilter=getMBufPtr<wchar_t>(filt);
	ofn.lpstrFile=buffer;
	ofn.nMaxFile=ArbitraryMaximumLength;
	ofn.Flags=dwFlags;

	bool fResult;
	size_t l = GetCurrentDirectoryW(0,0);
	wchar_t *wd = new wchar_t[l];
	GetCurrentDirectoryW(l,wd);
	if (fSave)
		fResult = !!GetSaveFileNameW(&ofn);
	else
		fResult = !!GetOpenFileNameW(&ofn);
	// Reset working directory to exe path as Windows file dialog might have changed it
	SetCurrentDirectoryW(wd);
	delete[] wd;
	len = WideCharToMultiByte(CP_UTF8, 0, buffer, ArbitraryMaximumLength, nullptr, 0, 0, 0);
	sFilename->SetLength(len - 1);
	WideCharToMultiByte(CP_UTF8, 0, buffer, ArbitraryMaximumLength, sFilename->getMData(), sFilename->getSize(), 0, 0);
	return fResult;
}

void C4ConsoleGUI::AddMenuItemForPlayer(C4Player* player, StdStrBuf& player_text)
{
	AddMenuItem(this, GetSubMenu(GetMenu(hWindow),state->MenuIndexViewport),IDM_VIEWPORT_NEW1+player->Number,player_text.getData(), true);
}

void C4ConsoleGUI::AddKickPlayerMenuItem(C4Player *player, StdStrBuf& player_text, bool enabled)
{
	AddMenuItem(this, GetSubMenu(GetMenu(hWindow),state->MenuIndexPlayer),IDM_PLAYER_QUIT1+player->Number,player_text.getData(),(!::Network.isEnabled() || ::Network.isHost()) && Editing);
}

void C4ConsoleGUI::AddNetMenu()
{
	if (!InsertMenuW(GetMenu(hWindow),state->MenuIndexHelp,MF_BYPOSITION | MF_POPUP,(UINT_PTR)CreateMenu(),LoadResStrW("IDS_MNU_NET"))) return;
	state->MenuIndexNet=state->MenuIndexHelp;
	state->MenuIndexHelp++;
	DrawMenuBar(hWindow);
}

void C4ConsoleGUI::ClearNetMenu()
{
	if (state->MenuIndexNet<0) return;
	DeleteMenu(GetMenu(hWindow),state->MenuIndexNet,MF_BYPOSITION);
	state->MenuIndexNet=-1;
	state->MenuIndexHelp--;
	DrawMenuBar(hWindow);
}

void C4ConsoleGUI::AddNetMenuItemForPlayer(int32_t client_id, const char *text, C4ConsoleGUI::ClientOperation op)
{
	AddMenuItem(this, GetSubMenu(GetMenu(hWindow),state->MenuIndexNet), IDM_NET_CLIENT1+Game.Clients.getLocalID(), text, true);
}

void C4ConsoleGUI::ClearViewportMenu()
{
	HMENU hMenu = GetSubMenu(GetMenu(hWindow),state->MenuIndexViewport);
	while (DeleteMenu(hMenu,1,MF_BYPOSITION));
}

void C4ConsoleGUI::ToolsDlgClose()
{
	if (Console.ToolsDlg.state)
		Console.ToolsDlg.state->Clear();
}

bool C4ConsoleGUI::PropertyDlgOpen()
{
	if (state->hPropertyDlg) return true;
	HWND hDialog = CreateDialog(Application.GetInstance(),
	                       MAKEINTRESOURCE(IDD_PROPERTIES),
	                       Console.hWindow,
	                       PropertyDlgProc);
	if (!hDialog) return false;
	state->hPropertyDlg = hDialog;
	// Remember initial layout
	state->PropertyDlgInitLayout();
	// Set text
	SetWindowTextW(hDialog,LoadResStrW("IDS_DLG_PROPERTIES"));
	// Enable controls
	EnableWindow( GetDlgItem(hDialog,IDOK), Editing );
	EnableWindow( GetDlgItem(hDialog,IDC_COMBOINPUT), Editing );
	EnableWindow( GetDlgItem(hDialog,IDC_BUTTONRELOADDEF), Editing );
	// Show window
	RestoreWindowPosition(hDialog, "Property", Config.GetSubkeyPath("Console"));
	SetWindowPos(hDialog,hWindow,0,0,0,0,SWP_NOSIZE | SWP_NOMOVE);
	ShowWindow(hDialog,SW_SHOWNOACTIVATE);
	return true;
}

void C4ConsoleGUI::PropertyDlgClose()
{
	::ClearDlg(state->hPropertyDlg);
}

static void SetComboItems(HWND hCombo, std::list<const char*> &items)
{
	for (std::list<const char*>::iterator it = items.begin(); it != items.end(); it++)
	{
		if (!*it)
			SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"----------");
		else
			SendMessage(hCombo,CB_ADDSTRING,0,GetWideLPARAM(*it));
	}
}

void C4ConsoleGUI::PropertyDlgUpdate(C4EditCursorSelection &rSelection, bool force_function_update)
{
	HWND hDialog = state->hPropertyDlg;
	if (!hDialog) return;
	int iLine = SendDlgItemMessage(hDialog,IDC_EDITOUTPUT,EM_GETFIRSTVISIBLELINE,(WPARAM)0,(LPARAM)0);
	SetDlgItemTextW(hDialog,IDC_EDITOUTPUT,rSelection.GetDataString().GetWideChar());
	SendDlgItemMessage(hDialog,IDC_EDITOUTPUT,EM_LINESCROLL,(WPARAM)0,(LPARAM)iLine);
	UpdateWindow(GetDlgItem(hDialog,IDC_EDITOUTPUT));

	if (PropertyDlgObject == rSelection.GetObject() && !force_function_update) return;
	PropertyDlgObject = rSelection.GetObject();
	
	std::list<const char *> functions = ::Console.GetScriptSuggestions(PropertyDlgObject, C4Console::MRU_Object);

	HWND hCombo = GetDlgItem(state->hPropertyDlg, IDC_COMBOINPUT);
	wchar_t szLastText[500+1];
	// Remember old window text
	GetWindowTextW(hCombo, szLastText, 500);
	// Clear
	SendMessage(hCombo, CB_RESETCONTENT, 0, 0);

	SetComboItems(GetDlgItem(state->hPropertyDlg,IDC_COMBOINPUT), functions);
	
	// Restore
	SetWindowTextW(hCombo, szLastText);
}

void C4ConsoleGUI::SetInputFunctions(std::list<const char*> &functions)
{
	HWND hCombo = GetDlgItem(hWindow, IDC_COMBOINPUT);
	SendMessage(hCombo, CB_RESETCONTENT, 0, 0);
	SetComboItems(hCombo, functions);
}

void C4ConsoleGUI::ClearPlayerMenu()
{
	if (!Active) return;
	HMENU hMenu = GetSubMenu(GetMenu(hWindow),state->MenuIndexPlayer);
	while (DeleteMenu(hMenu,1,MF_BYPOSITION));
}

/*
void C4ConsoleGUI::ClearPropertyDlg(C4PropertyDlg *dlg)
{
	if (dlg->state->hDialog) DestroyWindow(PropertyDlg.hDialog); PropertyDlg.hDialog=nullptr;
}
*/

// Wrapper window around preview control: Used to create GL context and target surface
class C4ConsoleGUIPreviewWindow : public C4Window
{
public:
	C4ConsoleGUIPreviewWindow(HWND hwndControl)
	{
		Init(C4Window::WindowKind::W_Control, &Application, nullptr, nullptr);
		this->hWindow = this->renderwnd = hwndControl;
		pSurface = new C4Surface(&Application, this);
	}

	~C4ConsoleGUIPreviewWindow()
	{
		delete pSurface;
	}

	virtual void Close() {}
};

bool C4ConsoleGUI::ToolsDlgOpen(C4ToolsDlg *dlg)
{
	if (dlg->state->hDialog) return true;
	dlg->state->hDialog = CreateDialog(Application.GetInstance(),
	                       MAKEINTRESOURCE(IDD_TOOLS),
	                       Console.hWindow,
	                       ToolsDlgProc);
	if (!dlg->state->hDialog) return false;
	// Set text
	SetWindowTextW(dlg->state->hDialog,LoadResStrW("IDS_DLG_TOOLS"));
	SetDlgItemTextW(dlg->state->hDialog,IDC_STATICMATERIAL,LoadResStrW("IDS_CTL_MATERIAL"));
	SetDlgItemTextW(dlg->state->hDialog,IDC_STATICTEXTURE,LoadResStrW("IDS_CTL_TEXTURE"));
	SetDlgItemTextW(dlg->state->hDialog, IDC_STATICFOREGROUND, LoadResStrW("IDS_CTL_FOREGROUND"));
	SetDlgItemTextW(dlg->state->hDialog, IDC_STATICBACKGROUND, LoadResStrW("IDS_CTL_BACKGROUND"));
	// Load bitmaps if necessary
	dlg->state->LoadBitmaps(Application.GetInstance());
	// create target ctx for OpenGL rendering
	if (pDraw && !dlg->state->pPreviewWindow)
	{
		dlg->state->pPreviewWindow = new C4ConsoleGUIPreviewWindow(GetDlgItem(dlg->state->hDialog, IDC_PREVIEW));
	}
	// Show window
	RestoreWindowPosition(dlg->state->hDialog, "Tools", Config.GetSubkeyPath("Console"));
	SetWindowPos(dlg->state->hDialog,Console.hWindow,0,0,0,0,SWP_NOSIZE | SWP_NOMOVE);
	ShowWindow(dlg->state->hDialog,SW_SHOWNOACTIVATE);
	return true;
}

void C4ConsoleGUI::ToolsDlgInitMaterialCtrls(class C4ToolsDlg *dlg)
{
	// All foreground materials
	SendDlgItemMessage(dlg->state->hDialog, IDC_COMBOFGMATERIAL,CB_ADDSTRING,0,GetWideLPARAM(C4TLS_MatSky));
	for (int32_t cnt = 0; cnt < ::MaterialMap.Num; cnt++)
	{
		SendDlgItemMessage(dlg->state->hDialog, IDC_COMBOFGMATERIAL, CB_ADDSTRING, 0, GetWideLPARAM(::MaterialMap.Map[cnt].Name));
	}
	// Background materials: True background materials first; then the "funny" stuff
	SendDlgItemMessage(dlg->state->hDialog, IDC_COMBOBGMATERIAL, CB_ADDSTRING, 0, GetWideLPARAM(C4TLS_MatSky));
	for (int32_t cnt = 0; cnt < ::MaterialMap.Num; cnt++)
	{
		if (::MaterialMap.Map[cnt].Density == C4M_Background)
		{
			SendDlgItemMessage(dlg->state->hDialog, IDC_COMBOBGMATERIAL, CB_ADDSTRING, 0, GetWideLPARAM(::MaterialMap.Map[cnt].Name));
		}
	}
	SendDlgItemMessage(dlg->state->hDialog, IDC_COMBOBGMATERIAL, CB_ADDSTRING, 0, (LPARAM)L"----------");
	for (int32_t cnt = 0; cnt < ::MaterialMap.Num; cnt++)
	{
		if (::MaterialMap.Map[cnt].Density != C4M_Background)
		{
			SendDlgItemMessage(dlg->state->hDialog, IDC_COMBOBGMATERIAL, CB_ADDSTRING, 0, GetWideLPARAM(::MaterialMap.Map[cnt].Name));
		}
	}
	// Select current materials
	SendDlgItemMessage(dlg->state->hDialog,IDC_COMBOFGMATERIAL,CB_SELECTSTRING,0,GetWideLPARAM(dlg->Material));
	SendDlgItemMessage(dlg->state->hDialog, IDC_COMBOBGMATERIAL, CB_SELECTSTRING, 0, GetWideLPARAM(dlg->BackMaterial));
}

void C4ToolsDlg::UpdateToolCtrls()
{
	HWND hDialog = state->hDialog;
	int32_t Tool = this->Tool;
	SendDlgItemMessage(hDialog,IDC_BUTTONBRUSH,BM_SETSTATE,(Tool==C4TLS_Brush),0);
	UpdateWindow(GetDlgItem(hDialog,IDC_BUTTONBRUSH));
	SendDlgItemMessage(hDialog,IDC_BUTTONLINE,BM_SETSTATE,(Tool==C4TLS_Line),0);
	UpdateWindow(GetDlgItem(hDialog,IDC_BUTTONLINE));
	SendDlgItemMessage(hDialog,IDC_BUTTONRECT,BM_SETSTATE,(Tool==C4TLS_Rect),0);
	UpdateWindow(GetDlgItem(hDialog,IDC_BUTTONRECT));
	SendDlgItemMessage(hDialog,IDC_BUTTONFILL,BM_SETSTATE,(Tool==C4TLS_Fill),0);
	UpdateWindow(GetDlgItem(hDialog,IDC_BUTTONFILL));
	SendDlgItemMessage(hDialog,IDC_BUTTONPICKER,BM_SETSTATE,(Tool==C4TLS_Picker),0);
	UpdateWindow(GetDlgItem(hDialog,IDC_BUTTONPICKER));
}

void C4ConsoleGUI::ToolsDlgSelectTexture(C4ToolsDlg *dlg, const char *texture)
{
	SendDlgItemMessage(dlg->state->hDialog,IDC_COMBOFGTEXTURE,CB_SELECTSTRING,0,GetWideLPARAM(texture));
}

void C4ConsoleGUI::ToolsDlgSelectMaterial(C4ToolsDlg *dlg, const char *material)
{
	SendDlgItemMessage(dlg->state->hDialog,IDC_COMBOFGMATERIAL,CB_SELECTSTRING,0,GetWideLPARAM(material));
}

void C4ConsoleGUI::ToolsDlgSelectBackTexture(C4ToolsDlg *dlg, const char *texture)
{
	SendDlgItemMessage(dlg->state->hDialog, IDC_COMBOBGTEXTURE, CB_SELECTSTRING, 0, GetWideLPARAM(texture));
}

void C4ConsoleGUI::ToolsDlgSelectBackMaterial(C4ToolsDlg *dlg, const char *material)
{
	SendDlgItemMessage(dlg->state->hDialog, IDC_COMBOBGMATERIAL, CB_SELECTSTRING, 0, GetWideLPARAM(material));
}

void C4ToolsDlg::UpdateTextures()
{
	// Refill foreground and background combo boxes in dlg
	WORD boxes[2] = { IDC_COMBOFGTEXTURE, IDC_COMBOBGTEXTURE };
	const char *materials[2] = { Material, BackMaterial };
	const char *textures[2] = { Texture, BackTexture };
	for (int i = 0; i < 2; ++i)
	{
		WORD box = boxes[i];
		const char *material = materials[i];
		const char *texture = textures[i];
		// clear previous
		SendDlgItemMessage(state->hDialog, box, CB_RESETCONTENT, 0, (LPARAM)0);
		// bottom-most: any invalid textures
		bool fAnyEntry = false; int32_t cnt; const char *szTexture;
		if (::Landscape.GetMode() != LandscapeMode::Exact)
			for (cnt = 0; (szTexture = ::TextureMap.GetTexture(cnt)); cnt++)
			{
				if (!::TextureMap.GetIndex(material, szTexture, false))
				{
					// hide normal maps from texture selection
					// theoretically, they could be used for drawing but they clutter the list and they don't look good
					if (!WildcardMatch("*_NRM", szTexture))
					{
						fAnyEntry = true;
						SendDlgItemMessage(state->hDialog, box, CB_INSERTSTRING, 0, GetWideLPARAM(szTexture));
					}
				}
			}
		// separator
		if (fAnyEntry)
		{
			SendDlgItemMessage(state->hDialog, box, CB_INSERTSTRING, 0, (LPARAM)L"-------");
		}

		// atop: valid textures
		for (cnt = 0; (szTexture = ::TextureMap.GetTexture(cnt)); cnt++)
		{
			// Current material-texture valid? Always valid for exact mode
			if (::TextureMap.GetIndex(material, szTexture, false) || ::Landscape.GetMode() == LandscapeMode::Exact)
			{
				SendDlgItemMessage(state->hDialog, box, CB_INSERTSTRING, 0, GetWideLPARAM(szTexture));
			}
		}
		// reselect current
		SendDlgItemMessage(state->hDialog, box, CB_SELECTSTRING, -1, GetWideLPARAM(texture));
	}
}

void C4ToolsDlg::NeedPreviewUpdate()
{
	if (!state->hDialog || !state->pPreviewWindow) return;

	C4Surface * sfcPreview = state->pPreviewWindow->pSurface;
	if (!sfcPreview) return;

	int32_t iPrvWdt,iPrvHgt;
	RECT rect;

	GetClientRect(GetDlgItem(state->hDialog,IDC_PREVIEW),&rect);

	iPrvWdt=rect.right-rect.left;
	iPrvHgt=rect.bottom-rect.top;

	if (!sfcPreview->UpdateSize(iPrvWdt, iPrvHgt)) return;
	sfcPreview->NoClip();
	if (!pDraw->PrepareRendering(sfcPreview)) return;

	// fill bg
	pDraw->DrawBoxDw(sfcPreview,0,0,iPrvWdt-1,iPrvHgt-1,C4RGB(0xa0,0xa0,0xa0));
	BYTE bCol = 0;
	C4Pattern Pattern;
	// Sky material: sky as pattern only
	if (SEqual(Material,C4TLS_MatSky))
	{
		Pattern.Set(::Landscape.GetSky().Surface, 0);
	}
	// Material-Texture
	else
	{
		bCol=Mat2PixColDefault(::MaterialMap.Get(Material));
		// Get/Create TexMap entry
		BYTE iTex = ::TextureMap.GetIndex(Material, Texture, true);
		if (iTex)
		{
			// Define texture pattern
			const C4TexMapEntry *pTex = ::TextureMap.GetEntry(iTex);
			// Security
			if (pTex)
			{
				// Set drawing pattern
				Pattern = pTex->GetPattern();
			}
		}
	}
	if (IsWindowEnabled(GetDlgItem(state->hDialog,IDC_PREVIEW)))
		pDraw->DrawPatternedCircle( sfcPreview,
		                              iPrvWdt/2,iPrvHgt/2,
		                              Grade,
		                              bCol, Pattern, *::Landscape.GetPal());

	sfcPreview->PageFlip();
}

void C4ToolsDlg::InitGradeCtrl()
{
	if (!state->hDialog) return;
	HWND hwndTrack = GetDlgItem(state->hDialog,IDC_SLIDERGRADE);
	SendMessage(hwndTrack,TBM_SETPAGESIZE,0,(LPARAM)5);
	SendMessage(hwndTrack,TBM_SETLINESIZE,0,(LPARAM)1);
	SendMessage(hwndTrack,TBM_SETRANGE,(WPARAM)false,
		(LPARAM) MAKELONG(C4TLS_GradeMin,C4TLS_GradeMax));
	SendMessage(hwndTrack,TBM_SETPOS,(WPARAM)true,(LPARAM)C4TLS_GradeMax-Grade);
	UpdateWindow(hwndTrack);
}

bool C4ToolsDlg::PopMaterial()
{
	if (!state->hDialog) return false;
	SetFocus(GetDlgItem(state->hDialog,IDC_COMBOFGMATERIAL));
	SendDlgItemMessage(state->hDialog,IDC_COMBOFGMATERIAL,CB_SHOWDROPDOWN,true,0);
	return true;
}

bool C4ToolsDlg::PopTextures()
{
	if (!state->hDialog) return false;
	SetFocus(GetDlgItem(state->hDialog,IDC_COMBOFGTEXTURE));
	SendDlgItemMessage(state->hDialog,IDC_COMBOFGTEXTURE,CB_SHOWDROPDOWN,true,0);
	return true;
}

void C4ToolsDlg::UpdateIFTControls()
{
	// not using IFT
}

void C4ToolsDlg::UpdateLandscapeModeCtrls()
{
	LandscapeMode iMode = ::Landscape.GetMode();
	// Dynamic: enable only if dynamic anyway
	SendDlgItemMessage(state->hDialog,IDC_BUTTONMODEDYNAMIC,BM_SETSTATE,(iMode==LandscapeMode::Dynamic),0);
	EnableWindow(GetDlgItem(state->hDialog,IDC_BUTTONMODEDYNAMIC),(iMode==LandscapeMode::Dynamic));
	UpdateWindow(GetDlgItem(state->hDialog,IDC_BUTTONMODEDYNAMIC));
	// Static: enable only if map available
	SendDlgItemMessage(state->hDialog,IDC_BUTTONMODESTATIC,BM_SETSTATE,(iMode==LandscapeMode::Static),0);
	EnableWindow(GetDlgItem(state->hDialog,IDC_BUTTONMODESTATIC),(::Landscape.HasMap()));
	UpdateWindow(GetDlgItem(state->hDialog,IDC_BUTTONMODESTATIC));
	// Exact: enable always
	SendDlgItemMessage(state->hDialog,IDC_BUTTONMODEEXACT,BM_SETSTATE,(iMode==LandscapeMode::Exact),0);
	UpdateWindow(GetDlgItem(state->hDialog,IDC_BUTTONMODEEXACT));
	// Set dialog caption
	SetWindowTextW(state->hDialog,LoadResStrW(iMode==LandscapeMode::Dynamic ? "IDS_DLG_DYNAMIC" : iMode==LandscapeMode::Static ? "IDS_DLG_STATIC" : "IDS_DLG_EXACT"));
}


void C4ToolsDlg::EnableControls()
{
	HWND hDialog = state->hDialog;
	LandscapeMode iLandscapeMode = ::Landscape.GetMode();
	// Set bitmap buttons
	SendDlgItemMessage(hDialog,IDC_BUTTONBRUSH,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)((iLandscapeMode>=LandscapeMode::Static) ? state->hbmBrush : state->hbmBrush2));
	SendDlgItemMessage(hDialog,IDC_BUTTONLINE,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)((iLandscapeMode>=LandscapeMode::Static) ? state->hbmLine : state->hbmLine2));
	SendDlgItemMessage(hDialog,IDC_BUTTONRECT,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)((iLandscapeMode>=LandscapeMode::Static) ? state->hbmRect : state->hbmRect2));
	SendDlgItemMessage(hDialog,IDC_BUTTONFILL,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)((iLandscapeMode>=LandscapeMode::Exact) ? state->hbmFill : state->hbmFill2));
	SendDlgItemMessage(hDialog,IDC_BUTTONPICKER,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)((iLandscapeMode>=LandscapeMode::Static) ? state->hbmPicker : state->hbmPicker2));
	SendDlgItemMessage(hDialog,IDC_BUTTONMODEDYNAMIC,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)state->hbmDynamic);
	SendDlgItemMessage(hDialog,IDC_BUTTONMODESTATIC,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)state->hbmStatic);
	SendDlgItemMessage(hDialog,IDC_BUTTONMODEEXACT,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)state->hbmExact);
	// Enable drawing controls
	EnableWindow(GetDlgItem(hDialog,IDC_BUTTONBRUSH),(iLandscapeMode>=LandscapeMode::Static));
	EnableWindow(GetDlgItem(hDialog,IDC_BUTTONLINE),(iLandscapeMode>=LandscapeMode::Static));
	EnableWindow(GetDlgItem(hDialog,IDC_BUTTONRECT),(iLandscapeMode>=LandscapeMode::Static));
	EnableWindow(GetDlgItem(hDialog,IDC_BUTTONFILL),(iLandscapeMode>=LandscapeMode::Exact));
	EnableWindow(GetDlgItem(hDialog,IDC_BUTTONPICKER),(iLandscapeMode>=LandscapeMode::Static));
	EnableWindow(GetDlgItem(hDialog,IDC_COMBOFGMATERIAL),(iLandscapeMode>=LandscapeMode::Static));
	EnableWindow(GetDlgItem(hDialog,IDC_COMBOFGTEXTURE),(iLandscapeMode>=LandscapeMode::Static) && !SEqual(Material,C4TLS_MatSky));
	EnableWindow(GetDlgItem(hDialog,IDC_COMBOBGMATERIAL), (iLandscapeMode >= LandscapeMode::Static) && !SEqual(Material, C4TLS_MatSky));
	EnableWindow(GetDlgItem(hDialog, IDC_COMBOBGTEXTURE), (iLandscapeMode >= LandscapeMode::Static) && !SEqual(Material, C4TLS_MatSky) && !SEqual(BackMaterial, C4TLS_MatSky));
	EnableWindow(GetDlgItem(hDialog,IDC_STATICMATERIAL),(iLandscapeMode>=LandscapeMode::Static));
	EnableWindow(GetDlgItem(hDialog,IDC_STATICTEXTURE),(iLandscapeMode>=LandscapeMode::Static) && !SEqual(Material,C4TLS_MatSky));
	EnableWindow(GetDlgItem(hDialog,IDC_STATICFOREGROUND), (iLandscapeMode >= LandscapeMode::Static));
	EnableWindow(GetDlgItem(hDialog,IDC_STATICBACKGROUND), (iLandscapeMode >= LandscapeMode::Static));
	EnableWindow(GetDlgItem(hDialog,IDC_SLIDERGRADE),(iLandscapeMode>=LandscapeMode::Static));
	EnableWindow(GetDlgItem(hDialog,IDC_PREVIEW),(iLandscapeMode>=LandscapeMode::Static));

	NeedPreviewUpdate();
}

#include "editor/C4ConsoleGUICommon.h"
