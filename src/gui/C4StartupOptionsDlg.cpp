/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2008, 2011  Sven Eberhardt
 * Copyright (c) 2006, 2008, 2010  Günther Brammer
 * Copyright (c) 2006  Florian Groß
 * Copyright (c) 2007  Matthes Bender
 * Copyright (c) 2007  Julian Raschke
 * Copyright (c) 2008, 2010  Armin Burgmeier
 * Copyright (c) 2009  Carli@Carli-PC
 * Copyright (c) 2010  Benjamin Herr
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
#include <C4MouseControl.h>

#include <C4DrawGL.h>

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
	pDraw->DrawQuadDw(cgo.Surface, iDrawQuadTop, dwClrHigh,dwClrHigh,dwClrHigh,dwClrHigh);
	pDraw->DrawQuadDw(cgo.Surface, iDrawQuadLeft, dwClrHigh,dwClrHigh,dwClrHigh,dwClrHigh);
	pDraw->DrawQuadDw(cgo.Surface, iDrawQuadRight, dwClrLow,dwClrLow,dwClrLow,dwClrLow);
	pDraw->DrawQuadDw(cgo.Surface, iDrawQuadBottom, dwClrLow,dwClrLow,dwClrLow,dwClrLow);
	//pDraw->DrawFrameDw(cgo.Surface, x0+1, y0+1, x1-1, y1-1, aC4StartupBtnBorderColor2);
	// draw selection highlight
	int32_t iTxtOff = fDown ? iIndent : 0;
	if (fEnabled) if (HasDrawFocus() || (fMouseOver && IsInActiveDlg(false)))
		{
			pDraw->SetBlitMode(C4GFXBLIT_ADDITIVE);
			::GraphicsResource.fctButtonHighlight.DrawX(cgo.Surface, x0+5+iTxtOff, y0+3+iTxtOff, rcBounds.Wdt-10, rcBounds.Hgt-6);
			pDraw->ResetBlitMode();
		}
	// draw button text
	pDraw->TextOut(sText.getData(), rUseFont, 1.0f, cgo.Surface, (x0+x1)/2 + iTxtOff, (y0+y1-iTextHgt)/2 + iTxtOff, C4StartupBtnFontClr, ACenter, true);
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
	if (SEqualNoCase(Config.GetRegistrationData("Nick"), "flgr")) iResChangeSwitchTime *= 10;
	// get positions
	C4GUI::ComponentAligner caMain(GetClientRect(), C4GUI_DefDlgIndent, C4GUI_DefDlgIndent, true);
	// place icon
	C4Rect rcIcon = caMain.GetFromLeft(C4GUI_IconWdt); rcIcon.Hgt = C4GUI_IconHgt;
	C4GUI::Icon *pIcon = new C4GUI::Icon(rcIcon, C4GUI::Ico_Confirm); AddElement(pIcon);
	// place message labels
	// use text with line breaks
	StdStrBuf sMsgBroken;
	int iMsgHeight = ::GraphicsResource.TextFont.BreakMessage(LoadResStr("IDS_MNU_SWITCHRESOLUTION_LIKEIT"), caMain.GetInnerWidth(), &sMsgBroken, true);
	C4GUI::Label *pLblMessage = new C4GUI::Label(sMsgBroken.getData(), caMain.GetFromTop(iMsgHeight), ACenter, C4GUI_MessageFontClr, &::GraphicsResource.TextFont, false);
	AddElement(pLblMessage);
	iMsgHeight = ::GraphicsResource.TextFont.BreakMessage(FormatString(LoadResStr("IDS_MNU_SWITCHRESOLUTION_UNDO"),
	             (int)iResChangeSwitchTime).getData(),
	             caMain.GetInnerWidth(), &sMsgBroken, true);
	pOperationCancelLabel = new C4GUI::Label(sMsgBroken.getData(), caMain.GetFromTop(iMsgHeight), ACenter, C4GUI_MessageFontClr, &::GraphicsResource.TextFont, false, false);
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
	::GraphicsResource.TextFont.BreakMessage(FormatString(LoadResStr("IDS_MNU_SWITCHRESOLUTION_UNDO"),
	                                       (int)iResChangeSwitchTime).getData(),
	                                       pOperationCancelLabel->GetBounds().Wdt, &sTimerText, true);
	pOperationCancelLabel->SetText(sTimerText.getData());
}

// ------------------------------------------------
// --- C4StartupOptionsDlg::KeySelDialog

