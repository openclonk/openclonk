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
// grouping elements and control base classes

#include "C4Include.h"
#include "gui/C4Gui.h"

#include "gui/C4MouseControl.h"
#include "graphics/C4GraphicsResource.h"
#include "graphics/C4Draw.h"

namespace C4GUI
{

// --------------------------------------------------
// Container

	void Container::Draw(C4TargetFacet &cgo)
	{
		// self visible?
		if (!IsVisible()) return;
		// then draw all visible child elements
		for (Element *pEl = pFirst; pEl; pEl = pEl->pNext)
			if (pEl->fVisible)
			{
				// skip viewport dialogs
				if (!pEl->IsExternalDrawDialog())
				{
					if (pEl->GetDialogWindow())
						pEl->GetDialogWindow()->RequestUpdate();
					else
						pEl->Draw(cgo);
				}
			}
	}

	Container::Container() : Element()
	{
		// zero fields
		pFirst = pLast = nullptr;
	}

	Container::~Container()
	{
		// empty
		Clear();
	}

	void Container::Clear()
	{
		ClearChildren();
	}

	void Container::ClearChildren()
	{
		// delete all items; dtor will update list
		while (pFirst)
		{
			if (pFirst->IsOwnPtrElement())
			{
				// unlink from list
				Element *pANext = pFirst->pNext;
				pFirst->pPrev = pFirst->pNext = nullptr;
				pFirst->pParent = nullptr;
				if ((pFirst = pANext))
					pFirst->pPrev = nullptr;
				else
					pLast = nullptr;
			}
			else
				delete pFirst;
		}
	}

	void Container::RemoveElement(Element *pChild)
	{
		// safety
		if (!pChild) return;
		// inherited
		Element::RemoveElement(pChild);
		// must be from same container
		if (pChild->pParent != this) return;
		// unlink from list
		if (pChild->pPrev) pChild->pPrev->pNext = pChild->pNext; else pFirst = pChild->pNext;
		if (pChild->pNext) pChild->pNext->pPrev = pChild->pPrev; else pLast = pChild->pPrev;
		// unset parent; invalidates pPrev/pNext
		pChild->pParent = nullptr;
		// element has been removed
		AfterElementRemoval();
	}

	void Container::MakeLastElement(Element *pChild)
	{
		// must be from same container
		if (pChild->pParent != this) return;
		// unlink from list
		if (pChild->pPrev) pChild->pPrev->pNext = pChild->pNext; else pFirst = pChild->pNext;
		if (pChild->pNext) pChild->pNext->pPrev = pChild->pPrev; else pLast = pChild->pPrev;
		// readd to front of list
		if (pLast) pLast->pNext = pChild; else pFirst = pChild;
		pChild->pPrev = pLast; pChild->pNext = nullptr; pLast = pChild;
	}

	void Container::AddElement(Element *pChild)
	{
		// safety
		if (!pChild) return;
		// remove from any previous container
		if (pChild->pParent) pChild->pParent->RemoveElement(pChild);
		// add to end of list
		if (pLast) pLast->pNext = pChild; else pFirst = pChild;
		pChild->pPrev = pLast; pChild->pNext = nullptr; pLast = pChild;
		pChild->pParent = this;

		assert(pChild->pNext != pChild);
		assert(pChild->pPrev != pChild);
		assert(pChild->pParent != pChild);
	}

	void Container::ReaddElement(Element *pChild)
	{
		// safety
		if (!pChild || pChild->pParent != this) return;
		// remove from any previous container
		if (pChild->pPrev) pChild->pPrev->pNext = pChild->pNext; else pFirst = pChild->pNext;
		if (pChild->pNext) pChild->pNext->pPrev = pChild->pPrev; else pLast = pChild->pPrev;
		// add to end of list
		if (pLast) pLast->pNext = pChild; else pFirst = pChild;
		pChild->pPrev = pLast; pChild->pNext = nullptr; pLast = pChild;

		assert(pChild->pNext != pChild);
		assert(pChild->pPrev != pChild);
		assert(pChild->pParent != pChild);
	}

