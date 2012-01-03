/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2004, 2008  Matthes Bender
 * Copyright (c) 2001-2010  Sven Eberhardt
 * Copyright (c) 2001  Michael Käser
 * Copyright (c) 2001-2008, 2010  Peter Wortmann
 * Copyright (c) 2004-2005, 2007-2010  Armin Burgmeier
 * Copyright (c) 2004-2011  Günther Brammer
 * Copyright (c) 2009-2010  Nicolas Hake
 * Copyright (c) 2009-2011  Tobias Zwick
 * Copyright (c) 2009-2010  Richard Gerum
 * Copyright (c) 2010  Benjamin Herr
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

/* Functions mapped by C4Script */

#include <C4Include.h>
#include <C4Script.h>

#include <C4Application.h>
#include <C4AulDefFunc.h>
#include <C4Command.h>
#include <C4DefList.h>
#include <C4Console.h>
#include <C4Game.h>
#include <C4GameObjects.h>
#include <C4GameControl.h>
#include <C4GameMessage.h>
#include <C4GraphicsSystem.h>
#include <C4Log.h>
#include <C4MessageInput.h>
#include <C4ObjectInfoList.h>
#include <C4Player.h>
#include <C4PlayerList.h>
#include <C4PXS.h>
#include <C4RoundResults.h>
#include <C4Texture.h>
#include <C4Weather.h>
#include <C4Viewport.h>


static C4Void Fn_goto(C4AulContext *cthr, long iCounter)
{
	::GameScript.Counter=iCounter;
	return C4Void();
}

static bool FnIncinerateLandscape(C4AulContext *cthr, long iX, long iY)
{
	if (cthr->Obj) { iX += cthr->Obj->GetX(); iY += cthr->Obj->GetY(); }
	return !!::Landscape.Incinerate(iX, iY);
}

static C4Void FnSetGravity(C4AulContext *cthr, long iGravity)
{
	::Landscape.Gravity = itofix(BoundBy<long>(iGravity,-300,300)) / 500;
	return C4Void();
}

static long FnGetGravity(C4AulContext *cthr)
{
	return fixtoi(::Landscape.Gravity * 500);
}

static C4Value FnPlayerObjectCommand(C4AulContext *cthr, C4Value *pPars)
{
	PAR(int, iPlr); PAR(string, szCommand); PAR(object, pTarget); PAR(int, iTx); PAR(int, iTy);
	PAR(object, pTarget2); PAR(any, Data);
	// Player
	if (!ValidPlr(iPlr) || !szCommand) return C4VFalse;
	C4Player *pPlr = ::Players.Get(iPlr);
	// Command
	long iCommand = CommandByName(FnStringPar(szCommand)); if (!iCommand) return C4VFalse;
	// Set
	pPlr->ObjectCommand(iCommand, pTarget, iTx, iTy, pTarget2, Data, C4P_Command_Set);
	// Success
	return C4VTrue;
}

static C4String *FnGetPlayerName(C4AulContext *cthr, long iPlayer)
{
	if (!ValidPlr(iPlayer)) return NULL;
	return String(::Players.Get(iPlayer)->GetName());
}

static long FnGetPlayerType(C4AulContext *cthr, long iPlayer)
{
	C4Player *pPlr = ::Players.Get(iPlayer);
	if (!pPlr) return 0;
	return pPlr->GetType();
}

static long FnGetPlayerColor(C4AulContext *cthr, long iPlayer)
{
	C4Player *plr = ::Players.Get(iPlayer);
	return plr ? plr->ColorDw : 0;
}

static Nillable<long> FnGetX(C4AulContext *cthr, long iPrec)
{
	if (!cthr->Obj) return C4Void();
	if (!iPrec) iPrec = 1;
	return fixtoi(cthr->Obj->fix_x, iPrec);
}

static Nillable<long> FnGetY(C4AulContext *cthr, long iPrec)
{
	if (!cthr->Obj) return C4Void();
	if (!iPrec) iPrec = 1;
	return fixtoi(cthr->Obj->fix_y, iPrec);
}

static C4Object *FnCreateObject(C4AulContext *cthr,
                                C4PropList * PropList, long iXOffset, long iYOffset, Nillable<long> owner)
{
	if (cthr->Obj) // Local object calls override
	{
		iXOffset+=cthr->Obj->GetX();
		iYOffset+=cthr->Obj->GetY();
	}

	long iOwner = owner;
	if (owner.IsNil())
	{
		if (cthr->Obj)
			iOwner = cthr->Obj->Controller;
		else
			iOwner = NO_OWNER;
	}

	C4Object *pNewObj = Game.CreateObject(PropList,cthr->Obj,iOwner,iXOffset,iYOffset);

	// Set initial controller to creating controller, so more complicated cause-effect-chains can be traced back to the causing player
	if (pNewObj && cthr->Obj && cthr->Obj->Controller > NO_OWNER) pNewObj->Controller = cthr->Obj->Controller;

	return pNewObj;
}

static C4Object *FnCreateConstruction(C4AulContext *cthr,
                                      C4PropList * PropList, long iXOffset, long iYOffset, Nillable<long> owner,
                                      long iCompletion, bool fTerrain, bool fCheckSite)
{
	// Make sure parameters are valid
	if (!PropList || !PropList->GetDef())
		return NULL;

	// Local object calls override position offset, owner
	if (cthr->Obj)
	{
		iXOffset+=cthr->Obj->GetX();
		iYOffset+=cthr->Obj->GetY();
	}

	// Check site
	if (fCheckSite)
		if (!ConstructionCheck(PropList,iXOffset,iYOffset,cthr->Obj))
			return NULL;

	long iOwner = owner;
	if (owner.IsNil())
	{
		if (cthr->Obj)
			iOwner = cthr->Obj->Controller;
		else
			iOwner = NO_OWNER;
	}

	// Create site object
	C4Object *pNewObj = Game.CreateObjectConstruction(PropList,cthr->Obj,iOwner,iXOffset,iYOffset,iCompletion*FullCon/100,fTerrain);

	// Set initial controller to creating controller, so more complicated cause-effect-chains can be traced back to the causing player
	if (pNewObj && cthr->Obj && cthr->Obj->Controller>NO_OWNER) pNewObj->Controller = cthr->Obj->Controller;

	return pNewObj;
}

static C4ValueArray *FnFindConstructionSite(C4AulContext *cthr, C4PropList * PropList, int32_t v1, int32_t v2)
{
	// Get def
	C4Def *pDef;
	if (!(pDef=PropList->GetDef())) return NULL;
	// Construction check at starting position
	if (ConstructionCheck(PropList,v1,v2))
		return NULL;
	// Search for real
	bool result = !!FindConSiteSpot(v1, v2,
	                                pDef->Shape.Wdt,pDef->Shape.Hgt,
	                                pDef->GetPlane(),
	                                20);
	if(!result) return 0;
	C4ValueArray *pArray = new C4ValueArray(2);
	pArray->SetItem(0, C4VInt(v1));
	pArray->SetItem(1, C4VInt(v2));
	return pArray;
}

C4FindObject *CreateCriterionsFromPars(C4Value *pPars, C4FindObject **pFOs, C4SortObject **pSOs)
{
	int i, iCnt = 0, iSortCnt = 0;
	// Read all parameters
	for (i = 0; i < C4AUL_MAX_Par; i++)
	{
		PAR(any, Data);
		// No data given?
		if (!Data) break;
		// Construct
		C4SortObject *pSO = NULL;
		C4FindObject *pFO = C4FindObject::CreateByValue(Data, pSOs ? &pSO : NULL);
		// Add FindObject
		if (pFO)
		{
			pFOs[iCnt++] = pFO;
		}
		// Add SortObject
		if (pSO)
		{
			pSOs[iSortCnt++] = pSO;
		}
	}
	// No criterions?
	if (!iCnt)
	{
		for (i = 0; i < iSortCnt; ++i) delete pSOs[i];
		return NULL;
	}
	// create sort criterion
	C4SortObject *pSO = NULL;
	if (iSortCnt)
	{
		if (iSortCnt == 1)
			pSO = pSOs[0];
		else
			pSO = new C4SortObjectMultiple(iSortCnt, pSOs, false);
	}
	// Create search object
	C4FindObject *pFO;
	if (iCnt == 1)
		pFO = pFOs[0];
	else
		pFO = new C4FindObjectAnd(iCnt, pFOs, false);
	if (pSO) pFO->SetSort(pSO);
	return pFO;
}

static C4Value FnObjectCount(C4AulContext *cthr, C4Value *pPars)
{
	// Create FindObject-structure
	C4FindObject *pFOs[C4AUL_MAX_Par];
	C4FindObject *pFO = CreateCriterionsFromPars(pPars, pFOs, NULL);
	// Error?
	if (!pFO)
		throw new C4AulExecError(cthr->Obj, "ObjectCount: No valid search criterions supplied");
	// Search
	int32_t iCnt = pFO->Count(::Objects, ::Objects.Sectors);
	// Free
	delete pFO;
	// Return
	return C4VInt(iCnt);
}

static C4Value FnFindObject(C4AulContext *cthr, C4Value *pPars)
{
	// Create FindObject-structure
	C4FindObject *pFOs[C4AUL_MAX_Par];
	C4SortObject *pSOs[C4AUL_MAX_Par];
	C4FindObject *pFO = CreateCriterionsFromPars(pPars, pFOs, pSOs);
	// Error?
	if (!pFO)
		throw new C4AulExecError(cthr->Obj, "FindObject: No valid search criterions supplied");
	// Search
	C4Object *pObj = pFO->Find(::Objects, ::Objects.Sectors);
	// Free
	delete pFO;
	// Return
	return C4VObj(pObj);
}

static C4Value FnFindObjects(C4AulContext *cthr, C4Value *pPars)
{
	// Create FindObject-structure
	C4FindObject *pFOs[C4AUL_MAX_Par];
	C4SortObject *pSOs[C4AUL_MAX_Par];
	C4FindObject *pFO = CreateCriterionsFromPars(pPars, pFOs, pSOs);
	// Error?
	if (!pFO)
		throw new C4AulExecError(cthr->Obj, "FindObjects: No valid search criterions supplied");
	// Search
	C4ValueArray *pResult = pFO->FindMany(::Objects, ::Objects.Sectors);
	// Free
	delete pFO;
	// Return
	return C4VArray(pResult);
}

static bool FnSmoke(C4AulContext *cthr, long tx, long ty, long level, long dwClr)
{
	if (cthr->Obj) { tx+=cthr->Obj->GetX(); ty+=cthr->Obj->GetY(); }
	Smoke(tx,ty,level,dwClr);
	return true;
}

static bool FnInsertMaterial(C4AulContext *cthr, long mat, long x, long y, long vx, long vy)
{
	if (cthr->Obj) { x+=cthr->Obj->GetX(); y+=cthr->Obj->GetY(); }
	return !!::Landscape.InsertMaterial(mat,x,y,vx,vy);
}

static long FnGetMaterialCount(C4AulContext *cthr, long iMaterial, bool fReal)
{
	if (!MatValid(iMaterial)) return -1;
	if (fReal || !::MaterialMap.Map[iMaterial].MinHeightCount)
		return ::Landscape.MatCount[iMaterial];
	else
		return ::Landscape.EffectiveMatCount[iMaterial];
}

static long FnGetMaterial(C4AulContext *cthr, long x, long y)
{
	if (cthr->Obj) { x+=cthr->Obj->GetX(); y+=cthr->Obj->GetY(); }
	return GBackMat(x,y);
}

static C4String *FnGetTexture(C4AulContext* cthr, long x, long y)
{
	if (cthr->Obj) { x+=cthr->Obj->GetX(); y+=cthr->Obj->GetY(); }

	// Get texture
	int32_t iTex = PixCol2Tex(GBackPix(x, y));
	if (!iTex) return NULL;
	// Get material-texture mapping
	const C4TexMapEntry *pTex = ::TextureMap.GetEntry(iTex);
	if (!pTex) return NULL;
	// Return tex name
	return String(pTex->GetTextureName());
}

// Note: Might be async in case of 16<->32 bit textures!
static Nillable<long> FnGetAverageTextureColor(C4AulContext* cthr, C4String* Texture)
{
	// Safety
	if(!Texture) return C4Void();
	// Check texture
	C4Texture* Tex = ::TextureMap.GetTexture(Texture->GetData().getData());
	if(!Tex) return C4Void();
	return Tex->GetAverageColor();
}

static bool FnGBackSolid(C4AulContext *cthr, long x, long y)
{
	if (cthr->Obj) { x+=cthr->Obj->GetX(); y+=cthr->Obj->GetY(); }
	return GBackSolid(x,y);
}

static bool FnGBackSemiSolid(C4AulContext *cthr, long x, long y)
{
	if (cthr->Obj) { x+=cthr->Obj->GetX(); y+=cthr->Obj->GetY(); }
	return GBackSemiSolid(x,y);
}

static bool FnGBackLiquid(C4AulContext *cthr, long x, long y)
{
	if (cthr->Obj) { x+=cthr->Obj->GetX(); y+=cthr->Obj->GetY(); }
	return GBackLiquid(x,y);
}

static bool FnGBackSky(C4AulContext *cthr, long x, long y)
{
	if (cthr->Obj) { x+=cthr->Obj->GetX(); y+=cthr->Obj->GetY(); }
	return !GBackIFT(x, y);
}

static long FnExtractMaterialAmount(C4AulContext *cthr, long x, long y, long mat, long amount)
{
	if (cthr->Obj) { x+=cthr->Obj->GetX(); y+=cthr->Obj->GetY(); }
	long extracted=0; for (; extracted<amount; extracted++)
	{
		if (GBackMat(x,y)!=mat) return extracted;
		if (::Landscape.ExtractMaterial(x,y)!=mat) return extracted;
	}
	return extracted;
}

static C4Void FnBlastFree(C4AulContext *cthr, long iX, long iY, long iLevel, Nillable<long> iCausedBy)
{
	if (iCausedBy.IsNil() && cthr->Obj) iCausedBy = cthr->Obj->Controller;

	::Landscape.BlastFree(iX, iY, iLevel, iCausedBy, cthr->Obj);
	return C4Void();
}

static bool FnSound(C4AulContext *cthr, C4String *szSound, bool fGlobal, Nillable<long> iLevel, Nillable<long> iAtPlayer, long iLoop, bool fMultiple, long iCustomFalloffDistance)
{
	// play here?
	if (!iAtPlayer.IsNil())
	{
		// get player to play at
		C4Player *pPlr = ::Players.Get(iAtPlayer);
		// not existant? fail
		if (!pPlr) return false;
		// network client: don't play here
		// return true for network sync
		if (!pPlr->LocalControl) return true;
	}
	// even less than nothing?
	if (iLevel<0) return true;
	// default sound level
	if (iLevel.IsNil() || iLevel>100)
		iLevel=100;
	// target object
	C4Object *pObj = NULL;
	if (!fGlobal) pObj = cthr->Obj;
	// already playing?
	if (iLoop >= 0 && !fMultiple && GetSoundInstance(FnStringPar(szSound), pObj))
		return false;
	// try to play effect
	if (iLoop >= 0)
		StartSoundEffect(FnStringPar(szSound),!!iLoop,iLevel,pObj, iCustomFalloffDistance);
	else
		StopSoundEffect(FnStringPar(szSound),pObj);
	// always return true (network safety!)
	return true;
}

static bool FnMusic(C4AulContext *cthr, C4String *szSongname, bool fLoop)
{
	bool success;
	if (!szSongname)
	{
		success = Application.MusicSystem.Stop();
	}
	else
	{
		success = Application.MusicSystem.Play(FnStringPar(szSongname), !!fLoop);
	}
	if (::Control.SyncMode()) return true;
	return success;
}