C4StartupOptionsDlg::KeySelDialog::KeySelDialog(const C4PlayerControlAssignment *assignment, const C4PlayerControlAssignmentSet *assignment_set)
	: C4GUI::MessageDialog(GetDlgMessage(assignment, assignment_set).getData(), LoadResStr("IDS_MSG_DEFINEKEY"), C4GUI::MessageDialog::btnAbort | C4GUI::MessageDialog::btnReset, GetDlgIcon(assignment_set), C4GUI::MessageDialog::dsRegular),
		key(KEY_Undefined), assignment(assignment), assignment_set(assignment_set)
{
	pKeyListener = new C4KeyBinding(C4KeyCodeEx(KEY_Any, KEYS_None), "DefineKey", KEYSCOPE_Gui, new C4GUI::DlgKeyCBPassKey<C4StartupOptionsDlg::KeySelDialog>(*this, &C4StartupOptionsDlg::KeySelDialog::KeyDown), C4CustomKey::PRIO_PlrControl);
}

StdStrBuf C4StartupOptionsDlg::KeySelDialog::GetDlgMessage(const C4PlayerControlAssignment *assignment, const C4PlayerControlAssignmentSet *assignment_set)
{
	// compose message asking for key, gamepad button and/or mouse button depending on used control set
	if (!assignment || !assignment_set) return StdStrBuf("err");
	StdStrBuf result_string;
	if (assignment_set->HasGamepad())
		result_string.Take(FormatString(LoadResStr("IDS_MSG_PRESSBTN"), assignment->GetGUIName(Game.PlayerControlDefs)));
	else
		result_string.Take(FormatString(LoadResStr("IDS_MSG_PRESSKEY"), assignment->GetGUIName(Game.PlayerControlDefs)));
	const char *ctrl_desc = assignment->GetGUIDesc(Game.PlayerControlDefs);

	if (ctrl_desc && *ctrl_desc)
	{
		result_string.Append("||");
		result_string.Append(ctrl_desc);
	}
	return result_string;
}

C4GUI::Icons C4StartupOptionsDlg::KeySelDialog::GetDlgIcon(const C4PlayerControlAssignmentSet *assignment_set)
{
	if (!assignment_set) return C4GUI::Ico_Error;
	if (assignment_set->HasGamepad()) return C4GUI::Ico_Gamepad;
	return C4GUI::Ico_Keyboard;
}



C4StartupOptionsDlg::KeySelDialog::~KeySelDialog()
{
	delete pKeyListener;
}

bool C4StartupOptionsDlg::KeySelDialog::KeyDown(const C4KeyCodeEx &key)
{
	// safety
	if (!assignment || !assignment_set) return false;
	// check if key is valid for this set
	if (Key_IsGamepad(key.Key))
	{
		if (!assignment_set->HasGamepad()) return false;
	}
	else if (Key_IsMouse(key.Key))
	{
		if (!assignment_set->HasMouse()) return false;
	}
	else
	{
		if (!assignment_set->HasKeyboard()) return false;
	}
	// okay, use it
	this->key=key;
	Close(true);
	return true;
}


// ------------------------------------------------
// --- C4StartupOptionsDlg::ControlConfigListBox::ControlAssignmentLabel

C4StartupOptionsDlg::ControlConfigListBox::ControlAssignmentLabel::ControlAssignmentLabel(class C4PlayerControlAssignment *assignment, class C4PlayerControlAssignmentSet *assignment_set, const C4Rect &bounds)
	: C4GUI::Label("", bounds, ALeft, 0xffffffff, NULL, false, false, false), assignment(assignment), assignment_set(assignment_set)
{
	UpdateAssignmentString();
}

void C4StartupOptionsDlg::ControlConfigListBox::ControlAssignmentLabel::MouseInput(C4GUI::CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam)
	{
	// left-click to change key
	if (iButton == C4MC_Button_LeftDown && assignment)
	{
		if(!assignment->IsGUIDisabled())
		{
			KeySelDialog *dlg = new KeySelDialog(assignment, assignment_set);
			dlg->SetDelOnClose(false);
			bool success = GetScreen()->ShowModalDlg(dlg, false);
			C4KeyCodeEx key = dlg->GetKeyCode();
			delete dlg;
			if (success)
			{
				// dialog closed by pressing a key or by the Reset button (in which case, key==KEY_Undefined)
				// assign new config
				C4StartupOptionsDlg::ControlConfigListBox::SetUserKey(assignment_set, assignment, key);
				UpdateAssignmentString();
			}
		}
	}
	// inherited
	Element::MouseInput(rMouse, iButton, iX, iY, dwKeyParam);
	}

