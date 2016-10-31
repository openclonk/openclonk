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

/* Textures used by the landscape */

#include "C4Include.h"
#include "landscape/C4TextureShape.h"

#include "c4group/C4Group.h"
#include "landscape/C4Landscape.h"
#include "lib/C4Log.h"
#include "graphics/StdPNG.h"

// -------------------------------------- C4TextureShape

void C4TextureShape::Clear()
{
	data.Clear();
	num_shapes = 0;
	shape_border_x.clear();
	shape_border_y.clear();
	shape_pixnum.clear();
	shape_pixnum.reserve(128);
}

bool C4TextureShape::Load(C4Group &group, const char *filename, int32_t base_tex_wdt, int32_t base_tex_hgt)
{
	Clear();
	// Material shapes loading
	StdBuf png_data;
	if (!group.LoadEntry(filename, &png_data)) return false;
	CPNGFile png;
	if (!png.Load(static_cast<BYTE *>(png_data.getMData()), png_data.getSize())) return false;
	assert(base_tex_wdt > 0);
	int32_t zoom = png.iWdt / base_tex_wdt;
	if (base_tex_wdt * zoom != static_cast<int32_t>(png.iWdt) || base_tex_hgt * zoom != static_cast<int32_t>(png.iHgt))
	{
		LogF("ERROR: Material shape texture %s size (%d,%d) not a multiple of associated texture size (%d,%d). Not loading.", filename, int(png.iWdt), int(png.iHgt), int(base_tex_wdt), int(base_tex_hgt));
		return false;
	}
	// Prepare wrap info
	shape_border_x.resize(255, false);
	shape_border_y.resize(255, false);
	// Create shape data surface as downscaled version where equal pixel colors are assigned the same index
	std::map<uint32_t, uint8_t> clr2shape;
	std::vector<int32_t> first_px_pos_x(255), first_px_pos_y(255);
	if (!data.Create(base_tex_wdt, base_tex_hgt)) return false;
	for (int32_t y = 0; y < data.Hgt; ++y)
	{
		for (int32_t x = 0; x < data.Wdt; ++x)
		{
			uint32_t px = png.GetPix(x * zoom, y * zoom);
			uint8_t px_data;
			if ((px & 0xff000000u) == 0u)
			{
				// Fully transparent pixels are not part of a shape
				px_data = Shape_None;
			}
			else
			{
				// Otherwise, ignore alpha and lookup
				px &= 0xffffff;
				auto shape_idx = clr2shape.find(px);
				if (shape_idx == clr2shape.end())
				{
					// New shape
					px_data = num_shapes;
					clr2shape[px] = num_shapes;
					shape_pixnum.push_back(1);
					first_px_pos_x[num_shapes] = x;
					first_px_pos_y[num_shapes] = y;
					++num_shapes;
					if (num_shapes >= 255)
					{
						LogF("Material shape texture %s invalid: Too many shape colors (>=255).", filename);
						Clear();
						return false;
					}
				}
				else
				{
					// Another pixel of previous shape
					px_data = shape_idx->second;
					++shape_pixnum[px_data];
				}
				// Remember wrapping
				if (!x || x == data.Wdt - 1) shape_border_x[px_data] = true;
				if (!y || y == data.Hgt - 1) shape_border_y[px_data] = true;
			}
			data._SetPix(x, y, px_data);
		}
	}
	// Show a summary about found shapes in this texture
	if (Config.Developer.DebugShapeTextures)
	{
		LogF("Shape texture summary for %s (%d x %d downscaled to %d x %d):", filename, (int)png.iWdt, (int)png.iHgt, (int)base_tex_wdt, (int)base_tex_wdt);
		for (auto iter : clr2shape)
		{
			unsigned int clr = iter.first;
			int i = iter.second;
			LogF("  Color 0x%08x: %d pixels. First seen pos: %d/%d", clr, int(shape_pixnum[i]), int(first_px_pos_x[i]), int(first_px_pos_y[i]));
		}
	}

	return true;
}