	void Container::InsertElement(Element *pChild, Element *pInsertBefore)
	{
		// add?
		if (!pInsertBefore) { AddElement(pChild); return; }
		// safety
		if (!pChild || pInsertBefore->pParent != this) return;
		// remove from any previous container
		if (pChild->pParent) pChild->pParent->RemoveElement(pChild);
		// add before given element
		if ((pChild->pPrev = pInsertBefore->pPrev))
			pInsertBefore->pPrev->pNext = pChild;
		else
			pFirst = pChild;
		pChild->pNext = pInsertBefore; pInsertBefore->pPrev = pChild;
		pChild->pParent = this;

		assert(pChild->pNext != pChild);
		assert(pChild->pPrev != pChild);
		assert(pChild->pParent != pChild);
	}

	Element *Container::GetNextNestedElement(Element *pPrevElement, bool fBackwards)
	{
		if (fBackwards)
		{
			// this is last
			if (pPrevElement == this) return nullptr;
			// no previous given?
			if (!pPrevElement)
				// then use last nested for backwards search
				return GetFirstNestedElement(true);
			// get nested, previous element if present
			if (pPrevElement->pPrev) return pPrevElement->pPrev->GetFirstNestedElement(true);
			// if not, return parent (could be this)
			return pPrevElement->pParent;
		}
		else
		{
			// forward search: first element is this
			if (!pPrevElement) return this;
			// check next nested
			Element *pEl;
			if ((pEl = pPrevElement->GetFirstContained())) return pEl;
			// check next in list, going upwards until this container is reached
			while (pPrevElement && pPrevElement != this)
			{
				if ((pEl = pPrevElement->pNext)) return pEl;
				pPrevElement = pPrevElement->pParent;
			}
			// nothing found
		}
		return nullptr;
	}

	Element *Container::GetFirstNestedElement(bool fBackwards)
	{
		// get first/last in own list
		if (pFirst) return (fBackwards ? pLast : pFirst)->GetFirstNestedElement(fBackwards);
		// no own list: return this one
		return this;
	}

	bool Container::OnHotkey(uint32_t cHotkey)
	{
		if (!IsVisible()) return false;
		// check all nested elements
		for (Element *pEl = pFirst; pEl; pEl=pEl->pNext)
			if (pEl->fVisible)
				if (pEl->OnHotkey(cHotkey)) return true;
		// no match found
		return false;
	}

	Element *Container::GetElementByIndex(int32_t i)
	{
		// get next until end of list or queried index is reached
		// if i is negative or equal or larger than childcount, the loop will never break and nullptr returned
		Element *pEl;
		for (pEl = pFirst; i-- && pEl; pEl=pEl->pNext) {}
		return pEl;
	}

	int32_t Container::GetElementCount()
	{
		int32_t cnt=0;
		for (Element *pEl = pFirst; pEl; pEl=pEl->pNext) ++cnt;
		return cnt;
	}

	bool Container::IsParentOf(Element *pEl)
	{
		// return whether this is the parent container (directly or recursively) of the passed element
		for (Container *pC = pEl->GetParent(); pC; pC = pC->GetParent())
			if (pC == this) return true;
		return false;
	}

	void Container::SetVisibility(bool fToValue)
	{
		// inherited
		Element::SetVisibility(fToValue);
		// remove focus from contained elements
		if (!fToValue)
		{
			Dialog *pDlg = GetDlg();
			if (pDlg)
			{
				Control *pFocus = pDlg->GetFocus();
				if (pFocus)
				{
					if (IsParentOf(pFocus))
					{
						pDlg->SetFocus(nullptr, false);
					}
				}
			}
		}
	}


// --------------------------------------------------
// Window

