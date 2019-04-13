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

/* Logic for C4Object: Actions */

#include "C4Include.h"
#include "C4ForbidLibraryCompilation.h"
#include "object/C4Object.h"

#include "game/C4Physics.h"
#include "landscape/C4MaterialList.h"
#include "landscape/C4PXS.h"
#include "object/C4Command.h"
#include "object/C4Def.h"
#include "object/C4MeshAnimation.h"
#include "object/C4ObjectCom.h"
#include "platform/C4SoundSystem.h"


void GrabLost(C4Object *cObj, C4Object *prev_target)
{
	// Grab lost script call on target (quite hacky stuff...)
	if (prev_target && prev_target->Status) prev_target->Call(PSF_GrabLost);
	// Clear commands down to first PushTo (if any) in command stack
	for (C4Command *pCom=cObj->Command; pCom; pCom=pCom->Next)
		if (pCom->Next && pCom->Next->Command==C4CMD_PushTo)
		{
			cObj->ClearCommand(pCom);
			break;
		}
}

static void DoGravity(C4Object *cobj)
{
	// Floatation in liquids
	if (cobj->InLiquid && cobj->Def->Float)
	{
		cobj->ydir-=GravAccel * C4REAL100(80);
		if (cobj->ydir<C4REAL100(-160)) cobj->ydir=C4REAL100(-160);
		if (cobj->xdir<-FloatFriction) cobj->xdir+=FloatFriction;
		if (cobj->xdir>+FloatFriction) cobj->xdir-=FloatFriction;
		if (cobj->rdir<-FloatFriction) cobj->rdir+=FloatFriction;
		if (cobj->rdir>+FloatFriction) cobj->rdir-=FloatFriction;
		// Reduce upwards speed when about to leave liquid to prevent eternal moving up and down.
		// Check both for no liquid one and two pixels above the surface, because the object could
		// skip one pixel as its speed can be more than 100.
		int32_t y_float = cobj->GetY() - 1 + cobj->Def->Float * cobj->GetCon() / FullCon;
		if (!GBackLiquid(cobj->GetX(), y_float - 1) || !GBackLiquid(cobj->GetX(), y_float - 2))
			if (cobj->ydir < 0)
			{
				// Reduce the upwards speed and set to zero for small values to prevent fluctuations.
				cobj->ydir /= 2;
				if (Abs(cobj->ydir) < C4REAL100(10))
					cobj->ydir = 0;
			}
	}
	// Free fall gravity
	else if (~cobj->Category & C4D_StaticBack)
		cobj->ydir+=GravAccel;
}

bool ReduceLineSegments(C4Shape &rShape, bool fAlternate)
{
	// try if line could go by a path directly when skipping on evertex. If fAlternate is true, try by skipping two vertices
	for (int32_t cnt=0; cnt+2+fAlternate<rShape.VtxNum; cnt++)
		if (PathFree(rShape.VtxX[cnt],rShape.VtxY[cnt],
		             rShape.VtxX[cnt+2+fAlternate],rShape.VtxY[cnt+2+fAlternate]))
		{
			if (fAlternate) rShape.RemoveVertex(cnt+2);
			rShape.RemoveVertex(cnt+1);
			return true;
		}
	return false;
}

void StopActionDelayCommand(C4Object *cobj)
{
	ObjectComStop(cobj);
	cobj->AddCommand(C4CMD_Wait,nullptr,0,0,50);
}

void Towards(C4Real &val, C4Real target, C4Real step)
{
	if (val==target) return;
	if (Abs(val-target)<=step) { val=target; return; }
	if (val<target) val+=step; else val-=step;
}

void C4Object::UpdateFace(bool bUpdateShape, bool fTemp)
{

	// Update shape - NOT for temp call, because temnp calls are done in drawing routine
	// must not change sync relevant data here (although the shape and pos *should* be updated at that time anyway,
	// because a runtime join would desync otherwise)
	if (!fTemp) { if (bUpdateShape) UpdateShape(); else UpdatePos(); }

	// SolidMask
	if (!fTemp) UpdateSolidMask(false);

	// Null defaults
	TopFace.Default();

	// newgfx: TopFace only
	if (Con>=FullCon || Def->GrowthType)
		if (!Def->Rotateable || (fix_r == Fix0))
			if (Def->TopFace.Wdt>0) // Fullcon & no rotation
				TopFace.Set(GetGraphics()->GetBitmap(Color),
				            Def->TopFace.x,Def->TopFace.y,
				            Def->TopFace.Wdt,Def->TopFace.Hgt);

	// Active face
	UpdateActionFace();
}

void C4Object::UpdateFlipDir()
{
	int32_t iFlipDir;
	// We're active
	C4PropList* pActionDef = GetAction();
	if (pActionDef)
		// Get flipdir value from action
		if ((iFlipDir = pActionDef->GetPropertyInt(P_FlipDir)))
			// Action dir is in flipdir range
			if (Action.Dir >= iFlipDir)
			{
				// Calculate flipped drawing dir (from the flipdir direction going backwards)
				Action.DrawDir = (iFlipDir - 1 - (Action.Dir - iFlipDir));
				// Set draw transform, creating one if necessary
				if (pDrawTransform)
					pDrawTransform->SetFlipDir(-1);
				else
					pDrawTransform = new C4DrawTransform(-1);
				// Done setting flipdir
				return;
			}
	// No flipdir necessary
	Action.DrawDir = Action.Dir;
	// Draw transform present?
	if (pDrawTransform)
	{
		// reset flip dir
		pDrawTransform->SetFlipDir(1);
		// if it's identity now, remove the matrix
		if (pDrawTransform->IsIdentity())
		{
			delete pDrawTransform;
			pDrawTransform=nullptr;
		}
	}
}

C4PropList* C4Object::GetAction() const
{
	C4Value value;
	GetProperty(P_Action, &value);
	return value.getPropList();
}

