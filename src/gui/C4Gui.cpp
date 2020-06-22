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
// all generic classes that do not fit into other C4Gui*-files

#include "C4Include.h"
#include "gui/C4Gui.h"

#include "game/C4Application.h"
#include "game/C4FullScreen.h"
#include "game/C4GraphicsSystem.h"
#include "game/C4Viewport.h"
#include "graphics/C4Draw.h"
#include "graphics/C4GraphicsResource.h"
#include "gui/C4LoaderScreen.h"
#include "gui/C4MouseControl.h"
#include "platform/C4GamePadCon.h"

namespace C4GUI
{

// --------------------------------------------------
// Generic helpers

	bool ExpandHotkeyMarkup(StdStrBuf &sText, uint32_t &rcHotkey, bool for_tooltip)
	{
		const char *HotkeyMarkup = (for_tooltip ? "<c ff800000>%s</c>" : "<c ffffff7f>%s</c>");

		StdStrBuf output;

		const char *input = sText.getData();
		rcHotkey = 0;

		// Iterate over all input characters
		while (input && *input)
		{
			if (*input != '&')
			{
				// This will correctly copy UTF-8 chars too
				output.AppendChar(*input++);
			}
			else
			{
				++input;
				if (*input == '\0' || *input == '&')
				{
					// If the ampersand is followed by another ampersand, or it is the last character, copy it verbatimly
					// Note: This means you can't use an ampersand as an accelerator key.
					output.AppendChar(*input);
				}
				else
				{
					// Store the start of the hotkey so we can copy it later
					const char *accel_start = input;
					rcHotkey = GetNextCharacter(&input);
					// Using std::string because StdStrBuf doesn't have a ctor from two iterators
					std::string accel(accel_start, input);
					output.AppendFormat(HotkeyMarkup, accel.c_str());

					// Converting a char code to upper case isn't trivial for unicode. (This should really just use ICU.)
					if (Inside(rcHotkey, static_cast<uint32_t>('a'), static_cast<uint32_t>('z')))
					{
						rcHotkey += static_cast<uint32_t>('A') - 'a';
					} 
					else if (!Inside(rcHotkey, static_cast<uint32_t>('A'), static_cast<uint32_t>('Z')))
					{
						// Warn about accelerator keys outside the basic latin alphabet.
						LogF(LoadResStr("IDS_ERR_UNSUPPORTED_ACCELERATOR"), accel.c_str(), sText.getData());
					}
				}
			}
		}

		if (rcHotkey == 0)
		{
			// No accelerator found
			return false;
		}

		sText.Take(output);
		// done, success
		return true;
	}

	DWORD MakeColorReadableOnBlack(DWORD &rdwClr)
	{
		// max alpha
		DWORD dwAlpha = std::max<DWORD>(rdwClr>>24&255, 0xff)<<24;
		rdwClr &= 0xffffff;
		// determine brightness
		// 50% red, 87% green, 27% blue (max 164 * 255)
		DWORD r=(rdwClr>>16&255), g=(rdwClr>>8&255), b=(rdwClr&255);
		int32_t iLightness = r*50 + g*87 + b*27;
		// above 65/164 (*255) is OK
		if (iLightness < 16575)
		{
			int32_t iInc = (16575-iLightness) / 164;
			// otherwise, lighten
			rdwClr = (std::min<DWORD>(r+iInc, 255)<<16) | (std::min<DWORD>(g+iInc, 255)<<8) | std::min<DWORD>(b+iInc, 255);
		}
		// return color and alpha
		rdwClr |= dwAlpha;
		return rdwClr;
	}

	void DynBarFacet::SetHorizontal(C4Surface &rBySfc, int iHeight, int iBorderWidth)
	{
		if (!iHeight) iHeight = rBySfc.Hgt;
		if (!iBorderWidth) iBorderWidth = iHeight;
		fctBegin.Set(&rBySfc,0,0,iBorderWidth,iHeight);
		fctMiddle.Set(&rBySfc,iBorderWidth,0,rBySfc.Wdt-2*iBorderWidth,iHeight);
		fctEnd.Set(&rBySfc,rBySfc.Wdt-iBorderWidth,0,iBorderWidth,iHeight);
	}

	void DynBarFacet::SetHorizontal(C4Facet &rByFct, int32_t iBorderWidth)
	{
		if (!iBorderWidth) iBorderWidth = rByFct.Hgt;
		fctBegin.Set(rByFct.Surface,rByFct.X,rByFct.Y,iBorderWidth,rByFct.Hgt);
		fctMiddle.Set(rByFct.Surface,rByFct.Hgt,rByFct.X,rByFct.Y+rByFct.Wdt-2*iBorderWidth,rByFct.Hgt);
		fctEnd.Set(rByFct.Surface,rByFct.X+rByFct.Wdt-iBorderWidth,rByFct.Y,iBorderWidth,rByFct.Hgt);
	}

	void ScrollBarFacets::Set(const C4Facet &rByFct, int32_t iPinIndex)
	{
		// set by hardcoded size
		barScroll.fctBegin.Set(rByFct.Surface,0,0,16,16);
		barScroll.fctMiddle.Set(rByFct.Surface,0,16,16,16);
		barScroll.fctEnd.Set(rByFct.Surface,0,32,16,16);
		fctScrollDTop.Set(rByFct.Surface,16,0,16,16);
		if (iPinIndex)
			fctScrollPin.Set(rByFct.Surface,32,16*(iPinIndex-1),16,16);
		else
			fctScrollPin.Set(rByFct.Surface,16,16,16,16);
		fctScrollDBottom.Set(rByFct.Surface,16,32,16,16);
	}

// --------------------------------------------------
// Element

	Element::Element()
	{
		// pParent=nullptr invalidates pPrev/pNext
		// fDragging=false invalidates iDragX/Y
		// zero fields
		rcBounds.Set(0,0,0,0);
	}

	Element::~Element()
	{
		// delete context handler
		if (pContextHandler) { pContextHandler->DeRef(); pContextHandler=nullptr; }
		// remove from any container
		if (pParent)
			pParent->RemoveElement(this);
		else if (this != Screen::GetScreenS() && Screen::GetScreenS())
			// always ensure removal from screen!
			Screen::GetScreenS()->RemoveElement(this);
	}

