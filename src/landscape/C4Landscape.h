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

/* Handles landscape and sky */

#ifndef INC_C4Landscape
#define INC_C4Landscape

#include "C4Sky.h"
#include "C4Shape.h"

#include <CSurface8.h>
#include <C4Material.h>

const int32_t C4MaxMaterial = 125;

const int32_t C4LSC_Undefined = 0,
              C4LSC_Dynamic = 1,
              C4LSC_Static = 2,
              C4LSC_Exact = 3;

const int32_t C4LS_MaxRelights = 50;

class C4Landscape
{
public:
	C4Landscape();
	~C4Landscape();
public:
	int32_t Mode;
	int32_t Width,Height;
	int32_t MapWidth,MapHeight,MapZoom;
	DWORD MatCount[C4MaxMaterial]; // NoSave //
	DWORD EffectiveMatCount[C4MaxMaterial]; // NoSave //

	bool NoScan; // ExecuteScan() disabled
	int32_t ScanX,ScanSpeed; // SyncClearance-NoSave //
	int32_t LeftOpen,RightOpen,TopOpen,BottomOpen;
	C4Real Gravity;
	uint32_t Modulation;    // landscape blit modulation; 0 means normal
	int32_t MapSeed; // random seed for MapToLandscape
	C4Sky Sky;
	C4MapCreatorS2 *pMapCreator; // map creator for script-generated maps
	bool fMapChanged;
	BYTE *pInitial; // Initial landscape after creation - used for diff
	BYTE *pInitialBkg; // Initial bkg landscape after creation - used for diff
	class C4FoW *pFoW;

private:
	CSurface8 * Surface8;
	CSurface8 * Surface8Bkg; // Background material
	CSurface8 * Map;
	CSurface8 * MapBkg;
	class C4LandscapeRender *pLandscapeRender;
	uint8_t *TopRowPix, *BottomRowPix; // array size of landscape width: Filled with 0s for border pixels that are open and MCVehic for pixels that are closed
	int32_t Pix2Mat[C4M_MaxTexIndex], Pix2Dens[C4M_MaxTexIndex], Pix2Place[C4M_MaxTexIndex];
	bool Pix2Light[C4M_MaxTexIndex];
	int32_t PixCntPitch;
	uint8_t *PixCnt;
	C4Rect Relights[C4LS_MaxRelights];
	mutable uint8_t *BridgeMatConversion[C4M_MaxTexIndex]; // NoSave //

public:
	// Use this with the various drawing functions to keep current material for
	// either foreground or background map. We can use C4M_MaxTexIndex as a value
	// here because this value is reserved anyway for the differential landscape
	// encoding.
	static const uint8_t Transparent = C4M_MaxTexIndex;

	void Default();
	void Clear(bool fClearMapCreator=true, bool fClearSky=true, bool fClearRenderer=true);
	void Execute();
	void Synchronize();
	void Draw(C4TargetFacet &cgo, class C4FoWRegion *pLight = NULL);
	void ScenarioInit();

	void ClearRectDensity(int32_t iTx, int32_t iTy, int32_t iWdt, int32_t iHgt, int32_t iOfDensity);
	void ClearMatCount();
	void ScanSideOpen();

	void DrawMaterialRect(int32_t mat, int32_t tx, int32_t ty, int32_t wdt, int32_t hgt);

