/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2008  Sven Eberhardt
 * Copyright (c) 2006, 2008  Günther Brammer
 * Copyright (c) 2006  Florian Groß
 * Copyright (c) 2007  Julian Raschke
 * Copyright (c) 2007  Matthes Bender
 * Copyright (c) 2008  Armin Burgmeier
 * Copyright (c) 2005-2009, RedWolf Design GmbH, http://www.clonk.de
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
// Startup screen for non-parameterized engine start: Options dialog

#include <C4Include.h>
#include <C4StartupOptionsDlg.h>

#include <C4StartupMainDlg.h>
#include <C4Language.h>
#include <C4GamePadCon.h>
#include <C4Game.h>
#include <C4Log.h>
#include <C4GraphicsResource.h>
#include <C4Network2.h>

#include <StdGL.h>

// ------------------------------------------------
// --- C4StartupOptionsDlg::SmallButton

void C4StartupOptionsDlg::SmallButton::DrawElement(C4TargetFacet &cgo)
	{
	// draw the button
	CStdFont &rUseFont = C4Startup::Get()->Graphics.BookFont;
	// get text pos
	float x0 = cgo.TargetX + rcBounds.x, y0 = cgo.TargetY + rcBounds.y, x1 = cgo.TargetX + rcBounds.x + rcBounds.Wdt, y1 = cgo.TargetY + rcBounds.y + rcBounds.Hgt;
	int32_t iTextHgt = rUseFont.GetLineHeight();
	// draw frame
	uint32_t dwClrHigh = C4StartupBtnBorderColor1, dwClrLow = C4StartupBtnBorderColor2;
	if (fDown) Swap<uint32_t>(dwClrHigh, dwClrLow);
	int32_t iIndent = BoundBy<int32_t>((rcBounds.Hgt-iTextHgt)/3, 2, 5);
	float iDrawQuadTop[8] = { x0,y0, x1,y0, x1-iIndent,y0+iIndent, x0,y0+iIndent };
	float iDrawQuadLeft[8] = { x0,y0, x0+iIndent,y0, x0+iIndent,y1-iIndent, x0,y1 };
	float iDrawQuadRight[8] = { x1,y0, x1,y1, x1-iIndent,y1, x1-iIndent,y0+iIndent };
	float iDrawQuadBottom[8] = { x1,y1, x0,y1, x0+iIndent,y1-iIndent, x1,y1-iIndent };
	lpDDraw->DrawQuadDw(cgo.Surface, iDrawQuadTop, dwClrHigh,dwClrHigh,dwClrHigh,dwClrHigh);
	lpDDraw->DrawQuadDw(cgo.Surface, iDrawQuadLeft, dwClrHigh,dwClrHigh,dwClrHigh,dwClrHigh);
	lpDDraw->DrawQuadDw(cgo.Surface, iDrawQuadRight, dwClrLow,dwClrLow,dwClrLow,dwClrLow);
	lpDDraw->DrawQuadDw(cgo.Surface, iDrawQuadBottom, dwClrLow,dwClrLow,dwClrLow,dwClrLow);
	//lpDDraw->DrawFrameDw(cgo.Surface, x0+1, y0+1, x1-1, y1-1, aC4StartupBtnBorderColor2);
	// draw selection highlight
	int32_t iTxtOff = fDown ? iIndent : 0;
	if (fEnabled) if (HasDrawFocus() || (fMouseOver && IsInActiveDlg(false)))
		{
		lpDDraw->SetBlitMode(C4GFXBLIT_ADDITIVE);
		C4GUI::GetRes()->fctButtonHighlight.DrawX(cgo.Surface, x0+5+iTxtOff, y0+3+iTxtOff, rcBounds.Wdt-10, rcBounds.Hgt-6);
		lpDDraw->ResetBlitMode();
		}
	// draw button text
	lpDDraw->TextOut(sText.getData(), rUseFont, 1.0f, cgo.Surface, (x0+x1)/2 + iTxtOff, (y0+y1-iTextHgt)/2 + iTxtOff, C4StartupBtnFontClr, ACenter, true);
	}

int32_t C4StartupOptionsDlg::SmallButton::GetDefaultButtonHeight()
	{
	// button height is used font height plus a small indent
	CStdFont *pUseFont = &(C4Startup::Get()->Graphics.BookFont);
	return pUseFont->GetLineHeight()*6/5+6;
	}


// ------------------------------------------------
// --- C4StartupOptionsDlg::ResChangeConfirmDlg

C4StartupOptionsDlg::ResChangeConfirmDlg::ResChangeConfirmDlg()
: C4GUI::Dialog(C4GUI_MessageDlgWdt, 100 /* will be resized */, LoadResStr("IDS_MNU_SWITCHRESOLUTION"), false)
	{
	// update-timer
	Application.Add(this);
	// An independant group of fourteen highly trained apes and one blind lawnmower have determined
	//  that twelve seconds is just right for normal people
	iResChangeSwitchTime = 12;
	// However, some people need more time
	// Those can be identified by their configuration settings
	if (Config.Graphics.SaveDefaultPortraits) iResChangeSwitchTime += 2;
	if (SEqualNoCase(Config.GetRegistrationData("Nick"), "flgr")) iResChangeSwitchTime *= 10;
	// get positions
	C4GUI::ComponentAligner caMain(GetClientRect(), C4GUI_DefDlgIndent, C4GUI_DefDlgIndent, true);
	// place icon
	C4Rect rcIcon = caMain.GetFromLeft(C4GUI_IconWdt); rcIcon.Hgt = C4GUI_IconHgt;
	C4GUI::Icon *pIcon = new C4GUI::Icon(rcIcon, C4GUI::Ico_Confirm); AddElement(pIcon);
	// place message labels
	// use text with line breaks
	StdStrBuf sMsgBroken;
	int iMsgHeight = C4GUI::GetRes()->TextFont.BreakMessage(LoadResStr("IDS_MNU_SWITCHRESOLUTION_LIKEIT"), caMain.GetInnerWidth(), &sMsgBroken, true);
	C4GUI::Label *pLblMessage = new C4GUI::Label(sMsgBroken.getData(), caMain.GetFromTop(iMsgHeight), ACenter, C4GUI_MessageFontClr, &C4GUI::GetRes()->TextFont, false);
	AddElement(pLblMessage);
	iMsgHeight = C4GUI::GetRes()->TextFont.BreakMessage(FormatString(LoadResStr("IDS_MNU_SWITCHRESOLUTION_UNDO"),
                                                                    (int)iResChangeSwitchTime).getData(),
                                                       caMain.GetInnerWidth(), &sMsgBroken, true);
	pOperationCancelLabel = new C4GUI::Label(sMsgBroken.getData(), caMain.GetFromTop(iMsgHeight), ACenter, C4GUI_MessageFontClr, &C4GUI::GetRes()->TextFont, false, false);
	AddElement(pOperationCancelLabel);
	// place buttons
	C4GUI::ComponentAligner caButtonArea(caMain.GetFromTop(C4GUI_ButtonAreaHgt), 0,0);
	int32_t iButtonCount = 2;
	C4Rect rcBtn = caButtonArea.GetCentered(iButtonCount*C4GUI_DefButton2Wdt+(iButtonCount-1)*C4GUI_DefButton2HSpace, C4GUI_ButtonHgt);
	rcBtn.Wdt = C4GUI_DefButton2Wdt;
	// Yes
	C4GUI::Button *pBtnYes = new C4GUI::YesButton(rcBtn);
	AddElement(pBtnYes); //pBtnYes->SetToolTip(LoadResStr("IDS_DLGTIP_OK2"));
	rcBtn.x += C4GUI_DefButton2Wdt+C4GUI_DefButton2HSpace;
	// No
	C4GUI::Button *pBtnNo = new C4GUI::NoButton(rcBtn);
	AddElement(pBtnNo); //pBtnNo->SetToolTip(LoadResStr("IDS_DLGTIP_CANCEL"));
	// initial focus on abort button, to prevent accidental acceptance of setting by "blind" users
	SetFocus(pBtnNo, false);
	// resize to actually needed size
	SetClientSize(GetClientRect().Wdt, GetClientRect().Hgt - caMain.GetHeight());
	}

C4StartupOptionsDlg::ResChangeConfirmDlg::~ResChangeConfirmDlg()
	{
	Application.Remove(this);
	}

void C4StartupOptionsDlg::ResChangeConfirmDlg::OnSec1Timer()
	{
	// timer ran out? Then cancel dlg
	if (!--iResChangeSwitchTime)
		{
		Close(false); return;
		}
	// update timer label
	StdStrBuf sTimerText;
	C4GUI::GetRes()->TextFont.BreakMessage(FormatString(LoadResStr("IDS_MNU_SWITCHRESOLUTION_UNDO"),
                                                       (int)iResChangeSwitchTime).getData(),
                                          pOperationCancelLabel->GetBounds().Wdt, &sTimerText, true);
	pOperationCancelLabel->SetText(sTimerText.getData());
	}

// ------------------------------------------------
// --- C4StartupOptionsDlg::KeySelDialog

const char *KeyID2Desc(int32_t iKeyID)
	{
	const char *KeyIDStringIDs[C4MaxKey] =
		{ "IDS_CTL_SELECTLEFT", "IDS_CTL_SELECTTOGGLE", "IDS_CTL_SELECTRIGHT",
		  "IDS_CTL_THROW"     , "IDS_CTL_UPJUMP"      , "IDS_CTL_DIG"        ,
			"IDS_CTL_LEFT"      , "IDS_CTL_DOWNSTOP"    , "IDS_CTL_RIGHT"      ,
			"IDS_CTL_PLAYERMENU", "IDS_CTL_SPECIAL1"    , "IDS_CTL_SPECIAL2" };
	if (!Inside<int32_t>(iKeyID, 0, C4MaxKey)) return NULL;
	return LoadResStr(KeyIDStringIDs[iKeyID]);
	}

C4StartupOptionsDlg::KeySelDialog::KeySelDialog(int32_t iKeyID, int32_t iCtrlSet, bool fGamepad)
: C4GUI::MessageDialog(FormatString(LoadResStr(!fGamepad ? "IDS_MSG_PRESSKEY" : "IDS_MSG_PRESSBTN"),
                                    KeyID2Desc(iKeyID), iCtrlSet+1).getData(), LoadResStr("IDS_MSG_DEFINEKEY"),
																		C4GUI::MessageDialog::btnAbort, fGamepad ? C4GUI::Ico_Gamepad : C4GUI::Ico_Keyboard, C4GUI::MessageDialog::dsRegular),
  key(KEY_Undefined), fGamepad(fGamepad), iCtrlSet(iCtrlSet)
	{
	pKeyListener = new C4KeyBinding(C4KeyCodeEx(KEY_Any, KEYS_None), "DefineKey", KEYSCOPE_Gui, new C4GUI::DlgKeyCBPassKey<C4StartupOptionsDlg::KeySelDialog>(*this, &C4StartupOptionsDlg::KeySelDialog::KeyDown), C4CustomKey::PRIO_PlrControl);
	}

C4StartupOptionsDlg::KeySelDialog::~KeySelDialog()
	{
	delete pKeyListener;
	}

bool C4StartupOptionsDlg::KeySelDialog::KeyDown(const C4KeyCodeEx &key)
	{
	// check if key is valid for this set
	// do not mix gamepad and keyboard keys
	if (Key_IsGamepad(key.Key) != fGamepad) return false;
	// allow selected gamepad only
	if (fGamepad && Key_GetGamepad(key.Key) != iCtrlSet) return false;
	// okay, use it
	this->key=key.Key;
	Close(true);
	return true;
	}

// ------------------------------------------------
// --- C4StartupOptionsDlg::KeySelButton

// key display arrangement - -1 denotes no key
// every key from 0 to C4MaxKey-1 MUST BE present here, or the engine will crash
const int32_t iKeyPosMaxX = 3, iKeyPosMaxY = 4; // arrange keys in a 4-by-5-array
const int32_t iKeyPosis[iKeyPosMaxY][iKeyPosMaxX] = {
  { 0, 1, 2 },
	{ 3, 4, 5 },
	{ 6, 7, 8 },
	{ 9,10,11 } };