	void Element::RemoveElement(Element *pChild)
	{
		// child removed: forward to parent
		if (pParent)
			pParent->RemoveElement(pChild);
		else if (this != Screen::GetScreenS())
			// always ensure removal from screen!
			// but not if this is the context menu, to avoid endless flip-flop!
			if (!IsMenu())
				Screen::GetScreenS()->RemoveElement(pChild);
	}

	void Element::UpdateSize()
	{
		// update own fields
		UpdateOwnPos();
		// notify container
		if (pParent) pParent->ElementSizeChanged(this);
	}

	void Element::UpdatePos()
	{
		// update own fields
		UpdateOwnPos();
		// notify container
		if (pParent) pParent->ElementPosChanged(this);
	}

	bool Element::IsVisible()
	{
		// self and parent must be visible
		return fVisible && (!pParent || pParent->IsVisible());
	}

	void Element::SetVisibility(bool fToValue)
	{
		fVisible = fToValue;
		// stop mouseover for invisible
		if (!fVisible)
		{
			Screen *pScreen = GetScreen();
			if (pScreen) pScreen->Mouse.OnElementGetsInvisible(this);
		}
	}

	void Element::ScreenPos2ClientPos(int32_t &riX, int32_t &riY)
	{
		// apply all parent offsets
		Container *pCont = pParent;
		while (pCont)
		{
			pCont->ApplyElementOffset(riX, riY);
			pCont = pCont->GetParent();
		}
		// apply own offset
		riX -= rcBounds.x; riY -= rcBounds.y;
	}

	void Element::ClientPos2ScreenPos(int32_t &riX, int32_t &riY)
	{
		// apply all parent offsets
		Container *pCont = pParent;
		while (pCont)
		{
			pCont->ApplyInvElementOffset(riX, riY);
			pCont = pCont->GetParent();
		}
		// apply own offset
		riX += rcBounds.x; riY += rcBounds.y;
	}