void C4StartupOptionsDlg::ControlConfigListBox::ControlAssignmentLabel::UpdateAssignmentString()
{
	// assignment label text from assigned key
	StdStrBuf strKey;
	C4KeyCodeEx key(0);
	if (assignment)
	{
		SetText(assignment->GetKeysAsString(true, false).getData());
	}
	else
	{
		SetText("");
	}
	DWORD color = C4GUI_CaptionFontClr;
	if (assignment)
	{
		if(assignment->IsGUIDisabled())
			color = C4GUI_InactCaptionFontClr;
		else if(assignment->IsKeyChanged())
			color = C4GUI_Caption2FontClr;
	}
	SetColor(color);
}

// ------------------------------------------------
// --- C4StartupOptionsDlg::ControlConfigListBox::ListItem

C4StartupOptionsDlg::ControlConfigListBox::ListItem::ListItem(ControlConfigListBox *parent_list, class C4PlayerControlAssignment *assignment, class C4PlayerControlAssignmentSet *assignment_set, bool has_extra_spacing)
	: C4GUI::Window(), parent_list(parent_list), assignment_label(NULL), has_extra_spacing(has_extra_spacing)
{
	int32_t margin = 2;
	// adding to listbox will size the element horizontally and move to proper position
	int32_t height = ::GraphicsResource.TextFont.GetLineHeight() + 2 * margin;
	SetBounds(C4Rect(0,0,42,height));
	parent_list->InsertElement(this, NULL);
	int32_t name_col_width = GetBounds().Wdt * 2/3;
	// child elements: two labels for two columns
	const char *gui_name = assignment->GetGUIName(Game.PlayerControlDefs);
	const char *gui_desc = assignment->GetGUIDesc(Game.PlayerControlDefs);
	C4GUI::Label *name_label = new C4GUI::Label(gui_name ? gui_name : "?undefined?", margin, margin);
	C4Rect assignment_label_bounds = C4Rect(name_col_width + margin, margin, GetBounds().Wdt - name_col_width - margin, GetBounds().Hgt - 2 * margin);
	assignment_label = new ControlAssignmentLabel(assignment, assignment_set, assignment_label_bounds);
	AddElement(name_label);
	AddElement(assignment_label);
	if (gui_desc && *gui_desc) SetToolTip(gui_desc);
}


// ------------------------------------------------
// --- C4StartupOptionsDlg::ControlConfigListBox

C4StartupOptionsDlg::ControlConfigListBox::ControlConfigListBox(const C4Rect &rcBounds, class C4PlayerControlAssignmentSet *set)
	: C4GUI::ListBox(rcBounds), set(NULL)
{
	SetAssignmentSet(set);
}

void C4StartupOptionsDlg::ControlConfigListBox::SetAssignmentSet(class C4PlayerControlAssignmentSet *new_set)
{
	set = new_set;
	// clear previous elements
	if (pClientWindow) pClientWindow->ClearChildren();
	// populate with new assignments
	if (set)
	{
		C4PlayerControlAssignment *assignment;
		
		std::vector<C4PlayerControlAssignment *> grouped_assignments;
		for (int32_t i=0; assignment = set->GetAssignmentByIndex(i); ++i)
			grouped_assignments.push_back(assignment);

		std::stable_sort(grouped_assignments.begin(),grouped_assignments.end(),&C4StartupOptionsDlg::ControlConfigListBox::sort_by_group);

		int32_t current_group = 0;
		for (std::vector<C4PlayerControlAssignment *>::iterator i = grouped_assignments.begin(); i != grouped_assignments.end(); ++i)
		{
			assignment = *i;
			bool first_element_of_group = assignment->GetGUIGroup() > current_group;
			current_group = assignment->GetGUIGroup();
			// only show assignments of GUI-named controls
			const char *gui_name = assignment->GetGUIName(Game.PlayerControlDefs);
			if (gui_name && *gui_name)
			{
				ListItem *element = new ListItem(this, assignment, set, first_element_of_group);
				AddElement(element);
			}
		}
	}
}

void C4StartupOptionsDlg::ControlConfigListBox::SetUserKey(class C4PlayerControlAssignmentSet *assignment_set, class C4PlayerControlAssignment *assignment, C4KeyCodeEx &key)
{
	// change key in the specified assignment set to the specified value
	// also changes config values so change is kept after restart
	// safety
	if (!assignment || !assignment_set) return;
	class C4PlayerControlAssignmentSet *config_set = Config.Controls.UserSets.GetSetByName(assignment_set->GetName());
	// change key
	if (key.Key == KEY_Undefined)
	{
		// reset to default
		assignment->ResetKeyToInherited();
		// also reset in config
		if (config_set)
		{
			config_set->RemoveAssignmentByControlName(assignment->GetControlName());
			if (!config_set->GetAssignmentByIndex(0))
			{
				// if config set is empty, no overrides exist and the set can be deleted (unless it's a custom config set)
				if (Game.PlayerControlDefaultAssignmentSets.GetSetByName(config_set->GetName()))
				{
					Config.Controls.UserSets.RemoveSetByName(assignment_set->GetName());
				}
			}
		}
	}
	else
	{
		// set to specified value
		assignment->SetKey(key);
		// also set in config
		if (!config_set) config_set = Config.Controls.UserSets.CreateEmptySetByTemplate(*assignment_set);
		C4PlayerControlAssignment *config_assignment = config_set->GetAssignmentByControlName(assignment->GetControlName());
		if (!config_assignment) config_assignment = config_set->CreateAssignmentForControl(assignment->GetControlName());
		config_assignment->SetKey(key);
	}
}