static long FnMusicLevel(C4AulContext *cthr, long iLevel)
{
	Game.SetMusicLevel(iLevel);
	return Application.MusicSystem.SetVolume(iLevel);
}

static long FnSetPlayList(C4AulContext *cth, C4String *szPlayList)
{
	long iFilesInPlayList = Application.MusicSystem.SetPlayList(FnStringPar(szPlayList));
	Game.PlayList.Copy(FnStringPar(szPlayList));
	// network/record/replay: return 0
	if (::Control.SyncMode()) return 0;
	return iFilesInPlayList;
}

static bool FnGameOver(C4AulContext *cthr, long iGameOverValue /* provided for future compatibility */)
{
	return !!Game.DoGameOver();
}

static bool FnGainMissionAccess(C4AulContext *cthr, C4String *szPassword)
{
	if (std::strlen(Config.General.MissionAccess)+std::strlen(FnStringPar(szPassword))+3>CFG_MaxString) return false;
	SAddModule(Config.General.MissionAccess,FnStringPar(szPassword));
	return true;
}

static C4Value FnPlayerMessage_C4V(C4AulContext *cthr, C4Value * iPlayer, C4Value *c4vMessage, C4Value * iPar0, C4Value * iPar1, C4Value * iPar2, C4Value * iPar3, C4Value * iPar4, C4Value * iPar5, C4Value * iPar6, C4Value * iPar7)
{
	if (!cthr->Obj) throw new NeedObjectContext("PlayerMessage");
	char buf[MaxFnStringParLen+1];
	C4String * szMessage = c4vMessage->getStr();
	if (!szMessage) return C4VBool(false);

	// Speech
	bool fSpoken=false;
	if (SCopySegment(FnStringPar(szMessage),1,buf,'$'))
		if (StartSoundEffect(buf,false,100, cthr->Obj))
			fSpoken=true;

	// Text
	if (!fSpoken)
		if (SCopySegment(FnStringFormat(cthr,FnStringPar(szMessage),iPar0,iPar1,iPar2,iPar3,iPar4,iPar5,iPar6,iPar7).getData(),0,buf,'$'))
		{
			GameMsgObjectPlayer(buf,cthr->Obj,iPlayer->getInt());
		}

	return C4VBool(true);
}

static C4Value FnMessage_C4V(C4AulContext *cthr, C4Value *c4vMessage, C4Value * iPar0, C4Value * iPar1, C4Value * iPar2, C4Value * iPar3, C4Value * iPar4, C4Value * iPar5, C4Value * iPar6, C4Value * iPar7, C4Value * iPar8)
{
	if (!cthr->Obj) throw new NeedObjectContext("Message");
	char buf[MaxFnStringParLen+1];
	C4String * szMessage = c4vMessage->getStr();
	if (!szMessage) return C4VBool(false);

	// Speech
	bool fSpoken=false;
	if (SCopySegment(FnStringPar(szMessage),1,buf,'$'))
		if (StartSoundEffect(buf,false,100,cthr->Obj))
			fSpoken=true;

	// Text
	if (!fSpoken)
		if (SCopySegment(FnStringFormat(cthr,FnStringPar(szMessage),iPar0,iPar1,iPar2,iPar3,iPar4,iPar5,iPar6,iPar7,iPar8).getData(),0,buf,'$'))
		{
			GameMsgObject(buf,cthr->Obj);
		}

	return C4VBool(true);
}

static C4Value FnAddMessage_C4V(C4AulContext *cthr, C4Value *c4vMessage, C4Value * iPar0, C4Value * iPar1, C4Value * iPar2, C4Value * iPar3, C4Value * iPar4, C4Value * iPar5, C4Value * iPar6, C4Value * iPar7, C4Value * iPar8)
{
	if (!cthr->Obj) throw new NeedObjectContext("AddMessage");
	C4String * szMessage = c4vMessage->getStr();
	if (!szMessage) return C4VBool(false);

	::Messages.Append(C4GM_Target,FnStringFormat(cthr,FnStringPar(szMessage),iPar0,iPar1,iPar2,iPar3,iPar4,iPar5,iPar6,iPar7,iPar8).getData(),cthr->Obj,NO_OWNER,0,0,C4RGB(0xff, 0xff, 0xff));

	return C4VBool(true);
}

static bool FnScriptGo(C4AulContext *cthr, bool go)
{
	::GameScript.Go=!!go;
	return true;
}

static long FnMaterial(C4AulContext *cthr, C4String *mat_name)
{
	return ::MaterialMap.Get(FnStringPar(mat_name));
}

C4Object* FnPlaceVegetation(C4AulContext *cthr, C4ID id, long iX, long iY, long iWdt, long iHgt, long iGrowth)
{
	// Local call: relative coordinates
	if (cthr->Obj) { iX+=cthr->Obj->GetX(); iY+=cthr->Obj->GetY(); }
	// Place vegetation
	return Game.PlaceVegetation(id,iX,iY,iWdt,iHgt,iGrowth);
}

C4Object* FnPlaceAnimal(C4AulContext *cthr, C4ID id)
{
	return Game.PlaceAnimal(id);
}

static bool FnHostile(C4AulContext *cthr, long iPlr1, long iPlr2, bool fCheckOneWayOnly)
{
	if (fCheckOneWayOnly)
	{
		return ::Players.HostilityDeclared(iPlr1,iPlr2);
	}
	else
		return !!Hostile(iPlr1,iPlr2);
}

static bool FnSetHostility(C4AulContext *cthr, long iPlr, long iPlr2, bool fHostile, bool fSilent, bool fNoCalls)
{
	C4Player *pPlr = ::Players.Get(iPlr);
	if (!pPlr) return false;
	// do rejection test first
	if (!fNoCalls)
	{
		if (!!::GameScript.GRBroadcast(PSF_RejectHostilityChange, &C4AulParSet(C4VInt(iPlr), C4VInt(iPlr2), C4VBool(fHostile)), true, true))
			return false;
	}
	// OK; set hostility
	bool fOldHostility = ::Players.HostilityDeclared(iPlr, iPlr2);
	if (!pPlr->SetHostility(iPlr2,fHostile, fSilent)) return false;
	// calls afterwards
	::GameScript.GRBroadcast(PSF_OnHostilityChange, &C4AulParSet(C4VInt(iPlr), C4VInt(iPlr2), C4VBool(fHostile), C4VBool(fOldHostility)), true);
	return true;
}

static bool FnSetPlrView(C4AulContext *cthr, long iPlr, C4Object *tobj)
{
	if (!ValidPlr(iPlr)) return false;
	::Players.Get(iPlr)->SetViewMode(C4PVM_Target,tobj);
	return true;
}

static long FnGetPlrViewMode(C4AulContext *cthr, long iPlr)
{
	if (!ValidPlr(iPlr)) return -1;
	if (::Control.SyncMode()) return -1;
	return ::Players.Get(iPlr)->ViewMode;
}

static C4Void FnResetCursorView(C4AulContext *cthr, long plr)
{
	C4Player *pplr = ::Players.Get(plr);
	if (pplr) pplr->ResetCursorView();
	return C4Void();
}

static C4Object *FnGetPlrView(C4AulContext *cthr, long iPlr)
{
	C4Player *pPlr = ::Players.Get(iPlr);
	if (!pPlr || pPlr->ViewMode != C4PVM_Target) return NULL;
	return pPlr->ViewTarget;
}

// flags for SetPlayerZoom* calls
static const int PLRZOOM_Direct     = 0x01,
                 PLRZOOM_NoIncrease = 0x04,
                 PLRZOOM_NoDecrease = 0x08,
                 PLRZOOM_LimitMin   = 0x10,
                 PLRZOOM_LimitMax   = 0x20;

static bool FnSetPlayerZoomByViewRange(C4AulContext *cthr, long plr_idx, long range_wdt, long range_hgt, long flags)
{
	// zoom size safety - both ranges 0 is fine, it causes a zoom reset to default
	if (range_wdt < 0 || range_hgt < 0) return false;
	// special player NO_OWNER: apply to all viewports
	if (plr_idx == NO_OWNER)
	{
		for (C4Player *plr = ::Players.First; plr; plr=plr->Next)
			if (plr->Number != NO_OWNER) // can't happen, but would be a crash if it did...
				FnSetPlayerZoomByViewRange(cthr, plr->Number, range_wdt, range_hgt, flags);
		return true;
	}
	else
	{
		// safety check on player only, so function return result is always in sync
		C4Player *plr = ::Players.Get(plr_idx);
		if (!plr) return false;
		// adjust values in player
		if (!(flags & (PLRZOOM_LimitMin | PLRZOOM_LimitMax)))
			plr->SetZoomByViewRange(range_wdt, range_hgt, !!(flags & PLRZOOM_Direct), !!(flags & PLRZOOM_NoIncrease), !!(flags & PLRZOOM_NoDecrease));
		else
		{
			if (flags & PLRZOOM_LimitMin) plr->SetMinZoomByViewRange(range_wdt, range_hgt, !!(flags & PLRZOOM_NoIncrease), !!(flags & PLRZOOM_NoDecrease));
			if (flags & PLRZOOM_LimitMax) plr->SetMaxZoomByViewRange(range_wdt, range_hgt, !!(flags & PLRZOOM_NoIncrease), !!(flags & PLRZOOM_NoDecrease));
		}
	}
	return true;
}

static bool FnSetPlayerViewLock(C4AulContext *cthr, long plr_idx, bool is_locked)
{
	// special player NO_OWNER: apply to all players
	if (plr_idx == NO_OWNER)
	{
		for (C4Player *plr = ::Players.First; plr; plr=plr->Next)
			if (plr->Number != NO_OWNER) // can't happen, but would be a crash if it did...
				FnSetPlayerViewLock(cthr, plr->Number, is_locked);
		return true;
	}
	C4Player *plr = ::Players.Get(plr_idx);
	if (!plr) return false;
	plr->SetViewLocked(is_locked);
	return true;
}

static bool FnDoHomebaseMaterial(C4AulContext *cthr, long iPlr, C4ID id, long iChange)
{
	// validity check
	if (!ValidPlr(iPlr)) return false;
	C4Def *pDef = C4Id2Def(id);
	if (!pDef) return false;
	// add to material
	long iLastcount = ::Players.Get(iPlr)->HomeBaseMaterial.GetIDCount(id);
	return ::Players.Get(iPlr)->HomeBaseMaterial.SetIDCount(id,iLastcount+iChange,true);
}

static bool FnDoHomebaseProduction(C4AulContext *cthr, long iPlr, C4ID id, long iChange)
{
	// validity check
	if (!ValidPlr(iPlr)) return false;
	C4Def *pDef = C4Id2Def(id);
	if (!pDef) return false;
	// add to material
	long iLastcount = ::Players.Get(iPlr)->HomeBaseProduction.GetIDCount(id);
	return ::Players.Get(iPlr)->HomeBaseProduction.SetIDCount(id,iLastcount+iChange,true);
}

static bool FnSetPlrKnowledge(C4AulContext *cthr, long iPlr, C4ID id, bool fRemove)
{
	C4Player *pPlr=::Players.Get(iPlr);
	if (!pPlr) return false;
	if (fRemove)
	{
		long iIndex=pPlr->Knowledge.GetIndex(id);
		if (iIndex<0) return false;
		return pPlr->Knowledge.DeleteItem(iIndex);
	}
	else
	{
		if (!C4Id2Def(id)) return false;
		return pPlr->Knowledge.SetIDCount(id,1,true);
	}
}

static C4Value FnGetPlrKnowledge_C4V(C4AulContext *cthr, C4Value* iPlr_C4V, C4Value* id_C4V, C4Value* iIndex_C4V, C4Value* dwCategory_C4V)
{
	long iPlr = iPlr_C4V->getInt();
	C4ID id = id_C4V->getC4ID();
	long iIndex = iIndex_C4V->getInt();
	DWORD dwCategory = dwCategory_C4V->getInt();
	if (!ValidPlr(iPlr)) return C4VBool(false);
	// Search by id, check if available, return bool
	if (id) return C4VBool(::Players.Get(iPlr)->Knowledge.GetIDCount(id,1) != 0);
	// Search indexed item of given category, return C4ID
	return C4VPropList(C4Id2Def(::Players.Get(iPlr)->Knowledge.GetID( ::Definitions, dwCategory, iIndex )));
}

static C4Def * FnGetDefinition(C4AulContext *cthr, long iIndex)
{
	return ::Definitions.GetDef(iIndex);
}

static C4Value FnGetComponent_C4V(C4AulContext *cthr, C4Value* idComponent_C4V, C4Value* iIndex_C4V, C4Value* pObj_C4V, C4Value* idDef_C4V)
{
	C4ID idComponent = idComponent_C4V->getC4ID();
	long iIndex = iIndex_C4V->getInt();
	C4Object *pObj = pObj_C4V->getObj();
	C4ID idDef = idDef_C4V->getC4ID();

	// Def component - as seen by scope object as builder
	if (idDef)
	{
		// Get def
		C4Def *pDef=C4Id2Def(idDef); if (!pDef) return C4Value();
		// Component count
		if (idComponent) return C4VInt(pDef->GetComponentCount(idComponent));
		// Indexed component
		return C4VPropList(C4Id2Def(pDef->GetIndexedComponent(iIndex)));
	}
	// Object component
	else
	{
		// Get object
		if (!pObj) pObj=cthr->Obj; if (!pObj) return C4Value();
		// Component count
		if (idComponent) return C4VInt(pObj->Component.GetIDCount(idComponent));
		// Indexed component
		return C4VPropList(C4Id2Def(pObj->Component.GetID(iIndex)));
	}
	return C4Value();
}

static C4Value FnGetHomebaseMaterial_C4V(C4AulContext *cthr, C4Value* iPlr_C4V, C4Value* id_C4V, C4Value* iIndex_C4V, C4Value* dwCategory_C4V)
{
	long iPlr = iPlr_C4V->getInt();
	C4ID id = id_C4V->getC4ID();
	long iIndex = iIndex_C4V->getInt();
	DWORD dwCategory = dwCategory_C4V->getInt();

	if (!ValidPlr(iPlr)) return C4VBool(false);
	// Search by id, return available count
	if (id) return C4VInt(::Players.Get(iPlr)->HomeBaseMaterial.GetIDCount(id));
	// Search indexed item of given category, return C4ID
	return C4VPropList(C4Id2Def(::Players.Get(iPlr)->HomeBaseMaterial.GetID( ::Definitions, dwCategory, iIndex )));
}

static C4Value FnGetHomebaseProduction_C4V(C4AulContext *cthr, C4Value* iPlr_C4V, C4Value* id_C4V, C4Value* iIndex_C4V, C4Value* dwCategory_C4V)
{
	long iPlr = iPlr_C4V->getInt();
	C4ID id = id_C4V->getC4ID();
	long iIndex = iIndex_C4V->getInt();
	DWORD dwCategory = dwCategory_C4V->getInt();

	if (!ValidPlr(iPlr)) return C4VBool(false);
	// Search by id, return available count
	if (id) return C4VInt(::Players.Get(iPlr)->HomeBaseProduction.GetIDCount(id));
	// Search indexed item of given category, return C4ID
	return C4VPropList(C4Id2Def(::Players.Get(iPlr)->HomeBaseProduction.GetID( ::Definitions, dwCategory, iIndex )));
}

static long FnGetWealth(C4AulContext *cthr, long iPlr)
{
	if (!ValidPlr(iPlr)) return 0;
	return ::Players.Get(iPlr)->Wealth;
}

