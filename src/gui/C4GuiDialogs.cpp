/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
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
// dialog base classes and some user dialogs

#include "C4Include.h"
#include "C4ForbidLibraryCompilation.h"
#include "gui/C4Gui.h"

#include "game/C4Application.h"
#include "game/C4GameScript.h"
#include "game/C4Viewport.h"
#include "graphics/C4Draw.h"
#include "graphics/C4GraphicsResource.h"
#include "object/C4DefList.h"
#include "object/C4Def.h"

#include "graphics/C4DrawGL.h"
#include "platform/StdRegistry.h"

namespace C4GUI
{

// --------------------------------------------------
// FrameDecoration

	void FrameDecoration::Clear()
	{
		pSourceDef = nullptr;
		idSourceDef = C4ID::None;
		dwBackClr = C4GUI_StandardBGColor;
		iBorderTop=iBorderLeft=iBorderRight=iBorderBottom=0;
		fHasGfxOutsideClientArea = false;
		fctTop.Default();
		fctTopRight.Default();
		fctRight.Default();
		fctBottomRight.Default();
		fctBottom.Default();
		fctBottomLeft.Default();
		fctLeft.Default();
		fctTopLeft.Default();
	}

	bool FrameDecoration::SetFacetByAction(C4Def *pOfDef, C4TargetFacet &rfctTarget, const char *szFacetName)
	{
		// get action
		StdStrBuf sActName;
		sActName.Format("FrameDeco%s", szFacetName);
		C4PropList *act = pOfDef->GetActionByName(sActName.getData());
		if (!act) return false;
		// set facet by it
		int32_t x = act->GetPropertyInt(P_X);
		int32_t y = act->GetPropertyInt(P_Y);
		int32_t wdt = act->GetPropertyInt(P_Wdt);
		int32_t hgt = act->GetPropertyInt(P_Hgt);
		int32_t tx = act->GetPropertyInt(P_OffX);
		int32_t ty = act->GetPropertyInt(P_OffY);
		if (!wdt || !hgt) return false;
		rfctTarget.Set(pOfDef->Graphics.GetBitmap(), x, y, wdt, hgt, tx, ty);
		return true;
	}

	bool FrameDecoration::SetByDef(C4ID idSourceDef)
	{
		return SetByDef(C4Id2Def(idSourceDef));
	}

	bool FrameDecoration::SetByDef(C4Def *pSrcDef)
	{
		if (!pSrcDef) return false;
		// script compiled?
		if (!pSrcDef->Script.IsReady()) return false;
		// reset old
		Clear();
		this->pSourceDef = pSrcDef;
		this->idSourceDef = pSrcDef->id;
		// query values
		dwBackClr     = pSrcDef->Call(FormatString(PSF_FrameDecoration, "BackClr"     ).getData()).getInt();
		iBorderTop    = pSrcDef->Call(FormatString(PSF_FrameDecoration, "BorderTop"   ).getData()).getInt();
		iBorderLeft   = pSrcDef->Call(FormatString(PSF_FrameDecoration, "BorderLeft"  ).getData()).getInt();
		iBorderRight  = pSrcDef->Call(FormatString(PSF_FrameDecoration, "BorderRight" ).getData()).getInt();
		iBorderBottom = pSrcDef->Call(FormatString(PSF_FrameDecoration, "BorderBottom").getData()).getInt();
		// get gfx
		SetFacetByAction(pSrcDef, fctTop        , "Top"        );
		SetFacetByAction(pSrcDef, fctTopRight   , "TopRight"   );
		SetFacetByAction(pSrcDef, fctRight      , "Right"      );
		SetFacetByAction(pSrcDef, fctBottomRight, "BottomRight");
		SetFacetByAction(pSrcDef, fctBottom     , "Bottom"     );
		SetFacetByAction(pSrcDef, fctBottomLeft , "BottomLeft" );
		SetFacetByAction(pSrcDef, fctLeft       , "Left"       );
		SetFacetByAction(pSrcDef, fctTopLeft    , "TopLeft"    );
		// check for gfx outside main area
		fHasGfxOutsideClientArea = (fctTopLeft.TargetY < 0) || (fctTop.TargetY < 0) || (fctTopRight.TargetY < 0)
		                           || (fctTopLeft.TargetX < 0) || (fctLeft.TargetX < 0) || (fctBottomLeft.TargetX < 0)
		                           || (fctTopRight.TargetX + fctTopRight.Wdt > iBorderRight) || (fctRight.TargetX + fctRight.Wdt > iBorderRight) || (fctBottomRight.TargetX + fctBottomRight.Wdt > iBorderRight)
		                           || (fctBottomLeft.TargetY + fctBottomLeft.Hgt > iBorderBottom) || (fctBottom.TargetY + fctBottom.Hgt > iBorderBottom) || (fctBottomRight.TargetY + fctBottomRight.Hgt > iBorderBottom);
		// k, done
		return true;
	}

	bool FrameDecoration::UpdateGfx()
	{
		// simply re-set by def
		return SetByDef(idSourceDef);
	}

	void FrameDecoration::Draw(C4TargetFacet &cgo, C4Rect &rcBounds)
	{
		// draw BG
		int ox = cgo.TargetX+rcBounds.x, oy = cgo.TargetY+rcBounds.y;
		pDraw->DrawBoxDw(cgo.Surface, ox,oy,ox+rcBounds.Wdt-1,oy+rcBounds.Hgt-1,dwBackClr);
		// draw borders
		int x,y,Q;
		// top
		if ((Q=fctTop.Wdt))
		{
			for (x = iBorderLeft; x < rcBounds.Wdt-iBorderRight; x += fctTop.Wdt)
			{
				int w = std::min<int>(fctTop.Wdt, rcBounds.Wdt-iBorderRight-x);
				fctTop.Wdt = w;
				fctTop.Draw(cgo.Surface, ox+x, oy+fctTop.TargetY);
			}
			fctTop.Wdt = Q;
		}
		// left
		if ((Q=fctLeft.Hgt))
		{
			for (y = iBorderTop; y < rcBounds.Hgt-iBorderBottom; y += fctLeft.Hgt)
			{
				int h = std::min<int>(fctLeft.Hgt, rcBounds.Hgt-iBorderBottom-y);
				fctLeft.Hgt = h;
				fctLeft.Draw(cgo.Surface, ox+fctLeft.TargetX, oy+y);
			}
			fctLeft.Hgt = Q;
		}
		// right
		if ((Q=fctRight.Hgt))
		{
			for (y = iBorderTop; y < rcBounds.Hgt-iBorderBottom; y += fctRight.Hgt)
			{
				int h = std::min<int>(fctRight.Hgt, rcBounds.Hgt-iBorderBottom-y);
				fctRight.Hgt = h;
				fctRight.Draw(cgo.Surface, ox+rcBounds.Wdt-iBorderRight+fctRight.TargetX, oy+y);
			}
			fctRight.Hgt = Q;
		}
		// bottom
		if ((Q=fctBottom.Wdt))
		{
			for (x = iBorderLeft; x < rcBounds.Wdt-iBorderRight; x += fctBottom.Wdt)
			{
				int w = std::min<int>(fctBottom.Wdt, rcBounds.Wdt-iBorderRight-x);
				fctBottom.Wdt = w;
				fctBottom.Draw(cgo.Surface, ox+x, oy+rcBounds.Hgt-iBorderBottom+fctBottom.TargetY);
			}
			fctBottom.Wdt = Q;
		}
		// draw edges
		fctTopLeft.Draw(cgo.Surface, ox+fctTopLeft.TargetX,oy+fctTopLeft.TargetY);
		fctTopRight.Draw(cgo.Surface, ox+rcBounds.Wdt-iBorderRight+fctTopRight.TargetX,oy+fctTopRight.TargetY);
		fctBottomLeft.Draw(cgo.Surface, ox+fctBottomLeft.TargetX,oy+rcBounds.Hgt-iBorderBottom+fctBottomLeft.TargetY);
		fctBottomRight.Draw(cgo.Surface, ox+rcBounds.Wdt-iBorderRight+fctBottomRight.TargetX,oy+rcBounds.Hgt-iBorderBottom+fctBottomRight.TargetY);
	}

// --------------------------------------------------
// DialogWindow

#ifdef USE_WIN32_WINDOWS

