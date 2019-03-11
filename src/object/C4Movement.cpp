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
	C4Real force_redirected = std::min(Abs(from), FRedirect);
	from -= force_redirected * Sign(from);
	to += force_redirected * tdir;
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

bool ContactVtxCNAT(C4Object *object, BYTE cnat_dir)
{
	for (int32_t index = 0; index < object->Shape.VtxNum; index++)
	{
		if (object->Shape.VtxContactCNAT[index] & cnat_dir)
		{
			return true;
		}
	}
	return false;
}

// Finds first vertex with contact flag set.
// Returns -1/0/+1 for relation on vertex to object center.

int32_t ContactVtxWeight(C4Object *object)
{
	for (int32_t index = 0; index < object->Shape.VtxNum; index++)
	{
		if (object->Shape.VtxContactCNAT[index])
		{
			if (object->Shape.VtxX[index] < 0)
			{
				return -1;
			}
			if (object->Shape.VtxX[index] > 0)
			{
				return +1;
			}
		}
	}
	return 0;
}

// ContactVtxFriction: Returns 0-100 friction value of first
//                     contacted vertex;

int32_t ContactVtxFriction(C4Object *object)
{
	for (int32_t index = 0; index < object->Shape.VtxNum; index++)
	{
		if (object->Shape.VtxContactCNAT[index])
		{
			return object->Shape.VtxFriction[index];
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

void C4Object::DoMotion(int32_t distance_x, int32_t distance_y)
{
	RemoveSolidMask(true);
	fix_x += distance_x;
	fix_y += distance_y;
}

void C4Object::StopAndContact(C4Real &contact_coordinate, C4Real limit, C4Real &speed, int32_t cnat)
{
	contact_coordinate = limit;
	speed = 0;
	Contact(cnat);
}

int32_t C4Object::ContactCheck(int32_t at_x, int32_t at_y, uint32_t *border_hack_contacts, bool collide_halfvehic)
{
	// Check shape contact at given position
	Shape.ContactCheck(at_x, at_y, border_hack_contacts, collide_halfvehic);

	// Store shape contact values in object t_contact
	t_contact = Shape.ContactCNAT;

	// Contact script call for the first contacted cnat
	if (Shape.ContactCNAT)
	{
		for (int32_t ccnat = 0; ccnat < 4; ccnat++) // Left, right, top bottom
		{
			// Will stop on first positive return contact call!
			if ((Shape.ContactCNAT & (1 << ccnat)) && Contact(1 << ccnat))
			{
				break;
			}
		}
	}

	// Return shape contact count
	return Shape.ContactCount;
}

// Stop the object and do contact calls if it collides with the border
void C4Object::SideBounds(C4Real &target_x)
{
	// Layer bounds
	if (Layer && (Layer->GetPropertyInt(P_BorderBound) & C4D_Border_Layer))
	{
		C4PropList* pActionDef = GetAction();
		if (!pActionDef || pActionDef->GetPropertyP(P_Procedure) != DFA_ATTACH)
		{
			C4Real lbound = itofix(Layer->GetX() + Layer->Shape.GetX() - Shape.GetX());
			C4Real rbound = itofix(Layer->GetX() + Layer->Shape.GetX() + Layer->Shape.Wdt - (Shape.GetX() + Shape.Wdt));
			if (target_x < lbound)
			{
				StopAndContact(target_x, lbound, xdir, CNAT_Left);
			}
			if (target_x > rbound)
			{
				StopAndContact(target_x, rbound, xdir, CNAT_Right);
			}
		}
	}
	// Landscape bounds
	C4Real lbound = itofix(0 - Shape.GetX());
	C4Real rbound = itofix(::Landscape.GetWidth() - (Shape.GetX() + Shape.Wdt));
	if (target_x < lbound && (GetPropertyInt(P_BorderBound) & C4D_Border_Sides))
	{
		StopAndContact(target_x, lbound, xdir, CNAT_Left);
	}
	if (target_x > rbound && (GetPropertyInt(P_BorderBound) & C4D_Border_Sides))
	{
		StopAndContact(target_x, rbound, xdir, CNAT_Right);
	}
}

void C4Object::VerticalBounds(C4Real &target_y)
{
	// Layer bounds
	if (Layer && (Layer->GetPropertyInt(P_BorderBound) & C4D_Border_Layer))
	{
		C4PropList* pActionDef = GetAction();
		if (!pActionDef || pActionDef->GetPropertyP(P_Procedure) != DFA_ATTACH)
		{
			C4Real tbound = itofix(Layer->GetY() + Layer->Shape.GetY() - Shape.GetY());
			C4Real bbound = itofix(Layer->GetY() + Layer->Shape.GetY() + Layer->Shape.Hgt - (Shape.GetY() + Shape.Hgt));
			if (target_y < tbound)
			{
				StopAndContact(target_y, tbound, ydir, CNAT_Top);
			}
			if (target_y > bbound)
			{
				StopAndContact(target_y, bbound, ydir, CNAT_Bottom);
			}
		}
	}
	// Landscape bounds
	C4Real tbound = itofix(0 - Shape.GetY());
	C4Real bbound = itofix(::Landscape.GetHeight() - (Shape.GetY() + Shape.Hgt));
	if (target_y < tbound && (GetPropertyInt(P_BorderBound) & C4D_Border_Top))
	{
		StopAndContact(target_y, tbound, ydir, CNAT_Top);
	}
	if (target_y > bbound && (GetPropertyInt(P_BorderBound) & C4D_Border_Bottom))
	{
		StopAndContact(target_y, bbound, ydir, CNAT_Bottom);
	}
}

void C4Object::DoMovement()
{
	int contact_bits = 0;
	bool has_moved = false;
	bool has_contact = false;
	bool has_turned = false;
	bool lost_attachment = false;
	bool redirected_force_from_ydir_to_rdir = false;
	// Restrictions
	if (Def->NoHorizontalMove)
	{
		xdir = 0;
	}

	// Dig free target area
	MovementDigFreeTargetArea();

	// Store previous movement and ocf
	C4Real old_xdir(xdir);
	C4Real old_ydir(ydir);
	uint32_t old_ocf = OCF;

	// Store new target x and y
	C4Real new_x = fix_x + xdir;
	C4Real new_y = fix_y + ydir;

	// Apply bounds
	SideBounds(new_x);

	if (!Action.t_attach) // Unattached movement  = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
	{
		// Horizontal movement - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		// Move to target
		while (fixtoi(new_x) != fixtoi(fix_x))
		{
			// Next step
			int step_x = Sign(new_x - fix_x);
			uint32_t border_hack_contacts = 0;
			int32_t current_contacts = ContactCheck(GetX() + step_x, GetY(), &border_hack_contacts);
			if (current_contacts || border_hack_contacts)
			{
				has_contact = true;
				contact_bits |= t_contact | border_hack_contacts;
			}
			if (current_contacts)
			{
				// Abort horizontal movement
				new_x = fix_x;
				// Vertical redirection (always)
				RedirectForce(xdir, ydir, -1);
				ApplyFriction(ydir, ContactVtxFriction(this));
			}
			else // Free horizontal movement
			{
				DoMotion(step_x, 0);
				has_moved = true;
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
			int step_y = Sign(new_y - fix_y);
			int32_t current_contacts = ContactCheck(GetX(), GetY() + step_y, nullptr, ydir > 0);
			if (current_contacts)
			{
				has_contact = true;
				contact_bits |= t_contact;
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
					// Living things are always capable of keeping their rotation
					if ((OCF & OCF_Rotate) && current_contacts == 1 && !Alive)
					{
						RedirectForce(ydir, rdir, -ContactVtxWeight(this));
						redirected_force_from_ydir_to_rdir = true;
					}
					ydir = 0;
				}
			}
			else // Free vertical movement
			{
				DoMotion(0, step_y);
				has_moved = true;
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
			int32_t step_target_x = GetX() + step_x;
			int32_t step_target_y = GetY() + step_y;
			// Attachment check
			if (!Shape.Attach(step_target_x, step_target_y, Action.t_attach))
			{
				lost_attachment = true;
			}
			else
			{
				// Attachment change to step_target_x/step_target_y overrides target
				if (step_target_x != GetX() + step_x)
				{
					xdir = Fix0;
					new_x = itofix(step_target_x);
				}
				if (step_target_y != GetY() + step_y)
				{
					ydir = Fix0;
					new_y = itofix(step_target_y);
				}
			}
			// Contact check & evaluation
			uint32_t border_hack_contacts = 0;
			int32_t current_contacts = ContactCheck(step_target_x, step_target_y, &border_hack_contacts);
			if (current_contacts || border_hack_contacts)
			{
				has_contact = true;
				contact_bits |= border_hack_contacts | t_contact;
			}
			if (current_contacts)
			{
				// Abort movement
				if (step_target_x != GetX())
				{
					step_target_x = GetX();
					new_x = fix_x;
				}
				if (step_target_y != GetY())
				{
					step_target_y = GetY();
					new_y = fix_y;
				}
			}
			DoMotion(step_target_x - GetX(), step_target_y - GetY());
			has_moved = true;
		}
		while (fixtoi(new_x) != GetX() || fixtoi(new_y) != GetY());
	}

	if (fix_x != new_x || fix_y != new_y)
	{
		has_moved = true;
		RemoveSolidMask(true);
		fix_x = new_x;
		fix_y = new_y;
	}
	// Rotation  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	if ((OCF & OCF_Rotate) && !!rdir)
	{
		C4Real target_r = fix_r + rdir * 5;
		// Rotation limit
		if (Def->Rotateable > 1)
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
		int32_t current_x = GetX();
		int32_t current_y = GetY();
		// Move to target
		while (fixtoi(fix_r) != fixtoi(target_r))
		{
			// Save step undos
			C4Real old_rotation = fix_r;
			C4Shape old_shape = Shape;
			// Try next step
			fix_r += Sign(target_r - fix_r);
			UpdateShape();
			// Attached rotation: rotate around attachment pos
			if (Action.t_attach && !lost_attachment)
			{
				// More accurately, attachment should be evaluated by a rotation around the attachment vertex
				// however, as long as this code is only used for some surfaces adjustment for large vehicles,
				// it's enough to assume rotation around the center
				current_x = GetX();
				current_y = GetY();
				// Evaluate attachment, but do not bother about attachment loss
				// that will then be done in next execution cycle
				Shape.Attach(current_x, current_y, Action.t_attach);
			}
			// Check for contact
			int32_t current_contacts = ContactCheck(current_x, current_y);
			if (current_contacts) // Contact
			{
				has_contact = true;
				contact_bits |= t_contact;
				// Undo step and abort movement
				Shape = old_shape;
				target_r = fix_r = old_rotation;
				// Last UpdateShape-call might have changed sector lists!
				UpdatePos();
				// Redirect to GetY()
				if (current_contacts == 1 && !redirected_force_from_ydir_to_rdir)
				{
					RedirectForce(rdir, ydir, -1);
				}
				// Stop rotation
				rdir = 0;
			}
			else
			{
				has_turned = true;
				if (current_x != GetX() || current_y != GetY())
				{
					fix_x = itofix(current_x);
					fix_y = itofix(current_y);
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
	if (has_moved || has_turned)
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
			if ((OCF & OCF_HitSpeed2) && (Mass > 3))
			{
				Splash();
			}
			lost_attachment = false;
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
	if (has_contact)
	{
		t_contact = contact_bits;
		ContactAction();
	}
	// Attachment Loss Action
	if (lost_attachment)
	{
		NoAttachAction();
	}
	// Movement Script Execution
	if (has_contact)
	{
		C4AulParSet pars(fixtoi(old_xdir, 100), fixtoi(old_ydir, 100));
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
	// Update graphics to rotation
	if (has_turned)
	{
		UpdateFace(true);
	}
	else
	{
		// Position has changed?
		if (has_moved)
		{
			UpdatePos();
		}
	}
}

void C4Object::Stabilize()
{
	// Definition allows stabilization?
	if (Def->NoStabilize)
	{
		return;
	}
	// Normalize angle
	C4Real normalized_rotation = fix_r;
	while (normalized_rotation < itofix(-180))
	{
		normalized_rotation += 360;
	}
	while (normalized_rotation > itofix(180))
	{
		normalized_rotation -= 360;
	}
	if ((normalized_rotation != Fix0) && Inside<C4Real>(normalized_rotation, itofix(-StableRange), itofix(+StableRange)))
	{
		// Save step undos
		C4Real old_rotation = fix_r;
		C4Shape old_shape = Shape;
		// Try rotation
		fix_r = Fix0;
		UpdateShape();
		if (ContactCheck(GetX(), GetY()))
		{ // Undo rotation
			Shape = old_shape;
			fix_r = old_rotation;
		}
		else
		{ // Stabilization okay
			UpdateFace(true);
		}
	}
}

void C4Object::CopyMotion(C4Object *from_object)
{
	// Designed for contained objects, no static
	if (fix_x != from_object->fix_x || fix_y != from_object->fix_y)
	{
		fix_x = from_object->fix_x;
		fix_y = from_object->fix_y;
		// Resort into sectors
		UpdatePos();
	}
	xdir = from_object->xdir;
	ydir = from_object->ydir;
}

void C4Object::ForcePosition(C4Real target_x, C4Real target_y)
{
	fix_x = target_x;
	fix_y = target_y;
	UpdatePos();
	UpdateSolidMask(false);
}

void C4Object::MovePosition(int32_t distance_x, int32_t distance_y)
{
	MovePosition(itofix(distance_x), itofix(distance_y));
}

void C4Object::MovePosition(C4Real distance_x, C4Real distance_y)
{
	// Move object position; repositions SolidMask
	RemoveSolidMask(true);
	fix_x += distance_x;
	fix_y += distance_y;
	UpdatePos();
	UpdateSolidMask(true);
}


bool C4Object::ExecMovement() // Every Tick1 by Execute
{
	// Update in which material this object is
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
		C4PropList* current_action = GetAction();
		// Never remove attached objects: If they are truly outside landscape, their target will be removed,
		// and the attached objects follow one frame later
		if (!current_action || !Action.Target || current_action->GetPropertyP(P_Procedure) != DFA_ATTACH)
		{
			bool should_remove = true;
			// Never remove HUD objects
			if (Category & C4D_Parallax)
			{
				int parallaxity_x, parallaxity_y;
				GetParallaxity(&parallaxity_x, &parallaxity_y);

				should_remove = false;
				if (GetX() > ::Landscape.GetWidth() || GetY() > ::Landscape.GetHeight())
				{
					should_remove = true; // except if they are really out of the viewport to the right...
				}
				else if (GetX() < 0 && !!parallaxity_x)
				{
					should_remove = true; // ...or it's not HUD horizontally and it's out to the left
				}
				else if (!parallaxity_x && GetX() < -::Landscape.GetWidth())
				{
					should_remove = true; // ...or it's HUD horizontally and it's out to the left
				}
			}
			if (should_remove)
			{
				AssignDeath(true);
				AssignRemoval();
			}
		}
	}
	return true;
}

void C4Object::MovementDigFreeTargetArea()
{
	C4PropList* current_action = GetAction();
	if (current_action && (current_action->GetPropertyInt(P_DigFree)))
	{
		int target_x = fixtoi(fix_x + xdir);
		int target_y = fixtoi(fix_y + ydir);
		// Shape size square
		if (current_action->GetPropertyInt(P_DigFree) == 1)
		{
			::Landscape.DigFreeRect(target_x + Shape.GetX(), target_y + Shape.GetY(), Shape.Wdt, Shape.Hgt, this);
		}
		// Free size round (variable size)
		else
		{
			int32_t rad = current_action->GetPropertyInt(P_DigFree);
			if (Con < FullCon)
			{
				rad = rad * 6 * Con / 5 / FullCon;
			}
			::Landscape.DigFree(target_x, target_y - 1, rad, this);
		}
	}
}

bool SimFlight(C4Real &x, C4Real &y, C4Real &xdir, C4Real &ydir, int32_t min_density, int32_t max_density, int32_t &iterations)
{
	bool hit_on_time = true;
	bool break_main_loop = false;
	int32_t target_x, target_y;
	int32_t current_x = fixtoi(x);
	int32_t current_y = fixtoi(y);
	int32_t index = iterations;
	do
	{
		if (!--index)
		{
			hit_on_time = false;
			break;
		}
		// If the object isn't moving and there is no gravity either, abort
		if (xdir == 0 && ydir == 0 && GravAccel == 0)
		{
			return false;
		}
		// If the object is above the landscape flying upwards in no/negative gravity, abort
		if (ydir <= 0 && GravAccel <= 0 && current_y < 0)
		{
			return false;
		}
		// Set target position by momentum
		x += xdir;
		y += ydir;
		// Set target position, then iterate towards this position, checking for contact
		target_x = fixtoi(x);
		target_y = fixtoi(y);
		// Is the target position outside of the landscape?
		if (!Inside<int32_t>(target_x, 0, ::Landscape.GetWidth()) || (target_y >= ::Landscape.GetHeight()))
		{
			return false;
		}
		// Check the way towards the target position, by doing minimal movement steps
		do
		{
			// Set next step target
			current_x += Sign(target_x - current_x);
			current_y += Sign(target_y - current_y);
			// Contact check
			if (Inside(GBackDensity(current_x, current_y), min_density, max_density))
			{
				break_main_loop = true;
				break;
			}
		}
		while ((current_x != target_x) || (current_y != target_y));
		// Adjust GravAccel once per frame
		ydir += GravAccel;
	}
	while (!break_main_loop);
	// Write position back
	x = itofix(current_x);
	y = itofix(current_y);

	// How many steps did it take to get here?
	iterations -= index;

	return hit_on_time;
}

bool SimFlightHitsLiquid(C4Real start_x, C4Real start_y, C4Real xdir, C4Real ydir)
{
	// Start in water?
	if (DensityLiquid(GBackDensity(fixtoi(start_x), fixtoi(start_y))))
	{
		int temp = 10;
		if (!SimFlight(start_x, start_y, xdir, ydir, 0, C4M_Liquid - 1, temp))
		{
			return false;
		}
	}
	// Hits liquid?
	int temp = -1;
	if (!SimFlight(start_x, start_y, xdir, ydir, C4M_Liquid, C4M_Vehicle, temp))
	{
		return false;
	}
	// Liquid & deep enough?
	return GBackLiquid(fixtoi(start_x), fixtoi(start_y)) && GBackLiquid(fixtoi(start_x), fixtoi(start_y) + 9);
}