	void Element::MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam)
	{
		// store self as mouse-over-component
		rMouse.pMouseOverElement = this;
		// evaluate dragging
		if (pDragTarget && iButton == C4MC_Button_LeftDown && !rMouse.pDragElement)
			StartDragging(rMouse, iX, iY, dwKeyParam);
		// right button down: open context menu
		if (iButton == C4MC_Button_RightDown)
		{
			ContextHandler *pCtx = GetContextHandler();
			if (pCtx) pCtx->OnContext(this, iX, iY);
		}
	}

	void Element::StartDragging(CMouse &rMouse, int32_t iX, int32_t iY, DWORD dwKeyParam)
	{
		// set flag
		fDragging = true;
		// set drag start pos
		iDragX = iX; iDragY = iY;
		// mark drag in mouse
		rMouse.pDragElement = this;
	}

	void Element::DoDragging(CMouse &rMouse, int32_t iX, int32_t iY, DWORD dwKeyParam)
	{
		// check if anything moved
		if (pDragTarget && (iX != iDragX || iY != iDragY))
		{
			// move position, then
			pDragTarget->rcBounds.x += iX-iDragX;
			pDragTarget->rcBounds.y += iY-iDragY;
			// drag X/Y is up-to-date if this is a child element of the drag target
			pDragTarget->UpdatePos();
		}
	}

	void Element::StopDragging(CMouse &rMouse, int32_t iX, int32_t iY, DWORD dwKeyParam)
	{
		// move element pos
		DoDragging(rMouse, iX, iY, dwKeyParam);
	}

	Dialog *Element::GetDlg   () { if (pParent) return pParent->GetDlg   (); return nullptr; }
	Screen *Element::GetScreen() { if (pParent) return pParent->GetScreen(); return nullptr; }

	void Element::Draw3DFrame(C4TargetFacet &cgo, bool fUp, int32_t iIndent, BYTE byAlpha, bool fDrawTop, int32_t iTopOff, bool fDrawLeft, int32_t iLeftOff)
	{
		DWORD dwAlpha = byAlpha<<24;
		int32_t x0 = cgo.TargetX + rcBounds.x + iLeftOff,
		             y0 = cgo.TargetY + rcBounds.y + iTopOff,
		                  x1 = cgo.TargetX + rcBounds.x + rcBounds.Wdt - 1,
		                       y1 = cgo.TargetY + rcBounds.y + rcBounds.Hgt - 1;
		if (fDrawTop) pDraw->DrawLineDw(cgo.Surface, (float)x0,(float)y0,(float)x1,(float)y0, C4GUI_BorderColor1 | dwAlpha);
		if (fDrawLeft) pDraw->DrawLineDw(cgo.Surface, (float)x0,(float)y0,(float)x0,(float)y1, C4GUI_BorderColor1 | dwAlpha);
		if (fDrawTop) pDraw->DrawLineDw(cgo.Surface, (float)(x0+1),(float)(y0+1),(float)(x1-1),(float)(y0+1), C4GUI_BorderColor2 | dwAlpha);
		if (fDrawLeft) pDraw->DrawLineDw(cgo.Surface, (float)(x0+1),(float)(y0+1),(float)(x0+1),(float)(y1-1), C4GUI_BorderColor2 | dwAlpha);
		pDraw->DrawLineDw(cgo.Surface, (float)x0,(float)y1,(float)x1,(float)y1, C4GUI_BorderColor3 | dwAlpha);
		pDraw->DrawLineDw(cgo.Surface, (float)x1,(float)y0,(float)x1,(float)y1, C4GUI_BorderColor3 | dwAlpha);
		pDraw->DrawLineDw(cgo.Surface, (float)(x0+1),(float)(y1-1),(float)(x1-1),(float)(y1-1), C4GUI_BorderColor1 | dwAlpha);
		pDraw->DrawLineDw(cgo.Surface, (float)(x1-1),(float)(y0+1),(float)(x1-1),(float)(y1-1), C4GUI_BorderColor1 | dwAlpha);
	}

	void Element::DrawBar(C4TargetFacet &cgo, DynBarFacet &rFacets)
	{
		if (rcBounds.Hgt == rFacets.fctMiddle.Hgt)
		{
			// exact bar
			int32_t x0=cgo.TargetX+rcBounds.x, y0=cgo.TargetY+rcBounds.y;
			int32_t iX = rFacets.fctBegin.Wdt, w=rFacets.fctMiddle.Wdt, wLeft=rFacets.fctBegin.Wdt, wRight=rFacets.fctEnd.Wdt;
			int32_t iRightShowLength = wRight/3;
			bool fOverflow = (wLeft > rcBounds.Wdt);
			if (fOverflow) rFacets.fctBegin.Wdt = rcBounds.Wdt;
			rFacets.fctBegin.Draw(cgo.Surface, x0,y0);
			if (fOverflow) rFacets.fctBegin.Wdt = wLeft;
			while (iX < rcBounds.Wdt-iRightShowLength)
			{
				int32_t w2=std::min(w, rcBounds.Wdt-iRightShowLength-iX); rFacets.fctMiddle.Wdt=w2;
				rFacets.fctMiddle.Draw(cgo.Surface, x0+iX, y0);
				iX += w;
			}
			rFacets.fctMiddle.Wdt=w;
			fOverflow = (wRight > rcBounds.Wdt);
			if (fOverflow)
			{
				rFacets.fctEnd.X += wRight - rcBounds.Wdt;
				rFacets.fctEnd.Wdt = rcBounds.Wdt;
			}
			rFacets.fctEnd.Draw(cgo.Surface, x0+rcBounds.Wdt-rFacets.fctEnd.Wdt, y0);
			if (fOverflow)
			{
				rFacets.fctEnd.X -= wRight - rcBounds.Wdt;
				rFacets.fctEnd.Wdt = wRight;
			}
		}
		else
		{
			// zoomed bar
			float fZoom = (float) rcBounds.Hgt / rFacets.fctMiddle.Hgt;
			int32_t x0=cgo.TargetX+rcBounds.x, y0=cgo.TargetY+rcBounds.y;
			int32_t iX = int32_t(fZoom*rFacets.fctBegin.Wdt), w=int32_t(fZoom*rFacets.fctMiddle.Wdt), wOld=rFacets.fctMiddle.Wdt;
			int32_t iRightShowLength = rFacets.fctEnd.Wdt/3;
			rFacets.fctBegin.DrawX(cgo.Surface, x0,y0,int32_t(fZoom*rFacets.fctBegin.Wdt),rcBounds.Hgt);
			while (iX < rcBounds.Wdt-(fZoom*iRightShowLength))
			{
				int32_t w2=std::min<int32_t>(w, rcBounds.Wdt-int32_t(fZoom*iRightShowLength)-iX); rFacets.fctMiddle.Wdt=long(float(w2)/fZoom);
				rFacets.fctMiddle.DrawX(cgo.Surface, x0+iX, y0, w2,rcBounds.Hgt);
				iX += w;
			}
			rFacets.fctMiddle.Wdt=wOld;
			rFacets.fctEnd.DrawX(cgo.Surface, x0+rcBounds.Wdt-int32_t(fZoom*rFacets.fctEnd.Wdt), y0,int32_t(fZoom*rFacets.fctEnd.Wdt),rcBounds.Hgt);
		}
	}

	void Element::DrawVBar(C4TargetFacet &cgo, DynBarFacet &rFacets)
	{
		C4DrawTransform trf(1);
		DrawHVBar(cgo, rFacets, trf, rcBounds.Hgt);
	}

	void Element::DrawHBarByVGfx(C4TargetFacet &cgo, DynBarFacet &rFacets)
	{
		C4DrawTransform trf;
		float fOffX = cgo.TargetX + rcBounds.x + rcBounds.Hgt/2;
		float fOffY = cgo.TargetY + rcBounds.y + rcBounds.Hgt/2;
		trf.SetRotate(-90.0f, fOffX, fOffY);

		DrawHVBar(cgo, rFacets, trf, rcBounds.Wdt);
	}

	void Element::DrawHVBar(C4TargetFacet &cgo, DynBarFacet &rFacets, C4DrawTransform &trf, int32_t iMiddleLength)
	{
		int32_t y0 = cgo.TargetY + rcBounds.y;
		int32_t x0 = cgo.TargetX + rcBounds.x;

		// draw up arrow
		rFacets.fctBegin.DrawT(cgo.Surface, x0, y0, 0, 0, &trf);

		// draw middle part
		int32_t h = rFacets.fctMiddle.Hgt;
		int32_t barHeight = iMiddleLength - (rFacets.fctBegin.Hgt + rFacets.fctEnd.Hgt);

		for (int32_t iY = 0; iY <= barHeight; iY += h)
		{
			int32_t h2 = std::min(h, barHeight - iY);
			rFacets.fctMiddle.Hgt = h2;
			rFacets.fctMiddle.DrawT(cgo.Surface, x0, y0 + rFacets.fctBegin.Hgt + iY, 0, 0, &trf);
		}
		rFacets.fctMiddle.Hgt = h;

		// draw lower arrow
		rFacets.fctEnd.DrawT(cgo.Surface, x0, y0 + iMiddleLength - rFacets.fctEnd.Hgt, 0, 0, &trf);
	}

	C4Rect Element::GetToprightCornerRect(int32_t iWidth, int32_t iHeight, int32_t iHIndent, int32_t iVIndent, int32_t iIndexX)
	{
		// bounds by topright corner of element
		C4Rect rtBounds = (GetContainer() != this) ? GetClientRect() : GetContainedClientRect();
		rtBounds.x += rtBounds.Wdt - (iWidth + iHIndent) * (iIndexX + 1);
		rtBounds.y += iVIndent;
		rtBounds.Wdt = rtBounds.Hgt = iHeight;
		return rtBounds;
	}

	void Element::SetToolTip(const char *szNewTooltip, bool is_immediate)
	{
		// store tooltip
		if (szNewTooltip) ToolTip.Copy(szNewTooltip); else ToolTip.Clear();
		// store immediate flag
		is_immediate_tooltip = is_immediate;
	}

	bool Element::DoContext()
	{
		if (!pContextHandler) return false;
		return pContextHandler->OnContext(this, rcBounds.Wdt/2, rcBounds.Hgt/2);
	}

	const char *Element::GetToolTip()
	{
		// fallback to parent tooltip, if own is not assigned
		return (!pParent || !ToolTip.isNull()) ? ToolTip.getData() : pParent->GetToolTip();
	}

	ContextHandler *Element::GetContextHandler()
	{
		// fallback to parent context, if own is not assigned
		return (!pParent || pContextHandler) ? pContextHandler : pParent->GetContextHandler();
	}

	bool Element::IsInActiveDlg(bool fForKeyboard)
	{
		// get dlg
		Dialog *pDlg=GetDlg();
		if (!pDlg) return false;
		// check if dlg is active
		return pDlg->IsActive(fForKeyboard);
	}


