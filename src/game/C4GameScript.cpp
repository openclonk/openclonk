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

/* Functions mapped by C4Script */

#include "C4Include.h"
#include "C4ForbidLibraryCompilation.h"
#include "game/C4GameScript.h"

#include "control/C4GameControl.h"
#include "control/C4RoundResults.h"
#include "editor/C4Console.h"
#include "game/C4Application.h"
#include "game/C4GraphicsSystem.h"
#include "game/C4Viewport.h"
#include "graphics/C4GraphicsResource.h"
#include "graphics/C4Shader.h"
#include "gui/C4GameMessage.h"
#include "gui/C4MessageInput.h"
#include "gui/C4MouseControl.h"
#include "gui/C4ScriptGuiWindow.h"
#include "landscape/C4Landscape.h"
#include "landscape/C4PXS.h"
#include "landscape/C4Particles.h"
#include "landscape/C4Sky.h"
#include "landscape/C4Texture.h"
#include "landscape/C4Weather.h"
#include "landscape/fow/C4FoW.h"
#include "object/C4Command.h"
#include "object/C4DefList.h"
#include "object/C4GameObjects.h"
#include "platform/C4GamePadCon.h"
#include "player/C4Player.h"
#include "player/C4PlayerList.h"
#include "script/C4AulDefFunc.h"

C4Effect ** FnGetEffectsFor(C4PropList * pTarget)
{
	if (pTarget)
	{
		if (pTarget == ScriptEngine.GetPropList())
			return &ScriptEngine.pGlobalEffects;
		if (pTarget == GameScript.ScenPropList.getPropList())
			return &GameScript.pScenarioEffects;
		C4Object * Obj = pTarget->GetObject();
		if (!Obj)
			throw C4AulExecError("Effect target has to be an object");
		return &Obj->pEffects;
	}
	return &ScriptEngine.pGlobalEffects;
}

// undocumented!
static bool FnIncinerateLandscape(C4PropList * _this, long iX, long iY, long caused_by_plr)
{
	if (Object(_this)) { iX += Object(_this)->GetX(); iY += Object(_this)->GetY(); }
	return !!::Landscape.Incinerate(iX, iY, caused_by_plr);
}

static void FnSetGravity(C4PropList * _this, long iGravity)
{
	::Landscape.SetGravity(C4REAL100(Clamp<long>(iGravity,-1000,1000)));
}

static long FnGetGravity(C4PropList * _this)
{
	return fixtoi(::Landscape.GetGravity() * 100);
}

static C4String *FnGetPlayerName(C4PropList * _this, long iPlayer)
{
	if (!ValidPlr(iPlayer)) return nullptr;
	return String(::Players.Get(iPlayer)->GetName());
}

static long FnGetPlayerType(C4PropList * _this, long iPlayer)
{
	C4Player *pPlr = ::Players.Get(iPlayer);
	if (!pPlr) return 0;
	return pPlr->GetType();
}

static long FnGetPlayerColor(C4PropList * _this, long iPlayer)
{
	C4Player *plr = ::Players.Get(iPlayer);
	return plr ? plr->ColorDw : 0;
}

// undocumented!
static Nillable<long> FnGetPlrClonkSkin(C4PropList * _this, long iPlayer)
{
	C4Player *plr = ::Players.Get(iPlayer);
	if (plr)
		return plr->PrefClonkSkin;
	return C4Void();
}

static Nillable<long> FnGetX(C4PropList * _this, long iPrec)
{
	if (!Object(_this)) return C4Void();
	if (!iPrec) iPrec = 1;
	return fixtoi(Object(_this)->fix_x, iPrec);
}

static Nillable<long> FnGetY(C4PropList * _this, long iPrec)
{
	if (!Object(_this)) return C4Void();
	if (!iPrec) iPrec = 1;
	return fixtoi(Object(_this)->fix_y, iPrec);
}

static C4Object *FnCreateObjectAbove(C4PropList * _this,
                                C4PropList * PropList, long iXOffset, long iYOffset, Nillable<long> owner)
{
	if (Object(_this)) // Local object calls override
	{
		iXOffset += Object(_this)->GetX();
		iYOffset += Object(_this)->GetY();
	}

	long iOwner = owner;
	if (owner.IsNil())
	{
		if (Object(_this))
			iOwner = Object(_this)->Controller;
		else
			iOwner = NO_OWNER;
	}

	C4Object *pNewObj = Game.CreateObject(PropList, Object(_this), iOwner, iXOffset, iYOffset);

	// Set initial controller to creating controller, so more complicated cause-effect-chains can be traced back to the causing player
	if (pNewObj && Object(_this) && Object(_this)->Controller > NO_OWNER) pNewObj->Controller = Object(_this)->Controller;

	return pNewObj;
}

static C4Object *FnCreateObject(C4PropList * _this,
	C4PropList * PropList, long iXOffset, long iYOffset, Nillable<long> owner)
{
	if (Object(_this)) // Local object calls override
	{
		iXOffset += Object(_this)->GetX();
		iYOffset += Object(_this)->GetY();
	}

	long iOwner = owner;
	if (owner.IsNil())
	{
		if (Object(_this))
			iOwner = Object(_this)->Controller;
		else
			iOwner = NO_OWNER;
	}

	C4Object *pNewObj = Game.CreateObject(PropList, Object(_this), iOwner, iXOffset, iYOffset, 0, true);

	// Set initial controller to creating controller, so more complicated cause-effect-chains can be traced back to the causing player
	if (pNewObj && Object(_this) && Object(_this)->Controller > NO_OWNER) pNewObj->Controller = Object(_this)->Controller;

	return pNewObj;
}


static C4Object *FnCreateConstruction(C4PropList * _this,
                                      C4PropList * PropList, long iXOffset, long iYOffset, Nillable<long> owner,
                                      long iCompletion, bool fTerrain, bool fCheckSite)
{
	// Make sure parameters are valid
	if (!PropList || !PropList->GetDef())
		return nullptr;

	// Local object calls override position offset, owner
	if (Object(_this))
	{
		iXOffset+=Object(_this)->GetX();
		iYOffset+=Object(_this)->GetY();
	}

	// Check site
	if (fCheckSite)
		if (!ConstructionCheck(PropList,iXOffset,iYOffset,Object(_this)))
			return nullptr;

	long iOwner = owner;
	if (owner.IsNil())
	{
		if (Object(_this))
			iOwner = Object(_this)->Controller;
		else
			iOwner = NO_OWNER;
	}

	// Create site object
	C4Object *pNewObj = Game.CreateObjectConstruction(PropList,Object(_this),iOwner,iXOffset,iYOffset,iCompletion*FullCon/100,fTerrain);

	// Set initial controller to creating controller, so more complicated cause-effect-chains can be traced back to the causing player
	if (pNewObj && Object(_this) && Object(_this)->Controller>NO_OWNER) pNewObj->Controller = Object(_this)->Controller;

	return pNewObj;
}

static C4ValueArray *FnFindConstructionSite(C4PropList * _this, C4PropList * PropList, int32_t v1, int32_t v2)
{
	// Get def
	C4Def *pDef;
	if (!(pDef=PropList->GetDef())) return nullptr;
	// Construction check at starting position
	if (ConstructionCheck(PropList,v1,v2))
		return nullptr;
	// Search for real
	bool result = !!FindConSiteSpot(v1, v2, pDef->Shape.Wdt,pDef->Shape.Hgt, 20);
	if(!result) return nullptr;
	auto *pArray = new C4ValueArray(2);
	pArray->SetItem(0, C4VInt(v1));
	pArray->SetItem(1, C4VInt(v2));
	return pArray;
}

static bool FnCheckConstructionSite(C4PropList * _this, C4PropList * PropList, int32_t iXOffset, int32_t iYOffset)
{
	// Make sure parameters are valid
	if (!PropList || !PropList->GetDef())
		return false;

	// Local object calls override position offset, owner
	if (Object(_this))
	{
		iXOffset+=Object(_this)->GetX();
		iYOffset+=Object(_this)->GetY();
	}

	// Check construction site
	return ConstructionCheck(PropList, iXOffset, iYOffset);
}

C4FindObject *CreateCriterionsFromPars(C4Value *pPars, C4FindObject **pFOs, C4SortObject **pSOs, const C4Object *context)
{
	int i, iCnt = 0, iSortCnt = 0;
	bool has_layer_check = false;
	// Read all parameters
	for (i = 0; i < C4AUL_MAX_Par; i++)
	{
		C4Value Data = *(pPars++);
		// No data given?
		if (!Data) break;
		// Construct
		C4SortObject *pSO = nullptr;
		C4FindObject *pFO = C4FindObject::CreateByValue(Data, pSOs ? &pSO : nullptr, context, &has_layer_check);
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
		return nullptr;
	}
	// Implicit layer check
	if (context && !has_layer_check)
	{
		pFOs[iCnt++] = new C4FindObjectLayer(context->Layer);
	}
	// create sort criterion
	C4SortObject *pSO = nullptr;
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

static C4Value FnObjectCount(C4PropList * _this, C4Value *pPars)
{
	// Create FindObject-structure
	C4FindObject *pFOs[C4AUL_MAX_Par+1]; // +1 array element to include space for implicit layer check
	C4FindObject *pFO = CreateCriterionsFromPars(pPars, pFOs, nullptr, Object(_this));
	// Error?
	if (!pFO)
		throw C4AulExecError("ObjectCount: No valid search criterions supplied");
	// Search
	int32_t iCnt = pFO->Count(::Objects, ::Objects.Sectors);
	// Free
	delete pFO;
	// Return
	return C4VInt(iCnt);
}

static C4Value FnFindObject(C4PropList * _this, C4Value *pPars)
{
	// Create FindObject-structure
	C4FindObject *pFOs[C4AUL_MAX_Par]; // +1 array element to include space for implicit layer check
	C4SortObject *pSOs[C4AUL_MAX_Par];
	C4FindObject *pFO = CreateCriterionsFromPars(pPars, pFOs, pSOs, Object(_this));
	// Error?
	if (!pFO)
		throw C4AulExecError("FindObject: No valid search criterions supplied");
	// Search
	C4Object *pObj = pFO->Find(::Objects, ::Objects.Sectors);
	// Free
	delete pFO;
	// Return
	return C4VObj(pObj);
}

static C4Value FnFindObjects(C4PropList * _this, C4Value *pPars)
{
	// Create FindObject-structure
	C4FindObject *pFOs[C4AUL_MAX_Par]; // +1 array element to include space for implicit layer check
	C4SortObject *pSOs[C4AUL_MAX_Par];
	C4FindObject *pFO = CreateCriterionsFromPars(pPars, pFOs, pSOs, Object(_this));
	// Error?
	if (!pFO)
		throw C4AulExecError("FindObjects: No valid search criterions supplied");
	// Search
	C4ValueArray *pResult = pFO->FindMany(::Objects, ::Objects.Sectors);
	// Free
	delete pFO;
	// Return
	return C4VArray(pResult);
}

static bool FnInsertMaterial(C4PropList * _this, long mat, long x, long y, long vx, long vy, C4PropList *insert_position)
{
	if (Object(_this)) { x+=Object(_this)->GetX(); y+=Object(_this)->GetY(); }
	int32_t insert_x=x, insert_y=y;
	if (!::Landscape.InsertMaterial(mat,&insert_x,&insert_y,vx,vy)) return false;
	// output insertion position if desired (may be out of landscape range)
	if (insert_position && !insert_position->IsFrozen())
	{
		insert_position->SetProperty(P_X, C4VInt(insert_x));
		insert_position->SetProperty(P_Y, C4VInt(insert_y));
	}
	return true;
}

static bool FnCanInsertMaterial(C4PropList * _this, long mat, long x, long y, C4PropList *insert_position)
{
	if (Object(_this)) { x+=Object(_this)->GetX(); y+=Object(_this)->GetY(); }
	int32_t insert_x=x, insert_y=y;
	if (!::Landscape.InsertMaterial(mat,&insert_x,&insert_y,0,0,true)) return false;
	// output insertion position if desired
	if (insert_position && !insert_position->IsFrozen())
	{
		insert_position->SetProperty(P_X, C4VInt(insert_x));
		insert_position->SetProperty(P_Y, C4VInt(insert_y));
	}
	return true;
}

static long FnGetMaterialCount(C4PropList * _this, long iMaterial, bool fReal)
{
	if (!MatValid(iMaterial)) return -1;
	if (fReal || !::MaterialMap.Map[iMaterial].MinHeightCount)
		return ::Landscape.GetMatCount(iMaterial);
	else
		return ::Landscape.GetEffectiveMatCount(iMaterial);
}

static long FnGetMaterial(C4PropList * _this, long x, long y)
{
	if (Object(_this)) { x+=Object(_this)->GetX(); y+=Object(_this)->GetY(); }
	return GBackMat(x,y);
}

static long FnGetBackMaterial(C4PropList * _this, long x, long y)
{
	if (Object(_this)) { x+=Object(_this)->GetX(); y+=Object(_this)->GetY(); }
	return ::Landscape.GetBackMat(x, y);
}

static C4String *FnGetTexture(C4PropList * _this, long x, long y)
{
	if (Object(_this)) { x+=Object(_this)->GetX(); y+=Object(_this)->GetY(); }

	// Get texture
	int32_t iTex = PixCol2Tex(::Landscape.GetPix(x, y));
	if (!iTex) return nullptr;
	// Get material-texture mapping
	const C4TexMapEntry *pTex = ::TextureMap.GetEntry(iTex);
	if (!pTex) return nullptr;
	// Return tex name
	return String(pTex->GetTextureName());
}

static C4String *FnGetBackTexture(C4PropList * _this, long x, long y)
{
	if (Object(_this)) { x+=Object(_this)->GetX(); y+=Object(_this)->GetY(); }

	// Get texture
	int32_t iTex = PixCol2Tex(::Landscape.GetBackPix(x, y));
	if (!iTex) return nullptr;
	// Get material-texture mapping
	const C4TexMapEntry *pTex = ::TextureMap.GetEntry(iTex);
	if (!pTex) return nullptr;
	// Return tex name
	return String(pTex->GetTextureName());
}

// Note: Might be async in case of 16<->32 bit textures!
static Nillable<long> FnGetAverageTextureColor(C4PropList * _this, C4String* Texture)
{
	// Safety
	if(!Texture) return C4Void();
	// Check texture
	StdStrBuf texture_name;
	texture_name.Ref(Texture->GetCStr());
	const char* pch = strchr(texture_name.getData(), '-');
	if (pch != nullptr)
	{
		size_t len = pch - texture_name.getData();
		texture_name.Copy();
		texture_name.SetLength(len);
	}
	C4Texture* Tex = ::TextureMap.GetTexture(texture_name.getData());
	if(!Tex) return C4Void();
	return Tex->GetAverageColor();
}

static bool FnGBackSolid(C4PropList * _this, long x, long y)
{
	if (Object(_this)) { x+=Object(_this)->GetX(); y+=Object(_this)->GetY(); }
	return GBackSolid(x,y);
}

static bool FnGBackSemiSolid(C4PropList * _this, long x, long y)
{
	if (Object(_this)) { x+=Object(_this)->GetX(); y+=Object(_this)->GetY(); }
	return GBackSemiSolid(x,y);
}

static bool FnGBackLiquid(C4PropList * _this, long x, long y)
{
	if (Object(_this)) { x+=Object(_this)->GetX(); y+=Object(_this)->GetY(); }
	return GBackLiquid(x,y);
}

static bool FnGBackSky(C4PropList * _this, long x, long y)
{
	if (Object(_this)) { x+=Object(_this)->GetX(); y+=Object(_this)->GetY(); }
	return Landscape.GetBackPix(x, y) == 0;
}

static long FnExtractMaterialAmount(C4PropList * _this, long x, long y, long mat, long amount, bool distant_first)
{
	if (Object(_this)) { x+=Object(_this)->GetX(); y+=Object(_this)->GetY(); }
	long extracted=0; for (; extracted<amount; extracted++)
	{
		if (GBackMat(x,y)!=mat) return extracted;
		if (::Landscape.ExtractMaterial(x,y,distant_first)!=mat) return extracted;
	}
	return extracted;
}

static void FnBlastFree(C4PropList * _this, long iX, long iY, long iLevel, Nillable<long> iCausedBy, Nillable<long> iMaxDensity)
{
	if (iCausedBy.IsNil() && Object(_this)) iCausedBy = Object(_this)->Controller;
	if (iMaxDensity.IsNil()) iMaxDensity = C4M_Vehicle;

	::Landscape.BlastFree(iX, iY, iLevel, iCausedBy, Object(_this), iMaxDensity);
}

static bool FnSoundAt(C4PropList * _this, C4String *szSound, long iX, long iY, Nillable<long> iLevel, Nillable<long> iAtPlayer, long iCustomFalloffDistance, long iPitch, C4PropList *modifier_props)
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
	// modifier from prop list
	C4SoundModifier *modifier;
	if (modifier_props)
		modifier = Application.SoundSystem.Modifiers.Get(modifier_props, true);
	else
		modifier = nullptr;
	// target object
	C4Object *pObj = Object(_this);
	if (pObj)
	{
		iX += pObj->GetX();
		iY += pObj->GetY();
	}
	StartSoundEffectAt(FnStringPar(szSound), iX, iY, iLevel, iCustomFalloffDistance, iPitch, modifier);
	// always return true (network safety!)
	return true;
}

