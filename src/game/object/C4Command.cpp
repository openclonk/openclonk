/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2003-2005, 2008  Matthes Bender
 * Copyright (c) 2001-2008  Sven Eberhardt
 * Copyright (c) 2001  Michael Käser
 * Copyright (c) 2002, 2004, 2006  Peter Wortmann
 * Copyright (c) 2004-2006, 2008-2009  Günther Brammer
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

/* The command stack controls an object's complex and independent behavior */

#include <C4Include.h>
#include <C4Command.h>

#ifndef BIG_C4INCLUDE
#include <C4Object.h>
#include <C4ObjectCom.h>
#include <C4ObjectInfo.h>
#include <C4Random.h>
#include <C4GameMessage.h>
#include <C4ObjectMenu.h>
#include <C4Player.h>
#include <C4Landscape.h>
#include <C4Game.h>
#include <C4PlayerList.h>
#include <C4GameObjects.h>
#endif

const int32_t MoveToRange=5,LetGoRange1=7,LetGoRange2=30,DigRange=1;
const int32_t FollowRange=6,PushToRange=10,DigOutPositionRange=15;
const int32_t PathRange=20,MaxPathRange=1000;
const int32_t JumpAngle=35,JumpLowAngle=80,JumpAngleRange=10,JumpHighAngle=0;
const int32_t FlightAngleRange=60;
const int32_t DigOutDirectRange=130;
const int32_t LetGoHangleAngle=110;

StdEnumAdapt<int32_t>::Entry EnumAdaptCommandEntries[C4CMD_Last - C4CMD_First + 2];

const char *CommandName(int32_t iCommand)
  {
	static const char *szCommandName[] = {
		"None","Follow","MoveTo","Enter","Exit","Grab","Build","Throw","Chop",
		"UnGrab","Jump","Wait","Get","Put","Drop","Dig","Activate","PushTo",
		"Construct","Transfer","Attack","Context","Buy","Sell","Acquire",
		"Energy","Retry","Home","Call","Take","Take2" };

	if (!Inside<int32_t>(iCommand,C4CMD_First,C4CMD_Last)) return "None";

	return szCommandName[iCommand];
	}

const char* CommandNameID(int32_t iCommand)
  {
	static const char* dwCommandNameID[] = {
		"IDS_COMM_NONE","IDS_COMM_FOLLOW","IDS_COMM_MOVETO","IDS_COMM_ENTER",
		"IDS_COMM_EXIT","IDS_COMM_GRAB","IDS_COMM_BUILD","IDS_COMM_THROW","IDS_COMM_CHOP",
		"IDS_COMM_UNGRAB","IDS_COMM_JUMP","IDS_COMM_WAIT","IDS_COMM_GET","IDS_COMM_PUT",
		"IDS_COMM_DROP","IDS_COMM_DIG","IDS_COMM_ACTIVATE","IDS_COMM_PUSHTO",
		"IDS_COMM_CONSTRUCT","IDS_COMM_TRANSFER","IDS_COMM_ATTACK","IDS_COMM_CONTEXT",
		"IDS_COMM_BUY","IDS_COMM_SELL","IDS_COMM_ACQUIRE","IDS_COMM_ENERGY","IDS_COMM_RETRY",
		"IDS_CON_HOME","IDS_COMM_CALL","IDS_COMM_TAKE","IDS_COMM_TAKE2" };

	if (!Inside<int32_t>(iCommand, C4CMD_First, C4CMD_Last)) return "IDS_COMM_NONE";

	return dwCommandNameID[iCommand];
	}

bool InitEnumAdaptCommandEntries()
  {
  for(int32_t i = C4CMD_First; i <= C4CMD_Last; i++)
    {
    EnumAdaptCommandEntries[i - C4CMD_First].Name = CommandName(i);
    EnumAdaptCommandEntries[i - C4CMD_First].Val = i;
    }
  EnumAdaptCommandEntries[C4CMD_Last - C4CMD_First + 1].Name = NULL;
  return true;
  }
const bool InitEnumAdaptCommandEntriesDummy = InitEnumAdaptCommandEntries();

int32_t CommandByName(const char *szCommand)
	{
	for (int32_t cnt=C4CMD_First; cnt<=C4CMD_Last; cnt++)
		if (SEqual(szCommand,CommandName(cnt)))
			return cnt;
	return C4CMD_None;
	}

void AdjustMoveToTarget(int32_t &rX, int32_t &rY, BOOL fFreeMove, int32_t iShapeHgt)
	{
	// Above solid (always)
	int32_t iY;
	for (iY=rY; (iY>=0) && GBackSolid(rX,iY); iY--) {}
	if (iY>=0) rY=iY;
	// No-free-move adjustments (i.e. if walking)
	if (!fFreeMove)
		{
		// Drop down to bottom of free space
		if (!GBackSemiSolid(rX,rY))
			{
			for (iY=rY; (iY<GBackHgt) && !GBackSemiSolid(rX,iY+1); iY++) {}
			if (iY<GBackHgt) rY=iY;
			}
		// Vertical shape offset above solid
		if (GBackSolid(rX,rY+1) || GBackSolid(rX,rY+5))
			if (!GBackSemiSolid(rX,rY-iShapeHgt/2))
				rY-=iShapeHgt/2;
		}

	}

BOOL FreeMoveTo(C4Object *cObj)
	{
	// Floating: we accept any move-to target
	if (cObj->GetProcedure()==DFA_FLOAT) return TRUE;
	// Can fly: we accept any move-to target
	if (cObj->GetPhysical()->CanFly) return TRUE;
	// Assume we're walking: move-to targets are adjusted
	return FALSE;
	}

BOOL AdjustSolidOffset(int32_t &rX, int32_t &rY, int32_t iXOff, int32_t iYOff)
	{
	// In solid: fail
	if (GBackSolid(rX,rY)) return FALSE;
	// Y Offset
	int32_t cnt;
	for (cnt=1; cnt<iYOff; cnt++)
		{
		if (GBackSolid(rX,rY+cnt) && !GBackSolid(rX,rY-cnt)) rY--;
		if (GBackSolid(rX,rY-cnt) && !GBackSolid(rX,rY+cnt)) rY++;
		}
	// X Offset
	for (cnt=1; cnt<iXOff; cnt++)
		{
		if (GBackSolid(rX+cnt,rY) && !GBackSolid(rX-cnt,rY)) rX--;
		if (GBackSolid(rX-cnt,rY) && !GBackSolid(rX+cnt,rY)) rX++;
		}
	// Done
	return TRUE;
	}

int32_t SolidOnWhichSide(int32_t iX, int32_t iY)
	{
	for (int32_t cx=1; cx<10; cx++)
		for (int32_t cy=0; cy<10; cy++)
			{
			if (GBackSolid(iX-cx,iY-cy) || GBackSolid(iX-cx,iY+cy)) return -1;
			if (GBackSolid(iX+cx,iY-cy) || GBackSolid(iX+cx,iY+cy)) return +1;
			}
	return 0;
	}

C4Command::C4Command()
  {
	Default();
  }

C4Command::~C4Command()
  {
	Clear();
  }

void C4Command::Default()
	{
  Command=C4CMD_None;
	cObj=NULL;
  Evaluated=FALSE;
	PathChecked=FALSE;
	Finished=FALSE;
	Tx=C4VNull;
	Ty=0;
  Target=Target2=NULL;
	Data.Set0();
  UpdateInterval=0;
	Failures=0;
	Retries=0;
	Permit=0;
	Text=NULL;
  Next=NULL;
	iExec=0;
	BaseMode=C4CMD_Mode_SilentSub;
	}

static bool ObjectAddWaypoint(int32_t iX, int32_t iY, intptr_t iTransferTarget, intptr_t ipObject)
	{
	C4Object *cObj = (C4Object*) ipObject; if (!cObj) return FALSE;

	// Transfer waypoint
	if (iTransferTarget)
		return cObj->AddCommand(C4CMD_Transfer,(C4Object*)iTransferTarget,iX,iY,0,NULL,FALSE);

  // Solid offset
	AdjustSolidOffset(iX,iY,cObj->Shape.Wdt/2,cObj->Shape.Hgt/2);

	// Standard movement waypoint update interval
	int32_t iUpdate = 25;
	// Waypoints before transfer zones are not updated (enforce move to that waypoint)
	if (cObj->Command && (cObj->Command->Command==C4CMD_Transfer)) iUpdate=0;
	// Add waypoint
	//AddCommand(iCommand,pTarget,iTx,iTy,iUpdateInterval,pTarget2,fInitEvaluation,iData,fAppend,iRetries,szText,iBaseMode)
	assert(cObj->Command);
	if (!cObj->AddCommand(C4CMD_MoveTo,NULL,iX,iY,25,NULL,FALSE,cObj->Command->Data)) return FALSE;

	return TRUE;
	}

