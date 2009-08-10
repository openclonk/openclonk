/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2007-2008  Matthes Bender
 * Copyright (c) 2001-2004  Peter Wortmann
 * Copyright (c) 2001, 2005-2006, 2008-2009  Sven Eberhardt
 * Copyright (c) 2001  Michael Käser
 * Copyright (c) 2005-2006, 2008  Günther Brammer
 * Copyright (c) 2006  Armin Burgmeier
 * Copyright (c) 2009  Nicolas Hake
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de
 *
 * Portions might be copyrighted by other authors who have contributed
 * to OpenClonk.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * See isc_license.txt for full license and disclaimer.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 * See clonk_trademark_license.txt for full license.
 */

/* Lots of handler functions for object action */

#include <C4Include.h>
#include <C4ObjectCom.h>

#ifndef BIG_C4INCLUDE
#include <C4Object.h>
#include <C4Physics.h>
#include <C4Command.h>
#include <C4Random.h>
#include <C4GameMessage.h>
#include <C4ObjectMenu.h>
#include <C4Player.h>
#include <C4GraphicsResource.h>
#include <C4Material.h>
#include <C4Game.h>
#include <C4PlayerList.h>
#include <C4GameObjects.h>
#endif

BOOL SimFlightHitsLiquid(FIXED fcx, FIXED fcy, FIXED xdir, FIXED ydir);
BOOL CreateConstructionSite(int32_t ctx, int32_t bty, C4ID strid, int32_t owner, C4Object *pByObj);

BOOL ObjectActionWalk(C4Object *cObj)
  {
  if (!cObj->SetActionByName("Walk")) return FALSE;
  cObj->xdir=cObj->ydir=0;
  return TRUE;
  }

BOOL ObjectActionStand(C4Object *cObj)
  {
  cObj->Action.ComDir=COMD_Stop;
  if (!ObjectActionWalk(cObj)) return FALSE;
  return TRUE;
  }

BOOL ObjectActionJump(C4Object *cObj, FIXED xdir, FIXED ydir, bool fByCom)
  {
	// scripted jump?
	assert(cObj);
	C4AulParSet pars(C4VInt(fixtoi(xdir, 100)), C4VInt(fixtoi(ydir, 100)), C4VBool(fByCom));
	if (!!cObj->Call(PSF_OnActionJump, &pars)) return TRUE;
	// hardcoded jump by action
  if (!cObj->SetActionByName("Jump")) return FALSE;
  cObj->xdir=xdir; cObj->ydir=ydir;
  cObj->Mobile=1;
	// unstick from ground, because jump command may be issued in an Action-callback,
	// where attach-values have already been determined for that frame
	cObj->Action.t_attach&=~CNAT_Bottom;
  return TRUE;
  }

BOOL ObjectActionDive(C4Object *cObj, FIXED xdir, FIXED ydir)
  {
  if (!cObj->SetActionByName("Dive")) return FALSE;
  cObj->xdir=xdir; cObj->ydir=ydir;
  cObj->Mobile=1;
	// unstick from ground, because jump command may be issued in an Action-callback,
	// where attach-values have already been determined for that frame
	cObj->Action.t_attach&=~CNAT_Bottom;
  return TRUE;
  }

BOOL ObjectActionTumble(C4Object *cObj, int32_t dir, FIXED xdir, FIXED ydir)
  {
  if (!cObj->SetActionByName("Tumble")) return FALSE;
  cObj->SetDir(dir);
  cObj->xdir=xdir; cObj->ydir=ydir;
  return TRUE;
  }

BOOL ObjectActionGetPunched(C4Object *cObj, FIXED xdir, FIXED ydir)
  {
  if (!cObj->SetActionByName("GetPunched")) return FALSE;
  cObj->xdir=xdir; cObj->ydir=ydir;
  return TRUE;
  }

BOOL ObjectActionKneel(C4Object *cObj)
  {
  if (!cObj->SetActionByName("KneelDown")) return FALSE;
  cObj->xdir=cObj->ydir=0;
  return TRUE;
  }

BOOL ObjectActionFlat(C4Object *cObj, int32_t dir)
  {
  if (!cObj->SetActionByName("FlatUp")) return FALSE;
  cObj->xdir=cObj->ydir=0;
  cObj->SetDir(dir);
  return TRUE;
  }

BOOL ObjectActionScale(C4Object *cObj, int32_t dir)
  {
  if (!cObj->SetActionByName("Scale")) return FALSE;
  cObj->xdir=cObj->ydir=0;
  cObj->SetDir(dir);
  return TRUE;
  }

BOOL ObjectActionHangle(C4Object *cObj, int32_t dir)
  {
  if (!cObj->SetActionByName("Hangle")) return FALSE;
  cObj->xdir=cObj->ydir=0;
  cObj->SetDir(dir);
  return TRUE;
  }

BOOL ObjectActionThrow(C4Object *cObj, C4Object *pThing)
  {
  // No object specified, first from contents
	if (!pThing) pThing = cObj->Contents.GetObject();
	// Nothing to throw
	if (!pThing) return FALSE;
	// Force and direction
  FIXED pthrow=ValByPhysical(400, cObj->GetPhysical()->Throw);
  int32_t iDir=1; if (cObj->Action.Dir==DIR_Left) iDir=-1;
	// Set action
  if (!cObj->SetActionByName("Throw")) return FALSE;
	// Exit object
	pThing->Exit(cObj->GetX(),
              cObj->GetY()+cObj->Shape.y-1,
              Random(360),
              pthrow*iDir,-pthrow,pthrow*iDir);
	// Success
  return TRUE;
  }

BOOL ObjectActionDig(C4Object *cObj)
  {
  if (!cObj->SetActionByName("Dig")) return FALSE;
  cObj->Action.Data=0; // Material Dig2Object request
  return TRUE;
  }