static bool FnSound(C4PropList * _this, C4String *szSound, bool fGlobal, Nillable<long> iLevel, Nillable<long> iAtPlayer, long iLoop, long iCustomFalloffDistance, long iPitch, C4PropList *modifier_props)
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
	// modifier from prop list
	C4SoundModifier *modifier;
	if (modifier_props)
		modifier = Application.SoundSystem.Modifiers.Get(modifier_props, true);
	else
		modifier = nullptr;
	// target object
	C4Object *pObj = nullptr;
	if (!fGlobal) pObj = Object(_this);
	// play/stop?
	if (iLoop >= 0)
	{
		// already playing?
		C4SoundInstance *inst = GetSoundInstance(FnStringPar(szSound), pObj);
		if (inst)
		{
			// then just update parameters
			SoundUpdate(inst, iLevel, iPitch);
		}
		else
		{
			// try to play effect
			StartSoundEffect(FnStringPar(szSound), !!iLoop, iLevel, pObj, iCustomFalloffDistance, iPitch, modifier);
		}
	}
	else
	{
		StopSoundEffect(FnStringPar(szSound), pObj);
	}
	// always return true (network safety!)
	return true;
}

static bool FnChangeSoundModifier(C4PropList * _this, C4PropList *modifier_props, bool release)
{
	// internal function to be used by sound library: Updates sound modifier
	C4SoundModifier *modifier = Application.SoundSystem.Modifiers.Get(modifier_props, false);
	// modifier not found. May be due to savegame resume.
	// In that case, creation/updates will happen automatically next time Sound() is called
	// always return true for sync safety because the internal C4SoundModifierList is not synchronized
	if (!modifier) return true;
	if (release)
		modifier->Release();
	else
		modifier->Update();
	return true;
}

static bool FnSetGlobalSoundModifier(C4PropList * _this, C4PropList *modifier_props, Nillable<long> at_player)
{
	// set modifier to be applied to all future sounds
	if (at_player.IsNil())
	{
		// no player given: Global modifier for all players.
		Game.SetGlobalSoundModifier(modifier_props);
	}
	else
	{
		// modifier for all viewports of a player
		C4Player *plr = ::Players.Get(at_player);
		if (!plr) return false;
		plr->SetSoundModifier(modifier_props);
	}
	// always true on valid params for sync safety
	return true;
}

static bool FnMusic(C4PropList * _this, C4String *szSongname, bool fLoop, long iFadeTime_ms, long max_resume_time_ms)
{
	bool success;
	if (max_resume_time_ms < 0) return false; // Safety
	if (!szSongname)
	{
		success = Application.MusicSystem.Stop();
	}
	else
	{
		success = Application.MusicSystem.Play(FnStringPar(szSongname), !!fLoop, iFadeTime_ms, double(max_resume_time_ms)/1000.0);
	}
	if (::Control.SyncMode()) return true;
	return success;
}

static long FnMusicLevel(C4PropList * _this, long iLevel)
{
	return ::Application.MusicSystem.SetGameMusicLevel(iLevel);
}

static long FnSetPlayList(C4PropList * _this, const C4Value & playlist_data, Nillable<long> iAtPlayer, bool fForceSwitch, long iFadeTime_ms, long max_resume_time_ms)
{
	// Safety
	if (max_resume_time_ms < 0) return 0;
	// If a player number is provided, set play list for clients where given player is local only
	if (!iAtPlayer.IsNil() && iAtPlayer != NO_OWNER)
	{
		C4Player *at_plr = ::Players.Get(iAtPlayer);
		if (!at_plr) return 0;
		if (!at_plr->LocalControl) return 0;
	}
	// Playlist might be a string for the new playlist, a proplist with more info, or nil to reset the playlist
	C4String * szPlayList = playlist_data.getStr();
	C4PropList *playlist_props = nullptr;
	if (!szPlayList)
	{
		playlist_props = playlist_data.getPropList();
		if (playlist_props)
		{
			szPlayList = playlist_props->GetPropertyStr(P_PlayList);
			// Update playlist properties
			C4Value val;
			if (playlist_props->GetProperty(P_MusicBreakMin, &val)) ::Application.MusicSystem.SetMusicBreakMin(val.getInt());
			if (playlist_props->GetProperty(P_MusicBreakMax, &val)) ::Application.MusicSystem.SetMusicBreakMax(val.getInt());
			if (playlist_props->GetProperty(P_MusicBreakChance, &val)) ::Application.MusicSystem.SetMusicBreakChance(val.getInt());
			if (playlist_props->GetProperty(P_MusicMaxPositionMemory, &val)) ::Application.MusicSystem.SetMusicMaxPositionMemory(val.getInt());
		}
	}
	// Set playlist; count entries
	long iFilesInPlayList = ::Application.MusicSystem.SetPlayList(FnStringPar(szPlayList), fForceSwitch, iFadeTime_ms, double(max_resume_time_ms)/1000.0f);
	// network/record/replay: return 0 for sync reasons
	if (::Control.SyncMode()) return 0;
	return iFilesInPlayList;
}

static bool FnGameOver(C4PropList * _this, long iGameOverValue /* provided for future compatibility */)
{
	return !!Game.DoGameOver();
}

static bool FnGainMissionAccess(C4PropList * _this, C4String *szPassword)
{
	if (std::strlen(Config.General.MissionAccess)+std::strlen(FnStringPar(szPassword))+3>CFG_MaxString) return false;
	SAddModule(Config.General.MissionAccess,FnStringPar(szPassword));
	return true;
}

static C4Value FnPlayerMessage(C4PropList * _this, C4Value * Pars)
{
	if (!Object(_this)) throw NeedObjectContext("PlayerMessage");
	int iPlayer = Pars[0].getInt();
	C4String * szMessage = Pars[1].getStr();
	if (!szMessage) return C4VBool(false);
	StdStrBuf buf;
	buf.SetLength(szMessage->GetData().getLength());

	// Speech
	bool fSpoken=false;
	if (SCopySegment(FnStringPar(szMessage),1,buf.getMData(),'$'))
		if (StartSoundEffect(buf.getData(),false,100, Object(_this)))
			fSpoken=true;

	// Text
	if (!fSpoken)
	{
		buf.Take(FnStringFormat(_this, szMessage, &Pars[2], 8));
		const char * dollar = strchr(buf.getData(), '$');
		if (dollar) buf.Shrink(dollar - buf.getData());
		GameMsgObjectPlayer(buf.getData(),Object(_this), iPlayer);
	}
	return C4VBool(true);
}

static C4Value FnMessage(C4PropList * _this, C4Value * Pars)
{
	if (!Object(_this)) throw NeedObjectContext("Message");
	C4String * szMessage = Pars[0].getStr();
	if (!szMessage) return C4VBool(false);
	StdStrBuf buf;
	buf.SetLength(szMessage->GetData().getLength());

	// Speech
	bool fSpoken=false;
	if (SCopySegment(FnStringPar(szMessage),1,buf.getMData(),'$'))
		if (StartSoundEffect(buf.getData(),false,100, Object(_this)))
			fSpoken=true;

	// Text
	if (!fSpoken)
	{
		buf.Take(FnStringFormat(_this,szMessage, &Pars[1], 9));
		const char * dollar = strchr(buf.getData(), '$');
		if (dollar) buf.Shrink(dollar - buf.getData());
		GameMsgObject(buf.getData(),Object(_this));
	}
	return C4VBool(true);
}

// undocumented!
static C4Value FnAddMessage(C4PropList * _this, C4Value * Pars)
{
	if (!Object(_this)) throw NeedObjectContext("AddMessage");
	C4String * szMessage = Pars[0].getStr();
	if (!szMessage) return C4VBool(false);

	::Messages.Append(C4GM_Target, FnStringFormat(_this, szMessage, &Pars[1], 9).getData(),
	                  Object(_this),NO_OWNER,0,0,C4RGB(0xff, 0xff, 0xff));
	return C4VBool(true);
}

static long FnMaterial(C4PropList * _this, C4String *mat_name)
{
	return ::MaterialMap.Get(FnStringPar(mat_name));
}

C4Object* FnPlaceVegetation(C4PropList * _this, C4PropList * Def, long iX, long iY, long iWdt, long iHgt, long iGrowth, C4PropList * shape)
{
	if (shape)
	{
		// New-style call with scripted shape
		C4PropList *out_pos = C4PropList::New(nullptr);
		C4Value vout_pos = C4VPropList(out_pos);
		return Game.PlaceVegetation(Def, iX, iY, iWdt, iHgt, iGrowth, shape, out_pos);
	}
	else
	{
		// Call in old-style shape
		// Local call: relative coordinates
		if (Object(_this)) { iX += Object(_this)->GetX(); iY += Object(_this)->GetY(); }
		// Place vegetation
		return Game.PlaceVegetation(Def, iX, iY, iWdt, iHgt, iGrowth, nullptr, nullptr);
	}
}

