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

/* Object motion, collision, friction */

#include "C4Include.h"

#include "game/C4Physics.h"
#include "landscape/C4Landscape.h"
#include "landscape/C4SolidMask.h"
#include "object/C4Def.h"
#include "object/C4Object.h"
#include "script/C4Effect.h"

/* Some physical constants */

const C4Real FRedirect = C4REAL100(50);
const C4Real FFriction = C4REAL100(30);
const C4Real FixFullCircle = itofix(360);
const C4Real FixHalfCircle = FixFullCircle / 2;
const C4Real FloatFriction = C4REAL100(2);
const C4Real RotateAccel = C4REAL100(20);
const C4Real HitSpeed1 = C4REAL100(150); // Hit Event
const C4Real HitSpeed2 = itofix(2); // Cross Check Hit
const C4Real HitSpeed3 = itofix(6); // Scale disable, kneel
const C4Real HitSpeed4 = itofix(8); // Flat
const C4Real DefaultGravAccel = C4REAL100(20);

/* Some helper functions */

void RedirectForce(C4Real &from, C4Real &to, int32_t tdir)
{
	C4Real fred;
	fred = std::min(Abs(from), FRedirect);
	from -= fred * Sign(from);
	to += fred * tdir;
}

void ApplyFriction(C4Real &tval, int32_t percent)
{
	C4Real ffric = FFriction * percent / 100;
	if (tval > +ffric)
	{
		tval -= ffric;
	}
	else if (tval < -ffric)
	{
		tval += ffric;
	}
	else
	{
		tval = 0;
	}
}

// Compares all Shape.VtxContactCNAT[] CNAT flags to search flag.
// Returns true if CNAT match has been found.

bool ContactVtxCNAT(C4Object *cobj, BYTE cnat_dir)
{
	for (int32_t cnt = 0; cnt < cobj->Shape.VtxNum; cnt++)
	{
		if (cobj->Shape.VtxContactCNAT[cnt] & cnat_dir)
		{
			return true;
		}
	}
	return false;
}

// Finds first vertex with contact flag set.
// Returns -1/0/+1 for relation on vertex to object center.

int32_t ContactVtxWeight(C4Object *cobj)
{
	for (int32_t cnt = 0; cnt < cobj->Shape.VtxNum; cnt++)
	{
		if (cobj->Shape.VtxContactCNAT[cnt])
		{
			if (cobj->Shape.VtxX[cnt] < 0)
			{
				return -1;
			}
			if (cobj->Shape.VtxX[cnt] > 0)
			{
				return +1;
			}
		}
	}
	return 0;
}

// ContactVtxFriction: Returns 0-100 friction value of first
//                     contacted vertex;

int32_t ContactVtxFriction(C4Object *cobj)
{
	for (int32_t cnt = 0; cnt < cobj->Shape.VtxNum; cnt++)
	{
		if (cobj->Shape.VtxContactCNAT[cnt])
		{
			return cobj->Shape.VtxFriction[cnt];
		}
	}
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
	if (GetPropertyInt(P_ContactCalls))
	{
		return !! Call(FormatString(PSF_Contact, CNATName(iCNAT)).getData());
	}
	return false;
}

void C4Object::DoMotion(int32_t mx, int32_t my)
{
	RemoveSolidMask(true);
	fix_x += mx;
	fix_y += my;
}

void C4Object::StopAndContact(C4Real & ctco, C4Real limit, C4Real & speed, int32_t cnat)
{
	ctco = limit;
	speed = 0;
	Contact(cnat);
}

int32_t C4Object::ContactCheck(int32_t iAtX, int32_t iAtY, uint32_t *border_hack_contacts, bool collide_halfvehic)
{
	// Check shape contact at given position
	Shape.ContactCheck(iAtX, iAtY, border_hack_contacts, collide_halfvehic);

	// Store shape contact values in object t_contact
	t_contact = Shape.ContactCNAT;

	// Contact script call for the first contacted cnat
	if (Shape.ContactCNAT)
	{
		for (int32_t ccnat = 0; ccnat < 4; ccnat++) // Left, right, top bottom
		{
			if (Shape.ContactCNAT & (1 << ccnat))
			{
				if (Contact(1 << ccnat))
				{
					break; // Will stop on first positive return contact call!
				}
			}
		}
	}

	// Return shape contact count
	return Shape.ContactCount;
}

