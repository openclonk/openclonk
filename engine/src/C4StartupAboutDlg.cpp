/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2007  Sven Eberhardt
 * Copyright (c) 2007  Matthes Bender
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

#ifndef BIG_C4INCLUDE
#include <C4StartupMainDlg.h>
#endif

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
	//	new C4GUI::DlgKeyCB<C4StartupAboutDlg>(*this, &C4StartupAboutDlg::KeyBack), C4CustomKey::PRIO_Dlg);

	// version and registration info in topright corner
	C4Rect rcClient = GetContainedClientRect();
	StdStrBuf sVersion; sVersion.Format(LoadResStr("IDS_DLG_VERSION"), C4VERSION);
	CStdFont &rUseFont = C4GUI::GetRes()->TextFont;
	int32_t iInfoWdt = Min<int32_t>(rcClient.Wdt/2, rUseFont.GetTextWidth("General info text width")*2);
	C4GUI::ComponentAligner caInfo(C4Rect(rcClient.x + rcClient.Wdt - iInfoWdt, rcClient.y, iInfoWdt, rcClient.Hgt/8), 0,0, false);
	AddElement(new C4GUI::Label(sVersion.getData(), caInfo.GetGridCell(0,1,0,4), ARight));
	StdStrBuf sRegStr, sKeyFile;
	if (Config.Registered())
		{
		StdStrBuf sRegName, sFirstName, sLastName, sNick;
		sFirstName.Copy(Config.GetRegistrationData("FirstName"));
		sLastName.Copy(Config.GetRegistrationData("LastName"));
		sNick.Copy(Config.GetRegistrationData("Nick"));
		sRegName.Format("%s %s (%s)", sFirstName.getData(), sLastName.getData(), sNick.getData());
		sRegStr.Format(LoadResStr("IDS_PRC_REG"), sRegName.getData());
		sKeyFile.Format(LoadResStr("IDS_CTL_KEYFILE")); sKeyFile+=" "; sKeyFile+=Config.GetKeyFilename();
		C4GUI::Label *pLbl;
		AddElement(pLbl = new C4GUI::Label(sRegStr.getData(), caInfo.GetGridCell(0,1,1,4), ARight));
		pLbl->SetToolTip(sKeyFile.getData());
		AddElement(pLbl = new C4GUI::Label(FormatString("%s %s", LoadResStr("IDS_CTL_CUID"), Config.GetRegistrationData("Cuid")).getData(), caInfo.GetGridCell(0,1,2,4), ARight));
		pLbl->SetToolTip(sKeyFile.getData());
		AddElement(pWebCodeLbl = new C4GUI::Label("", caInfo.GetGridCell(0,1,3,4), ARight));
		pWebCodeLbl ->SetToolTip(sKeyFile.getData());
		}
	else
		{
		AddElement(new C4GUI::Label(LoadResStr("IDS_CTL_UNREGISTERED"), caInfo.GetGridCell(0,1,1,4), ARight));
		AddElement(new C4GUI::Label(Config.GetRegistrationError(), caInfo.GetGridCell(0,1,2,4), ARight));
		pWebCodeLbl = NULL;
		}
	// webcode-display timer
	iWebCodeTimer = C4AboutWebCodeShowTime + 1;
	Application.Add(this);
	OnSec1Timer();

	// bottom line buttons
	C4GUI::ComponentAligner caMain(rcClient, 0,0, true);
	C4GUI::ComponentAligner caButtons(caMain.GetFromBottom(caMain.GetHeight()*1/8), 0,0, false);
	C4GUI::CallbackButton<C4StartupAboutDlg> *btn;
	int32_t iButtonWidth = caButtons.GetInnerWidth() / 4;
	AddElement(btn = new C4GUI::CallbackButton<C4StartupAboutDlg>(LoadResStr("IDS_BTN_BACK"), caButtons.GetGridCell(0,3,0,1,iButtonWidth,C4GUI_ButtonHgt,true), &C4StartupAboutDlg::OnBackBtn));
	btn->SetToolTip(LoadResStr("IDS_DLGTIP_BACKMAIN"));
	if (!Config.Registered())
		{
		AddElement(btn = new C4GUI::CallbackButton<C4StartupAboutDlg>(LoadResStr("IDS_BTN_REGISTERNOW"), caButtons.GetGridCell(1,3,0,1,iButtonWidth,C4GUI_ButtonHgt,true), &C4StartupAboutDlg::OnRegisterBtn));
		btn->SetToolTip(LoadResStr("IDS_DESC_GOTOTHEONLINEREGISTRATION"));
		}
	AddElement(btn = new C4GUI::CallbackButton<C4StartupAboutDlg>(LoadResStr("IDS_BTN_CHECKFORUPDATES"), caButtons.GetGridCell(2,3,0,1,iButtonWidth,C4GUI_ButtonHgt,true), &C4StartupAboutDlg::OnUpdateBtn));
	btn->SetToolTip(LoadResStr("IDS_DESC_CHECKONLINEFORNEWVERSIONS"));
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
	// no webcode label in unregistered
	if (!pWebCodeLbl) return;
	// display countdown first
	// display webcode after time is up
	if (iWebCodeTimer)
		{
		if (--iWebCodeTimer)
			{
			// countdown
			pWebCodeLbl->SetText(FormatString("%s %s (%d)", LoadResStr("IDS_CTL_WEBCODE"), LoadResStr("IDS_CTL_PLEASEWAIT"), iWebCodeTimer).getData(), false);
			}
		else
			{
			// webcode display
			pWebCodeLbl->SetText(FormatString("%s %s", LoadResStr("IDS_CTL_WEBCODE"), Config.GetRegistrationData("WebCode")).getData(), false);
			}
		}
	}

void C4StartupAboutDlg::DrawElement(C4TargetFacet &cgo)
	{
	// draw background - do not use bg drawing proc, because it stretches
	// pre-clear background instead to prevent blinking borders
	if (!IsFading()) lpDDraw->FillBG();
	C4Startup::Get()->Graphics.fctAboutBG.Draw(cgo, FALSE);
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

void C4StartupAboutDlg::OnRegisterBtn(C4GUI::Control *btn)
	{
	// open hardcoded registration URL
	// URL needs lowercase language code, two-character code only
	StdStrBuf sLangCode; sLangCode.Format("%.2s", Config.General.Language);
	sLangCode.ToLowerCase();
	OpenURL(FormatString("http://www.clonk.de/register.php?lng=%s&product=cr", sLangCode.getData()).getData());
	}

void C4StartupAboutDlg::OnUpdateBtn(C4GUI::Control *btn)
	{
	C4UpdateDlg::CheckForUpdates(GetScreen());
	}