C4Object* FnPlaceAnimal(C4PropList * _this, C4PropList * Def)
{
	return Game.PlaceAnimal(Def? Def : _this);
}

static bool FnHostile(C4PropList * _this, long iPlr1, long iPlr2, bool fCheckOneWayOnly)
{
	if (fCheckOneWayOnly)
	{
		return ::Players.HostilityDeclared(iPlr1,iPlr2);
	}
	else
		return !!Hostile(iPlr1,iPlr2);
}

static bool FnSetHostility(C4PropList * _this, long iPlr, long iPlr2, bool fHostile, bool fSilent, bool fNoCalls)
{
	C4Player *pPlr = ::Players.Get(iPlr);
	if (!pPlr) return false;
	// do rejection test first
	if (!fNoCalls)
	{
		if (!!::Game.GRBroadcast(PSF_RejectHostilityChange, &C4AulParSet(iPlr, iPlr2, fHostile), true, true))
			return false;
	}
	// OK; set hostility
	bool fOldHostility = ::Players.HostilityDeclared(iPlr, iPlr2);
	if (!pPlr->SetHostility(iPlr2,fHostile, fSilent)) return false;
	// calls afterwards
	::Game.GRBroadcast(PSF_OnHostilityChange, &C4AulParSet(C4VInt(iPlr), C4VInt(iPlr2), C4VBool(fHostile), C4VBool(fOldHostility)), true);
	return true;
}

static bool FnSetPlrView(C4PropList * _this, long iPlr, C4Object *tobj, bool immediate_position)
{
	if (!ValidPlr(iPlr)) return false;
	::Players.Get(iPlr)->SetViewMode(C4PVM_Target, tobj, immediate_position);
	return true;
}

static long FnGetPlrViewMode(C4PropList * _this, long iPlr)
{
	if (!ValidPlr(iPlr)) return -1;
	if (::Control.SyncMode()) return -1;
	return ::Players.Get(iPlr)->ViewMode;
}

static void FnResetCursorView(C4PropList * _this, long plr, bool immediate_position)
{
	C4Player *pplr = ::Players.Get(plr);
	if (pplr) pplr->ResetCursorView(immediate_position);
}

static C4Object *FnGetPlrView(C4PropList * _this, long iPlr)
{
	C4Player *pPlr = ::Players.Get(iPlr);
	if (!pPlr || pPlr->ViewMode != C4PVM_Target) return nullptr;
	return pPlr->ViewTarget;
}

// flags for SetPlayerZoom* calls
static const int PLRZOOM_Direct     = 0x01,
                 PLRZOOM_NoIncrease = 0x04,
                 PLRZOOM_NoDecrease = 0x08,
                 PLRZOOM_LimitMin   = 0x10,
                 PLRZOOM_LimitMax   = 0x20,
                 PLRZOOM_Set        = 0x40;

static bool FnSetPlayerZoomByViewRange(C4PropList * _this, long plr_idx, long range_wdt, long range_hgt, long flags)
{
	// zoom size safety - both ranges 0 is fine, it causes a zoom reset to default
	if (range_wdt < 0 || range_hgt < 0) return false;
	// special player NO_OWNER: apply to all viewports
	if (plr_idx == NO_OWNER)
	{
		for (C4Player *plr = ::Players.First; plr; plr=plr->Next)
			if (plr->Number != NO_OWNER) // can't happen, but would be a crash if it did...
				FnSetPlayerZoomByViewRange(_this, plr->Number, range_wdt, range_hgt, flags);
		return true;
	}
	else
	{
		// safety check on player only, so function return result is always in sync
		C4Player *plr = ::Players.Get(plr_idx);
		if (!plr) return false;
		// adjust values in player
		if (flags & PLRZOOM_LimitMin) plr->SetMinZoomByViewRange(range_wdt, range_hgt, !!(flags & PLRZOOM_NoIncrease), !!(flags & PLRZOOM_NoDecrease));
		if (flags & PLRZOOM_LimitMax) plr->SetMaxZoomByViewRange(range_wdt, range_hgt, !!(flags & PLRZOOM_NoIncrease), !!(flags & PLRZOOM_NoDecrease));
		// set values after setting min/max to ensure old limits don't block new value
		if ((flags & PLRZOOM_Set) || !(flags & (PLRZOOM_LimitMin | PLRZOOM_LimitMax)))
		{
			plr->SetZoomByViewRange(range_wdt, range_hgt, !!(flags & PLRZOOM_Direct), !!(flags & PLRZOOM_NoIncrease), !!(flags & PLRZOOM_NoDecrease));
		}

	}
	return true;
}

static C4PropList *FnGetPlayerZoomLimits(C4PropList * _this, long plr_idx)
{
	// get player
	C4Player *plr = ::Players.Get(plr_idx);
	if (!plr) return nullptr;
	// collect limits in a prop list
	// if neither width not height is set for zoom limits, return engine defaults.
	C4PropList *result = C4PropList::New();
	if (!result) return nullptr;
	result->SetPropertyByS(::Strings.RegString("MaxWidth"), C4VInt((plr->ZoomLimitMaxWdt || plr->ZoomLimitMaxHgt) ? plr->ZoomLimitMaxWdt : C4VP_DefMaxViewRangeX));
	result->SetPropertyByS(::Strings.RegString("MaxHeight"), C4VInt(plr->ZoomLimitMaxHgt));
	result->SetPropertyByS(::Strings.RegString("MaxValue"), C4VInt(fixtoi(plr->ZoomLimitMaxVal, 100)));
	result->SetPropertyByS(::Strings.RegString("MinWidth"), C4VInt((plr->ZoomLimitMinWdt || plr->ZoomLimitMinHgt) ? plr->ZoomLimitMinWdt : C4VP_DefMinViewRangeX));
	result->SetPropertyByS(::Strings.RegString("MinHeight"), C4VInt(plr->ZoomLimitMinHgt));
	result->SetPropertyByS(::Strings.RegString("MinValue"), C4VInt(fixtoi(plr->ZoomLimitMinVal, 100)));
	return result;
}

static bool FnSetPlayerZoom(C4PropList * _this, long plr_idx, long zoom, long precision, long flags)
{
	// parameter safety. 0/0 means "reset to default".
	if (zoom < 0 || precision < 0) return false;
	// special player NO_OWNER: apply to all viewports
	if (plr_idx == NO_OWNER)
	{
		for (C4Player *plr = ::Players.First; plr; plr=plr->Next)
			if (plr->Number != NO_OWNER) // can't happen, but would be a crash if it did...
				FnSetPlayerZoom(_this, plr->Number, zoom, precision, flags);
		return true;
	}
	else
	{
		// zoom factor calculation
		if (!precision) precision = 1;
		C4Real fZoom = itofix(zoom, precision);
		// safety check on player only, so function return result is always in sync
		C4Player *plr = ::Players.Get(plr_idx);
		if (!plr) return false;
		// adjust values in player
		if (flags & PLRZOOM_LimitMin) plr->SetMinZoom(fZoom, !!(flags & PLRZOOM_NoIncrease), !!(flags & PLRZOOM_NoDecrease));
		if (flags & PLRZOOM_LimitMax) plr->SetMaxZoom(fZoom, !!(flags & PLRZOOM_NoIncrease), !!(flags & PLRZOOM_NoDecrease));
		if ((flags & PLRZOOM_Set) || !(flags & (PLRZOOM_LimitMin | PLRZOOM_LimitMax)))
		{
			plr->SetZoom(fZoom, !!(flags & PLRZOOM_Direct), !!(flags & PLRZOOM_NoIncrease), !!(flags & PLRZOOM_NoDecrease));
		}

	}
	return true;
}

static bool FnSetPlayerViewLock(C4PropList * _this, long plr_idx, bool is_locked)
{
	// special player NO_OWNER: apply to all players
	if (plr_idx == NO_OWNER)
	{
		for (C4Player *plr = ::Players.First; plr; plr=plr->Next)
			if (plr->Number != NO_OWNER) // can't happen, but would be a crash if it did...
				FnSetPlayerViewLock(_this, plr->Number, is_locked);
		return true;
	}
	C4Player *plr = ::Players.Get(plr_idx);
	if (!plr) return false;
	plr->SetViewLocked(is_locked);
	return true;
}

static bool FnDoBaseMaterial(C4PropList * _this, long iPlr, C4ID id, long iChange)
{
	// validity check
	if (!ValidPlr(iPlr)) return false;
	C4Def *pDef = C4Id2Def(id);
	if (!pDef) return false;
	// add to material
	long iLastcount = ::Players.Get(iPlr)->BaseMaterial.GetIDCount(id);
	return ::Players.Get(iPlr)->BaseMaterial.SetIDCount(id,iLastcount+iChange,true);
}

static bool FnDoBaseProduction(C4PropList * _this, long iPlr, C4ID id, long iChange)
{
	// validity check
	if (!ValidPlr(iPlr)) return false;
	C4Def *pDef = C4Id2Def(id);
	if (!pDef) return false;
	// add to material
	long iLastcount = ::Players.Get(iPlr)->BaseProduction.GetIDCount(id);
	return ::Players.Get(iPlr)->BaseProduction.SetIDCount(id,iLastcount+iChange,true);
}

static bool FnSetPlrKnowledge(C4PropList * _this, Nillable<long> iPlr, C4ID id, bool fRemove)
{
	
	bool success = false;
	// iPlr == nil: Call for all players
	if (iPlr.IsNil())
	{
		for (C4Player *player = ::Players.First; player; player = player->Next)
			if (player->SetKnowledge(id, fRemove))
				success = true;
	}
	else
	{
		// Otherwise call for requested player
		C4Player *player = ::Players.Get(iPlr);
		if (player) success = player->SetKnowledge(id, fRemove);
	}
	return success;
}

static C4Value FnGetPlrKnowledge(C4PropList * _this, int iPlr, C4ID id, int iIndex, int dwCategory)
{
	if (!ValidPlr(iPlr)) return C4VBool(false);
	// Search by id, check if available, return bool
	if (id) return C4VBool(::Players.Get(iPlr)->Knowledge.GetIDCount(id,1) != 0);
	// Search indexed item of given category, return C4ID
	return C4VPropList(C4Id2Def(::Players.Get(iPlr)->Knowledge.GetID( ::Definitions, dwCategory, iIndex )));
}

static C4Def * FnGetDefinition(C4PropList * _this, long iIndex)
{
	return ::Definitions.GetDef(iIndex);
}

static C4String * FnGetDefinitionGroupPath(C4PropList * _this)
{
	// Must have loaded all paths
	if (Application.isEditor)
		::Console.EnsureDefinitionListInitialized();
	// Resolve definition
	C4Def *def = _this->GetDef();
	if (!def || !def->ConsoleGroupPath.getData()) return nullptr;
	return ::Strings.RegString(def->ConsoleGroupPath.getData());
}

static C4Value FnGetBaseMaterial(C4PropList * _this, int iPlr, C4ID id, int iIndex, int dwCategory)
{
	if (!ValidPlr(iPlr)) return C4VBool(false);
	// Search by id, return available count
	if (id) return C4VInt(::Players.Get(iPlr)->BaseMaterial.GetIDCount(id));
	// Search indexed item of given category, return C4ID
	return C4VPropList(C4Id2Def(::Players.Get(iPlr)->BaseMaterial.GetID( ::Definitions, dwCategory, iIndex )));
}

static C4Value FnGetBaseProduction(C4PropList * _this, int iPlr, C4ID id, int iIndex, int dwCategory)
{
	if (!ValidPlr(iPlr)) return C4VBool(false);
	// Search by id, return available count
	if (id) return C4VInt(::Players.Get(iPlr)->BaseProduction.GetIDCount(id));
	// Search indexed item of given category, return C4ID
	return C4VPropList(C4Id2Def(::Players.Get(iPlr)->BaseProduction.GetID( ::Definitions, dwCategory, iIndex )));
}

static long FnGetWealth(C4PropList * _this, long iPlr)
{
	if (!ValidPlr(iPlr)) return 0;
	return ::Players.Get(iPlr)->Wealth;
}

static bool FnSetWealth(C4PropList * _this, long iPlr, long iValue)
{
	if (!ValidPlr(iPlr)) return false;
	::Players.Get(iPlr)->SetWealth(iValue);
	return true;
}

static long FnDoPlayerScore(C4PropList * _this, long iPlr, long iChange)
{
	if (!ValidPlr(iPlr)) return false;
	return ::Players.Get(iPlr)->DoScore(iChange);
}

static long FnGetPlayerScore(C4PropList * _this, long iPlr)
{
	if (!ValidPlr(iPlr)) return 0;
	return ::Players.Get(iPlr)->CurrentScore;
}

static long FnGetPlayerScoreGain(C4PropList * _this, long iPlr)
{
	if (!ValidPlr(iPlr)) return 0;
	return ::Players.Get(iPlr)->CurrentScore - ::Players.Get(iPlr)->InitialScore;
}

static C4Object *FnGetHiRank(C4PropList * _this, long iPlr)
{
	if (!ValidPlr(iPlr)) return nullptr;
	return ::Players.Get(iPlr)->GetHiRankActiveCrew();
}

