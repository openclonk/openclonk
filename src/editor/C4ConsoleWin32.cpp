/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2003  Matthes Bender
 * Copyright (c) 2004  Peter Wortmann
 * Copyright (c) 2005, 2007  Sven Eberhardt
 * Copyright (c) 2005-2007, 2009-2011  Günther Brammer
 * Copyright (c) 2009  David Dormagen
 * Copyright (c) 2009, 2011  Nicolas Hake
 * Copyright (c) 2010  Martin Plicht
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

#include <C4Include.h>
#include <C4Console.h>

#include <C4Aul.h>
#include <C4Application.h>
#include <C4GameSave.h>
#include <C4Game.h>
#include <C4MessageInput.h>
#include <C4UserMessages.h>
#include <C4Version.h>
#include <C4Language.h>
#include <C4Object.h>
#include <C4Player.h>
#include <C4Landscape.h>
#include <C4GraphicsSystem.h>
#include <C4PlayerList.h>
#include <C4GameControl.h>
#include <C4Texture.h>

#include <StdFile.h>
#include <StdRegistry.h>

#ifdef USE_GL
#include <StdGL.h>
#endif

#ifdef USE_DIRECTX
#include <StdD3D.h>
#endif

#include "C4ConsoleGUI.h"
#include "C4Viewport.h"

#include <C4windowswrapper.h>
#include <mmsystem.h>
#include <commdlg.h>
#include "resource.h"

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

	State(C4ConsoleGUI *console)
	{
		hbmMouse=NULL;
		hbmMouse2=NULL;
		hbmCursor=NULL;
		hbmCursor2=NULL;
		hbmBrush=NULL;
		hbmBrush2=NULL;
		hbmPlay=NULL;
		hbmPlay2=NULL;
		hbmHalt=NULL;
		hbmHalt2=NULL;
		hPropertyDlg=NULL;
		MenuIndexFile       =  0;
		MenuIndexPlayer     =  1;
		MenuIndexViewport   =  2;
		MenuIndexNet        = -1;
		MenuIndexHelp       =  3;
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

	void CreateBitmaps(CStdApp *application)
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
};

void C4ConsoleGUI::UpdateMenuText(HMENU hMenu) { state->UpdateMenuText(*this, hMenu); }

