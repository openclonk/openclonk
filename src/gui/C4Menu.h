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

#ifndef INC_C4Menu
#define INC_C4Menu

#include "C4Id.h"
#include "C4FacetEx.h"
#include "C4Gui.h"
#include "C4IDList.h"

enum
{
	C4MN_SymbolSize   = 16,
	C4MN_FrameWidth   = 2
};
enum
{
	C4MN_Style_Normal  = 0,
	C4MN_Style_Context = 1,
};
enum
{
	C4MN_Align_Left   = 1,
	C4MN_Align_Right  = 2,
	C4MN_Align_Top    = 4,
	C4MN_Align_Bottom = 8,
};

class C4MenuItem : public C4GUI::Element
{
	friend class C4Menu;
public:
	~C4MenuItem();
protected:
	char Caption[C4MaxTitle+1];
	char Command[_MAX_FNAME+30+1];
	char InfoCaption[2*C4MaxTitle+1];
	C4FacetSurface Symbol;
	C4DefGraphics* pSymbolGraphics; // drawn instead of symbol, if non-null
	uint32_t dwSymbolClr;
	bool fSelected;  // item is selected; set by menu
	int32_t iStyle;
	class C4Menu *pMenu;
	int32_t iIndex;

private:
	int32_t GetSymbolWidth(int32_t iForHeight);

protected:
	virtual void DrawElement(C4TargetFacet &cgo); // draw menu item

	// ctor
	C4MenuItem(C4Menu *pMenu, int32_t iIndex, const char *szCaption, const char *szCommand, const char *szInfoCaption, int32_t iStyle);
	void GrabSymbol(C4FacetSurface &fctSymbol) { Symbol.GrabFrom(fctSymbol); if (Symbol.Surface) dwSymbolClr=Symbol.Surface->GetClr(); }
	void SetGraphics(C4DefGraphics* pGfx) { pSymbolGraphics = pGfx; }
	void RefSymbol(const C4Facet &fctSymbol) { Symbol.Set(fctSymbol); if (Symbol.Surface) dwSymbolClr=Symbol.Surface->GetClr(); }
	void SetSelected(bool fToVal) { fSelected = fToVal; }

public:
	const char *GetCommand() const { return Command; }

	// GUI calls
	virtual void MouseInput(class C4GUI::CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam); // input: mouse movement or buttons
	virtual void MouseEnter(class C4GUI::CMouse &rMouse); // called when mouse cursor enters element region: Select this item (deselects any other)
};

class C4Menu : public C4GUI::Dialog
{
	typedef C4GUI::Dialog BaseClass;
public:
	C4Menu();
	~C4Menu() { Clear(); }

