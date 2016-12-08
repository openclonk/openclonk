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
// eye candy

#include "C4Include.h"
#include "gui/C4Gui.h"

#include "gui/C4MouseControl.h"
#include "graphics/C4GraphicsResource.h"
#include "graphics/C4Draw.h"

namespace C4GUI
{

// --------------------------------------------------
// Label

	void Label::DrawElement(C4TargetFacet &cgo)
	{
		// print out
		pDraw->TextOut(sText.getData(), *pFont, 1.0f, cgo.Surface, x0 + cgo.TargetX, rcBounds.y + cgo.TargetY, dwFgClr, iAlign, fMarkup);
	}

	Label::Label(const char *szLblText, int32_t iX0, int32_t iTop, int32_t iAlign, DWORD dwFClr, CStdFont *pFont, bool fMakeReadableOnBlack, bool fMarkup)
			: Element(), dwFgClr(dwFClr), x0(iX0), iAlign(iAlign), pFont(pFont), cHotkey(0), fAutosize(true), fMarkup(fMarkup), pClickFocusControl(nullptr)
	{
		// make color readable
		if (fMakeReadableOnBlack) MakeColorReadableOnBlack(dwFgClr);
		// default font
		if (!this->pFont) this->pFont = &::GraphicsResource.TextFont;
		// set top
		rcBounds.y = iTop;
		// update text
		SetText(szLblText);
	}

	Label::Label(const char *szLblText, const C4Rect &rcBounds, int32_t iAlign, DWORD dwFClr, CStdFont *pFont, bool fMakeReadableOnBlack, bool fAutosize, bool fMarkup)
			: Element(), dwFgClr(dwFClr), iAlign(iAlign), pFont(pFont), cHotkey(0), fAutosize(fAutosize), fMarkup(fMarkup), pClickFocusControl(nullptr)
	{
		// make color readable
		if (fMakeReadableOnBlack) MakeColorReadableOnBlack(dwFgClr);
		this->rcBounds = rcBounds;
		// default font
		if (!this->pFont) this->pFont = &::GraphicsResource.TextFont;
		// set x0
		UpdateOwnPos();
		// update text
		SetText(szLblText);
	}

	void Label::MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam)
	{
		// left-click changes focus
		if (pClickFocusControl && iButton == C4MC_Button_LeftDown)
			GetDlg()->SetFocus(pClickFocusControl, true);
		// inherited
		Element::MouseInput(rMouse, iButton, iX, iY, dwKeyParam);
	}

	void Label::SetText(const char *szToText, bool fAllowHotkey)
	{
		// set new text
		if (szToText)
		{
			sText.Copy(szToText);
			// expand hotkey markup
			if (fAllowHotkey && fMarkup) ExpandHotkeyMarkup(sText, cHotkey);
		}
		else
		{
			sText="";
			cHotkey=0;
		}
		// update according to text only if autosize label (not wooden)
		if (!fAutosize) return;
		// get text extents
		pFont->GetTextExtent(sText.getData(), rcBounds.Wdt, rcBounds.Hgt, fMarkup);
		// update pos
		SetX0(x0);
	}

	void Label::UpdateOwnPos()
	{
		// update text drawing pos
		switch (iAlign)
		{
		case ALeft: x0 = rcBounds.x + GetLeftIndent(); break;
		case ACenter: x0 = rcBounds.x+rcBounds.Wdt/2; break;
		case ARight: x0 = rcBounds.x+rcBounds.Wdt; break;
		}
	}

	bool Label::OnHotkey(uint32_t cHotkey)
	{
		// if hotkey matches and focus control is assigned, set focus
		if (this->cHotkey == cHotkey && pClickFocusControl)
		{
			GetDlg()->SetFocus(pClickFocusControl, false);
			return true;
		}
		else return false;
	}

	void Label::SetX0(int32_t iToX0)
	{
		x0 = iToX0;
		// update x-startpos
		switch (iAlign)
		{
		case ALeft: rcBounds.x = x0; break;
		case ACenter: rcBounds.x = x0 - rcBounds.Wdt/2; break;
		case ARight: rcBounds.x = x0 - rcBounds.Wdt; break;
		}
		// size might have changed
		UpdateSize();
	}


