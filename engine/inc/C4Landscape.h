/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de
 *
 * Portions might be copyrighted by other authors who have contributed
 * to OpenClonk.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * See isc_license.txt for full license and disclaimer.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 * See clonk_trademark_license.txt for full license.
 */

/* Handles landscape and sky */

#ifndef INC_C4Landscape
#define INC_C4Landscape

#include "C4Sky.h"
#include "C4Shape.h"

#include <StdSurface8.h>

const uint8_t GBM = 128,
           GBM_ColNum = 64,
           IFT = 0x80,
					 IFTOld = GBM_ColNum;

const uint8_t CSkyDef1=104,CSkyDef2=123;

const int32_t C4MaxMaterial = 125;

const int32_t C4LSC_Undefined = 0,
          C4LSC_Dynamic = 1,
					C4LSC_Static = 2,
					C4LSC_Exact = 3;

const int32_t C4LS_MaxRelights = 50;

class C4MapCreatorS2;

class C4Landscape
  {
  public:
    C4Landscape();
		~C4Landscape();
  public:
		int32_t Mode;
		int32_t Width,Height;
		int32_t MapWidth,MapHeight,MapZoom;
		CSurface8 * Map;
    DWORD MatCount[C4MaxMaterial]; // NoSave //
    DWORD EffectiveMatCount[C4MaxMaterial]; // NoSave //
		int32_t BlastMatCount[C4MaxMaterial]; // SyncClearance-NoSave //
		bool NoScan; // ExecuteScan() disabled
    int32_t ScanX,ScanSpeed; // SyncClearance-NoSave //
		int32_t LeftOpen,RightOpen,TopOpen,BottomOpen;
		FIXED Gravity;
		uint32_t Modulation;		// landscape blit modulation; 0 means normal
		int32_t MapSeed; // random seed for MapToLandscape
    C4Sky Sky;
		C4MapCreatorS2 *pMapCreator; // map creator for script-generated maps
		bool fMapChanged;
    BYTE *pInitial; // Initial landscape after creation - used for diff
	protected:
		CSurface * Surface32;
		CSurface8 * Surface8;
		int32_t Pix2Mat[256], Pix2Dens[256], Pix2Place[256];
		int32_t PixCntPitch;
		uint8_t *PixCnt;
		C4Rect Relights[C4LS_MaxRelights];
	public:
		void Default();
	  void Clear(bool fClearMapCreator=true, bool fClearSky=true);
    void Execute();
		void Synchronize();
		void Draw(C4TargetFacet &cgo, int32_t iPlayer=-1);
    void ScenarioInit();
		void ClearRect(int32_t iTx, int32_t iTy, int32_t iWdt, int32_t iHgt);
		void ClearRectDensity(int32_t iTx, int32_t iTy, int32_t iWdt, int32_t iHgt, int32_t iOfDensity);
		void ClearMatCount();
		void ClearBlastMatCount();
    void ScanSideOpen();
		void CheckInstabilityRange(int32_t tx, int32_t ty);
		void ShakeFree(int32_t tx, int32_t ty, int32_t rad);
		void DigFree(int32_t tx, int32_t ty, int32_t rad, BOOL fRequest=FALSE, C4Object *pByObj=NULL);
		void DigFreeRect(int32_t tx, int32_t ty, int32_t wdt, int32_t hgt, BOOL fRequest=FALSE, C4Object *pByObj=NULL);
		void DigFreeMat(int32_t tx, int32_t ty, int32_t wdt, int32_t hgt, int32_t mat);
		void BlastFree(int32_t tx, int32_t ty, int32_t rad, int32_t grade, int32_t iByPlayer);
		void DrawMaterialRect(int32_t mat, int32_t tx, int32_t ty, int32_t wdt, int32_t hgt);
		void RaiseTerrain(int32_t tx, int32_t ty, int32_t wdt);
		void FindMatTop(int32_t mat, int32_t &x, int32_t &y);
		BYTE GetMapIndex(int32_t iX, int32_t iY);
		BOOL Load(C4Group &hGroup, bool fLoadSky, bool fSavegame);
		BOOL Save(C4Group &hGroup);
		BOOL SaveDiff(C4Group &hGroup, bool fSyncSave);
		BOOL SaveMap(C4Group &hGroup);
    BOOL SaveInitial();
		bool SaveTextures(C4Group &hGroup);
		BOOL Init(C4Group &hGroup, bool fOverloadCurrent, bool fLoadSky, bool &rfLoaded, bool fSavegame);
		BOOL MapToLandscape();
		BOOL ApplyDiff(C4Group &hGroup);
		BOOL SetMode(int32_t iMode);
		BOOL SetPix(int32_t x, int32_t y, BYTE npix); // set landscape pixel (bounds checked)
		BOOL SetPixDw(int32_t x, int32_t y, DWORD dwPix); // set pixel how it is visible only
		BOOL _SetPix(int32_t x, int32_t y, BYTE npix); // set landsape pixel (bounds not checked)
		BOOL _SetPixIfMask(int32_t x, int32_t y, BYTE npix, BYTE nMask) ; // set landscape pixel, if it matches nMask color (no bound-checks)
		BOOL CheckInstability(int32_t tx, int32_t ty);
		BOOL ClearPix(int32_t tx, int32_t ty);
		BOOL InsertMaterial(int32_t mat, int32_t tx, int32_t ty, int32_t vx = 0, int32_t vy = 0);
		BOOL FindMatPath(int32_t &fx, int32_t &fy, int32_t ydir, int32_t mdens, int32_t mslide);
		BOOL FindMatSlide(int32_t &fx, int32_t &fy, int32_t ydir, int32_t mdens, int32_t mslide);
		BOOL FindMatPathPush(int32_t &fx, int32_t &fy, int32_t mdens, int32_t mslide, bool liquid);
		BOOL FindMatPathPull(int32_t &fx, int32_t &fy, int32_t mdens, int32_t mslide, bool liquid);
		BOOL Incinerate(int32_t x, int32_t y);
		BOOL DrawBrush(int32_t iX, int32_t iY, int32_t iGrade, const char *szMaterial, const char *szTexture, BOOL fIFT);
		BOOL DrawLine(int32_t iX1, int32_t iY1, int32_t iX2, int32_t iY2, int32_t iGrade, const char *szMaterial, const char *szTexture, BOOL fIFT);
		BOOL DrawBox(int32_t iX1, int32_t iY1, int32_t iX2, int32_t iY2, int32_t iGrade, const char *szMaterial, const char *szTexture, BOOL fIFT);
		BOOL DrawChunks(int32_t tx, int32_t ty, int32_t wdt, int32_t hgt, int32_t icntx, int32_t icnty, const char *szMaterial, const char *szTexture, bool bIFT);
		BOOL DrawQuad(int32_t iX1, int32_t iY1, int32_t iX2, int32_t iY2, int32_t iX3, int32_t iY3, int32_t iX4, int32_t iY4, const char *szMaterial, bool bIFT);
		CStdPalette *GetPal() const { return Surface8 ? Surface8->pPal : NULL; }
		inline BYTE _GetPix(int32_t x, int32_t y) // get landscape pixel (bounds not checked)
			{
			return Surface8->_GetPix(x,y);
			}
		inline DWORD _GetPixDw(int32_t x, int32_t y, bool fApplyModulation) // get landscape pixel (bounds not checked)
			{
			return Surface32->GetPixDw(x, y, fApplyModulation);
			}
		inline BYTE GetPix(int32_t x, int32_t y) // get landscape pixel (bounds checked)
			{
			extern BYTE MCVehic;
			// Border checks
			if (x<0)
				if (y<LeftOpen) return 0;
				else return MCVehic;
			if (x>=Width)
				if (y<RightOpen) return 0;
				else return MCVehic;
			if (y<0)
				if (TopOpen) return 0;
				else return MCVehic;
			if (y>=Height)
				if (BottomOpen) return 0;
				else return MCVehic;
			return Surface8->_GetPix(x,y);
			}
		inline int32_t _GetMat(int32_t x, int32_t y) // get landscape material (bounds not checked)
			{
			return Pix2Mat[_GetPix(x, y)];
			}
		inline int32_t _GetDensity(int32_t x, int32_t y) // get landscape density (bounds not checked)
			{
			return Pix2Dens[_GetPix(x, y)];
			}
		inline int32_t _GetPlacement(int32_t x, int32_t y) // get landscape material placement (bounds not checked)
			{
			return Pix2Place[_GetPix(x, y)];
			}
		inline int32_t GetMat(int32_t x, int32_t y) // get landscape material (bounds checked)
			{
			return Pix2Mat[GetPix(x, y)];
			}
		inline int32_t GetDensity(int32_t x, int32_t y) // get landscape density (bounds checked)
			{
			return Pix2Dens[GetPix(x, y)];
			}
		inline int32_t GetPlacement(int32_t x, int32_t y) // get landscape material placement (bounds checked)
			{
			return Pix2Place[GetPix(x, y)];
			}
		inline int32_t GetPixMat(BYTE byPix) { return Pix2Mat[byPix]; }
		inline int32_t GetPixDensity(BYTE byPix) { return Pix2Dens[byPix]; }
		bool _PathFree(int32_t x, int32_t y, int32_t x2, int32_t y2); // quickly checks wether there *might* be pixel in the path.
    int32_t GetMatHeight(int32_t x, int32_t y, int32_t iYDir, int32_t iMat, int32_t iMax);
		int32_t DigFreePix(int32_t tx, int32_t ty);
		int32_t ShakeFreePix(int32_t tx, int32_t ty);
		int32_t BlastFreePix(int32_t tx, int32_t ty, int32_t grade, int32_t iBlastSize);
		int32_t AreaSolidCount(int32_t x, int32_t y, int32_t wdt, int32_t hgt);
		int32_t ExtractMaterial(int32_t fx, int32_t fy);
		BOOL DrawMap(int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, const char *szMapDef); // creates and draws a map section using MapCreatorS2
		bool ClipRect(int32_t &rX, int32_t &rY, int32_t &rWdt, int32_t &rHgt); // clip given rect by landscape size; return whether anything is left unclipped
		BOOL DrawDefMap(int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, const char *szMapDef); // creates and draws a map section using MapCreatorS2 and a map from the loaded Landscape.txt
		bool SetModulation(DWORD dwWithClr) // adjust the way the landscape is blitted
			{ Modulation=dwWithClr; return true; }
		DWORD GetModulation() { return Modulation; }
		void DiscardMap(); // discard map if present
		bool PostInitMap();   // do script callbacks of MapCreatorS2 in finished landscape
		bool ReplaceMapColor(BYTE iOldIndex, BYTE iNewIndex); // find every occurance of iOldIndex in map; replace it by new index
		BOOL SetTextureIndex(const char *szMatTex, BYTE iNewIndex, bool fInsert); // change color index of map texture, or insert a new one
		void SetMapChanged() { fMapChanged = true; }
		void HandleTexMapUpdate();
		void UpdatePixMaps();
    bool DoRelights();
    void RemoveUnusedTexMapEntries();
  protected:
    void ExecuteScan();
		int32_t DoScan(int32_t x, int32_t y, int32_t mat, int32_t dir);
		int32_t ChunkyRandom(int32_t &iOffset, int32_t iRange); // return static random value, according to offset and MapSeed
		void DrawChunk(int32_t tx, int32_t ty, int32_t wdt, int32_t hgt, int32_t mcol, int32_t iChunkType, int32_t cro);
		void DrawSmoothOChunk(int32_t tx, int32_t ty, int32_t wdt, int32_t hgt, int32_t mcol, BYTE flip, int32_t cro);
		void ChunkOZoom(CSurface8 * sfcMap, int32_t iMapX, int32_t iMapY, int32_t iMapWdt, int32_t iMapHgt, int32_t iTexture,int32_t iOffX=0,int32_t iOffY=0);
		BOOL GetTexUsage(CSurface8 * sfcMap, int32_t iMapX, int32_t iMapY, int32_t iMapWdt, int32_t iMapHgt, DWORD *dwpTextureUsage);
		BOOL TexOZoom(CSurface8 * sfcMap, int32_t iMapX, int32_t iMapY, int32_t iMapWdt, int32_t iMapHgt, DWORD *dwpTextureUsage, int32_t iToX=0,int32_t iToY=0);
		BOOL MapToSurface(CSurface8 * sfcMap, int32_t iMapX, int32_t iMapY, int32_t iMapWdt, int32_t iMapHgt, int32_t iToX, int32_t iToY, int32_t iToWdt, int32_t iToHgt, int32_t iOffX, int32_t iOffY);
		BOOL MapToLandscape(CSurface8 * sfcMap, int32_t iMapX, int32_t iMapY, int32_t iMapWdt, int32_t iMapHgt, int32_t iOffsX = 0, int32_t iOffsY = 0); // zoom map segment to surface (or sector surfaces)
	  BOOL GetMapColorIndex(const char *szMaterial, const char *szTexture, BOOL fIFT, BYTE &rbyCol);
	  BOOL SkyToLandscape(int32_t iToX, int32_t iToY, int32_t iToWdt, int32_t iToHgt, int32_t iOffX, int32_t iOffY);
		CSurface8 * CreateMap(); // create map by landscape attributes
		CSurface8 * CreateMapS2(C4Group &ScenFile); // create map by def file
		bool Relight(C4Rect To);
		bool ApplyLighting(C4Rect To);
		DWORD GetClrByTex(int32_t iX, int32_t iY);
		bool Mat2Pal(); // assign material colors to landscape palette
    void DigFreeSinglePix(int32_t x, int32_t y, int32_t dx, int32_t dy)
      {
      if(GetDensity(x, y) > GetDensity(x + dx, y + dy))
        DigFreePix(x, y);
      }
		void UpdatePixCnt(const class C4Rect &Rect, bool fCheck = false);
		void UpdateMatCnt(C4Rect Rect, bool fPlus);
		void PrepareChange(C4Rect BoundingBox);
		void FinishChange(C4Rect BoundingBox);
    static bool DrawLineLandscape(int32_t iX, int32_t iY, int32_t iGrade);
  public:
    void CompileFunc(StdCompiler *pComp); // without landscape bitmaps and sky
		bool DebugSave(const char *szFilename);
  };

