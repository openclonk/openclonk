/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2014-2015, The OpenClonk Team and contributors
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

#ifndef C4FOWREGION_H
#define C4FOWREGION_H

#include "C4Rect.h"
#include "C4Surface.h"
#include "C4FacetEx.h"
#include "C4Player.h"
#include "C4FoW.h"

class C4FoWRegion
{
public:
	C4FoWRegion(C4FoW *pFoW, C4Player *pPlayer);
	~C4FoWRegion();

private:
	C4FoW *pFoW;
	C4Player *pPlayer;
	C4Rect Region, OldRegion;
	FLOAT_RECT ViewportRegion; // Region covered by visible viewport
	C4Surface *pSurface, *pBackSurface;
	GLuint hFrameBufDraw, hFrameBufRead;

public:
	const C4FoW* getFoW() const { return pFoW; }
	const C4Rect &getRegion() const { return Region; }
	const FLOAT_RECT &getViewportRegion() const { return ViewportRegion; }
	const C4Surface *getSurface() const { return pSurface; }
	const C4Surface *getBackSurface() const { return pBackSurface; }

	void Clear();

	void Update(C4Rect r, const FLOAT_RECT& vp);
	void Render(const C4TargetFacet *pOnScreen = NULL);

	// Fills a 2x3 matrix to transform fragment coordinates to light texture coordinates
	void GetFragTransform(const C4Rect& clipRect, const C4Rect& outRect, float lightTransform[6]) const;
private:
	bool BindFramebuf();

};

#endif