// --------------------------------------------------
// CMouse

	CMouse::CMouse(int32_t iX, int32_t iY) : fActive(true), fActiveInput(false)
	{
		// set pos
		x=iX; y=iY;
		// reset fields
		LDown=MDown=RDown=false;
		dwKeys=0;
		pMouseOverElement = pPrevMouseOverElement = nullptr;
		pDragElement = nullptr;
		ResetToolTipTime();
		// LDownX/Y initialized upon need
	}

	CMouse::~CMouse() = default;

	void CMouse::Input(int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam)
	{
		// pos changed or click issued?
		if (iButton || iX!=x || iY!=y)
		{
			// then hide tooltips for a while
			ResetToolTipTime();
			// and mark as active input device
			fActiveInput = true;
		}
		// copy fields
		x=iX; y=iY; dwKeys=dwKeyParam;
		// update buttons
		switch (iButton)
		{
		case C4MC_Button_LeftDown:   LDown=true;  LDownX=x; LDownY=y; break;
		case C4MC_Button_LeftUp:     LDown=false; break;
		case C4MC_Button_RightDown:  RDown=true;  break;
		case C4MC_Button_RightUp:    RDown=false; break;
		}
	}

	void CMouse::Draw(C4TargetFacet &cgo, TooltipShowState draw_tool_tips)
	{
		// only if owned
		if (!fActive) return;

		// Make sure to draw the cursor without zoom.
		ZoomData GuiZoom;
		pDraw->GetZoom(&GuiZoom);
		const float oldZoom = GuiZoom.Zoom;
		GuiZoom.Zoom = 1.0;
		pDraw->SetZoom(GuiZoom);

		int32_t iOffsetX = -GfxR->fctMouseCursor.Wdt/2;
		int32_t iOffsetY = -GfxR->fctMouseCursor.Hgt/2;
		GfxR->fctMouseCursor.Draw(cgo.Surface,x+iOffsetX,y+iOffsetY,0);
		// ToolTip
		if (pMouseOverElement && draw_tool_tips != TTST_None)
		{
			if (draw_tool_tips == TTST_All || pMouseOverElement->IsImmediateToolTip())
			{
				const char *szTip = pMouseOverElement->GetToolTip();
				if (szTip && *szTip)
				{
					C4TargetFacet cgoTip; cgoTip.Set(cgo.Surface, cgo.X, cgo.Y, cgo.Wdt, cgo.Hgt);
					Screen::DrawToolTip(szTip, cgoTip, x, y);
				}
			}
		}

		// And restore old zoom settings.
		GuiZoom.Zoom = oldZoom;
		pDraw->SetZoom(GuiZoom);
	}

	void CMouse::ReleaseElements()
	{
		// release MouseOver
		if (pMouseOverElement) pMouseOverElement->MouseLeave(*this);
		// release drag
		if (pDragElement)
		{
			int32_t iX, iY; DWORD dwKeys;
			GetLastXY(iX, iY, dwKeys);
			pDragElement->ScreenPos2ClientPos(iX, iY);
			pDragElement->StopDragging(*this, iX, iY, dwKeys);
		}
		pPrevMouseOverElement = pMouseOverElement = pDragElement = nullptr;
	}

	void CMouse::RemoveElement(Element *pChild)
	{
		// clear ptr
		if (pMouseOverElement == pChild)
		{
			pMouseOverElement->MouseLeave(*this); // do leave callback so any tooltip is cleared!
			pMouseOverElement = nullptr;
		}
		if (pPrevMouseOverElement == pChild) pPrevMouseOverElement = nullptr;
		if (pDragElement == pChild) pDragElement = nullptr;
	}

	void CMouse::OnElementGetsInvisible(Element *pChild)
	{
		// clear ptr
		RemoveElement(pChild);
	}