	C4Window * DialogWindow::Init(C4AbstractApp * pApp, const char * Title, const C4Rect &rcBounds, const char *szID)
	{
		C4Window * result = C4Window::Init(C4Window::W_GuiWindow, pApp, Title, &rcBounds);
		if (result)
		{
			// update pos
			if (szID && *szID)
				RestoreWindowPosition(hWindow, FormatString("ConsoleGUI_%s", szID).getData(), Config.GetSubkeyPath("Console"), false);
			// and show
			::ShowWindow(hWindow, SW_SHOW);
		}
		return result;
	}

#else
	C4Window * DialogWindow::Init(C4AbstractApp * pApp, const char * Title, const C4Rect &rcBounds, const char *szID)
	{
		C4Window * result = C4Window::Init(C4Window::W_GuiWindow, pApp, Title, &rcBounds);
		if (result)
		{
			// update pos
			if (szID && *szID)
				RestorePosition(FormatString("ConsoleGUI_%s", szID).getData(), Config.GetSubkeyPath("Console"), false);
			else
				SetSize(rcBounds.Wdt, rcBounds.Hgt);
		}
		return result;
	}
#endif // _WIN32

	void DialogWindow::PerformUpdate()
	{
		if (!pDialog)
			return; // safety
		C4Rect r;
		GetSize(&r);
		if (pSurface)
		{
			pSurface->Wdt = r.Wdt;
			pSurface->Hgt = r.Hgt;
#ifndef USE_CONSOLE
			pGL->PrepareRendering(pSurface);
			glClear(GL_COLOR_BUFFER_BIT);
#endif
		}
		C4TargetFacet cgo;
		cgo.Set(nullptr, 0, 0, r.Wdt, r.Hgt, 0, 0);
		pDialog->Draw(cgo);
	}

	void DialogWindow::Close()
	{
		// FIXME: Close the dialog of this window
	}

	bool Dialog::CreateConsoleWindow()
	{
#ifdef WITH_QT_EDITOR
		// TODO: Implement these as Qt editor windows.
		// This currently creates an empty window in Windows and a segfault in Linux.
		return false;
#endif
		// already created?
		if (pWindow) return true;
		// create it!
		pWindow = new DialogWindow();
		if (!pWindow->Init(&Application, TitleString.getData(), rcBounds, GetID()))
		{
			delete pWindow;
			pWindow = nullptr;
			return false;
		}
		// create rendering context
		pWindow->pSurface = new C4Surface(&Application, pWindow);
		pWindow->pDialog = this;
		return true;
	}

	void Dialog::DestroyConsoleWindow()
	{
		if (pWindow)
		{
			delete pWindow->pSurface;
			pWindow->Clear();
			delete pWindow;
			pWindow = nullptr;
		}
	}

	Dialog::Dialog(int32_t iWdt, int32_t iHgt, const char *szTitle, bool fViewportDlg):
			Window(), pTitle(nullptr), pCloseBtn(nullptr), fDelOnClose(false), fViewportDlg(fViewportDlg), pWindow(nullptr), pFrameDeco(nullptr)
	{
		// zero fields
		pActiveCtrl = nullptr;
		fShow = fOK = false;
		iFade = 100; eFade = eFadeNone;
		// add title
		rcBounds.Wdt = iWdt;
		SetTitle(szTitle);
		// set size - calcs client rect as well
		SetBounds(C4Rect(0,0,iWdt,iHgt));
		// create key callbacks
		C4CustomKey::CodeList Keys;
		Keys.emplace_back(K_TAB);
		if (Config.Controls.GamepadGuiControl)
		{
			ControllerKeys::Right(Keys);
		}
		pKeyAdvanceControl = new C4KeyBinding(Keys, "GUIAdvanceFocus", KEYSCOPE_Gui,
		                                      new DlgKeyCBEx<Dialog, bool>(*this, false, &Dialog::KeyAdvanceFocus), C4CustomKey::PRIO_Dlg);
		Keys.clear();
		Keys.emplace_back(K_TAB, KEYS_Shift);
		if (Config.Controls.GamepadGuiControl)
		{
			ControllerKeys::Left(Keys);
		}
		pKeyAdvanceControlB = new C4KeyBinding(Keys, "GUIAdvanceFocusBack", KEYSCOPE_Gui,
		                                       new DlgKeyCBEx<Dialog, bool>(*this, true, &Dialog::KeyAdvanceFocus), C4CustomKey::PRIO_Dlg);
		Keys.clear();
		Keys.emplace_back(KEY_Any, KEYS_Alt);
		Keys.emplace_back(KEY_Any, C4KeyShiftState(KEYS_Alt | KEYS_Shift));
		pKeyHotkey = new C4KeyBinding(Keys, "GUIHotkey", KEYSCOPE_Gui,
		                              new DlgKeyCBPassKey<Dialog>(*this, &Dialog::KeyHotkey), C4CustomKey::PRIO_Ctrl);
		Keys.clear();
		Keys.emplace_back(K_RETURN);
		if (Config.Controls.GamepadGuiControl)
		{
			ControllerKeys::Ok(Keys);
		}
		pKeyEnter = new C4KeyBinding(Keys, "GUIDialogOkay", KEYSCOPE_Gui,
		                             new DlgKeyCB<Dialog>(*this, &Dialog::KeyEnter), C4CustomKey::PRIO_Dlg);
		Keys.clear();
		Keys.emplace_back(K_ESCAPE);
		if (Config.Controls.GamepadGuiControl)
		{
			ControllerKeys::Cancel(Keys);
		}
		pKeyEscape = new C4KeyBinding(Keys, "GUIDialogAbort", KEYSCOPE_Gui,
		                              new DlgKeyCB<Dialog>(*this, &Dialog::KeyEscape), C4CustomKey::PRIO_Dlg);
		Keys.clear();
		Keys.emplace_back(KEY_Any);
		Keys.emplace_back(KEY_Any, KEYS_Shift);
		pKeyFocusDefControl = new C4KeyBinding(Keys, "GUIFocusDefault", KEYSCOPE_Gui,
		                                       new DlgKeyCB<Dialog>(*this, &Dialog::KeyFocusDefault), C4CustomKey::PRIO_Dlg);
	}