// Stop the object and do contact calls if it collides with the border
void C4Object::SideBounds(C4Real &ctcox)
{
	// layer bounds
	if (Layer && (Layer->GetPropertyInt(P_BorderBound) & C4D_Border_Layer))
	{
		C4PropList* pActionDef = GetAction();
		if (!pActionDef || pActionDef->GetPropertyP(P_Procedure) != DFA_ATTACH)
		{
			C4Real lbound = itofix(Layer->GetX() + Layer->Shape.GetX() - Shape.GetX());
			C4Real rbound = itofix(Layer->GetX() + Layer->Shape.GetX() + Layer->Shape.Wdt - (Shape.GetX() + Shape.Wdt));
			if (ctcox < lbound)
			{
				StopAndContact(ctcox, lbound, xdir, CNAT_Left);
			}
			if (ctcox > rbound)
			{
				StopAndContact(ctcox, rbound, xdir, CNAT_Right);
			}
		}
	}
	// landscape bounds
	C4Real lbound = itofix(0 - Shape.GetX());
	C4Real rbound = itofix(::Landscape.GetWidth() - (Shape.GetX() + Shape.Wdt));
	if (ctcox < lbound && (GetPropertyInt(P_BorderBound) & C4D_Border_Sides))
	{
		StopAndContact(ctcox, lbound, xdir, CNAT_Left);
	}
	if (ctcox > rbound && (GetPropertyInt(P_BorderBound) & C4D_Border_Sides))
	{
		StopAndContact(ctcox, rbound, xdir, CNAT_Right);
	}
}

void C4Object::VerticalBounds(C4Real &ctcoy)
{
	// layer bounds
	if (Layer && (Layer->GetPropertyInt(P_BorderBound) & C4D_Border_Layer))
	{
		C4PropList* pActionDef = GetAction();
		if (!pActionDef || pActionDef->GetPropertyP(P_Procedure) != DFA_ATTACH)
		{
			C4Real tbound = itofix(Layer->GetY() + Layer->Shape.GetY() - Shape.GetY());
			C4Real bbound = itofix(Layer->GetY() + Layer->Shape.GetY() + Layer->Shape.Hgt - (Shape.GetY() + Shape.Hgt));
			if (ctcoy < tbound)
			{
				StopAndContact(ctcoy, tbound, ydir, CNAT_Top);
			}
			if (ctcoy > bbound)
			{
				StopAndContact(ctcoy, bbound, ydir, CNAT_Bottom);
			}
		}
	}
	// landscape bounds
	C4Real tbound = itofix(0 - Shape.GetY());
	C4Real bbound = itofix(::Landscape.GetHeight() - (Shape.GetY() + Shape.Hgt));
	if (ctcoy < tbound && (GetPropertyInt(P_BorderBound) & C4D_Border_Top))
	{
		StopAndContact(ctcoy, tbound, ydir, CNAT_Top);
	}
	if (ctcoy > bbound && (GetPropertyInt(P_BorderBound) & C4D_Border_Bottom))
	{
		StopAndContact(ctcoy, bbound, ydir, CNAT_Bottom);
	}
}