// --------------------------------------------------
// WoodenLabel

	void WoodenLabel::DrawElement(C4TargetFacet &cgo)
	{
		// draw wood
		DrawBar(cgo, ::GraphicsResource.barCaption);
		// draw symbol
		if (fctIcon.Surface)
		{
			C4Facet cgoSymbol(cgo.Surface, cgo.TargetX + rcBounds.x + 1, cgo.TargetY + rcBounds.y + 1, rcBounds.Hgt-2, rcBounds.Hgt-2);
			fctIcon.Draw(cgoSymbol);
		}
		// calculations for automatic scrolling
		int32_t iXOff = 0;
		if (iAlign == ALeft) iXOff += 5;
		if (iAutoScrollDelay)
		{
			C4TimeMilliseconds tNow = C4TimeMilliseconds::Now();
			if (tNow >= tLastChangeTime + iAutoScrollDelay)
			{
				if (!iScrollDir) iScrollDir=1;
				int32_t iMaxScroll = std::max<int32_t>(pFont->GetTextWidth(sText.getData(), true) + (x0 - rcBounds.x) + iXOff + GetRightIndent() - rcBounds.Wdt, 0);
				if (iMaxScroll)
				{
					iScrollPos += iScrollDir;
					if (iScrollPos >= iMaxScroll || iScrollPos < 0)
					{
						iScrollDir = -iScrollDir;
						iScrollPos += iScrollDir;
						tLastChangeTime = tNow;
					}
				}
			}
			iXOff -= iScrollPos;
		}
		// print out text; clipped
		pDraw->StorePrimaryClipper();
		pDraw->SetPrimaryClipper(rcBounds.x + GetLeftIndent() + cgo.TargetX, rcBounds.y + cgo.TargetY, rcBounds.x+rcBounds.Wdt - GetRightIndent() + cgo.TargetX, rcBounds.y+rcBounds.Hgt + cgo.TargetY);
		pDraw->TextOut(sText.getData(), *pFont, 1.0f, cgo.Surface, x0 + cgo.TargetX + iXOff, rcBounds.y + cgo.TargetY + (rcBounds.Hgt-pFont->GetLineHeight())/2-1, dwFgClr, iAlign);
		pDraw->RestorePrimaryClipper();
	}

	int32_t WoodenLabel::GetDefaultHeight(CStdFont *pUseFont)
	{
		if (!pUseFont) pUseFont = &(::GraphicsResource.TextFont);
		return std::max<int32_t>(pUseFont->GetLineHeight(), C4GUI_MinWoodBarHgt);
	}

	void WoodenLabel::SetIcon(const C4Facet &rfctIcon)
	{
		// set icon
		fctIcon = rfctIcon;
		// realign text to left for set icons
		if (fctIcon.Surface)
			iAlign = ALeft;
		else
			iAlign = ACenter;
		UpdateOwnPos();
	}

	void WoodenLabel::ResetAutoScroll()
	{
		iScrollPos = iScrollDir = 0;
	}

// --------------------------------------------------
// MultilineLabel

	MultilineLabel::MultilineLabel(const C4Rect &rcBounds, int32_t iMaxLines, int32_t iMaxBuf, const char *szIndentChars, bool fAutoGrow, bool fMarkup) // ctor
			: Element(), Lines(iMaxBuf, iMaxLines, rcBounds.Wdt, szIndentChars, fAutoGrow, fMarkup), fMarkup(fMarkup)
	{
		// set bounds
		this->rcBounds = rcBounds;
		// update height (min height)
		UpdateOwnPos();
	}

	void MultilineLabel::DrawElement(C4TargetFacet &cgo)
	{
		// get clipping
		int iClipX, iClipY, iClipX2, iClipY2;
		pDraw->GetPrimaryClipper(iClipX, iClipY, iClipX2, iClipY2);
		// draw all lines
		int32_t iIndex = 0; const char *szLine;
		int32_t iY = rcBounds.y + cgo.TargetY;
		CStdFont *pLineFont; DWORD dwLineClr; bool fNewParagraph;
		while ((szLine = Lines.GetLine(iIndex, &pLineFont, &dwLineClr, &fNewParagraph)))
		{
			int32_t iFontLineHeight = pLineFont->GetLineHeight();
			// indents between paragraphs
			if (fNewParagraph && iIndex) iY += iFontLineHeight/3;
			// clip
			if (iY > iClipY2) break;
			if (iY >= iClipY-iFontLineHeight)
			{
				// draw line
				pDraw->TextOut(szLine, *pLineFont, 1.0f, cgo.Surface, rcBounds.x + cgo.TargetX, iY, dwLineClr, ALeft, fMarkup);
			}
			// advance line
			iY += iFontLineHeight;
			++iIndex;
		}
	}

	void MultilineLabel::UpdateSize()
	{
		// forward change to line buffer
		Lines.SetLBWidth(rcBounds.Wdt);
		UpdateHeight();
	}

	void MultilineLabel::UpdateHeight()
	{
		// size by line count
		int32_t iIndex = 0; const char *szLine; int32_t iHgt = 0;
		CStdFont *pLineFont; bool fNewPar;
		while ((szLine = Lines.GetLine(iIndex, &pLineFont, nullptr, &fNewPar)))
		{
			int32_t iFontLineHeight = pLineFont->GetLineHeight();
			// indents between separate messages
			if (fNewPar && iIndex) iHgt += iFontLineHeight/3;
			// text line height
			iHgt += iFontLineHeight;
			++iIndex;
		}
		rcBounds.Hgt = std::max<int32_t>(iHgt, 5);
		// update parent container
		Element::UpdateSize();
	}

	void MultilineLabel::AddLine(const char *szLine, CStdFont *pFont, DWORD dwClr, bool fDoUpdate, bool fMakeReadableOnBlack, CStdFont *pCaptionFont)
	{
		// make color readable
		if (fMakeReadableOnBlack) MakeColorReadableOnBlack(dwClr);
		// forward to line buffer
		if (szLine) Lines.AppendLines(szLine, pFont, dwClr, pCaptionFont);
		// adjust height
		if (fDoUpdate) UpdateSize();
	}

	void MultilineLabel::Clear(bool fDoUpdate)
	{
		// forward to line buffer
		Lines.Clear();
		// adjust height
		if (fDoUpdate) UpdateSize();
	}



