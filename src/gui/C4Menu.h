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

#ifndef INC_C4Menu
#define INC_C4Menu

#include "graphics/C4FacetEx.h"
#include "gui/C4Gui.h"
#include "object/C4Id.h"
#include "object/C4IDList.h"

enum
{
	C4MN_SymbolSize   = 16,
	C4MN_FrameWidth   = 2
};
enum
{
	C4MN_Style_Normal  = 0,
	C4MN_Style_Context = 1,
	C4MN_Style_Info    = 2,
	C4MN_Style_Dialog  = 3,
	C4MN_Style_BaseMask = 127,
	C4MN_Style_EqualItemHeight = 128
};
enum
{
	C4MN_Extra_None       = 0,
	C4MN_Extra_Value      = 2,
	C4MN_Extra_Info       = 4,
};
enum
{
	C4MN_Align_Left   = 1,
	C4MN_Align_Right  = 2,
	C4MN_Align_Top    = 4,
	C4MN_Align_Bottom = 8,
	C4MN_Align_Free   = 16
};
enum
{
	C4MN_Item_NoCount = 12345678
};
enum
{
	C4MN_AdjustPosition = 1<<31
};

class C4MenuItem : public C4GUI::Element
{
	friend class C4Menu;
public:
	~C4MenuItem() override;
protected:
	char Caption[C4MaxTitle+1];
	char Command[_MAX_FNAME_LEN+30];
	char Command2[_MAX_FNAME_LEN+30];
	char InfoCaption[2*C4MaxTitle+1];
	int32_t Count;
	C4ID id;
	C4Object *Object;
	C4FacetSurface Symbol;
	C4Object* pSymbolObj; // drawn instead of symbol, if non-null
	C4DefGraphics* pSymbolGraphics; // drawn instead of symbol, if non-null
	uint32_t dwSymbolClr;
	bool fOwnValue;   // if set, a specific value is to be shown
	int32_t iValue;       // specific value to be shown
	bool fSelected;  // item is selected; set by menu
	int32_t iStyle;
	class C4Menu *pMenu;
	int32_t iIndex;
	bool IsSelectable;
	int32_t TextDisplayProgress; // dialog menus only: Amount of text which is to be displayed already (-1 for everything)

private:
	bool IsDragElement();
	int32_t GetSymbolWidth(int32_t iForHeight);

protected:
	void DrawElement(C4TargetFacet &cgo) override; // draw menu item

	// ctor
	C4MenuItem(C4Menu *pMenu, int32_t iIndex, const char *szCaption, const char *szCommand,
	           int32_t iCount, C4Object *pObject, const char *szInfoCaption,
	           C4ID idID, const char *szCommand2, bool fOwnValue, int32_t iValue, int32_t iStyle, bool fIsSelectable);
	void GrabSymbol(C4FacetSurface &fctSymbol) { Symbol.GrabFrom(fctSymbol); if (Symbol.Surface) dwSymbolClr=Symbol.Surface->GetClr(); }
	void SetGraphics(C4Object* pObj) { pSymbolObj = pObj; }
	void SetGraphics(C4DefGraphics* pGfx) { pSymbolGraphics = pGfx; }
	void RefSymbol(const C4Facet &fctSymbol) { Symbol.Set(fctSymbol); if (Symbol.Surface) dwSymbolClr=Symbol.Surface->GetClr(); }
	void SetSelected(bool fToVal) { fSelected = fToVal; }
	void DoTextProgress(int32_t &riByVal); // progress number of shown characters by given amount

public:
	C4ID GetC4ID() const { return id; }
	int32_t GetValue() const { return iValue; }
	C4Object *GetObject() const { return Object; }
	const char *GetCommand() const { return Command; }

	void ClearPointers(C4Object* pObj) { if(pObj == Object) Object = nullptr; if(pObj == pSymbolObj) pSymbolObj = nullptr; }

