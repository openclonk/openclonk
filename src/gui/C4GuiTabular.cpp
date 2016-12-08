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
// tab control

#include "C4Include.h"
#include "gui/C4Gui.h"

#include "graphics/C4FacetEx.h"
#include "gui/C4MouseControl.h"
#include "graphics/C4GraphicsResource.h"
#include "graphics/C4Draw.h"

namespace C4GUI
{


// ----------------------------------------------------
// Tabular::Sheet

	Tabular::Sheet::Sheet(const char *szTitle, const C4Rect &rcBounds, int32_t icoTitle, bool fHasCloseButton, bool fTitleMarkup)
			: Window(), icoTitle(icoTitle), cHotkey(0), dwCaptionClr(0u), fHasCloseButton(fHasCloseButton), fCloseButtonHighlighted(false), fTitleMarkup(fTitleMarkup)
	{
		// store title
		if (szTitle)
		{
			sTitle.Copy(szTitle);
			if (fTitleMarkup) ExpandHotkeyMarkup(sTitle, cHotkey);
		}
		// set bounds
		SetBounds(rcBounds);
	}

	void Tabular::Sheet::DrawCaption(C4TargetFacet &cgo, int32_t x, int32_t y, int32_t iMaxWdt, bool fLarge, bool fActive, bool fFocus, C4Facet *pfctClip, C4Facet *pfctIcon, CStdFont *pUseFont)
	{
		// calculations
		int32_t iTxtHgt, iTxtWdt;
		GetCaptionSize(&iTxtWdt, &iTxtHgt, fLarge, fActive, pfctClip, pfctIcon, pUseFont);
		if (pfctClip) iMaxWdt = iTxtWdt;
		CStdFont &rUseFont = pUseFont ? *pUseFont : (fLarge ? ::GraphicsResource.CaptionFont : ::GraphicsResource.TextFont);
		if (pfctClip && pfctIcon)
		{
			// tab with clip gfx: Icon on top of text
			// x and y mark topleft pos; iTxtWdt and iTxtHgt mark overall size
			pfctClip->Draw(cgo.Surface, x, y);
			int32_t xCenter = x + iTxtWdt/2, yCenter = y + iTxtHgt/2;
			int32_t iLabelHgt = rUseFont.GetLineHeight(); int32_t iIconLabelSpacing = 2;
			int32_t yTop = yCenter - (pfctIcon->Hgt+iIconLabelSpacing+iLabelHgt)/2;
			pfctIcon->Draw(cgo.Surface, xCenter-pfctIcon->Wdt/2, yTop, icoTitle);
			pDraw->TextOut(sTitle.getData(), rUseFont, 1.0f, cgo.Surface, xCenter, yTop + pfctIcon->Hgt+iIconLabelSpacing, fActive ? C4GUI_GfxTabCaptActiveClr : C4GUI_GfxTabCaptInactiveClr , ACenter);
		}
		// focus highlight
		if (fFocus)
		{
			pDraw->SetBlitMode(C4GFXBLIT_ADDITIVE);
			::GraphicsResource.fctButtonHighlightRound.DrawX(cgo.Surface, (fLarge ? x : x - iTxtWdt/2)+5, y+3, (fLarge ? iMaxWdt : iTxtWdt)-10, iTxtHgt-6);
			pDraw->ResetBlitMode();
		}
		if (!(pfctClip && pfctIcon))
		{
			// classical tab without clip
			// icon
			int32_t xo = x;
			if (icoTitle>=0)
			{
				C4Facet cgoIcon(cgo.Surface, x, y+1, iTxtHgt-2, iTxtHgt-2);
				if (fLarge)
				{
					// large caption: x parameter denotes left pos of icon
					x += iTxtHgt + 2;
				}
				else
				{
					// small caption: x parameter denotes drawing center
					// note that iTxtWdt includes the icon (and close button) as well
					cgoIcon.X -= iTxtWdt / 2;
					x += iTxtHgt / 2;
				}
				Icon::GetIconFacet((Icons)icoTitle).Draw(cgoIcon);
			}
			// text
			if (!fLarge && fHasCloseButton) x -= iTxtHgt/2;
			uint32_t dwClr = dwCaptionClr;
			if (!dwClr) dwClr = fActive ? C4GUI_CaptionFontClr : C4GUI_InactCaptionFontClr;
			pDraw->TextOut(sTitle.getData(), rUseFont, fLarge ? 1.2f : 1.0f, cgo.Surface, x, y, dwClr, fLarge ? ALeft : ACenter, fTitleMarkup);
			// close button
			if (fHasCloseButton)
			{
				xo += iTxtWdt / (2 - fLarge) - iTxtHgt + 1;
				C4Facet cgoCloseBtn(cgo.Surface, xo, y+1, iTxtHgt-2, iTxtHgt-2);
				if (!fCloseButtonHighlighted) pDraw->ActivateBlitModulation(0x7f7f7f);
				Icon::GetIconFacet(Ico_Close).Draw(cgoCloseBtn);
				if (!fCloseButtonHighlighted) pDraw->DeactivateBlitModulation();
			}
		}
	}