// --------------------------------------------------
// Screen

	void Screen::RemoveElement(Element *pChild)
	{
		// inherited
		Window::RemoveElement(pChild);
		// clear ptrs
		if (pActiveDlg == pChild) { pActiveDlg = nullptr; Mouse.ResetElements(); }
		Mouse.RemoveElement(pChild);
		if (pContext)
		{
			if (pContext == pChild) pContext=nullptr;
			else pContext->RemoveElement(pChild);
		}
	}

	Screen::Screen() : Window(), Mouse(0, 0)
	{
		// no dialog active
		pActiveDlg = nullptr;
		// set static var
		pScreen = this;
	}

	void Screen::Init(int32_t tx, int32_t ty, int32_t twdt, int32_t thgt)
	{
		Mouse.x = tx+twdt/2;
		Mouse.y = ty+thgt/2;
		fZoom = 1.0f;
		// set size - calcs client area as well
		SetBounds(C4Rect(tx,ty,twdt,thgt));
		SetPreferredDlgRect(C4Rect(0,0,twdt,thgt));
	}

	void Screen::Clear()
	{
		Container::Clear();
		// dtor: Close context menu
		AbortContext(false);
		// fields reset
		fExclusive = true;
		fZoom = 1.0f;
	}

	Screen::~Screen()
	{
		// clear singleton
		if (this == pScreen) pScreen = nullptr;
	}

	void Screen::ElementPosChanged(Element *pOfElement)
	{
		// redraw fullscreen BG if dlgs are dragged around in shared mode
		if (!IsExclusive())
			::GraphicsSystem.InvalidateBg();
	}

	void Screen::ShowDialog(Dialog *pDlg, bool fFade)
	{
		assert(pDlg);
		// do place console mode dialogs
		if (!Application.isEditor || pDlg->IsViewportDialog())
			// exclusive or free dlg: center pos
			// evaluate own placement proc first
			if (!pDlg->DoPlacement(this, PreferredDlgRect))
			{
				if (pDlg->IsFreePlaceDialog())
					pDlg->SetPos((GetWidth() - pDlg->GetWidth()) / 2, (GetHeight() - pDlg->GetHeight()) / 2 + pDlg->IsBottomPlacementDialog()*GetHeight()/3);
				else if (IsExclusive())
					pDlg->SetPos((GetWidth() - pDlg->GetWidth()) / 2, (GetHeight() - pDlg->GetHeight()) / 2);
				else
					// non-exclusive mode at preferred viewport pos
					pDlg->SetPos(PreferredDlgRect.x+30, PreferredDlgRect.y+30);
			}
		// add to local component list at correct ordering
		int32_t iNewZ = pDlg->GetZOrdering(); Element *pEl; Dialog *pOtherDlg;
		for (pEl = GetFirst(); pEl; pEl = pEl->GetNext())
			if ((pOtherDlg = pEl->GetDlg()))
				if (pOtherDlg->GetZOrdering() > iNewZ)
					break;
		InsertElement(pDlg, pEl);
		// set as active, if not fading and on top
		if (!fFade && !pEl)
			// but not viewport dialogs!
			if (!pDlg->IsExternalDrawDialog())
				pActiveDlg = pDlg;
		// show it
		pDlg->fOK = false;
		pDlg->fShow = true;
		// mouse focus might have changed
		UpdateMouseFocus();
	}

	void Screen::ActivateDialog(Dialog *pDlg)
	{
		// no change?
		if (pActiveDlg == pDlg) return;
		// in single-mode: release any MouseOver/Drag of previous dlg
		if (IsExclusive())
			Mouse.ReleaseElements();
		// close any context menu
		AbortContext(false);
		// set as active dlg
		pActiveDlg = pDlg;
		// ensure it's last in the list, if it's not a specially ordered dlg
		if (!pDlg->GetZOrdering() && pDlg->GetNext())
			MakeLastElement(pDlg);
	}

	void Screen::CloseDialog(Dialog *pDlg, bool fFade)
	{
		// hide dlg
		if (!fFade) pDlg->fShow = false;
		// kill from active
		if (pActiveDlg == pDlg)
		{
			// release any MouseOver/Drag of previous dlg
			Mouse.ReleaseElements();
			// close context menu: probably belonging to closed dlg anyway
			AbortContext(false);
			// set new active dlg
			pActiveDlg = GetTopDialog();
			// do not set yet if it's fading
			if (pActiveDlg && pActiveDlg->IsFading()) pActiveDlg = nullptr;
		}
		// redraw background; clip update
		::GraphicsSystem.InvalidateBg(); UpdateMouseFocus();
	}

	void Screen::RecheckActiveDialog()
	{
		Dialog *pNewTop = GetTopDialog();
		if (pActiveDlg == pNewTop) return;
		Mouse.ReleaseElements();
		// do not set yet if it's fading
		if (pActiveDlg && pActiveDlg->IsFading()) pActiveDlg = nullptr;
	}

	Dialog *Screen::GetTopDialog()
	{
		// search backwards in component list
		Dialog *pDlg;
		for (Element *pEl = pLast; pEl; pEl = pEl->GetPrev())
			if ((pDlg = pEl->GetDlg()))
				if (pDlg->IsShown())
					return pDlg;
		// no dlg found
		return nullptr;
	}

	void Screen::CloseAllDialogs(bool fWithOK)
	{
		while (pActiveDlg) pActiveDlg->Close(fWithOK);
	}
#ifdef USE_WIN32_WINDOWS
	Dialog *Screen::GetDialog(HWND hWindow)
	{
		// get dialog with matching handle
		Dialog *pDlg;
		for (Element *pEl = pLast; pEl; pEl = pEl->GetPrev())
			if ((pDlg = pEl->GetDlg()))
				if (pDlg->pWindow && pDlg->pWindow->hWindow == hWindow)
					return pDlg;
		return nullptr;
	}
