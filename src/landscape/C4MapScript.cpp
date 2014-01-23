/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2013, The OpenClonk Team and contributors
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

/* Handles scripted map creation */

#include <C4Include.h>
#include <C4MapScript.h>
#include <C4AulDefFunc.h>
#include <C4Landscape.h>
#include <C4Texture.h>
#include <C4Random.h>

C4MapScriptAlgo *FnParAlgo(C4PropList *algo_par);

static const char *DrawFn_Transparent_Name = "Transparent";
static const char *DrawFn_Sky_Name         = "Sky";
static const char *DrawFn_Background_Name  = "Background";
static const char *DrawFn_Liquid_Name      = "Liquid";
static const char *DrawFn_Solid_Name       = "Solid";

int32_t FnParTexCol(C4String *mattex, int32_t default_col = -1)
{
	// Return index of material-texture definition for a single color
	// Defaults to underground (tunnel background) color. Prefix material with ^ to get overground (sky background) color.
	if (!mattex || !mattex->GetCStr()) return default_col;
	if (mattex->GetData() == DrawFn_Transparent_Name) return 0;
	if (mattex->GetData() == DrawFn_Sky_Name) return IFT;
	const char *cmattex = mattex->GetCStr();
	int32_t ift = IFT;
	if (*cmattex == '^') { ift=0; ++cmattex; }
	int32_t col = ::TextureMap.GetIndexMatTex(cmattex);
	return col ? col|ift : default_col;
}

void C4MapScriptMatTexMask::UnmaskSpec(C4String *spec)
{
	// Mask all indices of material-texture definitions
	// Possible definitions:
	// Material-Texture - Given material-texture combination (both sky and tunnel background)
	// Material         - All defined default textures of given material
	// *                - All materials
	// Sky              - Index IFT
	// Transparent      - Index 0
	// Background       - All tunnel materials plus sky
	// Liquid           - All liquid materials
	// Solid            - All solid materials
	// Possible modifiers:
	// ^Material        - Given material only with sky background
	// &Material        - Given material only with tunnel background
	// ~Material        - Inverse of given definition; i.e. everything except Material
	if (!spec || !spec->GetCStr()) return;
	const char *cspec = spec->GetCStr();
	bool invert=false, bgsky=false, bgtunnel=false, prefix_done=false;
	while (*cspec)
	{
		switch (*cspec)
		{
		case '~': invert=!invert; break;
		case '^': bgsky=true; break;
		case '&': bgtunnel=true; break;
		default: prefix_done=true; break;
		}
		if (prefix_done) break;
		++cspec;
	}
	std::vector<bool> mat_mask(IFT, false);
	if (SEqual(cspec, DrawFn_Transparent_Name))
	{
		// "Transparent" is zero index. Force to non-IFT
		mat_mask[0] = true;
		bgsky = true;  bgtunnel = false;
	}
	else if (SEqual(cspec, DrawFn_Sky_Name))
	{
		// Sky material: Force to IFT
		mat_mask[0] = true;
		bgsky = false;  bgtunnel = true;
	}
	else if (SEqual(cspec, DrawFn_Background_Name))
	{
		// All background materials
		for (int32_t i=0; i<IFT; ++i) if (!DensitySemiSolid(Landscape.GetPixDensity(i))) mat_mask[i] = true;
		// Background includes sky
		mat_mask[0] = true;
	}
	else if (SEqual(cspec, DrawFn_Liquid_Name))
	{
		// All liquid materials
		for (int32_t i=0; i<IFT; ++i) if (DensityLiquid(Landscape.GetPixDensity(i))) mat_mask[i] = true;
	}
	else if (SEqual(cspec, DrawFn_Solid_Name))
	{
		// All solid materials
		for (int32_t i=0; i<IFT; ++i) if (DensitySolid(Landscape.GetPixDensity(i))) mat_mask[i] = true;
	}
	else if (SEqual(cspec, "*"))
	{
		// All materials
		for (int32_t i=0; i<IFT; ++i) mat_mask[i] = true;
	}
	else
	{
		// Specified material
		if (SCharCount('-', cspec))
		{
			// Material+Texture
			int32_t col = ::TextureMap.GetIndexMatTex(cspec, NULL, false);
			if (col) mat_mask[col] = true;
		}
		else
		{
			// Only material: Mask all textures of this material
			int32_t mat = ::MaterialMap.Get(cspec);
			if (mat!=MNone)
			{
				const char *tex_name;
				int32_t col;
				for (int32_t itex=0; (tex_name=::TextureMap.GetTexture(itex)); itex++)
					if (col = ::TextureMap.GetIndex(cspec,tex_name,false))
						mat_mask[col] = true;
			}
		}
	}
	// 'OR' spec onto this->mask. Apply bgsky, bgtunnel and invert.
	for (int32_t i=0; i<IFT; ++i)
		if ((mat_mask[i] && (bgsky || !bgtunnel)) != invert)
			mask[i] = true;
	for (int32_t i=0; i<IFT; ++i)
		if ((mat_mask[i] && (!bgsky || bgtunnel)) != invert)
			mask[i+IFT] = true;
}

