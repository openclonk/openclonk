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

#include <C4Include.h>
#include <C4Texture.h>

#include <C4Group.h>
#include <C4Game.h>
#include <C4Config.h>
#include <C4Components.h>
#include <C4Material.h>
#include <C4Landscape.h>
#include <C4Log.h>
#include <StdPNG.h>

#include <ctype.h>
#include <algorithm>

// -------------------------------------- C4TextureShape

C4TextureShape::C4TextureShape() : data(), num_shapes(0)
{
}

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
	int32_t zoom = png.iWdt / base_tex_wdt;
	if (base_tex_wdt * zoom != png.iWdt || base_tex_hgt * zoom != png.iHgt)
	{
		LogF("ERROR: Material shape texture %s size (%d,%d) not a multiple of associated texture size (%d,%d). Not loading.", filename, int(png.iWdt), int(png.iHgt), int(base_tex_wdt), int(base_tex_hgt));
		return false;
	}
	// Prepare wrap info
	shape_border_x.resize(255, false);
	shape_border_y.resize(255, false);
	// Create shape data surface as downscaled version where equal pixel colors are assigned the same index
	std::map<uint32_t, uint8_t> clr2shape;
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


void C4TextureShape::Draw(CSurface8 * sfcMap, CSurface8* sfcMapBkg, int32_t iMapX, int32_t iMapY, int32_t iMapWdt, int32_t iMapHgt, uint8_t iTexture, int32_t iOffX, int32_t iOffY, int32_t MapZoom, int32_t min_overlap_ratio)
{
	// Safety
	if (!num_shapes) return;
	// Get affected range of shapes in pixels
	// Add max polygon size because polygons may extent far onto outside pixels
	int32_t x0 = Max<int32_t>(0, iMapX*MapZoom + iOffX - GetMaxPolyWidth()),
		y0 = Max<int32_t>(0, iMapY*MapZoom + iOffY - GetMaxPolyHeight());
	int32_t x1 = Min<int32_t>(::Landscape.Width, x0 + iMapWdt*MapZoom + GetMaxPolyWidth() * 2),
		y1 = Min<int32_t>(::Landscape.Height, y0 + iMapHgt*MapZoom + GetMaxPolyHeight() * 2);
	// Range in shape blocks.
	// A shape block is the coverage of the size of one loaded shape data surface
	int32_t rblock_x0 = x0 / data.Wdt;
	int32_t rblock_y0 = y0 / data.Hgt;
	int32_t rblock_x1 = (x1 - 1) / data.Wdt + 1;
	int32_t rblock_y1 = (y1 - 1) / data.Hgt + 1;
	int32_t n_blocks_x = rblock_x1 - rblock_x0 + 1;
	int32_t n_blocks_y = rblock_x1 - rblock_x0 + 1;

	// Step 1: Find all active shapes and store them in activation map
	// The activation map covers all repeated shape blocks in the updated areas and tiles each block into 2x2 sub-blocks.
	// Sub-blocks handle the case where shapes wrap out of the side of a block into the next block.
	C4TextureShapeActivationMap activation(rblock_x0, rblock_y0, n_blocks_x, n_blocks_y, num_shapes);
	for (int32_t map_y = iMapY; map_y < iMapY + iMapHgt; ++map_y)
	{
		for (int32_t map_x = iMapX; map_x < iMapX + iMapWdt; ++map_x)
		{
			if (sfcMap->GetPix(map_x, map_y) == iTexture)
			{
				// Here we have a pixel of the texture drawn in this shape
				// Find all shapes covered by this map pixel and remember background pixel for them
				const BYTE pixBkg = sfcMapBkg->GetPix(map_x, map_y);
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
						if (shape_border_y[shape_idx]) if (spx_y < data.Hgt / 2) --block_y2; else ++block_y1;
						// Store activation (performs additional border checks)
						// This may overwrite the previous pixBkg when multiple chunks are covered
						// So effectively, it will always have the background of the bottom right map coverage
						// Could manage priorities here and ensure the center determines the background
						// - but it's just for the corner case of map designers switching background material within a single path.
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

C4Texture::C4Texture()
{
	Surface32=NULL;
	AvgColor = 0x00000000;
	Next=NULL;
}

C4Texture::~C4Texture()
{
	delete Surface32;
}

C4TexMapEntry::C4TexMapEntry()
		: iMaterialIndex(MNone), pMaterial(NULL)
{
}

void C4TexMapEntry::Clear()
{
	Material.Clear(); Texture.Clear();
	iMaterialIndex = MNone;
	pMaterial = NULL;
	MatPattern.Clear();
}

bool C4TexMapEntry::Create(const char *szMaterial, const char *szTexture)
{
	// Clear previous data
	Clear();
	// Save names
	Material = szMaterial; Texture = szTexture;
	return true;
}

bool C4TexMapEntry::Init()
{
	// Find material
	iMaterialIndex = ::MaterialMap.Get(Material.getData());
	if (!MatValid(iMaterialIndex))
	{
		DebugLogF("Error initializing material %s-%s: Invalid material!", Material.getData(), Texture.getData());
		return false;
	}
	pMaterial = &::MaterialMap.Map[iMaterialIndex];
	// Find texture
	StdStrBuf FirstTexture;
	FirstTexture.CopyUntil(Texture.getData(), '-');
	C4Texture * sfcTexture = ::TextureMap.GetTexture(FirstTexture.getData());
	if (!sfcTexture)
	{
		DebugLogF("Error initializing material %s-%s: Invalid texture!", Material.getData(), FirstTexture.getData());
		Clear();
		return false;
	}
	// Get overlay properties
	int32_t iOverlayType=pMaterial->OverlayType;
	int32_t iZoom=0;
	if (iOverlayType & C4MatOv_Exact) iZoom=1;
	if (iOverlayType & C4MatOv_HugeZoom) iZoom=4;
	// Create pattern
	MatPattern.Set(sfcTexture->Surface32, iZoom);
	return true;
}

C4TextureMap::C4TextureMap() : FirstTexture(NULL), fEntriesAdded(false), fOverloadMaterials(false), fOverloadTextures(false), fInitialized(false)
{
	Order.reserve(C4M_MaxTexIndex);
}

C4TextureMap::~C4TextureMap()
{
	Clear();
}

bool C4TextureMap::AddEntry(BYTE byIndex, const char *szMaterial, const char *szTexture)
{
	// Security
	if (byIndex <= 0 || byIndex >= C4M_MaxTexIndex)
		return false;
	// Set entry and initialize
	Entry[byIndex].Create(szMaterial, szTexture);
	if (fInitialized)
	{
		if (!Entry[byIndex].Init())
		{
			// Clear entry if it could not be initialized
			Entry[byIndex].Clear();
			return false;
		}
		// Landscape must be notified (new valid pixel clr)
		::Landscape.HandleTexMapUpdate();
	}
	// Add last in order list
	Order.push_back(byIndex);
	return true;
}

bool C4TextureMap::AddTexture(const char *szTexture, C4Surface * sfcSurface)
{
	C4Texture *pTexture;
	if (!(pTexture=new C4Texture)) return false;
	pTexture->Name.Copy(szTexture);
	pTexture->Surface32=sfcSurface;
	pTexture->Next=FirstTexture;
	FirstTexture=pTexture;

	// Compute average texture color
	if(sfcSurface)
	{
		sfcSurface->Lock();
		uint32_t avg_c[4] = { 0, 0, 0, 0 };
		for(int32_t y = 0; y < sfcSurface->Hgt; ++y)
		{
			for(int32_t x = 0; x < sfcSurface->Wdt; ++x)
			{
				DWORD c = sfcSurface->GetPixDw(x, y, false);
				avg_c[0] += c & 0xff;
				avg_c[1] += (c >> 8) & 0xff;
				avg_c[2] += (c >> 16) & 0xff;
				avg_c[3] += (c >> 24) & 0xff;
			}
		}
		sfcSurface->Unlock();

		double Size = sfcSurface->Wdt * sfcSurface->Hgt;
		avg_c[0] = static_cast<uint32_t>(avg_c[0] / Size + 0.5);
		avg_c[1] = static_cast<uint32_t>(avg_c[1] / Size + 0.5);
		avg_c[2] = static_cast<uint32_t>(avg_c[2] / Size + 0.5);
		avg_c[3] = static_cast<uint32_t>(avg_c[3] / Size + 0.5);
		pTexture->SetAverageColor(avg_c[0] | (avg_c[1] << 8) | (avg_c[2] << 16) | (avg_c[3] << 24));
	}
	else
	{
		pTexture->SetAverageColor(0x00000000);
	}

	return true;
}

void C4TextureMap::Clear()
{
	for (int32_t i = 1; i < C4M_MaxTexIndex; i++)
		Entry[i].Clear();
	C4Texture *ctex,*next2;
	for (ctex=FirstTexture; ctex; ctex=next2)
	{
		next2=ctex->Next;
		delete ctex;
	}
	FirstTexture=NULL;
	fInitialized = false;
	fEntriesAdded = false;
	fOverloadMaterials = false;
	fOverloadTextures = false;
	Order.clear();
	Order.reserve(C4M_MaxTexIndex);
}

bool C4TextureMap::LoadFlags(C4Group &hGroup, const char *szEntryName, bool *pOverloadMaterials, bool *pOverloadTextures)
{
	// Load the file
	StdStrBuf TexMap;
	if (!hGroup.LoadEntryString(szEntryName, &TexMap))
		return false;
	// Reset flags
	if (pOverloadMaterials) *pOverloadMaterials = false;
	if (pOverloadTextures) *pOverloadTextures = false;
	// Check if there are flags in there
	for (const char *pPos = TexMap.getData(); pPos && *pPos; pPos = SSearch(pPos + 1, "\n"))
	{
		// Go over newlines
		while (*pPos == '\r' || *pPos == '\n') pPos++;
		// Flag?
		if (pOverloadMaterials && SEqual2(pPos, "OverloadMaterials"))
			*pOverloadMaterials = true;
		if (pOverloadTextures && SEqual2(pPos, "OverloadTextures"))
			*pOverloadTextures = true;
	}
	// Done
	return true;
}

int32_t C4TextureMap::LoadMap(C4Group &hGroup, const char *szEntryName, bool *pOverloadMaterials, bool *pOverloadTextures)
{
	static re::regex line_terminator("\r?\n", static_cast<re::regex::flag_type>(re::regex_constants::optimize | re::regex_constants::ECMAScript));

	char *bpMap;
	size_t map_size;
	int32_t iTextures = 0;
	// Load text file into memory
	if (!hGroup.LoadEntry(szEntryName,&bpMap,&map_size,1)) return 0;

	char *begin = bpMap;
	char *end = begin + map_size;

	size_t line = 1; // Counter for error messages
	for (auto it = re::cregex_token_iterator(begin, end, line_terminator, -1); it != re::cregex_token_iterator(); ++it, ++line)
	{
		if (it->compare("OverloadMaterials") == 0)
		{
			fOverloadMaterials = true;
			if (pOverloadMaterials)
				*pOverloadMaterials = true;
		}
		else if (it->compare("OverloadTextures") == 0)
		{
			fOverloadTextures = true;
			if (pOverloadTextures)
				*pOverloadTextures = true;
		}
		else if (it->length() == 0 || it->first[0] == '#' || std::all_of(it->first, it->second, &isspace))
		{
			// Skip empty lines, comments, and all-whitespace lines
			continue;
		}
		else
		{
			// This must be a texmap entry now
			std::string value;
			
			// Read index
			unsigned long index;
			try
			{
				size_t separator;
				index = std::stoul(it->str(), &separator, 10);
				if (index >= C4M_MaxTexIndex)
					throw std::out_of_range("Texture index out of range");
				value.assign(it->first + separator + 1, it->second);
			}
			catch (std::invalid_argument &)
			{
				DebugLogF("TexMap line %u: Texture index is not numeric", static_cast<unsigned>(line));
				continue;
			}
			catch (std::out_of_range &)
			{
				DebugLogF("TexMap line %u: Texture index is out of range", static_cast<unsigned>(line));
				continue;
			}

			// Split material/texture combination
			std::string::const_iterator separator = std::find(value.cbegin(), value.cend(), '-');
			if (separator == value.cend())
			{
				DebugLogF("TexMap line %u: Texture name \"%s\" is invalid (missing \"-\")", static_cast<unsigned>(line), value.c_str());
				continue;
			}

			std::string material(value.cbegin(), separator);
			std::string texture(separator + 1, value.cend());

			if (AddEntry(index, material.c_str(), texture.c_str()))
				++iTextures;
		}
	}

	// Delete buffer, return entry count
	delete [] bpMap;
	fEntriesAdded=false;
	return iTextures;
}

int32_t C4TextureMap::Init()
{
	int32_t iRemoved = 0;
	// Initialize texture mappings
	int32_t i;
	for (i = 0; i < C4M_MaxTexIndex; i++)
		if (!Entry[i].isNull())
			if (!Entry[i].Init())
			{
				LogF("Error in TextureMap initialization at entry %d", (int) i);
				Entry[i].Clear();
				iRemoved++;
			}
	fInitialized = true;
	return iRemoved;
}

bool C4TextureMap::SaveMap(C4Group &hGroup, const char *szEntryName)
{
	// build file in memory
	StdStrBuf sTexMapFile;
	// add desc
	sTexMapFile.Append("# Automatically generated texture map" LineFeed);
	sTexMapFile.Append("# Contains material-texture-combinations added at runtime" LineFeed);
	// add overload-entries
	if (fOverloadMaterials) sTexMapFile.Append("# Import materials from global file as well" LineFeed "OverloadMaterials" LineFeed);
	if (fOverloadTextures) sTexMapFile.Append("# Import textures from global file as well" LineFeed "OverloadTextures" LineFeed);
	sTexMapFile.Append(LineFeed);
	// add entries
	for (auto iter = Order.begin(); iter != Order.end(); ++iter)
	{
		int32_t i = *iter;
		if (!Entry[i].isNull())
		{
			// compose line
			sTexMapFile.AppendFormat("%d=%s-%s" LineFeed, i, Entry[i].GetMaterialName(), Entry[i].GetTextureName());
		}
	}
	// create new buffer allocated with new [], because C4Group cannot handle StdStrBuf-buffers
	size_t iBufSize = sTexMapFile.getLength();
	BYTE *pBuf = new BYTE[iBufSize];
	memcpy(pBuf, sTexMapFile.getData(), iBufSize);
	// add to group
	bool fSuccess = !!hGroup.Add(szEntryName, pBuf, iBufSize, false, true);
	if (!fSuccess) delete [] pBuf;
	// done
	return fSuccess;
}

int32_t C4TextureMap::LoadTextures(C4Group &hGroup, C4Group* OverloadFile)
{
	int32_t texnum=0;
	// overload: load from other file
	if (OverloadFile) texnum+=LoadTextures(*OverloadFile);

	char texname[256+1];
	C4Surface *ctex;
	size_t binlen;

	hGroup.ResetSearch();
	while (hGroup.AccessNextEntry("*",&binlen,texname))
	{
		// check if it already exists in the map
		const char *base_filename = GetFilenameOnly(texname);
		if (GetTexture(base_filename)) continue;
		// check if it needs to be associated as shape to a loaded texture instead
		// (makes use of the fact that due to filename ordering, shapes will always be loaded after their textures)
		if (WildcardMatch("*" C4CFN_MaterialShapeFiles, texname))
		{
			// get associated texture
			StdStrBuf texname4shape(texname, true);
			texname4shape.SetLength(texname4shape.getLength() - SLen(C4CFN_MaterialShapeFiles));
			C4Texture *base_tex = GetTexture(texname4shape.getData());
			if (!base_tex || !base_tex->Surface32)
			{
				LogF("ERROR: Texture shape %s not loaded because associated texture (%s) not found or invalid.", hGroup.GetFullName().getData(), texname4shape.getData());
				continue;
			}
			std::unique_ptr<C4TextureShape> shape(new C4TextureShape());
			int32_t scaler_zoom = 4;
			if (!shape->Load(hGroup, texname, base_tex->Surface32->Wdt / scaler_zoom, base_tex->Surface32->Hgt / scaler_zoom))
			{
				LogF("Error loading texture shape %s.", hGroup.GetFullName().getData());
				continue;
			}
			base_tex->SetMaterialShape(shape.release());
			// never load as regular texture
			continue;
		}
		// create surface
		ctex = new C4Surface();
		if (ctex->Read(hGroup, GetExtension(texname), C4SF_MipMap))
		{
			SReplaceChar(texname,'.',0);
			if (AddTexture(texname,ctex)) texnum++;
			else delete ctex;
		}
		else
		{
			delete ctex;
		}
	}

	return texnum;
}

bool C4TextureMap::HasTextures(C4Group &hGroup)
{
	return hGroup.EntryCount(C4CFN_PNGFiles) || hGroup.EntryCount(C4CFN_BitmapFiles);
}

void C4TextureMap::MoveIndex(BYTE byOldIndex, BYTE byNewIndex)
{
	if (byNewIndex == byOldIndex) return;
	Entry[byNewIndex] = Entry[byOldIndex];
	Entry[byOldIndex].Clear();
	auto old_entry = std::find_if(Order.begin(), Order.end(),
		[byOldIndex](const int32_t &entry) { return entry == byOldIndex; });
	if (old_entry != Order.end()) *old_entry = byNewIndex;
	fEntriesAdded = true;
}

int32_t C4TextureMap::GetIndex(const char *szMaterial, const char *szTexture, bool fAddIfNotExist, const char *szErrorIfFailed)
{
	BYTE byIndex;
	// Find existing
	for (byIndex = 1; byIndex < C4M_MaxTexIndex; byIndex++)
		if (!Entry[byIndex].isNull())
			if (SEqualNoCase(Entry[byIndex].GetMaterialName(), szMaterial))
				if (!szTexture || SEqualNoCase(Entry[byIndex].GetTextureName(), szTexture))
					return byIndex;
	// Add new entry
	if (fAddIfNotExist)
		for (byIndex=1; byIndex<C4M_MaxTexIndex; byIndex++)
			if (Entry[byIndex].isNull())
			{
				if (AddEntry(byIndex, szMaterial, szTexture))
				{
					fEntriesAdded=true;
					return byIndex;
				}
				if (szErrorIfFailed) DebugLogF("Error getting MatTex %s-%s for %s from TextureMap: Init failed.", szMaterial, szTexture, szErrorIfFailed);
				return 0;
			}
	// Else, fail
	if (szErrorIfFailed) DebugLogF("Error getting MatTex %s-%s for %s from TextureMap: %s.", szMaterial, szTexture, szErrorIfFailed, fAddIfNotExist ? "Map is full!" : "Entry not found.");
	return 0;
}

int32_t C4TextureMap::GetIndexMatTex(const char *szMaterialTexture, const char *szDefaultTexture, bool fAddIfNotExist, const char *szErrorIfFailed)
{
	// split material/texture pair
	StdStrBuf Material, Texture;
	Material.CopyUntil(szMaterialTexture, '-');
	Texture.Copy(SSearch(szMaterialTexture, "-"));
	// texture not given or invalid?
	int32_t iMatTex = 0;
	if (Texture.getData())
		if ((iMatTex = GetIndex(Material.getData(), Texture.getData(), fAddIfNotExist)))
			return iMatTex;
	if (szDefaultTexture)
		if ((iMatTex = GetIndex(Material.getData(), szDefaultTexture, fAddIfNotExist)))
			return iMatTex;
	// search material
	long iMaterial = ::MaterialMap.Get(szMaterialTexture);
	if (!MatValid(iMaterial))
	{
		if (szErrorIfFailed) DebugLogF("Error getting MatTex for %s: Invalid material", szErrorIfFailed);
		return 0;
	}
	// return default map entry
	return ::MaterialMap.Map[iMaterial].DefaultMatTex;
}

C4Texture * C4TextureMap::GetTexture(const char *szTexture)
{
	C4Texture *pTexture;
	for (pTexture=FirstTexture; pTexture; pTexture=pTexture->Next)
		if (SEqualNoCase(pTexture->Name.getData(),szTexture))
			return pTexture;
	return NULL;
}

int32_t C4TextureMap::GetTextureIndex(const char *szName)
{
	C4Texture *pTexture;
	int32_t i=0;
	for (pTexture=FirstTexture; pTexture; pTexture=pTexture->Next, i++)
		if (SEqualNoCase(pTexture->Name.getData(),szName))
			return i;
	return -1;
}

bool C4TextureMap::CheckTexture(const char *szTexture)
{
	C4Texture *pTexture;
	for (pTexture=FirstTexture; pTexture; pTexture=pTexture->Next)
		if (SEqualNoCase(pTexture->Name.getData(),szTexture))
			return true;
	return false;
}

const char* C4TextureMap::GetTexture(int32_t iIndex)
{
	C4Texture *pTexture;
	int32_t cindex;
	for (pTexture=FirstTexture,cindex=0; pTexture; pTexture=pTexture->Next,cindex++)
		if (cindex==iIndex)
			return pTexture->Name.getData();
	return NULL;
}

BYTE C4TextureMap::DefaultBkgMatTex(BYTE fg) const
{
	// For the given foreground index, find the default background index
	// If fg is semisolid, this is tunnel.
	// Otherwise it is fg itself, so that tunnel and background bricks
	// stay the way they are.
	int32_t iTex = PixCol2Tex(fg);
	if (!iTex) return fg; // sky

	// Get material-texture mapping
	const C4TexMapEntry *pTex = GetEntry(iTex);
	// Texmap entry does not exist
	if(!pTex || !pTex->GetMaterial()) return fg;

	if(DensitySemiSolid(pTex->GetMaterial()->Density))
		return Mat2PixColDefault(MTunnel);

	return fg;

}

void C4TextureMap::RemoveEntry(int32_t iIndex)
{
	// remove entry from table and order vector
	if (Inside<int32_t>(iIndex, 1, C4M_MaxTexIndex - 1))
	{
		Entry[iIndex].Clear();
		auto last_entry = std::remove_if(Order.begin(), Order.end(),
			[iIndex](const int32_t &entry) { return entry == iIndex; });
		Order.erase(last_entry, Order.end());
	}
}

void C4TextureMap::StoreMapPalette(CStdPalette *Palette, C4MaterialMap &rMaterial)
{
	// Sky color
	Palette->Colors[0] = C4RGB(192, 196, 252);
	// Material colors by texture map entries
	bool fSet[C4M_MaxTexIndex];
	ZeroMem(&fSet, sizeof (fSet));
	int32_t i;
	for (i = 0; i < C4M_MaxTexIndex; i++)
	{
		// Find material
		DWORD dwPix = Entry[i].GetPattern().PatternClr(0, 0);
		Palette->Colors[i] = dwPix;
		fSet[i] = true;
	}
	// Crosscheck colors, change equal palette entries
	for (i = 0; i < C4M_MaxTexIndex; i++) if (fSet[i])
			for (;;)
			{
				// search equal entry
				int32_t j = 0;
				for (; j < i; j++)
					if (fSet[j] && Palette->Colors[i] == Palette->Colors[j])
							break;
				// not found? ok then
				if (j >= i) break;
				// change randomly
				Palette->Colors[i] = C4RGB(
					(rand() < RAND_MAX / 2) ? GetRedValue(Palette->Colors[i]) + 3 : GetRedValue(Palette->Colors[i]) - 3,
					(rand() < RAND_MAX / 2) ? GetGreenValue(Palette->Colors[i]) + 3 : GetGreenValue(Palette->Colors[i]) - 3,
					(rand() < RAND_MAX / 2) ? GetBlueValue(Palette->Colors[i]) + 3 : GetBlueValue(Palette->Colors[i]) - 3);
			}
}

C4TextureMap TextureMap;