	int32_t Dialog::GetDefaultTitleHeight()
	{
		// default title font
		return std::min<int32_t>(::GraphicsResource.TextFont.GetLineHeight(), C4GUI_MinWoodBarHgt);
	}

	void Dialog::SetTitle(const char *szTitle, bool fShowCloseButton)
	{
		// always keep local copy of title
		TitleString.Copy(szTitle);
		// console mode dialogs: Use window bar
		if (Application.isEditor && !IsViewportDialog())
		{
			if (pWindow) pWindow->SetTitle(szTitle ? szTitle : "");
			return;
		}
		// set new
		if (szTitle && *szTitle)
		{
			int32_t iTextHgt = WoodenLabel::GetDefaultHeight(&::GraphicsResource.TextFont);
			if (pTitle)
			{
				pTitle->GetBounds() = C4Rect(-GetMarginLeft(), -iTextHgt, rcBounds.Wdt, iTextHgt);
				// noupdate if title is same - this is necessary to prevent scrolling reset when refilling internal menus
				if (SEqual(pTitle->GetText(), szTitle)) return;
				pTitle->SetText(szTitle);
			}
			else
				AddElement(pTitle = new WoodenLabel(szTitle, C4Rect(-GetMarginLeft(), -iTextHgt, rcBounds.Wdt, iTextHgt), C4GUI_CaptionFontClr, &::GraphicsResource.TextFont, ALeft, false));
			pTitle->SetToolTip(szTitle);
			pTitle->SetDragTarget(this);
			pTitle->SetAutoScrollTime(C4GUI_TitleAutoScrollTime);
			if (fShowCloseButton)
			{
				pTitle->SetRightIndent(20); // for close button
				if (!pCloseBtn)
				{
					AddElement(pCloseBtn = new CallbackButton<Dialog, IconButton>(Ico_Close, pTitle->GetToprightCornerRect(16,16,4,4,0), '\0', &Dialog::OnUserClose));
					pCloseBtn->SetToolTip(LoadResStr("IDS_MNU_CLOSE"));
				}
				else
					pCloseBtn->GetBounds() = pTitle->GetToprightCornerRect(16,16,4,4,0);
			}
		}
		else
		{
			if (pTitle) { delete pTitle; pTitle=nullptr; }
			if (pCloseBtn) { delete pCloseBtn; pCloseBtn = nullptr; }
		}
	}

	Dialog::~Dialog()
	{
		// kill key bindings
		delete pKeyAdvanceControl;
		delete pKeyAdvanceControlB;
		delete pKeyHotkey;
		delete pKeyEscape;
		delete pKeyEnter;
		delete pKeyFocusDefControl;
		// clear window
		DestroyConsoleWindow();
		// avoid endless delete/close-recursion
		fDelOnClose = false;
		// free deco
		if (pFrameDeco) pFrameDeco->Deref();
	}

	void Dialog::UpdateSize()
	{
		// update title bar position
		if (pTitle)
		{
			int32_t iTextHgt = WoodenLabel::GetDefaultHeight(&::GraphicsResource.TextFont);
			pTitle->SetBounds(C4Rect(-GetMarginLeft(), -iTextHgt, rcBounds.Wdt, iTextHgt));
			if (pCloseBtn) pCloseBtn->SetBounds(pTitle->GetToprightCornerRect(16,16,4,4,0));
		}
		// inherited
		Window::UpdateSize();
		// update assigned window
		if (pWindow)
		{
			pWindow->SetSize(rcBounds.Wdt,rcBounds.Hgt);
		}
	}

	void Dialog::UpdatePos()
	{
		// Dialogs with their own windows can only be at 0/0
		if (pWindow)
		{
			rcBounds.x = 0;
			rcBounds.y = 0;
		}
		Window::UpdatePos();
	}

	void Dialog::RemoveElement(Element *pChild)
	{
		// inherited
		Window::RemoveElement(pChild);
		// clear ptr
		if (pChild == pActiveCtrl) pActiveCtrl = nullptr;
	}

	void Dialog::Draw(C4TargetFacet &cgo0)
	{
		C4TargetFacet cgo; cgo.Set(cgo0);
		// Dialogs with a window just ignore the cgo.
		if (pWindow)
		{
			cgo.Surface = pWindow->pSurface;
			cgo.X = 0; cgo.Y = 0; cgo.Wdt = rcBounds.Wdt; cgo.Hgt = rcBounds.Hgt;
		}
		Screen *pScreen;
		// evaluate fading
		switch (eFade)
		{
		case eFadeNone: break; // no fading
		case eFadeIn:
			// fade in
			if ((iFade+=10) >= 100)
			{
				if ((pScreen = GetScreen()))
				{
					if (pScreen->GetTopDialog() == this)
						pScreen->ActivateDialog(this);
				}
				eFade = eFadeNone;
			}
			break;
		case eFadeOut:
			// fade out
			if ((iFade-=10) <= 0)
			{
				fVisible = fShow = false;
				if ((pScreen = GetScreen()))
					pScreen->RecheckActiveDialog();
				eFade = eFadeNone;
			}
		}
		// set fade
		if (iFade < 100)
		{
			if (iFade <= 0) return;
			pDraw->ActivateBlitModulation((iFade*255/100)<<24 | 0xffffff);
		}
		// separate window: Clear background
		if (pWindow)
			pDraw->DrawBoxDw(cgo.Surface, rcBounds.x, rcBounds.y, rcBounds.Wdt-1, rcBounds.Hgt-1, (0xff << 24) | (C4GUI_StandardBGColor & 0xffffff) );
		// draw window + contents (evaluates IsVisible)
		Window::Draw(cgo);
		// reset blit modulation
		if (iFade<100) pDraw->DeactivateBlitModulation();
		// blit output to own window
		if (pWindow)
		{
			// Draw context menu on editor window
			ContextMenu *menu;
			if ((menu = GetScreen()->pContext))
			{
				if (menu->GetTargetDialog() == this)
				{
					menu->Draw(cgo);
				}
			}
			// Editor window: Blit to output
			C4Rect rtSrc,rtDst;
			rtSrc.x=rcBounds.x; rtSrc.y=rcBounds.y;  rtSrc.Wdt=rcBounds.Wdt; rtSrc.Hgt=rcBounds.Hgt;
			rtDst.x=0; rtDst.y=0;    rtDst.Wdt=rcBounds.Wdt; rtDst.Hgt=rcBounds.Hgt;
			pWindow->pSurface->PageFlip(&rtSrc, &rtDst);
		}
	}

