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

/* Logic for C4Object: Physics and landscape interaction */

#include "C4Include.h"
#include "C4ForbidLibraryCompilation.h"
#include "object/C4Object.h"

#include "game/C4Physics.h"
#include "gui/C4GameMessage.h"
#include "landscape/C4MaterialList.h"
#include "landscape/C4PXS.h"
#include "landscape/C4SolidMask.h"
#include "lib/C4Random.h"
#include "lib/C4Real.h"
#include "object/C4Def.h"
#include "object/C4GameObjects.h"
#include "object/C4ObjectCom.h"
#include "platform/C4SoundSystem.h"


void C4Object::UpdatePos()
{
	// Get new area covered:
	// Do *NOT* do this while initializing, because object cannot be sorted by main list
	if (!Initializing && Status == C4OS_NORMAL)
	{
		::Objects.UpdatePos(this);
	}
}

void C4Object::UpdateMass()
{
	Mass = std::max<int32_t>((Def->Mass + OwnMass) * Con / FullCon, 1);
	if (!Def->NoMassFromContents)
	{
		Mass += Contents.Mass;
	}
	if (Contained)
	{
		Contained->Contents.MassCount();
		Contained->UpdateMass();
	}
}

void C4Object::UpdateInMat()
{
	// Get the current material
	int32_t newmat;
	if (Contained)
	{
		newmat = Contained->Def->ClosedContainer ? MNone : Contained->InMat;
	}
	else
	{
		newmat = GBackMat(GetX(), GetY());
	}

	// Has the material changed?
	if (newmat != InMat)
	{
		Call(PSF_OnMaterialChanged, &C4AulParSet(newmat, InMat));
		InMat = newmat;
	}
}

bool C4Object::At(int32_t ctx, int32_t cty) const
{
	return Status
		&& !Contained
		&& Def
		// TODO: Do a separate check in C4Shape
		&& Inside<int32_t>(cty - (GetY() + Shape.GetY() - addtop()), 0, Shape.Hgt - 1 + addtop())
		&& Inside<int32_t>(ctx - (GetX() + Shape.GetX()), 0, Shape.Wdt - 1);
}

bool C4Object::At(int32_t ctx, int32_t cty, DWORD &ocf) const
{
	if (Status
	&& !Contained
	&& Def
	&& (OCF & ocf)
	// TODO: Do a separate check in C4Shape
	&& Inside<int32_t>(cty - (GetY() + Shape.GetY() - addtop()), 0, Shape.Hgt - 1 + addtop())
	&& Inside<int32_t>(ctx - (GetX() + Shape.GetX()), 0, Shape.Wdt - 1))
	{
		// Set ocf return value
		GetOCFForPos(ctx, cty, ocf);
		return true;
	}
	return false;
}

void C4Object::Fling(C4Real fling_xdir, C4Real fling_ydir, bool add_speed)
{
	if (add_speed)
	{
		fling_xdir += xdir / 2;
		fling_ydir += ydir / 2;
	}
	if (!ObjectActionTumble(this, (fling_xdir < 0), fling_xdir, fling_ydir)
	&&  !ObjectActionJump(this, fling_xdir, fling_ydir, false))
	{
		xdir = fling_xdir;
		ydir = fling_ydir;
		Mobile = true;
		Action.t_attach &= ~CNAT_Bottom;
	}
}