void C4MapScriptMatTexMask::Init(const C4Value &spec)
{
	// Mask may be initialized by a simple string or by an array of strings, of which the effects are OR'ed
	const C4ValueArray *arr = spec.getArray();
	if (arr)
	{
		// Init by array
		for (int32_t i=0; i<arr->GetSize(); ++i)
		{
			C4String *smask = arr->GetItem(i).getStr();
			if (!smask) throw new C4AulExecError(FormatString("MatTexMask expected string as %dth element in array.", (int)i).getData());
			UnmaskSpec(smask);
		}
	}
	else
	{
		// Init by string
		C4String *smask = spec.getStr();
		if (smask)
			UnmaskSpec(smask);
		else
		{
			if (spec) throw new C4AulExecError("MatTexMask expected string or array of strings.");
			// nil defaults to everything except index zero unmasked
			mask = std::vector<bool>(256, true);
			mask[0] = false;
		}
	}
}


bool FnParRect(C4MapScriptLayer *layer, C4ValueArray *rect, C4Rect *rc_bounds)
{
	// Convert rect parameter passed to script function to C4Rect structure
	// and makes sure it is completely contained in bounding rectangle of layer
	// rect==NULL defaults to bounding rectangle of layer
	*rc_bounds = layer->GetBounds();
	if (!rect) return true; // nil is OK for rect parameter. Defaults to bounds rectangle
	if (rect->GetSize() != 4) return false;
	rc_bounds->Intersect(C4Rect(rect->GetItem(0).getInt(), rect->GetItem(1).getInt(), rect->GetItem(2).getInt(), rect->GetItem(3).getInt()));
	return true;
}

static bool FnLayerDraw(C4PropList * _this, C4String *mattex, C4PropList *mask_algo, C4ValueArray *rect)
{
	// Layer script function: Draw material mattex in shape of mask_algo in _this layer within bounds given by rect
	C4MapScriptLayer *layer = _this->GetMapScriptLayer();
	int32_t icol = FnParTexCol(mattex);
	if (!layer || icol<0) return false;
	C4Rect rcBounds;
	if (!FnParRect(layer, rect, &rcBounds)) return false;
	std::unique_ptr<C4MapScriptAlgo> algo(FnParAlgo(mask_algo));
	return layer->Fill(icol, rcBounds, algo.get());
}

static bool FnLayerBlit(C4PropList * _this, C4PropList *mask_algo, C4ValueArray *rect)
{
	// Layer script function: Blit mask_algo onto surface of _this  within bounds given by rect
	C4MapScriptLayer *layer = _this->GetMapScriptLayer();
	if (!layer) return false;
	C4Rect rcBounds;
	if (!FnParRect(layer, rect, &rcBounds)) return false;
	std::unique_ptr<C4MapScriptAlgo> algo(FnParAlgo(mask_algo));
	if (!algo.get()) return false;
	return layer->Blit(rcBounds, algo.get());
}

