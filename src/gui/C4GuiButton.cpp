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
// that which can be pressed

#include "C4Include.h"
#include "gui/C4Gui.h"

#include "gui/C4MouseControl.h"
#include "graphics/C4GraphicsResource.h"
#include "graphics/C4Draw.h"

namespace C4GUI
{


// --------------------------------------------------
// Button

	Button::Button(const char *szBtnText, const C4Rect &rtBounds)
			: Control(rtBounds), pCustomGfx(nullptr), pCustomGfxDown(nullptr), fDown(false), fMouseOver(false), fEnabled(true),
			  dwCustomFontClr(0), pCustomFont(nullptr)
	{
		// key callbacks
		C4CustomKey::CodeList keys;
		keys.push_back(C4KeyCodeEx(K_SPACE));
		keys.push_back(C4KeyCodeEx(K_RETURN));
		if (Config.Controls.GamepadGuiControl)
			ControllerKeys::Ok(keys);
		pKeyButton = new C4KeyBinding(keys, "GUIButtonPress", KEYSCOPE_Gui,
		                              new ControlKeyCB<Button>(*this, &Button::KeyButtonDown, &Button::KeyButtonUp), C4CustomKey::PRIO_Ctrl);
		sText = "";
		// set new button text
		SetText(szBtnText);
	}

	Button::~Button()
	{
		delete pKeyButton;
	}

	void Button::SetText(const char *szToText)
	{
		// set new button text
		if (szToText)
		{
			sText.Copy(szToText);
			// expand hotkey markup
			ExpandHotkeyMarkup(sText, cHotkey);
		}
		else
		{
			sText="";
			cHotkey=0;
		}
	}

	bool Button::OnHotkey(uint32_t cHotkey)
	{
		// if hotkey matches, press the button
		if (this->cHotkey == cHotkey && fEnabled)
		{
			OnPress();
			return true;
		}
		else return false;
	}

	void Button::DrawElement(C4TargetFacet &cgo)
	{
		// draw base
		if (fDown)
			// pressed
			DrawBar(cgo, pCustomGfxDown ? *pCustomGfxDown : ::GraphicsResource.barButtonD);
		else
			// released
			DrawBar(cgo, pCustomGfx ? *pCustomGfx : ::GraphicsResource.barButton);
		// get text pos
		int32_t x0 = cgo.TargetX + rcBounds.x, y0 = cgo.TargetY + rcBounds.y, x1 = cgo.TargetX + rcBounds.x + rcBounds.Wdt - 1, y1 = cgo.TargetY + rcBounds.y + rcBounds.Hgt - 1;
		int32_t iTxtOff = fDown ? 1 : 0;
		// draw selection highlight
		if (fEnabled) if (HasDrawFocus() || (fMouseOver && IsInActiveDlg(false)))
			{
				pDraw->SetBlitMode(C4GFXBLIT_ADDITIVE);
				::GraphicsResource.fctButtonHighlight.DrawX(cgo.Surface, x0+5, y0+3, rcBounds.Wdt-10, rcBounds.Hgt-6);
				pDraw->ResetBlitMode();
			}
		// draw text
		int32_t iTextHgt = rcBounds.Hgt-2;
		CStdFont &rUseFont =
		  (::GraphicsResource.TitleFont.GetLineHeight() > iTextHgt ?
		   (::GraphicsResource.CaptionFont.GetLineHeight() > iTextHgt ?
		    ::GraphicsResource.TextFont :
		    ::GraphicsResource.CaptionFont) :
				   ::GraphicsResource.TitleFont);
		iTextHgt = rUseFont.GetLineHeight();
		pDraw->TextOut(sText.getData(), rUseFont, 1.0f, cgo.Surface, (x0+x1)/2 + iTxtOff, (y0+y1-iTextHgt)/2 + iTxtOff, C4GUI_ButtonFontClr, ACenter, true);
	}

	bool Button::KeyButtonDown()
{
		// not on disabled
		if (!fEnabled) return false;
		// space downs button
		SetDown();
		return true;
	}

	bool Button::KeyButtonUp()
	{
		// space press activates button
		if (!fDown) return false;
		SetUp(true);
		if (fEnabled) OnPress();
		return true;
	}