// --------------------------------------------------
// HorizontalLine

	void HorizontalLine::DrawElement(C4TargetFacet &cgo)
	{
		// draw horizontal line
		int32_t iX1 = rcBounds.x + cgo.TargetX, iX2 = iX1 + rcBounds.Wdt,
		              iY = rcBounds.y + cgo.TargetY;
		pDraw->DrawLineDw(cgo.Surface, (float)(iX1+1), (float)(iY+1), (float)(iX2-1), (float)(iY+1), dwShadowClr);
		pDraw->DrawLineDw(cgo.Surface, (float)iX1, (float)iY, (float)(iX2-2), (float)iY, dwClr);
	}


// --------------------------------------------------
// ProgressBar

	void ProgressBar::DrawElement(C4TargetFacet &cgo)
	{
		// do not draw in negative progress
		if (iProgress<0) return;
		CStdFont &rFont = ::GraphicsResource.TextFont;
		// draw border
		Draw3DFrame(cgo);
		// calc progress width
		int32_t iProgressWdt = (rcBounds.Wdt-4) * iProgress / iMax;
		// draw progress
		::GraphicsResource.fctProgressBar.DrawX(cgo.Surface, cgo.TargetX+rcBounds.x+2, cgo.TargetY+rcBounds.y+2, iProgressWdt, rcBounds.Hgt-2);
		// print out progress text
		char szPrg[32+1];
		sprintf(szPrg, "%i%%", 100 * iProgress / iMax);
		pDraw->TextOut(szPrg, rFont, 1.0f, cgo.Surface, cgo.TargetX+rcBounds.GetMiddleX(), rcBounds.y + cgo.TargetY + (rcBounds.Hgt-rFont.GetLineHeight())/2-1, C4GUI_ProgressBarFontClr, ACenter);
	}


// --------------------------------------------------
// Picture

	Picture::Picture(const C4Rect &rcBounds, bool fAspect) : fCustomDrawClr(false), fAnimate(false)
	{
		// set values
		this->fAspect = fAspect;
		this->rcBounds = rcBounds;
		// no facet yet
	}

	void Picture::DrawElement(C4TargetFacet &cgo)
	{
		// animation?
		C4Facet *pDrawFacet, DrawFacet;
		if (fAnimate)
		{
			if (++iPhaseTime > iDelay)
			{
				int32_t iPhasesX=1, iPhasesY=1;
				Facet.GetPhaseNum(iPhasesX, iPhasesY);
				if (++iAnimationPhase >= iPhasesX) iAnimationPhase = 0;
				iPhaseTime = 0;
			}
			DrawFacet = Facet.GetPhase(iAnimationPhase);
			pDrawFacet = &DrawFacet;
		}
		else
			pDrawFacet = &Facet;
		// draw the facet
		C4Facet cgo2 = cgo;
		cgo2.X = rcBounds.x + cgo.TargetX;
		cgo2.Y = rcBounds.y + cgo.TargetY;
		cgo2.Wdt = rcBounds.Wdt;
		cgo2.Hgt = rcBounds.Hgt;
		if (fCustomDrawClr)
		{
			pDrawFacet->DrawClr(cgo2, fAspect, dwDrawClr);
		}
		else
			pDrawFacet->Draw(cgo2, fAspect);
	}

	bool Picture::EnsureOwnSurface()
	{
		// no surface?
		if (!Facet.Surface) return false;
		// equals face already?
		if (Facet.Surface == &Facet.GetFace()) return true;
		// then create as a copy
		C4Facet cgo = Facet;
		if (!Facet.Create(cgo.Wdt, cgo.Hgt)) return false;
		cgo.Draw(Facet);
		return true;
	}


	void Picture::SetAnimated(bool fEnabled, int iDelay)
	{
		if ((fAnimate = fEnabled))
		{
			// starts cycling through all phases of the specified facet
			iAnimationPhase=iPhaseTime=0;
			this->iDelay = iDelay;
		}
	}



