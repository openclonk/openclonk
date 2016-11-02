/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2008-2009, RedWolf Design GmbH, http://www.clonk.de/
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

#include "C4Include.h"
#include "object/C4ObjectMenu.h"

#include "control/C4Control.h"
#include "object/C4Def.h"
#include "object/C4Object.h"
#include "object/C4ObjectCom.h"
#include "player/C4Player.h"
#include "game/C4Viewport.h"
#include "gui/C4MouseControl.h"
#include "graphics/C4GraphicsResource.h"
#include "game/C4Game.h"
#include "player/C4PlayerList.h"
#include "object/C4GameObjects.h"
#include "script/C4AulExec.h"

// -----------------------------------------------------------
// C4ObjectMenu

C4ObjectMenu::C4ObjectMenu() : C4Menu()
{
	Default();
}

void C4ObjectMenu::Default()
{
	C4Menu::Default();
	eCallbackType = CB_None;
	Object = ParentObject = RefillObject = nullptr;
	RefillObjectContentsCount=0;
	UserMenu = false;
	CloseQuerying = false;
}

bool C4ObjectMenu::IsCloseDenied()
{
	// abort if menu is permanented by script; stop endless recursive calls if user opens a new menu by CloseQuerying-flag
	if (UserMenu && !CloseQuerying)
	{
		CloseQuerying = true;
		bool fResult = false;
		C4AulParSet pars(Selection, ParentObject);
		if (eCallbackType == CB_Object)
		{
			if (Object) fResult = !!Object->Call(PSF_MenuQueryCancel, &pars);
		}
		else if (eCallbackType == CB_Scenario)
			fResult = !!::GameScript.Call(PSF_MenuQueryCancel, &pars);
		CloseQuerying = false;
		if (fResult) return true;
	}
	// close OK
	return false;
}

void C4ObjectMenu::LocalInit(C4Object *pObject, bool fUserMenu)
{
	Object=pObject;
	UserMenu=fUserMenu;
	ParentObject=GetParentObject();
	if (pObject) eCallbackType = CB_Object; else eCallbackType = CB_Scenario;
}

bool C4ObjectMenu::Init(C4FacetSurface &fctSymbol, const char *szEmpty, C4Object *pObject, int32_t iExtra, int32_t iExtraData, int32_t iId, int32_t iStyle, bool fUserMenu)
{
	if (!DoInit(fctSymbol, szEmpty, iExtra, iExtraData, iId, iStyle)) return false;
	LocalInit(pObject, fUserMenu);
	return true;
}

bool C4ObjectMenu::InitRefSym(const C4TargetFacet &fctSymbol, const char *szEmpty, C4Object *pObject, int32_t iExtra, int32_t iExtraData, int32_t iId, int32_t iStyle, bool fUserMenu)
{
	if (!DoInitRefSym(fctSymbol, szEmpty, iExtra, iExtraData, iId, iStyle)) return false;
	LocalInit(pObject, fUserMenu);
	return true;
}

void C4ObjectMenu::OnSelectionChanged(int32_t iNewSelection)
{
	// do selection callback
	if (UserMenu)
	{
		C4AulParSet pars(iNewSelection, ParentObject);
		if (eCallbackType == CB_Object && Object)
			Object->Call(PSF_MenuSelection, &pars);
		else if (eCallbackType == CB_Scenario)
			::GameScript.Call(PSF_MenuSelection, &pars);
	}
}

void C4ObjectMenu::ClearPointers(C4Object *pObj)
{
	if (Object==pObj) { Object=nullptr; }
	if (ParentObject==pObj) ParentObject=nullptr; // Reason for menu close anyway.
	if (RefillObject==pObj) RefillObject=nullptr;
	C4Menu::ClearPointers(pObj);
}

C4Object* C4ObjectMenu::GetParentObject()
{
	for (C4Object *cObj : Objects)
		if (cObj->Menu == this)
			return cObj;
	return nullptr;
}