BOOL ObjectActionBuild(C4Object *cObj, C4Object *target)
  {
  return cObj->SetActionByName("Build",target);
  }

BOOL ObjectActionPush(C4Object *cObj, C4Object *target)
  {
  return cObj->SetActionByName("Push",target);
  }

BOOL ObjectActionFight(C4Object *cObj, C4Object *target)
  {
  return cObj->SetActionByName("Fight",target);
  }

BOOL ObjectActionChop(C4Object *cObj, C4Object *target)
  {
  return cObj->SetActionByName("Chop",target);
  }

BOOL CornerScaleOkay(C4Object *cObj, int32_t iRangeX, int32_t iRangeY)
  {
  int32_t ctx,cty;
  cty=cObj->GetY()-iRangeY;
  if (cObj->Action.Dir==DIR_Left) ctx=cObj->GetX()-iRangeX;
  else ctx=cObj->GetX()+iRangeX;
  cObj->ContactCheck(ctx,cty); // (resets VtxContact & t_contact)
  if (!(cObj->t_contact & CNAT_Top))
    if (!(cObj->t_contact & CNAT_Left))
      if (!(cObj->t_contact & CNAT_Right))
				if (!(cObj->t_contact & CNAT_Bottom))
					return TRUE;
  return FALSE;
  }

BOOL CheckCornerScale(C4Object *cObj, int32_t &iRangeX, int32_t &iRangeY)
	{
	for (iRangeX=CornerRange; iRangeX>=1; iRangeX--)
		for (iRangeY=CornerRange; iRangeY>=1; iRangeY--)
			if (CornerScaleOkay(cObj,iRangeX,iRangeY))
				return TRUE;
	return FALSE;
	}

BOOL ObjectActionCornerScale(C4Object *cObj)
  {
	int32_t iRangeX,iRangeY;
	// Scaling: check range max to min
	if (cObj->GetProcedure()==DFA_SCALE)
		{
		if (!CheckCornerScale(cObj,iRangeX,iRangeY)) return FALSE;
		}
	// Swimming: check range min to max
	else
		{
		iRangeY=2;
		while (!CornerScaleOkay(cObj,iRangeY,iRangeY))
			{ iRangeY++; if (iRangeY>CornerRange) return FALSE; }
		iRangeX=iRangeY;
		}
	// Do corner scale
	if (!cObj->SetActionByName("KneelUp"))
		cObj->SetActionByName("Walk");
	cObj->xdir=cObj->ydir=0;
	//if (cObj->Action.Dir==DIR_Left) cObj->Action.ComDir=COMD_Left; else cObj->Action.ComDir=COMD_Right;
	if (cObj->Action.Dir==DIR_Left) cObj->fix_x-=itofix(iRangeX);
		else cObj->fix_x+=itofix(iRangeX);
	cObj->fix_y-=itofix(iRangeY);
	return TRUE;
	}

BOOL ObjectComMovement(C4Object *cObj, int32_t comdir)
  {
  cObj->Action.ComDir=comdir;

  PlayerObjectCommand(cObj->Owner,C4CMD_Follow,cObj);
	// direkt turnaround if standing still
	if (!cObj->xdir && (cObj->GetProcedure() == DFA_WALK || cObj->GetProcedure() == DFA_HANGLE))
		switch (comdir)
			{
			case COMD_Left: case COMD_UpLeft: case COMD_DownLeft:
				cObj->SetDir(DIR_Left);
				break;
			case COMD_Right: case COMD_UpRight: case COMD_DownRight:
				cObj->SetDir(DIR_Right);
				break;
			}
  return TRUE;
  }

BOOL ObjectComTurn(C4Object *cObj)
	{
	// turn around, if standing still
	if (!cObj->xdir && cObj->GetProcedure() == DFA_WALK)
		{
		cObj->SetDir(1-cObj->Action.Dir);
		return TRUE;
		}
	return FALSE;
	}

BOOL ObjectComStop(C4Object *cObj)
  {
  // Cease current action
  cObj->SetActionByName("Idle");
  // Action walk if possible
  return ObjectActionStand(cObj);
  }

BOOL ObjectComGrab(C4Object *cObj, C4Object *pTarget)
	{
	if (!pTarget) return FALSE;
	if (cObj->GetProcedure()!=DFA_WALK) return FALSE;
	if (!ObjectActionPush(cObj,pTarget)) return FALSE;
	cObj->Call(PSF_Grab, &C4AulParSet(C4VObj(pTarget), C4VBool(true)));
	if (pTarget->Status && cObj->Status)
		pTarget->Call(PSF_Grabbed, &C4AulParSet(C4VObj(cObj), C4VBool(true)));
	return TRUE;
	}

BOOL ObjectComUnGrab(C4Object *cObj)
	{
	// Only if pushing, -> stand
	if (cObj->GetProcedure() == DFA_PUSH)
		{
		C4Object *pTarget = cObj->Action.Target;
		if (ObjectActionStand(cObj))
			{
			if (!cObj->CloseMenu(false)) return FALSE;
			cObj->Call(PSF_Grab, &C4AulParSet(C4VObj(pTarget), C4VBool(false)));
			if (pTarget && pTarget->Status && cObj->Status)
				pTarget->Call(PSF_Grabbed, &C4AulParSet(C4VObj(cObj), C4VBool(false)));
			return TRUE;
			}
		}

	return FALSE;
	}

