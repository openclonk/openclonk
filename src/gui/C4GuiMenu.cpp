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
// context menu

#include "C4Include.h"
#include "gui/C4Gui.h"

#include "graphics/C4FacetEx.h"
#include "gui/C4MouseControl.h"
#include "graphics/C4GraphicsResource.h"
#include "graphics/C4Draw.h"

#include "platform/C4Window.h"

namespace C4GUI
{

	int32_t ContextMenu::iGlobalMenuIndex = 0;


// ----------------------------------------------------
// ContextMenu::Entry

	void ContextMenu::Entry::DrawElement(C4TargetFacet &cgo)
	{
		// icon
		if (icoIcon > Ico_None)
		{
			// get icon counts
			int32_t iXMax, iYMax;
			::GraphicsResource.fctIcons.GetPhaseNum(iXMax, iYMax);
			if (!iXMax)
				iXMax = 6;
			// load icon
			const C4Facet &rfctIcon = ::GraphicsResource.fctIcons.GetPhase(icoIcon % iXMax, icoIcon / iXMax);
			rfctIcon.DrawX(cgo.Surface, rcBounds.x + cgo.TargetX, rcBounds.y + cgo.TargetY, rcBounds.Hgt, rcBounds.Hgt);
		}
		// print out label
		if (!!sText)
			pDraw->TextOut(sText.getData(), ::GraphicsResource.TextFont, 1.0f, cgo.Surface, cgo.TargetX+rcBounds.x+GetIconIndent(), rcBounds.y + cgo.TargetY, C4GUI_ContextFontClr, ALeft);
		// submenu arrow
		if (pSubmenuHandler)
		{
			C4Facet &rSubFct = ::GraphicsResource.fctSubmenu;
			rSubFct.Draw(cgo.Surface, cgo.TargetX+rcBounds.x+rcBounds.Wdt - rSubFct.Wdt, cgo.TargetY+rcBounds.y+(rcBounds.Hgt - rSubFct.Hgt)/2);
		}
	}

	ContextMenu::Entry::Entry(const char *szText, Icons icoIcon, MenuHandler *pMenuHandler, ContextHandler *pSubmenuHandler)
			: Element(), cHotkey(0), icoIcon(icoIcon), pMenuHandler(pMenuHandler), pSubmenuHandler(pSubmenuHandler)
	{
		// set text with hotkey
		if (szText)
		{
			sText.Copy(szText);
			ExpandHotkeyMarkup(sText, cHotkey);
			// adjust size
			::GraphicsResource.TextFont.GetTextExtent(sText.getData(), rcBounds.Wdt, rcBounds.Hgt, true);
		}
		else
		{
			rcBounds.Wdt = 40;
			rcBounds.Hgt = ::GraphicsResource.TextFont.GetLineHeight();
		}
		// regard icon
		rcBounds.Wdt += GetIconIndent();
		// submenu arrow
		if (pSubmenuHandler) rcBounds.Wdt += ::GraphicsResource.fctSubmenu.Wdt+2;
	}

// ----------------------------------------------------
// ContextMenu