	void Tabular::Sheet::GetCaptionSize(int32_t *piWdt, int32_t *piHgt, bool fLarge, bool fActive, C4Facet *pfctClip, C4Facet *pfctIcon, CStdFont *pUseFont)
	{
		// caption by gfx?
		if (pfctClip && pfctIcon)
		{
			if (piWdt) *piWdt = Tabular::GetLeftClipSize(pfctClip);
			if (piHgt) *piHgt = pfctClip->Hgt;
			return;
		}
		// caption by text
		int32_t iWdt, iHgt;
		CStdFont &rUseFont = pUseFont ? *pUseFont : (fLarge ? ::GraphicsResource.CaptionFont : ::GraphicsResource.TextFont);
		if (!rUseFont.GetTextExtent(sTitle.getData(), iWdt, iHgt, fTitleMarkup))
		{
			iWdt=70; iHgt=rUseFont.GetLineHeight();
		}
		if (fLarge) { iWdt = iWdt * 6 / 5; iHgt = iHgt * 6 / 5; }
		// add icon width
		if (icoTitle>=0) iWdt += iHgt + fLarge*2;
		// add close button width
		if (fHasCloseButton) iWdt += iHgt + fLarge*2;
		// assign output vars
		if (piWdt) *piWdt = iWdt;
		if (piHgt) *piHgt = iHgt;
	}

	bool Tabular::Sheet::IsPosOnCloseButton(int32_t x, int32_t y, int32_t iCaptWdt, int32_t iCaptHgt, bool fLarge)
	{
		// close button is on right end of tab
		return fHasCloseButton && Inside<int32_t>(x, iCaptWdt-iCaptHgt+1, iCaptWdt-2) && Inside<int32_t>(y, 1, iCaptHgt-2);
	}

	void Tabular::Sheet::SetTitle(const char *szNewTitle)
	{
		if (sTitle == szNewTitle) return;
		if (szNewTitle)
		{
			sTitle.Copy(szNewTitle);
			if (fTitleMarkup) ExpandHotkeyMarkup(sTitle, cHotkey);
		}
		else
		{
			sTitle.Clear();
			cHotkey = '\0';
		}
		Tabular *pTabular = static_cast<Tabular *>(GetParent());
		if (pTabular) pTabular->SheetsChanged();
	}

	bool Tabular::Sheet::IsActiveSheet()
	{
		Tabular *pTabular = static_cast<Tabular *>(GetParent());
		if (pTabular) return pTabular->GetActiveSheet() == this;
		return false;
	}

// ----------------------------------------------------
// Tabular