BOOL ObjectComJump(C4Object *cObj) // by ObjectComUp, ExecCMDFMoveTo, FnJump
  {
	// Only if walking
	if (cObj->GetProcedure()!=DFA_WALK) return FALSE;
	// Calculate direction & forces
	FIXED TXDir=Fix0;
	C4PhysicalInfo *pPhysical=cObj->GetPhysical();
	FIXED iPhysicalWalk = ValByPhysical(280, pPhysical->Walk) * itofix(cObj->GetCon(), FullCon);
	FIXED iPhysicalJump = ValByPhysical(1000, pPhysical->Jump) * itofix(cObj->GetCon(), FullCon);

	if (cObj->Action.ComDir==COMD_Left || cObj->Action.ComDir==COMD_UpLeft)  TXDir=-iPhysicalWalk;
	else if (cObj->Action.ComDir==COMD_Right || cObj->Action.ComDir==COMD_UpRight) TXDir=+iPhysicalWalk;
	else
		{
		if (cObj->Action.Dir==DIR_Left)  TXDir=-iPhysicalWalk;
		if (cObj->Action.Dir==DIR_Right) TXDir=+iPhysicalWalk;
		}
	FIXED x = cObj->fix_x, y = cObj->fix_y;
	// find bottom-most vertex, correct starting position for simulation
	int32_t iBtmVtx = cObj->Shape.GetBottomVertex();
	if(iBtmVtx != -1) { x += cObj->Shape.GetVertexX(iBtmVtx); y += cObj->Shape.GetVertexY(iBtmVtx); }
	// Try dive
	if(cObj->Shape.ContactDensity > C4M_Liquid)
		if (SimFlightHitsLiquid(x,y,TXDir,-iPhysicalJump))
			if (ObjectActionDive(cObj,TXDir,-iPhysicalJump))
				return TRUE;
  // Regular jump
  return ObjectActionJump(cObj,TXDir,-iPhysicalJump,true);
  }

BOOL ObjectComLetGo(C4Object *cObj, int32_t xdirf)
  { // by ACTSCALE, ACTHANGLE or ExecCMDFMoveTo
  return ObjectActionJump(cObj,itofix(xdirf),Fix0,true);
  }

BOOL ObjectComEnter(C4Object *cObj) // by pusher
  {
  if (!cObj) return FALSE;

	// NoPushEnter
	if (cObj->Def->NoPushEnter) return FALSE;

  // Check object entrance, try command enter
  C4Object *pTarget;
  DWORD ocf=OCF_Entrance;
  if ((pTarget=::Objects.AtObject(cObj->GetX(),cObj->GetY(),ocf,cObj)))
    if (ocf & OCF_Entrance)
      { cObj->SetCommand(C4CMD_Enter,pTarget); return TRUE; }

  return FALSE;
  }


BOOL ObjectComUp(C4Object *cObj) // by DFA_WALK or DFA_SWIM
  {
  if (!cObj) return FALSE;

  // Check object entrance, try command enter
  C4Object *pTarget;
  DWORD ocf=OCF_Entrance;
  if ((pTarget=::Objects.AtObject(cObj->GetX(),cObj->GetY(),ocf,cObj)))
    if (ocf & OCF_Entrance)
      return PlayerObjectCommand(cObj->Owner,C4CMD_Enter,pTarget);

  // Try jump
	if (cObj->GetProcedure()==DFA_WALK)
		return PlayerObjectCommand(cObj->Owner,C4CMD_Jump);

	return FALSE;
  }

BOOL ObjectComDig(C4Object *cObj) // by DFA_WALK
  {
  C4PhysicalInfo *phys=cObj->GetPhysical();
  if (!phys->CanDig || !ObjectActionDig(cObj))
    {
		GameMsgObject(FormatString(LoadResStr("IDS_OBJ_NODIG"),cObj->GetName()).getData(),cObj);
    return FALSE;
    }
  return TRUE;
  }

C4Object *CreateLine(C4ID idType, int32_t iOwner, C4Object *pFrom, C4Object *pTo)
	{
	C4Object *pLine;
	if (!pFrom || !pTo) return NULL;
	if (!(pLine=Game.CreateObject(idType,pFrom,iOwner,0,0))) return NULL;
	pLine->Shape.VtxNum=2;
	pLine->Shape.VtxX[0]=pFrom->GetX();
	pLine->Shape.VtxY[0]=pFrom->GetY()+pFrom->Shape.Hgt/4;
	pLine->Shape.VtxX[1]=pTo->GetX();
	pLine->Shape.VtxY[1]=pTo->GetY()+pTo->Shape.Hgt/4;
	pLine->Action.Target=pFrom;
	pLine->Action.Target2=pTo;
	return pLine;
	}

