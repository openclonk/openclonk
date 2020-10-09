/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2013-2016, The OpenClonk Team and contributors
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

#ifndef INC_C4MapScript
#define INC_C4MapScript

#include "c4group/C4ComponentHost.h"
#include "graphics/CSurface8.h"
#include "landscape/C4Landscape.h"
#include "lib/C4Rect.h"
#include "script/C4Aul.h"
#include "script/C4ScriptHost.h"

bool FnParTexCol(C4String *mattex, uint8_t& fg, uint8_t& bg);

// mattex masks: Array of bools for each possible material-texture index
class C4MapScriptMatTexMask
{
	std::vector<bool> sky_mask; // vector of size C4M_MaxTexIndex + 1: true means pixel color is let through; false means it is blocked
	std::vector<bool> tunnel_mask; // vector of size C4M_MaxTexIndex + 1: true means pixel color is let through; false means it is blocked

	// sky_mask stores the mask for pixels with sky or transparent background, and tunnel_mask
	// stores the mask for pixels with non-sky background. We don't allow
	// different masks for different background materials since this would
	// need a C4M_MaxTexIndex + 1 x C4M_MaxTexIndex + 1 matrix to account
	// for each possible combination of foreground and background pixel.

	void UnmaskSpec(C4String *spec);
public:
	C4MapScriptMatTexMask() : sky_mask(C4M_MaxTexIndex + 1,false), tunnel_mask(C4M_MaxTexIndex + 1, false) { }
	C4MapScriptMatTexMask(const C4Value &spec) : sky_mask(C4M_MaxTexIndex + 1, false), tunnel_mask(C4M_MaxTexIndex + 1, false) { Init(spec); }
	void Init(const C4Value &spec);

	bool operator()(uint8_t fg, uint8_t bg) const { if (bg == C4M_MaxTexIndex) return sky_mask[fg]; else return tunnel_mask[fg]; }
};

// algorithms may be either indicator functions (int,int)->bool that tell whether a map pixel should be
// set (to be used in C4MapScriptLayer::Fill) or functions (int,int)->int that tell which material should
// be set (to be used in Fill or C4MapScriptLayer::Blit).
class C4MapScriptAlgo
{
protected:
	bool GetXYProps(const C4PropList *props, C4PropertyName k, int32_t *out_xy, bool zero_defaults);
public:
	virtual bool operator () (int32_t x, int32_t y, uint8_t& fg, uint8_t& bg) const = 0;
	virtual ~C4MapScriptAlgo() = default;
};

// List of possible algorithms. Also registered as script constants in void C4MapScriptHost::InitFunctionMap.
enum C4MapScriptAlgoType
{
	MAPALGO_None       = 0,

	MAPALGO_Layer      = 1,

	MAPALGO_RndChecker = 10,

	MAPALGO_Rect       = 20,
	MAPALGO_Ellipsis   = 21, // This is "MAPALGO_Ellipse", but misspelled. Use will result in a warning.
	MAPALGO_Polygon    = 22,
	MAPALGO_Lines      = 23,
	MAPALGO_Ellipse    = 24,

	MAPALGO_And        = 30,
	MAPALGO_Or         = 31,
	MAPALGO_Not        = 32,
	MAPALGO_Xor        = 33,

	MAPALGO_Offset     = 40,
	MAPALGO_Scale      = 41,
	MAPALGO_Rotate     = 42,
	MAPALGO_Turbulence = 43,

	MAPALGO_Border     = 50,
	MAPALGO_Filter     = 51,
};

// MAPALGO_Layer: Just query pixel in layer. Pixels outside the layer range are zero.
class C4MapScriptAlgoLayer : public C4MapScriptAlgo
{
	const class C4MapScriptLayer *layer;
public:
	C4MapScriptAlgoLayer(const C4MapScriptLayer *layer) : layer(layer) {}
	C4MapScriptAlgoLayer(const C4PropList *props);

	bool operator () (int32_t x, int32_t y, uint8_t& fg, uint8_t& bg) const override;
};