void C4Command::MoveTo()
  {
	// Determine move-to range
	int32_t iMoveToRange = MoveToRange;
	if (cObj->Def->MoveToRange > 0) iMoveToRange = cObj->Def->MoveToRange;

	// Current object position
  int32_t cx,cy; cx=cObj->GetX(); cy=cObj->GetY();
	BOOL fWaypoint=FALSE;
	if (Next && (Next->Command==C4CMD_MoveTo)) fWaypoint=TRUE;

  // Contained: exit
  if (cObj->Contained)
    { cObj->AddCommand(C4CMD_Exit,NULL,0,0,50); return; }

	// Check path (crew members or specific only)
	if ((cObj->OCF & OCF_CrewMember) || cObj->Def->Pathfinder)
		if (!PathChecked)
			// Not too far
			if (Distance(cx,cy,Tx._getInt(),Ty)<MaxPathRange)
			// Not too close
			if (!(Inside(cx-Tx._getInt(),-PathRange,+PathRange) && Inside(cy-Ty,-PathRange,+PathRange)))
				{
				// Path not free: find path
				if (!PathFree(cx,cy,Tx._getInt(),Ty))
					{
					Game.PathFinder.EnableTransferZones(!cObj->Def->NoTransferZones);
					Game.PathFinder.SetLevel(cObj->Def->Pathfinder);
					if (!Game.PathFinder.Find( cObj->GetX(),cObj->GetY(),
																		 Tx._getInt(),Ty,
																		 &ObjectAddWaypoint,
																		 (intptr_t)cObj)) // intptr for 64bit?
						{ /* Path not found: react? */ PathChecked=TRUE; /* recheck delay */ }
					return;
					}
				// Path free: recheck delay
				else
					PathChecked=TRUE;
				}
	// Path recheck
	if (!::Game.iTick35) PathChecked=FALSE;

	// Pushing grab only or not desired: let go (pulling, too?)
  if (cObj->GetProcedure()==DFA_PUSH)
    if (cObj->Action.Target)
			if (cObj->Action.Target->Def->Grab == 2 || !(Data.getInt() & C4CMD_MoveTo_PushTarget))
		    {
				// Re-evaluate this command because vehicle control might have blocked evaluation
				Evaluated=FALSE;
				cObj->AddCommand(C4CMD_UnGrab,NULL,0,0,50); return;
				}

	// Special by procedure
	switch (cObj->GetProcedure())
		{
		// Push/pull
		case DFA_PUSH: case DFA_PULL:
			// Use target object's position if on final waypoint
			if (!fWaypoint)
				if (cObj->Action.Target)
					{ cx=cObj->Action.Target->GetX(); cy=cObj->Action.Target->GetY(); }
			break;
		// Chop, build, dig, bridge: stop
		case DFA_CHOP: case DFA_BUILD: case DFA_DIG: case DFA_BRIDGE:
			ObjectComStop(cObj);
			break;
		}

	// Target range
	int32_t iTargetRange = iMoveToRange;
	int32_t iRangeFactorTop=1, iRangeFactorBottom=1, iRangeFactorSide=1;

	// Crew members/pathfinder specific target range
	if (cObj->OCF & OCF_CrewMember) // || cObj->Def->Pathfinder ? (Sven2)
		{
		// Range by size
		iTargetRange=cObj->Shape.Wdt/5;
		// Easier range if waypoint
		if (fWaypoint)
			if (cObj->GetProcedure()!=DFA_SCALE)
				{ iRangeFactorTop=3; iRangeFactorSide=3; iRangeFactorBottom=2; }
		}

  // Target reached (success)
  if (Inside(cx-Tx._getInt(),-iRangeFactorSide*iTargetRange,+iRangeFactorSide*iTargetRange)
	 && Inside(cy-Ty,-iRangeFactorBottom*iTargetRange,+iRangeFactorTop*iTargetRange))
    {
		cObj->Action.ComDir=COMD_Stop;
		Finish(TRUE); return;
    }

  // Idles can't move to
  if (!cObj->Action.pActionDef)
    { Finish(); return; }

  // Action
  switch (cObj->GetProcedure())
    {
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case DFA_WALK:
      // Head to target
      if (cx<Tx._getInt()-iTargetRange) cObj->Action.ComDir=COMD_Right;
      if (cx>Tx._getInt()+iTargetRange) cObj->Action.ComDir=COMD_Left;
			// Flight control
			if (FlightControl()) return;
			// Jump control
			if (JumpControl()) return;
      break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case DFA_PUSH: case DFA_PULL:
      // Head to target
      if (cx<Tx._getInt()-iTargetRange) cObj->Action.ComDir=COMD_Right;
      if (cx>Tx._getInt()+iTargetRange) cObj->Action.ComDir=COMD_Left;
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case DFA_SCALE:
      // Head to target
      if (cy>Ty+iTargetRange) cObj->Action.ComDir=COMD_Up;
      if (cy<Ty-iTargetRange) cObj->Action.ComDir=COMD_Down;
      // Let-Go Control
      if (cObj->Action.Dir==DIR_Left)
				{
				// Target direction
        if ((Tx._getInt()>cx+LetGoRange1) && (Inside(cy-Ty,-LetGoRange2,+LetGoRange2)))
          { ObjectComLetGo(cObj,+1); return; }
				// Contact (not if just started)
				if (cObj->Action.Time>2)
					if (cObj->t_contact/* & CNAT_Left*/)
						{ ObjectComLetGo(cObj,+1); return; }
				}
      if (cObj->Action.Dir==DIR_Right)
				{
				// Target direction
        if ((Tx._getInt()<cx-LetGoRange1) && (Inside(cy-Ty,-LetGoRange2,+LetGoRange2)))
          { ObjectComLetGo(cObj,-1); return; }
				// Contact (not if just started)
				if (cObj->Action.Time>2)
					if (cObj->t_contact/* & CNAT_Right*/)
		        { ObjectComLetGo(cObj,-1); return; }
				}
      break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case DFA_SWIM:
      // Head to target
      if (::Game.iTick2)
        { if (cx<Tx._getInt()-iTargetRange) cObj->Action.ComDir=COMD_Right;
          if (cx>Tx._getInt()+iTargetRange) cObj->Action.ComDir=COMD_Left;  }
      else
        { if (cy<Ty) cObj->Action.ComDir=COMD_Down;
          if (cy>Ty) cObj->Action.ComDir=COMD_Up;    }
      break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case DFA_HANGLE:
      // Head to target
      if (cx<Tx._getInt()-iTargetRange) cObj->Action.ComDir=COMD_Right;
      if (cx>Tx._getInt()+iTargetRange) cObj->Action.ComDir=COMD_Left;
      // Let-Go Control
			if (Abs(Angle(cx,cy,Tx._getInt(),Ty))>LetGoHangleAngle)
	      ObjectComLetGo(cObj,0);
      break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case DFA_FLOAT:
			{
			FIXED dx = itofix(Tx._getInt()) - cObj->fix_x, dy = itofix(Ty) - cObj->fix_y;
			// normalize
			FIXED dScale = FIXED100(cObj->GetPhysical()->Float) / Max(Abs(dx), Abs(dy));
			dx *= dScale; dy *= dScale;
			// difference to momentum
			dx -= cObj->xdir; dy -= cObj->ydir;
			// steer
			if(Abs(dx)+Abs(dy) < FIXED100(20)) cObj->Action.ComDir = COMD_None;
			else if(Abs(dy) * 3 <  dx) cObj->Action.ComDir = COMD_Right;
			else if(Abs(dy) * 3 < -dx) cObj->Action.ComDir = COMD_Left;
			else if(Abs(dx) * 3 <  dy) cObj->Action.ComDir = COMD_Down;
			else if(Abs(dx) * 3 < -dy) cObj->Action.ComDir = COMD_Up;
			else if(dx > 0 && dy > 0)		cObj->Action.ComDir = COMD_DownRight;
			else if(dx < 0 && dy > 0)		cObj->Action.ComDir = COMD_DownLeft;
			else if(dx > 0 && dy < 0)		cObj->Action.ComDir = COMD_UpRight;
			else												cObj->Action.ComDir = COMD_UpLeft;
			}
      break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case DFA_FLIGHT:
			// Flight control
			if (FlightControl()) return;
      break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    }

  }

void C4Command::Dig()
  {
	// Current object and target coordinates
  int32_t cx,cy,tx,ty;
  cx=cObj->GetX(); cy=cObj->GetY();
  tx=Tx._getInt(); ty=Ty+cObj->Shape.GetY() + 3; // Target coordinates are bottom center
	BOOL fDigOutMaterial=Data.getBool();

  // Grabbing: let go
  if (cObj->GetProcedure()==DFA_PUSH)
    { cObj->AddCommand(C4CMD_UnGrab,NULL,0,0,50); return; }

  // If contained: exit
  if (cObj->Contained)
    { cObj->AddCommand(C4CMD_Exit,NULL,0,0,50); return; }

  // Building or chopping: stop
  if ((cObj->GetProcedure()==DFA_CHOP) || (cObj->GetProcedure()==DFA_BUILD))
    ObjectComStop(cObj);

	// Scaling or hangling: let go
	if ((cObj->GetProcedure()==DFA_SCALE) || (cObj->GetProcedure()==DFA_HANGLE))
		ObjectComLetGo(cObj,(cObj->Action.Dir==DIR_Left) ? +1 : -1);

	// Determine move-to range
	int32_t iMoveToRange = MoveToRange;
	if (cObj->Def->MoveToRange > 0) iMoveToRange = cObj->Def->MoveToRange;

  // Target reached: success
  if (Inside(cx - tx, -iMoveToRange, +iMoveToRange)
   && Inside(cy - ty, -iMoveToRange, +iMoveToRange))
    {
		ObjectComStop(cObj);
    Finish(TRUE); return;
    }

	// Can start digging only from walk
	if (cObj->GetProcedure()!=DFA_DIG)
		if (cObj->GetProcedure()!=DFA_WALK)
			// Continue trying (assume currently in flight)
			return;

	// Start digging
	if (cObj->GetProcedure()!=DFA_DIG)
		if (!ObjectComDig(cObj))
			{ Finish(); return; }

	// Dig2Object
	if (fDigOutMaterial) cObj->Action.Data=1;

  // Head to target
  if (cx<tx-DigRange) cObj->Action.ComDir=COMD_Right;
  if (cx>tx+DigRange) cObj->Action.ComDir=COMD_Left;
  if (cy<ty-DigRange) cObj->Action.ComDir=COMD_Down;
	if (cx<tx-DigRange) if (cy<ty-DigRange) cObj->Action.ComDir=COMD_DownRight;
	if (cx>tx-DigRange) if (cy<ty-DigRange) cObj->Action.ComDir=COMD_DownLeft;
	if (cx<tx-DigRange) if (cy>ty+DigRange) cObj->Action.ComDir=COMD_UpRight;
	if (cx>tx+DigRange) if (cy>ty+DigRange) cObj->Action.ComDir=COMD_UpLeft;

	}


void C4Command::Follow()
  {

  // If crew member, only selected objects can follow
	if (cObj->Def->CrewMember)
		// Finish successfully to avoid fail message
		if (!cObj->Select && cObj->Owner != NO_OWNER) { Finish(TRUE); return; }

  // No-one to follow
  if (!Target) { Finish(); return; }

  // Follow containment
  if (cObj->Contained!=Target->Contained)
    {
    // Only crew members can follow containment
		if (!cObj->Def->CrewMember)
			{ Finish(); return; }
		// Exit/enter
    if (cObj->Contained) cObj->AddCommand(C4CMD_Exit,NULL,0,0,50);
    else cObj->AddCommand(C4CMD_Enter,Target->Contained,0,0,50);
    return;
    }

  // Follow target grabbing
  if (Target->GetProcedure()==DFA_PUSH)
    {
    // Grabbing same: copy ComDir
    if (cObj->GetProcedure()==DFA_PUSH)
      if (cObj->Action.Target==Target->Action.Target)
        {
        cObj->Action.ComDir=Target->Action.ComDir;
        return;
        }
    // Else, grab target's grab
    cObj->AddCommand(C4CMD_Grab,Target->Action.Target);
    return;
    }
	else if (cObj->GetProcedure()==DFA_PUSH)
		{
		// Follow ungrabbing
		cObj->AddCommand(C4CMD_UnGrab);
		return;
		}

  // If in following range
  if (Inside<int32_t>(cObj->GetX()-Target->GetX(),-FollowRange,+FollowRange)
   && Inside<int32_t>(cObj->GetY()-Target->GetY(),-FollowRange,+FollowRange))
    {
    // Copy target's Action.ComDir
    cObj->Action.ComDir=Target->Action.ComDir;
    }
  else // Else, move to target
    {
    cObj->AddCommand(C4CMD_MoveTo,NULL,Target->GetX(),Target->GetY(),10);
    }

  }

void C4Command::Enter()
  {
  DWORD ocf;

  // No object to enter or can't enter by def
  if (!Target || cObj->Def->NoPushEnter) { Finish(); return; }

  // Already in target object
  if (cObj->Contained==Target) { Finish(TRUE); return; }

  // Building or chopping: stop
  if ((cObj->GetProcedure()==DFA_CHOP) || (cObj->GetProcedure()==DFA_BUILD))
    ObjectComStop(cObj);

	// Digging: stop
	if (cObj->GetProcedure()==DFA_DIG) ObjectComStop(cObj);

	// Pushing grab only or pushing not desired: let go
  if (cObj->GetProcedure()==DFA_PUSH)
    if (cObj->Action.Target)
			if (cObj->Action.Target->Def->Grab == 2 || !(Data.getInt() & C4CMD_Enter_PushTarget))
		    { cObj->AddCommand(C4CMD_UnGrab,NULL,0,0,50); return; }

	// Pushing target: let go
  if (cObj->GetProcedure()==DFA_PUSH)
    if (cObj->Action.Target==Target)
	    { cObj->AddCommand(C4CMD_UnGrab,NULL,0,0,50); return; }

  // Grabbing overrides position for target
  int32_t cx,cy;
  cx=cObj->GetX(); cy=cObj->GetY();
  if (cObj->GetProcedure()==DFA_PUSH)
    if (cObj->Action.Target)
      { cx=cObj->Action.Target->GetX(); cy=cObj->Action.Target->GetY(); }

  // If in entrance range
  ocf=OCF_Entrance;
  if (!cObj->Contained && Target->At(cx,cy,ocf) && (ocf & OCF_Entrance))
    {
		// Stop
    cObj->Action.ComDir=COMD_Stop;
    // Pushing: push target into entrance, then stop
    if ((cObj->GetProcedure()==DFA_PUSH) && cObj->Action.Target)
      {
      cObj->Action.Target->SetCommand(C4CMD_Enter,Target);
      Finish(TRUE); return;
      }
    // Else, enter self
    else
      {
      // If entrance open, enter object
      if (Target->EntranceStatus!=0)
        {
        cObj->Enter(Target);
        Finish(TRUE); return;
        }
      else // Else, activate entrance
        Target->ActivateEntrance(cObj->Controller,cObj);
      }

    }
  else // Else, move to object's entrance
    {
    int32_t ex,ey,ewdt,ehgt;
    if (Target->GetEntranceArea(ex,ey,ewdt,ehgt))
			cObj->AddCommand(C4CMD_MoveTo,NULL,ex+ewdt/2,ey+ehgt/2,50, NULL, true, C4VInt((Data.getInt() & C4CMD_Enter_PushTarget) ? C4CMD_MoveTo_PushTarget : 0));
    }

  }