	void Window::MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam)
	{
		// invisible?
		if (!IsVisible()) return;
		// inherited
		Container::MouseInput(rMouse, iButton, iX, iY, dwKeyParam);
		// get client pos
		C4Rect &rcClientRect = GetClientRect(), &rcBounds = GetBounds();
		iX -= rcClientRect.x - rcBounds.x; iY -= rcClientRect.y - rcBounds.y;
		// forward to topmost child element
		for (Element *pChild = pLast; pChild; pChild = pChild->GetPrev())
			if (pChild->fVisible && pChild->GetBounds().Contains(iX, iY))
			{
				// forward
				pChild->MouseInput(rMouse, iButton, iX - pChild->GetBounds().x, iY - pChild->GetBounds().y, dwKeyParam);
				// forward to one control only
				break;
			}
	}

	void Window::Draw(C4TargetFacet &cgo)
	{
		// invisible?
		if (!IsVisible()) return;
		// draw window itself
		DrawElement(cgo);
		// get target area
		C4Rect &rcClientRect = GetClientRect();
		C4Rect &rcClipArea = (IsComponentOutsideClientArea() ? GetBounds() : GetClientRect());
		// clip to window area
		int clx1, cly1, clx2, cly2;
		pDraw->GetPrimaryClipper(clx1, cly1, clx2, cly2);
		float nclx1 = cgo.TargetX+rcClipArea.x, ncly1 = cgo.TargetY+rcClipArea.y, nclx2 = cgo.TargetX+rcClipArea.x+rcClipArea.Wdt-1, ncly2 = cgo.TargetY+rcClipArea.y+rcClipArea.Hgt-1;
		pDraw->ApplyZoom(nclx1, ncly1);
		pDraw->ApplyZoom(nclx2, ncly2);
		pDraw->SubPrimaryClipper(nclx1, ncly1, nclx2, ncly2);
		// update target area
		cgo.TargetX += rcClientRect.x; cgo.TargetY += rcClientRect.y;
		// draw contents
		Container::Draw(cgo);
		// reset target area
		cgo.TargetX -= rcClientRect.x; cgo.TargetY -= rcClientRect.y;
		// reset clipper
		pDraw->SetPrimaryClipper(clx1, cly1, clx2, cly2);
	}

	Window::Window() : Container()
	{
		UpdateOwnPos();
	}

	void Window::UpdateOwnPos()
	{
		Container::UpdateOwnPos();
		// set client rect
		int32_t iMarginL=GetMarginLeft(), iMarginT=GetMarginTop();
		rcClientRect.Set(rcBounds.x + iMarginL, rcBounds.y + iMarginT, std::max<int32_t>(rcBounds.Wdt - iMarginL - GetMarginRight(), 0), std::max<int32_t>(rcBounds.Hgt - iMarginT - GetMarginBottom(), 0));
	}


