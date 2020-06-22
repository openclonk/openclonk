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

#ifndef C4FOWREGION_H
#define C4FOWREGION_H

#include "C4ForbidLibraryCompilation.h"
#include "lib/C4Rect.h"
#include "graphics/C4FacetEx.h"
#include "player/C4Player.h"
#include "landscape/fow/C4FoW.h"
#ifndef USE_CONSOLE
#include <epoxy/gl.h>
#endif

class C4Surface;

class C4FoWRegion
{
public:
	C4FoWRegion(C4FoW *pFoW, C4Player *pPlayer);
	~C4FoWRegion();

private:
	C4FoW *pFoW;
#ifndef USE_CONSOLE
	C4Player *pPlayer;
#endif
	std::unique_ptr<C4Surface> pSurface, pBackSurface;
	C4Rect Region, OldRegion;
	FLOAT_RECT ViewportRegion; // Region covered by visible viewport

#ifndef USE_CONSOLE
	GLuint hFrameBufDraw, hFrameBufRead;
	GLuint hVBO;
	unsigned int vaoid;
#endif

public:
	const C4FoW* getFoW() const { return pFoW; }
	const C4Rect &getRegion() const { return Region; }
	const FLOAT_RECT &getViewportRegion() const { return ViewportRegion; }
	int32_t getSurfaceHeight() const;
	int32_t getSurfaceWidth() const;

#ifndef USE_CONSOLE
	GLuint getSurfaceName() const;
#endif

	void Update(C4Rect r, const FLOAT_RECT& vp);
	bool Render(const C4TargetFacet *pOnScreen = nullptr);

	// Fills a 2x3 matrix to transform fragment coordinates to light texture coordinates
	void GetFragTransform(const C4Rect& clipRect, const C4Rect& outRect, float lightTransform[6]) const;
private:
#ifndef USE_CONSOLE
	bool BindFramebuf(GLuint prev_fb);
#endif
};

#endif