// --------------------------------------------------
// OverlayPicture

	OverlayPicture::OverlayPicture(const C4Rect &rcBounds, bool fAspect, const C4Facet &rOverlayImage, int iBorderSize)
			: Picture(rcBounds, fAspect), iBorderSize(iBorderSize), OverlayImage(rOverlayImage)
	{
	}

	void OverlayPicture::DrawElement(C4TargetFacet &cgo)
	{
		// draw inner image
		C4Facet cgo2 = cgo;
		cgo2.X = rcBounds.x + cgo.TargetX + iBorderSize * rcBounds.Wdt / std::max<int>(OverlayImage.Wdt, 1);
		cgo2.Y = rcBounds.y + cgo.TargetY + iBorderSize * rcBounds.Hgt / std::max<int>(OverlayImage.Hgt, 1);
		cgo2.Wdt = rcBounds.Wdt - 2 * iBorderSize * rcBounds.Wdt / std::max<int>(OverlayImage.Wdt, 1);
		cgo2.Hgt = rcBounds.Hgt - 2 * iBorderSize * rcBounds.Hgt / std::max<int>(OverlayImage.Hgt, 1);
		Facet.Draw(cgo2, fAspect);
		// draw outer image
		cgo2.X = rcBounds.x + cgo.TargetX;
		cgo2.Y = rcBounds.y + cgo.TargetY;
		cgo2.Wdt = rcBounds.Wdt;
		cgo2.Hgt = rcBounds.Hgt;
		OverlayImage.Draw(cgo2, fAspect);
	}


// --------------------------------------------------
// Icon

	Icon::Icon(const C4Rect &rcBounds, Icons icoIconIndex)
			: Picture(rcBounds, true)
	{
		// set icon facet
		SetIcon(icoIconIndex);
	}

	void Icon::SetIcon(Icons icoNewIconIndex)
	{
		// load icon
		SetFacet(GetIconFacet(icoNewIconIndex));
	}

	C4Facet Icon::GetIconFacet(Icons icoIconIndex)
	{
		if (icoIconIndex == Ico_None) return C4Facet();
		C4Facet *rFacet;
		switch (icoIconIndex & ~0xff)
		{
		case Ico_Extended:    rFacet = &::GraphicsResource.fctIconsEx; break;
		case Ico_Controller:  rFacet = &::GraphicsResource.fctControllerIcons; break;
		default:              rFacet = &::GraphicsResource.fctIcons;
		}
		icoIconIndex = Icons(icoIconIndex & 0xff);
		int32_t iXMax, iYMax;
		rFacet->GetPhaseNum(iXMax, iYMax);
		if (!iXMax) iXMax = 6;
		return rFacet->GetPhase(icoIconIndex % iXMax, icoIconIndex / iXMax);
	}


// --------------------------------------------------
// PaintBox

	void PaintBox::MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam)
	{
	}

	void PaintBox::DrawElement(C4TargetFacet &cgo)
	{
		// draw it
		fctPaint.Draw(cgo.Surface, rcBounds.x+cgo.TargetX, rcBounds.y+cgo.TargetY);
	}

	PaintBox::PaintBox(C4Rect &rtBounds, int32_t iSfcWdt, int32_t iSfcHgt)
	{
		// default surface bounds
		if (iSfcWdt<0) iSfcWdt = rtBounds.Wdt;
		if (iSfcHgt<0) iSfcHgt = rtBounds.Hgt;
		// create surface
		if (iSfcWdt<=0 || iSfcHgt<=0) return;
		fctPaint.Create(iSfcWdt, iSfcHgt);
	}

	PaintBox::~PaintBox()
	{
	}