bool C4Object::SetAction(C4PropList * Act, C4Object *pTarget, C4Object *pTarget2, int32_t iCalls, bool fForce)
{
	C4Value vLastAction;
	GetProperty(P_Action, &vLastAction);
	C4PropList * LastAction = vLastAction.getPropList();
	int32_t iLastPhase=Action.Phase;
	C4Object *pLastTarget = Action.Target;
	C4Object *pLastTarget2 = Action.Target2;
	// No other action
	if (LastAction)
		if (LastAction->GetPropertyInt(P_NoOtherAction) && !fForce)
			if (Act != LastAction)
				return false;
	// Set animation on instance. Abort if the mesh does not have
	// such an animation.
	if (pMeshInstance)
	{
		if (Action.Animation) pMeshInstance->StopAnimation(Action.Animation);
		Action.Animation = nullptr;

		C4String* Animation = Act ? Act->GetPropertyStr(P_Animation) : nullptr;
		if (Animation)
		{
			// note that weight is ignored
			Action.Animation = pMeshInstance->PlayAnimation(Animation->GetData(), 0, nullptr, new C4ValueProviderAction(this), new C4ValueProviderConst(itofix(1)), true);
		}
	}
	// Stop previous act sound
	if (LastAction)
		if (Act != LastAction)
			if (LastAction->GetPropertyStr(P_Sound))
				StopSoundEffect(LastAction->GetPropertyStr(P_Sound)->GetCStr(),this);
	// Unfullcon objects no action
	if (Con<FullCon)
		if (!Def->IncompleteActivity)
			Act = nullptr;
	// Reset action time on change
	if (Act!=LastAction)
	{
		Action.Time=0;
		// reset action data and targets if procedure is changed
		if ((Act ? Act->GetPropertyP(P_Procedure) : -1)
			!= (LastAction ? LastAction->GetPropertyP(P_Procedure) : -1))
		{
			Action.Data = 0;
			Action.Target = nullptr;
			Action.Target2 = nullptr;
		}
	}
	// Set new action
	SetProperty(P_Action, C4VPropList(Act));
	Action.Phase=Action.PhaseDelay=0;
	// Set target if specified
	if (pTarget) Action.Target=pTarget;
	if (pTarget2) Action.Target2=pTarget2;
	// Set Action Facet
	UpdateActionFace();
	// update flipdir
	if ((LastAction ? LastAction->GetPropertyInt(P_FlipDir) : 0)
	    != (Act ? Act->GetPropertyInt(P_FlipDir) : 0)) UpdateFlipDir();
	// Start act sound
	if (Act)
		if (Act != LastAction)
			if (Act->GetPropertyStr(P_Sound))
				StartSoundEffect(Act->GetPropertyStr(P_Sound)->GetCStr(),+1,100,this);
	// Reset OCF
	SetOCF();
	// issue calls
	// Execute EndCall for last action
	if (iCalls & SAC_EndCall && !fForce)
		if (LastAction)
		{
			if (LastAction->GetPropertyStr(P_EndCall))
			{
				C4Def *pOldDef = Def;
				Call(LastAction->GetPropertyStr(P_EndCall)->GetCStr());
				// abort exeution if def changed
				if (Def != pOldDef || !Status) return true;
			}
		}
	// Execute AbortCall for last action
	if (iCalls & SAC_AbortCall && !fForce)
		if (LastAction)
		{
			if (LastAction->GetPropertyStr(P_AbortCall))
			{
				C4Def *pOldDef = Def;
				if (pLastTarget && !pLastTarget->Status) pLastTarget = nullptr;
				if (pLastTarget2 && !pLastTarget2->Status) pLastTarget2 = nullptr;
				Call(LastAction->GetPropertyStr(P_AbortCall)->GetCStr(), &C4AulParSet(iLastPhase, pLastTarget, pLastTarget2));
				// abort exeution if def changed
				if (Def != pOldDef || !Status) return true;
			}
		}
	// Execute StartCall for new action
	if (iCalls & SAC_StartCall)
		if (Act)
		{
			if (Act->GetPropertyStr(P_StartCall))
			{
				C4Def *pOldDef = Def;
				Call(Act->GetPropertyStr(P_StartCall)->GetCStr());
				// abort exeution if def changed
				if (Def != pOldDef || !Status) return true;
			}
		}

	C4Def *pOldDef = Def;
	Call(PSF_OnActionChanged, &C4AulParSet(LastAction ? LastAction->GetName() : "Idle"));
	if (Def != pOldDef || !Status) return true;

	return true;
}

void C4Object::UpdateActionFace()
{
	// Default: no action face
	Action.Facet.Default();
	// Active: get action facet from action definition
	C4PropList* pActionDef = GetAction();
	if (pActionDef)
	{
		if (pActionDef->GetPropertyInt(P_Wdt)>0)
		{
			Action.Facet.Set(GetGraphics()->GetBitmap(Color),
			                 pActionDef->GetPropertyInt(P_X),pActionDef->GetPropertyInt(P_Y),
			                 pActionDef->GetPropertyInt(P_Wdt),pActionDef->GetPropertyInt(P_Hgt));
			Action.FacetX=pActionDef->GetPropertyInt(P_OffX);
			Action.FacetY=pActionDef->GetPropertyInt(P_OffY);
		}
	}
}

bool C4Object::SetActionByName(C4String *ActName,
                               C4Object *pTarget, C4Object *pTarget2,
                               int32_t iCalls, bool fForce)
{
	assert(ActName);
	// If we get the null string or ActIdle by name, set ActIdle
	if (!ActName || ActName == &Strings.P[P_Idle])
		return SetAction(nullptr,nullptr,nullptr,iCalls,fForce);
	C4Value ActMap; GetProperty(P_ActMap, &ActMap);
	if (!ActMap.getPropList()) return false;
	C4Value Action; ActMap.getPropList()->GetPropertyByS(ActName, &Action);
	if (!Action.getPropList()) return false;
	return SetAction(Action.getPropList(),pTarget,pTarget2,iCalls,fForce);
}