static bool FnSetWealth(C4AulContext *cthr, long iPlr, long iValue)
{
	if (!ValidPlr(iPlr)) return false;
	::Players.Get(iPlr)->SetWealth(iValue);
	return true;
}

static long FnDoPlayerScore(C4AulContext *cthr, long iPlr, long iChange)
{
	if (!ValidPlr(iPlr)) return false;
	return ::Players.Get(iPlr)->DoScore(iChange);
}

static long FnGetPlayerScore(C4AulContext *cthr, long iPlr)
{
	if (!ValidPlr(iPlr)) return 0;
	return ::Players.Get(iPlr)->CurrentScore;
}

static long FnGetPlayerScoreGain(C4AulContext *cthr, long iPlr)
{
	if (!ValidPlr(iPlr)) return 0;
	return ::Players.Get(iPlr)->CurrentScore - ::Players.Get(iPlr)->InitialScore;
}

static C4Object *FnGetHiRank(C4AulContext *cthr, long iPlr)
{
	if (!ValidPlr(iPlr)) return NULL;
	return ::Players.Get(iPlr)->GetHiRankActiveCrew();
}

static C4Object *FnGetCrew(C4AulContext *cthr, long iPlr, long index)
{
	if (!ValidPlr(iPlr)) return NULL;
	return ::Players.Get(iPlr)->Crew.GetObject(index);
}

static long FnGetCrewCount(C4AulContext *cthr, long iPlr)
{
	if (!ValidPlr(iPlr)) return 0;
	return ::Players.Get(iPlr)->Crew.ObjectCount();
}

static long FnGetPlayerCount(C4AulContext *cthr, long iType)
{
	if (!iType)
		return ::Players.GetCount();
	else
		return ::Players.GetCount((C4PlayerType) iType);
}

static long FnGetPlayerByIndex(C4AulContext *cthr, long iIndex, long iType)
{
	C4Player *pPlayer;
	if (iType)
		pPlayer = ::Players.GetByIndex(iIndex, (C4PlayerType) iType);
	else
		pPlayer = ::Players.GetByIndex(iIndex);
	if (!pPlayer) return NO_OWNER;
	return pPlayer->Number;
}

static long FnEliminatePlayer(C4AulContext *cthr, long iPlr, bool fRemoveDirect)
{
	C4Player *pPlr=::Players.Get(iPlr);
	if (!pPlr) return false;
	// direct removal?
	if (fRemoveDirect)
	{
		// do direct removal (no fate)
		return ::Players.CtrlRemove(iPlr, false);
	}
	else
	{
		// do regular elimination
		if (pPlr->Eliminated) return false;
		pPlr->Eliminate();
	}
	return true;
}

static bool FnSurrenderPlayer(C4AulContext *cthr, long iPlr)
{
	C4Player *pPlr=::Players.Get(iPlr);
	if (!pPlr) return false;
	if (pPlr->Eliminated) return false;
	pPlr->Surrender();
	return true;
}

static bool FnSetLeaguePerformance(C4AulContext *cthr, long iScore)
{
	Game.RoundResults.SetLeaguePerformance(iScore);
	return true;
}

static const int32_t CSPF_FixedAttributes = 1<<0,
    CSPF_NoScenarioInit  = 1<<1,
                           CSPF_NoEliminationCheck = 1<<2,
                                                     CSPF_Invisible          = 1<<3;

static bool FnCreateScriptPlayer(C4AulContext *cthr, C4String *szName, long dwColor, long idTeam, long dwFlags, C4ID idExtra)
{
	// safety
	if (!szName || !szName->GetData().getLength()) return false;
	// this script command puts a new script player info into the list
	// the actual join will be delayed and synchronized via queue
	// processed by control host only - clients/replay/etc. will perform the join via queue
	if (!::Control.isCtrlHost()) return true;
	C4PlayerInfo *pScriptPlrInfo = new C4PlayerInfo();
	uint32_t dwInfoFlags = 0u;
	if (dwFlags & CSPF_FixedAttributes   ) dwInfoFlags |= C4PlayerInfo::PIF_AttributesFixed;
	if (dwFlags & CSPF_NoScenarioInit    ) dwInfoFlags |= C4PlayerInfo::PIF_NoScenarioInit;
	if (dwFlags & CSPF_NoEliminationCheck) dwInfoFlags |= C4PlayerInfo::PIF_NoEliminationCheck;
	if (dwFlags & CSPF_Invisible         ) dwInfoFlags |= C4PlayerInfo::PIF_Invisible;
	pScriptPlrInfo->SetAsScriptPlayer(szName->GetCStr(), dwColor, dwInfoFlags, idExtra);
	pScriptPlrInfo->SetTeam(idTeam);
	C4ClientPlayerInfos JoinPkt(NULL, true, pScriptPlrInfo);
	// add to queue!
	Game.PlayerInfos.DoPlayerInfoUpdate(&JoinPkt);
	// always successful for sync reasons
	return true;
}

static C4Object *FnGetCursor(C4AulContext *cthr, long iPlr)
{
	// get player
	C4Player *pPlr = ::Players.Get(iPlr);
	// invalid player?
	if (!pPlr) return NULL;
	return pPlr->Cursor;
}

static C4Object *FnGetViewCursor(C4AulContext *cthr, long iPlr)
{
	// get player
	C4Player *pPlr = ::Players.Get(iPlr);
	// get viewcursor
	return pPlr ? pPlr->ViewCursor : NULL;
}

static bool FnSetCursor(C4AulContext *cthr, long iPlr, C4Object *pObj, bool fNoSelectArrow)
{
	C4Player *pPlr = ::Players.Get(iPlr);
	if (!pPlr || (pObj && !pObj->Status) || (pObj && pObj->CrewDisabled)) return false;
	pPlr->SetCursor(pObj, !fNoSelectArrow);
	return true;
}

static bool FnSetViewCursor(C4AulContext *cthr, long iPlr, C4Object *pObj)
{
	// get player
	C4Player *pPlr = ::Players.Get(iPlr);
	// invalid player?
	if (!pPlr) return false;
	// set viewcursor
	pPlr->ViewCursor = pObj;
	return true;
}

static long FnGetWind(C4AulContext *cthr, long x, long y, bool fGlobal)
{
	// global wind
	if (fGlobal) return ::Weather.Wind;
	// local wind
	if (cthr->Obj) { x+=cthr->Obj->GetX(); y+=cthr->Obj->GetY(); }
	return ::Weather.GetWind(x,y);
}

static C4Void FnSetWind(C4AulContext *cthr, long iWind)
{
	::Weather.SetWind(iWind);
	return C4Void();
}

static C4Void FnSetTemperature(C4AulContext *cthr, long iTemperature)
{
	::Weather.SetTemperature(iTemperature);
	return C4Void();
}

static long FnGetTemperature(C4AulContext *cthr)
{
	return ::Weather.GetTemperature();
}

static C4Void FnSetSeason(C4AulContext *cthr, long iSeason)
{
	::Weather.SetSeason(iSeason);
	return C4Void();
}

static long FnGetSeason(C4AulContext *cthr)
{
	return ::Weather.GetSeason();
}

static C4Void FnSetClimate(C4AulContext *cthr, long iClimate)
{
	::Weather.SetClimate(iClimate);
	return C4Void();
}

static long FnGetClimate(C4AulContext *cthr)
{
	return ::Weather.GetClimate();
}

static long FnLandscapeWidth(C4AulContext *cthr)
{
	return GBackWdt;
}

static long FnLandscapeHeight(C4AulContext *cthr)
{
	return GBackHgt;
}

static C4Void FnShakeFree(C4AulContext *cthr, long x, long y, long rad)
{
	::Landscape.ShakeFree(x,y,rad);
	return C4Void();
}

static C4Void FnDigFree(C4AulContext *cthr, long x, long y, long rad)
{
	::Landscape.DigFree(x,y,rad,cthr->Obj);
	return C4Void();
}

static C4Void FnDigFreeRect(C4AulContext *cthr, long iX, long iY, long iWdt, long iHgt)
{
	::Landscape.DigFreeRect(iX,iY,iWdt,iHgt,cthr->Obj);
	return C4Void();
}

static C4Void FnClearFreeRect(C4AulContext *cthr, long iX, long iY, long iWdt, long iHgt)
{
	::Landscape.ClearFreeRect(iX,iY,iWdt,iHgt);
	return C4Void();
}

static bool FnPathFree(C4AulContext *cthr, long X1, long Y1, long X2, long Y2)
{
	return !!PathFree(X1, Y1, X2, Y2);
}

static C4ValueArray* FnPathFree2(C4AulContext *cthr, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
	int32_t x = -1, y = -1;
	if (!PathFree(x1, y1, x2, y2, &x, &y))
	{
		C4ValueArray *pArray = new C4ValueArray(2);
		pArray->SetItem(0, C4VInt(x));
		pArray->SetItem(1, C4VInt(y));
		return pArray;
	}
	return 0;
}

C4Object* FnObject(C4AulContext *cthr, long iNumber)
{
	return ::Objects.SafeObjectPointer(iNumber);
	// See FnObjectNumber
}

static C4Value FnCall_C4V(C4AulContext *cthr, C4Value* szFunction_C4V,
                          C4Value* par0, C4Value* par1, C4Value* par2, C4Value* par3, C4Value* par4,
                          C4Value* par5, C4Value* par6, C4Value* par7, C4Value* par8/*, C4Value* par9*/)
{
	// safety
	C4String *szFunction = szFunction_C4V->getStr();

	if (!szFunction || !cthr->Obj) return C4Value();
	C4AulParSet Pars;
	Copy2ParSet9(Pars, *par);
	return cthr->Obj->Call(FnStringPar(szFunction),&Pars, true);
}

static C4Value FnDefinitionCall_C4V(C4AulContext *cthr,
                                    C4Value* idID_C4V, C4Value* szFunction_C4V,
                                    C4Value* par0, C4Value* par1, C4Value* par2, C4Value* par3, C4Value* par4,
                                    C4Value* par5, C4Value* par6, C4Value* par7/*, C4Value* par8, C4Value* par9*/)
{
	C4ID idID = idID_C4V->getC4ID();
	C4String *szFunction = szFunction_C4V->getStr();

	if (!idID || !szFunction) return C4Value();
	// Make failsafe
	char szFunc2[500+1]; sprintf(szFunc2,"~%s",FnStringPar(szFunction));
	// Get definition
	C4Def *pDef;
	if (!(pDef=C4Id2Def(idID))) return C4Value();
	// copy parameters
	C4AulParSet Pars;
	Copy2ParSet8(Pars, *par);
	// Call
	return pDef->Script.Call(szFunc2, 0, &Pars, true);
}

static C4Value FnGameCall_C4V(C4AulContext *cthr,
                              C4Value* szFunction_C4V,
                              C4Value* par0, C4Value* par1, C4Value* par2, C4Value* par3, C4Value* par4,
                              C4Value* par5, C4Value* par6, C4Value* par7, C4Value* par8/*, C4Value* par9*/)
{
	C4String *szFunction = szFunction_C4V->getStr();

	if (!szFunction) return C4Value();
	// Make failsafe
	char szFunc2[500+1]; sprintf(szFunc2,"~%s",FnStringPar(szFunction));
	// copy parameters
	C4AulParSet Pars;
	Copy2ParSet9(Pars, *par);
	// Call
	return ::GameScript.Call(szFunc2, 0, &Pars, true);
}

static C4Value FnGameCallEx_C4V(C4AulContext *cthr,
                                C4Value* szFunction_C4V,
                                C4Value* par0, C4Value* par1, C4Value* par2, C4Value* par3, C4Value* par4,
                                C4Value* par5, C4Value* par6, C4Value* par7, C4Value* par8/*, C4Value* par9*/)
{
	C4String *szFunction = szFunction_C4V->getStr();

	if (!szFunction) return C4Value();
	// Make failsafe
	char szFunc2[500+1]; sprintf(szFunc2,"~%s",FnStringPar(szFunction));
	// copy parameters
	C4AulParSet Pars;
	Copy2ParSet9(Pars, *par);
	// Call
	return ::GameScript.GRBroadcast(szFunc2,&Pars, true);
}

static C4Value FnProtectedCall_C4V(C4AulContext *cthr,
                                   C4Value* pObj_C4V, C4Value* szFunction_C4V,
                                   C4Value* par0, C4Value* par1, C4Value* par2, C4Value* par3, C4Value* par4,
                                   C4Value* par5, C4Value* par6, C4Value* par7/*, C4Value* par8, C4Value* par9*/)
{
	C4Object *pObj = pObj_C4V->getObj();
	C4String *szFunction = szFunction_C4V->getStr();

	if (!pObj || !szFunction) return C4Value();
	if (!pObj->Def) return C4Value();
	// get func
	C4AulScriptFunc *f;
	if (!(f = pObj->Def->Script.GetSFunc(FnStringPar(szFunction), AA_PROTECTED, true))) return C4Value();
	// copy parameters
	C4AulParSet Pars;
	Copy2ParSet8(Pars, *par);
	// exec
	return f->Exec(pObj,&Pars, true);
}


static C4Value FnPrivateCall_C4V(C4AulContext *cthr,
                                 C4Value* pObj_C4V, C4Value* szFunction_C4V,
                                 C4Value* par0, C4Value* par1, C4Value* par2, C4Value* par3, C4Value* par4,
                                 C4Value* par5, C4Value* par6, C4Value* par7/*, C4Value* par8, C4Value* par9*/)
{
	C4Object *pObj = pObj_C4V->getObj();
	C4String *szFunction = szFunction_C4V->getStr();

	if (!pObj || !szFunction) return C4Value();
	if (!pObj->Def) return C4Value();
	// get func
	C4AulScriptFunc *f;
	if (!(f = pObj->Def->Script.GetSFunc(FnStringPar(szFunction), AA_PRIVATE, true))) return C4Value();
	// copy parameters
	C4AulParSet Pars;
	Copy2ParSet8(Pars, *par);
	// exec
	return f->Exec(pObj,&Pars, true);
}

static C4Value FnEditCursor(C4AulContext *cth, C4Value *pPars)
{
	if (::Control.SyncMode()) return C4VNull;
	return C4VObj(Console.EditCursor.GetTarget());
}

static bool FnIsNetwork(C4AulContext *cthr) { return Game.Parameters.IsNetworkGame; }

static C4String *FnGetLeague(C4AulContext *cthr, long idx)
{
	// get indexed league
	StdStrBuf sIdxLeague;
	if (!Game.Parameters.League.GetSection(idx, &sIdxLeague)) return NULL;
	return String(sIdxLeague.getData());
}

static bool FnTestMessageBoard(C4AulContext *cthr, long iForPlr, bool fTestIfInUse)
{
	// multi-query-MessageBoard is always available if the player is valid =)
	// (but it won't do anything in developer mode...)
	C4Player *pPlr = ::Players.Get(iForPlr);
	if (!pPlr) return false;
	if (!fTestIfInUse) return true;
	// single query only if no query is scheduled
	return pPlr->HasMessageBoardQuery();
}

static bool FnCallMessageBoard(C4AulContext *cthr, C4Object *pObj, bool fUpperCase, C4String *szQueryString, long iForPlr)
{
	if (!pObj) pObj=cthr->Obj;
	if (pObj && !pObj->Status) return false;
	// check player
	C4Player *pPlr = ::Players.Get(iForPlr);
	if (!pPlr) return false;
	// remove any previous
	pPlr->CallMessageBoard(pObj, StdStrBuf(FnStringPar(szQueryString)), !!fUpperCase);
	return true;
}

