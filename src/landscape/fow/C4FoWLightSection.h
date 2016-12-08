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

#ifndef C4FOWLIGHTSECTION_H
#define C4FOWLIGHTSECTION_H

#include "C4ForbidLibraryCompilation.h"

#ifndef USE_CONSOLE

#include "lib/C4Rect.h"
#include <list>

class C4FoWLight;
class C4FoWRegion;
class C4FoWBeam;
class C4FoWBeamTriangle;

/** The light section manages the beams for one light for one direction of 90°.
    
	For understanding the internal calculations made in this class, note that this class assumes all of its beams to 
	go into a positive y-direction (="down" in display coordinates). Only after the calculation, it transforms its
	result into the global coordinate system again by applying a transformation matrix. 
	*/
class C4FoWLightSection
{
public:
	C4FoWLightSection(C4FoWLight *pLight, int r);
	~C4FoWLightSection();

private:

	/* Center light */
	C4FoWLight *pLight;

	/* Transformation matrix */
	int iRot;
	int a, b, c, d;
	int ra, rb, rc, rd;

	/* This section's beams */
	C4FoWBeam *pBeams;
	
public:
	
	/** Recalculate of all light beams within the given rectangle because the landscape changed. */
	void Invalidate(C4Rect r);
	/** Update all light beams within the given rectangle */
	void Update(C4Rect r);


	std::list<C4FoWBeamTriangle> CalculateTriangles(C4FoWRegion *region) const;

	/** Shorten all light beams to the given reach.
	    Called when the size of the light has decreased to the given value */
	void Prune(int32_t reach);
	/** Extend all light beams to the given reach.
	    Called when the size of the light has increased to the given value */
	void Dirty(int32_t reach);

private:

	/** Remove all beams. pBeams is nullptr after that. */
	void ClearBeams();

	// Beam coordinate to landscape coordinate. Beam coordinates are relative to the light source.
	template <class T> T transDX(T dx, T dy) const;
	template <class T> T transDY(T dx, T dy) const;
	template <class T> T transX(T x, T y) const;
	template <class T> T transY(T x, T y) const;

	// Landscape coordinate to beam coordinate. Beam coordinates are relative to the light source.
	template <class T> T rtransDX(T dx, T dy) const;
	template <class T> T rtransDY(T dx, T dy) const;
	template <class T> T rtransX(T x, T y) const;
	template <class T> T rtransY(T x, T y) const;

	/** Convert triangles to landscape coordinates */
	void transTriangles(std::list<C4FoWBeamTriangle> &triangles) const;

	/** Returns a rectangle in beam coordinates */
	C4Rect rtransRect(C4Rect r) const {
		C4Rect Rect(rtransX(r.x, r.y), rtransY(r.x, r.y),
		            rtransDX(r.Wdt, r.Hgt), rtransDY(r.Wdt, r.Hgt));
		Rect.Normalize();
		return Rect;
	}

	bool isConsistent() const;

	/** These methods return the position of the left delimiter point of a beam from the origin that would enclose the 
	    given rectangle. Note that we assume the rect to have a positive Y-position.
		In other words, the given rectangle's right most point when looked at from the origin.
	  */
	inline int32_t RectLeftMostX(const C4Rect &r) const { return r.x; }
	inline int32_t RectLeftMostY(const C4Rect &r) const { return std::max(0, r.x >= 0 ? r.y + r.Hgt : r.y); }
	/** These methods return the position of the left delimiter point of a beam from the origin that would enclose the 
	    given rectangle. Note that we assume the rect to have a positive Y-position.
		In other words, the given rectangle's right most point when looked at from the origin.
	  */
	inline int32_t RectRightMostX(const C4Rect &r) const { return r.x + r.Wdt; }
	inline int32_t RectRightMostY(const C4Rect &r) const { return std::max(0, r.x + r.Wdt <= 0 ? r.y + r.Hgt : r.y); }

	inline void LightBallExtremePoint(float x, float y, float dir, float &lightX, float &lightY) const;

	/** Outputs the rightmost position of the light ball, as seen from the given point. Shrinks the light if it is too close
	    to work against excessive fades. The light ball is the imaginery size of the light to enable soft shadows. */
	inline void LightBallRightMostPoint(float x, float y, float &lightX, float &lightY) const;

	/** Outputs the leftmost position of the light ball, as seen from the given point. Shrinks the light if it is too close
	    to work against excessive fades. The light ball is the imaginery size of the light to enable soft shadows. */
	inline void LightBallLeftMostPoint(float x, float y, float &lightX, float &lightY) const;


	/** Find right-most beam left of point */
	C4FoWBeam *FindBeamLeftOf(int32_t x, int32_t y) const;

	/** Find beams that go through the given rectangle. Returns the number of beams that are in the rectangle and makes
	    firstBeam point to the first and endBeam point to the beam after the last of these. Thus, endBeam is nullptr if 
		no beams were clipped at the end. */
	int32_t FindBeamsClipped(const C4Rect &rect, C4FoWBeam *&firstBeam, C4FoWBeam *&endBeam) const;

public:
	// Serialization for debugging purposes
	void CompileFunc(StdCompiler *pComp);

};

#endif

#endif
