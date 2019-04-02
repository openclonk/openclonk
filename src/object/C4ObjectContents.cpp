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

/* Logic for C4Object: Contents, containers, collection */

#include "C4Include.h"
#include "C4ForbidLibraryCompilation.h"
#include "object/C4Object.h"

#include "game/C4Physics.h"
#include "object/C4Def.h"
#include "object/C4DefList.h"
#include "object/C4ObjectCom.h"
#include "object/C4ObjectMenu.h"
#include "platform/C4SoundSystem.h"


bool C4Object::Exit(int32_t iX, int32_t iY, int32_t iR, C4Real iXDir, C4Real iYDir, C4Real iRDir, bool fCalls)
{
	// 1. Exit the current container.
	// 2. Update Contents of container object and set Contained to nullptr.
	// 3. Set offset position/motion if desired.
	// 4. Call Ejection for container and Departure for object.

	// Not contained
	C4Object *pContainer=Contained;
	if (!pContainer) return false;
	// Remove object from container
	pContainer->Contents.Remove(this);
	pContainer->UpdateMass();
	pContainer->SetOCF();
	// No container
	Contained=nullptr;
	// Position/motion
	fix_x=itofix(iX); fix_y=itofix(iY);
	fix_r=itofix(iR);
	BoundsCheck(fix_x, fix_y);
	xdir=iXDir; ydir=iYDir; rdir=iRDir;
	// Misc updates
	Mobile=true;
	InLiquid=false;
	CloseMenu(true);
	UpdateFace(true);
	SetOCF();
	// Object list callback (before script callbacks, because script callbacks may enter again)
	ObjectListChangeListener.OnObjectContainerChanged(this, pContainer, nullptr);
	// Engine calls
	if (fCalls) pContainer->Call(PSF_Ejection,&C4AulParSet(this));
	if (fCalls) Call(PSF_Departure,&C4AulParSet(pContainer));
	// Success (if the obj wasn't "re-entered" by script)
	return !Contained;
}

bool C4Object::Enter(C4Object *pTarget, bool fCalls, bool fCopyMotion, bool *pfRejectCollect)
{
	// 0. Query entrance and collection
	// 1. Exit if contained.
	// 2. Set new container.
	// 3. Update Contents and mass of the new container.
	// 4. Call collection for container
	// 5. Call entrance for object.

	// No valid target or target is self
	if (!pTarget || (pTarget==this)) return false;
	// check if entrance is allowed
	if (!! Call(PSF_RejectEntrance, &C4AulParSet(pTarget))) return false;
	// check if we end up in an endless container-recursion
	for (C4Object *pCnt=pTarget->Contained; pCnt; pCnt=pCnt->Contained)
		if (pCnt==this) return false;
	// Check RejectCollect, if desired
	if (pfRejectCollect)
	{
		if (!!pTarget->Call(PSF_RejectCollection,&C4AulParSet(Def, this)))
		{
			*pfRejectCollect = true;
			return false;
		}
		*pfRejectCollect = false;
	}
	// Exit if contained
	if (Contained) if (!Exit(GetX(),GetY())) return false;
	if (Contained || !Status || !pTarget->Status) return false;
	// Failsafe updates
	if (Menu)
	{
		CloseMenu(true);
		// CloseMenu might do bad stuff
		if (Contained || !Status || !pTarget->Status) return false;
	}
	SetOCF();
	// Set container
	Contained=pTarget;
	// Enter
	if (!Contained->Contents.Add(this, C4ObjectList::stContents))
	{
		Contained=nullptr;
		return false;
	}
	// Assume that the new container controls this object, if it cannot control itself (i.e.: Alive)
	// So it can be traced back who caused the damage, if a projectile hits its target
	if (!Alive)
		Controller = pTarget->Controller;
	// Misc updates
	// motion must be copied immediately, so the position will be correct when OCF is set, and
	// OCF_Available will be set for newly bought items, even if 50/50 is solid in the landscape
	// however, the motion must be preserved sometimes to keep flags like OCF_HitSpeed upon collection
	if (fCopyMotion)
	{
		// remove any solidmask before copying the motion...
		UpdateSolidMask(false);
		CopyMotion(Contained);
	}
	SetOCF();
	UpdateFace(true);
	// Update container
	Contained->UpdateMass();
	Contained->SetOCF();
	// Object list callback (before script callbacks, because script callbacks may exit again)
	ObjectListChangeListener.OnObjectContainerChanged(this, nullptr, Contained);
	// Collection call
	if (fCalls) pTarget->Call(PSF_Collection2,&C4AulParSet(this));
	if (!Contained || !Contained->Status || !pTarget->Status) return true;
	// Entrance call
	if (fCalls) Call(PSF_Entrance,&C4AulParSet(Contained));
	if (!Contained || !Contained->Status || !pTarget->Status) return true;
	// Success
	return true;
}

