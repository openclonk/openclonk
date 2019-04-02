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

/* Object Character Flag logic */

#include "C4Include.h"
#include "C4ForbidLibraryCompilation.h"
#include "object/C4Object.h"

#include "control/C4Record.h"
#include "game/C4Physics.h"
#include "landscape/C4Landscape.h"
#include "object/C4Def.h"


void C4Object::SetOCF()
{
	C4PropList* pActionDef = GetAction();
	uint32_t dwOCFOld = OCF;
	// Update the object character flag according to the object's current situation
	C4Real cspeed=GetSpeed();
#ifdef _DEBUG
	if (Contained && !C4PropListNumbered::CheckPropList(Contained))
		{ LogF("Warning: contained in wild object %p!", static_cast<void*>(Contained)); }
	else if (Contained && !Contained->Status)
		{ LogF("Warning: contained in deleted object (#%d) (%s)!", Contained->Number, Contained->GetName()); }
#endif
	// OCF_Normal: The OCF is never zero
	OCF=OCF_Normal;
	// OCF_Construct: Can be built outside
	if (Def->Constructable && (Con<FullCon)
	    && (fix_r==Fix0) && !OnFire)
		OCF|=OCF_Construct;
	// OCF_Grab: Can be pushed
	if (GetPropertyInt(P_Touchable))
		OCF|=OCF_Grab;
	// OCF_Carryable: Can be picked up
	if (GetPropertyInt(P_Collectible))
		OCF|=OCF_Carryable;
	// OCF_OnFire: Is burning
	if (OnFire)
		OCF|=OCF_OnFire;
	// OCF_Inflammable: Is not burning and is inflammable
	if (!OnFire && GetPropertyInt(P_ContactIncinerate) > 0)
		OCF|=OCF_Inflammable;
	// OCF_FullCon: Is fully completed/grown
	if (Con>=FullCon)
		OCF|=OCF_FullCon;
	// OCF_Rotate: Can be rotated
	if (Def->Rotateable)
		// Don't rotate minimum (invisible) construction sites
		if (Con>100)
			OCF|=OCF_Rotate;
	// OCF_Exclusive: No action through this, no construction in front of this
	if (Def->Exclusive)
		OCF|=OCF_Exclusive;
	// OCF_Entrance: Can currently be entered/activated
	if ((Def->Entrance.Wdt>0) && (Def->Entrance.Hgt>0))
		if ((OCF & OCF_FullCon) && ((Def->RotatedEntrance == 1) || (GetR() <= Def->RotatedEntrance)))
			OCF|=OCF_Entrance;
	// HitSpeeds
	if (cspeed>=HitSpeed1) OCF|=OCF_HitSpeed1;
	if (cspeed>=HitSpeed2) OCF|=OCF_HitSpeed2;
	if (cspeed>=HitSpeed3) OCF|=OCF_HitSpeed3;
	if (cspeed>=HitSpeed4) OCF|=OCF_HitSpeed4;
	// OCF_Collection
	if ((OCF & OCF_FullCon) || Def->IncompleteActivity)
		if ((Def->Collection.Wdt>0) && (Def->Collection.Hgt>0))
			if (!pActionDef || (!pActionDef->GetPropertyInt(P_ObjectDisabled)))
					OCF|=OCF_Collection;
	// OCF_Alive
	if (Alive) OCF|=OCF_Alive;
	// OCF_CrewMember
	if (Def->CrewMember)
		if (Alive)
			OCF|=OCF_CrewMember;
	// OCF_NotContained
	if (!Contained)
		OCF|=OCF_NotContained;
	// OCF_InLiquid
	if (InLiquid)
		if (!Contained)
			OCF|=OCF_InLiquid;
	// OCF_InSolid
	if (!Contained)
		if (GBackSolid(GetX(), GetY()))
			OCF|=OCF_InSolid;
	// OCF_InFree
	if (!Contained)
		if (!GBackSemiSolid(GetX(), GetY()-1))
			OCF|=OCF_InFree;
	// OCF_Available
	if (!Contained || (Contained->Def->GrabPutGet & C4D_Grab_Get) || (Contained->OCF & OCF_Entrance))
		if (!GBackSemiSolid(GetX(), GetY()-1) || (!GBackSolid(GetX(), GetY()-1) && !GBackSemiSolid(GetX(), GetY()-8)))
			OCF|=OCF_Available;
	// OCF_Container
	if ((Def->GrabPutGet & C4D_Grab_Put) || (Def->GrabPutGet & C4D_Grab_Get) || (OCF & OCF_Entrance))
		OCF|=OCF_Container;
	if (DEBUGREC_OCF && Config.General.DebugRec)
	{
		C4RCOCF rc = { dwOCFOld, OCF, false };
		AddDbgRec(RCT_OCF, &rc, sizeof(rc));
	}
}