// --------------------------------------------------
// ScrollBar

	ScrollBar::ScrollBar(C4Rect &rcBounds, ScrollWindow *pWin) : fAutoHide(false), fHorizontal(false), iCBMaxRange(100), pScrollCallback(nullptr), pCustomGfx(nullptr)
	{
		// set bounds
		this->rcBounds = rcBounds;
		// set initial values
		pScrollWindow = pWin;
		fScrolling = false;
		iScrollThumbSize = C4GUI_ScrollThumbHgt; // vertical
		iScrollPos = 0;
		fTopDown = fBottomDown = false;
		// update scroll bar pos
		Update();
	}

	ScrollBar::ScrollBar(C4Rect &rcBounds, bool fHorizontal, BaseParCallbackHandler<int32_t> *pCB, int32_t iCBMaxRange) : fAutoHide(false), fHorizontal(fHorizontal), iCBMaxRange(iCBMaxRange), pScrollWindow(nullptr), pCustomGfx(nullptr)
	{
		// set bounds
		this->rcBounds = rcBounds;
		// set initial values
		if ((pScrollCallback = pCB)) pScrollCallback->Ref();
		fScrolling = true;
		iScrollThumbSize = fHorizontal ? C4GUI_ScrollThumbWdt : C4GUI_ScrollThumbHgt;
		iScrollPos = 0;
		fTopDown = fBottomDown = false;
	}

	ScrollBar::~ScrollBar()
	{
		if (pScrollWindow) { pScrollWindow->pScrollBar = nullptr; }
		if (pScrollCallback) pScrollCallback->DeRef();
	}

	void ScrollBar::Update()
	{
		// check associated control
		if (pScrollWindow)
		{
			int32_t iVisHgt = pScrollWindow->GetBounds().Hgt;
			int32_t iClientHgt = pScrollWindow->GetClientRect().Hgt;
			if ((fScrolling = (iVisHgt < iClientHgt)))
			{
				// scrolling necessary
				// get vertical scroll pos
				int32_t iMaxWinScroll = iClientHgt - iVisHgt;
				int32_t iMaxBarScroll = GetBounds().Hgt - 2*C4GUI_ScrollArrowHgt - iScrollThumbSize;
				int32_t iWinScroll = pScrollWindow->iScrollY;
				// scroll thumb height is currently hardcoded
				// calc scroll pos
				iScrollPos = Clamp<int32_t>(iMaxBarScroll * iWinScroll / iMaxWinScroll, 0, iMaxBarScroll);
			}
		}
		else fScrolling = !!pScrollCallback;
		// reset buttons
		if (!fScrolling)
			fTopDown = fBottomDown = false;
		// set visibility by scroll status
		if (fAutoHide) SetVisibility(fScrolling);
	}

	void ScrollBar::OnPosChanged()
	{
		int32_t iMaxBarScroll = GetMaxScroll();
		if (!iMaxBarScroll) iMaxBarScroll=1;
		// CB - passes scroll pos
		if (pScrollCallback) pScrollCallback->DoCall(Clamp<int32_t>(iScrollPos * (iCBMaxRange-1) / iMaxBarScroll, 0, (iCBMaxRange-1)));
		// safety
		if (!pScrollWindow || !fScrolling) return;
		// get scrolling values
		assert(!fHorizontal); // nyi
		int32_t iVisHgt = pScrollWindow->GetBounds().Hgt;
		int32_t iClientHgt = pScrollWindow->GetClientRect().Hgt;
		int32_t iMaxWinScroll = iClientHgt - iVisHgt;
		int32_t iWinScroll = pScrollWindow->iScrollY;
		// calc new window scrolling
		int32_t iNewWinScroll = Clamp<int32_t>(iMaxWinScroll * iScrollPos / iMaxBarScroll, 0, iMaxWinScroll);
		// apply it, if it is different
		if (iWinScroll != iNewWinScroll)
			pScrollWindow->SetScroll(iNewWinScroll);
	}

	void ScrollBar::MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam)
	{
		// inherited
		Element::MouseInput(rMouse, iButton, iX, iY, dwKeyParam);
		// not if scrolling is disabled
		if (!fScrolling) return;
		// reset arrow states
		bool fPrevDown = fTopDown || fBottomDown;
		fTopDown = fBottomDown = false;
		// not if dragging
		if (rMouse.pDragElement) return;
		// left mouse button down?
		if (rMouse.IsLDown())
			// non-scroll-direction area check
			if (fHorizontal ? Inside<int32_t>(iY, 0, C4GUI_ScrollBarHgt) : Inside<int32_t>(iX, 0, C4GUI_ScrollBarWdt))
			{
				// scroll-direction area check: up/left arrow
				if (fHorizontal ? Inside<int32_t>(iX, 0, C4GUI_ScrollArrowWdt-1) : Inside<int32_t>(iY, 0, C4GUI_ScrollArrowHgt-1))
					fTopDown = true;
				// check down arrow
				else if (fHorizontal ? Inside<int32_t>(iX, GetBounds().Wdt-C4GUI_ScrollArrowWdt, GetBounds().Wdt-1)
				         : Inside<int32_t>(iY, GetBounds().Hgt-C4GUI_ScrollArrowHgt, GetBounds().Hgt-1))
					fBottomDown = true;
				else if (HasPin() && (fHorizontal ? Inside<int32_t>(iX, C4GUI_ScrollArrowWdt, GetBounds().Wdt-C4GUI_ScrollArrowWdt-1)
				                      : Inside<int32_t>(iY, C4GUI_ScrollArrowHgt, GetBounds().Hgt-C4GUI_ScrollArrowHgt-1)))
				{
					// move thumb here
					iScrollPos = GetScrollByPos(iX, iY);
					// reflect movement in associated window or do CB
					OnPosChanged();
					// start dragging
					rMouse.pDragElement = this;
					GUISound("UI::Select");
				}
			}
		// sound effekt when buttons are pressed
		if ((fTopDown || fBottomDown) != fPrevDown) GUISound("UI::Tick");
	}

	void ScrollBar::DoDragging(CMouse &rMouse, int32_t iX, int32_t iY, DWORD dwKeyParam)
	{
		// move thumb
		iScrollPos = GetScrollByPos(iX, iY);
		// reflect movement in associated window
		OnPosChanged();
	}

	void ScrollBar::MouseLeave(CMouse &rMouse)
	{
		// inherited
		Element::MouseLeave(rMouse);
		// reset button states
		fTopDown = fBottomDown = false;
	}

	void ScrollBar::DrawElement(C4TargetFacet &cgo)
	{
		// do scrolling
		// not quite perfect; but there's no OnIdle, and it's be a bit of overkill starting a timer
		if (fTopDown && fScrolling && iScrollPos>0)
			{ --iScrollPos; OnPosChanged(); }
		if (fBottomDown && fScrolling)
		{
			if (iScrollPos < GetMaxScroll()) { ++iScrollPos; OnPosChanged(); }
		}
		// draw bar
		ScrollBarFacets &rUseGfx = pCustomGfx ? *pCustomGfx : ::GraphicsResource.sfctScroll;
		DynBarFacet bar = rUseGfx.barScroll;
		if (fTopDown) bar.fctBegin = rUseGfx.fctScrollDTop;
		if (fBottomDown) bar.fctEnd = rUseGfx.fctScrollDBottom;
		if (fHorizontal)
			DrawHBarByVGfx(cgo, bar);
		else
			DrawVBar(cgo, bar);
		// draw scroll pin
		if (fScrolling && HasPin())
		{
			if (fHorizontal)
				rUseGfx.fctScrollPin.Draw(cgo.Surface, cgo.TargetX+rcBounds.x+C4GUI_ScrollArrowWdt+iScrollPos, cgo.TargetY+rcBounds.y);
			else
				rUseGfx.fctScrollPin.Draw(cgo.Surface, cgo.TargetX+rcBounds.x, cgo.TargetY+rcBounds.y+C4GUI_ScrollArrowHgt+iScrollPos);
		}
	}