void C4StartupOptionsDlg::KeySelButton::DrawElement(C4TargetFacet &cgo)
	{
	// draw key
	C4Facet cgoDraw(cgo.Surface, rcBounds.x+cgo.TargetX, rcBounds.y+cgo.TargetY, rcBounds.Wdt, rcBounds.Hgt);
	::GraphicsResource.fctKey.Draw(cgoDraw, true, fDown);
	int32_t iKeyIndent = cgoDraw.Wdt / 5;
	cgoDraw.X += iKeyIndent; cgoDraw.Wdt -= 2*iKeyIndent;
	cgoDraw.Y += iKeyIndent*3/4; cgoDraw.Hgt -= 2*iKeyIndent;
	if (fDown) cgoDraw.Y += iKeyIndent/2;
	bool fDoHightlight = fHighlight || HasDrawFocus() || (fMouseOver && IsInActiveDlg(false));
	bool fHadBlitMod=false; DWORD dwOldBlitModClr=0xffffffff;
	if (!fDoHightlight)
		{
		DWORD dwModClr = 0xff7f7f7f;
		if ((fHadBlitMod = lpDDraw->GetBlitModulation(dwOldBlitModClr)))
			ModulateClr(dwModClr, dwOldBlitModClr);
		lpDDraw->ActivateBlitModulation(dwModClr);
		}
	::GraphicsResource.fctCommand.Draw(cgoDraw, true, iKeyID, 0);
	if (!fDoHightlight)
		{
		if (fHadBlitMod)
			lpDDraw->ActivateBlitModulation(dwOldBlitModClr);
		else
			lpDDraw->DeactivateBlitModulation();
		}
	// draw the labels - beside the key
	float fZoom;
	CStdFont &rUseFont = C4Startup::Get()->Graphics.GetBlackFontByHeight(cgoDraw.Hgt/2+5, &fZoom);
	lpDDraw->TextOut(KeyID2Desc(iKeyID), rUseFont, fZoom, cgo.Surface, cgo.TargetX+rcBounds.x+rcBounds.Wdt+5, cgoDraw.Y-3, fDoHightlight ? 0xffff0000 : C4StartupFontClr, ALeft, false);
	StdStrBuf strKey; strKey.Copy(C4KeyCodeEx::KeyCode2String(key, true, false));
	lpDDraw->TextOut(strKey.getData(), rUseFont, fZoom, cgo.Surface, cgo.TargetX+rcBounds.x+rcBounds.Wdt+5, cgoDraw.Y+cgoDraw.Hgt/2, fDoHightlight ? 0xffff0000 : C4StartupFontClr, ALeft, false);
	}


C4StartupOptionsDlg::KeySelButton::KeySelButton(int32_t iKeyID, const C4Rect &rcBounds, char cHotkey)
: C4GUI::IconButton(C4GUI::Ico_None, rcBounds, cHotkey), iKeyID(iKeyID), key(KEY_Undefined)
	{
	}

// ------------------------------------------------
// --- C4StartupOptionsDlg::ControlConfigArea

C4StartupOptionsDlg::ControlConfigArea::ControlConfigArea(const C4Rect &rcArea, int32_t iHMargin, int32_t iVMargin, bool fGamepad, C4StartupOptionsDlg *pOptionsDlg)
: C4GUI::Window(), fGamepad(fGamepad), pGamepadOpener(NULL), pOptionsDlg(pOptionsDlg), pGUICtrl(NULL)
	{
	CStdFont *pUseFont = &(C4Startup::Get()->Graphics.BookFont);
	SetBounds(rcArea);
	C4GUI::ComponentAligner caArea(rcArea, iHMargin, iVMargin, true);
	// get number of control sets to be configured
	iMaxControlSets = 1; // do not devide by zero
	if (fGamepad && Application.pGamePadControl)
		iMaxControlSets = Max(1, Application.pGamePadControl->GetGamePadCount());
	if (!fGamepad)
		iMaxControlSets = C4MaxKeyboardSet;
	ppKeyControlSetBtns = new C4GUI::IconButton *[iMaxControlSets];
	// top line buttons to select keyboard set or gamepad
	C4Facet fctCtrlPic = fGamepad ? ::GraphicsResource.fctGamepad : ::GraphicsResource.fctKeyboard;
	int32_t iCtrlSetWdt = caArea.GetWidth() - caArea.GetHMargin()*2;
	int32_t iCtrlSetHMargin = 5, iCtrlSetVMargin = 5;
	int32_t iCtrlSetBtnWdt = BoundBy<int32_t>((iCtrlSetWdt - iMaxControlSets*iCtrlSetHMargin*2) / iMaxControlSets, 5, fctCtrlPic.Wdt);
	int32_t iCtrlSetBtnHgt = fctCtrlPic.GetHeightByWidth(iCtrlSetBtnWdt);
	iCtrlSetHMargin = (iCtrlSetWdt - iCtrlSetBtnWdt*iMaxControlSets) / (iMaxControlSets*2);
	C4GUI::ComponentAligner caKeyboardSetSel(caArea.GetFromTop(2*iCtrlSetVMargin+iCtrlSetBtnHgt), iCtrlSetHMargin, iCtrlSetVMargin);
	const char *szCtrlSetHotkeys = "1234567890"; /* 2do */
	uint32_t i;
	for (i = 0; i < (uint32_t)iMaxControlSets; ++i)
		{
		char cCtrlSetHotkey = 0;
		if (i <= strlen(szCtrlSetHotkeys)) cCtrlSetHotkey = szCtrlSetHotkeys[i];
		C4GUI::CallbackButton<C4StartupOptionsDlg::ControlConfigArea, C4GUI::IconButton> *pBtn = new C4GUI::CallbackButton<C4StartupOptionsDlg::ControlConfigArea, C4GUI::IconButton>(C4GUI::Ico_None, caKeyboardSetSel.GetFromLeft(iCtrlSetBtnWdt), cCtrlSetHotkey, &C4StartupOptionsDlg::ControlConfigArea::OnCtrlSetBtn, this);
		pBtn->SetFacet(fctCtrlPic);
		fctCtrlPic.X += fctCtrlPic.Wdt;
		AddElement(ppKeyControlSetBtns[i] = pBtn);
		pBtn->SetToolTip(LoadResStr("IDS_MSG_SELECTKEYSET"));
		}
	iSelectedCtrlSet = fGamepad ? 0 : C4P_Control_Keyboard1;
	caArea.ExpandTop(caArea.GetVMargin());
	AddElement(new C4GUI::HorizontalLine(caArea.GetFromTop(2)));
	caArea.ExpandTop(caArea.GetVMargin());
	C4Facet &rfctKey = ::GraphicsResource.fctKey;
	int32_t iKeyAreaMaxWdt = caArea.GetWidth()-2*caArea.GetHMargin(), iKeyAreaMaxHgt = caArea.GetHeight()-2*caArea.GetVMargin();
	int32_t iKeyWdt = rfctKey.Wdt*3/2, iKeyHgt = rfctKey.Hgt*3/2;
	int32_t iKeyUseWdt = iKeyWdt + iKeyHgt*3; // add space for label
	int32_t iKeyMargin = 20;
	int32_t iKeyAreaWdt = (iKeyUseWdt+2*iKeyMargin) * iKeyPosMaxX, iKeyAreaHgt = (iKeyHgt+2*iKeyMargin) * iKeyPosMaxY;
	if (iKeyAreaWdt > iKeyAreaMaxWdt || iKeyAreaHgt > iKeyAreaMaxHgt)
		{
		// scale down
		float fScaleX = float(iKeyAreaMaxWdt) / float(Max<int32_t>(iKeyAreaWdt,1)),
		      fScaleY = float(iKeyAreaMaxHgt) / float(Max<int32_t>(iKeyAreaHgt,1)), fScale;
		if (fScaleX > fScaleY) fScale = fScaleY; else fScale = fScaleX;
		iKeyMargin = int32_t(fScale*iKeyMargin);
		iKeyWdt = int32_t(fScale*iKeyWdt);
		iKeyUseWdt = int32_t(fScale*iKeyUseWdt);
		iKeyHgt = int32_t(fScale*iKeyHgt);
		iKeyAreaWdt = int32_t(fScale*iKeyAreaWdt);
		iKeyAreaHgt = int32_t(fScale*iKeyAreaHgt);
		}
	C4GUI::ComponentAligner caCtrlKeys(caArea.GetFromTop(iKeyAreaHgt, iKeyAreaWdt), 0,iKeyMargin);
	int32_t iKeyNum;
	for (int iY = 0; iY < iKeyPosMaxY; ++iY)
		{
		C4GUI::ComponentAligner caCtrlKeysLine(caCtrlKeys.GetFromTop(iKeyHgt), iKeyMargin,0);
		for (int iX = 0; iX < iKeyPosMaxX; ++iX)
			{
			C4Rect rcKey = caCtrlKeysLine.GetFromLeft(iKeyWdt);
			caCtrlKeysLine.ExpandLeft(iKeyWdt - iKeyUseWdt);
			if ((iKeyNum=iKeyPosis[iY][iX])<0) continue;
			KeySelButton *pKeyBtn = new C4GUI::CallbackButton<C4StartupOptionsDlg::ControlConfigArea, KeySelButton>(iKeyNum, rcKey, 0 /* no hotkey :( */, &C4StartupOptionsDlg::ControlConfigArea::OnCtrlKeyBtn, this);
			AddElement(KeyControlBtns[iKeyNum] = pKeyBtn);
			pKeyBtn->SetToolTip(KeyID2Desc(iKeyNum));
			}
		}
	// bottom area controls
	caArea.ExpandBottom(-iKeyHgt/2);
	C4GUI::ComponentAligner caKeyBottomBtns(caArea.GetFromBottom(C4GUI_ButtonHgt), 2,0);
	// gamepad: Use for GUI
	if (fGamepad)
		{
		int iWdt=100,iHgt=20;
		const char *szResetText = LoadResStr("IDS_CTL_GAMEPADFORMENU");
		C4GUI::CheckBox::GetStandardCheckBoxSize(&iWdt, &iHgt, szResetText, pUseFont);
		pGUICtrl = new C4GUI::CheckBox(caKeyBottomBtns.GetFromLeft(iWdt, iHgt), szResetText, !!Config.Controls.GamepadGuiControl);
		pGUICtrl->SetOnChecked(new C4GUI::CallbackHandler<C4StartupOptionsDlg::ControlConfigArea>(this, &C4StartupOptionsDlg::ControlConfigArea::OnGUIGamepadCheckChange));
		pGUICtrl->SetToolTip(LoadResStr("IDS_DESC_GAMEPADFORMENU"));
		pGUICtrl->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
		AddElement(pGUICtrl);
		}
	// reset button
	const char *szBtnText = LoadResStr("IDS_BTN_RESETKEYBOARD");
	int32_t iButtonWidth=100, iButtonHeight=20; C4GUI::Button *btn;
	C4GUI::GetRes()->CaptionFont.GetTextExtent(szBtnText, iButtonWidth, iButtonHeight, true);
	C4Rect rcResetBtn = caKeyBottomBtns.GetFromRight(Min<int32_t>(iButtonWidth+iButtonHeight*4, caKeyBottomBtns.GetInnerWidth()));
	AddElement(btn = new C4GUI::CallbackButton<C4StartupOptionsDlg::ControlConfigArea, SmallButton>(szBtnText, rcResetBtn, &C4StartupOptionsDlg::ControlConfigArea::OnResetKeysBtn, this));
	btn->SetToolTip(LoadResStr("IDS_MSG_RESETKEYSETS"));

	UpdateCtrlSet();
	}

C4StartupOptionsDlg::ControlConfigArea::~ControlConfigArea()
	{
	delete [] ppKeyControlSetBtns;
	if (pGamepadOpener) delete pGamepadOpener;
	}

void C4StartupOptionsDlg::ControlConfigArea::OnCtrlSetBtn(C4GUI::Control *btn)
	{
	// select Ctrl set of pressed btn
	int i;
	for (i = 0; i < iMaxControlSets; ++i)
		if (btn == ppKeyControlSetBtns[i])
			{
			iSelectedCtrlSet = i;
			break;
			}
	// update shown keys
	UpdateCtrlSet();
	}