void C4Object::DoMovement()
{
	int32_t iContact = 0;
	bool fAnyContact = false;
	int iContacts = 0;
	BYTE fTurned = 0;
	BYTE fRedirectYR = 0;
	BYTE fNoAttach = 0;
	// Restrictions
	if (Def->NoHorizontalMove)
	{
		xdir = 0;
	}
	// Dig free target area
	C4PropList* pActionDef = GetAction();
	if (pActionDef)
	{
		if (pActionDef->GetPropertyInt(P_DigFree))
		{
			int ctcox, ctcoy;
			// Shape size square
			if (pActionDef->GetPropertyInt(P_DigFree) == 1)
			{
				ctcox = fixtoi(fix_x + xdir);
				ctcoy = fixtoi(fix_y + ydir);
				::Landscape.DigFreeRect(ctcox + Shape.GetX(), ctcoy + Shape.GetY(), Shape.Wdt, Shape.Hgt, this);
			}
			// Free size round (variable size)
			else
			{
				ctcox = fixtoi(fix_x + xdir);
				ctcoy = fixtoi(fix_y + ydir);
				int32_t rad = pActionDef->GetPropertyInt(P_DigFree);
				if (Con < FullCon)
				{
					rad = rad * 6 * Con / 5 / FullCon;
				}
				::Landscape.DigFree(ctcox, ctcoy - 1, rad, this);
			}
		}
	}

	// store previous movement and ocf
	C4Real oldxdir(xdir);
	C4Real oldydir(ydir);
	uint32_t old_ocf = OCF;

	bool fMoved = false;
	C4Real new_x = fix_x + xdir;
	C4Real new_y = fix_y + ydir;
	SideBounds(new_x);

	if (!Action.t_attach) // Unattached movement  = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
	{
		// Horizontal movement - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		// Move to target
		while (fixtoi(new_x) != fixtoi(fix_x))
		{
			// Next step
			int step = Sign(new_x - fix_x);
			uint32_t border_hack_contacts = 0;
			iContact = ContactCheck(GetX() + step, GetY(), &border_hack_contacts);
			if (iContact || border_hack_contacts)
			{
				fAnyContact = true;
				iContacts |= t_contact | border_hack_contacts;
			}
			if (iContact)
			{
				// Abort horizontal movement
				new_x = fix_x;
				// Vertical redirection (always)
				RedirectForce(xdir, ydir, -1);
				ApplyFriction(ydir, ContactVtxFriction(this));
			}
			else // Free horizontal movement
			{
				DoMotion(step, 0);
				fMoved = true;
			}
		}
		// Vertical movement - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		// Movement target
		new_y = fix_y + ydir;
		// Movement bounds (vertical)
		VerticalBounds(new_y);
		// Move to target
		while (fixtoi(new_y) != fixtoi(fix_y))
		{
			// Next step
			int step = Sign(new_y - fix_y);
			if ((iContact = ContactCheck(GetX(), GetY() + step, nullptr, ydir > 0)))
			{
				fAnyContact = true;
				iContacts |= t_contact;
				new_y = fix_y;
				// Vertical contact horizontal friction
				ApplyFriction(xdir, ContactVtxFriction(this));
				// Redirection slide or rotate
				if (!ContactVtxCNAT(this, CNAT_Left))
				{
					RedirectForce(ydir, xdir, -1);
				}
				else if (!ContactVtxCNAT(this, CNAT_Right))
				{
					RedirectForce(ydir, xdir, +1);
				}
				else
				{
					// living things are always capable of keeping their rotation
					if ((OCF & OCF_Rotate) && iContact == 1 && !Alive)
					{
						RedirectForce(ydir, rdir, -ContactVtxWeight(this));
						fRedirectYR = 1;
					}
					ydir = 0;
				}
			}
			else // Free vertical movement
			{
				DoMotion(0, step);
				fMoved = true;
			}
		}
	}
	if (Action.t_attach) // Attached movement = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
	{
		VerticalBounds(new_y);
		// Move to target
		do
		{
			// Set next step target
			int step_x = 0;
			int step_y = 0;
			if (fixtoi(new_x) != GetX())
			{
				step_x = Sign(fixtoi(new_x) - GetX());
			}
			else if (fixtoi(new_y) != GetY())
			{
				step_y = Sign(fixtoi(new_y) - GetY());
			}
			int32_t ctx = GetX() + step_x;
			int32_t cty = GetY() + step_y;
			// Attachment check
			if (!Shape.Attach(ctx, cty, Action.t_attach))
			{
				fNoAttach = 1;
			}
			else
			{
				// Attachment change to ctx/cty overrides target
				if (ctx != GetX() + step_x)
				{
					xdir = Fix0;
					new_x = itofix(ctx);
				}
				if (cty != GetY() + step_y)
				{
					ydir = Fix0;
					new_y = itofix(cty);
				}
			}
			// Contact check & evaluation
			uint32_t border_hack_contacts = 0;
			iContact = ContactCheck(ctx, cty, &border_hack_contacts);
			if (iContact || border_hack_contacts)
			{
				fAnyContact = true;
				iContacts |= border_hack_contacts | t_contact;
			}
			if (iContact)
			{
				// Abort movement
				if (ctx != GetX())
				{
					ctx = GetX();
					new_x = fix_x;
				}
				if (cty != GetY())
				{
					cty = GetY();
					new_y = fix_y;
				}
			}
			DoMotion(ctx - GetX(), cty - GetY());
			fMoved = true;
		}
		while (fixtoi(new_x) != GetX() || fixtoi(new_y) != GetY());
	}

	if (fix_x != new_x || fix_y != new_y)
	{
		fMoved = true;
		RemoveSolidMask(true);
		fix_x = new_x;
		fix_y = new_y;
	}
	// Rotation  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	if ((OCF & OCF_Rotate) && !!rdir)
	{
		C4Real target_r = fix_r + rdir * 5;
		// Rotation limit
		if (Def->Rotateable>1)
		{
			if (target_r > itofix(Def->Rotateable))
			{
				target_r = itofix(Def->Rotateable);
				rdir = 0;
			}
			if (target_r < itofix(-Def->Rotateable))
			{
				target_r = itofix(-Def->Rotateable);
				rdir = 0;
			}
		}
		int32_t ctx = GetX();
		int32_t cty = GetY();
		// Move to target
		while (fixtoi(fix_r) != fixtoi(target_r))
		{
			// Save step undos
			C4Real lcobjr = fix_r;
			C4Shape lshape = Shape;
			// Try next step
			fix_r += Sign(target_r - fix_r);
			UpdateShape();
			// attached rotation: rotate around attachment pos
			if (Action.t_attach && !fNoAttach)
			{
				// more accurately, attachment should be evaluated by a rotation around the attachment vertex
				// however, as long as this code is only used for some surfaces adjustment for large vehicles,
				// it's enough to assume rotation around the center
				ctx = GetX();
				cty = GetY();
				// evaluate attachment, but do not bother about attachment loss
				// that will then be done in next execution cycle
				Shape.Attach(ctx, cty, Action.t_attach);
			}
			// check for contact
			if ((iContact = ContactCheck(ctx, cty))) // Contact
			{
				fAnyContact = true;
				iContacts |= t_contact;
				// Undo step and abort movement
				Shape = lshape;
				target_r = fix_r = lcobjr;
				// last UpdateShape-call might have changed sector lists!
				UpdatePos();
				// Redirect to GetY()
				if (iContact == 1 && !fRedirectYR)
				{
					RedirectForce(rdir, ydir, -1);
				}
				// Stop rotation
				rdir = 0;
			}
			else
			{
				fTurned = 1;
				if (ctx != GetX() || cty != GetY())
				{
					fix_x = itofix(ctx);
					fix_y = itofix(cty);
				}
			}
		}
		// Circle bounds
		if (target_r < -FixHalfCircle)
		{
			target_r += FixFullCircle;
		}
		if (target_r > +FixHalfCircle)
		{
			target_r -= FixFullCircle;
		}
		fix_r = target_r;
	}
	// Reput solid mask if moved by motion
	if (fMoved || fTurned)
	{
		UpdateSolidMask(true);
	}
	// Misc checks ===========================================================================================
	// InLiquid check
	// this equals C4Object::UpdateLiquid, but the "fNoAttach = false;"-line
	if (IsInLiquidCheck()) // In Liquid
	{
		if (!InLiquid) // Enter liquid
		{
			if (OCF & OCF_HitSpeed2)
			{
				if (Mass > 3)
				{
					Splash();
				}
			}
			fNoAttach = false;
			InLiquid = true;
		}
	}
	else // Out of liquid
	{
		if (InLiquid) // Leave liquid
		{
			InLiquid = false;
		}
	}
	// Contact Action
	if (fAnyContact)
	{
		t_contact = iContacts;
		ContactAction();
	}
	// Attachment Loss Action
	if (fNoAttach)
	{
		NoAttachAction();
	}
	// Movement Script Execution
	if (fAnyContact)
	{
		C4AulParSet pars(fixtoi(oldxdir, 100), fixtoi(oldydir, 100));
		if (old_ocf & OCF_HitSpeed1)
		{
			Call(PSF_Hit, &pars);
		}
		if (old_ocf & OCF_HitSpeed2)
		{
			Call(PSF_Hit2, &pars);
		}
		if (old_ocf & OCF_HitSpeed3)
		{
			Call(PSF_Hit3, &pars);
		}
	}
	// Rotation gfx
	if (fTurned)
	{
		UpdateFace(true);
	}
	else
	{
		// pos changed?
		if (fMoved)
		{
			UpdatePos();
		}
	}
}