bool C4Object::SetActionByName(const char * szActName,
                               C4Object *pTarget, C4Object *pTarget2,
                               int32_t iCalls, bool fForce)
{
	C4String * ActName = Strings.RegString(szActName);
	ActName->IncRef();
	bool r = SetActionByName(ActName, pTarget, pTarget2, iCalls, fForce);
	ActName->DecRef();
	return r;
}

void C4Object::SetDir(int32_t iDir)
{
	// Not active
	C4PropList* pActionDef = GetAction();
	if (!pActionDef) return;
	// Invalid direction
	if (!Inside<int32_t>(iDir,0,pActionDef->GetPropertyInt(P_Directions)-1)) return;
	// Execute turn action
	if (iDir != Action.Dir)
		if (pActionDef->GetPropertyStr(P_TurnAction))
			{ SetActionByName(pActionDef->GetPropertyStr(P_TurnAction)); }
	// Set dir
	Action.Dir=iDir;
	// update by flipdir?
	if (pActionDef->GetPropertyInt(P_FlipDir))
		UpdateFlipDir();
	else
		Action.DrawDir=iDir;
}

bool C4Object::SetPhase(int32_t iPhase)
{
	C4PropList* pActionDef = GetAction();
	if (!pActionDef) return false;
	const int32_t length = pActionDef->GetPropertyInt(P_Length);
	Action.Phase=Clamp<int32_t>(iPhase,0,length);
	Action.PhaseDelay = 0;
	return true;
}

int32_t C4Object::GetProcedure() const
{
	C4PropList* pActionDef = GetAction();
	if (!pActionDef) return -1;
	return pActionDef->GetPropertyP(P_Procedure);
}

void C4Object::NoAttachAction()
{
	// Active objects
	if (GetAction())
	{
		int32_t iProcedure = GetProcedure();
		C4Object *prev_target = Action.Target;
		// Scaling upwards: corner scale
		if (iProcedure == DFA_SCALE && Action.ComDir != COMD_Stop && ComDirLike(Action.ComDir, COMD_Up))
			if (ObjectActionCornerScale(this)) return;
		if (iProcedure == DFA_SCALE && Action.ComDir == COMD_Left && Action.Dir == DIR_Left)
			if (ObjectActionCornerScale(this)) return;
		if (iProcedure == DFA_SCALE && Action.ComDir == COMD_Right && Action.Dir == DIR_Right)
			if (ObjectActionCornerScale(this)) return;
		// Scaling and stopped: fall off to side (avoid zuppel)
		if ((iProcedure == DFA_SCALE) && (Action.ComDir == COMD_Stop))
		{
			if (Action.Dir == DIR_Left)
				{ if (ObjectActionJump(this,itofix(1),Fix0,false)) return; }
			else
				{ if (ObjectActionJump(this,itofix(-1),Fix0,false)) return; }
		}
		// Pushing: grab loss
		if (iProcedure==DFA_PUSH) GrabLost(this, prev_target);
		// Else jump
		ObjectActionJump(this,xdir,ydir,false);
	}
	// Inactive objects, simple mobile natural gravity
	else
	{
		DoGravity(this);
		Mobile=true;
	}
}

