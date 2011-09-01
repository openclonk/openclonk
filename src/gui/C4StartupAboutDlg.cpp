/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2007  Matthes Bender
 * Copyright (c) 2007  Sven Eberhardt
 * Copyright (c) 2010  Nicolas Hake
 * Copyright (c) 2010  Armin Burgmeier
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
// About/credits screen

#include <C4Include.h>
#include <C4StartupAboutDlg.h>
#include <C4UpdateDlg.h>
#include <C4GraphicsResource.h>
#include <C4Version.h>

#include <C4StartupMainDlg.h>

// ------------------------------------------------
// --- C4StartupAboutDlg

C4StartupAboutDlg::C4StartupAboutDlg() : C4StartupDlg("")
{
	// ctor
	UpdateSize();

	// key bindings: No longer back on any key
	pKeyBack = NULL;
	//C4CustomKey::CodeList keys;
	//keys.push_back(C4KeyCodeEx(KEY_Any)); keys.push_back(C4KeyCodeEx(KEY_JOY_AnyButton));
	//pKeyBack = new C4KeyBinding(keys, "StartupAboutBack", KEYSCOPE_Gui,
	//  new C4GUI::DlgKeyCB<C4StartupAboutDlg>(*this, &C4StartupAboutDlg::KeyBack), C4CustomKey::PRIO_Dlg);

	// version and registration info in topright corner
	C4Rect rcClient = GetContainedClientRect();
	StdStrBuf sVersion; sVersion.Format(LoadResStr("IDS_DLG_VERSION"), C4VERSION);
	CStdFont &rUseFont = ::GraphicsResource.TextFont;
	int32_t iInfoWdt = Min<int32_t>(rcClient.Wdt/2, rUseFont.GetTextWidth("General info text width")*2);
	C4GUI::ComponentAligner caInfo(C4Rect(rcClient.x + rcClient.Wdt - iInfoWdt, rcClient.y, iInfoWdt, rcClient.Hgt/8), 0,0, false);
	AddElement(new C4GUI::Label(sVersion.getData(), caInfo.GetGridCell(0,1,0,4), ARight));
	StdStrBuf sRegStr, sKeyFile;
	Application.Add(this);
	OnSec1Timer();

	// bottom line buttons and copyright messages
	C4GUI::ComponentAligner caMain(rcClient, 0,0, true);
	C4GUI::ComponentAligner caButtons(caMain.GetFromBottom(caMain.GetHeight()*1/8), 0,0, false);
	C4GUI::CallbackButton<C4StartupAboutDlg> *btn;
#ifdef HAVE_FMOD
	AddElement(new C4GUI::Label("Using FMOD Sound System, copyright (c) Firelight Technologies Pty, Ltd., 1994-2010.",
		caMain.GetFromBottom(rUseFont.GetLineHeight())));
#endif
	int32_t iButtonWidth = caButtons.GetInnerWidth() / 4;
	AddElement(btn = new C4GUI::CallbackButton<C4StartupAboutDlg>(LoadResStr("IDS_BTN_BACK"), caButtons.GetGridCell(0,3,0,1,iButtonWidth,C4GUI_ButtonHgt,true), &C4StartupAboutDlg::OnBackBtn));
	btn->SetToolTip(LoadResStr("IDS_DLGTIP_BACKMAIN"));
#ifdef WITH_AUTOMATIC_UPDATE
	AddElement(btn = new C4GUI::CallbackButton<C4StartupAboutDlg>(LoadResStr("IDS_BTN_CHECKFORUPDATES"), caButtons.GetGridCell(2,3,0,1,iButtonWidth,C4GUI_ButtonHgt,true), &C4StartupAboutDlg::OnUpdateBtn));
	btn->SetToolTip(LoadResStr("IDS_DESC_CHECKONLINEFORNEWVERSIONS"));
#endif
}

C4StartupAboutDlg::~C4StartupAboutDlg()
{
	Application.Remove(this);
	delete pKeyBack;
}

void C4StartupAboutDlg::DoBack()
{
	C4Startup::Get()->SwitchDialog(C4Startup::SDID_Main);
}

void C4StartupAboutDlg::OnSec1Timer()
{
}

void C4StartupAboutDlg::DrawElement(C4TargetFacet &cgo)
{
	// draw background - do not use bg drawing proc, because it stretches
	// pre-clear background instead to prevent blinking borders
	if (!IsFading()) lpDDraw->FillBG();
	C4Startup::Get()->Graphics.fctAboutBG.Draw(cgo, false);
}

void C4StartupAboutDlg::MouseInput(C4GUI::CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam)
{
	// back on any mouse button? Better not, because mouse input is required
	/*if (iButton == C4MC_Button_LeftDown || iButton == C4MC_Button_RightDown || iButton == C4MC_Button_MiddleDown)
	  DoBack();
	else*/
	// otherwise, inherited for tooltips
	C4StartupDlg::MouseInput(rMouse, iButton, iX, iY, dwKeyParam);
}

#ifdef WITH_AUTOMATIC_UPDATE
void C4StartupAboutDlg::OnUpdateBtn(C4GUI::Control *btn)
{
	C4UpdateDlg::CheckForUpdates(GetScreen());
}
#endif