static C4PropList *FnCreateLayer(C4PropList * _this, C4String *mattex_fill, int32_t width, int32_t height)
{
	// Layer script function: Create new layer filled by mattex_fill of size width,height as sub layer of _this map
	// Size defaults to _this layer size
	int32_t icol = FnParTexCol(mattex_fill, 0);
	if (icol<0) throw new C4AulExecError(FormatString("CreateLayer: Invalid fill material.").getData());
	C4MapScriptLayer *layer = _this->GetMapScriptLayer();
	if (!layer) return NULL;
	if (!width && !height)
	{
		width = layer->GetWdt();
		height = layer->GetHgt();
	}
	if (width<=0 || height<=0) throw new C4AulExecError(FormatString("CreateLayer: Invalid size (%d*%d).", (int)width, (int)height).getData());
	C4MapScriptMap *map = layer->GetMap();
	if (!map) return NULL;
	layer = map->CreateLayer(width, height);
	if (icol) layer->Fill(icol, layer->GetBounds(), NULL);
	return layer;
}

static C4PropList *FnLayerDuplicate(C4PropList * _this, const C4Value &mask_spec, C4ValueArray *rect)
{
	// Layer script function: Create a copy of _this layer within bounds. If mask_spec is specified, copy only materials selected by mask spec
	C4MapScriptLayer *layer = _this->GetMapScriptLayer();
	if (!layer) return NULL;
	C4MapScriptMap *map = layer->GetMap();
	if (!map) return NULL;
	C4MapScriptMatTexMask mat_mask(mask_spec);
	C4Rect src_rect;
	if (!FnParRect(layer, rect, &src_rect)) return NULL;
	if (!src_rect.Wdt || !src_rect.Hgt) return NULL;
	C4MapScriptLayer *new_layer = map->CreateLayer(src_rect.Wdt, src_rect.Hgt);
	new_layer->Blit(layer, src_rect, mat_mask, 0,0);
	return new_layer;
}

static int32_t FnLayerGetPixel(C4PropList * _this, int32_t x, int32_t y)
{
	// Layer script function: Query pixel at position x,y from _this layer
	C4MapScriptLayer *layer = _this->GetMapScriptLayer();
	if (!layer) return 0;
	return layer->GetPix(x,y,0);
}

static bool FnLayerSetPixel(C4PropList * _this, int32_t x, int32_t y, const C4Value &to_value_c4v)
{
	// Layer script function: Set pixel at position x,y to to_value in _this layer
	C4MapScriptLayer *layer = _this->GetMapScriptLayer();
	if (!layer) return false;
	int32_t to_value; C4String *to_value_s;
	if (to_value_s = to_value_c4v.getStr())
	{
		to_value = FnParTexCol(to_value_s);
	}
	else
	{
		to_value = to_value_c4v.getInt();
	}
	if (!Inside(to_value, 0, 255)) throw new C4AulExecError("MapLayer::SetPixel: Trying to set invalid pixel value.");
	return layer->SetPix(x,y,to_value);
}

static int32_t FnLayerGetPixelCount(C4PropList * _this, const C4Value &mask_spec, C4ValueArray *rect)
{
	// Layer script function: Count all pixels within rect that match mask_spec specification
	C4MapScriptLayer *layer = _this->GetMapScriptLayer();
	if (!layer) return -1;
	C4MapScriptMatTexMask mat_mask(mask_spec);
	C4Rect check_rect;
	if (!FnParRect(layer, rect, &check_rect)) return -1;
	return layer->GetPixCount(check_rect, mask_spec);
}

static bool FnLayerResize(C4PropList * _this, int32_t new_wdt, int32_t new_hgt)
{
	// Layer script function: Recreate layer in new size. Resulting layer is empty (color 0)
	C4MapScriptLayer *layer = _this->GetMapScriptLayer();
	// safety
	if (!layer || new_wdt<=0 || new_hgt<=0) return false;
	// recreate surface in new size
	layer->ClearSurface();
	return layer->CreateSurface(new_wdt, new_hgt);
}

