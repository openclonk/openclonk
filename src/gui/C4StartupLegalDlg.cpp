/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2010-2020, The OpenClonk Team and contributors
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

#include "C4Include.h"
#include "gui/C4StartupLegalDlg.h"

#include "C4Version.h"
#include "graphics/C4GraphicsResource.h"
#include "game/C4Application.h"
#include "C4Licenses.h"


// Sorted by commit count this release, e.g.: git shortlog -s v7.0.. | sort -rn
// Stuff from the milestone project sorted in-between as those commits usually end up squashed.

// ------------------------------------------------
// --- C4StartupLegalDlg

C4StartupLegalDlg::C4StartupLegalDlg() : C4StartupDlg(LoadResStr("IDS_DLG_DISPLAYLEGALNOTICE"))
{
	// ctor
	UpdateSize();

	C4Rect rcClient = GetContainedClientRect();

	// bottom line buttons and copyright messages
	C4GUI::ComponentAligner caMain(rcClient, 0,0, true);
	C4GUI::ComponentAligner caButtons(caMain.GetFromBottom(caMain.GetHeight()*1/8), 0,0, false);
	C4GUI::CallbackButton<C4StartupLegalDlg> *btn;
	int32_t iButtonWidth = caButtons.GetInnerWidth() / 4;
	AddElement(btn = new C4GUI::CallbackButton<C4StartupLegalDlg>(LoadResStr("IDS_BTN_BACK"), caButtons.GetGridCell(0,3,0,1,iButtonWidth,C4GUI_ButtonHgt,true), &C4StartupLegalDlg::OnBackBtn));
	btn->SetToolTip(LoadResStr("IDS_DLGTIP_BACKMAIN"));

	C4Rect rect = caMain.GetFromTop(caMain.GetHeight());
	auto textbox = new C4GUI::TextWindow(rect, 0, 0, 0, 100, 4096, "", true, nullptr, 0, true);
	AddElement(textbox);
	textbox->SetDecoration(false, false, nullptr, true);

	for(const auto& license : OCLicenses) {
		textbox->AddTextLine(license.name.c_str(), &::GraphicsResource.TitleFont, C4GUI_NotifyFontClr, false, true);
		textbox->AddTextLine(license.content.c_str(), &::GraphicsResource.TextFont, C4GUI_MessageFontClr, false, true);
		textbox->AddTextLine(" ", &::GraphicsResource.TitleFont, C4GUI_MessageFontClr, false, true);
	}

	textbox->UpdateHeight();
}

C4StartupLegalDlg::~C4StartupLegalDlg() = default;

void C4StartupLegalDlg::DoBack()
{
	C4Startup::Get()->SwitchDialog(C4Startup::SDID_About);
}