// MAPALGO_RndChecker: checkerboard on which areas are randomly set or unset
class C4MapScriptAlgoRndChecker : public C4MapScriptAlgo
{
	int32_t seed, set_percentage, checker_wdt, checker_hgt;
	bool is_fixed_offset;
public:
	C4MapScriptAlgoRndChecker(const C4PropList *props);

	bool operator () (int32_t x, int32_t y, uint8_t& fg, uint8_t& bg) const override;
};

// MAPALGO_Rect: 1 for pixels contained in rect, 0 otherwise
class C4MapScriptAlgoRect : public C4MapScriptAlgo
{
	C4Rect rect;
public:
	C4MapScriptAlgoRect(const C4PropList *props);

	bool operator () (int32_t x, int32_t y, uint8_t& fg, uint8_t& bg) const override;
};

// MAPALGO_Ellipse: 1 for pixels within ellipse, 0 otherwise
class C4MapScriptAlgoEllipse : public C4MapScriptAlgo
{
	int32_t cx,cy;
	int32_t wdt,hgt;
public:
	C4MapScriptAlgoEllipse(const C4PropList *props);

	bool operator () (int32_t x, int32_t y, uint8_t& fg, uint8_t& bg) const override;
};

// MAPALGO_Polygon: 1 for pixels within polygon or on border, 0 otherwise
class C4MapScriptAlgoPolygon : public C4MapScriptAlgo
{
	struct Pt { int32_t x,y; };
	std::vector<Pt> poly;
	int32_t wdt;
	bool empty; // don't fill inside of polygon
	bool open; // don't draw closing segment. only valid if empty=true
public:
	C4MapScriptAlgoPolygon(const C4PropList *props);

	bool operator () (int32_t x, int32_t y, uint8_t& fg, uint8_t& bg) const override;
};

// MAPALGO_Lines: 1 for pixels on stripes in direction ("X","Y"). Stripe distance "Distance". Optional offset "OffX", "OffY"
class C4MapScriptAlgoLines : public C4MapScriptAlgo
{
	int32_t lx,ly,distance,ox,oy;
	int64_t ll,dl; // (line width)^2 and distance * line width
public:
	C4MapScriptAlgoLines(const C4PropList *props);

	bool operator () (int32_t x, int32_t y, uint8_t& fg, uint8_t& bg) const override;
};

// base class for algo that takes one or more operands
class C4MapScriptAlgoModifier : public C4MapScriptAlgo
{
protected:
	std::vector<C4MapScriptAlgo *> operands;
public:
	C4MapScriptAlgoModifier(const C4PropList *props, int32_t min_ops=0, int32_t max_ops=0);
	~C4MapScriptAlgoModifier() override { Clear(); }
	void Clear();
};

// MAPALGO_And: 0 if any of the operands is 0. Otherwise, returns value of last operand.
class C4MapScriptAlgoAnd : public C4MapScriptAlgoModifier
{
public:
	C4MapScriptAlgoAnd(const C4PropList *props) : C4MapScriptAlgoModifier(props) { }

	bool operator () (int32_t x, int32_t y, uint8_t& fg, uint8_t& bg) const override;
};

// MAPALGO_Or: First nonzero operand
class C4MapScriptAlgoOr : public C4MapScriptAlgoModifier
{
public:
	C4MapScriptAlgoOr(const C4PropList *props) : C4MapScriptAlgoModifier(props) { }

	bool operator () (int32_t x, int32_t y, uint8_t& fg, uint8_t& bg) const override;
};

// MAPALGO_Not: 1 if operand is 0, 0 otherwise.
class C4MapScriptAlgoNot : public C4MapScriptAlgoModifier
{
public:
	C4MapScriptAlgoNot(const C4PropList *props) : C4MapScriptAlgoModifier(props,1,1) { }

	bool operator () (int32_t x, int32_t y, uint8_t& fg, uint8_t& bg) const override;
};

// MAPALGO_Xor: If exactly one of the two operands is nonzero, return it. Otherwise, return zero.
class C4MapScriptAlgoXor : public C4MapScriptAlgoModifier
{
public:
	C4MapScriptAlgoXor(const C4PropList *props) : C4MapScriptAlgoModifier(props,2,2) { }