void C4Command::Exit()
  {

  // Outside: done
  if (!cObj->Contained) { Finish(TRUE); return; }

  // Building: stop
  if (cObj->GetProcedure()==DFA_BUILD)
    ObjectComStop(cObj);

  // Entrance open, leave object
  if (cObj->Contained->EntranceStatus)
    {
		// Exit to container's container
		if (cObj->Contained->Contained)
			{ cObj->Enter(cObj->Contained->Contained); Finish(TRUE); return; }
		// Exit to entrance area
    int32_t ex,ey,ewdt,ehgt;
		if (cObj->Contained->OCF & OCF_Entrance)
			if (cObj->Contained->GetEntranceArea(ex,ey,ewdt,ehgt))
				{ cObj->Exit(ex+ewdt/2,ey+ehgt+cObj->Shape.GetY()-1); Finish(TRUE); return; }
		// Exit jump out of collection area
		if (cObj->GetPropertyInt(P_Collectible))
			if (cObj->Contained->Def->Collection.Wdt)
				{
				cObj->Exit(cObj->Contained->GetX(),cObj->Contained->GetY()+cObj->Contained->Def->Collection.y-1);
				ObjectComJump(cObj);
				Finish(TRUE); return;
				}
		// Plain exit
    cObj->Exit(cObj->GetX(),cObj->GetY());
		Finish(TRUE); return;
    }

	// Entrance closed, activate entrance
  else
    {
    if (!cObj->Contained->ActivateEntrance(cObj->Controller,cObj))
			// Entrance activation failed: fail
      { Finish(); return; }
    }
  }

void C4Command::Grab()
  {
  DWORD ocf;
  // Command fulfilled
  if (cObj->GetProcedure()==DFA_PUSH)
    if (cObj->Action.Target==Target)
      { Finish(TRUE); return; }
  // Building or chopping: stop
  if ((cObj->GetProcedure()==DFA_CHOP) || (cObj->GetProcedure()==DFA_BUILD))
    ObjectComStop(cObj);
	// Digging: stop
	if (cObj->GetProcedure()==DFA_DIG) ObjectComStop(cObj);
  // Grabbing: let go
  if (cObj->GetProcedure()==DFA_PUSH)
    { cObj->AddCommand(C4CMD_UnGrab,NULL,0,0,50); return; }
  // No target
  if (!Target) { Finish(); return; }
  // At target object: grab
  ocf=OCF_All;
  if (!cObj->Contained && Target->At(cObj->GetX(),cObj->GetY(),ocf))
    {
		// Scaling or hangling: let go
		if ((cObj->GetProcedure()==DFA_SCALE) || (cObj->GetProcedure()==DFA_HANGLE))
			ObjectComLetGo(cObj,(cObj->Action.Dir==DIR_Left) ? +1 : -1);
		// Grab
    cObj->Action.ComDir=COMD_Stop;
    ObjectComGrab(cObj,Target);
    }
  // Else, move to object
	else
    {
    cObj->AddCommand(C4CMD_MoveTo,NULL,Target->GetX()+Tx._getInt(),Target->GetY()+Ty,50);
    }
  }

void C4Command::PushTo()
  {
	// Target check
	if (!Target) { Finish(); return; }

	// Target is target self: fail
	if (Target==Target2) { Finish(); return; }

  // Command fulfilled
	if (Target2)
		{
		// Object in correct target container: success
		if (Target->Contained==Target2)
			{ Finish(TRUE); return; }
		}
	else
		{
		// Object at target position: success
		if (Inside(Target->GetX()-Tx._getInt(),-PushToRange,+PushToRange))
			if (Inside(Target->GetY()-Ty,-PushToRange,+PushToRange))
				{
		    cObj->Action.ComDir=COMD_Stop;
				cObj->AddCommand(C4CMD_UnGrab);
				cObj->AddCommand(C4CMD_Wait,NULL,0,0,10);
				Finish(TRUE);	return;
				}
		}

  // Building or chopping: stop
  if ((cObj->GetProcedure()==DFA_CHOP) || (cObj->GetProcedure()==DFA_BUILD))
    ObjectComStop(cObj);

	// Digging: stop
	if (cObj->GetProcedure()==DFA_DIG) ObjectComStop(cObj);

	// Target contained: activate
	if (Target->Contained)
		{ cObj->AddCommand(C4CMD_Activate,Target,0,0,40); return; }

	// Grab target
	if (!((cObj->GetProcedure()==DFA_PUSH) && (cObj->Action.Target==Target)))
		{	cObj->AddCommand(C4CMD_Grab,Target,0,0,40); return; }

	// Move to target position / enter target object
	if (Target2)
		{	cObj->AddCommand(C4CMD_Enter,Target2,0,0,40, NULL, true, C4Value(C4CMD_Enter_PushTarget)); return; }
	else
		{	cObj->AddCommand(C4CMD_MoveTo,NULL,Tx,Ty,40, NULL, true, C4Value(C4CMD_MoveTo_PushTarget)); return; }

	}

void C4Command::Chop()
  {
  DWORD ocf;
  // No target: fail
  if (!Target) { Finish(); return; }
  // Can not chop: fail
  if (!cObj->GetPhysical()->CanChop)
    { Finish(); return; }
  // Target not chopable: done (assume was successfully chopped)
	if (!(Target->OCF & OCF_Chop))
    { Finish(TRUE); return; }
  // Chopping target: wait
  if ((cObj->GetProcedure()==DFA_CHOP) && (cObj->Action.Target==Target))
    return;
  // Grabbing: let go
  if (cObj->GetProcedure()==DFA_PUSH)
    { cObj->AddCommand(C4CMD_UnGrab,NULL,0,0,50); return; }
	// Building, digging or chopping other: stop
	if ((cObj->GetProcedure()==DFA_DIG) || (cObj->GetProcedure()==DFA_CHOP) || (cObj->GetProcedure()==DFA_BUILD))
		{ ObjectComStop(cObj); return; }
  // At target object, in correct shopping position
  ocf=OCF_All;
  if (!cObj->Contained && Target->At(cObj->GetX(),cObj->GetY(),ocf) && Inside<int32_t>(Abs(cObj->GetX() - Target->GetX()), 4, 9))
    {
    cObj->Action.ComDir=COMD_Stop;
    ObjectComChop(cObj,Target);
    }
  // Else, move to object
	else
    {
    cObj->AddCommand(C4CMD_MoveTo, NULL, (cObj->GetX() > Target->GetX()) ? Target->GetX()+6 : Target->GetX()-6, Target->GetY(), 50);
		// Too close? Move away first.
		if (Abs(cObj->GetX() - Target->GetX()) < 5)
	    cObj->AddCommand(C4CMD_MoveTo, NULL, (cObj->GetX() > Target->GetX()) ? Target->GetX()+15 : Target->GetX()-15, Target->GetY(), 50);
    }
  }

void C4Command::Build()
  {
  DWORD ocf;
  // No target: cancel
  if (!Target)
		{ Finish(); return; }
	// Lost the ability to build? Fail.
	if (cObj->GetPhysical() && !cObj->GetPhysical()->CanConstruct)
		{
		Finish(FALSE, FormatString(LoadResStr("IDS_TEXT_CANTBUILD"), cObj->GetName()).getData());
		return;
		}
  // Target complete: Command fulfilled
  if (Target->GetCon()>=FullCon)
		{
		// Activate internal vehicles
		if (Target->Contained && (Target->Category & C4D_Vehicle))
			cObj->AddCommand(C4CMD_Activate,Target);
		// Energy supply (if necessary and nobody else is doing so already)
		if (Game.Rules & C4RULE_StructuresNeedEnergy)
			if (Target->Def->LineConnect & C4D_Power_Input)
				if (!Game.FindObjectByCommand(C4CMD_Energy,Target))
					{
					// if another Clonk is also building this structure and carries a linekit already, that Clonk should rather perform the energy command
					C4Object *pOtherBuilder = NULL;
					if (!cObj->Contents.Find(C4ID_Linekit))
						{
						while (pOtherBuilder = Game.FindObjectByCommand(C4CMD_Build,Target, C4VNull,0, NULL, pOtherBuilder))
							if (pOtherBuilder->Contents.Find(C4ID_Linekit))
								break;
						}
					if (!pOtherBuilder)
						cObj->AddCommand(C4CMD_Energy,Target);
					}
		// Done
		cObj->Action.ComDir=COMD_Stop;
		Finish(TRUE); return;
		}
  // Currently working on target: continue
  if (cObj->GetProcedure()==DFA_BUILD)
    if (cObj->Action.Target==Target)
      return;
  // Grabbing: let go
  if (cObj->GetProcedure()==DFA_PUSH)
    { cObj->AddCommand(C4CMD_UnGrab,NULL,0,0,50); return; }
	// Digging: stop
	if (cObj->GetProcedure()==DFA_DIG) ObjectComStop(cObj);
  // Worker ist structure or static back: internal target build only (old stuff)
  if ((cObj->Category & C4D_Structure) || (cObj->Category & C4D_StaticBack))
    {
    // Target is internal
    if (Target->Contained==cObj)
      { ObjectComBuild(cObj,Target); return; }
    // Target is not internal: cancel
    Finish(); return;
    }
  // At target check
  ocf=OCF_All;
  if ( (Target->Contained && (cObj->Contained==Target->Contained))
		|| (Target->At(cObj->GetX(),cObj->GetY(),ocf) && (cObj->GetProcedure()==DFA_WALK)) )
    {
    ObjectComStop(cObj);
    ObjectComBuild(cObj,Target);
    return;
    }
  // Else, move to object
  else
    {
		if (Target->Contained) cObj->AddCommand(C4CMD_Enter,Target->Contained,0,0,50);
    else cObj->AddCommand(C4CMD_MoveTo,NULL,Target->GetX(),Target->GetY(),50);
    return;
    }
  }

void C4Command::UnGrab()
  {
  ObjectComUnGrab(cObj);
  cObj->Action.ComDir=COMD_Stop;
  Finish(TRUE);
  }

