/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2013, The OpenClonk Team and contributors
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

/* In-game menu as used by objects, players, and fullscreen options */

#include <C4Include.h>
#include <C4Menu.h>

#include <C4DefList.h>
#include <C4Object.h>
#include <C4Viewport.h>
#include <C4Player.h>
#include <C4MouseControl.h>
#include <C4GraphicsResource.h>
#include <C4Game.h>
#include <C4PlayerList.h>
#include <C4GameControl.h>

const int32_t C4MN_InfoCaption_Delay = 90;

// -----------------------------------------------------------
// C4MenuItem

C4MenuItem::C4MenuItem(C4Menu *pMenu, int32_t iIndex, const char *szCaption,
                       const char *szCommand, int32_t iCount, C4Object *pObject, const char *szInfoCaption,
                       C4ID idID, const char *szCommand2, bool fOwnValue, int32_t iValue, int32_t iStyle, bool fIsSelectable)
		: C4GUI::Element(), pSymbolGraphics(NULL), dwSymbolClr(0u), fSelected(false),
		  iStyle(iStyle), pMenu(pMenu), iIndex(iIndex)
{
	*Caption=*Command=*InfoCaption=0;
	Symbol.Default();
	SCopy(szCaption,Caption,C4MaxTitle);
	SCopy(szCommand,Command,_MAX_FNAME+30);
	SCopy(szInfoCaption,InfoCaption,C4MaxTitle);
	// some info caption corrections
	SReplaceChar(InfoCaption, 10, ' '); SReplaceChar(InfoCaption, 13, '|');
	SetToolTip(InfoCaption);
}

C4MenuItem::~C4MenuItem()
{
	Symbol.Clear();
}

int32_t C4MenuItem::GetSymbolWidth(int32_t iForHeight)
{
	// Context menus
	if (iStyle==C4MN_Style_Context)
		return std::max(Symbol.Wdt * iForHeight / std::max(Symbol.Hgt, 1.0f), static_cast<float>(iForHeight));
	// no symbol
	return 0;
}

void C4MenuItem::DrawElement(C4TargetFacet &cgo)
{
	// get target pos
	C4Facet cgoOut(cgo.Surface, cgo.TargetX + rcBounds.x, cgo.TargetY + rcBounds.y, rcBounds.Wdt, rcBounds.Hgt);
	// Select mark
	if (fSelected)
		pDraw->DrawBoxDw(cgo.Surface, cgoOut.X, cgoOut.Y, cgoOut.X + cgoOut.Wdt - 1, cgoOut.Y + cgoOut.Hgt - 1, C4RGB(0xca, 0, 0));
	// Symbol/text areas
	C4Facet cgoItemSymbol,cgoItemText;
	cgoItemSymbol=cgoItemText=cgoOut;
	int32_t iSymWidth;
	if ((iSymWidth = GetSymbolWidth(cgoItemText.Hgt)))
	{
		// get symbol area
		cgoItemSymbol=cgoItemText.Truncate(C4FCT_Left, iSymWidth);
	}
	// cgoItemSymbol.Hgt is 0. This means rcBounds.Hgt is 0. That
	// makes no sense at this point, so let's just draw in a
	// square area at item y.
	C4Facet cgoSymbolOut(cgoItemSymbol.Surface, cgoItemSymbol.X, cgoItemSymbol.Y, cgoItemSymbol.Wdt, cgoItemSymbol.Wdt);

	// Draw item symbol:
	if (pSymbolGraphics)
	{
		pSymbolGraphics->Draw(cgoSymbolOut, dwSymbolClr ? dwSymbolClr : 0xffffffff, NULL, 0, 0, NULL);
	}
	else if (Symbol.Surface)
		Symbol.DrawClr(cgoItemSymbol, true, dwSymbolClr);

	// Draw item text
	pDraw->StorePrimaryClipper(); pDraw->SubPrimaryClipper(cgoItemText.X, cgoItemText.Y, cgoItemText.X+cgoItemText.Wdt-1, cgoItemText.Y+cgoItemText.Hgt-1);
	if (iStyle == C4MN_Style_Context)
		pDraw->TextOut(Caption,::GraphicsResource.FontRegular, 1.0, cgoItemText.Surface,cgoItemText.X,cgoItemText.Y,C4Draw::DEFAULT_MESSAGE_COLOR,ALeft);
	pDraw->RestorePrimaryClipper();
}