// Activation map: Temp storage for whether shapes are activated in each covered block
// Contains Act_None if not active and background pixel color to be used otherwise
class C4TextureShapeActivationMap
{
private:
	std::vector<int32_t> activation; // number of pixels covered
	std::vector<uint8_t> back_pix; // last encountered background pixel
	int32_t block_x0, block_y0;
	int32_t n_blocks_x, n_blocks_y;
	int32_t num_shapes;

private:
	int32_t Idx(int32_t block_x, int32_t block_y, int32_t shape_idx, int32_t xpart, int32_t ypart) const; // get index into 5D activation and back_pix arrays

public:
	C4TextureShapeActivationMap(int32_t block_x0, int32_t block_y0, int32_t n_blocks_x, int32_t n_blocks_y, int32_t num_shapes) : block_x0(block_x0), block_y0(block_y0), n_blocks_x(n_blocks_x), n_blocks_y(n_blocks_y), num_shapes(num_shapes)
	{
		activation.resize(n_blocks_x * n_blocks_y * num_shapes * 4, 0);
		back_pix.resize(n_blocks_x * n_blocks_y * num_shapes * 4, 0u);
	}

	int32_t Get(int32_t block_x, int32_t block_y, int32_t shape_idx, int32_t xpart, int32_t ypart, uint8_t *bg_pix) const;
	void Add(int32_t block_x, int32_t block_y, int32_t shape_idx, int32_t xpart, int32_t ypart, uint8_t back_pix);
};

int32_t C4TextureShapeActivationMap::Idx(int32_t block_x, int32_t block_y, int32_t shape_idx, int32_t xpart, int32_t ypart) const
{
	// return index from 5D coords. -1 for out of bounds.
	block_x -= block_x0; block_y -= block_y0;
	if (!Inside(block_x, 0, n_blocks_x - 1) || !Inside(block_y, 0, n_blocks_y - 1)) return -1;
	return ypart + 2 * (xpart + 2 * (shape_idx + num_shapes * (block_x + n_blocks_x * block_y)));
}

int32_t C4TextureShapeActivationMap::Get(int32_t block_x, int32_t block_y, int32_t shape_idx, int32_t xpart, int32_t ypart, uint8_t *bg_pix) const
{
	// activation map access
	int32_t idx = Idx(block_x, block_y, shape_idx, xpart, ypart);
	assert(idx >= 0);
	if (idx < 0) return 0;
	*bg_pix = back_pix[idx];
	return activation[idx];
}

void C4TextureShapeActivationMap::Add(int32_t block_x, int32_t block_y, int32_t shape_idx, int32_t xpart, int32_t ypart, uint8_t bg_pix)
{
	// activation map access
	int32_t idx = Idx(block_x, block_y, shape_idx, xpart, ypart);
	if (idx < 0)
	{
		// This can happen for maps in which the shape is too large or simply at landscape borders. So ignore.
		return;
	}
	++activation[idx];
	back_pix[idx] = bg_pix;
}