	Tabular::Tabular(C4Rect &rtBounds, TabPosition eTabPos) : Control(rtBounds), pActiveSheet(nullptr), eTabPos(eTabPos), iMaxTabWidth(0),
			iCaptionLengthTotal(0), iCaptionScrollPos(0), fScrollingLeft(false), fScrollingRight(false), fScrollingLeftDown(false),
			fScrollingRightDown(false), iSheetMargin(4), fDrawSelf(true), pfctBack(nullptr), pfctClip(nullptr), pfctIcons(nullptr), pSheetCaptionFont(nullptr)
	{
		// calc client rect
		UpdateOwnPos();
		// key bindings for tab selection, if this is not an invisible "blind" tabular
		if (eTabPos != tbNone)
		{
			// Ctrl+(Shift-)Tab works with dialog focus only (assumes max one tabular per dialog)
			// Arrow keys work if control is focused only
			C4CustomKey::CodeList Keys;
			Keys.push_back(C4KeyCodeEx(K_UP));
			if (Config.Controls.GamepadGuiControl)
			{
				ControllerKeys::Up(Keys);
			}
			pKeySelUp = new C4KeyBinding(Keys, "GUITabularSelUp", KEYSCOPE_Gui,
			                             new ControlKeyCB<Tabular>(*this, &Tabular::KeySelUp), C4CustomKey::PRIO_Ctrl);

			Keys.clear();
			Keys.push_back(C4KeyCodeEx(K_DOWN));
			if (Config.Controls.GamepadGuiControl)
			{
				ControllerKeys::Down(Keys);
			}
			pKeySelDown = new C4KeyBinding(Keys, "GUITabularSelDown", KEYSCOPE_Gui,
			                               new ControlKeyCB<Tabular>(*this, &Tabular::KeySelDown), C4CustomKey::PRIO_Ctrl);

			pKeySelUp2 = new C4KeyBinding(C4KeyCodeEx(K_TAB, C4KeyShiftState(KEYS_Shift | KEYS_Control)), "GUITabularSelUp2", KEYSCOPE_Gui,
			                              new DlgKeyCB<Tabular>(*this, &Tabular::KeySelUp), C4CustomKey::PRIO_Ctrl);
			pKeySelDown2 = new C4KeyBinding(C4KeyCodeEx(K_TAB, KEYS_Control), "GUITabularSelDown2", KEYSCOPE_Gui,
			                                new DlgKeyCB<Tabular>(*this, &Tabular::KeySelDown), C4CustomKey::PRIO_Ctrl);
			pKeyCloseTab = new C4KeyBinding(C4KeyCodeEx(K_F4, KEYS_Control), "GUITabularCloseTab", KEYSCOPE_Gui,
			                                new DlgKeyCB<Tabular>(*this, &Tabular::KeyCloseTab), C4CustomKey::PRIO_Ctrl);
		}
		else
		{
			pKeySelUp = pKeySelDown = pKeySelUp2 = pKeySelDown2 = pKeyCloseTab = nullptr;
		}
		SheetsChanged();
	}

	Tabular::~Tabular()
	{
		if (pKeyCloseTab) delete pKeyCloseTab;
		if (pKeySelDown2) delete pKeySelDown2;
		if (pKeySelUp2) delete pKeySelUp2;
		if (pKeySelDown) delete pKeySelDown;
		if (pKeySelUp) delete pKeySelUp;
	}

	bool Tabular::KeySelUp()
	{
		// keyboard callback: Select previous sheet
		int32_t iNewSel = GetActiveSheetIndex() - 1;
		if (iNewSel < 0) iNewSel = GetSheetCount() - 1;
		if (iNewSel < 0) return false;
		SelectSheet(iNewSel, true);
		return true;
	}

	bool Tabular::KeySelDown()
	{
		// keyboard callback: Select next sheet
		int32_t iNewSel = GetActiveSheetIndex() + 1, iSheetCount = GetSheetCount();
		if (iNewSel >= iSheetCount)
		{
			if (!iSheetCount) return false;
			else iNewSel = 0;
		}
		SelectSheet(iNewSel, true);
		return true;
	}

	bool Tabular::KeyCloseTab()
	{
		// keyboard callback: Close currnet sheet
		// only for sheets that can be closed
		Sheet *pCurrentSheet = GetActiveSheet();
		if (!pCurrentSheet) return false;
		if (!pCurrentSheet->HasCloseButton()) return false;
		pCurrentSheet->UserClose();
		return true;
	}