void C4Command::Throw()
  {

	// Digging: stop
	if (cObj->GetProcedure()==DFA_DIG) ObjectComStop(cObj);

	// Throw specific object not in contents: get object
	if (Target)
		if (!cObj->Contents.GetLink(Target))
			{
			cObj->AddCommand(C4CMD_Get,Target,0,0,40);
			return;
			}

	// Determine move-to range
	int32_t iMoveToRange = MoveToRange;
	if (cObj->Def->MoveToRange > 0) iMoveToRange = cObj->Def->MoveToRange;

	// Target coordinates are not default 0/0: targeted throw
	if ((Tx._getInt()!=0) || (Ty!=0))
		{

		// Grabbing: let go
		if (cObj->GetProcedure()==DFA_PUSH)
			{ cObj->AddCommand(C4CMD_UnGrab,NULL,0,0,50); return; }

		// Preferred throwing direction
		int32_t iDir=+1; if (cObj->GetX() > Tx._getInt()) iDir=-1;

		// Find throwing position
		int32_t iTx,iTy;
	  FIXED pthrow=ValByPhysical(400, cObj->GetPhysical()->Throw);
		int32_t iHeight = cObj->Shape.Hgt;
		if (!FindThrowingPosition(Tx._getInt(),Ty,pthrow*iDir,-pthrow,iHeight,iTx,iTy))
			if (!FindThrowingPosition(Tx._getInt(),Ty,pthrow*iDir*-1,-pthrow,iHeight,iTx,iTy))
				// No throwing position: fail
				{ Finish(); return; }

		// At throwing position: face target and throw
		if (Inside<int32_t>(cObj->GetX() - iTx, -iMoveToRange, +iMoveToRange) && Inside<int32_t>(cObj->GetY()-iTy,-15,+15))
			{
			if (cObj->GetX() < Tx._getInt()) cObj->SetDir(DIR_Right); else cObj->SetDir(DIR_Left);
			cObj->Action.ComDir=COMD_Stop;
			if (ObjectComThrow(cObj,Target))
				Finish(TRUE); // Throw successfull: done, else continue
			return;
			}

		// Move to target position
		cObj->AddCommand(C4CMD_MoveTo,NULL,iTx,iTy,20);

		return;
		}

  // Contained: put or take
  if (cObj->Contained)
    {
    ObjectComPutTake(cObj,cObj->Contained,Target);
    Finish(TRUE); return;
    }

  // Pushing: put or take
  if (cObj->GetProcedure()==DFA_PUSH)
    {
    if (cObj->Action.Target)
      ObjectComPutTake(cObj,cObj->Action.Target,Target);
    Finish(TRUE); return;
    }

  // Outside: Throw
  ObjectComThrow(cObj,Target);
  Finish(TRUE);
  }

void C4Command::Take()
{
	ObjectComTake(cObj);
	Finish(TRUE);
}

void C4Command::Take2()
{
	ObjectComTake2(cObj);
	Finish(TRUE);
}

void C4Command::Drop()
  {

	// Digging: stop
	if (cObj->GetProcedure()==DFA_DIG) ObjectComStop(cObj);

	// Drop specific object not in contents: get object
	if (Target)
		if (!cObj->Contents.GetLink(Target))
			{
			cObj->AddCommand(C4CMD_Get,Target,0,0,40);
			return;
			}

	// Determine move-to range
	int32_t iMoveToRange = MoveToRange;
	if (cObj->Def->MoveToRange > 0) iMoveToRange = cObj->Def->MoveToRange;

	// Target coordinates are not default 0/0: targeted drop
	if ((Tx._getInt()!=0) || (Ty!=0))
		{
		// Grabbing: let go
		if (cObj->GetProcedure()==DFA_PUSH)
			{ cObj->AddCommand(C4CMD_UnGrab,NULL,0,0,50); return; }
		// At target position: drop
		if (Inside<int32_t>(cObj->GetX() - Tx._getInt(), -iMoveToRange, +iMoveToRange) && Inside<int32_t>(cObj->GetY()-Ty,-15,+15))
			{
			cObj->Action.ComDir=COMD_Stop;
			ObjectComDrop(cObj,Target);
			Finish(TRUE);
			return;
			}
		// Move to target position
		cObj->AddCommand(C4CMD_MoveTo,NULL,Tx._getInt(),Ty,20);
		return;
		}

  // Contained: put
  if (cObj->Contained)
    {
    ObjectComPutTake(cObj,cObj->Contained,Target);
    Finish(TRUE); return;
    }

  // Pushing: put
  if (cObj->GetProcedure()==DFA_PUSH)
    {
    if (cObj->Action.Target)
      ObjectComPutTake(cObj,cObj->Action.Target,Target);
    Finish(TRUE); return;
    }

  // Outside: Drop
  ObjectComDrop(cObj,Target);
  Finish(TRUE);
  }

void C4Command::Jump()
  {
	// Tx not default 0: adjust jump direction
	if (Tx._getInt())
		{
		if (Tx._getInt()<cObj->GetX()) cObj->SetDir(DIR_Left);
		if (Tx._getInt()>cObj->GetX()) cObj->SetDir(DIR_Right);
		}
	// Jump
  ObjectComJump(cObj);
	// Done
  Finish(TRUE);
  }

void C4Command::Wait()
  {
	// Digging: stop
	if (cObj->GetProcedure()==DFA_DIG) ObjectComStop(cObj);
  }

void C4Command::Context()
  {
	// Not context object specified (in Target2): fail
	if (!Target2) { Finish(); return; }
	// Open context menu for target
	cObj->ActivateMenu(C4MN_Context,0,0,0,Target2);
	if (Tx._getInt()!=0 && Ty!=0)
		if (cObj->Menu)
			{
			cObj->Menu->SetAlignment(C4MN_Align_Free);
			cObj->Menu->SetLocation(Tx._getInt(),Ty);
			}
	// Done
	Finish(TRUE);
	}

bool C4Command::GetTryEnter()
	{
	// No minimum con knowledge vehicles/items: fail
	if (Target->Contained && CheckMinimumCon(Target)) { /* fail??! */ return false; }
	// Target contained and container has RejectContents: fail
	if (Target->Contained && !!Target->Contained->Call(PSF_RejectContents)) { Finish(); return false; }
	// Collection limit: drop other object
	// return after drop, so multiple objects may be dropped
	if (cObj->Def->CollectionLimit && (cObj->Contents.ObjectCount()>=cObj->Def->CollectionLimit))
		{
		if (!cObj->PutAwayUnusedObject(Target)) { Finish(); return false; }
		return false;
		}
	bool fWasContained = !!Target->Contained;
	// Grab target object
	bool fRejectCollect = false;
	bool fSuccess = !!Target->Enter(cObj, TRUE, true, &fRejectCollect);
	// target is void?
	// well...most likely the new container has done something with it
	// so count it as success
	if (!Target) { Finish(TRUE); return true; }
	// collection rejected by target: make room for more contents
	if (fRejectCollect)
		{
		if (cObj->PutAwayUnusedObject(Target)) return false;
		// Can't get due to RejectCollect: fail
		Finish();
		return false;
		}
	// if not successfully entered for any other reason, fail
	if (!fSuccess) { Finish(); return false; }
	// get-callback for getting out of containers
	if (fWasContained) cObj->Call(PSF_Get, &C4AulParSet(C4VObj(Target)));
	// entered
	return true;
	}

void C4Command::Get()
  {

	// Data set and target specified: open get menu & done (old style)
		if (((Data.getInt()==1) || (Data.getInt()==2)) && Target)
		{
		cObj->ActivateMenu((Data.getInt()==1) ? C4MN_Get : C4MN_Contents,0,0,0,Target);
		Finish(TRUE); return;
		}

	// Get target specified by container and type
	if (!Target && Target2 && Data)
		if (!(Target = Target2->Contents.Find(Data.getC4ID())))
			{ Finish(); return; }

	// No target: failure
  if (!Target) { Finish(); return; }

	// Target not carryable: failure
	if (!(Target->OCF & OCF_Carryable))
		{ Finish(); return; }

	// Target collected
	if (Target->Contained == cObj)
		// Get-count specified: decrease count and continue with next object
		if (Tx._getInt() > 1)
			{ Target = NULL; Tx--; return; }
		// We're done
		else
			{	cObj->Action.ComDir=COMD_Stop; Finish(TRUE); return; }

  // Grabbing other than target container: let go
  if (cObj->GetProcedure()==DFA_PUSH)
		if (cObj->Action.Target!=Target->Contained)
			{ cObj->AddCommand(C4CMD_UnGrab,NULL,0,0,50); return; }

	// Target in solid: dig out
	if (!Target->Contained && (Target->OCF & OCF_InSolid))
		{
		// Check for closest free position
		int32_t iX=Target->GetX(),iY=Target->GetY();
		// Find all-closest dig-out position
		if (!FindClosestFree(iX,iY,-120,+120,-1,-1))
			// None found
			{ Finish(); return; }
		// Check good-angle left/right dig-out position
		int32_t iX2=Target->GetX(),iY2=Target->GetY();
		if (FindClosestFree(iX2,iY2,-140,+140,-40,+40))
			// Use good-angle position if it's not way worse
			if ( Distance(Target->GetX(),Target->GetY(),iX2,iY2) < 10*Distance(Target->GetX(),Target->GetY(),iX,iY) )
				{ iX=iX2; iY=iY2; }
		// Move to closest free position (if not in dig-direct range)
		if (!Inside(cObj->GetX()-iX,-DigOutPositionRange,+DigOutPositionRange)
		 || !Inside(cObj->GetY()-iY,-DigOutPositionRange,+DigOutPositionRange))
			{ cObj->AddCommand(C4CMD_MoveTo,NULL,iX,iY,50); return; }
		// DigTo
		cObj->AddCommand(C4CMD_Dig,NULL,Target->GetX(),Target->GetY()+4,50); return;
		}

	// Digging: stop
	if (cObj->GetProcedure()==DFA_DIG) ObjectComStop(cObj);

  // Target contained
  if (Target->Contained)
    {
		// target can't be taken out of containers?
		if(Target->Def->NoGet) return;
		// In same container: grab target
		if (cObj->Contained==Target->Contained)
			{
			GetTryEnter();
			// Done
			return;
			}

		// Leave own container
    if (cObj->Contained)
			{ cObj->AddCommand(C4CMD_Exit,NULL,0,0,50); return; }

		// Target container has grab get: grab target container
		if (Target->Contained->Def->GrabPutGet & C4D_Grab_Get)
			{
			// Grabbing target container
			if ((cObj->GetProcedure()==DFA_PUSH) && (cObj->Action.Target==Target->Contained))
				{
				GetTryEnter();
				// Done
				return;
				}
			// Grab target container
			cObj->AddCommand(C4CMD_Grab,Target->Contained,0,0,50);
			return;
			}

		// Target container has entrance: enter target container
		if (Target->Contained->OCF & OCF_Entrance)
			{ cObj->AddCommand(C4CMD_Enter,Target->Contained,0,0,50); return; }

		// Can't get to target due to target container: fail
		Finish();
		return;
    }

	// Target outside

	// Leave own container
  if (cObj->Contained) { cObj->AddCommand(C4CMD_Exit,NULL,0,0,50); return; }

	// Outside
	if (!cObj->Contained)
    {

		// Target in collection range
		DWORD ocf = OCF_Normal | OCF_Collection;
		if (cObj->At(Target->GetX(),Target->GetY(),ocf))
			{
			// stop here
			cObj->Action.ComDir=COMD_Stop;
			// try getting the object
			if (GetTryEnter()) return;
			}

		// Target not in range
		else
			{
			// Target in jumping range above clonk: try side-move jump
			if (Inside<int32_t>(cObj->GetX()-Target->GetX(),-10,+10))
				if (Inside<int32_t>(cObj->GetY()-Target->GetY(),30,50))
					{
					int32_t iSideX=1; if (Random(2)) iSideX=-1;
					iSideX=cObj->GetX()+iSideX*(cObj->GetY()-Target->GetY());
					if (PathFree(iSideX,cObj->GetY(),Target->GetX(),Target->GetY()))
						{
						// Side-move jump
						cObj->AddCommand(C4CMD_Jump,NULL,Tx._getInt(),Ty);
						if (cObj->Def->CollectionLimit && (cObj->Contents.ObjectCount()>=cObj->Def->CollectionLimit))
							cObj->AddCommand(C4CMD_Drop); // Drop object if necessary due to collection limit
						// Need to kill NoCollectDelay after drop...!
						cObj->AddCommand(C4CMD_MoveTo,NULL,iSideX,cObj->GetY(),50);
						}
					}
			// Move to target (random offset for difficult pickups)
			// ...avoid offsets into solid which would lead to high above surface locations!
			cObj->AddCommand(C4CMD_MoveTo,NULL,Target->GetX()+Random(15)-7,Target->GetY(),25,NULL);
			}

    }

  }