void C4Object::Stabilize()
{
	// def allows stabilization?
	if (Def->NoStabilize)
	{
		return;
	}
	// normalize angle
	C4Real nr = fix_r;
	while (nr < itofix(-180))
	{
		nr += 360;
	}
	while (nr > itofix(180))
	{
		nr -= 360;
	}
	if (nr != Fix0)
	{
		if (Inside<C4Real>(nr, itofix(-StableRange), itofix(+StableRange)))
		{
			// Save step undos
			C4Real lcobjr = fix_r;
			C4Shape lshape = Shape;
			// Try rotation
			fix_r = Fix0;
			UpdateShape();
			if (ContactCheck(GetX(), GetY()))
			{ // Undo rotation
				Shape = lshape;
				fix_r = lcobjr;
			}
			else
			{ // Stabilization okay
				UpdateFace(true);
			}
		}
	}
}

void C4Object::CopyMotion(C4Object *from)
{
	// Designed for contained objects, no static
	if (fix_x != from->fix_x || fix_y != from->fix_y)
	{
		fix_x = from->fix_x;
		fix_y = from->fix_y;
		// Resort into sectors
		UpdatePos();
	}
	xdir = from->xdir;
	ydir = from->ydir;
}

void C4Object::ForcePosition(C4Real tx, C4Real ty)
{
	fix_x = tx;
	fix_y = ty;
	UpdatePos();
	UpdateSolidMask(false);
}