	bool operator () (int32_t x, int32_t y, uint8_t& fg, uint8_t& bg) const override;
};

// MAPALGO_Offset: Base layer shifted by ox,oy
class C4MapScriptAlgoOffset : public C4MapScriptAlgoModifier
{
	int32_t ox,oy;
public:
	C4MapScriptAlgoOffset(const C4PropList *props);

	bool operator () (int32_t x, int32_t y, uint8_t& fg, uint8_t& bg) const override;
};

// MAPALGO_Scale: Base layer scaled by sx,sy percent from fixed point cx,cy
class C4MapScriptAlgoScale : public C4MapScriptAlgoModifier
{
	int32_t sx,sy,cx,cy;
public:
	C4MapScriptAlgoScale(const C4PropList *props);

	bool operator () (int32_t x, int32_t y, uint8_t& fg, uint8_t& bg) const override;
};

// MAPALGO_Rotate: Base layer rotated by angle r around point ox,oy
class C4MapScriptAlgoRotate : public C4MapScriptAlgoModifier
{
	int32_t sr,cr,ox,oy;
	enum {Precision=1000};
public:
	C4MapScriptAlgoRotate(const C4PropList *props);

	bool operator () (int32_t x, int32_t y, uint8_t& fg, uint8_t& bg) const override;
};

// MAPALGO_Turbulence: move by a random offset iterations times
class C4MapScriptAlgoTurbulence : public C4MapScriptAlgoModifier
{
	int32_t amp[2], scale[2], seed, iterations;
public:
	C4MapScriptAlgoTurbulence(const C4PropList *props);

	bool operator () (int32_t x, int32_t y, uint8_t& fg, uint8_t& bg) const override;
};

// MAPALGO_Border: Border of operand layer
class C4MapScriptAlgoBorder : public C4MapScriptAlgoModifier
{
	int32_t left[2], top[2], right[2], bottom[2];
	void ResolveBorderProps(int32_t *p);
public:
	C4MapScriptAlgoBorder(const C4PropList *props);

	bool operator () (int32_t x, int32_t y, uint8_t& fg, uint8_t& bg) const override;
};

// MAPALGO_Filter: Original color of operand if it's marked to go through filter. 0 otherwise.
class C4MapScriptAlgoFilter : public C4MapScriptAlgoModifier
{
	C4MapScriptMatTexMask filter;
public:
	C4MapScriptAlgoFilter(const C4PropList *props);

	bool operator () (int32_t x, int32_t y, uint8_t& fg, uint8_t& bg) const override;
};

class C4MapScriptAlgoSetMaterial : public C4MapScriptAlgo {
	C4MapScriptAlgo *inner;
	uint8_t fg, bg;
public:
	C4MapScriptAlgoSetMaterial(C4MapScriptAlgo *inner, int fg, int bg);
	~C4MapScriptAlgoSetMaterial() override;
	bool operator () (int32_t x, int32_t y, uint8_t& fg, uint8_t& bg) const override;
};

// layer of a script-controlled map
// C4M_MaxTexIndex can be used to mark Sky
class C4MapScriptLayer : public C4PropListNumbered
{
	std::unique_ptr<CSurface8> fg_surface;
	std::unique_ptr<CSurface8> bg_surface;
protected:
	class C4MapScriptMap *map;

public:
	C4MapScriptLayer(C4PropList *prototype, C4MapScriptMap *map);
	~C4MapScriptLayer() override { ClearSurface(); }
	C4MapScriptLayer * GetMapScriptLayer() override { return this; }
	class C4MapScriptMap *GetMap() { return map; }

	// Surface management
	bool CreateSurface(int32_t wdt, int32_t hgt);
	void ClearSurface();
	void SetSurfaces(std::unique_ptr<CSurface8> fg, std::unique_ptr<CSurface8> bg)
	{
		fg_surface = std::move(fg);
		bg_surface = std::move(bg);
		UpdateSurfaceSize();
	}
	std::pair<std::unique_ptr<CSurface8>, std::unique_ptr<CSurface8>> ReleaseSurfaces()
	{
		return std::make_pair(std::move(fg_surface), std::move(bg_surface));
	}
	bool HasSurface() const { return fg_surface && fg_surface->Bits && bg_surface && bg_surface->Bits; }