void C4StartupOptionsDlg::ControlConfigArea::UpdateCtrlSet()
	{
	// selected keyboard set btn gets a highlight
	int i;
	for (i = 0; i < iMaxControlSets; ++i)
		ppKeyControlSetBtns[i]->SetHighlight(i == iSelectedCtrlSet);
	// update keys by config
	if (fGamepad)
		{
		for (i = 0; i < C4MaxKey; ++i)
			KeyControlBtns[i]->SetKey(Config.Gamepads[iSelectedCtrlSet].Button[i]);
		}
	else
		for (i = 0; i < C4MaxKey; ++i)
			KeyControlBtns[i]->SetKey(Config.Controls.Keyboard[iSelectedCtrlSet][i]);
	// open gamepad
	if (fGamepad && Config.General.GamepadEnabled)
		{
		if (!pGamepadOpener) pGamepadOpener = new C4GamePadOpener(iSelectedCtrlSet);
		else pGamepadOpener->SetGamePad(iSelectedCtrlSet);
		}
	// show/hide gamepad-gui-control checkbox
	if (fGamepad && pGUICtrl)
		pGUICtrl->SetVisibility(iSelectedCtrlSet == 0);
	}

void C4StartupOptionsDlg::ControlConfigArea::OnCtrlKeyBtn(C4GUI::Control *btn)
	{
	// determine which key has been pressed
	int32_t idKey;
	for (idKey = 0; idKey < C4MaxKey; ++idKey) if (KeyControlBtns[idKey] == btn) break;
	if (idKey==C4MaxKey) return; // can't happen
	// show key selection dialog for it
	KeySelDialog *pDlg = new KeySelDialog(idKey, iSelectedCtrlSet, fGamepad);
	pDlg->SetDelOnClose(false);
	bool fSuccess = GetScreen()->ShowModalDlg(pDlg, false);
	C4KeyCode key = pDlg->GetKeyCode();
	delete pDlg;
	if (!fSuccess) return;
	// key defined: Set it
	KeyControlBtns[idKey]->SetKey(key);
	// and update config
	if (fGamepad)
		Config.Gamepads[iSelectedCtrlSet].Button[idKey] = key;
	else
		Config.Controls.Keyboard[iSelectedCtrlSet][idKey] = key;
	}

void C4StartupOptionsDlg::ControlConfigArea::OnResetKeysBtn(C4GUI::Control *btn)
	{
	// default keys and axis reset
	if (fGamepad)
		{
		for (int i = 0; i < C4ConfigMaxGamepads; ++i)
			Config.Gamepads[i].Reset();
		}
	else
		Config.Controls.ResetKeys();
	UpdateCtrlSet();
	}

void C4StartupOptionsDlg::ControlConfigArea::OnGUIGamepadCheckChange(C4GUI::Element *pCheckBox)
	{
	// same as before?
	bool fChecked = ((C4GUI::CheckBox *)(pCheckBox))->GetChecked();
	if (fChecked == !!Config.Controls.GamepadGuiControl) return;
	// reflect change
	Config.Controls.GamepadGuiControl = fChecked;
	::pGUI->UpdateGamepadGUIControlEnabled();
	pOptionsDlg->RecreateDialog(false);
	}



// ------------------------------------------------
// --- C4StartupOptionsDlg::NetworkPortConfig

C4StartupOptionsDlg::NetworkPortConfig::NetworkPortConfig(const C4Rect &rcBounds, const char *szName, int32_t *pConfigValue, int32_t iDefault)
: C4GUI::Window(), pConfigValue(pConfigValue)
	{
	// ctor
	CStdFont *pUseFont = &(C4Startup::Get()->Graphics.BookFont);
	SetBounds(rcBounds);
	C4GUI::ComponentAligner caMain(GetClientRect(), 0,2, true);
	bool fEnabled = (*pConfigValue != -1);
	C4GUI::Label *pLbl = new C4GUI::Label(szName, caMain.GetFromTop(pUseFont->GetLineHeight()), ALeft, C4StartupFontClr, pUseFont, false);
	AddElement(pLbl);
	C4GUI::ComponentAligner caBottomLine(caMain.GetAll(), 2,0, false);
	const char *szText = LoadResStr("IDS_CTL_ACTIVE");
	int iWdt=100, iHgt=12;
	C4GUI::CheckBox::GetStandardCheckBoxSize(&iWdt, &iHgt, szText, pUseFont);
	pEnableCheck = new C4GUI::CheckBox(caBottomLine.GetFromLeft(iWdt, iHgt), szText, fEnabled);
	pEnableCheck->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
	pEnableCheck->SetOnChecked(new C4GUI::CallbackHandler<C4StartupOptionsDlg::NetworkPortConfig>(this, &C4StartupOptionsDlg::NetworkPortConfig::OnEnabledCheckChange));
	AddElement(pEnableCheck);
	pPortEdit = new C4GUI::Edit(caBottomLine.GetAll(), true);
	pPortEdit->SetColors(C4StartupEditBGColor, C4StartupFontClr, C4StartupEditBorderColor);
	pPortEdit->SetFont(pUseFont);
	pPortEdit->InsertText(FormatString("%d", fEnabled ? ((int) *pConfigValue) : (int) iDefault).getData(), false);
	pPortEdit->SetMaxText(10); // 65535 is five characters long - but allow some more for easier editing
	pPortEdit->SetVisibility(fEnabled);
	//pPortEdit->SetEnabled(fEnabled);
	AddElement(pPortEdit);
	}

void C4StartupOptionsDlg::NetworkPortConfig::OnEnabledCheckChange(C4GUI::Element *pCheckBox)
	{
	pPortEdit->SetVisibility(pEnableCheck->GetChecked());
	}

void C4StartupOptionsDlg::NetworkPortConfig::SavePort()
	{
	*pConfigValue=GetPort();
	}

int32_t C4StartupOptionsDlg::NetworkPortConfig::GetPort()
	{
		// controls to config
	if (!pEnableCheck->GetChecked())
		return -1;
	else
		return atoi(pPortEdit->GetText());
	}

bool C4StartupOptionsDlg::NetworkPortConfig::GetControlSize(int *piWdt, int *piHgt)
	{
	CStdFont *pUseFont = &(C4Startup::Get()->Graphics.BookFont);
	// get size needed for control
	if (piWdt)
		{
		const char *szText = LoadResStr("IDS_CTL_ACTIVE");
		if (!C4GUI::CheckBox::GetStandardCheckBoxSize(piWdt, piHgt, szText, pUseFont)) return false;
		*piWdt *= 2;
		}
	if (piHgt) *piHgt = C4GUI::Edit::GetCustomEditHeight(pUseFont) + pUseFont->GetLineHeight()+2*4;
	return true;
	}


// ------------------------------------------------
// --- C4StartupOptionsDlg::NetworkServerAddressConfig

C4StartupOptionsDlg::NetworkServerAddressConfig::NetworkServerAddressConfig(const C4Rect &rcBounds, const char *szName, int32_t *piConfigEnableValue, char *szConfigAddressValue, int iTabWidth)
: C4GUI::Window(), piConfigEnableValue(piConfigEnableValue), szConfigAddressValue(szConfigAddressValue)
	{
	// ctor
	CStdFont *pUseFont = &(C4Startup::Get()->Graphics.BookFont);
	SetBounds(rcBounds);
	C4GUI::ComponentAligner caMain(GetClientRect(), 0,2, true);
	pEnableCheck = new C4GUI::CheckBox(caMain.GetFromLeft(iTabWidth, pUseFont->GetLineHeight()), szName, !!*piConfigEnableValue);
	pEnableCheck->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
	pEnableCheck->SetOnChecked(new C4GUI::CallbackHandler<C4StartupOptionsDlg::NetworkServerAddressConfig>(this, &C4StartupOptionsDlg::NetworkServerAddressConfig::OnEnabledCheckChange));
	AddElement(pEnableCheck);
	caMain.ExpandLeft(-2);
	pAddressEdit = new C4GUI::Edit(caMain.GetAll(), true);
	pAddressEdit->SetColors(C4StartupEditBGColor, C4StartupFontClr, C4StartupEditBorderColor);
	pAddressEdit->SetFont(pUseFont);
	pAddressEdit->InsertText(szConfigAddressValue, false);
	pAddressEdit->SetMaxText(CFG_MaxString);
	pAddressEdit->SetVisibility(!!*piConfigEnableValue);
	AddElement(pAddressEdit);
	}

void C4StartupOptionsDlg::NetworkServerAddressConfig::OnEnabledCheckChange(C4GUI::Element *pCheckBox)
	{
	// warn about using alternate servers
	if (pEnableCheck->GetChecked())
		{
		GetScreen()->ShowMessage(LoadResStr("IDS_NET_NOOFFICIALLEAGUE"), LoadResStr("IDS_NET_QUERY_MASTERSRV"), C4GUI::Ico_Notify, &Config.Startup.HideMsgNoOfficialLeague);
		}
	// callback when checkbox is ticked
	pAddressEdit->SetVisibility(pEnableCheck->GetChecked());
	}

void C4StartupOptionsDlg::NetworkServerAddressConfig::Save2Config()
	{
	// controls to config
	*piConfigEnableValue = pEnableCheck->GetChecked();
	SCopy(pAddressEdit->GetText(), szConfigAddressValue, CFG_MaxString);
	}

bool C4StartupOptionsDlg::NetworkServerAddressConfig::GetControlSize(int *piWdt, int *piHgt, int *piTabPos, const char *szForText)
	{
	CStdFont *pUseFont = &(C4Startup::Get()->Graphics.BookFont);
	int iWdt=120;
	C4GUI::CheckBox::GetStandardCheckBoxSize(&iWdt, piHgt, szForText, pUseFont);
	int32_t iEdtWdt=120, iEdtHgt=24;
	pUseFont->GetTextExtent("sorgentelefon@treffpunktclonk.net", iEdtWdt, iEdtHgt, false);
	if (piWdt) *piWdt = iWdt+iEdtWdt+2;
	if (piTabPos) *piTabPos = iWdt+2;
	if (piHgt) *piHgt = C4GUI::Edit::GetCustomEditHeight(pUseFont)+2*2;
	return true;
	}



// ------------------------------------------------
// --- C4StartupOptionsDlg::BoolConfig

C4StartupOptionsDlg::BoolConfig::BoolConfig(const C4Rect &rcBounds, const char *szName, bool *pbVal, int32_t *piVal, bool fInvert, int32_t *piRestartChangeCfgVal)
: C4GUI::CheckBox(rcBounds, szName, fInvert != (pbVal ? *pbVal : !!*piVal)), pbVal(pbVal), fInvert(fInvert), piVal(piVal), piRestartChangeCfgVal(piRestartChangeCfgVal)
	{
	SetOnChecked(new C4GUI::CallbackHandler<BoolConfig>(this, &BoolConfig::OnCheckChange));
	}

void C4StartupOptionsDlg::BoolConfig::OnCheckChange(C4GUI::Element *pCheckBox)
	{
	if (pbVal) *pbVal = (GetChecked() != fInvert);
	if (piVal) *piVal = (GetChecked() != fInvert);
	if (piRestartChangeCfgVal) GetScreen()->ShowMessage(LoadResStr("IDS_MSG_RESTARTCHANGECFG"), GetText(),
                                                       C4GUI::Ico_Notify, piRestartChangeCfgVal);
	}

// ------------------------------------------------
// --- C4StartupOptionsDlg::EditConfig

C4StartupOptionsDlg::EditConfig::EditConfig(const C4Rect &rcBounds, const char *szName, ValidatedStdCopyStrBufBase *psConfigVal, int32_t *piConfigVal, bool fMultiline)
: C4GUI::LabeledEdit(rcBounds, szName, fMultiline, psConfigVal ? psConfigVal->getData() : NULL, &(C4Startup::Get()->Graphics.BookFont), C4StartupFontClr), psConfigVal(psConfigVal), piConfigVal(piConfigVal)
	{
	// ctor
	GetEdit()->SetColors(C4StartupEditBGColor, C4StartupFontClr, C4StartupEditBorderColor);
	if (piConfigVal) SetIntVal(*piConfigVal);
	GetEdit()->SetMaxText(CFG_MaxString);
	}

void C4StartupOptionsDlg::EditConfig::Save2Config()
	{
	// controls to config
	if (psConfigVal)
		psConfigVal->CopyValidated(GetEdit()->GetText());
	if (piConfigVal)
		*piConfigVal = GetIntVal();
	}