static bool FnLayerFindPosition(C4PropList * _this, C4PropList *out_pos, const C4Value &mask_spec, C4ValueArray *rect, int32_t max_tries)
{
	// Layer script function: Find a position (x,y) that has a color matching mask_spec. Set resulting position as X,Y properties in out_pos prop list
	C4MapScriptLayer *layer = _this->GetMapScriptLayer();
	if (!layer) return NULL;
	C4MapScriptMatTexMask mat_mask(mask_spec);
	C4Rect search_rect;
	if (!FnParRect(layer, rect, &search_rect)) return NULL;
	int32_t x,y; bool result;
	if (!max_tries) max_tries = 500;
	if (result = layer->FindPos(search_rect, mat_mask, &x, &y, max_tries))
	{
		if (out_pos && !out_pos->IsFrozen())
		{
			out_pos->SetProperty(P_X, C4VInt(x));
			out_pos->SetProperty(P_Y, C4VInt(y));
		}
	}
	return result;
}

static C4ValueArray *FnLayerCreateMatTexMask(C4PropList * _this, const C4Value &mask_spec)
{
	// layer script function: Generate an array 256 bools representing the given mask_spec
	C4MapScriptMatTexMask mat_mask(mask_spec);
	C4ValueArray *result = new C4ValueArray(256);
	for (int32_t i=0; i<256; ++i)
	{
		result->SetItem(i, C4VBool(mat_mask(uint8_t(i))));
	}
	return result;
}

C4MapScriptLayer::C4MapScriptLayer(C4PropList *prototype, C4MapScriptMap *map) : C4PropListNumbered(prototype), surface(NULL), surface_owned(false), map(map)
{
	// It seems like numbered PropLists need a number. I don't know why.
	AcquireNumber();
}

bool C4MapScriptLayer::CreateSurface(int32_t wdt, int32_t hgt)
{
	// Create new surface of given size. Surface is filled with color 0
	ClearSurface();
	if (wdt<=0 || hgt<=0) return false;
	surface = new CSurface8;
	surface_owned = true;
	if (!surface->Create(wdt, hgt))
	{
		ClearSurface();
		return false;
	}
	UpdateSurfaceSize();
	return true;
}

void C4MapScriptLayer::ClearSurface()
{
	// Delete surface if owned or just set to zero if unowned
	if (surface_owned) delete surface;
	surface=NULL; surface_owned=false;
	// if there is no surface, width and height parameters are undefined. no need to update them.
}

void C4MapScriptLayer::UpdateSurfaceSize()
{
	// Called when surface size changes: Update internal property values
	if (surface)
	{
		SetProperty(P_Wdt, C4VInt(surface->Wdt));
		SetProperty(P_Hgt, C4VInt(surface->Hgt));
	}
}

void C4MapScriptLayer::ConvertSkyToTransparent()
{
	// Convert all sky (color==IFT) pixels to transparent (color==0)
	// Needed because C4Landscape map zoom assumes sky to be 0
	if (!HasSurface()) return;
	for (int32_t y=0; y<surface->Hgt; ++y)
		for (int32_t x=0; x<surface->Wdt; ++x)
			if (surface->_GetPix(x,y) == IFT)
				surface->_SetPix(x,y, 0);
}

C4Rect C4MapScriptLayer::GetBounds() const
{
	// Return bounding rectangle of surface. Surface always starts at 0,0.
	return surface ? C4Rect(0,0,surface->Wdt,surface->Hgt) : C4Rect();
}