void C4MenuItem::MouseInput(C4GUI::CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam)
{
	// clicky clicky!
	if (iButton == C4MC_Button_LeftUp || iButton == C4MC_Button_RightUp)
	{
		pMenu->UserEnter(::MouseControl.GetPlayer(), this);
		return;
	}
	// inherited; this is just setting some vars
	typedef C4GUI::Element ParentClass;
	ParentClass::MouseInput(rMouse, iButton, iX, iY, dwKeyParam);
}

void C4MenuItem::MouseEnter(C4GUI::CMouse &rMouse)
{
	// callback to menu: Select item
	pMenu->UserSelectItem(::MouseControl.GetPlayer(), this);
	typedef C4GUI::Element ParentClass;
	ParentClass::MouseEnter(rMouse);
}

// -----------------------------------------------------------
// C4Menu

C4Menu::C4Menu() : C4GUI::Dialog(100, 100, NULL, true) // will be re-adjusted later
{
	Default();
	AddElement(pClientWindow = new C4GUI::ScrollWindow(this));
	// initially invisible: Will be made visible at first drawing by viewport
	// when the location will be inialized
	SetVisibility(false);
}

void C4Menu::Default()
{
	Selection=-1;
	Style=C4MN_Style_Normal;
	ItemCount=0;
	ItemWidth=ItemHeight=C4SymbolSize;
	NeedRefill=false;
	Symbol.Default();
	Caption[0]=0;
	Permanent=false;
	TimeOnSelection=0;
	Identification=0;
	LocationSet=false;
	Columns=Lines=0;
	Alignment= C4MN_Align_Right | C4MN_Align_Bottom;
	CloseCommand.Clear();
	fActive = false;
}

void C4Menu::Clear()
{
	Close(false);
	Symbol.Clear();
	ClearItems();
	ClearFrameDeco();
	fActive = false;
}

bool C4Menu::TryClose(bool fOK, bool fControl)
{
	// abort if menu is permanented by script
	if (!fOK) if (IsCloseDenied()) return false;

	// close the menu
	Close(fOK);
	Clear();
	::pGUI->RemoveElement(this);

	// invoke close command
	if (fControl && CloseCommand.getData())
	{
		MenuCommand(CloseCommand.getData(), true);
	}

	// done
	return true;
}

bool C4Menu::DoInit(C4FacetSurface &fctSymbol, const char *szEmpty, int32_t iExtra, int32_t iExtraData, int32_t iId, int32_t iStyle)
{
	Clear(); Default();
	Symbol.GrabFrom(fctSymbol);
	return InitMenu(szEmpty, iExtra, iExtraData, iId, iStyle);
}

bool C4Menu::DoInitRefSym(const C4Facet &fctSymbol, const char *szEmpty, int32_t iExtra, int32_t iExtraData, int32_t iId, int32_t iStyle)
{
	Clear(); Default();
	Symbol.Set(fctSymbol);
	return InitMenu(szEmpty, iExtra, iExtraData, iId, iStyle);
}

bool C4Menu::InitMenu(const char *szEmpty, int32_t iExtra, int32_t iExtraData, int32_t iId, int32_t iStyle)
{
	SCopy(szEmpty,Caption,C4MaxTitle);
	Identification=iId;
	if (*Caption) SetTitle(Caption, HasMouse()); else SetTitle(" ", HasMouse());
	if (pTitle) pTitle->SetIcon(Symbol);
	Style=iStyle;
	// Menus are synchronous to allow COM_MenuUp/Down to be converted to movements at the clients
	if (Style == C4MN_Style_Normal)
		Columns = 5;
	else
		Columns=1;
	::pGUI->ShowDialog(this, false);
	fActive = true;
	return true;
}

