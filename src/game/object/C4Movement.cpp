/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001-2002, 2004-2006, 2008  Sven Eberhardt
 * Copyright (c) 2002-2004  Peter Wortmann
 * Copyright (c) 2006, 2008  Günther Brammer
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

/* Object motion, collision, friction */

#include <C4Include.h>
#include <C4Object.h>

#include <C4Physics.h>
#include <C4SolidMask.h>
#include <C4Landscape.h>
#include <C4Game.h>

/* Some physical constants */

const FIXED FRedirect=FIXED100(50);
const FIXED FFriction=FIXED100(30);
const FIXED FixFullCircle=itofix(360),FixHalfCircle=FixFullCircle/2;
const FIXED FloatFriction=FIXED100(2);
const FIXED RotateAccel=FIXED100(20);
const FIXED FloatAccel=FIXED100(10);
const FIXED WalkAccel=FIXED100(50),SwimAccel=FIXED100(20);
const FIXED HitSpeed1=FIXED100(150); // Hit Event
const FIXED HitSpeed2=itofix(2); // Cross Check Hit
const FIXED HitSpeed3=itofix(6); // Scale disable, kneel
const FIXED HitSpeed4=itofix(8); // Flat

/* Some helper functions */

void RedirectForce(FIXED &from, FIXED &to, int32_t tdir)
  {
  FIXED fred;
  fred=Min(Abs(from), FRedirect);
  from-=fred*Sign(from);
  to+=fred*tdir;
  }

void ApplyFriction(FIXED &tval, int32_t percent)
  {
  FIXED ffric=FFriction*percent/100;
  if (tval>+ffric) { tval-=ffric; return; }
  if (tval<-ffric) { tval+=ffric; return; }
  tval=0;
  }

// Compares all Shape.VtxContactCNAT[] CNAT flags to search flag.
// Returns true if CNAT match has been found.

bool ContactVtxCNAT(C4Object *cobj, BYTE cnat_dir)
  {
  int32_t cnt;
  bool fcontact=false;
  for (cnt=0; cnt<cobj->Shape.VtxNum; cnt++)
    if (cobj->Shape.VtxContactCNAT[cnt] & cnat_dir)
      fcontact=true;
  return fcontact;
  }

// Finds first vertex with contact flag set.
// Returns -1/0/+1 for relation on vertex to object center.

int32_t ContactVtxWeight(C4Object *cobj)
  {
  int32_t cnt;
  for (cnt=0; cnt<cobj->Shape.VtxNum; cnt++)
    if (cobj->Shape.VtxContactCNAT[cnt])
      {
      if (cobj->Shape.VtxX[cnt]<0) return -1;
      if (cobj->Shape.VtxX[cnt]>0) return +1;
      }
  return 0;
  }

// ContactVtxFriction: Returns 0-100 friction value of first
//                     contacted vertex;

int32_t ContactVtxFriction(C4Object *cobj)
  {
  int32_t cnt;
  for (cnt=0; cnt<cobj->Shape.VtxNum; cnt++)
    if (cobj->Shape.VtxContactCNAT[cnt])
      return cobj->Shape.VtxFriction[cnt];
  return 0;
  }

const char *CNATName(int32_t cnat)
  {
  switch (cnat)
    {
    case CNAT_None:   return "None";
    case CNAT_Left:   return "Left";
    case CNAT_Right:  return "Right";
    case CNAT_Top:    return "Top";
    case CNAT_Bottom: return "Bottom";
    case CNAT_Center: return "Center";
    }
  return "Undefined";
  }

bool C4Object::Contact(int32_t iCNAT)
	{
	if (Def->ContactFunctionCalls)
		{
		return !! Call(FormatString(PSF_Contact, CNATName(iCNAT)).getData());
		}
  return false;
	}

void C4Object::DoMotion(int32_t mx, int32_t my)
  {
  if (pSolidMaskData) pSolidMaskData->Remove(true, true);
  fix_x += mx; fix_y += my;
  }

static inline int32_t ForceLimits(int32_t &rVal, int32_t iLow, int32_t iHi)
	{
	if (rVal<iLow) { rVal=iLow; return -1; }
	if (rVal>iHi)  { rVal=iHi;  return +1; }
	return 0;
	}