	void Tabular::SelectionChanged(bool fByUser)
	{
		Control *pFocusCtrl = nullptr;
		Dialog *pDlg = GetDlg();
		if (pDlg) pFocusCtrl = pDlg->GetFocus();
		// any selection?
		if (pActiveSheet)
		{
			// effect
			if (fByUser) GUISound("UI::Select");
			// update in sheet
			pActiveSheet->OnShown(fByUser);
		}
		// make only active sheet visible
		for (Element *pSheet = GetFirst(); pSheet; pSheet = pSheet->GetNext())
		{
			pSheet->SetVisibility(pSheet == pActiveSheet);
		}
		// if nothing is selected now, but something was selected before, focus new default control
		if (pFocusCtrl && !pDlg->GetFocus()) pDlg->SetFocus(pDlg->GetDefaultControl(), fByUser);
	}

	void Tabular::SheetsChanged()
	{
		Sheet *pSheet;
		if (eTabPos)
		{
			// update iMaxTabWidth by new set of sheet labels
			iSheetOff = 20;
			iMaxTabWidth = 20;
			iSheetSpacing = (eTabPos == tbLeft) ? -10 : 20;
			int32_t iSheetNum=0, iTotalHgt=iSheetOff;
			iCaptionLengthTotal = iSheetOff;
			for (pSheet = (Sheet *) GetFirst(); pSheet; pSheet = (Sheet *) pSheet->GetNext())
			{
				int32_t iTabWidth, iTabHeight;
				pSheet->GetCaptionSize(&iTabWidth, &iTabHeight, HasLargeCaptions(), pSheet == pActiveSheet, pfctClip, pfctIcons, pSheetCaptionFont);
				iTabWidth += (eTabPos == tbLeft) ? 20 : iSheetSpacing;
				iMaxTabWidth = std::max(iTabWidth, iMaxTabWidth);
				if (eTabPos == tbLeft)
				{
					iTotalHgt += iTabHeight;
					if (iSheetNum++) iTotalHgt += iSheetSpacing;
				}
				else
				{
					iCaptionLengthTotal += iTabWidth;
				}
			}
			// update sheet positioning
			if (eTabPos == tbLeft && iTotalHgt > rcBounds.Hgt-GetMarginBottom())
			{
				// sheet captions dont fit - condense them
				iSheetSpacing -= (iTotalHgt-rcBounds.Hgt+GetMarginBottom()-iSheetOff) / iSheetNum;
				iSheetOff = 0;
			}
			else if (eTabPos == tbTop)
			{
			}
		}
		// update all sheet sizes
		UpdateSize();
		// update scrolling range/status
		UpdateScrolling();
	}

	void Tabular::UpdateScrolling()
	{
		// any scrolling necessary?
		int32_t iAvailableTabSpace = rcBounds.Wdt;
		int32_t iScrollPinSize = GetTopSize();
		if (eTabPos != tbTop || iCaptionLengthTotal <= iAvailableTabSpace || iAvailableTabSpace <= iScrollPinSize*2)
		{
			fScrollingLeft = fScrollingRight = fScrollingLeftDown = fScrollingRightDown = false;
			iCaptionScrollPos = 0;
		}
		else
		{
			// must scroll; update scrolling parameters
			fScrollingLeft = !!iCaptionScrollPos;
			if (!fScrollingLeft)
			{
				fScrollingRight = true;
				fScrollingLeftDown = false;
			}
			else
			{
				iAvailableTabSpace -= iScrollPinSize;
				fScrollingRight = (iCaptionLengthTotal - iCaptionScrollPos > iAvailableTabSpace);
				// do not scroll past right end
				if (!fScrollingRight)
				{
					iCaptionScrollPos = iCaptionLengthTotal - iAvailableTabSpace;
					fScrollingRightDown = false;
				}
			}
		}
	}

	void Tabular::DoCaptionScroll(int32_t iDir)
	{
		// store time of scrolling change
		tLastScrollTime = C4TimeMilliseconds::Now();
		// change scrolling within max range
		int32_t iAvailableTabSpace = rcBounds.Wdt;
		int32_t iScrollPinSize = GetTopSize();
		iCaptionScrollPos = Clamp<int32_t>(iCaptionScrollPos + iDir*iAvailableTabSpace/2, 0, iCaptionLengthTotal - iAvailableTabSpace + iScrollPinSize);
		UpdateScrolling();
	}