void C4Object::MovePosition(int32_t dx, int32_t dy)
{
	MovePosition(itofix(dx), itofix(dy));
}

void C4Object::MovePosition(C4Real dx, C4Real dy)
{
	// move object position; repositions SolidMask
	RemoveSolidMask(true);
	fix_x += dx;
	fix_y += dy;
	UpdatePos();
	UpdateSolidMask(true);
}


bool C4Object::ExecMovement() // Every Tick1 by Execute
{
	// update in which material this object is
	UpdateInMat();

	// Containment check
	if (Contained)
	{
		CopyMotion(Contained);

		return true;
	}

	// General mobility check
	if (Category & C4D_StaticBack)
	{
		return false;
	}

	// Movement execution
	if (Mobile) // Object is moving
	{
		// Move object
		DoMovement();
		// Demobilization check
		if ((xdir == 0) && (ydir == 0) && (rdir == 0))
		{
			Mobile = false;
		}
		// Check for stabilization
		if (rdir == 0)
		{
			Stabilize();
		}
	}
	else // Object is static
	{
		// Check for stabilization
		Stabilize();
		// Check for mobilization
		if (!::Game.iTick10)
		{
			// Gravity mobilization
			xdir = ydir = rdir = 0;
			Mobile = true;
		}
	}

	// Enforce zero rotation
	if (!Def->Rotateable)
	{
		fix_r = Fix0;
	}

	// Out of bounds check
	if ((!Inside<int32_t>(GetX() + Shape.GetX(), -Shape.Wdt, ::Landscape.GetWidth()) && !(GetPropertyInt(P_BorderBound) & C4D_Border_Sides))
	    || ((GetY() + Shape.GetY() > ::Landscape.GetHeight()) && !(GetPropertyInt(P_BorderBound) & C4D_Border_Bottom)))
	{
		C4PropList* pActionDef = GetAction();
		// Never remove attached objects: If they are truly outside landscape, their target will be removed,
		//  and the attached objects follow one frame later
		if (!pActionDef || !Action.Target || pActionDef->GetPropertyP(P_Procedure) != DFA_ATTACH)
		{
			bool fRemove = true;
			// never remove HUD objects
			if (Category & C4D_Parallax)
			{
				int parX, parY;
				GetParallaxity(&parX, &parY);
				fRemove = false;
				if (GetX() > ::Landscape.GetWidth() || GetY() > ::Landscape.GetHeight())
				{
					fRemove = true; // except if they are really out of the viewport to the right...
				}
				else if (GetX() < 0 && !!parX)
				{
					fRemove = true; // ...or it's not HUD horizontally and it's out to the left
				}
				else if (!parX && GetX() < -::Landscape.GetWidth())
				{
					fRemove = true; // ...or it's HUD horizontally and it's out to the left
				}
			}
			if (fRemove)
			{
				AssignDeath(true);
				AssignRemoval();
			}
		}
	}
	return true;
}