bool C4Object::Push(C4Real txdir, C4Real dforce, bool fStraighten)
{
	// Valid check
	if (!Status || !Def || Contained || !(OCF & OCF_Grab))
	{
		return false;
	}
	// Grabbing okay, no pushing
	if (GetPropertyInt(P_Touchable) == 2)
	{
		return true;
	}
	// Mobilization check (pre-mobilization zero)
	if (!Mobile)
	{
		xdir = ydir = Fix0;
	}
	// General pushing force vs. object mass
	dforce = dforce * 100 / Mass;
	// Set dir
	if (xdir < 0) SetDir(DIR_Left);
	if (xdir > 0) SetDir(DIR_Right);
	// Work towards txdir
	if (Abs(xdir - txdir) <= dforce) // Close-enough-set
	{
		xdir = txdir;
	}
	else // Work towards
	{
		if (xdir < txdir) xdir += dforce;
		if (xdir > txdir) xdir -= dforce;
	}
	// Straighten
	if (fStraighten)
	{
		if (Inside<int32_t>(GetR(), -StableRange, +StableRange))
		{
			rdir = 0; // Cheap way out
		}
		else
		{
			if (fix_r > Fix0)
			{
				if (rdir > -RotateAccel)
				{
					rdir -= dforce;
				}
			}
			else
			{
				if (rdir < +RotateAccel)
				{
					rdir += dforce;
				}
			}
		}
	}

	// Mobilization check
	if (!!xdir || !!ydir || !!rdir)
	{
		Mobile = true;
	}

	// Stuck check
	if (!::Game.iTick35
	&&  txdir
	&& !Def->NoHorizontalMove
	&&  ContactCheck(GetX(), GetY())) // Resets t_contact
	{
		GameMsgObjectError(FormatString(LoadResStr("IDS_OBJ_STUCK"), GetName()).getData(), this);
		Call(PSF_Stuck);
	}

	return true;
}

bool C4Object::Lift(C4Real tydir, C4Real dforce)
{
	// Valid check
	if (!Status || !Def || Contained)
	{
		return false;
	}
	// Mobilization check
	if (!Mobile)
	{
		xdir = ydir = Fix0;
		Mobile = true;
	}
	// General pushing force vs. object mass
	dforce = dforce * 100 / Mass;
	// If close enough, set tydir
	if (Abs(tydir - ydir) <= Abs(dforce))
	{
		ydir = tydir;
	}
	else // Work towards tydir
	{
		if (ydir<tydir) ydir+=dforce;
		if (ydir>tydir) ydir-=dforce;
	}
	// Stuck check
	if (tydir != -GravAccel
	&& ContactCheck(GetX(), GetY())) // Resets t_contact
	{
		GameMsgObjectError(FormatString(LoadResStr("IDS_OBJ_STUCK"), GetName()).getData(), this);
		Call(PSF_Stuck);
	}
	return true;
}

BYTE C4Object::GetMomentum(C4Real &rxdir, C4Real &rydir) const
{
	rxdir = rydir = 0;
	if (!Status || !Def)
	{
		return 0;
	}
	rxdir = xdir;
	rydir = ydir;
	return 1;
}

C4Real C4Object::GetSpeed() const
{
	// TODO: This is actually not the real speed of the object,
	// it is actually a "Manhattan" speed :)
	// Note: Relevant for OCF calculation only...
	C4Real speed = Fix0;
	if (xdir < 0) speed -= xdir; else speed += xdir;
	if (ydir < 0) speed -= ydir; else speed += ydir;
	return speed;
}

void C4Object::SetSolidMask(int32_t x, int32_t y, int32_t wdt, int32_t hgt, int32_t tx, int32_t ty)
{
	// Remove old
	if (pSolidMaskData)
	{
		delete pSolidMaskData;
		pSolidMaskData = nullptr;
	}
	// Set new data
	SolidMask.Set(x, y, wdt, hgt, tx, ty);
	// Re-put if valid
	if (CheckSolidMaskRect())
	{
		UpdateSolidMask(false);
	}
}

void C4Object::SetHalfVehicleSolidMask(bool set)
{
	if (pSolidMaskData)
	{
		HalfVehicleSolidMask = set;
		pSolidMaskData->SetHalfVehicle(set);
	}
}