	void Dialog::DrawElement(C4TargetFacet &cgo)
	{
		// custom border?
		if (pFrameDeco)
			pFrameDeco->Draw(cgo, rcBounds);
		else
		{
			// standard border/bg then
			// draw background
			pDraw->DrawBoxDw(cgo.Surface, cgo.TargetX+rcBounds.x,cgo.TargetY+rcBounds.y,rcBounds.x+rcBounds.Wdt-1+cgo.TargetX,rcBounds.y+rcBounds.Hgt-1+cgo.TargetY,C4GUI_StandardBGColor);
			// draw frame
			Draw3DFrame(cgo);
		}
	}

	bool Dialog::CharIn(const char * c)
	{
		// reroute to active control
		if (pActiveCtrl && pActiveCtrl->CharIn(c)) return true;
		// unprocessed: Focus default control
		// Except for space, which may have been processed as a key already
		// (changing focus here would render buttons unusable, because they switch on KeyUp)
		Control *pDefCtrl = GetDefaultControl();
		if (pDefCtrl && pDefCtrl != pActiveCtrl && (!c || *c != 0x20))
		{
			SetFocus(pDefCtrl, false);
			if (pActiveCtrl && pActiveCtrl->CharIn(c))
				return true;
		}
		return false;
	}

	bool Dialog::KeyHotkey(const C4KeyCodeEx &key)
	{
		StdStrBuf sKey = C4KeyCodeEx::KeyCode2String(key.Key, true, true);
		// do hotkey procs for standard alphanumerics only
		if (sKey.getLength() != 1) return false;
		WORD wKey = WORD(*sKey.getData());
		if (Inside<WORD>(TOUPPERIFX11(wKey), 'A', 'Z')) if (OnHotkey(char(TOUPPERIFX11(wKey)))) return true;
		if (Inside<WORD>(TOUPPERIFX11(wKey), '0', '9')) if (OnHotkey(char(TOUPPERIFX11(wKey)))) return true;
		return false;
	}

	bool Dialog::KeyFocusDefault()
	{
		// unprocessed key: Focus default control
		Control *pDefCtrl = GetDefaultControl();
		if (pDefCtrl && pDefCtrl != pActiveCtrl)
			SetFocus(pDefCtrl, false);
		// never mark this as processed, so a later char message to the control may be sent (for deselected chat)
		return false;
	}

	void Dialog::MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam)
	{
		// inherited will do...
		Window::MouseInput(rMouse, iButton, iX, iY, dwKeyParam);
	}

	void Dialog::SetFocus(Control *pCtrl, bool fByMouse)
	{
		// no change?
		if (pCtrl == pActiveCtrl) return;
		// leave old focus
		if (pActiveCtrl)
		{
			Control *pC = pActiveCtrl;
			pActiveCtrl = nullptr;
			pC->OnLooseFocus();
			// if leaving the old focus set a new one, abort here because it looks like the control didn't want to lose focus
			if (pActiveCtrl) return;
		}
		// set new
		if ((pActiveCtrl = pCtrl)) pCtrl->OnGetFocus(fByMouse);
	}

	void Dialog::AdvanceFocus(bool fBackwards)
	{
		// get element to start from
		Element *pCurrElement = pActiveCtrl;
		// find new control
		for (;;)
		{
			// get next element
			pCurrElement = GetNextNestedElement(pCurrElement, fBackwards);
			// end reached: start from beginning
			if (!pCurrElement && pActiveCtrl) if (!(pCurrElement = GetNextNestedElement(nullptr, fBackwards))) return;
			// cycled?
			if (pCurrElement == pActiveCtrl)
			{
				// but current is no longer a focus element? Then defocus it and return
				if (pCurrElement && !pCurrElement->IsFocusElement())
					SetFocus(nullptr, false);
				return;
			}
			// for list elements, check whether the child can be selected
			if (pCurrElement->GetParent() && !pCurrElement->GetParent()->IsSelectedChild(pCurrElement)) continue;
			// check if this is a new control
			Control *pFocusCtrl = pCurrElement->IsFocusElement();
			if (pFocusCtrl && pFocusCtrl != pActiveCtrl && pFocusCtrl->IsVisible())
			{
				// set focus here...
				SetFocus(pFocusCtrl, false);
				// ...done!
				return;
			}
		}
		// never reached
	}

	bool Dialog::Show(Screen *pOnScreen, bool fCB)
	{
		// already shown?
		if (fShow) return false;
		// default screen
		if (!pOnScreen) if (!(pOnScreen = Screen::GetScreenS())) return false;
		// show there
		pOnScreen->ShowDialog(this, false);
		fVisible = true;
		// developer mode: Create window
		if (Application.isEditor && !IsViewportDialog())
			if (!CreateConsoleWindow()) return false;
		// CB
		if (fCB) OnShown();
		return true;
	}

	void Dialog::Close(bool fOK)
	{
		// already closed?
		if (!fShow) return;
		// set OK flag
		this->fOK = fOK;
		// get screen
		Screen *pScreen = GetScreen();
		if (pScreen) pScreen->CloseDialog(this, false); else fShow = false;
		// developer mode: Remove window
		if (pWindow) DestroyConsoleWindow();
		// do callback - last call, because it might do perilous things
		OnClosed(fOK);
	}

	void Dialog::OnClosed(bool fOK)
	{
		// developer mode: Remove window
		if (pWindow) DestroyConsoleWindow();
		// delete when closing?
		if (fDelOnClose)
		{
			fDelOnClose=false;
			delete this;
		}
	}

	bool Dialog::DoModal()
	{
		// Cancel all dialogues if game is left (including e.g. league dialogues)
		if (::Application.IsQuittingGame()) return false;
		// main message loop
		while (fShow)
		{
			// dialog idle proc
			OnIdle();
			// Modal dialogue during running game is tricky. Do not execute game!
			bool fGameWasRunning = ::Game.IsRunning;
			::Game.IsRunning = false;
			// handle messages - this may block until the next timer
			if (!Application.ScheduleProcs())
				return false; // game GUI and lobby will deleted in Game::Clear()
			// reset game run state
			if (fGameWasRunning) ::Game.IsRunning = true;
		}
		// return whether dlg was OK
		return fOK;
	}

	bool Dialog::Execute()
	{
		// process messages
		if (!Application.ScheduleProcs(0))
			return false;
		// check status
		if (!fShow) return false;
		return true;
	}

	bool Dialog::Execute2()
	{
		// execute
		if (Execute()) return true;
		// delete self if closed
		delete this;
		return false;
	}

	bool Dialog::IsActive(bool fForKeyboard)
	{
		// must be fully visible
		if (!IsShown() || IsFading()) return false;
		// screen-less dialogs are always inactive (not yet added)
		Screen *pScreen = GetScreen();
		if (!pScreen) return false;
		// no keyboard focus if screen is in context mode
		if (fForKeyboard && pScreen->HasContext()) return false;
		// always okay in shared mode: all dlgs accessible by mouse
		if (!pScreen->IsExclusive() && !fForKeyboard) return true;
		// exclusive mode or keyboard input: Only one dlg active
		return pScreen->pActiveDlg == this;
	}

	bool Dialog::FadeIn(Screen *pOnScreen)
	{
		// default screen
		if (!pOnScreen) pOnScreen = Screen::GetScreenS();
		// fade in there
		pOnScreen->ShowDialog(this, true);
		iFade = 0;
		eFade = eFadeIn;
		fVisible = true;
		OnShown();
		// done, success
		return true;
	}

	void Dialog::FadeOut(bool fCloseWithOK)
	{
		// only if shown, or being faded in
		if (!IsShown() && (!fVisible || eFade!=eFadeIn)) return;
		// set OK flag
		this->fOK = fCloseWithOK;
		// fade out
		Screen *pOnScreen = GetScreen();
		if (!pOnScreen) return;
		pOnScreen->CloseDialog(this, true);
		eFade = eFadeOut;
		// do callback - last call, because it might do perilous things
		OnClosed(fCloseWithOK);
	}

	void Dialog::ApplyElementOffset(int32_t &riX, int32_t &riY)
	{
		// inherited
		Window::ApplyElementOffset(riX, riY);
		// apply viewport offset, if a viewport is assigned
		C4Viewport *pVP = GetViewport();
		if (pVP)
		{
			C4Rect rcVP(pVP->GetOutputRect());
			riX -= rcVP.x; riY -= rcVP.y;
		}
	}

	void Dialog::ApplyInvElementOffset(int32_t &riX, int32_t &riY)
	{
		// inherited
		Window::ApplyInvElementOffset(riX, riY);
		// apply viewport offset, if a viewport is assigned
		C4Viewport *pVP = GetViewport();
		if (pVP)
		{
			C4Rect rcVP(pVP->GetOutputRect());
			riX += rcVP.x; riY += rcVP.y;
		}
	}

	void Dialog::SetClientSize(int32_t iToWdt, int32_t iToHgt)
	{
		// calc new bounds
		iToWdt += GetMarginLeft()+GetMarginRight();
		iToHgt += GetMarginTop()+GetMarginBottom();
		rcBounds.x += (rcBounds.Wdt - iToWdt)/2;
		rcBounds.y += (rcBounds.Hgt - iToHgt)/2;
		rcBounds.Wdt = iToWdt; rcBounds.Hgt = iToHgt;
		// reflect changes
		UpdatePos();
	}