bool C4Command::CheckMinimumCon (C4Object *pObj)
	{
	if ((pObj->Category & C4D_Vehicle) || (pObj->Category & C4D_Object))
		if (pObj->Category & C4D_SelectKnowledge)
			if (pObj->GetCon() < FullCon)
				{
				//SoundEffect("Error",0,100,cObj);
				Finish(false, FormatString(LoadResStr("IDS_OBJ_NOCONACTIV"),pObj->GetName()).getData());
				return true;
				}
	return false;
	}

void C4Command::Activate()
  {

	// Container specified, but no Target & no type: open activate menu for container
	if (Target2 && !Target && !Data)
		{
		cObj->ActivateMenu(C4MN_Activate,0,0,0,Target2);
		Finish(TRUE);
		return;
		}

	// Target object specified & outside: success
	if (Target)
		if (!Target->Contained)
			{	Finish(TRUE); return; }

	// No container specified: determine container by target object
	if (!Target2)
		if (Target)
			Target2=Target->Contained;

	// No container specified: fail
  if (!Target2) { Finish(); return; }

	// Digging: stop
	if (cObj->GetProcedure()==DFA_DIG) ObjectComStop(cObj);

	// In container
	if (cObj->Contained==Target2)
		{
		for (Tx.SetInt(Data ? Max<int32_t>(Tx._getInt(),1) : 1); Tx._getInt(); --Tx)
			{
			// If not specified get object from target contents by type
			// Find first object requested id that has no command exit yet
			C4Object *pObj; C4ObjectLink *cLnk;
			if (!Target)
				for (cLnk=Target2->Contents.First; cLnk && (pObj=cLnk->Obj); cLnk=cLnk->Next)
					if (pObj->Status && (pObj->Def->id==Data.getC4ID()))
						if (!pObj->Command || (pObj->Command->Command!=C4CMD_Exit))
							{ Target=pObj; break; }
			// No target
			if (!Target) { Finish(); return; }

			// Thing in own container (target2)
			if (Target->Contained!=Target2) { Finish(); return; }

			// No minimum con knowledge vehicles/items
			if (CheckMinimumCon(Target)) return;

			// Activate object to exit
			Target->Controller = cObj->Controller;
			Target->SetCommand(C4CMD_Exit);
			Target = 0;
			}

		Finish(true); return;
		}

	// Leave own container
  if (cObj->Contained)
		{ cObj->AddCommand(C4CMD_Exit,NULL,0,0,50); return; }

	// Target container has entrance: enter
	if (Target2->OCF & OCF_Entrance)
		{ cObj->AddCommand(C4CMD_Enter,Target2,0,0,50); return; }

	// Can't get to target due to target container: fail
	Finish();

  }


void C4Command::Put() // Notice: Put command is currently using Ty as an internal reminder flag for letting go after putting...
  {
  // No target container: failure
  if (!Target) { Finish(); return; }

	// Thing to put specified by type
	if (!Target2 && Data)
		if (!(Target2 = cObj->Contents.Find(Data.getC4ID())))
			{ Finish(); return; }

	// No thing to put specified
	if (!Target2)
		// Assume first contents object
		if (!(Target2 = cObj->Contents.GetObject()))
			// No contents object to put - most likely we did have a target but it was deleted,
			// e.g. by AutoSellContents in a base. New behaviour: if there is nothing to put, we
			// now consider the command succesfully completed.
			{ Finish(TRUE); return; }

	// Thing is in target
	if (Target2->Contained == Target)
		// Put-count specified: decrease count and continue with next object
		if (Tx._getInt() > 1)
			{ Target2 = NULL; Tx--; return; }
		// We're done
		else
			{	Finish(TRUE); return; }

	// Thing to put not in contents: get object
	if (!cObj->Contents.GetLink(Target2))
		{
		// Object is nearby and traveling: wait
		if (!Target2->Contained)
			if (Distance(cObj->GetX(),cObj->GetY(),Target2->GetX(),Target2->GetY())<80)
				if (Target2->OCF & OCF_HitSpeed1)
					return;
		// Go get it
		cObj->AddCommand(C4CMD_Get,Target2,0,0,40); return;
		}

	// Target is contained: can't do it
	if (Target->Contained)
		{	Finish(); return; }

	// Digging: stop
	if (cObj->GetProcedure()==DFA_DIG) ObjectComStop(cObj);

  // Grabbing other than target: let go
	C4Object *pGrabbing=NULL;
  if (cObj->GetProcedure()==DFA_PUSH)
		pGrabbing = cObj->Action.Target;
	if (pGrabbing && (pGrabbing!=Target))
		{ cObj->AddCommand(C4CMD_UnGrab,NULL,0,0,50); return; }

	// Inside target container
	if (cObj->Contained == Target)
		{
		// Try to put
		if (!ObjectComPut(cObj,Target,Target2))
			Finish(); // Putting failed
		return;
		}

	// Leave own container
  if (cObj->Contained)
		{ cObj->AddCommand(C4CMD_Exit,NULL,0,0,50); return; }

	// Target has collection: throw in if not fragile, not grabbing target and throwing position found
	if (Target->OCF & OCF_Collection)
		if (!Target2->Def->Fragile)
			if (pGrabbing!=Target)
				{
				int32_t iTx = Target->GetX() + Target->Def->Collection.x + Target->Def->Collection.Wdt/2;
				int32_t iTy = Target->GetY() + Target->Def->Collection.y + Target->Def->Collection.Hgt/2;
				FIXED pthrow=ValByPhysical(400, cObj->GetPhysical()->Throw);
				int32_t iHeight = cObj->Shape.Hgt;
				int32_t iPosX,iPosY;
				int32_t iObjDist = Distance(cObj->GetX(),cObj->GetY(),Target->GetX(),Target->GetY());
				if ( (FindThrowingPosition(iTx,iTy,pthrow,-pthrow,iHeight,iPosX,iPosY) && (Distance(iPosX,iPosY,cObj->GetX(),cObj->GetY()) < iObjDist))
					|| (FindThrowingPosition(iTx,iTy,pthrow*-1,-pthrow,iHeight,iPosX,iPosY) && (Distance(iPosX,iPosY,cObj->GetX(),cObj->GetY()) < iObjDist)) )
						{
						// Throw
						cObj->AddCommand(C4CMD_Throw,Target2,iTx,iTy,5);
						return;
						}
				}

	// Target has C4D_Grab_Put: grab target and put
	if (Target->Def->GrabPutGet & C4D_Grab_Put)
		{
		// Grabbing target container
		if (pGrabbing==Target)
			{
			// Try to put
			if (!ObjectComPut(cObj,Target,Target2))
				// Putting failed
				{ Finish(); return; }
			// Let go (if we grabbed the target because of this command)
			if (Ty) cObj->AddCommand(C4CMD_UnGrab,NULL,0,0);
			return;
			}
		// Grab target and let go after putting
		cObj->AddCommand(C4CMD_Grab,Target,0,0,50);
		Ty = 1;
		return;
		}

	// Target can be entered: enter target
	if (Target->OCF & OCF_Entrance)
		{ cObj->AddCommand(C4CMD_Enter,Target,0,0,50); return; }

  }

void C4Command::ClearPointers(C4Object *pObj)
	{
	if (cObj==pObj) cObj=NULL;
	if (Target==pObj) Target=NULL;
	if (Target2==pObj) Target2=NULL;
	}

void C4Command::Execute()
	{

	// Finished?!
	if (Finished) return;

	// Parent object safety
	if (!cObj) { Finish(); return; }

	// Delegated command failed
	if (Failures)
		{
		// Retry
		if (Retries>0)
			{ Failures=0; Retries--; cObj->AddCommand(C4CMD_Retry,NULL,0,0,10); return; }
		// Too many failures
		else
			{ Finish(); return; }
		}

  // Command update
  if (UpdateInterval>0)
    {
    UpdateInterval--;
    if (UpdateInterval==0) {
			Finish(TRUE); return; }
    }

  // Initial command evaluation
	if (InitEvaluation()) return;

	// from now on, we are executing this command... and nobody
	// should dare deleting us
	iExec = 1;

  // Execute
  switch (Command)
    {
		case C4CMD_Follow: Follow(); break;
		case C4CMD_MoveTo: MoveTo(); break;
		case C4CMD_Enter: Enter(); break;
		case C4CMD_Exit: Exit(); break;
		case C4CMD_Grab: Grab(); break;
		case C4CMD_UnGrab: UnGrab(); break;
		case C4CMD_Throw: Throw(); break;
		case C4CMD_Chop: Chop(); break;
		case C4CMD_Build: Build(); break;
		case C4CMD_Jump: Jump(); break;
		case C4CMD_Wait: Wait(); break;
		case C4CMD_Get: Get(); break;
		case C4CMD_Put: Put(); break;
		case C4CMD_Drop: Drop(); break;
		case C4CMD_Dig: Dig();	break;
		case C4CMD_Activate:	Activate();	break;
		case C4CMD_PushTo:	PushTo(); break;
		case C4CMD_Construct: Construct(); break;
		case C4CMD_Transfer: Transfer(); break;
		case C4CMD_Attack: Attack(); break;
		case C4CMD_Context: Context(); break;
		case C4CMD_Buy: Buy(); break;
		case C4CMD_Sell: Sell(); break;
		case C4CMD_Acquire: Acquire(); break;
		case C4CMD_Energy: Energy(); break;
		case C4CMD_Retry: Retry(); break;
		case C4CMD_Home: Home(); break;
		case C4CMD_Call: Call(); break;
		case C4CMD_Take: Take(); break; // carlo
		case C4CMD_Take2: Take2(); break; // carlo
    default: Finish(); break;
    }

/*	// Remember this command might have already been deleted through calls
	// made during execution. You must not do anything here... */

	// check: command must be deleted?
	if(iExec > 1)
		delete this;
	else
		iExec = 0;

	}

void C4Command::Finish(BOOL fSuccess, const char *szFailMessage)
	{
	// Mark finished
	Finished=TRUE;
	// Failed
	if (!fSuccess)
		Fail(szFailMessage);
	else
		{
		// successful commands might gain EXP
		int32_t iExpGain = GetExpGain();
		if (iExpGain && cObj) if (cObj->Info)
			for (int32_t i=iExpGain; i; --i)
				if (!(++cObj->Info->ControlCount%5))
					cObj->DoExperience(1);
		}
	}

