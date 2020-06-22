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

/* Handles landscape and sky */

#ifndef INC_C4Landscape
#define INC_C4Landscape

#include "config/C4Constants.h"

const int32_t C4MaxMaterial = 125;

const int32_t C4LS_MaxRelights = 50;

enum class LandscapeMode
{
	Undefined = 0,
	Dynamic = 1,
	Static = 2,
	Exact = 3
};

class C4Landscape
{
	struct P;
	std::unique_ptr<P> p;

public:
	C4Landscape();
	~C4Landscape();

	// Use this with the various drawing functions to keep current material for
	// either foreground or background map. We can use C4M_MaxTexIndex as a value
	// here because this value is reserved anyway for the differential landscape
	// encoding.
	static const uint8_t Transparent = C4M_MaxTexIndex;

	void Default();
	void Clear(bool fClearMapCreator=true, bool fClearSky=true, bool fClearRenderer=true);
	void Execute();
	void Synchronize();
	void Draw(C4TargetFacet &cgo, class C4FoWRegion *pLight = nullptr);
	void ScenarioInit();

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
	bool HasMap() const;
	bool MapToLandscape();
	bool ApplyDiff(C4Group &hGroup);
	
	void SetMode(LandscapeMode iMode);
	LandscapeMode GetMode() const;
	
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
	CStdPalette *GetPal() const;
	
	int32_t GetWidth() const;
	int32_t GetHeight() const;
	int32_t GetMapZoom() const;
	C4Real GetGravity() const;
	void SetGravity(C4Real g);

	BYTE _GetPix(int32_t x, int32_t y) const; // get landscape pixel (bounds not checked)
	BYTE GetPix(int32_t x, int32_t y) const;
	
	int32_t _GetMat(int32_t x, int32_t y) const;
	int32_t _GetDensity(int32_t x, int32_t y) const;
	int32_t _GetPlacement(int32_t x, int32_t y) const;
	int32_t GetMat(int32_t x, int32_t y) const;
	int32_t GetDensity(int32_t x, int32_t y) const;
	int32_t GetPlacement(int32_t x, int32_t y) const;

	BYTE _GetBackPix(int32_t x, int32_t y) const;
	BYTE GetBackPix(int32_t x, int32_t y) const;
	int32_t _GetBackMat(int32_t x, int32_t y) const;
	int32_t _GetBackDensity(int32_t x, int32_t y) const;
	int32_t _GetBackPlacement(int32_t x, int32_t y) const;
	int32_t GetBackMat(int32_t x, int32_t y) const;
	int32_t GetBackDensity(int32_t x, int32_t y) const;
	int32_t GetBackPlacement(int32_t x, int32_t y) const;

	bool GetLight(int32_t x, int32_t y);
	bool _GetLight(int32_t x, int32_t y);

	bool _FastSolidCheck(int32_t x, int32_t y) const;
	static int32_t FastSolidCheckNextX(int32_t x);
	int32_t GetPixMat(BYTE byPix) const;
	int32_t GetPixDensity(BYTE byPix) const;
	bool _PathFree(int32_t x, int32_t y, int32_t x2, int32_t y2) const; // quickly checks wether there *might* be pixel in the path.
	int32_t GetMatHeight(int32_t x, int32_t y, int32_t iYDir, int32_t iMat, int32_t iMax) const;

	int32_t AreaSolidCount(int32_t x, int32_t y, int32_t wdt, int32_t hgt) const;
	int32_t ExtractMaterial(int32_t fx, int32_t fy, bool distant_first);
	bool DrawMap(int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, const char *szMapDef, bool ignoreSky = false); // creates and draws a map section using MapCreatorS2
	bool ClipRect(int32_t &rX, int32_t &rY, int32_t &rWdt, int32_t &rHgt) const; // clip given rect by landscape size; return whether anything is left unclipped
	bool DrawDefMap(int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, const char *szMapDef, bool ignoreSky = false); // creates and draws a map section using MapCreatorS2 and a map from the loaded Landscape.txt
	bool SetModulation(DWORD dwWithClr);
	DWORD GetModulation() const;
	bool PostInitMap();   // do script callbacks of MapCreatorS2 in finished landscape
	bool ReplaceMapColor(BYTE iOldIndex, BYTE iNewIndex); // find every occurance of iOldIndex in map; replace it by new index
	bool SetTextureIndex(const char *szMatTex, BYTE iNewIndex, bool fInsert); // change color index of map texture, or insert a new one
	void SetMapChanged();
	void HandleTexMapUpdate();
	void UpdatePixMaps();
	bool DoRelights();
	void RemoveUnusedTexMapEntries();

	class C4Sky &GetSky();

	bool HasFoW() const;
	class C4FoW *GetFoW();

	int32_t GetMatCount(int material) const;
	int32_t GetEffectiveMatCount(int material) const;

	int32_t DigFreeShape(int *vtcs, int length, C4Object *by_object = nullptr, bool no_dig2objects = false, bool no_instability_check = false);
	void BlastFreeShape(int *vtcs, int length, C4Object *by_object = nullptr, int32_t by_player = NO_OWNER, int32_t iMaxDensity = C4M_Vehicle);

	void ClearFreeRect(int32_t tx, int32_t ty, int32_t wdt, int32_t hgt);
	int32_t DigFreeRect(int32_t tx, int32_t ty, int32_t wdt, int32_t hgt, C4Object *by_object = nullptr, bool no_dig2objects = false, bool no_instability_check = false);
	int32_t DigFree(int32_t tx, int32_t ty, int32_t rad, C4Object *by_object = nullptr, bool no_dig2objects = false, bool no_instability_check = false);
	void ShakeFree(int32_t tx, int32_t ty, int32_t rad);
	void BlastFree(int32_t tx, int32_t ty, int32_t rad, int32_t caused_by = NO_OWNER, C4Object *by_object = nullptr, int32_t iMaxDensity = C4M_Vehicle);

	void CheckInstabilityRange(int32_t tx, int32_t ty);
	bool CheckInstability(int32_t tx, int32_t ty, int32_t recursion_count=0);

	bool ClearPix(int32_t tx, int32_t ty);	// also used by mass mover (corrode)

	void ClearPointers(C4Object *pObj);

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
bool FindConSiteSpot(int32_t &rx, int32_t &ry, int32_t wdt, int32_t hgt, int32_t hrange=-1);
bool FindThrowingPosition(int32_t iTx, int32_t iTy, C4Real fXDir, C4Real fYDir, int32_t iHeight, int32_t &rX, int32_t &rY);
bool PathFree(int32_t x1, int32_t y1, int32_t x2, int32_t y2);
bool PathFree(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t *ix, int32_t *iy);
bool PathFreeIgnoreVehicle(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t *ix=nullptr, int32_t *iy=nullptr);
bool FindClosestFree(int32_t &rX, int32_t &rY, int32_t iAngle1, int32_t iAngle2, int32_t iExcludeAngle1, int32_t iExcludeAngle2);
bool ConstructionCheck(C4PropList *, int32_t iX, int32_t iY, C4Object *pByObj=nullptr);
int32_t PixCol2Mat(BYTE pixc);

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