	void Button::MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam)
	{
		// inherited
		Control::MouseInput(rMouse, iButton, iX, iY, dwKeyParam);
		// process left down and up
		if (fEnabled) switch (iButton)
			{
			case C4MC_Button_LeftDown:

				// mark button as being down
				SetDown();
				// remember drag target
				// no dragging movement will be done w/o drag component assigned
				// but it should be remembered if the user leaves the button with the mouse down
				// and then re-enters w/o having released the button
				if (!rMouse.pDragElement) rMouse.pDragElement = this;
				break;

			case C4MC_Button_LeftUp:
				// only if button was down... (might have dragged here)
				if (fDown)
				{
					// it's now up :)
					SetUp(true);
					// process event
					OnPress();
				}
				break;
			};
	}

	void Button::MouseEnter(CMouse &rMouse)
	{
		Control::MouseEnter(rMouse);
		// remember mouse state for button highlight
		fMouseOver = true;
		// mouse re-enters with left button down?
		if (rMouse.pDragElement == this) if (fEnabled) SetDown();
	}

	void Button::MouseLeave(CMouse &rMouse)
	{
		Control::MouseLeave(rMouse);
		// mouse left
		fMouseOver = false;
		// reset down-state if mouse leves with button down
		if (rMouse.pDragElement == this) if (fEnabled) SetUp(false);
	}

	void Button::OnPress()
	{
		// nothing in base
	}

	void Button::SetDown()
	{
		// already down?
		if (fDown) return;
		// play sound
		GUISound("UI::Tick");
		// set down
		fDown = true;
	}

	void Button::SetUp(bool fPress)
	{
		// already up?
		if (!fDown) return;
		// play sound
		GUISound(fPress ? "UI::Click" : "UI::Tick");
		// set up
		fDown = false;
	}


// --------------------------------------------------------
// IconButton

	void IconButton::DrawElement(C4TargetFacet &cgo)
	{
		// get drawing bounds
		int32_t x0 = cgo.TargetX + rcBounds.x, y0 = cgo.TargetY + rcBounds.y;
		// draw selection highlight
		if (fEnabled) if (fHighlight || HasDrawFocus() || (fMouseOver && IsInActiveDlg(false)))
			{
				pDraw->SetBlitMode(C4GFXBLIT_ADDITIVE);
				::GraphicsResource.fctButtonHighlightRound.DrawX(cgo.Surface, x0, y0, rcBounds.Wdt, rcBounds.Hgt);
				pDraw->ResetBlitMode();
			}
		// draw the icon
		if (fHasClr && fctIcon.Surface) fctIcon.Surface->SetClr(dwClr);
		fctIcon.DrawX(cgo.Surface, x0, y0, rcBounds.Wdt, rcBounds.Hgt);
		// "button" down?
		if (fEnabled) if (fDown || fHighlight)
			{
				pDraw->SetBlitMode(C4GFXBLIT_ADDITIVE);
				::GraphicsResource.fctButtonHighlightRound.DrawX(cgo.Surface, x0, y0, rcBounds.Wdt, rcBounds.Hgt);
				pDraw->ResetBlitMode();
			}
		// some icon buttons have captions. draw caption below button
		if (sText.getLength())
		{
			CStdFont &rUseFont = pCustomFont ? *pCustomFont : ::GraphicsResource.TextFont;
			pDraw->TextOut(sText.getData(), rUseFont, 1.0f, cgo.Surface, x0+rcBounds.Wdt/2, y0+rcBounds.Hgt, pCustomFont ? dwCustomFontClr : C4GUI_CaptionFontClr, ACenter);
		}
	}

	IconButton::IconButton(Icons eUseIcon, const C4Rect &rtBounds, char caHotkey, const char *tooltip_text)
			: Button("", rtBounds), dwClr(0u), fHasClr(false), fHighlight(false)
	{
		// ctor
		cHotkey = caHotkey;
		SetIcon(eUseIcon);
		// set tooltip and expand hotkey
		if (tooltip_text)
		{
			StdStrBuf tooltip_text_buf(tooltip_text);
			if (!cHotkey) ExpandHotkeyMarkup(tooltip_text_buf, cHotkey, true);
			SetToolTip(tooltip_text_buf.getData(), true);
		}
	}

	void IconButton::SetIcon(Icons eUseIcon)
	{
		if (eUseIcon>=0) fctIcon = Icon::GetIconFacet(eUseIcon); else fctIcon.Surface=nullptr;
	}