	void RaiseTerrain(int32_t tx, int32_t ty, int32_t wdt);
	void FindMatTop(int32_t mat, int32_t &x, int32_t &y, bool distant_first) const;
	BYTE GetMapIndex(int32_t iX, int32_t iY) const;
	BYTE GetBackMapIndex(int32_t iX, int32_t iY) const;
	bool Load(C4Group &hGroup, bool fLoadSky, bool fSavegame);
	bool Save(C4Group &hGroup) const;
	bool SaveDiff(C4Group &hGroup, bool fSyncSave) const;
	bool SaveMap(C4Group &hGroup) const;
	bool SaveInitial();
	bool SaveTextures(C4Group &hGroup) const;
	bool Init(C4Group &hGroup, bool fOverloadCurrent, bool fLoadSky, bool &rfLoaded, bool fSavegame);
	bool HasMap() const { return Map != NULL && MapBkg != NULL; }
	bool MapToLandscape();
	bool ApplyDiff(C4Group &hGroup);
	bool SetMode(int32_t iMode);
	bool SetPix2(int32_t x, int32_t y, BYTE fgPix, BYTE bgPix); // set landscape pixel (bounds checked)
	bool _SetPix2(int32_t x, int32_t y, BYTE fgPix, BYTE bgPix); // set landsape pixel (bounds not checked)
	void _SetPix2Tmp(int32_t x, int32_t y, BYTE fgPix, BYTE bgPix); // set landsape pixel (bounds not checked, no material count updates, no landscape relighting). Material must be reset to original value with this function before modifying landscape in any other way. Only used for temporary pixel changes by SolidMask (C4SolidMask::RemoveTemporary, C4SolidMask::PutTemporary).
	bool InsertMaterialOutsideLandscape(int32_t tx, int32_t ty, int32_t mdens); // return whether material insertion would be successful on an out-of-landscape position. Does not actually insert material.
	bool InsertMaterial(int32_t mat, int32_t *tx, int32_t *ty, int32_t vx = 0, int32_t vy = 0, bool query_only=false); // modifies tx/ty to actual insertion position
	bool InsertDeadMaterial(int32_t mat, int32_t tx, int32_t ty);
	bool FindMatPath(int32_t &fx, int32_t &fy, int32_t ydir, int32_t mdens, int32_t mslide) const;
	bool FindMatSlide(int32_t &fx, int32_t &fy, int32_t ydir, int32_t mdens, int32_t mslide) const;
	bool FindMatPathPush(int32_t &fx, int32_t &fy, int32_t mdens, int32_t mslide, bool liquid) const;
	bool Incinerate(int32_t x, int32_t y, int32_t cause_player);
	bool DrawBrush(int32_t iX, int32_t iY, int32_t iGrade, const char *szMaterial, const char *szTexture, const char *szBackMaterial, const char *szBackTexture);
	bool DrawLine(int32_t iX1, int32_t iY1, int32_t iX2, int32_t iY2, int32_t iGrade, const char *szMaterial, const char *szTexture, const char *szBackMaterial, const char *szBackTexture);
	bool DrawBox(int32_t iX1, int32_t iY1, int32_t iX2, int32_t iY2, int32_t iGrade, const char *szMaterial, const char *szTexture, const char *szBackMaterial, const char *szBackTexture);
	bool DrawChunks(int32_t tx, int32_t ty, int32_t wdt, int32_t hgt, int32_t icntx, int32_t icnty, const char *szMaterial, const char *szTexture, bool bIFT);
	bool DrawQuad(int32_t iX1, int32_t iY1, int32_t iX2, int32_t iY2, int32_t iX3, int32_t iY3, int32_t iX4, int32_t iY4, const char *szMaterial, const char *szBackMaterial, bool fDrawBridge);
	bool DrawPolygon(int *vtcs, int length, const char *szMaterial, const char *szBackMaterial, bool fDrawBridge);
	CStdPalette *GetPal() const { return Surface8 ? Surface8->pPal : NULL; }
	inline BYTE _GetPix(int32_t x, int32_t y) const // get landscape pixel (bounds not checked)
	{
#ifdef _DEBUG
		if (x<0 || y<0 || x>=Width || y>=Height) { BREAKPOINT_HERE; }
#endif
		return Surface8->_GetPix(x,y);
	}
	inline BYTE GetPix(int32_t x, int32_t y) const // get landscape pixel (bounds checked)
	{
		extern BYTE MCVehic;
		// Border checks
		if (x<0)
		{
			if (y<LeftOpen) return 0;
			else return MCVehic;
		}
		if (static_cast<uint32_t>(x) >= static_cast<uint32_t>(Width))
		{
			if (y<RightOpen) return 0;
			else return MCVehic;
		}
		if (y<0)
		{
			return TopRowPix[x];
		}
		if (static_cast<uint32_t>(y) >= static_cast<uint32_t>(Height))
		{
			return BottomRowPix[x];
		}
		return Surface8->_GetPix(x,y);
	}
	inline int32_t _GetMat(int32_t x, int32_t y) const // get landscape material (bounds not checked)
	{
		return Pix2Mat[_GetPix(x, y)];
	}
	inline int32_t _GetDensity(int32_t x, int32_t y) const // get landscape density (bounds not checked)
	{
		return Pix2Dens[_GetPix(x, y)];
	}
	inline int32_t _GetPlacement(int32_t x, int32_t y) const // get landscape material placement (bounds not checked)
	{
		return Pix2Place[_GetPix(x, y)];
	}
	inline int32_t GetMat(int32_t x, int32_t y) const // get landscape material (bounds checked)
	{
		return Pix2Mat[GetPix(x, y)];
	}
	inline int32_t GetDensity(int32_t x, int32_t y) const // get landscape density (bounds checked)
	{
		return Pix2Dens[GetPix(x, y)];
	}
	inline int32_t GetPlacement(int32_t x, int32_t y) const // get landscape material placement (bounds checked)
	{
		return Pix2Place[GetPix(x, y)];
	}