BOOL C4Command::InitEvaluation()
	{
	// Already evaluated
	if (Evaluated) return FALSE;
	// Set evaluation flag
  Evaluated=TRUE;
	// Evaluate
  switch (Command)
    {
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case C4CMD_MoveTo:
			{
			// Target coordinates by Target
			if (Target) { Tx+=Target->GetX(); Ty+=Target->GetY(); Target=NULL; }
			// Adjust coordinates
			int32_t iTx = Tx._getInt();
			if (~Data.getInt() & C4CMD_MoveTo_NoPosAdjust) AdjustMoveToTarget(iTx,Ty,FreeMoveTo(cObj),cObj->Shape.Hgt);
			Tx.SetInt(iTx);
 			return TRUE;
			}
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4CMD_PushTo:
			{
			// Adjust coordinates
			int32_t iTx = Tx._getInt();
			AdjustMoveToTarget(iTx,Ty,FreeMoveTo(cObj),cObj->Shape.Hgt);
			Tx.SetInt(iTx);
 			return TRUE;
			}
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4CMD_Exit:
			// Cancel attach
			ObjectComCancelAttach(cObj);
			return TRUE;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4CMD_Wait:
			// Update interval by Data
			if (!Data) UpdateInterval=Data.getInt();
			// Else update interval by Tx
			else if (Tx._getInt()) UpdateInterval=Tx._getInt();
			return TRUE;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4CMD_Acquire:
			// update default search range
			if (!Tx._getInt()) Tx.SetInt(500);
			if (!Ty) Ty=250;
			return TRUE;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    }
	// Need not be evaluated
  return FALSE;
	}

void C4Command::Clear()
	{
  Command=C4CMD_None;
	cObj=NULL;
  Evaluated=FALSE;
	PathChecked=FALSE;
	Tx=C4VNull;
	Ty=0;
  Target=Target2=NULL;
  UpdateInterval=0;
	if (Text) Text->DecRef(); Text=NULL;
	BaseMode=C4CMD_Mode_SilentSub;
	}

void C4Command::Construct()
	{
	// Only those who can
	if (cObj->GetPhysical() && !cObj->GetPhysical()->CanConstruct)
		{
		Finish(FALSE, FormatString(LoadResStr("IDS_TEXT_CANTBUILD"), cObj->GetName()).getData());
		return;
		}
	// No target type to construct: open menu & done
  if (!Data)
		{
		cObj->ActivateMenu(C4MN_Construction);
		Finish(TRUE); return;
		}

	// Determine move-to range
	int32_t iMoveToRange = MoveToRange;
	if (cObj->Def->MoveToRange > 0) iMoveToRange = cObj->Def->MoveToRange;

	// this is a secondary construct command (i.e., help with construction)?
	if (Target)
		{
		// check if target is building something
		C4Command *pBuildCmd = Target->FindCommand(C4CMD_Build);
		if (pBuildCmd)
			{
			// then help
			Finish(TRUE);
			cObj->AddCommand(C4CMD_Build,pBuildCmd->Target);
			}
		// construct command still present? (might find another stacked command, which doesn't really matter for now...)
		if (!Target->FindCommand(C4CMD_Construct))
			// command aborted (or done?): failed to help; don't issue another construct command, because it is likely to fail anyway
			// (and maybe, it had been finished while this Clonk was still moving to the site)
			{ Finish(FALSE); return; }
		// site not yet placed: move to target, if necessary and known
		if (Tx._getInt() || Ty)
			if (!Inside<int32_t>(cObj->GetX() - Tx._getInt(), -iMoveToRange, +iMoveToRange) || !Inside<int32_t>(cObj->GetY()-Ty,-20,+20))
				{ cObj->AddCommand(C4CMD_MoveTo,NULL,Tx,Ty,50); return; }
		// at target construction site and site not yet placed: wait
		cObj->AddCommand(C4CMD_Wait,NULL,0,0,10);
		return;
		}

  // No valid target type: fail
	C4Def *pDef; if (!(pDef=C4Id2Def(Data.getC4ID()))) { Finish(); return; }

	// player has knowledge of this construction?
	C4Player *pPlayer = ::Players.Get(cObj->Owner);
	if(pPlayer) if(!pPlayer->Knowledge.GetIDCount(Data.getC4ID(), 1)) { Finish(); return; }

  // Building, chopping, digging: stop
  if ((cObj->GetProcedure()==DFA_CHOP) || (cObj->GetProcedure()==DFA_BUILD) || (cObj->GetProcedure()==DFA_DIG))
    ObjectComStop(cObj);

	// Pushing: let go
  if (cObj->GetProcedure()==DFA_PUSH)
    if (cObj->Action.Target)
	    { cObj->AddCommand(C4CMD_UnGrab,NULL,0,0,50); return; }

	// No construction site specified: find one
	if ((Tx._getInt()==0) && (Ty==0))
		{
		Tx.SetInt(cObj->GetX()); Ty=cObj->GetY();
		int32_t iTx = Tx._getInt();
		if (!FindConSiteSpot(iTx,Ty,pDef->Shape.Wdt,pDef->Shape.Hgt,pDef->Category,20))
			// No site found: fail
			{ Finish(); return; }
		Tx.SetInt(iTx);
		}

	// command has been validated: check for script overload now
	int32_t scriptresult = cObj->Call(PSF_ControlCommandConstruction, &C4AulParSet(C4VObj(Target), Tx, C4VInt(Ty), C4VObj(Target2), Data)).getInt ();
	// script call might have deleted object
	if (!cObj->Status) return;
  if (1 == scriptresult) return;
	if (2 == scriptresult)
		 { Finish(TRUE); return; }
	if (3 == scriptresult)
		 { Finish(); return; }

	// Has no construction kit: acquire one
	C4Object *pKit;
	if (!(pKit=cObj->Contents.Find(C4ID_Conkit)))
		{ cObj->AddCommand(C4CMD_Acquire,0,0,0,50,0,TRUE,C4VID(C4ID_Conkit),FALSE,5,0,C4CMD_Mode_Sub); return; }

	// Move to construction site
	if (!Inside<int32_t>(cObj->GetX() - Tx._getInt(), -iMoveToRange, +iMoveToRange)
	 || !Inside<int32_t>(cObj->GetY() - Ty, -20, +20))
		{ cObj->AddCommand(C4CMD_MoveTo,NULL,Tx,Ty,50); return; }

	// Check construction site
	if (!ConstructionCheck(Data.getPropList(),Tx._getInt(),Ty,cObj))
		// Site no good: fail
		{ Finish(); return; }

	// Create construction
	C4Object *pConstruction = Game.CreateObjectConstruction(Data.getPropList(),NULL,cObj->Owner,Tx._getInt(),Ty,1,TRUE);

	// Remove conkit
	pKit->AssignRemoval();

	// Finish, start building
	Finish(TRUE);
	cObj->AddCommand(C4CMD_Build,pConstruction);

	}

BOOL C4Command::FlightControl() // Called by DFA_WALK, DFA_FLIGHT
{
	// Objects with CanFly physical only
	if (!cObj->GetPhysical()->CanFly) return FALSE;
	// Crew members or pathfinder objects only
	if (!((cObj->OCF & OCF_CrewMember) || cObj->Def->Pathfinder)) return FALSE;

	// Not while in a disabled action
	if (cObj->Action.pActionDef)
	{
		if (cObj->Action.pActionDef->GetPropertyInt(P_ObjectDisabled)) return FALSE;
	}

	// Target angle
	int32_t cx=cObj->GetX(),cy=cObj->GetY();
	int32_t iAngle = Angle(cx,cy,Tx._getInt(),Ty); while (iAngle>180) iAngle-=360;

	// Target in flight angle (sector +/- from straight up), beyond minimum distance, and top free
	if (Inside(iAngle, -FlightAngleRange, +FlightAngleRange)
	 || Inside(iAngle, -FlightAngleRange, +FlightAngleRange))
			if (Distance(cx,cy,Tx._getInt(),Ty) > 30)
				{
				int32_t iTopFree;
				for (iTopFree=0; (iTopFree<50) && !GBackSolid(cx,cy+cObj->Shape.GetY()-iTopFree); ++iTopFree) {}
				if (iTopFree>=15)
					{
					//sprintf(OSTR,"Flight take off at %d (%d)",iAngle,Distance(cx,cy,Tx,Ty)); Log(OSTR); GameMsgObject(OSTR,cObj);
					//cObj->AddCommand(C4CMD_Jump,NULL,Tx,Ty); return TRUE;

					// Take off
					cObj->SetActionByName("Fly"); // This is a little primitive... we should have a ObjectActionFly or maybe a command for this...
					}
				}

	// No flight control
	return FALSE;
}

BOOL C4Command::JumpControl() // Called by DFA_WALK
	{

	// Crew members or pathfinder objects only
	if (! ((cObj->OCF & OCF_CrewMember) || cObj->Def->Pathfinder) ) return FALSE;

	// Target angle
	int32_t cx=cObj->GetX(),cy=cObj->GetY();
	int32_t iAngle = Angle(cx,cy,Tx._getInt(),Ty); while (iAngle>180) iAngle-=360;

	// Diagonal free jump (if in angle range, minimum distance, and top free)
	if (Inside(iAngle-JumpAngle,-JumpAngleRange,+JumpAngleRange)
	 || Inside(iAngle+JumpAngle,-JumpAngleRange,+JumpAngleRange))
		if (PathFree(cx,cy,Tx._getInt(),Ty))
			if (Distance(cx,cy,Tx._getInt(),Ty)>30)
				{
				int32_t iTopFree;
				for (iTopFree=0; (iTopFree<50) && !GBackSolid(cx,cy+cObj->Shape.GetY()-iTopFree); ++iTopFree) {}
				if (iTopFree>=15)
					{
					//sprintf(OSTR,"Diagonal %d (%d)",iAngle,Distance(cx,cy,Tx,Ty)); GameMsgObject(OSTR,cObj);
					cObj->AddCommand(C4CMD_Jump,NULL,Tx,Ty); return TRUE;
					}
				}

	// High angle side move - jump (2x range)
	if (Inside<int32_t>(iAngle-JumpHighAngle,-3*JumpAngleRange,+3*JumpAngleRange))
		// Vertical range
		if (Inside<int32_t>(cy-Ty,10,40))
			{
			int32_t iSide=SolidOnWhichSide(Tx._getInt(),Ty); // take jump height of side move position into consideration...!
			int32_t iDist=5*Abs(cy-Ty)/6;
			int32_t iSideX=cx-iDist*iSide,iSideY=cy; AdjustMoveToTarget(iSideX,iSideY,FALSE,0);
			// Side move target in range
			if (Inside<int32_t>(iSideY-cy,-20,+20))
				{
				// Path free from side move target to jump target
				if (PathFree(iSideX,iSideY,Tx._getInt(),Ty))
					{
					//sprintf(OSTR,"High side move %d (%d,%d)",iAngle,iSideX-cx,iSideY-cy); GameMsgObject(OSTR,cObj);
					cObj->AddCommand(C4CMD_Jump,NULL,Tx,Ty);
					cObj->AddCommand(C4CMD_MoveTo,NULL,iSideX,iSideY,50);
					return TRUE;
					}
				/*else
					{ sprintf(OSTR,"Side move %d/%d path not free",iSideX,iSideY); GameMsgObject(OSTR,cObj); }*/
				}
			/*else
				{ sprintf(OSTR,"Side move %d out of range",iSideX-cx); GameMsgObject(OSTR,cObj); }*/
			}
		/*else
			{ sprintf(OSTR,"No high range %d",cy-Ty); GameMsgObject(OSTR,cObj); }*/

	// Low side contact jump
	int32_t iLowSideRange=5;
	if (cObj->t_contact & CNAT_Right)
		if (Inside(iAngle-JumpLowAngle,-iLowSideRange*JumpAngleRange,+iLowSideRange*JumpAngleRange))
			{
			//sprintf(OSTR,"Low contact right %d",iAngle); GameMsgObject(OSTR,cObj);
			cObj->AddCommand(C4CMD_Jump,NULL,Tx,Ty); return TRUE;
			}
	if (cObj->t_contact & CNAT_Left)
		if (Inside(iAngle+JumpLowAngle,-iLowSideRange*JumpAngleRange,+iLowSideRange*JumpAngleRange))
			{
			//sprintf(OSTR,"Low contact left %d",iAngle); GameMsgObject(OSTR,cObj);
			cObj->AddCommand(C4CMD_Jump,NULL,Tx,Ty); return TRUE;
			}

	// No jump control
	return FALSE;
	}