void C4Object::TargetBounds(int32_t &ctco, int32_t limit_low, int32_t limit_hi, int32_t cnat_low, int32_t cnat_hi)
	{
	switch (ForceLimits(ctco,limit_low,limit_hi))
		{
		case -1:
			// stop
			if (cnat_low==CNAT_Left) xdir=0; else ydir=0;
			// do calls
			Contact(cnat_low);
			break;
		case +1:
			// stop
			if (cnat_hi==CNAT_Right) xdir=0; else ydir=0;
			// do calls
			Contact(cnat_hi);
			break;
		}
	}

int32_t C4Object::ContactCheck(int32_t iAtX, int32_t iAtY)
	{
	// Check shape contact at given position
	Shape.ContactCheck(iAtX,iAtY);

	// Store shape contact values in object t_contact
	t_contact=Shape.ContactCNAT;

  // Contact script call for the first contacted cnat
  if (Shape.ContactCNAT)
    for (int32_t ccnat=0; ccnat<4; ccnat++) // Left, right, top bottom
      if (Shape.ContactCNAT & (1<<ccnat))
				if (Contact(1<<ccnat))
					break; // Will stop on first positive return contact call!

	// Return shape contact count
	return Shape.ContactCount;
	}

void C4Object::SideBounds(int32_t &ctcox)
	{
	// layer bounds
	if (pLayer) if (pLayer->Def->BorderBound & C4D_Border_Layer)
		if (!Action.pActionDef || Action.pActionDef->GetPropertyInt(P_Procedure) != DFA_ATTACH)
			if (Category & C4D_StaticBack)
				TargetBounds(ctcox,pLayer->GetX()+pLayer->Shape.GetX(),pLayer->GetX()+pLayer->Shape.GetX()+pLayer->Shape.Wdt,CNAT_Left,CNAT_Right);
			else
				TargetBounds(ctcox,pLayer->GetX()+pLayer->Shape.GetX()-Shape.GetX(),pLayer->GetX()+pLayer->Shape.GetX()+pLayer->Shape.Wdt+Shape.GetX(),CNAT_Left,CNAT_Right);
	// landscape bounds
  if (Def->BorderBound & C4D_Border_Sides)
		TargetBounds(ctcox,0-Shape.GetX(),GBackWdt+Shape.GetX(),CNAT_Left,CNAT_Right);
	}

void C4Object::VerticalBounds(int32_t &ctcoy)
	{
	// layer bounds
	if (pLayer) if (pLayer->Def->BorderBound & C4D_Border_Layer)
		if (!Action.pActionDef || Action.pActionDef->GetPropertyInt(P_Procedure) != DFA_ATTACH)
			if (Category & C4D_StaticBack)
				TargetBounds(ctcoy,pLayer->GetY()+pLayer->Shape.GetY(),pLayer->GetY()+pLayer->Shape.GetY()+pLayer->Shape.Hgt,CNAT_Top,CNAT_Bottom);
			else
				TargetBounds(ctcoy,pLayer->GetY()+pLayer->Shape.GetY()-Shape.GetY(),pLayer->GetY()+pLayer->Shape.GetY()+pLayer->Shape.Hgt+Shape.GetY(),CNAT_Top,CNAT_Bottom);
	// landscape bounds
  if (Def->BorderBound & C4D_Border_Top)
		TargetBounds(ctcoy,0-Shape.GetY(),+1000000,CNAT_Top,CNAT_Bottom);
  if (Def->BorderBound & C4D_Border_Bottom)
		TargetBounds(ctcoy,-1000000,GBackHgt+Shape.GetY(),CNAT_Top,CNAT_Bottom);
	}

