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

/* Lots of handler functions for object action */

#include "C4Include.h"
#include "object/C4ObjectCom.h"

#include "game/C4Physics.h"
#include "graphics/C4GraphicsResource.h"
#include "gui/C4GameMessage.h"
#include "landscape/C4Material.h"
#include "lib/C4Random.h"
#include "object/C4Command.h"
#include "object/C4Def.h"
#include "object/C4GameObjects.h"
#include "object/C4Object.h"
#include "object/C4ObjectMenu.h"
#include "player/C4Player.h"
#include "player/C4PlayerList.h"
#include "script/C4Effect.h"

bool SimFlightHitsLiquid(C4Real fcx, C4Real fcy, C4Real xdir, C4Real ydir);

bool ObjectActionWalk(C4Object *cObj)
{
	return cObj->SetActionByName("Walk");
}

bool ObjectActionStand(C4Object *cObj)
{
	cObj->Action.ComDir = COMD_Stop;
	return ObjectActionWalk(cObj);
}

bool ObjectActionJump(C4Object *cObj, C4Real xdir, C4Real ydir, bool fByCom)
{
	// Scripted jump?
	assert(cObj);
	C4AulParSet pars(fixtoi(xdir, 100), fixtoi(ydir, 100), fByCom);
	if (!!cObj->Call(PSF_OnActionJump, &pars))
	{
		return true;
	}
	// Hardcoded jump by action
	if (!cObj->SetActionByName("Jump"))
	{
		return false;
	}
	cObj->xdir = xdir;
	cObj->ydir = ydir;
	cObj->Mobile = true;
	// Unstick from ground, because jump command may be issued in an Action-callback,
	// where attach-values have already been determined for that frame
	cObj->Action.t_attach &= ~CNAT_Bottom;
	return true;
}

bool ObjectActionDive(C4Object *cObj, C4Real xdir, C4Real ydir)
{
	if (!cObj->SetActionByName("Dive"))
	{
		return false;
	}
	cObj->xdir = xdir;
	cObj->ydir = ydir;
	cObj->Mobile = true;
	// Unstick from ground, because jump command may be issued in an Action-callback,
	// where attach-values have already been determined for that frame
	cObj->Action.t_attach &= ~CNAT_Bottom;
	return true;
}

bool ObjectActionTumble(C4Object *cObj, int32_t dir, C4Real xdir, C4Real ydir)
{
	if (!cObj->SetActionByName("Tumble"))
	{
		return false;
	}
	cObj->SetDir(dir);
	cObj->xdir = xdir;
	cObj->ydir = ydir;
	return true;
}

bool ObjectActionGetPunched(C4Object *cObj, C4Real xdir, C4Real ydir)
{
	if (!cObj->SetActionByName("GetPunched"))
	{
		return false;
	}
	cObj->xdir = xdir;
	cObj->ydir = ydir;
	return true;
}

bool ObjectActionKneel(C4Object *cObj)
{
	return cObj->SetActionByName("KneelDown");
}

bool ObjectActionFlat(C4Object *cObj, int32_t dir)
{
	if (!cObj->SetActionByName("FlatUp"))
	{
		return false;
	}
	cObj->SetDir(dir);
	return true;
}

bool ObjectActionScale(C4Object *cObj, int32_t dir)
{
	if (!cObj->SetActionByName("Scale"))
	{
		return false;
	}
	cObj->SetDir(dir);
	return true;
}

bool ObjectActionHangle(C4Object *cObj)
{
	return cObj->SetActionByName("Hangle");
}

bool ObjectActionThrow(C4Object *cObj, C4Object *pThing)
{
	// No object specified, first from contents
	if (!pThing)
	{
		pThing = cObj->Contents.GetObject();
	}
	// Nothing to throw
	if (!pThing)
	{
		return false;
	}
	// TODO: Initialize force and direction AFTER action is set.
	// Force and direction
	C4Real pthrow = C4REAL100(cObj->GetPropertyInt(P_ThrowSpeed));
	int32_t iDir = 1;
	if (cObj->Action.Dir == DIR_Left)
	{
		iDir = -1;
	}
	// Set action
	if (!cObj->SetActionByName("Throw"))
	{
		return false;
	}
	// Exit object
	pThing->Exit(cObj->GetX(),
	             cObj->GetY() + cObj->Shape.y - 1,
	             Random(360),
	             pthrow * iDir + cObj->xdir, -pthrow + cObj->ydir, pthrow * iDir);
	// Success
	return true;
}