// --------------------------------------------------
// FullscreenDialog

	FullscreenDialog::FullscreenDialog(const char *szTitle, const char *szSubtitle)
			: Dialog(Screen::GetScreenS()->GetClientRect().Wdt, Screen::GetScreenS()->GetClientRect().Hgt, nullptr /* create own title */, false), pFullscreenTitle(nullptr)
	{
		// set margins
		int32_t iScreenX = Screen::GetScreenS()->GetClientRect().Wdt;
		int32_t iScreenY = Screen::GetScreenS()->GetClientRect().Hgt;
		if (iScreenX < 500) iDlgMarginX = 2; else iDlgMarginX = iScreenX/50;
		if (iScreenY < 320) iDlgMarginY = 2; else iDlgMarginY = iScreenY*2/75;
		// set size - calcs client rect as well
		SetBounds(C4Rect(0,0,iScreenX,iScreenY));
		// create title
		SetTitle(szTitle);
		// create subtitle (only with upperboard)
		if (szSubtitle && *szSubtitle && HasUpperBoard())
		{
			AddElement(pSubTitle = new Label(szSubtitle, rcClientRect.Wdt, C4UpperBoardHeight-::GraphicsResource.CaptionFont.GetLineHeight()/2-25-GetMarginTop(), ARight, C4GUI_CaptionFontClr, &::GraphicsResource.TextFont));
			pSubTitle->SetToolTip(szTitle);
		}
		else pSubTitle = nullptr;
	}

	void FullscreenDialog::SetTitle(const char *szTitle)
	{
		if (pFullscreenTitle) { delete pFullscreenTitle; pFullscreenTitle=nullptr; }
		// change title text; creates or removes title bar if necessary
		if (szTitle && *szTitle)
		{
			// not using dlg label, which is a wooden label
			if (HasUpperBoard())
				pFullscreenTitle = new Label(szTitle, 0, C4UpperBoardHeight/2 - ::GraphicsResource.TitleFont.GetLineHeight()/2-GetMarginTop(), ALeft, C4GUI_CaptionFontClr, &::GraphicsResource.TitleFont);
			else
				// non-woodbar: Title is centered and in big font
				pFullscreenTitle = new Label(szTitle, GetClientRect().Wdt/2, C4UpperBoardHeight/2 - ::GraphicsResource.TitleFont.GetLineHeight()/2-GetMarginTop(), ACenter, C4GUI_FullscreenCaptionFontClr, &::GraphicsResource.TitleFont);
			AddElement(pFullscreenTitle);
			pFullscreenTitle->SetToolTip(szTitle);
		}
	}

	void FullscreenDialog::DrawElement(C4TargetFacet &cgo)
	{
		// draw upper board
		if (HasUpperBoard())
			pDraw->BlitSurfaceTile(::GraphicsResource.fctUpperBoard.Surface,cgo.Surface,0,std::min<int32_t>(iFade-::GraphicsResource.fctUpperBoard.Hgt, 0),cgo.Wdt,::GraphicsResource.fctUpperBoard.Hgt, 0, 0, nullptr);
	}

	void FullscreenDialog::UpdateOwnPos()
	{
		// inherited to update client rect
		Dialog::UpdateOwnPos();
	}

	void FullscreenDialog::DrawBackground(C4TargetFacet &cgo, C4Facet &rFromFct)
	{
		// draw across fullscreen bounds - zoom 1px border to prevent flashing borders by blit offsets
		rFromFct.DrawFullScreen(cgo);
	}