	inline BYTE _GetBackPix(int32_t x, int32_t y) const // get landscape pixel (bounds not checked)
	{
#ifdef _DEBUG
		if (x<0 || y<0 || x>=Width || y>=Height) { BREAKPOINT_HERE; }
#endif
		return Surface8Bkg->_GetPix(x,y);
	}
	inline BYTE GetBackPix(int32_t x, int32_t y) const // get landscape pixel (bounds checked)
	{
		// Border checks
		if (x<0)
		{
			if (y<LeftOpen) return 0;
			else return Mat2PixColDefault(MTunnel);
		}
		if (static_cast<uint32_t>(x) >= static_cast<uint32_t>(Width))
		{
			if (y<RightOpen) return 0;
			else return Mat2PixColDefault(MTunnel);
		}
		if (y<0)
		{
			return DefaultBkgMat(TopRowPix[x]);
		}
		if (static_cast<uint32_t>(y) >= static_cast<uint32_t>(Height))
		{
			return DefaultBkgMat(BottomRowPix[x]);
		}

		return Surface8Bkg->_GetPix(x,y);
	}
	inline int32_t _GetBackMat(int32_t x, int32_t y) const // get landscape material (bounds not checked)
	{
		return Pix2Mat[_GetBackPix(x, y)];
	}
	inline int32_t _GetBackDensity(int32_t x, int32_t y) const // get landscape density (bounds not checked)
	{
		return Pix2Dens[_GetBackPix(x, y)];
	}
	inline int32_t _GetBackPlacement(int32_t x, int32_t y) const // get landscape material placement (bounds not checked)
	{
		return Pix2Place[_GetBackPix(x, y)];
	}
	inline int32_t GetBackMat(int32_t x, int32_t y) const // get landscape material (bounds checked)
	{
		return Pix2Mat[GetBackPix(x, y)];
	}
	inline int32_t GetBackDensity(int32_t x, int32_t y) const // get landscape density (bounds checked)
	{
		return Pix2Dens[GetBackPix(x, y)];
	}
	inline int32_t GetBackPlacement(int32_t x, int32_t y) const // get landscape material placement (bounds checked)
	{
		return Pix2Place[GetBackPix(x, y)];
	}

	inline bool GetLight(int32_t x, int32_t y)
	{
		return GetBackPix(x, y) == 0 || Pix2Light[GetPix(x, y)];
	}
	inline bool _GetLight(int32_t x, int32_t y)
	{
		return _GetBackPix(x, y) == 0 || Pix2Light[_GetPix(x, y)];
	}

