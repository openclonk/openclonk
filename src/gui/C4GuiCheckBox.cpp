/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2006  Sven Eberhardt
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
// generic user interface
// checkbox

#include <C4Include.h>
#include <C4Gui.h>
#include <C4FacetEx.h>
#include <C4MouseControl.h>
#include <C4GraphicsResource.h>

#include <StdWindow.h>

namespace C4GUI
{

// ----------------------------------------------------
// CheckBox

	CheckBox::CheckBox(const C4Rect &rtBounds, const char *szCaption, bool fChecked)
			: Control(rtBounds), fChecked(fChecked), fMouseOn(false), fEnabled(true), pFont(NULL)
			, dwEnabledClr(C4GUI_CheckboxFontClr), dwDisabledClr(C4GUI_CheckboxDisabledFontClr), cHotkey(0)
	{
		if (szCaption)
		{
			sCaption.Copy(szCaption);
			ExpandHotkeyMarkup(sCaption, cHotkey);
		}
		// key callbacks: Check/Uncheck on space and primary joy button
		C4CustomKey::CodeList Keys;
		Keys.push_back(C4KeyCodeEx(K_SPACE));
		if (Config.Controls.GamepadGuiControl)
		{
			Keys.push_back(C4KeyCodeEx(KEY_Gamepad(0, KEY_JOY_AnyLowButton)));
		}
		pKeyCheck = new C4KeyBinding(Keys, "GUICheckboxToggle", KEYSCOPE_Gui,
		                             new ControlKeyCB<CheckBox>(*this, &CheckBox::KeyCheck), C4CustomKey::PRIO_Ctrl);
		pCBHandler = NULL;
	}

	CheckBox::~CheckBox()
	{
		delete pKeyCheck;
		if (pCBHandler) pCBHandler->DeRef();
	}

	void CheckBox::UpdateOwnPos()
	{
	}

	bool CheckBox::OnHotkey(char cHotkey)
	{
		if (cHotkey != this->cHotkey) return false;
		ToggleCheck(true);
		return true;
	}

	void CheckBox::ToggleCheck(bool fByUser)
	{
		// user can't toggle if disabled
		if (fByUser && !fEnabled) return;
		// sound
		if (fByUser) GUISound("ArrowHit");
		// toggle state
		fChecked = !fChecked;
		// callback (last call; may destroy element)
		if (pCBHandler) pCBHandler->DoCall(this);
	}

	void CheckBox::MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam)
	{
		if (fEnabled)
		{
			// set mouse-on flag depending on whether mouse is over box area
			fMouseOn = Inside<int32_t>(iX, 0, rcBounds.Hgt) && Inside<int32_t>(iY, 0, rcBounds.Hgt);
			// left-click within checkbox toggles it
			if (iButton == C4MC_Button_LeftDown && fMouseOn)
			{
				ToggleCheck(true);
				return;
			}
		}
		// not recognized; base call
		Control::MouseInput(rMouse, iButton, iX, iY, dwKeyParam);
	}

	void CheckBox::MouseEnter(CMouse &rMouse)
	{
		Control::MouseEnter(rMouse);
	}

	void CheckBox::MouseLeave(CMouse &rMouse)
	{
		fMouseOn = false;
		Control::MouseLeave(rMouse);
	}

	void CheckBox::DrawElement(C4TargetFacet &cgo)
	{
		// left side: check facet (squared)
		int x0 = rcBounds.x + cgo.TargetX;
		int y0 = rcBounds.y + cgo.TargetY;
		::GraphicsResource.fctCheckbox.GetPhase(fChecked + 2*!fEnabled).DrawX(cgo.Surface, x0, y0, rcBounds.Hgt, rcBounds.Hgt);
		// right of it: checkbox text
		CStdFont *pUseFont = pFont ? pFont : &(::GraphicsResource.TextFont);
		int32_t yOff; float fZoom;
		if (pUseFont->GetLineHeight() <= rcBounds.Hgt)
		{
			yOff = Max<int32_t>(rcBounds.Hgt - pUseFont->GetLineHeight(), 0)/2;
			fZoom = 1.0f;
		}
		else
		{
			yOff = 0;
			fZoom = (float) rcBounds.Hgt / Max(pUseFont->GetLineHeight(), 1);
		}
		lpDDraw->TextOut(sCaption.getData(), *pUseFont, fZoom, cgo.Surface, x0 + rcBounds.Hgt + C4GUI_CheckBoxLabelSpacing, y0 + yOff, fEnabled ? dwEnabledClr : dwDisabledClr, ALeft, true);
		// selection marker
		if ((fMouseOn && IsInActiveDlg(false)) || HasDrawFocus())
		{
			lpDDraw->SetBlitMode(C4GFXBLIT_ADDITIVE);
			::GraphicsResource.fctButtonHighlight.DrawX(cgo.Surface, x0+rcBounds.Hgt*1/4, y0+rcBounds.Hgt*1/4, rcBounds.Hgt*1/2, rcBounds.Hgt*1/2);
			lpDDraw->ResetBlitMode();
		}
	}

	void CheckBox::SetOnChecked(BaseCallbackHandler *pCB)
	{
		if (pCBHandler) pCBHandler->DeRef();
		if ((pCBHandler = pCB)) pCB->Ref();
	}

	bool CheckBox::GetStandardCheckBoxSize(int *piWdt, int *piHgt, const char *szForCaptionText, CStdFont *pUseFont)
	{
		// get needed text size
		if (!pUseFont) pUseFont = &(::GraphicsResource.TextFont);
		int32_t iWdt=100, iHgt=32;
		pUseFont->GetTextExtent(szForCaptionText, iWdt, iHgt, true);
		// check box height equals text height
		// add check box plus indent
		if (piWdt) *piWdt = iWdt + iHgt + C4GUI_CheckBoxLabelSpacing;
		if (piHgt) *piHgt = iHgt;
		return true;
	}

} // namespace C4GUI