bool C4Menu::AddRefSym(const char *szCaption, const C4Facet &fctSymbol, const char *szCommand,
                       int32_t iCount, C4Object *pObject, const char *szInfoCaption,
                       C4ID idID, const char *szCommand2, bool fOwnValue, int32_t iValue, bool fIsSelectable)
{
	if (!IsActive()) return false;
	// Create new menu item
	C4MenuItem *pNew = new C4MenuItem(this, ItemCount, szCaption,szCommand,iCount,pObject,szInfoCaption,idID,szCommand2,fOwnValue,iValue,Style,fIsSelectable);
	// Ref Symbol
	pNew->RefSymbol(fctSymbol);
	// Add
	return AddItem(pNew, szCaption, szCommand, iCount, pObject, szInfoCaption, idID, szCommand2, fOwnValue, iValue, fIsSelectable);
}

bool C4Menu::Add(const char *szCaption, C4FacetSurface &fctSymbol, const char *szCommand,
                 int32_t iCount, C4Object *pObject, const char *szInfoCaption,
                 C4ID idID, const char *szCommand2, bool fOwnValue, int32_t iValue, bool fIsSelectable)
{
	if (!IsActive()) return false;
	// Create new menu item
	C4MenuItem *pNew = new C4MenuItem(this, ItemCount, szCaption,szCommand,iCount,pObject,szInfoCaption,idID,szCommand2,fOwnValue,iValue,Style,fIsSelectable);
	// Set Symbol
	pNew->GrabSymbol(fctSymbol);
	// Add
	return AddItem(pNew, szCaption, szCommand, iCount, pObject, szInfoCaption, idID, szCommand2, fOwnValue, iValue, fIsSelectable);
}

bool C4Menu::AddItem(C4MenuItem *pNew, const char *szCaption, const char *szCommand,
                     int32_t iCount, C4Object *pObject, const char *szInfoCaption,
                     C4ID idID, const char *szCommand2, bool fOwnValue, int32_t iValue, bool fIsSelectable)
{
	// Add it to the list
	pClientWindow->AddElement(pNew);
	// Item count
	ItemCount++;
	// set new item size
	if (!pClientWindow->IsFrozen()) UpdateElementPositions();
	// Init selection if not frozen
	if (Selection==-1 && !pClientWindow->IsFrozen()) SetSelection(ItemCount-1, false, false);
	// adjust scrolling, etc.
	UpdateScrollBar();
	// Success
	return true;
}

bool C4Menu::Control(BYTE byCom, int32_t iData)
{
	if (!IsActive()) return false;

	switch (byCom)
	{
	case COM_MenuEnter: Enter(); break;
	case COM_MenuClose: TryClose(false, true); break;

		// organize with nicer subfunction...
	case COM_MenuLeft:
		// Top wrap-around
		if (Selection-1 < 0)
			MoveSelection(ItemCount - 1 - Selection, true, true);
		else
			MoveSelection(-1, true, true);
		break;
	case COM_MenuRight:
		// Bottom wrap-around
		if (Selection+1 >= ItemCount)
			MoveSelection(-Selection, true, true);
		else
			MoveSelection(+1, true, true);
		break;
	case COM_MenuUp:
		iData = -Columns;
		// Top wrap-around
		if (Selection + iData < 0)
			while (Selection + iData + Columns < ItemCount)
				iData += Columns;
		MoveSelection(iData, true, true);
		break;
	case COM_MenuDown:
		iData = +Columns;
		// Bottom wrap-around
		if (Selection+iData >= ItemCount)
			while (Selection+iData-Columns >= 0)
				iData-=Columns;
		MoveSelection(iData, true, true);
		break;
	}

	return true;
}