BOOL ObjectComLineConstruction(C4Object *cObj)
	{
	C4Object *linekit,*tstruct,*cline;
	DWORD ocf;

  ObjectActionStand(cObj);

  // Check physical
  if (!cObj->GetPhysical()->CanConstruct)
		{
		GameMsgObject(FormatString(LoadResStr("IDS_OBJ_NOLINECONSTRUCT"),cObj->GetName()).getData(),cObj); return FALSE;
		}

	// - - - - - - - - - - - - - - - - - - Line pickup - - - - - - - - - - - - - - - - -

	// Check for linekit
	if (!(linekit=cObj->Contents.Find(C4ID_Linekit)))
		{
		// Check for collection limit
		if (cObj->Def->CollectionLimit && (cObj->Contents.ObjectCount()>=cObj->Def->CollectionLimit) ) return FALSE;
    // Check line pickup
		ocf=OCF_LineConstruct;
		tstruct=::Objects.AtObject(cObj->GetX(),cObj->GetY(),ocf,cObj);
		if (!tstruct || !(ocf & OCF_LineConstruct)) return FALSE;
		if (!(cline=Game.FindObject(C4ID_None,0,0,0,0,OCF_All,"Connect",tstruct))) return FALSE;
		// Check line connected to linekit at other end
		if ( (cline->Action.Target && (cline->Action.Target->Def->id==C4ID_Linekit))
			|| (cline->Action.Target2 && (cline->Action.Target2->Def->id==C4ID_Linekit)) )
				{
				StartSoundEffect("Error",false,100,cObj);
				GameMsgObject(FormatString(LoadResStr("IDS_OBJ_NODOUBLEKIT"),cline->GetName()).getData(),cObj);	return FALSE;
				}
		// Create new linekit
    if (!(linekit=Game.CreateObject(C4ID_Linekit,cObj,cline->Owner))) return FALSE;
		// Enter linekit into clonk
		bool fRejectCollect;
		if (!linekit->Enter(cObj, TRUE, true, &fRejectCollect))
			{
			// Enter failed: abort operation
			linekit->AssignRemoval(); return FALSE;
			}
		// Attach line to collected linekit
		StartSoundEffect("Connect",false,100,cObj);
		if (cline->Action.Target==tstruct) cline->Action.Target=linekit;
		if (cline->Action.Target2==tstruct) cline->Action.Target2=linekit;
		// Message
		GameMsgObject(FormatString(LoadResStr("IDS_OBJ_DISCONNECT"),cline->GetName(),tstruct->GetName()).getData(),tstruct);
		return TRUE;
		}

	// - - - - - - - - - -  - - - - - Active construction - - - - - - - - - - - - - - - - -

	// Active line construction
	if (cline=Game.FindObject(C4ID_None,0,0,0,0,OCF_All,"Connect",linekit))
		{

		// Check for structure connection
		ocf=OCF_LineConstruct;
		tstruct=::Objects.AtObject(cObj->GetX(),cObj->GetY(),ocf,cObj);
		// No structure
		if (!tstruct || !(ocf & OCF_LineConstruct))
			{
			// No connect
			StartSoundEffect("Error",false,100,cObj);
			GameMsgObject(FormatString(LoadResStr("IDS_OBJ_NOCONNECT")).getData(),cObj);	return FALSE;
			}

		// Check short circuit -> removal
		if ((cline->Action.Target==tstruct)
		 || (cline->Action.Target2==tstruct))
			{
			StartSoundEffect("Connect",false,100,cObj);
			GameMsgObject(FormatString(LoadResStr("IDS_OBJ_LINEREMOVAL"),cline->GetName()).getData(),tstruct);
			cline->AssignRemoval();
			return TRUE;
			}

		// Check for correct connection type
		BOOL connect_okay=FALSE;
		switch (cline->Def->Line)
			{
			case C4D_Line_Power:
				if (tstruct->Def->LineConnect & C4D_Power_Input) connect_okay=TRUE;
				if (tstruct->Def->LineConnect & C4D_Power_Output) connect_okay=TRUE;
				break;
			case C4D_Line_Source:
				if (tstruct->Def->LineConnect & C4D_Liquid_Output) connect_okay=TRUE;	break;
			case C4D_Line_Drain:
				if (tstruct->Def->LineConnect & C4D_Liquid_Input) connect_okay=TRUE;	break;
			default: return FALSE; // Undefined line type
			}
	  if (!connect_okay)
			{
			StartSoundEffect("Error",false,100,cObj);
			GameMsgObject(FormatString(LoadResStr("IDS_OBJ_NOCONNECTTYPE"),cline->GetName(),tstruct->GetName()).getData(),tstruct);
			return FALSE;
			}

		// Connect line to structure
		StartSoundEffect("Connect",false,100,cObj);
		if (cline->Action.Target==linekit) cline->Action.Target=tstruct;
		if (cline->Action.Target2==linekit) cline->Action.Target2=tstruct;
		linekit->Exit();
		linekit->AssignRemoval();

		GameMsgObject(FormatString(LoadResStr("IDS_OBJ_CONNECT"),cline->GetName(),tstruct->GetName()).getData(),tstruct);

		return TRUE;
		}

	// - - - - - - - - - - - - - - - - New line - - - - - - - - - - - - - - - - - - - - -

	// Check for new structure connection
	ocf=OCF_LineConstruct;
	tstruct=::Objects.AtObject(cObj->GetX(),cObj->GetY(),ocf,cObj);
	if (!tstruct || !(ocf & OCF_LineConstruct))
		{
		StartSoundEffect("Error",false,100,cObj);
		GameMsgObject(FormatString(LoadResStr("IDS_OBJ_NONEWLINE")).getData(),cObj);	return FALSE;
		}

	// Determine new line type
	C4ID linetype=C4ID_None;
	// Check source pipe
	if (linetype==C4ID_None)
		if (tstruct->Def->LineConnect & C4D_Liquid_Pump)
			if (!Game.FindObject(C4ID_SourcePipe,0,0,0,0,OCF_All,"Connect",tstruct))
				linetype = C4ID_SourcePipe;
	// Check drain pipe
	if (linetype==C4ID_None)
		if (tstruct->Def->LineConnect & C4D_Liquid_Output)
			if (!Game.FindObject(C4ID_DrainPipe,0,0,0,0,OCF_All,"Connect",tstruct))
				linetype = C4ID_DrainPipe;
	// Check power
	if (linetype==C4ID_None)
		if (tstruct->Def->LineConnect & C4D_Power_Output)
			linetype = C4ID_PowerLine;
	// No good
	if (linetype==C4ID_None)
		{
		StartSoundEffect("Error",false,100,cObj);
		GameMsgObject(FormatString(LoadResStr("IDS_OBJ_NONEWLINE")).getData(),cObj);	return FALSE;
		}

	// Create new line
	C4Object *newline=CreateLine(linetype,cObj->Owner,
															 tstruct,linekit);
	if (!newline) return FALSE;
	StartSoundEffect("Connect",false,100,cObj);
	GameMsgObject(FormatString(LoadResStr("IDS_OBJ_NEWLINE"),newline->GetName()).getData(),tstruct);

	return TRUE;
	}