	void Tabular::DrawElement(C4TargetFacet &cgo)
	{
		if (!fDrawSelf) return;
		bool fGfx = HasGfx();
		// execute scrolling
		bool fCaptionScrollDelayOver = C4TimeMilliseconds::Now() - tLastScrollTime >= C4GUI_TabCaptionScrollTime;
		if ((fScrollingLeftDown || fScrollingRightDown) && fCaptionScrollDelayOver)
			DoCaptionScroll(fScrollingRightDown - fScrollingLeftDown);
		// border
		if (!fGfx) Draw3DFrame(cgo, false, 1, 0xaf, eTabPos!=tbTop, GetTopSize(), eTabPos!=tbLeft, GetLeftSize());
		// calc positions
		int32_t x0 = cgo.TargetX + rcBounds.x + GetLeftSize(),
		        y0 = cgo.TargetY + rcBounds.y + GetTopSize(),
		        x1 = cgo.TargetX + rcBounds.x + rcBounds.Wdt - 1,
		        y1 = cgo.TargetY + rcBounds.y + rcBounds.Hgt - 1;
		// main area BG
		if (!fGfx) pDraw->DrawBoxDw(cgo.Surface, x0,y0,x1,y1, C4GUI_StandardBGColor);
		// no tabs?
		if (!eTabPos)
		{
			if (fGfx)
				pfctBack->DrawX(cgo.Surface, x0, y0, x1-x0+1, y1-y0+1);
			return;
		}
		bool fLeft = (eTabPos == tbLeft);
		// top or left bar
		int32_t d=(fLeft ? y0 : x0)+iSheetOff; // current tab position (leave some space to the left/top)
		int32_t ad0=0,ad1=0, aCptTxX=0, aCptTxY=0;
		// scrolling in captions
		int32_t iScrollSize = GetTopSize();
		if (fScrollingLeft) d -= iCaptionScrollPos + iScrollSize;
		// tabs
		for (Sheet *pSheet = (Sheet *) GetFirst(); pSheet; pSheet = (Sheet *) pSheet->GetNext())
		{
			// get tab size
			int32_t iTabWidth, iTabHeight;
			pSheet->GetCaptionSize(&iTabWidth, &iTabHeight, HasLargeCaptions(), pSheet == pActiveSheet, pfctClip, pfctIcons, pSheetCaptionFont);
			// leave some space around caption
			iTabWidth += fLeft ? 20 : iSheetSpacing;
			iTabHeight += fLeft ? iSheetSpacing : 10;
			// draw caption bg
			if (!fGfx)
			{
				float vtx[8];
				if (fLeft)
				{
					vtx[0] = x0; vtx[1] = d;
					vtx[2] = x0-GetLeftSize(); vtx[3] = d;
					vtx[4] = x0-GetLeftSize(); vtx[5] = d+iTabHeight;
					vtx[6] = x0; vtx[7] = d+iTabHeight;
				}
				else
				{
					vtx[0] = d+1; vtx[1] = y0;
					vtx[2] = d+4+1; vtx[3] = y0-GetTopSize();
					vtx[4] = d+iTabWidth-4; vtx[5] = y0-GetTopSize();
					vtx[6] = d+iTabWidth; vtx[7] = y0;
				}
				DWORD dwClr = (pSheet == pActiveSheet) ? C4GUI_ActiveTabBGColor : C4GUI_StandardBGColor;
				pDraw->DrawQuadDw(cgo.Surface, vtx, dwClr, dwClr, dwClr, dwClr, nullptr);
				// draw caption frame
				// TODO: Switch to PerformMultiLines
				pDraw->DrawLineDw(cgo.Surface, (float)vtx[0]-1     , (float)vtx[1]      , (float)vtx[2]-1    ,(float)vtx[3]        , C4GUI_BorderColorA1);
				pDraw->DrawLineDw(cgo.Surface, (float)vtx[2]-1     , (float)vtx[3]      , (float)vtx[4]-fLeft,(float)vtx[5]        , C4GUI_BorderColorA1);
				pDraw->DrawLineDw(cgo.Surface, (float)vtx[4]       , (float)vtx[5]      , (float)vtx[6]      ,(float)vtx[7]        , C4GUI_BorderColorA1);
				pDraw->DrawLineDw(cgo.Surface, (float)vtx[0]       , (float)vtx[1]+fLeft, (float)vtx[2]      ,(float)vtx[3]+fLeft  , C4GUI_BorderColorA2);
				pDraw->DrawLineDw(cgo.Surface, (float)vtx[2]-!fLeft, (float)vtx[3]+1    , (float)vtx[4]      ,(float)vtx[5]+!fLeft , C4GUI_BorderColorA2);
				pDraw->DrawLineDw(cgo.Surface, (float)vtx[4]+1     , (float)vtx[5]+fLeft, (float)vtx[6]      ,(float)vtx[7]+fLeft  , C4GUI_BorderColorA2);
			}
			// draw caption text
			int32_t iCptTextX = fLeft ? (x0-GetLeftSize()+10) : (d+iTabWidth/2);
			int32_t iCptTextY = fLeft ? (d+iSheetSpacing/2) : (y0-GetTopSize()+2);
			if (pSheet == pActiveSheet)
			{
				// store active sheet pos for border line or later drawing
				ad0=d; ad1=d+(fLeft ? iTabHeight : iTabWidth);
				aCptTxX = iCptTextX; aCptTxY = iCptTextY;
				// draw active caption
				if (!fGfx) pSheet->DrawCaption(cgo, iCptTextX, iCptTextY, iMaxTabWidth, fLeft, true, HasDrawFocus(), nullptr, nullptr, nullptr);
			}
			else
			{
				// draw inactive caption
				pSheet->DrawCaption(cgo, iCptTextX, iCptTextY, iMaxTabWidth, fLeft, false, false, pfctClip, pfctIcons, pSheetCaptionFont);
			}
			// advance position
			d += (fLeft ? iTabHeight : iTabWidth)+2;
		}
		// draw tab border line across everything but active tab
		if (!fGfx) if (ad0||ad1)
			{
				pDraw->DrawLineDw(cgo.Surface, (float)x0  ,(float)y0  ,(float)(fLeft ? x0 : ad0), (float)(fLeft ? ad0 : y0), C4GUI_BorderColorA1);
				pDraw->DrawLineDw(cgo.Surface, (float)(x0+1),(float)(y0+1),(float)((fLeft ? x0 : ad0)+1), (float)((fLeft ? ad0 : y0)+1) , C4GUI_BorderColorA2);
				pDraw->DrawLineDw(cgo.Surface, (float)(fLeft ? x0 : ad1), (float)(fLeft ? ad1 : y0), (float)(fLeft ? x0 : x1), (float)(fLeft ? y1 : y0), C4GUI_BorderColorA1);
				pDraw->DrawLineDw(cgo.Surface, (float)((fLeft ? x0 : ad1)+1), (float)((fLeft ? ad1 : y0)+1), (float)((fLeft ? x0 : x1)+1), (float)((fLeft ? y1 : y0)+1), C4GUI_BorderColorA2);
			}
		// main area bg in gfx: Atop inactive tabs
		if (fGfx)
		{
			pfctBack->DrawX(cgo.Surface, x0, y0, x1-x0+1, y1-y0+1);
			// and active tab on top of that
			if (pActiveSheet)
				pActiveSheet->DrawCaption(cgo, aCptTxX, aCptTxY, iMaxTabWidth, fLeft, true, HasDrawFocus(), pfctClip, pfctIcons, pSheetCaptionFont);
		}
		// scrolling
		if (fScrollingLeft) ::GraphicsResource.fctBigArrows.DrawX(cgo.Surface, x0+iSheetOff,y0-iScrollSize, iScrollSize,iScrollSize, fScrollingLeftDown*2);
		if (fScrollingRight) ::GraphicsResource.fctBigArrows.DrawX(cgo.Surface, x1-iScrollSize,y0-iScrollSize, iScrollSize,iScrollSize, 1+fScrollingRightDown*2);
	}