static C4Object *FnGetCrew(C4PropList * _this, long iPlr, long index)
{
	if (!ValidPlr(iPlr)) return nullptr;
	return ::Players.Get(iPlr)->Crew.GetObject(index);
}

static long FnGetCrewCount(C4PropList * _this, long iPlr)
{
	if (!ValidPlr(iPlr)) return 0;
	return ::Players.Get(iPlr)->Crew.ObjectCount();
}

static long FnGetPlayerCount(C4PropList * _this, long iType)
{
	if (!iType)
		return ::Players.GetCount();
	else
		return ::Players.GetCount((C4PlayerType) iType);
}

static long FnGetPlayerByIndex(C4PropList * _this, long iIndex, long iType)
{
	C4Player *pPlayer;
	if (iType)
		pPlayer = ::Players.GetByIndex(iIndex, (C4PlayerType) iType);
	else
		pPlayer = ::Players.GetByIndex(iIndex);
	if (!pPlayer) return NO_OWNER;
	return pPlayer->Number;
}

static long FnEliminatePlayer(C4PropList * _this, long iPlr, bool fRemoveDirect)
{
	C4Player *pPlr=::Players.Get(iPlr);
	if (!pPlr) return false;
	// direct removal?
	if (fRemoveDirect)
	{
		// do direct removal (no fate)
		if (::Control.isCtrlHost()) ::Players.CtrlRemove(iPlr, false);
		return true;
		}
	else
	{
		// do regular elimination
		if (pPlr->Eliminated) return false;
		pPlr->Eliminate();
	}
	return true;
}

// undocumented!
static bool FnSurrenderPlayer(C4PropList * _this, long iPlr)
{
	C4Player *pPlr=::Players.Get(iPlr);
	if (!pPlr) return false;
	if (pPlr->Eliminated) return false;
	pPlr->Surrender();
	return true;
}

// undocumented!
static bool FnSetLeaguePerformance(C4PropList * _this, long iScore, long idPlayer)
	{
	if(!Game.Parameters.isLeague()) return false;
	if(idPlayer && !Game.PlayerInfos.GetPlayerInfoByID(idPlayer)) return false;
	Game.RoundResults.SetLeaguePerformance(iScore, idPlayer);
	return true;
	}


static const int32_t CSPF_FixedAttributes = 1 << 0,
                     CSPF_NoScenarioInit = 1 << 1,
                     CSPF_NoEliminationCheck = 1 << 2,
                     CSPF_Invisible = 1 << 3,
                     CSPF_NoScenarioSave = 1 << 4;


static bool FnCreateScriptPlayer(C4PropList * _this, C4String *szName, long dwColor, long idTeam, long dwFlags, C4ID idExtra)
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
	if (dwFlags & CSPF_NoScenarioSave)     dwInfoFlags |= C4PlayerInfo::PIF_NoScenarioSave;
	pScriptPlrInfo->SetAsScriptPlayer(szName->GetCStr(), dwColor, dwInfoFlags, idExtra);
	pScriptPlrInfo->SetTeam(idTeam);
	C4ClientPlayerInfos JoinPkt(nullptr, true, pScriptPlrInfo);
	// add to queue!
	Game.PlayerInfos.DoPlayerInfoUpdate(&JoinPkt);
	// always successful for sync reasons
	return true;
}

static C4Object *FnGetCursor(C4PropList * _this, long iPlr)
{
	// get player
	C4Player *pPlr = ::Players.Get(iPlr);
	// invalid player?
	if (!pPlr) return nullptr;
	return pPlr->Cursor;
}

// undocumented!
static C4Object *FnGetViewCursor(C4PropList * _this, long iPlr)
{
	// get player
	C4Player *pPlr = ::Players.Get(iPlr);
	// get viewcursor
	return pPlr ? pPlr->ViewCursor : nullptr;
}

static bool FnSetCursor(C4PropList * _this, long iPlr, C4Object *pObj, bool fNoSelectArrow)
{
	C4Player *pPlr = ::Players.Get(iPlr);
	if (!pPlr || (pObj && !pObj->Status) || (pObj && pObj->CrewDisabled)) return false;
	pPlr->SetCursor(pObj, !fNoSelectArrow);
	return true;
}

// undocumented!
static bool FnSetViewCursor(C4PropList * _this, long iPlr, C4Object *pObj)
{
	// get player
	C4Player *pPlr = ::Players.Get(iPlr);
	// invalid player?
	if (!pPlr) return false;
	// set viewcursor
	pPlr->ViewCursor = pObj;
	return true;
}

static long FnGetWind(C4PropList * _this, long x, long y, bool fGlobal)
{
	// global wind
	if (fGlobal) return ::Weather.Wind;
	// local wind
	if (Object(_this)) { x+=Object(_this)->GetX(); y+=Object(_this)->GetY(); }
	return ::Weather.GetWind(x,y);
}

static void FnSetWind(C4PropList * _this, long iWind)
{
	::Weather.SetWind(iWind);
}

static void FnSetTemperature(C4PropList * _this, long iTemperature)
{
	::Weather.SetTemperature(iTemperature);
}

static long FnGetTemperature(C4PropList * _this)
{
	return ::Weather.GetTemperature();
}

static void FnSetAmbientBrightness(C4PropList * _this, long iBrightness)
{
	if (::Landscape.HasFoW())
		::Landscape.GetFoW()->Ambient.SetBrightness(iBrightness / 100.);
}

static long FnGetAmbientBrightness(C4PropList * _this)
{
	if (!::Landscape.HasFoW())
		return 100;
	return static_cast<long>(::Landscape.GetFoW()->Ambient.GetBrightness() * 100. + 0.5);
}

static void FnSetSeason(C4PropList * _this, long iSeason)
{
	::Weather.SetSeason(iSeason);
}

static long FnGetSeason(C4PropList * _this)
{
	return ::Weather.GetSeason();
}

static void FnSetClimate(C4PropList * _this, long iClimate)
{
	::Weather.SetClimate(iClimate);
}

static long FnGetClimate(C4PropList * _this)
{
	return ::Weather.GetClimate();
}

static long FnLandscapeWidth(C4PropList * _this)
{
	return ::Landscape.GetWidth();
}

static long FnLandscapeHeight(C4PropList * _this)
{
	return ::Landscape.GetHeight();
}

static void FnShakeFree(C4PropList * _this, long x, long y, long rad)
{
	::Landscape.ShakeFree(x,y,rad);
}

static long FnDigFree(C4PropList * _this, long x, long y, long rad, bool no_dig2objects, bool no_instability_check)
{
	return ::Landscape.DigFree(x,y,rad,Object(_this),no_dig2objects,no_instability_check);
}

static long FnDigFreeRect(C4PropList * _this, long iX, long iY, long iWdt, long iHgt, bool no_dig2objects, bool no_instability_check)
{
	return ::Landscape.DigFreeRect(iX,iY,iWdt,iHgt,Object(_this),no_dig2objects,no_instability_check);
}

static void FnClearFreeRect(C4PropList * _this, long iX, long iY, long iWdt, long iHgt)
{
	::Landscape.ClearFreeRect(iX,iY,iWdt,iHgt);
}

static bool FnPathFree(C4PropList * _this, long X1, long Y1, long X2, long Y2)
{
	return !!PathFree(X1, Y1, X2, Y2);
}

// undocumented!
static C4ValueArray* FnPathFree2(C4PropList * _this, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
	int32_t x = -1, y = -1;
	if (!PathFree(x1, y1, x2, y2, &x, &y))
	{
		C4ValueArray *pArray = new C4ValueArray(2);
		pArray->SetItem(0, C4VInt(x));
		pArray->SetItem(1, C4VInt(y));
		return pArray;
	}
	return nullptr;
}

C4Object* FnObject(C4PropList * _this, long iNumber)
{
	return ::Objects.SafeObjectPointer(iNumber);
	// See FnObjectNumber
}

static C4Value FnGameCallEx(C4PropList * _this, C4Value * Pars)
{
	C4String * fn = Pars[0].getStr();
	if (!fn) return C4Value();

	// copy parameters
	C4AulParSet ParSet;
	ParSet.Copy(&Pars[1], 9);
	// Call
	return ::Game.GRBroadcast(fn->GetCStr(), &ParSet, true);
}

static C4Object * FnEditCursor(C4PropList * _this)
{
	if (::Control.SyncMode()) return nullptr;
	return Console.EditCursor.GetTarget();
}

static bool FnIsNetwork(C4PropList * _this) { return Game.Parameters.IsNetworkGame; }

static bool FnIsEditor(C4PropList * _this) { return Game.Parameters.IsEditor; }

// undocumented!
static C4String *FnGetLeague(C4PropList * _this, long idx)
{
	// get indexed league
	StdStrBuf sIdxLeague;
	if (!Game.Parameters.League.GetSection(idx, &sIdxLeague)) return nullptr;
	return String(sIdxLeague.getData());
}

static int32_t FnGetLeagueScore(C4PropList * _this, long idPlayer)
{
	// security
	if (idPlayer < 1) return 0;
	// get info
	C4PlayerInfo *pInfo = Game.PlayerInfos.GetPlayerInfoByID(idPlayer);
	if (!pInfo) return 0;
	// get league score
	return pInfo->getLeagueScore();
}

static bool FnSetLeagueProgressData(C4PropList * _this, C4String *pNewData, long idPlayer)
{
	if(!Game.Parameters.League.getLength() || !idPlayer) return false;
	C4PlayerInfo *info = Game.PlayerInfos.GetPlayerInfoByID(idPlayer);
	if (!info) return false;
	info->SetLeagueProgressData(pNewData ? pNewData->GetCStr() : nullptr);
	return true;
}

static C4String *FnGetLeagueProgressData(C4PropList * _this, long idPlayer)
{
	if(!Game.Parameters.League.getLength()) return nullptr;
	C4PlayerInfo *info = Game.PlayerInfos.GetPlayerInfoByID(idPlayer);
	if (!info) return nullptr;
	return String(info->GetLeagueProgressData());
}

// undocumented!
static bool FnTestMessageBoard(C4PropList * _this, long iForPlr, bool fTestIfInUse)
{
	// multi-query-MessageBoard is always available if the player is valid =)
	// (but it won't do anything in developer mode...)
	C4Player *pPlr = ::Players.Get(iForPlr);
	if (!pPlr) return false;
	if (!fTestIfInUse) return true;
	// single query only if no query is scheduled
	return pPlr->HasMessageBoardQuery();
}

// undocumented!
static bool FnCallMessageBoard(C4PropList * _this, C4Object *pObj, bool fUpperCase, C4String *szQueryString, long iForPlr)
{
	if (!pObj) pObj=Object(_this);
	if (pObj && !pObj->Status) return false;
	// check player
	C4Player *pPlr = ::Players.Get(iForPlr);
	if (!pPlr) return false;
	// remove any previous
	pPlr->CallMessageBoard(pObj, StdStrBuf(FnStringPar(szQueryString)), !!fUpperCase);
	return true;
}

// undocumented!
static bool FnAbortMessageBoard(C4PropList * _this, C4Object *pObj, long iForPlr)
{
	if (!pObj) pObj=Object(_this);
	// check player
	C4Player *pPlr = ::Players.Get(iForPlr);
	if (!pPlr) return false;
	// close TypeIn if active
	::MessageInput.AbortMsgBoardQuery(pObj, iForPlr);
	// abort for it
	return pPlr->RemoveMessageBoardQuery(pObj);
}

static void FnSetFoW(C4PropList * _this, bool fEnabled, long iPlr)
{
	// safety
	if (!ValidPlr(iPlr)) return;
	// set enabled
	::Players.Get(iPlr)->SetFoW(!!fEnabled);
}

static long FnSetMaxPlayer(C4PropList * _this, long iTo)
{
	// think positive! :)
	if (iTo < 0) return false;
	// script functions don't need to pass ControlQueue!
	Game.Parameters.MaxPlayers = iTo;
	// success
	return true;
}

static bool FnGetMissionAccess(C4PropList * _this, C4String *strMissionAccess)
{
	// safety
	if (!strMissionAccess) return false;

	// non-sync mode: warn
	if (::Control.SyncMode())
		Log("Warning: using GetMissionAccess may cause desyncs when playing records!");

	return SIsModule(Config.General.MissionAccess, FnStringPar(strMissionAccess));
}

// Helper to read or write a value from/to a structure. Must be two
class C4ValueCompiler : public StdCompiler
{
public:
	C4ValueCompiler(const char **pszNames, int iNameCnt, int iEntryNr)
			: pszNames(pszNames), iNameCnt(iNameCnt), iEntryNr(iEntryNr)
	{  }

	bool isDeserializer() override { return false; }
	bool hasNaming() override { return true; }
	bool isVerbose() override { return false; }

	bool Name(const char *szName) override
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

	bool Default(const char *szName) override
	{
		// Always process values even if they are default!
		return false;
	}

	void NameEnd(bool fBreak = false) override
	{
		// end of matched name section?
		if (haveCurrentMatch())
		{
			iMatchCount--;
			if (!iMatchCount) iMatchStart = 0;
		}
		iDepth--;
	}

