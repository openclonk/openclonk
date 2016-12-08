/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2014-2016, The OpenClonk Team and contributors
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

#include "C4Include.h"
#include "C4ForbidLibraryCompilation.h"

#ifndef USE_CONSOLE

#include "landscape/fow/C4FoWLight.h"
#include "landscape/fow/C4FoWLightSection.h"
#include "landscape/fow/C4FoWBeamTriangle.h"
#include "landscape/fow/C4FoWDrawStrategy.h"
#include "player/C4PlayerList.h"
#include "player/C4Player.h"
#include "lib/StdColors.h"

#include <vector>

C4FoWLight::C4FoWLight(C4Object *pObj)
	: iX(fixtoi(pObj->fix_x)),
	  iY(fixtoi(pObj->fix_y)),
	  iReach(pObj->lightRange),
	  iFadeout(pObj->lightFadeoutRange),
	  iSize(20), gBright(0.5), colorR(1.0), colorG(1.0), colorB(1.0),
	  colorV(1.0), colorL(1.0),
	  pNext(nullptr),
	  pObj(pObj),
	  sections(4)
{
	sections[0] = new C4FoWLightSection(this,0);
	sections[1] = new C4FoWLightSection(this,90);
	sections[2] = new C4FoWLightSection(this,180);
	sections[3] = new C4FoWLightSection(this,270);
}

C4FoWLight::~C4FoWLight()
{
	for(size_t i = 0; i < sections.size(); ++i )
		delete sections[i];
}

void C4FoWLight::Invalidate(C4Rect r)
{
	for(size_t i = 0; i < sections.size(); ++i )
		sections[i]->Invalidate(r);
}

void C4FoWLight::SetReach(int32_t iReach2, int32_t iFadeout2)
{
	// Fadeout changes don't matter
	iFadeout = iFadeout2;

	if (iReach == iReach2) return;

	if (iReach2 < iReach)
	{
		// Reach decreased? Prune beams
		iReach = iReach2;
		for(size_t i = 0; i < sections.size(); ++i )
			sections[i]->Prune(iReach);

	} else {

		// Reach increased? Dirty beams that might get longer now
		iReach = iReach2;
		for(size_t i = 0; i < sections.size(); ++i )
			sections[i]->Dirty(iReach);
	}
}

void C4FoWLight::SetColor(uint32_t iValue)
{
	colorR = GetRedValue(iValue) / 255.0f;
	colorG = GetGreenValue(iValue) / 255.0f;
	colorB = GetBlueValue(iValue) / 255.0f;

	float min = std::min(colorR, std::min(colorG, colorB));
	colorV = std::max(std::max(colorR, std::max(colorG, colorB)), 1e-3f); // prevent division by 0
	colorL = (min + colorV) / 2.0f;

	// maximize color, so that dark colors will not be desaturated after normalization
	colorR = std::min(colorR / colorV, 1.0f);
	colorG = std::min(colorG / colorV, 1.0f);
	colorB = std::min(colorB / colorV, 1.0f);
}

void C4FoWLight::Update(C4Rect Rec)
{
	// Update position from object.
	int32_t iNX = fixtoi(pObj->fix_x), iNY = fixtoi(pObj->fix_y);
	// position may be affected by LightOffset property
	C4ValueArray *light_offset = pObj->GetPropertyArray(P_LightOffset);
	if (light_offset)
	{
		iNX += light_offset->GetItem(0).getInt();
		iNY += light_offset->GetItem(1).getInt();
	}
	// Clear if we moved in any way
	if (iNX != iX || iNY != iY)
	{
		for(size_t i = 0; i < sections.size(); ++i )
			sections[i]->Prune(0);
		iX = iNX; iY = iNY;
	}

	for(size_t i = 0; i < sections.size(); ++i )
		sections[i]->Update(Rec);
}

void C4FoWLight::Render(C4FoWRegion *region, const C4TargetFacet *onScreen, C4ShaderCall& call)
{
	TriangleList triangles;

	bool clip = false;
	
	for(size_t i = 0; i < sections.size(); ++i )
	{
		TriangleList sectionTriangles = sections[i]->CalculateTriangles(region);

		// if the triangles of one section are clipped completely, the neighbouring triangles
		// must be marked as clipped
		if(!triangles.empty()) triangles.rbegin()->clipRight |= clip;
		if(!sectionTriangles.empty()) sectionTriangles.begin()->clipLeft |= clip;

		clip = sectionTriangles.empty();
		triangles.splice(triangles.end(), sectionTriangles);
	}

	CalculateFanMaxed(triangles);
	CalculateIntermediateFadeTriangles(triangles);

	std::unique_ptr<C4FoWDrawStrategy>& strategy = onScreen ? OnScreenStrategy : OffScreenStrategy;
	if (!strategy.get())
	{
		if (onScreen)
			strategy.reset(new C4FoWDrawWireframeStrategy(this, onScreen));
		else
			strategy.reset(new C4FoWDrawLightTextureStrategy(this));
	}

	strategy->Begin(region);

	DrawFan(strategy.get(), triangles);
	DrawFanMaxed(strategy.get(), triangles);
	DrawFade(strategy.get(), triangles);
	DrawIntermediateFadeTriangles(strategy.get(), triangles);

	strategy->End(call);
}

