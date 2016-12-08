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
// container for a dynamic number of vertically stacked controls

#include "C4Include.h"
#include "gui/C4Gui.h"

#include "gui/C4MouseControl.h"
#include "graphics/C4Draw.h"
#include <algorithm>

namespace C4GUI
{


// ----------------------------------------------------
// ListBox

	ListBox::ListBox(const C4Rect &rtBounds, int32_t iMultiColItemWidth) : Control(rtBounds), iMultiColItemWidth(iMultiColItemWidth), iColCount(1)
			, pSelectedItem(nullptr), pSelectionChangeHandler(nullptr), pSelectionDblClickHandler(nullptr), fDrawBackground(true), fDrawBorder(false), fSelectionDisabled(false)
	{
		// calc client rect
		UpdateOwnPos();
		// create content scroll window
		pClientWindow = new ScrollWindow(this);
		// calc column count
		UpdateColumnCount();
		// create key bindings
		pKeyContext = new C4KeyBinding(C4KeyCodeEx(K_MENU), "GUIListBoxContext", KEYSCOPE_Gui,
		                               new ControlKeyCB<ListBox>(*this, &ListBox::KeyContext), C4CustomKey::PRIO_Ctrl);
		C4CustomKey::CodeList keys;
		keys.push_back(C4KeyCodeEx(K_UP));
		if (Config.Controls.GamepadGuiControl) ControllerKeys::Up(keys);
		pKeyUp = new C4KeyBinding(keys, "GUIListBoxUp", KEYSCOPE_Gui,
		                          new ControlKeyCB<ListBox>(*this, &ListBox::KeyUp), C4CustomKey::PRIO_Ctrl);
		keys.clear();
		keys.push_back(C4KeyCodeEx(K_DOWN));
		if (Config.Controls.GamepadGuiControl) ControllerKeys::Down(keys);
		pKeyDown = new C4KeyBinding(keys, "GUIListBoxDown", KEYSCOPE_Gui,
		                            new ControlKeyCB<ListBox>(*this, &ListBox::KeyDown), C4CustomKey::PRIO_Ctrl);
		keys.clear();
		keys.push_back(C4KeyCodeEx(K_LEFT));
		if (Config.Controls.GamepadGuiControl) ControllerKeys::Left(keys);
		pKeyLeft = new C4KeyBinding(keys, "GUIListBoxLeft", KEYSCOPE_Gui,
		                            new ControlKeyCB<ListBox>(*this, &ListBox::KeyLeft), C4CustomKey::PRIO_Ctrl);
		keys.clear();
		keys.push_back(C4KeyCodeEx(K_RIGHT));
		if (Config.Controls.GamepadGuiControl) ControllerKeys::Right(keys);
		pKeyRight = new C4KeyBinding(keys, "GUIListBoxRight", KEYSCOPE_Gui,
		                             new ControlKeyCB<ListBox>(*this, &ListBox::KeyRight), C4CustomKey::PRIO_Ctrl);
		pKeyPageUp = new C4KeyBinding(C4KeyCodeEx(K_PAGEUP), "GUIListBoxPageUp", KEYSCOPE_Gui,
		                              new ControlKeyCB<ListBox>(*this, &ListBox::KeyPageUp), C4CustomKey::PRIO_Ctrl);
		pKeyPageDown = new C4KeyBinding(C4KeyCodeEx(K_PAGEDOWN), "GUIListBoxPageDown", KEYSCOPE_Gui,
		                                new ControlKeyCB<ListBox>(*this, &ListBox::KeyPageDown), C4CustomKey::PRIO_Ctrl);
		pKeyHome = new C4KeyBinding(C4KeyCodeEx(K_HOME), "GUIListBoxHome", KEYSCOPE_Gui,
		                            new ControlKeyCB<ListBox>(*this, &ListBox::KeyHome), C4CustomKey::PRIO_Ctrl);
		pKeyEnd = new C4KeyBinding(C4KeyCodeEx(K_END), "GUIListBoxEnd", KEYSCOPE_Gui,
		                           new ControlKeyCB<ListBox>(*this, &ListBox::KeyEnd), C4CustomKey::PRIO_Ctrl);
		// "activate" current item
		keys.clear();
		keys.push_back(C4KeyCodeEx(K_RETURN));
		keys.push_back(C4KeyCodeEx(K_RETURN, KEYS_Alt));
		if (Config.Controls.GamepadGuiControl)
		{
			ControllerKeys::Ok(keys);
		}
		pKeyActivate = new C4KeyBinding(keys, "GUIListActivate", KEYSCOPE_Gui,
		                                new ControlKeyCB<ListBox>(*this, &ListBox::KeyActivate), C4CustomKey::PRIO_Ctrl);
	}