/* Some global landscape functions */

BOOL AboveSolid(int32_t &rx, int32_t &ry);
BOOL AboveSemiSolid(int32_t &rx, int32_t &ry);
BOOL SemiAboveSolid(int32_t &rx, int32_t &ry);
BOOL FindSolidGround(int32_t &rx, int32_t &ry, int32_t width);
BOOL FindLiquid(int32_t &rx, int32_t &ry, int32_t width, int32_t height);
BOOL FindSurfaceLiquid(int32_t &rx, int32_t &ry, int32_t width, int32_t height);
BOOL FindLevelGround(int32_t &rx, int32_t &ry, int32_t width, int32_t hrange);
BOOL FindConSiteSpot(int32_t &rx, int32_t &ry, int32_t wdt, int32_t hgt, DWORD category, int32_t hrange=-1);
BOOL FindThrowingPosition(int32_t iTx, int32_t iTy, FIXED fXDir, FIXED fYDir, int32_t iHeight, int32_t &rX, int32_t &rY);
BOOL PathFree(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t *ix=NULL, int32_t *iy=NULL);
BOOL PathFreeIgnoreVehicle(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t *ix=NULL, int32_t *iy=NULL);
BOOL FindClosestFree(int32_t &rX, int32_t &rY, int32_t iAngle1, int32_t iAngle2, int32_t iExcludeAngle1, int32_t iExcludeAngle2);
BOOL ConstructionCheck(C4ID id, int32_t iX, int32_t iY, C4Object *pByObj=NULL);

#endif