bool C4StartupOptionsDlg::EditConfig::GetControlSize(int *piWdt, int *piHgt, const char *szForText, bool fMultiline)
	{
	CStdFont *pUseFont = &(C4Startup::Get()->Graphics.BookFont);
	typedef C4GUI::LabeledEdit BaseEdit;
	return BaseEdit::GetControlSize(piWdt, piHgt, szForText, pUseFont, fMultiline);
	}

// ------------------------------------------------
// --- C4StartupOptionsDlg

C4StartupOptionsDlg::C4StartupOptionsDlg() : C4StartupDlg(LoadResStrNoAmp("IDS_DLG_OPTIONS")), fConfigSaved(false), fCanGoBack(true)
	{
	// ctor
	UpdateSize();
	bool fSmall = (GetClientRect().Wdt < 750);
	CStdFont *pUseFont = &(C4Startup::Get()->Graphics.BookFont);

	// key bindings
	C4CustomKey::CodeList keys;
	keys.push_back(C4KeyCodeEx(K_BACK)); keys.push_back(C4KeyCodeEx(K_LEFT));
	pKeyBack = new C4KeyBinding(keys, "StartupOptionsBack", KEYSCOPE_Gui,
		new C4GUI::DlgKeyCB<C4StartupOptionsDlg>(*this, &C4StartupOptionsDlg::KeyBack), C4CustomKey::PRIO_Dlg);
	keys.clear();
	keys.push_back(C4KeyCodeEx(K_F3)); // overloading global toggle with higher priority here, so a new name is required
	pKeyToggleMusic = new C4KeyBinding(keys, "OptionsMusicToggle", KEYSCOPE_Gui,
		new C4GUI::DlgKeyCB<C4StartupOptionsDlg>(*this, &C4StartupOptionsDlg::KeyMusicToggle), C4CustomKey::PRIO_Dlg);

	// screen calculations
	int32_t iButtonWidth,iCaptionFontHgt;
	int32_t iButtonHeight = C4GUI_ButtonHgt;
	C4GUI::GetRes()->CaptionFont.GetTextExtent("<< BACK", iButtonWidth, iCaptionFontHgt, true);
	iButtonWidth *= 3;
	int iIndentX1, iIndentX2, iIndentY1, iIndentY2;
	if (fSmall)
		{
		iIndentX1=20; iIndentX2=1;
		}
	else
		{
		iIndentX1 = GetClientRect().Wdt/40;
		iIndentX2 = iIndentX1/2;
		}
	if (fSmall)
		{
		iIndentY1=1; iIndentY2=1;
		}
	else
		{
		iIndentY1 = GetClientRect().Hgt/200;
		iIndentY2 = Max<int32_t>(1, iIndentY1/2);
		}
	C4GUI::ComponentAligner caMain(GetClientRect(), 0,0, true);
	C4GUI::ComponentAligner caButtonArea(caMain.GetFromBottom(caMain.GetHeight()/(fSmall ? 20 : 7)),0,0);
	C4GUI::ComponentAligner caButtons(caButtonArea.GetCentered(caMain.GetWidth()*7/8, iButtonHeight),0,0);
	C4GUI::ComponentAligner caConfigArea(caMain.GetAll(), fSmall ? 0 : (caMain.GetWidth()*69/1730), fSmall ? 0 : (caMain.GetHeight()/200));

	// back button
	C4GUI::CallbackButton<C4StartupOptionsDlg> *btn;
	AddElement(btn = new C4GUI::CallbackButton<C4StartupOptionsDlg>(LoadResStr("IDS_BTN_BACK"), caButtons.GetFromLeft(iButtonWidth), &C4StartupOptionsDlg::OnBackBtn));
	btn->SetToolTip(LoadResStr("IDS_DLGTIP_BACKMAIN"));

	// main config area tabular
	pOptionsTabular = new C4GUI::Tabular(caConfigArea.GetAll(), C4GUI::Tabular::tbLeft);
	pOptionsTabular->SetGfx(&C4Startup::Get()->Graphics.fctOptionsDlgPaper, &C4Startup::Get()->Graphics.fctOptionsTabClip, &C4Startup::Get()->Graphics.fctOptionsIcons, &C4Startup::Get()->Graphics.BookSmallFont, true);
	AddElement(pOptionsTabular);
	C4GUI::Tabular::Sheet *pSheetGeneral  = pOptionsTabular->AddSheet(LoadResStr("IDS_DLG_PROGRAM") , 0);
	C4GUI::Tabular::Sheet *pSheetGraphics = pOptionsTabular->AddSheet(LoadResStr("IDS_DLG_GRAPHICS"), 1);
	C4GUI::Tabular::Sheet *pSheetSound    = pOptionsTabular->AddSheet(LoadResStr("IDS_DLG_SOUND")   , 2);
	C4GUI::Tabular::Sheet *pSheetKeyboard = pOptionsTabular->AddSheet(LoadResStr("IDS_DLG_KEYBOARD"), 3);
	C4GUI::Tabular::Sheet *pSheetGamepad  = pOptionsTabular->AddSheet(LoadResStr("IDS_DLG_GAMEPAD") , 4);
	C4GUI::Tabular::Sheet *pSheetNetwork  = pOptionsTabular->AddSheet(LoadResStr("IDS_DLG_NETWORK") , 5);

	C4GUI::CheckBox *pCheck; C4GUI::Label *pLbl;
	int iCheckWdt=100, iCheckHgt=20, iEdit2Wdt=100, iEdit2Hgt=40;
	BoolConfig::GetStandardCheckBoxSize(&iCheckWdt, &iCheckHgt, "Default text", pUseFont);
	EditConfig::GetControlSize(&iEdit2Wdt, &iEdit2Hgt, "Default text", false);

	// --- page program
	C4GUI::ComponentAligner caSheetProgram(pSheetGeneral->GetClientRect(), caMain.GetWidth()/20, caMain.GetHeight()/20, true);
	// language
	const char *szLangTip = LoadResStr("IDS_MSG_SELECTLANG");
	C4GUI::ComponentAligner caLanguage(caSheetProgram.GetGridCell(0,1,0,7,-1,-1,true,1,2), 0, C4GUI_DefDlgSmallIndent, false);
	C4GUI::ComponentAligner caLanguageBox(caLanguage.GetFromTop(C4GUI::ComboBox::GetDefaultHeight()), 0, 0, false);
	StdStrBuf sLangStr; sLangStr.Copy(LoadResStr("IDS_CTL_LANGUAGE")); sLangStr.AppendChar(':');
	int32_t w,q;
	pUseFont->GetTextExtent(sLangStr.getData(), w,q,true);
	pLbl = new C4GUI::Label(sLangStr.getData(), caLanguageBox.GetFromLeft(w+C4GUI_DefDlgSmallIndent), ALeft, C4StartupFontClr, pUseFont, false);
	pLbl->SetToolTip(szLangTip);
	pSheetGeneral->AddElement(pLbl);
	pUseFont->GetTextExtent("XX: Top Secret Language", w,q,true);
	pLangCombo = new C4GUI::ComboBox(caLanguageBox.GetFromLeft(Min(w, caLanguageBox.GetWidth())));
	pLangCombo->SetToolTip(szLangTip);
	pLangCombo->SetComboCB(new C4GUI::ComboBox_FillCallback<C4StartupOptionsDlg>(this, &C4StartupOptionsDlg::OnLangComboFill, &C4StartupOptionsDlg::OnLangComboSelChange));
	pLangCombo->SetColors(C4StartupFontClr, C4StartupEditBGColor, C4StartupEditBorderColor);
	pLangCombo->SetFont(pUseFont);
	pLangCombo->SetDecoration(&(C4Startup::Get()->Graphics.fctContext));
	pSheetGeneral->AddElement(pLangCombo);
	pLangInfoLabel = new C4GUI::Label(NULL, caLanguage.GetFromTop(C4GUI::GetRes()->TextFont.GetLineHeight()*3), ALeft, C4StartupFontClr, pUseFont, false);
	pLangInfoLabel->SetToolTip(szLangTip);
	pSheetGeneral->AddElement(pLangInfoLabel);
	UpdateLanguage();
	// font
	const char *szFontTip = LoadResStr("IDS_DESC_SELECTFONT");
	C4GUI::ComponentAligner caFontBox(caSheetProgram.GetGridCell(0,1,2,7,-1,C4GUI::ComboBox::GetDefaultHeight(), true), 0, 0, false);
	StdStrBuf sFontStr; sFontStr.Copy(LoadResStr("IDS_CTL_FONT")); sFontStr.AppendChar(':');
	pUseFont->GetTextExtent(sFontStr.getData(), w,q,true);
	pLbl = new C4GUI::Label(sFontStr.getData(), caFontBox.GetFromLeft(w+C4GUI_DefDlgSmallIndent), ALeft, C4StartupFontClr, pUseFont, false);
	pLbl->SetToolTip(szFontTip);
	pSheetGeneral->AddElement(pLbl);
	pUseFont->GetTextExtent("Comic Sans MS", w,q,true);
	pFontFaceCombo = new C4GUI::ComboBox(caFontBox.GetFromLeft(Min<int32_t>(caFontBox.GetInnerWidth()*3/4, w*3)));
	pFontFaceCombo->SetToolTip(szFontTip);
	pFontFaceCombo->SetComboCB(new C4GUI::ComboBox_FillCallback<C4StartupOptionsDlg>(this, &C4StartupOptionsDlg::OnFontFaceComboFill, &C4StartupOptionsDlg::OnFontComboSelChange));
	pFontFaceCombo->SetColors(C4StartupFontClr, C4StartupEditBGColor, C4StartupEditBorderColor);
	pFontFaceCombo->SetFont(pUseFont);
	pFontFaceCombo->SetDecoration(&(C4Startup::Get()->Graphics.fctContext));
	caFontBox.ExpandLeft(-C4GUI_DefDlgSmallIndent);
	pSheetGeneral->AddElement(pFontFaceCombo);
	pFontSizeCombo = new C4GUI::ComboBox(caFontBox.GetFromLeft(Min<int32_t>(caFontBox.GetInnerWidth(), w)));
	pFontSizeCombo->SetToolTip(LoadResStr("IDS_DESC_FONTSIZE"));
	pFontSizeCombo->SetComboCB(new C4GUI::ComboBox_FillCallback<C4StartupOptionsDlg>(this, &C4StartupOptionsDlg::OnFontSizeComboFill, &C4StartupOptionsDlg::OnFontComboSelChange));
	pFontSizeCombo->SetColors(C4StartupFontClr, C4StartupEditBGColor, C4StartupEditBorderColor);
	pFontSizeCombo->SetFont(pUseFont);
	pFontSizeCombo->SetDecoration(&(C4Startup::Get()->Graphics.fctContext));
	pSheetGeneral->AddElement(pFontSizeCombo);
	UpdateFontControls();
	// MM timer
	pCheck = new BoolConfig(caSheetProgram.GetGridCell(0,1,3,7,-1,iCheckHgt, true), LoadResStr("IDS_CTL_MMTIMER"), NULL, &Config.General.MMTimer, true, &Config.Startup.HideMsgMMTimerChange);
	pCheck->SetToolTip(LoadResStr("IDS_MSG_MMTIMER_DESC"));
	pCheck->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
	pSheetGeneral->AddElement(pCheck);
	// startup video
#ifdef _WIN32
	pCheck = new BoolConfig(caSheetProgram.GetGridCell(0,1,4,7,-1,iCheckHgt, true), LoadResStr("IDS_MSG_STARTUPVIDEO"), NULL, &Config.Startup.NoSplash, true);
	pCheck->SetToolTip(LoadResStr("IDS_MSG_STARTUPVIDEO_DESC"));
	pCheck->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
	pSheetGeneral->AddElement(pCheck);
#endif
	// fair crew strength
	C4GUI::GroupBox *pGroupFairCrewStrength = new C4GUI::GroupBox(caSheetProgram.GetGridCell(0,2,5,7,-1,pUseFont->GetLineHeight()*2+iIndentY2*2+C4GUI_ScrollBarHgt, true, 1,2));
	pGroupFairCrewStrength->SetTitle(LoadResStr("IDS_CTL_FAIRCREWSTRENGTH"));
	pGroupFairCrewStrength->SetFont(pUseFont);
	pGroupFairCrewStrength->SetColors(C4StartupEditBorderColor, C4StartupFontClr);
	pSheetGeneral->AddElement(pGroupFairCrewStrength);
	C4GUI::ComponentAligner caGroupFairCrewStrength(pGroupFairCrewStrength->GetClientRect(), 1,0, true);
	StdStrBuf sLabelTxt; sLabelTxt.Copy(LoadResStr("IDS_CTL_FAIRCREWWEAK"));
	w=20; q=12; pUseFont->GetTextExtent(sLabelTxt.getData(), w,q, true);
	pGroupFairCrewStrength->AddElement(new C4GUI::Label(sLabelTxt.getData(), caGroupFairCrewStrength.GetFromLeft(w,q), ACenter, C4StartupFontClr, pUseFont, false, false));
	sLabelTxt.Copy(LoadResStr("IDS_CTL_FAIRCREWSTRONG"));
	pUseFont->GetTextExtent(sLabelTxt.getData(), w,q, true);
	pGroupFairCrewStrength->AddElement(new C4GUI::Label(sLabelTxt.getData(), caGroupFairCrewStrength.GetFromRight(w,q), ACenter, C4StartupFontClr, pUseFont, false, false));
	C4GUI::ParCallbackHandler<C4StartupOptionsDlg, int32_t> *pCB = new C4GUI::ParCallbackHandler<C4StartupOptionsDlg, int32_t>(this, &C4StartupOptionsDlg::OnFairCrewStrengthSliderChange);
	C4GUI::ScrollBar *pSlider = new C4GUI::ScrollBar(caGroupFairCrewStrength.GetCentered(caGroupFairCrewStrength.GetInnerWidth(), C4GUI_ScrollBarHgt), true, pCB, 101);
	pSlider->SetDecoration(&C4Startup::Get()->Graphics.sfctBookScroll, false);
	pGroupFairCrewStrength->SetToolTip(LoadResStr("IDS_DESC_FAIRCREWSTRENGTH"));
	pSlider->SetScrollPos(FairCrewStrength2Slider(Config.General.FairCrewStrength));
	pGroupFairCrewStrength->AddElement(pSlider);
	// reset configuration
	const char *szBtnText = LoadResStr("IDS_BTN_RESETCONFIG");
	C4GUI::CallbackButton<C4StartupOptionsDlg, SmallButton> *pSmallBtn;
	C4GUI::GetRes()->CaptionFont.GetTextExtent(szBtnText, iButtonWidth, iButtonHeight, true);
	C4Rect rcResetBtn = caSheetProgram.GetGridCell(1,2,6,7, Min<int32_t>(iButtonWidth+iButtonHeight*4, caSheetProgram.GetInnerWidth()*2/5), SmallButton::GetDefaultButtonHeight(), true);
	pSheetGeneral->AddElement(pSmallBtn = new C4GUI::CallbackButton<C4StartupOptionsDlg, SmallButton>(szBtnText, rcResetBtn, &C4StartupOptionsDlg::OnResetConfigBtn, this));
	pSmallBtn->SetToolTip(LoadResStr("IDS_DESC_RESETCONFIG"));

	// --- page graphics
	C4GUI::ComponentAligner caSheetGraphics(pSheetGraphics->GetClientRect(), iIndentX1, iIndentY1, true);
	// --subgroup engine
	C4GUI::GroupBox *pGroupEngine = new C4GUI::GroupBox(caSheetGraphics.GetGridCell(0,2,0,3));
	pGroupEngine->SetTitle(LoadResStrNoAmp("IDS_CTL_GFXENGINE"));
	pGroupEngine->SetFont(pUseFont);
	pGroupEngine->SetColors(C4StartupEditBorderColor, C4StartupFontClr);
	pGroupEngine->SetToolTip(LoadResStr("IDS_MSG_GFXENGINE_DESC"));
	pSheetGraphics->AddElement(pGroupEngine);
	C4GUI::ComponentAligner caGroupEngine(pGroupEngine->GetClientRect(), iIndentX1, iIndentY2, true);
	const char *szGfxEngineNames[3] = { "DirectX", "OpenGL", "DirectX Software" };
	C4GUI::BaseCallbackHandler *pGfxEngineCheckCB = new C4GUI::CallbackHandler<C4StartupOptionsDlg>(this, &C4StartupOptionsDlg::OnGfxEngineCheck);
	for (int32_t iGfxEngine = 0; iGfxEngine<3; ++iGfxEngine)
		{
		pCheckGfxEngines[iGfxEngine] = new C4GUI::CheckBox(caGroupEngine.GetGridCell(0,1,iGfxEngine,3,-1,iCheckHgt,true), szGfxEngineNames[iGfxEngine], (Config.Graphics.Engine == iGfxEngine));
		pCheckGfxEngines[iGfxEngine]->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
		pCheckGfxEngines[iGfxEngine]->SetOnChecked(pGfxEngineCheckCB);
		pGroupEngine->AddElement(pCheckGfxEngines[iGfxEngine]);
		}
#ifndef USE_DIRECTX
	pCheckGfxEngines[GFXENGN_DIRECTX]->SetEnabled(false);
	pCheckGfxEngines[GFXENGN_DIRECTXS]->SetEnabled(false);
#endif
#ifndef USE_GL
	pCheckGfxEngines[GFXENGN_OPENGL]->SetEnabled(false);
#endif
	pCheckGfxEngines[GFXENGN_DIRECTXS]->SetEnabled(false); // better not using this
	// --subgroup resolution
	C4GUI::GroupBox *pGroupResolution = new C4GUI::GroupBox(caSheetGraphics.GetGridCell(1,2,0,3));
	pGroupResolution->SetTitle(LoadResStrNoAmp("IDS_CTL_RESOLUTION"));
	pGroupResolution->SetFont(pUseFont);
	pGroupResolution->SetColors(C4StartupEditBorderColor, C4StartupFontClr);
	pSheetGraphics->AddElement(pGroupResolution);
	C4GUI::ComponentAligner caGroupResolution(pGroupResolution->GetClientRect(), iIndentX1, iIndentY2, true);
	// resolution combobox
	pUseFont->GetTextExtent("1600 x 1200", w,q,true); w = Min<int32_t>(caGroupResolution.GetInnerWidth(), w+40);
	C4GUI::ComboBox *pGfxResCombo = new C4GUI::ComboBox(caGroupResolution.GetGridCell(0,1,0,4,w,C4GUI::ComboBox::GetDefaultHeight(), true));
	pGfxResCombo->SetToolTip(LoadResStr("IDS_MSG_RESOLUTION_DESC"));
	pGfxResCombo->SetComboCB(new C4GUI::ComboBox_FillCallback<C4StartupOptionsDlg>(this, &C4StartupOptionsDlg::OnGfxResComboFill, &C4StartupOptionsDlg::OnGfxResComboSelChange));
	pGfxResCombo->SetColors(C4StartupFontClr, C4StartupEditBGColor, C4StartupEditBorderColor);
	pGfxResCombo->SetFont(pUseFont);
	pGfxResCombo->SetDecoration(&(C4Startup::Get()->Graphics.fctContext));
	pGfxResCombo->SetText(GetGfxResString(Config.Graphics.ResX, Config.Graphics.ResY).getData());
	pGroupResolution->AddElement(pGfxResCombo);
	// all resolutions checkbox
	pCheck = new C4GUI::CheckBox(caGroupResolution.GetGridCell(0,1,1,4,-1,iCheckHgt,true), LoadResStr("IDS_CTL_SHOWALLRESOLUTIONS"), !!Config.Graphics.ShowAllResolutions);
	pCheck->SetOnChecked(new C4GUI::CallbackHandler<C4StartupOptionsDlg>(this, &C4StartupOptionsDlg::OnGfxAllResolutionsChange));
	pCheck->SetToolTip(LoadResStr("IDS_DESC_SHOWALLRESOLUTIONS"));
	pCheck->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
#ifndef _WIN32
	pCheck->SetEnabled(false);
#endif
	pGroupResolution->AddElement(pCheck);
	// color depth checkboxes
	C4GUI::BaseCallbackHandler *pGfxClrDepthCheckCB = new C4GUI::CallbackHandler<C4StartupOptionsDlg>(this, &C4StartupOptionsDlg::OnGfxClrDepthCheck);
	for (int32_t iBitDepthIdx = 0; iBitDepthIdx<2; ++iBitDepthIdx)
		{
		int iBitDepth = (iBitDepthIdx+1) * 16;
		pCheckGfxClrDepth[iBitDepthIdx] = new C4GUI::CheckBox(caGroupEngine.GetGridCell(iBitDepthIdx,2,2,4,-1,iCheckHgt,true), FormatString("%d Bit", (int)iBitDepth).getData(), (Config.Graphics.BitDepth == iBitDepth));
		pCheckGfxClrDepth[iBitDepthIdx]->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
		pCheckGfxClrDepth[iBitDepthIdx]->SetOnChecked(pGfxClrDepthCheckCB);
		pCheckGfxClrDepth[iBitDepthIdx]->SetToolTip(LoadResStr("IDS_CTL_BITDEPTH"));
		pGroupResolution->AddElement(pCheckGfxClrDepth[iBitDepthIdx]);
		}
	// fullscreen checkbox
	pCheck = new C4GUI::CheckBox(caGroupResolution.GetGridCell(0,1,3,4,-1,iCheckHgt,true), LoadResStr("IDS_MSG_FULLSCREEN"), !Config.Graphics.Windowed);
	pCheck->SetOnChecked(new C4GUI::CallbackHandler<C4StartupOptionsDlg>(this, &C4StartupOptionsDlg::OnFullscreenChange));
	pCheck->SetToolTip(LoadResStr("IDS_MSG_FULLSCREEN_DESC"));
	pCheck->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
	pGroupResolution->AddElement(pCheck);
	// --subgroup troubleshooting
	pGroupTrouble = new C4GUI::GroupBox(caSheetGraphics.GetGridCell(0,1,1,3));
	pGroupTrouble->SetTitle(LoadResStrNoAmp("IDS_CTL_TROUBLE"));
	pGroupTrouble->SetFont(pUseFont);
	pGroupTrouble->SetColors(C4StartupEditBorderColor, C4StartupFontClr);
	pSheetGraphics->AddElement(pGroupTrouble);
	C4GUI::ComponentAligner caGroupTrouble(pGroupTrouble->GetClientRect(), iIndentX1, iIndentY2, true);
	C4GUI::BaseCallbackHandler *pGfxGroubleCheckCB = new C4GUI::CallbackHandler<C4StartupOptionsDlg>(this, &C4StartupOptionsDlg::OnGfxTroubleCheck);
	int32_t iNumGfxOptions = 6, iOpt=0;
	// Shaders
	pShaders = new C4GUI::CheckBox(caGroupTrouble.GetGridCell(0,2,iOpt++,iNumGfxOptions,-1,iCheckHgt,true), "Shaders", false);
	pShaders->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
	pShaders->SetToolTip("Shaders");
	pShaders->SetOnChecked(pGfxGroubleCheckCB);
	pGroupTrouble->AddElement(pShaders);
	// load values of currently selected engine for troubleshooting
	LoadGfxTroubleshoot();
	// --subgroup options
	iNumGfxOptions = 2; iOpt=0;
	C4GUI::GroupBox *pGroupOptions = new C4GUI::GroupBox(caSheetGraphics.GetGridCell(0,2,2,3));
	pGroupOptions->SetTitle(LoadResStrNoAmp("IDS_DLG_OPTIONS"));
	pGroupOptions->SetFont(pUseFont);
	pGroupOptions->SetColors(C4StartupEditBorderColor, C4StartupFontClr);
	pSheetGraphics->AddElement(pGroupOptions);
	C4GUI::ComponentAligner caGroupOptions(pGroupOptions->GetClientRect(), iIndentX1, iIndentY2, true);
	// add new crew portraits
	pCheck = new BoolConfig(caGroupOptions.GetGridCell(0,1,iOpt++,iNumGfxOptions,-1,iCheckHgt,true), LoadResStr("IDS_MSG_ADDPORTRAITS"), NULL, &Config.Graphics.AddNewCrewPortraits);
	pCheck->SetToolTip(LoadResStr("IDS_MSG_ADDPORTRAITS_DESC"));
	pCheck->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
	pGroupOptions->AddElement(pCheck);
	// store default portraits in crew
	pCheck = new BoolConfig(caGroupOptions.GetGridCell(0,1,iOpt++,iNumGfxOptions,-1,iCheckHgt,true), LoadResStr("IDS_MSG_STOREPORTRAITS"), NULL, &Config.Graphics.SaveDefaultPortraits);
	pCheck->SetToolTip(LoadResStr("IDS_DESC_STOREPORTRAITS"));
	pCheck->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
	pGroupOptions->AddElement(pCheck);
	// --subgroup effects
	C4GUI::GroupBox *pGroupEffects = new C4GUI::GroupBox(caSheetGraphics.GetGridCell(1,2,2,3));
	pGroupEffects->SetTitle(LoadResStrNoAmp("IDS_CTL_SMOKE"));
	pGroupEffects->SetFont(pUseFont);
	pGroupEffects->SetColors(C4StartupEditBorderColor, C4StartupFontClr);
	pSheetGraphics->AddElement(pGroupEffects);
	C4GUI::ComponentAligner caGroupEffects(pGroupEffects->GetClientRect(), iIndentX1, iIndentY2, true);
	iNumGfxOptions = 3; iOpt=0;
	// effects level slider
	C4GUI::ComponentAligner caEffectsLevel(caGroupEffects.GetGridCell(0,1,iOpt++,iNumGfxOptions), 1,0,false);
	StdStrBuf sEffectsTxt; sEffectsTxt.Copy(LoadResStr("IDS_CTL_SMOKELOW"));
	w=20; q=12; pUseFont->GetTextExtent(sEffectsTxt.getData(), w,q, true);
	pGroupEffects->AddElement(new C4GUI::Label(sEffectsTxt.getData(), caEffectsLevel.GetFromLeft(w,q), ACenter, C4StartupFontClr, pUseFont, false, false));
	sEffectsTxt.Copy(LoadResStr("IDS_CTL_SMOKEHI"));
	w=20; q=12; pUseFont->GetTextExtent(sEffectsTxt.getData(), w,q, true);
	pGroupEffects->AddElement(new C4GUI::Label(sEffectsTxt.getData(), caEffectsLevel.GetFromRight(w,q), ACenter, C4StartupFontClr, pUseFont, false, false));
	pEffectLevelSlider = new C4GUI::ScrollBar(caEffectsLevel.GetCentered(caEffectsLevel.GetInnerWidth(), C4GUI_ScrollBarHgt), true, new C4GUI::ParCallbackHandler<C4StartupOptionsDlg, int32_t>(this, &C4StartupOptionsDlg::OnEffectsSliderChange), 301);
	pEffectLevelSlider->SetDecoration(&C4Startup::Get()->Graphics.sfctBookScroll, false);
	pEffectLevelSlider->SetToolTip(LoadResStr("IDS_MSG_PARTICLES_DESC"));
	pEffectLevelSlider->SetScrollPos(Config.Graphics.SmokeLevel);
	pGroupEffects->AddElement(pEffectLevelSlider);
	// fire particles
	pCheck = new BoolConfig(caGroupEffects.GetGridCell(0,1,iOpt++,iNumGfxOptions,-1,iCheckHgt,true), LoadResStr("IDS_MSG_FIREPARTICLES"), NULL, &Config.Graphics.FireParticles);
	pCheck->SetToolTip(LoadResStr("IDS_MSG_FIREPARTICLES_DESC"));
	pCheck->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
	pGroupEffects->AddElement(pCheck);
	// high resolution landscape
	pCheck = new BoolConfig(caGroupEffects.GetGridCell(0,1,iOpt++,iNumGfxOptions,-1,iCheckHgt,true), LoadResStr("[!]High resolution landscape"), NULL, &Config.Graphics.HighResLandscape);
	pCheck->SetToolTip(LoadResStr("[!]An expensive effect."));
	pCheck->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
	pGroupEffects->AddElement(pCheck);

	// --- page sound
	C4GUI::ComponentAligner caSheetSound(pSheetSound->GetClientRect(), iIndentX1, iIndentY1, true);
	if (!C4GUI::CheckBox::GetStandardCheckBoxSize(&iCheckWdt, &iCheckHgt, "Lorem ipsum", pUseFont)) { iCheckWdt=100; iCheckHgt=20; }
	int32_t iGridWdt = iCheckWdt * 2, iGridHgt = iCheckHgt * 5/2;
	// --subgroup menu system sound
	C4GUI::GroupBox *pGroupFESound = new C4GUI::GroupBox(caSheetSound.GetGridCell(0,2,0,5, iGridWdt, iGridHgt, false, 1,2));
	pGroupFESound->SetTitle(LoadResStrNoAmp("IDS_CTL_FRONTEND"));
	pGroupFESound->SetFont(pUseFont);
	pGroupFESound->SetColors(C4StartupEditBorderColor, C4StartupFontClr);
	pSheetSound->AddElement(pGroupFESound);
	C4GUI::ComponentAligner caGroupFESound(pGroupFESound->GetClientRect(), iIndentX1, iIndentY2, true);
	// menu system music
	pCheck = pFEMusicCheck = new C4GUI::CheckBox(caGroupFESound.GetGridCell(0,1,0,2,-1,iCheckHgt,true), LoadResStr("IDS_CTL_MUSIC"), !!Config.Sound.FEMusic);
	pCheck->SetToolTip(LoadResStr("IDS_DESC_MENUMUSIC"));
	pCheck->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
	pCheck->SetOnChecked(new C4GUI::CallbackHandler<C4StartupOptionsDlg>(this, &C4StartupOptionsDlg::OnFEMusicCheck));
	pGroupFESound->AddElement(pCheck);
	// menu system sound effects
	pCheck = pFESoundCheck = new BoolConfig(caGroupFESound.GetGridCell(0,1,1,2,-1,iCheckHgt,true), LoadResStr("IDS_CTL_SOUNDFX"), NULL, &Config.Sound.FESamples);
	pCheck->SetToolTip(LoadResStr("IDS_DESC_MENUSOUND"));
	pCheck->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
	pGroupFESound->AddElement(pCheck);
	// --subgroup game sound
	C4GUI::GroupBox *pGroupRXSound = new C4GUI::GroupBox(caSheetSound.GetGridCell(1,2,0,5, iGridWdt, iGridHgt, false, 1,2));
	pGroupRXSound->SetTitle(LoadResStrNoAmp("IDS_CTL_GAME"));
	pGroupRXSound->SetFont(pUseFont);
	pGroupRXSound->SetColors(C4StartupEditBorderColor, C4StartupFontClr);
	pSheetSound->AddElement(pGroupRXSound);
	C4GUI::ComponentAligner caGroupRXSound(pGroupRXSound->GetClientRect(), iIndentX1, iIndentY2, true);
	// game music
	pCheck = new BoolConfig(caGroupRXSound.GetGridCell(0,1,0,2,-1,iCheckHgt,true), LoadResStr("IDS_CTL_MUSIC"), NULL, &Config.Sound.RXMusic);
	pCheck->SetToolTip(LoadResStr("IDS_DESC_GAMEMUSIC"));
	pCheck->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
	pGroupRXSound->AddElement(pCheck);
	// game sound effects
	pCheck = new C4GUI::CheckBox(caGroupRXSound.GetGridCell(0,1,1,2,-1,iCheckHgt,true), LoadResStr("IDS_CTL_SOUNDFX"), !!Config.Sound.RXSound);
	pCheck->SetToolTip(LoadResStr("IDS_DESC_GAMESOUND"));
	pCheck->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
	pCheck->SetOnChecked(new C4GUI::CallbackHandler<C4StartupOptionsDlg>(this, &C4StartupOptionsDlg::OnRXSoundCheck));
	pGroupRXSound->AddElement(pCheck);
	// -- subgroup volume
	C4GUI::GroupBox *pGroupVolume = new C4GUI::GroupBox(caSheetSound.GetGridCell(0,2,2,5, iGridWdt, iGridHgt, false, 2,3));
	pGroupVolume->SetTitle(LoadResStrNoAmp("IDS_BTN_VOLUME"));
	pGroupVolume->SetFont(pUseFont);
	pGroupVolume->SetColors(C4StartupEditBorderColor, C4StartupFontClr);
	pSheetSound->AddElement(pGroupVolume);
	C4GUI::ComponentAligner caGroupVolume(pGroupVolume->GetClientRect(), iIndentX1, iIndentY2, true);
	// volume sliders
	int32_t i;
	for (i=0; i<2; ++i)
		{
		C4GUI::ComponentAligner caVolumeSlider(caGroupVolume.GetGridCell(0,1,i,2, -1, pUseFont->GetLineHeight()+iIndentY2*2+C4GUI_ScrollBarHgt, true), 1,0,false);
		pGroupVolume->AddElement(new C4GUI::Label(FormatString("%s:", LoadResStr(i ? "IDS_CTL_SOUNDFX" : "IDS_CTL_MUSIC")).getData(), caVolumeSlider.GetFromTop(pUseFont->GetLineHeight()), ALeft, C4StartupFontClr, pUseFont, false, false));
		sLabelTxt.Copy(LoadResStr("IDS_CTL_SILENT"));
		w=20; q=12; pUseFont->GetTextExtent(sLabelTxt.getData(), w,q, true);
		pGroupVolume->AddElement(new C4GUI::Label(sLabelTxt.getData(), caVolumeSlider.GetFromLeft(w,q), ACenter, C4StartupFontClr, pUseFont, false, false));
		sLabelTxt.Copy(LoadResStr("IDS_CTL_LOUD"));
		pUseFont->GetTextExtent(sLabelTxt.getData(), w,q, true);
		pGroupVolume->AddElement(new C4GUI::Label(sLabelTxt.getData(), caVolumeSlider.GetFromRight(w,q), ACenter, C4StartupFontClr, pUseFont, false, false));
		C4GUI::ParCallbackHandler<C4StartupOptionsDlg, int32_t> *pCB = new C4GUI::ParCallbackHandler<C4StartupOptionsDlg, int32_t>(this, i ? &C4StartupOptionsDlg::OnSoundVolumeSliderChange : &C4StartupOptionsDlg::OnMusicVolumeSliderChange);
		C4GUI::ScrollBar *pSlider = new C4GUI::ScrollBar(caVolumeSlider.GetCentered(caVolumeSlider.GetInnerWidth(), C4GUI_ScrollBarHgt), true, pCB, 101);
		pSlider->SetDecoration(&C4Startup::Get()->Graphics.sfctBookScroll, false);
		pSlider->SetToolTip(i ? LoadResStr("IDS_DESC_VOLUMESOUND") : LoadResStr("IDS_DESC_VOLUMEMUSIC"));
		pSlider->SetScrollPos(i ? Config.Sound.SoundVolume : Config.Sound.MusicVolume);
		pGroupVolume->AddElement(pSlider);
		}

	// --- page keyboard controls
	pSheetKeyboard->AddElement(new ControlConfigArea(pSheetKeyboard->GetClientRect(), caMain.GetWidth()/20, caMain.GetHeight()/40, false, this));

	// --- page gamepad
	pSheetGamepad->AddElement(new ControlConfigArea(pSheetGamepad->GetClientRect(), caMain.GetWidth()/20, caMain.GetHeight()/40, true, this));

	// --- page network
	C4GUI::ComponentAligner caSheetNetwork(pSheetNetwork->GetClientRect(), caMain.GetWidth()/20, caMain.GetHeight()/20, true);
	int iPortCfgWdt=200, iPortCfgHgt=48; NetworkPortConfig::GetControlSize(&iPortCfgWdt, &iPortCfgHgt);
	pPortCfgTCP = new NetworkPortConfig(caSheetNetwork.GetGridCell(0,2,0,2, iPortCfgWdt, iPortCfgHgt), LoadResStr("IDS_NET_PORT_TCP"), &(Config.Network.PortTCP), C4NetStdPortTCP);
	pPortCfgUDP = new NetworkPortConfig(caSheetNetwork.GetGridCell(1,2,0,2, iPortCfgWdt, iPortCfgHgt), LoadResStr("IDS_NET_PORT_UDP"), &(Config.Network.PortUDP), C4NetStdPortUDP);
	pPortCfgRef = new NetworkPortConfig(caSheetNetwork.GetGridCell(0,2,1,2, iPortCfgWdt, iPortCfgHgt), LoadResStr("IDS_NET_PORT_REFERENCE"), &(Config.Network.PortRefServer), C4NetStdPortRefServer);
	pPortCfgDsc = new NetworkPortConfig(caSheetNetwork.GetGridCell(1,2,1,2, iPortCfgWdt, iPortCfgHgt), LoadResStr("IDS_NET_PORT_DISCOVERY"), &(Config.Network.PortDiscovery), C4NetStdPortDiscovery);
	pPortCfgTCP->SetToolTip(LoadResStr("IDS_NET_PORT_TCP_DESC"));
	pPortCfgUDP->SetToolTip(LoadResStr("IDS_NET_PORT_UDP_DESC"));
	pPortCfgRef->SetToolTip(LoadResStr("IDS_NET_PORT_REFERENCE_DESC"));
	pPortCfgDsc->SetToolTip(LoadResStr("IDS_NET_PORT_DISCOVERY_DESC"));
	pSheetNetwork->AddElement(pPortCfgTCP);
	pSheetNetwork->AddElement(pPortCfgUDP);
	pSheetNetwork->AddElement(pPortCfgRef);
	pSheetNetwork->AddElement(pPortCfgDsc);
	int iNetHgt0=pPortCfgDsc->GetBounds().GetBottom();
	caSheetNetwork.ExpandTop(-iNetHgt0);
	int iServerCfgWdt=120, iServerCfgHgt=20, iServerCfgWdtMid=0;
	StdStrBuf sServerText; sServerText.Copy(LoadResStr("IDS_CTL_USEOTHERSERVER"));
	NetworkServerAddressConfig::GetControlSize(&iServerCfgWdt, &iServerCfgHgt, &iServerCfgWdtMid, sServerText.getData());
	pLeagueServerCfg = new NetworkServerAddressConfig(caSheetNetwork.GetFromTop(iServerCfgHgt), sServerText.getData(), &(Config.Network.UseAlternateServer), Config.Network.AlternateServerAddress, iServerCfgWdtMid);
	pLeagueServerCfg->SetToolTip(LoadResStr("IDS_NET_MASTERSRV_DESC"));
	pSheetNetwork->AddElement(pLeagueServerCfg);
	pCheck = new BoolConfig(caSheetNetwork.GetFromTop(pUseFont->GetLineHeight()), LoadResStr("IDS_CTL_AUTOMATICUPDATES"), NULL, &Config.Network.AutomaticUpdate, false);
	pCheck->SetToolTip(LoadResStr("IDS_DESC_AUTOMATICUPDATES"));
	pCheck->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
	pSheetNetwork->AddElement(pCheck);
	const char *szNameCfgText = LoadResStr("IDS_NET_COMPUTERNAME");
	int iNameCfgWdt=200, iNameCfgHgt=48; C4StartupOptionsDlg::EditConfig::GetControlSize(&iNameCfgWdt, &iNameCfgHgt, szNameCfgText, false);
	iNameCfgWdt += 5;
	pNetworkNameEdit = new EditConfig(caSheetNetwork.GetGridCell(0,2,0,1, iNameCfgWdt, iNameCfgHgt), szNameCfgText, &Config.Network.LocalName, NULL, false);
	pNetworkNickEdit = new EditConfig(caSheetNetwork.GetGridCell(1,2,0,1, iNameCfgWdt, iNameCfgHgt), LoadResStr("IDS_NET_USERNAME"), &Config.Network.Nick, NULL, false);
	pNetworkNameEdit->SetToolTip(LoadResStr("IDS_NET_COMPUTERNAME_DESC"));
	pNetworkNickEdit->SetToolTip(LoadResStr("IDS_NET_USERNAME_DESC"));
	pSheetNetwork->AddElement(pNetworkNameEdit);
	pSheetNetwork->AddElement(pNetworkNickEdit);

	StdCopyStrBuf NickBuf(Config.Network.Nick);
	if(!NickBuf.getLength()) NickBuf.Copy(Config.GetRegistrationData("Nick"));
	if(!NickBuf.getLength()) NickBuf.Copy(Config.Network.LocalName);
	pNetworkNickEdit->GetEdit()->SetText(NickBuf.getData(), false);

	// initial focus is on tab selection
	SetFocus(pOptionsTabular, false);
	}