void ObjectComDigDouble(C4Object *cObj) // "Activation" by DFA_WALK, DFA_DIG, DFA_SWIM
  {
	C4Object *pTarget;
  DWORD ocf;
  C4PhysicalInfo *phys=cObj->GetPhysical();

	// Contents activation (first contents object only)
	if (cObj->Contents.GetObject())
		if (!! cObj->Contents.GetObject()->Call(PSF_Activate,&C4AulParSet(C4VObj(cObj))))
			return;

	// Linekit: Line construction (move to linekit script...)
  if (cObj->Contents.GetObject() && (cObj->Contents.GetObject()->id==C4ID_Linekit))
    {
		ObjectComLineConstruction(cObj);
		return;
    }

	// Chop
  ocf=OCF_Chop;
  if (phys->CanChop)
		if (cObj->GetProcedure()!=DFA_SWIM)
	    if ((pTarget=::Objects.AtObject(cObj->GetX(),cObj->GetY(),ocf,cObj)))
		    if (ocf & OCF_Chop)
			    {
				  PlayerObjectCommand(cObj->Owner,C4CMD_Chop,pTarget);
					return;
					}

	// Line construction pick up
  ocf=OCF_LineConstruct;
  if (phys->CanConstruct)
	  if (!cObj->Contents.GetObject())
	    if ((pTarget=::Objects.AtObject(cObj->GetX(),cObj->GetY(),ocf,cObj)))
		    if (ocf & OCF_LineConstruct)
			    if (ObjectComLineConstruction(cObj))
						return;

  // Own activation call
  if (!! cObj->Call(PSF_Activate, &C4AulParSet(C4VObj(cObj)))) return;

  }

BOOL ObjectComDownDouble(C4Object *cObj) // by DFA_WALK
  {
  C4Object *pTarget;
  DWORD ocf= OCF_Construct | OCF_Grab;
  if ((pTarget=::Objects.AtObject(cObj->GetX(),cObj->GetY(),ocf,cObj)))
    {
    if (ocf & OCF_Construct)
      { PlayerObjectCommand(cObj->Owner,C4CMD_Build,pTarget); return TRUE; }
    if (ocf & OCF_Grab)
      { PlayerObjectCommand(cObj->Owner,C4CMD_Grab,pTarget); return TRUE; }
    }
  return FALSE;
  }

BOOL ObjectComPut(C4Object *cObj, C4Object *pTarget, C4Object *pThing)
  {
  // No object specified, first from contents
	if (!pThing) pThing = cObj->Contents.GetObject();
	// Nothing to put
	if (!pThing) return FALSE;
	// No target
  if (!pTarget) return FALSE;
  // Grabbing: check C4D_Grab_Put
  if (pTarget!=cObj->Contained)
    if (!(pTarget->Def->GrabPutGet & C4D_Grab_Put))
      {
      // Was meant to be a drop anyway
      if (ValidPlr(cObj->Owner))
        if (::Players.Get(cObj->Owner)->LastComDownDouble)
          return ObjectComDrop(cObj, pThing);
      // No grab put: fail
      return FALSE;
      }
  // Target no fullcon
  if (!(pTarget->OCF & OCF_FullCon)) return FALSE;
	// Check target collection limit
	if (pTarget->Def->CollectionLimit && (pTarget->Contents.ObjectCount()>=pTarget->Def->CollectionLimit)) return FALSE;
  // Transfer thing
	bool fRejectCollect;
  if (!pThing->Enter(pTarget, TRUE, true, &fRejectCollect)) return FALSE;
	// Put call to object script
  cObj->Call(PSF_Put);
	// Target collection call
  pTarget->Call(PSF_Collection,&C4AulParSet(C4VObj(pThing), C4VBool(TRUE)));
	// Success
  return TRUE;
  }

BOOL ObjectComThrow(C4Object *cObj, C4Object *pThing)
	{
	// No object specified, first from contents
	if (!pThing) pThing = cObj->Contents.GetObject();
	// Nothing to throw
	if (!pThing) return FALSE;
	// Throw com
	switch (cObj->GetProcedure())
		{
		case DFA_WALK: return ObjectActionThrow(cObj,pThing);
		}
	// Failure
	return FALSE;
	}

BOOL ObjectComDrop(C4Object *cObj, C4Object *pThing)
  {
  // No object specified, first from contents
	if (!pThing) pThing = cObj->Contents.GetObject();
	// Nothing to throw
	if (!pThing) return FALSE;
	// Force and direction
	// When dropping diagonally, drop from edge of shape
	// When doing a diagonal forward drop during flight, exit a bit closer to the Clonk to allow planned tumbling
	// Except when hangling, so you can mine effectively form the ceiling, and when swimming because you cannot tumble then
  FIXED pthrow=ValByPhysical(400, cObj->GetPhysical()->Throw);
  int32_t tdir=0; int right=0;
	bool isHanglingOrSwimming = false;
	int32_t iProc = DFA_NONE;
	if (cObj->Action.pActionDef)
		{
		iProc = cObj->Action.pActionDef->GetPropertyInt(P_Procedure);
		if (iProc == DFA_HANGLE || iProc == DFA_SWIM) isHanglingOrSwimming = true;
		}
	int32_t iOutposReduction = 1; // don't exit object too far forward during jump
	if (iProc != DFA_SCALE) // never diagonal during scaling (can have com into wall during scaling!)
		{
		if (ComDirLike(cObj->Action.ComDir, COMD_Left)) { tdir=-1; right = 0; if (cObj->xdir < FIXED10(15) && !isHanglingOrSwimming) --iOutposReduction; }
		if (ComDirLike(cObj->Action.ComDir, COMD_Right)) { tdir=+1; right = 1;  if (cObj->xdir > FIXED10(-15) && !isHanglingOrSwimming) --iOutposReduction; }
		}
	// Exit object
	pThing->Exit(cObj->GetX() + (cObj->Shape.x + cObj->Shape.Wdt * right) * !!tdir * iOutposReduction,
		cObj->GetY()+cObj->Shape.y+cObj->Shape.Hgt-(pThing->Shape.y+pThing->Shape.Hgt),0,pthrow*tdir,Fix0,Fix0);
	// NoCollectDelay
	cObj->NoCollectDelay=2;
	// Update OCF
	cObj->SetOCF();
	// Ungrab
  ObjectComUnGrab(cObj);
	// Done
  return TRUE;
  }