bool C4MapScriptLayer::Fill(int col, const C4Rect &rcBounds, const C4MapScriptAlgo *algo)
{
	// safety
	if (!HasSurface()) return false;
	assert(rcBounds.x>=0 && rcBounds.y>=0 && rcBounds.x+rcBounds.Wdt<=surface->Wdt && rcBounds.y+rcBounds.Hgt<=surface->Hgt);
	// set all non-masked pixels within bounds that fulfill algo
	for (int32_t y=rcBounds.y; y<rcBounds.y+rcBounds.Hgt; ++y)
		for (int32_t x=rcBounds.x; x<rcBounds.x+rcBounds.Wdt; ++x)
			if (!algo || (*algo)(x,y))
				surface->_SetPix(x,y,col);
	return true;
}

bool C4MapScriptLayer::Blit(const C4Rect &rcBounds, const C4MapScriptAlgo *algo)
{
	// safety
	if (!HasSurface()) return false;
	assert(rcBounds.x>=0 && rcBounds.y>=0 && rcBounds.x+rcBounds.Wdt<=surface->Wdt && rcBounds.y+rcBounds.Hgt<=surface->Hgt);
	assert(algo);
	// set all pixels within bounds by algo, if algo is not transparent
	uint8_t col;
	for (int32_t y=rcBounds.y; y<rcBounds.y+rcBounds.Hgt; ++y)
		for (int32_t x=rcBounds.x; x<rcBounds.x+rcBounds.Wdt; ++x)
			if (col=(*algo)(x,y))
				surface->_SetPix(x,y,col);
	return true;
}

bool C4MapScriptLayer::Blit(const C4MapScriptLayer *src, const C4Rect &src_rect, const C4MapScriptMatTexMask &col_mask, int32_t tx, int32_t ty)
{
	// safety
	assert(src);
	if (!HasSurface() || !src->HasSurface()) return false;
	// cannot assert this, because C4Rect::Contains(C4Rect &) has an off-by-one-error which I don't dare to fix right now
	// TODO: Fix C4Rect::Contains and check if the sector code still works
	// assert(src->GetBounds().Contains(src_rect));
	// copy all pixels that aren't masked
	uint8_t col;
	for (int32_t y=src_rect.y; y<src_rect.y+src_rect.Hgt; ++y)
		for (int32_t x=src_rect.x; x<src_rect.x+src_rect.Wdt; ++x)
			if (col_mask(col=src->surface->_GetPix(x,y)))
				surface->_SetPix(x-src_rect.x+tx,y-src_rect.y+ty,col);
	return true;
}

int32_t C4MapScriptLayer::GetPixCount(const C4Rect &rcBounds, const C4MapScriptMatTexMask &col_mask)
{
	// safety
	if (!HasSurface()) return 0;
	assert(rcBounds.x>=0 && rcBounds.y>=0 && rcBounds.x+rcBounds.Wdt<=surface->Wdt && rcBounds.y+rcBounds.Hgt<=surface->Hgt);
	// count matching pixels in rect
	int32_t count = 0;
	for (int32_t y=rcBounds.y; y<rcBounds.y+rcBounds.Hgt; ++y)
		for (int32_t x=rcBounds.x; x<rcBounds.x+rcBounds.Wdt; ++x)
			count += col_mask(surface->_GetPix(x,y));
	return count;
}

bool C4MapScriptLayer::FindPos(const C4Rect &search_rect, const C4MapScriptMatTexMask &col_mask, int32_t *out_x, int32_t *out_y, int32_t max_tries)
{
	// safety
	if (!HasSurface() || search_rect.Wdt<=0 || search_rect.Hgt<=0) return false;
	// Search random positions
	for (int32_t i=0; i<max_tries; ++i)
	{
		int32_t x=search_rect.x + Random(search_rect.Wdt);
		int32_t y=search_rect.y + Random(search_rect.Hgt);
		if (col_mask(surface->_GetPix(x,y))) { *out_x=x; *out_y=y; return true; }
	}
	// Nothing found yet: Start at a random position and search systemically
	// (this guantuess to find a pixel if there is one, but favours border pixels)
	int32_t sx=search_rect.x + Random(search_rect.Wdt);
	int32_t sy=search_rect.y + Random(search_rect.Hgt);
	for (int32_t x=sx; x<surface->Wdt; ++x)
		if (col_mask(surface->_GetPix(x,sy))) { *out_x=x; *out_y=sy; return true; }
	for (int32_t y=sy+1; y<surface->Hgt; ++y)
		for (int32_t x=0; x<surface->Wdt; ++x)
			if (col_mask(surface->_GetPix(x,y))) { *out_x=x; *out_y=y; return true; }
	for (int32_t y=0; y<sy; ++y)
		for (int32_t x=0; x<surface->Wdt; ++x)
			if (col_mask(surface->_GetPix(x,y))) { *out_x=x; *out_y=y; return true; }
	for (int32_t x=0; x<sx; ++x)
		if (col_mask(surface->_GetPix(x,sy))) { *out_x=x; *out_y=sy; return true; }
	// Nothing found
	return false;
}