void C4FoWLight::CalculateFanMaxed(TriangleList &triangles) const
{
	for (TriangleList::iterator it = triangles.begin(); it != triangles.end(); ++it)
	{
		C4FoWBeamTriangle &tri = *it;

		// Is the left point close enough that normals don't max out?
		float dist = sqrt(GetSquaredDistanceTo(tri.fanLX, tri.fanLY));
		if (dist <= getNormalSize()) {
			tri.nfanLX = tri.fanLX;
			tri.nfanLY = tri.fanLY;
		} else {
			// Otherwise, calculate point where they do. We will add a seperate
			// triangle/quad later on to capture that.
			float f = float(getNormalSize() / dist);
			tri.nfanLX = f * tri.fanLX + (1.0f - f) * getX();
			tri.nfanLY = f * tri.fanLY + (1.0f - f) * getY();
		}

		// Same for the right point
		dist = sqrt(GetSquaredDistanceTo(tri.fanRX, tri.fanRY));
		if (dist <= getNormalSize()) {
			tri.nfanRX = tri.fanRX;
			tri.nfanRY = tri.fanRY;
		} else {
			float f = float(getNormalSize()) / dist;
			tri.nfanRX = f * tri.fanRX + (1.0f - f) * getX();
			tri.nfanRY = f * tri.fanRY + (1.0f - f) * getY();
		}
	}
}

void C4FoWLight::ProjectPointOutward(float &x, float &y, float maxDistance) const
{
	float distanceDifference = std::min(maxDistance, (float) getTotalReach()) / sqrt((x - getX()) * (x - getX()) + (y - getY()) * (y - getY()));

	x = getX() + distanceDifference * (x-getX());
	y = getY() + distanceDifference * (y-getY());
}

void C4FoWLight::CalculateIntermediateFadeTriangles(TriangleList &triangles) const
{
	for (TriangleList::iterator it = triangles.begin(), nextIt = it; it != triangles.end(); ++it)
	{
		// wrap around
		++nextIt;
		if(nextIt == triangles.end()) nextIt = triangles.begin();

		C4FoWBeamTriangle &tri = *it, &nextTri = *nextIt; // just for convenience

		// don't calculate if it should not be drawn anyway
		if (tri.clipRight || nextTri.clipLeft) continue;

		float distFadeR = GetSquaredDistanceTo(tri.fadeRX, tri.fadeRY);
		float distNextFadeL = GetSquaredDistanceTo(nextTri.fadeLX, nextTri.fadeLY);
		float distFanR = GetSquaredDistanceTo(tri.fanRX, tri.fanRY);
		float distNextFanL = GetSquaredDistanceTo(nextTri.fanLX, nextTri.fanLY);
				
		// an extra intermediate fade point is only necessary on cliffs
		tri.descending = distFanR > distNextFanL;
		if (tri.descending) {
			if (distFanR < distNextFadeL)
			{
				tri.fadeIX = nextTri.fadeLX;
				tri.fadeIY = nextTri.fadeLY;
			}
			else
			{
				tri.fadeIX = (tri.fanRX + nextTri.fadeLX) / 2;
				tri.fadeIY = (tri.fanRY + nextTri.fadeLY) / 2;
				ProjectPointOutward(tri.fadeIX, tri.fadeIY, sqrt(distFadeR));
			}
		}
		else
		{
			if (distNextFanL < distFadeR)
			{
				tri.fadeIX = tri.fadeRX;
				tri.fadeIY = tri.fadeRY;
			}
			else
			{
				tri.fadeIX = (tri.fadeRX + nextTri.fanLX) / 2;
				tri.fadeIY = (tri.fadeRY + nextTri.fanLY) / 2;
				ProjectPointOutward(tri.fadeIX, tri.fadeIY, sqrt(distNextFadeL));
			}
		}
	}
}


void C4FoWLight::DrawFan(C4FoWDrawStrategy* pen, TriangleList &triangles) const
{
	pen->BeginFan();
	if (!triangles.empty()) pen->DrawLightVertex(getX(), getY());

	for (TriangleList::iterator it = triangles.begin(), nextIt = it; it != triangles.end(); ++it)
	{
		// wrap around
		++nextIt;
		if(nextIt == triangles.end()) nextIt = triangles.begin();

		C4FoWBeamTriangle &tri = *it, &nextTri = *nextIt; // just for convenience

		pen->DrawLightVertex(tri.nfanLX, tri.nfanLY);

		if(nextIt == triangles.begin() || nextTri.nfanLX != tri.nfanRX || nextTri.nfanLY != tri.nfanRY)
			pen->DrawLightVertex(tri.nfanRX, tri.nfanRY);
	}
	pen->EndFan();
}