// --------------------------------------------------
// MessageDialog

	MessageDialog::MessageDialog(const char *szMessage, const char *szCaption, DWORD dwButtons, Icons icoIcon, DlgSize eSize, int32_t *piConfigDontShowAgainSetting, bool fDefaultNo)
			: Dialog(eSize, 100 /* will be resized */, szCaption, false), piConfigDontShowAgainSetting(piConfigDontShowAgainSetting), pKeyCopy(nullptr), sCopyText()
	{
		CStdFont &rUseFont = ::GraphicsResource.TextFont;
		// get positions
		ComponentAligner caMain(GetClientRect(), C4GUI_DefDlgIndent, C4GUI_DefDlgIndent, true);
		// place icon
		C4Rect rcIcon = caMain.GetFromLeft(C4GUI_IconWdt); rcIcon.Hgt = C4GUI_IconHgt;
		Icon *pIcon = new Icon(rcIcon, icoIcon); AddElement(pIcon);
		// centered text for small dialogs and/or dialogs w/o much text (i.e.: no linebreaks)
		bool fTextCentered;
		if (eSize != dsRegular)
			fTextCentered = true;
		else
		{
			int32_t iMsgWdt=0, iMsgHgt=0;
			rUseFont.GetTextExtent(szMessage, iMsgWdt, iMsgHgt);
			fTextCentered = ((iMsgWdt <= caMain.GetInnerWidth() - C4GUI_IconWdt - C4GUI_DefDlgIndent*2) && iMsgHgt<=rUseFont.GetLineHeight());
		}
		// centered text dialog: waste some icon space on the right to balance dialog
		if (fTextCentered) caMain.GetFromRight(C4GUI_IconWdt);
		// place message label
		// use text with line breaks
		StdStrBuf sMsgBroken;
		int iMsgHeight = rUseFont.BreakMessage(szMessage, caMain.GetInnerWidth(), &sMsgBroken, true);
		Label *pLblMessage = new Label("", caMain.GetFromTop(iMsgHeight), fTextCentered ? ACenter : ALeft, C4GUI_MessageFontClr, &rUseFont, false);
		pLblMessage->SetText(sMsgBroken.getData(), false);
		AddElement(pLblMessage);
		// place do-not-show-again-checkbox
		if (piConfigDontShowAgainSetting)
		{
			int w=100,h=20;
			const char *szCheckText = LoadResStr("IDS_MSG_DONTSHOW");
			CheckBox::GetStandardCheckBoxSize(&w, &h, szCheckText, nullptr);
			CheckBox *pCheck = new C4GUI::CheckBox(caMain.GetFromTop(h, w), szCheckText, !!*piConfigDontShowAgainSetting);
			pCheck->SetOnChecked(new C4GUI::CallbackHandler<MessageDialog>(this, &MessageDialog::OnDontShowAgainCheck));
			AddElement(pCheck);
		}
		if (!fTextCentered) caMain.ExpandLeft(C4GUI_DefDlgIndent*2 + C4GUI_IconWdt);
		// place button(s)
		ComponentAligner caButtonArea(caMain.GetFromTop(C4GUI_ButtonAreaHgt), 0,0);
		int32_t iButtonCount = 0;
		int32_t i=1; while (i) { if (dwButtons & i) ++iButtonCount; i=i<<1; }
		fHasOK = !!(dwButtons & btnOK) || !!(dwButtons & btnYes);
		Button *btnFocus = nullptr;
		if (iButtonCount)
		{
			C4Rect rcBtn = caButtonArea.GetCentered(iButtonCount*C4GUI_DefButton2Wdt+(iButtonCount-1)*C4GUI_DefButton2HSpace, C4GUI_ButtonHgt);
			rcBtn.Wdt = C4GUI_DefButton2Wdt;
			// OK
			if (dwButtons & btnOK)
			{
				Button *pBtnOK = new OKButton(rcBtn);
				AddElement(pBtnOK);
				rcBtn.x += C4GUI_DefButton2Wdt+C4GUI_DefButton2HSpace;
				if (!fDefaultNo) btnFocus = pBtnOK;
			}
			// Retry
			if (dwButtons & btnRetry)
			{
				Button *pBtnRetry = new RetryButton(rcBtn);
				AddElement(pBtnRetry);
				rcBtn.x += C4GUI_DefButton2Wdt+C4GUI_DefButton2HSpace;
				if (!btnFocus) btnFocus = pBtnRetry;

			}
			// Cancel
			if (dwButtons & btnAbort)
			{
				Button *pBtnAbort = new CancelButton(rcBtn);
				AddElement(pBtnAbort);
				rcBtn.x += C4GUI_DefButton2Wdt+C4GUI_DefButton2HSpace;
				if (!btnFocus) btnFocus = pBtnAbort;
			}
			// Yes
			if (dwButtons & btnYes)
			{
				Button *pBtnYes = new YesButton(rcBtn);
				AddElement(pBtnYes);
				rcBtn.x += C4GUI_DefButton2Wdt+C4GUI_DefButton2HSpace;
				if (!btnFocus && !fDefaultNo) btnFocus = pBtnYes;
			}
			// No
			if (dwButtons & btnNo)
			{
				Button *pBtnNo = new NoButton(rcBtn);
				AddElement(pBtnNo);
				if (!btnFocus) btnFocus = pBtnNo;
			}
			// Reset
			if (dwButtons & btnReset)
			{
				Button *pBtnReset = new ResetButton(rcBtn);
				AddElement(pBtnReset);
				rcBtn.x += C4GUI_DefButton2Wdt+C4GUI_DefButton2HSpace;
				if (!btnFocus) btnFocus = pBtnReset;

			}
		}
		if (btnFocus) SetFocus(btnFocus, false);
		// resize to actually needed size
		SetClientSize(GetClientRect().Wdt, GetClientRect().Hgt - caMain.GetHeight());
		// Control+C copies text to clipboard
		sCopyText = strprintf("[%s] %s", szCaption ? szCaption : "", szMessage ? szMessage : "");
		pKeyCopy = new C4KeyBinding(C4KeyCodeEx(K_C, KEYS_Control), "GUIEditCopy", KEYSCOPE_Gui,
		               new DlgKeyCB<MessageDialog>(*this, &MessageDialog::KeyCopy), C4CustomKey::PRIO_CtrlOverride);
	}

MessageDialog::~MessageDialog()
{
	delete pKeyCopy;
}

bool MessageDialog::KeyCopy()
{
	// Copy text to clipboard
	::Application.Copy(sCopyText);
	return true;
}


// --------------------------------------------------
// ConfirmationDialog

	ConfirmationDialog::ConfirmationDialog(const char *szMessage, const char *szCaption, BaseCallbackHandler *pCB, DWORD dwButtons, bool fSmall, Icons icoIcon)
			: MessageDialog(szMessage, szCaption, dwButtons, icoIcon, fSmall ? MessageDialog::dsSmall : MessageDialog::dsRegular)
	{
		if ((this->pCB=pCB)) pCB->Ref();
		// always log confirmation messages
		LogSilentF("[Cnf] %s: %s", szCaption, szMessage);
		// confirmations always get deleted on close
		SetDelOnClose();
	}

	void ConfirmationDialog::OnClosed(bool fOK)
	{
		// confirmed only on OK
		BaseCallbackHandler *pStackCB = fOK ? pCB : nullptr;
		if (pStackCB) pStackCB->Ref();
		// caution: this will usually delete the dlg (this)
		// so the CB-interface is backed up
		MessageDialog::OnClosed(fOK);
		if (pStackCB)
		{
			pStackCB->DoCall(nullptr);
			pStackCB->DeRef();
		}
	}