C4Object* C4Object::CreateContents(C4PropList * PropList)
{
	C4Object *nobj;
	if (!(nobj=Game.CreateObject(PropList,this,Owner))) return nullptr;
	if (!nobj->Enter(this)) { nobj->AssignRemoval(); return nullptr; }
	return nobj;
}

bool C4Object::CreateContentsByList(C4IDList &idlist)
{
	int32_t cnt,cnt2;
	for (cnt=0; idlist.GetID(cnt); cnt++)
		for (cnt2=0; cnt2<idlist.GetCount(cnt); cnt2++)
			if (!CreateContents(C4Id2Def(idlist.GetID(cnt))))
				return false;
	return true;
}

bool C4Object::Collect(C4Object *pObj)
{
	// Object enter container
	bool fRejectCollect;
	if (!pObj->Enter(this, true, false, &fRejectCollect))
		return false;
	// Cancel attach (hacky)
	ObjectComCancelAttach(pObj);
	// Container Collection call
	Call(PSF_Collection,&C4AulParSet(pObj));
	// Object Hit call
	if (pObj->Status && pObj->OCF & OCF_HitSpeed1) pObj->Call(PSF_Hit);
	if (pObj->Status && pObj->OCF & OCF_HitSpeed2) pObj->Call(PSF_Hit2);
	if (pObj->Status && pObj->OCF & OCF_HitSpeed3) pObj->Call(PSF_Hit3);
	// post-copy the motion of the new container
	if (pObj->Contained == this) pObj->CopyMotion(this);
	// done, success
	return true;
}

bool C4Object::ShiftContents(bool fShiftBack, bool fDoCalls)
{
	// get current object
	C4Object *c_obj = Contents.GetObject();
	if (!c_obj) return false;
	// get next/previous
	auto it = fShiftBack ? Contents.reverse().begin() : ++Contents.begin();
	while (!it.atEnd())
	{
		auto pObj = (*it);
		// check object
		if (pObj->Status)
			if (!c_obj->CanConcatPictureWith(pObj))
			{
				// object different: shift to this
				DirectComContents(pObj, !!fDoCalls);
				return true;
			}
		// next/prev item
		it++;
	}
	return false;
}

void C4Object::DirectComContents(C4Object *pTarget, bool fDoCalls)
{
	// safety
	if (!pTarget || !pTarget->Status || pTarget->Contained != this) return;
	// Desired object already at front?
	if (Contents.GetObject() == pTarget) return;
	// select object via script?
	if (fDoCalls)
		if (Call("~ControlContents", &C4AulParSet(pTarget)))
			return;
	// default action
	if (!(Contents.ShiftContents(pTarget))) return;
	// Selection sound
	if (fDoCalls) if (!Contents.GetObject()->Call("~Selection", &C4AulParSet(this))) StartSoundEffect("Clonk::Action::Grab",false,100,this);
	// update menu with the new item in "put" entry
	if (Menu && Menu->IsActive() && Menu->IsContextMenu())
	{
		Menu->Refill();
	}
	// Done
	return;
}

int32_t C4Object::AddObjectAndContentsToArray(C4ValueArray *target_array, int32_t index)
{
	// add self, contents and child contents count recursively to value array. Return index after last added item.
	target_array->SetItem(index++, C4VObj(this));
	for (C4Object *cobj : Contents)
	{
		index = cobj->AddObjectAndContentsToArray(target_array, index);
	}
	return index;
}

void C4Object::ClearContentsAndContained(bool fDoCalls)
{
	// exit contents from container
	for (C4Object *cobj : Contents)
	{
		cobj->Exit(GetX(), GetY(), 0,Fix0,Fix0,Fix0, fDoCalls);
	}
	// remove from container *after* contents have been removed!
	if (Contained) Exit(GetX(), GetY(), 0, Fix0, Fix0, Fix0, fDoCalls);
}

void C4Object::GrabContents(C4Object *pFrom)
{
	// create a temp list of all objects and transfer it
	// this prevents nasty deadlocks caused by RejectEntrance-scripts
	C4ObjectList tmpList; tmpList.Copy(pFrom->Contents);
	for (C4Object *obj : tmpList)
		if (obj->Status)
			obj->Enter(this);
}
