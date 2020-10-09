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

/* In-game menu as used by objects, players, and fullscreen options */

#include "C4Include.h"
#include "gui/C4Menu.h"

#include "control/C4GameControl.h"
#include "game/C4Viewport.h"
#include "graphics/C4Draw.h"
#include "graphics/C4GraphicsResource.h"
#include "gui/C4MouseControl.h"
#include "lib/StdColors.h"
#include "object/C4Def.h"
#include "object/C4DefList.h"
#include "object/C4Object.h"
#include "player/C4Player.h"
#include "player/C4PlayerList.h"

const int32_t     C4MN_DefInfoWdt     = 270, // default width of info windows
                  C4MN_DlgWdt         = 270, // default width of dialog windows
                  C4MN_DlgLines       = 5,  // default number of text lines visible in a dialog window
                  C4MN_DlgLineMargin  = 5,  // px distance between text items
                  C4MN_DlgOptionLineMargin = 3,  // px distance between dialog option items
                  C4MN_DlgPortraitWdt = 64, // size of portrait
                  C4MN_DlgPortraitIndent = 5; // space between portrait and text

const int32_t C4MN_InfoCaption_Delay = 90;

// -----------------------------------------------------------
// C4MenuItem

C4MenuItem::C4MenuItem(C4Menu *pMenu, int32_t iIndex, const char *szCaption,
                       const char *szCommand, int32_t iCount, C4Object *pObject, const char *szInfoCaption,
                       C4ID idID, const char *szCommand2, bool fOwnValue, int32_t iValue, int32_t iStyle, bool fIsSelectable)
		: C4GUI::Element(), Count(iCount), id(idID), Object(pObject), pSymbolObj(nullptr), pSymbolGraphics(nullptr), dwSymbolClr(0u),
		fOwnValue(fOwnValue), iValue(iValue), fSelected(false), iStyle(iStyle), pMenu(pMenu),
		iIndex(iIndex), IsSelectable(fIsSelectable), TextDisplayProgress(-1)
{
	*Caption=*Command=*Command2=*InfoCaption=0;
	Symbol.Default();
	SCopy(szCaption,Caption,C4MaxTitle);
	SCopy(szCommand,Command,_MAX_FNAME+30);
	SCopy(szCommand2,Command2,_MAX_FNAME+30);
	SCopy(szInfoCaption,InfoCaption,C4MaxTitle);
	// some info caption corrections
	SReplaceChar(InfoCaption, 10, ' '); SReplaceChar(InfoCaption, 13, '|');
	SetToolTip(InfoCaption);
}

C4MenuItem::~C4MenuItem()
{
	Symbol.Clear();
}

void C4MenuItem::DoTextProgress(int32_t &riByVal)
{
	// any progress to be done?
	if (TextDisplayProgress<0) return;
	// if this is an option or empty text, show it immediately
	if (IsSelectable || !*Caption) { TextDisplayProgress=-1; return; }
	// normal text: move forward in unbroken message, ignoring markup
	StdStrBuf sText(Caption);
	C4Markup MarkupChecker(false);
	const char *szPos = sText.getPtr(std::min<int>(TextDisplayProgress, sText.getLength()));
	while (riByVal && *szPos)
	{
		MarkupChecker.SkipTags(&szPos);
		if (!*szPos) break;
		--riByVal;

		// Advance one UTF-8 character
		uint32_t c = GetNextCharacter(&szPos);
		// Treat embedded images {{XXX}} as one entity
		if(c == '{' && *szPos == '{')
		{
			int32_t end = SCharPos('}', szPos);
			if(end > 0 && szPos[end+1] == '}')
				szPos += end + 2;
		}
	}
	if (!*szPos)
		TextDisplayProgress=-1;
	else
		TextDisplayProgress = szPos - Caption;
}

bool C4MenuItem::IsDragElement()
{
	// any constructibles can be dragged
	C4Def *pDef = C4Id2Def(id);
	return pDef && pDef->Constructable;
}