static void ClearDlg(HWND &handle)
{
	if (handle)
		DestroyWindow(handle);
	handle = NULL;
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
		StoreWindowPosition(hDlg, "Main", Config.GetSubkeyPath("Console"), false);
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
		SendMessage(hDlg,DM_SETDEFID,(WPARAM)IDOK,(LPARAM)0);
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
			GetDlgItemTextW(hDlg,IDC_COMBOINPUT,buffer,16000);
			if (buffer[0])
				Console.In(StdStrBuf(buffer).getData());
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
		if (Inside((int) LOWORD(wParam),IDM_VIEWPORT_NEW1,IDM_VIEWPORT_NEW2))
		{
			::Viewports.CreateViewport(LOWORD(wParam)-IDM_VIEWPORT_NEW1);
			return true;
		}
		// Remove player
		if (Inside((int) LOWORD(wParam),IDM_PLAYER_QUIT1,IDM_PLAYER_QUIT2))
		{
			::Control.Input.Add(CID_Script, new C4ControlScript(
			                      FormatString("EliminatePlayer(%d)", LOWORD(wParam)-IDM_PLAYER_QUIT1).getData()));
			return true;
		}
		// Remove client
		if (Inside((int) LOWORD(wParam),IDM_NET_CLIENT1,IDM_NET_CLIENT2))
		{
			if (!::Control.isCtrlHost()) return false;
			Game.Clients.CtrlRemove(Game.Clients.getClientByID(LOWORD(wParam)-IDM_NET_CLIENT1), LoadResStr("IDS_MSG_KICKBYMENU"));
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

	return false;
}

class C4ToolsDlg::State: public C4ConsoleGUI::InternalState<class C4ToolsDlg>
{
public:
	HWND hDialog;
#ifdef USE_GL
	CStdGLCtx* pGLCtx;
#endif
	friend INT_PTR CALLBACK ToolsDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
	HBITMAP hbmBrush,hbmBrush2;
	HBITMAP hbmLine,hbmLine2;
	HBITMAP hbmRect,hbmRect2;
	HBITMAP hbmFill,hbmFill2;
	HBITMAP hbmPicker,hbmPicker2;
	HBITMAP hbmIFT;
	HBITMAP hbmNoIFT;
	HBITMAP hbmDynamic;
	HBITMAP hbmStatic;
	HBITMAP hbmExact;
	
	State(C4ToolsDlg *toolsDlg): C4ConsoleGUI::InternalState<class C4ToolsDlg>(toolsDlg), hDialog(0),
		hbmBrush(0), hbmBrush2(0),
		hbmLine(0), hbmLine2(0),
		hbmRect(0), hbmRect2(0),
		hbmFill(0), hbmFill2(0),
		hbmPicker(0), hbmPicker2(0),
		hbmIFT(0),
		hbmNoIFT(0),
		hbmDynamic(0),
		hbmStatic(0),
		hbmExact(0)
	{
		pGLCtx = NULL;
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
		if (!hbmIFT) hbmIFT=(HBITMAP)LoadBitmapW(instance,MAKEINTRESOURCEW(IDB_IFT));
		if (!hbmNoIFT) hbmNoIFT=(HBITMAP)LoadBitmapW(instance,MAKEINTRESOURCEW(IDB_NOIFT));
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
		if (hbmBrush) DeleteObject(hbmBrush);
		if (hbmLine) DeleteObject(hbmLine);
		if (hbmRect) DeleteObject(hbmRect);
		if (hbmFill) DeleteObject(hbmFill);
		if (hbmIFT) DeleteObject(hbmIFT);
		if (hbmNoIFT) DeleteObject(hbmNoIFT);
#ifdef USE_GL
		if (pGLCtx)
		{
			delete pGLCtx;
			pGLCtx = NULL;
		}
#endif
		if (hDialog) DestroyWindow(hDialog); hDialog=NULL;
	}

	void Default()
	{
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
		StoreWindowPosition(hDlg, "Property", Config.GetSubkeyPath("Console"), false);
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
			Console.ToolsDlg.SetLandscapeMode(C4LSC_Dynamic);
			return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_BUTTONMODESTATIC:
			Console.ToolsDlg.SetLandscapeMode(C4LSC_Static);
			return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_BUTTONMODEEXACT:
			Console.ToolsDlg.SetLandscapeMode(C4LSC_Exact);
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
		case IDC_BUTTONIFT:
			Console.ToolsDlg.SetIFT(true);
			return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_BUTTONNOIFT:
			Console.ToolsDlg.SetIFT(false);
			return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_COMBOMATERIAL:
			switch (HIWORD(wParam))
			{
			case CBN_SELCHANGE:
			{
				wchar_t str[100];
				int32_t cursel = SendDlgItemMessage(hDlg,IDC_COMBOMATERIAL,CB_GETCURSEL,0,0);
				SendDlgItemMessage(hDlg,IDC_COMBOMATERIAL,CB_GETLBTEXT,cursel,(LPARAM)str);
				Console.ToolsDlg.SetMaterial(StdStrBuf(str).getData());
				break;
			}
			}
			return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_COMBOTEXTURE:
			switch (HIWORD(wParam))
			{
			case CBN_SELCHANGE:
			{
				wchar_t str[100];
				int32_t cursel = SendDlgItemMessage(hDlg,IDC_COMBOTEXTURE,CB_GETCURSEL,0,0);
				SendDlgItemMessage(hDlg,IDC_COMBOTEXTURE,CB_GETLBTEXT,cursel,(LPARAM)str);
				Console.ToolsDlg.SetTexture(StdStrBuf(str).getData());
				break;
			}
			}
			return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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
		StoreWindowPosition(hDlg, "Property", Config.GetSubkeyPath("Console"), false);
		break;
		//------------------------------------------------------------------------------------------------
	case WM_INITDIALOG:
		SendMessage(hDlg,DM_SETDEFID,(WPARAM)IDOK,(LPARAM)0);
		return true;
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

CStdWindow* C4ConsoleGUI::CreateConsoleWindow(CStdApp *application)
{
	hWindow = CreateDialog(application->GetInstance(), MAKEINTRESOURCE(IDD_CONSOLE), NULL, ConsoleDlgProc);
	if (!hWindow)
	{
		wchar_t * lpMsgBuf;
		FormatMessage(
		  FORMAT_MESSAGE_ALLOCATE_BUFFER |
		  FORMAT_MESSAGE_FROM_SYSTEM |
		  FORMAT_MESSAGE_IGNORE_INSERTS,
		  NULL,
		  GetLastError(),
		  0,
		  (wchar_t *)&lpMsgBuf, // really.
		  0,
		  NULL);
		Log(FormatString("Error creating dialog window: %s", StdStrBuf(lpMsgBuf).getData()).getData());
		// Free the buffer.
		LocalFree(lpMsgBuf);
		return NULL;
	}
	// Restore window position
	RestoreWindowPosition(hWindow, "Main", Config.GetSubkeyPath("Console"));
	// Set icon
	SendMessage(hWindow,WM_SETICON,ICON_BIG,(LPARAM)LoadIcon(application->GetInstance(),MAKEINTRESOURCE(IDI_00_C4X)));
	SendMessage(hWindow,WM_SETICON,ICON_SMALL,(LPARAM)LoadIcon(application->GetInstance(),MAKEINTRESOURCE(IDI_00_C4X)));
	// Set text
	SetCaption(LoadResStr("IDS_CNS_CONSOLE"));
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
	hRenderWindow = hWindow;
	// Success
	return this;
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
	len2 = len+Min<int32_t>(messageW.getSize()/sizeof(wchar_t)+2, 5000);
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
	case CONSOLE_ScriptCounter:
		dialog_item = IDC_STATICSCRIPT;
		break;
	case CONSOLE_TimeFPS:
		dialog_item = IDC_STATICTIME;
		break;
	default:
		assert(false);
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
	MessageBoxW(NULL, strMessage.GetWideChar(), ADDL(C4ENGINECAPTION), MB_ICONINFORMATION | MB_TASKMODAL);
}

bool C4ConsoleGUI::FileSelect(StdStrBuf *sFilename, const char * szFilter, DWORD dwFlags, bool fSave)
{
	enum { ArbitraryMaximumLength = 4096 };
	wchar_t buffer[ArbitraryMaximumLength];
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
	int len = MultiByteToWideChar(CP_UTF8, 0, szFilter, n, NULL, 0);
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
	len = WideCharToMultiByte(CP_UTF8, 0, buffer, ArbitraryMaximumLength, NULL, 0, 0, 0);
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

void C4ConsoleGUI::AddNetMenuItemForPlayer(int32_t index, StdStrBuf &text)
{
	AddMenuItem(this, GetSubMenu(GetMenu(hWindow),state->MenuIndexNet), IDM_NET_CLIENT1+Game.Clients.getLocalID(), text.getData(), true);
}

void C4ConsoleGUI::ClearViewportMenu()
{
	HMENU hMenu = GetSubMenu(GetMenu(hWindow),state->MenuIndexViewport);
	while (DeleteMenu(hMenu,1,MF_BYPOSITION));
}

void C4ConsoleGUI::ToolsDlgClose()
{
	::ClearDlg(Console.ToolsDlg.state->hDialog);
}

void C4ConsoleGUI::ToolsDlgSetTexture(class C4ToolsDlg *dlg, const char *texture)
{
	SendDlgItemMessage(dlg->state->hDialog,IDC_COMBOTEXTURE,CB_SELECTSTRING,0,GetWideLPARAM(texture));
}

void C4ConsoleGUI::ToolsDlgSetMaterial(class C4ToolsDlg *dlg, const char *material)
{
	SendDlgItemMessage(dlg->state->hDialog,IDC_COMBOMATERIAL,CB_SELECTSTRING,0,GetWideLPARAM(material));
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

static void SetComboItems(HWND hCombo, std::list<char*> &items)
{
	for (std::list<char*>::iterator it = items.begin(); it != items.end(); it++)
	{
		char *item = *it;
		if (!item)
			SendMessage(hCombo,CB_INSERTSTRING,0,(LPARAM)L"----------");
		else
			SendMessage(hCombo,CB_ADDSTRING,0,GetWideLPARAM(*it));
	}
}

void C4ConsoleGUI::PropertyDlgUpdate(C4ObjectList &rSelection)
{
	HWND hDialog = state->hPropertyDlg;
	if (!hDialog) return;
	int iLine = SendDlgItemMessage(hDialog,IDC_EDITOUTPUT,EM_GETFIRSTVISIBLELINE,(WPARAM)0,(LPARAM)0);
	SetDlgItemTextW(hDialog,IDC_EDITOUTPUT,rSelection.GetDataString().GetWideChar());
	SendDlgItemMessage(hDialog,IDC_EDITOUTPUT,EM_LINESCROLL,(WPARAM)0,(LPARAM)iLine);
	UpdateWindow(GetDlgItem(hDialog,IDC_EDITOUTPUT));

	if (PropertyDlgObject == rSelection.GetObject()) return;
	PropertyDlgObject = rSelection.GetObject();
	
	std::list<char *> functions = ::ScriptEngine.GetFunctionNames(PropertyDlgObject ? &PropertyDlgObject->Def->Script : 0);
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

void C4ConsoleGUI::SetInputFunctions(std::list<char*> &functions)
{
	SetComboItems(GetDlgItem(hWindow,IDC_COMBOINPUT), functions);
}

void C4ConsoleGUI::ClearPlayerMenu()
{
	if (!Active) return;
	HMENU hMenu = GetSubMenu(GetMenu(hWindow),state->MenuIndexPlayer);
	while (DeleteMenu(hMenu,1,MF_BYPOSITION));
}

void C4ConsoleGUI::ClearInput()
{
	HWND hCombo = GetDlgItem(hWindow,IDC_COMBOINPUT);
	// Clear
	SendMessage(hCombo,CB_RESETCONTENT,0,0);
}

/*
void C4ConsoleGUI::ClearPropertyDlg(C4PropertyDlg *dlg)
{
	if (dlg->state->hDialog) DestroyWindow(PropertyDlg.hDialog); PropertyDlg.hDialog=NULL;
}
*/

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
	// Load bitmaps if necessary
	dlg->state->LoadBitmaps(Application.GetInstance());
	// create target ctx for OpenGL rendering
#ifdef USE_GL
	if (lpDDraw && !dlg->state->pGLCtx)
		dlg->state->pGLCtx = lpDDraw->CreateContext(GetDlgItem(dlg->state->hDialog,IDC_PREVIEW), &Application);
#endif
	// Show window
	RestoreWindowPosition(dlg->state->hDialog, "Property", Config.GetSubkeyPath("Console"));
	SetWindowPos(dlg->state->hDialog,Console.hWindow,0,0,0,0,SWP_NOSIZE | SWP_NOMOVE);
	ShowWindow(dlg->state->hDialog,SW_SHOWNOACTIVATE);
	return true;
}

void C4ConsoleGUI::ToolsDlgInitMaterialCtrls(class C4ToolsDlg *dlg)
{
	SendDlgItemMessage(dlg->state->hDialog,IDC_COMBOMATERIAL,CB_ADDSTRING,0,GetWideLPARAM(C4TLS_MatSky));
	for (int32_t cnt=0; cnt< ::MaterialMap.Num; cnt++)
		SendDlgItemMessage(dlg->state->hDialog,IDC_COMBOMATERIAL,CB_ADDSTRING,0,GetWideLPARAM(::MaterialMap.Map[cnt].Name));
	SendDlgItemMessage(dlg->state->hDialog,IDC_COMBOMATERIAL,CB_SELECTSTRING,0,GetWideLPARAM(dlg->Material));
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
	SendDlgItemMessage(dlg->state->hDialog,IDC_COMBOTEXTURE,CB_SELECTSTRING,0,GetWideLPARAM(texture));
}

void C4ConsoleGUI::ToolsDlgSelectMaterial(C4ToolsDlg *dlg, const char *material)
{
	SendDlgItemMessage(dlg->state->hDialog,IDC_COMBOMATERIAL,CB_SELECTSTRING,0,GetWideLPARAM(material));
}

void C4ToolsDlg::UpdateTextures()
{
	// Refill dlg
	SendDlgItemMessage(state->hDialog,IDC_COMBOTEXTURE,CB_RESETCONTENT,0,(LPARAM)0);
	// bottom-most: any invalid textures
	bool fAnyEntry = false; int32_t cnt; const char *szTexture;
	if (::Landscape.Mode!=C4LSC_Exact)
		for (cnt=0; (szTexture=::TextureMap.GetTexture(cnt)); cnt++)
		{
			if (!::TextureMap.GetIndex(Material, szTexture, false))
			{
				fAnyEntry = true;
				SendDlgItemMessage(state->hDialog,IDC_COMBOTEXTURE,CB_INSERTSTRING,0,GetWideLPARAM(szTexture));
			}
		}
	// separator
	if (fAnyEntry)
	{
		SendDlgItemMessage(state->hDialog,IDC_COMBOTEXTURE,CB_INSERTSTRING,0,(LPARAM)L"-------");
	}

	// atop: valid textures
	for (cnt=0; (szTexture=::TextureMap.GetTexture(cnt)); cnt++)
	{
		// Current material-texture valid? Always valid for exact mode
		if (::TextureMap.GetIndex(Material,szTexture,false) || ::Landscape.Mode==C4LSC_Exact)
		{
			SendDlgItemMessage(state->hDialog,IDC_COMBOTEXTURE,CB_INSERTSTRING,0,GetWideLPARAM(szTexture));
		}
	}
	// reselect current
	SendDlgItemMessage(state->hDialog,IDC_COMBOTEXTURE,CB_SELECTSTRING,0,GetWideLPARAM(Texture));
}

void C4ToolsDlg::NeedPreviewUpdate()
{
	if (!state->hDialog) return;

	SURFACE sfcPreview;
	int32_t iPrvWdt,iPrvHgt;
	RECT rect;

	GetClientRect(GetDlgItem(state->hDialog,IDC_PREVIEW),&rect);

	iPrvWdt=rect.right-rect.left;
	iPrvHgt=rect.bottom-rect.top;

	if (!(sfcPreview=new CSurface(iPrvWdt,iPrvHgt))) return;

	// fill bg
	lpDDraw->DrawBoxDw(sfcPreview,0,0,iPrvWdt-1,iPrvHgt-1,C4RGB(0x80,0x80,0x80));
	BYTE bCol = 0;
	CPattern Pattern;
	// Sky material: sky as pattern only
	if (SEqual(Material,C4TLS_MatSky))
	{
		Pattern.Set(::Landscape.Sky.Surface, 0);
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
		lpDDraw->DrawPatternedCircle( sfcPreview,
		                              iPrvWdt/2,iPrvHgt/2,
		                              Grade,
		                              bCol, Pattern, *::Landscape.GetPal());

	//Application.DDraw->AttachPrimaryPalette(sfcPreview);

#ifdef USE_DIRECTX
	if (pD3D)
		pD3D->BlitSurface2Window( sfcPreview,
		                          0,0,iPrvWdt,iPrvHgt,
		                          GetDlgItem(state->hDialog,IDC_PREVIEW),
		                          rect.left,rect.top,rect.right,rect.bottom);
#endif
#ifdef USE_GL
	// FIXME: This activates the wrong GL context. To avoid breaking the main window display,
	// FIXME: it has been disabled for the moment
    //if (pGLCtx->Select())
	//{
	//	pGL->Blit(sfcPreview, 0,0,(float)iPrvWdt,(float)iPrvHgt, Application.pWindow->pSurface, rect.left,rect.top, iPrvWdt,iPrvHgt);
	//	Application.pWindow->pSurface->PageFlip();
	//}
#endif
	delete sfcPreview;
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
	SetFocus(GetDlgItem(state->hDialog,IDC_COMBOMATERIAL));
	SendDlgItemMessage(state->hDialog,IDC_COMBOMATERIAL,CB_SHOWDROPDOWN,true,0);
	return true;
}

bool C4ToolsDlg::PopTextures()
{
	if (!state->hDialog) return false;
	SetFocus(GetDlgItem(state->hDialog,IDC_COMBOTEXTURE));
	SendDlgItemMessage(state->hDialog,IDC_COMBOTEXTURE,CB_SHOWDROPDOWN,true,0);
	return true;
}

void C4ToolsDlg::UpdateIFTControls()
{
	if (!state->hDialog)
		return;
	SendDlgItemMessage(state->hDialog,IDC_BUTTONNOIFT,BM_SETSTATE,(ModeIFT==0),0);
	UpdateWindow(GetDlgItem(state->hDialog,IDC_BUTTONNOIFT));
	SendDlgItemMessage(state->hDialog,IDC_BUTTONIFT,BM_SETSTATE,(ModeIFT==1),0);
	UpdateWindow(GetDlgItem(state->hDialog,IDC_BUTTONIFT));
}

void C4ToolsDlg::UpdateLandscapeModeCtrls()
{
	int32_t iMode = ::Landscape.Mode;
	// Dynamic: enable only if dynamic anyway
	SendDlgItemMessage(state->hDialog,IDC_BUTTONMODEDYNAMIC,BM_SETSTATE,(iMode==C4LSC_Dynamic),0);
	EnableWindow(GetDlgItem(state->hDialog,IDC_BUTTONMODEDYNAMIC),(iMode==C4LSC_Dynamic));
	UpdateWindow(GetDlgItem(state->hDialog,IDC_BUTTONMODEDYNAMIC));
	// Static: enable only if map available
	SendDlgItemMessage(state->hDialog,IDC_BUTTONMODESTATIC,BM_SETSTATE,(iMode==C4LSC_Static),0);
	EnableWindow(GetDlgItem(state->hDialog,IDC_BUTTONMODESTATIC),(::Landscape.Map!=NULL));
	UpdateWindow(GetDlgItem(state->hDialog,IDC_BUTTONMODESTATIC));
	// Exact: enable always
	SendDlgItemMessage(state->hDialog,IDC_BUTTONMODEEXACT,BM_SETSTATE,(iMode==C4LSC_Exact),0);
	UpdateWindow(GetDlgItem(state->hDialog,IDC_BUTTONMODEEXACT));
	// Set dialog caption
	SetWindowTextW(state->hDialog,LoadResStrW(iMode==C4LSC_Dynamic ? "IDS_DLG_DYNAMIC" : iMode==C4LSC_Static ? "IDS_DLG_STATIC" : "IDS_DLG_EXACT"));
}


void C4ToolsDlg::EnableControls()
{
	HWND hDialog = state->hDialog;
	int32_t iLandscapeMode=::Landscape.Mode;
	// Set bitmap buttons
	SendDlgItemMessage(hDialog,IDC_BUTTONBRUSH,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)((iLandscapeMode>=C4LSC_Static) ? state->hbmBrush : state->hbmBrush2));
	SendDlgItemMessage(hDialog,IDC_BUTTONLINE,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)((iLandscapeMode>=C4LSC_Static) ? state->hbmLine : state->hbmLine2));
	SendDlgItemMessage(hDialog,IDC_BUTTONRECT,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)((iLandscapeMode>=C4LSC_Static) ? state->hbmRect : state->hbmRect2));
	SendDlgItemMessage(hDialog,IDC_BUTTONFILL,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)((iLandscapeMode>=C4LSC_Exact) ? state->hbmFill : state->hbmFill2));
	SendDlgItemMessage(hDialog,IDC_BUTTONPICKER,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)((iLandscapeMode>=C4LSC_Static) ? state->hbmPicker : state->hbmPicker2));
	SendDlgItemMessage(hDialog,IDC_BUTTONIFT,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)state->hbmIFT);
	SendDlgItemMessage(hDialog,IDC_BUTTONNOIFT,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)state->hbmNoIFT);
	SendDlgItemMessage(hDialog,IDC_BUTTONMODEDYNAMIC,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)state->hbmDynamic);
	SendDlgItemMessage(hDialog,IDC_BUTTONMODESTATIC,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)state->hbmStatic);
	SendDlgItemMessage(hDialog,IDC_BUTTONMODEEXACT,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)state->hbmExact);
	// Enable drawing controls
	EnableWindow(GetDlgItem(hDialog,IDC_BUTTONBRUSH),(iLandscapeMode>=C4LSC_Static));
	EnableWindow(GetDlgItem(hDialog,IDC_BUTTONLINE),(iLandscapeMode>=C4LSC_Static));
	EnableWindow(GetDlgItem(hDialog,IDC_BUTTONRECT),(iLandscapeMode>=C4LSC_Static));
	EnableWindow(GetDlgItem(hDialog,IDC_BUTTONFILL),(iLandscapeMode>=C4LSC_Exact));
	EnableWindow(GetDlgItem(hDialog,IDC_BUTTONPICKER),(iLandscapeMode>=C4LSC_Static));
	EnableWindow(GetDlgItem(hDialog,IDC_BUTTONIFT),(iLandscapeMode>=C4LSC_Static));
	EnableWindow(GetDlgItem(hDialog,IDC_BUTTONNOIFT),(iLandscapeMode>=C4LSC_Static));
	EnableWindow(GetDlgItem(hDialog,IDC_COMBOMATERIAL),(iLandscapeMode>=C4LSC_Static));
	EnableWindow(GetDlgItem(hDialog,IDC_COMBOTEXTURE),(iLandscapeMode>=C4LSC_Static) && !SEqual(Material,C4TLS_MatSky));
	EnableWindow(GetDlgItem(hDialog,IDC_STATICMATERIAL),(iLandscapeMode>=C4LSC_Static));
	EnableWindow(GetDlgItem(hDialog,IDC_STATICTEXTURE),(iLandscapeMode>=C4LSC_Static) && !SEqual(Material,C4TLS_MatSky));
	EnableWindow(GetDlgItem(hDialog,IDC_SLIDERGRADE),(iLandscapeMode>=C4LSC_Static));
	EnableWindow(GetDlgItem(hDialog,IDC_PREVIEW),(iLandscapeMode>=C4LSC_Static));

	NeedPreviewUpdate();
}

#include "C4ConsoleGUICommon.h"
