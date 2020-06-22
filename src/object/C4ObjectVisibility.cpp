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

/* Logic for C4Object: Light, audibility, visibility */

#include "C4Include.h"
#include "C4ForbidLibraryCompilation.h"
#include "object/C4Object.h"

#include "game/C4Application.h"
#include "landscape/fow/C4FoW.h"
#include "player/C4PlayerList.h"


bool C4Object::AssignLightRange()
{
	if (!lightRange && !lightFadeoutRange) return true;

	UpdateLight();
	return true;
}

bool C4Object::SetLightRange(int32_t iToRange, int32_t iToFadeoutRange)
{
	// set new range
	lightRange = iToRange;
	lightFadeoutRange = iToFadeoutRange;
	// resort into player's FoW-repeller-list
	UpdateLight();
	// success
	return true;
}

bool C4Object::SetLightColor(uint32_t iValue)
{
	// set new color value
	lightColor = iValue;
	// resort into player's FoW-repeller-list
	UpdateLight();
	// success
	return true;
}

void C4Object::UpdateLight()
{
	if (Landscape.HasFoW()) Landscape.GetFoW()->Add(this);
}

void C4Object::SetAudibilityAt(C4TargetFacet &cgo, int32_t iX, int32_t iY, int32_t player)
{
	// target pos (parallax)
	float offX, offY, newzoom;
	GetDrawPosition(cgo, iX, iY, cgo.Zoom, offX, offY, newzoom);
	int32_t audible_at_pos = Clamp(100 - 100 * Distance(cgo.X + cgo.Wdt / 2, cgo.Y + cgo.Hgt / 2, offX, offY) / 700, 0, 100);
	if (audible_at_pos > Audible)
	{
		Audible = audible_at_pos;
		AudiblePan = Clamp<int>(200 * (offX - cgo.X - (cgo.Wdt / 2)) / cgo.Wdt, -100, 100);
		AudiblePlayer = player;
	}
}

bool C4Object::IsVisible(int32_t iForPlr, bool fAsOverlay) const
{
	bool fDraw;
	C4Value vis;
	if (!GetProperty(P_Visibility, &vis))
		return true;

	int32_t Visibility;
	C4ValueArray *parameters = vis.getArray();
	if (parameters && parameters->GetSize())
	{
		Visibility = parameters->GetItem(0).getInt();
	}
	else
	{
		Visibility = vis.getInt();
	}
	// check layer
	if (Layer && Layer != this && !fAsOverlay)
	{
		fDraw = Layer->IsVisible(iForPlr, false);
		if (Layer->GetPropertyInt(P_Visibility) & VIS_LayerToggle) fDraw = !fDraw;
		if (!fDraw) return false;
	}
	// no flags set?
	if (!Visibility) return true;
	// check overlay
	if (Visibility & VIS_OverlayOnly)
	{
		if (!fAsOverlay) return false;
		if (Visibility == VIS_OverlayOnly) return true;
	}
	// editor visibility
	if (::Application.isEditor)
	{
		if (Visibility & VIS_Editor) return true;
	}
	// check visibility
	fDraw=false;
	if (Visibility & VIS_Owner) fDraw = fDraw || (iForPlr==Owner);
	if (iForPlr!=NO_OWNER)
	{
		// check all
		if (Visibility & VIS_Allies)  fDraw = fDraw || (iForPlr!=Owner && !Hostile(iForPlr, Owner));
		if (Visibility & VIS_Enemies) fDraw = fDraw || (iForPlr!=Owner && Hostile(iForPlr, Owner));
		if (parameters)
		{
			if (Visibility & VIS_Select)  fDraw = fDraw || parameters->GetItem(1+iForPlr).getBool();
		}
	}
	else fDraw = fDraw || (Visibility & VIS_God);
	return fDraw;
}