BOOL ObjectComChop(C4Object *cObj, C4Object *pTarget)
  {
  if (!pTarget) return FALSE;
  if (!cObj->GetPhysical()->CanChop)
		{
    GameMsgObject(FormatString(LoadResStr("IDS_OBJ_NOCHOP"),cObj->GetName()).getData(),cObj);
		return FALSE;
		}
  if (cObj->GetProcedure()!=DFA_WALK) return FALSE;
  return ObjectActionChop(cObj,pTarget);
  }

BOOL ObjectComBuild(C4Object *cObj, C4Object *pTarget)
  {
  if (!pTarget) return FALSE;
  // Needs to be idle or walking
  if (cObj->Action.pActionDef)
    if (cObj->GetProcedure()!=DFA_WALK)
      return FALSE;
  return ObjectActionBuild(cObj,pTarget);
  }

BOOL ObjectComPutTake(C4Object *cObj, C4Object *pTarget, C4Object *pThing) // by C4CMD_Throw
  {																																				 // by C4CMD_Drop
  // Valid checks
  if (!pTarget) return FALSE;
  // No object specified, first from contents
	if (!pThing) pThing = cObj->Contents.GetObject();
  // Has thing, put to target
  if (pThing)
    return ObjectComPut(cObj,pTarget,pThing);
  // If target is own container, activate activation menu
  if (pTarget==cObj->Contained)
		return ObjectComTake(cObj); // carlo
  // Assuming target is grabbed, check for grab get
  if (pTarget->Def->GrabPutGet & C4D_Grab_Get)
    {
    // Activate get menu
    return cObj->ActivateMenu(C4MN_Get,0,0,0,pTarget);
    }
  // Failure
  return FALSE;
  }

// carlo
BOOL ObjectComTake(C4Object *cObj) // by C4CMD_Take
	{
	return cObj->ActivateMenu(C4MN_Activate);
	}

// carlo
BOOL ObjectComTake2(C4Object *cObj) // by C4CMD_Take2
	{
	return cObj->ActivateMenu(C4MN_Get,0,0,0,cObj->Contained);
	}

BOOL ObjectComPunch(C4Object *cObj, C4Object *pTarget, int32_t punch)
  {
  if (!cObj || !pTarget) return FALSE;
	if (!punch)
		if (pTarget->GetPhysical()->Fight)
			punch=BoundBy<int32_t>(5*cObj->GetPhysical()->Fight/pTarget->GetPhysical()->Fight,0,10);
	if (!punch) return TRUE;
	bool fBlowStopped = !!pTarget->Call(PSF_QueryCatchBlow,&C4AulParSet(C4VObj(cObj)));
	if (fBlowStopped && punch>1) punch=punch/2; // half damage for caught blow, so shield+armor help in fistfight and vs monsters
	pTarget->DoEnergy(-punch, false, C4FxCall_EngGetPunched, cObj->Controller);
  int32_t tdir=+1; if (cObj->Action.Dir==DIR_Left) tdir=-1;
  pTarget->Action.ComDir=COMD_Stop;
	// No tumbles when blow was caught
	if (fBlowStopped) return FALSE;
  // Hard punch
  if (punch>=10)
    if (ObjectActionTumble(pTarget,pTarget->Action.Dir,FIXED100(150)*tdir,itofix(-2)))
			{ pTarget->Call(PSF_CatchBlow,&C4AulParSet(C4VInt(punch),
																								 C4VObj(cObj)));
		    return TRUE; }

  // Regular punch
  if (ObjectActionGetPunched(pTarget,FIXED100(250)*tdir,Fix0))
		{ pTarget->Call(PSF_CatchBlow,&C4AulParSet(C4VInt(punch),
																							 C4VObj(cObj)));
			return TRUE; }

	return FALSE;
  }

BOOL ObjectComCancelAttach(C4Object *cObj)
  {
  if (cObj->GetProcedure()==DFA_ATTACH)
    return cObj->SetAction(0);
  return FALSE;
  }

void ObjectComStopDig(C4Object *cObj)
	{
	// Stand
	ObjectActionStand(cObj);
	// Clear digging command
	if (cObj->Command)
		if (cObj->Command->Command == C4CMD_Dig)
			cObj->ClearCommand(cObj->Command);
	}

int32_t ComOrder(int32_t iIndex)
	{
	static BYTE bComOrder[ComOrderNum] =
		{
		COM_Left,	COM_Right, COM_Up, COM_Down, COM_Throw, COM_Dig, COM_Special, COM_Special2,
		COM_Left_S,	COM_Right_S, COM_Up_S, COM_Down_S, COM_Throw_S, COM_Dig_S, COM_Special_S, COM_Special2_S,
		COM_Left_D,	COM_Right_D, COM_Up_D, COM_Down_D, COM_Throw_D, COM_Dig_D, COM_Special_D, COM_Special2_D
		};

	if (Inside<int32_t>(iIndex,0,ComOrderNum-1)) return bComOrder[iIndex];

	return COM_None;
	}