	void Clear();
	virtual void Default();

protected:
	bool LocationSet;
	bool Permanent;
	bool NeedRefill;
	int32_t Style;
	int32_t Selection,TimeOnSelection;
	int32_t ItemCount;
	int32_t ItemWidth,ItemHeight;
	int32_t Identification;
	int32_t Columns; // sync
	int32_t Lines; // async
	int32_t Alignment;
	StdStrBuf CloseCommand; // script command that will be executed on menu close
	char Caption[C4MaxTitle+1];
	C4FacetSurface Symbol;
	C4GUI::ScrollWindow *pClientWindow; // window containing the menu items
	bool fActive; // set if menu is shown - independant of GUI to keep synchronized when there's no GUI
public:
	bool Refill();
	void Execute();
	void SetPermanent(bool fPermanent);
	void SetAlignment(int32_t iAlignment);
	int32_t GetIdentification();
	int32_t GetItemCount();
	int32_t GetPosition();
	int32_t GetSelection();
	bool IsContextMenu() { return Style == C4MN_Style_Context; }
	int32_t GetItemHeight() { return ItemHeight; }
	C4MenuItem* GetSelectedItem();
	C4MenuItem* GetItem(int32_t iIndex);
	virtual C4Object *GetParentObject() { return NULL; }
	bool MoveSelection(int32_t iBy, bool fAdjustPosition, bool fDoCalls);
	bool SetSelection(int32_t iSelection, bool fAdjustPosition, bool fDoCalls);
	bool SetPosition(int32_t iPosition);
	void SetSize(int32_t iToWdt, int32_t iToHgt);
	bool Enter();
	bool IsActive();
	bool Control(BYTE byCom, int32_t iData);
	bool KeyControl(BYTE byCom); // direct keyboard callback
	bool AddRefSym(const char *szCaption, const C4Facet &fctSymbol, const char *szCommand, const char *szInfoCaption=NULL);
	bool Add(const char *szCaption, C4FacetSurface &fctSymbol, const char *szCommand, const char *szInfoCaption=NULL);
	void ClearItems();
	void ResetLocation() { LocationSet = false; }
	bool SetLocation(int32_t iX, int32_t iY); // set location relative to user viewport
	bool TryClose(bool fOK, bool fControl);
	void SetCloseCommand(const char *strCommand);

#ifdef _DEBUG
	void AssertSurfaceNotUsed(C4Surface *sfc);
#endif

private:
	bool AddItem(C4MenuItem *pNew, const char *szCaption, const char *szCommand, const char *szInfoCaption);
	bool InitMenu(const char *szEmpty, int32_t iId, int32_t iStyle);
protected:
	bool DoInitRefSym(const C4Facet &fctSymbol, const char *szEmpty, int32_t iId=0, int32_t iStyle=C4MN_Style_Normal);
	bool DoInit(C4FacetSurface &fctSymbol, const char *szEmpty, int32_t iId=0, int32_t iStyle=C4MN_Style_Normal);
	void DrawBuffer(C4Facet &cgo);
	void AdjustSelection();
	void AdjustPosition();
	bool CheckBuffer();
	bool RefillInternal();
	void DrawButton(C4Facet &cgo);
	void DrawScrollbar(C4Facet &cgo, int32_t iTotal, int32_t iVisible, int32_t iPosition);
	void DrawFrame(C4Surface * sfcSurface, int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt);
	void InitLocation(C4Facet &cgo);
	void InitSize();
	void UpdateScrollBar(); // call InitSize if a scroll bar is needed but not present or vice vera
	void UserSelectItem(int32_t Player, C4MenuItem *pItem); // select item (direct) or do control (object menus)
	void UserEnter(int32_t Player, C4MenuItem *pItem); // enter on an item
	bool HasMouse(); // returns whether the controlling player has mouse control

	virtual bool DoRefillInternal(bool &rfRefilled) { return true; };
	virtual bool MenuCommand(const char *szCommand, bool fIsCloseCommand) { return true; }
	virtual void OnSelectionChanged(int32_t iNewSelection) {} // do object callbacks if selection changed in user menus
	virtual bool IsCloseDenied() { return false; } // do MenuQueryCancel-callbacks for user menus
	virtual void OnUserSelectItem(int32_t Player, int32_t iIndex) {}
	virtual void OnUserEnter(int32_t Player, int32_t iIndex) {}
	virtual void OnUserClose() {};
	virtual bool IsReadOnly() { return false; } // determine whether the menu is just viewed by an observer, and should not issue any calls
	virtual int32_t GetControllingPlayer() { return NO_OWNER; }

	virtual const char *GetID() { return 0; } // no ID needed, because it's a viewport dlg

protected:
	// C4GUI
	virtual C4Viewport *GetViewport();              // return associated viewport
	virtual bool IsExternalDrawDialog() { return true; } // drawn by viewport drawing proc
	virtual bool IsMouseControlled() { return false; }
	virtual void UpdateOwnPos();
	void UpdateElementPositions();            // reposition list items so they are stacked vertically
	virtual int32_t GetZOrdering() { return -1; }
	virtual void Draw(C4TargetFacet &cgo);
	virtual bool IsOwnPtrElement() { return true; }
	virtual void UserClose(bool fOK);

	// bottom area needed for extra info
	virtual int32_t GetMarginBottom() { return C4MN_FrameWidth + BaseClass::GetMarginBottom(); }
	virtual int32_t GetMarginLeft() { return C4MN_FrameWidth + BaseClass::GetMarginLeft(); }
	virtual int32_t GetMarginRight() { return C4MN_FrameWidth + BaseClass::GetMarginRight(); }

	friend class C4Viewport; // for drawing
	friend class C4MenuItem;
};

#endif