C4StartupOptionsDlg::~C4StartupOptionsDlg()
	{
	delete pKeyToggleMusic;
	delete pKeyBack;
	}

void C4StartupOptionsDlg::OnClosed(bool fOK)
	{
	// callback when dlg got closed - save config
	SaveConfig(true, false);
	C4StartupDlg::OnClosed(fOK);
	}

int32_t C4StartupOptionsDlg::FairCrewSlider2Strength(int32_t iSliderVal)
	{
	// slider linear in rank to value linear in exp
	return int32_t(pow(double(iSliderVal)/9.5, 1.5) * 1000.0);
	}

int32_t C4StartupOptionsDlg::FairCrewStrength2Slider(int32_t iStrengthVal)
	{
	// value linear in exp to slider linear in rank
	return int32_t(pow(double(iStrengthVal)/1000.0, 1.0/1.5) * 9.5);
	}

void C4StartupOptionsDlg::OnFairCrewStrengthSliderChange(int32_t iNewVal)
	{
	// fair crew strength determined by exponential fn
	Config.General.FairCrewStrength = FairCrewSlider2Strength(iNewVal);
	}

void C4StartupOptionsDlg::OnResetConfigBtn(C4GUI::Control *btn)
	{
	// confirmation
	StdStrBuf sWarningText; sWarningText.Copy(LoadResStr("IDS_MSG_PROMPTRESETCONFIG"));
	sWarningText.AppendChar('|');
	sWarningText.Append(LoadResStr("IDS_MSG_RESTARTCHANGECFG"));
	if (!GetScreen()->ShowMessageModal(sWarningText.getData(), LoadResStr("IDS_BTN_RESETCONFIG"), C4GUI::MessageDialog::btnYesNo, C4GUI::Ico_Notify))
		// user cancelled
		return;
	// reset cfg
	Config.Default();
	Config.fConfigLoaded = true;
	// engine must be restarted now, because some crucial fields such as resolution and used gfx engine do not match their initialization
	Application.Quit();
	}