bool ObjectActionDig(C4Object *cObj)
{
	if (!cObj->SetActionByName("Dig"))
	{
		return false;
	}
	cObj->Action.Data = 0; // Material Dig2Object request
	return true;
}

bool ObjectActionPush(C4Object *cObj, C4Object *target)
{
	return cObj->SetActionByName("Push", target);
}

static bool CornerScaleOkay(C4Object *cObj, int32_t iRangeX, int32_t iRangeY)
{
	int32_t cty = cObj->GetY() - iRangeY;
	int32_t ctx;
	if (cObj->Action.Dir == DIR_Left)
	{
		ctx = cObj->GetX() - iRangeX;
	}
	else
	{
		ctx = cObj->GetX() + iRangeX;
	}

	cObj->ContactCheck(ctx, cty); // (Resets VtxContact & t_contact)
	return !(cObj->t_contact & CNAT_Top)
		&& !(cObj->t_contact & CNAT_Left)
		&& !(cObj->t_contact & CNAT_Right)
		&& !(cObj->t_contact & CNAT_Bottom);
}

bool ObjectActionCornerScale(C4Object *cObj)
{
	int32_t iRangeX = 1;
	int32_t iRangeY = 1;
	if (!CornerScaleOkay(cObj, iRangeX, iRangeY))
	{
		return false;
	}
	// Do corner scale
	if (!cObj->SetActionByName("KneelUp"))
	{
		cObj->SetActionByName("Walk");
	}
	cObj->xdir = 0;
	cObj->ydir = 0;
	if (cObj->Action.Dir == DIR_Left)
	{
		cObj->fix_x -= itofix(iRangeX);
	}
	else
	{
		cObj->fix_x += itofix(iRangeX);
	}
	cObj->fix_y -= itofix(iRangeY);
	return true;
}

bool ObjectComStop(C4Object *cObj)
{
	// Cease current action
	cObj->SetActionByName("Idle");
	// Action walk if possible
	return ObjectActionStand(cObj);
}

bool ObjectComGrab(C4Object *cObj, C4Object *pTarget)
{
	if (!pTarget)
	{
		return false;
	}
	if (cObj->GetProcedure() != DFA_WALK)
	{
		return false;
	}
	if (!ObjectActionPush(cObj, pTarget))
	{
		return false;
	}
	cObj->Call(PSF_Grab, &C4AulParSet(pTarget, true));
	if (pTarget->Status && cObj->Status)
	{
		pTarget->Call(PSF_Grabbed, &C4AulParSet(cObj, true));
	}
	return true;
}

bool ObjectComUnGrab(C4Object *cObj)
{
	// Only if pushing, -> stand
	if (cObj->GetProcedure() == DFA_PUSH)
	{
		C4Object *pTarget = cObj->Action.Target;
		if (ObjectActionStand(cObj))
		{
			if (!cObj->CloseMenu(false))
			{
				return false;
			}
			cObj->Call(PSF_Grab, &C4AulParSet(pTarget, false));
			// Clear action target
			cObj->Action.Target = nullptr;
			if (pTarget && pTarget->Status && cObj->Status)
			{
				pTarget->Call(PSF_Grabbed, &C4AulParSet(cObj, false));
			}
			return true;
		}
	}

	return false;
}

bool ObjectComJump(C4Object *cObj) // by ObjectComUp, ExecCMDFMoveTo, FnJump
{
	// Only if walking
	if (cObj->GetProcedure() != DFA_WALK)
	{
		return false;
	}
	// Calculate direction & forces
	C4Real TXDir = Fix0;
	C4PropList *pActionWalk = cObj->GetAction();
	C4Real iPhysicalWalk = C4REAL100(pActionWalk->GetPropertyInt(P_Speed)) * itofix(cObj->GetCon(), FullCon);
	C4Real iPhysicalJump = C4REAL100(cObj->GetPropertyInt(P_JumpSpeed)) * itofix(cObj->GetCon(), FullCon);

	if (cObj->Action.ComDir == COMD_Left || cObj->Action.ComDir == COMD_UpLeft)
	{
		TXDir = -iPhysicalWalk;
	}
	else if (cObj->Action.ComDir == COMD_Right || cObj->Action.ComDir == COMD_UpRight)
	{
		TXDir = +iPhysicalWalk;
	}
	C4Real x = cObj->fix_x;
	C4Real y = cObj->fix_y;
	// Find bottom-most vertex, correct starting position for simulation
	int32_t iBtmVtx = cObj->Shape.GetBottomVertex();
	if (iBtmVtx != -1)
	{
		x += cObj->Shape.GetVertexX(iBtmVtx);
		y += cObj->Shape.GetVertexY(iBtmVtx);
	}
	// Try dive
	if ((cObj->Shape.ContactDensity > C4M_Liquid)
	&&   SimFlightHitsLiquid(x, y, TXDir, -iPhysicalJump)
	&&  ObjectActionDive(cObj, TXDir, -iPhysicalJump))
	{
		return true;
	}
	// Regular jump
	return ObjectActionJump(cObj, TXDir, -iPhysicalJump, true);
}