	// Size management
	void UpdateSurfaceSize(); // updates width and height properties by current surface
	C4Rect GetBounds () const;
	int32_t GetWdt() const { return fg_surface ? fg_surface->Wdt : 0; }
	int32_t GetHgt() const { return fg_surface ? fg_surface->Hgt : 0; }

	// Pixel functions
	uint8_t GetPix(int32_t x, int32_t y, uint8_t outside_col) const { return (!HasSurface()||x<0||y<0||x>=fg_surface->Wdt||y>=fg_surface->Hgt) ? outside_col : fg_surface->_GetPix(x,y); }
	uint8_t GetBackPix(int32_t x, int32_t y, uint8_t outside_col) const { return (!HasSurface()||x<0||y<0||x>=bg_surface->Wdt||y>=bg_surface->Hgt) ? outside_col : bg_surface->_GetPix(x,y); }
	bool SetPix(int32_t x, int32_t y, uint8_t fg, uint8_t bg) const { if (!HasSurface()||x<0||y<0||x>=bg_surface->Wdt||y>=bg_surface->Hgt) return false; fg_surface->_SetPix(x,y,fg); bg_surface->_SetPix(x,y,bg); return true; }
	bool IsPixMasked(int32_t x, int32_t y) const { return GetPix(x,y,0) != 0 || GetBackPix(x,y,0) != 0; } // masking: If pixel is inside surface and not transparent
	void ConvertSkyToTransparent(); // change all pixels that are C4M_MaxTexIndex to 0
	int32_t GetPixCount(const C4Rect &rcBounds, const C4MapScriptMatTexMask &mask); // return number of pixels that match mask

	// Drawing functions
	bool Fill(uint8_t fg, uint8_t bg, const C4Rect &rcBounds, const C4MapScriptAlgo *algo);
	bool Blit(const C4Rect &rcBounds, const C4MapScriptAlgo *algo);
	bool Blit(const C4MapScriptLayer *src, const C4Rect &src_rect, const C4MapScriptMatTexMask &mask, int32_t tx, int32_t ty);

	// Search functions
	bool FindPos(const C4Rect &search_rect, const C4MapScriptMatTexMask &mask, int32_t *out_x, int32_t *out_y, int32_t max_tries);
};

class C4MapScriptMap : public C4MapScriptLayer
{
	std::list<C4MapScriptLayer *> layers;
public:
	C4MapScriptMap(C4PropList *prototype) : C4MapScriptLayer(prototype, nullptr) { map=this; }
	~C4MapScriptMap() override { Clear(); }
	void Clear();
	C4MapScriptMap * GetMapScriptMap() override { return this; }

	C4MapScriptLayer *CreateLayer(int32_t wdt, int32_t hgt);
};

// Script host for scenario Map.c, parsed in static MapLayer prop list context
// Also hosts engine functions MapLayer prop list.
class C4MapScriptHost : public C4ScriptHost
{
private:
	C4PropListStaticMember *LayerPrototype{nullptr}, *MapPrototype{nullptr};

	C4MapScriptMap *CreateMap();
public:
	C4MapScriptHost();
	~C4MapScriptHost() override;
	void InitFunctionMap(C4AulScriptEngine *pEngine);
	void AddEngineFunctions() override;
	bool Load(C4Group &, const char *, const char *, C4LangStringTable *) override;
	bool LoadData(const char *, const char *, C4LangStringTable *) override;
	void Clear();
	C4PropListStatic * GetPropList() override;
	bool InitializeMap(C4SLandscape *pLandscape, C4TextureMap *pTexMap, C4MaterialMap *pMatMap, uint32_t iPlayerCount, std::unique_ptr<CSurface8> *pmap_fg_surface, std::unique_ptr<CSurface8>* pmap_bg_surface);
	C4PropListStatic *GetLayerPrototype() { return LayerPrototype; }

	C4TextureMap* pTexMap{nullptr};
	C4MaterialMap* pMatMap{nullptr};
};

extern C4MapScriptHost MapScript;

#endif