	// GUI calls
	void MouseInput(class C4GUI::CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam) override; // input: mouse movement or buttons
	void MouseEnter(class C4GUI::CMouse &rMouse) override; // called when mouse cursor enters element region: Select this item (deselects any other)
};

class C4Menu : public C4GUI::Dialog
{
	typedef C4GUI::Dialog BaseClass;
public:
	C4Menu();
	~C4Menu() override { Clear(); }

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
	int32_t Extra,ExtraData;
	int32_t Identification;
	int32_t Columns; // sync
	int32_t Lines; // async
	int32_t Alignment;
	int32_t VisibleCount;
	StdStrBuf CloseCommand; // script command that will be executed on menu close
	char Caption[C4MaxTitle+1];
	C4FacetSurface Symbol;
	C4GUI::ScrollWindow *pClientWindow; // window containing the menu items
	bool fHasPortrait; // if set, first menu item is used at a portrait at topleft of menu
	bool fTextProgressing; // if true, text is being shown progressively (dialog menus)
	bool fEqualIconItemHeight; // for dialog menus only: If set, all options with an icon are forced to have the same height
	bool fActive; // set if menu is shown - independant of GUI to keep synchronized when there's no GUI
public:
	void ClearPointers(C4Object *pObj);
	bool Refill();
	void Execute();
	void SetPermanent(bool fPermanent);
	void SetAlignment(int32_t iAlignment);
	int32_t GetIdentification();
	int32_t GetItemCount();
	int32_t GetPosition();
	int32_t GetSelection();
	bool IsContextMenu() { return Style == C4MN_Style_Context; }
	int GetSymbolSize() { return (Style == C4MN_Style_Dialog) ? 64 : C4SymbolSize; }
	int32_t GetItemHeight() { return ItemHeight; }
	C4MenuItem* GetSelectedItem();
	C4MenuItem* GetItem(int32_t iIndex);
	virtual C4Object *GetParentObject() { return nullptr; }
	bool MoveSelection(int32_t iBy, bool fAdjustPosition, bool fDoCalls);
	bool SetSelection(int32_t iSelection, bool fAdjustPosition, bool fDoCalls);
	bool SetPosition(int32_t iPosition);
	void SetSize(int32_t iToWdt, int32_t iToHgt);
	bool Enter(bool fRight=false);
	bool IsActive();
	bool Control(BYTE byCom, int32_t iData);
	bool KeyControl(BYTE byCom); // direct keyboard callback
	bool AddRefSym(const char *szCaption, const C4Facet &fctSymbol, const char *szCommand,
	               int32_t iCount=C4MN_Item_NoCount, C4Object *pObject=nullptr,
	               const char *szInfoCaption=nullptr,
	               C4ID idID=C4ID::None, const char *szCommand2=nullptr, bool fOwnValue=false, int32_t iValue=0, bool fIsSelectable=true);
	bool Add(const char *szCaption, C4FacetSurface &fctSymbol, const char *szCommand,
	         int32_t iCount=C4MN_Item_NoCount, C4Object *pObject=nullptr,
	         const char *szInfoCaption=nullptr,
	         C4ID idID=C4ID::None, const char *szCommand2=nullptr, bool fOwnValue=false, int32_t iValue=0, bool fIsSelectable=true);
	bool Add(const char *szCaption, C4Object* pGfxObj, const char *szCommand,
	         int32_t iCount=C4MN_Item_NoCount, C4Object *pObject=nullptr,
	         const char *szInfoCaption=nullptr,
	         C4ID idID=C4ID::None, const char *szCommand2=nullptr, bool fOwnValue=false, int32_t iValue=0, bool fIsSelectable=true);
	bool Add(const char *szCaption, C4DefGraphics* pGfx, const char *szCommand,
	         int32_t iCount=C4MN_Item_NoCount, C4Object *pObject=nullptr,
	         const char *szInfoCaption=nullptr,
	         C4ID idID=C4ID::None, const char *szCommand2=nullptr, bool fOwnValue=false, int32_t iValue=0, bool fIsSelectable=true);
	void ClearItems();
	void ResetLocation() { LocationSet = false; }
	bool SetLocation(int32_t iX, int32_t iY); // set location relative to user viewport
	bool SetTextProgress(int32_t iToProgress, bool fAdd); // enable/disable progressive text display and set starting pos
	void SetEqualItemHeight(bool fToVal) { fEqualIconItemHeight = fToVal; } // enable/disable equal item heights
	bool TryClose(bool fOK, bool fControl);
	void SetCloseCommand(const char *strCommand);
	bool IsTextProgressing() const { return fTextProgressing; }

#ifdef _DEBUG
	void AssertSurfaceNotUsed(C4Surface *sfc);
#endif

private:
	bool AddItem(C4MenuItem *pNew, const char *szCaption, const char *szCommand,
	             int32_t iCount, C4Object *pObject, const char *szInfoCaption,
	             C4ID idID, const char *szCommand2, bool fOwnValue, int32_t iValue, bool fIsSelectable);
	bool InitMenu(const char *szEmpty, int32_t iExtra, int32_t iExtraData, int32_t iId, int32_t iStyle);
protected:
	bool DoInitRefSym(const C4Facet &fctSymbol, const char *szEmpty, int32_t iExtra=C4MN_Extra_None, int32_t iExtraData=0, int32_t iId=0, int32_t iStyle=C4MN_Style_Normal);
	bool DoInit(C4FacetSurface &fctSymbol, const char *szEmpty, int32_t iExtra=C4MN_Extra_None, int32_t iExtraData=0, int32_t iId=0, int32_t iStyle=C4MN_Style_Normal);
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
	void UserEnter(int32_t Player, C4MenuItem *pItem, bool fRight); // enter on an item
	bool HasMouse(); // returns whether the controlling player has mouse control