// --------------------------------------------------------
// ArrowButton

	ArrowButton::ArrowButton(ArrowFct eDir, const C4Rect &rtBounds, char cHotkey)
			: Button("", rtBounds), eDir(eDir)
	{
		// ctor
		this->cHotkey = cHotkey;
	}

	void ArrowButton::DrawElement(C4TargetFacet &cgo)
	{
		// get drawing bounds
		int32_t x0 = cgo.TargetX + rcBounds.x, y0 = cgo.TargetY + rcBounds.y;
		// draw selection highlight
		if (fEnabled) if (HasDrawFocus() || (fMouseOver && IsInActiveDlg(false)))
			{
				pDraw->SetBlitMode(C4GFXBLIT_ADDITIVE);
				::GraphicsResource.fctButtonHighlightRound.DrawX(cgo.Surface, x0, y0, rcBounds.Wdt, rcBounds.Hgt);
				pDraw->ResetBlitMode();
			}
		// draw the arrow - down if pressed
		int32_t iFctIdx = eDir;
		if (fDown) iFctIdx += Down;
		::GraphicsResource.fctBigArrows.GetPhase(iFctIdx).DrawX(cgo.Surface, x0, y0, rcBounds.Wdt, rcBounds.Hgt);
	}


	int32_t ArrowButton::GetDefaultWidth()
	{
		// default by gfx size
		return ::GraphicsResource.fctBigArrows.Wdt;
	}

	int32_t ArrowButton::GetDefaultHeight()
	{
		// default by gfx size
		return ::GraphicsResource.fctBigArrows.Hgt;
	}


// --------------------------------------------------------
// FacetButton

	FacetButton::FacetButton(const C4Facet &rBaseFct, const C4Facet &rHighlightFct, const FLOAT_RECT &rtfBounds, char cHotkey)
			: Button("", C4Rect(rtfBounds)), fctBase(rBaseFct), fctHighlight(rHighlightFct), dwTextClrInact(0x7f000000), dwTextClrAct(0xff000000), rcfDrawBounds(rtfBounds), pFont(nullptr), fFontZoom(1.0f)
	{
		// ctor
		this->cHotkey = cHotkey;
		iTxtOffX=iTxtOffY=0;
		byTxtAlign=ALeft;
	}

	void FacetButton::DrawElement(C4TargetFacet &cgo)
	{
		// get drawing bounds
		float x0 = rcfDrawBounds.left + cgo.TargetX, y0 = rcfDrawBounds.top + cgo.TargetY;
		// draw button or highlight facet
		uint32_t dwTextClr;
		if ((HasDrawFocus() || (fMouseOver && IsInActiveDlg(false))) && fEnabled)
		{
			fctHighlight.DrawXFloat(cgo.Surface, x0, y0, rcfDrawBounds.right-rcfDrawBounds.left, rcfDrawBounds.bottom-rcfDrawBounds.top);
			dwTextClr = dwTextClrAct;
		}
		else
		{
			fctBase.DrawXFloat(cgo.Surface, x0, y0, rcfDrawBounds.right-rcfDrawBounds.left, rcfDrawBounds.bottom-rcfDrawBounds.top);
			dwTextClr = dwTextClrInact;
		}
		// draw caption text
		if (sText.getLength()>0)
		{
			CStdFont *pUseFont = pFont ? pFont : &(::GraphicsResource.GetFontByHeight(rcBounds.Hgt, &fFontZoom));
			pDraw->TextOut(sText.getData(), *pUseFont, fFontZoom, cgo.Surface, (int)(x0+iTxtOffX), (int)(y0+iTxtOffY), dwTextClr, byTxtAlign, true);
		}
		/*
		if (fEnabled) if (fMouseOver && IsInActiveDlg(false))
		  {
		  pDraw->SetBlitMode(C4GFXBLIT_ADDITIVE);
		  GetRes()->fctButtonHighlight.DrawXFloat(cgo.Surface, x0, y0, rcfDrawBounds.right-rcfDrawBounds.left, rcfDrawBounds.bottom-rcfDrawBounds.top);
		  pDraw->ResetBlitMode();
		  }*/
	}


} // end of namespace