const char *ComName(int32_t iCom)
  {
  switch (iCom)
    {
    case COM_Up:					return "Up";
    case COM_Up_S:				return "UpSingle";
    case COM_Up_D:				return "UpDouble";
    case COM_Up_R:				return "UpReleased";
    case COM_Down:				return "Down";
    case COM_Down_S:			return "DownSingle";
    case COM_Down_D:			return "DownDouble";
    case COM_Down_R:			return "DownReleased";
    case COM_Left:				return "Left";
    case COM_Left_S:			return "LeftSingle";
    case COM_Left_D:			return "LeftDouble";
    case COM_Left_R:			return "LeftReleased";
    case COM_Right:				return "Right";
    case COM_Right_S:			return "RightSingle";
    case COM_Right_D:			return "RightDouble";
    case COM_Right_R:			return "RightReleased";
    case COM_Dig:					return "Dig";
    case COM_Dig_S:				return "DigSingle";
    case COM_Dig_D:				return "DigDouble";
    case COM_Dig_R:				return "DigReleased";
    case COM_Throw:				return "Throw";
    case COM_Throw_S:			return "ThrowSingle";
    case COM_Throw_D:			return "ThrowDouble";
    case COM_Throw_R:			return "ThrowReleased";
    case COM_Special:			return "Special";
    case COM_Special_S:		return "SpecialSingle";
    case COM_Special_D:		return "SpecialDouble";
    case COM_Special_R:     return "SpecialReleased";
    case COM_Special2:		return "Special2";
    case COM_Special2_S:	return "Special2Single";
    case COM_Special2_D:	return "Special2Double";
    case COM_Special2_R:    return "Special2Released";
    case COM_WheelUp:			return "WheelUp";
    case COM_WheelDown:		return "WheelDown";
    }
  return "Undefined";
  }

int32_t Com2Control(int32_t iCom)
  {
	iCom = iCom & ~(COM_Double | COM_Single);
  switch (iCom)
    {
    case COM_CursorLeft:		return CON_CursorLeft;
    case COM_CursorToggle:	return CON_CursorToggle;
    case COM_CursorRight:		return CON_CursorRight;
    case COM_Throw:					return CON_Throw;
    case COM_Up:						return CON_Up;
    case COM_Dig:						return CON_Dig;
    case COM_Left:					return CON_Left;
    case COM_Down:					return CON_Down;
    case COM_Right:					return CON_Right;
    case COM_Special:				return CON_Special;
    case COM_Special2:			return CON_Special2;
    }
	return CON_Menu;
  }

int32_t Control2Com(int32_t iControl, bool fUp)
	{
	static const char con2com[C4MaxKey]=
		{
		COM_CursorLeft, COM_CursorToggle, COM_CursorRight,
		COM_Throw,      COM_Up,           COM_Dig,
		COM_Left,       COM_Down,         COM_Right,
		COM_PlayerMenu, COM_Special,      COM_Special2
		};
	static const char con2com_r[C4MaxKey]=
	  {
		COM_None,    COM_None,   COM_None,
		COM_Throw_R, COM_Up_R,   COM_Dig_R,
		COM_Left_R,  COM_Down_R, COM_Right_R,
		COM_None,    COM_Special_R,   COM_Special2_R
		};
	if (fUp) {
		if (Inside<int32_t>(iControl,0,C4MaxKey-1))
			return con2com_r[iControl];
	} else {
		if (Inside<int32_t>(iControl,0,C4MaxKey-1))
			return con2com[iControl];
	}
	return COM_None;
	}

int32_t Coms2ComDir(int32_t iComs)
	{
	// This is possible because COM_Left - COM_Down are < 32
	static int32_t DirComs = (1 << COM_Left) | (1 << COM_Right) | (1 << COM_Up) | (1 << COM_Down);
	switch (iComs & DirComs)
		{
		case (1 << COM_Up):                      return COMD_Up;
		case (1 << COM_Up) | (1 << COM_Right):   return COMD_UpRight;
		case (1 << COM_Right):                   return COMD_Right;
		case (1 << COM_Down) | (1 << COM_Right): return COMD_DownRight;
		case (1 << COM_Down):                    return COMD_Down;
		case (1 << COM_Down) | (1 << COM_Left):  return COMD_DownLeft;
		case (1 << COM_Left):                    return COMD_Left;
		case (1 << COM_Up) | (1 << COM_Left):    return COMD_UpLeft;
		// up, right and left could be interpreted as COMD_Up etc., but that's too complicated for now
		default:                                 return COMD_Stop;
		}
	}

bool ComDirLike(int32_t iComDir, int32_t iSample)
	{
	if (iComDir == iSample) return true;
	if (iComDir % 8 + 1 == iSample) return true;
	if (iComDir == iSample % 8 + 1 ) return true;
	return false;
	}

void DrawCommandKey(C4Facet &cgo, int32_t iCom, BOOL fPressed, const char *szText)
	{
	// Draw key
	::GraphicsResource.fctKey.Draw(cgo,FALSE,fPressed);
	// Draw control symbol
	if (iCom == COM_PlayerMenu)
		::GraphicsResource.fctOKCancel.Draw(cgo,TRUE,1,1);
	else
		::GraphicsResource.fctCommand.Draw(cgo,TRUE,Com2Control(iCom),((iCom & COM_Double)!=0));
	// Use smaller font on smaller buttons
	CStdFont &rFont = (cgo.Hgt <= C4MN_SymbolSize) ? ::GraphicsResource.FontTiny : ::GraphicsResource.FontRegular;
	// Draw text
	if (szText && Config.Graphics.ShowCommandKeys)
		Application.DDraw->TextOut(szText, rFont, 1.0, cgo.Surface,cgo.X+cgo.Wdt/2,cgo.Y+cgo.Hgt-rFont.iLineHgt-2,CStdDDraw::DEFAULT_MESSAGE_COLOR,ACenter);
	}