	ContextMenu::ContextMenu() : Window(), pTarget(nullptr), pSelectedItem(nullptr), pSubmenu(nullptr)
	{
		iMenuIndex = ++iGlobalMenuIndex;
		// set min size
		rcBounds.Wdt=40; rcBounds.Hgt=7;
		// key bindings
		C4CustomKey::CodeList Keys;
		Keys.push_back(C4KeyCodeEx(K_UP));
		if (Config.Controls.GamepadGuiControl)
		{
			ControllerKeys::Up(Keys);
		}
		pKeySelUp = new C4KeyBinding(Keys, "GUIContextSelUp", KEYSCOPE_Gui,
		                             new C4KeyCB<ContextMenu>(*this, &ContextMenu::KeySelUp), C4CustomKey::PRIO_Context);

		Keys.clear();
		Keys.push_back(C4KeyCodeEx(K_DOWN));
		if (Config.Controls.GamepadGuiControl)
		{
			ControllerKeys::Down(Keys);
		}
		pKeySelDown = new C4KeyBinding(Keys, "GUIContextSelDown", KEYSCOPE_Gui,
		                               new C4KeyCB<ContextMenu>(*this, &ContextMenu::KeySelDown), C4CustomKey::PRIO_Context);

		Keys.clear();
		Keys.push_back(C4KeyCodeEx(K_RIGHT));
		if (Config.Controls.GamepadGuiControl)
		{
			ControllerKeys::Right(Keys);
		}
		pKeySubmenu = new C4KeyBinding(Keys, "GUIContextSubmenu", KEYSCOPE_Gui,
		                               new C4KeyCB<ContextMenu>(*this, &ContextMenu::KeySubmenu), C4CustomKey::PRIO_Context);

		Keys.clear();
		Keys.push_back(C4KeyCodeEx(K_LEFT));
		if (Config.Controls.GamepadGuiControl)
		{
			ControllerKeys::Left(Keys);
		}
		pKeyBack = new C4KeyBinding(Keys, "GUIContextBack", KEYSCOPE_Gui,
		                            new C4KeyCB<ContextMenu>(*this, &ContextMenu::KeyBack), C4CustomKey::PRIO_Context);

		Keys.clear();
		Keys.push_back(C4KeyCodeEx(K_ESCAPE));
		if (Config.Controls.GamepadGuiControl)
		{
			ControllerKeys::Cancel(Keys);
		}
		pKeyAbort = new C4KeyBinding(Keys, "GUIContextAbort", KEYSCOPE_Gui,
		                             new C4KeyCB<ContextMenu>(*this, &ContextMenu::KeyAbort), C4CustomKey::PRIO_Context);

		Keys.clear();
		Keys.push_back(C4KeyCodeEx(K_RETURN));
		if (Config.Controls.GamepadGuiControl)
		{
			ControllerKeys::Ok(Keys);
		}
		pKeyConfirm = new C4KeyBinding(Keys, "GUIContextConfirm", KEYSCOPE_Gui,
		                               new C4KeyCB<ContextMenu>(*this, &ContextMenu::KeyConfirm), C4CustomKey::PRIO_Context);

		pKeyHotkey = new C4KeyBinding(C4KeyCodeEx(KEY_Any), "GUIContextHotkey", KEYSCOPE_Gui,
		                              new C4KeyCBPassKey<ContextMenu>(*this, &ContextMenu::KeyHotkey), C4CustomKey::PRIO_Context);
	}

	ContextMenu::~ContextMenu()
	{
		// del any submenu
		if (pSubmenu) { delete pSubmenu; pSubmenu=nullptr; }
		// forward RemoveElement to screen
		Screen *pScreen = GetScreen();
		if (pScreen) pScreen->RemoveElement(this);
		// clear key bindings
		delete pKeySelUp;
		delete pKeySelDown;
		delete pKeySubmenu;
		delete pKeyBack;
		delete pKeyAbort;
		delete pKeyConfirm;
		delete pKeyHotkey;
		// clear children to get appropriate callbacks
		Clear();
	}

	void ContextMenu::Abort(bool fByUser)
	{
		// effect
		if (fByUser) GUISound("UI::Close");
		// simply del menu: dtor will remove itself
		delete this;
	}

	void ContextMenu::DrawElement(C4TargetFacet &cgo)
	{
		// draw context menu bg
		pDraw->DrawBoxDw(cgo.Surface, rcBounds.x+cgo.TargetX, rcBounds.y+cgo.TargetY,
		                   rcBounds.x+rcBounds.Wdt+cgo.TargetX-1, rcBounds.y+rcBounds.Hgt+cgo.TargetY-1,
		                   C4GUI_ContextBGColor);
		// context bg: mark selected item
		if (pSelectedItem)
		{
			// get marked item bounds
			C4Rect rcSelArea = pSelectedItem->GetBounds();
			// do indent
			rcSelArea.x += GetClientRect().x;
			rcSelArea.y += GetClientRect().y;
			// draw
			pDraw->DrawBoxDw(cgo.Surface, rcSelArea.x+cgo.TargetX, rcSelArea.y+cgo.TargetY,
			                   rcSelArea.x+rcSelArea.Wdt+cgo.TargetX-1, rcSelArea.y+rcSelArea.Hgt+cgo.TargetY-1,
			                   C4GUI_ContextSelColor);
		}
		// draw frame
		Draw3DFrame(cgo);
	}