bool ObjectComLetGo(C4Object *cObj, int32_t xdirf)
{
	// by ACTSCALE, ACTHANGLE or ExecCMDFMoveTo
	return ObjectActionJump(cObj, itofix(xdirf), Fix0, true);
}

bool ObjectComDig(C4Object *cObj) // by DFA_WALK
{
	if (!ObjectActionDig(cObj))
	{
		GameMsgObjectError(FormatString(LoadResStr("IDS_OBJ_NODIG"), cObj->GetName()).getData(), cObj);
		return false;
	}
	return true;
}

bool ObjectComPut(C4Object *cObj, C4Object *pTarget, C4Object *pThing)
{
	// No object specified, first from contents
	if (!pThing)
	{
		pThing = cObj->Contents.GetObject();
	}
	// Nothing to put
	if (!pThing)
	{
		return false;
	}
	// No target
	if (!pTarget)
	{
		return false;
	}
	// Grabbing: check C4D_Grab_Put
	if (pTarget != cObj->Contained
	&& !(pTarget->Def->GrabPutGet & C4D_Grab_Put))
	{
		// No grab put: fail
		return false;
	}
	// Target no fullcon
	if (!(pTarget->OCF & OCF_FullCon))
	{
		return false;
	}
	// Transfer thing
	bool fRejectCollect;
	if (!pThing->Enter(pTarget, true, true, &fRejectCollect))
	{
		return false;
	}
	// Put call to object script
	cObj->Call(PSF_Put);
	// Target collection call
	pTarget->Call(PSF_Collection,&C4AulParSet(pThing, true));
	// Success
	return true;
}

bool ObjectComThrow(C4Object *cObj, C4Object *pThing)
{
	// No object specified, first from contents
	if (!pThing)
	{
		pThing = cObj->Contents.GetObject();
	}
	// Nothing to throw
	if (!pThing)
	{
		return false;
	}
	// Throw com
	switch (cObj->GetProcedure())
	{
	case DFA_WALK:
		return ObjectActionThrow(cObj, pThing);
	}
	// Failure
	return false;
}

bool ObjectComDrop(C4Object *cObj, C4Object *pThing)
{
	// No object specified, first from contents
	if (!pThing)
	{
		pThing = cObj->Contents.GetObject();
	}
	// Nothing to throw
	if (!pThing)
	{
		return false;
	}
	// Force and direction
	// When dropping diagonally, drop from edge of shape
	// When doing a diagonal forward drop during flight, exit a bit closer to the Clonk to allow planned tumbling
	// Except when hangling, so you can mine effectively form the ceiling, and when swimming because you cannot tumble then
	C4Real pthrow = C4REAL100(cObj->GetPropertyInt(P_ThrowSpeed));
	int32_t tdir = 0;
	int right = 0;
	bool isHanglingOrSwimming = false;
	int32_t iProc = -1;
	C4PropList* pActionDef = cObj->GetAction();
	if (pActionDef)
	{
		iProc = pActionDef->GetPropertyP(P_Procedure);
		if (iProc == DFA_HANGLE || iProc == DFA_SWIM)
		{
			isHanglingOrSwimming = true;
		}
	}
	int32_t iOutposReduction = 1; // don't exit object too far forward during jump
	if (iProc != DFA_SCALE) // never diagonal during scaling (can have com into wall during scaling!)
	{
		// Table format is OK here
		if (ComDirLike(cObj->Action.ComDir, COMD_Left))  { tdir = -1; right = 0; if (cObj->xdir < C4REAL10(15)  && !isHanglingOrSwimming) --iOutposReduction; }
		if (ComDirLike(cObj->Action.ComDir, COMD_Right)) { tdir = +1; right = 1; if (cObj->xdir > C4REAL10(-15) && !isHanglingOrSwimming) --iOutposReduction; }
	}
	// Exit object
	pThing->Exit(cObj->GetX() + (cObj->Shape.x + cObj->Shape.Wdt * right) * !!tdir * iOutposReduction,
	             cObj->GetY() +  cObj->Shape.y + cObj->Shape.Hgt - (pThing->Shape.y + pThing->Shape.Hgt), 0, pthrow * tdir, Fix0, Fix0);
	// Update OCF
	cObj->SetOCF();
	// Ungrab
	ObjectComUnGrab(cObj);
	// Done
	return true;
}