// --------------------------------------------------
// ScrollWindow

	ScrollWindow::ScrollWindow(Window *pParentWindow)
			: Window(), pScrollBar(nullptr), iScrollY(0), iClientHeight(0), fHasBar(true), iFrozen(0)
	{
		// place within client rect
		C4Rect rtBounds = pParentWindow->GetClientRect();
		rtBounds.x = rtBounds.y = 0;
		rtBounds.Wdt -= C4GUI_ScrollBarWdt;
		SetBounds(rtBounds);
		// create scroll bar
		rtBounds.x += rtBounds.Wdt; rtBounds.Wdt = C4GUI_ScrollBarWdt;
		pScrollBar = new ScrollBar(rtBounds, this);
		// add self and scroll bar to window
		if (pParentWindow != this)
		{
			pParentWindow->AddElement(this);
			pParentWindow->AddElement(pScrollBar);
		}
	}

	void ScrollWindow::Update()
	{
		// not if window is being refilled
		if (iFrozen) return;
		// do not scroll outside range
		iScrollY = Clamp<int32_t>(iScrollY, 0, std::max<int32_t>(iClientHeight - GetBounds().Hgt, 0));
		// update client rect
		rcClientRect.x = 0;
		rcClientRect.y = -iScrollY;
		rcClientRect.Wdt = rcBounds.Wdt;
		rcClientRect.Hgt = iClientHeight;
		// update scroll bar
		if (pScrollBar) pScrollBar->Update();
	}

	void ScrollWindow::SetScroll(int32_t iToScroll)
	{
		// set values
		rcClientRect.y = -(iScrollY = iToScroll);
	}

	void ScrollWindow::ScrollToBottom()
	{
		int32_t iVisHgt = GetBounds().Hgt;
		int32_t iClientHgt = GetClientRect().Hgt;
		int32_t iMaxScroll = iClientHgt - iVisHgt;
		if (iScrollY < iMaxScroll)
		{
			// scrolling possible: do it
			iScrollY = iMaxScroll;
			// update (self + bar)
			Update();
		}
	}

	void ScrollWindow::ScrollPages(int iPageCount)
	{
		int32_t iVisHgt = GetBounds().Hgt;
		ScrollBy(iPageCount * iVisHgt);
	}

	void ScrollWindow::ScrollBy(int iAmount)
	{
		int32_t iVisHgt = GetBounds().Hgt;
		int32_t iClientHgt = GetClientRect().Hgt;
		int32_t iMaxScroll = iClientHgt - iVisHgt;
		int iNewScrollY = Clamp<int>(iScrollY + iAmount, 0, iMaxScroll);
		if (iScrollY != iNewScrollY)
		{
			// scrolling possible: do it
			iScrollY = iNewScrollY;
			// update (self + bar)
			Update();
		}
	}

	void ScrollWindow::ScrollRangeInView(int32_t iY, int32_t iHgt)
	{
		// safety bounds
		if (iY<0) iY=0;
		int32_t iClientHgt = GetClientRect().Hgt;
		if (iY+iHgt > iClientHgt) { ScrollToBottom(); return; }
		// check top
		if (iScrollY > iY)
		{
			iScrollY = iY;
			Update(); // update (self+bar)
		}
		else
		{
			// check bottom
			int32_t iVisHgt = GetBounds().Hgt;
			// if no height is given, scroll given Y-pos to top
			if (!iHgt) iHgt = iVisHgt;
			if (iScrollY + iVisHgt < iY + iHgt)
			{
				iScrollY = iY + iHgt - iVisHgt;
				Update(); // update (self+bar)
			}
		}
	}

	bool ScrollWindow::IsRangeInView(int32_t iY, int32_t iHgt)
	{
		// returns whether scrolling range is in view
		// check top
		if (iScrollY > iY) return false;
		// check height
		return iScrollY + GetBounds().Hgt >= iY+iHgt;
	}

	void ScrollWindow::UpdateOwnPos()
	{
		if (!GetParent()) { Update(); return; }
		// place within client rect
		C4Rect rtBounds = GetParent()->GetContainedClientRect();
		rtBounds.x = rtBounds.y = 0;
		if (fHasBar) rtBounds.Wdt -= C4GUI_ScrollBarWdt;
		if (GetBounds() != rtBounds)
		{
			SetBounds(rtBounds);
			// scroll bar
			if (fHasBar)
			{
				rtBounds.x += rtBounds.Wdt; rtBounds.Wdt = C4GUI_ScrollBarWdt;
				pScrollBar->SetBounds(rtBounds);
			}
		}
		// standard updates
		Update();
	}

	void ScrollWindow::SetScrollBarEnabled(bool fToVal, bool noAutomaticPositioning)
	{
		if (fHasBar == fToVal) return;
		pScrollBar->SetVisibility(fHasBar = fToVal);
		// in some cases the windows will already care for the correct positioning themselves (see C4ScriptGuiWindow)
		if (!noAutomaticPositioning)
			UpdateOwnPos();
	}

	void ScrollWindow::MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam)
	{
		// process wheel: Scroll
		if (iButton == C4MC_Button_Wheel)
		{
			short iDelta = (short)(dwKeyParam >> 16);
			ScrollBy(-iDelta);
			return;
		}
		// other mouse input: inherited (forward to children)
		Window::MouseInput(rMouse, iButton, iX, iY, dwKeyParam);
	}


