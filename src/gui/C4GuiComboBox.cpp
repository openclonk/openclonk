/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
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
// generic user interface
// dropdown box
// implemented via context menu

#include "C4Include.h"
#include "gui/C4Gui.h"

#include "graphics/C4FacetEx.h"
#include "gui/C4MouseControl.h"
#include "graphics/C4GraphicsResource.h"
#include "graphics/C4Draw.h"

#include "platform/C4Window.h"

namespace C4GUI
{

// ----------------------------------------------------
// ComboBox_FillCB

	void ComboBox_FillCB::AddEntry(const char *szText, int32_t id)
	{
		if (!szText) szText = "";
		typedef C4GUI::CBMenuHandlerEx<ComboBox, ComboBox::ComboMenuCBStruct> Handler;
		Handler *pHandler = new Handler(pCombo, &ComboBox::OnCtxComboSelect);
		pHandler->SetExtra(ComboBox::ComboMenuCBStruct(szText, id));
		pDrop->AddItem(szText, FormatString(LoadResStr("IDS_MSG_SELECT"), szText).getData(), Ico_Empty, pHandler);
	}

	bool ComboBox_FillCB::FindEntry(const char *szText)
	{
		// check for entry with same name
		ContextMenu::Entry *pEntry; int32_t idx=0;
		while ((pEntry = pDrop->GetIndexedEntry(idx++))) if (SEqual(pEntry->GetText(), szText)) return true;
		return false;
	}

	void ComboBox_FillCB::SelectEntry(int32_t iEntry)
	{
		pDrop->SelectItem(iEntry);
	}

	void ComboBox_FillCB::ClearEntries()
	{
		pDrop->Clear();
	}


// ----------------------------------------------------
// ComboBox

	ComboBox::ComboBox(const C4Rect &rtBounds) :
			Control(rtBounds), iOpenMenu(0), pFillCallback(nullptr), fReadOnly(false), fSimple(false), fMouseOver(false),
			pUseFont(nullptr), dwFontClr(C4GUI_ComboFontClr), dwBGClr(C4GUI_StandardBGColor), dwBorderClr(0), pFctSideArrow(nullptr)
	{
		*Text=0;
		// key callbacks - lots of possibilities to get the dropdown
		C4CustomKey::CodeList cbKeys;
		cbKeys.push_back(C4KeyCodeEx(K_DOWN));
		cbKeys.push_back(C4KeyCodeEx(K_SPACE));
		cbKeys.push_back(C4KeyCodeEx(K_DOWN, KEYS_Alt));
		cbKeys.push_back(C4KeyCodeEx(K_SPACE, KEYS_Alt));
		if (Config.Controls.GamepadGuiControl)
		{
			ControllerKeys::Ok(cbKeys);
			ControllerKeys::Down(cbKeys);
		}
		pKeyOpenCombo = new C4KeyBinding(cbKeys, "GUIComboOpen", KEYSCOPE_Gui,
		                                 new ControlKeyCB<ComboBox>(*this, &ComboBox::KeyDropDown), C4CustomKey::PRIO_Ctrl);
		cbKeys.clear();
		cbKeys.push_back(C4KeyCodeEx(K_ESCAPE));
		if (Config.Controls.GamepadGuiControl)
		{
			ControllerKeys::Cancel(cbKeys);
		}
		pKeyCloseCombo = new C4KeyBinding(cbKeys, "GUIComboClose", KEYSCOPE_Gui,
		                                  new ControlKeyCB<ComboBox>(*this, &ComboBox::KeyAbortDropDown), C4CustomKey::PRIO_Ctrl);
	}

	ComboBox::~ComboBox()
	{
		delete pKeyCloseCombo;
		delete pKeyOpenCombo;
		if (pFillCallback) delete pFillCallback;
	}

	void ComboBox::SetComboCB(ComboBox_FillCB *pNewFillCallback)
	{
		if (pFillCallback) delete pFillCallback;
		pFillCallback = pNewFillCallback;
	}

	bool ComboBox::DoDropdown()
	{
		// not if readonly
		if (fReadOnly) return false;
		// get dropdown pos
		int32_t iX = 0;
		int32_t iY = rcBounds.Hgt;
		// do dropdown
		Screen *pScreen = GetScreen();
		if (!pScreen) return false;
		// item list as context menu
		if (!pFillCallback) return false;
		ContextMenu *pNewMenu = new C4GUI::ContextMenu();
		// init with minimum size
		pNewMenu->GetBounds().Wdt = std::max(rcBounds.Wdt, pNewMenu->GetBounds().Wdt);
		// fill with items
		pFillCallback->FillDropDown(this, pNewMenu);
		// open it on screen
		pScreen->DoContext(pNewMenu, this, iX, iY);
		// store menu
		iOpenMenu = pNewMenu->GetMenuIndex();
		// done, success
		return true;
	}

	bool ComboBox::AbortDropdown(bool fByUser)
	{
		// recheck open menu
		Screen *pScr = GetScreen();
		if (!pScr || (iOpenMenu != pScr->GetLastContextMenuIndex())) iOpenMenu = 0;
		if (!iOpenMenu) return false;
		// abort it
		pScr->AbortContext(fByUser);
		return true;
	}