// ------------------------------------------------
// --- C4StartupOptionsDlg::ControlConfigArea

C4StartupOptionsDlg::ControlConfigArea::ControlConfigArea(const C4Rect &rcArea, int32_t iHMargin, int32_t iVMargin, bool fGamepad, C4StartupOptionsDlg *pOptionsDlg)
		: C4GUI::Window(), fGamepad(fGamepad), pGamepadOpener(NULL), pOptionsDlg(pOptionsDlg), pGUICtrl(NULL)
{
	CStdFont *pUseFont = &(C4Startup::Get()->Graphics.BookFont);
	CStdFont *pUseFontSmall = &(C4Startup::Get()->Graphics.BookSmallFont);
	SetBounds(rcArea);
	C4GUI::ComponentAligner caArea(rcArea, iHMargin, iVMargin, true);
	// get number of control sets to be configured
	C4PlayerControlAssignmentSets &assignment_sets = Game.PlayerControlUserAssignmentSets;
	iMaxControlSets = Max<size_t>(assignment_sets.GetSetCount(),1u); // do not devide by zero
	ppKeyControlSetBtns = new C4GUI::IconButton *[iMaxControlSets];
	// top line buttons to select control configuration
	C4Facet fctCtrlDefPic = ::GraphicsResource.fctKeyboard; // 
	int32_t iCtrlSetWdt = caArea.GetWidth() - caArea.GetHMargin()*2;
	int32_t iCtrlSetHMargin = 5, iCtrlSetVMargin = 5;
	int32_t iCtrlSetBtnWdt = BoundBy<int32_t>((iCtrlSetWdt - iMaxControlSets*iCtrlSetHMargin*2) / iMaxControlSets, 5, fctCtrlDefPic.Wdt);
	int32_t iCtrlSetBtnHgt = fctCtrlDefPic.GetHeightByWidth(iCtrlSetBtnWdt);
	iCtrlSetHMargin = (iCtrlSetWdt - iCtrlSetBtnWdt*iMaxControlSets) / (iMaxControlSets*2);
	C4GUI::ComponentAligner caKeyboardSetSel(caArea.GetFromTop(2*iCtrlSetVMargin+iCtrlSetBtnHgt), iCtrlSetHMargin, iCtrlSetVMargin);
	const char *szCtrlSetHotkeys = "1234567890"; /* 2do */
	uint32_t i;
	for (i = 0; i < assignment_sets.GetSetCount(); ++i)
	{
		C4PlayerControlAssignmentSet *assignment_set = assignment_sets.GetSetByIndex(i);
		char cCtrlSetHotkey = 0;
		if (i <= strlen(szCtrlSetHotkeys)) cCtrlSetHotkey = szCtrlSetHotkeys[i];
		C4GUI::CallbackButton<C4StartupOptionsDlg::ControlConfigArea, C4GUI::IconButton> *pBtn = new C4GUI::CallbackButton<C4StartupOptionsDlg::ControlConfigArea, C4GUI::IconButton>(C4GUI::Ico_None, caKeyboardSetSel.GetFromLeft(iCtrlSetBtnWdt), cCtrlSetHotkey, &C4StartupOptionsDlg::ControlConfigArea::OnCtrlSetBtn, this);
		C4Facet fctCtrlPic = assignment_set->GetPicture();
		pBtn->SetFacet(fctCtrlPic);
		AddElement(ppKeyControlSetBtns[i] = pBtn);
		pBtn->SetText(assignment_set->GetGUIName());
		pBtn->SetFont(pUseFontSmall, C4StartupFontClr);
		pBtn->SetToolTip(LoadResStr("IDS_DLGTIP_CHANGECTRL"));
	}
	iSelectedCtrlSet = 0;
	caArea.ExpandTop(caArea.GetVMargin());
	AddElement(new C4GUI::HorizontalLine(caArea.GetFromTop(2)));
	caArea.ExpandTop(caArea.GetVMargin());
	control_list = new ControlConfigListBox(caArea.GetFromLeft(caArea.GetInnerWidth()), NULL);
	AddElement(control_list);
	/*C4Facet &rfctKey = ::GraphicsResource.fctKey;
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
			KeySelButton *pKeyBtn = new C4GUI::CallbackButton<C4StartupOptionsDlg::ControlConfigArea, KeySelButton>(iKeyNum, rcKey, 0, &C4StartupOptionsDlg::ControlConfigArea::OnCtrlKeyBtn, this);
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
	::GraphicsResource.CaptionFont.GetTextExtent(szBtnText, iButtonWidth, iButtonHeight, true);
	C4Rect rcResetBtn = caKeyBottomBtns.GetFromRight(Min<int32_t>(iButtonWidth+iButtonHeight*4, caKeyBottomBtns.GetInnerWidth()));
	AddElement(btn = new C4GUI::CallbackButton<C4StartupOptionsDlg::ControlConfigArea, SmallButton>(szBtnText, rcResetBtn, &C4StartupOptionsDlg::ControlConfigArea::OnResetKeysBtn, this));
	btn->SetToolTip(LoadResStr("IDS_MSG_RESETKEYSETS"));*/

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
	// update list
	control_list->SetAssignmentSet(Game.PlayerControlUserAssignmentSets.GetSetByIndex(iSelectedCtrlSet));
	// update keys by config
	/*if (fGamepad)
	{
		for (i = 0; i < C4MaxKey; ++i)
			KeyControlBtns[i]->SetKey(Config.Gamepads[iSelectedCtrlSet].Button[i]);
	}
	// open gamepad
	if (fGamepad && Config.General.GamepadEnabled)
	{
		if (!pGamepadOpener) pGamepadOpener = new C4GamePadOpener(iSelectedCtrlSet);
		else pGamepadOpener->SetGamePad(iSelectedCtrlSet);
	}
	// show/hide gamepad-gui-control checkbox
	if (fGamepad && pGUICtrl)
		pGUICtrl->SetVisibility(iSelectedCtrlSet == 0);*/
}