int32_t C4MenuItem::GetSymbolWidth(int32_t iForHeight)
{
	// Context or dialog menus
	if (iStyle==C4MN_Style_Context || (iStyle==C4MN_Style_Dialog && Symbol.Surface))
		return std::max(Symbol.Wdt * iForHeight / std::max(Symbol.Hgt, 1.0f), static_cast<float>(iForHeight));
	// Info menus
	if (iStyle==C4MN_Style_Info && Symbol.Surface && Symbol.Wdt)
		return Symbol.Wdt;
	// no symbol
	return 0;
}

void C4MenuItem::DrawElement(C4TargetFacet &cgo)
{
	// get target pos
	C4Facet cgoOut(cgo.Surface, cgo.TargetX + rcBounds.x, cgo.TargetY + rcBounds.y, rcBounds.Wdt, rcBounds.Hgt);
	// Select mark
	if (iStyle!=C4MN_Style_Info)
		if (fSelected && TextDisplayProgress)
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
	// Draw if there is no text progression at all (TextDisplayProgress==-1, or if it's progressed far enough already (TextDisplayProgress>0)
	if(pSymbolObj && TextDisplayProgress)
	{
		pSymbolObj->DrawPicture(cgoSymbolOut, false, nullptr);
	}
	else if (pSymbolGraphics && TextDisplayProgress)
	{
		pSymbolGraphics->Draw(cgoSymbolOut, dwSymbolClr ? dwSymbolClr : 0xffffffff, nullptr, 0, 0, nullptr);
	}
	else if (Symbol.Surface && TextDisplayProgress)
		Symbol.DrawClr(cgoItemSymbol, true, dwSymbolClr);

	// Draw item text
	pDraw->StorePrimaryClipper(); pDraw->SubPrimaryClipper(cgoItemText.X, cgoItemText.Y, cgoItemText.X+cgoItemText.Wdt-1, cgoItemText.Y+cgoItemText.Hgt-1);
	switch (iStyle)
	{
	case C4MN_Style_Context:
		pDraw->TextOut(Caption,::GraphicsResource.FontRegular, 1.0, cgoItemText.Surface,cgoItemText.X,cgoItemText.Y,C4Draw::DEFAULT_MESSAGE_COLOR,ALeft);
		break;
	case C4MN_Style_Info:
	{
		StdStrBuf sText;
		::GraphicsResource.FontRegular.BreakMessage(InfoCaption, cgoItemText.Wdt, &sText, true);
		pDraw->TextOut(sText.getData(), ::GraphicsResource.FontRegular, 1.0, cgoItemText.Surface,cgoItemText.X,cgoItemText.Y);
		break;
	}
	case C4MN_Style_Dialog:
	{
		// cut buffer at text display pos
		char cXChg='\0'; int iStopPos = 0;
		if (TextDisplayProgress>-1)
		{
			iStopPos = std::min<int>(TextDisplayProgress, strlen(Caption));
			cXChg = Caption[iStopPos];
			Caption[iStopPos] = '\0';
		}
		// display broken text
		StdStrBuf sText;
		::GraphicsResource.FontRegular.BreakMessage(Caption, cgoItemText.Wdt, &sText, true);
		pDraw->TextOut(sText.getData(),::GraphicsResource.FontRegular, 1.0, cgoItemText.Surface,cgoItemText.X,cgoItemText.Y);
		// restore complete text
		if (cXChg) Caption[iStopPos] = cXChg;
		break;
	}
	}
	pDraw->RestorePrimaryClipper();
	// Draw count
	if (Count!=C4MN_Item_NoCount)
	{
		char szCount[10+1];
		sprintf(szCount,"%ix",Count);
		pDraw->TextOut(szCount, ::GraphicsResource.FontRegular, 1.0, cgoItemText.Surface, cgoItemText.X+cgoItemText.Wdt-1, cgoItemText.Y+cgoItemText.Hgt-1-::GraphicsResource.FontRegular.GetLineHeight(), C4Draw::DEFAULT_MESSAGE_COLOR, ARight);
	}
}