	ListBox::~ListBox()
	{
		delete pKeyActivate;
		delete pKeyEnd;
		delete pKeyHome;
		delete pKeyPageDown;
		delete pKeyPageUp;
		delete pKeyRight;
		delete pKeyLeft;
		delete pKeyDown;
		delete pKeyUp;
		delete pKeyContext;
		if (pSelectionDblClickHandler) pSelectionDblClickHandler->DeRef();
		if (pSelectionChangeHandler) pSelectionChangeHandler->DeRef();
	}

	void ListBox::DrawElement(C4TargetFacet &cgo)
	{
		if (fDrawBackground)
			pDraw->DrawBoxDw(cgo.Surface, cgo.TargetX+rcBounds.x, cgo.TargetY+rcBounds.y, cgo.TargetX+rcBounds.x+rcBounds.Wdt-1, cgo.TargetY+rcBounds.y+rcBounds.Hgt-1, 0x7f000000);
		if (fDrawBorder) Draw3DFrame(cgo);
		// listbox bg: mark selected item
		if (!pClientWindow) return;
		if (pSelectedItem)
		{
			C4Rect rcSelArea = pSelectedItem->GetBounds();
			rcSelArea.x += GetClientRect().x;
			rcSelArea.y += GetClientRect().y + pClientWindow->GetClientRect().y;
			// clip
			if (rcSelArea.y < GetClientRect().y)
			{
				rcSelArea.Hgt -= GetClientRect().y - rcSelArea.y;
				rcSelArea.y = GetClientRect().y;
			}
			rcSelArea.Hgt = std::min(rcSelArea.Hgt, GetClientRect().y + GetClientRect().Hgt - rcSelArea.y);
			// draw
			if (rcSelArea.Hgt>=0)
				pDraw->DrawBoxDw(cgo.Surface, rcSelArea.x+cgo.TargetX, rcSelArea.y+cgo.TargetY,
				                   rcSelArea.x+rcSelArea.Wdt+cgo.TargetX-1, rcSelArea.y+rcSelArea.Hgt+cgo.TargetY-1,
				                   HasDrawFocus() ? C4GUI_ListBoxSelColor : C4GUI_ListBoxInactSelColor);
		}
		// draw delimeter bars
		Element *pCurr = pClientWindow->GetFirst();
		if (!pCurr) return;
		while ((pCurr = pCurr->GetNext()))
			if (pCurr->GetListItemTopSpacingBar())
			{
				int32_t iYSpace = pCurr->GetListItemTopSpacing();
				int32_t iY = pCurr->GetBounds().y + GetClientRect().y + pClientWindow->GetClientRect().y - iYSpace/2;
				int32_t iX0 = pCurr->GetBounds().x + GetClientRect().x + C4GUI_ListBoxBarIndent;
				int32_t iX1 = iX0 + pClientWindow->GetClientRect().Wdt - 2*C4GUI_ListBoxBarIndent;
				// clip
				if (iY < GetClientRect().y || iY >= GetClientRect().y+GetClientRect().Hgt) continue;
				// draw
				pDraw->DrawLineDw(cgo.Surface, (float)(iX0+cgo.TargetX), (float)(iY+cgo.TargetY), (float)(iX1+cgo.TargetX), (float)(iY+cgo.TargetY), C4GUI_ListBoxBarColor);
			}
	}