	virtual bool DoRefillInternal(bool &rfRefilled) { return true; };
	virtual bool MenuCommand(const char *szCommand, bool fIsCloseCommand) { return true; }
	virtual void OnSelectionChanged(int32_t iNewSelection) {} // do object callbacks if selection changed in user menus
	virtual bool IsCloseDenied() { return false; } // do MenuQueryCancel-callbacks for user menus
	virtual void OnUserSelectItem(int32_t Player, int32_t iIndex) {}
	virtual void OnUserEnter(int32_t Player, int32_t iIndex, bool fRight) {}
	virtual void OnUserClose() {};
	virtual bool IsReadOnly() { return false; } // determine whether the menu is just viewed by an observer, and should not issue any calls
	virtual int32_t GetControllingPlayer() { return NO_OWNER; }

	const char *GetID() override { return nullptr; } // no ID needed, because it's a viewport dlg

	bool HasPortrait() { return fHasPortrait; } // dialog menus only: Whether a portrait is shown in the topleft

protected:
	// C4GUI
	C4Viewport *GetViewport() override;              // return associated viewport
	bool IsExternalDrawDialog() override { return true; } // drawn by viewport drawing proc
	bool IsMouseControlled() override { return false; }
	void UpdateOwnPos() override;
	void UpdateElementPositions();            // reposition list items so they are stacked vertically
	int32_t GetZOrdering() override { return -1; }
	void Draw(C4TargetFacet &cgo) override;
	void DrawElement(C4TargetFacet &cgo) override; // draw menu
	bool IsOwnPtrElement() override { return true; }
	void UserClose(bool fOK) override;

	// bottom area needed for extra info
	int32_t GetMarginBottom() override { return ((Extra) ? C4MN_SymbolSize : 0) + C4MN_FrameWidth + BaseClass::GetMarginBottom(); }
	int32_t GetMarginLeft() override { return C4MN_FrameWidth + BaseClass::GetMarginLeft(); }
	int32_t GetMarginRight() override { return C4MN_FrameWidth + BaseClass::GetMarginRight(); }

	friend class C4Viewport; // for drawing
	friend class C4MenuItem;
};

#endif