#endif
	Dialog *Screen::GetDialog(C4Window * pWindow)
	{
		// get dialog with matching window
		Dialog *pDlg;
		for (Element *pEl = pLast; pEl; pEl = pEl->GetPrev())
			if ( (pDlg = pEl->GetDlg()) != nullptr)
				if (pDlg->pWindow == pWindow)
					return pDlg;
		return nullptr;
	}
	void Screen::Render(bool fDoBG)
	{
		// get output cgo
		C4TargetFacet cgo;
		cgo.Set(FullScreen.pSurface, rcBounds);
		// draw to it
		Draw(cgo, fDoBG);
	}

	void Screen::RenderMouse(C4TargetFacet &cgo)
	{
		// draw mouse cursor
		// All tool tips hidden during keyboard input. Immediate tooltips hidden if mouse was moving recently.
		Mouse.Draw(cgo, Mouse.IsActiveInput() ? Mouse.IsMouseStill() ? CMouse::TTST_All : CMouse::TTST_Immediate : CMouse::TTST_None);
	}

	void Screen::Draw(C4TargetFacet &cgo, bool fDoBG)
	{
		// draw bg, if this won't be done by a fullscreen dialog
		if (fDoBG)
		{
			Dialog *pFSDlg = GetFullscreenDialog(false);
			if (!pFSDlg || !pFSDlg->HasBackground())
			{
				if (::GraphicsSystem.pLoaderScreen)
					::GraphicsSystem.pLoaderScreen->Draw(cgo, C4LoaderScreen::Flag::BACKGROUND);
				else
					// loader not yet loaded: black BG
					pDraw->DrawBoxDw(cgo.Surface, 0,0, cgo.Wdt+1, cgo.Hgt+1, 0x00000000);
			}
		}
		// draw contents (if GUI-gfx are loaded, which is assumed in GUI-drawing-functions)
		if (IsVisible() && ::GraphicsResource.IsInitialized())
		{
			Window::Draw(cgo);
			if (pContext) pContext->Draw(cgo);
		}
		// draw mouse cursor
		if (!Application.isEditor) RenderMouse(cgo);
	}

	bool Screen::KeyAny()
	{
		// mark keystroke in mouse
		Mouse.ResetActiveInput();
		// key not yet processed
		return false;
	}

	bool Screen::CharIn(const char * c)
	{
		// Special: Tab chars are ignored, because they are always handled as focus advance
		if (c[0] == 0x09) return false;
		// mark in mouse
		Mouse.ResetActiveInput();
		// no processing if focus is not set
		if (!HasKeyboardFocus()) return false;
		// always return true in exclusive mode (which means: key processed)
		bool fResult = IsExclusive();
		// context menu: forward to context
		if (pContext) return pContext->CharIn(c) || fResult;
		// no active dlg?
		if (!pActiveDlg || !pActiveDlg->IsVisible()) return fResult;
		// forward to dialog
		return pActiveDlg->CharIn(c) || fResult;
	}

	void Screen::MouseMove(int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam, class C4Viewport *pVP)
	{
		// Special: Pass to MouseControl if dragging and button is not upped
		if (IsActive() && !::MouseControl.IsDragging())
		{
			bool fResult = MouseInput(iButton, iX, iY, dwKeyParam, nullptr, pVP);
			if (HasMouseFocus()) { SetMouseInGUI(true, true); return; }
			// non-exclusive GUI: inform mouse-control about GUI-result
			SetMouseInGUI(fResult, true);
			// abort if GUI processed it
			if (fResult) return;
		}
		else
			// no GUI: mouse is not in GUI
			SetMouseInGUI(false, true);
		// mouse control enabled?
		if (!::MouseControl.IsActive())
		{
			// enable mouse in GUI, if a mouse-only-dlg is displayed
			if (GetMouseControlledDialogCount())
				SetMouseInGUI(true, true);
			return;
		}
		// Pass on to mouse controlled viewport
		::Viewports.MouseMoveToViewport(iButton, iX, iY, dwKeyParam);
	}

	void Screen::SetMouseInGUI(bool fInGUI, bool fByMouse)
	{
		// inform mouse control and GUI
		Mouse.SetOwnedMouse(fInGUI);
		// initial movement to ensure mouse control pos is correct
		if (!::MouseControl.IsMouseOwned() && !fInGUI && !fByMouse)
		{
			::MouseControl.SetOwnedMouse(true);
			::Viewports.MouseMoveToViewport(C4MC_Button_None, int32_t(::pGUI->Mouse.x*C4GUI::GetZoom()), int32_t(::pGUI->Mouse.y*C4GUI::GetZoom()), ::pGUI->Mouse.dwKeys);
		}
		::MouseControl.SetOwnedMouse(!fInGUI);
	}

	bool Screen::MouseInput(int32_t iButton, int32_t iPxX, int32_t iPxY, DWORD dwKeyParam, Dialog *pForDlg, class C4Viewport *pForVP)
	{
		// convert from screen pixel coordinates to GUI coordinates
		float fZoom = pForDlg ? 1.0f : GetZoom(); // Developer mode dialogs are currently drawn unzoomed
		float fX = float(iPxX) / fZoom;
		float fY = float(iPxY) / fZoom;
		// forward to mouse
		Mouse.Input(iButton, fX, fY, dwKeyParam);

		// dragging
		if (Mouse.pDragElement)
		{
			int32_t iX2=fX, iY2=fY;
			Mouse.pDragElement->ScreenPos2ClientPos(iX2, iY2);
			if (!Mouse.IsLDown())
			{
				// stop dragging
				Mouse.pDragElement->StopDragging(Mouse, iX2, iY2, dwKeyParam);
				Mouse.pDragElement = nullptr;
			}
			else
			{
				// continue dragging
				Mouse.pDragElement->DoDragging(Mouse, iX2, iY2, dwKeyParam);
			}
		}
		// backup previous MouseOver-element
		Mouse.pPrevMouseOverElement = Mouse.pMouseOverElement;
		Mouse.pMouseOverElement = nullptr;
		bool fProcessed = false;
		// active context menu?
		if (!pForVP && pContext && pContext->CtxMouseInput(Mouse, iButton, fX, fY, dwKeyParam))
		{
			// processed by context menu: OK!
		}
		// otherwise: active dlg and inside screen? (or direct forward to specific dlg/viewport dlg)
		else if (rcBounds.Contains(fX, fY) || pForDlg || pForVP)
		{
			// context menu open but mouse down command issued? close context then
			if (pContext && (iButton == C4MC_Button_LeftDown || iButton == C4MC_Button_RightDown))
				AbortContext(true);
			// get client pos
			if (!pForDlg && !pForVP)
			{
				C4Rect &rcClientArea = GetClientRect();
				fX -= rcClientArea.x; fY -= rcClientArea.y;
			}
			// exclusive mode: process active dialog only
			if (IsExclusive() && !pForDlg && !pForVP)
			{
				if (pActiveDlg && pActiveDlg->IsVisible() && !pActiveDlg->IsFading())
				{
					// bounds check to dlg: only if not dragging
					C4Rect &rcDlgBounds = pActiveDlg->GetBounds();
					if (Mouse.IsLDown() || rcDlgBounds.Contains(fX, fY))
						// forward to active dialog
						pActiveDlg->MouseInput(Mouse, iButton, fX - rcDlgBounds.x, fY - rcDlgBounds.y, dwKeyParam);
					else
						Mouse.pMouseOverElement = nullptr;
				}
				else
					// outside dialog: own handling (for screen context menu)
					Window::MouseInput(Mouse, iButton, fX, fY, dwKeyParam);
			}
			else
			{
				// non-exclusive mode: process all dialogs; make them active on left-click
				Dialog *pDlg;
				for (Element *pEl = pLast; pEl; pEl = pEl->GetPrev())
					if ((pDlg = pEl->GetDlg()))
						if (pDlg->IsShown())
						{
							// if specified: process specified dlg only
							if (pForDlg && pDlg != pForDlg) continue;
							// if specified: process specified viewport only
							bool fIsExternalDrawDialog = pDlg->IsExternalDrawDialog();
							C4Viewport *pVP = fIsExternalDrawDialog ? pDlg->GetViewport() : nullptr;
							if (pForVP && pForVP != pVP) continue;
							// calc offset
							C4Rect &rcDlgBounds = pDlg->GetBounds();
							int32_t iOffX=0, iOffY=0;
							// special handling for viewport dialogs
							if (fIsExternalDrawDialog)
							{
								// ignore external drawing dialogs without a viepwort assigned
								if (!pVP) continue;
								// always clip to viewport bounds
								C4Rect rcOut(pVP->GetOutputRect());
								if (!rcOut.Contains(fX + rcBounds.x, fY + rcBounds.y)) continue;
								// viewport dialogs: Offset determined by viewport position
								iOffX = rcOut.x; iOffY = rcOut.y;
							}
							// hit test; or special: dragging possible outside active dialog
							if (rcDlgBounds.Contains(fX-iOffX, fY-iOffY) || (pDlg == pActiveDlg && Mouse.pDragElement && Mouse.pDragElement->GetDlg() == pDlg))
							{
								// Okay; do input
								pDlg->MouseInput(Mouse, iButton, fX - rcDlgBounds.x - iOffX, fY - rcDlgBounds.y - iOffY, dwKeyParam);
								// CAUTION: pDlg may be invalid now!
								// set processed-flag manually
								fProcessed = true;
								// inactive dialogs get activated by clicks
								if (Mouse.IsLDown() && pDlg != pActiveDlg)
									// but not viewport dialogs!
									if (!pDlg->IsExternalDrawDialog())
										ActivateDialog(pDlg);
								// one dlg only; break loop here
								break;
							}
						}
			}
		}

		// check if MouseOver has changed
		if (Mouse.pPrevMouseOverElement != Mouse.pMouseOverElement)
		{
			// send events
			if (Mouse.pPrevMouseOverElement) Mouse.pPrevMouseOverElement->MouseLeave(Mouse);
			if (Mouse.pMouseOverElement) Mouse.pMouseOverElement->MouseEnter(Mouse);
		}
		// return whether anything processed it
		return fProcessed || Mouse.pDragElement || (Mouse.pMouseOverElement && Mouse.pMouseOverElement!=this) || pContext;
	}

	bool Screen::RecheckMouseInput()
	{
		return MouseInput(C4MC_Button_None, Mouse.x, Mouse.y, Mouse.dwKeys, nullptr, nullptr);
	}

	void Screen::UpdateMouseFocus()
	{
		// when exclusive mode has changed: Make sure mouse clip is correct
		::MouseControl.UpdateClip();
	}

	void Screen::DoContext(ContextMenu *pNewCtx, Element *pAtElement, int32_t iX, int32_t iY)
	{
		assert(pNewCtx); assert(pNewCtx != pContext);
		// close previous context menu
		AbortContext(false);
		// element offset
		if (pAtElement) pAtElement->ClientPos2ScreenPos(iX, iY);
		// usually open bottom right
		// check bottom bounds
		if (iY + pNewCtx->GetBounds().Hgt >= GetBounds().Hgt)
		{
			// bottom too narrow: open to top, if height is sufficient
			// otherwise, open to top from bottom screen pos
			if (iY < pNewCtx->GetBounds().Hgt) iY = GetBounds().Hgt;
			iY -= pNewCtx->GetBounds().Hgt;
		}
		// check right bounds likewise
		if (iX + pNewCtx->GetBounds().Wdt >= GetBounds().Wdt)
		{
			// bottom too narrow: open to top, if height is sufficient
			// otherwise, open to top from bottom screen pos
			if (iX < pNewCtx->GetBounds().Wdt) iX = GetBounds().Wdt;
			iX -= pNewCtx->GetBounds().Wdt;
		}
		// open new
		(pContext = pNewCtx)->Open(pAtElement, iX, iY);
	}

	int32_t Screen::GetMouseControlledDialogCount()
	{
		Dialog *pDlg; int32_t iResult=0;
		for (Element *pEl = GetFirst(); pEl; pEl = pEl->GetNext())
			if ((pDlg = pEl->GetDlg()))
				if (pDlg->IsShown() && pDlg->IsMouseControlled())
					++iResult;
		return iResult;
	}

	void Screen::DrawToolTip(const char *szTip, C4TargetFacet &cgo, float x, float y)
	{
		CStdFont *pUseFont = &(::GraphicsResource.TooltipFont);
		StdStrBuf sText;
		pUseFont->BreakMessage(szTip, std::min<int32_t>(C4GUI_MaxToolTipWdt, std::max<int32_t>(cgo.Wdt, 50)), &sText, true);
		// get tooltip rect
		int32_t tWdt,tHgt;
		if (pUseFont->GetTextExtent(sText.getData(), tWdt, tHgt, true))
		{
			tWdt+=6; tHgt+=4;
			int32_t tX, tY;
			if (y < cgo.Y+cgo.TargetY+tHgt+5) tY = std::min<int32_t>(y+5, cgo.TargetY+cgo.Hgt-tHgt); else tY = y-tHgt-5;
			tX = Clamp<int32_t>(x-tWdt/2, cgo.TargetX+cgo.X, cgo.TargetX+cgo.Wdt-tWdt);
			// draw tooltip box
			pDraw->DrawBoxDw(cgo.Surface, tX,tY,tX+tWdt-1,tY+tHgt-2, C4GUI_ToolTipBGColor);
			pDraw->DrawFrameDw(cgo.Surface, tX,tY,tX+tWdt-1,tY+tHgt-1, C4GUI_ToolTipFrameColor);
			// draw tooltip
			pDraw->TextOut(sText.getData(), *pUseFont, 1.0f, cgo.Surface, tX+3,tY+1, C4GUI_ToolTipColor, ALeft);
			// while there's a tooltip, redraw the bg, because it might overlap
			::GraphicsSystem.InvalidateBg();
		}
	}

	bool Screen::HasFullscreenDialog(bool fIncludeFading)
	{
		return !!GetFullscreenDialog(fIncludeFading);
	}

	Dialog *Screen::GetFullscreenDialog(bool fIncludeFading)
	{
		Dialog *pDlg;
		for (Element *pEl = GetFirst(); pEl; pEl = pEl->GetNext())
			if ((pDlg = pEl->GetDlg()))
				if (pDlg->IsVisible())
					if (pDlg->IsFullscreenDialog())
						if (fIncludeFading || !pDlg->IsFading())
							return pDlg;
		return nullptr;
	}

	void Screen::UpdateGamepadGUIControlEnabled()
	{
		// Gamepad is always kept open now.
	}

	Screen TheScreen;