void C4Command::Transfer()
	{

  // No target: failure
  if (!Target) { Finish(); return; }

	// Find transfer zone
	C4TransferZone *pZone;
	int32_t iEntryX,iEntryY;
	if (!(pZone=Game.TransferZones.Find(Target))) { Finish(); return; }

	// Not at or in transfer zone: move to entry point
	if (!Inside<int32_t>(cObj->GetX()-pZone->X,-5,pZone->Wdt-1+5))
		{
		if (!pZone->GetEntryPoint(iEntryX,iEntryY,cObj->GetX(),cObj->GetY())) { Finish(); return; }
		cObj->AddCommand(C4CMD_MoveTo,NULL,iEntryX,iEntryY,25);
		return;
		}

	// Call target transfer script
	if (!::Game.iTick5)
		{

		C4AulScriptFunc *f;
		BOOL fHandled = (f = Target->Def->Script.SFn_ControlTransfer) != NULL;
		if (fHandled) fHandled = f->Exec(Target,&C4AulParSet(C4VObj(cObj), Tx, C4VInt(Ty))).getBool();

		if (!fHandled)
			// Transfer not handled by target: done
			{ Finish(TRUE); return; }
		}

	}

void C4Command::Attack()
	{

  // No target: failure
  if (!Target) { Finish(); return; }

	// Target is crew member
	if (Target->OCF & OCF_CrewMember)
		{

		C4Object *pProjectile=NULL;
		// Throw projectile at target
		for (C4ObjectLink *pLnk=cObj->Contents.First; pLnk && (pProjectile=pLnk->Obj); pLnk=pLnk->Next)
			if (pProjectile->Def->Projectile)
				{
				// Add throw command
				cObj->AddCommand(C4CMD_Throw,pProjectile,Target->GetX(),Target->GetY(),2);
				return;
				}

		// Follow containment
		if (cObj->Contained!=Target->Contained)
			{
			// Exit
			if (cObj->Contained) cObj->AddCommand(C4CMD_Exit,NULL,0,0,10);
			// Enter
			else cObj->AddCommand(C4CMD_Enter,Target->Contained,0,0,10);
			return;
			}

		// Move to target
		cObj->AddCommand(C4CMD_MoveTo,NULL,Target->GetX(),Target->GetY(),10);
		return;

		}

	// For now, attack crew members only
	else
		{
		// Success, target might be no crew member due to that is has been killed
		Finish(TRUE);
		return;
		}

	}

void C4Command::Buy()
	{
	// Base buying disabled? Fail.
	if (~Game.C4S.Game.Realism.BaseFunctionality & BASEFUNC_Buy) { Finish(); return; }
	// No target (base) object specified: find closest base
	int32_t cnt; C4Object *pBase;
	if (!Target)
		for (cnt=0; pBase=Game.FindFriendlyBase(cObj->Owner,cnt); cnt++)
			if (!Target || Distance(cObj->GetX(),cObj->GetY(),pBase->GetX(),pBase->GetY())<Distance(cObj->GetX(),cObj->GetY(),Target->GetX(),Target->GetY()))
				Target=pBase;
	// No target (base) object: fail
	if (!Target) { Finish(); return; }
	// No type to buy specified: open buy menu for base
	if (!Data)
		{
		cObj->ActivateMenu(C4MN_Buy,0,0,0,Target);
		Finish(TRUE); return;
		}
	// Target object is no base or hostile: fail
	if (!ValidPlr(Target->Base) || Hostile(Target->Base,cObj->Owner))
		{ Finish(); return; }
	// Target material undefined: fail
	C4Def *pDef = C4Id2Def(Data.getC4ID());
	if (!pDef) { Finish(); return; }
	// Material not available for purchase at base: fail
	if (!::Players.Get(Target->Base)->HomeBaseMaterial.GetIDCount(Data.getC4ID()))
		{
		Finish(false, FormatString(LoadResStr("IDS_PLR_NOTAVAIL"),pDef->GetName()).getData());
		return;
		}
	// Base owner has not enough funds: fail
	if (::Players.Get(Target->Base)->Wealth < pDef->GetValue(Target, cObj->Owner))
		{ Finish(false, LoadResStr("IDS_PLR_NOWEALTH")); return; }
	// Not within target object: enter
	if (cObj->Contained!=Target)
		{ cObj->AddCommand(C4CMD_Enter,Target,0,0,50); return; }
	// Buy object(s)
	for (Tx.SetInt(Max<int32_t>(Tx._getInt(),1)); Tx._getInt(); Tx--)
		if (!Buy2Base(cObj->Owner,Target,Data.getC4ID()))
			// Failed (with ugly message)
			{ Finish(); return; }
	// Done: success
	Finish(TRUE);
	}

void C4Command::Sell()
	{
	// Base sale disabled? Fail.
	if (~Game.C4S.Game.Realism.BaseFunctionality & BASEFUNC_Sell) { Finish(); return; }
	// No target (base) object specified: find closest base
	int32_t cnt; C4Object *pBase;
	if (!Target)
		for (cnt=0; pBase=Game.FindBase(cObj->Owner,cnt); cnt++)
			if (!Target || Distance(cObj->GetX(),cObj->GetY(),pBase->GetX(),pBase->GetY())<Distance(cObj->GetX(),cObj->GetY(),Target->GetX(),Target->GetY()))
				Target=pBase;
	// No target (base) object: fail
	if (!Target) { Finish(); return; }
	// No type to sell specified: open sell menu for base
	if (!Data)
		{
		cObj->ActivateMenu(C4MN_Sell,0,0,0,Target);
		Finish(TRUE); return;
		}
	// Target object is no base or hostile: fail
	if (!ValidPlr(Target->Base) || Hostile(Target->Base,cObj->Owner))
		{ Finish(); return; }
	// Not within target object: enter
	if (cObj->Contained!=Target)
		{ cObj->AddCommand(C4CMD_Enter,Target,0,0,50); return; }
	// Sell object(s)
	for (Tx.SetInt(Max<int32_t>(Tx._getInt(),1)); Tx._getInt(); Tx--)
		if (!SellFromBase(cObj->Owner,Target,Data.getC4ID(),Target2))
			// Failed
			{ Finish(); return; }
		else
			// preferred sell object can be sold once only :)
			Target2=NULL;
	// Done
	Finish(TRUE);
	}

void C4Command::Acquire()
	{

	// No type to acquire specified: fail
	if (!Data) { Finish(); return; }

	// Target material in inventory: done
	if (cObj->Contents.Find(Data.getC4ID()))
		 { Finish(TRUE); return; }

	// script overload
	int32_t scriptresult = cObj->Call(PSF_ControlCommandAcquire, &C4AulParSet(C4VObj(Target), Tx, C4VInt(Ty), C4VObj(Target2), Data)).getInt ();

	// script call might have deleted object
	if (!cObj->Status) return;
  if (1 == scriptresult) return;
	if (2 == scriptresult)
		 { Finish(TRUE); return; }
	if (3 == scriptresult)
		 { Finish(); return; }

	// Find available material
	C4Object *pMaterial=NULL;
	// Next closest
	while (pMaterial = Game.FindObject(Data.getC4ID(),cObj->GetX(),cObj->GetY(),-1,-1,OCF_Available,NULL,NULL,NULL,NULL,ANY_OWNER,pMaterial))
		// Object is not in container to be ignored
		if (!Target2 || pMaterial->Contained!=Target2)
			// Object is near enough
			if (Inside(cObj->GetX()-pMaterial->GetX(),-Tx._getInt(),+Tx._getInt()))
			if (Inside(cObj->GetY()-pMaterial->GetY(),-Ty,+Ty))
				// Object is not connected to a pipe (for line construction kits)
				if (!Game.FindObject(C4ID_SourcePipe,0,0,0,0,OCF_All,"Connect",pMaterial))
				if (!Game.FindObject(C4ID_DrainPipe,0,0,0,0,OCF_All,"Connect",pMaterial))
					// Must be complete
					if (pMaterial->OCF & OCF_FullCon)
						// Doesn't burn
						if (!pMaterial->GetOnFire())
							// We found one
							break;

	// Available material found: get material
	if (pMaterial)
		{ cObj->AddCommand(C4CMD_Get,pMaterial,0,0,40); return; }

	// No available material found: buy material
	// This command will fail immediately if buying at bases is not possible
	// - but the command should be created anyway because it might be overloaded
	cObj->AddCommand(C4CMD_Buy,NULL,0,0,100,NULL,TRUE,Data,false,0,0,C4CMD_Mode_Sub);

	}

void C4Command::Fail(const char *szFailMessage)
	{
	// Check for base command (next unfinished)
	C4Command *pBase;
	for (pBase=Next; pBase; pBase=pBase->Next)	if (!pBase->Finished) break;

	bool ExecFail = false;
	switch (BaseMode)
		{
		// silent subcommand
		case C4CMD_Mode_SilentSub:
			{
			// Just count failures in base command
			if (pBase) { pBase->Failures++; return; }
			else ExecFail = true;
			break;
			}
		// verbose subcommand
		case C4CMD_Mode_Sub:
			{
			// Count failures in base command
			if (pBase) { pBase->Failures++; }
			// Execute fail message, if base will fail
			if (!pBase || !pBase->Retries) ExecFail = true;
			break;
			}
		// base command
		case C4CMD_Mode_Base:
			{
			// Just execute fail notice
			ExecFail = true;
			break;
			}
		// silent base command: do nothing
		}

	char szCommandName[24 + 1];
	char szObjectName[C4MaxName + 1];
	StdStrBuf str;

	if (ExecFail && cObj && (cObj->OCF & OCF_CrewMember))
		{
		// Fail message
		if (szFailMessage)
			str = szFailMessage;
		C4Object * l_Obj = cObj;
		switch (Command)
			{
			case C4CMD_Build:
				// Needed components
				if (!Target) break;
				// BuildNeedsMaterial call to builder script...
				if (!!cObj->Call(PSF_BuildNeedsMaterial, &C4AulParSet(
					C4VID(Target->Component.GetID(0)), C4VInt(Target->Component.GetCount(0))))) // WTF? This is passing current components. Not needed ones!
					break; // no message
				if (szFailMessage) break;
				str = Target->GetNeededMatStr(cObj);
				break;
			case C4CMD_Call:
				{
				// Call fail-function in target object (no message if non-zero)
				int32_t l_Command = Command;
				if (CallFailed()) return;
				// Fail-function not available or returned zero: standard message
				SCopy(LoadResStr(CommandNameID(l_Command)),szCommandName);
				str.Format(LoadResStr("IDS_CON_FAILURE"),szCommandName);
				break;
				}
			case C4CMD_Exit:
				// No message
				break;
			case C4CMD_Dig:
				// No message
				break;
			case C4CMD_Acquire:
			case C4CMD_Construct:
				// Already has a fail message
				if (szFailMessage) break;
				// Fail message with name of target type
				SCopy(LoadResStr(CommandNameID(Command)), szCommandName);
				C4Def *pDef; pDef = ::Definitions.ID2Def(Data.getC4ID());
				SCopy(pDef ? pDef->GetName() : LoadResStr("IDS_OBJ_UNKNOWN"), szObjectName);
				str.Format(LoadResStr("IDS_CON_FAILUREOF"), szCommandName, szObjectName);
				break;
			default:
				// Already has a fail message
				if (szFailMessage) break;
				// Standard no-can-do message
				SCopy(LoadResStr(CommandNameID(Command)), szCommandName);
				str.Format(LoadResStr("IDS_CON_FAILURE"), szCommandName);
				break;
			}
		if (l_Obj) if (l_Obj->Status && !l_Obj->Def->SilentCommands)
			{
			// Message (if not empty)
			if (!!str)
				{
				::Messages.Append(C4GM_Target, str.getData(), l_Obj, NO_OWNER, 0, 0, FWhite, TRUE);
				}
			// Fail sound
			StartSoundEffect("CommandFailure*",false,100,l_Obj);
			// Stop Clonk
			l_Obj->Action.ComDir = COMD_Stop;
			}
		}
	}