void C4FoWLight::DrawFanMaxed(C4FoWDrawStrategy* pen, TriangleList &triangles) const
{
	pen->BeginFanMaxed();
	for (TriangleList::iterator it = triangles.begin(), nextIt = it; it != triangles.end(); ++it)
	{
		// Wrap around for next triangle
		++nextIt; if(nextIt == triangles.end()) nextIt = triangles.begin();
		C4FoWBeamTriangle &tri = *it, &nextTri = *nextIt;

		// First for the current beam
		if (tri.nfanLX != tri.nfanRX || tri.nfanLY != tri.nfanRY)
		{
			pen->DrawLightVertex(tri.nfanLX, tri.nfanLY);
			pen->DrawLightVertex(tri.nfanRX, tri.nfanRY);
			pen->DrawLightVertex(tri.fanRX, tri.fanRY);
			pen->DrawLightVertex(tri.fanLX, tri.fanLY);
		}
		// Then for the space in-between
		if (tri.nfanRX != nextTri.nfanLX || tri.nfanRY != nextTri.nfanLY)
		{
			pen->DrawLightVertex(tri.nfanRX, tri.nfanRY);
			pen->DrawLightVertex(nextTri.nfanLX, nextTri.nfanLY);
			pen->DrawLightVertex(nextTri.fanLX, nextTri.fanLY);
			pen->DrawLightVertex(tri.fanRX, tri.fanRY);
		}
	}
	pen->EndFanMaxed();
}

void C4FoWLight::DrawFade(C4FoWDrawStrategy* pen, TriangleList &triangles) const
{
	pen->BeginFade();

	for (TriangleList::iterator it = triangles.begin(); it != triangles.end(); ++it)
	{
		C4FoWBeamTriangle &tri = *it; // just for convenience

		// The quad will be empty if fan points match
		if (tri.fanLX == tri.fanRX && tri.fanLY == tri.fanRY) continue;

		pen->DrawLightVertex(tri.fanLX, tri.fanLY);
		pen->DrawLightVertex(tri.fanRX, tri.fanRY);
		pen->DrawDarkVertex(tri.fadeRX, tri.fadeRY);
		pen->DrawDarkVertex(tri.fadeLX, tri.fadeLY);
	}
	pen->EndFade();
}

void C4FoWLight::DrawIntermediateFadeTriangles(C4FoWDrawStrategy* pen, TriangleList &triangles) const
{

	for (TriangleList::iterator it = triangles.begin(), nextIt = it; it != triangles.end(); ++it)
	{
		// wrap around
		++nextIt;
		if(nextIt == triangles.end()) nextIt = triangles.begin();

		C4FoWBeamTriangle &tri = *it, &nextTri = *nextIt; // just for convenience
			
		// no inter-fade triangles when it should be clipped
		if (tri.clipRight || nextTri.clipLeft) continue;

		pen->BeginIntermediateFade();

		if (tri.descending) {

			pen->DrawLightVertex(tri.fanRX, tri.fanRY);
			pen->DrawLightVertex(nextTri.fanLX, nextTri.fanLY);
			pen->DrawDarkVertex(nextTri.fadeLX, nextTri.fadeLY);

			// if necessary
			if (tri.fadeIY != nextTri.fadeLY || tri.fadeIX != nextTri.fadeLX) {
				pen->DrawDarkVertex(tri.fadeIX, tri.fadeIY);
			}

			pen->DrawDarkVertex(tri.fadeRX, tri.fadeRY);

		} else {

			pen->DrawLightVertex(nextTri.fanLX, nextTri.fanLY);
			pen->DrawDarkVertex(nextTri.fadeLX, nextTri.fadeLY);

			// if necessary
			if (tri.fadeIY != tri.fadeRY || tri.fadeIX != tri.fadeRX) {
				pen->DrawDarkVertex(tri.fadeIX, tri.fadeIY);
			}

			pen->DrawDarkVertex(tri.fadeRX, tri.fadeRY);
			pen->DrawLightVertex(tri.fanRX, tri.fanRY);			
		}

		pen->EndIntermediateFade();
	}
}

bool C4FoWLight::IsVisibleForPlayer(C4Player *player) const
{
	// check if attached to an object that is not hostile to the given player
	if (!pObj || !player) return true;
	return !::Hostile(pObj->Owner,player->Number);
}

#endif