// --------------------------------------------------
// ProgressDialog

	ProgressDialog::ProgressDialog(const char *szMessage, const char *szCaption, int32_t iMaxProgress, int32_t iInitialProgress, Icons icoIcon)
			: Dialog(C4GUI_ProgressDlgWdt, std::max(::GraphicsResource.TextFont.BreakMessage(szMessage, C4GUI_ProgressDlgWdt-3*C4GUI_DefDlgIndent-C4GUI_IconWdt, nullptr, 0, true), C4GUI_IconHgt) + C4GUI_ProgressDlgVRoom, szCaption, false)
	{
		// get positions
		ComponentAligner caMain(GetClientRect(), C4GUI_DefDlgIndent, C4GUI_DefDlgIndent, true);
		ComponentAligner caButtonArea(caMain.GetFromBottom(C4GUI_ButtonAreaHgt), 0,0);
		C4Rect rtProgressBar = caMain.GetFromBottom(C4GUI_ProgressDlgPBHgt);
		// place icon
		C4Rect rcIcon = caMain.GetFromLeft(C4GUI_IconWdt); rcIcon.Hgt = C4GUI_IconHgt;
		Icon *pIcon = new Icon(rcIcon, icoIcon); AddElement(pIcon);
		// place message label
		// use text with line breaks
		StdStrBuf str;
		::GraphicsResource.TextFont.BreakMessage(szMessage, C4GUI_ProgressDlgWdt-3*C4GUI_DefDlgIndent-C4GUI_IconWdt, &str, true);
		pLblMessage = new Label(str.getData(), caMain.GetAll().GetMiddleX(), caMain.GetAll().y, ACenter, C4GUI_MessageFontClr, &::GraphicsResource.TextFont);
		AddElement(pLblMessage);
		// place progress bar
		pBar = new ProgressBar(rtProgressBar, iMaxProgress);
		pBar->SetProgress(iInitialProgress);
		pBar->SetToolTip(LoadResStr("IDS_DLGTIP_PROGRESS"));
		AddElement(pBar);
		// place abort button
		Button *pBtnAbort = new CancelButton(caButtonArea.GetCentered(C4GUI_DefButtonWdt, C4GUI_ButtonHgt));
		AddElement(pBtnAbort);
	}

	void ProgressDialog::SetMessage(const char *szMessage)
	{
		StdStrBuf str;
		::GraphicsResource.TextFont.BreakMessage(szMessage, C4GUI_ProgressDlgWdt - 3 * C4GUI_DefDlgIndent - C4GUI_IconWdt, &str, true);
		pLblMessage->SetText(szMessage);
	}

// --------------------------------------------------
// Some dialog wrappers in Screen class

	bool Screen::ShowMessage(const char *szMessage, const char *szCaption, Icons icoIcon, int32_t *piConfigDontShowAgainSetting)
	{
		// always log messages
		LogSilentF("[Msg] %s: %s", szCaption, szMessage);
		if (piConfigDontShowAgainSetting && *piConfigDontShowAgainSetting) return true;
#ifdef USE_CONSOLE
		// skip in console mode
		return true;
#endif
		return ShowRemoveDlg(new MessageDialog(szMessage, szCaption, MessageDialog::btnOK, icoIcon, MessageDialog::dsRegular, piConfigDontShowAgainSetting));
	}

	bool Screen::ShowErrorMessage(const char *szMessage)
	{
		return ShowMessage(szMessage, LoadResStr("IDS_DLG_ERROR"), Ico_Error);
	}

	bool Screen::ShowMessageModal(const char *szMessage, const char *szCaption, DWORD dwButtons, Icons icoIcon, int32_t *piConfigDontShowAgainSetting)
	{
		// always log messages
		LogSilentF("[Modal] %s: %s", szCaption, szMessage);
		// skip if user doesn't want to see it
		if (piConfigDontShowAgainSetting && *piConfigDontShowAgainSetting) return true;
		// create message dlg and show modal
		return ShowModalDlg(new MessageDialog(szMessage, szCaption, dwButtons, icoIcon, MessageDialog::dsRegular, piConfigDontShowAgainSetting));
	}

	ProgressDialog *Screen::ShowProgressDlg(const char *szMessage, const char *szCaption, int32_t iMaxProgress, int32_t iInitialProgress, Icons icoIcon)
	{
		// create progress dlg
		ProgressDialog *pDlg = new ProgressDialog(szMessage, szCaption, iMaxProgress, iInitialProgress, icoIcon);
		// show it
		if (!pDlg->Show(this, true)) { delete pDlg; return nullptr; }
		// return dlg pointer
		return pDlg;
	}

	bool Screen::ShowModalDlg(Dialog *pDlg, bool fDestruct)
	{
#ifdef USE_CONSOLE
		// no modal dialogs in console build
		// (there's most likely no way to close them!)
		if (fDestruct) delete pDlg;
		return true;
#endif
		// safety
		if (!pDlg) return false;
		// show it
		if (!pDlg->Show(this, true)) { delete pDlg; return false; }
		// wait until it is closed
		bool fResult = pDlg->DoModal();
		if (fDestruct) delete pDlg;
		// return result
		return fResult;
	}

	bool Screen::ShowRemoveDlg(Dialog *pDlg)
	{
		// safety
		if (!pDlg) return false;
		// mark removal when done
		pDlg->SetDelOnClose();
		// show it
		if (!pDlg->Show(this, true)) { delete pDlg; return false; }
		// done, success
		return true;
	}