// --------------------------------------------------
// GroupBox

	CStdFont *GroupBox::GetTitleFont() const
	{
		// get font; fallback to GUI caption font
		return pFont ? pFont : &(::GraphicsResource.CaptionFont);
	}

	void GroupBox::DrawElement(C4TargetFacet &cgo)
	{
		// draw background
		if (dwBackClr != 0xffffffff)
		{
			pDraw->DrawBoxDw(cgo.Surface, cgo.TargetX+rcBounds.x, cgo.TargetY+rcBounds.y, cgo.TargetX+rcBounds.x+rcBounds.Wdt-1, cgo.TargetY+rcBounds.y+rcBounds.Hgt-1, dwBackClr);
		}
		// draw title label
		int32_t iBorderYOff = 0;
		int32_t iTitleGapX = 0;
		int32_t iTitleGapWdt = 0;
		if (HasTitle())
		{
			CStdFont *pTitleFont = GetTitleFont();
			iBorderYOff = pTitleFont->GetLineHeight()/2;
			pTitleFont->GetTextExtent(sTitle.getData(), iTitleGapWdt, iTitleGapX, true);
			iTitleGapX = 7; iTitleGapWdt += 4;
			pDraw->TextOut(sTitle.getData(), *pTitleFont, 1.0f, cgo.Surface, cgo.TargetX+rcBounds.x+iTitleGapX+2, cgo.TargetY+rcBounds.y, dwTitleClr);
		}
		// draw frame
		if (dwFrameClr)
		{
			int32_t x1=cgo.TargetX+rcBounds.x,y1=cgo.TargetY+rcBounds.y+iBorderYOff,x2=x1+rcBounds.Wdt,y2=y1+rcBounds.Hgt-iBorderYOff;
			if (iTitleGapWdt)
			{
				for (int i=0; i<2; ++i)
				{
					pDraw->DrawLineDw(cgo.Surface, (float) x1+i, (float)y1, (float)(x1+i), (float)(y2-1), dwFrameClr); // left
					pDraw->DrawLineDw(cgo.Surface, (float) (x1+2), (float)(y1+i), (float)(x1+iTitleGapX), (float)(y1+i), dwFrameClr); // top - left side
					pDraw->DrawLineDw(cgo.Surface, (float) (x1+iTitleGapX+iTitleGapWdt), (float)(y1+i), (float)(x2-3), (float)(y1+i), dwFrameClr); // top - right side
					pDraw->DrawLineDw(cgo.Surface, (float) (x2-1-i), (float)y1, (float)(x2-1-i), (float)(y2-1), dwFrameClr); // right
					pDraw->DrawLineDw(cgo.Surface, (float) (x1+2), (float)(y2-1-i), (float)(x2-3), (float)(y2-1-i), dwFrameClr); // bottom
				}
			}
			else
			{
				pDraw->DrawFrameDw(cgo.Surface, x1, y1, x2, (y2-1), dwFrameClr);
				pDraw->DrawFrameDw(cgo.Surface, (x1+1), (y1+1), (x2-1), (y2-2), dwFrameClr);
			}
		}
		else
			// default frame color
			// 2do: Make this work with titled group boxes
			Draw3DFrame(cgo);
	}