	void Begin() override
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
	virtual void ProcessString(std::string &str, bool isID) = 0;

	// value functions
private:
	template<class T>
	void MaybeProcessInt(T &t)
	{
		if (haveCompleteMatch() && !iEntryNr--)
		{
			int32_t i = t;
			ProcessInt(i);
			t = i; 
		}
	}
public:
	void DWord(int32_t &rInt) override { MaybeProcessInt(rInt); }
	void DWord(uint32_t &rInt) override { MaybeProcessInt(rInt); }
	void Word(int16_t &rShort) override { MaybeProcessInt(rShort); }
	void Word(uint16_t &rShort) override { MaybeProcessInt(rShort); }
	void Byte(int8_t &rByte) override { MaybeProcessInt(rByte); }
	void Byte(uint8_t &rByte) override { MaybeProcessInt(rByte); }
	void Boolean(bool &rBool) override { if (haveCompleteMatch()) if (!iEntryNr--) ProcessBool(rBool); }
	void Character(char &rChar) override { if (haveCompleteMatch()) if (!iEntryNr--) ProcessChar(rChar); }

	// The C4ID-Adaptor will set RCT_ID for it's strings (see C4Id.h), so we don't have to guess the type.
	void String(char *szString, size_t iMaxLength, RawCompileType eType) override
	{ if (haveCompleteMatch()) if (!iEntryNr--) ProcessString(szString, iMaxLength, eType == StdCompiler::RCT_ID); }
	void String(char **pszString, RawCompileType eType) override
	{ if (haveCompleteMatch()) if (!iEntryNr--) ProcessString(pszString, eType == StdCompiler::RCT_ID); }
	void String(std::string &str, RawCompileType type) override
	{ if (haveCompleteMatch()) if (!iEntryNr--) ProcessString(str, type == StdCompiler::RCT_ID); }
	void Raw(void *pData, size_t iSize, RawCompileType eType = RCT_Escaped) override
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
	void ProcessInt(int32_t &rInt) override { Res = C4VInt(rInt); }
	void ProcessBool(bool &rBool) override { Res = C4VBool(rBool); }
	void ProcessChar(char &rChar) override { Res = C4VString(FormatString("%c", rChar)); }

	void ProcessString(char *szString, size_t iMaxLength, bool fIsID) override
	{ Res = (fIsID ? C4VPropList(C4Id2Def(C4ID(szString))) : C4VString(szString)); }
	void ProcessString(char **pszString, bool fIsID) override
	{ Res = (fIsID ? C4VPropList(C4Id2Def(C4ID(*pszString))) : C4VString(*pszString)); }
	void ProcessString(std::string &str, bool fIsID) override
	{ Res = (fIsID ? C4VPropList(C4Id2Def(C4ID(str.c_str()))) : C4VString(str.c_str())); }
};