static bool FnAbortMessageBoard(C4AulContext *cthr, C4Object *pObj, long iForPlr)
{
	if (!pObj) pObj=cthr->Obj;
	// check player
	C4Player *pPlr = ::Players.Get(iForPlr);
	if (!pPlr) return false;
	// close TypeIn if active
	::MessageInput.AbortMsgBoardQuery(pObj, iForPlr);
	// abort for it
	return pPlr->RemoveMessageBoardQuery(pObj);
}

static bool FnOnMessageBoardAnswer(C4AulContext *cthr, C4Object *pObj, long iForPlr, C4String *szAnswerString)
{
	// remove query
	// fail if query doesn't exist to prevent any doubled answers
	C4Player *pPlr = ::Players.Get(iForPlr);
	if (!pPlr) return false;
	if (!pPlr->RemoveMessageBoardQuery(pObj)) return false;
	// if no answer string is provided, the user did not answer anything
	// just remove the query
	if (!szAnswerString || !szAnswerString->GetCStr()) return true;
	// get script
	C4ScriptHost *scr;
	if (pObj) scr = &pObj->Def->Script; else scr = &::GameScript;
	// exec func
	return !!scr->Call(PSF_InputCallback, pObj, &C4AulParSet(C4VString(FnStringPar(szAnswerString)), C4VInt(iForPlr)));
}

static long FnScriptCounter(C4AulContext *cthr)
{
	return ::GameScript.Counter;
}

static C4Void FnSetFoW(C4AulContext *cthr, bool fEnabled, long iPlr)
{
	// safety
	if (!ValidPlr(iPlr)) return C4Void();
	// set enabled
	::Players.Get(iPlr)->SetFoW(!!fEnabled);
	// success
	return C4Void();
}

static long FnSetMaxPlayer(C4AulContext *cthr, long iTo)
{
	// think positive! :)
	if (iTo < 0) return false;
	// script functions don't need to pass ControlQueue!
	Game.Parameters.MaxPlayers = iTo;
	// success
	return true;
}

static bool FnGetMissionAccess(C4AulContext *cthr, C4String *strMissionAccess)
{
	// safety
	if (!strMissionAccess) return false;

	// non-sync mode: warn
	if (::Control.SyncMode())
		Log("Warning: using GetMissionAccess may cause desyncs when playing records!");

	if (!Config.General.MissionAccess) return false;
	return SIsModule(Config.General.MissionAccess, FnStringPar(strMissionAccess));
}

// Helper to read or write a value from/to a structure. Must be two
class C4ValueCompiler : public StdCompiler
{
public:
	C4ValueCompiler(const char **pszNames, int iNameCnt, int iEntryNr)
			: pszNames(pszNames), iNameCnt(iNameCnt), iEntryNr(iEntryNr)
	{  }

	virtual bool isCompiler() { return false; }
	virtual bool hasNaming() { return true; }
	virtual bool isVerbose() { return false; }

	virtual bool Name(const char *szName)
	{
		// match possible? (no match yet / continued match)
		if (!iMatchStart || haveCurrentMatch())
			// already got all names?
			if (!haveCompleteMatch())
				// check name
				if (SEqual(pszNames[iMatchCount], szName))
				{
					// got match
					if (!iMatchCount) iMatchStart = iDepth + 1;
					iMatchCount++;
				}
		iDepth++;
		return true;
	}

	virtual bool Default(const char *szName)
	{
		// Always process values even if they are default!
		return false;
	}

	virtual void NameEnd(bool fBreak = false)
	{
		// end of matched name section?
		if (haveCurrentMatch())
		{
			iMatchCount--;
			if (!iMatchCount) iMatchStart = 0;
		}
		iDepth--;
	}

	virtual void Begin()
	{
		// set up
		iDepth = iMatchStart = iMatchCount = 0;
	}

protected:
	// value function forward to be overwritten by get or set compiler
	virtual void ProcessInt(int32_t &rInt) = 0;
	virtual void ProcessBool(bool &rBool) = 0;
	virtual void ProcessChar(char &rChar) = 0;
	virtual void ProcessString(char *szString, size_t iMaxLength, bool fIsID) = 0;
	virtual void ProcessString(char **pszString, bool fIsID) = 0;

public:
	// value functions
	virtual void DWord(int32_t &rInt)      { if (haveCompleteMatch()) if (!iEntryNr--) ProcessInt(rInt); }
	virtual void DWord(uint32_t &rInt)     { if (haveCompleteMatch()) if (!iEntryNr--) { int32_t i=rInt;   ProcessInt(i); rInt  =i; } }
	virtual void Word(int16_t &rShort)     { if (haveCompleteMatch()) if (!iEntryNr--) { int32_t i=rShort; ProcessInt(i); rShort=i; } }
	virtual void Word(uint16_t &rShort)    { if (haveCompleteMatch()) if (!iEntryNr--) { int32_t i=rShort; ProcessInt(i); rShort=i; } }
	virtual void Byte(int8_t &rByte)       { if (haveCompleteMatch()) if (!iEntryNr--) { int32_t i=rByte;  ProcessInt(i); rByte =i; } }
	virtual void Byte(uint8_t &rByte)      { if (haveCompleteMatch()) if (!iEntryNr--) { int32_t i=rByte;  ProcessInt(i); rByte =i; } }
	virtual void Boolean(bool &rBool)      { if (haveCompleteMatch()) if (!iEntryNr--) ProcessBool(rBool); }
	virtual void Character(char &rChar)    { if (haveCompleteMatch()) if (!iEntryNr--) ProcessChar(rChar); }

	// The C4ID-Adaptor will set RCT_ID for it's strings (see C4Id.h), so we don't have to guess the type.
	virtual void String(char *szString, size_t iMaxLength, RawCompileType eType)
	{ if (haveCompleteMatch()) if (!iEntryNr--) ProcessString(szString, iMaxLength, eType == StdCompiler::RCT_ID); }
	virtual void String(char **pszString, RawCompileType eType)
	{ if (haveCompleteMatch()) if (!iEntryNr--) ProcessString(pszString, eType == StdCompiler::RCT_ID); }
	virtual void Raw(void *pData, size_t iSize, RawCompileType eType = RCT_Escaped)
	{ /* C4Script can't handle this */ }

private:

	// Name(s) of the entry to read
	const char **pszNames;
	int iNameCnt;
	// Number of the element that is to be read
	int iEntryNr;

	// current depth
	int iDepth;
	// match start (where did the first name match?),
	// match count (how many names did match, from that point?)
	int iMatchStart, iMatchCount;

private:
	// match active?
	bool haveCurrentMatch() const { return iDepth + 1 == iMatchStart + iMatchCount; }
	// match complete?
	bool haveCompleteMatch() const { return haveCurrentMatch() && iMatchCount == iNameCnt; }
};

class C4ValueGetCompiler : public C4ValueCompiler
{
private:
	// Result
	C4Value Res;
public:
	C4ValueGetCompiler(const char **pszNames, int iNameCnt, int iEntryNr)
			: C4ValueCompiler(pszNames, iNameCnt, iEntryNr)
	{  }

	// Result-getter
	const C4Value &getResult() const { return Res; }

protected:
	// get values as C4Value
	virtual void ProcessInt(int32_t &rInt) { Res = C4VInt(rInt); }
	virtual void ProcessBool(bool &rBool)  { Res = C4VBool(rBool); }
	virtual void ProcessChar(char &rChar)  { Res = C4VString(FormatString("%c", rChar)); }

	virtual void ProcessString(char *szString, size_t iMaxLength, bool fIsID)
	{ Res = (fIsID ? C4VPropList(C4Id2Def(C4ID(szString))) : C4VString(szString)); }
	virtual void ProcessString(char **pszString, bool fIsID)
	{ Res = (fIsID ? C4VPropList(C4Id2Def(C4ID(*pszString))) : C4VString(*pszString)); }
};

// Use the compiler to find a named value in a structure
template <class T>
C4Value GetValByStdCompiler(const char *strEntry, const char *strSection, int iEntryNr, const T &rFrom)
{
	// Set up name array, create compiler
	const char *szNames[2] = { strSection ? strSection : strEntry, strSection ? strEntry : NULL };
	C4ValueGetCompiler Comp(szNames, strSection ? 2 : 1, iEntryNr);

	// Compile
	try
	{
		Comp.Decompile(rFrom);
		return Comp.getResult();
	}
	// Should not happen, catch it anyway.
	catch (StdCompiler::Exception *)
	{
		return C4VNull;
	}
}

static C4Value FnGetDefCoreVal(C4AulContext* cthr, C4Value* strEntry_C4V, C4Value* strSection_C4V, C4Value *iEntryNr_C4V)
{
	if (!cthr->Def)
		throw new NeedNonGlobalContext("GetDefCoreVal");

	const char *strEntry = FnStringPar(strEntry_C4V->getStr());
	const char *strSection = FnStringPar(strSection_C4V->getStr());
	if (strSection && !*strSection) strSection = NULL;
	long iEntryNr = iEntryNr_C4V->getInt();

	return GetValByStdCompiler(strEntry, strSection, iEntryNr, mkNamingAdapt(*cthr->Def, "DefCore"));
}

static C4Value FnGetObjectVal(C4AulContext* cthr, C4Value* strEntry_C4V, C4Value* strSection_C4V, C4Value *iEntryNr_C4V)
{
	if (!cthr->Obj) throw new NeedObjectContext("GetObjectVal");
	const char *strEntry = FnStringPar(strEntry_C4V->getStr());
	const char *strSection = FnStringPar(strSection_C4V->getStr());
	if (!*strSection) strSection = NULL;

	long iEntryNr = iEntryNr_C4V->getInt();

	// get value
	C4ValueNumbers numbers;
	C4Value retval = GetValByStdCompiler(strEntry, strSection, iEntryNr, mkNamingAdapt(mkParAdapt(*cthr->Obj, &numbers), "Object"));
	numbers.Denumerate();
	retval.Denumerate(&numbers);
	return retval;
}

static C4Value FnGetObjectInfoCoreVal(C4AulContext* cthr, C4Value* strEntry_C4V, C4Value* strSection_C4V, C4Value *iEntryNr_C4V)
{
	if (!cthr->Obj) throw new NeedObjectContext("GetObjectInfoCoreVal");
	const char *strEntry = FnStringPar(strEntry_C4V->getStr());
	const char *strSection = FnStringPar(strSection_C4V->getStr());
	if (strSection && !*strSection) strSection = NULL;
	long iEntryNr = iEntryNr_C4V->getInt();

	// get obj info
	C4ObjectInfo* pObjInfo = cthr->Obj->Info;

	if (!pObjInfo) return C4VNull;

	// get obj info core
	C4ObjectInfoCore* pObjInfoCore = (C4ObjectInfoCore*) pObjInfo;

	// get value
	return GetValByStdCompiler(strEntry, strSection, iEntryNr, mkNamingAdapt(*pObjInfoCore, "ObjectInfo"));
}

static C4Value FnGetScenarioVal(C4AulContext* cthr, C4Value* strEntry_C4V, C4Value* strSection_C4V, C4Value *iEntryNr_C4V)
{
	const char *strEntry = FnStringPar(strEntry_C4V->getStr());
	const char *strSection = FnStringPar(strSection_C4V->getStr());
	long iEntryNr = iEntryNr_C4V->getInt();

	if (strSection && !*strSection) strSection = NULL;

	return GetValByStdCompiler(strEntry, strSection, iEntryNr, mkParAdapt(Game.C4S, false));
}

static C4Value FnGetPlayerVal(C4AulContext* cthr, C4Value* strEntry_C4V, C4Value* strSection_C4V, C4Value* iPlayer_C4V, C4Value *iEntryNr_C4V)
{
	const char *strEntry = FnStringPar(strEntry_C4V->getStr());
	const char *strSection = FnStringPar(strSection_C4V->getStr());
	if (strSection && !*strSection) strSection = NULL;
	long iPlr = iPlayer_C4V->getInt();
	long iEntryNr = iEntryNr_C4V->getInt();

	if (!ValidPlr(iPlr)) return C4Value();

	// get player
	C4Player* pPlayer = ::Players.Get(iPlr);

	// get value
	C4ValueNumbers numbers;
	C4Value retval = GetValByStdCompiler(strEntry, strSection, iEntryNr, mkNamingAdapt(mkParAdapt(*pPlayer, &numbers), "Player"));
	numbers.Denumerate();
	retval.Denumerate(&numbers);
	return retval;
}

static C4Value FnGetPlayerInfoCoreVal(C4AulContext* cthr, C4Value* strEntry_C4V, C4Value* strSection_C4V, C4Value* iPlayer_C4V, C4Value *iEntryNr_C4V)
{
	const char *strEntry = FnStringPar(strEntry_C4V->getStr());
	const char *strSection = FnStringPar(strSection_C4V->getStr());
	if (strSection && !*strSection) strSection = NULL;
	long iPlr = iPlayer_C4V->getInt();
	long iEntryNr = iEntryNr_C4V->getInt();

	if (!ValidPlr(iPlr)) return C4Value();

	// get player
	C4Player* pPlayer = ::Players.Get(iPlr);

	// get plr info core
	C4PlayerInfoCore* pPlayerInfoCore = (C4PlayerInfoCore*) pPlayer;

	// get value
	return GetValByStdCompiler(strEntry, strSection, iEntryNr, *pPlayerInfoCore);
}

static C4Value FnGetMaterialVal(C4AulContext* cthr, C4Value* strEntry_C4V, C4Value* strSection_C4V, C4Value* iMat_C4V, C4Value *iEntryNr_C4V)
{
	const char *strEntry = FnStringPar(strEntry_C4V->getStr());
	const char *strSection = FnStringPar(strSection_C4V->getStr());
	if (strSection && !*strSection) strSection = NULL;
	long iMat = iMat_C4V->getInt();
	long iEntryNr = iEntryNr_C4V->getInt();

	if (iMat < 0 || iMat >= ::MaterialMap.Num) return C4Value();

	// get material
	C4Material *pMaterial = &::MaterialMap.Map[iMat];

	// get plr info core
	C4MaterialCore* pMaterialCore = static_cast<C4MaterialCore*>(pMaterial);

	// material core implicates section "Material"
	if (!SEqual(strSection, "Material")) return C4Value();

	// get value
	return GetValByStdCompiler(strEntry, NULL, iEntryNr, *pMaterialCore);
}

static C4String *FnMaterialName(C4AulContext* cthr, long iMat)
{
	// mat valid?
	if (!MatValid(iMat)) return NULL;
	// return mat name
	return String(::MaterialMap.Map[iMat].Name);
}

static C4String *FnGetNeededMatStr(C4AulContext* cthr)
{
	// local/safety
	if (!cthr->Obj) throw new NeedObjectContext("GetNeededMatStr");
	return String(cthr->Obj->GetNeededMatStr().getData());
}

static bool FnSetSkyAdjust(C4AulContext* cthr, long dwAdjust, long dwBackClr)
{
	// set adjust
	::Landscape.Sky.SetModulation(dwAdjust, dwBackClr);
	// success
	return true;
}

static bool FnSetMatAdjust(C4AulContext* cthr, long dwAdjust)
{
	// set adjust
	::Landscape.SetModulation(dwAdjust);
	// success
	return true;
}

static long FnGetSkyAdjust(C4AulContext* cthr, bool fBackColor)
{
	// get adjust
	return ::Landscape.Sky.GetModulation(!!fBackColor);
}

static long FnGetMatAdjust(C4AulContext* cthr)
{
	// get adjust
	return ::Landscape.GetModulation();
}

static long FnGetTime(C4AulContext *)
{
	// check network, record, etc
	if (::Control.SyncMode()) return 0;
	return GetTime();
}

