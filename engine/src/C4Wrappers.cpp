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

/* Some useful wrappers to globals */

#include <C4Include.h>
#include <C4Wrappers.h>

#ifndef BIG_C4INCLUDE
#include <C4Random.h>
#include <C4Object.h>
#endif

//==================================== Materials =========================================

int32_t PixCol2MatOld(BYTE pixc)
  {
  if (pixc < GBM) return MNone;
  pixc &= 63; // Substract GBM, ignore IFT
  if (pixc > Game.Material.Num*C4M_ColsPerMat-1) return MNone;
  return pixc / C4M_ColsPerMat;
  }

int32_t PixCol2MatOld2(BYTE pixc)
  {
	int32_t iMat = ((int32_t) (pixc&0x7f)) -1;
	// if above MVehic, don't forget additional vehicle-colors
	if (iMat<=MVehic) return iMat;
	// equals middle vehicle-color
	if (iMat==MVehic+1) return MVehic;
	// above: range check
	iMat-=2; if (iMat >= Game.Material.Num) return MNone;
	return iMat;
  }

//=============================== Sound ==================================================

C4SoundInstance *StartSoundEffect(const char *szSndName, bool fLoop, int32_t iVolume, C4Object *pObj, int32_t iCustomFalloffDistance)
  {
  // Sound check
  if (!Config.Sound.RXSound) return FALSE;
  // Start new
  return Application.SoundSystem.NewEffect(szSndName, fLoop, iVolume, pObj, iCustomFalloffDistance);
  }

C4SoundInstance *StartSoundEffectAt(const char *szSndName, int32_t iX, int32_t iY, bool fLoop, int32_t iVolume)
  {
  // Sound check
  if (!Config.Sound.RXSound) return FALSE;
  // Create
  C4SoundInstance *pInst = StartSoundEffect(szSndName, fLoop, iVolume);
  // Set volume by position
  if(pInst) pInst->SetVolumeByPos(iX, iY);
  // Return
  return pInst;
  }

C4SoundInstance *GetSoundInstance(const char *szSndName, C4Object *pObj)
  {
  return Application.SoundSystem.FindInstance(szSndName, pObj);
  }

void StopSoundEffect(const char *szSndName, C4Object *pObj)
  {
  // Find instance
  C4SoundInstance *pInst = Application.SoundSystem.FindInstance(szSndName, pObj);
  if(!pInst) return;
  // Stop
  pInst->Stop();
  }
void SoundLevel(const char *szSndName, C4Object *pObj, int32_t iLevel)
  {
  // Sound level zero? Stop
  if(!iLevel) { StopSoundEffect(szSndName, pObj); return; }
  // Find or create instance
  C4SoundInstance *pInst = Application.SoundSystem.FindInstance(szSndName, pObj);
  if(!pInst) pInst = StartSoundEffect(szSndName, true, iLevel, pObj);
  if(!pInst) return;
  // Set volume
  pInst->SetVolume(iLevel);
  pInst->Execute();
  }
void SoundPan(const char *szSndName, C4Object *pObj, int32_t iPan)
  {
  // Find instance
  C4SoundInstance *pInst = Application.SoundSystem.FindInstance(szSndName, pObj);
  if(!pInst) return;
  // Set pan
  pInst->SetPan(iPan);
  pInst->Execute();
  }

//=========================== Graphics Resource =========================================

#define GfxR (&(Game.GraphicsResource))

//=============================== Messages =============================================

void GameMsgObject(const char *szText, C4Object *pTarget, int32_t iFCol)
	{
	Game.Messages.New(C4GM_Target,szText,pTarget,NO_OWNER,0,0,(uint8_t) iFCol);
	}

void GameMsgObjectPlayer(const char *szText, C4Object *pTarget, int32_t iPlayer, int32_t iFCol)
	{
	Game.Messages.New(C4GM_TargetPlayer,szText,pTarget,iPlayer,0,0,(uint8_t) iFCol);
	}

void GameMsgGlobal(const char *szText, int32_t iFCol)
	{
	Game.Messages.New(C4GM_Global,szText,NULL,ANY_OWNER,0,0,(uint8_t) iFCol);
	}

void GameMsgPlayer(const char *szText, int32_t iPlayer, int32_t iFCol)
	{
	Game.Messages.New(C4GM_GlobalPlayer,szText,NULL,iPlayer,0,0,(uint8_t) iFCol);
	}

void GameMsgObjectDw(const char *szText, C4Object *pTarget, uint32_t dwClr)
	{
	Game.Messages.New(C4GM_Target,szText,pTarget,NO_OWNER,0,0,dwClr);
	}

//================================== Players ============================================

int32_t ValidPlr(int32_t plr)
	{
	return Game.Players.Valid(plr);
	}

int32_t Hostile(int32_t plr1, int32_t plr2)
	{
	return Game.Players.Hostile(plr1,plr2);
	}

//================================== StdCompiler ============================================

void StdCompilerWarnCallback(void *pData, const char *szPosition, const char *szError)
	{
	const char *szName = reinterpret_cast<const char *>(pData);
	if(!szPosition || !*szPosition)
		DebugLogF("WARNING: %s (in %s)", szError, szName);
	else
		DebugLogF("WARNING: %s (in %s, %s)", szError, szPosition, szName);
	}
