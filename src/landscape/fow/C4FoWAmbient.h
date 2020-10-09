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

#ifndef C4FOWAMBIENT_H
#define C4FOWAMBIENT_H

#include "C4ForbidLibraryCompilation.h"

#include "landscape/C4Landscape.h"
#ifndef USE_CONSOLE
#include <epoxy/gl.h>
#endif

/**
	This class manages a texture that holds the ambient light intensity
*/
class C4FoWAmbient
{
public:
	C4FoWAmbient();
	~C4FoWAmbient();

#ifndef USE_CONSOLE
	GLuint Tex{0};
#endif

private:
	// Parameters
	double Resolution{0.};
	double Radius{0.};
	double FullCoverage{0.};
	// Landscape size
	unsigned int LandscapeX{0};
	unsigned int LandscapeY{0};
	// Size of ambient map
	unsigned int SizeX{0};
	unsigned int SizeY{0};
	// Brightness (not premultiplied)
	double Brightness{1.};
public:
	void Clear();

	void SetBrightness(double brightness) { Brightness = brightness; }
	double GetBrightness() const { return Brightness; }

	// High resolution will make the map coarser, but speed up the generation process
	// and save video memory.
	// The radius specifies the radius of landscape pixels that are sampled around each pixel
	// to obtain the ambient light.
	// full_coverage is a number between 0 and 1, and it specifies what portion of the full circle
	// needs to be illuminated for full ambient light intensity.
	void CreateFromLandscape(const C4Landscape& landscape, double resolution, double radius, double full_coverage);

	// Update the map after the landscape has changed in the region indicated by update.
	void UpdateFromLandscape(const C4Landscape& landscape, const C4Rect& update);

	// Fills a 2x3 matrix to transform fragment coordinates to ambient map texture coordinates
	void GetFragTransform(const struct FLOAT_RECT& vpRect, const C4Rect& clipRect, const C4Rect& outRect, float ambientTransform[6]) const;

	unsigned int GetLandscapeWidth() const { return LandscapeX; }
	unsigned int GetLandscapeHeight() const { return LandscapeY; }
};

#endif // C4FOWAMBIENT_H