bool C4Menu::KeyControl(BYTE byCom)
{
	// direct keyboard callback
	if (!IsActive()) return false;
	return !!Control(byCom, 0);
}

bool C4Menu::IsActive()
{
	return fActive;
}

bool C4Menu::Enter()
{
	// Not active
	if (!IsActive()) return false;
	// Get selected item
	C4MenuItem *pItem = GetSelectedItem();
	if (!pItem)
		return true;
	// Copy command to buffer (menu might be cleared)
	char szCommand[_MAX_FNAME+30+1];
	SCopy(pItem->Command,szCommand);

	// Close if not permanent
	if (!Permanent) { Close(true); fActive = false; }

	// exec command (note that menu callback may delete this!)
	MenuCommand(szCommand, false);

	return true;
}

C4MenuItem* C4Menu::GetItem(int32_t iIndex)
{
	return static_cast<C4MenuItem *>(pClientWindow->GetElementByIndex(iIndex));
}

int32_t C4Menu::GetItemCount()
{
	return ItemCount;
}

bool C4Menu::MoveSelection(int32_t iBy, bool fAdjustPosition, bool fDoCalls)
{
	if (!iBy) return false;
	// find next item that can be selected by moving in iBy steps
	int32_t iNewSel = Selection;
	for (;;)
	{
		// determine new selection
		iNewSel += iBy;
		// selection is out of menu range
		if (!Inside<int32_t>(iNewSel, 0, ItemCount-1)) return false;
		// got something: go select it
		if (GetItem(iNewSel)) break;
	}
	// select it
	return !!SetSelection(iNewSel, fAdjustPosition, fDoCalls);
}

bool C4Menu::SetSelection(int32_t iSelection, bool fAdjustPosition, bool fDoCalls)
{
	// Not active
	if (!IsActive()) return false;
	// Outside Limits / Selectable
	C4MenuItem *pNewSel = GetItem(iSelection);
	if ((iSelection==-1 && !ItemCount) || pNewSel)
	{
		// Selection change
		if (iSelection!=Selection)
		{
			// calls
			C4MenuItem *pSel = GetSelectedItem();
			if (pSel) pSel->SetSelected(false);
			// Set selection
			Selection=iSelection;
			// Reset time on selection
			TimeOnSelection=0;
		}
		// always recheck selection for internal refill
		C4MenuItem *pSel = GetSelectedItem();
		if (pSel) pSel->SetSelected(true);
		// set main caption by selection
		if (Style == C4MN_Style_Normal)
		{
			if (pSel)
				SetTitle(*(pSel->Caption) ? pSel->Caption : " ", HasMouse());
			else
				SetTitle(*Caption ? Caption : " ", HasMouse());
		}
	}
	// adjust position, if desired
	if (fAdjustPosition) AdjustPosition();
	// do selection callback
	if (fDoCalls) OnSelectionChanged(Selection);
	// Done
	return true;
}

C4MenuItem* C4Menu::GetSelectedItem()
{
	return GetItem(Selection);
}

void C4Menu::AdjustPosition()
{
	// Adjust position by selection (works only after InitLocation)
	if ((Lines > 1) && Columns)
	{
		C4MenuItem *pSel = GetSelectedItem();
		if (pSel)
			pClientWindow->ScrollRangeInView(pSel->GetBounds().y, pSel->GetBounds().Hgt);
	}
}

int32_t C4Menu::GetSelection()
{
	return Selection;
}

bool C4Menu::SetPosition(int32_t iPosition)
{
	if (!IsActive()) return false;
	// update scroll pos, if location is initialized
	if (IsVisible() && pClientWindow) pClientWindow->SetScroll((iPosition/Columns) * ItemHeight);
	return true;
}

int32_t C4Menu::GetIdentification()
{
	return Identification;
}

