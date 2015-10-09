/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2013, The OpenClonk Team and contributors
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

/* Textures used by the landscape */

#ifndef INC_C4TextureShape
#define INC_C4TextureShape

#include <CSurface8.h>

// Custom texture drawing shape for Map2Landscape zooming
class C4TextureShape
{
private:
	enum { Shape_None = 0xff }; // special value in data surface: No shape defined here.
	CSurface8 data;
	int32_t num_shapes;
	std::vector<bool> shape_border_x, shape_border_y; // whether shapes are touching horizontal/vertical borders
	std::vector<int32_t> shape_pixnum; // number of pixels
public:
	C4TextureShape() : data(), num_shapes(0) {}
	~C4TextureShape() {}

	void Clear();
	bool Load(C4Group &group, const char *filename, int32_t base_tex_wdt, int32_t base_tex_hgt);

	int32_t GetWidth() const { return data.Wdt; }
	int32_t GetHeight() const { return data.Hgt; }
	// Poly range used to ensure update range in editor mode is large enough
	// not calculated on loading for now. Just assume something reasonably safe
	int32_t GetMaxPolyWidth() const { return GetWidth() / 4; }
	int32_t GetMaxPolyHeight() const { return GetHeight() / 4; }

	void Draw(CSurface8 * sfcMap, CSurface8* sfcMapBkg, int32_t iMapX, int32_t iMapY, int32_t iMapWdt, int32_t iMapHgt, uint8_t iTexture, int32_t iOffX, int32_t iOffY, int32_t MapZoom, int32_t min_overlap_ratio);
};

#endif
