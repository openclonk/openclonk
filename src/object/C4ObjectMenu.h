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
// Menus attached to objects; script created or internal
// These menus are shown to players if the target object is the current cursor

#ifndef INC_C4ObjectMenu
#define INC_C4ObjectMenu

#include "gui/C4Menu.h"

enum
{
	C4MN_None         = 0,
	/*C4MN_Construction = 1, obsolete, now reserved */
	/*C4MN_Bridge       = 2, obsolete, now reserved */
	C4MN_Take         = 3,
	C4MN_Buy          = 4,
	C4MN_Sell         = 5,
	C4MN_Activate     = 6,
	/*C4MN_Hostility    = 7, now defined in C4MainMenu*/
	/*C4MN_Surrender    = 8, obsolete, now reserved*/
	/*C4MN_Put          = 9, obsolete, now reserved*/
	/*C4MN_Magic        = 10, obsolete, now reserved*/
	/*C4MN_Main         = 12, now defined in C4MainMenu*/
	C4MN_Get          = 13,
	/*C4MN_Context      = 14, obsolete, now reserved*/
	C4MN_Info         = 15,
	/*C4MN_TeamSelection= 16, now defined in C4MainMenu */
	/*C4MN_TeamSwitch   = 17, now defined in C4MainMenu */
	C4MN_Contents     = 18
};

class C4ObjectMenu : public C4Menu
{
public:
	C4ObjectMenu();

	void Default() override;

	enum CallbackType { CB_None=0, CB_Object, CB_Scenario };
protected:
	C4Object *Object;
	C4Object *ParentObject;
	C4Object *RefillObject;
	int32_t RefillObjectContentsCount;
	CallbackType eCallbackType;
	bool UserMenu; // set for script created menus; user menus do CloseQuery and MenuSelection callbacks
	bool CloseQuerying; // recursion check for close query callback

	void LocalInit(C4Object *pObject, bool fUserMenu);

public:
	void SetRefillObject(C4Object *pObj);
	void ClearPointers(C4Object *pObj);
	bool Init(C4FacetSurface &fctSymbol, const char *szEmpty, C4Object *pObject, int32_t iExtra=C4MN_Extra_None, int32_t iExtraData=0, int32_t iId=0, int32_t iStyle=C4MN_Style_Normal, bool fUserMenu=false);
	bool InitRefSym(const C4TargetFacet &fctSymbol, const char *szEmpty, C4Object *pObject, int32_t iExtra=C4MN_Extra_None, int32_t iExtraData=0, int32_t iId=0, int32_t iStyle=C4MN_Style_Normal, bool fUserMenu=false);
	void Execute();

	C4Object* GetParentObject() override;
	bool IsCloseQuerying() const { return !!CloseQuerying; }

protected:
	bool MenuCommand(const char *szCommand, bool fIsCloseCommand) override;

	bool DoRefillInternal(bool &rfRefilled) override;
	void OnSelectionChanged(int32_t iNewSelection) override; // do object callbacks if selection changed in user menus
	bool IsCloseDenied() override; // do MenuQueryCancel-callbacks for user menus
	bool IsReadOnly() override; // determine whether the menu is just viewed by an observer, and should not issue any calls
	void OnUserSelectItem(int32_t Player, int32_t iIndex) override;
	void OnUserEnter(int32_t Player, int32_t iIndex, bool fRight) override;
	void OnUserClose() override;
	int32_t GetControllingPlayer() override;
};

#endif