void C4MenuItem::MouseInput(C4GUI::CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam)
{
	// clicky clicky!
	if (iButton == C4MC_Button_LeftDown)
	{
		// button down: Init drag only; Enter selection only by button up
		if (IsDragElement())
			StartDragging(rMouse, iX, iY, dwKeyParam);
	}
	else if (iButton == C4MC_Button_LeftUp)
	{
		// left-click performed
		pMenu->UserEnter(::MouseControl.GetPlayer(), this, false);
		return;
	}
	else if (iButton == C4MC_Button_RightUp)
	{
		// right-up: Alternative enter command
		pMenu->UserEnter(::MouseControl.GetPlayer(), this, true);
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

C4Menu::C4Menu() : C4GUI::Dialog(100, 100, nullptr, true) // will be re-adjusted later
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
	Extra=C4MN_Extra_None;
	ExtraData=0;
	TimeOnSelection=0;
	Identification=0;
	LocationSet=false;
	Columns=Lines=0;
	Alignment= C4MN_Align_Right | C4MN_Align_Bottom;
	VisibleCount=0;
	fHasPortrait = false;
	fTextProgressing = false;
	fEqualIconItemHeight = false;
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
	Extra=iExtra; ExtraData=iExtraData;
	Identification=iId;
	if (*Caption || iStyle == C4MN_Style_Dialog) SetTitle(Caption, HasMouse()); else SetTitle(" ", HasMouse());
	if (pTitle) pTitle->SetIcon(Symbol);
	Style=iStyle & C4MN_Style_BaseMask;
	// Menus are synchronous to allow COM_MenuUp/Down to be converted to movements at the clients
	if (Style == C4MN_Style_Normal)
		Columns = 5;
	else
		// in reality, Dialog menus may have two coloumns (first for the portrait)
		// however, they are not uniformly spaced and stuff; so they are better just ignored and handled by the drawing routine
		Columns=1;
	if (iStyle & C4MN_Style_EqualItemHeight) SetEqualItemHeight(true);
	if (Style == C4MN_Style_Dialog) Alignment = C4MN_Align_Top;
	::pGUI->ShowDialog(this, false);
	fTextProgressing = false;
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

bool C4Menu::Add(const char *szCaption, C4Object* pGfxObj, const char *szCommand,
                 int32_t iCount, C4Object *pObject, const char *szInfoCaption,
                 C4ID idID, const char *szCommand2, bool fOwnValue, int32_t iValue, bool fIsSelectable)
{
	if (!IsActive()) return false;
	// Create new menu item
	C4MenuItem *pNew = new C4MenuItem(this, ItemCount, szCaption,szCommand,iCount,pObject,szInfoCaption,idID,szCommand2,fOwnValue,iValue,Style,fIsSelectable);
	// Set Symbol
	pNew->SetGraphics(pGfxObj);
	// Add
	return AddItem(pNew, szCaption, szCommand, iCount, pObject, szInfoCaption, idID, szCommand2, fOwnValue, iValue, fIsSelectable);
}

bool C4Menu::Add(const char *szCaption, C4DefGraphics* pGfx, const char *szCommand,
                 int32_t iCount, C4Object *pObject, const char *szInfoCaption,
                 C4ID idID, const char *szCommand2, bool fOwnValue, int32_t iValue, bool fIsSelectable)
{
	if (!IsActive()) return false;
	// Create new menu item
	C4MenuItem *pNew = new C4MenuItem(this, ItemCount, szCaption,szCommand,iCount,pObject,szInfoCaption,idID,szCommand2,fOwnValue,iValue,Style,fIsSelectable);
	// Set Symbol
	pNew->SetGraphics(pGfx);
	// Add
	return AddItem(pNew, szCaption, szCommand, iCount, pObject, szInfoCaption, idID, szCommand2, fOwnValue, iValue, fIsSelectable);
}

bool C4Menu::AddItem(C4MenuItem *pNew, const char *szCaption, const char *szCommand,
                     int32_t iCount, C4Object *pObject, const char *szInfoCaption,
                     C4ID idID, const char *szCommand2, bool fOwnValue, int32_t iValue, bool fIsSelectable)
{
	if (DEBUGREC_MENU && Config.General.DebugRec)
		if (pObject)
		{
			C4RCMenuAdd rc = { pObject ? pObject->Number : -1, iCount, idID, fOwnValue, iValue, fIsSelectable };
			AddDbgRec(RCT_MenuAdd, &rc, sizeof(C4RCMenuAdd));
			if (szCommand) AddDbgRec(RCT_MenuAddC, szCommand, strlen(szCommand)+1);
			if (szCommand2) AddDbgRec(RCT_MenuAddC, szCommand2, strlen(szCommand2)+1);
		}
	// Add it to the list
	pClientWindow->AddElement(pNew);
	// first menuitem is portrait, if it does not have text but a facet
	if (!ItemCount && (!szCaption || !*szCaption))
		fHasPortrait = true;
	// Item count
	ItemCount++;
	// set new item size
	if (!pClientWindow->IsFrozen()) UpdateElementPositions();
	// Init selection if not frozen
	if (Selection==-1 && fIsSelectable && !pClientWindow->IsFrozen()) SetSelection(ItemCount-1, false, false);
	// initial progress
	if (fTextProgressing) pNew->TextDisplayProgress = 0;
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
	case COM_MenuEnterAll: Enter(true); break;
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
	case COM_MenuSelect:
		if (ItemCount)
			SetSelection(iData & (~C4MN_AdjustPosition), !!(iData & C4MN_AdjustPosition), true);
		break;
	case COM_MenuShowText:
		SetTextProgress(-1, false);
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

bool C4Menu::Enter(bool fRight)
{
	// Not active
	if (!IsActive()) return false;
	if (Style==C4MN_Style_Info) return false;
	// Get selected item
	C4MenuItem *pItem = GetSelectedItem();
	if (!pItem)
	{
		// okay for dialogs: Just close them
		if (Style == C4MN_Style_Dialog) TryClose(false, true);
		return true;
	}
	// Copy command to buffer (menu might be cleared)
	char szCommand[_MAX_FNAME_LEN+30];
	SCopy(pItem->Command,szCommand);
	if (fRight && pItem->Command2[0]) SCopy(pItem->Command2,szCommand);

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
		// determine newly selected item
		C4MenuItem *pNewSel = GetItem(iNewSel);
		// nothing selectable
		if (!pNewSel || !pNewSel->IsSelectable) continue;
		// got something: go select it
		break;
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
	if ((iSelection==-1 && !ItemCount) || (pNewSel && pNewSel->IsSelectable))
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
	case C4MN_Style_Info:
	{
		// calculate size from a default size determined by a window width of C4MN_DefInfoWdt
		int32_t iWdt,iHgt,iLargestTextWdt;
		::GraphicsResource.FontRegular.GetTextExtent(Caption,iWdt,iHgt, true);
		iLargestTextWdt = iWdt + 2 * C4MN_SymbolSize + C4MN_FrameWidth;
		ItemWidth=std::min<int>(cgoArea.Wdt - 2*C4MN_FrameWidth, std::max(iLargestTextWdt, C4MN_DefInfoWdt));
		ItemHeight=0;
		StdStrBuf sText;
		C4MenuItem *pItem;
		for (int32_t i=0; (pItem=GetItem(i)); ++i)
		{
			::GraphicsResource.FontRegular.BreakMessage(pItem->InfoCaption, ItemWidth, &sText, true);
			::GraphicsResource.FontRegular.GetTextExtent(sText.getData(),iWdt,iHgt, true);
			assert(iWdt <= ItemWidth);
			ItemWidth=std::max(ItemWidth,iWdt); ItemHeight=std::max(ItemHeight,iHgt);
			iLargestTextWdt = std::max(iLargestTextWdt, iWdt);
		}
		// although width calculation is done from C4MN_DefInfoWdt, this may be too large for some very tiny info windows
		// so make sure no space is wasted
		ItemWidth = std::min(ItemWidth, iLargestTextWdt);
		// Add some extra space so text doesn't touch right border frame...
		ItemWidth += 3;
		// Now add some space to show the picture on the left
		ItemWidth += C4PictureSize;
		// And set a minimum item height (again, for the picture)
		ItemHeight = std::max<int>(ItemHeight, C4PictureSize);
		break;
	}

	case C4MN_Style_Dialog:
	{
		// dialog window: Item width is whole dialog, portrait subtracted if any
		// Item height varies
		int32_t iWdt,iHgt;
		::GraphicsResource.FontRegular.GetTextExtent(Caption,iWdt,iHgt, true);
		ItemWidth=std::min<int>(cgoArea.Wdt - 2*C4MN_FrameWidth, std::max<int>(iWdt + 2 * C4MN_SymbolSize + C4MN_FrameWidth, C4MN_DlgWdt));
		ItemHeight=iHgt; // Items may be multiline and higher
		if (HasPortrait())
		{
			// subtract portrait only if this would not make the dialog too small
			if (ItemWidth > C4MN_DlgPortraitWdt*2 && cgoArea.Hgt > cgoArea.Wdt)
				ItemWidth = std::max<int>(ItemWidth - C4MN_DlgPortraitWdt - C4MN_DlgPortraitIndent, 40);
		}
	}
	}

	int DisplayedItemCount = ItemCount - HasPortrait();
	if (Style == C4MN_Style_Dialog)
		Lines = C4MN_DlgLines;
	else
		Lines = DisplayedItemCount/Columns+std::min<int32_t>(DisplayedItemCount%Columns,1);
	// adjust by max. height
	Lines=std::max<int32_t>(std::min<int32_t>((cgoArea.Hgt-100)/std::max<int32_t>(ItemHeight,1),Lines),1);

	InitSize();
	int32_t X,Y;
	if (Alignment & C4MN_Align_Free)
	{
		X = rcBounds.x;
		Y = rcBounds.y;
	}
	else
	{
		X = (cgoArea.Wdt - rcBounds.Wdt)/2;
		Y = (cgoArea.Hgt - rcBounds.Hgt)/2;
	}
	// Alignment
	if (Alignment & C4MN_Align_Left) X=C4SymbolSize;
	if (Alignment & C4MN_Align_Right) X=cgoArea.Wdt-2*C4SymbolSize-rcBounds.Wdt;
	if (Alignment & C4MN_Align_Top) Y=C4SymbolSize;
	if (Alignment & C4MN_Align_Bottom) Y=cgoArea.Hgt-C4SymbolSize-rcBounds.Hgt;
	if (Alignment & C4MN_Align_Free) { X=Clamp<int32_t>(X,0,cgoArea.Wdt-rcBounds.Wdt); Y=Clamp<int32_t>(Y,0,cgoArea.Hgt-rcBounds.Hgt); }
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
	VisibleCount = Columns*Lines;
	bool fBarNeeded;
	if (HasPortrait()) Width += C4MN_DlgPortraitWdt + C4MN_DlgPortraitIndent;
	// dialogs have auto-enlarge vertically
	if (pLast && Style == C4MN_Style_Dialog)
	{
		Height = std::max<int>(Height, pLast->GetBounds().y + pLast->GetBounds().Hgt + C4MN_DlgLineMargin);
		fBarNeeded = false;
	}
	else
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
	if (!fTextProgressing) ++TimeOnSelection;
	if (TimeOnSelection >= C4MN_InfoCaption_Delay)
		if (Style != C4MN_Style_Info) // No tooltips in info menus - doesn't make any sense...
			if (!::Control.isReplay())
				if (!::pGUI->Mouse.IsActiveInput())
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


void C4Menu::DrawElement(C4TargetFacet &cgo)
{
	// inherited (background)
	typedef C4GUI::Dialog ParentClass;
	ParentClass::DrawElement(cgo);

	// Get selected item id
	C4ID idSelected; C4MenuItem *pItem;
	if ((pItem = GetSelectedItem())) idSelected = pItem->id; else idSelected = C4ID::None;
	C4Def *pDef = C4Id2Def(idSelected);
	// Get item value
	int32_t iValue = 0;
	if (pDef)
	{
		if (pItem && pItem->fOwnValue)
			iValue = pItem->iValue;
		else
			iValue = pDef->GetValue(nullptr, NO_OWNER);
	}

	// Store and clear global clipper
//  int32_t iX1,iY1,iX2,iY2;
//  pDraw->GetPrimaryClipper(iX1,iY1,iX2,iY2);
//  pDraw->SubPrimaryClipper(rcBounds.x, rcBounds.y, rcBounds.x+rcBounds.Wdt-1, rcBounds.y+rcBounds.Hgt-1);

	C4Facet cgoExtra(cgo.Surface, cgo.TargetX+rcBounds.x+1, cgo.TargetY+rcBounds.y+rcBounds.Hgt-C4MN_SymbolSize-1, rcBounds.Wdt-2, C4MN_SymbolSize);

	// Draw bar divider
	if (Extra)
	{
		DrawFrame(cgoExtra.Surface, cgoExtra.X-1, cgoExtra.Y-1, cgoExtra.Wdt+1, cgoExtra.Hgt+1);
	}

	// Draw specified extra
	switch (Extra)
	{
	case C4MN_Extra_Value:
	{
		if (pDef) ::GraphicsResource.fctWealth.DrawValue(cgoExtra,iValue,0,0,C4FCT_Right);
	}
	break;
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
	// text progress
	if (fTextProgressing)
		SetTextProgress(+1, true);
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
	if (!pSelection || !pSelection->IsSelectable)
	{
		// set to new first valid selection: Downwards first
		iSel = Selection;
		while (--iSel>=0)
			if ((pSelection = GetItem(iSel)))
				if (pSelection->IsSelectable)
					break;
		// no success: upwards then
		if (iSel<0)
			for (iSel=Selection+1; (pSelection = GetItem(iSel)); ++iSel)
				if (pSelection->IsSelectable)
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

bool C4Menu::SetTextProgress(int32_t iToProgress, bool fAdd)
{
	// menu active at all?
	if (!IsActive()) return false;
	// set: enable or disable progress?
	if (!fAdd)
		fTextProgressing = (iToProgress >= 0);
	else
	{
		// add: Does not enable progressing
		if (!fTextProgressing) return false;
	}
	// update menu items
	C4MenuItem *pItem;
	bool fAnyItemUnfinished = false;
	for (int32_t i=HasPortrait(); (pItem = GetItem(i)); ++i)
	{
		// disabled progress: set all progresses to shown
		if (!fTextProgressing)
		{
			pItem->TextDisplayProgress = -1;
			continue;
		}
		// do progress on item, if any is left
		// this call automatically reduces iToProgress as it's used up
		if (!fAdd) pItem->TextDisplayProgress = 0;
		if (iToProgress) pItem->DoTextProgress(iToProgress);
		if (pItem->TextDisplayProgress > -1) fAnyItemUnfinished = true;
	}
	// if that progress showed everything already, mark as not progressing
	fTextProgressing = fAnyItemUnfinished;
	// done, success
	return true;
}

C4Viewport *C4Menu::GetViewport()
{
	// ask all viewports
	for (C4Viewport *pVP = ::Viewports.GetFirstViewport(); pVP; pVP = pVP->GetNext())
		if (pVP->IsViewportMenu(this))
			return pVP;
	// none matching
	return nullptr;
}

void C4Menu::UpdateElementPositions()
{
	// only if already shown and made visible by first drawing
	// this will postpone the call of menu initialization until it's really needed
	if (!IsVisible() || !pClientWindow) return;
	// reposition client scrolling window
	pClientWindow->SetBounds(GetContainedClientRect());
	// re-stack all list items
	int xOff, yOff = 0;
	C4MenuItem *pCurr = static_cast<C4MenuItem *>(pClientWindow->GetFirst()), *pPrev = nullptr;
	if (HasPortrait() && pCurr)
	{
		// recheck portrait
		xOff = C4MN_DlgPortraitWdt + C4MN_DlgPortraitIndent;
		C4Facet &fctPortrait = pCurr->Symbol;
		C4Rect rcPortraitBounds(0,0, C4MN_DlgPortraitWdt + C4MN_DlgPortraitIndent, fctPortrait.Hgt * C4MN_DlgPortraitWdt / std::max<int>(fctPortrait.Wdt, 1));
		if (pCurr->GetBounds() != rcPortraitBounds)
		{
			pCurr->GetBounds() = rcPortraitBounds;
			pCurr->UpdateOwnPos();
		}
		pCurr = static_cast<C4MenuItem *>(pCurr->GetNext());
	}
	else xOff = 0;
	// recheck list items
	int32_t iMaxDlgOptionHeight = -1;
	int32_t iIndex = 0; C4Rect rcNewBounds(0,0,ItemWidth,ItemHeight);
	C4MenuItem *pFirstStack = pCurr, *pNext = pFirstStack;
	while ((pCurr = pNext))
	{
		pNext = static_cast<C4MenuItem *>(pCurr->GetNext());
		if (Style == C4MN_Style_Dialog)
		{
			// y-margin always, except between options
			if (!pPrev || (!pPrev->IsSelectable || !pCurr->IsSelectable)) yOff += C4MN_DlgLineMargin; else yOff += C4MN_DlgOptionLineMargin;
			// determine item height.
			StdStrBuf sText;
			int32_t iAssumedItemHeight = ::GraphicsResource.FontRegular.GetLineHeight();
			int32_t iWdt, iAvailWdt = ItemWidth, iSymWdt;
			for (;;)
			{
				iSymWdt = std::min<int32_t>(pCurr->GetSymbolWidth(iAssumedItemHeight), iAvailWdt/2);
				iAvailWdt = ItemWidth - iSymWdt;
				::GraphicsResource.FontRegular.BreakMessage(pCurr->Caption, iAvailWdt, &sText, true);
				::GraphicsResource.FontRegular.GetTextExtent(sText.getData(),iWdt,rcNewBounds.Hgt, true);
				if (!iSymWdt || rcNewBounds.Hgt <= iAssumedItemHeight) break;
				// If there is a symbol, the symbol grows as more lines become available
				// Thus, less space is available for the text, and it might become larger
				iAssumedItemHeight = rcNewBounds.Hgt;
			}
			if (fEqualIconItemHeight && iSymWdt)
			{
				// force equal height for all symbol items
				if (iMaxDlgOptionHeight < 0)
				{
					// first selectable item inits field
					iMaxDlgOptionHeight = rcNewBounds.Hgt;
				}
				else if (rcNewBounds.Hgt <= iMaxDlgOptionHeight)
				{
					// following item height smaller or equal: Force equal
					rcNewBounds.Hgt = iMaxDlgOptionHeight;
				}
				else
				{
					// following item larger height: Need to re-stack from beginning
					iMaxDlgOptionHeight = rcNewBounds.Hgt;
					pNext = pFirstStack;
					pPrev = nullptr;
					yOff = 0;
					iIndex = 0;
					continue;
				}
			}
			assert(iWdt <= iAvailWdt);
			rcNewBounds.x = 0;
			rcNewBounds.y = yOff;
			yOff += rcNewBounds.Hgt;
		}
		else
		{
			rcNewBounds.x = (iIndex % std::max<int32_t>(Columns, 1)) * ItemWidth;
			rcNewBounds.y = (iIndex / std::max<int32_t>(Columns, 1)) * ItemHeight;
		}
		rcNewBounds.x += xOff;
		if (pCurr->GetBounds() != rcNewBounds)
		{
			pCurr->GetBounds() = rcNewBounds;
			pCurr->UpdateOwnPos();
		}
		++iIndex;
		pPrev = pCurr;
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
	SetTitle((*szCapt || Style == C4MN_Style_Dialog) ? szCapt : " ", HasMouse());
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
	// the item must be selectable
	if (!pItem || !pItem->IsSelectable) return;
	// queue or direct selection
	OnUserSelectItem(Player, pItem->iIndex);
}

void C4Menu::UserEnter(int32_t Player, C4MenuItem *pItem, bool fRight)
{
	// not if user con't control anything
	if (IsReadOnly()) return;
	// the item must be selectable
	if (!pItem || !pItem->IsSelectable) return;
	// queue or direct enter
	OnUserEnter(Player, pItem->iIndex, fRight);
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

void C4Menu::ClearPointers(C4Object *pObj)
{
	C4MenuItem *pItem;
	for (int32_t i=0; (pItem = GetItem(i)); ++i)
		pItem->ClearPointers(pObj);
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