// --------------------------------------------------
// InputDialog

	InputDialog::InputDialog(const char *szMessage, const char *szCaption, Icons icoIcon, BaseInputCallback *pCB, bool fChatLayout)
			: Dialog(fChatLayout ? C4GUI::GetScreenWdt()*4/5 : C4GUI_InputDlgWdt,
			         fChatLayout ? C4GUI::Edit::GetDefaultEditHeight() + 2 :
			         std::max(::GraphicsResource.TextFont.BreakMessage(szMessage, C4GUI_InputDlgWdt - 3 * C4GUI_DefDlgIndent - C4GUI_IconWdt, nullptr, 0, true),
			             C4GUI_IconHgt) + C4GUI_InputDlgVRoom, szCaption, false),
			pEdit(nullptr), pCB(pCB), fChatLayout(fChatLayout), pChatLbl(nullptr)
	{
		if (fChatLayout)
		{
			// chat input layout
			C4GUI::ComponentAligner caChat(GetContainedClientRect(), 1,1);
			// normal chatbox layout: Left chat label
			int32_t w=40,h;
			::GraphicsResource.TextFont.GetTextExtent(szMessage, w,h, true);
			pChatLbl = new C4GUI::WoodenLabel(szMessage, caChat.GetFromLeft(w+4), C4GUI_CaptionFontClr, &::GraphicsResource.TextFont);
			caChat.ExpandLeft(2); // undo margin
			rcEditBounds = caChat.GetAll();
			SetCustomEdit(new Edit(rcEditBounds));
			pChatLbl->SetToolTip(LoadResStr("IDS_DLGTIP_CHAT"));
			AddElement(pChatLbl);
		}
		else
		{
			// regular input dialog layout
			// get positions
			ComponentAligner caMain(GetClientRect(), C4GUI_DefDlgIndent, C4GUI_DefDlgIndent, true);
			ComponentAligner caButtonArea(caMain.GetFromBottom(C4GUI_ButtonAreaHgt), 0,0);
			rcEditBounds = caMain.GetFromBottom(Edit::GetDefaultEditHeight());
			// place icon
			C4Rect rcIcon = caMain.GetFromLeft(C4GUI_IconWdt); rcIcon.Hgt = C4GUI_IconHgt;
			Icon *pIcon = new Icon(rcIcon, icoIcon); AddElement(pIcon);
			// place message label
			// use text with line breaks
			StdStrBuf str;
			::GraphicsResource.TextFont.BreakMessage(szMessage, C4GUI_InputDlgWdt - 3 * C4GUI_DefDlgIndent - C4GUI_IconWdt, &str, true);
			Label *pLblMessage = new Label(str.getData(), caMain.GetAll().GetMiddleX(), caMain.GetAll().y, ACenter, C4GUI_MessageFontClr, &::GraphicsResource.TextFont);
			AddElement(pLblMessage);
			// place input edit
			SetCustomEdit(new Edit(rcEditBounds));
			// place buttons
			C4Rect rcBtn = caButtonArea.GetCentered(2 * C4GUI_DefButton2Wdt + C4GUI_DefButton2HSpace, C4GUI_ButtonHgt);
			rcBtn.Wdt = C4GUI_DefButton2Wdt;
			// OK
			Button *pBtnOK = new OKButton(rcBtn);
			AddElement(pBtnOK);
			rcBtn.x += rcBtn.Wdt + C4GUI_DefButton2HSpace;
			// Cancel
			Button *pBtnAbort = new CancelButton(rcBtn);
			AddElement(pBtnAbort);
			rcBtn.x += rcBtn.Wdt + C4GUI_DefButton2HSpace;
		}
		// input dlg always closed in the end
		SetDelOnClose();
	}

	void InputDialog::SetInputText(const char *szToText)
	{
		pEdit->SelectAll(); pEdit->DeleteSelection();
		if (szToText)
		{
			pEdit->InsertText(szToText, false);
			pEdit->SelectAll();
		}
	}

	void InputDialog::SetCustomEdit(Edit *pCustomEdit)
	{
		// del old
		if (pEdit) delete pEdit;
		// add new
		pEdit = pCustomEdit;
		pEdit->SetBounds(rcEditBounds);
		if (fChatLayout)
		{
			pEdit->SetToolTip(LoadResStr("IDS_DLGTIP_CHAT"));
			pChatLbl->SetClickFocusControl(pEdit); // 2do: to all, to allies, etc.
		}
		AddElement(pEdit);
		SetFocus(pEdit, false);
	}


// --------------------------------------------------
// InfoDialog

	InfoDialog::InfoDialog(const char *szCaption, int32_t iLineCount)
			: Dialog(C4GUI_InfoDlgWdt, ::GraphicsResource.TextFont.GetLineHeight()*iLineCount + C4GUI_InfoDlgVRoom, szCaption, false), iScroll(0)
	{
		// timer
		Application.Add(this);
		CreateSubComponents();
	}

	InfoDialog::InfoDialog(const char *szCaption, int iLineCount, const StdStrBuf &sText)
			: Dialog(C4GUI_InfoDlgWdt, ::GraphicsResource.TextFont.GetLineHeight()*iLineCount + C4GUI_InfoDlgVRoom, szCaption, false), iScroll(0)
	{
		// ctor - init w/o timer
		CreateSubComponents();
		// fill in initial text
		for (size_t i=0; i < sText.getLength(); ++i)
		{
			size_t i0 = i;
			while (sText[i] != '|' && sText[i]) ++i;
			StdStrBuf sLine = sText.copyPart(i0, i-i0);
			pTextWin->AddTextLine(sLine.getData(), &::GraphicsResource.TextFont, C4GUI_MessageFontClr, false, true);
		}
		pTextWin->UpdateHeight();
	}
	InfoDialog::~InfoDialog()
	{
		Application.Remove(this);
	}

	void InfoDialog::CreateSubComponents()
	{
		// get positions
		ComponentAligner caMain(GetClientRect(), C4GUI_DefDlgIndent, C4GUI_DefDlgIndent, true);
		ComponentAligner caButtonArea(caMain.GetFromBottom(C4GUI_ButtonAreaHgt), 0,0);
		// place info box
		pTextWin = new TextWindow(caMain.GetAll(), 0, 0, 0, 100, 4096, "  ", true, nullptr, 0);
		AddElement(pTextWin);
		// place close button
		Button *pBtnClose = new DlgCloseButton(caButtonArea.GetCentered(C4GUI_DefButtonWdt, C4GUI_ButtonHgt));
		AddElement(pBtnClose); pBtnClose->SetToolTip(LoadResStr("IDS_MNU_CLOSE"));
	}

	void InfoDialog::AddLine(const char *szText)
	{
		// add line to text window
		if (!pTextWin) return;
		pTextWin->AddTextLine(szText, &::GraphicsResource.TextFont, C4GUI_MessageFontClr, false, true);
	}

	void InfoDialog::AddLineFmt(const char *szFmtString, ...)
	{
		// compose formatted line
		va_list lst; va_start(lst, szFmtString);
		StdStrBuf buf;
		buf.FormatV(szFmtString, lst);
		// add it
		AddLine(buf.getData());
	}

	void InfoDialog::BeginUpdateText()
	{
		// safety
		if (!pTextWin) return;
		// backup scrolling
		iScroll = pTextWin->GetScrollPos();
		// clear text window, so new text can be added
		pTextWin->ClearText(false);
	}

	void InfoDialog::EndUpdateText()
	{
		// safety
		if (!pTextWin) return;
		// update text height
		pTextWin->UpdateHeight();
		// restore scrolling
		pTextWin->SetScrollPos(iScroll);
	}

	void InfoDialog::OnSec1Timer()
	{
		// always update
		UpdateText();
	}

} // end of namespace

