/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2012, Julius Michaelis
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

/* Functions for displaying a settings dialogue to users when the graphics system failed */

#include <C4Include.h>

#ifdef _WIN32

#include <resource.h>
#include <C4Version.h>
#include <C4Application.h>
#include <C4windowswrapper.h>
#include <C4GfxErrorDlg.h>

static int edittext_toi(HWND hWnd, int field)
{
	WCHAR buf[512]; buf[511] = 0;
	GetDlgItemTextW(hWnd,field,buf,509);
	StdStrBuf data(buf);
	const char* bufp = data.getData();
	while(*bufp == ' ') ++bufp;
	int res = strtol(bufp, NULL, 0);
	if(errno != ERANGE)
		return res;
	return -1;
}

static INT_PTR CALLBACK GfxErrProcedure(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch(Msg)
    {
	case WM_INITDIALOG:
		// Set Icon, Caption and static Texts
		SendMessage(hWnd,WM_SETICON,ICON_BIG,(LPARAM)LoadIcon(Application.GetInstance(),MAKEINTRESOURCE(IDI_00_C4X)));
		SendMessage(hWnd,WM_SETICON,ICON_SMALL,(LPARAM)LoadIcon(Application.GetInstance(),MAKEINTRESOURCE(IDI_00_C4X)));
		SetWindowTextW(hWnd, GetWideChar(C4ENGINECAPTION));
		SetDlgItemTextW(hWnd,IDC_GFXERR_MSG  ,GetWideChar(LoadResStr("IDS_MSG_GFXERR_TXT")));
		SetDlgItemTextW(hWnd,IDC_GFXERR_RES  ,GetWideChar(LoadResStr("IDS_CTL_RESOLUTION")));
		SetDlgItemTextW(hWnd,IDC_GFXERR_FSCRN,GetWideChar(LoadResStr("IDS_MSG_FULLSCREEN")));
		SetDlgItemTextW(hWnd,IDCANCEL        ,GetWideChar(LoadResStr("IDS_DLG_EXIT")));
		SetDlgItemTextW(hWnd,IDOK            ,GetWideChar(LoadResStr("IDS_BTN_RESTART")));
		// Set Options
		SendMessage(GetDlgItem(hWnd, IDC_GFXERR_FSCRN), BM_SETCHECK, Config.Graphics.Windowed?0:1, 0);
		SetDlgItemTextW(hWnd,IDC_GFXERR_XINP ,FormatString("%d",Config.Graphics.ResX).GetWideChar());
		SetDlgItemTextW(hWnd,IDC_GFXERR_YINP ,FormatString("%d",Config.Graphics.ResY).GetWideChar());
		return TRUE;

	case WM_DESTROY:
		EndDialog(hWnd,1);
		return TRUE;

	case WM_COMMAND:
	{
		switch(LOWORD(wParam))
		{
		case IDCANCEL:
			EndDialog(hWnd,1);
			return TRUE;
		case IDC_GFXERR_FSCRN:
		case IDC_GFXERR_XINP:
		case IDC_GFXERR_YINP:
		{ // Handle Resolution values
			if(SendMessage(GetDlgItem(hWnd, IDC_GFXERR_FSCRN),BM_GETCHECK,0,0) == BST_CHECKED)
			{
				int resx = edittext_toi(hWnd,IDC_GFXERR_XINP);
				int resy = edittext_toi(hWnd,IDC_GFXERR_YINP);
				if(resx < 1 || resy < 1) // res. will be 0 if the user supplies an invalid value
					SetDlgItemTextW(hWnd,IDC_GFXERR_INVAL,GetWideChar(LoadResStr("IDS_MSG_GFXERR_RESINVAL")));
				else
				{
					// Check if res is in list of supportet
					bool found = false;
					int32_t idx = 0, iXRes, iYRes, iBitDepth;
					while (Application.GetIndexedDisplayMode(idx++, &iXRes, &iYRes, &iBitDepth, NULL, Config.Graphics.Monitor))
						if (iBitDepth == Config.Graphics.BitDepth)
							if(iXRes == resx && iYRes == resy)
							{
								found = true;
								break;
							}
					SetDlgItemTextW(hWnd,IDC_GFXERR_INVAL,found?L"":GetWideChar(LoadResStr("IDS_MSG_GFXERR_RESNOTFOUND")));
				}
			}
			else
				SetDlgItemTextW(hWnd,IDC_GFXERR_INVAL,L"");
			return TRUE;
		}
		case IDOK:
		{
			int resx = edittext_toi(hWnd,IDC_GFXERR_XINP);
			int resy = edittext_toi(hWnd,IDC_GFXERR_YINP);
			if(resx < 1 || resy < 1) break;
			Config.Graphics.Windowed = !(SendMessage(GetDlgItem(hWnd, IDC_GFXERR_FSCRN),BM_GETCHECK,0,0) == BST_CHECKED);
			Config.Graphics.ResX = resx;
			Config.Graphics.ResY = resy;
			Config.Save();
			TCHAR selfpath[4096];
			GetModuleFileName(NULL, selfpath, 4096);
			STARTUPINFOW siStartupInfo;
			PROCESS_INFORMATION piProcessInfo;
			memset(&siStartupInfo, 0, sizeof(siStartupInfo));
			memset(&piProcessInfo, 0, sizeof(piProcessInfo));
			siStartupInfo.cb = sizeof(siStartupInfo);
			CreateProcessW(selfpath, L"",
				NULL, NULL, FALSE, 0, NULL, Config.General.ExePath.GetWideChar(), &siStartupInfo, &piProcessInfo);
			EndDialog(hWnd,2);
			return TRUE;
		}
		}
	}
    }
    return FALSE;
}

void ShowGfxErrorDialog()
{
	int ret = DialogBox(Application.GetInstance(), MAKEINTRESOURCE(IDD_GFXERROR), NULL, GfxErrProcedure);
	if (ret == 0 || ret == -1)
	{
		LPVOID lpMsgBuf;
		DWORD err = GetLastError();
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			err,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &lpMsgBuf,
			0, NULL );
		LogF("Error in GfxErrorDlg: %d - %s", err, StdStrBuf((wchar_t*)lpMsgBuf).getData());
		LocalFree(lpMsgBuf);
	}
}

#else
void ShowGfxErrorDialog(){} // To be implemented? It's mainly a windows (users') problem.
#endif