void C4StartupOptionsDlg::OnGfxEngineCheck(C4GUI::Element *pCheckBox)
	{
	C4GUI::CheckBox *pCheck = static_cast<C4GUI::CheckBox *>(pCheckBox);
	// radiogroup: do not allow unchecking!
	if (!pCheck->GetChecked())
		{
		pCheck->SetChecked(true);
		return;
		}
	// get new engine
	int i;
	for (i=0; i<3; ++i) if (pCheck == pCheckGfxEngines[i]) break;
	if (i==3 || i == Config.Graphics.Engine) return;
	// okay, engine change
	pCheckGfxEngines[Config.Graphics.Engine]->SetChecked(false);
	StdStrBuf sTitle; sTitle.Copy(LoadResStrNoAmp("IDS_CTL_GFXENGINE"));
	GetScreen()->ShowMessage(LoadResStr("IDS_MSG_RESTARTCHANGECFG"), sTitle.getData(), C4GUI::Ico_Notify, &Config.Startup.HideMsgGfxEngineChange);
	SaveGfxTroubleshoot();
	Config.Graphics.Engine = i;
	LoadGfxTroubleshoot();
	}

void C4StartupOptionsDlg::OnGfxResComboFill(C4GUI::ComboBox_FillCB *pFiller)
	{
	// clear all old entries first to allow a clean refill
	pFiller->ClearEntries();
	// fill with all possible resolutions
	int32_t idx = 0, iXRes, iYRes, iBitDepth;
	while (Application.GetIndexedDisplayMode(idx++, &iXRes, &iYRes, &iBitDepth, Config.Graphics.Monitor))
#ifdef _WIN32 // why only WIN32?
		if (iBitDepth == Config.Graphics.BitDepth)
			if ((iXRes <= 1024 && iXRes>=600 && iYRes>=460) || Config.Graphics.ShowAllResolutions)
#endif
				{
				StdStrBuf sGfxString = GetGfxResString(iXRes, iYRes);
				if (!pFiller->FindEntry(sGfxString.getData()))
					pFiller->AddEntry(sGfxString.getData(), iXRes + (uint32_t(iYRes)<<16));
				}
	}