	inline bool _FastSolidCheck(int32_t x, int32_t y) const // checks whether there *might* be something solid at the point
	{
		return PixCnt[(x / 17) * PixCntPitch + (y / 15)] > 0;
	}
	static inline int32_t FastSolidCheckNextX(int32_t x)
	{
		return (x / 17) * 17 + 17;
	}
	inline int32_t GetPixMat(BYTE byPix) const { return Pix2Mat[byPix]; }
	inline int32_t GetPixDensity(BYTE byPix) const { return Pix2Dens[byPix]; }
	bool _PathFree(int32_t x, int32_t y, int32_t x2, int32_t y2) const; // quickly checks wether there *might* be pixel in the path.
	int32_t GetMatHeight(int32_t x, int32_t y, int32_t iYDir, int32_t iMat, int32_t iMax) const;

	int32_t AreaSolidCount(int32_t x, int32_t y, int32_t wdt, int32_t hgt) const;
	int32_t ExtractMaterial(int32_t fx, int32_t fy, bool distant_first);
	bool DrawMap(int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, const char *szMapDef, bool ignoreSky = false); // creates and draws a map section using MapCreatorS2
	bool ClipRect(int32_t &rX, int32_t &rY, int32_t &rWdt, int32_t &rHgt) const; // clip given rect by landscape size; return whether anything is left unclipped
	bool DrawDefMap(int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, const char *szMapDef, bool ignoreSky = false); // creates and draws a map section using MapCreatorS2 and a map from the loaded Landscape.txt
	bool SetModulation(DWORD dwWithClr) // adjust the way the landscape is blitted
	{ Modulation=dwWithClr; return true; }
	DWORD GetModulation() const { return Modulation; }
	bool PostInitMap();   // do script callbacks of MapCreatorS2 in finished landscape
	bool ReplaceMapColor(BYTE iOldIndex, BYTE iNewIndex); // find every occurance of iOldIndex in map; replace it by new index
	bool SetTextureIndex(const char *szMatTex, BYTE iNewIndex, bool fInsert); // change color index of map texture, or insert a new one
	void SetMapChanged() { fMapChanged = true; }
	void HandleTexMapUpdate();
	void UpdatePixMaps();
	bool DoRelights();
	void RemoveUnusedTexMapEntries();

private:
	void ExecuteScan();
	int32_t DoScan(int32_t x, int32_t y, int32_t mat, int32_t dir);
	uint32_t ChunkyRandom(uint32_t &iOffset, uint32_t iRange) const; // return static random value, according to offset and MapSeed
	void DrawChunk(int32_t tx, int32_t ty, int32_t wdt, int32_t hgt, uint8_t mcol, uint8_t mcolBkg, C4MaterialCoreShape Shape, uint32_t cro);
	void DrawSmoothOChunk(int32_t tx, int32_t ty, int32_t wdt, int32_t hgt, uint8_t mcol, uint8_t mcolBkg, int flip, uint32_t cro);
	void ChunkOZoom(CSurface8 * sfcMap, CSurface8* sfcMapBkg, int32_t iMapX, int32_t iMapY, int32_t iMapWdt, int32_t iMapHgt, uint8_t iTexture, int32_t iOffX=0,int32_t iOffY=0);
	bool GetTexUsage(CSurface8 * sfcMap, CSurface8* sfcMapBkg, int32_t iMapX, int32_t iMapY, int32_t iMapWdt, int32_t iMapHgt, DWORD *dwpTextureUsage) const;
	bool TexOZoom(CSurface8 * sfcMap, CSurface8* sfcMapBkg, int32_t iMapX, int32_t iMapY, int32_t iMapWdt, int32_t iMapHgt, DWORD *dwpTextureUsage, int32_t iToX=0,int32_t iToY=0);
	bool MapToSurface(CSurface8 * sfcMap, CSurface8* sfcMapBkg, int32_t iMapX, int32_t iMapY, int32_t iMapWdt, int32_t iMapHgt, int32_t iToX, int32_t iToY, int32_t iToWdt, int32_t iToHgt, int32_t iOffX, int32_t iOffY);
	bool MapToLandscape(CSurface8 * sfcMap, CSurface8* sfcMapBkg, int32_t iMapX, int32_t iMapY, int32_t iMapWdt, int32_t iMapHgt, int32_t iOffsX = 0, int32_t iOffsY = 0, bool noClear = false); // zoom map segment to surface (or sector surfaces)
	bool InitTopAndBottomRowPix(); // inti out-of-landscape pixels for bottom side
	bool GetMapColorIndex(const char *szMaterial, const char *szTexture, BYTE &rbyCol) const;
	bool SkyToLandscape(int32_t iToX, int32_t iToY, int32_t iToWdt, int32_t iToHgt, int32_t iOffX, int32_t iOffY);
	bool CreateMap(CSurface8*& sfcMap, CSurface8*& sfcMapBkg); // create map by landscape attributes
	bool CreateMapS2(C4Group &ScenFile, CSurface8*& sfcMap, CSurface8*& sfcMapBkg); // create map by def file
	bool Mat2Pal(); // assign material colors to landscape palette
	void UpdatePixCnt(const class C4Rect &Rect, bool fCheck = false);
	void UpdateMatCnt(C4Rect Rect, bool fPlus);
	void PrepareChange(C4Rect BoundingBox);
	void FinishChange(C4Rect BoundingBox);
	bool DrawLineLandscape(int32_t iX, int32_t iY, int32_t iGrade, uint8_t line_color, uint8_t line_color_bkg);
	bool DrawLineMap(int32_t iX, int32_t iY, int32_t iRadius, uint8_t line_color, uint8_t line_color_bkg);
	uint8_t *GetBridgeMatConversion(int32_t for_material_col) const;
	bool SaveInternal(C4Group &hGroup) const;
	bool SaveDiffInternal(C4Group &hGroup, bool fSyncSave) const;