void DrawControlKey(C4Facet &cgo, int32_t iControl, BOOL fPressed, const char *szText)
	{
	// Draw key
	::GraphicsResource.fctKey.Draw(cgo,FALSE,fPressed);
	// Draw control symbol
	::GraphicsResource.fctCommand.Draw(cgo,TRUE,iControl);
	// Use smaller font on smaller buttons
	CStdFont &rFont = (cgo.Hgt <= C4MN_SymbolSize) ? ::GraphicsResource.FontRegular : ::GraphicsResource.FontTiny;
	// Draw text
	if (szText)
		Application.DDraw->TextOut(szText, rFont, 1.0, cgo.Surface,cgo.X+cgo.Wdt/2,cgo.Y+cgo.Hgt-rFont.iLineHgt-2,CStdDDraw::DEFAULT_MESSAGE_COLOR,ACenter);
	}

BOOL SellFromBase(int32_t iPlr, C4Object *pBaseObj, C4ID id, C4Object *pSellObj)
  {
  C4Object *pThing;
  // Valid checks
  if (!ValidPlr(iPlr)) return FALSE;
  if (!pBaseObj || !ValidPlr(pBaseObj->Base)) return FALSE;
	if (~Game.C4S.Game.Realism.BaseFunctionality & BASEFUNC_Sell) return FALSE;
  // Base owner eliminated
  if (::Players.Get(pBaseObj->Base)->Eliminated)
    {
    StartSoundEffect("Error",false,100,pBaseObj);
    GameMsgPlayer(FormatString(LoadResStr("IDS_PLR_ELIMINATED"),::Players.Get(pBaseObj->Base)->GetName()).getData(),iPlr);
		return FALSE;
    }
  // Base owner hostile
  if (Hostile(iPlr,pBaseObj->Base))
    {
    StartSoundEffect("Error",false,100,pBaseObj);
    GameMsgPlayer(FormatString(LoadResStr("IDS_PLR_HOSTILE"),::Players.Get(pBaseObj->Base)->GetName()).getData(),iPlr);
		return FALSE;
    }
	// check validity of sell object, if specified
	if (pThing = pSellObj)
		if (!pThing->Status || pThing->Contained != pBaseObj)
			pThing = NULL;
  // Get object from home pBaseObj via selected id, if no or an anvalid thing has been specified
	if (!pThing)
		if (!(pThing=pBaseObj->Contents.Find(id))) return FALSE;
	// check definition NoSell
	if (pThing->Def->NoSell) return FALSE;
  // Sell object (pBaseObj owner gets the money)
  return ::Players.Get(pBaseObj->Base)->Sell2Home(pThing);
  }

BOOL Buy2Base(int32_t iPlr, C4Object *pBase, C4ID id, BOOL fShowErrors)
	{
	C4Object *pThing;
	// Validity
	if (!ValidPlr(iPlr)) return FALSE;
	if (!pBase || !ValidPlr(pBase->Base)) return FALSE;
	if (~Game.C4S.Game.Realism.BaseFunctionality & BASEFUNC_Buy) return FALSE;
	// Base owner hostile
	if (Hostile(iPlr,pBase->Base))
		{
		if (!fShowErrors) return FALSE;
		StartSoundEffect("Error",false,100,pBase);
		GameMsgPlayer(FormatString(LoadResStr("IDS_PLR_HOSTILE"),::Players.Get(pBase->Base)->GetName()).getData(),iPlr); return FALSE;
		}
	// buy
	if (!(pThing=::Players.Get(pBase->Base)->Buy(id, fShowErrors, iPlr, pBase))) return FALSE;
	// Object enter target object
	pThing->Enter(pBase);
	// Success
	return TRUE;
	}

BOOL PlayerObjectCommand(int32_t plr, int32_t cmdf, C4Object *pTarget, int32_t tx, int32_t ty)
	{
	C4Player *pPlr=::Players.Get(plr);
	if (!pPlr) return FALSE;
	int32_t iAddMode = C4P_Command_Set;
	// Adjust for old-style keyboard throw/drop control: add & in range
	if (cmdf==C4CMD_Throw || cmdf==C4CMD_Drop) iAddMode = C4P_Command_Add | C4P_Command_Range;
	if (cmdf==C4CMD_Throw)
		{
		bool fConvertToDrop = false;
		// Drop on down-down-throw (classic)
		if (pPlr->LastComDownDouble)
			{
			fConvertToDrop = true;
			// Dropping one object automatically reenables LastComDownDouble to
			// allow easy dropping of multiple objects. Also set LastCom for
			// script compatibility (custom scripted dropping) and to prevent
			// unwanted ThrowDouble for mass dropping
			pPlr->LastCom = COM_Down | COM_Double;
			pPlr->LastComDownDouble = C4DoubleClick;
			}
		// Jump'n'Run: Drop on combined Down/Left/Right+Throw
		if (pPlr->PrefControlStyle && (pPlr->PressedComs & (1 << COM_Down))) fConvertToDrop = true;
		if (fConvertToDrop) return pPlr->ObjectCommand(C4CMD_Drop,pTarget,tx,ty,NULL,C4VNull,iAddMode);
		}
	// Route to player
	return pPlr->ObjectCommand(cmdf,pTarget,tx,ty,NULL,C4VNull,iAddMode);
	}