	void Tabular::MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam)
	{
		// tabular contains controls?
		if (eTabPos)
		{
			bool fLeft = (eTabPos == tbLeft);
			bool fInCaptionArea = ((!fLeft && Inside<int32_t>(iY, 0, GetTopSize())) || (fLeft && Inside<int32_t>(iX, 0, GetLeftSize())));
			if (!fInCaptionArea || iButton == C4MC_Button_LeftUp)
			{
				MouseLeaveCaptionArea();
			}
			// then check for mousedown in coloumn area
			else if ((iButton == C4MC_Button_LeftDown || iButton == C4MC_Button_None) && fInCaptionArea)
			{
				int32_t d=iSheetOff;
				// check inside scrolling buttons
				bool fProcessed = false;
				if (fScrollingLeft || fScrollingRight)
				{
					int32_t iScrollSize = GetTopSize();
					if (iButton == C4MC_Button_LeftDown && fScrollingRight && Inside(iX, rcBounds.Wdt-iScrollSize,rcBounds.Wdt))
					{
						fProcessed = fScrollingRightDown = true;
						GUISound("UI::Select");
						DoCaptionScroll(+1);
					}
					else if (fScrollingLeft)
					{
						if (iButton == C4MC_Button_LeftDown && Inside(iX, d,d+iScrollSize))
						{
							fProcessed = fScrollingLeftDown = true;
							GUISound("UI::Select");
							DoCaptionScroll(-1);
						}
						d -= iCaptionScrollPos + iScrollSize;
					}
				}
				// check on sheet captions
				if (!fProcessed) for (Sheet *pSheet = (Sheet *) GetFirst(); pSheet; pSheet = (Sheet *) pSheet->GetNext())
					{
						// default: Mouse not on close button
						pSheet->SetCloseButtonHighlight(false);
						// get tab width
						int32_t iCaptWidth,iCaptHeight,iTabWidth,iTabHeight;
						pSheet->GetCaptionSize(&iCaptWidth, &iCaptHeight, HasLargeCaptions(), pSheet == pActiveSheet, pfctClip, pfctIcons, pSheetCaptionFont);
						iTabWidth = iCaptWidth + (fLeft ? 20 : iSheetSpacing);
						iTabHeight = iCaptHeight + (fLeft ? iSheetSpacing : 10);
						// check containment in this tab (check rect only, may catch some side-clicks...)
						if ((!fLeft && Inside(iX, d, d+iTabWidth)) || (fLeft && Inside(iY, d, d+iTabHeight)))
						{
							// close button
							if (pSheet->IsPosOnCloseButton(iX-d*!fLeft-(iTabWidth-iCaptWidth)/2, iY-d*fLeft, iCaptWidth, iCaptHeight, HasLargeCaptions()))
							{
								if (iButton == C4MC_Button_LeftDown)
								{
									// Closing: Callback to sheet
									pSheet->UserClose();
								}
								else
									// just moving :Highlight
									pSheet->SetCloseButtonHighlight(true);
							}
							// mouse press outside close button area: Switch sheet
							else if (iButton == C4MC_Button_LeftDown)
							{
								if (pSheet != pActiveSheet)
								{
									pActiveSheet = pSheet;
									SelectionChanged(true);
								}
							}
							break;
						}
						// next tab
						d += (fLeft ? iTabHeight : iTabWidth)+2;
					}
			}
		}
		// inherited
		Control::MouseInput(rMouse, iButton, iX, iY, dwKeyParam);
	}

	void Tabular::MouseLeaveCaptionArea()
	{
		// no more close buttons or scroll buttons highlighted
		for (Sheet *pSheet = (Sheet *) GetFirst(); pSheet; pSheet = (Sheet *) pSheet->GetNext())
		{
			// default: Mouse not on close button
			pSheet->SetCloseButtonHighlight(false);
		}
		if (fScrollingLeftDown || fScrollingRightDown)
		{
			// stop scrolling
			GUISound("UI::Select");
			fScrollingLeftDown = fScrollingRightDown = false;
		}
	}

	void Tabular::MouseLeave(CMouse &rMouse)
	{
		MouseLeaveCaptionArea();
		// inherited
		Control::MouseLeave(rMouse);
	}

	void Tabular::OnGetFocus(bool fByMouse)
	{
		// inherited (tooltip)
		Control::OnGetFocus(fByMouse);
	}

	void Tabular::RemoveElement(Element *pChild)
	{
		// inherited
		Control::RemoveElement(pChild);
		// clear selection var
		if (pChild == pActiveSheet)
		{
			// select new active sheet
			pActiveSheet = (Sheet *) GetFirst();
			SelectionChanged(false);
		}
		// update sheet labels
		SheetsChanged();
	}

	Tabular::Sheet *Tabular::AddSheet(const char *szTitle, int32_t icoTitle)
	{
		// create new sheet in client area
		Sheet *pNewSheet = new Sheet(szTitle, GetContainedClientRect(), icoTitle);
		AddCustomSheet(pNewSheet);
		// k, new sheet ready!
		return pNewSheet;
	}

	void Tabular::AddCustomSheet(Sheet *pAddSheet)
	{
		AddElement(pAddSheet);
		// select it if it's first
		pAddSheet->SetVisibility(!pActiveSheet);
		if (!pActiveSheet) pActiveSheet = pAddSheet;
		// update sheet labels
		SheetsChanged();
	}

	void Tabular::ClearSheets()
	{
		// del all sheets
		Sheet *pSheet;
		while ((pSheet = GetSheet(0))) delete pSheet;
		SheetsChanged();
	}

	void Tabular::SelectSheet(int32_t iIndex, bool fByUser)
	{
		pActiveSheet = GetSheet(iIndex);
		SelectionChanged(fByUser);
	}

	void Tabular::SelectSheet(Sheet *pSelSheet, bool fByUser)
	{
		pActiveSheet = pSelSheet;
		SelectionChanged(fByUser);
	}

	int32_t Tabular::GetActiveSheetIndex()
	{
		int32_t i=-1;
		Sheet *pSheet;
		while ((pSheet = GetSheet(++i))) if (pSheet == pActiveSheet) return i;
		return -1;
	}

	void Tabular::SetGfx(C4Facet *pafctBack, C4Facet *pafctClip, C4Facet *pafctIcons, CStdFont *paSheetCaptionFont, bool fResizeByAspect)
	{
		// set gfx files
		pfctBack=pafctBack;
		pfctClip=pafctClip;
		pfctIcons=pafctIcons;
		pSheetCaptionFont=paSheetCaptionFont;
		// make sure aspect of background is used correctly
		if (pfctBack && fResizeByAspect)
		{
			int32_t iEffWdt = rcBounds.Wdt - GetLeftSize(), iEffHgt = rcBounds.Hgt - GetTopSize();
			if (iEffWdt * pfctBack->Hgt > pfctBack->Wdt * iEffHgt)
			{
				// control is too wide: center it
				int32_t iOversize = iEffWdt - pfctBack->Wdt * iEffHgt / pfctBack->Hgt;
				C4Rect rtBounds = GetBounds();
				rtBounds.x += iOversize/2;
				rtBounds.Wdt -= iOversize;
				SetBounds(rtBounds);
			}
			else
			{
				// control is too tall: cap at bottom
				int32_t iOversize = iEffHgt - pfctBack->Hgt * iEffWdt / pfctBack->Wdt;
				C4Rect rtBounds = GetBounds();
				rtBounds.y += iOversize;
				rtBounds.Hgt -= iOversize;
				SetBounds(rtBounds);
			}
		}
		SheetsChanged();
	}

	void Tabular::UpdateSize()
	{
		Control::UpdateSize();
		// update all sheets
		for (Sheet *pSheet = static_cast<Sheet *>(GetFirst()); pSheet; pSheet = static_cast<Sheet *>(pSheet->GetNext()))
			pSheet->SetBounds(GetContainedClientRect());
	}


} // end of namespace