void C4MapScriptMap::Clear()
{
	// Layers are owned by map. Free them.
	for (std::list<C4MapScriptLayer *>::iterator i=layers.begin(); i!=layers.end(); ++i) delete *i;
	layers.clear();
}

C4MapScriptLayer *C4MapScriptMap::CreateLayer(int32_t wdt, int32_t hgt)
{
	// Create layer and register to map. Layer's created by a map are freed when the map is freed.
	C4MapScriptLayer *new_layer = new C4MapScriptLayer(MapScript.GetLayerPrototype(), this);
	layers.push_back(new_layer); // push before CreateSurface for exception safety
	if (!new_layer->CreateSurface(wdt, hgt))
	{
		layers.remove(new_layer);
		delete new_layer;
		return NULL;
	}
	return new_layer;
}

C4MapScriptHost::C4MapScriptHost(): LayerPrototype(NULL), MapPrototype(NULL) { }

C4MapScriptHost::~C4MapScriptHost() { Clear(); }

void C4MapScriptHost::InitFunctionMap(C4AulScriptEngine *pEngine)
{
	// Register script host. Add Map and MapLayer prototypes, related constants and engine functions
	assert(pEngine && pEngine->GetPropList());
	Clear();
	LayerPrototype = new C4PropListStaticMember(NULL, NULL, ::Strings.RegString("MapLayer"));
	MapPrototype = new C4PropListStaticMember(LayerPrototype, NULL, ::Strings.RegString("Map"));
	LayerPrototype->SetName("MapLayer");
	MapPrototype->SetName("Map");
	::ScriptEngine.RegisterGlobalConstant("MapLayer", C4VPropList(LayerPrototype));
	::ScriptEngine.RegisterGlobalConstant("Map", C4VPropList(MapPrototype));
	::ScriptEngine.RegisterGlobalConstant("MAPALGO_Layer", C4VInt(MAPALGO_Layer));
	::ScriptEngine.RegisterGlobalConstant("MAPALGO_RndChecker", C4VInt(MAPALGO_RndChecker));
	::ScriptEngine.RegisterGlobalConstant("MAPALGO_And", C4VInt(MAPALGO_And));
	::ScriptEngine.RegisterGlobalConstant("MAPALGO_Or", C4VInt(MAPALGO_Or));
	::ScriptEngine.RegisterGlobalConstant("MAPALGO_Xor", C4VInt(MAPALGO_Xor));
	::ScriptEngine.RegisterGlobalConstant("MAPALGO_Not", C4VInt(MAPALGO_Not));
	::ScriptEngine.RegisterGlobalConstant("MAPALGO_Scale", C4VInt(MAPALGO_Scale));
	::ScriptEngine.RegisterGlobalConstant("MAPALGO_Rotate", C4VInt(MAPALGO_Rotate));
	::ScriptEngine.RegisterGlobalConstant("MAPALGO_Offset", C4VInt(MAPALGO_Offset));
	::ScriptEngine.RegisterGlobalConstant("MAPALGO_Rect", C4VInt(MAPALGO_Rect));
	::ScriptEngine.RegisterGlobalConstant("MAPALGO_Ellipsis", C4VInt(MAPALGO_Ellipsis));
	::ScriptEngine.RegisterGlobalConstant("MAPALGO_Polygon", C4VInt(MAPALGO_Polygon));
	::ScriptEngine.RegisterGlobalConstant("MAPALGO_Lines", C4VInt(MAPALGO_Lines));
	::ScriptEngine.RegisterGlobalConstant("MAPALGO_Turbulence", C4VInt(MAPALGO_Turbulence));
	::ScriptEngine.RegisterGlobalConstant("MAPALGO_Border", C4VInt(MAPALGO_Border));
	::ScriptEngine.RegisterGlobalConstant("MAPALGO_Filter", C4VInt(MAPALGO_Filter));
	Reg2List(pEngine);
	AddEngineFunctions();
}