// Use the compiler to find a named value in a structure
template <class T>
C4Value GetValByStdCompiler(const char *strEntry, const char *strSection, int iEntryNr, const T &rFrom)
{
	// Set up name array, create compiler
	const char *szNames[2] = { strSection ? strSection : strEntry, strSection ? strEntry : nullptr };
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

static C4Value FnGetDefCoreVal(C4PropList * _this, C4String * strEntry, C4String * strSection, int iEntryNr)
{
	if (!_this || !_this->GetDef())
		throw NeedNonGlobalContext("GetDefCoreVal");

	return GetValByStdCompiler(FnStringPar(strEntry), strSection ? strSection->GetCStr() : nullptr,
			iEntryNr, mkNamingAdapt(*_this->GetDef(), "DefCore"));
}

static C4Value FnGetObjectVal(C4Object * _this, C4String * strEntry, C4String * strSection, int iEntryNr)
{
	// get value
	C4ValueNumbers numbers;
	C4Value retval = GetValByStdCompiler(FnStringPar(strEntry), strSection ? strSection->GetCStr() : nullptr,
			iEntryNr, mkNamingAdapt(mkParAdapt(*Object(_this), &numbers), "Object"));
	numbers.Denumerate();
	retval.Denumerate(&numbers);
	return retval;
}

static C4Value FnGetObjectInfoCoreVal(C4Object * _this, C4String * strEntry, C4String * strSection, int iEntryNr)
{
	// get obj info
	C4ObjectInfo* pObjInfo = Object(_this)->Info;

	if (!pObjInfo) return C4VNull;

	// get obj info core
	C4ObjectInfoCore* pObjInfoCore = (C4ObjectInfoCore*) pObjInfo;

	// get value
	return GetValByStdCompiler(FnStringPar(strEntry), strSection ? strSection->GetCStr() : nullptr,
			iEntryNr, mkNamingAdapt(*pObjInfoCore, "ObjectInfo"));
}

static C4Value FnGetScenarioVal(C4PropList * _this, C4String * strEntry, C4String * strSection, int iEntryNr)
{
	return GetValByStdCompiler(FnStringPar(strEntry), strSection ? strSection->GetCStr() : nullptr,
			iEntryNr, mkParAdapt(Game.C4S, false));
}

static C4Value FnGetPlayerVal(C4PropList * _this, C4String * strEntry, C4String * strSection, int iPlayer, int iEntryNr)
{
	// get player
	C4Player* pPlayer = ::Players.Get(iPlayer);
	if (!pPlayer) return C4Value();

	// get value
	C4ValueNumbers numbers;
	C4Value retval = GetValByStdCompiler(FnStringPar(strEntry), strSection ? strSection->GetCStr() : nullptr,
			iEntryNr, mkNamingAdapt(mkParAdapt(*pPlayer, &numbers), "Player"));
	numbers.Denumerate();
	retval.Denumerate(&numbers);
	return retval;
}

static C4Value FnGetPlayerInfoCoreVal(C4PropList * _this, C4String * strEntry, C4String * strSection, int iPlayer, int iEntryNr)
{
	// get player
	C4Player* pPlayer = ::Players.Get(iPlayer);
	if (!pPlayer) return C4Value();

	// get plr info core
	C4PlayerInfoCore* pPlayerInfoCore = (C4PlayerInfoCore*) pPlayer;

	// get value
	return GetValByStdCompiler(FnStringPar(strEntry), strSection ? strSection->GetCStr() : nullptr,
			iEntryNr, *pPlayerInfoCore);
}

static C4Value FnGetMaterialVal(C4PropList * _this, C4String * strEntry,  C4String* strSection, int iMat, int iEntryNr)
{
	if (iMat < 0 || iMat >= ::MaterialMap.Num) return C4Value();

	// get material
	C4Material *pMaterial = &::MaterialMap.Map[iMat];

	// get plr info core
	C4MaterialCore* pMaterialCore = static_cast<C4MaterialCore*>(pMaterial);

	// get value
	return GetValByStdCompiler(FnStringPar(strEntry), strSection ? strSection->GetCStr() : nullptr, iEntryNr, *pMaterialCore);
}

static C4String *FnMaterialName(C4PropList * _this, long iMat)
{
	// mat valid?
	if (!MatValid(iMat)) return nullptr;
	// return mat name
	return String(::MaterialMap.Map[iMat].Name);
}

static bool FnSetSky(C4PropList * _this, C4String * name)
{
	if (!name) return false;
	auto& sky = ::Landscape.GetSky();

	// Open Graphics.ocg -- we might need to fetch some shader (slices)
	// from there when reloading the sky.
	if (!::GraphicsResource.RegisterGlobalGraphics()) return false;
	if (!::GraphicsResource.RegisterMainGroups()) return false;

	sky.Clear();
	const bool result = sky.Init(false, name->GetCStr());

	// close Graphics.ocg again
	::GraphicsResource.CloseFiles();

	return result;
}

static bool FnSetSkyAdjust(C4PropList * _this, long dwAdjust, long dwBackClr)
{
	// set adjust
	::Landscape.GetSky().SetModulation(dwAdjust, dwBackClr);
	// success
	return true;
}

static bool FnSetMatAdjust(C4PropList * _this, long dwAdjust)
{
	// set adjust
	::Landscape.SetModulation(dwAdjust);
	// success
	return true;
}

static long FnGetSkyAdjust(C4PropList * _this, bool fBackColor)
{
	// get adjust
	return ::Landscape.GetSky().GetModulation(!!fBackColor);
}

static long FnGetMatAdjust(C4PropList * _this)
{
	// get adjust
	return ::Landscape.GetModulation();
}

static long FnGetTime(C4PropList * _this)
{
	// check network, record, etc
	if (::Control.SyncMode()) return 0;
	return C4TimeMilliseconds::Now().AsInt();
}

static C4Value FnSetPlrExtraData(C4PropList * _this, int iPlayer, C4String * DataName, const C4Value & Data)
{
	const char * strDataName = FnStringPar(DataName);
	// do not allow data type C4V_Array or C4V_C4Object
	if (Data.GetType() != C4V_Nil &&
	    Data.GetType() != C4V_Int &&
	    Data.GetType() != C4V_Bool &&
	    Data.GetType() != C4V_String) return C4VNull;
	C4Player* pPlayer = ::Players.Get(iPlayer);
	if (!pPlayer) return C4Value();
	// no name list created yet?
	if (!pPlayer->ExtraData.pNames)
		// create name list
		pPlayer->ExtraData.CreateTempNameList();
	// data name already exists?
	long ival;
	if ((ival = pPlayer->ExtraData.pNames->GetItemNr(strDataName)) != -1)
		pPlayer->ExtraData[ival] = Data;
	else
	{
		// add name
		pPlayer->ExtraData.pNames->AddName(strDataName);
		// get val id & set
		if ((ival = pPlayer->ExtraData.pNames->GetItemNr(strDataName)) == -1) return C4Value();
		pPlayer->ExtraData[ival] = Data;
	}
	// ok, return the value that has been set
	return Data;
}

static C4Value FnGetPlrExtraData(C4PropList * _this, int iPlayer, C4String * DataName)
{
	const char *strDataName = FnStringPar(DataName);
	C4Player* pPlayer = ::Players.Get(iPlayer);
	if (!pPlayer) return C4Value();
	// no name list?
	if (!pPlayer->ExtraData.pNames) return C4Value();
	long ival;
	if ((ival = pPlayer->ExtraData.pNames->GetItemNr(strDataName)) == -1) return C4Value();
	// return data
	return pPlayer->ExtraData[ival];
}

// undocumented!
static long FnDrawMatChunks(C4PropList * _this, long tx, long ty, long twdt, long thgt, long icntx, long icnty, C4String *strMaterial, C4String *strTexture, bool bIFT)
{
	return ::Landscape.DrawChunks(tx, ty, twdt, thgt, icntx, icnty, FnStringPar(strMaterial), FnStringPar(strTexture), bIFT != 0);
}

static long FnDrawMap(C4PropList * _this, long iX, long iY, long iWdt, long iHgt, C4String *szMapDef)
{
	// draw it!
	// don't clear the old map before drawing
	return ::Landscape.DrawMap(iX, iY, iWdt, iHgt, FnStringPar(szMapDef), true);
}

static long FnDrawDefMap(C4PropList * _this, long iX, long iY, long iWdt, long iHgt, C4String *szMapDef)
{
	// draw it!
	// don't clear the old map before drawing
	return ::Landscape.DrawDefMap(iX, iY, iWdt, iHgt, FnStringPar(szMapDef), true);
}

static bool FnCreateParticle(C4PropList * _this, C4String *name, C4Value x, C4Value y, C4Value speedX, C4Value speedY, C4Value lifetime, C4PropList *properties, int amount)
{
	// safety
	C4Object *obj = Object(_this);
	if (obj && !_this->Status) return false;
#ifndef USE_CONSOLE
	if (amount <= 0) amount = 1;
	
	// get particle
	C4ParticleDef *pDef = ::Particles.definitions.GetDef(FnStringPar(name));
	if (!pDef) return false;
	// construct data
	C4ParticleValueProvider valueX, valueY, valueSpeedX, valueSpeedY, valueLifetime;
	valueX.Set(x);
	valueY.Set(y);
	valueSpeedX.Set(speedX);
	valueSpeedY.Set(speedY);
	valueLifetime.Set(lifetime);
	// create
	::Particles.Create(pDef, valueX, valueY, valueSpeedX, valueSpeedY, valueLifetime, properties, amount, obj);
#endif
	// success, even if not created
	return true;
}

static bool FnClearParticles(C4PropList * _this)
{
#ifndef USE_CONSOLE
	C4Object *obj;
	if ((obj = Object(_this)))
	{
		if (obj->BackParticles)
			obj->BackParticles->Clear();
		if (obj->FrontParticles)
			obj->FrontParticles->Clear();
	}
	else
	{
		if (::Particles.GetGlobalParticles())
			::Particles.GetGlobalParticles()->Clear();
	}
#endif
	// always return true
	return true;
}

static C4ValueArray* FnPV_Linear(C4PropList * _this, C4Value startValue, C4Value endValue)
{
	C4ValueArray *pArray = new C4ValueArray(3);
	pArray->SetItem(0, C4VInt(C4PV_Linear));
	pArray->SetItem(1, startValue);
	pArray->SetItem(2, endValue);
	return pArray;
}

static C4ValueArray* FnPV_Random(C4PropList * _this, C4Value startValue, C4Value endValue, C4Value rerollInterval, C4Value seed)
{
	C4ValueArray *pArray = new C4ValueArray(4);
	pArray->SetItem(0, C4VInt(C4PV_Random));
	pArray->SetItem(1, startValue);
	pArray->SetItem(2, endValue);
	pArray->SetItem(3, rerollInterval);
	pArray->SetItem(4, seed);
	return pArray;
}

static C4ValueArray* FnPV_Direction(C4PropList * _this, C4Value factor)
{
	C4ValueArray *pArray = new C4ValueArray(2);
	pArray->SetItem(0, C4VInt(C4PV_Direction));
	pArray->SetItem(1, factor.GetType() != C4V_Nil ? factor : C4VInt(1000));
	return pArray;
}

static C4ValueArray* FnPV_Step(C4PropList * _this, C4Value step, C4Value startValue, C4Value delay, C4Value maximumValue)
{
	C4ValueArray *pArray = new C4ValueArray(5);
	pArray->SetItem(0, C4VInt(C4PV_Step));
	pArray->SetItem(1, step);
	pArray->SetItem(2, startValue);
	pArray->SetItem(3, delay);
	pArray->SetItem(4, maximumValue);
	return pArray;
}

static C4Value FnPV_KeyFrames(C4PropList * _this, C4Value *pars)
{
	C4ValueArray *pArray = new C4ValueArray(C4AUL_MAX_Par);
	pArray->SetItem(0, C4VInt(C4PV_KeyFrames));
	const int offset = 1;

	// Read all parameters
	int i = 0;
	for (; i < C4AUL_MAX_Par; i++)
	{
		C4Value Data = *(pars++);
		// No data given?
		if (Data.GetType() == C4V_Nil) break;

		pArray->SetItem(offset + i, Data);
	}
	pArray->SetSize(i + offset);
	return C4Value(pArray);
}

static C4ValueArray* FnPV_Sin(C4PropList * _this, C4Value value, C4Value amplitude, C4Value offset)
{
	C4ValueArray *pArray = new C4ValueArray(5);
	pArray->SetItem(0, C4VInt(C4PV_Sin));
	pArray->SetItem(1, value);
	pArray->SetItem(2, amplitude);
	pArray->SetItem(3, offset);
	return pArray;
}

static C4ValueArray* FnPV_Cos(C4PropList * _this, C4Value value, C4Value amplitude, C4Value offset)
{
	C4ValueArray *pArray = new C4ValueArray(5);
	pArray->SetItem(0, C4VInt(C4PV_Cos));
	pArray->SetItem(1, value);
	pArray->SetItem(2, amplitude);
	pArray->SetItem(3, offset);
	return pArray;
}

static C4ValueArray* FnPV_Speed(C4PropList * _this, C4Value factor, C4Value startValue)
{
	C4ValueArray *pArray = new C4ValueArray(3);
	pArray->SetItem(0, C4VInt(C4PV_Speed));
	pArray->SetItem(1, factor.GetType() == C4V_Nil ? C4VInt(1000) : factor);
	pArray->SetItem(2, startValue);
	return pArray;
}

static C4ValueArray* FnPV_Wind(C4PropList * _this, C4Value factor, C4Value startValue)
{
	C4ValueArray *pArray = new C4ValueArray(3);
	pArray->SetItem(0, C4VInt(C4PV_Wind));
	pArray->SetItem(1, factor.GetType() == C4V_Nil ? C4VInt(1000) : factor);
	pArray->SetItem(2, startValue);
	return pArray;
}

static C4ValueArray* FnPV_Gravity(C4PropList * _this, C4Value factor, C4Value startValue)
{
	C4ValueArray *pArray = new C4ValueArray(3);
	pArray->SetItem(0, C4VInt(C4PV_Gravity));
	pArray->SetItem(1, factor.GetType() == C4V_Nil ? C4VInt(1000) : factor);
	pArray->SetItem(2, startValue);
	return pArray;
}

static C4ValueArray* FnPC_Die(C4PropList * _this)
{
	C4ValueArray *pArray = new C4ValueArray(1);
	pArray->SetItem(0, C4VInt(C4PC_Die));
	return pArray;
}

static C4ValueArray* FnPC_Bounce(C4PropList * _this, C4Value bouncyness)
{
	C4ValueArray *pArray = new C4ValueArray(2);
	pArray->SetItem(0, C4VInt(C4PC_Bounce));
	pArray->SetItem(1, bouncyness.GetType() != C4V_Nil ? bouncyness : C4VInt(1000));
	return pArray;
}

static C4ValueArray* FnPC_Stop(C4PropList * _this)
{
	C4ValueArray *pArray = new C4ValueArray(1);
	pArray->SetItem(0, C4VInt(C4PC_Stop));
	return pArray;
}

static bool FnSetSkyParallax(C4PropList * _this, Nillable<long> iMode, Nillable<long> iParX, Nillable<long> iParY, Nillable<long> iXDir, Nillable<long> iYDir, Nillable<long> iX, Nillable<long> iY)
{
	// set all parameters that aren't nil
	if (!iMode.IsNil())
		if (Inside<long>(iMode, 0, 1)) ::Landscape.GetSky().ParallaxMode = iMode;
	if (!iParX.IsNil() && iParX) ::Landscape.GetSky().ParX = iParX;
	if (!iParY.IsNil() && iParY) ::Landscape.GetSky().ParY = iParY;
	if (!iXDir.IsNil()) ::Landscape.GetSky().xdir = itofix(iXDir);
	if (!iYDir.IsNil()) ::Landscape.GetSky().ydir = itofix(iYDir);
	if (!iX.IsNil()) ::Landscape.GetSky().x = itofix(iX);
	if (!iY.IsNil()) ::Landscape.GetSky().y = itofix(iY);
	// success
	return true;
}

static long FnReloadDef(C4PropList * _this, C4ID idDef, long iReloadWhat)
{
	// get def
	C4Def *pDef=nullptr;
	if (!idDef)
	{
		// no def given: local def
		if (Object(_this)) pDef=Object(_this)->Def;
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

static long FnReloadParticle(C4PropList * _this, C4String *szParticleName)
{
	// perform reload
	return Game.ReloadParticle(FnStringPar(szParticleName));
}

static bool FnSetGamma(C4PropList * _this, long iRed, long iGreen, long iBlue, long iRampIndex)
{
	pDraw->SetGamma(float(iRed) / 100,
	                float(iGreen) / 100,
	                float(iBlue) / 100,
	                iRampIndex);
	return true;
}

static bool FnResetGamma(C4PropList * _this, long iRampIndex)
{
	::Game.SetDefaultGamma();
	return true;
}

static Nillable<long> FnAddFragmentShader(C4PropList * _this, C4String * name, C4String * shader)
{
	if (!name || !shader) return C4Void();
	return ScriptShader.Add(name->GetCStr(), C4ScriptShader::FragmentShader, shader->GetCStr());
}

static bool FnRemoveShader(C4PropList * _this, long id)
{
	return ScriptShader.Remove(id);
}

// undocumented!
static long FnFrameCounter(C4PropList * _this) { return Game.FrameCounter; }

struct PathInfo
{
	long ilx, ily;
	long ilen;
};

struct SumPathLength
{
	explicit SumPathLength(PathInfo *info) : pPathInfo(info) {}
	bool operator()(int32_t iX, int32_t iY, C4Object *TransferTarget)
	{
		pPathInfo->ilen += Distance(pPathInfo->ilx, pPathInfo->ily, iX, iY);
		pPathInfo->ilx = iX;
		pPathInfo->ily = iY;
		return true;
	}

private:
	PathInfo *pPathInfo;
};

static Nillable<long> FnGetPathLength(C4PropList * _this, long iFromX, long iFromY, long iToX, long iToY, long iLevel)
{
	PathInfo PathInfo;
	PathInfo.ilx = iFromX;
	PathInfo.ily = iFromY;
	PathInfo.ilen = 0;
	if (!iLevel)
		iLevel = 1;
	Game.PathFinder.SetLevel(iLevel);
	if (!Game.PathFinder.Find(iFromX, iFromY, iToX, iToY, SumPathLength(&PathInfo)))
		return C4Void();
	return PathInfo.ilen + Distance(PathInfo.ilx, PathInfo.ily, iToX, iToY);
}

// undocumented!
static long FnSetTextureIndex(C4PropList * _this, C4String *psMatTex, long iNewIndex, bool fInsert)
{
	if (!Inside(iNewIndex, 0l, 255l)) return false;
	return ::Landscape.SetTextureIndex(FnStringPar(psMatTex), BYTE(iNewIndex), !!fInsert);
}

// undocumented!
static long FnRemoveUnusedTexMapEntries(C4PropList * _this)
{
	::Landscape.RemoveUnusedTexMapEntries();
	return true;
}

static const int32_t DMQ_Sky = 0, // draw w/ sky IFT
                     DMQ_Sub = 1, // draw w/ tunnel IFT
                     DMQ_Bridge = 2; // draw only over materials you can bridge over

static bool FnDrawMaterialQuad(C4PropList * _this, C4String *szMaterial, long iX1, long iY1, long iX2, long iY2, long iX3, long iY3, long iX4, long iY4, const C4Value& draw_mode)
{
	const char *szMat = FnStringPar(szMaterial);

	const char *szBackMat = nullptr;
	bool fBridge = false;
	if (draw_mode.GetType() == C4V_Int)
	{
		// Default behaviour: Default background material
		const int draw_mode_value = draw_mode.getInt();
		switch(draw_mode_value)
		{
		case DMQ_Sky: break;
		case DMQ_Sub: szBackMat = "Tunnel"; break; // TODO: Go via DefaultBkgMat
		case DMQ_Bridge: fBridge = true; break;
		}
	}
	else if (draw_mode.GetType() == C4V_String)
	{
		szBackMat = FnStringPar(draw_mode.getStr());
	}

	return !! ::Landscape.DrawQuad(iX1, iY1, iX2, iY2, iX3, iY3, iX4, iY4, szMat, szBackMat, fBridge);
}

static bool FnSetFilmView(C4PropList * _this, long iToPlr)
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

static bool FnAddMsgBoardCmd(C4PropList * _this, C4String *pstrCommand, C4String *pstrScript)
{
	// safety
	if (!pstrCommand || !pstrScript) return false;
	// add command
	::MessageInput.AddCommand(FnStringPar(pstrCommand), FnStringPar(pstrScript));
	return true;
}

static bool FnSetGameSpeed(C4PropList * _this, long iSpeed)
{
	// safety
	if (iSpeed) if (!Inside<long>(iSpeed, 0, 1000)) return false;
	if (!iSpeed) iSpeed = 38;
	// set speed, restart timer
	Application.SetGameTickDelay(1000 / iSpeed);
	return true;
}

bool SimFlight(C4Real &x, C4Real &y, C4Real &xdir, C4Real &ydir, int32_t iDensityMin, int32_t iDensityMax, int32_t &iIter);

static C4ValueArray* FnSimFlight(C4PropList * _this, int X, int Y, Nillable<int> pvrXDir, Nillable<int> pvrYDir, Nillable<int> pviDensityMin, Nillable<int> pviDensityMax, Nillable<int> pviIter, int iPrec)
{
	// check and set parameters
	if (Object(_this))
	{
		X += Object(_this)->GetX();
		Y += Object(_this)->GetY();
	}
	int XDir = pvrXDir.IsNil() && Object(_this) ? fixtoi(Object(_this)->xdir) : static_cast<int>(pvrXDir);
	int YDir = pvrXDir.IsNil() && Object(_this) ? fixtoi(Object(_this)->ydir) : static_cast<int>(pvrYDir);

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

// undocumented!
static long FnLoadScenarioSection(C4PropList * _this, C4String *pstrSection, long dwFlags)
{
	// safety
	const char *szSection;
	if (!pstrSection || !*(szSection=FnStringPar(pstrSection))) return false;
	// try to load it
	return Game.LoadScenarioSection(szSection, dwFlags);
}

static bool FnSetViewOffset(C4PropList * _this, long iPlayer, long iX, long iY)
{
	if (!ValidPlr(iPlayer)) return false;
	// get player viewport
	C4Viewport *pView = ::Viewports.GetViewport(iPlayer);
	if (!pView) return true; // sync safety
	// set
	pView->SetViewOffset(iX, iY);
	// ok
	return true;
}

// undocumented!
static bool FnSetPreSend(C4PropList * _this, long iToVal, C4String *pNewName)
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

static long FnGetPlayerID(C4PropList * _this, long iPlayer)
{
	C4Player *pPlr = ::Players.Get(iPlayer);
	return pPlr ? pPlr->ID : 0;
}

static long FnGetPlayerTeam(C4PropList * _this, long iPlayer)
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

static bool FnSetPlayerTeam(C4PropList * _this, long iPlayer, long idNewTeam, bool fNoCalls)
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
	if (!Game.Teams.IsJoin2TeamAllowed(idNewTeam, pPlrInfo->GetType())) return false;
	// ask script if it's allowed
	if (!fNoCalls)
	{
		if (!!::Game.GRBroadcast(PSF_RejectTeamSwitch, &C4AulParSet(iPlayer, idNewTeam), true, true))
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
		::Game.GRBroadcast(PSF_OnTeamSwitch, &C4AulParSet(iPlayer, idNewTeam, idOldTeam), true);
	return true;
}

// undocumented!
static C4PropList *FnGetScriptPlayerExtraID(C4PropList * _this, long player_number)
{
	C4Player *plr = ::Players.Get(player_number);
	if (!plr) return nullptr;
	C4PlayerInfo *info = plr->GetInfo();
	if (!info) return nullptr;
	return C4Id2Def(info->GetScriptPlayerExtraID());
}

// undocumented!
static long FnGetTeamConfig(C4PropList * _this, long iConfigValue)
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

static C4String *FnGetTeamName(C4PropList * _this, long iTeam)
{
	C4Team *pTeam = Game.Teams.GetTeamByID(iTeam);
	if (!pTeam) return nullptr;
	return String(pTeam->GetName());
}

static long FnGetTeamColor(C4PropList * _this, long iTeam)
{
	C4Team *pTeam = Game.Teams.GetTeamByID(iTeam);
	return pTeam ? pTeam->GetColor() : 0u;
}

static long FnGetTeamByIndex(C4PropList * _this, long iIndex)
{
	C4Team *pTeam = Game.Teams.GetTeamByIndex(iIndex);
	return pTeam ? pTeam->GetID() : 0;
}

static long FnGetTeamCount(C4PropList * _this)
{
	return Game.Teams.GetTeamCount();
}

// undocumented!
static bool FnInitScenarioPlayer(C4PropList * _this, long iPlayer, long idTeam)
{
	C4Player *pPlr = ::Players.Get(iPlayer);
	if (!pPlr) return false;
	return pPlr->ScenarioAndTeamInit(idTeam);
}

static bool FnSetScoreboardData(C4PropList * _this, long iRowID, long iColID, C4String *pText, long iData)
{
	Game.Scoreboard.SetCell(iColID, iRowID, pText ? pText->GetCStr() : nullptr, iData);
	return true;
}

// undocumented!
static C4String *FnGetScoreboardString(C4PropList * _this, long iRowID, long iColID)
{
	return String(Game.Scoreboard.GetCellString(iColID, iRowID));
}

// undocumented!
static int32_t FnGetScoreboardData(C4PropList * _this, long iRowID, long iColID)
{
	return Game.Scoreboard.GetCellData(iColID, iRowID);
}

static bool FnDoScoreboardShow(C4PropList * _this, long iChange, long iForPlr)
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
	return true;
}

static bool FnSortScoreboard(C4PropList * _this, long iByColID, bool fReverse)
{
	return Game.Scoreboard.SortBy(iByColID, !!fReverse);
}

// undocumented!
static bool FnAddEvaluationData(C4PropList * _this, C4String *pText, long idPlayer)
{
	// safety
	if (!pText) return false;
	if (!pText->GetCStr()) return false;
	if (idPlayer && !Game.PlayerInfos.GetPlayerInfoByID(idPlayer)) return false;
	// add data
	Game.RoundResults.AddCustomEvaluationString(pText->GetCStr(), idPlayer);
	return true;
}

// undocumented!
static void FnHideSettlementScoreInEvaluation(C4PropList * _this, bool fHide)
{
	Game.RoundResults.HideSettlementScore(fHide);
}

static bool FnCustomMessage(C4PropList * _this, C4String *pMsg, C4Object *pObj, Nillable<long> iOwner, long iOffX, long iOffY, long dwClr, C4ID idDeco, C4PropList *pSrc, long dwFlags, long iHSize)
{
	// safeties: for global messages pSrc needs to be object/definition. For object-local messages, any proplist is OK
	if (pSrc)
		if(!pSrc->GetDef() && !pSrc->GetObject() && !pSrc->GetPropertyPropList(P_Source) && !pObj) return false;
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
		throw C4AulExecError("CustomMessage: Only one horizontal positioning flag allowed");
	}
	if (((vpos | (vpos-1)) + 1)>>1 != vpos)
	{
		throw C4AulExecError("CustomMessage: Only one vertical positioning flag allowed");
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
	if (dwFlags & C4GM_DropSpeech) sMsg.SplitAtChar('$', nullptr);
	// create it!
	return ::Messages.New(iType,sMsg,pObj,iOwner,iOffX,iOffY,(uint32_t)dwClr, idDeco, pSrc, dwFlags, iHSize);
}

static int FnGuiOpen(C4PropList * _this, C4PropList *menu)
{
	C4ScriptGuiWindow *window = new C4ScriptGuiWindow;

	::Game.ScriptGuiRoot->AddChild(window);

	if (!window->CreateFromPropList(menu, true))
	{
		::Game.ScriptGuiRoot->RemoveChild(window, false);
		return 0;
	}

	return window->GetID();
}

static bool FnGuiUpdateTag(C4PropList * _this, C4String *tag, int32_t guiID, int32_t childID, C4Object *target)
{
	C4ScriptGuiWindow *window = ::Game.ScriptGuiRoot->GetChildByID(guiID);
	if (!window) return false;
	if (childID) // note: valid child IDs are always non-zero
	{
		C4ScriptGuiWindow *subwindow = window->GetSubWindow(childID, target);
		if (!subwindow) return false;
		subwindow->SetTag(tag);
		return true;
	}
	window->SetTag(tag);
	return true;
}

static bool FnGuiClose(C4PropList *_this, int32_t guiID, int32_t childID, C4Object *target)
{
	C4ScriptGuiWindow *window = ::Game.ScriptGuiRoot->GetChildByID(guiID);
	if (!window) return false;
	if (childID) // note: valid child IDs are always non-zero
	{
		C4ScriptGuiWindow *subwindow = window->GetSubWindow(childID, target);
		if (!subwindow) return false;
		subwindow->Close();
		return true;
	}
	window->Close();
	return true;
}

static bool FnGuiUpdate(C4PropList *_this, C4PropList *update, int32_t guiID, int32_t childID, C4Object *target)
{
	if (!update) return false;
	C4ScriptGuiWindow *window = ::Game.ScriptGuiRoot->GetChildByID(guiID);
	if (!window) return false;
	if (childID) // note: valid child IDs are always non-zero
	{
		C4ScriptGuiWindow *subwindow = window->GetSubWindow(childID, target);
		if (!subwindow) return false;
		subwindow->CreateFromPropList(update, false, true);
		return true;
	}
	window->CreateFromPropList(update, false, true);
	return true;
}

// undocumented!
static bool FnPauseGame(C4PropList * _this, bool fToggle)
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

static bool FnSetNextMission(C4PropList * _this, C4String *szNextMission, C4String *szNextMissionText, C4String *szNextMissionDesc)
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

static long FnGetPlayerControlState(C4PropList * _this, long iPlr, long iControl, bool fMovedState)
{
	// get control set to check
	C4PlayerControl *pCheckCtrl = nullptr;
	if (iPlr != NO_OWNER)
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
	// otherwise take either down-value or moved-value
	return fMovedState ? pControlState->MovedState.iStrength : pControlState->DownState.iStrength;
}

// undocumented!
static bool FnSetPlayerControlEnabled(C4PropList * _this, long iplr, long ctrl, bool is_enabled)
{
	// get control set to check
	C4PlayerControl *plrctrl = nullptr;
	if (iplr != NO_OWNER)
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

// undocumented!
static bool FnGetPlayerControlEnabled(C4PropList * _this, long iplr, long ctrl)
{
	// get control set to check
	C4PlayerControl *plrctrl = nullptr;
	if (iplr != NO_OWNER)
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

// undocumented!
static C4String *FnGetPlayerControlAssignment(C4PropList * _this, long player, long control, bool human_readable, bool short_name)
{
	// WARNING: As many functions returning strings, the result is not sync safe!
	// "" is returned for invalid controls to make the obvious if(GetPlayerControlAssignmentName(...)) not cause a sync loss
	// get desired assignment from parameters
	C4Player *plr = ::Players.Get(player);
	if (!plr) return nullptr; // invalid player
	if (!plr->ControlSet) return String(""); // player has no control (remote player)
	C4PlayerControlAssignment *assignment = plr->ControlSet->GetAssignmentByControl(control);
	if (!assignment) return String("");
	// get assignment as readable string
	return String(assignment->GetKeysAsString(human_readable, short_name).getData());
}

// strength: 0-1000, length: milliseconds
static bool FnPlayRumble(C4PropList * _this, long player, long strength, long length)
{
	// Check parameters.
	if (strength <= 0 || strength > 1000) return false;
	if (length <= 0) return false;
	// NO_OWNER: play rumble for all players (e.g. earthquakes)
	if (player == NO_OWNER)
	{
		for (C4Player *plr = ::Players.First; plr; plr=plr->Next)
			if (plr->Number != NO_OWNER) // can't happen, but would be a crash if it did...
				FnPlayRumble(_this, plr->Number, strength, length);
		return true;
	}
	C4Player *plr = ::Players.Get(player);
	if (!plr) return false;
	if (plr->pGamepad)
		plr->pGamepad->PlayRumble(strength / 1000.f, length);
	// We can't return whether the rumble was actually played.
	return true;
}

static bool FnStopRumble(C4PropList * _this, long player)
{
	// NO_OWNER: stop rumble for all players
	// Not sure whether this makes sense to do - mainly provided for symmetry with PlayRumble().
	if (player == NO_OWNER)
	{
		for (C4Player *plr = ::Players.First; plr; plr=plr->Next)
			if (plr->Number != NO_OWNER) // can't happen, but would be a crash if it did...
				FnStopRumble(_this, plr->Number);
		return true;
	}
	C4Player *plr = ::Players.Get(player);
	if (!plr) return false;
	if (plr->pGamepad)
		plr->pGamepad->StopRumble();
	return true;
}

static int32_t FnGetStartupPlayerCount(C4PropList * _this)
{
	// returns number of players when game was initially started
	return ::Game.StartupPlayerCount;
}

static int32_t FnGetStartupTeamCount(C4PropList * _this)
{
	// returns number of non-empty teams when game was initially started
	return ::Game.StartupTeamCount;
}

static bool FnGainScenarioAchievement(C4PropList * _this, C4String *achievement_name, Nillable<long> avalue, Nillable<long> player, C4String *for_scenario)
{
	// safety
	if (!achievement_name || !achievement_name->GetData().getLength()) return false;
	// default parameter
	long value = avalue.IsNil() ? 1 : (long)avalue;
	// gain achievement
	bool result = true;
	if (!player.IsNil() && player != NO_OWNER)
	{
		C4Player *plr = ::Players.Get(player);
		if (!plr) return false;
		result = plr->GainScenarioAchievement(achievement_name->GetCStr(), value, for_scenario ? for_scenario->GetCStr() : nullptr);
	}
	else
	{
		for (C4Player *plr = ::Players.First; plr; plr = plr->Next)
			if (!plr->GainScenarioAchievement(achievement_name->GetCStr(), value, for_scenario ? for_scenario->GetCStr() : nullptr))
				result = false;
	}
	return result;
}

static long FnGetPXSCount(C4PropList * _this, Nillable<long> iMaterial, Nillable<long> iX0, Nillable<long> iY0, Nillable<long> iWdt, Nillable<long> iHgt)
{
	if (iX0.IsNil())
	{
		// Search everywhere
		// All materials everywhere
		if (iMaterial.IsNil() || iMaterial == MNone) return ::PXS.GetCount();
		// Specific material everywhere
		return ::PXS.GetCount(iMaterial);
	}
	else
	{
		// Material in area; offset by caller
		int32_t x = iX0, y = iY0;
		if (Object(_this)) { x += Object(_this)->GetX(); y += Object(_this)->GetY(); }
		return ::PXS.GetCount(iMaterial.IsNil() ? MNone : static_cast<int32_t>(iMaterial), x, y, iWdt, iHgt);
	}
}

static C4String *FnGetTranslatedString(C4PropList * _this, const C4Value & string_data)
{
	// Resolve proplists containing localized strings to the current localization
	return ::Game.GetTranslatedString(string_data, nullptr, false);
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
	C4PropListStatic * p = pEngine->GetPropList();
	// add all def script funcs
	for (C4ScriptFnDef *pDef = &C4ScriptGameFnMap[0]; pDef->Identifier; pDef++)
		new C4AulDefFunc(p, pDef);
#define F(f) ::AddFunc(p, #f, Fn##f)
	F(GetX);
	F(GetY);
	F(GetDefinition);
	F(GetDefinitionGroupPath);
	F(GetPlayerName);
	F(GetPlayerType);
	F(GetPlayerColor);
	F(GetPlrClonkSkin);
	F(CreateObject);
	F(CreateObjectAbove);
	F(CreateConstruction);
	F(FindConstructionSite);
	F(CheckConstructionSite);
	F(Sound);
	F(SoundAt);
	F(ChangeSoundModifier);
	F(SetGlobalSoundModifier);
	F(Music);
	F(MusicLevel);
	F(SetPlayList);
	F(SetPlrView);
	F(SetPlrKnowledge);
	F(GetPlrViewMode);
	F(ResetCursorView);
	F(GetPlrView);
	F(GetWealth);
	F(SetWealth);
	F(DoPlayerScore);
	F(GetPlayerScore);
	F(GetPlayerScoreGain);
	F(GetWind);
	F(SetWind);
	F(GetTemperature);
	F(SetTemperature);
	F(ShakeFree);
	F(DigFree);
	F(DigFreeRect);
	F(ClearFreeRect);
	F(Hostile);
	F(SetHostility);
	F(PlaceVegetation);
	F(PlaceAnimal);
	F(GameOver);
	F(GetHiRank);
	F(GetCrew);
	F(GetCrewCount);
	F(GetPlayerCount);
	F(GetPlayerByIndex);
	F(EliminatePlayer);
	F(SurrenderPlayer);
	F(GetLeagueScore);
	F(SetLeaguePerformance);
	F(SetLeagueProgressData);
	F(GetLeagueProgressData);
	F(CreateScriptPlayer);
	F(GetCursor);
	F(GetViewCursor);
	F(SetCursor);
	F(SetViewCursor);
	F(GetMaterial);
	F(GetBackMaterial);
	F(GetTexture);
	F(GetBackTexture);
	F(GetAverageTextureColor);
	F(GetMaterialCount);
	F(GBackSolid);
	F(GBackSemiSolid);
	F(GBackLiquid);
	F(GBackSky);
	F(Material);
	F(BlastFree);
	F(InsertMaterial);
	F(CanInsertMaterial);
	F(LandscapeWidth);
	F(LandscapeHeight);
	F(SetAmbientBrightness);
	F(GetAmbientBrightness);
	F(SetSeason);
	F(GetSeason);
	F(SetClimate);
	F(GetClimate);
	F(SetPlayerZoomByViewRange);
	F(GetPlayerZoomLimits);
	F(SetPlayerZoom);
	F(SetPlayerViewLock);
	F(DoBaseMaterial);
	F(DoBaseProduction);
	F(GainMissionAccess);
	F(IsNetwork);
	F(IsEditor);
	F(GetLeague);
	::AddFunc(p, "TestMessageBoard", FnTestMessageBoard, false);
	::AddFunc(p, "CallMessageBoard", FnCallMessageBoard, false);
	::AddFunc(p, "AbortMessageBoard", FnAbortMessageBoard, false);
	F(SetFoW);
	F(SetMaxPlayer);
	F(Object);
	F(GetTime);
	F(GetMissionAccess);
	F(MaterialName);
	F(DrawMap);
	F(DrawDefMap);
	F(CreateParticle);
	F(ClearParticles);
	F(SetSky);
	F(SetSkyAdjust);
	F(SetMatAdjust);
	F(GetSkyAdjust);
	F(GetMatAdjust);
	F(SetSkyParallax);
	F(ReloadDef);
	F(ReloadParticle);
	F(SetGamma);
	F(ResetGamma);
	F(AddFragmentShader);
	F(RemoveShader);
	F(FrameCounter);
	F(DrawMaterialQuad);
	F(SetFilmView);
	F(AddMsgBoardCmd);
	::AddFunc(p, "SetGameSpeed", FnSetGameSpeed, false);
	::AddFunc(p, "DrawMatChunks", FnDrawMatChunks, false);
	F(GetPathLength);
	F(SetTextureIndex);
	F(RemoveUnusedTexMapEntries);
	F(SimFlight);
	F(LoadScenarioSection);
	F(SetViewOffset);
	::AddFunc(p, "SetPreSend", FnSetPreSend, false);
	F(GetPlayerID);
	F(GetPlayerTeam);
	F(SetPlayerTeam);
	F(GetScriptPlayerExtraID);
	F(GetTeamConfig);
	F(GetTeamName);
	F(GetTeamColor);
	F(GetTeamByIndex);
	F(GetTeamCount);
	::AddFunc(p, "InitScenarioPlayer", FnInitScenarioPlayer, false);
	F(SetScoreboardData);
	::AddFunc(p, "GetScoreboardString", FnGetScoreboardString, false);
	::AddFunc(p, "GetScoreboardData", FnGetScoreboardData, false);
	F(DoScoreboardShow);
	F(SortScoreboard);
	F(AddEvaluationData);
	F(HideSettlementScoreInEvaluation);
	F(ExtractMaterialAmount);
	F(CustomMessage);
	F(GuiOpen);
	F(GuiUpdateTag);
	F(GuiClose);
	F(GuiUpdate);
	::AddFunc(p, "PauseGame", FnPauseGame, false);
	F(PathFree);
	F(PathFree2);
	F(SetNextMission);
	F(GetPlayerControlState);
	F(SetPlayerControlEnabled);
	F(GetPlayerControlEnabled);
	F(GetPlayerControlAssignment);
	F(PlayRumble);
	F(StopRumble);
	F(GetStartupPlayerCount);
	F(GetStartupTeamCount);
	F(EditCursor);
	F(GainScenarioAchievement);
	F(GetPXSCount);
	F(GetPlrKnowledge);
	F(GetBaseMaterial);
	F(GetBaseProduction);
	F(GetDefCoreVal);
	F(GetObjectVal);
	F(GetObjectInfoCoreVal);
	F(GetScenarioVal);
	F(GetPlayerVal);
	F(GetPlayerInfoCoreVal);
	F(GetMaterialVal);
	F(SetPlrExtraData);
	F(GetPlrExtraData);
	F(PV_Linear);
	F(PV_Random);
	F(PV_Direction);
	F(PV_Step);
	F(PV_Speed);
	F(PV_Wind);
	F(PV_Gravity);
	// F(PV_KeyFrames); added below
	F(PV_Sin);
	F(PV_Cos);
	F(PC_Die);
	F(PC_Bounce);
	F(PC_Stop);
	F(IncinerateLandscape);
	F(GetGravity);
	F(SetGravity);
	F(GetTranslatedString);
#undef F
}

C4ScriptConstDef C4ScriptGameConstMap[]=
{
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
	{ "C4FO_Procedure"            ,C4V_Int,     C4FO_Procedure      },
	{ "C4FO_Container"            ,C4V_Int,     C4FO_Container      },
	{ "C4FO_AnyContainer"         ,C4V_Int,     C4FO_AnyContainer   },
	{ "C4FO_Owner"                ,C4V_Int,     C4FO_Owner          },
	{ "C4FO_Controller"           ,C4V_Int,     C4FO_Controller     },
	{ "C4FO_Func"                 ,C4V_Int,     C4FO_Func           },
	{ "C4FO_Layer"                ,C4V_Int,     C4FO_Layer          },
	{ "C4FO_InArray"              ,C4V_Int,     C4FO_InArray        },
	{ "C4FO_Property"             ,C4V_Int,     C4FO_Property       },
	{ "C4FO_AnyLayer"             ,C4V_Int,     C4FO_AnyLayer       },
	{ "C4FO_Cone"                 ,C4V_Int,     C4FO_Cone           },

	{ "MD_DragSource"             ,C4V_Int,     C4MC_MD_DragSource  },
	{ "MD_DropTarget"             ,C4V_Int,     C4MC_MD_DropTarget  },
	{ "MD_NoClick"                ,C4V_Int,     C4MC_MD_NoClick     },

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
	{ "C4SECT_ReinitScenario"     ,C4V_Int,      C4S_REINIT_SCENARIO },

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
	{ "MSG_Zoom"                  ,C4V_Int,      C4GM_Zoom },

	{ "C4PT_User"                 ,C4V_Int,      C4PT_User },
	{ "C4PT_Script"               ,C4V_Int,      C4PT_Script },

	{ "CSPF_FixedAttributes"      ,C4V_Int,      CSPF_FixedAttributes },
	{ "CSPF_NoScenarioInit"       ,C4V_Int,      CSPF_NoScenarioInit },
	{ "CSPF_NoEliminationCheck"   ,C4V_Int,      CSPF_NoEliminationCheck },
	{ "CSPF_Invisible"            ,C4V_Int,      CSPF_Invisible },
	{ "CSPF_NoScenarioSave"       ,C4V_Int,      CSPF_NoScenarioSave },

	{ "DMQ_Sky"                   ,C4V_Int,      DMQ_Sky },
	{ "DMQ_Sub"                   ,C4V_Int,      DMQ_Sub },
	{ "DMQ_Bridge"                ,C4V_Int,      DMQ_Bridge },

	{ "PLRZOOM_Direct"            ,C4V_Int,      PLRZOOM_Direct },
	{ "PLRZOOM_NoIncrease"        ,C4V_Int,      PLRZOOM_NoIncrease },
	{ "PLRZOOM_NoDecrease"        ,C4V_Int,      PLRZOOM_NoDecrease },
	{ "PLRZOOM_LimitMin"          ,C4V_Int,      PLRZOOM_LimitMin },
	{ "PLRZOOM_LimitMax"          ,C4V_Int,      PLRZOOM_LimitMax },
	{ "PLRZOOM_Set"               ,C4V_Int,      PLRZOOM_Set },

	{ "ATTACH_Front"              ,C4V_Int,      C4ATTACH_Front },
	{ "ATTACH_Back"               ,C4V_Int,      C4ATTACH_Back },
	{ "ATTACH_MoveRelative"       ,C4V_Int,      C4ATTACH_MoveRelative },

	// sound modifier type
	{ "C4SMT_Reverb"              ,C4V_Int,      C4SoundModifier::C4SMT_Reverb },
	{ "C4SMT_Echo"                ,C4V_Int,      C4SoundModifier::C4SMT_Echo },
	{ "C4SMT_Equalizer"           ,C4V_Int,      C4SoundModifier::C4SMT_Equalizer },

	{ "GUI_SetTag"               ,C4V_Int,      C4ScriptGuiWindowActionID::SetTag },
	{ "GUI_Call"                 ,C4V_Int,      C4ScriptGuiWindowActionID::Call },
	{ "GUI_GridLayout"           ,C4V_Int,      C4ScriptGuiWindowStyleFlag::GridLayout },
	{ "GUI_TightGridLayout"      ,C4V_Int,      C4ScriptGuiWindowStyleFlag::TightGridLayout },
	{ "GUI_VerticalLayout"       ,C4V_Int,      C4ScriptGuiWindowStyleFlag::VerticalLayout },
	{ "GUI_TextVCenter"          ,C4V_Int,      C4ScriptGuiWindowStyleFlag::TextVCenter },
	{ "GUI_TextHCenter"          ,C4V_Int,      C4ScriptGuiWindowStyleFlag::TextHCenter },
	{ "GUI_TextRight"            ,C4V_Int,      C4ScriptGuiWindowStyleFlag::TextRight },
	{ "GUI_TextBottom"           ,C4V_Int,      C4ScriptGuiWindowStyleFlag::TextBottom },
	{ "GUI_TextTop"              ,C4V_Int,      C4ScriptGuiWindowStyleFlag::None }, // note that top and left are considered default
	{ "GUI_TextLeft"             ,C4V_Int,      C4ScriptGuiWindowStyleFlag::None }, // they are only included for completeness
	{ "GUI_FitChildren"          ,C4V_Int,      C4ScriptGuiWindowStyleFlag::FitChildren },
	{ "GUI_Multiple"             ,C4V_Int,      C4ScriptGuiWindowStyleFlag::Multiple },
	{ "GUI_IgnoreMouse"          ,C4V_Int,      C4ScriptGuiWindowStyleFlag::IgnoreMouse },
	{ "GUI_NoCrop"               ,C4V_Int,      C4ScriptGuiWindowStyleFlag::NoCrop },

	// control states
	{ "CONS_Down"                ,C4V_Int,      C4PlayerControl::CONS_Down },
	{ "CONS_Up"                  ,C4V_Int,      C4PlayerControl::CONS_Up },
	{ "CONS_Moved"               ,C4V_Int,      C4PlayerControl::CONS_Moved },

	{ "PLRCON_MaxStrength"       ,C4V_Int,      C4GamePadControl::MaxStrength },

	{ nullptr, C4V_Nil, 0}
};

C4ScriptFnDef C4ScriptGameFnMap[]=
{
	{ "FindObject",    true, C4V_Object, { C4V_Array   ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}, FnFindObject    },
	{ "FindObjects",   true, C4V_Array,  { C4V_Array   ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}, FnFindObjects   },
	{ "ObjectCount",   true, C4V_Int,    { C4V_Array   ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}, FnObjectCount   },
	{ "GameCallEx",    true, C4V_Any,    { C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}, FnGameCallEx    },
	{ "PlayerMessage", true, C4V_Int,    { C4V_Int     ,C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}, FnPlayerMessage },
	{ "Message",       true, C4V_Bool,   { C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}, FnMessage       },
	{ "AddMessage",    true, C4V_Bool,   { C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}, FnAddMessage    },
	{ "PV_KeyFrames",  true, C4V_Array,  { C4V_Int     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}, FnPV_KeyFrames  },

	{ nullptr,            false, C4V_Nil,    { C4V_Nil     ,C4V_Nil     ,C4V_Nil     ,C4V_Nil     ,C4V_Nil     ,C4V_Nil     ,C4V_Nil    ,C4V_Nil    ,C4V_Nil    ,C4V_Nil}, nullptr               }
};