void C4Object::ContactAction()
{
	// Take certain action on contact. Evaluate t_contact-CNAT and Procedure.

	// Determine Procedure
	C4PropList* pActionDef = GetAction();
	if (!pActionDef) return;
	int32_t iProcedure=pActionDef->GetPropertyP(P_Procedure);
	int32_t fDisabled=pActionDef->GetPropertyInt(P_ObjectDisabled);

	//------------------------------- Hit Bottom ---------------------------------------------
	if (t_contact & CNAT_Bottom)
		switch (iProcedure)
		{
		case DFA_FLIGHT:
			if (ydir < 0) return;
			// Jump: FlatHit / HardHit / Walk
			if ((OCF & OCF_HitSpeed4) || fDisabled)
				if (ObjectActionFlat(this,Action.Dir)) return;
			if (OCF & OCF_HitSpeed3)
				if (ObjectActionKneel(this)) return;
			ObjectActionWalk(this);
			ydir = 0;
			return;
		case DFA_SCALE:
			// Scale down: stand
			if (ComDirLike(Action.ComDir, COMD_Down))
			{
				ObjectActionStand(this);
				return;
			}
			break;
		case DFA_DIG:
			// no special action
			break;
		case DFA_SWIM:
			// Try corner scale out
			if (!GBackSemiSolid(GetX(),GetY()-1+Def->Float*Con/FullCon-1))
				if (ObjectActionCornerScale(this)) return;
			break;
		}

	//------------------------------- Hit Ceiling -----------------------------------------
	if (t_contact & CNAT_Top)
		switch (iProcedure)
		{
		case DFA_WALK:
			// Walk: Stop
			ObjectActionStand(this); return;
		case DFA_SCALE:
			// Scale: Try hangle, else stop if going upward
			if (ComDirLike(Action.ComDir, COMD_Up))
			{
				if (ObjectActionHangle(this))
				{
					SetDir(Action.Dir == DIR_Left ? DIR_Right : DIR_Left);
					return;
				}
				Action.ComDir=COMD_Stop;
			}
			break;
		case DFA_FLIGHT:
			// Jump: Try hangle, else bounce off
			// High Speed Flight: Tumble
			if ((OCF & OCF_HitSpeed3) || fDisabled)
				{ ObjectActionTumble(this, Action.Dir, xdir, ydir); break; }
			if (ObjectActionHangle(this)) return;
			break;
		case DFA_DIG:
			// No action
			break;
		case DFA_HANGLE:
			Action.ComDir=COMD_Stop;
			break;
		}

	//---------------------------- Hit Left Wall ----------------------------------------
	if (t_contact & CNAT_Left)
	{
		switch (iProcedure)
		{
		case DFA_FLIGHT:
			// High Speed Flight: Tumble
			if ((OCF & OCF_HitSpeed3) || fDisabled)
				{ ObjectActionTumble(this, DIR_Left, xdir, ydir); break; }
			// Else
			else if (!ComDirLike(Action.ComDir, COMD_Right) && ObjectActionScale(this,DIR_Left)) return;
			break;
		case DFA_WALK:
			// Walk: Try scale
			if (ComDirLike(Action.ComDir, COMD_Left))
			{
				if (ObjectActionScale(this,DIR_Left))
				{
					ydir = C4REAL100(-1);
					return;
				}
			}
			// Heading away from solid
			if (ComDirLike(Action.ComDir, COMD_Right))
			{
				// Slide off
				ObjectActionJump(this,xdir/2,ydir,false);
			}
			return;
		case DFA_SWIM:
			// Only scale if swimming at the surface
			if (!GBackSemiSolid(GetX(),GetY()-1+Def->Float*Con/FullCon-1))
			{
				// Try scale, only if swimming at the surface.
				if (ComDirLike(Action.ComDir, COMD_Left))
					if (ObjectActionScale(this,DIR_Left)) return;
				// Try corner scale out
				if (ObjectActionCornerScale(this)) return;
			}
			return;
		case DFA_HANGLE:
			// Hangle: Try scale
			if (ObjectActionScale(this,DIR_Left))
			{
				ydir = C4REAL100(1);
				return;
			}
			return;
		case DFA_DIG:
			// Dig: no action
			break;
		}
	}

	//------------------------------ Hit Right Wall --------------------------------------
	if (t_contact & CNAT_Right)
	{
		switch (iProcedure)
		{
		case DFA_FLIGHT:
			// High Speed Flight: Tumble
			if ((OCF & OCF_HitSpeed3) || fDisabled)
				{ ObjectActionTumble(this, DIR_Right, xdir, ydir); break; }
			// Else Scale
			else if (!ComDirLike(Action.ComDir, COMD_Left) && ObjectActionScale(this,DIR_Right)) return;
			break;
		case DFA_WALK:
			// Walk: Try scale
			if (ComDirLike(Action.ComDir, COMD_Right))
			{
				if (ObjectActionScale(this,DIR_Right))
				{
					ydir = C4REAL100(-1);
					return;
				}
			}
			// Heading away from solid
			if (ComDirLike(Action.ComDir, COMD_Left))
			{
				// Slide off
				ObjectActionJump(this,xdir/2,ydir,false);
			}
			return;
		case DFA_SWIM:
			// Only scale if swimming at the surface
			if (!GBackSemiSolid(GetX(),GetY()-1+Def->Float*Con/FullCon-1))
			{
				// Try scale
				if (ComDirLike(Action.ComDir, COMD_Right))
					if (ObjectActionScale(this,DIR_Right)) return;
				// Try corner scale out
				if (ObjectActionCornerScale(this)) return;
			}
			return;
		case DFA_HANGLE:
			// Hangle: Try scale
			if (ObjectActionScale(this,DIR_Right))
			{
				ydir = C4REAL100(1);
				return;
			}
			return;
		case DFA_DIG:
			// Dig: no action
			break;
		}
	}

	//---------------------------- Unresolved Cases ---------------------------------------

	// Flight stuck
	if (iProcedure==DFA_FLIGHT)
	{
		// Enforce slide free (might slide through tiny holes this way)
		if (!ydir)
		{
			int fAllowDown = !(t_contact & CNAT_Bottom);
			if (t_contact & CNAT_Right)
			{
				ForcePosition(fix_x - 1, fix_y + fAllowDown);
				xdir=ydir=0;
			}
			if (t_contact & CNAT_Left)
			{
				ForcePosition(fix_x + 1, fix_y + fAllowDown);
				xdir=ydir=0;
			}
		}
		if (!xdir)
		{
			if (t_contact & CNAT_Top)
			{
				ForcePosition(fix_x, fix_y + 1);
				xdir=ydir=0;
			}
		}
	}
}