	void ListBox::MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam)
	{
		// inherited
		Control::MouseInput(rMouse, iButton, iX, iY, dwKeyParam);
		// safety
		if (pClientWindow)
		{
			// check list area bounds
			if (pClientWindow->GetBounds().Contains(iX, iY))
				// left btn down: select item (regardless of key states)
				if (iButton == C4MC_Button_LeftDown || iButton == C4MC_Button_LeftDouble)
				{
					// reset selection
					Element *pPrevSelectedItem = pSelectedItem;
					pSelectedItem = nullptr;
					// get client component the mouse is over
					iX -= GetMarginLeft(); iY -= GetMarginTop();
					iY += pClientWindow->GetScrollY();
					for (Element *pCurr = pClientWindow->GetFirst(); pCurr; pCurr = pCurr->GetNext())
						if (pCurr->GetBounds().Contains(iX, iY))
							pSelectedItem = pCurr;
					// selection change sound
					if (pSelectedItem != pPrevSelectedItem) SelectionChanged(true);
					// item double-clicked? Callback
					if (iButton == C4MC_Button_LeftDouble && pSelectedItem)
						if (pSelectionDblClickHandler) pSelectionDblClickHandler->DoCall(pSelectedItem);
				}
		}
	}

	void ListBox::UpdateColumnCount()
	{
		if (iMultiColItemWidth && pClientWindow)
		{
			// multicoloumn-listbox
			iColCount = std::max<int32_t>(pClientWindow->GetClientRect().Wdt / iMultiColItemWidth, 1);
		}
		else
		{
			// regular 1-col-listbox
			iColCount = 1;
		}
	}

	int32_t ListBox::ContractToElementHeight()
	{
		if (!pClientWindow) return 0;
		// calc superfluous bottom space
		int32_t iExtraSpace = pClientWindow->GetBounds().Hgt - pClientWindow->GetClientRect().Hgt;
		if (iExtraSpace <= 0) return 0;
		// contract by it
		C4Rect rcNewBounds = GetBounds();
		rcNewBounds.Hgt -= iExtraSpace;
		SetBounds(rcNewBounds);
		return iExtraSpace;
	}

	void ListBox::OnGetFocus(bool fByMouse)
	{
		// inherited (tooltip)
		Control::OnGetFocus(fByMouse);
		// select list item if none is selected (only for keyboard; mouse will select with left-click anyway)
		if (!pSelectedItem && pClientWindow && !fByMouse)
		{
			pSelectedItem = pClientWindow->GetFirstContained();
			SelectionChanged(false);
		}
	}

	bool ListBox::KeyContext()
	{
		// key: context menu
		if (pSelectedItem && pSelectedItem->DoContext()) return true;
		return false;
	}

	bool ListBox::KeyUp()
	{
		// key: selection up
		Element *pPrevSelectedItem = pSelectedItem;
		if (!pSelectedItem)
			// select last
			pSelectedItem = pClientWindow->GetLastContained();
		else
		{
			// select prev row
			int32_t cnt = iColCount;
			while (pSelectedItem && cnt--) pSelectedItem = pSelectedItem->GetPrev();
			if (!pSelectedItem) pSelectedItem = pPrevSelectedItem; // was in start row
		}
		// selection might have changed
		if (pSelectedItem != pPrevSelectedItem) SelectionChanged(true);
		return true;
	}

	bool ListBox::KeyDown()
	{
		// key: selection down
		Element *pPrevSelectedItem = pSelectedItem;
		if (!pSelectedItem)
			// select first
			pSelectedItem = pClientWindow->GetFirstContained();
		else
		{
			// select next row
			int32_t cnt = iColCount;
			while (pSelectedItem && cnt--) pSelectedItem = pSelectedItem->GetNext();
			if (!pSelectedItem) pSelectedItem = pPrevSelectedItem; // was in end row
		}
		// selection might have changed
		if (pSelectedItem != pPrevSelectedItem) SelectionChanged(true);
		return true;
	}

	bool ListBox::KeyLeft()
	{
		// key: Selection left
		// only in multi-col-listboxes
		if (!IsMultiColumn()) return false;
		Element *pPrevSelectedItem = pSelectedItem;
		if (!pSelectedItem)
			// select last
			pSelectedItem = pClientWindow->GetLastContained();
		else
		{
			// select prev
			if (pSelectedItem->GetPrev()) pSelectedItem = pSelectedItem->GetPrev();
		}
		// selection might have changed
		if (pSelectedItem != pPrevSelectedItem) SelectionChanged(true);
		return true;
	}

	bool ListBox::KeyRight()
	{
		// key: Selection right
		// only in multi-col-listboxes
		if (!IsMultiColumn()) return false;
		Element *pPrevSelectedItem = pSelectedItem;
		if (!pSelectedItem)
			// select first
			pSelectedItem = pClientWindow->GetFirstContained();
		else
		{
			// select next
			if (pSelectedItem->GetNext()) pSelectedItem = pSelectedItem->GetNext();
		}
		// selection might have changed
		if (pSelectedItem != pPrevSelectedItem) SelectionChanged(true);
		return true;
	}

	bool ListBox::KeyPageDown()
	{
		// key: selection one page down
		// start from first item or selected
		Element *pNextSelectedItem = pSelectedItem ? pSelectedItem : pClientWindow->GetFirstContained(), *pNext;
		if (!pNextSelectedItem) return false;
		if ((pNext = pNextSelectedItem->GetNext()))
		{
			pNextSelectedItem = pNext;
			// if this is not the last, visible item in the list: go down until item is no longer fully in view
			if (pClientWindow->IsRangeInView(pNextSelectedItem->GetBounds().y, pNextSelectedItem->GetBounds().Hgt))
			{
				while ((pNext = pNextSelectedItem->GetNext()))
					if (pClientWindow->IsRangeInView(pNext->GetBounds().y, pNext->GetBounds().Hgt))
						pNextSelectedItem = pNext;
					else
						break;
			}
			else
			{
				// selected item was last visible: Just scroll one page down and select last visible
				pClientWindow->ScrollPages(+1);
				pNextSelectedItem = pClientWindow->GetLastContained();
				while (!pClientWindow->IsRangeInView(pNextSelectedItem->GetBounds().y, pNextSelectedItem->GetBounds().Hgt))
						if ((pNext = pNextSelectedItem->GetPrev())) pNextSelectedItem = pNext; else break;
			}
		}
		// selection might have changed
		if (pSelectedItem != pNextSelectedItem)
		{
			pSelectedItem = pNextSelectedItem;
			SelectionChanged(true);
		}
		return true;
	}

	bool ListBox::KeyPageUp()
	{
		// key: selection one page up
		// start from last item or selected
		Element *pNextSelectedItem = pSelectedItem ? pSelectedItem : pClientWindow->GetLastContained(), *pNext;
		if (!pNextSelectedItem) return false;
		if ((pNext = pNextSelectedItem->GetPrev()))
		{
			pNextSelectedItem = pNext;
			// if this is not the first, visible item in the list: go up until item is no longer fully in view
			if (pClientWindow->IsRangeInView(pNextSelectedItem->GetBounds().y, pNextSelectedItem->GetBounds().Hgt))
			{
				while ((pNext = pNextSelectedItem->GetPrev()))
					if (pClientWindow->IsRangeInView(pNext->GetBounds().y, pNext->GetBounds().Hgt))
						pNextSelectedItem = pNext;
					else
						break;
			}
			else
			{
				// selected item was last visible: Just scroll one page up and select first visible
				pClientWindow->ScrollPages(-1);
				pNextSelectedItem = pClientWindow->GetFirstContained();
				while (!pClientWindow->IsRangeInView(pNextSelectedItem->GetBounds().y, pNextSelectedItem->GetBounds().Hgt))
						if ((pNext = pNextSelectedItem->GetNext())) pNextSelectedItem = pNext; else break;
			}
		}
		// selection might have changed
		if (pSelectedItem != pNextSelectedItem)
		{
			pSelectedItem = pNextSelectedItem;
			SelectionChanged(true);
		}
		return true;
	}

	bool ListBox::KeyHome()
	{
		// key: selection to first item
		Element *pPrevSelectedItem = pSelectedItem;
		pSelectedItem = pClientWindow->GetFirstContained();
		// selection might have changed
		if (pSelectedItem != pPrevSelectedItem) SelectionChanged(true);
		return true;
	}

	bool ListBox::KeyEnd()
	{
		// key: selection to last item
		Element *pPrevSelectedItem = pSelectedItem;
		pSelectedItem = pClientWindow->GetLastContained();
		// selection might have changed
		if (pSelectedItem != pPrevSelectedItem) SelectionChanged(true);
		return true;
	}

	bool ListBox::KeyActivate()
	{
		// process as doubleclick
		if (pSelectedItem && pSelectionDblClickHandler)
		{
			pSelectionDblClickHandler->DoCall(pSelectedItem);
			return true;
		}
		return false;
	}

	void ListBox::ScrollItemInView(Element *pItem)
	{
		// safety
		if (!pItem) return;
		// scroll covered range into view
		pClientWindow->ScrollRangeInView(pItem->GetBounds().y, pItem->GetBounds().Hgt);
	}

	void ListBox::UpdateElementPositions()
	{
		// safety
		if (!pClientWindow) return;
		// first item at zero offset
		Element *pCurr = pClientWindow->GetFirst();
		int iOverallHgt;
		if (pCurr)
		{
			if (!iMultiColItemWidth)
			{
				// Single column box: All stacked vertically
				if (pCurr->GetBounds().y)
				{
					pCurr->GetBounds().y = 0;
					pCurr->UpdateOwnPos();
				}
				if(pCurr->fVisible) iOverallHgt = pCurr->GetBounds().Hgt;
				else iOverallHgt = 0;
				// others stacked under it
				while ((pCurr = pCurr->GetNext()))
				{
					if(!pCurr->fVisible) continue; //Do not reserve space for hidden elements
					int32_t iYSpace = pCurr->GetListItemTopSpacing();
					int32_t iNewY = iOverallHgt + iYSpace;
					iOverallHgt += pCurr->GetBounds().Hgt + iYSpace;
					if (iNewY != pCurr->GetBounds().y)
					{
						pCurr->GetBounds().y = iNewY;
						pCurr->UpdateOwnPos();
					}
				}
			}
			else
			{
				// Multi column box: Keep element size; reposition horizontally+vertically
				int32_t y=0, iLineHgt=0, col=0;
				for (; pCurr; pCurr=pCurr->GetNext())
				{
					const C4Rect &rcCurrBounds = pCurr->GetBounds();
					iLineHgt = std::max<int32_t>(rcCurrBounds.Hgt, iLineHgt);
					int32_t x = col * iMultiColItemWidth;
					if (rcCurrBounds.x != x || rcCurrBounds.y != y || rcCurrBounds.Wdt != iMultiColItemWidth)
						pCurr->SetBounds(C4Rect(x,y,iMultiColItemWidth,rcCurrBounds.Hgt));
					if (++col >= iColCount)
					{
						col = 0;
						y += iLineHgt;
					}
				}
				iOverallHgt = y + iLineHgt;
			}
		}
		else
			iOverallHgt = 0;
		// update scrolling
		pClientWindow->SetClientHeight(iOverallHgt);
	}

	void ListBox::UpdateElementPosition(Element *pOfElement, int32_t iIndent)
	{
		// resize it
		C4Rect &rcChildBounds = pOfElement->GetBounds();
		rcChildBounds.x = iIndent;
		rcChildBounds.Wdt = GetItemWidth() - iIndent ;
		pOfElement->UpdateOwnPos();
		// re-stack elements
		UpdateElementPositions();
	}

	void ListBox::RemoveElement(Element *pChild)
	{
		// inherited
		Control::RemoveElement(pChild);
		// clear selection var
		if (pChild == pSelectedItem)
		{
			pSelectedItem = nullptr;
			SelectionChanged(false);
		}
		// position update in AfterElementRemoval
	}

	bool ListBox::AddElement(Element *pChild, int32_t iIndent)
	{
		// fail if no client window is present
		if (!pClientWindow) return false;
		// add to scroll window
		pClientWindow->AddElement(pChild);
		// resize to horizontal list extents
		C4Rect &rcChildBounds = pChild->GetBounds();
		rcChildBounds.x = iIndent;
		rcChildBounds.Wdt = GetItemWidth() - iIndent ;
		// reposition to end of list
		if (pChild->GetPrev())
		{
			if (iMultiColItemWidth)
			{
				rcChildBounds.y = pChild->GetPrev()->GetBounds().y;
				int32_t col = pChild->GetPrev()->GetBounds().x / iMultiColItemWidth + 1;
				if (col >= iColCount)
				{
					col = 0;
					int32_t cnt = iColCount;
					int32_t iPrevLineHgt = 0;
					Element *pPrevChild = pChild->GetPrev();
					while (cnt-- && pPrevChild)
					{
						iPrevLineHgt = std::max<int32_t>(iPrevLineHgt, pPrevChild->GetBounds().Hgt);
						pPrevChild = pPrevChild->GetPrev();
					}
					rcChildBounds.y += iPrevLineHgt;
				}
				rcChildBounds.x = col * iMultiColItemWidth;
			}
			else
			{
				rcChildBounds.y = pChild->GetPrev()->GetBounds().y + pChild->GetPrev()->GetBounds().Hgt + pChild->GetListItemTopSpacing();
			}
		}
		else
			rcChildBounds.y = 0;
		pChild->UpdateOwnPos();
		// update scrolling
		pClientWindow->SetClientHeight(rcChildBounds.y+rcChildBounds.Hgt);
		// success
		return true;
	}

	bool ListBox::InsertElement(Element *pChild, Element *pInsertBefore, int32_t iIndent)
	{
		// fail if no client window is present
		if (!pClientWindow) return false;
		// add to scroll window
		pClientWindow->InsertElement(pChild, pInsertBefore);
		// resize to horizontal list extents
		C4Rect &rcChildBounds = pChild->GetBounds();
		rcChildBounds.x = iIndent;
		rcChildBounds.Wdt = GetItemWidth() - iIndent ;
		pChild->UpdateOwnPos();
		// update all element positions (and scrolling)
		UpdateElementPositions();
		// done, success
		return true;
	}

	void ListBox::ElementSizeChanged(Element *pOfElement)
	{
		// inherited
		if (pOfElement->GetParent() == this)
		{
			Control::ElementSizeChanged(pOfElement);
			// update col count if list element container was resized
			UpdateColumnCount();
		}
		// update positions of all list items
		UpdateElementPositions();
	}

	void ListBox::ElementPosChanged(Element *pOfElement)
	{
		// inherited
		if (pOfElement->GetParent() == this)
			Control::ElementSizeChanged(pOfElement);
		// update positions of all list items
		UpdateElementPositions();
	}

	void ListBox::SelectionChanged(bool fByUser)
	{
		// selections disabled?
		if (fSelectionDisabled) { pSelectedItem = nullptr; return; }
		// any selection?
		if (pSelectedItem)
		{
			// effect
			if (fByUser) GUISound("UI::Select");
		}
		// callback (caution: May do periluous things...)
		if (pSelectionChangeHandler) pSelectionChangeHandler->DoCall(pSelectedItem);
		// let's hope it wasn't perilous enough to delete this,
		// because scrolling the item into view must be done AFTER the callback, as the callback might resize
		if (pSelectedItem) ScrollItemInView(pSelectedItem);
	}

	void ListBox::SelectEntry(Element *pNewSel, bool fByUser)
	{
		assert(!pNewSel || pNewSel->GetParent() == pClientWindow);
		if (pSelectedItem == pNewSel) return;
		pSelectedItem = pNewSel;
		SelectionChanged(fByUser);
	}

	bool ListBox::CharIn(const char * c)
	{
		// Jump to first/next entry beginning with typed letter
		Element *pSel = GetSelectedItem();
		Element *pStartCheck = pSel;
		if (pSel) pSel = pSel->GetNext();
		if (!pSel)
		{
			pSel = GetFirst();
			if (!pSel) return false;
		}
		while (pSel != pStartCheck && !pSel->CheckNameHotkey(c))
			if (!(pSel = pSel->GetNext()))
				if (pStartCheck)
					// list end reached while another entry had been selected before: Re-check start of list
					pSel = GetFirst();
		// ok, change selection - might do nothing if list was cycled, which is OK
		if (pSel)
		{
			SelectEntry(pSel, true);
			return true;
		}
		return Control::CharIn(c);
	}

	class SortCompareElements
	{
		void *par;
		ListBox::SortFunction SortFunc;

	public:
		SortCompareElements(ListBox::SortFunction SortFunc, void *par) : par(par), SortFunc(SortFunc) {}

		int operator()(const Element *pEl1, const Element *pEl2)
		{ return (*SortFunc)(pEl1, pEl2, par)>0; }
	};

	void ListBox::SortElements(SortFunction SortFunc, void *par)
	{
		// sort list items:
		// create an array of all list items, sort it, and reorder them afterwards
		if (!pClientWindow) return;
		int32_t iElemCount = pClientWindow->GetElementCount();
		if (iElemCount <= 1) return;
		Element **ppElements = new Element *[iElemCount];
		try
		{
			int32_t i=0;
			for (Element *pEl = pClientWindow->GetFirst(); pEl; pEl = pEl->GetNext())
				ppElements[i++] = pEl;
			std::sort(ppElements, ppElements+iElemCount, SortCompareElements(SortFunc, par));
			for (i=0; i<iElemCount; ++i)
				pClientWindow->ReaddElement(ppElements[i]);
		}
		catch (...)
		{
			delete [] ppElements;
			throw;
		}
		delete [] ppElements;
		UpdateElementPositions();
	}

} // end of namespace