	void ComboBox::DrawElement(C4TargetFacet &cgo)
	{
		CStdFont *pUseFont = this->pUseFont ? this->pUseFont : &(::GraphicsResource.TextFont);
		// recheck open menu
		Screen *pScr = GetScreen();
		if (!pScr || (iOpenMenu != pScr->GetContextMenuIndex())) iOpenMenu = 0;
		// calc drawing bounds
		int32_t x0 = cgo.TargetX + rcBounds.x, y0 = cgo.TargetY + rcBounds.y;
		int32_t iRightTextEnd = x0 + rcBounds.Wdt - ::GraphicsResource.fctContext.Wdt - 1;
		if (!fReadOnly && !fSimple)
		{
			// draw background
			pDraw->DrawBoxDw(cgo.Surface, x0,y0,x0+rcBounds.Wdt-1,y0+rcBounds.Hgt-1,dwBGClr);
			// draw frame
			if (dwBorderClr)
			{
				int32_t x1=cgo.TargetX+rcBounds.x,y1=cgo.TargetY+rcBounds.y,x2=x1+rcBounds.Wdt,y2=y1+rcBounds.Hgt;
				pDraw->DrawFrameDw(cgo.Surface, x1, y1, x2, y2-1, dwBorderClr);
				pDraw->DrawFrameDw(cgo.Surface, x1+1, y1+1, x2-1, y2-2, dwBorderClr);
			}
			else
				// default frame color
				Draw3DFrame(cgo);
			// draw button; down (phase 1) if combo is down
			(pFctSideArrow ? pFctSideArrow : &(::GraphicsResource.fctContext))->Draw(cgo.Surface, iRightTextEnd, y0 + (rcBounds.Hgt-::GraphicsResource.fctContext.Hgt)/2, iOpenMenu ? 1 : 0);
		}
		else if (!fReadOnly)
		{
			// draw button in simple mode: Left of text
			(pFctSideArrow ? pFctSideArrow : &(::GraphicsResource.fctContext))->Draw(cgo.Surface, x0, y0 + (rcBounds.Hgt-::GraphicsResource.fctContext.Hgt)/2, iOpenMenu ? 1 : 0);
		}
		// draw text
		if (*Text)
		{
			pDraw->StorePrimaryClipper();
			pDraw->SubPrimaryClipper(x0,y0,iRightTextEnd-1,y0+rcBounds.Hgt-1);
			pDraw->TextOut(Text, *pUseFont, 1.0f, cgo.Surface, x0 + ::GraphicsResource.fctContext.Wdt + 2, y0 + (rcBounds.Hgt-pUseFont->GetLineHeight())/2, dwFontClr, ALeft);
			pDraw->RestorePrimaryClipper();
		}
		// draw selection highlight
		if ((HasDrawFocus() || iOpenMenu || fMouseOver) && !fReadOnly)
		{
			pDraw->SetBlitMode(C4GFXBLIT_ADDITIVE);
			::GraphicsResource.fctButtonHighlightRound.DrawX(cgo.Surface, x0, y0, rcBounds.Wdt, rcBounds.Hgt);
			pDraw->ResetBlitMode();
		}
	}

	void ComboBox::MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam)
	{
		// left-click activates menu
		if (!fReadOnly) if (iButton == C4MC_Button_LeftDown)
			{
				// recheck open menu
				Screen *pScr = GetScreen();
				if (!pScr || (iOpenMenu != pScr->GetLastContextMenuIndex())) iOpenMenu = 0;
				if (iOpenMenu)
					// left-click with combo down: abort has been done by screen; ignore
					return;
				else
					// otherwise, open it
					if (DoDropdown()) return;
			}
		// inherited
		Control::MouseInput(rMouse, iButton, iX, iY, dwKeyParam);
	}

	void ComboBox::MouseEnter(CMouse &rMouse)
	{
		fMouseOver = true;
		Control::MouseEnter(rMouse);
	}

	void ComboBox::MouseLeave(CMouse &rMouse)
	{
		fMouseOver = false;
		Control::MouseLeave(rMouse);
	}

	int32_t ComboBox::GetDefaultHeight()
	{
		return ::GraphicsResource.TextFont.GetLineHeight() + 4;
	}

	void ComboBox::SetText(const char *szToText)
	{
		// set text without accelerator keys
		if (szToText)
		{
			StdStrBuf sTxt(szToText);
			sTxt.Replace("&", "");
			SCopy(sTxt.getData(), Text, C4MaxTitle);
		}
		else
			*Text=0;
	}

	void ComboBox::OnCtxComboSelect(C4GUI::Element *pListItem, const ComboMenuCBStruct &rNewSel)
	{
		// ignore in readonly
		if (fReadOnly) return;
		// do callback
		if (!pFillCallback || !pFillCallback->OnComboSelChange(this, rNewSel.id))
			// callback didn't process: default behaviour
			SetText(rNewSel.sText.getData());
		// don't do anything else, because this might be deleted
	}

} // namespace C4GUI