static C4Value FnSetPlrExtraData(C4AulContext *cthr, C4Value *iPlayer_C4V, C4Value *strDataName_C4V, C4Value *Data)
{
	long iPlayer = iPlayer_C4V->getInt();
	const char *strDataName = FnStringPar(strDataName_C4V->getStr());
	// valid player? (for great nullpointer prevention)
	if (!ValidPlr(iPlayer)) return C4Value();
	// do not allow data type C4V_Array or C4V_C4Object
	if (Data->GetType() != C4V_Nil &&
	    Data->GetType() != C4V_Int &&
	    Data->GetType() != C4V_Bool &&
	    Data->GetType() != C4V_String) return C4VNull;
	// get pointer on player...
	C4Player* pPlayer = ::Players.Get(iPlayer);
	// no name list created yet?
	if (!pPlayer->ExtraData.pNames)
		// create name list
		pPlayer->ExtraData.CreateTempNameList();
	// data name already exists?
	long ival;
	if ((ival = pPlayer->ExtraData.pNames->GetItemNr(strDataName)) != -1)
		pPlayer->ExtraData[ival] = *Data;
	else
	{
		// add name
		pPlayer->ExtraData.pNames->AddName(strDataName);
		// get val id & set
		if ((ival = pPlayer->ExtraData.pNames->GetItemNr(strDataName)) == -1) return C4Value();
		pPlayer->ExtraData[ival] = *Data;
	}
	// ok, return the value that has been set
	return *Data;
}

static C4Value FnGetPlrExtraData(C4AulContext *cthr, C4Value *iPlayer_C4V, C4Value *strDataName_C4V)
{
	long iPlayer = iPlayer_C4V->getInt();
	const char *strDataName = FnStringPar(strDataName_C4V->getStr());
	// valid player?
	if (!ValidPlr(iPlayer)) return C4Value();
	// get pointer on player...
	C4Player* pPlayer = ::Players.Get(iPlayer);
	// no name list?
	if (!pPlayer->ExtraData.pNames) return C4Value();
	long ival;
	if ((ival = pPlayer->ExtraData.pNames->GetItemNr(strDataName)) == -1) return C4Value();
	// return data
	return pPlayer->ExtraData[ival];
}

static long FnDrawMatChunks(C4AulContext *cctx, long tx, long ty, long twdt, long thgt, long icntx, long icnty, C4String *strMaterial, C4String *strTexture, bool bIFT)
{
	return ::Landscape.DrawChunks(tx, ty, twdt, thgt, icntx, icnty, FnStringPar(strMaterial), FnStringPar(strTexture), bIFT != 0);
}

static long FnDrawMap(C4AulContext *cctx, long iX, long iY, long iWdt, long iHgt, C4String *szMapDef)
{
	// draw it!
	return ::Landscape.DrawMap(iX, iY, iWdt, iHgt, FnStringPar(szMapDef));
}

static long FnDrawDefMap(C4AulContext *cctx, long iX, long iY, long iWdt, long iHgt, C4String *szMapDef)
{
	// draw it!
	return ::Landscape.DrawDefMap(iX, iY, iWdt, iHgt, FnStringPar(szMapDef));
}

static bool FnCreateParticle(C4AulContext *cthr, C4String *szName, long iX, long iY, long iXDir, long iYDir, long a, long b, C4Object *pObj, bool fBack)
{
	// safety
	if (pObj && !pObj->Status) return false;
	// local offset
	if (cthr->Obj)
	{
		iX+=cthr->Obj->GetX();
		iY+=cthr->Obj->GetY();
	}
	// get particle
	C4ParticleDef *pDef=::Particles.GetDef(FnStringPar(szName));
	if (!pDef) return false;
	// create
	::Particles.Create(pDef, (float) iX, (float) iY, (float) iXDir/10.0f, (float) iYDir/10.0f, (float) a/10.0f, b, pObj ? (fBack ? &pObj->BackParticles : &pObj->FrontParticles) : NULL, pObj);
	// success, even if not created
	return true;
}

static bool FnCastAParticles(C4AulContext *cthr, C4String *szName, long iAmount, long iLevel, long iX, long iY, long a0, long a1, long b0, long b1, C4Object *pObj, bool fBack)
{
	// safety
	if (pObj && !pObj->Status) return false;
	// local offset
	if (cthr->Obj)
	{
		iX+=cthr->Obj->GetX();
		iY+=cthr->Obj->GetY();
	}
	// get particle
	C4ParticleDef *pDef=::Particles.GetDef(FnStringPar(szName));
	if (!pDef) return false;
	// cast
	::Particles.Cast(pDef, iAmount, (float) iX, (float) iY, iLevel, (float) a0/10.0f, b0, (float) a1/10.0f, b1, pObj ? (fBack ? &pObj->BackParticles : &pObj->FrontParticles) : NULL, pObj);
	// success, even if not created
	return true;
}

static bool FnCastParticles(C4AulContext *cthr, C4String *szName, long iAmount, long iLevel, long iX, long iY, long a0, long a1, long b0, long b1, C4Object *pObj)
{
	return FnCastAParticles(cthr, szName, iAmount, iLevel, iX, iY, a0, a1, b0, b1, pObj, false);
}

static bool FnCastBackParticles(C4AulContext *cthr, C4String *szName, long iAmount, long iLevel, long iX, long iY, long a0, long a1, long b0, long b1, C4Object *pObj)
{
	return FnCastAParticles(cthr, szName, iAmount, iLevel, iX, iY, a0, a1, b0, b1, pObj, true);
}

static bool FnPushParticles(C4AulContext *cthr, C4String *szName, long iAX, long iAY)
{
	// particle given?
	C4ParticleDef *pDef=NULL;
	if (szName)
	{
		pDef=::Particles.GetDef(FnStringPar(szName));
		if (!pDef) return false;
	}
	// push them
	::Particles.Push(pDef, (float) iAX/10.0f, (float)iAY/10.0f);
	// success
	return true;
}

static bool FnClearParticles(C4AulContext *cthr, C4String *szName, C4Object *pObj)
{
	// particle given?
	C4ParticleDef *pDef=NULL;
	if (szName)
	{
		pDef=::Particles.GetDef(FnStringPar(szName));
		if (!pDef) return false;
	}
	// delete them
	if (pObj)
	{
		pObj->FrontParticles.Remove(pDef);
		pObj->BackParticles.Remove(pDef);
	}
	else
		::Particles.GlobalParticles.Remove(pDef);
	// success
	return true;
}

static bool FnSetSkyParallax(C4AulContext* ctx, Nillable<long> iMode, Nillable<long> iParX, Nillable<long> iParY, Nillable<long> iXDir, Nillable<long> iYDir, Nillable<long> iX, Nillable<long> iY)
{
	// set all parameters that aren't nil
	if (!iMode.IsNil())
		if (Inside<long>(iMode, 0, 1)) ::Landscape.Sky.ParallaxMode = iMode;
	if (!iParX.IsNil() && iParX) ::Landscape.Sky.ParX = iParX;
	if (!iParY.IsNil() && iParY) ::Landscape.Sky.ParY = iParY;
	if (!iXDir.IsNil()) ::Landscape.Sky.xdir = itofix(iXDir);
	if (!iYDir.IsNil()) ::Landscape.Sky.ydir = itofix(iYDir);
	if (!iX.IsNil()) ::Landscape.Sky.x = itofix(iX);
	if (!iY.IsNil()) ::Landscape.Sky.y = itofix(iY);
	// success
	return true;
}

static long FnReloadDef(C4AulContext* ctx, C4ID idDef, long iReloadWhat)
{
	// get def
	C4Def *pDef=NULL;
	if (!idDef)
	{
		// no def given: local def
		if (ctx->Obj) pDef=ctx->Obj->Def;
	}
	else
		// def by ID
		pDef=::Definitions.ID2Def(idDef);
	// safety
	if (!pDef) return false;
	// reload everything if nothing has been specified
	if (!iReloadWhat) iReloadWhat=C4D_Load_RX;
	// perform reload
	return Game.ReloadDef(pDef->id);
}

static long FnReloadParticle(C4AulContext* ctx, C4String *szParticleName)
{
	// perform reload
	return Game.ReloadParticle(FnStringPar(szParticleName));
}

static bool FnSetGamma(C4AulContext* ctx, long dwClr1, long dwClr2, long dwClr3, long iRampIndex)
{
	lpDDraw->SetGamma(dwClr1, dwClr2, dwClr3, iRampIndex);
	return true;
}

static bool FnResetGamma(C4AulContext* ctx, long iRampIndex)
{
	lpDDraw->SetGamma(0x000000, 0x808080, 0xffffff, iRampIndex);
	return true;
}

static long FnFrameCounter(C4AulContext*) { return Game.FrameCounter; }

struct PathInfo
{
	long ilx, ily;
	long ilen;
};

static bool SumPathLength(int32_t iX, int32_t iY, intptr_t iTransferTarget, intptr_t ipPathInfo)
{
	PathInfo *pPathInfo = (PathInfo*) ipPathInfo;
	pPathInfo->ilen += Distance(pPathInfo->ilx, pPathInfo->ily, iX, iY);
	pPathInfo->ilx = iX;
	pPathInfo->ily = iY;
	return true;
}

static Nillable<long> FnGetPathLength(C4AulContext* ctx, long iFromX, long iFromY, long iToX, long iToY)
{
	PathInfo PathInfo;
	PathInfo.ilx = iFromX;
	PathInfo.ily = iFromY;
	PathInfo.ilen = 0;
	if (!Game.PathFinder.Find(iFromX, iFromY, iToX, iToY, &SumPathLength, (intptr_t) &PathInfo))
		return C4Void();
	return PathInfo.ilen + Distance(PathInfo.ilx, PathInfo.ily, iToX, iToY);
}

static long FnSetTextureIndex(C4AulContext *ctx, C4String *psMatTex, long iNewIndex, bool fInsert)
{
	if (!Inside(iNewIndex, 0l, 255l)) return false;
	return ::Landscape.SetTextureIndex(FnStringPar(psMatTex), BYTE(iNewIndex), !!fInsert);
}

static long FnRemoveUnusedTexMapEntries(C4AulContext *ctx)
{
	::Landscape.RemoveUnusedTexMapEntries();
	return true;
}

static bool FnSetLandscapePixel(C4AulContext* ctx, long iX, long iY, long dwValue)
{
	// local call
	if (ctx->Obj) { iX+=ctx->Obj->GetX(); iY+=ctx->Obj->GetY(); }
	// set pixel in 32bit-sfc only
	// TODO: ::Landscape.SetPixDw(iX, iY, dwValue);
	// success
	return true;
}

static const int32_t DMQ_Sky = 0, // draw w/ sky IFT
                               DMQ_Sub = 1, // draw w/ tunnel IFT
                                         DMQ_Bridge = 2; // draw only over materials you can bridge over

static bool FnDrawMaterialQuad(C4AulContext* ctx, C4String *szMaterial, long iX1, long iY1, long iX2, long iY2, long iX3, long iY3, long iX4, long iY4, int draw_mode)
{
	const char *szMat = FnStringPar(szMaterial);
	return !! ::Landscape.DrawQuad(iX1, iY1, iX2, iY2, iX3, iY3, iX4, iY4, szMat, draw_mode == DMQ_Sub, draw_mode==DMQ_Bridge);
}

static bool FnSetFilmView(C4AulContext *ctx, long iToPlr)
{
	// check player
	if (!ValidPlr(iToPlr) && iToPlr != NO_OWNER) return false;
	// real switch in replays only
	if (!::Control.isReplay()) return true;
	// set new target plr
	if (C4Viewport *vp = ::Viewports.GetFirstViewport()) vp->Init(iToPlr, true);
	// done, always success (sync)
	return true;
}

static bool FnAddMsgBoardCmd(C4AulContext *ctx, C4String *pstrCommand, C4String *pstrScript)
{
	// safety
	if (!pstrCommand || !pstrScript) return false;
	// add command
	::MessageInput.AddCommand(FnStringPar(pstrCommand), FnStringPar(pstrScript));
	return true;
}

static bool FnSetGameSpeed(C4AulContext *ctx, long iSpeed)
{
	// safety
	if (iSpeed) if (!Inside<long>(iSpeed, 0, 1000)) return false;
	if (!iSpeed) iSpeed = 38;
	// set speed, restart timer
	Application.SetGameTickDelay(1000 / iSpeed);
	return true;
}

bool SimFlight(C4Real &x, C4Real &y, C4Real &xdir, C4Real &ydir, int32_t iDensityMin, int32_t iDensityMax, int32_t &iIter);

static C4ValueArray* FnSimFlight(C4AulContext *ctx, int X, int Y, Nillable<int> pvrXDir, Nillable<int> pvrYDir, Nillable<int> pviDensityMin, Nillable<int> pviDensityMax, Nillable<int> pviIter, int iPrec)
{
	// check and set parameters
	if (ctx->Obj)
	{
		X += ctx->Obj->GetX();
		Y += ctx->Obj->GetY();
	}
	int XDir = pvrXDir.IsNil() && ctx->Obj ? fixtoi(ctx->Obj->xdir) : static_cast<int>(pvrXDir);
	int YDir = pvrXDir.IsNil() && ctx->Obj ? fixtoi(ctx->Obj->ydir) : static_cast<int>(pvrYDir);

	int iDensityMin = pviDensityMin.IsNil() ? C4M_Solid : static_cast<int>(pviDensityMin);
	int iDensityMax = pviDensityMax.IsNil() ? 100 : static_cast<int>(pviDensityMax);
	int iIter = pviIter.IsNil() ? -1 : static_cast<int>(pviIter);
	if (!iPrec) iPrec = 10;

	// convert to C4Real
	C4Real x = itofix(X), y = itofix(Y),
		xdir = itofix(XDir, iPrec), ydir = itofix(YDir, iPrec);

	// simulate
	if (!SimFlight(x, y, xdir, ydir, iDensityMin, iDensityMax, iIter))
	{
		iIter *= -1;
	}

	// write results to array
	C4ValueArray *pResults = new C4ValueArray(5);
	pResults->SetItem(0, C4VInt(fixtoi(x)));
	pResults->SetItem(1, C4VInt(fixtoi(y)));
	pResults->SetItem(2, C4VInt(fixtoi(xdir * iPrec)));
	pResults->SetItem(3, C4VInt(fixtoi(ydir * iPrec)));
	pResults->SetItem(4, C4VInt(iIter));
	return pResults;
}

static long FnLoadScenarioSection(C4AulContext *ctx, C4String *pstrSection, long dwFlags)
{
	// safety
	const char *szSection;
	if (!pstrSection || !*(szSection=FnStringPar(pstrSection))) return false;
	// try to load it
	return Game.LoadScenarioSection(szSection, dwFlags);
}

static C4Value FnAddEffect_C4V(C4AulContext *ctx, C4Value *pvsEffectName, C4Value *pvpTarget, C4Value *pviPrio, C4Value *pviTimerInterval, C4Value *pvpCmdTarget, C4Value *pvidCmdTarget, C4Value *pvVal1, C4Value *pvVal2, C4Value *pvVal3, C4Value *pvVal4)
{
	// evaluate parameters
	C4String *szEffect = pvsEffectName->getStr();
	C4Object *pTarget = pvpTarget->getObj();
	long iPrio = pviPrio->getInt(), iTimerInterval = pviTimerInterval->getInt();
	C4Object *pCmdTarget = pvpCmdTarget->getObj();
	C4ID idCmdTarget = pvidCmdTarget->getC4ID();
	// safety
	if (pTarget && !pTarget->Status) return C4Value();
	if (!szEffect || !*szEffect->GetCStr() || !iPrio) return C4Value();
	// create effect
	C4Effect * pEffect = C4Effect::New(pTarget, szEffect, iPrio, iTimerInterval, pCmdTarget, idCmdTarget, *pvVal1, *pvVal2, *pvVal3, *pvVal4);
	// return effect - may be 0 if he effect has been denied by another effect
	if (!pEffect) return C4Value();
	return C4VPropList(pEffect);
}