// --------------------------------------------------
// ComponentAligner

	bool ComponentAligner::GetFromTop(int32_t iHgt, int32_t iWdt, C4Rect &rcOut)
	{
		rcOut.x = rcClientArea.x + iMarginX;
		rcOut.y = rcClientArea.y + iMarginY;
		rcOut.Wdt = rcClientArea.Wdt - iMarginX * 2;
		rcOut.Hgt = iHgt;
		int32_t d = iHgt + iMarginY * 2;
		rcClientArea.y += d; rcClientArea.Hgt -= d;
		// get centered in width as specified
		if (iWdt >= 0)
		{
			rcOut.x += (rcOut.Wdt - iWdt) / 2;
			rcOut.Wdt = iWdt;
		}
		return rcClientArea.Hgt >= 0;
	}

	bool ComponentAligner::GetFromLeft(int32_t iWdt, int32_t iHgt, C4Rect &rcOut)
	{
		rcOut.x = rcClientArea.x + iMarginX;
		rcOut.y = rcClientArea.y + iMarginY;
		rcOut.Wdt = iWdt;
		rcOut.Hgt = rcClientArea.Hgt - iMarginY * 2;
		int32_t d = iWdt + iMarginX * 2;
		rcClientArea.x += d; rcClientArea.Wdt -= d;
		// get centered in height as specified
		if (iHgt >= 0)
		{
			rcOut.y += (rcOut.Hgt - iHgt) / 2;
			rcOut.Hgt = iHgt;
		}
		return rcClientArea.Wdt >= 0;
	}

	bool ComponentAligner::GetFromRight(int32_t iWdt, int32_t iHgt, C4Rect &rcOut)
	{
		rcOut.x = rcClientArea.x + rcClientArea.Wdt - iWdt - iMarginX;
		rcOut.y = rcClientArea.y + iMarginY;
		rcOut.Wdt = iWdt;
		rcOut.Hgt = rcClientArea.Hgt - iMarginY * 2;
		rcClientArea.Wdt -= iWdt + iMarginX * 2;
		// get centered in height as specified
		if (iHgt >= 0)
		{
			rcOut.y += (rcOut.Hgt - iHgt) / 2;
			rcOut.Hgt = iHgt;
		}
		return rcClientArea.Wdt >= 0;
	}

	bool ComponentAligner::GetFromBottom(int32_t iHgt, int32_t iWdt, C4Rect &rcOut)
	{
		rcOut.x = rcClientArea.x + iMarginX;
		rcOut.y = rcClientArea.y + rcClientArea.Hgt - iHgt - iMarginY;
		rcOut.Wdt = rcClientArea.Wdt - iMarginX * 2;
		rcOut.Hgt = iHgt;
		rcClientArea.Hgt -= iHgt + iMarginY * 2;
		// get centered in width as specified
		if (iWdt >= 0)
		{
			rcOut.x += (rcOut.Wdt - iWdt) / 2;
			rcOut.Wdt = iWdt;
		}
		return rcClientArea.Hgt >= 0;
	}

	void ComponentAligner::GetAll(C4Rect &rcOut)
	{
		rcOut.x = rcClientArea.x + iMarginX;
		rcOut.y = rcClientArea.y + iMarginY;
		rcOut.Wdt = rcClientArea.Wdt - iMarginX * 2;
		rcOut.Hgt = rcClientArea.Hgt - iMarginY * 2;
	}

	bool ComponentAligner::GetCentered(int32_t iWdt, int32_t iHgt, C4Rect &rcOut)
	{
		rcOut.x = rcClientArea.GetMiddleX() - iWdt/2;
		rcOut.y = rcClientArea.GetMiddleY() - iHgt/2;
		rcOut.Wdt = iWdt;
		rcOut.Hgt = iHgt;
		// range check
		return rcOut.Wdt+iMarginX*2 <= rcClientArea.Wdt && rcOut.Hgt+iMarginY*2 <= rcClientArea.Hgt;
	}

	void ComponentAligner::LogIt(const char *szName)
	{
		LogF("ComponentAligner %s: (%d,%d)+(%d,%d), Margin (%d,%d)", szName, rcClientArea.x, rcClientArea.y, rcClientArea.Wdt, rcClientArea.Hgt, iMarginX, iMarginY);
	}

	C4Rect &ComponentAligner::GetGridCell(int32_t iSectX, int32_t iSectXMax, int32_t iSectY, int32_t iSectYMax, int32_t iSectSizeX, int32_t iSectSizeY, bool fCenterPos, int32_t iSectNumX, int32_t iSectNumY)
	{
		int32_t iSectSizeXO = iSectSizeX, iSectSizeYO = iSectSizeY;
		int32_t iSectSizeXMax = (rcClientArea.Wdt-iMarginX) / iSectXMax - iMarginX;
		int32_t iSectSizeYMax = (rcClientArea.Hgt-iMarginY) / iSectYMax - iMarginY;
		if (iSectSizeX<0 || fCenterPos) iSectSizeX=iSectSizeXMax; else iSectSizeX=std::min<int32_t>(iSectSizeX, iSectSizeXMax);
		if (iSectSizeY<0 || fCenterPos) iSectSizeY=iSectSizeYMax; else iSectSizeY=std::min<int32_t>(iSectSizeY, iSectSizeYMax);
		rcTemp.x = iSectX * (iSectSizeX+iMarginX) + iMarginX + rcClientArea.x;
		rcTemp.y = iSectY * (iSectSizeY+iMarginY) + iMarginY + rcClientArea.y;
		rcTemp.Wdt = iSectSizeX * iSectNumX + iMarginX*(iSectNumX-1); rcTemp.Hgt = iSectSizeY * iSectNumY + iMarginY*(iSectNumY-1);
		if (iSectSizeXO>=0 && fCenterPos)
		{
			rcTemp.x += (iSectSizeX - iSectSizeXO)/2;
			rcTemp.Wdt = iSectSizeXO;
		}
		if (iSectSizeYO>=0 && fCenterPos)
		{
			rcTemp.y += (iSectSizeY - iSectSizeYO)/2;
			rcTemp.Hgt = iSectSizeYO;
		}
		return rcTemp;
	}


// --------------------------------------------------
// Global stuff

	void GUISound(const char *szSound)
	{
		if (Config.Sound.FESamples)
			StartSoundEffect(szSound);
	}


// --------------------------------------------------
// Static vars

	C4Rect ComponentAligner::rcTemp;
	Screen *Screen::pScreen;


} // end of namespace

C4GUIScreen *pGUI = &C4GUI::TheScreen;