void C4Object::UpdateOCF()
{
	C4PropList* pActionDef = GetAction();
	uint32_t dwOCFOld = OCF;
	// Update the object character flag according to the object's current situation
	C4Real cspeed=GetSpeed();
#ifdef _DEBUG
	if (Contained && !C4PropListNumbered::CheckPropList(Contained))
		{ LogF("Warning: contained in wild object %p!", static_cast<void*>(Contained)); }
	else if (Contained && !Contained->Status)
		{ LogF("Warning: contained in deleted object %p (%s)!", static_cast<void*>(Contained), Contained->GetName()); }
#endif
	// Keep the bits that only have to be updated with SetOCF (def, category, con, alive, onfire)
	OCF=OCF & (OCF_Normal | OCF_Exclusive | OCF_FullCon | OCF_Rotate | OCF_OnFire
		| OCF_Alive | OCF_CrewMember);
	// OCF_inflammable: can catch fire and is not currently burning.
	if (!OnFire && GetPropertyInt(P_ContactIncinerate) > 0)
		OCF |= OCF_Inflammable;
	// OCF_Carryable: Can be picked up
	if (GetPropertyInt(P_Collectible))
		OCF|=OCF_Carryable;
	// OCF_Grab: Can be grabbed.
	if (GetPropertyInt(P_Touchable))
		OCF |= OCF_Grab;
	// OCF_Construct: Can be built outside
	if (Def->Constructable && (Con<FullCon)
	    && (fix_r == Fix0) && !OnFire)
		OCF|=OCF_Construct;
	// OCF_Entrance: Can currently be entered/activated
	if ((Def->Entrance.Wdt>0) && (Def->Entrance.Hgt>0))
		if ((OCF & OCF_FullCon) && ((Def->RotatedEntrance == 1) || (GetR() <= Def->RotatedEntrance)))
			OCF|=OCF_Entrance;
	// HitSpeeds
	if (cspeed>=HitSpeed1) OCF|=OCF_HitSpeed1;
	if (cspeed>=HitSpeed2) OCF|=OCF_HitSpeed2;
	if (cspeed>=HitSpeed3) OCF|=OCF_HitSpeed3;
	if (cspeed>=HitSpeed4) OCF|=OCF_HitSpeed4;
	// OCF_Collection
	if ((OCF & OCF_FullCon) || Def->IncompleteActivity)
		if ((Def->Collection.Wdt>0) && (Def->Collection.Hgt>0))
			if (!pActionDef || (!pActionDef->GetPropertyInt(P_ObjectDisabled)))
					OCF|=OCF_Collection;
	// OCF_NotContained
	if (!Contained)
		OCF|=OCF_NotContained;
	// OCF_InLiquid
	if (InLiquid)
		if (!Contained)
			OCF|=OCF_InLiquid;
	// OCF_InSolid
	if (!Contained)
		if (GBackSolid(GetX(), GetY()))
			OCF|=OCF_InSolid;
	// OCF_InFree
	if (!Contained)
		if (!GBackSemiSolid(GetX(), GetY()-1))
			OCF|=OCF_InFree;
	// OCF_Available
	if (!Contained || (Contained->Def->GrabPutGet & C4D_Grab_Get) || (Contained->OCF & OCF_Entrance))
		if (!GBackSemiSolid(GetX(), GetY()-1) || (!GBackSolid(GetX(), GetY()-1) && !GBackSemiSolid(GetX(), GetY()-8)))
			OCF|=OCF_Available;
	// OCF_Container
	if ((Def->GrabPutGet & C4D_Grab_Put) || (Def->GrabPutGet & C4D_Grab_Get) || (OCF & OCF_Entrance))
		OCF|=OCF_Container;
	if (DEBUGREC_OCF && Config.General.DebugRec)
	{
		C4RCOCF rc = { dwOCFOld, OCF, true };
		AddDbgRec(RCT_OCF, &rc, sizeof(rc));
	}
#ifdef _DEBUG
	DEBUGREC_OFF
	uint32_t updateOCF = OCF;
	SetOCF();
	assert (updateOCF == OCF);
	DEBUGREC_ON
#endif
}

void C4Object::GetOCFForPos(int32_t ctx, int32_t cty, DWORD &ocf) const
{
	DWORD rocf=OCF;
	// Verify entrance area OCF return
	if (rocf & OCF_Entrance)
		if (!Inside<int32_t>(cty - (GetY() + Def->Entrance.y), 0, Def->Entrance.Hgt - 1)
		    || !Inside<int32_t>(ctx - (GetX() + Def->Entrance.x), 0, Def->Entrance.Wdt - 1))
			rocf &= (~OCF_Entrance);
	// Verify collection area OCF return
	if (rocf & OCF_Collection)
		if (!Inside<int32_t>(cty - (GetY() + Def->Collection.y), 0, Def->Collection.Hgt - 1)
		    || !Inside<int32_t>(ctx - (GetX() + Def->Collection.x), 0, Def->Collection.Wdt - 1))
			rocf &= (~OCF_Collection);
	ocf=rocf;
}