static C4Effect * FnGetEffect(C4AulContext *ctx, C4String *psEffectName, C4Object *pTarget, int index, int iMaxPriority)
{
	const char *szEffect = FnStringPar(psEffectName);
	// get effects
	C4Effect *pEffect = pTarget ? pTarget->pEffects : Game.pGlobalEffects;
	if (!pEffect) return NULL;
	// name/wildcard given: find effect by name and index
	if (szEffect && *szEffect)
		return pEffect->Get(szEffect, index, iMaxPriority);
	return NULL;
}

static bool FnRemoveEffect(C4AulContext *ctx, C4String *psEffectName, C4Object *pTarget, C4Effect * pEffect2, bool fDoNoCalls)
{
	// evaluate parameters
	const char *szEffect = FnStringPar(psEffectName);
	// get effects
	C4Effect *pEffect = pTarget ? pTarget->pEffects : Game.pGlobalEffects;
	if (!pEffect) return 0;
	// name/wildcard given: find effect by name
	if (szEffect && *szEffect)
		pEffect = pEffect->Get(szEffect, 0);
	else
		pEffect = pEffect2;
	// effect found?
	if (!pEffect) return 0;
	// kill it
	if (fDoNoCalls)
		pEffect->SetDead();
	else
		pEffect->Kill(pTarget);
	// done, success
	return true;
}

static C4Value FnCheckEffect_C4V(C4AulContext *ctx, C4Value *pvsEffectName, C4Value *pvpTarget, C4Value *pviPrio, C4Value *pviTimerInterval, C4Value *pvVal1, C4Value *pvVal2, C4Value *pvVal3, C4Value *pvVal4)
{
	// evaluate parameters
	C4String *psEffectName = pvsEffectName->getStr();
	C4Object *pTarget = pvpTarget->getObj();
	long iPrio = pviPrio->getInt(), iTimerInterval = pviTimerInterval->getInt();
	const char *szEffect = FnStringPar(psEffectName);
	// safety
	if (pTarget && !pTarget->Status) return C4Value();
	if (!szEffect || !*szEffect) return C4Value();
	// get effects
	C4Effect *pEffect = pTarget ? pTarget->pEffects : Game.pGlobalEffects;
	if (!pEffect) return C4Value();
	// let them check
	C4Effect * r = pEffect->Check(pTarget, szEffect, iPrio, iTimerInterval, *pvVal1, *pvVal2, *pvVal3, *pvVal4);
	if (r == (C4Effect *)C4Fx_Effect_Deny) return C4VInt(C4Fx_Effect_Deny);
	if (r == (C4Effect *)C4Fx_Effect_Annul) return C4VInt(C4Fx_Effect_Annul);
	return C4VPropList(r);
}

static long FnGetEffectCount(C4AulContext *ctx, C4String *psEffectName, C4Object *pTarget, long iMaxPriority)
{
	// evaluate parameters
	const char *szEffect = FnStringPar(psEffectName);
	// get effects
	C4Effect *pEffect = pTarget ? pTarget->pEffects : Game.pGlobalEffects;
	if (!pEffect) return false;
	// count effects
	if (!*szEffect) szEffect = 0;
	return pEffect->GetCount(szEffect, iMaxPriority);
}

static C4Value FnEffectCall_C4V(C4AulContext *ctx, C4Value *pvpTarget, C4Value *pvEffect, C4Value *pvsCallFn, C4Value *pvVal1, C4Value *pvVal2, C4Value *pvVal3, C4Value *pvVal4, C4Value *pvVal5, C4Value *pvVal6, C4Value *pvVal7)
{
	// evaluate parameters
	C4String *psCallFn = pvsCallFn->getStr();
	C4Object *pTarget = pvpTarget->getObj();
	C4Effect * pEffect = pvEffect->getPropList() ? pvEffect->getPropList()->GetEffect() : 0;
	const char *szCallFn = FnStringPar(psCallFn);
	// safety
	if (pTarget && !pTarget->Status) return C4Value();
	if (!szCallFn || !*szCallFn) return C4Value();
	if (!pEffect) return C4Value();
	// do call
	return pEffect->DoCall(pTarget, szCallFn, *pvVal1, *pvVal2, *pvVal3, *pvVal4, *pvVal5, *pvVal6, *pvVal7);
}

static bool FnSetViewOffset(C4AulContext *ctx, long iPlayer, long iX, long iY)
{
	if (!ValidPlr(iPlayer)) return 0;
	// get player viewport
	C4Viewport *pView = ::Viewports.GetViewport(iPlayer);
	if (!pView) return 1; // sync safety
	// set
	pView->ViewOffsX = iX;
	pView->ViewOffsY = iY;
	// ok
	return 1;
}

static bool FnSetPreSend(C4AulContext *cthr, long iToVal, C4String *pNewName)
{
	if (!::Control.isNetwork()) return true;
	// dbg: manual presend
	const char *szClient = FnStringPar(pNewName);
	if (!szClient || !*szClient || WildcardMatch(szClient, Game.Clients.getLocalName()))
	{
		::Control.Network.setTargetFPS(iToVal);
		::GraphicsSystem.FlashMessage(FormatString("TargetFPS: %ld", iToVal).getData());
	}
	return true;
}

static long FnGetPlayerID(C4AulContext *cthr, long iPlayer)
{
	C4Player *pPlr = ::Players.Get(iPlayer);
	return pPlr ? pPlr->ID : 0;
}

static long FnGetPlayerTeam(C4AulContext *cthr, long iPlayer)
{
	// get player
	C4Player *pPlr = ::Players.Get(iPlayer);
	if (!pPlr) return 0;
	// search team containing this player
	C4Team *pTeam = Game.Teams.GetTeamByPlayerID(pPlr->ID);
	if (pTeam) return pTeam->GetID();
	// special value of -1 indicating that the team is still to be chosen
	if (pPlr->IsChosingTeam()) return -1;
	// No team.
	return 0;
}

static bool FnSetPlayerTeam(C4AulContext *cthr, long iPlayer, long idNewTeam, bool fNoCalls)
{
	// no team changing in league games
	if (Game.Parameters.isLeague()) return false;
	// get player
	C4Player *pPlr = ::Players.Get(iPlayer);
	if (!pPlr) return false;
	C4PlayerInfo *pPlrInfo = pPlr->GetInfo();
	if (!pPlrInfo) return false;
	// already in that team?
	if (pPlr->Team == idNewTeam) return true;
	// ask team setting if it's allowed (also checks for valid team)
	if (!Game.Teams.IsJoin2TeamAllowed(idNewTeam)) return false;
	// ask script if it's allowed
	if (!fNoCalls)
	{
		if (!!::GameScript.GRBroadcast(PSF_RejectTeamSwitch, &C4AulParSet(C4VInt(iPlayer), C4VInt(idNewTeam)), true, true))
			return false;
	}
	// exit previous team
	C4Team *pOldTeam = Game.Teams.GetTeamByPlayerID(pPlr->ID);
	int32_t idOldTeam = 0;
	if (pOldTeam)
	{
		idOldTeam = pOldTeam->GetID();
		pOldTeam->RemovePlayerByID(pPlr->ID);
	}
	// enter new team
	if (idNewTeam)
	{
		C4Team *pNewTeam = Game.Teams.GetGenerateTeamByID(idNewTeam);
		if (pNewTeam)
		{
			pNewTeam->AddPlayer(*pPlrInfo, true);
			idNewTeam = pNewTeam->GetID();
		}
		else
		{
			// unknown error
			pPlr->Team = idNewTeam = 0;
		}
	}
	// update hositlities if this is not a "silent" change
	if (!fNoCalls)
	{
		pPlr->SetTeamHostility();
	}
	// do callback to reflect change in scenario
	if (!fNoCalls)
		::GameScript.GRBroadcast(PSF_OnTeamSwitch, &C4AulParSet(C4VInt(iPlayer), C4VInt(idNewTeam), C4VInt(idOldTeam)), true);
	return true;
}

static long FnGetTeamConfig(C4AulContext *cthr, long iConfigValue)
{
	// query value
	switch (iConfigValue)
	{
	case C4TeamList::TEAM_Custom:               return Game.Teams.IsCustom();
	case C4TeamList::TEAM_Active:               return Game.Teams.IsMultiTeams();
	case C4TeamList::TEAM_AllowHostilityChange: return Game.Teams.IsHostilityChangeAllowed();
	case C4TeamList::TEAM_Dist:                 return Game.Teams.GetTeamDist();
	case C4TeamList::TEAM_AllowTeamSwitch:      return Game.Teams.IsTeamSwitchAllowed();
	case C4TeamList::TEAM_AutoGenerateTeams:    return Game.Teams.IsAutoGenerateTeams();
	case C4TeamList::TEAM_TeamColors:           return Game.Teams.IsTeamColors();
	}
	// undefined value
	DebugLogF("GetTeamConfig: Unknown config value: %ld", iConfigValue);
	return 0;
}

static C4String *FnGetTeamName(C4AulContext *cthr, long iTeam)
{
	C4Team *pTeam = Game.Teams.GetTeamByID(iTeam);
	if (!pTeam) return NULL;
	return String(pTeam->GetName());
}

static long FnGetTeamColor(C4AulContext *cthr, long iTeam)
{
	C4Team *pTeam = Game.Teams.GetTeamByID(iTeam);
	return pTeam ? pTeam->GetColor() : 0u;
}

static long FnGetTeamByIndex(C4AulContext *cthr, long iIndex)
{
	C4Team *pTeam = Game.Teams.GetTeamByIndex(iIndex);
	return pTeam ? pTeam->GetID() : 0;
}

static long FnGetTeamCount(C4AulContext *cthr)
{
	return Game.Teams.GetTeamCount();
}

static bool FnInitScenarioPlayer(C4AulContext *cthr, long iPlayer, long idTeam)
{
	C4Player *pPlr = ::Players.Get(iPlayer);
	if (!pPlr) return false;
	return pPlr->ScenarioAndTeamInit(idTeam);
}

static bool FnSetScoreboardData(C4AulContext *cthr, long iRowID, long iColID, C4String *pText, long iData)
{
	Game.Scoreboard.SetCell(iColID, iRowID, pText ? pText->GetCStr() : NULL, iData);
	return true;
}

static C4String *FnGetScoreboardString(C4AulContext *cthr, long iRowID, long iColID)
{
	return String(Game.Scoreboard.GetCellString(iColID, iRowID));
}

static int32_t FnGetScoreboardData(C4AulContext *cthr, long iRowID, long iColID)
{
	return Game.Scoreboard.GetCellData(iColID, iRowID);
}

static bool FnDoScoreboardShow(C4AulContext *cthr, long iChange, long iForPlr)
{
	C4Player *pPlr;
	if (iForPlr)
	{
		// abort if the specified player is not local - but always return if the player exists,
		// to ensure sync safety
		if (!(pPlr = ::Players.Get(iForPlr-1))) return false;
		if (!pPlr->LocalControl) return true;
	}
	Game.Scoreboard.DoDlgShow(iChange, false);
	return true; //Game.Scoreboard.ShouldBeShown();
}

static bool FnSortScoreboard(C4AulContext *cthr, long iByColID, bool fReverse)
{
	return Game.Scoreboard.SortBy(iByColID, !!fReverse);
}

static bool FnAddEvaluationData(C4AulContext *cthr, C4String *pText, long idPlayer)
{
	// safety
	if (!pText) return false;
	if (!pText->GetCStr()) return false;
	if (idPlayer && !Game.PlayerInfos.GetPlayerInfoByID(idPlayer)) return false;
	// add data
	Game.RoundResults.AddCustomEvaluationString(pText->GetCStr(), idPlayer);
	return true;
}

static C4Void FnHideSettlementScoreInEvaluation(C4AulContext *cthr, bool fHide)
{
	Game.RoundResults.HideSettlementScore(fHide);
	return C4Void();
}

static long FnActivateGameGoalMenu(C4AulContext *ctx, long iPlayer)
{
	// get target player
	C4Player *pPlr = ::Players.Get(iPlayer);
	if (!pPlr) return false;
	// open menu
	return pPlr->Menu.ActivateGoals(pPlr->Number, pPlr->LocalControl && !::Control.isReplay());
}

static bool FnPlayVideo(C4AulContext *ctx, C4String *pFilename)
{
	// filename must be valid
	if (!pFilename || !pFilename->GetCStr()) return false;
	// play it!
	return Game.VideoPlayer.PlayVideo(pFilename->GetCStr());
}

static bool FnCustomMessage(C4AulContext *ctx, C4String *pMsg, C4Object *pObj, Nillable<long> iOwner, long iOffX, long iOffY, long dwClr, C4ID idDeco, C4PropList *pSrc, long dwFlags, long iHSize)
{
	// safeties
	if (pSrc)
		if(!pSrc->GetDef() && !pSrc->GetObject()) return false;
	if (!pMsg) return false;
	if (pObj && !pObj->Status) return false;
	const char *szMsg = pMsg->GetCStr();
	if (!szMsg) return false;
	if (idDeco && !C4Id2Def(idDeco)) return false;
	if (iOwner.IsNil()) iOwner = NO_OWNER;
	// only one positioning flag per direction allowed
	uint32_t hpos = dwFlags & (C4GM_Left | C4GM_HCenter | C4GM_Right);
	uint32_t vpos = dwFlags & (C4GM_Top | C4GM_VCenter | C4GM_Bottom);
	if (((hpos | (hpos-1)) + 1)>>1 != hpos)
	{
		throw new C4AulExecError(ctx->Obj, "CustomMessage: Only one horizontal positioning flag allowed");
	}
	if (((vpos | (vpos-1)) + 1)>>1 != vpos)
	{
		throw new C4AulExecError(ctx->Obj, "CustomMessage: Only one vertical positioning flag allowed");
	}
	// message color
	if (!dwClr) dwClr = 0xffffffff;
	else dwClr = (dwClr&0xffffff) | (0xff000000u - uint32_t(dwClr|0xff000000)); // message internals use inverted alpha channel
	// message type
	int32_t iType;
	if (pObj)
		if (iOwner != NO_OWNER)
			iType = C4GM_TargetPlayer;
		else
			iType = C4GM_Target;
	else if (iOwner != NO_OWNER)
		iType = C4GM_GlobalPlayer;
	else
		iType = C4GM_Global;
	// remove speech?
	StdStrBuf sMsg;
	sMsg.Ref(szMsg);
	if (dwFlags & C4GM_DropSpeech) sMsg.SplitAtChar('$', NULL);
	// create it!
	return ::Messages.New(iType,sMsg,pObj,iOwner,iOffX,iOffY,(uint32_t)dwClr, idDeco, pSrc, dwFlags, iHSize);
}

/*static long FnSetSaturation(C4AulContext *ctx, long s)
  {
  return lpDDraw->SetSaturation(BoundBy(s,0l,255l));
  }*/

static bool FnPauseGame(C4AulContext *ctx, bool fToggle)
{
	// not in replay (film)
	if (::Control.isReplay()) return true;
	// script method for halting game (for films)
	if (fToggle)
		Console.TogglePause();
	else
		Console.DoHalt();
	return true;
}