bool C4StartupOptionsDlg::OnGfxResComboSelChange(C4GUI::ComboBox *pForCombo, int32_t idNewSelection)
	{
	// get new resolution from string
	int iResX=(idNewSelection & 0xffff), iResY=(uint32_t(idNewSelection) & 0xffff0000) >> 16;
	// different than current?
	if (iResX == Config.Graphics.ResX && iResY == Config.Graphics.ResY) return true;
	// try setting it
	if (!TryNewResolution(iResX, iResY))
		{
		// didn't work or declined by user
		return true; // do not change label, because dialog might hae been recreated!
		}
	// dialog has been recreated; so do not change the combo label
	return true;
	}

bool C4StartupOptionsDlg::TryNewResolution(int32_t iResX, int32_t iResY)
	{
	int32_t iOldResX = Config.Graphics.ResX, iOldResY = Config.Graphics.ResY;
	int32_t iOldFontSize = Config.General.RXFontSize;
	C4GUI::Screen *pScreen = GetScreen();
	// resolution change may imply font size change
	int32_t iNewFontSize;
	if (iResX < 700)
		iNewFontSize = 12;
	else if (iResX < 950)
		iNewFontSize = 14; // default (at 800x600)
	else
		iNewFontSize = 16;
	// call application to set it
	if (!Application.SetVideoMode(iResX, iResY,Config.Graphics.BitDepth, Config.Graphics.Monitor,!Config.Graphics.Windowed))
		{
		StdCopyStrBuf strChRes(LoadResStr("IDS_MNU_SWITCHRESOLUTION"));
		pScreen->ShowMessage(FormatString(LoadResStr("IDS_ERR_SWITCHRES"), Application.GetLastError()).getData(), strChRes.getData(), C4GUI::Ico_Clonk, NULL);
		return false;
		}
	// implied font change
	if (iNewFontSize != iOldFontSize)
		if (!Application.SetGameFont(Config.General.RXFontName, iNewFontSize))
			{
			// not changing font size is not fatal - just keep old size
			iNewFontSize = iOldFontSize;
			}
	// since the resolution was changed, everything needs to be moved around a bit
	RecreateDialog(false);
	ResChangeConfirmDlg *pConfirmDlg = new ResChangeConfirmDlg();
	if (!pScreen->ShowModalDlg(pConfirmDlg, true))
		{
		// abort: Restore screen, if this was not some program abort
		if (C4GUI::IsGUIValid())
			{
			if (Application.SetVideoMode(iOldResX, iOldResY, Config.Graphics.BitDepth, Config.Graphics.Monitor,!Config.Graphics.Windowed))
				{
				if (iNewFontSize != iOldFontSize) Application.SetGameFont(Config.General.RXFontName, iOldFontSize);
				RecreateDialog(false);
				}
			}
		else
			{
			// make sure config is restored even if the program is closed during the confirmation dialog
			Config.Graphics.ResX = iOldResX, Config.Graphics.ResY = iOldResY;
			}
		return false;
		}
	// resolution may be kept!
	Config.Graphics.ResX = iResX;
	Config.Graphics.ResY = iResY;
	return true;
	}

StdStrBuf C4StartupOptionsDlg::GetGfxResString(int32_t iResX, int32_t iResY)
	{
	// Display in format like "640 x 480"
	return FormatString("%d x %d", (int)iResX, (int)iResY);
	}