bool C4Object::CheckSolidMaskRect()
{
	// Ensure SolidMask rect lies within bounds of SolidMask bitmap in definition
	CSurface8 *mask = Def->pSolidMask;
	if (!mask)
	{
		// No graphics to set solid in
		SolidMask.Set(0, 0, 0, 0, 0, 0);
		return false;
	}
	SolidMask.Set(std::max<int32_t>(SolidMask.x,0), std::max<int32_t>(SolidMask.y, 0),
	              std::min<int32_t>(SolidMask.Wdt, mask->Wdt - SolidMask.x), std::min<int32_t>(SolidMask.Hgt, mask->Hgt-SolidMask.y),
	              SolidMask.tx, SolidMask.ty);
	if (SolidMask.Hgt <= 0)
	{
		SolidMask.Wdt = 0;
	}
	return SolidMask.Wdt > 0;
}

bool C4Object::IsInLiquidCheck() const
{
	return GBackLiquid(GetX(), GetY() + Def->Float * Con / FullCon - 1);
}

void C4Object::SetRotation(int32_t new_rotation)
{
	while (new_rotation < 0)
	{
		new_rotation += 360;
	}
	new_rotation %= 360;
	// Remove solid mask
	RemoveSolidMask(false);
	// Set rotation
	fix_r = itofix(new_rotation);
	// Update face
	UpdateFace(true);
}

void C4Object::DrawSolidMask(C4TargetFacet &cgo) const
{
	if (pSolidMaskData)
	{
		pSolidMaskData->Draw(cgo);
	}
}

void C4Object::RemoveSolidMask(bool fBackupAttachment)
{
	if (pSolidMaskData)
	{
		pSolidMaskData->Remove(fBackupAttachment);
	}
}

void C4Object::UpdateSolidMask(bool fRestoreAttachedObjects)
{
	// Solidmask doesn't make sense with non-existant objects
	// (the solidmask has already been destroyed in AssignRemoval -
	//  do not reset it!)
	if (!Status)
	{
		return;
	}
	// Determine necessity, update cSolidMask, put or remove mask
	// Mask if enabled, fullcon, not contained
	if (SolidMask.Wdt > 0 && Con >= FullCon && !Contained)
	{
		// Recheck and put mask
		if (!pSolidMaskData)
		{
			pSolidMaskData = new C4SolidMask(this);
		}
		else
		{
			pSolidMaskData->Remove(false);
		}
		pSolidMaskData->Put(true, nullptr, fRestoreAttachedObjects);
		SetHalfVehicleSolidMask(HalfVehicleSolidMask);
	}
	// Otherwise, remove and destroy mask
	else if (pSolidMaskData)
	{
		delete pSolidMaskData;
		pSolidMaskData = nullptr;
	}
}

bool C4Object::AdjustWalkRotation(int32_t range_x, int32_t range_y, int32_t speed)
{
	int32_t dest_angle;
	// Attachment at middle (bottom) vertex?
	if (Shape.iAttachVtx < 0 || !Def->Shape.VtxX[Shape.iAttachVtx])
	{
		// Evaluate floor around attachment pos
		int32_t y_solid_left = 0;
		int32_t y_solid_right = 0;
		// Left
		int32_t check_x = Shape.iAttachX - range_x;
		if (GBackSolid(check_x, Shape.iAttachY))
		{
			// Up
			while (--y_solid_left > -range_y)
			{
				if (GBackSolid(check_x, Shape.iAttachY + y_solid_left))
				{
					++y_solid_left;
					break;
				}
			}
		}
		else
		{
			// Down
			while (++y_solid_left < range_y)
			{
				if (GBackSolid(check_x, Shape.iAttachY + y_solid_left))
				{
					--y_solid_left;
					break;
				}
			}
		}
		// Right
		check_x += 2 * range_x;
		if (GBackSolid(check_x, Shape.iAttachY))
		{
			// Up
			while (--y_solid_right > -range_y)
			{
				if (GBackSolid(check_x, Shape.iAttachY + y_solid_right))
				{
					++y_solid_right;
					break;
				}
			}
		}
		else
		{
			// Down
			while (++y_solid_right < range_y)
			{
				if (GBackSolid(check_x, Shape.iAttachY + y_solid_right))
				{
					--y_solid_right;
					break;
				}
			}
		}
		// Calculate destination angle
		// 100% accurate for large values of Pi ;)
		dest_angle = (y_solid_right - y_solid_left) * (35 / std::max<int32_t>(range_x, 1));
	}
	else
	{
		// Attachment at other than horizontal middle vertex: get your feet to the ground!
		// Rotate target to large angle is OK, because rotation will stop once the real
		// bottom vertex hits solid ground
		if (Shape.VtxX[Shape.iAttachVtx] > 0)
		{
			dest_angle = -50;
		}
		else
		{
			dest_angle = 50;
		}
	}
	// Move to destination angle
	if (Abs(dest_angle - GetR()) > 2)
	{
		rdir = itofix(Clamp<int32_t>(dest_angle-GetR(), -15, +15));
		rdir /= (10000 / speed);
	}
	else
	{
		rdir = 0;
	}
	// Done, success
	return true;
}