static bool FnSetNextMission(C4AulContext *ctx, C4String *szNextMission, C4String *szNextMissionText, C4String *szNextMissionDesc)
{
	if (!szNextMission || !szNextMission->GetData().getLength())
	{
		// param empty: clear next mission
		Game.NextMission.Clear();
		Game.NextMissionText.Clear();
	}
	else
	{
		// set next mission, button and button desc if given
		Game.NextMission.Copy(szNextMission->GetData());
		if (szNextMissionText && szNextMissionText->GetCStr())
		{
			Game.NextMissionText.Copy(szNextMissionText->GetData());
		}
		else
		{
			Game.NextMissionText.Copy(LoadResStr("IDS_BTN_NEXTMISSION"));
		}
		if (szNextMissionDesc && szNextMissionDesc->GetCStr())
		{
			Game.NextMissionDesc.Copy(szNextMissionDesc->GetData());
		}
		else
		{
			Game.NextMissionDesc.Copy(LoadResStr("IDS_DESC_NEXTMISSION"));
		}
	}
	return true;
}

static long FnGetPlayerControlState(C4AulContext *ctx, long iPlr, long iControl)
{
	// get control set to check
	C4PlayerControl *pCheckCtrl = NULL;
	if (iPlr == NO_OWNER)
	{
		//pCheckCtrl = Game.GlobalPlayerControls;
	}
	else
	{
		C4Player *pPlr = ::Players.Get(iPlr);
		if (pPlr)
		{
			pCheckCtrl = &(pPlr->Control);
		}
	}
	// invalid player or no controls
	if (!pCheckCtrl) return 0;
	// query control
	const C4PlayerControl::CSync::ControlDownState *pControlState = pCheckCtrl->GetControlDownState(iControl);
	// no state means not down
	if (!pControlState) return 0;
	// otherwise take down-value
	return pControlState->DownState.iStrength;
}

static bool FnSetPlayerControlEnabled(C4AulContext *ctx, long iplr, long ctrl, bool is_enabled)
{
	// get control set to check
	C4PlayerControl *plrctrl = NULL;
	if (iplr == NO_OWNER)
	{
		//plrctrl = Game.GlobalPlayerControls;
	}
	else
	{
		C4Player *plr = ::Players.Get(iplr);
		if (plr)
		{
			plrctrl = &(plr->Control);
		}
	}
	// invalid player or no controls
	if (!plrctrl) return false;
	// invalid control
	if (ctrl >= int32_t(Game.PlayerControlDefs.GetCount())) return false;
	// query
	return plrctrl->SetControlDisabled(ctrl, !is_enabled);
}

static bool FnGetPlayerControlEnabled(C4AulContext *ctx, long iplr, long ctrl)
{
	// get control set to check
	C4PlayerControl *plrctrl = NULL;
	if (iplr == NO_OWNER)
	{
		//plrctrl = Game.GlobalPlayerControls;
	}
	else
	{
		C4Player *plr = ::Players.Get(iplr);
		if (plr)
		{
			plrctrl = &(plr->Control);
		}
	}
	// invalid player or no controls
	if (!plrctrl) return false;
	return !plrctrl->IsControlDisabled(ctrl);
}

extern C4ScriptConstDef C4ScriptGameConstMap[];
extern C4ScriptFnDef C4ScriptGameFnMap[];

void InitGameFunctionMap(C4AulScriptEngine *pEngine)
{
	// add all def constants (all Int)
	for (C4ScriptConstDef *pCDef = &C4ScriptGameConstMap[0]; pCDef->Identifier; pCDef++)
	{
		assert(pCDef->ValType == C4V_Int); // only int supported currently
		pEngine->RegisterGlobalConstant(pCDef->Identifier, C4VInt(pCDef->Data));
	}
	// add all def script funcs
	for (C4ScriptFnDef *pDef = &C4ScriptGameFnMap[0]; pDef->Identifier; pDef++)
		pEngine->AddFunc(pDef->Identifier, pDef);
//  AddFunc(pEngine, "SetSaturation", FnSetSaturation); //public: 0
	AddFunc(pEngine, "Smoke", FnSmoke);

	AddFunc(pEngine, "GetX", FnGetX);
	AddFunc(pEngine, "GetY", FnGetY);
	AddFunc(pEngine, "GetDefinition", FnGetDefinition);
	AddFunc(pEngine, "GetPlayerName", FnGetPlayerName);
	AddFunc(pEngine, "GetPlayerType", FnGetPlayerType);
	AddFunc(pEngine, "GetPlayerColor", FnGetPlayerColor);
	AddFunc(pEngine, "CreateObject", FnCreateObject);
	AddFunc(pEngine, "CreateConstruction", FnCreateConstruction);
	AddFunc(pEngine, "FindConstructionSite", FnFindConstructionSite);
	AddFunc(pEngine, "Sound", FnSound);
	AddFunc(pEngine, "Music", FnMusic);
	AddFunc(pEngine, "MusicLevel", FnMusicLevel);
	AddFunc(pEngine, "SetPlayList", FnSetPlayList);
	AddFunc(pEngine, "SetPlrView", FnSetPlrView);
	AddFunc(pEngine, "SetPlrKnowledge", FnSetPlrKnowledge);
	AddFunc(pEngine, "GetPlrViewMode", FnGetPlrViewMode);
	AddFunc(pEngine, "ResetCursorView", FnResetCursorView);
	AddFunc(pEngine, "GetPlrView", FnGetPlrView);
	AddFunc(pEngine, "GetWealth", FnGetWealth);
	AddFunc(pEngine, "SetWealth", FnSetWealth);
	AddFunc(pEngine, "DoPlayerScore", FnDoPlayerScore);
	AddFunc(pEngine, "GetPlayerScore", FnGetPlayerScore);
	AddFunc(pEngine, "GetPlayerScoreGain", FnGetPlayerScoreGain);
	AddFunc(pEngine, "GetWind", FnGetWind);
	AddFunc(pEngine, "SetWind", FnSetWind);
	AddFunc(pEngine, "GetTemperature", FnGetTemperature);
	AddFunc(pEngine, "SetTemperature", FnSetTemperature);
	AddFunc(pEngine, "ShakeFree", FnShakeFree);
	AddFunc(pEngine, "DigFree", FnDigFree);
	AddFunc(pEngine, "DigFreeRect", FnDigFreeRect);
	AddFunc(pEngine, "ClearFreeRect", FnClearFreeRect);
	AddFunc(pEngine, "Hostile", FnHostile);
	AddFunc(pEngine, "SetHostility", FnSetHostility);
	AddFunc(pEngine, "PlaceVegetation", FnPlaceVegetation);
	AddFunc(pEngine, "PlaceAnimal", FnPlaceAnimal);
	AddFunc(pEngine, "GameOver", FnGameOver);
	AddFunc(pEngine, "ScriptGo", FnScriptGo);
	AddFunc(pEngine, "GetHiRank", FnGetHiRank);
	AddFunc(pEngine, "GetCrew", FnGetCrew);
	AddFunc(pEngine, "GetCrewCount", FnGetCrewCount);
	AddFunc(pEngine, "GetPlayerCount", FnGetPlayerCount);
	AddFunc(pEngine, "GetPlayerByIndex", FnGetPlayerByIndex);
	AddFunc(pEngine, "EliminatePlayer", FnEliminatePlayer);
	AddFunc(pEngine, "SurrenderPlayer", FnSurrenderPlayer);
	AddFunc(pEngine, "SetLeaguePerformance", FnSetLeaguePerformance);
	AddFunc(pEngine, "CreateScriptPlayer", FnCreateScriptPlayer);
	AddFunc(pEngine, "GetCursor", FnGetCursor);
	AddFunc(pEngine, "GetViewCursor", FnGetViewCursor);
	AddFunc(pEngine, "SetCursor", FnSetCursor);
	AddFunc(pEngine, "SetViewCursor", FnSetViewCursor);
	AddFunc(pEngine, "GetMaterial", FnGetMaterial);
	AddFunc(pEngine, "GetTexture", FnGetTexture);
	AddFunc(pEngine, "GetAverageTextureColor", FnGetAverageTextureColor);
	AddFunc(pEngine, "GetMaterialCount", FnGetMaterialCount);
	AddFunc(pEngine, "GBackSolid", FnGBackSolid);
	AddFunc(pEngine, "GBackSemiSolid", FnGBackSemiSolid);
	AddFunc(pEngine, "GBackLiquid", FnGBackLiquid);
	AddFunc(pEngine, "GBackSky", FnGBackSky);
	AddFunc(pEngine, "Material", FnMaterial);
	AddFunc(pEngine, "BlastFree", FnBlastFree);
	AddFunc(pEngine, "InsertMaterial", FnInsertMaterial);
	AddFunc(pEngine, "LandscapeWidth", FnLandscapeWidth);
	AddFunc(pEngine, "LandscapeHeight", FnLandscapeHeight);
	AddFunc(pEngine, "SetSeason", FnSetSeason);
	AddFunc(pEngine, "GetSeason", FnGetSeason);
	AddFunc(pEngine, "SetClimate", FnSetClimate);
	AddFunc(pEngine, "GetClimate", FnGetClimate);
	AddFunc(pEngine, "SetPlayerZoomByViewRange", FnSetPlayerZoomByViewRange);
	AddFunc(pEngine, "SetPlayerViewLock", FnSetPlayerViewLock);
	AddFunc(pEngine, "DoHomebaseMaterial", FnDoHomebaseMaterial);
	AddFunc(pEngine, "DoHomebaseProduction", FnDoHomebaseProduction);
	AddFunc(pEngine, "GainMissionAccess", FnGainMissionAccess);
	AddFunc(pEngine, "IsNetwork", FnIsNetwork);
	AddFunc(pEngine, "GetLeague", FnGetLeague);
	AddFunc(pEngine, "TestMessageBoard", FnTestMessageBoard, false);
	AddFunc(pEngine, "CallMessageBoard", FnCallMessageBoard, false);
	AddFunc(pEngine, "AbortMessageBoard", FnAbortMessageBoard, false);
	AddFunc(pEngine, "OnMessageBoardAnswer", FnOnMessageBoardAnswer, false);
	AddFunc(pEngine, "ScriptCounter", FnScriptCounter);
	AddFunc(pEngine, "SetFoW", FnSetFoW);
	AddFunc(pEngine, "SetMaxPlayer", FnSetMaxPlayer);
	AddFunc(pEngine, "ActivateGameGoalMenu", FnActivateGameGoalMenu);
	AddFunc(pEngine, "Object", FnObject);
	AddFunc(pEngine, "GetTime", FnGetTime);
	AddFunc(pEngine, "GetMissionAccess", FnGetMissionAccess);
	AddFunc(pEngine, "MaterialName", FnMaterialName);
	AddFunc(pEngine, "GetNeededMatStr", FnGetNeededMatStr);
	AddFunc(pEngine, "DrawMap", FnDrawMap);
	AddFunc(pEngine, "DrawDefMap", FnDrawDefMap);
	AddFunc(pEngine, "CreateParticle", FnCreateParticle);
	AddFunc(pEngine, "CastParticles", FnCastParticles);
	AddFunc(pEngine, "CastBackParticles", FnCastBackParticles);
	AddFunc(pEngine, "PushParticles", FnPushParticles);
	AddFunc(pEngine, "ClearParticles", FnClearParticles);
	AddFunc(pEngine, "SetSkyAdjust", FnSetSkyAdjust);
	AddFunc(pEngine, "SetMatAdjust", FnSetMatAdjust);
	AddFunc(pEngine, "GetSkyAdjust", FnGetSkyAdjust);
	AddFunc(pEngine, "GetMatAdjust", FnGetMatAdjust);
	AddFunc(pEngine, "SetSkyParallax", FnSetSkyParallax);
	AddFunc(pEngine, "ReloadDef", FnReloadDef);
	AddFunc(pEngine, "ReloadParticle", FnReloadParticle);
	AddFunc(pEngine, "SetGamma", FnSetGamma);
	AddFunc(pEngine, "ResetGamma", FnResetGamma);
	AddFunc(pEngine, "FrameCounter", FnFrameCounter);
	AddFunc(pEngine, "SetLandscapePixel", FnSetLandscapePixel);
	AddFunc(pEngine, "DrawMaterialQuad", FnDrawMaterialQuad);
	AddFunc(pEngine, "SetFilmView", FnSetFilmView);
	AddFunc(pEngine, "AddMsgBoardCmd", FnAddMsgBoardCmd);
	AddFunc(pEngine, "SetGameSpeed", FnSetGameSpeed, false);
	AddFunc(pEngine, "DrawMatChunks", FnDrawMatChunks, false);
	AddFunc(pEngine, "GetPathLength", FnGetPathLength);
	AddFunc(pEngine, "SetTextureIndex", FnSetTextureIndex);
	AddFunc(pEngine, "RemoveUnusedTexMapEntries", FnRemoveUnusedTexMapEntries);
	AddFunc(pEngine, "SimFlight", FnSimFlight);
	AddFunc(pEngine, "LoadScenarioSection", FnLoadScenarioSection);
	AddFunc(pEngine, "RemoveEffect", FnRemoveEffect);
	AddFunc(pEngine, "GetEffect", FnGetEffect);
	AddFunc(pEngine, "SetViewOffset", FnSetViewOffset);
	AddFunc(pEngine, "SetPreSend", FnSetPreSend, false);
	AddFunc(pEngine, "GetPlayerID", FnGetPlayerID);
	AddFunc(pEngine, "GetPlayerTeam", FnGetPlayerTeam);
	AddFunc(pEngine, "SetPlayerTeam", FnSetPlayerTeam);
	AddFunc(pEngine, "GetTeamConfig", FnGetTeamConfig);
	AddFunc(pEngine, "GetTeamName", FnGetTeamName);
	AddFunc(pEngine, "GetTeamColor", FnGetTeamColor);
	AddFunc(pEngine, "GetTeamByIndex", FnGetTeamByIndex);
	AddFunc(pEngine, "GetTeamCount", FnGetTeamCount);
	AddFunc(pEngine, "InitScenarioPlayer", FnInitScenarioPlayer, false);
	AddFunc(pEngine, "SetScoreboardData", FnSetScoreboardData);
	AddFunc(pEngine, "GetScoreboardString", FnGetScoreboardString, false);
	AddFunc(pEngine, "GetScoreboardData", FnGetScoreboardData, false);
	AddFunc(pEngine, "DoScoreboardShow", FnDoScoreboardShow);
	AddFunc(pEngine, "SortScoreboard", FnSortScoreboard);
	AddFunc(pEngine, "AddEvaluationData", FnAddEvaluationData);
	AddFunc(pEngine, "HideSettlementScoreInEvaluation", FnHideSettlementScoreInEvaluation);
	AddFunc(pEngine, "ExtractMaterialAmount", FnExtractMaterialAmount);
	AddFunc(pEngine, "GetEffectCount", FnGetEffectCount);
	AddFunc(pEngine, "PlayVideo", FnPlayVideo, false);
	AddFunc(pEngine, "CustomMessage", FnCustomMessage);
	AddFunc(pEngine, "PauseGame", FnPauseGame, false);
	AddFunc(pEngine, "PathFree", FnPathFree);
	AddFunc(pEngine, "PathFree2", FnPathFree2);
	AddFunc(pEngine, "SetNextMission", FnSetNextMission);
	AddFunc(pEngine, "GetPlayerControlState", FnGetPlayerControlState);
	AddFunc(pEngine, "SetPlayerControlEnabled", FnSetPlayerControlEnabled);
	AddFunc(pEngine, "GetPlayerControlEnabled", FnGetPlayerControlEnabled);

	AddFunc(pEngine, "goto", Fn_goto);
	AddFunc(pEngine, "IncinerateLandscape", FnIncinerateLandscape);
	AddFunc(pEngine, "GetGravity", FnGetGravity);
	AddFunc(pEngine, "SetGravity", FnSetGravity);
}