void C4Object::DoMovement()
	{
	int32_t ctcox,ctcoy,ctcor/*,ctx,cty*/,iContact=0;
	bool fAnyContact=false; int iContacts = 0;
	BYTE fTurned=0,fRedirectYR=0,fNoAttach=0;
	// Restrictions
	if (Def->NoHorizontalMove) xdir=0;
	// Dig free target area
	if (Action.pActionDef)
		if (Action.pActionDef->GetPropertyInt(P_DigFree))
			{
			// Shape size square
			if (Action.pActionDef->GetPropertyInt(P_DigFree)==1)
				{
				ctcox=fixtoi(fix_x+xdir); ctcoy=fixtoi(fix_y+ydir);
				::Landscape.DigFreeRect(ctcox+Shape.GetX(),ctcoy+Shape.GetY(),Shape.Wdt,Shape.Hgt,Action.Data,this);
				}
			// Free size round (variable size)
			else
				{
				ctcox=fixtoi(fix_x+xdir); ctcoy=fixtoi(fix_y+ydir);
				int32_t rad = Action.pActionDef->GetPropertyInt(P_DigFree);
				if (Con<FullCon) rad = rad*6*Con/5/FullCon;
				::Landscape.DigFree(ctcox,ctcoy-1,rad,Action.Data,this);
				}
			}

	// store previous position
	int32_t ix0=GetX(); int32_t iy0=GetY();

	// store previous movement and ocf
	FIXED oldxdir(xdir), oldydir(ydir);
	uint32_t old_ocf = OCF;

	FIXED new_x = fix_x + xdir;
	FIXED new_y = fix_y + ydir;
	ctcox=fixtoi(new_x);
	SideBounds(ctcox);
	if (!Action.t_attach) // Unattached movement  = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
		{
		// Horizontal movement - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		// Move to target
		while (GetX()!=ctcox)
			{
			// Next step
			int step = Sign(ctcox - GetX());
			if (iContact=ContactCheck(GetX() + step, GetY()))
				{
				fAnyContact=true; iContacts |= t_contact;
				// Abort horizontal movement
				ctcox = GetX();
				new_x = fix_x;
				// Vertical redirection (always)
				RedirectForce(xdir,ydir,-1);
				ApplyFriction(ydir,ContactVtxFriction(this));
				}
			else // Free horizontal movement
				DoMotion(step, 0);
			}
		// Vertical movement - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		// Movement target
		ctcoy=fixtoi(fix_y + ydir);
		new_y = fix_y + ydir;
		// Movement bounds (vertical)
		VerticalBounds(ctcoy);
		// Move to target
		while (GetY()!=ctcoy)
			{
			// Next step
			int step = Sign(ctcoy - GetY());
			if (iContact=ContactCheck(GetX(), GetY() + step))
				{
				fAnyContact=true; iContacts |= t_contact;
				ctcoy=GetY(); new_y = fix_y;
				// Vertical contact horizontal friction
				ApplyFriction(xdir,ContactVtxFriction(this));
				// Redirection slide or rotate
				if (!ContactVtxCNAT(this,CNAT_Left))
					RedirectForce(ydir,xdir,-1);
				else if (!ContactVtxCNAT(this,CNAT_Right))
					RedirectForce(ydir,xdir,+1);
				else
					{
					// living things are always capable of keeping their rotation
					if (OCF & OCF_Rotate) if (iContact==1) if (!Alive)
						{
						RedirectForce(ydir,rdir,-ContactVtxWeight(this));
						fRedirectYR=1;
						}
					ydir=0;
					}
				}
			else // Free vertical movement
				DoMotion(0,step);
			}
		}
	if (Action.t_attach) // Attached movement = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
		{
		ctcoy=fixtoi(new_y);
		VerticalBounds(ctcoy);
		bool at_xovr,at_yovr;
		// Move to target
		do
			{
			at_xovr=at_yovr=0;
			// Set next step target
			int step_x = Sign(ctcox-GetX());
			int step_y = Sign(ctcoy-GetY());
			int32_t ctx = GetX() + step_x; int32_t cty = GetY() + step_y;
			// Attachment check
			if (!Shape.Attach(ctx,cty,Action.t_attach))
				fNoAttach=1;
			else
				{
				// Attachment change to ctx/cty overrides ctco target
				if (cty != GetY() + step_y) at_yovr = true;
				if (ctx != GetX() + step_x) at_xovr = true;
				}
			// Contact check & evaluation
			if (iContact=ContactCheck(ctx,cty))
				{
				fAnyContact=true; iContacts |= t_contact;
				// Abort movement
				ctcox=GetX(); new_x = fix_x;
				ctcoy=GetY(); new_y = fix_y;
				}
			else // Continue free movement
				DoMotion(ctx-GetX(),cty-GetY());
			if (at_xovr) { ctcox=GetX(); xdir=Fix0; new_x = fix_x; }
			if (at_yovr) { ctcoy=GetY(); ydir=Fix0; new_y = fix_y; }
			}
		while ((GetX()!=ctcox) || (GetY()!=ctcoy));
		}
	fix_x = new_x;
	fix_y = new_y;
	// Rotation  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	if (OCF & OCF_Rotate && !!rdir)
		{
		// Set target
		fix_r+=rdir*5;
		// Rotation limit
		if (Def->Rotateable>1)
			{
			if (fix_r > itofix(Def->Rotateable))
				{ fix_r = itofix(Def->Rotateable); rdir=0; }
			if (fix_r < itofix(-Def->Rotateable))
				{ fix_r = itofix(-Def->Rotateable); rdir=0; }
			}
		ctcor=fixtoi(fix_r);
		int32_t ctx=GetX(); int32_t cty=GetY();
		// Move to target
		while (r!=ctcor)
			{
			// Save step undos
			int32_t lcobjr=r; C4Shape lshape=Shape;
			// Try next step
			r+=Sign(ctcor-r);
			UpdateShape();
			// attached rotation: rotate around attachment pos
			if (Action.t_attach && !fNoAttach)
				{
				// more accurately, attachment should be evaluated by a rotation around the attachment vertex
				// however, as long as this code is only used for some surfaces adjustment for large vehicles,
				// it's enough to assume rotation around the center
				ctx=GetX(); cty=GetY();
				// evaluate attachment, but do not bother about attachment loss
				// that will then be done in next execution cycle
				Shape.Attach(ctx,cty,Action.t_attach);
				}
			// check for contact
			if (iContact=ContactCheck(ctx,cty)) // Contact
				{
				fAnyContact=true; iContacts |= t_contact;
				// Undo step and abort movement
				Shape=lshape;
				r=lcobjr;
				ctcor=r;
				fix_r=itofix(r);
				// last UpdateShape-call might have changed sector lists!
				UpdatePos();
				// Redirect to GetY()
				if (iContact==1) if (!fRedirectYR)
					RedirectForce(rdir,ydir,-1);
				// Stop rotation
				rdir=0;
				}
			else
				{
				fTurned=1;
				fix_x=itofix(ctx); fix_y=itofix(cty);
				}
			}
		// Circle bounds
		if (fix_r<-FixHalfCircle) { fix_r+=FixFullCircle; r=fixtoi(fix_r); }
		if (fix_r>+FixHalfCircle) { fix_r-=FixFullCircle; r=fixtoi(fix_r); }
		}
	// Reput solid mask: Might have been removed by motion or
	// motion might be out of date from last frame.
	UpdateSolidMask(true);
	// Misc checks ===========================================================================================
	// InLiquid check
	// this equals C4Object::UpdateLiquid, but the "fNoAttach=false;"-line
	if (IsInLiquidCheck()) // In Liquid
		{
		if (!InLiquid) // Enter liquid
			{
			if (OCF & OCF_HitSpeed2) if (Mass>3)
				Splash(GetX(),GetY()+1,Min(Shape.Wdt*Shape.Hgt/10,20),this);
			fNoAttach=false;
			InLiquid=1;
			}
		}
	else // Out of liquid
		{
		if (InLiquid) // Leave liquid
			InLiquid=0;
		}
	// Contact Action
	if (fAnyContact)
		{
		t_contact = iContacts;
		ContactAction();
		}
	// Attachment Loss Action
	if (fNoAttach)
		NoAttachAction();
	// Movement Script Execution
	if (fAnyContact)
		{
		C4AulParSet pars(C4VInt(fixtoi(oldxdir, 100)), C4VInt(fixtoi(oldydir, 100)));
		if (old_ocf & OCF_HitSpeed1) Call(PSF_Hit, &pars);
		if (old_ocf & OCF_HitSpeed2) Call(PSF_Hit2, &pars);
		if (old_ocf & OCF_HitSpeed3) Call(PSF_Hit3, &pars);
		}
	// Rotation gfx
	if (fTurned)
		UpdateFace(true);
	else
		// pos changed?
		if ((ix0-GetX())|(iy0-GetY())) UpdatePos();
	}