void C4TextureShape::Draw(const CSurface8 &sfcMap, const CSurface8 &sfcMapBkg, int32_t iMapX, int32_t iMapY, int32_t iMapWdt, int32_t iMapHgt, uint8_t iTexture, int32_t iOffX, int32_t iOffY, int32_t MapZoom, int32_t min_overlap_ratio)
{
	// Safety
	if (!num_shapes) return;
	// Get affected range of shapes in pixels
	// Add max polygon size because polygons may extent far onto outside pixels
	int32_t x0 = std::max<int32_t>(0, iMapX*MapZoom + iOffX - GetMaxPolyWidth()),
		y0 = std::max<int32_t>(0, iMapY*MapZoom + iOffY - GetMaxPolyHeight());
	int32_t x1 = std::min<int32_t>(::Landscape.GetWidth(), x0 + iMapWdt*MapZoom + GetMaxPolyWidth() * 2),
		y1 = std::min<int32_t>(::Landscape.GetHeight(), y0 + iMapHgt*MapZoom + GetMaxPolyHeight() * 2);
	// Range in shape blocks.
	// A shape block is the coverage of the size of one loaded shape data surface
	int32_t rblock_x0 = x0 / data.Wdt;
	int32_t rblock_y0 = y0 / data.Hgt;
	int32_t rblock_x1 = (x1 - 1) / data.Wdt + 1;
	int32_t rblock_y1 = (y1 - 1) / data.Hgt + 1;
	int32_t n_blocks_x = rblock_x1 - rblock_x0 + 1;
	int32_t n_blocks_y = rblock_y1 - rblock_y0 + 1;

	// Step 1: Find all active shapes and store them in activation map
	// The activation map covers all repeated shape blocks in the updated areas and tiles each block into 2x2 sub-blocks.
	// Sub-blocks handle the case where shapes wrap out of the side of a block into the next block.
	C4TextureShapeActivationMap activation(rblock_x0, rblock_y0, n_blocks_x, n_blocks_y, num_shapes);
	for (int32_t map_y = iMapY; map_y < iMapY + iMapHgt; ++map_y)
	{
		for (int32_t map_x = iMapX; map_x < iMapX + iMapWdt; ++map_x)
		{
			if (sfcMap.GetPix(map_x, map_y) == iTexture)
			{
				// Here we have a pixel of the texture drawn in this shape
				// Find all shapes covered by this map pixel and remember background pixel for them
				const BYTE pixBkg = sfcMapBkg.GetPix(map_x, map_y);
				// Find all shape blocks to be checked
				int32_t px_check_rate = 1; // sample rate to check coverage, in pixels. Could also increase this if it turns out to be a bottleneck
				for (int32_t pxo_y = 0; pxo_y < MapZoom; pxo_y += px_check_rate)
				{
					int32_t px_y = map_y * MapZoom + pxo_y + iOffY;
					int32_t spx_y = px_y % data.Hgt;
					int32_t block_y = px_y / data.Hgt;
					for (int32_t pxo_x = 0; pxo_x < MapZoom; pxo_x += px_check_rate)
					{
						int32_t px_x = map_x * MapZoom + pxo_x + iOffX;
						int32_t spx_x = px_x % data.Wdt;
						int32_t block_x = px_x / data.Wdt;
						BYTE shape_idx = data._GetPix(spx_x, spx_y);
						// No shape here or already handled?
						if (shape_idx == Shape_None) continue;
						// We touched a new shape! Activate it in the proper activation vectors
						int32_t block_x1 = block_x, block_x2 = block_x;
						// Is this shape wrapping along x?
						if (shape_border_x[shape_idx])
						{
							// It is! Are we on the left or right side of the shape block?
							if (spx_x < data.Wdt / 2)
							{
								// Left side. Activate left sub-block of current block and right sub-block of block left to that
								--block_x2;
							}
							else
							{
								// Right side. Activate right sub-block of current block and left sub-block of block right to that
								++block_x1;
							}
						}
						// Same checks for vertical
						int32_t block_y1 = block_y, block_y2 = block_y;
						if (shape_border_y[shape_idx]) { if (spx_y < data.Hgt / 2) --block_y2; else ++block_y1; }
						// Store activation (performs additional border checks)
						// This may overwrite the previous pixBkg when multiple chunks are covered
						// So effectively, it will always have the background of the bottom right map coverage
						// Could manage priorities here and ensure the center determines the background
						// - but it's just for the corner case of map designers switching background material within a single patch.
						activation.Add(block_x1, block_y1, shape_idx, 0, 0, pixBkg);
						activation.Add(block_x2, block_y1, shape_idx, 1, 0, pixBkg);
						activation.Add(block_x1, block_y2, shape_idx, 0, 1, pixBkg);
						activation.Add(block_x2, block_y2, shape_idx, 1, 1, pixBkg);
					}
				}
			}
		}
	}

	// Step 2: Draw texture on all active shapes
	for (int32_t y = y0; y < y1; ++y)
	{
		int32_t block_y = y / data.Hgt;
		int32_t blockpos_y = y % data.Hgt;
		int32_t subblock_y = (blockpos_y >= data.Hgt / 2) ? 1 : 0;
		for (int32_t x = x0; x < x1; ++x)
		{
			int32_t block_x = x / data.Wdt;
			int32_t blockpos_x = x % data.Wdt;
			int32_t subblock_x = (blockpos_x >= data.Wdt / 2) ? 1 : 0;
			int32_t shape_idx = data._GetPix(blockpos_x, blockpos_y);
			if (shape_idx == Shape_None) continue;
			uint8_t pixBg = 0;
			int32_t act = activation.Get(block_x, block_y, shape_idx, subblock_x, subblock_y, &pixBg);
			if (!act || act < shape_pixnum[shape_idx] * min_overlap_ratio / 100) continue;
			// Shape active at this pixel. Draw it.
			::Landscape._SetPix2(x, y, iTexture, pixBg);
		}
	}
}