bool SimFlight(C4Real &x, C4Real &y, C4Real &xdir, C4Real &ydir, int32_t iDensityMin, int32_t iDensityMax, int32_t &iIter)
{
	bool hitOnTime = true;
	bool fBreak = false;
	int32_t ctcox, ctcoy;
	int32_t cx = fixtoi(x);
	int32_t cy = fixtoi(y);
	int32_t i = iIter;
	do
	{
		if (!--i)
		{
			hitOnTime = false;
			break;
		}
		// If the object isn't moving and there is no gravity either, abort
		if (xdir == 0 && ydir == 0 && GravAccel == 0)
		{
			return false;
		}
		// If the object is above the landscape flying upwards in no/negative gravity, abort
		if (ydir <= 0 && GravAccel <= 0 && cy < 0)
		{
			return false;
		}
		// Set target position by momentum
		x += xdir;
		y += ydir;
		// Movement to target
		ctcox = fixtoi(x);
		ctcoy = fixtoi(y);
		// Bounds
		if (!Inside<int32_t>(ctcox, 0, ::Landscape.GetWidth()) || (ctcoy >= ::Landscape.GetHeight()))
		{
			return false;
		}
		// Move to target
		do
		{
			// Set next step target
			cx += Sign(ctcox - cx);
			cy += Sign(ctcoy - cy);
			// Contact check
			if (Inside(GBackDensity(cx, cy), iDensityMin, iDensityMax))
			{
				fBreak = true;
				break;
			}
		}
		while ((cx != ctcox) || (cy != ctcoy));
		// Adjust GravAccel once per frame
		ydir += GravAccel;
	}
	while (!fBreak);
	// write position back
	x = itofix(cx);
	y = itofix(cy);

	// how many steps did it take to get here?
	iIter -= i;

	return hitOnTime;
}

bool SimFlightHitsLiquid(C4Real fcx, C4Real fcy, C4Real xdir, C4Real ydir)
{
	// Start in water?
	if (DensityLiquid(GBackDensity(fixtoi(fcx), fixtoi(fcy))))
	{
		int temp = 10;
		if (!SimFlight(fcx, fcy, xdir, ydir, 0, C4M_Liquid - 1, temp))
		{
			return false;
		}
	}
	// Hits liquid?
	int temp = -1;
	if (!SimFlight(fcx, fcy, xdir, ydir, C4M_Liquid, 100, temp))
	{
		return false;
	}
	// liquid & deep enough?
	return GBackLiquid(fixtoi(fcx), fixtoi(fcy)) && GBackLiquid(fixtoi(fcx), fixtoi(fcy) + 9);
}