	int32_t ForPolygon(int *vtcs, int length, bool (C4Landscape::*fnCallback)(int32_t, int32_t),
	                C4MaterialList *mats_count = NULL, uint8_t col = 0, uint8_t colBkg = 0, uint8_t *conversion_table = NULL);

public:
	int32_t DigFreeShape(int *vtcs, int length, C4Object *by_object = NULL, bool no_dig2objects = false, bool no_instability_check = false);
	void BlastFreeShape(int *vtcs, int length, C4Object *by_object = NULL, int32_t by_player = NO_OWNER, int32_t iMaxDensity = C4M_Vehicle);

	void ClearFreeRect(int32_t tx, int32_t ty, int32_t wdt, int32_t hgt);
	int32_t DigFreeRect(int32_t tx, int32_t ty, int32_t wdt, int32_t hgt, C4Object *by_object = NULL, bool no_dig2objects = false, bool no_instability_check = false);
	int32_t DigFree(int32_t tx, int32_t ty, int32_t rad, C4Object *by_object = NULL, bool no_dig2objects = false, bool no_instability_check = false);
	void ShakeFree(int32_t tx, int32_t ty, int32_t rad);
	void BlastFree(int32_t tx, int32_t ty, int32_t rad, int32_t caused_by = NO_OWNER, C4Object *by_object = NULL, int32_t iMaxDensity = C4M_Vehicle);

	void CheckInstabilityRange(int32_t tx, int32_t ty);
	bool CheckInstability(int32_t tx, int32_t ty, int32_t recursion_count=0);

	bool ClearPix(int32_t tx, int32_t ty);	// also used by mass mover (corrode)

private:
	CSurface8* CreateDefaultBkgSurface(CSurface8& sfcFg, bool msbAsIft) const;
	void DigMaterial2Objects(int32_t tx, int32_t ty, C4MaterialList *mat_list, C4Object *pCollect = NULL);
	void BlastMaterial2Objects(int32_t tx, int32_t ty, C4MaterialList *mat_list, int32_t caused_by, int32_t str, C4ValueArray *out_objects);