// --------------------------------------------------
// Control

	Control::Control(const C4Rect &rtBounds) : Window()
	{
		// set bounds
		SetBounds(rtBounds);
		// context menu key binding
		pKeyContext = new C4KeyBinding(C4KeyCodeEx(K_MENU), "GUIContext", KEYSCOPE_Gui,
		                               new ControlKeyCB<Control>(*this, &Control::KeyContext), C4CustomKey::PRIO_Ctrl);
	}

	Control::~Control()
	{
		delete pKeyContext;
	}

	void Control::MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam)
	{
		if (!IsVisible()) return;
		// left down on click=focus-components?
		if (IsFocusOnClick() && IsFocusElement()) if (iButton == C4MC_Button_LeftDown && !HasFocus())
			{
				// then set focus
				Dialog *pParentDlg = GetDlg();
				if (pParentDlg)
				{
					// but do not set focus to this if a child control has it already
					Control *pActiveCtrl = pParentDlg->GetFocus();
					if (!pActiveCtrl || !IsParentOf(pActiveCtrl))
						pParentDlg->SetFocus(this, true);
				}
			}
		// inherited - processing child elements
		Window::MouseInput(rMouse, iButton, iX, iY, dwKeyParam);
	}

	bool Control::HasDrawFocus()
	{
		// has focus at all?
		if (!HasFocus()) return false;
		// is screen ready and not in context?
		if (GetScreen() && GetScreen()->HasContext()) return false;
		// get dlg
		Dialog *pDlg=GetDlg();
		// dlg-less control has focus, OK (shouldn't happen)
		if (!pDlg) return true;
		// check if dlg is active
		return pDlg->IsActive(true);
	}

	void Control::DisableFocus()
	{
		// has it any focus at all?
		if (!HasFocus()) return;
		// then de-focus it
		Dialog *pDlg=GetDlg();
		if (!pDlg) return;
		pDlg->AdvanceFocus(true);
	}

} // end of namespace