C4Object *CreateLine(C4ID idType, int32_t iOwner, C4Object *pFrom, C4Object *pTo);

void C4Command::Energy()
	{
	DWORD ocf=OCF_All;
	// No target: fail
	if (!Target) { Finish(); return; }
	// Target can't be supplied: fail
	if (!(Target->Def->LineConnect & C4D_Power_Input)) { Finish(); return; }
	// Target supplied
	if ( !(Game.Rules & C4RULE_StructuresNeedEnergy)
		|| (Game.FindObject(C4ID_PowerLine,0,0,0,0,OCF_All,"Connect",Target) && !Target->NeedEnergy) )
			{ Finish(TRUE); return; }
	// No energy supply specified: find one
	if (!Target2)	Target2=Game.FindObject(0,Target->GetX(),Target->GetY(),-1,-1,OCF_PowerSupply,NULL,NULL,Target);
	// No energy supply: fail
	if (!Target2) { Finish(); return; }
	// Energy supply too far away: fail
	if (Distance(cObj->GetX(),cObj->GetY(),Target2->GetX(),Target2->GetY())>650) { Finish(); return; }
	// Not a valid energy supply: fail
	if (!(Target2->Def->LineConnect & C4D_Power_Output)) { Finish(); return; }
	// No linekit: get one
	C4Object *pKit, *pLine = NULL, *pKitWithLine;
	if (!(pKit=cObj->Contents.Find(C4ID_Linekit)))
		{ cObj->AddCommand(C4CMD_Acquire,NULL,0,0,50,NULL,TRUE,C4VID(C4ID_Linekit)); return; }
	// Find line constructing kit
	for (int32_t cnt=0; pKitWithLine=cObj->Contents.GetObject(cnt); cnt++)
		if ((pKitWithLine->id==C4ID_Linekit) && (pLine=Game.FindObject(C4ID_PowerLine,0,0,0,0,OCF_All,"Connect",pKitWithLine)))
			break;
	// No line constructed yet
	if (!pLine)
		{
		// Move to supply
		if (!Target2->At(cObj->GetX(),cObj->GetY(),ocf))
			{ cObj->AddCommand(C4CMD_MoveTo,Target2,0,0,50); return; }
		// At power supply: connect
		pLine = CreateLine(C4ID_PowerLine,cObj->Owner,Target2,pKit);
		if (!pLine) { Finish(); return; }
		StartSoundEffect("Connect",false,100,cObj);
		return;
		}
	else
		{
		// A line is already present: Make sure not to override the target
		if (pLine->Action.Target == pKitWithLine)
			Target2 = pLine->Action.Target2;
		else
			Target2 = pLine->Action.Target;
		}
	// Move to target
	if (!Target->At(cObj->GetX(),cObj->GetY(),ocf))
		{ cObj->AddCommand(C4CMD_MoveTo,Target,0,0,50); return; }
	// Connect
	pLine->SetActionByName("Connect",Target2,Target);
	pKitWithLine->AssignRemoval();
	StartSoundEffect("Connect",0,100,cObj);
	// Done
	cObj->Action.ComDir=COMD_Stop;
	// Success
	Finish(TRUE);
	}

void C4Command::Retry()
	{

	}

void C4Command::Home()
	{
	// At home base: done
	if (cObj->Contained && (cObj->Contained->Base==cObj->Owner))
		{ Finish(TRUE); return; }
	// No target (base) object specified: find closest base
	int32_t cnt; C4Object *pBase;
	if (!Target)
		for (cnt=0; pBase=Game.FindBase(cObj->Owner,cnt); cnt++)
			if (!Target || Distance(cObj->GetX(),cObj->GetY(),pBase->GetX(),pBase->GetY())<Distance(cObj->GetX(),cObj->GetY(),Target->GetX(),Target->GetY()))
				Target=pBase;
	// No base: fail
	if (!Target) { Finish(); return; }
	// Enter base
	cObj->AddCommand(C4CMD_Enter,Target);
	}

void C4Command::Set(int32_t iCommand, C4Object *pObj, C4Object *pTarget, C4Value nTx, int32_t iTy,
										C4Object *pTarget2, C4Value iData, int32_t iUpdateInterval,
										BOOL fEvaluated, int32_t iRetries, C4String * szText, int32_t iBaseMode)
	{
	// Reset
	Clear(); Default();
	// Set
	Command=iCommand;
	cObj=pObj;
	Target=pTarget;
	Tx=nTx; Ty=iTy;
	Target2=pTarget2;
	Data=iData;
	UpdateInterval=iUpdateInterval;
	Evaluated=fEvaluated;
	Retries=iRetries;
	Text = szText;
	if (Text) Text->IncRef();
	BaseMode=iBaseMode;
	}

void C4Command::Call()
	{
	// No function name: fail
	if (!Text || !Text->GetCStr() || !Text->GetCStr()[0]) { Finish(); return; }
	// No target object: fail
	if (!Target) { Finish(); return; }
	// Done: success
	Finish(TRUE);
	// Object call FIXME:use c4string-api
	Target->Call(Text->GetCStr(),&C4AulParSet(C4VObj(cObj), Tx, C4VInt(Ty), C4VObj(Target2)));
	// Extreme caution notice: the script call might do just about anything
	// including clearing all commands (including this) i.e. through a call
	// to SetCommand. Thus, we must not do anything in this command anymore
	// after the script call (even the Finish has to be called before).
	// The Finish call being misled to the freshly created Build command (by
	// chance, the this-pointer was simply crap at the time) was reason for
	// the latest sync losses in 4.62.
	}

void C4Command::CompileFunc(StdCompiler *pComp)
	{
	// Version
	int32_t iVersion = 0;
	if(pComp->Seperator(StdCompiler::SEP_DOLLAR))
		{
		iVersion = 1;
		pComp->Value(mkIntPackAdapt(iVersion));
		pComp->Seperator(StdCompiler::SEP_SEP);
		}
	else
		pComp->NoSeperator();
	// Command name
	pComp->Value(mkEnumAdaptT<uint8_t>(Command, EnumAdaptCommandEntries));
	pComp->Seperator(StdCompiler::SEP_SEP);
	// Target X/Y
	pComp->Value(Tx); pComp->Seperator(StdCompiler::SEP_SEP);
	pComp->Value(mkIntPackAdapt(Ty)); pComp->Seperator(StdCompiler::SEP_SEP);
	// Target
	pComp->Value(mkIntPackAdapt(reinterpret_cast<int32_t &>(Target))); pComp->Seperator(StdCompiler::SEP_SEP);
	pComp->Value(mkIntPackAdapt(reinterpret_cast<int32_t &>(Target2))); pComp->Seperator(StdCompiler::SEP_SEP);
	// Data
	pComp->Value(Data); pComp->Seperator(StdCompiler::SEP_SEP);
	// Update interval
	pComp->Value(mkIntPackAdapt(UpdateInterval)); pComp->Seperator(StdCompiler::SEP_SEP);
	// Flags
	pComp->Value(mkIntPackAdapt(Evaluated)); pComp->Seperator(StdCompiler::SEP_SEP);
	pComp->Value(mkIntPackAdapt(PathChecked)); pComp->Seperator(StdCompiler::SEP_SEP);
	pComp->Value(mkIntPackAdapt(Finished)); pComp->Seperator(StdCompiler::SEP_SEP);
	// Retries
	pComp->Value(mkIntPackAdapt(Failures)); pComp->Seperator(StdCompiler::SEP_SEP);
	pComp->Value(mkIntPackAdapt(Retries)); pComp->Seperator(StdCompiler::SEP_SEP);
	pComp->Value(mkIntPackAdapt(Permit)); pComp->Seperator(StdCompiler::SEP_SEP);
	// Base mode
	if(iVersion > 0)
		{
		pComp->Value(mkIntPackAdapt(BaseMode)); pComp->Seperator(StdCompiler::SEP_SEP);
		}
	// Text
	StdStrBuf TextBuf;
	if(pComp->isDecompiler())
		{
		if(Text)
			TextBuf.Ref(Text->GetData());
		else
			TextBuf.Ref("0");
		}
	pComp->Value(mkParAdapt(TextBuf, StdCompiler::RCT_All));
	if(pComp->isCompiler())
		{
		if(Text)
			Text->DecRef();
		if(TextBuf == "0")
			{ Text = NULL; }
		else
			{ Text = Strings.RegString(TextBuf); Text->IncRef(); }
		}
  }

void C4Command::DenumeratePointers()
	{
	Target = ::Objects.ObjectPointer((long)Target);
	Target2 = ::Objects.ObjectPointer((long)Target2);
	Tx.DenumeratePointer();
	}

void C4Command::EnumeratePointers()
	{
	Target = (C4Object*) ::Objects.ObjectNumber(Target);
	Target2 = (C4Object*) ::Objects.ObjectNumber(Target2);
	}

int32_t C4Command::CallFailed()
	{
	// No function name or no target object: cannot call fail-function
	if (!Text || !Text->GetCStr() || !Text->GetCStr()[0] || !Target) return 0;
	// Compose fail-function name
	char szFunctionFailed[1024+1]; sprintf(szFunctionFailed,"~%sFailed",Text->GetCStr());
	// Call failed-function
	return Target->Call(szFunctionFailed,&C4AulParSet(C4VObj(cObj), Tx, C4VInt(Ty), C4VObj(Target2)))._getInt();
	// Extreme caution notice: the script call might do just about anything
	// including clearing all commands (including this) i.e. through a call
	// to SetCommand. Thus, we must not do anything in this command anymore
	// after the script call.
	}

int32_t C4Command::GetExpGain()
	{
	// return exp gained by this command
	switch (Command)
		{
		// internal
		case C4CMD_Wait:
		case C4CMD_Transfer:
		case C4CMD_Retry:
		case C4CMD_Call:
			return 0;

		// regular move commands
		case C4CMD_Follow:
		case C4CMD_MoveTo:
		case C4CMD_Enter:
		case C4CMD_Exit:
			return 1;

		// simple activities
		case C4CMD_Grab:
		case C4CMD_UnGrab:
		case C4CMD_Throw:
		case C4CMD_Jump:
		case C4CMD_Get:
		case C4CMD_Put:
		case C4CMD_Drop:
		case C4CMD_Activate:
		case C4CMD_PushTo:
		case C4CMD_Dig:
		case C4CMD_Context:
		case C4CMD_Buy:
		case C4CMD_Sell:
		case C4CMD_Take:
		case C4CMD_Take2:
			return 1;

		// not that simple
		case C4CMD_Acquire:
		case C4CMD_Home:
			return 2;

		// advanced activities
		case C4CMD_Chop:
		case C4CMD_Build:
		case C4CMD_Construct:
		case C4CMD_Energy:
			return 5;

		// victory!
		case C4CMD_Attack:
			return 15;
		}
	// unknown command
	return 0;
	}