	void ContextMenu::MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam)
	{
		// inherited
		Window::MouseInput(rMouse, iButton, iX, iY, dwKeyParam);
		// mouse is in client area?
		if (GetClientRect().Contains(iX+rcBounds.x, iY+rcBounds.y))
		{
			// reset selection
			Element *pPrevSelectedItem = pSelectedItem;
			pSelectedItem = nullptr;
			// get client component the mouse is over
			iX -= GetMarginLeft(); iY -= GetMarginTop();
			for (Element *pCurr = GetFirst(); pCurr; pCurr = pCurr->GetNext())
				if (pCurr->GetBounds().Contains(iX, iY))
					pSelectedItem = pCurr;
			// selection change sound
			if (pSelectedItem != pPrevSelectedItem)
			{
				SelectionChanged(true);
				// selection by mouse: Check whether submenu can be opened
				CheckOpenSubmenu();
			}
			// check mouse click
			if (iButton == C4MC_Button_LeftDown)
				{ DoOK(); return; }
		}
	}

	void ContextMenu::MouseLeaveEntry(CMouse &rMouse, Entry *pOldEntry)
	{
		// no submenu open? then deselect any selected item
		if (pOldEntry==pSelectedItem && !pSubmenu)
		{
			pSelectedItem = nullptr;
			SelectionChanged(true);
		}
	}

	bool ContextMenu::KeySelUp()
	{
		// not if focus is in submenu
		if (pSubmenu) return false;
		Element *pPrevSelectedItem = pSelectedItem;
		// select prev
		if (pSelectedItem) pSelectedItem = pSelectedItem->GetPrev();
		// nothing selected or beginning reached: cycle
		if (!pSelectedItem) pSelectedItem = GetLastContained();
		// selection might have changed
		if (pSelectedItem != pPrevSelectedItem) SelectionChanged(true);
		return true;
	}

	bool ContextMenu::KeySelDown()
	{
		// not if focus is in submenu
		if (pSubmenu) return false;
		Element *pPrevSelectedItem = pSelectedItem;
		// select next
		if (pSelectedItem) pSelectedItem = pSelectedItem->GetNext();
		// nothing selected or end reached: cycle
		if (!pSelectedItem) pSelectedItem = GetFirstContained();
		// selection might have changed
		if (pSelectedItem != pPrevSelectedItem) SelectionChanged(true);
		return true;
	}

	bool ContextMenu::KeySubmenu()
	{
		// not if focus is in submenu
		if (pSubmenu) return false;
		CheckOpenSubmenu();
		return true;
	}

	bool ContextMenu::KeyBack()
	{
		// not if focus is in submenu
		if (pSubmenu) return false;
		// close submenu on keyboard input
		if (IsSubmenu()) { Abort(true); return true; }
		return false;
	}

	bool ContextMenu::KeyAbort()
	{
		// not if focus is in submenu
		if (pSubmenu) return false;
		Abort(true);
		return true;
	}

	bool ContextMenu::KeyConfirm()
	{
		// not if focus is in submenu
		if (pSubmenu) return false;
		CheckOpenSubmenu();
		DoOK();
		return true;
	}

	bool ContextMenu::KeyHotkey(const C4KeyCodeEx &key)
	{
		// not if focus is in submenu
		if (pSubmenu) return false;
		Element *pPrevSelectedItem = pSelectedItem;
		StdStrBuf sKey = C4KeyCodeEx::KeyCode2String(key.Key, true, true);
		// do hotkey procs for standard alphanumerics only
		if (sKey.getLength() != 1) return false;
		WORD wKey = WORD(*sKey.getData());
		if (Inside<C4KeyCode, C4KeyCode, C4KeyCode>(wKey, 'A', 'Z') || Inside<C4KeyCode, C4KeyCode, C4KeyCode>(wKey, '0', '9'))
		{
			// process hotkeys
			uint32_t ch = wKey;
			for (Element *pCurr = GetFirst(); pCurr; pCurr = pCurr->GetNext())
				if (pCurr->OnHotkey(ch))
				{
					pSelectedItem = pCurr;
					if (pSelectedItem != pPrevSelectedItem) SelectionChanged(true);
					CheckOpenSubmenu();
					DoOK();
					return true;
				}
			return false;
		}
		// unrecognized hotkey
		return false;
	}

	void ContextMenu::UpdateElementPositions()
	{
		// first item at zero offset
		Element *pCurr = GetFirst();
		if (!pCurr) return;
		pCurr->GetBounds().y = 0;
		int32_t iMinWdt = std::max<int32_t>(20, pCurr->GetBounds().Wdt);;
		int32_t iOverallHgt = pCurr->GetBounds().Hgt;
		// others stacked under it
		while ((pCurr = pCurr->GetNext()))
		{
			iMinWdt = std::max(iMinWdt, pCurr->GetBounds().Wdt);
			int32_t iYSpace = pCurr->GetListItemTopSpacing();
			int32_t iNewY = iOverallHgt + iYSpace;
			iOverallHgt += pCurr->GetBounds().Hgt + iYSpace;
			if (iNewY != pCurr->GetBounds().y)
			{
				pCurr->GetBounds().y = iNewY;
				pCurr->UpdateOwnPos();
			}
		}
		// don't make smaller
		iMinWdt = std::max(iMinWdt, rcBounds.Wdt - GetMarginLeft() - GetMarginRight());
		// all entries same size
		for (pCurr = GetFirst(); pCurr; pCurr = pCurr->GetNext())
			if (pCurr->GetBounds().Wdt != iMinWdt)
			{
				pCurr->GetBounds().Wdt = iMinWdt;
				pCurr->UpdateOwnPos();
			}
		// update own size
		rcBounds.Wdt = iMinWdt + GetMarginLeft() + GetMarginRight();
		rcBounds.Hgt = std::max<int32_t>(iOverallHgt, 8) + GetMarginTop() + GetMarginBottom();
		UpdateSize();
	}

	void ContextMenu::RemoveElement(Element *pChild)
	{
		// inherited
		Window::RemoveElement(pChild);
		// target lost?
		if (pChild == pTarget) { Abort(false); return; }
		// submenu?
		if (pChild == pSubmenu) pSubmenu = nullptr;
		// clear selection var
		if (pChild == pSelectedItem)
		{
			pSelectedItem = nullptr;
			SelectionChanged(false);
		}
		// forward to any submenu
		if (pSubmenu) pSubmenu->RemoveElement(pChild);
		// forward to mouse
		if (GetScreen())
			GetScreen()->Mouse.RemoveElement(pChild);
		// update positions
		UpdateElementPositions();
	}

	bool ContextMenu::AddElement(Element *pChild)
	{
		// add it
		Window::AddElement(pChild);
		// update own size and positions
		UpdateElementPositions();
		// success
		return true;
	}

	bool ContextMenu::InsertElement(Element *pChild, Element *pInsertBefore)
	{
		// insert it
		Window::InsertElement(pChild, pInsertBefore);
		// update own size and positions
		UpdateElementPositions();
		// success
		return true;
	}

	void ContextMenu::ElementSizeChanged(Element *pOfElement)
	{
		// inherited
		Window::ElementSizeChanged(pOfElement);
		// update positions of all list items
		UpdateElementPositions();
	}

	void ContextMenu::ElementPosChanged(Element *pOfElement)
	{
		// inherited
		Window::ElementSizeChanged(pOfElement);
		// update positions of all list items
		UpdateElementPositions();
	}

	void ContextMenu::SelectionChanged(bool fByUser)
	{
		// any selection?
		if (pSelectedItem)
		{
			// effect
			if (fByUser) GUISound("UI::Select");
		}
		// close any submenu from prev selection
		if (pSubmenu) pSubmenu->Abort(true);
	}

	Screen *ContextMenu::GetScreen()
	{
		// context menus don't have a parent; get screen by static var
		return Screen::GetScreenS();
	}

	bool ContextMenu::CtxMouseInput(CMouse &rMouse, int32_t iButton, int32_t iScreenX, int32_t iScreenY, DWORD dwKeyParam)
	{
		// check submenu
		if (pSubmenu)
			if (pSubmenu->CtxMouseInput(rMouse, iButton, iScreenX, iScreenY, dwKeyParam)) return true;
		// check bounds
		if (!rcBounds.Contains(iScreenX, iScreenY)) return false;
		// inside menu: do input in local coordinates
		MouseInput(rMouse, iButton, iScreenX - rcBounds.x, iScreenY - rcBounds.y, dwKeyParam);
		return true;
	}

	bool ContextMenu::CharIn(const char * c)
	{
		// forward to submenu
		if (pSubmenu) return pSubmenu->CharIn(c);
		return false;
	}

	void ContextMenu::Draw(C4TargetFacet &cgo)
	{
		// In editor mode, the surface is not assigned
		// The menu is drawn directly by the dialogue, so just exit here.
		if (!cgo.Surface) return;
		// draw self
		Window::Draw(cgo);
		// draw submenus on top
		if (pSubmenu) pSubmenu->Draw(cgo);
	}

	void ContextMenu::Open(Element *pTarget, int32_t iScreenX, int32_t iScreenY)
	{
		// set pos
		rcBounds.x = iScreenX; rcBounds.y = iScreenY;
		UpdatePos();
		// set target
		this->pTarget = pTarget;
		// effect :)
		GUISound("UI::Open");
		// done
	}

	void ContextMenu::CheckOpenSubmenu()
	{
		// safety
		if (!GetScreen()) return;
		// anything selected?
		if (!pSelectedItem) return;
		// get as entry
		Entry *pSelEntry = (Entry *) pSelectedItem;
		// has submenu handler?
		ContextHandler *pSubmenuHandler = pSelEntry->pSubmenuHandler;
		if (!pSubmenuHandler) return;
		// create submenu then
		if (pSubmenu) pSubmenu->Abort(false);
		pSubmenu = pSubmenuHandler->OnSubcontext(pTarget);
		// get open pos
		int32_t iX = GetClientRect().x + pSelEntry->GetBounds().x + pSelEntry->GetBounds().Wdt;
		int32_t iY = GetClientRect().y + pSelEntry->GetBounds().y + pSelEntry->GetBounds().Hgt/2;
		int32_t iScreenWdt = GetScreen()->GetBounds().Wdt, iScreenHgt = GetScreen()->GetBounds().Hgt;
		if (iY + pSubmenu->GetBounds().Hgt >= iScreenHgt)
		{
			// bottom too narrow: open to top, if height is sufficient
			// otherwise, open to top from bottom screen pos
			if (iY < pSubmenu->GetBounds().Hgt) iY = iScreenHgt;
			iY -= pSubmenu->GetBounds().Hgt;
		}
		if (iX + pSubmenu->GetBounds().Wdt >= iScreenWdt)
		{
			// right too narrow: try opening left of this menu
			// otherwise, open to left from right screen border
			if (GetClientRect().x < pSubmenu->GetBounds().Wdt)
				iX = iScreenWdt;
			else
				iX = GetClientRect().x;
			iX -= pSubmenu->GetBounds().Wdt;
		}
		// open it
		pSubmenu->Open(pTarget, iX, iY);
	}

	bool ContextMenu::IsSubmenu()
	{
		return GetScreen() && GetScreen()->pContext!=this;
	}

	void ContextMenu::DoOK()
	{
		// safety
		if (!GetScreen()) return;
		// anything selected?
		if (!pSelectedItem) return;
		// get as entry
		Entry *pSelEntry = (Entry *) pSelectedItem;
		// get CB; take over pointer
		MenuHandler *pCallback = pSelEntry->GetAndZeroCallback();
		Element *pTarget = this->pTarget;
		if (!pCallback) return;
		// close all menus (deletes this class!) w/o sound
		GetScreen()->AbortContext(false);
		// sound
		GUISound("UI::Click");
		// do CB
		pCallback->OnOK(pTarget);
		// free CB class
		delete pCallback;
	}

	void ContextMenu::SelectItem(int32_t iIndex)
	{
		// get item to be selected (may be nullptr on purpose!)
		Element *pNewSelElement = GetElementByIndex(iIndex);
		if (pNewSelElement != pSelectedItem) return;
		// set new
		pSelectedItem = pNewSelElement;
		SelectionChanged(false);
	}


