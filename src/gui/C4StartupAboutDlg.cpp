/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2010-2014, The OpenClonk Team and contributors
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
// About/credits screen

#include <C4Include.h>
#include <C4StartupAboutDlg.h>
#include <C4UpdateDlg.h>
#include <C4GraphicsResource.h>
#include <C4Version.h>

#include <C4StartupMainDlg.h>

// ------------------------------------------------
// --- C4StartupAboutDlg

C4StartupAboutDlg::C4StartupAboutDlg() : C4StartupDlg(LoadResStr("IDS_DLG_ABOUT"))
{
	// ctor
	UpdateSize();

	CStdFont &rUseFont = ::GraphicsResource.TextFont;
	C4Rect rcClient = GetContainedClientRect();

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
}

void C4StartupAboutDlg::DoBack()
{
	C4Startup::Get()->SwitchDialog(C4Startup::SDID_Main);
}

void C4StartupAboutDlg::DrawElement(C4TargetFacet &cgo)
{
	C4Startup::Get()->Graphics.fctAboutBG.Draw(cgo, true, 0, 0, true);
}

#ifdef WITH_AUTOMATIC_UPDATE
void C4StartupAboutDlg::OnUpdateBtn(C4GUI::Control *btn)
{
	C4UpdateDlg::CheckForUpdates(GetScreen());
}
#endif