void C4Object::Stabilize()
  {
	// def allows stabilization?
	if (Def->NoStabilize) return;
	// normalize angle
	int32_t nr = r; while(nr < -180) nr+=360; while(nr > 180) nr-=360;
	if (nr!=0)
    if (Inside<int32_t>(nr,-StableRange,+StableRange))
      {
      // Save step undos
      int32_t lcobjr=r;
      C4Shape lshape=Shape;
      // Try rotation
      r=0;
      UpdateShape();
      if (ContactCheck(GetX(),GetY()))
        { // Undo rotation
        Shape=lshape;
        r=lcobjr;
        }
      else
        { // Stabilization okay
        fix_r=itofix(r);
        UpdateFace(true);
        }
      }
  }

void C4Object::CopyMotion(C4Object *from)
	{
	// Designed for contained objects, no static
	if (fix_x != from->fix_x || fix_y != from->fix_y)
		{
		fix_x=from->fix_x; fix_y=from->fix_y;
		// Resort into sectors
		UpdatePos();
		}
	xdir=from->xdir; ydir=from->ydir;
	}

void C4Object::ForcePosition(int32_t tx, int32_t ty)
	{
	fix_x=itofix(tx); fix_y=itofix(ty);
	UpdatePos();
	UpdateSolidMask(false);
	}

