/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2007  Matthes Bender
 * Copyright (c) 2001-2002  Sven Eberhardt
 * Copyright (c) 2005, 2007  Peter Wortmann
 * Copyright (c) 2006-2007  Günther Brammer
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

/* Some useful wrappers to globals */

#ifndef INC_C4Wrappers
#define INC_C4Wrappers

#ifdef C4ENGINE

#ifndef BIG_C4INCLUDE
#include <C4Id.h>
#include <C4Game.h>
#include <C4Landscape.h>
#include <C4Log.h>
#endif

//=================================== ID2Def ==============================================

inline C4Def *C4Id2Def(C4ID id)
  {
  return Game.Defs.ID2Def(id);
  }

//=============================== Sound ==================================================

class C4SoundInstance *StartSoundEffect(const char *szSndName, bool fLoop = false, int32_t iVolume = 100, C4Object *pObj=NULL, int32_t iCustomFalloffDistance=0);
class C4SoundInstance *StartSoundEffectAt(const char *szSndName, int32_t iX, int32_t iY, bool fLoop = false, int32_t iVolume = 100);
class C4SoundInstance *GetSoundInstance(const char *szSndName, C4Object *pObj);
void StopSoundEffect(const char *szSndName, C4Object *pObj);
void SoundLevel(const char *szSndName, C4Object *pObj, int32_t iLevel);
void SoundPan(const char *szSndName, C4Object *pObj, int32_t iPan);

//=========================== Graphics Resource =========================================

#define GfxR (&(Game.GraphicsResource))


//===================================== Ticks ==========================================

#define Tick2 Game.iTick2
#define Tick3 Game.iTick3
#define Tick5 Game.iTick5
#define Tick10 Game.iTick10
#define Tick35 Game.iTick35
#define Tick255 Game.iTick255
#define Tick500 Game.iTick500
#define Tick1000 Game.iTick1000

//================================== Players ============================================

int32_t ValidPlr(int32_t plr);
int32_t Hostile(int32_t plr1, int32_t plr2);

//==================================== IFT ===============================================

inline BYTE PixColIFT(BYTE pixc)
  {
	return pixc & IFT;
  }

// always use OldGfx-version (used for convert)
inline BYTE PixColIFTOld(BYTE pixc)
  {
  if (pixc>=GBM+IFTOld) return IFTOld;
  return 0;
  }

//==================================== Density ===========================================

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

//==================================== Materials =========================================

extern int32_t MVehic,MTunnel,MWater,MSnow,MEarth,MGranite,MFlyAshes; // presearched materials
extern BYTE MCVehic; // precalculated material color

#define GBackWdt Game.Landscape.Width
#define GBackHgt Game.Landscape.Height
#define GBackPix Game.Landscape.GetPix
#define SBackPix Game.Landscape.SetPix
#define ClearBackPix Game.Landscape.ClearPix
#define _GBackPix Game.Landscape._GetPix
#define _SBackPix Game.Landscape._SetPix
#define _SBackPixIfMask Game.Landscape._SetPixIfMask

int32_t PixCol2MatOld(BYTE pixc);
int32_t PixCol2MatOld2(BYTE pixc);

inline bool MatValid(int32_t mat)
  {
  return Inside<int32_t>(mat,0,Game.Material.Num-1);
  }

inline bool MatVehicle(int32_t iMat)
	{
	return iMat == MVehic;
	}

inline int32_t PixCol2Tex(BYTE pixc)
	{
	// Remove IFT
	int32_t iTex = int32_t(pixc & (IFT - 1));
	// Validate
	if(iTex >= C4M_MaxTexIndex) return 0;
	// Done
	return iTex;
	}

inline int32_t PixCol2Mat(BYTE pixc)
  {
	// Get texture
	int32_t iTex = PixCol2Tex(pixc);
	if(!iTex) return MNone;
	// Get material-texture mapping
	const C4TexMapEntry *pTex = ::TextureMap.GetEntry(iTex);
	// Return material
	return pTex ? pTex->GetMaterialIndex() : MNone;
	}

inline BYTE MatTex2PixCol(int32_t tex)
  {
	return BYTE(tex);
  }

inline BYTE Mat2PixColDefault(int32_t mat)
	{
	return Game.Material.Map[mat].DefaultMatTex;
	}

inline int32_t MatDensity(int32_t mat)
  {
  if (!MatValid(mat)) return 0;
  return Game.Material.Map[mat].Density;
  }

inline int32_t MatPlacement(int32_t mat)
  {
  if (!MatValid(mat)) return 0;
  return Game.Material.Map[mat].Placement;
  }

inline int32_t MatDigFree(int32_t mat)
  {
  if (!MatValid(mat)) return 1;
  return Game.Material.Map[mat].DigFree;
  }

inline BYTE GBackIFT(int32_t x, int32_t y)
  {
  return PixColIFT(GBackPix(x,y));
  }

inline int32_t GBackMat(int32_t x, int32_t y)
  {
	return Game.Landscape.GetMat(x, y);
  }

inline int32_t GBackDensity(int32_t x, int32_t y)
  {
	return Game.Landscape.GetDensity(x, y);
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

inline int32_t GBackWind(int32_t x, int32_t y)
  {
  return GBackIFT(x, y) ? 0: Game.Weather.Wind;
  }

//==================================== StdCompiler =========================================

void StdCompilerWarnCallback(void *pData, const char *szPosition, const char *szError);

template <class CompT, class StructT>
	bool CompileFromBuf_Log(StructT &TargetStruct, const typename CompT::InT &SrcBuf, const char *szName)
	{
		try
		{
			CompileFromBuf<CompT>(TargetStruct, SrcBuf);
			return TRUE;
		}
		catch(StdCompiler::Exception *pExc)
		{
			LogF("ERROR: %s (in %s)", pExc->Msg.getData(), szName);
			delete pExc;
			return FALSE;
		}
	}
template <class CompT, class StructT>
	bool CompileFromBuf_LogWarn(StructT RREF TargetStruct, const typename CompT::InT &SrcBuf, const char *szName)
	{
		try
		{
			CompT Compiler;
			Compiler.setInput(SrcBuf.getRef());
			Compiler.setWarnCallback(StdCompilerWarnCallback, reinterpret_cast<void *>(const_cast<char *>(szName)));
			Compiler.Compile(TargetStruct);
			return TRUE;
		}
		catch(StdCompiler::Exception *pExc)
		{
			if(!pExc->Pos.getLength())
				LogF("ERROR: %s (in %s)", pExc->Msg.getData(), szName);
			else
				LogF("ERROR: %s (in %s, %s)", pExc->Msg.getData(), pExc->Pos.getData(), szName);
			delete pExc;
			return FALSE;
		}
	}
template <class CompT, class StructT>
	bool DecompileToBuf_Log(StructT RREF TargetStruct, typename CompT::OutT *pOut, const char *szName)
	{
		if(!pOut) return false;
		try
		{
			pOut->Take(DecompileToBuf<CompT>(TargetStruct));
			return TRUE;
		}
		catch(StdCompiler::Exception *pExc)
		{
			LogF("ERROR: %s (in %s)", pExc->Msg.getData(), szName);
			delete pExc;
			return FALSE;
		}
	}

#endif // C4ENGINE

#endif