void C4ObjectMenu::SetRefillObject(C4Object *pObj)
{
	RefillObject=pObj;
	NeedRefill=true;
	Refill();
}

bool C4ObjectMenu::DoRefillInternal(bool &rfRefilled)
{
	// Variables
	C4FacetSurface fctSymbol;
	C4Object *pObj;
	char szCaption[256+1],szCommand[256+1],szCommand2[256+1];
	int32_t iCount;
	C4Def *pDef;
	C4IDList ListItems;
	C4Object *pTarget;
	C4Facet fctTarget;

	// Refill
	switch (Identification)
	{
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case C4MN_Activate:
		// Clear items
		ClearItems();
		// Refill target
		if (!(pTarget=RefillObject)) return false;
		{
			// Add target contents items
			C4ObjectListIterator iter(pTarget->Contents);
			while ((pObj = iter.GetNext(&iCount)))
			{
				pDef = pObj->Def;
				if (pDef->NoGet) continue;
				// Prefer fully constructed objects
				if (~pObj->OCF & OCF_FullCon)
				{
					// easy way: only if first concat check matches
					// this doesn't catch all possibilities, but that will rarely matter
					C4Object *pObj2=pTarget->Contents.Find(pDef, ANY_OWNER, OCF_FullCon);
					if (pObj2) if (pObj2->CanConcatPictureWith(pObj)) pObj = pObj2;
				}
				// Caption
				sprintf(szCaption,LoadResStr("IDS_MENU_ACTIVATE"),(const char *) pObj->GetName());
				// Picture
				fctSymbol.Set(fctSymbol.Surface, 0,0,C4SymbolSize,C4SymbolSize);
				pObj->Picture2Facet(fctSymbol);
				// Commands
				sprintf(szCommand,"SetCommand(\"Activate\",Object(%d))&&ExecuteCommand()",pObj->Number);
				sprintf(szCommand2,"SetCommand(\"Activate\",nil,%d,0,Object(%d),%s)&&ExecuteCommand()",pTarget->Contents.ObjectCount(pDef->id),pTarget->Number,pDef->id.ToString());
				// Add menu item
				Add(szCaption,fctSymbol,szCommand,iCount,pObj,"",pDef->id,szCommand2,true,pObj->GetValue(pTarget, NO_OWNER));
				// facet taken over (arrg!)
				fctSymbol.Default();
			}
		}
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case C4MN_Get:
	case C4MN_Contents:
		// Clear items
		ClearItems();
		// Refill target
		if (!(pTarget = RefillObject)) return false;
		{
			// Add target contents items
			C4ObjectListIterator iter(pTarget->Contents);
			while ((pObj = iter.GetNext(&iCount)))
			{
				pDef = pObj->Def;
				if (pDef->NoGet) continue;
				// Prefer fully constructed objects
				if (~pObj->OCF & OCF_FullCon)
				{
					// easy way: only if first concat check matches
					// this doesn't catch all possibilities, but that will rarely matter
					C4Object *pObj2 = pTarget->Contents.Find(pDef, ANY_OWNER, OCF_FullCon);
					if (pObj2) if (pObj2->CanConcatPictureWith(pObj)) pObj = pObj2;
				}
				// Determine whether to get or activate
				bool fGet = true;
				if (!(pObj->OCF & OCF_Carryable)) fGet = false; // not a carryable item
				if (Identification == C4MN_Contents)
				{
					if (Object && !!Object->Call(PSF_RejectCollection, &C4AulParSet(pObj->Def, pObj))) fGet = false; // collection rejected
				}
				if (!(pTarget->OCF & OCF_Entrance)) fGet = true; // target object has no entrance: cannot activate - force get
				// Caption
				sprintf(szCaption, LoadResStr(fGet ? "IDS_MENU_GET" : "IDS_MENU_ACTIVATE"), (const char *)pObj->GetName());
				// Picture
				fctSymbol.Set(fctSymbol.Surface, 0, 0, C4SymbolSize, C4SymbolSize);
				pObj->Picture2Facet(fctSymbol);
				// Primary command: get/activate single object
				sprintf(szCommand, "SetCommand(\"%s\", Object(%d)) && ExecuteCommand()", fGet ? "Get" : "Activate", pObj->Number);
				// Secondary command: get/activate all objects of the chosen type
				szCommand2[0] = 0; int32_t iAllCount;
				if ((iAllCount = pTarget->Contents.ObjectCount(pDef->id)) > 1)
					sprintf(szCommand2, "SetCommand(\"%s\", nil, %d,0, Object(%d), %s) && ExecuteCommand()", fGet ? "Get" : "Activate", iAllCount, pTarget->Number, pDef->id.ToString());
				// Add menu item (with object)
				Add(szCaption, fctSymbol, szCommand, iCount, pObj, "", pDef->id, szCommand2);
				fctSymbol.Default();
			}
		}
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	default:
		// Not an internal menu
		return true;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	}

	// Successfull internal refill
	rfRefilled = true;
	return true;
}