void C4StartupOptionsDlg::OnGfxClrDepthCheck(C4GUI::Element *pCheckBox)
	{
	// get clr depth being checked
	int32_t i;
	for (i=0; i<2; ++i) if (pCheckBox == pCheckGfxClrDepth[i]) break;
	if (i==2) return;
	// do not allow unchecking
	if (!pCheckGfxClrDepth[i]->GetChecked())
		{
		pCheckGfxClrDepth[i]->SetChecked(true);
		return;
		}
	// change check to this one
	int32_t iCurrIdx = BoundBy<int32_t>(Config.Graphics.BitDepth / 16-1,0,1);
	pCheckGfxClrDepth[iCurrIdx]->SetChecked(false);
	pCheckGfxClrDepth[i]->SetChecked(true);
	// change config
	Config.Graphics.BitDepth = (i+1)*16;
	// notify user that he has to restart to see any changes
	StdStrBuf sTitle; sTitle.Copy(LoadResStrNoAmp("IDS_CTL_BITDEPTH"));
	GetScreen()->ShowMessage(LoadResStr("IDS_MSG_RESTARTCHANGECFG"), sTitle.getData(), C4GUI::Ico_Notify, &Config.Startup.HideMsgGfxBitDepthChange);
	}

void C4StartupOptionsDlg::OnFullscreenChange(C4GUI::Element *pCheckBox)
	{
	Config.Graphics.Windowed = !static_cast<C4GUI::CheckBox *>(pCheckBox)->GetChecked();
	Application.SetVideoMode(Config.Graphics.ResX, Config.Graphics.ResY, Config.Graphics.BitDepth, Config.Graphics.Monitor, !Config.Graphics.Windowed);
	}

void C4StartupOptionsDlg::OnGfxAllResolutionsChange(C4GUI::Element *pCheckBox)
	{
	Config.Graphics.ShowAllResolutions = static_cast<C4GUI::CheckBox *>(pCheckBox)->GetChecked();
	}

bool C4StartupOptionsDlg::SaveConfig(bool fForce, bool fKeepOpen)
	{
	// prevent double save
	if (fConfigSaved) return true;
	// save any config fields that are not stored directly; return whether all values are OK
	// check port validity
	if (!fForce)
		{
		StdCopyStrBuf strError(LoadResStr("IDS_ERR_CONFIG"));
		if (pPortCfgTCP->GetPort()>0 && pPortCfgTCP->GetPort() == pPortCfgRef->GetPort())
			{
			GetScreen()->ShowMessage(LoadResStr("IDS_NET_ERR_PORT_TCPREF"), strError.getData(), C4GUI::Ico_Error);
			return false;
			}
		if (pPortCfgUDP->GetPort()>0 && pPortCfgUDP->GetPort() == pPortCfgDsc->GetPort())
			{
			GetScreen()->ShowMessage(LoadResStr("IDS_NET_ERR_PORT_UDPDISC"), strError.getData(), C4GUI::Ico_Error);
			return false;
			}
		}
	// store some config values
	SaveGfxTroubleshoot();
	pPortCfgTCP->SavePort();
	pPortCfgUDP->SavePort();
	pPortCfgRef->SavePort();
	pPortCfgDsc->SavePort();
	pLeagueServerCfg->Save2Config();
	pNetworkNameEdit->Save2Config();
	pNetworkNickEdit->Save2Config();
	// if nick is same as default by registry, don't save in config
	// so regkey updates will change the nick as well
	const char *szRegNick = Config.GetRegistrationData("Nick");
	if (!szRegNick || !*szRegNick) szRegNick = Config.Network.LocalName.getData();
	if (SEqual(Config.Network.Nick.getData(), szRegNick)) Config.Network.Nick.Clear();
	// make sure config is saved, in case the game crashes later on or another instance is started
	Config.Save();
	if (!fKeepOpen) fConfigSaved = true;
	// done; config OK
	return true;
	}

void C4StartupOptionsDlg::DoBack()
	{
	if (!SaveConfig(false, false)) return;
	// back 2 main
	C4Startup::Get()->SwitchDialog(fCanGoBack ? (C4Startup::SDID_Back) : (C4Startup::SDID_Main));
	}

void C4StartupOptionsDlg::UpdateLanguage()
	{
	// find currently specified language in language list and display its info
	C4LanguageInfo *pNfo = Languages.FindInfo(Config.General.Language);
	if (pNfo)
		{
		pLangCombo->SetText(FormatString("%s - %s", pNfo->Code, pNfo->Name).getData());
		pLangInfoLabel->SetText(pNfo->Info);
		}
	else
		{
		pLangCombo->SetText(FormatString("unknown (%s)", Config.General.Language).getData());
		pLangInfoLabel->SetText(LoadResStr("IDS_CTL_NOLANGINFO"));
		return; // no need to mess with fallbacks
		}
	// update language fallbacks
	char *szLang = Config.General.LanguageEx;
	SCopy(pNfo->Code, szLang);
	if (*(pNfo->Fallback))
		{
		SAppend(",", szLang);
		Config.General.GetLanguageSequence(pNfo->Fallback, szLang + SLen(szLang));
		}
	// internal fallbacks
	if (!SSearch(Config.General.LanguageEx, "US"))
		{
		if (*szLang) SAppendChar(',', szLang);
		SAppend("US", szLang);
		}
	if (!SSearch(Config.General.LanguageEx, "DE"))
		{
		if (*szLang) SAppendChar(',', szLang);
		SAppend("DE", szLang);
		}
	}

void C4StartupOptionsDlg::OnLangComboFill(C4GUI::ComboBox_FillCB *pFiller)
	{
	// fill with all possible languages
	C4LanguageInfo *pNfo;
	for(int i=0; i<Languages.GetInfoCount(); ++i)
		if ((pNfo = Languages.GetInfo(i)))
			pFiller->AddEntry(FormatString("%s - %s", pNfo->Code, pNfo->Name).getData(), (unsigned char)(pNfo->Code[0]) + ((unsigned char)(pNfo->Code[1])<<8));
	}

bool C4StartupOptionsDlg::OnLangComboSelChange(C4GUI::ComboBox *pForCombo, int32_t idNewSelection)
	{
	// set new language by two-character-code
	Config.General.Language[0] = idNewSelection & 0xff;
	Config.General.Language[1] = (idNewSelection & 0xff00) >> 8;
	Config.General.Language[2] = '\0';
	UpdateLanguage();
	Languages.LoadLanguage(Config.General.LanguageEx);
	Game.UpdateLanguage();
	// recreate everything to reflect language changes
	RecreateDialog(true);
	return true;
	}

void C4StartupOptionsDlg::UpdateFontControls()
	{
	// display current language and size in comboboxes
	pFontFaceCombo->SetText(Config.General.RXFontName);
	StdStrBuf sSize; sSize.Format("%d", (int) Config.General.RXFontSize);
	pFontSizeCombo->SetText(sSize.getData());
	}

const char *DefaultFonts[] = { "Arial Unicode MS", "Comic Sans MS", "Endeavour", "Verdana", NULL };

void C4StartupOptionsDlg::OnFontFaceComboFill(C4GUI::ComboBox_FillCB *pFiller)
	{
	// 2do: enumerate Fonts.txt fonts; then enumerate system fonts
	for (int32_t i=0; DefaultFonts[i]; ++i) pFiller->AddEntry(DefaultFonts[i], i);
	}

void C4StartupOptionsDlg::OnFontSizeComboFill(C4GUI::ComboBox_FillCB *pFiller)
	{
	// 2do: enumerate possible font sizes by the font here
	// 2do: Hide font sizes that would be too large for the selected resolution
	pFiller->AddEntry("8", 8);
	pFiller->AddEntry("10", 10);
	pFiller->AddEntry("12", 12);
	pFiller->AddEntry("14", 14);
	pFiller->AddEntry("16", 16);
	pFiller->AddEntry("18", 18);
	pFiller->AddEntry("20", 20);
	pFiller->AddEntry("24", 24);
	pFiller->AddEntry("28", 28);
	//pFiller->AddEntry("32", 32);
	//pFiller->AddEntry("48", 48);
	}

bool C4StartupOptionsDlg::OnFontComboSelChange(C4GUI::ComboBox *pForCombo, int32_t idNewSelection)
	{
	// set new value
	const char *szNewFontFace = Config.General.RXFontName;
	int32_t iNewFontSize = Config.General.RXFontSize;
	if (pForCombo == pFontFaceCombo)
		szNewFontFace = DefaultFonts[idNewSelection];
	else if (pForCombo == pFontSizeCombo)
		iNewFontSize = idNewSelection;
	else
		// can't happen
		return true;
	// set new fonts
	if (!Application.SetGameFont(szNewFontFace, iNewFontSize))
		{
		GetScreen()->ShowErrorMessage(LoadResStr("IDS_ERR_INITFONTS"));
		return true;
		}
	// recreate everything to reflect font changes
	RecreateDialog(true);
	return true;
	}

void C4StartupOptionsDlg::RecreateDialog(bool fFade)
	{
	// MUST fade for now, or calling function will fail because dialog is deleted immediately
	fFade = true;
	// this actually breaks the possibility to go back :(
	int32_t iPage = pOptionsTabular->GetActiveSheetIndex();
	C4StartupOptionsDlg *pNewDlg = static_cast<C4StartupOptionsDlg *>(C4Startup::Get()->SwitchDialog(C4Startup::SDID_Options, fFade));
	pNewDlg->pOptionsTabular->SelectSheet(iPage, false);
	pNewDlg->fCanGoBack = false;
	}

void C4StartupOptionsDlg::LoadGfxTroubleshoot()
	{
	pShaders->SetChecked(!!DDrawCfg.Shader);
	// title of troubleshooting-box
	pGroupTrouble->SetTitle(LoadResStrNoAmp("IDS_CTL_TROUBLE"));
	}

void C4StartupOptionsDlg::SaveGfxTroubleshoot()
	{
	// get it from controls
	DDrawCfg.Shader=pShaders->GetChecked();
	// get config set to be used
	bool fUseGL = (Config.Graphics.Engine == GFXENGN_OPENGL);
	// and apply them directly, if the engine is current
	if (fUseGL == lpDDraw->IsOpenGL())
		{
		lpDDraw->RestoreDeviceObjects();
		}
	}

void C4StartupOptionsDlg::OnEffectsSliderChange(int32_t iNewVal)
	{
	Config.Graphics.SmokeLevel = iNewVal;
	}

void C4StartupOptionsDlg::OnFEMusicCheck(C4GUI::Element *pCheckBox)
	{
	// option change is reflected immediately
	bool fIsOn = static_cast<C4GUI::CheckBox *>(pCheckBox)->GetChecked();
	if ((Config.Sound.FEMusic = fIsOn))
		Application.MusicSystem.Play();
	else
		Application.MusicSystem.Stop();
	}

void C4StartupOptionsDlg::OnMusicVolumeSliderChange(int32_t iNewVal)
	{
	// option change is reflected immediately;
	Application.MusicSystem.SetVolume(Config.Sound.MusicVolume = iNewVal);
	}

void C4StartupOptionsDlg::OnSoundVolumeSliderChange(int32_t iNewVal)
	{
	// sound system reads this value directly
	Config.Sound.SoundVolume = iNewVal;
	// test sound
	StartSoundEffect("ArrowHit", false, 100, NULL);
	}

void C4StartupOptionsDlg::OnRXSoundCheck(C4GUI::Element *pCheckBox)
	{
	// toggling sounds on off must init/deinit sound system
	bool fIsOn = static_cast<C4GUI::CheckBox *>(pCheckBox)->GetChecked();
	if (fIsOn == !!Config.Sound.RXSound) return;
	if (fIsOn)
		{
		Config.Sound.RXSound = true;
		if (!Application.SoundSystem.Init())
			{
			GetScreen()->ShowMessage(StdCopyStrBuf(LoadResStr("IDS_PRC_NOSND")).getData(), StdCopyStrBuf(LoadResStr("IDS_DLG_LOG")).getData(), C4GUI::Ico_Error);
			Application.SoundSystem.Clear();
			Config.Sound.RXSound = false;
			static_cast<C4GUI::CheckBox *>(pCheckBox)->SetChecked(false);
			fIsOn = false;
			}
		}
	else
		{
		Application.SoundSystem.Clear();
		Config.Sound.RXSound = false;
		}
	// FE sound only enabled if global sound is enabled
	pFESoundCheck->SetEnabled(fIsOn);
	pFESoundCheck->SetChecked(fIsOn ? !!Config.Sound.FESamples : false);
	}

bool C4StartupOptionsDlg::KeyMusicToggle()
	{
	// do toggle
	Application.MusicSystem.ToggleOnOff();
	// reflect in checkbox
	pFEMusicCheck->SetChecked(!!Config.Sound.FEMusic);
	// key processed
	return true;
	}