C4ScriptConstDef C4ScriptGameConstMap[]=
{
	{ "FX_OK"                     ,C4V_Int,      C4Fx_OK                    }, // generic standard behaviour for all effect callbacks
	{ "FX_Effect_Deny"            ,C4V_Int,      C4Fx_Effect_Deny           }, // delete effect
	{ "FX_Effect_Annul"           ,C4V_Int,      C4Fx_Effect_Annul          }, // delete effect, because it has annulled a countereffect
	{ "FX_Effect_AnnulDoCalls"    ,C4V_Int,      C4Fx_Effect_AnnulCalls     }, // delete effect, because it has annulled a countereffect; temp readd countereffect
	{ "FX_Execute_Kill"           ,C4V_Int,      C4Fx_Execute_Kill          }, // execute callback: Remove effect now
	{ "FX_Stop_Deny"              ,C4V_Int,      C4Fx_Stop_Deny             }, // deny effect removal
	{ "FX_Start_Deny"             ,C4V_Int,      C4Fx_Start_Deny            }, // deny effect start

	{ "FX_Call_Normal"            ,C4V_Int,      C4FxCall_Normal            }, // normal call; effect is being added or removed
	{ "FX_Call_Temp"              ,C4V_Int,      C4FxCall_Temp              }, // temp call; effect is being added or removed in responce to a lower-level effect change
	{ "FX_Call_TempAddForRemoval" ,C4V_Int,      C4FxCall_TempAddForRemoval }, // temp call; effect is being added because it had been temp removed and is now removed forever
	{ "FX_Call_RemoveClear"       ,C4V_Int,      C4FxCall_RemoveClear       }, // effect is being removed because object is being removed
	{ "FX_Call_RemoveDeath"       ,C4V_Int,      C4FxCall_RemoveDeath       }, // effect is being removed because object died - return -1 to avoid removal
	{ "FX_Call_DmgScript"         ,C4V_Int,      C4FxCall_DmgScript         }, // damage through script call
	{ "FX_Call_DmgBlast"          ,C4V_Int,      C4FxCall_DmgBlast          }, // damage through blast
	{ "FX_Call_DmgFire"           ,C4V_Int,      C4FxCall_DmgFire           }, // damage through fire
	{ "FX_Call_Energy"            ,C4V_Int,      32                         }, // bitmask for generic energy loss
	{ "FX_Call_EngScript"         ,C4V_Int,      C4FxCall_EngScript         }, // energy loss through script call
	{ "FX_Call_EngBlast"          ,C4V_Int,      C4FxCall_EngBlast          }, // energy loss through blast
	{ "FX_Call_EngObjHit"         ,C4V_Int,      C4FxCall_EngObjHit         }, // energy loss through object hitting the living
	{ "FX_Call_EngFire"           ,C4V_Int,      C4FxCall_EngFire           }, // energy loss through fire
	{ "FX_Call_EngBaseRefresh"    ,C4V_Int,      C4FxCall_EngBaseRefresh    }, // energy reload in base (also by base object, but that's normally not called)
	{ "FX_Call_EngAsphyxiation"   ,C4V_Int,      C4FxCall_EngAsphyxiation   }, // energy loss through asphyxiaction
	{ "FX_Call_EngCorrosion"      ,C4V_Int,      C4FxCall_EngCorrosion      }, // energy loss through corrosion (acid)
	{ "FX_Call_EngGetPunched"     ,C4V_Int,      C4FxCall_EngGetPunched     }, // energy loss from punch

	{ "NO_OWNER"                  ,C4V_Int,      NO_OWNER                   }, // invalid player number

	// material density
	{ "C4M_Vehicle"               ,C4V_Int,      C4M_Vehicle                },
	{ "C4M_Solid"                 ,C4V_Int,      C4M_Solid                  },
	{ "C4M_SemiSolid"             ,C4V_Int,      C4M_SemiSolid              },
	{ "C4M_Liquid"                ,C4V_Int,      C4M_Liquid                 },
	{ "C4M_Background"            ,C4V_Int,      C4M_Background             },

	// scoreboard
	{ "SBRD_Caption"              ,C4V_Int,      C4Scoreboard::TitleKey     }, // used to set row/coloumn headers

	// teams - constants for GetTeamConfig
	{ "TEAM_Custom"               ,C4V_Int,      C4TeamList::TEAM_Custom               },
	{ "TEAM_Active"               ,C4V_Int,      C4TeamList::TEAM_Active               },
	{ "TEAM_AllowHostilityChange" ,C4V_Int,      C4TeamList::TEAM_AllowHostilityChange },
	{ "TEAM_Dist"                 ,C4V_Int,      C4TeamList::TEAM_Dist                 },
	{ "TEAM_AllowTeamSwitch"      ,C4V_Int,      C4TeamList::TEAM_AllowTeamSwitch      },
	{ "TEAM_AutoGenerateTeams"    ,C4V_Int,      C4TeamList::TEAM_AutoGenerateTeams    },
	{ "TEAM_TeamColors"           ,C4V_Int,      C4TeamList::TEAM_TeamColors           },

	{ "C4FO_Not"                  ,C4V_Int,     C4FO_Not            },
	{ "C4FO_And"                  ,C4V_Int,     C4FO_And            },
	{ "C4FO_Or"                   ,C4V_Int,     C4FO_Or             },
	{ "C4FO_Exclude"              ,C4V_Int,     C4FO_Exclude        },
	{ "C4FO_InRect"               ,C4V_Int,     C4FO_InRect         },
	{ "C4FO_AtPoint"              ,C4V_Int,     C4FO_AtPoint        },
	{ "C4FO_AtRect"               ,C4V_Int,     C4FO_AtRect         },
	{ "C4FO_OnLine"               ,C4V_Int,     C4FO_OnLine         },
	{ "C4FO_Distance"             ,C4V_Int,     C4FO_Distance       },
	{ "C4FO_ID"                   ,C4V_Int,     C4FO_ID             },
	{ "C4FO_OCF"                  ,C4V_Int,     C4FO_OCF            },
	{ "C4FO_Category"             ,C4V_Int,     C4FO_Category       },
	{ "C4FO_Action"               ,C4V_Int,     C4FO_Action         },
	{ "C4FO_ActionTarget"         ,C4V_Int,     C4FO_ActionTarget   },
	{ "C4FO_Procedure"            ,C4V_Int,     C4FO_Procedure          },
	{ "C4FO_Container"            ,C4V_Int,     C4FO_Container      },
	{ "C4FO_AnyContainer"         ,C4V_Int,     C4FO_AnyContainer   },
	{ "C4FO_Owner"                ,C4V_Int,     C4FO_Owner          },
	{ "C4FO_Controller"           ,C4V_Int,     C4FO_Controller         },
	{ "C4FO_Func"                 ,C4V_Int,     C4FO_Func           },
	{ "C4FO_Layer"                ,C4V_Int,     C4FO_Layer          },

	{ "C4SO_Reverse"              ,C4V_Int,     C4SO_Reverse        },
	{ "C4SO_Multiple"             ,C4V_Int,     C4SO_Multiple       },
	{ "C4SO_Distance"             ,C4V_Int,     C4SO_Distance       },
	{ "C4SO_Random"               ,C4V_Int,     C4SO_Random         },
	{ "C4SO_Speed"                ,C4V_Int,     C4SO_Speed          },
	{ "C4SO_Mass"                 ,C4V_Int,     C4SO_Mass           },
	{ "C4SO_Value"                ,C4V_Int,     C4SO_Value          },
	{ "C4SO_Func"                 ,C4V_Int,     C4SO_Func           },

	{ "C4SECT_SaveLandscape"      ,C4V_Int,      C4S_SAVE_LANDSCAPE },
	{ "C4SECT_SaveObjects"        ,C4V_Int,      C4S_SAVE_OBJECTS },
	{ "C4SECT_KeepEffects"        ,C4V_Int,      C4S_KEEP_EFFECTS },

	{ "TEAMID_New"                ,C4V_Int,      TEAMID_New },

	{ "MSG_NoLinebreak"           ,C4V_Int,      C4GM_NoBreak },
	{ "MSG_Bottom"                ,C4V_Int,      C4GM_Bottom },
	{ "MSG_Multiple"              ,C4V_Int,      C4GM_Multiple },
	{ "MSG_Top"                   ,C4V_Int,      C4GM_Top },
	{ "MSG_Left"                  ,C4V_Int,      C4GM_Left },
	{ "MSG_Right"                 ,C4V_Int,      C4GM_Right },
	{ "MSG_HCenter"               ,C4V_Int,      C4GM_HCenter },
	{ "MSG_VCenter"               ,C4V_Int,      C4GM_VCenter },
	{ "MSG_DropSpeech"            ,C4V_Int,      C4GM_DropSpeech },
	{ "MSG_WidthRel"              ,C4V_Int,      C4GM_WidthRel },
	{ "MSG_XRel"                  ,C4V_Int,      C4GM_XRel },
	{ "MSG_YRel"                  ,C4V_Int,      C4GM_YRel },

	{ "C4PT_User"                 ,C4V_Int,      C4PT_User },
	{ "C4PT_Script"               ,C4V_Int,      C4PT_Script },

	{ "CSPF_FixedAttributes"      ,C4V_Int,      CSPF_FixedAttributes },
	{ "CSPF_NoScenarioInit"       ,C4V_Int,      CSPF_NoScenarioInit },
	{ "CSPF_NoEliminationCheck"   ,C4V_Int,      CSPF_NoEliminationCheck },
	{ "CSPF_Invisible"            ,C4V_Int,      CSPF_Invisible },

	{ "DMQ_Sky"                   ,C4V_Int,      DMQ_Sky },
	{ "DMQ_Sub"                   ,C4V_Int,      DMQ_Sub },
	{ "DMQ_Bridge"                ,C4V_Int,      DMQ_Bridge },

	{ "PLRZOOM_Direct"            ,C4V_Int,      PLRZOOM_Direct },
	{ "PLRZOOM_NoIncrease"        ,C4V_Int,      PLRZOOM_NoIncrease },
	{ "PLRZOOM_NoDecrease"        ,C4V_Int,      PLRZOOM_NoDecrease },
	{ "PLRZOOM_LimitMin"          ,C4V_Int,      PLRZOOM_LimitMin },
	{ "PLRZOOM_LimitMax"          ,C4V_Int,      PLRZOOM_LimitMax },

	{ NULL, C4V_Nil, 0}
};

#define MkFnC4V (C4Value (*)(C4AulContext *cthr, C4Value*, C4Value*, C4Value*, C4Value*, C4Value*,\
                                                 C4Value*, C4Value*, C4Value*, C4Value*, C4Value*))

C4ScriptFnDef C4ScriptGameFnMap[]=
{

	{ "PlayerObjectCommand",  1  ,C4V_Bool     ,{ C4V_Int     ,C4V_String  ,C4V_C4Object,C4V_Int     ,C4V_Int     ,C4V_C4Object,C4V_Any    ,C4V_Int    ,C4V_Any    ,C4V_Any}  ,0 ,                                   FnPlayerObjectCommand },
	{ "FindObject",           1  ,C4V_C4Object ,{ C4V_Array   ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,0 ,                                   FnFindObject },
	{ "FindObjects",          1  ,C4V_Array    ,{ C4V_Array   ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,0 ,                                   FnFindObjects },
	{ "ObjectCount",          1  ,C4V_Int      ,{ C4V_Array   ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,0 ,                                   FnObjectCount },
	{ "ProtectedCall",        0  ,C4V_Any      ,{ C4V_C4Object,C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V FnProtectedCall_C4V ,         0 },
	{ "PrivateCall",          0  ,C4V_Any      ,{ C4V_C4Object,C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V FnPrivateCall_C4V ,           0 },
	{ "GameCall",             1  ,C4V_Any      ,{ C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V FnGameCall_C4V ,              0 },
	{ "GameCallEx",           1  ,C4V_Any      ,{ C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V FnGameCallEx_C4V ,            0 },
	{ "DefinitionCall",       0  ,C4V_Any      ,{ C4V_PropList,C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V FnDefinitionCall_C4V ,        0 },
	{ "Call",                 1  ,C4V_Any      ,{ C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V FnCall_C4V ,                  0 },
	{ "GetPlrKnowledge",      1  ,C4V_Int      ,{ C4V_Int     ,C4V_PropList,C4V_Int     ,C4V_Int     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V FnGetPlrKnowledge_C4V ,       0 },
	{ "GetComponent",         1  ,C4V_Int      ,{ C4V_PropList,C4V_Int     ,C4V_C4Object,C4V_PropList,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V FnGetComponent_C4V ,          0 },
	{ "PlayerMessage",        1  ,C4V_Int      ,{ C4V_Int     ,C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V &FnPlayerMessage_C4V,         0 },
	{ "Message",              1  ,C4V_Bool     ,{ C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V &FnMessage_C4V,               0 },
	{ "AddMessage",           1  ,C4V_Bool     ,{ C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V &FnAddMessage_C4V,            0 },
	{ "EditCursor",           1  ,C4V_C4Object ,{ C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,0 ,                                   FnEditCursor },
	{ "GetHomebaseMaterial",  1  ,C4V_Int      ,{ C4V_Int     ,C4V_PropList,C4V_Int     ,C4V_Int     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V FnGetHomebaseMaterial_C4V ,   0 },
	{ "GetHomebaseProduction",1  ,C4V_Int      ,{ C4V_Int     ,C4V_PropList,C4V_Int     ,C4V_Int     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V FnGetHomebaseProduction_C4V , 0 },

	{ "GetDefCoreVal",        1  ,C4V_Any      ,{ C4V_String  ,C4V_String  ,C4V_Int     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnGetDefCoreVal,             0 },
	{ "GetObjectVal",         1  ,C4V_Any      ,{ C4V_String  ,C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnGetObjectVal,              0 },
	{ "GetObjectInfoCoreVal", 1  ,C4V_Any      ,{ C4V_String  ,C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnGetObjectInfoCoreVal,      0 },
	{ "GetScenarioVal",       1  ,C4V_Any      ,{ C4V_String  ,C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnGetScenarioVal,            0 },
	{ "GetPlayerVal",         1  ,C4V_Any      ,{ C4V_String  ,C4V_String  ,C4V_Int     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnGetPlayerVal,              0 },
	{ "GetPlayerInfoCoreVal", 1  ,C4V_Any      ,{ C4V_String  ,C4V_String  ,C4V_Int     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnGetPlayerInfoCoreVal,      0 },
	{ "GetMaterialVal",       1  ,C4V_Any      ,{ C4V_String  ,C4V_String  ,C4V_Int     ,C4V_Int     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnGetMaterialVal,            0 },

	{ "SetPlrExtraData",      1  ,C4V_Any      ,{ C4V_Int     ,C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnSetPlrExtraData,           0 },
	{ "GetPlrExtraData",      1  ,C4V_Any      ,{ C4V_Int     ,C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnGetPlrExtraData,           0 },
	{ "AddEffect",            1  ,C4V_PropList ,{ C4V_String  ,C4V_C4Object,C4V_Int     ,C4V_Int     ,C4V_C4Object,C4V_PropList,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnAddEffect_C4V,             0 },
	{ "CheckEffect",          1  ,C4V_Int      ,{ C4V_String  ,C4V_C4Object,C4V_Int     ,C4V_Int     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnCheckEffect_C4V,           0 },
	{ "EffectCall",           1  ,C4V_Any      ,{ C4V_C4Object,C4V_PropList,C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnEffectCall_C4V,            0 },

	{ NULL,                   0  ,C4V_Nil      ,{ C4V_Nil     ,C4V_Nil     ,C4V_Nil     ,C4V_Nil     ,C4V_Nil     ,C4V_Nil     ,C4V_Nil    ,C4V_Nil    ,C4V_Nil    ,C4V_Nil}   ,0,                                   0 }

};