// --------------------------------------------------
// TextWindow

	TextWindow::TextWindow(C4Rect &rtBounds, size_t iPicWdt, size_t iPicHgt, size_t iPicPadding, size_t iMaxLines, size_t iMaxTextLen, const char *szIndentChars, bool fAutoGrow, const C4Facet *pOverlayPic, int iOverlayBorder, bool fMarkup)
			: Control(rtBounds), pLogBuffer(nullptr), fDrawBackground(true), fDrawFrame(true), iPicPadding(iPicPadding)
	{
		// calc client rect
		UpdateOwnPos();
		// create content scroll window
		pClientWindow = new ScrollWindow(this);
		pClientWindow->SetBounds(GetContainedClientRect());
		// create content multiline label
		pLogBuffer = new MultilineLabel(pClientWindow->GetContainedClientRect(), iMaxLines, iMaxTextLen, szIndentChars, fAutoGrow, fMarkup);
		// add to scroll window
		pClientWindow->AddElement(pLogBuffer);
		// update scrolling (for empty buffer)
		pClientWindow->SetClientHeight(1);
		// create content picture, if desired
		C4Rect rcContentSize = pClientWindow->GetClientRect();
		if (iPicWdt && iPicHgt)
		{
			C4Rect rcImage;
			rcImage.x = std::max<int32_t>(rcContentSize.GetMiddleX() - iPicWdt/2, 0);
			rcImage.y = 0;
			rcImage.Wdt = std::min<size_t>(iPicWdt, rcContentSize.Wdt);
			rcImage.Hgt = iPicHgt * rcImage.Wdt / iPicWdt;
			rcContentSize.y += rcImage.Hgt + iPicPadding;
			if (pOverlayPic)
				pTitlePicture = new OverlayPicture(rcImage, false, *pOverlayPic, iOverlayBorder);
			else
				pTitlePicture = new Picture(rcImage, false);
			pClientWindow->AddElement(pTitlePicture);
		}
		else pTitlePicture = nullptr;

		// update size
		UpdateSize();
	}

	void TextWindow::UpdateSize()
	{
		Control::UpdateSize();
		pClientWindow->SetBounds(GetContainedClientRect());
		// resize log buffer pos to horizontal extents
		C4Rect rcChildBounds = pLogBuffer->GetBounds();
		rcChildBounds.x = 0;
		rcChildBounds.y = (pTitlePicture && pTitlePicture->IsVisible()) ? pTitlePicture->GetBounds().Hgt + iPicPadding : 0;
		rcChildBounds.Wdt = pClientWindow->GetClientRect().Wdt;
		pLogBuffer->SetBounds(rcChildBounds);
	}

	void TextWindow::DrawElement(C4TargetFacet &cgo)
	{
		// draw background
		if (fDrawBackground) pDraw->DrawBoxDw(cgo.Surface, cgo.TargetX+rcBounds.x,cgo.TargetY+rcBounds.y,rcBounds.x+rcBounds.Wdt-1+cgo.TargetX,rcBounds.y+rcBounds.Hgt-1+cgo.TargetY,0x7f000000);
		// draw frame
		if (fDrawFrame) Draw3DFrame(cgo);
	}

	void TextWindow::ElementSizeChanged(Element *pOfElement)
	{
		// inherited
		if (pOfElement->GetParent() == this)
			Control::ElementSizeChanged(pOfElement);
		// update size of scroll control
		if (pClientWindow && pLogBuffer)
			pClientWindow->SetClientHeight(pLogBuffer->GetBounds().y + pLogBuffer->GetBounds().Hgt);
	}

	void TextWindow::ElementPosChanged(Element *pOfElement)
	{
		// inherited
		if (pOfElement->GetParent() == this)
			Control::ElementSizeChanged(pOfElement);
		// update size of scroll control
		if (pClientWindow && pLogBuffer)
			pClientWindow->SetClientHeight(pLogBuffer->GetBounds().y + pLogBuffer->GetBounds().Hgt);
	}

	void TextWindow::SetPicture(const C4Facet &rNewPic)
	{
		// update picture
		if (!pTitlePicture) return;
		pTitlePicture->SetFacet(rNewPic);
		// reposition multiline label below picture if any is assigned
		pLogBuffer->GetBounds().y = rNewPic.Surface ? pTitlePicture->GetBounds().Hgt + iPicPadding : 0;
		pLogBuffer->UpdateOwnPos();
		pTitlePicture->SetVisibility(!!rNewPic.Surface);
	}

} // end of namespace