void C4Object::MovePosition(int32_t dx, int32_t dy)
	{
	// move object position; repositions SolidMask
	if (pSolidMaskData) pSolidMaskData->Remove(true, true);
	fix_x+=dx;
	fix_y+=dy;
	UpdatePos();
	UpdateSolidMask(true);
	}


bool C4Object::ExecMovement() // Every Tick1 by Execute
  {

  // Containment check
  if (Contained)
    {
    CopyMotion(Contained);

    return true;
    }

  // General mobility check
  if (Category & C4D_StaticBack) return false;

  // Movement execution
  if (Mobile) // Object is moving
    {
    // Move object
    DoMovement();
    // Demobilization check
    if ((xdir==0) && (ydir==0) && (rdir==0)) Mobile=0;
    // Check for stabilization
    if (rdir==0) Stabilize();
    }
  else // Object is static
    {
    // Check for stabilization
    Stabilize();
    // Check for mobilization
    if (!::Game.iTick10)
      {
      // Gravity mobilization
      xdir=ydir=rdir=0;
      fix_r=itofix(r);
      Mobile=1;
      }
    }

  // Enforce zero rotation
  if (!Def->Rotateable) r=0;

  // Out of bounds check
  if ((!Inside<int32_t>(GetX(),0,GBackWdt) && !(Def->BorderBound & C4D_Border_Sides)) || (GetY()>GBackHgt && !(Def->BorderBound & C4D_Border_Bottom)))
		// Never remove attached objects: If they are truly outside landscape, their target will be removed,
		//  and the attached objects follow one frame later
		if (!Action.pActionDef || !Action.Target || Action.pActionDef->GetPropertyInt(P_Procedure) != DFA_ATTACH)
			{
			bool fRemove = true;
			// never remove HUD objects
			if (Category & C4D_Parallax)
				{
				int parX, parY;
				GetParallaxity(&parX, &parY);
				fRemove = false;
				if (GetX()>GBackWdt || GetY()>GBackHgt) fRemove = true; // except if they are really out of the viewport to the right...
				else if (GetX()<0 && !!parX) fRemove = true; // ...or it's not HUD horizontally and it's out to the left
				else if (!parX && GetX()<-GBackWdt) fRemove = true; // ...or it's HUD horizontally and it's out to the left
				}
			if (fRemove)
				{
				AssignDeath(true);
				AssignRemoval();
				}
			}

  return true;
  }

bool SimFlight(FIXED &x, FIXED &y, FIXED &xdir, FIXED &ydir, int32_t iDensityMin, int32_t iDensityMax, int32_t iIter)
{
  bool fBreak = false;
	int32_t ctcox,ctcoy,cx,cy;
	cx = fixtoi(x); cy = fixtoi(y);
  do
		{
		if(!iIter--) return false;
    // Set target position by momentum
    x+=xdir; y+=ydir;
    // Movement to target
    ctcox=fixtoi(x); ctcoy=fixtoi(y);
    // Bounds
    if (!Inside<int32_t>(ctcox,0,GBackWdt) || (ctcoy>=GBackHgt)) return false;
    // Move to target
    do
      {
      // Set next step target
      cx+=Sign(ctcox-cx); cy+=Sign(ctcoy-cy);
      // Contact check
      if(Inside(GBackDensity(cx,cy), iDensityMin, iDensityMax))
				{ fBreak = true; break; }
      }
    while ((cx!=ctcox) || (cy!=ctcoy));
    // Adjust GravAccel once per frame
    ydir+=GravAccel;
		}
	while(!fBreak);
	// write position back
	x = itofix(cx); y = itofix(cy);
	// ok
	return true;
}

bool SimFlightHitsLiquid(FIXED fcx, FIXED fcy, FIXED xdir, FIXED ydir)
  {
	// Start in water?
	if(DensityLiquid(GBackDensity(fixtoi(fcx), fixtoi(fcy))))
		if(!SimFlight(fcx, fcy, xdir, ydir, 0, C4M_Liquid - 1, 10))
			return false;
	// Hits liquid?
	if(!SimFlight(fcx, fcy, xdir, ydir, C4M_Liquid, 100, -1))
		return false;
	// liquid & deep enough?
	return GBackLiquid(fixtoi(fcx), fixtoi(fcy)) && GBackLiquid(fixtoi(fcx), fixtoi(fcy) + 9);
  }