void C4MapScriptHost::AddEngineFunctions()
{
	// adds all engine functions to the MapLayer context
	::AddFunc(this, "Draw", FnLayerDraw);
	::AddFunc(this, "Blit", FnLayerBlit);
	::AddFunc(this, "CreateLayer", FnCreateLayer);
	::AddFunc(this, "Duplicate", FnLayerDuplicate);
	::AddFunc(this, "GetPixel", FnLayerGetPixel);
	::AddFunc(this, "SetPixel", FnLayerSetPixel);
	::AddFunc(this, "GetPixelCount", FnLayerGetPixelCount);
	::AddFunc(this, "Resize", FnLayerResize);
	::AddFunc(this, "FindPosition", FnLayerFindPosition);
	::AddFunc(this, "CreateMatTexMask", FnLayerCreateMatTexMask);
}

bool C4MapScriptHost::Load(C4Group & g, const char * f, const char * l, C4LangStringTable * t)
{
	assert(LayerPrototype && MapPrototype);
	return C4ScriptHost::Load(g, f, l, t);
}

void C4MapScriptHost::Clear()
{
	delete LayerPrototype; delete MapPrototype;
	LayerPrototype = MapPrototype = NULL;
	C4ScriptHost::Clear();
}

C4PropListStatic * C4MapScriptHost::GetPropList()
{
	// Scripts are compiled in the MapLayer context so it's possible to use all map drawing functions directly without "map->" prefix
	return LayerPrototype;
}

C4MapScriptMap *C4MapScriptHost::CreateMap()
{
	return new C4MapScriptMap(MapPrototype);
}

bool C4MapScriptHost::InitializeMap(C4Group &group, CSurface8 **pmap_surface)
{
	// Init scripted map by calling InitializeMap in the proper scripts. If *pmap_surface is given, it will pass the existing map to be modified by script.
	assert(pmap_surface);
	// Don't bother creating surfaces if the functions aren't defined
	if (!LayerPrototype->GetFunc(PSF_InitializeMap))
	{
		C4PropList *scen_proplist = ::GameScript.ScenPropList._getPropList();
		if (!scen_proplist || !scen_proplist->GetFunc(PSF_InitializeMap)) return false;
	}
	// Create proplist as script context
	std::unique_ptr<C4MapScriptMap> map(CreateMap());
	// Drawing on existing map or create new?
	if (*pmap_surface)
	{
		// Existing map
		map->SetSurface(*pmap_surface);
	}
	else
	{
		// No existing map. Create new.
		int32_t map_wdt,map_hgt;
		::Game.C4S.Landscape.GetMapSize(map_wdt, map_hgt, ::Game.StartupPlayerCount);
		if (!map->CreateSurface(map_wdt, map_hgt)) return false;
	}
	C4AulParSet Pars(C4VPropList(map.get()));
	C4Value result = map->Call(PSF_InitializeMap, &Pars);
	if (!result) result = ::GameScript.Call(PSF_InitializeMap, &Pars);
	// Map creation done.
	if (result)
	{
		map->ConvertSkyToTransparent();
		*pmap_surface = map->ReleaseSurface();
	}
	return !!result;
}

C4MapScriptHost MapScript;