void C4Menu::SetSize(int32_t iToWdt, int32_t iToHgt)
{
	if (iToWdt) Columns=iToWdt;
	if (iToHgt) Lines=iToHgt;
	InitSize();
}

void C4Menu::InitLocation(C4Facet &cgoArea)
{

	// Item size by style
	switch (Style)
	{
	case C4MN_Style_Normal:
		ItemWidth=ItemHeight=C4SymbolSize;
		break;
	case C4MN_Style_Context:
	{
		ItemHeight = std::max<int32_t>(C4MN_SymbolSize, ::GraphicsResource.FontRegular.GetLineHeight());
		int32_t iWdt, iHgt;
		::GraphicsResource.FontRegular.GetTextExtent(Caption, ItemWidth, iHgt, true);
		// FIXME: Blah. This stuff should be calculated correctly by pTitle.
		ItemWidth += ItemHeight + 16;
		C4MenuItem *pItem;
		for (int i = 0; (pItem = GetItem(i)); ++i)
		{
			::GraphicsResource.FontRegular.GetTextExtent(pItem->Caption, iWdt, iHgt, true);
			ItemWidth = std::max(ItemWidth, iWdt + pItem->GetSymbolWidth(ItemHeight));
		}
		ItemWidth += 3; // Add some extra space so text doesn't touch right border frame...
		break;
	}
	}

	Lines = ItemCount/Columns+std::min<int32_t>(ItemCount%Columns,1);
	// adjust by max. height
	Lines=std::max<int32_t>(std::min<int32_t>((cgoArea.Hgt-100)/std::max<int32_t>(ItemHeight,1),Lines),1);

	InitSize();
	int32_t X,Y;
	X = (cgoArea.Wdt - rcBounds.Wdt)/2;
	Y = (cgoArea.Hgt - rcBounds.Hgt)/2;
	// Alignment
	if (Alignment & C4MN_Align_Left) X=C4SymbolSize;
	if (Alignment & C4MN_Align_Right) X=cgoArea.Wdt-2*C4SymbolSize-rcBounds.Wdt;
	if (Alignment & C4MN_Align_Top) Y=C4SymbolSize;
	if (Alignment & C4MN_Align_Bottom) Y=cgoArea.Hgt-C4SymbolSize-rcBounds.Hgt;
	// Centered (due to small viewport size)
	if (rcBounds.Wdt>cgoArea.Wdt-2*C4SymbolSize) X=(cgoArea.Wdt-rcBounds.Wdt)/2;
	if (rcBounds.Hgt>cgoArea.Hgt-2*C4SymbolSize) Y=(cgoArea.Hgt-rcBounds.Hgt)/2;
	SetPos(X, Y);

	// Position initialized: Make it visible to be used!
	SetVisibility(true);

	// now align all menu items correctly
	UpdateElementPositions();

	// and correct scroll pos
	UpdateScrollBar();
	AdjustPosition();
}


void C4Menu::InitSize()
{
	C4GUI::Element *pLast = pClientWindow->GetLast();
	// Size calculation
	int Width, Height;
	Width=Columns*ItemWidth;
	Height=Lines*ItemHeight;
	bool fBarNeeded;
	fBarNeeded = pLast && pLast->GetBounds().y + pLast->GetBounds().Hgt > pClientWindow->GetBounds().Hgt;
	// add dlg margins
	Width += GetMarginLeft() + GetMarginRight() + pClientWindow->GetMarginLeft() + pClientWindow->GetMarginRight();
	Height += GetMarginTop() + GetMarginBottom() + pClientWindow->GetMarginTop() + pClientWindow->GetMarginBottom();
	if (fBarNeeded) Width += C4GUI_ScrollBarWdt;
	SetBounds(C4Rect(rcBounds.x, rcBounds.y, Width, Height));
	pClientWindow->SetScrollBarEnabled(fBarNeeded);
	UpdateOwnPos();
}