// ----------------------------------------------------
// ContextButton

	ContextButton::ContextButton(C4Rect &rtBounds) : Control(rtBounds), iOpenMenu(0), fMouseOver(false)
	{
		RegisterContextKey();
	}

	ContextButton::ContextButton(Element *pForEl, bool fAdd, int32_t iHIndent, int32_t iVIndent)
			: Control(C4Rect(0,0,0,0)), iOpenMenu(0), fMouseOver(false)
	{
		SetBounds(pForEl->GetToprightCornerRect(16, 16, iHIndent, iVIndent));
		// copy context handler
		SetContextHandler(pForEl->GetContextHandler());
		// add if desired
		Container *pCont;
		if (fAdd) if ((pCont = pForEl->GetContainer()))
				pCont->AddElement(this);
		RegisterContextKey();
	}

	ContextButton::~ContextButton()
	{
		delete pKeyContext;
	}

	void ContextButton::RegisterContextKey()
	{
		// reg keys for pressing the context button
		C4CustomKey::CodeList ContextKeys;
		ContextKeys.push_back(C4KeyCodeEx(K_RIGHT));
		ContextKeys.push_back(C4KeyCodeEx(K_DOWN));
		ContextKeys.push_back(C4KeyCodeEx(K_SPACE));
		ContextKeys.push_back(C4KeyCodeEx(K_RIGHT, KEYS_Alt));
		ContextKeys.push_back(C4KeyCodeEx(K_DOWN, KEYS_Alt));
		ContextKeys.push_back(C4KeyCodeEx(K_SPACE, KEYS_Alt));
		pKeyContext = new C4KeyBinding(ContextKeys, "GUIContextButtonPress", KEYSCOPE_Gui,
		                               new ControlKeyCB<ContextButton>(*this, &ContextButton::KeyContext), C4CustomKey::PRIO_Ctrl);
	}

	bool ContextButton::DoContext(int32_t iX, int32_t iY)
	{
		// get context pos
		if (iX<0)
		{
			iX = rcBounds.Wdt/2;
			iY = rcBounds.Hgt/2;
		}
		// do context
		ContextHandler *pCtx = GetContextHandler();
		if (!pCtx) return false;
		if (!pCtx->OnContext(this, iX, iY)) return false;
		// store menu
		Screen *pScr = GetScreen();
		if (!pScr) return false;
		iOpenMenu = pScr->GetContextMenuIndex();
		// return whether all was successful
		return !!iOpenMenu;
	}

	void ContextButton::DrawElement(C4TargetFacet &cgo)
	{
		// recheck open menu
		Screen *pScr = GetScreen();
		if (!pScr || (iOpenMenu != pScr->GetContextMenuIndex())) iOpenMenu = 0;
		// calc drawing bounds
		int32_t x0 = cgo.TargetX + rcBounds.x, y0 = cgo.TargetY + rcBounds.y;
		// draw button; down (phase 1) if a menu is open
		::GraphicsResource.fctContext.Draw(cgo.Surface, x0, y0, iOpenMenu ? 1 : 0);
		// draw selection highlight
		if (HasDrawFocus() || (fMouseOver && IsInActiveDlg(false)) || iOpenMenu)
		{
			pDraw->SetBlitMode(C4GFXBLIT_ADDITIVE);
			::GraphicsResource.fctButtonHighlight.DrawX(cgo.Surface, x0, y0, rcBounds.Wdt, rcBounds.Hgt);
			pDraw->ResetBlitMode();
		}
	}

	void ContextButton::MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam)
	{
		// left-click activates menu
		if ((iButton == C4MC_Button_LeftDown) || (iButton == C4MC_Button_RightDown))
			if (DoContext()) return;
		// inherited
		Control::MouseInput(rMouse, iButton, iX, iY, dwKeyParam);
	}

	void ContextButton::MouseEnter(CMouse &rMouse)
	{
		Control::MouseEnter(rMouse);
		// remember mouse state for button highlight
		fMouseOver = true;
	}

	void ContextButton::MouseLeave(CMouse &rMouse)
	{
		Control::MouseLeave(rMouse);
		// mouse left
		fMouseOver = false;
	}



} // end of namespace