bool ObjectComPutTake(C4Object *cObj, C4Object *pTarget, C4Object *pThing) // by C4CMD_Throw
{                                                                          // by C4CMD_Drop
	// Valid checks
	if (!pTarget)
	{
		return false;
	}
	// No object specified, first from contents
	if (!pThing)
	{
		pThing = cObj->Contents.GetObject();
	}
	// Has thing, put to target
	if (pThing)
	{
		return ObjectComPut(cObj, pTarget, pThing);
	}
	// If target is own container, activate activation menu
	if (pTarget == cObj->Contained)
	{
		return ObjectComTake(cObj); // carlo
	}
	// Assuming target is grabbed, check for grab get
	if (pTarget->Def->GrabPutGet & C4D_Grab_Get)
	{
		// Activate get menu
		return cObj->ActivateMenu(C4MN_Get, 0, 0, 0, pTarget);
	}
	// Failure
	return false;
}

// carlo
bool ObjectComTake(C4Object *cObj) // by C4CMD_Take
{
	return cObj->ActivateMenu(C4MN_Activate);
}

// carlo
bool ObjectComTake2(C4Object *cObj) // by C4CMD_Take2
{
	return cObj->ActivateMenu(C4MN_Get, 0, 0, 0, cObj->Contained);
}

bool ObjectComPunch(C4Object *cObj, C4Object *pTarget, int32_t punch)
{
	if (!cObj || !pTarget)
	{
		return false;
	}
	if (!punch)
	{
		return true;
	}
	bool fBlowStopped = !!pTarget->Call(PSF_QueryCatchBlow, &C4AulParSet(cObj));
	if (fBlowStopped && punch > 1)
	{
		punch = punch / 2; // Half damage for caught blow, so shield+armor help in fistfight and vs monsters
	}
	pTarget->DoEnergy(-punch, false, C4FxCall_EngGetPunched, cObj->Controller);
	int32_t tdir = +1;
	if (cObj->Action.Dir == DIR_Left)
	{
		tdir = -1;
	}
	pTarget->Action.ComDir = COMD_Stop;
	// No tumbles when blow was caught
	if (fBlowStopped)
	{
		return false;
	}
	// Hard punch
	if (punch >= 10 && ObjectActionTumble(pTarget, pTarget->Action.Dir, C4REAL100(150) * tdir, itofix(-2)))
	{
		pTarget->Call(PSF_CatchBlow, &C4AulParSet(punch, cObj));
		return true;
	}

	// Regular punch
	if (ObjectActionGetPunched(pTarget, C4REAL100(250) * tdir, Fix0))
	{
		pTarget->Call(PSF_CatchBlow,&C4AulParSet(punch, cObj));
		return true;
	}

	return false;
}

bool ObjectComCancelAttach(C4Object *cObj)
{
	if (cObj->GetProcedure() == DFA_ATTACH)
	{
		return cObj->SetAction(nullptr);
	}
	return false;
}

void ObjectComStopDig(C4Object *cObj)
{
	// Stand - but keep momentum to allow more dynamic digging
	C4Real o_xdir = cObj->xdir;
	C4Real o_ydir = cObj->ydir;
	ObjectActionStand(cObj);
	cObj->xdir = o_xdir;
	cObj->ydir = o_ydir;
	// Clear digging command
	if (cObj->Command && cObj->Command->Command == C4CMD_Dig)
	{
		cObj->ClearCommand(cObj->Command);
	}
}

bool ComDirLike(int32_t iComDir, int32_t iSample)
{
	if (iComDir == iSample) return true;
	if (iComDir % 8 + 1 == iSample) return true;
	if (iComDir == iSample % 8 + 1 ) return true;
	return false;
}