void C4StartupOptionsDlg::ControlConfigArea::OnResetKeysBtn(C4GUI::Control *btn)
{
	// default keys and axis reset
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
	::GraphicsResource.CaptionFont.GetTextExtent("<< BACK", iButtonWidth, iCaptionFontHgt, true);
	iButtonWidth *= 3;
	int iIndentX1, iIndentY1, iIndentY2;
	if (fSmall)
	{
		iIndentX1=20;
	}
	else
	{
		iIndentX1 = GetClientRect().Wdt/40;
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
	C4GUI::Tabular::Sheet *pSheetControls= pOptionsTabular->AddSheet(LoadResStr("IDS_DLG_CONTROLS"), 3);
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
	pLangInfoLabel = new C4GUI::Label(NULL, caLanguage.GetFromTop(::GraphicsResource.TextFont.GetLineHeight()*3), ALeft, C4StartupFontClr, pUseFont, false);
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
	// reset configuration
	const char *szBtnText = LoadResStr("IDS_BTN_RESETCONFIG");
	C4GUI::CallbackButton<C4StartupOptionsDlg, SmallButton> *pSmallBtn;
	::GraphicsResource.CaptionFont.GetTextExtent(szBtnText, iButtonWidth, iButtonHeight, true);
	C4Rect rcResetBtn = caSheetProgram.GetGridCell(1,2,6,7, Min<int32_t>(iButtonWidth+iButtonHeight*4, caSheetProgram.GetInnerWidth()*2/5), SmallButton::GetDefaultButtonHeight(), true);
	pSmallBtn = new C4GUI::CallbackButton<C4StartupOptionsDlg, SmallButton>(szBtnText, rcResetBtn, &C4StartupOptionsDlg::OnResetConfigBtn, this);
	pSheetGeneral->AddElement(pSmallBtn);
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
	pCheckGfxEngines[GFXENGN_DIRECTX]->SetEnabled(false); // as long as DX doesnt work, its disabled
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
	// fullscreen combobox
	uint32_t wmax = 0;
	for(int i = 0; i < 3; ++i)
	{
		pUseFont->GetTextExtent(GetWindowedName(i),w,q,true);
		wmax = Max<int32_t>(w, wmax);
	}
	C4GUI::ComboBox * pCombo = new C4GUI::ComboBox(caGroupResolution.GetGridCell(0,1,3,4,wmax+40,C4GUI::ComboBox::GetDefaultHeight(), true));
	pCombo->SetComboCB(new C4GUI::ComboBox_FillCallback<C4StartupOptionsDlg>(this, &C4StartupOptionsDlg::OnWindowedModeComboFill, &C4StartupOptionsDlg::OnWindowedModeComboSelChange));
	pCombo->SetToolTip(LoadResStr("IDS_MSG_FULLSCREEN_DESC"));
	pCombo->SetColors(C4StartupFontClr, C4StartupEditBGColor, C4StartupEditBorderColor);
	pCombo->SetFont(pUseFont);
	pCombo->SetDecoration(&(C4Startup::Get()->Graphics.fctContext));
	pCombo->SetText(GetWindowedName());
	pGroupResolution->AddElement(pCombo);
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
	// multisampling
	C4GUI::ComponentAligner msBox(caGroupOptions.GetFromTop(C4GUI::ComboBox::GetDefaultHeight()), 0, 0, false);
	w=20; q=12; pUseFont->GetTextExtent(LoadResStr("IDS_CTL_ANTIALIASING"), w,q, true);
	pGroupOptions->AddElement(new C4GUI::Label(LoadResStr("IDS_CTL_ANTIALIASING"), msBox.GetFromLeft(w+C4GUI_DefDlgSmallIndent), ALeft, C4StartupFontClr, pUseFont, false, false));

	pUseFont->GetTextExtent("Off", w, q, true);
	C4GUI::ComboBox *pGfxMSCombo = new C4GUI::ComboBox(msBox.GetFromLeft(w+40,C4GUI::ComboBox::GetDefaultHeight()));
	pGfxMSCombo->SetToolTip(LoadResStr("IDS_MSG_ANTIALIASING_DESC"));
	pGfxMSCombo->SetComboCB(new C4GUI::ComboBox_FillCallback<C4StartupOptionsDlg>(this, &C4StartupOptionsDlg::OnGfxMSComboFill, &C4StartupOptionsDlg::OnGfxMSComboSelChange));
	pGfxMSCombo->SetColors(C4StartupFontClr, C4StartupEditBGColor, C4StartupEditBorderColor);
	pGfxMSCombo->SetFont(pUseFont);
	pGfxMSCombo->SetDecoration(&(C4Startup::Get()->Graphics.fctContext));
	// Pre-Select current setting
	StdStrBuf Current;
	if(Config.Graphics.MultiSampling) Current.Format("%dx", Config.Graphics.MultiSampling);
	else Current.Copy("Off");
	pGfxMSCombo->SetText(Current.getData());
	// Set control read only if multisampling is not available
	std::vector<int> multisamples;
	Application.pWindow->EnumerateMultiSamples(multisamples);
	pGfxMSCombo->SetReadOnly(multisamples.empty());
	pGroupOptions->AddElement(pGfxMSCombo);
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
	pCheck = new BoolConfig(caGroupEffects.GetGridCell(0,1,iOpt++,iNumGfxOptions,-1,iCheckHgt,true), LoadResStr("IDS_MSG_HIGHRESLANDSCAPE"), NULL, &Config.Graphics.HighResLandscape);
	pCheck->SetToolTip(LoadResStr("IDS_MSG_HIGHRESLANDSCAPE_DESC"));
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
		StdStrBuf sLabelTxt;
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

	// --- page controls
	pSheetControls->AddElement(pControlConfigArea = new ControlConfigArea(pSheetControls->GetClientRect(), caMain.GetWidth()/20, caMain.GetHeight()/40, false, this));

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
#ifdef WITH_AUTOMATIC_UPDATE
	pCheck = new BoolConfig(caSheetNetwork.GetFromTop(pUseFont->GetLineHeight()), LoadResStr("IDS_CTL_AUTOMATICUPDATES"), NULL, &Config.Network.AutomaticUpdate, false);
	pCheck->SetToolTip(LoadResStr("IDS_DESC_AUTOMATICUPDATES"));
	pCheck->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
	pSheetNetwork->AddElement(pCheck);
#endif
	const char *szNameCfgText = LoadResStr("IDS_NET_USERNAME");
	int iNameCfgWdt=200, iNameCfgHgt=48; C4StartupOptionsDlg::EditConfig::GetControlSize(&iNameCfgWdt, &iNameCfgHgt, szNameCfgText, false);
	iNameCfgWdt += 5;
	pNetworkNickEdit = new EditConfig(caSheetNetwork.GetFromTop(iNameCfgHgt), szNameCfgText, &Config.Network.Nick, NULL, false);
	pNetworkNickEdit->SetToolTip(LoadResStr("IDS_NET_USERNAME_DESC"));
	pSheetNetwork->AddElement(pNetworkNickEdit);

	StdCopyStrBuf NickBuf(Config.Network.Nick);
	if (!NickBuf.getLength()) NickBuf.Copy(Config.GetRegistrationData("Nick"));
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

void C4StartupOptionsDlg::OnGfxMSComboFill(C4GUI::ComboBox_FillCB *pFiller)
{
	// clear all old entries first to allow a clean refill
	pFiller->ClearEntries();

	pFiller->AddEntry("Off", 0);

	std::vector<int> multisamples;
	Application.pWindow->EnumerateMultiSamples(multisamples);

	std::sort(multisamples.begin(), multisamples.end());
	for(unsigned int i = 0; i < multisamples.size(); ++i)
	{
		StdStrBuf text;
		text.Format("%dx", multisamples[i]);
		pFiller->AddEntry(text.getData(), multisamples[i]);
	}
}

bool C4StartupOptionsDlg::OnGfxMSComboSelChange(C4GUI::ComboBox *pForCombo, int32_t idNewSelection)
{
	if(pTexMgr) pTexMgr->IntLock();
#ifdef USE_GL
	pDraw->InvalidateDeviceObjects();
	// Note: This assumes there is only one GL context (the main context). This
	// is true in fullscreen mode, and since the startup dlg is only shown in
	// fullscreen mode we are safe this way.
	if(pGL) pGL->pMainCtx->Clear();
#endif
#ifdef USE_DIRECTX
	// It should also be possible to clear+reinit DDraw also for GL, however,
	// if ReInit() does _not_ create a new window on X11 then all rendering
	// stops until the Window is being moved again (or tasked-out and back in
	// in fullscreen mode). This does not happen when only reinitializing the
	// GL context instead of whole DDraw so that's why we do this currently.
	if(pD3D) pDraw->Clear();
#endif

	int32_t PrevMultiSampling = Config.Graphics.MultiSampling;
	Config.Graphics.MultiSampling = idNewSelection;
	bool success = Application.pWindow->ReInit(&Application);

#ifdef USE_GL
	if(pGL) pGL->pMainCtx->Init(Application.pWindow, &Application);
	pDraw->RestoreDeviceObjects();
#endif
#ifdef USE_DIRECTX
	// Note: Editor is hardcoded to false at this point... I guess that's OK
	// because C4StartupOptionsDlg is never shown in editor mode anyway.
	if(pD3D) pDraw->Init(&Application, false, false, Config.Graphics.ResX, Config.Graphics.ResY, Config.Graphics.BitDepth, Config.Graphics.Monitor);
#endif

	if(pTexMgr) pTexMgr->IntUnlock();
	
	if(!success) Config.Graphics.MultiSampling = PrevMultiSampling;
	return !success;
}

void C4StartupOptionsDlg::OnGfxResComboFill(C4GUI::ComboBox_FillCB *pFiller)
{
	// clear all old entries first to allow a clean refill
	pFiller->ClearEntries();
	pFiller->AddEntry(LoadResStr("IDS_MNU_DEFAULTRESOLUTION"), -1);
	// fill with all possible resolutions
	int32_t idx = 0, iXRes, iYRes, iBitDepth;
	while (Application.GetIndexedDisplayMode(idx++, &iXRes, &iYRes, &iBitDepth, NULL, Config.Graphics.Monitor))
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
	if (idNewSelection == -1)
		iResX = iResY = -1;
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
	int32_t iNewFontSize = 14; // default (at 800x600)
	if (iResY >= 0 && iResY < 600)
		iNewFontSize = 12;
	else if (iResY > 800)
		iNewFontSize = 16;
	// call application to set it
	if (!Application.SetVideoMode(iResX, iResY, Config.Graphics.BitDepth, Config.Graphics.RefreshRate, Config.Graphics.Monitor, true))
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
	// Set new resolution in config before dialog recreation so that the initial combo box value is correct (#230)
	Config.Graphics.ResX = iResX;
	Config.Graphics.ResY = iResY;
	// since the resolution was changed, everything needs to be moved around a bit
	RecreateDialog(false);
	// Now set old resolution again to make sure config is restored even if the program is closed during the confirmation dialog
	Config.Graphics.ResX = iOldResX;
	Config.Graphics.ResY = iOldResY;
	// Show confirmation dialog
	ResChangeConfirmDlg *pConfirmDlg = new ResChangeConfirmDlg();
	if (!pScreen->ShowModalDlg(pConfirmDlg, true))
	{
		// abort: Restore screen, if this was not some program abort
		if (Application.SetVideoMode(iOldResX, iOldResY, Config.Graphics.BitDepth, Config.Graphics.RefreshRate, Config.Graphics.Monitor, !Config.Graphics.Windowed))
		{
			if (iNewFontSize != iOldFontSize) Application.SetGameFont(Config.General.RXFontName, iOldFontSize);
			RecreateDialog(false);
		}

		return false;
	}
	// resolution may be kept!
	Config.Graphics.ResX = iResX;
	Config.Graphics.ResY = iResY;
	if(Config.Graphics.Windowed)
		Application.SetVideoMode(Application.GetConfigWidth(), Application.GetConfigHeight(), Config.Graphics.BitDepth, Config.Graphics.RefreshRate, Config.Graphics.Monitor, false);
	return true;
}

StdStrBuf C4StartupOptionsDlg::GetGfxResString(int32_t iResX, int32_t iResY)
{
	// Display in format like "640 x 480"
	if (iResX == -1)
		return StdStrBuf(LoadResStr("IDS_MNU_DEFAULTRESOLUTION"));
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

const char * C4StartupOptionsDlg::GetWindowedName(int32_t mode /* = -1*/)
{
	if(mode == -1)
		mode = Config.Graphics.Windowed;
	     if(mode == 0) return LoadResStr("IDS_MSG_FULLSCREEN");
	else if(mode == 1) return LoadResStr("IDS_MSG_WINDOWED");
	else if(mode == 2) return LoadResStr("IDS_MSG_AUTOWINDOWED");
	assert(!"Requested name for config value which does not exist");
	return "ERR: Unknown";
}

void C4StartupOptionsDlg::OnWindowedModeComboFill(C4GUI::ComboBox_FillCB *pFiller)
{
	pFiller->ClearEntries();
	for(int32_t i = 0; i < 3; ++i)
		pFiller->AddEntry(GetWindowedName(i), i);
}

bool C4StartupOptionsDlg::OnWindowedModeComboSelChange(C4GUI::ComboBox *pForCombo, int32_t idNewSelection)
{
	Config.Graphics.Windowed = idNewSelection;
	Application.SetVideoMode(Config.Graphics.ResX, Config.Graphics.ResY, Config.Graphics.BitDepth, Config.Graphics.RefreshRate, Config.Graphics.Monitor, !Config.Graphics.Windowed);
	pForCombo->SetText(GetWindowedName(idNewSelection));
	return true;
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
	pNetworkNickEdit->Save2Config();
	// if nick is same as default by registry, don't save in config
	// so regkey updates will change the nick as well
	const char *szRegNick = Config.GetRegistrationData("Nick");
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

bool C4StartupOptionsDlg::SetSubscreen(const char *szToScreen)
{
	// go to specified property sheet
	// option sheets do not have a good identifier associated to them - just lookup from a static array
	const char *page_names[] = { "general", "graphics", "sound", "controls", "network", NULL };
	int32_t i = 0;
	while (page_names[i])
	{
		if (SEqualNoCase(szToScreen, page_names[i])) break;
		++i;
	}
	// page name does not match?
	if (!page_names[i]) return false;
	// page name OK; switch it
	pOptionsTabular->SelectSheet(i, false);
	return true;
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
	for (int i=0; i<Languages.GetInfoCount(); ++i)
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

const char *DefaultFonts[] = { "Arial Unicode MS", "Comic Sans MS", C4DEFAULT_FONT_NAME, "Verdana", NULL };

void C4StartupOptionsDlg::OnFontFaceComboFill(C4GUI::ComboBox_FillCB *pFiller)
{
	// 2do: enumerate Fonts.txt fonts; then enumerate system fonts
	for (int32_t i=0; DefaultFonts[i]; ++i) pFiller->AddEntry(DefaultFonts[i], i);
}

void C4StartupOptionsDlg::OnFontSizeComboFill(C4GUI::ComboBox_FillCB *pFiller)
{
	// 2do: enumerate possible font sizes by the font here
	// 2do: Hide font sizes that would be too large for the selected resolution
	//pFiller->AddEntry("8", 8);
	pFiller->AddEntry("10", 10);
	pFiller->AddEntry("12", 12);
	pFiller->AddEntry("14", 14);
	pFiller->AddEntry("16", 16);
	pFiller->AddEntry("18", 18);
	pFiller->AddEntry("20", 20);
	//pFiller->AddEntry("24", 24);
	//pFiller->AddEntry("28", 28);
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
	pShaders->SetChecked(!!Config.Graphics.EnableShaders);
	// title of troubleshooting-box
	pGroupTrouble->SetTitle(LoadResStrNoAmp("IDS_CTL_TROUBLE"));
}

void C4StartupOptionsDlg::SaveGfxTroubleshoot()
{
	// get it from controls
	Config.Graphics.EnableShaders=pShaders->GetChecked();
	// get config set to be used
	bool fUseGL = (Config.Graphics.Engine == GFXENGN_OPENGL);
	// and apply them directly, if the engine is current
	if (fUseGL == pDraw->IsOpenGL())
	{
		pDraw->RestoreDeviceObjects();
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

void C4StartupOptionsDlg::OnKeyboardLayoutChanged()
{
	// keyboard layout changed and thus some keys might have been updated from scan codes
	// update display in control set
	pControlConfigArea->UpdateCtrlSet();
}