void C4Object::ExecAction()
{
	C4Real iTXDir;
	C4Real lftspeed,tydir;
	int32_t iTargetX;
	int32_t iPushRange,iPushDistance;

	// Standard phase advance
	int32_t iPhaseAdvance=1;

	// Upright attachment check
	if (!Mobile)
		if (Def->UprightAttach)
			if (Inside<int32_t>(GetR(),-StableRange,+StableRange))
			{
				Action.t_attach|=Def->UprightAttach;
				Mobile=true;
			}

	C4PropList* pActionDef = GetAction();
	// No IncompleteActivity? Reset action if there was one
	if (!(OCF & OCF_FullCon) && !Def->IncompleteActivity && pActionDef)
	{
		SetAction(nullptr);
		pActionDef = nullptr;
	}

	// InLiquidAction check
	if (InLiquid)
		if (pActionDef && pActionDef->GetPropertyStr(P_InLiquidAction))
		{
			SetActionByName(pActionDef->GetPropertyStr(P_InLiquidAction));
			pActionDef = GetAction();
		}
	
	// Idle objects do natural gravity only
	if (!pActionDef)
	{
		Action.t_attach = CNAT_None;
		if (Mobile) DoGravity(this);
		return;
	}

	C4Real fWalk,fMove;

	// Action time advance
	Action.Time++;

	C4Value Attach;
	pActionDef->GetProperty(P_Attach, &Attach);
	if (Attach.GetType() != C4V_Nil)
	{
		Action.t_attach = Attach.getInt();
	}
	else switch (pActionDef->GetPropertyP(P_Procedure))
	{
	case DFA_SCALE:
		if (Action.Dir == DIR_Left)  Action.t_attach = CNAT_Left;
		if (Action.Dir == DIR_Right) Action.t_attach = CNAT_Right;
		break;
	case DFA_HANGLE:
		Action.t_attach = CNAT_Top;
		break;
	case DFA_WALK:
	case DFA_KNEEL:
	case DFA_THROW:
	case DFA_PUSH:
	case DFA_PULL:
	case DFA_DIG:
		Action.t_attach = CNAT_Bottom;
		break;
	default:
		Action.t_attach = CNAT_None;
	}

	// if an object is in controllable state, so it can be assumed that if it dies later because of NO_OWNER's cause,
	// it has been its own fault and not the fault of the last one who threw a flint on it
	// do not reset for burning objects to make sure the killer is set correctly if they fall out of the map while burning
	if (!pActionDef->GetPropertyInt(P_ObjectDisabled) && pActionDef->GetPropertyP(P_Procedure) != DFA_FLIGHT && !OnFire)
		LastEnergyLossCausePlayer = NO_OWNER;

	// Handle Default Action Procedure: evaluates Procedure and Action.ComDir
	// Update xdir,ydir,Action.Dir,attachment,iPhaseAdvance
	int32_t dir = Action.Dir;
	C4Real accel = C4REAL100(pActionDef->GetPropertyInt(P_Accel));
	C4Real decel = accel;
	{
		C4Value decel_val;
		pActionDef->GetProperty(P_Decel, &decel_val);
		if (decel_val.GetType() != C4V_Nil)
			decel = C4REAL100(decel_val.getInt());
	}
	C4Real limit = C4REAL100(pActionDef->GetPropertyInt(P_Speed));

	switch (pActionDef->GetPropertyP(P_Procedure))
	{
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case DFA_WALK:
		switch (Action.ComDir)
		{
		case COMD_Left: case COMD_UpLeft: case COMD_DownLeft:
			// breaaak!!!
			if (dir == DIR_Right)
				xdir-=decel;
			else
				xdir-=accel;
			if (xdir<-limit) xdir=-limit;
			break;
		case COMD_Right: case COMD_UpRight: case COMD_DownRight:
			if (dir == DIR_Left)
				xdir+=decel;
			else
				xdir+=accel;
			if (xdir>+limit) xdir=+limit;
			break;
		case COMD_Stop: case COMD_Up: case COMD_Down:
			if (xdir<0) xdir+=decel;
			if (xdir>0) xdir-=decel;
			if ((xdir>-decel) && (xdir<+decel)) xdir=0;
			break;
		}
		iPhaseAdvance=0;
		if (xdir<0)
		{
			if (dir != DIR_Left) { SetDir(DIR_Left); xdir = -1; }
			iPhaseAdvance=-fixtoi(xdir*10);
		}
		if (xdir>0)
		{
			if (dir != DIR_Right) { SetDir(DIR_Right); xdir = 1; }
			iPhaseAdvance=+fixtoi(xdir*10);
		}

		Mobile=true;
		// object is rotateable? adjust to ground, if in horizontal movement or not attached to the center vertex
		if (Def->Rotateable && Shape.AttachMat != MNone && (!!xdir || Def->Shape.VtxX[Shape.iAttachVtx]))
			AdjustWalkRotation(20, 20, 100);
		else
			rdir=0;
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case DFA_KNEEL:
		ydir=0;
		Mobile=true;
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case DFA_SCALE:
	{
		int ComDir = Action.ComDir;
		if (Shape.CheckScaleToWalk(GetX(), GetY()))
		{
			ObjectActionWalk(this);
			return;
		}
		if ((Action.Dir == DIR_Left && ComDir == COMD_Left) || (Action.Dir == DIR_Right && ComDir == COMD_Right))
		{
			ComDir = COMD_Up;
		}
		switch (ComDir)
		{
		case COMD_Up: case COMD_UpRight:  case COMD_UpLeft:
			if (ydir > 0) ydir -= decel;
			else ydir -= accel;
			if (ydir < -limit) ydir = -limit;
			break;
		case COMD_Down: case COMD_DownRight: case COMD_DownLeft:
			if (ydir < 0) ydir += decel;
			else ydir += accel;
			if (ydir > +limit) ydir = +limit;
			break;
		case COMD_Left: case COMD_Right: case COMD_Stop:
			if (ydir < 0) ydir += decel;
			if (ydir > 0) ydir -= decel;
			if ((ydir > -decel) && (ydir < +decel)) ydir = 0;
			break;
		}
		iPhaseAdvance=0;
		if (ydir<0) iPhaseAdvance=-fixtoi(ydir*14);
		if (ydir>0) iPhaseAdvance=+fixtoi(ydir*14);
		xdir=0;
		Mobile=true;
		break;
	}
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case DFA_HANGLE:
		switch (Action.ComDir)
		{
		case COMD_Left: case COMD_UpLeft: case COMD_DownLeft:
			if (xdir > 0) xdir -= decel;
			else xdir -= accel;
			if (xdir < -limit) xdir = -limit;
			break;
		case COMD_Right: case COMD_UpRight: case COMD_DownRight:
			if (xdir < 0) xdir += decel;
			else xdir += accel;
			if (xdir > +limit) xdir = +limit;
			break;
		case COMD_Up:
			if (Action.Dir == DIR_Left)
				if (xdir > 0) xdir -= decel;
				else xdir -= accel;
			else
				if (xdir < 0) xdir += decel;
				else xdir += accel;
			if (xdir < -limit) xdir = -limit;
			if (xdir > +limit) xdir = +limit;
			break;
		case COMD_Stop: case COMD_Down:
			if (xdir < 0) xdir += decel;
			if (xdir > 0) xdir -= decel;
			if ((xdir > -decel) && (xdir < +decel)) xdir = 0;
			break;
		}
		iPhaseAdvance=0;
		if (xdir<0) { iPhaseAdvance=-fixtoi(xdir*10); SetDir(DIR_Left); }
		if (xdir>0) { iPhaseAdvance=+fixtoi(xdir*10); SetDir(DIR_Right); }
		ydir=0;
		Mobile=true;
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case DFA_FLIGHT:
		// Contained: fall out (one try only)
		if (!::Game.iTick10)
			if (Contained)
			{
				StopActionDelayCommand(this);
				SetCommand(C4CMD_Exit);
			}

		switch (Action.ComDir)
		{
		case COMD_Left: case COMD_UpLeft: case COMD_DownLeft:
			xdir -= std::max(std::min(limit + xdir, xdir > 0 ? decel : accel), itofix(0));
			break;
		case COMD_Right: case COMD_UpRight: case COMD_DownRight:
			xdir += std::max(std::min(limit - xdir, xdir < 0 ? decel : accel), itofix(0));
			break;
		}

		// Gravity/mobile
		DoGravity(this);
		Mobile=true;
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case DFA_DIG:
	{
		int32_t smpx = GetX(), smpy = GetY();
		bool fAttachOK = false;
		if (Action.t_attach & CNAT_Bottom && Shape.Attach(smpx,smpy,CNAT_Bottom)) fAttachOK = true;
		else if (Action.t_attach & CNAT_Left && Shape.Attach(smpx,smpy,CNAT_Left)) { fAttachOK = true; }
		else if (Action.t_attach & CNAT_Right && Shape.Attach(smpx,smpy,CNAT_Right)) { fAttachOK = true; }
		else if (Action.t_attach & CNAT_Top && Shape.Attach(smpx,smpy,CNAT_Top)) fAttachOK = true;
		if (!fAttachOK)
			{ ObjectComStopDig(this); return; }
		iPhaseAdvance=fixtoi(itofix(40)*limit);

		if (xdir < 0) SetDir(DIR_Left); else if (xdir > 0) SetDir(DIR_Right);
		Action.t_attach=CNAT_None;
		Mobile=true;
		break;
	}
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case DFA_SWIM:
		// ComDir changes xdir/ydir
		switch (Action.ComDir)
		{
		case COMD_Up:       ydir-=accel; break;
		case COMD_UpRight:  ydir-=accel;  xdir+=accel; break;
		case COMD_Right:                  xdir+=accel; break;
		case COMD_DownRight:ydir+=accel;  xdir+=accel; break;
		case COMD_Down:     ydir+=accel; break;
		case COMD_DownLeft: ydir+=accel;  xdir-=accel; break;
		case COMD_Left:                   xdir-=accel; break;
		case COMD_UpLeft:   ydir-=accel;  xdir-=accel; break;
		case COMD_Stop:
			if (xdir<0) xdir+=decel;
			if (xdir>0) xdir-=decel;
			if ((xdir>-decel) && (xdir<+decel)) xdir=0;
			if (ydir<0) ydir+=decel;
			if (ydir>0) ydir-=decel;
			if ((ydir>-decel) && (ydir<+decel)) ydir=0;
			break;
		}

		// Out of liquid check
		if (!InLiquid)
		{
			// Just above liquid: move down
			if (GBackLiquid(GetX(),GetY()+1+Def->Float*Con/FullCon-1)) ydir=+accel;
			// Free fall: walk
			else { ObjectActionWalk(this); return; }
		}

		// xdir/ydir bounds, don't apply if COMD_None
		if (Action.ComDir != COMD_None)
		{
			ydir = Clamp(ydir, -limit, limit);
			xdir = Clamp(xdir, -limit, limit);
		}
		// Surface dir bound
		if (!GBackLiquid(GetX(),GetY()-1+Def->Float*Con/FullCon-1)) if (ydir<0) ydir=0;
		// Dir, Phase, Attach
		if (xdir<0) SetDir(DIR_Left);
		if (xdir>0) SetDir(DIR_Right);
		iPhaseAdvance=fixtoi(limit*10);
		Mobile=true;

		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case DFA_THROW:
		Mobile=true;
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case DFA_PUSH:
		// No target
		if (!Action.Target) { StopActionDelayCommand(this); return; }
		// Inside target
		if (Contained==Action.Target) { StopActionDelayCommand(this); return; }
		// Target pushing force
		bool fStraighten;
		iTXDir=0; fStraighten=false;
		switch (Action.ComDir)
		{
		case COMD_Left: case COMD_DownLeft:   iTXDir=-limit; break;
		case COMD_UpLeft:  fStraighten=true;     iTXDir=-limit; break;
		case COMD_Right: case COMD_DownRight: iTXDir=+limit; break;
		case COMD_UpRight: fStraighten=true;     iTXDir=+limit; break;
		case COMD_Up:      fStraighten=true; break;
		case COMD_Stop: case COMD_Down:       iTXDir=0;       break;
		}
		// Push object
		if (!Action.Target->Push(iTXDir,accel,fStraighten))
			{ StopActionDelayCommand(this); return; }
		// Set target controller
		Action.Target->Controller=Controller;
		// ObjectAction got hold check
		iPushDistance = std::max(Shape.Wdt/2-8,0);
		iPushRange = iPushDistance + 10;
		int32_t sax,say,sawdt,sahgt;
		Action.Target->GetArea(sax,say,sawdt,sahgt);
		// Object lost
		if (!Inside(GetX()-sax,-iPushRange,sawdt-1+iPushRange)
		    || !Inside(GetY()-say,-iPushRange,sahgt-1+iPushRange))
		{
			C4Object *prev_target = Action.Target;
			// Wait command (why, anyway?)
			StopActionDelayCommand(this);
			// Grab lost action
			GrabLost(this, prev_target);
			// Done
			return;
		}
		// Follow object (full xdir reset)
		// Vertical follow: If object moves out at top, assume it's being pushed upwards and the Clonk must run after it
		if (GetY()-iPushDistance > say+sahgt && iTXDir) { if (iTXDir>0) sax+=sawdt/2; sawdt/=2; }
		// Horizontal follow
		iTargetX=Clamp(GetX(),sax-iPushDistance,sax+sawdt-1+iPushDistance);
		if (GetX()==iTargetX)
		{
			xdir=0;
		}
		else
		{
			if (GetX()<iTargetX)
				xdir=+limit;
			else if (GetX()>iTargetX)
				xdir=-limit;
		}
		// Phase by XDir
		iPhaseAdvance=0;
		if (xdir<0) { iPhaseAdvance=-fixtoi(xdir*10); SetDir(DIR_Left);  }
		if (xdir>0) { iPhaseAdvance=+fixtoi(xdir*10); SetDir(DIR_Right); }
		// No YDir
		ydir=0;
		Mobile=true;
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case DFA_PULL:
		// No target
		if (!Action.Target) { StopActionDelayCommand(this); return; }
		// Inside target
		if (Contained==Action.Target) { StopActionDelayCommand(this); return; }
		// Target contained
		if (Action.Target->Contained) { StopActionDelayCommand(this); return; }

		int32_t iPullDistance;
		int32_t iPullX;

		iPullDistance = Action.Target->Shape.Wdt/2 + Shape.Wdt/2;

		iTargetX=GetX();
		if (Action.ComDir==COMD_Right) iTargetX = Action.Target->GetX()+iPullDistance;
		if (Action.ComDir==COMD_Left) iTargetX = Action.Target->GetX()-iPullDistance;

		iPullX=Action.Target->GetX();
		if (Action.ComDir==COMD_Right) iPullX = GetX()-iPullDistance;
		if (Action.ComDir==COMD_Left) iPullX = GetX()+iPullDistance;

		fWalk = limit;

		fMove = 0;
		if (Action.ComDir==COMD_Right) fMove = +fWalk;
		if (Action.ComDir==COMD_Left) fMove = -fWalk;

		iTXDir = fMove + fWalk * Clamp<int32_t>(iPullX-Action.Target->GetX(),-10,+10) / 10;

		// Push object
		if (!Action.Target->Push(iTXDir,accel,false))
			{ StopActionDelayCommand(this); return; }
		// Set target controller
		Action.Target->Controller=Controller;

		// Train pulling: com dir transfer
		if ( (Action.Target->GetProcedure()==DFA_WALK)
		     || (Action.Target->GetProcedure()==DFA_PULL) )
		{
			Action.Target->Action.ComDir=COMD_Stop;
			if (iTXDir<0) Action.Target->Action.ComDir=COMD_Left;
			if (iTXDir>0) Action.Target->Action.ComDir=COMD_Right;
		}

		// Pulling range
		iPushDistance = std::max(Shape.Wdt/2-8,0);
		iPushRange = iPushDistance + 20;
		Action.Target->GetArea(sax,say,sawdt,sahgt);
		// Object lost
		if (!Inside(GetX()-sax,-iPushRange,sawdt-1+iPushRange)
		    || !Inside(GetY()-say,-iPushRange,sahgt-1+iPushRange))
		{
			// Remember target. Will be lost on changing action.
			C4Object *prev_target = Action.Target;
			// Wait command (why, anyway?)
			StopActionDelayCommand(this);
			// Grab lost action
			GrabLost(this, prev_target);
			// Lose target
			Action.Target=nullptr;
			// Done
			return;
		}

		// Move to pulling position
		xdir = fMove + fWalk * Clamp<int32_t>(iTargetX-GetX(),-10,+10) / 10;

		// Phase by XDir
		iPhaseAdvance=0;
		if (xdir<0) { iPhaseAdvance=-fixtoi(xdir*10); SetDir(DIR_Left);  }
		if (xdir>0) { iPhaseAdvance=+fixtoi(xdir*10); SetDir(DIR_Right); }
		// No YDir
		ydir=0;
		Mobile=true;

		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case DFA_LIFT:
		// Valid check
		if (!Action.Target) { SetAction(nullptr); return; }
		// Target lifting force
		lftspeed=itofix(2); tydir=0;
		switch (Action.ComDir)
		{
		case COMD_Up:   tydir=-lftspeed; break;
		case COMD_Stop: tydir=-GravAccel; break;
		case COMD_Down: tydir=+lftspeed; break;
		}
		// Lift object
		if (!Action.Target->Lift(tydir,C4REAL100(50)))
			{ SetAction(nullptr); return; }
		// Check LiftTop
		if (Def->LiftTop)
			if (Action.Target->GetY()<=(GetY()+Def->LiftTop))
				if (Action.ComDir==COMD_Up)
					Call(PSF_LiftTop);
		// General
		DoGravity(this);
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case DFA_FLOAT:
		// ComDir changes xdir/ydir
		switch (Action.ComDir)
		{
		case COMD_Up:
			ydir-=accel;
			if (xdir<0) xdir+=decel;
			if (xdir>0) xdir-=decel;
			if ((xdir>-decel) && (xdir<+decel)) xdir=0;
			break;
		case COMD_UpRight:  
			ydir-=accel; xdir+=accel; break;
		case COMD_Right:
			xdir+=accel; 
			if (ydir<0) ydir+=decel;
			if (ydir>0) ydir-=decel;
			if ((ydir>-decel) && (ydir<+decel)) ydir=0;
			break;
		case COMD_DownRight:
			ydir+=accel; xdir+=accel; break;
		case COMD_Down: 
			ydir+=accel;
			if (xdir<0) xdir+=decel;
			if (xdir>0) xdir-=decel;
			if ((xdir>-decel) && (xdir<+decel)) xdir=0;
			break;
		case COMD_DownLeft:
			ydir+=accel; xdir-=accel; break;
		case COMD_Left:
			xdir-=accel; 
			if (ydir<0) ydir+=decel;
			if (ydir>0) ydir-=decel;
			if ((ydir>-decel) && (ydir<+decel)) ydir=0;
			break;
		case COMD_UpLeft:
			ydir-=accel; xdir-=accel; break;
		case COMD_Stop:
			if (xdir<0) xdir+=decel;
			if (xdir>0) xdir-=decel;
			if ((xdir>-decel) && (xdir<+decel)) xdir=0;
			if (ydir<0) ydir+=decel;
			if (ydir>0) ydir-=decel;
			if ((ydir>-decel) && (ydir<+decel)) ydir=0;
			break;
		}
		// xdir/ydir bounds, don't apply if COMD_None
		if (Action.ComDir != COMD_None)
		{
			ydir = Clamp(ydir, -limit, limit);
			xdir = Clamp(xdir, -limit, limit);
		}

		Mobile=true;
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		// ATTACH: Force position to target object
		//   own vertex index is determined by high-order byte of action data
		//   target vertex index is determined by low-order byte of action data
	case DFA_ATTACH:

		// No target
		if (!Action.Target)
		{
			if (Status)
			{
				SetAction(nullptr);
				Call(PSF_AttachTargetLost);
			}
			return;
		}

		// Target incomplete and no incomplete activity
		if (!(Action.Target->OCF & OCF_FullCon))
			if (!Action.Target->Def->IncompleteActivity)
				{ SetAction(nullptr); return; }

		// Force containment
		if (Action.Target->Contained!=Contained)
		{
			if (Action.Target->Contained)
				Enter(Action.Target->Contained);
			else
				Exit(GetX(),GetY(),GetR());
		}

		// Object might have detached in Enter/Exit call
		if (!Action.Target) break;

		// Move position (so objects on solidmask move)
		MovePosition(Action.Target->fix_x + Action.Target->Shape.VtxX[Action.Data&255]
		              -Shape.VtxX[Action.Data>>8] - fix_x,
		              Action.Target->fix_y + Action.Target->Shape.VtxY[Action.Data&255]
		              -Shape.VtxY[Action.Data>>8] - fix_y);
		// must zero motion...
		xdir=ydir=0;

		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case DFA_CONNECT:
		{
			bool fBroke=false;
			bool fLineChange = false;

			// Line destruction check: Target missing or incomplete
			if (!Action.Target || (Action.Target->Con<FullCon)) fBroke=true;
			if (!Action.Target2 || (Action.Target2->Con<FullCon)) fBroke=true;
			if (fBroke)
			{
				Call(PSF_OnLineBreak,&C4AulParSet(true));
				AssignRemoval();
				return;
			}

			// Movement by Target
			// Connect to attach vertex
			C4Value lineAttachV; C4ValueArray *lineAttach;
			Action.Target->GetProperty(P_LineAttach, &lineAttachV);
			lineAttach = lineAttachV.getArray();
			int32_t iConnectX1, iConnectY1;
			iConnectX1 = Action.Target->GetX();
			iConnectY1 = Action.Target->GetY();
			if (lineAttach)
			{
				iConnectX1 += lineAttach->GetItem(0).getInt();
				iConnectY1 += lineAttach->GetItem(1).getInt();
			}
			if ((iConnectX1!=Shape.VtxX[0]) || (iConnectY1!=Shape.VtxY[0]))
			{
				// Regular wrapping line
				if (Def->LineIntersect == 0)
					if (!Shape.LineConnect(iConnectX1,iConnectY1,0,+1,
											Shape.VtxX[0],Shape.VtxY[0])) fBroke=true;
				// No-intersection line
				if (Def->LineIntersect == 1)
					{ Shape.VtxX[0]=iConnectX1; Shape.VtxY[0]=iConnectY1; }
					
				fLineChange = true;
			}

			// Movement by Target2
			// Connect to attach vertex
			Action.Target2->GetProperty(P_LineAttach, &lineAttachV);
			lineAttach = lineAttachV.getArray();
			int32_t iConnectX2, iConnectY2;
			iConnectX2 = Action.Target2->GetX();
			iConnectY2 = Action.Target2->GetY();
			if (lineAttach)
			{
				iConnectX2 += lineAttach->GetItem(0).getInt();
				iConnectY2 += lineAttach->GetItem(1).getInt();
			}
			if ((iConnectX2!=Shape.VtxX[Shape.VtxNum-1]) || (iConnectY2!=Shape.VtxY[Shape.VtxNum-1]))
			{
				// Regular wrapping line
				if (Def->LineIntersect == 0)
					if (!Shape.LineConnect(iConnectX2,iConnectY2,Shape.VtxNum-1,-1,
											Shape.VtxX[Shape.VtxNum-1],Shape.VtxY[Shape.VtxNum-1])) fBroke=true;
				// No-intersection line
				if (Def->LineIntersect == 1)
					{ Shape.VtxX[Shape.VtxNum-1]=iConnectX2; Shape.VtxY[Shape.VtxNum-1]=iConnectY2; }
					
				fLineChange = true;
			}

			// Line fBroke
			if (fBroke)
			{
				Call(PSF_OnLineBreak,nullptr);
				AssignRemoval();
				return;
			}

			// Reduce line segments
			if (!::Game.iTick35)
				if (ReduceLineSegments(Shape, !::Game.iTick2))
					fLineChange = true;
					
			// Line change callback
			if (fLineChange)
				Call(PSF_OnLineChange);
		}
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	default:
		// Attach
		if (Action.t_attach)
		{
			xdir = ydir = 0;
			Mobile = true;
		}
		// Free gravity
		else
			DoGravity(this);
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	}

	// Phase Advance (zero delay means no phase advance)
	if (pActionDef->GetPropertyInt(P_Delay))
	{
		Action.PhaseDelay+=iPhaseAdvance;
		if (Action.PhaseDelay >= pActionDef->GetPropertyInt(P_Delay))
		{
			// Advance Phase
			Action.PhaseDelay=0;
			Action.Phase += pActionDef->GetPropertyInt(P_Step);
			// Phase call
			if (pActionDef->GetPropertyStr(P_PhaseCall))
			{
				Call(pActionDef->GetPropertyStr(P_PhaseCall)->GetCStr());
			}
			// Phase end
			if (Action.Phase>=pActionDef->GetPropertyInt(P_Length))
			{
				C4String *next_action = pActionDef->GetPropertyStr(P_NextAction);
				// Keep current action if there is no NextAction
				if (!next_action)
					Action.Phase = 0;
				// set new action if it's not Hold
				else if (next_action == &Strings.P[P_Hold])
				{
					Action.Phase = pActionDef->GetPropertyInt(P_Length)-1;
					Action.PhaseDelay = pActionDef->GetPropertyInt(P_Delay)-1;
				}
				else
				{
					// Set new action
					SetActionByName(next_action, nullptr, nullptr, SAC_StartCall | SAC_EndCall);
				}
			}
		}
	}

	return;
}
