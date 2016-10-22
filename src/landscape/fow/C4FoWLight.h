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
#ifndef C4FOWLIGHT_H
#define C4FOWLIGHT_H

#include "C4ForbidLibraryCompilation.h"

#ifndef USE_CONSOLE

#include "object/C4Object.h"
#include "graphics/C4Surface.h"
#include "graphics/C4FacetEx.h"
#include "landscape/fow/C4FoWLightSection.h"
#include "landscape/fow/C4FoWDrawStrategy.h"
#include "lib/C4Rect.h"

#include <vector>

/** This class represents one light source. A light source has an associated object with which the light source moves
    and one light section that handles the light beams for each direction (up, down, left, right).

    Furthermore, each light source has a size. This is usually the object's lightRange. See SetReach.
*/
class C4FoWLight
{
	friend class C4FoW;
public:
	C4FoWLight(C4Object *pObj);
	~C4FoWLight();

private:
	int32_t iX, iY; // center position
	int32_t iReach; // maximum length of beams
	int32_t iFadeout; // number of pixels over which beams fade out
	int32_t iSize; // size of the light source. Decides smoothness of shadows
	float gBright; // brightness of the light source. 1.0 is maximum.
	float colorR; // red color component of the light source. 1.0 is maximum.
	float colorG; // green color component of the light source. 1.0 is maximum.
	float colorB; // blue color component of the light source. 1.0 is maximum.
	float colorV; // color value. 1.0 is maximum.
	float colorL; // color lightness. 1.0 is maximum.
	C4FoWLight *pNext;
	C4Object *pObj; // Associated object

	std::vector<C4FoWLightSection*> sections;

public:
	int32_t getX() const { return iX; }
	int32_t getY() const { return iY; }
	int32_t getReach() const { return iReach; }
	int32_t getFadeout() const { return iFadeout; }
	int32_t getTotalReach() const { return iReach + iFadeout; }
	int32_t getSize() const { return iSize; }
	int32_t getNormalSize() const { return iSize * 2; }
	float getBrightness() const { return colorV * gBright; }
	float getR() const { return colorR; }
	float getG() const { return colorG; }
	float getB() const { return colorB; }
	float getValue() const { return colorV; }
	float getLightness() const { return colorL; }
	C4FoWLight *getNext() const { return pNext; }
	C4Object *getObj() const { return pObj; }

	/** Sets the light's size in pixels. The reach is the total radius of the light while the fadeout is the number of 
	    pixels after which the light should dim down */
	void SetReach(int32_t iReach, int32_t iFadeout);

	/** Sets the light's color in rgba format. */
	void SetColor(uint32_t iValue);
	
	/** Triggers the recalculation of all light beams within the given rectangle for this light because the landscape changed. */
	void Invalidate(C4Rect r);
	/** Update all light beams within the given rectangle for this light */
	void Update(C4Rect r);
	/** Render this light*/
	void Render(class C4FoWRegion *pRegion, const C4TargetFacet *pOnScreen, C4ShaderCall& call);

	bool IsVisibleForPlayer(C4Player *player) const; // check if attached to an object that is not hostile to the given player

private:
	typedef std::list<class C4FoWBeamTriangle> TriangleList;

	/** Calculate "normal" fan points - where the normal hasn't maxed out yet */
	void CalculateFanMaxed(TriangleList &triangles) const;
	/** Calculate the intermediate fade points used for constructing the intermediate fade triangles later on */
	void CalculateIntermediateFadeTriangles(TriangleList &triangles) const;

	void ProjectPointOutward(float &x, float &y, float maxDistance) const;

	/** Draws the triangle fan (the area with 100% light around the light source) with the given strategy */
	void DrawFan(class C4FoWDrawStrategy* pen, TriangleList &triangles) const;
	/** Draws the triangle fan (100% light, maxed out normals) with the given strategy */
	void DrawFanMaxed(class C4FoWDrawStrategy* pen, TriangleList &triangles) const;
	/** Draws the fadeoot triangles - those around the triangle fan - with the given strategy */
	void DrawFade(C4FoWDrawStrategy* pen, TriangleList &triangles) const;
	/** Draws the fadeout triangles in between the normal fadeout triangles with the given strategy */
	void DrawIntermediateFadeTriangles(C4FoWDrawStrategy* pen, TriangleList &triangles) const;
	/** Returns the (squared) distance from this light source to the given point. Squared simply because we only need this
	    for comparison of distances. So we don't bother to sqrt it */
	float GetSquaredDistanceTo(int32_t x, int32_t y) const { return (x - getX()) * (x - getX()) + (y - getY()) * (y - getY()); }

	/* Draw strategy instances. We keep them around once created, so they can
	 * reuse a VBO between individual renderings. */
	std::unique_ptr<C4FoWDrawStrategy> OnScreenStrategy;
	std::unique_ptr<C4FoWDrawStrategy> OffScreenStrategy;
};

#endif

#endif