static void BubbleOut(int32_t tx, int32_t ty)
{
	// No bubbles from nowhere
	if (!GBackSemiSolid(tx, ty))
	{
		return;
	}
	// Enough bubbles out there already
	if (::Objects.ObjectCount(C4ID::Bubble) >= 150)
	{
		return;
	}
	// Create bubble
	Game.CreateObject(C4ID::Bubble, nullptr, NO_OWNER, tx, ty);
}

void C4Object::Splash()
{
	int32_t tx = GetX();
	int32_t ty = GetY() + 1;
	int32_t amt = std::min(Shape.Wdt * Shape.Hgt / 10, 20);
	// Splash only if there is free space above
	if (GBackSemiSolid(tx, ty - 15))
	{
		return;
	}
	// Get back material
	int32_t iMat = GBackMat(tx, ty);
	// Check liquid
	if (MatValid(iMat)
	&&  DensityLiquid(::MaterialMap.Map[iMat].Density)
	&& ::MaterialMap.Map[iMat].Instable)
	{
		int32_t sy = ty;
		while (GBackLiquid(tx, sy) && sy > ty - 20 && sy >= 0)
		{
			sy--;
		}
		// Splash bubbles and liquid
		for (int32_t cnt = 0; cnt < amt; cnt++)
		{
			int32_t bubble_x = tx + Random(16) - 8;
			int32_t bubble_y = ty + Random(16) - 6;
			BubbleOut(bubble_x, bubble_y);
			if (GBackLiquid(tx, ty) && !GBackSemiSolid(tx, sy))
			{
				C4Real xdir = C4REAL100(Random(151) - 75);
				C4Real ydir = C4REAL100(-int32_t(Random(200)));
				::PXS.Create(::Landscape.ExtractMaterial(tx, ty, false),
							 itofix(tx), itofix(sy),
							 xdir,
							 ydir);
			}
		}
	}
	// Splash sound
	if (amt >= 20)
	{
		StartSoundEffect("Liquids::Splash2", false, 50, this);
	}
	else if (amt > 1)
	{
		StartSoundEffect("Liquids::Splash1", false, 50, this);
	}
}

void C4Object::UpdateInLiquid()
{
	// InLiquid check
	if (IsInLiquidCheck()) // In Liquid
	{
		if (!InLiquid) // Enter liquid
		{
			if ((OCF & OCF_HitSpeed2) && Mass > 3)
			{
				Splash();
			}
			InLiquid = true;
		}
	}
	else if (InLiquid) // Leave liquid
	{
		InLiquid = false;
	}
}

bool C4Object::IsMoveableBySolidMask(int ComparisonPlane) const
{
	return (Status == C4OS_NORMAL)
		&& !(Category & C4D_StaticBack)
		&& (ComparisonPlane < GetPlane())
		&& !Contained;
}

int32_t C4Object::GetSolidMaskPlane() const
{
	// Use SolidMaskPlane property. Fallback to object plane if unassigned.
	int32_t plane = GetPropertyInt(P_SolidMaskPlane);
	return plane ? plane : GetPlane();
}