void C4Menu::UpdateScrollBar()
{
	C4GUI::Element *pLast = pClientWindow->GetLast();
	bool fBarNeeded = pLast && pLast->GetBounds().y + pLast->GetBounds().Hgt > pClientWindow->GetBounds().Hgt;
	if (pClientWindow->IsScrollBarEnabled() == fBarNeeded) return;
	// resize for bar
	InitSize();
}

void C4Menu::Draw(C4TargetFacet &cgo)
{
	// Inactive
	if (!IsActive()) return;

	// Location
	if (!LocationSet) { InitLocation(cgo); LocationSet=true; }

	// If drawn by a viewport, then it's made visible
	SetVisibility(true);

	// do drawing
	typedef C4GUI::Dialog ParentClass;
	ParentClass::Draw(cgo);

	// draw tooltip if selection time has been long enough
	++TimeOnSelection;
	if (TimeOnSelection >= C4MN_InfoCaption_Delay && !::Control.isReplay() && !::pGUI->Mouse.IsActiveInput())
	{
		C4MenuItem *pSel = GetSelectedItem();
		if (pSel && *pSel->InfoCaption)
		{
			int32_t iX=0, iY=0;
			pSel->ClientPos2ScreenPos(iX, iY);
			C4GUI::Screen::DrawToolTip(pSel->InfoCaption, cgo, iX, iY);
		}
	}
}

void C4Menu::DrawFrame(C4Surface * sfcSurface, int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt)
{
	pDraw->DrawFrameDw(sfcSurface, iX+1, iY+1, iX+iWdt-1,iY+iHgt-1, C4RGB(0x44, 0, 0));
}

void C4Menu::SetAlignment(int32_t iAlignment)
{
	Alignment = iAlignment;
}

void C4Menu::SetPermanent(bool fPermanent)
{
	Permanent = fPermanent;
}

bool C4Menu::RefillInternal()
{
	// Reset flag
	NeedRefill=false;

	// do the refill in frozen window (no scrolling update)
	int32_t iLastItemCount=ItemCount;
	bool fRefilled = false;
	pClientWindow->Freeze();
	bool fSuccess = DoRefillInternal(fRefilled);
	pClientWindow->UnFreeze();
	UpdateElementPositions();
	if (!fSuccess) return false;

	// menu contents may have changed: Adjust menu size and selection
	if (fRefilled)
	{
		// Adjust selection
		AdjustSelection();
		// Item count increased: resize
		if (ItemCount > iLastItemCount) LocationSet = false;
		// Item count decreased: resize if we are a context menu
		if ((ItemCount < iLastItemCount) && IsContextMenu()) LocationSet = false;
	}
	// Success
	return true;
}

void C4Menu::ClearItems()
{
	C4MenuItem *pItem;
	while ((pItem = GetItem(0))) delete pItem;
	ItemCount=0;
	UpdateScrollBar();
}

void C4Menu::Execute()
{
	if (!IsActive()) return;
	// Refill (timer or flag)
	if (!Game.iTick35 || NeedRefill)
		if (!RefillInternal())
			Close(false);
}

bool C4Menu::Refill()
{
	if (!IsActive()) return false;
	// Refill (close if failure)
	if (!RefillInternal())
		{ Close(false); return false; }
	// Success
	return true;
}

void C4Menu::AdjustSelection()
{
	// selection valid?
	C4MenuItem *pSelection = GetItem(Selection);
	int iSel = Selection;
	if (!pSelection)
	{
		// set to new first valid selection: Downwards first
		iSel = Selection;
		while (--iSel>=0)
			if ((pSelection = GetItem(iSel)))
				break;
		// no success: upwards then
		if (iSel<0)
			for (iSel=Selection+1; (pSelection = GetItem(iSel)); ++iSel)
				break;
	}
	// set it then
	if (!pSelection)
		SetSelection(-1, Selection >= 0, false);
	else
		SetSelection(iSel, iSel != Selection, true);
}