	bool DigFreePix(int32_t tx, int32_t ty);
	bool DigFreePixNoInstability(int32_t tx, int32_t ty);
	bool BlastFreePix(int32_t tx, int32_t ty);
	bool ShakeFreePix(int32_t tx, int32_t ty);

	C4ValueArray *PrepareFreeShape(C4Rect &BoundingBox, C4Object *by_object);
	void PostFreeShape(C4ValueArray *dig_objects, C4Object *by_object);
	BYTE DefaultBkgMat(BYTE fg) const;

public:
	void CompileFunc(StdCompiler *pComp); // without landscape bitmaps and sky
};

extern C4Landscape Landscape;

/* Some global landscape functions */

bool AboveSolid(int32_t &rx, int32_t &ry);
bool AboveSemiSolid(int32_t &rx, int32_t &ry);
bool SemiAboveSolid(int32_t &rx, int32_t &ry);
bool FindSolidGround(int32_t &rx, int32_t &ry, int32_t width);
bool FindLiquid(int32_t &rx, int32_t &ry, int32_t width, int32_t height);
bool FindTunnel(int32_t &rx, int32_t &ry, int32_t width, int32_t height);
bool FindSurfaceLiquid(int32_t &rx, int32_t &ry, int32_t width, int32_t height);
bool FindLevelGround(int32_t &rx, int32_t &ry, int32_t width, int32_t hrange);
bool FindConSiteSpot(int32_t &rx, int32_t &ry, int32_t wdt, int32_t hgt, int32_t Plane, int32_t hrange=-1);
bool FindThrowingPosition(int32_t iTx, int32_t iTy, C4Real fXDir, C4Real fYDir, int32_t iHeight, int32_t &rX, int32_t &rY);
bool PathFree(int32_t x1, int32_t y1, int32_t x2, int32_t y2);
bool PathFree(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t *ix, int32_t *iy);
bool PathFreeIgnoreVehicle(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t *ix=NULL, int32_t *iy=NULL);
bool FindClosestFree(int32_t &rX, int32_t &rY, int32_t iAngle1, int32_t iAngle2, int32_t iExcludeAngle1, int32_t iExcludeAngle2);
bool ConstructionCheck(C4PropList *, int32_t iX, int32_t iY, C4Object *pByObj=NULL);
int32_t PixCol2Mat(BYTE pixc);

#define GBackWdt ::Landscape.Width
#define GBackHgt ::Landscape.Height
#define GBackPix ::Landscape.GetPix
#define ClearBackPix ::Landscape.ClearPix
#define _GBackPix ::Landscape._GetPix

inline bool DensitySolid(int32_t dens)
{
	return (dens>=C4M_Solid);
}

inline bool DensitySemiSolid(int32_t dens)
{
	return (dens>=C4M_SemiSolid);
}

inline bool DensityLiquid(int32_t dens)
{
	return ((dens>=C4M_Liquid) && (dens<C4M_Solid));
}

inline int32_t PixCol2Tex(BYTE pixc)
{
	// Validate
	if (pixc >= C4M_MaxTexIndex) return 0;
	// Done
	return pixc;
}

inline int32_t GBackMat(int32_t x, int32_t y)
{
	return ::Landscape.GetMat(x, y);
}

inline int32_t GBackDensity(int32_t x, int32_t y)
{
	return ::Landscape.GetDensity(x, y);
}

inline bool GBackSolid(int32_t x, int32_t y)
{
	return DensitySolid(GBackDensity(x,y));
}

inline bool GBackSemiSolid(int32_t x, int32_t y)
{
	return DensitySemiSolid(GBackDensity(x,y));
}

inline bool GBackLiquid(int32_t x, int32_t y)
{
	return DensityLiquid(GBackDensity(x,y));
}
#endif