void C4ObjectMenu::Execute()
{
	if (!IsActive()) return;
	// Immediate refill check by RefillObject contents count check
	if (RefillObject)
		if (RefillObject->Contents.ObjectCount()!=RefillObjectContentsCount)
			{ NeedRefill=true; RefillObjectContentsCount=RefillObject->Contents.ObjectCount(); }
	// inherited
	C4Menu::Execute();
}

void C4ObjectMenu::OnUserSelectItem(int32_t Player, int32_t iIndex)
{
	// queue.... 2do
	Game.Input.Add(CID_PlrControl, new C4ControlPlayerControl(Player,Game.PlayerControlDefs.InternalCons.CON_ObjectMenuSelect,iIndex | C4MN_AdjustPosition));
}

void C4ObjectMenu::OnUserEnter(int32_t Player, int32_t iIndex, bool fRight)
{
	// object menu: Through queue 2do
	Game.Input.Add(CID_PlrControl, new C4ControlPlayerControl(Player,fRight ? Game.PlayerControlDefs.InternalCons.CON_ObjectMenuOKAll : Game.PlayerControlDefs.InternalCons.CON_ObjectMenuOK,iIndex));
}

void C4ObjectMenu::OnUserClose()
{
	// Queue 2do
	Game.Input.Add(CID_PlrControl, new C4ControlPlayerControl(::MouseControl.GetPlayer(),Game.PlayerControlDefs.InternalCons.CON_ObjectMenuCancel,0));
}

bool C4ObjectMenu::IsReadOnly()
{
	// get viewport
	C4Viewport *pVP = GetViewport();
	if (!pVP) return false;
	// is it an observer viewport?
	if (pVP->fIsNoOwnerViewport)
		// is this a synced menu?
		if (eCallbackType == CB_Object || eCallbackType == CB_Scenario)
			// then don't control it!
			return true;
	// if the player is eliminated, do not control either!
	if (!pVP->fIsNoOwnerViewport)
	{
		C4Player *pPlr = ::Players.Get(::MouseControl.GetPlayer());
		if (pPlr && pPlr->Eliminated) return true;
	}
	return false;
}

int32_t C4ObjectMenu::GetControllingPlayer()
{
	// menu controlled by object controller
	return Object ? Object->Controller : NO_OWNER;
}

bool C4ObjectMenu::MenuCommand(const char *szCommand, bool fIsCloseCommand)
{
	switch (eCallbackType)
	{
	case CB_Object:
		// Object menu
		if (Object) Object->MenuCommand(szCommand);
		break;

	case CB_Scenario:
		// Object menu with scenario script callback
		::AulExec.DirectExec(nullptr, szCommand, "MenuCommand");
		break;

	case CB_None:
		// TODO
		break;
	}

	return true;
}