bool C4Menu::SetLocation(int32_t iX, int32_t iY)
{
	// just set position...
	SetPos(iX, iY);
	return true;
}

C4Viewport *C4Menu::GetViewport()
{
	// ask all viewports
	for (C4Viewport *pVP = ::Viewports.GetFirstViewport(); pVP; pVP = pVP->GetNext())
		if (pVP->IsViewportMenu(this))
			return pVP;
	// none matching
	return NULL;
}

void C4Menu::UpdateElementPositions()
{
	// only if already shown and made visible by first drawing
	// this will postpone the call of menu initialization until it's really needed
	if (!IsVisible() || !pClientWindow) return;
	// reposition client scrolling window
	pClientWindow->SetBounds(GetContainedClientRect());
	// re-stack all list items
	C4MenuItem *pCurr = static_cast<C4MenuItem *>(pClientWindow->GetFirst());
	// recheck list items
	int32_t iMaxDlgOptionHeight = -1;
	int32_t iIndex = 0; C4Rect rcNewBounds(0,0,ItemWidth,ItemHeight);
	C4MenuItem *pFirstStack = pCurr, *pNext = pFirstStack;
	while ((pCurr = pNext))
	{
		pNext = static_cast<C4MenuItem *>(pCurr->GetNext());
		rcNewBounds.x = (iIndex % std::max<int32_t>(Columns, 1)) * ItemWidth;
		rcNewBounds.y = (iIndex / std::max<int32_t>(Columns, 1)) * ItemHeight;
		if (pCurr->GetBounds() != rcNewBounds)
		{
			pCurr->GetBounds() = rcNewBounds;
			pCurr->UpdateOwnPos();
		}
		++iIndex;
	}
	// update scrolling
	pClientWindow->SetClientHeight(rcNewBounds.y + rcNewBounds.Hgt);
	// re-set caption
	C4MenuItem *pSel = GetSelectedItem();
	const char *szCapt;
	if (pSel && Style == C4MN_Style_Normal)
		szCapt = pSel->Caption;
	else
		szCapt = Caption;
	SetTitle((*szCapt) ? szCapt : " ", HasMouse());
}

void C4Menu::UpdateOwnPos()
{
	// client rect and stuff
	typedef C4GUI::Dialog ParentClass;
	ParentClass::UpdateOwnPos();
	UpdateElementPositions();
}

void C4Menu::UserSelectItem(int32_t Player, C4MenuItem *pItem)
{
	// not if user con't control anything
	if (IsReadOnly()) return;
	if (!pItem) return;
	// queue or direct selection
	OnUserSelectItem(Player, pItem->iIndex);
}

void C4Menu::UserEnter(int32_t Player, C4MenuItem *pItem)
{
	// not if user con't control anything
	if (IsReadOnly()) return;
	if (!pItem) return;
	// queue or direct enter
	OnUserEnter(Player, pItem->iIndex);
}

void C4Menu::UserClose(bool fOK)
{
	// not if user con't control anything
	if (IsReadOnly()) return;
	// queue or direct enter
	OnUserClose();
}

void C4Menu::SetCloseCommand(const char *strCommand)
{
	CloseCommand.Copy(strCommand);
}

bool C4Menu::HasMouse()
{
	int32_t iPlayer = GetControllingPlayer();
	if (iPlayer == NO_OWNER) return true; // free view dialog also has the mouse
	C4Player *pPlr = ::Players.Get(iPlayer);
	if (pPlr && pPlr->MouseControl) return true;
	return false;
}

#ifdef _DEBUG
void C4Menu::AssertSurfaceNotUsed(C4Surface *sfc)
{
	C4MenuItem *pItem;
	if (!sfc) return;
	assert(sfc != Symbol.Surface);
	for (int32_t i=0; (pItem = GetItem(i)); ++i)
		assert(pItem->Symbol.Surface != sfc) ;
}
#endif
