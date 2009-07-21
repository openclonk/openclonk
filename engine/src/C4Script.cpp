/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/*
 * OpenClonk, http://www.openclonk.org
 * Copyright (c) 1998-2000, 2004, 2008  Matthes Bender
 * Copyright (c) 2001-2008  Peter Wortmann
 * Copyright (c) 2001-2009  Sven Eberhardt
 * Copyright (c) 2001  Michael Käser
 * Copyright (c) 2004-2005, 2007-2008  Armin Burgmeier
 * Copyright (c) 2004-2009  Günther Brammer
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

/* Functions mapped by C4Script */

#include <C4Include.h>
#include <C4Script.h>
#include <C4Version.h>

#ifndef BIG_C4INCLUDE
#include <C4Application.h>
#include <C4Object.h>
#include <C4ObjectInfo.h>
#include <C4ObjectCom.h>
#include <C4Random.h>
#include <C4Command.h>
#include <C4Console.h>
#include <C4Viewport.h>
#include <C4Log.h>
#include <C4ObjectInfoList.h>
#include <C4Player.h>
#include <C4ObjectMenu.h>
#include <C4RankSystem.h>
#include <C4PXS.h>
#include <C4MessageInput.h>
#include <C4GameMessage.h>
#include <C4Weather.h>
#include <C4GraphicsResource.h>
#include <C4GraphicsSystem.h>
#include <C4Texture.h>
#include <C4PlayerList.h>
#include <C4Game.h>
#include <C4GameObjects.h>
#include <C4GameControl.h>
#endif

//========================== Some Support Functions =======================================

char pscOSTR[500];

const long MaxFnStringParLen=500;

inline const static char *FnStringPar(C4String *pString)
{
	return pString ? pString->GetCStr() : "";
}
inline C4String *String(const char * str)
{
	return str ? ::Strings.RegString(str) : NULL;
}

static StdStrBuf FnStringFormat(C4AulContext *cthr, const char *szFormatPar, C4Value * Par0=0, C4Value * Par1=0, C4Value * Par2=0, C4Value * Par3=0,
	C4Value * Par4=0, C4Value * Par5=0, C4Value * Par6=0, C4Value * Par7=0, C4Value * Par8=0, C4Value * Par9=0)
	{
	C4Value * Par[11];
	Par[0]=Par0; Par[1]=Par1; Par[2]=Par2; Par[3]=Par3; Par[4]=Par4;
	Par[5]=Par5; Par[6]=Par6; Par[7]=Par7; Par[8]=Par8; Par[9]=Par9;
	Par[10] = 0;
	int cPar=0;

	StdStrBuf StringBuf("", false);
	const char * cpFormat = szFormatPar;
	const char * cpType;
	char szField[MaxFnStringParLen+1];
	while (*cpFormat)
		{
		// Copy normal stuff
		while (*cpFormat && (*cpFormat!='%'))
			StringBuf.AppendChar(*cpFormat++);
		// Field
		if (*cpFormat=='%')
			{
			// Scan field type
			for (cpType=cpFormat+1; *cpType && (*cpType=='.' || Inside(*cpType,'0','9')); cpType++) {}
			// Copy field
			SCopy(cpFormat,szField,cpType-cpFormat+1);
			// Insert field by type
			switch (*cpType)
				{
				// number
				case 'd': case 'x': case 'X': case 'c':
					{
					if (!Par[cPar]) throw new C4AulExecError(cthr->Obj, "format placeholder without parameter");
					StringBuf.AppendFormat(szField, Par[cPar++]->getInt());
					cpFormat+=SLen(szField);
					break;
					}
				// C4ID
				case 'i':
					{
					if (!Par[cPar]) throw new C4AulExecError(cthr->Obj, "format placeholder without parameter");
					C4ID id = Par[cPar++]->getC4ID();
					StringBuf.Append(C4IdText(id));
					cpFormat+=SLen(szField);
					break;
					}
				// C4Value
				case 'v':
					{
					if (!Par[cPar]) throw new C4AulExecError(cthr->Obj, "format placeholder without parameter");
					StringBuf.Append(static_cast<const StdStrBuf&>(Par[cPar++]->GetDataString()));
					cpFormat+=SLen(szField);
					break;
					}
				// String
				case 's':
					{
					// get string
					if (!Par[cPar]) throw new C4AulExecError(cthr->Obj, "format placeholder without parameter");
					const char *szStr = "(null)";
					if (Par[cPar]->GetData())
						{
						C4String * pStr = Par[cPar++]->getStr();
						if (!pStr) throw new C4AulExecError(cthr->Obj, "string format placeholder without string");
						szStr = pStr->GetCStr();
						}
					StringBuf.AppendFormat(szField, szStr);
					cpFormat+=SLen(szField);
					break;
					}
				case '%':
					StringBuf.AppendChar('%');
					cpFormat+=SLen(szField);
					break;
				// Undefined / Empty
				default:
					StringBuf.AppendChar('%');
					cpFormat++;
					break;
				}
			}
		}
	return StringBuf;
	}


bool CheckEnergyNeedChain(C4Object *pObj, C4ObjectList &rEnergyChainChecked)
	{

	if (!pObj) return FALSE;

	// No recursion, flag check
	if (rEnergyChainChecked.GetLink(pObj)) return FALSE;
	rEnergyChainChecked.Add(pObj, C4ObjectList::stNone);

	// This object needs energy
	if (pObj->Def->LineConnect & C4D_Power_Consumer)
		if (pObj->NeedEnergy)
			return TRUE;

	// Check all power line connected structures
  C4Object *cline; C4ObjectLink *clnk;
  for (clnk=::Objects.First; clnk && (cline=clnk->Obj); clnk=clnk->Next)
		if (cline->Status) if (cline->Def->id==C4ID_PowerLine)
			if (cline->Action.Target==pObj)
				if (CheckEnergyNeedChain(cline->Action.Target2,rEnergyChainChecked))
					return TRUE;

	return FALSE;
	}


DWORD StringBitEval(const char *str)
	{
	DWORD rval=0;
	for (int cpos=0; str && str[cpos]; cpos++)
		if ((str[cpos]!='_') && (str[cpos]!=' '))
			rval += 1 << cpos;
	return rval;
	}

typedef int32_t t_int;
typedef bool t_bool;
typedef C4ID t_id;
typedef C4Object *t_object;
typedef C4String *t_string;
typedef C4Value &t_ref;
typedef C4Value &t_any;
typedef C4ValueArray *t_array;

inline t_int getPar_int(C4Value *pVal) { return pVal->getInt(); }
inline t_bool getPar_bool(C4Value *pVal) { return pVal->getBool(); }
inline t_id getPar_id(C4Value *pVal) { return pVal->getC4ID(); }
inline t_object getPar_object(C4Value *pVal) { return pVal->getObj(); }
inline t_string getPar_string(C4Value *pVal) { return pVal->getStr(); }
inline t_ref getPar_ref(C4Value *pVal) { return pVal->GetRefVal(); }
inline t_any getPar_any(C4Value *pVal) { return pVal->GetRefVal(); }
inline t_array getPar_array(C4Value *pVal) { return pVal->getArray(); }

#define PAR(type, name) t_##type name = getPar_##type(pPars++)

//=============================== C4Script Functions ====================================

static C4Value Fn_this(C4AulContext *cthr, C4Value *pPars)
  {
  return C4VObj(cthr->Obj);
  }

static C4Value Fn_goto(C4AulContext *cthr, C4Value *pPars)
  {
	PAR(int, iCounter);

	Game.Script.Counter=iCounter;
  return C4VInt(iCounter);
  }

static C4Value FnChangeDef(C4AulContext *cthr, C4Value *pPars)
	{
	PAR(id, to_id); PAR(object, pObj);

	if (!pObj) pObj=cthr->Obj; if (!pObj) return C4VFalse;
	return C4VBool(!! pObj->ChangeDef(to_id));
	}

static C4Value FnExplode(C4AulContext *cthr, C4Value *pPars)
  {
	PAR(int, iLevel); PAR(object, pObj); PAR(id, idEffect); PAR(string, szEffect);

	if (!pObj) pObj=cthr->Obj; if (!pObj) return C4VFalse;
  pObj->Explode(iLevel, idEffect, FnStringPar(szEffect));
  return C4VTrue;
  }

static C4Value FnIncinerate(C4AulContext *cthr, C4Value *pPars)
  {
	PAR(object, pObj);

	if (!pObj) pObj=cthr->Obj; if (!pObj) return C4VFalse;
	long iCausedBy = NO_OWNER; if (cthr->Obj) iCausedBy = cthr->Obj->Controller;
  return C4VBool(!! pObj->Incinerate(iCausedBy));
  }

static C4Value FnIncinerateLandscape(C4AulContext *cthr, C4Value *pPars)
	{
	PAR(int, iX); PAR(int, iY);
	if (cthr->Obj) { iX += cthr->Obj->GetX(); iY += cthr->Obj->GetY(); }
	return C4VBool(!!::Landscape.Incinerate(iX, iY));
	}

static C4Value FnExtinguish(C4AulContext *cthr, C4Value *pPars)
  {
	PAR(object, pObj);

	if (!pObj) pObj=cthr->Obj; if (!pObj) return C4VFalse;
	// extinguish all fires
  return C4VBool(!! pObj->Extinguish(0));
  }

static C4Value FnSetSolidMask(C4AulContext *cthr, C4Value *pPars)
  {
	PAR(int, iX); PAR(int, iY); PAR(int, iWdt); PAR(int, iHgt); PAR(int, iTX); PAR(int, iTY); PAR(object, pObj);

	if (!pObj) pObj=cthr->Obj; if (!pObj) return C4VFalse;
	pObj->SetSolidMask(iX,iY,iWdt,iHgt,iTX,iTY);
  return C4VTrue;
  }

static C4Value FnSetGravity(C4AulContext *cthr, C4Value *pPars)
	{
	PAR(int, iGravity);

	::Landscape.Gravity = itofix(BoundBy<long>(iGravity,-300,300)) / 500;
	return C4VTrue;
	}

static C4Value FnGetGravity(C4AulContext *cthr, C4Value *pPars)
	{
	return C4VInt(fixtoi(::Landscape.Gravity * 500));
	}

static C4Value FnDeathAnnounce(C4AulContext *cthr, C4Value *pPars)
  {
	const long MaxDeathMsg=7;
  if (!cthr->Obj) return C4VFalse;
	// Check if crew member has an own death message
	*pscOSTR=0;
	char *szMsg;
	if (cthr->Obj->Info)
		if(*(szMsg = cthr->Obj->Info->DeathMessage))
			sprintf(pscOSTR, szMsg);
	if (!*pscOSTR)
		{
		char idDeathMsg[128+1]; sprintf(idDeathMsg, "IDS_OBJ_DEATH%d", 1 + SafeRandom(MaxDeathMsg));
		sprintf(pscOSTR,LoadResStr(idDeathMsg) ,cthr->Obj->GetName());
		}
	if (!Game.C4S.Head.Film)
		GameMsgObject(pscOSTR,cthr->Obj);
  return C4VTrue;
  }

static C4Value FnGrabContents(C4AulContext *cthr, C4Value *pPars)
  {
	PAR(object, from); PAR(object, pTo);

  if (!pTo) if (!(pTo=cthr->Obj)) return C4VFalse;
  if (!from) return C4VFalse;
	if (pTo == from) return C4VFalse;
	pTo->GrabContents(from);
  return C4VTrue;
  }

static C4Value FnPunch(C4AulContext *cthr, C4Value *pPars)
  {
	PAR(object, target); PAR(int, punch);

  if (!cthr->Obj) return C4VFalse;
  return C4VBool(!!ObjectComPunch(cthr->Obj,target,punch));
  }

static C4Value FnKill(C4AulContext *cthr, C4Value *pPars)
	{
	PAR(object, pObj); PAR(bool, fForced);

	if (!pObj) pObj=cthr->Obj; if (!pObj) return C4VFalse;
	if (!pObj->GetAlive()) return C4VFalse;
	// Trace kills by player-owned objects
	// Do not trace for NO_OWNER, because that would include e.g. the Suicide-rule
	if (cthr->Obj && ValidPlr(cthr->Obj->Controller)) pObj->UpdatLastEnergyLossCause(cthr->Obj->Controller);
	// Do the kill
	pObj->AssignDeath(!!fForced);
	return C4VTrue;
	}

static C4Value FnFling(C4AulContext *cthr, C4Value *pPars)
  {
	PAR(object, pObj); PAR(int, iXDir); PAR(int, iYDir); PAR(int, iPrec); PAR(bool, fAddSpeed);

  if (!pObj) return C4VFalse;
	if (!iPrec) iPrec=1;
  pObj->Fling(itofix(iXDir, iPrec),itofix(iYDir, iPrec),fAddSpeed);
	// unstick from ground, because Fling command may be issued in an Action-callback,
	// where attach-values have already been determined for that frame
	pObj->Action.t_attach=0;
	return C4VTrue;
  }

static C4Value FnJump(C4AulContext *cthr, C4Value *pPars)
  {
	PAR(object, pObj);

	if (!pObj) pObj=cthr->Obj; if (!pObj) return C4VFalse;
  return C4VBool(!!ObjectComJump(pObj));
  }

static C4Value FnEnter(C4AulContext *cthr, C4Value *pPars)
	{
	PAR(object, pTarget); PAR(object, pObj);

	if (!pObj) pObj=cthr->Obj; if (!pObj) return C4VFalse;
	return C4VBool(!!pObj->Enter(pTarget));
	}

static C4Value FnExit(C4AulContext *cthr, C4Value *pPars)
  {
	PAR(object, pObj);
	PAR(int, tx); PAR(int, ty); PAR(int, tr);
	PAR(int, txdir); PAR(int, tydir); PAR(int, trdir);

  if (!pObj) pObj=cthr->Obj; if (!pObj) return C4VFalse;
  if (cthr->Obj)
		{	tx+=cthr->Obj->GetX(); ty+=cthr->Obj->GetY();	}
  if (tr==-1) tr=Random(360);
  ObjectComCancelAttach(pObj);
  return C4VBool(!!pObj->Exit(tx,
															ty+pObj->Shape.y,
															tr,
															itofix(txdir),itofix(tydir),
															itofix(trdir) / 10));
  }

static C4Value FnCollect(C4AulContext *cthr, C4Value *pPars)
	{
	PAR(object, pItem); PAR(object, pCollector);

	// local call / safety
  if (!pCollector) pCollector=cthr->Obj; if (!pItem || !pCollector) return C4VFalse;
	// check OCF of collector (MaxCarry)
	if (pCollector->OCF & OCF_Collection)
		// collect
		return C4VBool(!!pCollector->Collect(pItem));
	// failure
	return C4VFalse;
	}

static C4Value FnSplit2Components(C4AulContext *cthr, C4Value *pPars)
  {
	PAR(object, pObj);

  C4Object *pThing,*pNew,*pContainer;
  long cnt,cnt2;
  // Pointer required
	if (!pObj) pObj=cthr->Obj; if (!pObj) return C4VFalse;
	// Store container
  pContainer=pObj->Contained;
  // Contents: exit / transfer to container
  while (pThing=pObj->Contents.GetObject())
    if (pContainer) pThing->Enter(pContainer);
    else pThing->Exit(pThing->GetX(),pThing->GetY());
  // Destroy the object, create its components
	C4IDList ObjComponents;
	pObj->Def->GetComponents(&ObjComponents, pObj, cthr->Obj);
  if (pObj->Contained) pObj->Exit(pObj->GetX(),pObj->GetY());
  for (cnt=0; ObjComponents.GetID(cnt); cnt++)
    for (cnt2=0; cnt2<ObjComponents.GetCount(cnt); cnt2++)
      if (pNew=Game.CreateObject(ObjComponents.GetID(cnt),
				                    pObj,
                            pObj->Owner,
                            pObj->GetX(),pObj->GetY(),Random(360),
                            itofix(Rnd3()),itofix(Rnd3()),itofix(Rnd3())))
        {
        if (pObj->GetOnFire()) pNew->Incinerate(pObj->Owner);
        if (pContainer) pNew->Enter(pContainer);
        }
  pObj->AssignRemoval();
  return C4VTrue;
  }

static bool FnRemoveObject(C4AulContext *cthr, C4Object *pObj, bool fEjectContents)
  {
  if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
  pObj->AssignRemoval(fEjectContents);
  return TRUE;
  }

static bool FnSetPosition(C4AulContext *cthr, long iX, long iY, C4Object *pObj, bool fCheckBounds)
	{
  if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;

	if (fCheckBounds)
	{
		// BoundsCheck takes ref to int and not to long
		int32_t i_x = iX, i_y = iY;
		pObj->BoundsCheck(i_x, i_y);
		iX = i_x; iY = i_y;
	}
	pObj->ForcePosition(iX,iY);
	// update liquid
	pObj->UpdateInLiquid();
	return TRUE;
	}

static bool FnDoCon(C4AulContext *cthr, long iChange, C4Object *pObj) // in percent
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
  pObj->DoCon(FullCon*iChange/100);
  return TRUE;
  }

static long FnGetCon(C4AulContext *cthr, C4Object *pObj) // in percent
	{
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
	return 100*pObj->GetCon()/FullCon;
	}

static bool FnDoEnergy(C4AulContext *cthr, long iChange, C4Object *pObj, bool fExact, long iEngType, long iCausedByPlusOne)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
	if (!iEngType) iEngType = C4FxCall_EngScript;
	int32_t iCausedBy = iCausedByPlusOne-1; if (!iCausedByPlusOne && cthr->Obj) iCausedBy = cthr->Obj->Controller;
  pObj->DoEnergy(iChange, !!fExact, iEngType, iCausedBy);
  return TRUE;
  }

static bool FnDoBreath(C4AulContext *cthr, long iChange, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
  pObj->DoBreath(iChange);
  return TRUE;
  }

static bool FnDoDamage(C4AulContext *cthr, long iChange, C4Object *pObj, long iDmgType, long iCausedByPlusOne)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
	int32_t iCausedBy = iCausedByPlusOne-1; if (!iCausedByPlusOne && cthr->Obj) iCausedBy = cthr->Obj->Controller;
	if (!iDmgType) iDmgType = C4FxCall_DmgScript;
  pObj->DoDamage(iChange,iCausedBy, iDmgType);
  return TRUE;
  }

static bool FnDoMagicEnergy(C4AulContext *cthr, long iChange, C4Object *pObj, bool fAllowPartial)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return false;
	// Physical modification factor
	iChange *= MagicPhysicalFactor;
	// Maximum load
	if (iChange>0)
		if (pObj->MagicEnergy+iChange>pObj->GetPhysical()->Magic)
			{
			if (!fAllowPartial) return false;
			iChange = pObj->GetPhysical()->Magic - pObj->MagicEnergy;
			if (!iChange) return false;
			// partial change to max allowed
			}
	// Insufficient load
	if (iChange<0)
		if (pObj->MagicEnergy+iChange<0)
			{
			if (!fAllowPartial) return false;
			iChange = -pObj->MagicEnergy;
			if (!iChange) return false;
			// partial change to zero allowed
			}
	// Change energy level
  pObj->MagicEnergy=BoundBy<long>(pObj->MagicEnergy+iChange,0,pObj->GetPhysical()->Magic);
	pObj->ViewEnergy = C4ViewDelay;
  return true;
  }

static long FnGetMagicEnergy(C4AulContext *cthr, C4Object *pObj)
	{
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
	return pObj->MagicEnergy/MagicPhysicalFactor;
	}
const int32_t PHYS_Current = 0,
              PHYS_Permanent = 1,
							PHYS_Temporary = 2,
							PHYS_StackTemporary = 3;

static bool FnSetPhysical(C4AulContext *cthr, C4String *szPhysical, long iValue, long iMode, C4Object *pObj)
	{
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
	// Get physical offset
	C4PhysicalInfo::Offset off;
	if (!C4PhysicalInfo::GetOffsetByName(FnStringPar(szPhysical), &off)) return FALSE;
	// Set by mode
	switch (iMode)
		{
		// Currently active physical
		case PHYS_Current:
			// Info objects or temporary mode only
			if (!pObj->PhysicalTemporary) if (!pObj->Info || Game.Parameters.UseFairCrew) return FALSE;
			// Set physical
			pObj->GetPhysical()->*off = iValue;
			return TRUE;
		// Permanent physical
		case PHYS_Permanent:
			// Info objects only
			if (!pObj->Info) return FALSE;
			// In fair crew mode, changing the permanent physicals is only allowed via TrainPhysical
			// Otherwise, stuff like SetPhysical(..., GetPhysical(...)+1, ...) would screw up the crew in fair crew mode
			if (Game.Parameters.UseFairCrew) return FALSE;
			// Set physical
			pObj->Info->Physical.*off = iValue;
			return TRUE;
		// Temporary physical
		case PHYS_Temporary:
		case PHYS_StackTemporary:
			// Automatically switch to temporary mode
			if (!pObj->PhysicalTemporary)
				{
				pObj->TemporaryPhysical = *(pObj->GetPhysical());
				pObj->PhysicalTemporary = true;
				}
			// if old value is to be remembered, register the change
			if (iMode == PHYS_StackTemporary)
				pObj->TemporaryPhysical.RegisterChange(off);
			// Set physical
			pObj->TemporaryPhysical.*off = iValue;
			return TRUE;
		}
	// Invalid mode
	return FALSE;
	}

static bool FnTrainPhysical(C4AulContext *cthr, C4String *szPhysical, long iTrainBy, long iMaxTrain, C4Object *pObj)
	{
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
	// Get physical offset
	C4PhysicalInfo::Offset off;
	if (!C4PhysicalInfo::GetOffsetByName(FnStringPar(szPhysical), &off)) return FALSE;
	// train it
	return !!pObj->TrainPhysical(off, iTrainBy, iMaxTrain);
	}

static bool FnResetPhysical(C4AulContext *cthr, C4Object *pObj, C4String *sPhysical)
	{
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
	const char *szPhysical = FnStringPar(sPhysical);

	// Reset to permanent physical
	if (!pObj->PhysicalTemporary) return FALSE;

	// reset specified physical only?
	if (szPhysical && *szPhysical)
		{
		C4PhysicalInfo::Offset off;
		if (!C4PhysicalInfo::GetOffsetByName(szPhysical, &off)) return FALSE;
		if (!pObj->TemporaryPhysical.ResetPhysical(off)) return FALSE;
		// if other physical changes remain, do not reset complete physicals
		if (pObj->TemporaryPhysical.HasChanges(pObj->GetPhysical(true))) return TRUE;
		}

	// actual reset of temp physicals
	pObj->PhysicalTemporary = false;
	pObj->TemporaryPhysical.Default();

	return TRUE;
	}

static long FnGetPhysical(C4AulContext *cthr, C4String *szPhysical, long iMode, C4Object *pObj, C4ID idDef)
	{
	// Get physical offset
	C4PhysicalInfo::Offset off;
	if (!C4PhysicalInfo::GetOffsetByName(FnStringPar(szPhysical), &off)) return 0;
	// no object given?
	if (!pObj)
		{
		// def given?
		if (idDef)
			{
			// get def
			C4Def *pDef=::Definitions.ID2Def(idDef); if (!pDef) return 0;
			// return physical value
			return pDef->Physical.*off;
			}
		// local call?
		pObj=cthr->Obj; if (!pObj) return 0;
		}

	// Get by mode
	switch (iMode)
		{
		// Currently active physical
		case PHYS_Current:
			// Get physical
			return pObj->GetPhysical()->*off;
		// Permanent physical
		case PHYS_Permanent:
			// Info objects only
			if (!pObj->Info) return 0;
			// In fair crew mode, scripts may not read permanent physical values - fallback to fair def physical instead!
			if (Game.Parameters.UseFairCrew)
				if (pObj->Info->pDef)
					return pObj->Info->pDef->GetFairCrewPhysicals()->*off;
				else
					return pObj->Def->GetFairCrewPhysicals()->*off;
			// Get physical
			return pObj->Info->Physical.*off;
		// Temporary physical
		case PHYS_Temporary:
			// Info objects only
			if (!pObj->Info) return 0;
			// Only if in temporary mode
			if (!pObj->PhysicalTemporary) return 0;
			// Get physical
			return pObj->TemporaryPhysical.*off;
		}
	// Invalid mode
	return 0;
	}

static bool FnSetEntrance(C4AulContext *cthr, long e_status, C4Object *pObj)
	{
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
	pObj->EntranceStatus = !! e_status;
	return TRUE;
	}


static bool FnSetXDir(C4AulContext *cthr, long nxdir, C4Object *pObj, long iPrec)
  {
	// safety
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
	// precision (default 10.0)
	if (!iPrec) iPrec=10;
	// update xdir
	pObj->xdir=itofix(nxdir, iPrec);
	// special: negative dirs must be rounded
	//if (nxdir<0) pObj->xdir += FIXED100(-50)/iPrec;
  pObj->Mobile=1;
	// success
  return TRUE;
  }

static bool FnSetRDir(C4AulContext *cthr, long nrdir, C4Object *pObj, long iPrec)
  {
	// safety
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
	// precision (default 10.0)
	if (!iPrec) iPrec=10;
	// update rdir
	pObj->rdir=itofix(nrdir, iPrec);
	// special: negative dirs must be rounded
	//if (nrdir<0) pObj->rdir += FIXED100(-50)/iPrec;
  pObj->Mobile=1;
	// success
  return TRUE;
  }

static bool FnSetYDir(C4AulContext *cthr, long nydir, C4Object *pObj, long iPrec)
  {
	// safety
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
	// precision (default 10.0)
	if (!iPrec) iPrec=10;
	// update ydir
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
	pObj->ydir=itofix(nydir, iPrec);
	// special: negative dirs must be rounded
	//if (nydir<0) pObj->ydir += FIXED100(-50)/iPrec;
  pObj->Mobile=1;
	// success
  return TRUE;
  }

static bool FnSetR(C4AulContext *cthr, long nr, C4Object *pObj)
  {
	// safety
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
	// set rotation
	pObj->SetRotation(nr);
	// success
  return TRUE;
  }

static bool FnSetAction(C4AulContext *cthr, C4String *szAction,
								 C4Object *pTarget, C4Object *pTarget2, bool fDirect)
  {
  if (!cthr->Obj) return FALSE;
  if (!szAction) return FALSE;
  return !!cthr->Obj->SetActionByName(FnStringPar(szAction),pTarget,pTarget2,
	C4Object::SAC_StartCall | C4Object::SAC_AbortCall,!!fDirect);
  }

static bool FnSetBridgeActionData(C4AulContext *cthr, long iBridgeLength, bool fMoveClonk, bool fWall, long iBridgeMaterial, C4Object *pObj)
	{
	if (!pObj) pObj=cthr->Obj; if (!pObj || !pObj->Status) return FALSE;
	// action must be BRIDGE
	if (!pObj->Action.pActionDef) return FALSE;
	if (pObj->Action.pActionDef->GetPropertyInt(P_Procedure) != DFA_BRIDGE) return FALSE;
	// set data
	pObj->Action.SetBridgeData(iBridgeLength, fMoveClonk, fWall, iBridgeMaterial);
	return TRUE;
	}

static bool FnSetActionData(C4AulContext *cthr, long iData, C4Object *pObj)
	{
	if (!pObj) pObj=cthr->Obj; if (!pObj || !pObj->Status) return FALSE;
	// bridge: Convert from old style
	if (pObj->Action.pActionDef && (pObj->Action.pActionDef->GetPropertyInt(P_Procedure) == DFA_BRIDGE))
		return FnSetBridgeActionData(cthr, 0, false, false, iData, pObj);
	// attach: check for valid vertex indices
	if (pObj->Action.pActionDef && (pObj->Action.pActionDef->GetPropertyInt(P_Procedure) == DFA_ATTACH)) // Fixed Action.Act check here... matthes
		if (((iData&255) >= C4D_MaxVertex) || ((iData>>8) >= C4D_MaxVertex))
			return FALSE;
	// set data
	pObj->Action.Data = iData;
	return TRUE;
	}

static bool FnObjectSetAction(C4AulContext *cthr, C4Object *pObj, C4String *szAction,
											C4Object *pTarget, C4Object *pTarget2, bool fDirect)
	{
	if (!szAction || !pObj) return FALSE;
	// regular action change
	return !!pObj->SetActionByName(FnStringPar(szAction),pTarget,pTarget2,
		C4Object::SAC_StartCall | C4Object::SAC_AbortCall,!!fDirect);
	}

static bool FnSetComDir(C4AulContext *cthr, long ncomdir, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
  pObj->Action.ComDir=ncomdir;
  return TRUE;
  }

static bool FnSetDir(C4AulContext *cthr, long ndir, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
  pObj->SetDir(ndir);
  return TRUE;
  }

static bool FnSetCategory(C4AulContext *cthr, long iCategory, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
	if (!(iCategory & C4D_SortLimit)) iCategory |= (pObj->Category & C4D_SortLimit);
  pObj->SetCategory(iCategory);
  return TRUE;
  }

static bool FnSetAlive(C4AulContext *cthr, bool nalv, C4Object *pObj)
	{
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
	pObj->SetAlive(nalv);
	return TRUE;
	}

static bool FnSetOwner(C4AulContext *cthr, long iOwner, C4Object *pObj)
  {
	// Object safety
	if (!pObj) pObj=cthr->Obj; if (!pObj) return false;
	// Set owner
	return !!pObj->SetOwner(iOwner);
  }

static bool FnSetPhase(C4AulContext *cthr, long iVal, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return false;
	return !!pObj->SetPhase(iVal);
  }

static bool FnExecuteCommand(C4AulContext *cthr, C4Object *pObj)
	{
	if (!pObj) pObj=cthr->Obj; if (!pObj) return false;
	return !!pObj->ExecuteCommand();
	}

static C4Value FnSetCommand(C4AulContext *cthr, C4Value *pPars)
  {
	PAR(object, pObj); PAR(string, szCommand); PAR(object, pTarget); PAR(any, Tx); PAR(int, iTy);
	PAR(object, pTarget2); PAR(any, Data); PAR(int, iRetries);
	// Object
	if (!pObj) pObj=cthr->Obj; if (!pObj || !szCommand) return C4VFalse;
	// Command
	long iCommand = CommandByName(FnStringPar(szCommand));
	if (!iCommand) { pObj->ClearCommands(); return C4VFalse; }
	// Special: convert iData to szText
	C4String *szText=NULL;
	if (iCommand==C4CMD_Call)
		{
		szText=Data.getStr();
		}
	else
		{
		Tx.ConvertTo(C4V_Int);
		}
	// Set
	pObj->SetCommand(iCommand,pTarget,Tx,iTy,pTarget2,FALSE,Data,iRetries,szText);
	// Success
  return C4VTrue;
  }

static C4Value FnAddCommand(C4AulContext *cthr, C4Value *pPars)
	{
	PAR(object, pObj); PAR(string, szCommand); PAR(object, pTarget); PAR(any, Tx); PAR(int, iTy);
	PAR(object, pTarget2); PAR(int, iUpdateInterval); PAR(any, Data); PAR(int, iRetries); PAR(int, iBaseMode);
	// Object
	if (!pObj) pObj=cthr->Obj; if (!pObj || !szCommand) return C4VFalse;
	// Command
	long iCommand = CommandByName(FnStringPar(szCommand));
	if (!iCommand) return C4VFalse;
	// Special: convert iData to szText
	C4String *szText=NULL;
	if (iCommand==C4CMD_Call)
		{
		szText=Data.getStr();
		}
	else
		{
		Tx.ConvertTo(C4V_Int);
		}
	// Add
	return C4VBool(pObj->AddCommand(iCommand,pTarget,Tx,iTy,iUpdateInterval,pTarget2,TRUE,Data,FALSE,iRetries,szText,iBaseMode));
	}

static C4Value FnAppendCommand(C4AulContext *cthr, C4Value *pPars)
	{
	PAR(object, pObj); PAR(string, szCommand); PAR(object, pTarget); PAR(any, Tx); PAR(int, iTy);
	PAR(object, pTarget2); PAR(int, iUpdateInterval); PAR(any, Data); PAR(int, iRetries); PAR(int, iBaseMode);
	// Object
	if (!pObj) pObj=cthr->Obj; if (!pObj || !szCommand) return C4VFalse;
	// Command
	long iCommand = CommandByName(FnStringPar(szCommand));
	if (!iCommand) return C4VFalse;
	// Special: convert iData to szText
	C4String *szText=NULL;
	if (iCommand==C4CMD_Call)
		{
		szText=Data.getStr();
		}
	else
		{
		Tx.ConvertTo(C4V_Int);
		}
	// Add
	return C4VBool(pObj->AddCommand(iCommand,pTarget,Tx,iTy,iUpdateInterval,pTarget2,TRUE,Data,TRUE,iRetries,szText,iBaseMode));
	}

static C4Value FnGetCommand(C4AulContext *cthr, C4Value *pPars)
	{
	PAR(object, pObj); PAR(int, iElement); PAR(int, iCommandNum);

	if (!pObj) pObj=cthr->Obj; if (!pObj) return C4VNull;
	C4Command * Command = pObj->Command;
	// Move through list to Command iCommandNum
	while (Command && iCommandNum--) Command = Command->Next;
	// Object has no command or iCommandNum was to high or < 0
	if (!Command) return C4VNull;
	// Return command element
	switch (iElement)
		{
		case 0: // Name
			return C4VString(CommandName(Command->Command));
		case 1: // Target
			return C4VObj(Command->Target);
		case 2: // Tx
			return Command->Tx;
		case 3: // Ty
			return C4VInt(Command->Ty);
		case 4: // Target2
			return C4VObj(Command->Target2);
		case 5: // Data
			return Command->Command == C4CMD_Call ? C4VString(Command->Text) : Command->Data;
		}
	// Undefined element
	return C4VNull;
	}

static bool FnFinishCommand(C4AulContext *cthr, C4Object *pObj, bool fSuccess, long iCommandNum)
	{
	if (!pObj) pObj=cthr->Obj; if (!pObj) return false;
	C4Command * Command = pObj->Command;
	// Move through list to Command iCommandNum
	while (Command && iCommandNum--) Command = Command->Next;
	// Object has no command or iCommandNum was to high or < 0
	if (!Command) return false;
	if (!fSuccess) ++(Command->Failures);
	else Command->Finished = true;
	return TRUE;
	}

static C4Value FnPlayerObjectCommand(C4AulContext *cthr, C4Value *pPars)
  {
	PAR(int, iPlr); PAR(string, szCommand); PAR(object, pTarget); PAR(any, Tx); PAR(int, iTy);
	PAR(object, pTarget2); PAR(any, Data);
	// Player
	if (!ValidPlr(iPlr) || !szCommand) return C4VFalse;
	C4Player *pPlr = ::Players.Get(iPlr);
	// Command
	long iCommand = CommandByName(FnStringPar(szCommand)); if (!iCommand) return C4VFalse;
	// Special: convert iData to szText
	const char *szText=NULL;
	int32_t iTx;
	if (iCommand==C4CMD_Call)
		{
		szText=FnStringPar(Data.getStr());
		iTx=Tx.GetData().Int;
		}
	else
		{
		iTx=Tx.getInt();
		}
	// Set
	pPlr->ObjectCommand(iCommand, pTarget, iTx, iTy, pTarget2, Data, C4P_Command_Set);
	// Success
  return C4VTrue;
  }

static C4String *FnGetAction(C4AulContext *cthr, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
  if (!pObj->Action.pActionDef) return String("Idle");
  return String(pObj->Action.pActionDef->GetName());
  }

static C4PropList * FnCreatePropList(C4AulContext *cthr, C4PropList * prototype)
	{
	return new C4PropList(prototype);
	}

static C4Value FnGetProperty_C4V(C4AulContext *cthr, C4Value * key_C4V, C4Value * pObj_C4V)
	{
	C4PropList * pObj = pObj_C4V->_getPropList();
	if (!pObj) pObj=cthr->Obj;
	if (!pObj) pObj=cthr->Def;
	if (!pObj) return C4VFalse;
	C4String * key = key_C4V->_getStr();
	if(!key) return C4VFalse;
	C4Value r;
	pObj->GetProperty(key, r);
	return r;
	}

static C4Value FnSetProperty_C4V(C4AulContext *cthr, C4Value * key_C4V, C4Value * to, C4Value * pObj_C4V)
	{
	C4PropList * pObj = pObj_C4V->_getPropList();
	if (!pObj) pObj=cthr->Obj;
	if (!pObj) pObj=cthr->Def;
	if (!pObj) return C4VFalse;
	C4String * key = key_C4V->_getStr();
	if(!key) return C4VFalse;
	pObj->SetProperty(key, *to);
	return C4VTrue;
	}

static C4String *FnGetName(C4AulContext *cthr, C4Object *pObj, C4ID idDef/*, bool fFilename, long idx*/)
  {
	// Def name
	C4Def *pDef;
	if (idDef /*|| idx*/)
		{
		/*if (idx)
			{
			pDef = ::Definitions.GetDef(idx-1);
			}
		else*/
			pDef=C4Id2Def(idDef);
		if (pDef)
			/*if (fFilename)
				return String(pDef->Filename);
			else*/
				return String(pDef->GetName());
		return NULL;
		}
	// Object name
	if (!pObj) pObj=cthr->Obj; if (!pObj) return NULL;
	return String(pObj->GetName());
  }

static bool FnSetName(C4AulContext *cthr, C4String *pNewName, C4Object *pObj, C4ID idDef, bool fSetInInfo, bool fMakeValidIfExists)
  {
	// safety
	if (fSetInInfo && idDef) return false;

	// Def name
	C4Def *pDef;

	if (idDef)
		if (pDef=C4Id2Def(idDef))
			pDef->SetName(FnStringPar(pNewName));
		else
			return false;
	else
	{
		// Object name
		if (!pObj) pObj=cthr->Obj; if(!pObj) return false;
		if (fSetInInfo)
			{

			// setting name in info
			C4ObjectInfo *pInfo = pObj->Info;
			if (!pInfo) return false;
			const char *szName = pNewName->GetCStr();
			// empty names are bad; e.g., could cause problems in savegames
			if (!szName || !*szName) return false;
			// name must not be too long
			if (SLen(szName) > C4MaxName) return false;
			// any change at all?
			if (SEqual(szName, pInfo->Name)) return true;
			// make sure names in info list aren't duplicated
			// querying owner info list here isn't 100% accurate, as infos might have been stolen by other players
			// however, there is no good way to track the original list ATM
			C4ObjectInfoList *pInfoList = NULL;
			C4Player *pOwner = ::Players.Get(pObj->Owner);
			if (pOwner) pInfoList = &pOwner->CrewInfoList;
			char NameBuf[C4MaxName+1];
			if (pInfoList) if (pInfoList->NameExists(szName))
				{
				if (!fMakeValidIfExists) return false;
				SCopy(szName, NameBuf, C4MaxName);
				pInfoList->MakeValidName(NameBuf);
				szName = NameBuf;
				}
			SCopy(szName, pInfo->Name, C4MaxName);
			pObj->SetName(); // make sure object uses info name
			}
		else
			{
			if (!pNewName) pObj->SetName ();
			else pObj->SetName(pNewName->GetCStr());
			}
	}
	return true;
  }

static C4String *FnGetDesc(C4AulContext *cthr, C4Object *pObj, C4ID idDef)
  {
	C4Def *pDef;
	// find def
	if (!pObj && !idDef) pObj = cthr->Obj;
	if (pObj)
		pDef = pObj->Def;
	else
		pDef = ::Definitions.ID2Def(idDef);
	// nothing found?
	if(!pDef) return NULL;
	// return desc
	return String(pDef->GetDesc());
  }

static C4String *FnGetPlayerName(C4AulContext *cthr, long iPlayer)
  {
	if (!ValidPlr(iPlayer)) return NULL;
	return String(::Players.Get(iPlayer)->GetName());
	}

static C4String *FnGetTaggedPlayerName(C4AulContext *cthr, long iPlayer)
  {
	C4Player *pPlr = ::Players.Get(iPlayer);
	if (!pPlr) return NULL;
	DWORD dwClr = pPlr->ColorDw; C4GUI::MakeColorReadableOnBlack(dwClr);
	static char szFnFormatBuf[1024+1];
	sprintf(szFnFormatBuf, "<c %x>%s</c>", dwClr&0xffffff, pPlr->GetName());
	return String(szFnFormatBuf);
	}

static long FnGetPlayerType(C4AulContext *cthr, long iPlayer)
	{
	C4Player *pPlr = ::Players.Get(iPlayer);
	if (!pPlr) return 0;
	return pPlr->GetType();
	}

static C4Object *FnGetActionTarget(C4AulContext *cthr, long target_index, C4Object *pObj)
  {
  if (!pObj) pObj=cthr->Obj; if (!pObj) return NULL;
	if (target_index==0) return pObj->Action.Target;
	if (target_index==1) return pObj->Action.Target2;
	return NULL;
  }

static bool FnSetActionTargets(C4AulContext *cthr, C4Object *pTarget1, C4Object *pTarget2, C4Object *pObj)
  {
	// safety
  if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
	// set targets
	pObj->Action.Target=pTarget1;
	pObj->Action.Target2=pTarget2;
	return TRUE;
  }

static long FnGetDir(C4AulContext *cthr, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return 0;
  return pObj->Action.Dir;
  }

static long FnGetEntrance(C4AulContext *cthr, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return 0;
  return pObj->EntranceStatus;
  }

static long FnGetPhase(C4AulContext *cthr, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return 0;
  return pObj->Action.Phase;
  }

static long FnGetEnergy(C4AulContext *cthr, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return 0;
	return 100*pObj->Energy/C4MaxPhysical;
  }

static long FnGetBreath(C4AulContext *cthr, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return 0;
	return 100*pObj->Breath/C4MaxPhysical;
  }

static long FnGetMass(C4AulContext *cthr, C4Object *pObj, C4ID idDef)
  {
	if (idDef)
		{
		C4Def *pDef = ::Definitions.ID2Def(idDef);
		if (!pDef) return 0;
		return pDef->Mass;
		}
	if (!pObj) pObj=cthr->Obj; if (!pObj) return 0;
	return pObj->Mass;
  }

static long FnGetRDir(C4AulContext *cthr, C4Object *pObj, long iPrec)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return 0;
	if (!iPrec) iPrec = 10;
	return fixtoi(pObj->rdir, iPrec);
  }

static long FnGetXDir(C4AulContext *cthr, C4Object *pObj, long iPrec)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return 0;
	if (!iPrec) iPrec = 10;
	return fixtoi(pObj->xdir, iPrec);
  }

static long FnGetYDir(C4AulContext *cthr, C4Object *pObj, long iPrec)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return 0;
	if (!iPrec) iPrec = 10;
	return fixtoi(pObj->ydir, iPrec);
  }

static long FnGetR(C4AulContext *cthr, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return 0;
	// Adjust range
	long iR = pObj->r;
	while (iR>180) iR-=360;
	while (iR<-180) iR+=360;
  return iR;
  }

static long FnGetComDir(C4AulContext *cthr, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return 0;
  return pObj->Action.ComDir;
  }

static long FnGetX(C4AulContext *cthr, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return 0;
  return pObj->GetX();
  }

static long FnGetVertexNum(C4AulContext *cthr, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return 0;
  return pObj->Shape.VtxNum;
  }

static const int VTX_X        = 0, // vertex data indices
             VTX_Y        = 1,
						 VTX_CNAT     = 2,
						 VTX_Friction = 3,
						 VTX_SetPermanent    = 1,
						 VTX_SetPermanentUpd = 2;

static long FnGetVertex(C4AulContext *cthr, long iIndex, long iValueToGet, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return 0;
	if (pObj->Shape.VtxNum<1) return FALSE;
	iIndex=Min<long>(iIndex,pObj->Shape.VtxNum-1);
	switch (iValueToGet)
		{
		case VTX_X: return pObj->Shape.VtxX[iIndex]; break;
		case VTX_Y: return pObj->Shape.VtxY[iIndex]; break;
		case VTX_CNAT: return pObj->Shape.VtxCNAT[iIndex]; break;
		case VTX_Friction: return pObj->Shape.VtxFriction[iIndex]; break;
		default:
			// old-style behaviour for any value != 0 (normally not used)
			DebugLog(FormatString("FnGetVertex: Unknown vertex attribute: %ld; getting VtxY", iValueToGet).getData());
			return pObj->Shape.VtxY[iIndex];
			break;
		}
	// impossible mayhem!
	return 0;
  }

static bool FnSetVertex(C4AulContext *cthr, long iIndex, long iValueToSet, long iValue, C4Object *pObj, long iOwnVertexMode)
  {
	// local call / safety
	if (!pObj) pObj=cthr->Obj; if (!pObj || !pObj->Status) return FALSE;
	// own vertex mode?
	if (iOwnVertexMode)
		{
		// enter own custom vertex mode if not already set
		if (!pObj->fOwnVertices)
			{
			pObj->Shape.CreateOwnOriginalCopy(pObj->Def->Shape);
			pObj->fOwnVertices = 1;
			}
		// set vertices at end of buffer
		iIndex += C4D_VertexCpyPos;
		}
	// range check
	if (!Inside<long>(iIndex,0,C4D_MaxVertex-1)) return FALSE;
	// set desired value
	switch (iValueToSet)
		{
		case VTX_X: pObj->Shape.VtxX[iIndex]=iValue; break;
		case VTX_Y: pObj->Shape.VtxY[iIndex]=iValue; break;
		case VTX_CNAT: pObj->Shape.VtxCNAT[iIndex]=iValue; break;
		case VTX_Friction: pObj->Shape.VtxFriction[iIndex]=iValue; break;
		default:
			// old-style behaviour for any value != 0 (normally not used)
			pObj->Shape.VtxY[iIndex]=iValue;
			DebugLogF("FnSetVertex: Unknown vertex attribute: %ld; setting VtxY", iValueToSet);
			break;
		}
	// vertex update desired?
	if (iOwnVertexMode==VTX_SetPermanentUpd) pObj->UpdateShape(true);
	return TRUE;
  }

static bool FnAddVertex(C4AulContext *cthr, long iX, long iY, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return false;
	return !!pObj->Shape.AddVertex(iX,iY);
  }

static bool FnRemoveVertex(C4AulContext *cthr, long iIndex, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return false;
	return !!pObj->Shape.RemoveVertex(iIndex);
  }

static bool FnSetContactDensity(C4AulContext *cthr, long iDensity, C4Object *pObj)
	{
	if(!pObj) pObj=cthr->Obj; if(!pObj) return FALSE;
	pObj->Shape.ContactDensity = iDensity;
	return TRUE;
	}

static long FnGetY(C4AulContext *cthr, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return 0;
  return pObj->GetY();
  }

static long FnGetAlive(C4AulContext *cthr, C4Object *pObj)
	{
	if (!pObj) pObj=cthr->Obj; if (!pObj) return 0;
	return pObj->GetAlive();
	}

static long FnGetOwner(C4AulContext *cthr, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return NO_OWNER;
  return pObj->Owner;
  }

static long FnCrewMember(C4AulContext *cthr, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
  return pObj->Def->CrewMember;
  }

static long FnGetController(C4AulContext *cthr, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return NO_OWNER;
  return pObj->Controller;
  }

static bool FnSetController(C4AulContext *cthr, long iNewController, C4Object *pObj)
  {
	// validate player
	if (iNewController != NO_OWNER && !ValidPlr(iNewController)) return FALSE;
	// Object safety
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
	// Set controller
	pObj->Controller = iNewController;
	return TRUE;
  }

static long FnGetKiller(C4AulContext *cthr, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return NO_OWNER;
  return pObj->LastEnergyLossCausePlayer;
  }

static bool FnSetKiller(C4AulContext *cthr, long iNewKiller, C4Object *pObj)
  {
	// validate player
	if (iNewKiller != NO_OWNER && !ValidPlr(iNewKiller)) return FALSE;
	// object safety
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
	// set killer as last energy loss cause
  pObj->LastEnergyLossCausePlayer = iNewKiller;
	return TRUE;
  }

static long FnGetCategory(C4AulContext *cthr, C4Object *pObj, C4ID idDef)
  {
	// Def category
	C4Def *pDef;
	if (idDef) if (pDef=C4Id2Def(idDef)) return pDef->Category;
	// Object category
	if (!pObj) pObj=cthr->Obj; if (!pObj) return 0;
	return pObj->Category;
  }

static long FnGetOCF(C4AulContext *cthr, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
  return pObj->OCF;
  }

static long FnGetDamage(C4AulContext *cthr, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
  return pObj->Damage;
  }

static long FnGetValue(C4AulContext *cthr, C4Object *pObj, C4ID idDef, C4Object *pInBase, long iForPlayer)
  {
	// Def value
	C4Def *pDef;
	if (idDef)
		// return Def value or 0 if def unloaded
		if (pDef=C4Id2Def(idDef)) return pDef->GetValue(pInBase, iForPlayer); else return 0;
	// Object value
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
  return pObj->GetValue(pInBase, iForPlayer);
  }

static long FnGetRank(C4AulContext *cthr, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
	if (!pObj->Info) return 0;
  return pObj->Info->Rank;
  }

static long FnValue(C4AulContext *cthr, C4ID id)
  {
	C4Def *pDef = C4Id2Def(id);
	if (pDef) return pDef->Value;
  return 0;
  }

static long FnGetActTime(C4AulContext *cthr, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
  return pObj->Action.Time;
  }

static C4ID FnGetID(C4AulContext *cthr, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj;
	C4Def *pDef = pObj ? pObj->Def : cthr->Def;
	if(!pDef) return 0;
	// return id of object
  return pDef->id;
  }

static long FnGetBase(C4AulContext *cthr, C4Object *pObj)
	{
  if (!pObj) pObj=cthr->Obj; if (!pObj) return -1;
	return pObj->Base;
	}

static C4ID FnGetMenu(C4AulContext *cthr, C4Object *pObj)
	{
  if (!pObj) pObj=cthr->Obj; if (!pObj) return C4ID(-1);
	if (pObj->Menu && pObj->Menu->IsActive())
		return pObj->Menu->GetIdentification();
	return C4MN_None;
	}

static bool FnCreateMenu(C4AulContext *cthr, C4ID iSymbol, C4Object *pMenuObj, C4Object *pCommandObj,
																 long iExtra, C4String *szCaption, long iExtraData,
																 long iStyle, bool fPermanent, C4ID idMenuID)
	{
  if (!pMenuObj) { pMenuObj=cthr->Obj; if (!pMenuObj) return false; }
  if (!pCommandObj) pCommandObj=cthr->Obj;
	if (pCommandObj)
		{
		// object menu: Validate object
		if (!pCommandObj->Status) return false;
		}
	else
		{
		// scenario script callback: No command object OK
		}

	// Create symbol
	C4Def *pDef;
	C4FacetSurface fctSymbol;
	fctSymbol.Create(C4SymbolSize,C4SymbolSize);
	if (pDef = C4Id2Def(iSymbol))	pDef->Draw(fctSymbol);

	// Clear any old menu, init new menu
	if (!pMenuObj->CloseMenu(false)) return false;
	if (!pMenuObj->Menu) pMenuObj->Menu = new C4ObjectMenu; else pMenuObj->Menu->ClearItems(true);
	pMenuObj->Menu->Init(fctSymbol,FnStringPar(szCaption),pCommandObj,iExtra,iExtraData,idMenuID ? idMenuID : iSymbol,iStyle,TRUE);

	// Set permanent
	pMenuObj->Menu->SetPermanent(fPermanent);

	return true;
	}

const int C4MN_Add_ImgRank     =   1,
					C4MN_Add_ImgIndexed  =   2,
					C4MN_Add_ImgObjRank  =   3,
					C4MN_Add_ImgObject   =   4,
					C4MN_Add_ImgTextSpec =   5,
					C4MN_Add_ImgColor    =   6,
					C4MN_Add_MaxImage    = 127, // mask for param which decides what to draw as the menu symbol
					C4MN_Add_PassValue   = 128,
          C4MN_Add_ForceCount  = 256,
					C4MN_Add_ForceNoDesc = 512;

#ifndef _MSC_VER
#define _snprintf snprintf
#endif

static C4Value FnAddMenuItem(C4AulContext *cthr, C4Value *pPars)
{
	PAR(string, szCaption);
	PAR(string, szCommand);
	PAR(id,			idItem);
	PAR(object, pMenuObj);
	PAR(int,		iCount);
	PAR(any,		Parameter);
	PAR(string, szInfoCaption);
	PAR(int,		iExtra);
	PAR(any,		XPar);
	PAR(any,		XPar2);

	if (!pMenuObj) pMenuObj=cthr->Obj; if (!pMenuObj) return C4VBool(FALSE);
	if (!pMenuObj->Menu) return C4VBool(FALSE);

	char caption[256+1];
	char parameter[256+1];
	char dummy[256+1];
	char command[512+1];
	char command2[512+1];
	char infocaption[C4MaxTitle+1];

	// get needed symbol size
	int iSymbolSize = pMenuObj->Menu->GetSymbolSize();

	// Check specified def
	C4Def *pDef = C4Id2Def(idItem);
	if (!pDef) pDef=pMenuObj->Def;

	// Compose caption with def name
	if(szCaption)
		{
		const char * s = FnStringPar(szCaption);
		const char * sep = strstr(s, "%s");
		if (sep)
			{
			strncpy(caption, s, Min<intptr_t>(sep - s,256));
			caption[Min<intptr_t>(sep - s,256)] = 0;
			strncat(caption, pDef->GetName(), 256);
			strncat(caption, sep + 2, 256);
			}
		else
			{
			strncpy(caption, s, 256);
			caption[256] = 0;
			}
		}
	else
		caption[0] = 0;

	// create string to include type-information in command
	switch(Parameter.GetType())
	{
	case C4V_Int:
		sprintf(parameter, "%d", Parameter.getInt());
		break;
	case C4V_Bool:
		SCopy(Parameter.getBool() ? "true" : "false", parameter);
		break;
	case C4V_C4Object:
	case C4V_PropList:
		sprintf(parameter, "Object(%d)", Parameter.getPropList()->Number);
		break;
	case C4V_String:
    // note this breaks if there is '"' in the string.
    parameter[0] = '"';
    SCopy(Parameter.getStr()->GetCStr(), parameter + 1, sizeof(command)-3);
    SAppendChar('"', command);
		break;
	case C4V_Any:
		sprintf(parameter, "CastAny(%ld)", Parameter.GetData().Int);
		break;
	case C4V_Array:
		// Arrays were never allowed, so tell the scripter
		throw new C4AulExecError(cthr->Obj, "array as parameter to AddMenuItem");
	default:
		return C4VBool(FALSE);
	}

	// own value
	bool fOwnValue = false; long iValue=0;
	if (iExtra & C4MN_Add_PassValue)
		{
		fOwnValue = true;
		iValue = XPar2.getInt();
		}

	// New Style: native script command
	int i = 0;
	for(; i < SLen(FnStringPar(szCommand)); i++)
		if(!IsIdentifier(FnStringPar(szCommand)[i]))
			break;
	if (i < SLen(FnStringPar(szCommand)))
	{
		// Search for "%d" an replace it by "%s" for insertion of formatted parameter
		SCopy(FnStringPar(szCommand), dummy, 256);
		char* pFound = const_cast<char*>(SSearch(dummy, "%d"));
		if (pFound != 0)
			*(pFound - 1) = 's';
		// Compose left-click command
		sprintf(command, dummy, parameter, 0);
		// Compose right-click command
		sprintf(command2, dummy, parameter, 1);
	}

	// Old style: function name with id and parameter
	else
		{
		const char *szScriptCom = FnStringPar(szCommand);
		if (szScriptCom && *szScriptCom)
			{
			if (iExtra & C4MN_Add_PassValue)
				{
				// with value
				sprintf(command,"%s(%s,%s,0,%ld)",szScriptCom,C4IdText(idItem),parameter,iValue);
				sprintf(command2,"%s(%s,%s,1,%ld)",szScriptCom,C4IdText(idItem),parameter,iValue);
				}
			else
				{
				// without value
				sprintf(command,"%s(%s,%s)",szScriptCom,C4IdText(idItem),parameter);
				sprintf(command2,"%s(%s,%s,1)",szScriptCom,C4IdText(idItem),parameter);
				}
			}
		else
			{
			// no command
			*command = *command2 = '\0';
			}
		}

	// Info caption
	SCopy(FnStringPar(szInfoCaption),infocaption,C4MaxTitle);
	// Default info caption by def desc
	if (!infocaption[0] && !(iExtra & C4MN_Add_ForceNoDesc)) SCopy(pDef->GetDesc(),infocaption,C4MaxTitle);

	// Create symbol
	C4FacetSurface fctSymbol;
	switch (iExtra & C4MN_Add_MaxImage)
		{
		case C4MN_Add_ImgRank:
			{
			// symbol by rank
			C4Facet *pfctRankSym = &::GraphicsResource.fctRank;
			int32_t iRankSymNum = ::GraphicsResource.iNumRanks;
			if (pDef && pDef->pRankSymbols)
				{
				pfctRankSym = pDef->pRankSymbols;
				iRankSymNum = pDef->iNumRankSymbols;
				}
			C4RankSystem::DrawRankSymbol(&fctSymbol, iCount, pfctRankSym, iRankSymNum, true);
			iCount=0;
			break;
			}
		case C4MN_Add_ImgIndexed:
			// draw indexed facet
			fctSymbol.Create(iSymbolSize,iSymbolSize);
			pDef->Draw(fctSymbol, FALSE, 0, NULL, XPar.getInt());
			break;
		case C4MN_Add_ImgObjRank:
			{
			// draw current gfx of XPar_C4V including rank
			if (XPar.GetType() != C4V_C4Object) return C4VFalse;
			C4Object *pGfxObj = XPar.getObj();
			if (pGfxObj && pGfxObj->Status)
				{
				// create graphics
				// get rank gfx
				C4Facet *pRankRes=&::GraphicsResource.fctRank;
				long iRankCnt=::GraphicsResource.iNumRanks;
				C4Def *pDef=pGfxObj->Def;
				if (pDef->pRankSymbols)
					{
					pRankRes=pDef->pRankSymbols;
					iRankCnt=pDef->iNumRankSymbols;
					}
				// context menu
				C4Facet fctRank;
				if (pMenuObj->Menu->IsContextMenu())
					{
					// context menu entry: left object gfx
					long C4MN_SymbolSize = pMenuObj->Menu->GetItemHeight();
					fctSymbol.Create(C4MN_SymbolSize * 2,C4MN_SymbolSize);
					fctSymbol.Wdt = C4MN_SymbolSize;
					pGfxObj->Def->Draw(fctSymbol, FALSE, pGfxObj->Color, pGfxObj);
					// right of it the rank
					fctRank = fctSymbol;
					fctRank.X = C4MN_SymbolSize;
					fctSymbol.Wdt *= 2;
					}
				else
					{
					// regular menu: draw object picture
					fctSymbol.Create(iSymbolSize,iSymbolSize);
					pGfxObj->Def->Draw(fctSymbol, FALSE, pGfxObj->Color, pGfxObj);
					// rank at top-right corner
					fctRank = fctSymbol;
					fctRank.X = fctRank.Wdt - pRankRes->Wdt;
					fctRank.Wdt = pRankRes->Wdt;
					fctRank.Hgt = pRankRes->Hgt;
					}
				// draw rank
				if (pGfxObj->Info)
					{
					C4Facet fctBackup = (const C4Facet &) fctSymbol;
					fctSymbol.Set(fctRank);
					C4RankSystem::DrawRankSymbol(&fctSymbol, pGfxObj->Info->Rank, pRankRes, iRankCnt, true);
					fctSymbol.Set(fctBackup);
					}
				}
			}
			break;
		case C4MN_Add_ImgObject:
			{
			// draw object picture
			if (XPar.GetType() != C4V_C4Object) return C4Value();
			C4Object *pGfxObj = XPar.getObj();
			fctSymbol.Wdt = fctSymbol.Hgt = iSymbolSize;
			pGfxObj->Picture2Facet(fctSymbol);
			}
			break;

		case C4MN_Add_ImgTextSpec:
			{
			C4FacetSurface fctSymSpec;
			uint32_t dwClr = XPar.getInt();
			if (!szCaption || !Game.DrawTextSpecImage(fctSymSpec, caption, dwClr ? dwClr : 0xff))
				return C4Value();
			fctSymbol.Create(iSymbolSize,iSymbolSize);
			fctSymSpec.Draw(fctSymbol, true);
			*caption = '\0';
			}
			break;

		case C4MN_Add_ImgColor:
			// draw colored def facet
			fctSymbol.Create(iSymbolSize,iSymbolSize);
			pDef->Draw(fctSymbol, FALSE, XPar.getInt());
			break;

		default:
			// default: by def, if it is not specifically NONE
			if (idItem != C4Id("NONE"))
				{
				fctSymbol.Create(iSymbolSize,iSymbolSize);
				pDef->Draw(fctSymbol);
				}
			else
				{
				// otherwise: Clear symbol!
				}
			break;
		}

	// Convert default zero count to no count
	if (iCount==0 && !(iExtra & C4MN_Add_ForceCount)) iCount=C4MN_Item_NoCount;

	// menuitems without commands are never selectable
	bool fIsSelectable = !!*command;

	// Add menu item
	pMenuObj->Menu->Add(caption,fctSymbol,command,iCount,NULL,infocaption,idItem,command2,fOwnValue,iValue,fIsSelectable);

	return C4VBool(true);
	}

static bool FnSelectMenuItem(C4AulContext *cthr, long iItem, C4Object *pMenuObj)
	{
  if (!pMenuObj) pMenuObj=cthr->Obj; if (!pMenuObj) return false;
	if (!pMenuObj->Menu) return false;
	return !!pMenuObj->Menu->SetSelection(iItem, false, true);
	}

static bool FnSetMenuDecoration(C4AulContext *cthr, C4ID idNewDeco, C4Object *pMenuObj)
	{
	if (!pMenuObj || !pMenuObj->Menu) return FALSE;
	C4GUI::FrameDecoration *pNewDeco = new C4GUI::FrameDecoration();
	if (!pNewDeco->SetByDef(idNewDeco))
		{
		delete pNewDeco;
		return FALSE;
		}
	pMenuObj->Menu->SetFrameDeco(pNewDeco);
	return TRUE;
	}

static bool FnSetMenuTextProgress(C4AulContext *cthr, long iNewProgress, C4Object *pMenuObj)
	{
	if (!pMenuObj || !pMenuObj->Menu) return FALSE;
	return pMenuObj->Menu->SetTextProgress(iNewProgress, false);
	}


// Check / Status

static C4Object *FnContained(C4AulContext *cthr, C4Object *pObj)
	{
	if (!pObj) pObj=cthr->Obj; if (!pObj) return NULL;
	return pObj->Contained;
	}

static C4Object *FnContents(C4AulContext *cthr, long index, C4Object *pObj)
  {
  if (!pObj) pObj=cthr->Obj; if (!pObj) return NULL;
	// Special: objects attaching to another object
	//          cannot be accessed by FnContents
	C4Object *cobj;
	while	(cobj=pObj->Contents.GetObject(index))
		{
		if (cobj->GetProcedure()!=DFA_ATTACH) return cobj;
		index++;
		}
	return NULL;
  }

static bool FnShiftContents(C4AulContext *cthr, C4Object *pObj, bool fShiftBack, C4ID idTarget, bool fDoCalls)
	{
	// local call/safety
  if (!pObj) pObj=cthr->Obj; if (!pObj) return false;
	// regular shift
	if (!idTarget) return !!pObj->ShiftContents(fShiftBack, fDoCalls);
	// check if ID is present within target
	C4Object *pNewFront = pObj->Contents.Find(idTarget);
	if (!pNewFront) return false;
	// select it
	pObj->DirectComContents(pNewFront, fDoCalls);
	// done, success
	return true;
	}

static C4Object *FnScrollContents(C4AulContext *cthr, C4Object *pObj)
  {
  if (!pObj) pObj=cthr->Obj; if (!pObj) return NULL;

	C4Object *pMove = pObj->Contents.GetObject();
	if (pMove)
		{
		pObj->Contents.Remove(pMove);
		pObj->Contents.Add(pMove,C4ObjectList::stNone);
		}

	return pObj->Contents.GetObject();
	}

static long FnContentsCount(C4AulContext *cthr, C4ID id, C4Object *pObj)
  {
  if (!pObj) pObj=cthr->Obj; if (!pObj) return 0;
  return pObj->Contents.ObjectCount(id);
  }

static C4Object *FnFindContents(C4AulContext *cthr, C4ID c_id, C4Object *pObj)
  {
  if (!pObj) pObj=cthr->Obj; if (!pObj) return 0;
  return pObj->Contents.Find(c_id);
  }

static C4Object *FnFindOtherContents(C4AulContext *cthr, C4ID c_id, C4Object *pObj)
  {
  if (!pObj) pObj=cthr->Obj; if (!pObj) return 0;
  return pObj->Contents.FindOther(c_id);
  }

static bool FnActIdle(C4AulContext *cthr, C4Object *pObj)
  {
  if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
  if (!pObj->Action.pActionDef) return TRUE;
  return FALSE;
  }

static bool FnCheckEnergyNeedChain(C4AulContext *cthr, C4Object *pObj)
	{
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
	C4ObjectList EnergyChainChecked;
	return CheckEnergyNeedChain(pObj,EnergyChainChecked);
	}

static bool FnEnergyCheck(C4AulContext *cthr, long energy, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
	if ( !(Game.Rules & C4RULE_StructuresNeedEnergy)
	  || (pObj->Energy>=energy)
		|| !(pObj->Def->LineConnect & C4D_Power_Consumer) )
			{ pObj->NeedEnergy=0; return TRUE; }
	pObj->NeedEnergy=1;
	return FALSE;
  }

static bool FnStuck(C4AulContext *cthr, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return false;
  return !!pObj->Shape.CheckContact(pObj->GetX(),pObj->GetY());
  }

static bool FnInLiquid(C4AulContext *cthr, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
  return pObj->InLiquid;
  }

static bool FnOnFire(C4AulContext *cthr, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
	if (pObj->GetOnFire()) return TRUE;
	// check for effect
	if (!pObj->pEffects) return FALSE;
  return !!pObj->pEffects->Get(C4Fx_AnyFire);
  }

static bool FnComponentAll(C4AulContext *cthr, C4Object *pObj, C4ID c_id)
  {
  long cnt;
  if (!pObj) return FALSE;
	C4IDList Components;
	pObj->Def->GetComponents(&Components, pObj, cthr->Obj);
  for (cnt=0; Components.GetID(cnt); cnt++)
    if (Components.GetID(cnt)!=c_id)
      if (Components.GetCount(cnt)>0)
        return FALSE;
  return TRUE;
  }

static C4Object *FnCreateObject(C4AulContext *cthr,
                         C4PropList * PropList, long iXOffset, long iYOffset, long iOwner)
  {
	if (cthr->Obj) // Local object calls override
		{
		iXOffset+=cthr->Obj->GetX();
		iYOffset+=cthr->Obj->GetY();
		if (!cthr->Caller || !cthr->Caller->Func->Owner->Strict)
			iOwner=cthr->Obj->Owner;
		}

  C4Object *pNewObj = Game.CreateObject(PropList,cthr->Obj,iOwner,iXOffset,iYOffset);

	// Set initial controller to creating controller, so more complicated cause-effect-chains can be traced back to the causing player
	if (pNewObj && cthr->Obj && cthr->Obj->Controller > NO_OWNER) pNewObj->Controller = cthr->Obj->Controller;

	return pNewObj;
  }

static C4Object *FnCreateConstruction(C4AulContext *cthr,
            C4PropList * PropList, long iXOffset, long iYOffset, long iOwner,
						long iCompletion, bool fTerrain, bool fCheckSite)
  {
	// Local object calls override position offset, owner
	if (cthr->Obj)
		{
		iXOffset+=cthr->Obj->GetX();
		iYOffset+=cthr->Obj->GetY();
		if (!cthr->Caller || !cthr->Caller->Func->Owner->Strict)
			iOwner=cthr->Obj->Owner;
		}

	// Check site
	if (fCheckSite)
	  if (!ConstructionCheck(PropList,iXOffset,iYOffset,cthr->Obj))
			return NULL;

	// Create site object
	C4Object *pNewObj = Game.CreateObjectConstruction(PropList,cthr->Obj,iOwner,iXOffset,iYOffset,iCompletion*FullCon/100,fTerrain);

	// Set initial controller to creating controller, so more complicated cause-effect-chains can be traced back to the causing player
	if (pNewObj && cthr->Obj && cthr->Obj->Controller>NO_OWNER) pNewObj->Controller = cthr->Obj->Controller;

	return pNewObj;
  }

static C4Object *FnCreateContents(C4AulContext *cthr, C4PropList * PropList, C4Object *pObj, long iCount)
  {
	// local call / safety
  if (!pObj) pObj=cthr->Obj; if (!pObj) return NULL;
	// default amount parameter
	if (!iCount) ++iCount;
	// create objects
	C4Object *pNewObj = NULL;
	while (iCount-- > 0) pNewObj = pObj->CreateContents(PropList);
	// controller will automatically be set upon entrance
	// return last created
  return pNewObj;
  }

static C4Object *FnComposeContents(C4AulContext *cthr, C4ID c_id, C4Object *pObj)
	{
  if (!pObj) pObj=cthr->Obj; if (!pObj) return NULL;
  return pObj->ComposeContents(c_id);
	}


/*static C4Value FnFindConstructionSite(C4AulContext *cthr, C4Value *id_C4V, C4Value *iVarX_C4V, C4Value *iVarY_C4V)
	{
	C4Def *pDef = C4Id2Def(id_C4V->getC4ID());
  if (!pDef) return C4VBool(FALSE);

	// find target variables
	C4Value *pVarX, *pVarY;

	if(iVarX_C4V->GetType() == C4V_pC4Value)			Sorry, reference detection doesn't work...
		pVarX = &iVarX_C4V->GetRefVal();
	else if(iVarX_C4V->ConvertTo(C4V_Int))
		pVarX = &cthr->Vars[iVarX_C4V->getInt()];
	else
		return C4VBool(FALSE);

	if(iVarY_C4V->GetType() == C4V_pC4Value)
		pVarY = &iVarY_C4V->GetRefVal();
	else if(iVarY_C4V->ConvertTo(C4V_Int))
		pVarY = &cthr->Vars[iVarY_C4V->getInt()];
	else
		return C4VBool(FALSE);

	long iX = pVarX->getInt(), iY = pVarY->getInt();

	// Construction check at starting position
	if (ConstructionCheck(id_C4V->getC4ID(),iX,iY))
		return C4VBool(TRUE);

	// Search
  bool fSuccess = FindConSiteSpot(iX, iY,
												 pDef->Shape.Wdt,pDef->Shape.Hgt,
												 pDef->Category,
												 20);
	if(fSuccess)
	{
		// set
		pVarX->Set(iX, C4V_Int);
		pVarY->Set(iY, C4V_Int);
	}

	return C4VBool(!!fSuccess);
	}*/

static bool FnFindConstructionSite(C4AulContext *cthr, C4PropList * PropList, long iVarX, long iVarY)
  {
	// Get def																			Old-style implementation (fixed)...
  C4Def *pDef;
  if (!(pDef=PropList->GetDef())) return FALSE;
	// Var indices out of range
  if (!Inside<long>(iVarX,0,C4AUL_MAX_Par-1) || !Inside<long>(iVarY,0,C4AUL_MAX_Par-1)) return FALSE;
	// Get thread vars
	if(!cthr->Caller) return false;
  C4Value& V1 = cthr->Caller->NumVars[iVarX];
	C4Value& V2 = cthr->Caller->NumVars[iVarY];
  // Construction check at starting position
  if (ConstructionCheck(PropList,V1.getInt(),V2.getInt()))
    return TRUE;
  // Search for real
  int32_t v1 = V1.getInt(), v2 = V2.getInt();
  bool result = !!FindConSiteSpot(v1, v2,
                         pDef->Shape.Wdt,pDef->Shape.Hgt,
                         pDef->Category,
                         20);
  V1 = C4VInt(v1); V2 = C4VInt(v2);
  return result;
  }

static C4Object *FnFindBase(C4AulContext *cthr, long iOwner, long iIndex)
	{
	if (!ValidPlr(iOwner)) return NULL;
	return Game.FindBase(iOwner,iIndex);
	}

C4FindObject *CreateCriterionsFromPars(C4Value *pPars, C4FindObject **pFOs, C4SortObject **pSOs)
	{
	int i, iCnt = 0, iSortCnt = 0;
	// Read all parameters
	for(i = 0; i < C4AUL_MAX_Par; i++)
		{
		PAR(any, Data);
		// No data given?
		if(!Data) break;
		// Construct
		C4SortObject *pSO = NULL;
		C4FindObject *pFO = C4FindObject::CreateByValue(Data, pSOs ? &pSO : NULL);
		// Add FindObject
		if(pFO)
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
	if(!iCnt)
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
	if(iCnt == 1)
		pFO = pFOs[0];
	else
		pFO = new C4FindObjectAnd(iCnt, pFOs, false);
	if (pSO) pFO->SetSort(pSO);
	return pFO;
	}

static C4Value FnObjectCount2(C4AulContext *cthr, C4Value *pPars)
	{
	// Create FindObject-structure
	C4FindObject *pFOs[C4AUL_MAX_Par];
	C4FindObject *pFO = CreateCriterionsFromPars(pPars, pFOs, NULL);
	// Error?
	if(!pFO)
		throw new C4AulExecError(cthr->Obj, "ObjectCount: No valid search criterions supplied!");
	// Search
	int32_t iCnt = pFO->Count(::Objects, ::Objects.Sectors);
	// Free
	delete pFO;
	// Return
	return C4VInt(iCnt);
	}

static C4Value FnFindObject2(C4AulContext *cthr, C4Value *pPars)
	{
	// Create FindObject-structure
	C4FindObject *pFOs[C4AUL_MAX_Par];
	C4SortObject *pSOs[C4AUL_MAX_Par];
	C4FindObject *pFO = CreateCriterionsFromPars(pPars, pFOs, pSOs);
	// Error?
	if(!pFO)
		throw new C4AulExecError(cthr->Obj, "FindObject: No valid search criterions supplied!");
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
	if(!pFO)
		throw new C4AulExecError(cthr->Obj, "FindObjects: No valid search criterions supplied!");
	// Search
	C4ValueArray *pResult = pFO->FindMany(::Objects, ::Objects.Sectors);
	// Free
	delete pFO;
	// Return
	return C4VArray(pResult);
	}

static C4Value FnObjectCount(C4AulContext *cthr, C4Value *pPars)
	{
	// Got an array? Use the new-style function.
	if(pPars->getArray())
		return FnObjectCount2(cthr, pPars);
	// Get paramters
	PAR(id, id);
	PAR(int, x); PAR(int, y); PAR(int, wdt); PAR(int, hgt);
	PAR(int, dwOCF);
	PAR(string, szAction);
	PAR(object, pActionTarget);
	PAR(any, vContainer);
	PAR(int, iOwner);
  // Local call adjust coordinates
  if (cthr->Obj)
		if (x || y || wdt || hgt) // if not default full range
			{ x+=cthr->Obj->GetX(); y+=cthr->Obj->GetY(); }
	// Adjust default ocf
	if (dwOCF==0) dwOCF = OCF_All;
	// Adjust default owner
	if (iOwner==0) iOwner = ANY_OWNER; // imcomplete useless implementation
	// NO_CONTAINER/ANY_CONTAINER
	C4Object * pContainer = vContainer.getObj();
	if(vContainer.getInt() == NO_CONTAINER)
		pContainer = reinterpret_cast<C4Object *>(NO_CONTAINER);
	if(vContainer.getInt() == ANY_CONTAINER)
		pContainer = reinterpret_cast<C4Object *>(ANY_CONTAINER);
  // Find object
  return C4VInt(Game.ObjectCount(id,x,y,wdt,hgt,dwOCF,
																 FnStringPar(szAction),pActionTarget,
																 cthr->Obj, // Local calls exclude self
																 pContainer,
																 iOwner));
	}


static C4Value FnFindObject(C4AulContext *cthr, C4Value *pPars)
	{
	// Got an array? Use the new-style function.
	if(pPars->getArray())
		return FnFindObject2(cthr, pPars);
	// Get parameters
	PAR(id, id);
	PAR(int, x); PAR(int, y); PAR(int, wdt); PAR(int, hgt);
	PAR(int, dwOCF);
	PAR(string, szAction);
	PAR(object, pActionTarget);
	PAR(any, vContainer);
	PAR(object, pFindNext);
  // Local call adjust coordinates
  if (cthr->Obj)
		if (x || y || wdt || hgt) // if not default full range
			{ x+=cthr->Obj->GetX(); y+=cthr->Obj->GetY(); }
	// Adjust default ocf
	if (dwOCF==0) dwOCF = OCF_All;
	// NO_CONTAINER/ANY_CONTAINER
	C4Object * pContainer = vContainer.getObj();
	if(vContainer.getInt() == NO_CONTAINER)
		pContainer = reinterpret_cast<C4Object *>(NO_CONTAINER);
	if(vContainer.getInt() == ANY_CONTAINER)
		pContainer = reinterpret_cast<C4Object *>(ANY_CONTAINER);
  // Find object
  return C4Value(Game.FindObject(id,x,y,wdt,hgt,dwOCF,
												 FnStringPar(szAction),pActionTarget,
												 cthr->Obj, // Local calls exclude self
												 pContainer,
												 ANY_OWNER,
												 pFindNext));
  }

static C4Object *FnFindObjectOwner(C4AulContext *cthr,
											 C4ID id,
											 long iOwner,
   										 long x, long y, long wdt, long hgt,
										   long dwOCF,
											 C4String *szAction, C4Object *pActionTarget,
											 C4Object *pFindNext)
  {
	// invalid owner?
	if (!ValidPlr(iOwner) && iOwner != NO_OWNER) return NULL;
  // Local call adjust coordinates
  if (cthr->Obj)
		if (x || y || wdt || hgt) // if not default full range
			{ x+=cthr->Obj->GetX(); y+=cthr->Obj->GetY(); }
	// Adjust default ocf
	if (dwOCF==0) dwOCF = OCF_All;
  // Find object
  return Game.FindObject(id,x,y,wdt,hgt,dwOCF,
												 FnStringPar(szAction),pActionTarget,
												 cthr->Obj, // Local calls exclude self
												 NULL,
												 iOwner,
												 pFindNext);
  }

static bool FnMakeCrewMember(C4AulContext *cthr, C4Object *pObj, long iPlayer)
	{
	if (!ValidPlr(iPlayer)) return false;
	return !!::Players.Get(iPlayer)->MakeCrewMember(pObj);
	}

static bool FnGrabObjectInfo(C4AulContext *cthr, C4Object *pFrom, C4Object *pTo)
	{
	// local call, safety
	if (!pFrom) return false; if (!pTo) { pTo = cthr->Obj; if (!pTo) return false; }
	// grab info
	return !!pTo->GrabInfo(pFrom);
	}

static bool FnFlameConsumeMaterial(C4AulContext *cthr, long x, long y)
	{
	if (cthr->Obj) { x+=cthr->Obj->GetX(); y+=cthr->Obj->GetY(); }
	long mat=GBackMat(x,y);
	if (!MatValid(mat)) return false;
	if (!::MaterialMap.Map[mat].Inflammable) return false;
	if (::Landscape.ExtractMaterial(x,y)==MNone) return false;
	return true;
	}

static bool FnSmoke(C4AulContext *cthr, long tx, long ty, long level, long dwClr)
  {
	if (cthr->Obj) { tx+=cthr->Obj->GetX(); ty+=cthr->Obj->GetY(); }
  Smoke(tx,ty,level,dwClr);
  return TRUE;
  }

static bool FnBubble(C4AulContext *cthr, long tx, long ty)
  {
	if (cthr->Obj) { tx+=cthr->Obj->GetX(); ty+=cthr->Obj->GetY(); }
  BubbleOut(tx,ty);
  return TRUE;
  }

static long FnExtractLiquid(C4AulContext *cthr, long x, long y)
	{
	if (cthr->Obj) { x+=cthr->Obj->GetX(); y+=cthr->Obj->GetY(); }
	if (!GBackLiquid(x,y)) return MNone;
	return ::Landscape.ExtractMaterial(x,y);
	}

static bool FnInsertMaterial(C4AulContext *cthr, long mat, long x, long y, long vx, long vy)
	{
	if (cthr->Obj) { x+=cthr->Obj->GetX(); y+=cthr->Obj->GetY(); }
	return !!::Landscape.InsertMaterial(mat,x,y,vx,vy);
	}

static long FnGetMaterialCount(C4AulContext *cthr, long iMaterial, bool fReal)
	{
  if(!MatValid(iMaterial)) return -1;
  if(fReal || !::MaterialMap.Map[iMaterial].MinHeightCount)
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
	// Get texture
	int32_t iTex = PixCol2Tex(GBackPix(x, y));
	if(!iTex) return NULL;
	// Get material-texture mapping
	const C4TexMapEntry *pTex = ::TextureMap.GetEntry(iTex);
	if(!pTex) return NULL;
	// Return tex name
	return String(pTex->GetTextureName());
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
	long extracted=0; for (;extracted<amount; extracted++)
		{
		if (GBackMat(x,y)!=mat) return extracted;
		if (::Landscape.ExtractMaterial(x,y)!=mat) return extracted;
		}
	return extracted;
	}

static bool FnBlastObjects(C4AulContext *cthr, long iX, long iY, long iLevel, C4Object *pInObj, long iCausedByPlusOne)
	{
	long iCausedBy = iCausedByPlusOne-1; if (!iCausedByPlusOne && cthr->Obj) iCausedBy = cthr->Obj->Controller;
	Game.BlastObjects(iX,iY,iLevel,pInObj,iCausedBy,cthr->Obj);
	return true;
	}

static bool FnBlastObject(C4AulContext *cthr, long iLevel, C4Object *pObj, long iCausedByPlusOne)
	{
	long iCausedBy = iCausedByPlusOne-1; if (!iCausedByPlusOne && cthr->Obj) iCausedBy = cthr->Obj->Controller;
	if (!pObj) if (!(pObj = cthr->Obj)) return false;
	if (!pObj->Status) return false;
	pObj->Blast(iLevel, iCausedBy);
	return true;
	}

static bool FnBlastFree(C4AulContext *cthr, long iX, long iY, long iLevel, long iCausedByPlusOne)
	{
	int32_t iCausedBy = iCausedByPlusOne-1;
	if (!iCausedByPlusOne && cthr->Obj)
		{
		iCausedBy = cthr->Obj->Controller;
		iX += cthr->Obj->GetX();
		iY += cthr->Obj->GetY();
		}
	int grade = BoundBy<int>((iLevel/10)-1,1,3);
	::Landscape.BlastFree(iX, iY, iLevel, grade, iCausedBy);
	return true;
	}

static bool FnSound(C4AulContext *cthr, C4String *szSound, bool fGlobal, C4Object *pObj, long iLevel, long iAtPlayer, long iLoop, bool fMultiple, long iCustomFalloffDistance)
  {
	// play here?
	if (iAtPlayer)
		{
		// get player to play at
		C4Player *pPlr = ::Players.Get(iAtPlayer-1);
		// not existant? fail
		if (!pPlr) return FALSE;
		// network client: don't play here
		// return true for network sync
		if (!pPlr->LocalControl) return TRUE;
		}
	// even less than nothing?
	if (iLevel<0) return TRUE;
	// default sound level
	if (!iLevel || iLevel>100)
		iLevel=100;
  // target object
  if(fGlobal) pObj = NULL; else if(!pObj) pObj = cthr->Obj;
  // already playing?
  if(iLoop >= 0 && !fMultiple && GetSoundInstance(FnStringPar(szSound), pObj))
    return TRUE;
	// try to play effect
  if(iLoop >= 0)
	  StartSoundEffect(FnStringPar(szSound),!!iLoop,iLevel,pObj, iCustomFalloffDistance);
  else
	  StopSoundEffect(FnStringPar(szSound),pObj);
	// always return true (network safety!)
	return TRUE;
  }

static bool FnMusic(C4AulContext *cthr, C4String *szSongname, bool fLoop)
	{
	// FIXME: Script should not influence the user's configuration
	if (!szSongname)
		{
		Config.Sound.RXMusic=FALSE;
		Application.MusicSystem.Stop();
		}
	else
		{
		Config.Sound.RXMusic=TRUE;
		Application.MusicSystem.Stop();
		if (!Application.MusicSystem.Play(FnStringPar(szSongname), !!fLoop))
			{
			Config.Sound.RXMusic=FALSE;
			return TRUE;
			}
		}
	return TRUE;
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
	if(::Control.SyncMode()) return 0;
	return iFilesInPlayList;
	}

static bool FnSoundLevel(C4AulContext *cthr, C4String *szSound, long iLevel, C4Object *pObj)
  {
  SoundLevel(FnStringPar(szSound),pObj,iLevel);
  return TRUE;
  }

static bool FnGameOver(C4AulContext *cthr, long iGameOverValue /* provided for future compatibility */)
	{
	return !!Game.DoGameOver();
	}

static bool FnGainMissionAccess(C4AulContext *cthr, C4String *szPassword)
	{
	if (SLen(Config.General.MissionAccess)+SLen(FnStringPar(szPassword))+3>CFG_MaxString) return FALSE;
	SAddModule(Config.General.MissionAccess,FnStringPar(szPassword));
	return TRUE;
	}

static C4Value FnLog_C4V(C4AulContext *cthr, C4Value *szMessage, C4Value * iPar0, C4Value * iPar1, C4Value * iPar2, C4Value * iPar3, C4Value * iPar4, C4Value * iPar5, C4Value * iPar6, C4Value * iPar7, C4Value * iPar8)
	{
	Log(FnStringFormat(cthr, FnStringPar(szMessage->getStr()),iPar0,iPar1,iPar2,iPar3,iPar4,iPar5,iPar6,iPar7,iPar8).getData());
	return C4VBool(true);
	}

static C4Value FnDebugLog_C4V(C4AulContext *cthr, C4Value *szMessage, C4Value * iPar0, C4Value * iPar1, C4Value * iPar2, C4Value * iPar3, C4Value * iPar4, C4Value * iPar5, C4Value * iPar6, C4Value * iPar7, C4Value * iPar8)
	{
	DebugLog(FnStringFormat(cthr, FnStringPar(szMessage->getStr()),iPar0,iPar1,iPar2,iPar3,iPar4,iPar5,iPar6,iPar7,iPar8).getData());
	return C4VBool(true);
	}

static C4Value FnFormat_C4V(C4AulContext *cthr, C4Value *szFormat, C4Value * iPar0, C4Value * iPar1, C4Value * iPar2, C4Value * iPar3, C4Value * iPar4, C4Value * iPar5, C4Value * iPar6, C4Value * iPar7, C4Value * iPar8)
	{
	return C4VString(FnStringFormat(cthr, FnStringPar(szFormat->getStr()),iPar0,iPar1,iPar2,iPar3,iPar4,iPar5,iPar6,iPar7,iPar8));
	}

static C4ID FnC4Id(C4AulContext *cthr, C4String *szID)
	{
	return(C4Id(FnStringPar(szID)));
	}

static C4Value FnPlayerMessage_C4V(C4AulContext *cthr, C4Value * iPlayer, C4Value *c4vMessage, C4Value *c4vObj, C4Value * iPar0, C4Value * iPar1, C4Value * iPar2, C4Value * iPar3, C4Value * iPar4, C4Value * iPar5, C4Value * iPar6)
	{
	char buf[MaxFnStringParLen+1];
	C4String * szMessage = c4vMessage->getStr();
	C4Object * pObj = c4vObj->getObj();
	if (!szMessage) return C4VBool(false);

	// Speech
	bool fSpoken=FALSE;
	if (SCopySegment(FnStringPar(szMessage),1,buf,'$'))
		if (StartSoundEffect(buf,false,100,pObj ? pObj : cthr->Obj))
			fSpoken=TRUE;

	// Text
	if (!fSpoken)
		if (SCopySegment(FnStringFormat(cthr,FnStringPar(szMessage),iPar0,iPar1,iPar2,iPar3,iPar4,iPar5,iPar6).getData(),0,buf,'$'))
			if (pObj)	GameMsgObjectPlayer(buf,pObj,iPlayer->getInt());
			else GameMsgPlayer(buf, iPlayer->getInt());

	return C4VBool(true);
	}

static C4Value FnMessage_C4V(C4AulContext *cthr, C4Value *c4vMessage, C4Value *c4vObj, C4Value * iPar0, C4Value * iPar1, C4Value * iPar2, C4Value * iPar3, C4Value * iPar4, C4Value * iPar5, C4Value * iPar6, C4Value * iPar7)
	{
	char buf[MaxFnStringParLen+1];
	C4String * szMessage = c4vMessage->getStr();
	if (!szMessage) return C4VBool(false);

	// Speech
	bool fSpoken=FALSE;
	if (SCopySegment(FnStringPar(szMessage),1,buf,'$'))
		if (StartSoundEffect(buf,false,100,cthr->Obj))
			fSpoken=TRUE;

	// Text
	C4Object * pObj = c4vObj->getObj();
	if (!fSpoken)
		if (SCopySegment(FnStringFormat(cthr,FnStringPar(szMessage),iPar0,iPar1,iPar2,iPar3,iPar4,iPar5,iPar6,iPar7).getData(),0,buf,'$'))
			if (pObj)	GameMsgObject(buf,pObj);
			else GameMsgGlobal(buf);

	return C4VBool(true);
	}

static C4Value FnAddMessage_C4V(C4AulContext *cthr, C4Value *c4vMessage, C4Value *c4vObj, C4Value * iPar0, C4Value * iPar1, C4Value * iPar2, C4Value * iPar3, C4Value * iPar4, C4Value * iPar5, C4Value * iPar6, C4Value * iPar7)
	{
	C4String * szMessage = c4vMessage->getStr();
	if (!szMessage) return C4VBool(false);

	C4Object * pObj = c4vObj->getObj();
	if (pObj) ::Messages.Append(C4GM_Target,FnStringFormat(cthr,FnStringPar(szMessage),iPar0,iPar1,iPar2,iPar3,iPar4,iPar5,iPar6,iPar7).getData(),pObj,NO_OWNER,0,0,FWhite);
	else ::Messages.Append(C4GM_Global,FnStringFormat(cthr,FnStringPar(szMessage),iPar0,iPar1,iPar2,iPar3,iPar4,iPar5,iPar6,iPar7).getData(),0,ANY_OWNER,0,0,FWhite);

	return C4VBool(true);
	}

static C4Value FnPlrMessage_C4V(C4AulContext *cthr, C4Value *c4vMessage, C4Value * iPlr, C4Value * iPar0, C4Value * iPar1, C4Value * iPar2, C4Value * iPar3, C4Value * iPar4, C4Value * iPar5, C4Value * iPar6, C4Value * iPar7)
	{
	char buf[MaxFnStringParLen+1];
	C4String * szMessage = c4vMessage->getStr();
	if (!szMessage) return C4VBool(false);

	// Speech
	bool fSpoken=FALSE;
	if (SCopySegment(FnStringPar(szMessage),1,buf,'$'))
		if (StartSoundEffect(buf,false,100,cthr->Obj))
			fSpoken=TRUE;

	// Text
	if (!fSpoken)
		if (SCopySegment(FnStringFormat(cthr,FnStringPar(szMessage),iPar0,iPar1,iPar2,iPar3,iPar4,iPar5,iPar6,iPar7).getData(),0,buf,'$'))
			if (ValidPlr(iPlr->getInt())) GameMsgPlayer(buf,iPlr->getInt());
			else GameMsgGlobal(buf);

	return C4VBool(true);
	}

static bool FnScriptGo(C4AulContext *cthr, bool go)
	{
	Game.Script.Go=!!go;
	return TRUE;
	}

static bool FnCastPXS(C4AulContext *cthr, C4String *mat_name, long amt, long level, long tx, long ty)
  {
	if (cthr->Obj) { tx+=cthr->Obj->GetX(); ty+=cthr->Obj->GetY(); }
  ::PXS.Cast(::MaterialMap.Get(FnStringPar(mat_name)),amt,tx,ty,level);
  return TRUE;
  }

static bool FnCastObjects(C4AulContext *cthr, C4ID id, long amt, long level, long tx, long ty)
	{
	if (cthr->Obj) { tx+=cthr->Obj->GetX(); ty+=cthr->Obj->GetY(); }
  Game.CastObjects(id,cthr->Obj,amt,level,tx,ty, cthr->Obj ? cthr->Obj->Owner : NO_OWNER, cthr->Obj ? cthr->Obj->Controller : NO_OWNER);
	return TRUE;
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

static bool FnDrawVolcanoBranch(C4AulContext *cthr, long mat, long fx, long fy, long tx, long ty, long size)
	{
	long cx,cx2,cy;
	for (cy=ty; cy<fy; cy++)
		{
		cx=fx+(tx-fx)*(cy-fy)/(ty-fy);
		for (cx2=cx-size/2; cx2<cx+size/2; cx2++)
			SBackPix(cx2,cy,Mat2PixColDefault(mat)+GBackIFT(cx2,cy));
		}
	return TRUE;
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
		if (!!Game.Script.GRBroadcast(PSF_RejectHostilityChange, &C4AulParSet(C4VInt(iPlr), C4VInt(iPlr2), C4VBool(fHostile)), true, true))
			return false;
		}
	// OK; set hostility
	bool fOldHostility = ::Players.HostilityDeclared(iPlr, iPlr2);
	if (!pPlr->SetHostility(iPlr2,fHostile, fSilent)) return false;
	// calls afterwards
	Game.Script.GRBroadcast(PSF_OnHostilityChange, &C4AulParSet(C4VInt(iPlr), C4VInt(iPlr2), C4VBool(fHostile), C4VBool(fOldHostility)), true);
	return true;
  }

static bool FnSetPlrView(C4AulContext *cthr, long iPlr, C4Object *tobj)
  {
  if (!ValidPlr(iPlr)) return false;
  ::Players.Get(iPlr)->SetViewMode(C4PVM_Target,tobj);
  return true;
  }

static bool FnSetPlrShowControl(C4AulContext *cthr, long iPlr, C4String *defstring)
  {
  if (!ValidPlr(iPlr)) return false;
  ::Players.Get(iPlr)->ShowControl=StringBitEval(FnStringPar(defstring));
  return true;
  }

static bool FnSetPlrShowCommand(C4AulContext *cthr, long iPlr, long iCom)
	{
  if (!ValidPlr(iPlr)) return false;
  ::Players.Get(iPlr)->FlashCom=iCom;
	if (!Config.Graphics.ShowCommands) Config.Graphics.ShowCommands=TRUE;
  return true;
	}

static bool FnSetPlrShowControlPos(C4AulContext *cthr, long iPlr, long pos)
  {
  if (!ValidPlr(iPlr)) return false;
  ::Players.Get(iPlr)->ShowControlPos=pos;
  return true;
  }

static C4String *FnGetPlrControlName(C4AulContext *cthr, long iPlr, long iCon, bool fShort)
  {
  return String(PlrControlKeyName(iPlr,iCon,fShort).getData());
  }

static long FnGetPlrViewMode(C4AulContext *cthr, long iPlr)
  {
  if (!ValidPlr(iPlr)) return -1;
	if (::Control.SyncMode()) return -1;
  return ::Players.Get(iPlr)->ViewMode;
  }

static C4Object *FnGetPlrView(C4AulContext *cthr, long iPlr)
  {
	C4Player *pPlr = ::Players.Get(iPlr);
  if (!pPlr || pPlr->ViewMode != C4PVM_Target) return NULL;
  return pPlr->ViewTarget;
  }

static bool FnDoHomebaseMaterial(C4AulContext *cthr, long iPlr, C4ID id, long iChange)
	{
	// validity check
  if (!ValidPlr(iPlr)) return FALSE;
	C4Def *pDef = C4Id2Def(id);
	if (!pDef) return false;
	// add to material
	long iLastcount = ::Players.Get(iPlr)->HomeBaseMaterial.GetIDCount(id);
  return ::Players.Get(iPlr)->HomeBaseMaterial.SetIDCount(id,iLastcount+iChange,TRUE);
	}

static bool FnDoHomebaseProduction(C4AulContext *cthr, long iPlr, C4ID id, long iChange)
	{
	// validity check
  if (!ValidPlr(iPlr)) return FALSE;
	C4Def *pDef = C4Id2Def(id);
	if (!pDef) return false;
	// add to material
	long iLastcount = ::Players.Get(iPlr)->HomeBaseProduction.GetIDCount(id);
  return ::Players.Get(iPlr)->HomeBaseProduction.SetIDCount(id,iLastcount+iChange,TRUE);
	}

static long FnGetPlrDownDouble(C4AulContext *cthr, long iPlr)
  {
  if (!ValidPlr(iPlr)) return FALSE;
  return ::Players.Get(iPlr)->LastComDownDouble;
  }

static bool FnClearLastPlrCom(C4AulContext *cthr, long iPlr)
	{
	// get player
	C4Player *pPlr = ::Players.Get(iPlr);
	if (!pPlr) return FALSE;
	// reset last coms
	pPlr->LastCom = COM_None;
	pPlr->LastComDownDouble = 0;
	// done, success
	return TRUE;
	}

static bool FnSetPlrKnowledge(C4AulContext *cthr, long iPlr, C4ID id, bool fRemove)
  {
	C4Player *pPlr=::Players.Get(iPlr);
  if (!pPlr) return FALSE;
	if (fRemove)
		{
		long iIndex=pPlr->Knowledge.GetIndex(id);
		if (iIndex<0) return FALSE;
		return pPlr->Knowledge.DeleteItem(iIndex);
		}
	else
		{
		if(!C4Id2Def(id)) return FALSE;
		return pPlr->Knowledge.SetIDCount(id,1,TRUE);
		}
  }

static bool FnSetComponent(C4AulContext *cthr, C4ID idComponent, long iCount, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
  return pObj->Component.SetIDCount(idComponent,iCount,TRUE);
  }

static C4Value FnGetPlrKnowledge_C4V(C4AulContext *cthr, C4Value* iPlr_C4V, C4Value* id_C4V, C4Value* iIndex_C4V, C4Value* dwCategory_C4V)
  {
	long iPlr = iPlr_C4V->getInt();
	C4ID id = id_C4V->getC4ID();
	long iIndex = iIndex_C4V->getInt();
	DWORD dwCategory = dwCategory_C4V->getInt();
  if (!ValidPlr(iPlr)) return C4VBool(FALSE);
	// Search by id, check if available, return bool
	if (id)	return C4VBool(::Players.Get(iPlr)->Knowledge.GetIDCount(id,1) != 0);
	// Search indexed item of given category, return C4ID
	return C4VID(::Players.Get(iPlr)->Knowledge.GetID( ::Definitions, dwCategory, iIndex ));
  }

static C4ID FnGetDefinition(C4AulContext *cthr, long iIndex, long dwCategory)
	{
	C4Def *pDef;
	// Default: all categories
	if (!dwCategory) dwCategory=C4D_All;
	// Get def
	if (!(pDef = ::Definitions.GetDef(iIndex,dwCategory))) return C4ID_None;
	// Return id
	return pDef->id;
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
		if (idComponent) return C4VInt(pDef->GetComponentCount(idComponent, cthr->Obj));
		// Indexed component
		return C4VID(pDef->GetIndexedComponent(iIndex, cthr->Obj));
		}
	// Object component
	else
		{
		// Get object
		if (!pObj) pObj=cthr->Obj; if (!pObj) return C4Value();
		// Component count
		if (idComponent) return C4VInt(pObj->Component.GetIDCount(idComponent));
		// Indexed component
		return C4VID(pObj->Component.GetID(iIndex));
		}
	return C4Value();
  }

static C4Value FnGetHomebaseMaterial_C4V(C4AulContext *cthr, C4Value* iPlr_C4V, C4Value* id_C4V, C4Value* iIndex_C4V, C4Value* dwCategory_C4V)
	{
	long iPlr = iPlr_C4V->getInt();
	C4ID id = id_C4V->getC4ID();
	long iIndex = iIndex_C4V->getInt();
	DWORD dwCategory = dwCategory_C4V->getInt();

  if (!ValidPlr(iPlr)) return C4VBool(FALSE);
	// Search by id, return available count
	if (id)	return C4VInt(::Players.Get(iPlr)->HomeBaseMaterial.GetIDCount(id));
	// Search indexed item of given category, return C4ID
	return C4VID(::Players.Get(iPlr)->HomeBaseMaterial.GetID( ::Definitions, dwCategory, iIndex ));
	}

static C4Value FnGetHomebaseProduction_C4V(C4AulContext *cthr, C4Value* iPlr_C4V, C4Value* id_C4V, C4Value* iIndex_C4V, C4Value* dwCategory_C4V)
	{
	long iPlr = iPlr_C4V->getInt();
	C4ID id = id_C4V->getC4ID();
	long iIndex = iIndex_C4V->getInt();
	DWORD dwCategory = dwCategory_C4V->getInt();

  if (!ValidPlr(iPlr)) return C4VBool(FALSE);
	// Search by id, return available count
	if (id)	return C4VInt(::Players.Get(iPlr)->HomeBaseProduction.GetIDCount(id));
	// Search indexed item of given category, return C4ID
	return C4VID(::Players.Get(iPlr)->HomeBaseProduction.GetID( ::Definitions, dwCategory, iIndex ));
	}

static long FnSetPlrMagic(C4AulContext *cthr, long iPlr, C4ID id, bool fRemove)
  {
	C4Player *pPlr=::Players.Get(iPlr);
  if (!pPlr) return FALSE;
	if (fRemove)
		{
		long iIndex=pPlr->Magic.GetIndex(id);
		if (iIndex<0) return FALSE;
		return pPlr->Magic.DeleteItem(iIndex);
		}
	else
		{
		if(!C4Id2Def(id)) return FALSE;
		return pPlr->Magic.SetIDCount(id,1,TRUE);
		}
  }

static C4Value FnGetPlrMagic_C4V(C4AulContext *cthr, C4Value* iPlr_C4V, C4Value* id_C4V, C4Value* iIndex_C4V)
  {
	long iPlr = iPlr_C4V->getInt();
	C4ID id = id_C4V->getC4ID();
	long iIndex = iIndex_C4V->getInt();

  if (!ValidPlr(iPlr)) return C4VBool(FALSE);
	// Search by id, check if available, return bool
	if (id)	return C4VBool(::Players.Get(iPlr)->Magic.GetIDCount(id,1) != 0);
	// Search indexed item of given category, return C4ID
	return C4VID(::Players.Get(iPlr)->Magic.GetID( ::Definitions, C4D_Magic, iIndex ));
  }

static long FnGetWealth(C4AulContext *cthr, long iPlr)
  {
  if (!ValidPlr(iPlr)) return 0;
  return ::Players.Get(iPlr)->Wealth;
  }

static bool FnSetWealth(C4AulContext *cthr, long iPlr, long iValue)
  {
  if (!ValidPlr(iPlr)) return FALSE;
  ::Players.Get(iPlr)->Wealth = BoundBy<long>(iValue,0,100000);
	return TRUE;
  }

static long FnDoScore(C4AulContext *cthr, long iPlr, long iChange)
  {
  if (!ValidPlr(iPlr)) return FALSE;
  return ::Players.Get(iPlr)->DoPoints(iChange);
  }

static long FnGetPlrValue(C4AulContext *cthr, long iPlr)
  {
  if (!ValidPlr(iPlr)) return 0;
  return ::Players.Get(iPlr)->Value;
  }

static long FnGetPlrValueGain(C4AulContext *cthr, long iPlr)
  {
  if (!ValidPlr(iPlr)) return 0;
  return ::Players.Get(iPlr)->ValueGain;
  }

static long FnGetScore(C4AulContext *cthr, long iPlr)
  {
  if (!ValidPlr(iPlr)) return 0;
  return ::Players.Get(iPlr)->Points;
  }

static C4Object *FnGetHiRank(C4AulContext *cthr, long iPlr)
  {
  if (!ValidPlr(iPlr)) return FALSE;
  return ::Players.Get(iPlr)->GetHiRankActiveCrew(false);
  }

static C4Object *FnGetCrew(C4AulContext *cthr, long iPlr, long index)
  {
  if (!ValidPlr(iPlr)) return FALSE;
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
	if(!pPlayer) return NO_OWNER;
	return pPlayer->Number;
	}

static long FnEliminatePlayer(C4AulContext *cthr, long iPlr, bool fRemoveDirect)
  {
	C4Player *pPlr=::Players.Get(iPlr);
	if (!pPlr) return FALSE;
	// direct removal?
	if (fRemoveDirect)
		{
		// do direct removal (no fate)
		return ::Players.CtrlRemove(iPlr, false);
		}
	else
		{
		// do regular elimination
		if (pPlr->Eliminated) return FALSE;
		pPlr->Eliminate();
		}
	return TRUE;
  }

static bool FnSurrenderPlayer(C4AulContext *cthr, long iPlr)
  {
	C4Player *pPlr=::Players.Get(iPlr);
	if (!pPlr) return FALSE;
	if (pPlr->Eliminated) return FALSE;
  pPlr->Surrender();
	return TRUE;
  }

static bool FnSetLeaguePerformance(C4AulContext *cthr, long iScore)
	{
	Game.RoundResults.SetLeaguePerformance(iScore);
	return TRUE;
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

static C4Object *FnGetCursor(C4AulContext *cthr, long iPlr, long iIndex)
  {
	// get player
	C4Player *pPlr = ::Players.Get(iPlr);
	// invalid player?
	if (!pPlr) return NULL;
	// first index is always the cursor
  if (!iIndex) return pPlr->Cursor;
	// iterate through selected crew for iIndex times
	// status needs not be checked, as dead objects are never in Crew list
	C4Object *pCrew;
	for (C4ObjectLink *pLnk=pPlr->Crew.First; pLnk; pLnk=pLnk->Next)
		// get crew object
		if (pCrew = pLnk->Obj)
			// is it selected?
			if (pCrew->Select)
				// is it not the cursor? (which is always first)
				if (pCrew != pPlr->Cursor)
					// enough searched?
					if (!--iIndex)
						// return it
						return pCrew;
	// nothing found at that index
	return NULL;
  }

static C4Object *FnGetViewCursor(C4AulContext *cthr, long iPlr)
  {
	// get player
	C4Player *pPlr = ::Players.Get(iPlr);
	// get viewcursor
	return pPlr ? pPlr->ViewCursor : NULL;
	}

static C4Object *FnGetCaptain(C4AulContext *cthr, long iPlr)
  {
  if (!ValidPlr(iPlr)) return FALSE;
  return ::Players.Get(iPlr)->Captain;
  }

static bool FnSetCursor(C4AulContext *cthr, long iPlr, C4Object *pObj, bool fNoSelectMark, bool fNoSelectArrow, bool fNoSelectCrew)
  {
	C4Player *pPlr = ::Players.Get(iPlr);
	if (!pPlr || (pObj && !pObj->Status)) return FALSE;
	pPlr->SetCursor(pObj, !fNoSelectMark, !fNoSelectArrow);
	if (!fNoSelectCrew) pPlr->SelectCrew(pObj, true);
	return TRUE;
  }

static bool FnSetViewCursor(C4AulContext *cthr, long iPlr, C4Object *pObj)
  {
	// get player
	C4Player *pPlr = ::Players.Get(iPlr);
	// invalid player?
	if (!pPlr) return FALSE;
	// set viewcursor
	pPlr->ViewCursor = pObj;
	return TRUE;
	}

static bool FnSelectCrew(C4AulContext *cthr, long iPlr, C4Object *pObj, bool fSelect, bool fNoCursorAdjust)
  {
	C4Player *pPlr = ::Players.Get(iPlr);
	if (!pPlr || !pObj) return FALSE;
	if (fNoCursorAdjust)
		{ if (fSelect) pObj->DoSelect(); else pObj->UnSelect(); }
	else
		pPlr->SelectCrew(pObj,fSelect);
	return TRUE;
  }

static long FnGetSelectCount(C4AulContext *cthr, long iPlr)
  {
  if (!ValidPlr(iPlr)) return FALSE;
  return ::Players.Get(iPlr)->SelectCount;
  }

static long FnSetCrewStatus(C4AulContext *cthr, long iPlr, bool fInCrew, C4Object *pObj)
	{
	// validate player
	C4Player *pPlr = ::Players.Get(iPlr);
	if (!pPlr) return FALSE;
	// validate object / local call
	if (!pObj) if (!(pObj=cthr->Obj)) return FALSE;
	// set crew status
	return pPlr->SetObjectCrewStatus(pObj, fInCrew);
	}

static long FnGetWind(C4AulContext *cthr, long x, long y, bool fGlobal)
  {
	// global wind
	if (fGlobal) return ::Weather.Wind;
	// local wind
	if (cthr->Obj) { x+=cthr->Obj->GetX(); y+=cthr->Obj->GetY(); }
  return ::Weather.GetWind(x,y);
  }

static bool FnSetWind(C4AulContext *cthr, long iWind)
  {
	::Weather.SetWind(iWind);
  return TRUE;
  }

static bool FnSetTemperature(C4AulContext *cthr, long iTemperature)
  {
	::Weather.SetTemperature(iTemperature);
  return TRUE;
  }

static long FnGetTemperature(C4AulContext *cthr)
  {
  return ::Weather.GetTemperature();
  }

static bool FnSetSeason(C4AulContext *cthr, long iSeason)
  {
	::Weather.SetSeason(iSeason);
  return TRUE;
  }

static long FnGetSeason(C4AulContext *cthr)
  {
  return ::Weather.GetSeason();
  }

static bool FnSetClimate(C4AulContext *cthr, long iClimate)
  {
	::Weather.SetClimate(iClimate);
  return TRUE;
  }

static long FnGetClimate(C4AulContext *cthr)
  {
  return ::Weather.GetClimate();
  }

static bool FnSetSkyFade(C4AulContext *cthr, long iFromRed, long iFromGreen, long iFromBlue, long iToRed, long iToGreen, long iToBlue)
	{
	// newgfx: set modulation
	DWORD dwBack,dwMod=GetClrModulation(::Landscape.Sky.FadeClr1, C4RGB(iFromRed, iFromGreen, iFromBlue), dwBack);
	::Landscape.Sky.SetModulation(dwMod,dwBack);
	return TRUE;
	}

static bool FnSetSkyColor(C4AulContext *cthr, long iIndex, long iRed, long iGreen, long iBlue)
	{
	// set first index only
	if (iIndex) return TRUE;
	// get color difference
	DWORD dwBack,dwMod=GetClrModulation(::Landscape.Sky.FadeClr1, C4RGB(iRed, iGreen, iBlue), dwBack);
	::Landscape.Sky.SetModulation(dwMod,dwBack);
	// success
	return TRUE;
	}

static long FnGetSkyColor(C4AulContext *cthr, long iIndex, long iRGB)
	{
	// relict from OldGfx
	if (iIndex || !Inside<long>(iRGB,0,2)) return 0;
	DWORD dwClr=::Landscape.Sky.FadeClr1;
	BltAlpha(dwClr, ::Landscape.Sky.FadeClr2 | ((iIndex*0xff/19)<<24));
	switch (iRGB)
		{
		case 0: return (dwClr>>16)&0xff;
		case 1: return (dwClr>>8)&0xff;
		case 2: return dwClr&0xff;
		default: return 0;
		}
	}

static long FnLandscapeWidth(C4AulContext *cthr)
	{
	return GBackWdt;
	}

static long FnLandscapeHeight(C4AulContext *cthr)
	{
	return GBackHgt;
	}

static long FnLaunchLightning(C4AulContext *cthr, long x, long y, long xdir, long xrange, long ydir, long yrange, bool fDoGamma)
	{
	return ::Weather.LaunchLightning(x,y,xdir,xrange,ydir,yrange, fDoGamma);
	}

static long FnLaunchVolcano(C4AulContext *cthr, long x)
	{
	return ::Weather.LaunchVolcano(
		       ::MaterialMap.Get("Lava"),
					 x,GBackHgt-1,
					 BoundBy(15*GBackHgt/500+Random(10),10,60));
	}

static bool FnLaunchEarthquake(C4AulContext *cthr, long x, long y)
	{
	::Weather.LaunchEarthquake(x,y);
	return 1;
	}

static bool FnShakeFree(C4AulContext *cthr, long x, long y, long rad)
	{
	::Landscape.ShakeFree(x,y,rad);
	return 1;
	}

static bool FnShakeObjects(C4AulContext *cthr, long x, long y, long rad)
	{
	Game.ShakeObjects(x,y,rad);
	return 1;
	}

static bool FnDigFree(C4AulContext *cthr, long x, long y, long rad, bool fRequest)
	{
	::Landscape.DigFree(x,y,rad,fRequest,cthr->Obj);
	return 1;
	}

static bool FnDigFreeRect(C4AulContext *cthr, long iX, long iY, long iWdt, long iHgt, bool fRequest)
	{
	::Landscape.DigFreeRect(iX,iY,iWdt,iHgt,fRequest,cthr->Obj);
	return TRUE;
	}

static bool FnFreeRect(C4AulContext *cthr, long iX, long iY, long iWdt, long iHgt, long iFreeDensity)
	{
	if (iFreeDensity)
		::Landscape.ClearRectDensity(iX,iY,iWdt,iHgt,iFreeDensity);
	else
		::Landscape.ClearRect(iX,iY,iWdt,iHgt);
	return TRUE;
	}

static bool FnPathFree(C4AulContext *cthr, long X1, long Y1, long X2, long Y2)
	{
	return !!PathFree(X1, Y1, X2, Y2);
	}

static C4Value FnPathFree2_C4V(C4AulContext *cthr, C4Value * X1, C4Value * Y1, C4Value * X2, C4Value * Y2)
	{
	int32_t x = -1, y = -1;
	// Do not use getInt on the references, because it destroys them.
	bool r = !!PathFree(X1->GetRefVal().getInt(), Y1->GetRefVal().getInt(), X2->getInt(), Y2->getInt(), &x, &y);
	if (!r)
		{
		*X1 = C4VInt(x);
		*Y1 = C4VInt(y);
		}
	return C4VBool(r);
	}

static long FnSetTransferZone(C4AulContext *cthr, long iX, long iY, long iWdt, long iHgt, C4Object *pObj)
	{
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
	iX+=pObj->GetX(); iY+=pObj->GetY();
	return Game.TransferZones.Set(iX,iY,iWdt,iHgt,pObj);
	}

static bool FnNot(C4AulContext *cthr, bool fCondition)
  {
  return !fCondition;
  }

static bool FnOr(C4AulContext *cthr, bool fCon1, bool fCon2, bool fCon3, bool fCon4, bool fCon5)
  {
  return (fCon1 || fCon2 || fCon3 || fCon4 || fCon5);
  }

static bool FnAnd(C4AulContext *cthr, bool fCon1, bool fCon2)
  {
  return (fCon1 && fCon2);
  }

static long FnBitAnd(C4AulContext *cthr, long iVal1, long iVal2)
	{
	return (iVal1 & iVal2);
	}

static C4Value FnEqual_C4V(C4AulContext *cthr, C4Value * Val1, C4Value * Val2)
  {
  return C4VBool(Val1->GetData()==Val2->GetData());
  }

static long FnLessThan(C4AulContext *cthr, long iVal1, long iVal2)
  {
  return (iVal1<iVal2);
  }

static long FnGreaterThan(C4AulContext *cthr, long iVal1, long iVal2)
  {
  return (iVal1>iVal2);
  }

static long FnSum(C4AulContext *cthr, long iVal1, long iVal2, long iVal3, long iVal4)
  {
  return (iVal1+iVal2+iVal3+iVal4);
  }

static long FnSub(C4AulContext *cthr, long iVal1, long iVal2, long iVal3, long iVal4)
  {
  return (iVal1-iVal2-iVal3-iVal4);
  }

static long FnAbs(C4AulContext *cthr, long iVal)
  {
  return Abs(iVal);
  }

static long FnMul(C4AulContext *cthr, long iVal1, long iVal2)
  {
  return (iVal1*iVal2);
  }

static long FnDiv(C4AulContext *cthr, long iVal1, long iVal2)
  {
  if (!iVal2) return 0;
  return (iVal1/iVal2);
  }

static long FnMod(C4AulContext *cthr, long iVal1, long iVal2)
  {
  if (!iVal2) return 0;
  return (iVal1%iVal2);
  }

static long FnPow(C4AulContext *cthr, long iVal1, long iVal2)
  {
  return Pow(iVal1, iVal2);
  }

static long FnSin(C4AulContext *cthr, long iAngle, long iRadius, long iPrec)
	{
	if(!iPrec) iPrec = 1;
	// Precalculate the modulo operation so the C4Fixed argument to Sin does not overflow
	iAngle %= 360 * iPrec;
	// Let itofix and fixtoi handle the division and multiplication because that can handle higher ranges
	return fixtoi(Sin(itofix(iAngle, iPrec)), iRadius);
	}

static long FnCos(C4AulContext *cthr, long iAngle, long iRadius, long iPrec)
	{
	if(!iPrec) iPrec = 1;
	iAngle %= 360 * iPrec;
	return fixtoi(Cos(itofix(iAngle, iPrec)), iRadius);
	}

static long FnSqrt(C4AulContext *cthr, long iValue)
	{
	if (iValue<0) return 0;
	long iSqrt = long(sqrt(double(iValue)));
	if(iSqrt * iSqrt < iValue) iSqrt++;
	if(iSqrt * iSqrt > iValue) iSqrt--;
	return iSqrt;
	}

static long FnAngle(C4AulContext *cthr, long iX1, long iY1, long iX2, long iY2, long iPrec)
	{
	long iAngle;

	// Standard prec
	if(!iPrec) iPrec = 1;

	long dx=iX2-iX1,dy=iY2-iY1;
	if (!dx) if (dy>0) return 180 * iPrec; else return 0;
	if (!dy) if (dx>0) return 90 * iPrec; else return 270 * iPrec;

	iAngle = static_cast<long>(180.0 * iPrec * atan2(static_cast<double>(Abs(dy)), static_cast<double>(Abs(dx))) / pi);

	if (iX2>iX1 )
		{
		if (iY2<iY1) iAngle = (90 * iPrec) - iAngle;
		else iAngle = (90 * iPrec) + iAngle;
		}
	else
		{
		if (iY2<iY1) iAngle = (270 * iPrec) + iAngle;
		else iAngle = (270 * iPrec) - iAngle;
		}

	return iAngle;
	}

static long FnArcSin(C4AulContext *cthr, long iVal, long iRadius)
	{
	// safety
	if (!iRadius) return 0;
	if (iVal > iRadius) return 0;
	// calc arcsin
	double f1 = iVal;
	f1 = asin(f1/iRadius)*180.0/pi;
	// return rounded angle
	return (long) floor(f1+0.5);
	}

static long FnArcCos(C4AulContext *cthr, long iVal, long iRadius)
	{
	// safety
	if (!iRadius) return 0;
	if (iVal > iRadius) return 0;
	// calc arccos
	double f1 = iVal;
	f1 = acos(f1/iRadius)*180.0/pi;
	// return rounded angle
	return (long) floor(f1+0.5);
	}

static long FnMin(C4AulContext *cthr, long iVal1, long iVal2)
  {
  return Min(iVal1,iVal2);
  }

static long FnMax(C4AulContext *cthr, long iVal1, long iVal2)
  {
  return Max(iVal1,iVal2);
  }

static long FnDistance(C4AulContext *cthr, long iX1, long iY1, long iX2, long iY2)
	{
	return Distance(iX1,iY1,iX2,iY2);
	}

static long FnObjectDistance(C4AulContext *cthr, C4Object *pObj2, C4Object *pObj)
	{
	if (!pObj) pObj=cthr->Obj; if (!pObj || !pObj2) return 0;
	return Distance(pObj->GetX(),pObj->GetY(),pObj2->GetX(),pObj2->GetY());
	}

static long FnObjectNumber(C4AulContext *cthr, C4Object *pObj)
	{
	if (!pObj) pObj=cthr->Obj; if (!pObj) return 0;
	return pObj->Number;
	}

C4Object* FnObject(C4AulContext *cthr, long iNumber)
	{
	return ::Objects.SafeObjectPointer(iNumber);
	}

static long FnShowInfo(C4AulContext *cthr, C4Object *pObj)
	{
	if (!cthr->Obj) return FALSE;
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
	return cthr->Obj->ActivateMenu(C4MN_Info,0,0,0,pObj);
	}

static long FnBoundBy(C4AulContext *cthr, long iVal, long iRange1, long iRange2)
  {
  return BoundBy(iVal,iRange1,iRange2);
  }

static bool FnInside(C4AulContext *cthr, long iVal, long iRange1, long iRange2)
  {
  return Inside(iVal,iRange1,iRange2);
  }

static long FnSEqual(C4AulContext *cthr, C4String *szString1, C4String *szString2)
  {
  if(szString1 == szString2) return TRUE;
  return SEqual(FnStringPar(szString1),FnStringPar(szString2));
  }

static long FnRandom(C4AulContext *cthr, long iRange)
  {
  return Random(iRange);
  }

static long FnAsyncRandom(C4AulContext *cthr, long iRange)
  {
  return SafeRandom(iRange);
  }

static C4Value FnSetVar_C4V(C4AulContext *cthr, C4Value* iVarIndex, C4Value* iValue)
  {
	if(!cthr->Caller) return C4VNull;
  cthr->Caller->NumVars[iVarIndex->getInt()] = *iValue;
  return *iValue;
  }

static C4Value FnIncVar_C4V(C4AulContext *cthr, C4Value* iVarIndex)
  {
	if(!cthr->Caller) return C4VNull;
  return ++cthr->Caller->NumVars[iVarIndex->getInt()];
  }

static C4Value FnDecVar_C4V(C4AulContext *cthr, C4Value* iVarIndex)
  {
	if(!cthr->Caller) return C4VNull;
  return --cthr->Caller->NumVars[iVarIndex->getInt()];
  }

static C4Value FnVar_C4V(C4AulContext *cthr, C4Value* iVarIndex)
  {
	if(!cthr->Caller) return C4VNull;
	// Referenz zurückgeben
  return cthr->Caller->NumVars[iVarIndex->getInt()].GetRef();
  }

static C4Value FnSetGlobal_C4V(C4AulContext *cthr, C4Value* iVarIndex, C4Value* iValue)
  {
	::ScriptEngine.Global[iVarIndex->getInt()]=*iValue;
  return *iValue;
  }

static C4Value FnGlobal_C4V(C4AulContext *cthr, C4Value* iVarIndex)
  {
  return ::ScriptEngine.Global[iVarIndex->getInt()].GetRef();
  }

static C4Value FnSetLocal_C4V(C4AulContext *cthr, C4Value* iVarIndex, C4Value* iValue, C4Value* pObj_C4V)
  {
	C4Object* pObj = pObj_C4V->getObj();

	if (!pObj) pObj=cthr->Obj; if (!pObj) return C4VBool(FALSE);
	pObj->Local[iVarIndex->getInt()] = *iValue;
  return *iValue;
  }

static C4Value FnLocal_C4V(C4AulContext *cthr, C4Value* iVarIndex, C4Value* pObj_C4V)
  {
	C4Object* pObj = pObj_C4V->getObj();
	long iIndex = iVarIndex->getInt();

	if (!pObj) pObj=cthr->Obj; if (!pObj) return C4Value();
	if (iIndex<0) return C4Value();
  return pObj->Local[iIndex].GetRef();
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

static C4Value FnObjectCall_C4V(C4AulContext *cthr,
                 C4Value* pObj_C4V, C4Value* szFunction_C4V,
								 C4Value* par0, C4Value* par1, C4Value* par2, C4Value* par3, C4Value* par4,
								 C4Value* par5, C4Value* par6, C4Value* par7/*, C4Value* par8, C4Value* par9*/)
  {
	C4Object *pObj = pObj_C4V->getObj();
	C4String *szFunction = szFunction_C4V->getStr();

  if (!pObj || !szFunction) return C4Value();
	if (!pObj->Def) return C4Value();
	// get func
	C4AulFunc *f;
	if (!(f = pObj->Def->Script.GetSFunc(FnStringPar(szFunction), AA_PUBLIC, TRUE))) return C4Value();
	// copy pars
	C4AulParSet Pars;
	Copy2ParSet8(Pars, *par);
	// exec
  return f->Exec(pObj,&Pars, true);
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
  return Game.Script.Call(szFunc2, 0, &Pars, true);
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
  return Game.Script.GRBroadcast(szFunc2,&Pars, true);
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
	if (!(f = pObj->Def->Script.GetSFunc(FnStringPar(szFunction), AA_PROTECTED, TRUE))) return C4Value();
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
	if (!(f = pObj->Def->Script.GetSFunc(FnStringPar(szFunction), AA_PRIVATE, TRUE))) return C4Value();
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

static bool FnResort(C4AulContext *cthr, C4Object *pObj)
	{
  if (!pObj) pObj=cthr->Obj;
	// Resort single object
	if (pObj)
		pObj->Resort();
	// Resort object list
	else
		::Objects.SortByCategory();
	return TRUE;
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
	if (!pPlr) return FALSE;
	if (!fTestIfInUse) return TRUE;
	// single query only if no query is scheduled
	return pPlr->HasMessageBoardQuery();
	}

static bool FnCallMessageBoard(C4AulContext *cthr, C4Object *pObj, bool fUpperCase, C4String *szQueryString, long iForPlr)
	{
  if (!pObj) pObj=cthr->Obj;
	if (pObj && !pObj->Status) return FALSE;
	// check player
	C4Player *pPlr = ::Players.Get(iForPlr);
	if (!pPlr) return FALSE;
	// remove any previous
	pPlr->CallMessageBoard(pObj, StdStrBuf(FnStringPar(szQueryString)), !!fUpperCase);
	return TRUE;
	}

static bool FnAbortMessageBoard(C4AulContext *cthr, C4Object *pObj, long iForPlr)
	{
  if (!pObj) pObj=cthr->Obj;
	// check player
	C4Player *pPlr = ::Players.Get(iForPlr);
	if (!pPlr) return FALSE;
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
	if (!pPlr) return FALSE;
	if (!pPlr->RemoveMessageBoardQuery(pObj)) return FALSE;
	// if no answer string is provided, the user did not answer anything
	// just remove the query
	if (!szAnswerString || !szAnswerString->GetCStr()) return TRUE;
	// get script
	C4ScriptHost *scr;
	if (pObj) scr = &pObj->Def->Script; else scr = &Game.Script;
	// exec func
	return !!scr->Call(PSF_InputCallback, pObj, &C4AulParSet(C4VString(FnStringPar(szAnswerString)), C4VInt(iForPlr)));
	}

static long FnScriptCounter(C4AulContext *cthr)
	{
	return Game.Script.Counter;
	}

static long FnSetMass(C4AulContext *cthr, long iValue, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
	pObj->OwnMass=iValue-pObj->Def->Mass;
	pObj->UpdateMass();
	return TRUE;
  }

static long FnGetColor(C4AulContext *cthr, C4Object *pObj)
  {
	// oldgfx
	return 0;
  }

static long FnSetColor(C4AulContext *cthr, long iValue, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
	if (!Inside<long>(iValue, 0, C4MaxColor-1)) return FALSE;
	iValue=Application.DDraw->Pal.GetClr(FColors[FPlayer+iValue]);
	pObj->Color=iValue;
	pObj->UpdateGraphics(false);
	pObj->UpdateFace(false);
	return TRUE;
  }

static long FnGetColorDw(C4AulContext *cthr, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
	return pObj->Color;
  }

static long FnGetPlrColorDw(C4AulContext *cthr, long iPlr)
  {
	// get player
	C4Player *pPlr = ::Players.Get(iPlr);
	// safety
	if (!pPlr) return 0;
	// return player color
	return pPlr->ColorDw;
  }

static bool FnSetColorDw(C4AulContext *cthr, long iValue, C4Object *pObj)
  {
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
	pObj->Color=iValue;
	pObj->UpdateGraphics(false);
	pObj->UpdateFace(false);
	return TRUE;
  }

static long FnSetFoW(C4AulContext *cthr, bool fEnabled, long iPlr)
	{
	// safety
	if (!ValidPlr(iPlr)) return FALSE;
	// set enabled
	::Players.Get(iPlr)->SetFoW(!!fEnabled);
	// success
	return TRUE;
	}

static long FnSetPlrViewRange(C4AulContext *cthr, long iRange, C4Object *pObj, bool fExact)
	{
	// local/safety
  if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
	// backwards compatibility for low ranges
	if (!fExact && iRange < 128 && iRange>0) iRange = 128;
	// set range
	pObj->SetPlrViewRange(iRange);
	// success
	return TRUE;
	}

static long FnSetMaxPlayer(C4AulContext *cthr, long iTo)
	{
	// think positive! :)
	if (iTo < 0) return FALSE;
	// script functions don't need to pass ControlQueue!
	Game.Parameters.MaxPlayers = iTo;
	// success
	return TRUE;
	}

static long FnSetPicture(C4AulContext *cthr, long iX, long iY, long iWdt, long iHgt, C4Object *pObj)
	{
	// local/safety
	if (!pObj) pObj=cthr->Obj; if (!pObj) return FALSE;
	// set new picture rect
	pObj->PictureRect.Set(iX, iY, iWdt, iHgt);
	// success
	return TRUE;
	}

static C4String *FnGetProcedure(C4AulContext *cthr, C4Object *pObj)
	{
	// local/safety
	if (!pObj) pObj=cthr->Obj; if (!pObj) return NULL;
	// no action?
	if (!pObj->Action.pActionDef) return NULL;
	// get proc
	long iProc = pObj->Action.pActionDef->GetPropertyInt(P_Procedure);
	// NONE?
	if (iProc <= DFA_NONE) return NULL;
	// return procedure name
	return String(ProcedureName[iProc]);
	}

static C4Object *FnBuy(C4AulContext *cthr, C4ID idBuyObj, long iForPlr, long iPayPlr, C4Object *pToBase, bool fShowErrors)
	{
	// safety
	if (!ValidPlr(iForPlr) || !ValidPlr(iPayPlr)) return NULL;
	// buy
	C4Object *pThing;
	if (!(pThing=::Players.Get(iPayPlr)->Buy(idBuyObj, fShowErrors, iForPlr, pToBase ? pToBase : cthr->Obj))) return NULL;
	// enter object, if supplied
	if (pToBase)
		{
		pThing->Enter(pToBase, TRUE);
		}
	else
		{
		// no target object? get local pos
		if (cthr->Obj) pThing->ForcePosition(cthr->Obj->GetX(), cthr->Obj->GetY());
		}
	// done
	return pThing;
	}

static long FnSell(C4AulContext *cthr, long iToPlr, C4Object *pObj)
	{
	// local/safety
	if (!pObj) pObj=cthr->Obj; if (!pObj) return 0;
	if (!ValidPlr(iToPlr)) return FALSE;
	// sell
	return ::Players.Get(iToPlr)->Sell2Home(pObj);
	}

// ** additional funcs for references/type info

static C4Value FnSet(C4AulContext *cthr, C4Value* Dest, C4Value* Src)
{
	*Dest = *Src;
	return *Dest;
}

static C4Value FnInc(C4AulContext *cthr, C4Value* Value, C4Value* Diff)
{
	if(!Value->GetRefVal().ConvertTo(C4V_Int))
		return C4Value();

	*Value += Diff->getInt();

	return *Value;
}

static C4Value FnDec(C4AulContext *cthr, C4Value* Value, C4Value* Diff)
{
	if(!Value->GetRefVal().ConvertTo(C4V_Int))
		return C4Value();

	*Value -= Diff->getInt();

	return *Value;
}

static C4Value FnIsRef(C4AulContext *cthr, C4Value* Value)
{
	return C4VBool(Value->IsRef());
}

static C4Value FnGetType(C4AulContext *cthr, C4Value* Value)
{
	return C4VInt(Value->GetType());
}

static C4Value FnCreateArray(C4AulContext *cthr, C4Value *pPars)
{
	PAR(int, iSize);
	return C4VArray(new C4ValueArray(iSize));
}

static C4Value FnGetLength(C4AulContext *cthr, C4Value *pPars)
{
	// support GetLength() etc.
	if (!pPars[0]) return C4VNull;
	C4ValueArray * pArray = pPars->getArray();
	if (pArray)
		return C4VInt(pArray->GetSize());
	C4String * pStr = pPars->getStr();
	if (pStr)
		return C4VInt(pStr->GetData().getLength());
	throw new C4AulExecError(cthr->Obj, "func \"GetLength\" par 0 cannot be converted to string or array");
}

static C4Value FnGetIndexOf(C4AulContext *cthr, C4Value *pPars)
	{
	// find first occurance of first parameter in array
	// support GetIndexOf(x, 0)
	if (!pPars[1]) return C4VInt(-1);
	// if the second param is nonzero, it must be an array
	const C4ValueArray * pArray = pPars[1].getArray();
	if (!pArray)
		throw new C4AulExecError(cthr->Obj, "func \"GetIndexOf\" par 1 cannot be converted to array");
	// find the element by comparing data only - this may result in bogus results if an object ptr array is searched for an int
	// however, that's rather unlikely and strange scripting style
	int32_t iSize = pArray->GetSize();
	long cmp = pPars[0].GetData().Int;
	for (int32_t i = 0; i<iSize; ++i)
		if (cmp == pArray->GetItem(i).GetData().Int)
			// element found
			return C4VInt(i);
	// element not found
	return C4VInt(-1);
	}

static C4Value FnSetLength(C4AulContext *cthr, C4Value *pPars)
{
	PAR(ref, pArrayRef);
	PAR(int, iNewSize);

	// safety
	if (iNewSize<0 || iNewSize > C4ValueList::MaxSize)
		throw new C4AulExecError(cthr->Obj, FormatString("SetLength: invalid array size (%d)", iNewSize).getData());

	// set new size
	pArrayRef.SetArrayLength(iNewSize, cthr);

	// yeah, done!
	return C4VBool(true);
}

static bool FnSetVisibility(C4AulContext *cthr, long iVisibility, C4Object *pObj)
{
	// local call/safety
	if(!pObj) pObj = cthr->Obj;
	if(!pObj) return FALSE;

	pObj->Visibility=iVisibility;

	return TRUE;
}

static long FnGetVisibility(C4AulContext *cthr, C4Object *pObj)
{
	if(!pObj) pObj = cthr->Obj;
	if(!pObj) return 0;

	return pObj->Visibility;
}

static bool FnSetClrModulation(C4AulContext *cthr, long dwClr, C4Object *pObj, long iOverlayID)
	{
	// local call/safety
	if(!pObj) pObj = cthr->Obj; if(!pObj) return FALSE;
	// overlay?
	if (iOverlayID)
		{
		C4GraphicsOverlay *pOverlay = pObj->GetGraphicsOverlay(iOverlayID, false);
		if (!pOverlay)
			{
			DebugLogF("SetClrModulation: Overlay %d not defined for object %d (%s)", (int) iOverlayID, (int) pObj->Number, (const char *) pObj->GetName());
			return FALSE;
			}
		pOverlay->SetClrModulation(dwClr);
		}
	else
		{
		// set it
		pObj->ColorMod=dwClr;
		}
	// success
	return TRUE;
	}

static long FnGetClrModulation(C4AulContext *cthr, C4Object *pObj, long iOverlayID)
	{
	// local call/safety
	if(!pObj) pObj = cthr->Obj; if(!pObj) return FALSE;
	// overlay?
	if (iOverlayID)
		{
		C4GraphicsOverlay *pOverlay = pObj->GetGraphicsOverlay(iOverlayID, false);
		if (!pOverlay)
			{
			DebugLogF("GetClrModulation: Overlay %d not defined for object %d (%s)", (int) iOverlayID, (int) pObj->Number, (const char *) pObj->GetName());
			return FALSE;
			}
		return pOverlay->GetClrModulation();
		}
	else
		// get it
		return pObj->ColorMod;
	}

static bool FnGetMissionAccess(C4AulContext *cthr, C4String *strMissionAccess)
{
	// safety
	if (!strMissionAccess) return FALSE;

	// non-sync mode: warn
	if(::Control.SyncMode())
		Log("Warning: using GetMissionAccess may cause desyncs when playing records!");

	if (!Config.General.MissionAccess) return FALSE;
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
    if(!iMatchStart || haveCurrentMatch())
      // already got all names?
      if(!haveCompleteMatch())
        // check name
        if(SEqual(pszNames[iMatchCount], szName))
        {
          // got match
          if(!iMatchCount) iMatchStart = iDepth + 1;
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
    if(haveCurrentMatch())
    {
      iMatchCount--;
      if(!iMatchCount) iMatchStart = 0;
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
  virtual void DWord(int32_t &rInt)      { if(haveCompleteMatch()) if(!iEntryNr--) ProcessInt(rInt); }
  virtual void DWord(uint32_t &rInt)     { if(haveCompleteMatch()) if(!iEntryNr--) { int32_t i=rInt;   ProcessInt(i); rInt  =i; } }
  virtual void Word(int16_t &rShort)     { if(haveCompleteMatch()) if(!iEntryNr--) { int32_t i=rShort; ProcessInt(i); rShort=i; } }
  virtual void Word(uint16_t &rShort)    { if(haveCompleteMatch()) if(!iEntryNr--) { int32_t i=rShort; ProcessInt(i); rShort=i; } }
  virtual void Byte(int8_t &rByte)       { if(haveCompleteMatch()) if(!iEntryNr--) { int32_t i=rByte;  ProcessInt(i); rByte =i; } }
  virtual void Byte(uint8_t &rByte)      { if(haveCompleteMatch()) if(!iEntryNr--) { int32_t i=rByte;  ProcessInt(i); rByte =i; } }
  virtual void Boolean(bool &rBool)      { if(haveCompleteMatch()) if(!iEntryNr--) ProcessBool(rBool); }
  virtual void Character(char &rChar)    { if(haveCompleteMatch()) if(!iEntryNr--) ProcessChar(rChar); }

  // The C4ID-Adaptor will set RCT_ID for it's strings (see C4Id.h), so we don't have to guess the type.
  virtual void String(char *szString, size_t iMaxLength, RawCompileType eType)
    { if(haveCompleteMatch()) if(!iEntryNr--) ProcessString(szString, iMaxLength, eType == StdCompiler::RCT_ID); }
  virtual void String(char **pszString, RawCompileType eType)
    { if(haveCompleteMatch()) if(!iEntryNr--) ProcessString(pszString, eType == StdCompiler::RCT_ID); }
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
		{ Res = (fIsID ? C4VID(C4Id(szString)) : C4VString(szString)); }
	virtual void ProcessString(char **pszString, bool fIsID)
		{ Res = (fIsID ? C4VID(C4Id(*pszString)) : C4VString(*pszString)); }
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
		catch(StdCompiler::Exception *)
		{
			return C4VNull;
		}
	}

static C4Value FnGetDefCoreVal(C4AulContext* cthr, C4Value* strEntry_C4V, C4Value* strSection_C4V, C4Value* idDef_C4V, C4Value *iEntryNr_C4V)
{
	const char *strEntry = FnStringPar(strEntry_C4V->getStr());
	const char *strSection = FnStringPar(strSection_C4V->getStr());
	if (strSection && !*strSection) strSection = NULL;
	C4ID idDef = idDef_C4V->getC4ID();
	long iEntryNr = iEntryNr_C4V->getInt();

	if(!idDef) if(cthr->Def) idDef = cthr->Def->id;
	if(!idDef) return C4Value();

	C4Def* pDef = C4Id2Def(idDef);
	if(!pDef) return C4Value();

	return GetValByStdCompiler(strEntry, strSection, iEntryNr, mkNamingAdapt(*pDef, "DefCore"));
}

static C4Value FnGetObjectVal(C4AulContext* cthr, C4Value* strEntry_C4V, C4Value* strSection_C4V, C4Value* pObj_C4V, C4Value *iEntryNr_C4V)
{
	const char *strEntry = FnStringPar(strEntry_C4V->getStr());
	const char *strSection = FnStringPar(strSection_C4V->getStr());
	if (!*strSection) strSection = NULL;

	C4Object *pObj = pObj_C4V->getObj();
	long iEntryNr = iEntryNr_C4V->getInt();

	if(!pObj) pObj = cthr->Obj;
	if(!pObj) return C4Value();

	// get value
	return GetValByStdCompiler(strEntry, strSection, iEntryNr, mkNamingAdapt(*pObj, "Object"));
}

static C4Value FnGetObjectInfoCoreVal(C4AulContext* cthr, C4Value* strEntry_C4V, C4Value* strSection_C4V, C4Value* pObj_C4V, C4Value *iEntryNr_C4V)
{
	const char *strEntry = FnStringPar(strEntry_C4V->getStr());
	const char *strSection = FnStringPar(strSection_C4V->getStr());
	if (strSection && !*strSection) strSection = NULL;
	C4Object *pObj = pObj_C4V->getObj();
	long iEntryNr = iEntryNr_C4V->getInt();

	if(!pObj) pObj = cthr->Obj;
	if(!pObj) return C4Value();

	// get obj info
	C4ObjectInfo* pObjInfo = pObj->Info;

	if(!pObjInfo) return C4Value();

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

  if(strSection && !*strSection) strSection = NULL;

	return GetValByStdCompiler(strEntry, strSection, iEntryNr, mkParAdapt(Game.C4S, false));
}

static C4Value FnGetPlayerVal(C4AulContext* cthr, C4Value* strEntry_C4V, C4Value* strSection_C4V, C4Value* iPlayer_C4V, C4Value *iEntryNr_C4V)
{
	const char *strEntry = FnStringPar(strEntry_C4V->getStr());
	const char *strSection = FnStringPar(strSection_C4V->getStr());
	if(strSection && !*strSection) strSection = NULL;
	long iPlr = iPlayer_C4V->getInt();
	long iEntryNr = iEntryNr_C4V->getInt();

	if(!ValidPlr(iPlr)) return C4Value();

	// get player
	C4Player* pPlayer = ::Players.Get(iPlr);

	// get value
	return GetValByStdCompiler(strEntry, strSection, iEntryNr, mkNamingAdapt(*pPlayer, "Player"));
}

static C4Value FnGetPlayerInfoCoreVal(C4AulContext* cthr, C4Value* strEntry_C4V, C4Value* strSection_C4V, C4Value* iPlayer_C4V, C4Value *iEntryNr_C4V)
{
	const char *strEntry = FnStringPar(strEntry_C4V->getStr());
	const char *strSection = FnStringPar(strSection_C4V->getStr());
	if(strSection && !*strSection) strSection = NULL;
	long iPlr = iPlayer_C4V->getInt();
	long iEntryNr = iEntryNr_C4V->getInt();

	if(!ValidPlr(iPlr)) return C4Value();

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
	if(strSection && !*strSection) strSection = NULL;
	long iMat = iMat_C4V->getInt();
	long iEntryNr = iEntryNr_C4V->getInt();

	if(iMat < 0 || iMat >= ::MaterialMap.Num) return C4Value();

	// get material
	C4Material *pMaterial = &::MaterialMap.Map[iMat];

	// get plr info core
	C4MaterialCore* pMaterialCore = static_cast<C4MaterialCore*>(pMaterial);

	// material core implicates section "Material"
	if (!SEqual(strSection, "Material")) return C4Value();

	// get value
	return GetValByStdCompiler(strEntry, NULL, iEntryNr, *pMaterialCore);
}

static long FnCloseMenu(C4AulContext* cthr, C4Object* pObj)
{
	if(!pObj) pObj = cthr->Obj;
	if(!pObj) return FALSE;
	return pObj->CloseMenu(true);
}

static long FnGetMenuSelection(C4AulContext* cthr, C4Object* pObj)
{
	if(!pObj) pObj = cthr->Obj;
	if(!pObj) return -1;
	if(!pObj->Menu || !pObj->Menu->IsActive()) return -1;
	return pObj->Menu->GetSelection();
}

static bool FnResortObjects(C4AulContext* cthr, C4String *szFunc, long Category)
	{
	// safety
	if (!szFunc) return FALSE;
	if (!cthr->Caller) return FALSE;
	// default category
	if (!Category) Category=C4D_SortLimit;
	// get function
	C4AulFunc *pFn = cthr->Caller->Func->GetLocalSFunc(FnStringPar(szFunc));
	if (!pFn)
		throw new C4AulExecError(cthr->Obj, FormatString("ResortObjects: Resort function %s not found", FnStringPar(szFunc)).getData());
	// create object resort
	C4ObjResort *pObjRes = new C4ObjResort();
	pObjRes->Category=Category;
	pObjRes->OrderFunc=pFn;
	// insert into game resort proc list
	pObjRes->Next = ::Objects.ResortProc;
	::Objects.ResortProc = pObjRes;
	// success, so far
	return TRUE;
	}

static bool FnResortObject(C4AulContext* cthr, C4String *szFunc, C4Object *pObj)
	{
	// safety
	if (!szFunc) return FALSE;
	if (!cthr->Caller) return FALSE;
	if (!pObj) if (!(pObj=cthr->Obj)) return FALSE;
	// get function
	C4AulFunc *pFn = cthr->Caller->Func->GetLocalSFunc(FnStringPar(szFunc));
	if (!pFn)
		throw new C4AulExecError(cthr->Obj, FormatString("ResortObjects: Resort function %s not found", FnStringPar(szFunc)).getData());
	// create object resort
	C4ObjResort *pObjRes = new C4ObjResort();
	pObjRes->OrderFunc=pFn;
	pObjRes->pSortObj=pObj;
	// insert into game resort proc list
	pObjRes->Next = ::Objects.ResortProc;
	::Objects.ResortProc = pObjRes;
	// success, so far
	return TRUE;
	}

static long FnGetChar(C4AulContext* cthr, C4String *pString, long iIndex)
	{
	const char *szText = FnStringPar(pString);
	if (!szText) return 0;
	// loop and check for end of string
	for (int i=0; i<iIndex; i++, szText++)
		if (!*szText) return 0;
	// return indiced character value
	return (unsigned char) *szText;
	}

static bool FnSetGraphics(C4AulContext *pCtx, C4String *pGfxName, C4Object *pObj, C4ID idSrcGfx, long iOverlayID, long iOverlayMode, C4String *pAction, long dwBlitMode, C4Object *pOverlayObject)
	{
	// safety
	if (!pObj) if (!(pObj=pCtx->Obj)) return FALSE;
	if (!pObj->Status) return FALSE;
	// get def for source graphics
	C4Def *pSrcDef=NULL;
	if (idSrcGfx) if (!(pSrcDef=C4Id2Def(idSrcGfx))) return FALSE;
	// setting overlay?
	if (iOverlayID)
		{
		// any overlays must be positive for now
		if (iOverlayID<0) { Log("SetGraphics: Background overlays not implemented!"); return FALSE; }
		// deleting overlay?
		C4DefGraphics *pGrp;
		if (iOverlayMode == C4GraphicsOverlay::MODE_Object)
			{
			if (!pOverlayObject) return pObj->RemoveGraphicsOverlay(iOverlayID);
			}
		else
			{
			if (!pSrcDef) return pObj->RemoveGraphicsOverlay(iOverlayID);
			pGrp = pSrcDef->Graphics.Get(FnStringPar(pGfxName));
			if (!pGrp) return FALSE;
			}
		// adding/setting
		C4GraphicsOverlay *pOverlay = pObj->GetGraphicsOverlay(iOverlayID, true);
		switch (iOverlayMode)
			{
			case C4GraphicsOverlay::MODE_Base:
				pOverlay->SetAsBase(pGrp, dwBlitMode);
				break;

			case C4GraphicsOverlay::MODE_Action:
				pOverlay->SetAsAction(pGrp, FnStringPar(pAction), dwBlitMode);
				break;

			case C4GraphicsOverlay::MODE_IngamePicture:
				pOverlay->SetAsIngamePicture(pGrp, dwBlitMode);
				break;

			case C4GraphicsOverlay::MODE_Picture:
				pOverlay->SetAsPicture(pGrp, dwBlitMode);
				break;

			case C4GraphicsOverlay::MODE_Object:
				if (pOverlayObject && !pOverlayObject->Status) pOverlayObject = NULL;
				pOverlay->SetAsObject(pOverlayObject, dwBlitMode);
				break;

			case C4GraphicsOverlay::MODE_ExtraGraphics:
				pOverlay->SetAsExtraGraphics(pGrp, dwBlitMode);
				break;

			default:
				DebugLog("SetGraphics: Invalid overlay mode");
				pOverlay->SetAsBase(NULL, 0); // make invalid, so it will be removed
				break;
			}
		// remove if invalid
		if (!pOverlay->IsValid(pObj))
			{
			pObj->RemoveGraphicsOverlay(iOverlayID);
			return FALSE;
			}
		// Okay, valid overlay set!
		return TRUE;
		}
	// no overlay: Base graphics
	// set graphics - pSrcDef==NULL defaults to pObj->pDef
	return pObj->SetGraphics(FnStringPar(pGfxName), pSrcDef);
	}

static long FnGetDefBottom(C4AulContext* cthr, C4Object *pObj)
	{
	if (!pObj) if (!(pObj=cthr->Obj)) return 0;
	return pObj->GetY()+pObj->Def->Shape.y+pObj->Def->Shape.Hgt;
	}

static bool FnSetMaterialColor(C4AulContext* cthr, long iMat, long iClr1R, long iClr1G, long iClr1B, long iClr2R, long iClr2G, long iClr2B, long iClr3R, long iClr3G, long iClr3B)
	{
	// get mat
	if (!MatValid(iMat)) return FALSE;
	C4Material *pMat = &::MaterialMap.Map[iMat];
	// newgfx: emulate by landscape modulation - enlightment not allowed...
	DWORD dwBack, dwMod=GetClrModulation(C4RGB(pMat->Color[0], pMat->Color[1], pMat->Color[2]), C4RGB(iClr1R, iClr1G, iClr1B), dwBack);
	dwMod&=0xffffff;
	if (!dwMod) dwMod=1;
	if (dwMod==0xffffff) dwMod=0;
	::Landscape.SetModulation(dwMod);
	// done
	return TRUE;
	}

static long FnGetMaterialColor(C4AulContext* cthr, long iMat, long iNum, long iChannel)
	{
	// get mat
	if (!MatValid(iMat)) return FALSE;
	C4Material *pMat = &::MaterialMap.Map[iMat];
	// get color
	return pMat->Color[iNum*3+iChannel];
	}

static C4String *FnMaterialName(C4AulContext* cthr, long iMat)
	{
	// mat valid?
	if (!MatValid(iMat)) return NULL;
	// return mat name
	return String(::MaterialMap.Map[iMat].Name);
	}

static bool FnSetMenuSize(C4AulContext* cthr, long iCols, long iRows, C4Object *pObj)
	{
	// get object
	if(!pObj) pObj = cthr->Obj; if(!pObj) return FALSE;
	// get menu
	C4Menu *pMnu=pObj->Menu;
	if (!pMnu || !pMnu->IsActive()) return FALSE;
	pMnu->SetSize(BoundBy<long>(iCols, 0, 50), BoundBy<long>(iRows, 0, 50));
	return TRUE;
	}

char NeededMat[C4MaxMessage];

static C4String *FnGetNeededMatStr(C4AulContext* cthr, C4Object *pObj)
	{
	// local/safety
	if (!pObj) if (!(pObj=cthr->Obj)) return NULL;
	return String(pObj->GetNeededMatStr(cthr->Obj).getData());
	}

static C4Value FnEval(C4AulContext *cthr, C4Value *strScript_C4V)
{
	// execute script in the same object
	enum C4AulScript::Strict Strict = C4AulScript::MAXSTRICT;
	if (cthr->Caller)
		Strict = cthr->Caller->Func->pOrgScript->Strict;
	if (cthr->Obj)
		return cthr->Obj->Def->Script.DirectExec(cthr->Obj, FnStringPar(strScript_C4V->getStr()), "eval", true, Strict);
	else if (cthr->Def)
		return cthr->Def->Script.DirectExec(0, FnStringPar(strScript_C4V->getStr()), "eval", true, Strict);
	else
		return Game.Script.DirectExec(0, FnStringPar(strScript_C4V->getStr()), "eval", true, Strict);
}

static bool FnLocateFunc(C4AulContext *cthr, C4String *funcname, C4Object *pObj, C4ID idDef)
	{
	// safety
	if (!funcname || !funcname->GetCStr())
		{
		Log("No func name");
		return false;
		}
	// determine script context
	C4AulScript *pCheckScript;
	if (pObj)
		{
		pCheckScript = &pObj->Def->Script;
		}
	else if (idDef)
		{
		C4Def *pDef = C4Id2Def(idDef);
		if (!pDef) { Log("Invalid or unloaded def"); return false; }
		pCheckScript = &pDef->Script;
		}
	else
		{
		if (!cthr || !cthr->Caller || !cthr->Caller->Func || !cthr->Caller->Func->Owner)
			{
			Log("No valid script context");
			return false;
			}
		pCheckScript = cthr->Caller->Func->Owner;
		}
	// get function by name
	C4AulFunc *pFunc = pCheckScript->GetFuncRecursive(funcname->GetCStr());
	if (!pFunc)
		{
		LogF("Func %s not found", funcname->GetCStr());
		}
	else
		{
		const char *szPrefix = "";
		while (pFunc)
			{
			C4AulScriptFunc *pSFunc = pFunc->SFunc();
			if (!pSFunc)
				{
				LogF("%s%s (engine)", szPrefix, pFunc->Name);
				}
			else if (!pSFunc->pOrgScript)
				{
				LogF("%s%s (no owner)", szPrefix, pSFunc->Name);
				}
			else
				{
				int32_t iLine = SGetLine(pSFunc->pOrgScript->GetScript(), pSFunc->Script);
				LogF("%s%s (%s:%d)", szPrefix, pFunc->Name, pSFunc->pOrgScript->ScriptName.getData(), (int)iLine);
				}
			// next func in overload chain
			pFunc = pSFunc ? pSFunc->OwnerOverloaded : NULL;
			szPrefix = "overloads ";
			}
		}
	return true;
	}

static C4Value FnVarN(C4AulContext *cthr, C4Value *strName_C4V)
{
	const char *strName = FnStringPar(strName_C4V->getStr());

	if(!cthr->Caller) return C4VNull;

	// find variable
	int32_t iID = cthr->Caller->Func->VarNamed.GetItemNr(strName);
	if(iID < 0)
		return C4VNull;

	// return reference on variable
	return cthr->Caller->Vars[iID].GetRef();
}

static C4Value FnLocalN(C4AulContext* cthr, C4Value* strName_C4V, C4Value* pObj_C4V)
{
	const char* strName = FnStringPar(strName_C4V->getStr());
	C4Object* pObj = pObj_C4V->getObj();

	if(!pObj) pObj = cthr->Obj;
	if(!pObj) return C4Value();

	// find variable
	C4Value* pVarN = pObj->LocalNamed.GetItem(strName);

	if(!pVarN) return C4Value();

	// return reference on variable
	return pVarN->GetRef();
}

static C4Value FnGlobalN(C4AulContext* cthr, C4Value* strName_C4V)
{
	const char* strName = FnStringPar(strName_C4V->getStr());

	// find variable
	C4Value* pVarN = ::ScriptEngine.GlobalNamed.GetItem(strName);

	if(!pVarN) return C4Value();

	// return reference on variable
	return pVarN->GetRef();
}

static bool FnSetSkyAdjust(C4AulContext* cthr, long dwAdjust, long dwBackClr)
	{
	// set adjust
	::Landscape.Sky.SetModulation(dwAdjust, dwBackClr);
	// success
	return TRUE;
	}

static bool FnSetMatAdjust(C4AulContext* cthr, long dwAdjust)
	{
	// set adjust
	::Landscape.SetModulation(dwAdjust);
	// success
	return TRUE;
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

static long FnAnyContainer(C4AulContext*)		{ return ANY_CONTAINER;	}
static long FnNoContainer(C4AulContext*)			{ return NO_CONTAINER;	}

static long FnGetTime(C4AulContext *)
{
	// check network, record, etc
	if (::Control.SyncMode()) return 0;
	return timeGetTime();
}

static long FnGetSystemTime(C4AulContext *cthr, long iWhat)
	{
#ifdef _WIN32
	// check network, record, etc
	if (::Control.SyncMode()) return 0;
	// check bounds
	if (!Inside<long>(iWhat, 0, 7)) return 0;
	SYSTEMTIME time;
	GetLocalTime(&time);
	// return queried value
	return *(((WORD *) &time) + iWhat);
#else
	return 0;
#endif
	}

static C4Value FnSetPlrExtraData(C4AulContext *cthr, C4Value *iPlayer_C4V, C4Value *strDataName_C4V, C4Value *Data)
{
	long iPlayer = iPlayer_C4V->getInt();
	const char *strDataName = FnStringPar(strDataName_C4V->getStr());
	// valid player? (for great nullpointer prevention)
	if(!ValidPlr(iPlayer)) return C4Value();
	// do not allow data type C4V_String or C4V_C4Object
	if(Data->GetType() != C4V_Any &&
		Data->GetType() != C4V_Int &&
		Data->GetType() != C4V_Bool) return C4VNull;
	// get pointer on player...
	C4Player* pPlayer = ::Players.Get(iPlayer);
	// no name list created yet?
	if(!pPlayer->ExtraData.pNames)
		// create name list
		pPlayer->ExtraData.CreateTempNameList();
	// data name already exists?
	long ival;
	if((ival = pPlayer->ExtraData.pNames->GetItemNr(strDataName)) != -1)
		pPlayer->ExtraData[ival] = *Data;
	else
	{
		// add name
		pPlayer->ExtraData.pNames->AddName(strDataName);
		// get val id & set
		if((ival = pPlayer->ExtraData.pNames->GetItemNr(strDataName)) == -1) return C4Value();
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
	if(!ValidPlr(iPlayer)) return C4Value();
	// get pointer on player...
	C4Player* pPlayer = ::Players.Get(iPlayer);
	// no name list?
	if(!pPlayer->ExtraData.pNames) return C4Value();
	long ival;
	if((ival = pPlayer->ExtraData.pNames->GetItemNr(strDataName)) == -1) return C4Value();
	// return data
	return pPlayer->ExtraData[ival];
}

static C4Value FnSetCrewExtraData(C4AulContext *cthr, C4Value *pCrew_C4V, C4Value *strDataName_C4V, C4Value *Data)
{
	C4Object *pCrew = pCrew_C4V->getObj();
	if (!pCrew) pCrew = cthr->Obj;
	const char *strDataName = FnStringPar(strDataName_C4V->getStr());
	// valid crew with info? (for great nullpointer prevention)
	if(!pCrew || !pCrew->Info) return C4Value();
	// do not allow data type C4V_String or C4V_C4Object
	if(Data->GetType() != C4V_Any &&
		Data->GetType() != C4V_Int &&
		Data->GetType() != C4V_Bool) return C4VNull;
	// get pointer on info...
	C4ObjectInfo *pInfo = pCrew->Info;
	// no name list created yet?
	if(!pInfo->ExtraData.pNames)
		// create name list
		pInfo->ExtraData.CreateTempNameList();
	// data name already exists?
	long ival;
	if((ival = pInfo->ExtraData.pNames->GetItemNr(strDataName)) != -1)
		pInfo->ExtraData[ival] = *Data;
	else
	{
		// add name
		pInfo->ExtraData.pNames->AddName(strDataName);
		// get val id & set
		if((ival = pInfo->ExtraData.pNames->GetItemNr(strDataName)) == -1) return C4Value();
		pInfo->ExtraData[ival] = *Data;
	}
	// ok, return the value that has been set
  return *Data;
}

static C4Value FnGetCrewExtraData(C4AulContext *cthr, C4Value *pCrew_C4V, C4Value *strDataName_C4V)
{
	C4Object *pCrew = pCrew_C4V->getObj();
	if (!pCrew) pCrew = cthr->Obj;
	const char *strDataName = FnStringPar(strDataName_C4V->getStr());
	// valid crew with info?
	if(!pCrew || !pCrew->Info) return C4Value();
	// get pointer on info...
	C4ObjectInfo *pInfo = pCrew->Info;
	// no name list?
	if(!pInfo->ExtraData.pNames) return C4Value();
	long ival;
	if((ival = pInfo->ExtraData.pNames->GetItemNr(strDataName)) == -1) return C4Value();
	// return data
	return pInfo->ExtraData[ival];
}

static long FnDrawMatChunks(C4AulContext *cctx, long tx, long ty, long twdt, long thgt, long icntx, long icnty, C4String *strMaterial, C4String *strTexture, bool bIFT)
{
	return ::Landscape.DrawChunks(tx, ty, twdt, thgt, icntx, icnty, FnStringPar(strMaterial), FnStringPar(strTexture), bIFT != 0);
}

static bool FnGetCrewEnabled(C4AulContext *cctx, C4Object *pObj)
	{
	// local/safety
	if (!pObj) pObj=cctx->Obj; if (!pObj) return FALSE;
	// return status
	return !pObj->CrewDisabled;
	}

static bool FnSetCrewEnabled(C4AulContext *cctx, bool fEnabled, C4Object *pObj)
	{
	// local/safety
	if (!pObj) pObj=cctx->Obj; if (!pObj) return FALSE;
	// set status
	pObj->CrewDisabled=!fEnabled;
	// deselect
	if (!fEnabled)
		{
		pObj->Select=FALSE;
		C4Player *pOwner;
		if (pOwner=::Players.Get(pObj->Owner))
			{
			// if viewed player cursor gets deactivated and no new cursor is found, follow the old in target mode
			bool fWasCursorMode = (pOwner->ViewMode == C4PVM_Cursor);
			if (pOwner->Cursor==pObj)
				pOwner->AdjustCursorCommand();
			if (!pOwner->ViewCursor && !pOwner->Cursor && fWasCursorMode)
				pOwner->SetViewMode(C4PVM_Target, pObj);
			}
		}
	// success
	return TRUE;
	}

static bool FnUnselectCrew(C4AulContext *cctx, long iPlayer)
	{
	// get player
	C4Player *pPlr=::Players.Get(iPlayer);
	if (!pPlr) return FALSE;
	// unselect crew
	pPlr->UnselectCrew();
	// success
	return TRUE;
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

struct C4ModLandscapeMatRec
	{
	long iMode, iClr1, iClr2;
	};

DWORD FadeClr(DWORD dwClr1, DWORD dwClr2, BYTE byA1)
	{
	// darken colors and add them
	DarkenClrBy(dwClr1, 255-byA1); DarkenClrBy(dwClr2, byA1);
	return dwClr1+dwClr2;
	}

void SmoothChars(signed char &r1, signed char &r2)
	{
	// lower the difference between two numbers
	signed char d=(r2-r1)/4;
	r1+=d; r2-=d;
	}

static bool FnDrawModLandscape(C4AulContext *cctx, long iX, long iY, long iWdt, long iHgt, C4Object *pObj)
	{
	// hardcoded function for Lhûnrim.c4s - make the map look beautiful
	// safety
	if (!pObj) { pObj=cctx->Obj; if (!pObj) return FALSE; }
	// clip to landscape size

	// ClipRect wants int& instead of long&
	int32_t i_x = iX, i_y = iY, i_w = iWdt, i_h = iHgt;
	if (!::Landscape.ClipRect(i_x, i_y, i_w, i_h)) return FALSE;
	iX = i_x; iY = i_y; iWdt = i_w; iHgt = i_h;

	long iX2=iX+iWdt; long iY2=iY+iHgt;
	// get material modification values
	C4ModLandscapeMatRec *MatInfo = new C4ModLandscapeMatRec[::MaterialMap.Num];
	for (int i=0; i< ::MaterialMap.Num; ++i)
		{
		MatInfo[i].iMode=pObj->Local.GetItem(i*3+0).getInt();
		MatInfo[i].iClr1=pObj->Local.GetItem(i*3+1).getInt();
		MatInfo[i].iClr2=pObj->Local.GetItem(i*3+2).getInt();
		}
	// go through it horizontally and vertically in two directions, building a distance-to-lower-mat-map
	signed char *map = new signed char[iWdt*iHgt]; ZeroMemory(map, iWdt*iHgt);
	signed char *pZ;
	for (int d=0; d<=1; ++d) for (int i=0; i<=1; ++i)
		for (int x1 = d?(i?iX2-1:iY2-1):(i?iX:iY); d?(x1>=(i?iX:iY)):(x1<(i?iX2:iY2)); d?(--x1):(++x1))
			{
			long iLastPlacement=0,z=0; // placement out of map
			for (int x2 = d?(i?iY2-1:iX2-1):(i?iY:iX); d?(x2>=(i?iY:iX)):(x2<(i?iY2:iX2)); d?(--x2):(++x2))
				{
				// get current placement
				long iPlacement = MatPlacement(GBackMat(i?x1:x2, i?x2:x1));
				// compare ot to the given
				if (!iLastPlacement) iLastPlacement=iPlacement;
				if (iLastPlacement == iPlacement)
					{
					// it stayed the same: adjust z
					if (z<0) ++z; else if (z>0) --z;
					}
				else
					{
					// placement changed
					if (iLastPlacement < iPlacement)
						// upwards: count up from zero
						z=-30;
					else
						// downwards: count down
						z=30;
					iLastPlacement=iPlacement;
					}
				// adjust buffer
				if (z)
					{
					pZ=map + (i?x1:x2)-iX + iWdt*((i?x2:x1)-iY);
					*pZ = static_cast<signed char>(BoundBy<long>(*pZ+z, -128, 127));
					}
				}
			}

	// go through the buffer again and smooth it
	if (iWdt>2 && iHgt>2) for (int i=0; i<10; ++i)
		{
		pZ=map+iWdt+1;
		for (int y=iY+1; y<iY2-1; ++y)
			{
			for (int x=iX+1; x<iX2-1; ++x)
				{
				long iPlac1=MatPlacement(GBackMat(x,y));
				if (iPlac1 == MatPlacement(GBackMat(x,y-1))) SmoothChars(*pZ, *(pZ-iWdt));
				if (iPlac1 == MatPlacement(GBackMat(x,y+1))) SmoothChars(*pZ, *(pZ+iWdt));
				if (iPlac1 == MatPlacement(GBackMat(x-1,y))) SmoothChars(*pZ, *(pZ-1));
				if (iPlac1 == MatPlacement(GBackMat(x+1,y))) SmoothChars(*pZ, *(pZ+1));
				++pZ;
				}
			pZ+=2;
			}
		}

	// apply buffer
	//Application.DDraw->DefinePattern(::TextureMap.GetTexture("Smooth2"));
	pZ=map;
	for (int y=iY; y<iY2; ++y)
		for (int x=iX; x<iX2; ++x)
			{
			BYTE byPix=GBackPix(x,y);
			long iMat=PixCol2Mat(byPix);
			if (MatValid(iMat))
				{
				C4ModLandscapeMatRec *pMatI=MatInfo+iMat;
				DWORD dwPix; bool fDrawPix=true;
				switch (pMatI->iMode)
					{
					case 0: // draw nothing
						fDrawPix=false; break;

					case 1: // normal case: Clr1 is bordercolor
						if (*pZ<0)
							{
							if (*pZ<=-16)
								dwPix=pMatI->iClr1;
							else
								dwPix=FadeClr(pMatI->iClr1, pMatI->iClr2, (int)*pZ*-16);
							}
						else
							dwPix=pMatI->iClr2;
						// does the color contain transparency?
						if (dwPix>>24)
							{
							// apply it!
							DWORD dwPixUnder = ::Landscape._GetPixDw(x,y, false);
							BltAlpha(dwPixUnder, dwPix); dwPix=dwPixUnder;
							}
						break;
					default:
						dwPix=0xFF000000; //FIXME: What would be a sane default?
						break;
					}
				if (fDrawPix)
					{
					//dwPix=Application.DDraw->PatternedClr(x,y,dwPix);
					// TODO: ::Landscape.SetPixDw(x,y,dwPix);
					}
				}
			++pZ;
			}
	//Application.DDraw->NoPattern();
	// free map
	delete [] map;
	delete [] MatInfo;
	// success
	return TRUE;
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
	if (!pDef) return FALSE;
	// create
	::Particles.Create(pDef, (float) iX, (float) iY, (float) iXDir/10.0f, (float) iYDir/10.0f, (float) a/10.0f, b, pObj ? (fBack ? &pObj->BackParticles : &pObj->FrontParticles) : NULL, pObj);
	// success, even if not created
	return TRUE;
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
	if (!pDef) return FALSE;
	// cast
	::Particles.Cast(pDef, iAmount, (float) iX, (float) iY, iLevel, (float) a0/10.0f, b0, (float) a1/10.0f, b1, pObj ? (fBack ? &pObj->BackParticles : &pObj->FrontParticles) : NULL, pObj);
	// success, even if not created
	return TRUE;
	}

static bool FnCastParticles(C4AulContext *cthr, C4String *szName, long iAmount, long iLevel, long iX, long iY, long a0, long a1, long b0, long b1, C4Object *pObj)
	{
	return FnCastAParticles(cthr, szName, iAmount, iLevel, iX, iY, a0, a1, b0, b1, pObj, FALSE);
	}

static bool FnCastBackParticles(C4AulContext *cthr, C4String *szName, long iAmount, long iLevel, long iX, long iY, long a0, long a1, long b0, long b1, C4Object *pObj)
	{
	return FnCastAParticles(cthr, szName, iAmount, iLevel, iX, iY, a0, a1, b0, b1, pObj, TRUE);
	}

static bool FnPushParticles(C4AulContext *cthr, C4String *szName, long iAX, long iAY)
	{
	// particle given?
	C4ParticleDef *pDef=NULL;
	if (szName)
		{
		pDef=::Particles.GetDef(FnStringPar(szName));
		if (!pDef) return FALSE;
		}
	// push them
	::Particles.Push(pDef, (float) iAX/10.0f, (float)iAY/10.0f);
	// success
	return TRUE;
	}

static bool FnClearParticles(C4AulContext *cthr, C4String *szName, C4Object *pObj)
	{
	// particle given?
	C4ParticleDef *pDef=NULL;
	if (szName)
		{
		pDef=::Particles.GetDef(FnStringPar(szName));
		if (!pDef) return FALSE;
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
	return TRUE;
	}

static bool FnIsNewgfx(C4AulContext*) { return TRUE; }

#define SkyPar_KEEP -163764

static bool FnSetSkyParallax(C4AulContext* ctx, long iMode, long iParX, long iParY, long iXDir, long iYDir, long iX, long iY)
	{
	// set all parameters that aren't SkyPar_KEEP
	if (iMode != SkyPar_KEEP)
		if (Inside<long>(iMode, 0, 1)) ::Landscape.Sky.ParallaxMode = iMode;
	if (iParX != SkyPar_KEEP && iParX) ::Landscape.Sky.ParX = iParX;
	if (iParY != SkyPar_KEEP && iParY) ::Landscape.Sky.ParY = iParY;
	if (iXDir != SkyPar_KEEP) ::Landscape.Sky.xdir = itofix(iXDir);
	if (iYDir != SkyPar_KEEP) ::Landscape.Sky.ydir = itofix(iYDir);
	if (iX != SkyPar_KEEP) ::Landscape.Sky.x = itofix(iX);
	if (iY != SkyPar_KEEP) ::Landscape.Sky.y = itofix(iY);
	// success
	return TRUE;
	}

static bool FnDoCrewExp(C4AulContext* ctx, long iChange, C4Object *pObj)
	{
	// local call/safety
	if (!pObj) pObj=ctx->Obj; if (!pObj) return FALSE;
	// do exp
	pObj->DoExperience(iChange);
	// success
	return TRUE;
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
	::GraphicsSystem.SetGamma(dwClr1, dwClr2, dwClr3, iRampIndex);
	return TRUE;
	}

static bool FnResetGamma(C4AulContext* ctx, long iRampIndex)
	{
	::GraphicsSystem.SetGamma(0x000000, 0x808080, 0xffffff, iRampIndex);
	return TRUE;
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
	return TRUE;
}

static long FnGetPathLength(C4AulContext* ctx, long iFromX, long iFromY, long iToX, long iToY)
{
	PathInfo PathInfo;
	PathInfo.ilx = iFromX;
	PathInfo.ily = iFromY;
	PathInfo.ilen = 0;
	if(!Game.PathFinder.Find(iFromX, iFromY, iToX, iToY, &SumPathLength, (long) &PathInfo))
		return 0;
	return PathInfo.ilen + Distance(PathInfo.ilx, PathInfo.ily, iToX, iToY);
}

static long FnSetTextureIndex(C4AulContext *ctx, C4String *psMatTex, long iNewIndex, bool fInsert)
	{
	if(!Inside(iNewIndex, 0l, 255l)) return FALSE;
	return ::Landscape.SetTextureIndex(FnStringPar(psMatTex), BYTE(iNewIndex), !!fInsert);
	}

static long FnRemoveUnusedTexMapEntries(C4AulContext *ctx)
  {
  ::Landscape.RemoveUnusedTexMapEntries();
  return TRUE;
  }

static bool FnSetLandscapePixel(C4AulContext* ctx, long iX, long iY, long dwValue)
	{
	// local call
	if (ctx->Obj) { iX+=ctx->Obj->GetX(); iY+=ctx->Obj->GetY(); }
	// set pixel in 32bit-sfc only
	// TODO: ::Landscape.SetPixDw(iX, iY, dwValue);
	// success
	return TRUE;
	}

static bool FnSetObjectOrder(C4AulContext* ctx, C4Object *pObjBeforeOrAfter, C4Object *pSortObj, bool fSortAfter)
	{
	// local call/safety
	if (!pSortObj) pSortObj=ctx->Obj; if (!pSortObj) return FALSE;
	if (!pObjBeforeOrAfter) return FALSE;
	// note that no category check is done, so this call might corrupt the main list!
	// the scripter must be wise enough not to call it for objects with different categories
	// create object resort
	C4ObjResort *pObjRes = new C4ObjResort();
	pObjRes->pSortObj = pSortObj;
	pObjRes->pObjBefore = pObjBeforeOrAfter;
	pObjRes->fSortAfter = fSortAfter;
	// insert into game resort proc list
	pObjRes->Next = ::Objects.ResortProc;
	::Objects.ResortProc = pObjRes;
	// done, success so far
	return TRUE;
	}

static bool FnDrawMaterialQuad(C4AulContext* ctx, C4String *szMaterial, long iX1, long iY1, long iX2, long iY2, long iX3, long iY3, long iX4, long iY4, bool fSub)
	{
  const char *szMat = FnStringPar(szMaterial);
	return !! ::Landscape.DrawQuad(iX1, iY1, iX2, iY2, iX3, iY3, iX4, iY4, szMat, fSub);
	}

static bool FnFightWith(C4AulContext *ctx, C4Object *pTarget, C4Object *pClonk)
	{
	// local call/safety
	if (!pTarget) return FALSE;
	if (!pClonk) if (!(pClonk=ctx->Obj)) return FALSE;
	// check OCF
	if (~(pTarget->OCF & pClonk->OCF) & OCF_FightReady) return FALSE;
	// RejectFight callback
	C4AulParSet parset1(C4VObj(pTarget) );
	C4AulParSet parset2(C4VObj(pClonk) );
	if(pTarget->Call(PSF_RejectFight, &parset1).getBool() ) return FALSE;
	if(pClonk->Call(PSF_RejectFight, &parset2).getBool() ) return FALSE;
	// begin fighting
	ObjectActionFight(pClonk,pTarget);
	ObjectActionFight(pTarget,pClonk);
	// success
	return TRUE;
	}

static bool FnSetFilmView(C4AulContext *ctx, long iToPlr)
	{
	// check player
	if (!ValidPlr(iToPlr) && iToPlr != NO_OWNER) return FALSE;
	// real switch in replays only
	if (!::Control.isReplay()) return TRUE;
	// set new target plr
	if (C4Viewport *vp = ::GraphicsSystem.GetFirstViewport()) vp->Init(iToPlr, true);
	// done, always success (sync)
	return TRUE;
	}

static bool FnClearMenuItems(C4AulContext *ctx, C4Object *pObj)
  {
	// local call/safety
	if (!pObj) pObj=ctx->Obj; if (!pObj) return FALSE;
	// check menu
	if (!pObj->Menu) return FALSE;
	// clear the items
	pObj->Menu->ClearItems(true);
	// success
	return TRUE;
	}

static C4Object *FnGetObjectLayer(C4AulContext *ctx, C4Object *pObj)
	{
	// local call/safety
	if (!pObj) if (!(pObj=ctx->Obj)) return NULL;
	// get layer object
	return pObj->pLayer;
	}

static bool FnSetObjectLayer(C4AulContext *ctx, C4Object *pNewLayer, C4Object *pObj)
	{
	// local call/safety
	if (!pObj) if (!(pObj=ctx->Obj)) return FALSE;
	// set layer object
	pObj->pLayer = pNewLayer;
	// set for all contents as well
	for (C4ObjectLink *pLnk=pObj->Contents.First; pLnk; pLnk=pLnk->Next)
		if ((pObj=pLnk->Obj) && pObj->Status)
			pObj->pLayer = pNewLayer;
	// success
	return TRUE;
	}

static bool FnSetShape(C4AulContext *ctx, long iX, long iY, long iWdt, long iHgt, C4Object *pObj)
	{
	// local call / safety
	if (!pObj) if (!(pObj = ctx->Obj)) return FALSE;
	// update shape
	pObj->Shape.x = iX;
	pObj->Shape.y = iY;
	pObj->Shape.Wdt = iWdt;
	pObj->Shape.Hgt = iHgt;
	// section list needs refresh
	pObj->UpdatePos();
	// done, success
	return TRUE;
	}

static bool FnAddMsgBoardCmd(C4AulContext *ctx, C4String *pstrCommand, C4String *pstrScript, long iRestriction)
	{
	// safety
	if(!pstrCommand || !pstrScript) return FALSE;
	// unrestricted commands cannot be set by direct-exec script (like /script).
	if(iRestriction != C4MessageBoardCommand::C4MSGCMDR_Identifier)
		if(!ctx->Caller || !*ctx->Caller->Func->Name)
			return FALSE;
	C4MessageBoardCommand::Restriction eRestriction;
	switch (iRestriction)
		{
		case C4MessageBoardCommand::C4MSGCMDR_Escaped: eRestriction = C4MessageBoardCommand::C4MSGCMDR_Escaped; break;
		case C4MessageBoardCommand::C4MSGCMDR_Plain: eRestriction = C4MessageBoardCommand::C4MSGCMDR_Plain; break;
		case C4MessageBoardCommand::C4MSGCMDR_Identifier: eRestriction = C4MessageBoardCommand::C4MSGCMDR_Identifier; break;
		default: return FALSE;
		}
	// add command
	::MessageInput.AddCommand(FnStringPar(pstrCommand), FnStringPar(pstrScript), eRestriction);
	return TRUE;
	}

static bool FnSetGameSpeed(C4AulContext *ctx, long iSpeed)
	{
	// safety
	if(iSpeed) if(!Inside<long>(iSpeed, 0, 1000)) return FALSE;
	if(!iSpeed) iSpeed = 38;
	// set speed, restart timer
	Application.SetGameTickDelay(1000 / iSpeed);
	return TRUE;
	}

static bool FnSetObjDrawTransform(C4AulContext *ctx, long iA, long iB, long iC, long iD, long iE, long iF, C4Object *pObj, long iOverlayID)
	{
	// local call / safety
	if (!pObj) { if (!(pObj = ctx->Obj)) return FALSE; }
	C4DrawTransform *pTransform;
	// overlay?
	if (iOverlayID)
		{
		// set overlay transform
		C4GraphicsOverlay *pOverlay = pObj->GetGraphicsOverlay(iOverlayID, false);
		if (!pOverlay) return FALSE;
		pTransform = pOverlay->GetTransform();
		}
	else
		{
		// set base transform
		pTransform = pObj->pDrawTransform;
		// del transform?
		if (!iB && !iC && !iD && !iF && iA==iE && (!iA || iA==1000))
			{
			// identity/0 and no transform defined: nothing to do
			if (!pTransform) return TRUE;
			// transform has no flipdir?
			if (pTransform->FlipDir == 1)
				{
				// kill identity-transform, then
				delete pTransform;
				pObj->pDrawTransform=NULL;
				return TRUE;
				}
			// flipdir must remain: set identity
			pTransform->Set(1,0,0,0,1,0,0,0,1);
			return TRUE;
			}
		// create draw transform if not already present
		if (!pTransform) pTransform = pObj->pDrawTransform = new C4DrawTransform();
		}
	// assign values
	pTransform->Set((float) iA/1000, (float) iB/1000, (float) iC/1000, (float) iD/1000, (float) iE/1000, (float) iF/1000, 0, 0, 1);
	// done, success
	return TRUE;
	}

static bool FnSetObjDrawTransform2(C4AulContext *ctx, long iA, long iB, long iC, long iD, long iE, long iF, long iG, long iH, long iI, long iOverlayID)
	{
	// local call / safety
	C4Object * pObj = ctx->Obj;
	if (!pObj) return FALSE;
	C4DrawTransform *pTransform;
	// overlay?
	if (iOverlayID)
		{
		// set overlay transform
		C4GraphicsOverlay *pOverlay = pObj->GetGraphicsOverlay(iOverlayID, false);
		if (!pOverlay) return FALSE;
		pTransform = pOverlay->GetTransform();
		}
	else
		{
		// set base transform
		pTransform = pObj->pDrawTransform;
		// create draw transform if not already present
		if (!pTransform) pTransform = pObj->pDrawTransform = new C4DrawTransform(1);
		}
	// assign values
#define L2F(l) ((float)l/1000)
	CBltTransform matrix;
	matrix.Set(L2F(iA), L2F(iB), L2F(iC), L2F(iD), L2F(iE), L2F(iF), L2F(iG), L2F(iH), L2F(iI));
	*pTransform *= matrix;
#undef L2F
	// done, success
	return TRUE;
	}

#define COPY_C4V_PAR(Var, Par, ParType, Std) \
	Var = (Par && Par->GetType() == ParType \
         ? Par->GetData().Int \
				 : Std)

bool SimFlight(FIXED &x, FIXED &y, FIXED &xdir, FIXED &ydir, int32_t iDensityMin, int32_t iDensityMax, int32_t iIter);

static C4Value FnSimFlight(C4AulContext *ctx, C4Value *pvrX, C4Value *pvrY, C4Value *pvrXDir, C4Value *pvrYDir, C4Value *pviDensityMin, C4Value *pviDensityMax, C4Value *pviIter, C4Value *pviPrec)
	{
	// check and copy parameters
	if(!pvrX || !pvrY || !pvrXDir || !pvrYDir) return C4VBool(false);

	COPY_C4V_PAR(int iDensityMin, pviDensityMin, C4V_Int, C4M_Solid);
	COPY_C4V_PAR(int iDensityMax, pviDensityMax, C4V_Int, 100);
	COPY_C4V_PAR(int iIter, pviIter, C4V_Int, -1);
	COPY_C4V_PAR(int iPrec, pviPrec, C4V_Int, 10);

	// convert to FIXED
	FIXED x = itofix(pvrX->GetData().Int), y = itofix(pvrY->GetData().Int),
		    xdir = itofix(pvrXDir->GetData().Int, iPrec), ydir = itofix(pvrYDir->GetData().Int, iPrec);

	// simulate
	if(!SimFlight(x, y, xdir, ydir, iDensityMin, iDensityMax, iIter))
		return C4VBool(false);

	// write results back
	*pvrX = C4VInt(fixtoi(x)); *pvrY = C4VInt(fixtoi(y));
	*pvrXDir = C4VInt(fixtoi(xdir * iPrec)); *pvrYDir = C4VInt(fixtoi(ydir * iPrec));

	return C4VBool(true);
	}
#undef COPY_C4V_PAR
static bool FnSetPortrait(C4AulContext *ctx, C4String *pstrPortrait, C4Object *pTarget, C4ID idSourceDef, bool fPermanent, bool fCopyGfx)
	{
	// safety
	const char *szPortrait;
	if (!pstrPortrait || !*(szPortrait=FnStringPar(pstrPortrait))) return FALSE;
	if (!pTarget) if (!(pTarget=ctx->Obj)) return FALSE;
	if (!pTarget->Status || !pTarget->Info) return FALSE;
	// special case: clear portrait
	if (SEqual(szPortrait, C4Portrait_None)) return pTarget->Info->ClearPortrait(!!fPermanent);
	// get source def for portrait
	C4Def *pSourceDef;
	if (idSourceDef) pSourceDef = ::Definitions.ID2Def(idSourceDef); else pSourceDef=pTarget->Def;
	if (!pSourceDef) return FALSE;
	// special case: random portrait
	if (SEqual(szPortrait, C4Portrait_Random)) return pTarget->Info->SetRandomPortrait(pSourceDef->id, !!fPermanent, !!fCopyGfx);
	// try to set portrait
	return pTarget->Info->SetPortrait(szPortrait, pSourceDef, !!fPermanent, !!fCopyGfx);
	}

static C4Value FnGetPortrait(C4AulContext *ctx, C4Value *pvpObj, C4Value *pvfGetID, C4Value *pvfGetPermanent)
	{
	// get parameters
	C4Object *pObj = pvpObj->getObj(); bool fGetID = pvfGetID->getBool(); bool fGetPermanent = pvfGetPermanent->getBool();
	// check valid object with info section
	if (!pObj) if (!(pObj = ctx->Obj)) return C4Value();
	if (!pObj->Status || !pObj->Info) return C4Value();
	// get portrait to examine
	C4Portrait *pPortrait;
	if (fGetPermanent)
		{
		// permanent: new portrait assigned?
		if (!(pPortrait=pObj->Info->pNewPortrait))
			{
			// custom portrait?
			if (pObj->Info->pCustomPortrait)
				if (fGetID) return C4Value();
			else
				return C4VString(C4Portrait_Custom);
			// portrait string from info?
			const char *szPortrait = pObj->Info->PortraitFile;
			// no portrait string: portrait undefined ("none" would mean no portrait)
			if (!*szPortrait) return C4Value();
			// evaluate portrait string
			C4ID idPortraitSource=0;
			szPortrait = C4Portrait::EvaluatePortraitString(szPortrait, idPortraitSource, pObj->Info->id, NULL);
			// return desired value
			if (fGetID)
				return idPortraitSource ? C4VID(idPortraitSource) : C4Value();
			else
				return szPortrait ? C4VString(szPortrait) : C4Value();
			}
		}
	else
		// get current portrait
		pPortrait = &(pObj->Info->Portrait);
	// get portrait graphics
	C4DefGraphics *pPortraitGfx = pPortrait->GetGfx();
	// no portrait?
	if (!pPortraitGfx) return C4Value();
	// get def or name
	if (fGetID)
		return (pPortraitGfx->pDef ? C4VID(pPortraitGfx->pDef->id) : C4Value());
	else
		{
		const char *szPortraitName = pPortraitGfx->GetName();
		return C4VString(szPortraitName ? szPortraitName : C4Portrait_Custom);
		}
	}

static long FnLoadScenarioSection(C4AulContext *ctx, C4String *pstrSection, long dwFlags)
	{
	// safety
	const char *szSection;
	if (!pstrSection || !*(szSection=FnStringPar(pstrSection))) return FALSE;
	// try to load it
	return Game.LoadScenarioSection(szSection, dwFlags);
	}

static bool FnSetObjectStatus(C4AulContext *ctx, long iNewStatus, C4Object *pObj, bool fClearPointers)
	{
	// local call / safety
	if (!pObj) { if (!(pObj = ctx->Obj)) return FALSE; }
	if (!pObj->Status) return FALSE;
	// no change
	if (pObj->Status == iNewStatus) return TRUE;
	// set new status
	switch (iNewStatus)
		{
		case C4OS_NORMAL: return pObj->StatusActivate(); break;
		case C4OS_INACTIVE: return pObj->StatusDeactivate(!!fClearPointers); break;
		default: return FALSE; // status unknown
		}
	}

static long FnGetObjectStatus(C4AulContext *ctx, C4Object *pObj)
	{
	// local call / safety
	if (!pObj) { if (!(pObj = ctx->Obj)) return FALSE; }
	return pObj->Status;
	}

static bool FnAdjustWalkRotation(C4AulContext *ctx, long iRangeX, long iRangeY, long iSpeed, C4Object *pObj)
	{
	// local call / safety
	if (!pObj) { if (!(pObj = ctx->Obj)) return FALSE; }
	// must be rotateable and attached to solid ground
	if (!pObj->Def->Rotateable || ~pObj->Action.t_attach&CNAT_Bottom || pObj->Shape.AttachMat == MNone)
		return FALSE;
	// adjust rotation
	return pObj->AdjustWalkRotation(iRangeX, iRangeY, iSpeed);
	}

static C4Value FnAddEffect_C4V(C4AulContext *ctx, C4Value *pvsEffectName, C4Value *pvpTarget, C4Value *pviPrio, C4Value *pviTimerIntervall, C4Value *pvpCmdTarget, C4Value *pvidCmdTarget, C4Value *pvVal1, C4Value *pvVal2, C4Value *pvVal3, C4Value *pvVal4)
	{
	// evaluate parameters
	C4String *psEffectName = pvsEffectName->getStr();
	C4Object *pTarget = pvpTarget->getObj();
	long iPrio = pviPrio->getInt(), iTimerIntervall = pviTimerIntervall->getInt();
	C4Object *pCmdTarget = pvpCmdTarget->getObj();
	C4ID idCmdTarget = pvidCmdTarget->getC4ID();
	const char *szEffect = FnStringPar(psEffectName);
	// safety
 	if (pTarget && !pTarget->Status) return C4Value();
	if (!szEffect || !*szEffect || !iPrio) return C4Value();
	// create effect
	int32_t iEffectNumber;
	new C4Effect(pTarget, szEffect, iPrio, iTimerIntervall, pCmdTarget, idCmdTarget, *pvVal1, *pvVal2, *pvVal3, *pvVal4, true, iEffectNumber);
	// return assigned effect number - may be 0 if he effect has been denied by another effect
	// may also be the number of another effect
	return C4VInt(iEffectNumber);
	}

static C4Value FnGetEffect_C4V(C4AulContext *ctx, C4Value *pvsEffectName, C4Value *pvpTarget, C4Value *pviIndex, C4Value *pviQueryValue, C4Value *pviMaxPriority)
	{
	// evaluate parameters
	C4String *psEffectName = pvsEffectName->getStr();
	C4Object *pTarget = pvpTarget->getObj();
	long iIndex = pviIndex->getInt(), iQueryValue = pviQueryValue->getInt(), iMaxPriority = pviMaxPriority->getInt();
	const char *szEffect = FnStringPar(psEffectName);
	// get effects
	C4Effect *pEffect = pTarget ? pTarget->pEffects : Game.pGlobalEffects;
	if (!pEffect) return C4Value();
	// name/wildcard given: find effect by name and index
	if (szEffect && *szEffect)
		pEffect = pEffect->Get(szEffect, iIndex, iMaxPriority);
	else
		// otherwise, get by number
		pEffect = pEffect->Get(iIndex, true, iMaxPriority);
	// effect found?
	if (!pEffect) return C4Value();
	// evaluate desired value
	switch (iQueryValue)
		{
		case 0: return C4VInt(pEffect->iNumber);        // 0: number
		case 1: return C4VString(pEffect->Name);				// 1: name
		case 2: return C4VInt(Abs(pEffect->iPriority)); // 2: priority (may be negative for deactivated effects)
		case 3: return C4VInt(pEffect->iIntervall);     // 3: timer intervall
		case 4: return C4VObj(pEffect->pCommandTarget); // 4: command target
		case 5: return C4VID(pEffect->idCommandTarget); // 5: command target ID
		case 6: return C4VInt(pEffect->iTime);          // 6: effect time
		}
	// invalid data queried
	return C4Value();
	}

static bool FnRemoveEffect(C4AulContext *ctx, C4String *psEffectName, C4Object *pTarget, long iIndex, bool fDoNoCalls)
	{
	// evaluate parameters
	const char *szEffect = FnStringPar(psEffectName);
	// get effects
	C4Effect *pEffect = pTarget ? pTarget->pEffects : Game.pGlobalEffects;
	if (!pEffect) return 0;
	// name/wildcard given: find effect by name and index
	if (szEffect && *szEffect)
		pEffect = pEffect->Get(szEffect, iIndex);
	else
		// otherwise, get by number
		pEffect = pEffect->Get(iIndex, false);
	// effect found?
	if (!pEffect) return 0;
	// kill it
	if (fDoNoCalls)
		pEffect->SetDead();
	else
		pEffect->Kill(pTarget);
	// done, success
	return TRUE;
	}

static bool FnChangeEffect(C4AulContext *ctx, C4String *psEffectName, C4Object *pTarget, long iIndex, C4String *psNewEffectName, long iNewTimer)
	{
	// evaluate parameters
	const char *szEffect = FnStringPar(psEffectName);
	const char *szNewEffect = FnStringPar(psNewEffectName);
	if (!szNewEffect || !*szNewEffect) return FALSE;
	// get effects
	C4Effect *pEffect = pTarget ? pTarget->pEffects : Game.pGlobalEffects;
	if (!pEffect) return FALSE;
	// name/wildcard given: find effect by name and index
	if (szEffect && *szEffect)
		pEffect = pEffect->Get(szEffect, iIndex);
	else
		// otherwise, get by number
		pEffect = pEffect->Get(iIndex, false);
	// effect found?
	if (!pEffect) return FALSE;
	// set new name
	SCopy(szNewEffect, pEffect->Name, C4MaxName);
	pEffect->ReAssignCallbackFunctions();
	// set new timer
	if (iNewTimer>=0)
		{
		pEffect->iIntervall = iNewTimer;
		pEffect->iTime = 0;
		}
	// done, success
	return TRUE;
	}

static C4Value FnCheckEffect_C4V(C4AulContext *ctx, C4Value *pvsEffectName, C4Value *pvpTarget, C4Value *pviPrio, C4Value *pviTimerIntervall, C4Value *pvVal1, C4Value *pvVal2, C4Value *pvVal3, C4Value *pvVal4)
	{
	// evaluate parameters
	C4String *psEffectName = pvsEffectName->getStr();
	C4Object *pTarget = pvpTarget->getObj();
	long iPrio = pviPrio->getInt(), iTimerIntervall = pviTimerIntervall->getInt();
	const char *szEffect = FnStringPar(psEffectName);
	// safety
	if (pTarget && !pTarget->Status) return C4Value();
	if (!szEffect || !*szEffect) return C4Value();
	// get effects
	C4Effect *pEffect = pTarget ? pTarget->pEffects : Game.pGlobalEffects;
	if (!pEffect) return C4Value();
	// let them check
	return C4VInt(pEffect->Check(pTarget, szEffect, iPrio, iTimerIntervall, *pvVal1, *pvVal2, *pvVal3, *pvVal4));
	}

static long FnGetEffectCount(C4AulContext *ctx, C4String *psEffectName, C4Object *pTarget, long iMaxPriority)
	{
	// evaluate parameters
	const char *szEffect = FnStringPar(psEffectName);
	// get effects
	C4Effect *pEffect = pTarget ? pTarget->pEffects : Game.pGlobalEffects;
	if (!pEffect) return FALSE;
	// count effects
	if (!*szEffect) szEffect = 0;
	return pEffect->GetCount(szEffect, iMaxPriority);
	}

static C4Value FnEffectVar_C4V(C4AulContext *cthr, C4Value *pviVarIndex, C4Value *pvpObj, C4Value *pviEffectNumber)
  {
	// get parameters
	C4Object *pObj = pvpObj->getObj();
	long iVarIndex = pviVarIndex->getInt(), iEffectNumber = pviEffectNumber->getInt();
	// safety
	if (iVarIndex<0) return C4Value();
	// get effect
	C4Effect *pEffect = pObj ? pObj->pEffects : Game.pGlobalEffects;
	if (!pEffect) return C4Value();
	if (!(pEffect = pEffect->Get(iEffectNumber, true))) return C4Value();
	// return ref to var
  return pEffect->EffectVars[iVarIndex].GetRef();
  }

static C4Value FnEffectCall_C4V(C4AulContext *ctx, C4Value *pvpTarget, C4Value *pviNumber, C4Value *pvsCallFn, C4Value *pvVal1, C4Value *pvVal2, C4Value *pvVal3, C4Value *pvVal4, C4Value *pvVal5, C4Value *pvVal6, C4Value *pvVal7)
	{
	// evaluate parameters
	C4String *psCallFn = pvsCallFn->getStr();
	C4Object *pTarget = pvpTarget->getObj();
	long iNumber =  pviNumber->getInt();
	const char *szCallFn = FnStringPar(psCallFn);
	// safety
	if (pTarget && !pTarget->Status) return C4Value();
	if (!szCallFn || !*szCallFn) return C4Value();
	// get effect
	C4Effect *pEffect = pTarget ? pTarget->pEffects : Game.pGlobalEffects;
	if (!pEffect) return C4Value();
	if (!(pEffect = pEffect->Get(iNumber, true))) return C4Value();
	// do call
	return pEffect->DoCall(pTarget, szCallFn, *pvVal1, *pvVal2, *pvVal3, *pvVal4, *pvVal5, *pvVal6, *pvVal7);
	}

static long FnModulateColor(C4AulContext *cthr, long iClr1, long iClr2)
	{
	DWORD dwClr1 = iClr1;
	DWORD dwClr2 = iClr2;
	// default color
	if (!dwClr1) dwClr1 = 0xffffff;
	// get alpha
	long iA1=dwClr1>>24, iA2=dwClr2>>24;
	// modulate color values; mod alpha upwards
	DWORD r = ((dwClr1     & 0xff) * (dwClr2    &   0xff))    >>  8   | // blue
	       ((dwClr1>> 8 & 0xff) * (dwClr2>>8 &   0xff)) &   0xff00 | // green
				 ((dwClr1>>16 & 0xff) * (dwClr2>>8 & 0xff00)) & 0xff0000 | // red
				 Min<long>(iA1+iA2 - ((iA1*iA2)>>8), 255)              << 24   ; // alpha
	return r;
	}

static long FnWildcardMatch(C4AulContext *ctx, C4String *psString, C4String *psWildcard)
	{
	return SWildcardMatchEx(FnStringPar(psString), FnStringPar(psWildcard));
	}

static long FnGetContact(C4AulContext *ctx, C4Object *pObj, long iVertex, long dwCheck)
	{
	// local call / safety
	if (!pObj) if (!(pObj = ctx->Obj)) return 0;
	// vertex not specified: check all
	if (iVertex == -1)
		{
		long iResult = 0;
		for (int i=0; i<pObj->Shape.VtxNum; ++i)
			iResult |= pObj->Shape.GetVertexContact(i, dwCheck, pObj->GetX(), pObj->GetY());
		return iResult;
		}
	// vertex specified: check it
	if (!Inside<long>(iVertex, 0, pObj->Shape.VtxNum-1)) return 0;
	return pObj->Shape.GetVertexContact(iVertex, dwCheck, pObj->GetX(), pObj->GetY());
	}

static long FnSetObjectBlitMode(C4AulContext *ctx, long dwNewBlitMode, C4Object *pObj, long iOverlayID)
	{
	// local call / safety
	if (!pObj) if (!(pObj = ctx->Obj)) return 0;
	// overlay?
	if (iOverlayID)
		{
		C4GraphicsOverlay *pOverlay = pObj->GetGraphicsOverlay(iOverlayID, false);
		if (!pOverlay)
			{
			DebugLogF("SetObjectBlitMode: Overlay %d not defined for object %d (%s)", (int) iOverlayID, (int) pObj->Number, (const char *) pObj->GetName());
			return FALSE;
			}
		pOverlay->SetBlitMode(dwNewBlitMode);
		return TRUE;
		}
	// get prev blit mode
	DWORD dwPrevMode = pObj->BlitMode;
	// iNewBlitMode = 0: reset to definition default
	if (!dwNewBlitMode)
		pObj->BlitMode = pObj->Def->BlitMode;
	else
		// otherwise, set the desired value
		// also ensure that the custom flag is set
		pObj->BlitMode = dwNewBlitMode | C4GFXBLIT_CUSTOM;
	// return previous value
	return dwPrevMode;
	}

static long FnGetObjectBlitMode(C4AulContext *ctx, C4Object *pObj, long iOverlayID)
	{
	// local call / safety
	if (!pObj) if (!(pObj = ctx->Obj)) return 0;
	// overlay?
	if (iOverlayID)
		{
		C4GraphicsOverlay *pOverlay = pObj->GetGraphicsOverlay(iOverlayID, false);
		if (!pOverlay)
			{
			DebugLogF("SetObjectBlitMode: Overlay %d not defined for object %d (%s)", (int) iOverlayID, (int) pObj->Number, (const char *) pObj->GetName());
			return 0;
			}
		return pOverlay->GetBlitMode();
		}
	// get blitting mode
	return pObj->BlitMode;
	}

static bool FnSetViewOffset(C4AulContext *ctx, long iPlayer, long iX, long iY)
  {
  if(!ValidPlr(iPlayer)) return 0;
  // get player viewport
  C4Viewport *pView = ::GraphicsSystem.GetViewport(iPlayer);
  if(!pView) return 1; // sync safety
  // set
  pView->ViewOffsX = iX;
  pView->ViewOffsY = iY;
  // ok
  return 1;
  }

static bool FnSetPreSend(C4AulContext *cthr, long iToVal, C4String *pNewName)
  {
	if (!::Control.isNetwork()) return TRUE;
	// dbg: manual presend
	const char *szClient = FnStringPar(pNewName);
	if (!szClient || !*szClient || WildcardMatch(szClient, Game.Clients.getLocalName()))
		{
		::Control.Network.setTargetFPS(iToVal);
		::GraphicsSystem.FlashMessage(FormatString("TargetFPS: %ld", iToVal).getData());
		}
	return TRUE;
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
		if (!!Game.Script.GRBroadcast(PSF_RejectTeamSwitch, &C4AulParSet(C4VInt(iPlayer), C4VInt(idNewTeam)), true, true))
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
		Game.Script.GRBroadcast(PSF_OnTeamSwitch, &C4AulParSet(C4VInt(iPlayer), C4VInt(idNewTeam), C4VInt(idOldTeam)), true);
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

static bool FnOnOwnerRemoved(C4AulContext *cthr)
	{
	// safety
	C4Object *pObj = cthr->Obj; if (!pObj) return false;
	C4Player *pPlr = ::Players.Get(pObj->Owner); if (!pPlr) return false;
	if (pPlr->Crew.IsContained(pObj))
		{
		// crew members: Those are removed later (AFTER the player has been removed, for backwards compatiblity with relaunch scripting)
		}
	else if ((~pObj->Category & C4D_StaticBack) || (pObj->id == C4ID_Flag))
		{
		// Regular objects: Try to find a new, suitable owner from the same team
		// Ignore StaticBack, because this would not be backwards compatible with many internal objects such as team account
		// Do not ignore flags which might be StaticBack if being attached to castle parts
		int32_t iNewOwner = NO_OWNER;
		C4Team *pTeam;
		if (pPlr->Team) if (pTeam = Game.Teams.GetTeamByID(pPlr->Team))
			{
			for (int32_t i=0; i<pTeam->GetPlayerCount(); ++i)
				{
				int32_t iPlrID = pTeam->GetIndexedPlayer(i);
				if (iPlrID && iPlrID != pPlr->ID)
					{
					C4PlayerInfo *pPlrInfo = Game.PlayerInfos.GetPlayerInfoByID(iPlrID);
					if (pPlrInfo) if (pPlrInfo->IsJoined())
						{
						// this looks like a good new owner
						iNewOwner = pPlrInfo->GetInGameNumber();
						break;
						}
					}
				}
			}
		// if noone from the same team was found, try to find another non-hostile player
		// (necessary for cooperative rounds without teams)
		if (iNewOwner == NO_OWNER)
			for (C4Player *pOtherPlr = ::Players.First; pOtherPlr; pOtherPlr = pOtherPlr->Next)
				if (pOtherPlr != pPlr) if (!pOtherPlr->Eliminated)
					if (!::Players.Hostile(pOtherPlr->Number, pPlr->Number))
						iNewOwner = pOtherPlr->Number;

		// set this owner
		pObj->SetOwner(iNewOwner);
		}
	return true;
	}

static bool FnSetScoreboardData(C4AulContext *cthr, long iRowID, long iColID, C4String *pText, long iData)
	{
	Game.Scoreboard.SetCell(iColID, iRowID, pText ? pText->GetCStr() : NULL, iData);
	return TRUE;
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
		if (!(pPlr = ::Players.Get(iForPlr-1))) return FALSE;
		if (!pPlr->LocalControl) return TRUE;
		}
	Game.Scoreboard.DoDlgShow(iChange, false);
	return TRUE; //Game.Scoreboard.ShouldBeShown();
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

static long FnGetUnusedOverlayID(C4AulContext *ctx, long iBaseIndex, C4Object *pObj)
	{
	// local call / safety
	if (!iBaseIndex) return 0;
	if (!pObj) if (!(pObj = ctx->Obj)) return 0;
	// find search first unused index from there on
	int iSearchDir = (iBaseIndex < 0) ? -1 : 1;
	while (pObj->GetGraphicsOverlay(iBaseIndex, false)) iBaseIndex += iSearchDir;
	return iBaseIndex;
	}

static long FnActivateGameGoalMenu(C4AulContext *ctx, long iPlayer)
	{
	// get target player
	C4Player *pPlr = ::Players.Get(iPlayer);
	if (!pPlr) return FALSE;
	// open menu
	return pPlr->Menu.ActivateGoals(pPlr->Number, pPlr->LocalControl && !::Control.isReplay());
	}

static bool FnFatalError(C4AulContext *ctx, C4String *pErrorMsg)
	{
	throw new C4AulExecError(ctx->Obj, FormatString("User error: %s", pErrorMsg ? pErrorMsg->GetCStr() : "(no error)").getData());
	}

static bool FnPlayVideo(C4AulContext *ctx, C4String *pFilename)
	{
	// filename must be valid
	if (!pFilename || !pFilename->GetCStr()) return false;
	// play it!
	return Game.VideoPlayer.PlayVideo(pFilename->GetCStr());
	}

static bool FnStartCallTrace(C4AulContext *ctx)
	{
	extern void C4AulStartTrace();
	C4AulStartTrace();
	return true;
	}

static bool FnStartScriptProfiler(C4AulContext *ctx, C4ID idScript)
	{
	// get script to profile
	C4AulScript *pScript;
	if (idScript)
		{
		C4Def *pDef = C4Id2Def(idScript);
		if (!pDef) return false;
		pScript = &pDef->Script;
		}
	else
		pScript = &::ScriptEngine;
	// profile it
	C4AulProfiler::StartProfiling(pScript);
	return true;
	}

static bool FnStopScriptProfiler(C4AulContext *ctx)
	{
	C4AulProfiler::StopProfiling();
	return true;
	}

static bool FnCustomMessage(C4AulContext *ctx, C4String *pMsg, C4Object *pObj, long iOwner, long iOffX, long iOffY, long dwClr, C4ID idDeco, C4String *sPortrait, long dwFlags, long iHSize)
	{
	// safeties
	if (!pMsg) return false;
	if (pObj && !pObj->Status) return false;
	const char *szMsg = pMsg->GetCStr();
	if (!szMsg) return false;
	if (idDeco && !C4Id2Def(idDeco)) return false;
	// only one positioning flag per direction allowed
	uint32_t hpos = dwFlags & (C4GM_Left | C4GM_HCenter | C4GM_Right);
	uint32_t vpos = dwFlags & (C4GM_Top | C4GM_VCenter | C4GM_Bottom);
	if (((hpos | hpos-1) + 1)>>1 != hpos)
		{
		throw new C4AulExecError(ctx->Obj, "CustomMessage: Only one horizontal positioning flag allowed!");
		}
	if (((vpos | vpos-1) + 1)>>1 != vpos)
		{
		throw new C4AulExecError(ctx->Obj, "CustomMessage: Only one vertical positioning flag allowed!");
		}
	// message color
	if (!dwClr) dwClr = 0xffffff;
	dwClr = InvertRGBAAlpha(dwClr);
	// message type
	int32_t iType;
	if (pObj)
		if (iOwner != NO_OWNER)
			iType = C4GM_TargetPlayer;
		else
			iType = C4GM_Target;
	else
		if (iOwner != NO_OWNER)
			iType = C4GM_GlobalPlayer;
		else
			iType = C4GM_Global;
	// remove speech?
	StdStrBuf sMsg;
	sMsg.Ref(szMsg);
	if (dwFlags & C4GM_DropSpeech) sMsg.SplitAtChar('$', NULL);
	// create it!
	return ::Messages.New(iType,sMsg,pObj,iOwner,iOffX,iOffY,(uint32_t)dwClr, idDeco, sPortrait ? sPortrait->GetCStr() : NULL, dwFlags, iHSize);
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

//=========================== C4Script Function Map ===================================

// defined function class
class C4AulDefFuncHelper: public C4AulFunc
	{
	public:
		C4AulDefFuncHelper(C4AulScript *pOwner, const char *pName, bool Public,
			C4V_Type pt0 = C4V_Any, C4V_Type pt1 = C4V_Any, C4V_Type pt2 = C4V_Any, C4V_Type pt3 = C4V_Any, C4V_Type pt4 = C4V_Any,
			C4V_Type pt5 = C4V_Any, C4V_Type pt6 = C4V_Any, C4V_Type pt7 = C4V_Any, C4V_Type pt8 = C4V_Any, C4V_Type pt9 = C4V_Any):
			C4AulFunc(pOwner, pName),
			Public(Public)
			{
			ParType[0] = pt0;
			ParType[1] = pt1;
			ParType[2] = pt2;
			ParType[3] = pt3;
			ParType[4] = pt4;
			ParType[5] = pt5;
			ParType[6] = pt6;
			ParType[7] = pt7;
			ParType[8] = pt8;
			ParType[9] = pt9;
			}
		virtual C4V_Type* GetParType() { return ParType; }
		virtual bool GetPublic() { return Public; }
	protected:
		C4V_Type ParType[10];// type of the parameters
		bool Public;
	};

// A macro to create lists with some helper macros
// LIST(2, foo) would create ", foo(0), foo(1)
// TEXT can be a macro
#define LIST(N, TEXT) LIST##N(TEXT)
// The lists are used in a context where a leading comma is needed when the list is not empty
#define LIST0(TEXT)
#define LIST1(TEXT) LIST0(TEXT), TEXT(0)
#define LIST2(TEXT) LIST1(TEXT), TEXT(1)
#define LIST3(TEXT) LIST2(TEXT), TEXT(2)
#define LIST4(TEXT) LIST3(TEXT), TEXT(3)
#define LIST5(TEXT) LIST4(TEXT), TEXT(4)
#define LIST6(TEXT) LIST5(TEXT), TEXT(5)
#define LIST7(TEXT) LIST6(TEXT), TEXT(6)
#define LIST8(TEXT) LIST7(TEXT), TEXT(7)
#define LIST9(TEXT) LIST8(TEXT), TEXT(8)
#define LIST10(TEXT) LIST9(TEXT), TEXT(9)

// Macros which are passed to LIST
#define TYPENAMES(N) typename Par##N##_t
#define PARS(N) Par##N##_t
#define CONV_TYPE(N) C4ValueConv<Par##N##_t>::Type()
#define CONV_FROM_C4V(N) C4ValueConv<Par##N##_t>::_FromC4V(pPars[N])
// N is the number of parameters pFunc needs. Templates can only have a fixed number of arguments,
// so eleven templates are needed
#define TEMPLATE(N)                           \
template <typename RType LIST(N, TYPENAMES)> \
class C4AulDefFunc##N:                        \
public C4AulDefFuncHelper {                   \
	public:                                     \
/* A pointer to the function which this class wraps */ \
		typedef RType (*Func)(C4AulContext * LIST(N, PARS)); \
		virtual int GetParCount() { return N; }   \
		virtual C4V_Type GetRetType()             \
		{ return C4ValueConv<RType>::Type(); }    \
/* Constructor, using the base class to create the ParType array */ \
		C4AulDefFunc##N(C4AulScript *pOwner, const char *pName, Func pFunc, bool Public): \
			C4AulDefFuncHelper(pOwner, pName, Public LIST(N, CONV_TYPE)), pFunc(pFunc) { } \
/* Extracts the parameters from C4Values and wraps the return value in a C4Value */ \
		C4Value Exec(C4AulContext *pContext, C4Value pPars[], bool fPassErrors=false) \
		{ return C4ValueConv<RType>::ToC4V(pFunc(pContext LIST(N, CONV_FROM_C4V))); } \
	protected:                                  \
		Func pFunc;                               \
	};                                          \
template <typename RType LIST(N, TYPENAMES)> \
static void AddFunc(C4AulScript * pOwner, const char * Name, RType (*pFunc)(C4AulContext * LIST(N, PARS)), bool Public=true) \
	{ \
	new C4AulDefFunc##N<RType LIST(N, PARS)>(pOwner, Name, pFunc, Public); \
	}

TEMPLATE(0)
TEMPLATE(1)
TEMPLATE(2)
TEMPLATE(3)
TEMPLATE(4)
TEMPLATE(5)
TEMPLATE(6)
TEMPLATE(7)
TEMPLATE(8)
TEMPLATE(9)
TEMPLATE(10)


#undef LIST
#undef LIST0
#undef LIST1
#undef LIST2
#undef LIST3
#undef LIST4
#undef LIST5
#undef LIST6
#undef LIST7
#undef LIST8
#undef LIST9
#undef LIST10

#undef TYPENAMES
#undef PARS
#undef CONV_TYPE
#undef CONV_FROM_C4V
#undef TEMPLATE

// :'(
// is needed
class C4AulDefCastFunc: public C4AulDefFuncHelper
	{
	public:
		C4AulDefCastFunc(C4AulScript *pOwner, const char *pName, C4V_Type ParType, C4V_Type RType):
			C4AulDefFuncHelper(pOwner, pName, false, ParType), RType(RType) { }
		C4Value Exec(C4AulContext *pContext, C4Value pPars[], bool fPassErrors=false)
			{ return C4Value(pPars->GetData(), RType); }
		C4V_Type RType;
	};


void InitFunctionMap(C4AulScriptEngine *pEngine)
	{
	// add all def constants (all Int)
	for (C4ScriptConstDef *pCDef = &C4ScriptConstMap[0]; pCDef->Identifier; pCDef++)
		{
		assert(pCDef->ValType == C4V_Int); // only int supported currently
		::ScriptEngine.RegisterGlobalConstant(pCDef->Identifier, C4VInt(pCDef->Data));
		}

	// add all def script funcs
	for (C4ScriptFnDef *pDef = &C4ScriptFnMap[0]; pDef->Identifier; pDef++)
		pEngine->AddFunc(pDef->Identifier, pDef);
//	AddFunc(pEngine, "SetSaturation", FnSetSaturation); //public: 0
	AddFunc(pEngine, "Or", FnOr, false);
	AddFunc(pEngine, "Not", FnNot, false);
	AddFunc(pEngine, "And", FnAnd, false);
	AddFunc(pEngine, "BitAnd", FnBitAnd, false);
	AddFunc(pEngine, "Sum", FnSum, false);
	AddFunc(pEngine, "Sub", FnSub, false);
	AddFunc(pEngine, "Abs", FnAbs);
	AddFunc(pEngine, "Min", FnMin);
	AddFunc(pEngine, "Max", FnMax);
	AddFunc(pEngine, "Mul", FnMul, false);
	AddFunc(pEngine, "Div", FnDiv, false);
	AddFunc(pEngine, "Mod", FnMod, false);
	AddFunc(pEngine, "Pow", FnPow, false);
	AddFunc(pEngine, "Sin", FnSin);
	AddFunc(pEngine, "Cos", FnCos);
	AddFunc(pEngine, "Sqrt", FnSqrt);
	AddFunc(pEngine, "ArcSin", FnArcSin);
	AddFunc(pEngine, "ArcCos", FnArcCos);
	AddFunc(pEngine, "LessThan", FnLessThan, false);
	AddFunc(pEngine, "GreaterThan", FnGreaterThan, false);
	AddFunc(pEngine, "BoundBy", FnBoundBy);
	AddFunc(pEngine, "Inside", FnInside);
	AddFunc(pEngine, "SEqual", FnSEqual, false);
	AddFunc(pEngine, "Random", FnRandom);
	AddFunc(pEngine, "AsyncRandom", FnAsyncRandom);
	AddFunc(pEngine, "DoCon", FnDoCon);
	AddFunc(pEngine, "GetCon", FnGetCon);
	AddFunc(pEngine, "DoDamage", FnDoDamage);
	AddFunc(pEngine, "DoEnergy", FnDoEnergy);
	AddFunc(pEngine, "DoBreath", FnDoBreath);
	AddFunc(pEngine, "DoMagicEnergy", FnDoMagicEnergy);
	AddFunc(pEngine, "GetMagicEnergy", FnGetMagicEnergy);
	AddFunc(pEngine, "EnergyCheck", FnEnergyCheck);
	AddFunc(pEngine, "CheckEnergyNeedChain", FnCheckEnergyNeedChain);
	AddFunc(pEngine, "GetEnergy", FnGetEnergy);
	AddFunc(pEngine, "OnFire", FnOnFire);
	AddFunc(pEngine, "Smoke", FnSmoke);
	AddFunc(pEngine, "Stuck", FnStuck);
	AddFunc(pEngine, "InLiquid", FnInLiquid);
	AddFunc(pEngine, "Bubble", FnBubble);
	AddFunc(pEngine, "SetAction", FnSetAction);
	AddFunc(pEngine, "SetActionData", FnSetActionData);
	AddFunc(pEngine, "SetBridgeActionData", FnSetBridgeActionData);
	AddFunc(pEngine, "GetAction", FnGetAction);
	AddFunc(pEngine, "GetActTime", FnGetActTime);
	AddFunc(pEngine, "GetOwner", FnGetOwner);
	AddFunc(pEngine, "GetMass", FnGetMass);
	AddFunc(pEngine, "GetBreath", FnGetBreath);
	AddFunc(pEngine, "GetX", FnGetX);
	AddFunc(pEngine, "GetY", FnGetY);
	AddFunc(pEngine, "GetBase", FnGetBase);
	AddFunc(pEngine, "GetMenu", FnGetMenu);
	AddFunc(pEngine, "GetVertexNum", FnGetVertexNum);
	AddFunc(pEngine, "GetVertex", FnGetVertex);
	AddFunc(pEngine, "SetVertex", FnSetVertex);
	AddFunc(pEngine, "AddVertex", FnAddVertex);
	AddFunc(pEngine, "RemoveVertex", FnRemoveVertex);
	AddFunc(pEngine, "SetContactDensity", FnSetContactDensity, false);
	AddFunc(pEngine, "AnyContainer", FnAnyContainer);
	AddFunc(pEngine, "NoContainer", FnNoContainer);
	AddFunc(pEngine, "GetController", FnGetController);
	AddFunc(pEngine, "SetController", FnSetController);
	AddFunc(pEngine, "GetKiller", FnGetKiller);
	AddFunc(pEngine, "SetKiller", FnSetKiller);
	AddFunc(pEngine, "GetPhase", FnGetPhase);
	AddFunc(pEngine, "SetPhase", FnSetPhase);
	AddFunc(pEngine, "GetCategory", FnGetCategory);
	AddFunc(pEngine, "GetOCF", FnGetOCF);
	AddFunc(pEngine, "SetAlive", FnSetAlive);
	AddFunc(pEngine, "GetAlive", FnGetAlive);
	AddFunc(pEngine, "GetDamage", FnGetDamage);
	AddFunc(pEngine, "CrewMember", FnCrewMember);
	AddFunc(pEngine, "ObjectSetAction", FnObjectSetAction, false);
	AddFunc(pEngine, "ComponentAll", FnComponentAll);
	AddFunc(pEngine, "SetComDir", FnSetComDir);
	AddFunc(pEngine, "GetComDir", FnGetComDir);
	AddFunc(pEngine, "SetDir", FnSetDir);
	AddFunc(pEngine, "GetDir", FnGetDir);
	AddFunc(pEngine, "SetEntrance", FnSetEntrance);
	AddFunc(pEngine, "GetEntrance", FnGetEntrance);
	AddFunc(pEngine, "SetCategory", FnSetCategory);
	AddFunc(pEngine, "FinishCommand", FnFinishCommand);
	AddFunc(pEngine, "GetDefinition", FnGetDefinition);
	AddFunc(pEngine, "ActIdle", FnActIdle);
	AddFunc(pEngine, "SetRDir", FnSetRDir);
	AddFunc(pEngine, "GetRDir", FnGetRDir);
	AddFunc(pEngine, "GetXDir", FnGetXDir);
	AddFunc(pEngine, "GetYDir", FnGetYDir);
	AddFunc(pEngine, "GetR", FnGetR);
	AddFunc(pEngine, "GetName", FnGetName);
	AddFunc(pEngine, "SetName", FnSetName);
	AddFunc(pEngine, "GetDesc", FnGetDesc);
	AddFunc(pEngine, "GetPlayerName", FnGetPlayerName);
	AddFunc(pEngine, "GetTaggedPlayerName", FnGetTaggedPlayerName);
	AddFunc(pEngine, "GetPlayerType", FnGetPlayerType);
	AddFunc(pEngine, "SetXDir", FnSetXDir);
	AddFunc(pEngine, "SetYDir", FnSetYDir);
	AddFunc(pEngine, "SetR", FnSetR);
	AddFunc(pEngine, "SetOwner", FnSetOwner);
	AddFunc(pEngine, "CreatePropList", FnCreatePropList);
	AddFunc(pEngine, "CreateObject", FnCreateObject);
	AddFunc(pEngine, "MakeCrewMember", FnMakeCrewMember);
	AddFunc(pEngine, "GrabObjectInfo", FnGrabObjectInfo);
	AddFunc(pEngine, "CreateContents", FnCreateContents);
	AddFunc(pEngine, "ShiftContents", FnShiftContents);
	AddFunc(pEngine, "ComposeContents", FnComposeContents);
	AddFunc(pEngine, "CreateConstruction", FnCreateConstruction);
	AddFunc(pEngine, "GetID", FnGetID);
	AddFunc(pEngine, "Contents", FnContents);
	AddFunc(pEngine, "ScrollContents", FnScrollContents);
	AddFunc(pEngine, "Contained", FnContained);
	AddFunc(pEngine, "ContentsCount", FnContentsCount);
	AddFunc(pEngine, "FindContents", FnFindContents);
	AddFunc(pEngine, "FindConstructionSite", FnFindConstructionSite);
	AddFunc(pEngine, "FindOtherContents", FnFindOtherContents);
	AddFunc(pEngine, "FindBase", FnFindBase);
	AddFunc(pEngine, "Sound", FnSound);
	AddFunc(pEngine, "Music", FnMusic);
	AddFunc(pEngine, "MusicLevel", FnMusicLevel);
	AddFunc(pEngine, "SetPlayList", FnSetPlayList);
	AddFunc(pEngine, "SoundLevel", FnSoundLevel, false);
	AddFunc(pEngine, "FindObjectOwner", FnFindObjectOwner);
	AddFunc(pEngine, "RemoveObject", FnRemoveObject);
	AddFunc(pEngine, "GetActionTarget", FnGetActionTarget);
	AddFunc(pEngine, "SetActionTargets", FnSetActionTargets);
	AddFunc(pEngine, "SetPlrView", FnSetPlrView);
	AddFunc(pEngine, "SetPlrKnowledge", FnSetPlrKnowledge);
	AddFunc(pEngine, "SetPlrMagic", FnSetPlrMagic);
	AddFunc(pEngine, "GetPlrDownDouble", FnGetPlrDownDouble);
	AddFunc(pEngine, "ClearLastPlrCom", FnClearLastPlrCom);
	AddFunc(pEngine, "GetPlrViewMode", FnGetPlrViewMode);
	AddFunc(pEngine, "GetPlrView", FnGetPlrView);
	AddFunc(pEngine, "GetWealth", FnGetWealth);
	AddFunc(pEngine, "SetWealth", FnSetWealth);
	AddFunc(pEngine, "SetComponent", FnSetComponent);
	AddFunc(pEngine, "DoScore", FnDoScore);
	AddFunc(pEngine, "GetScore", FnGetScore);
	AddFunc(pEngine, "GetPlrValue", FnGetPlrValue);
	AddFunc(pEngine, "GetPlrValueGain", FnGetPlrValueGain);
	AddFunc(pEngine, "SetPlrShowControl", FnSetPlrShowControl);
	AddFunc(pEngine, "SetPlrShowControlPos", FnSetPlrShowControlPos);
  AddFunc(pEngine, "GetPlrControlName", FnGetPlrControlName);
	AddFunc(pEngine, "SetPlrShowCommand", FnSetPlrShowCommand);
	AddFunc(pEngine, "GetWind", FnGetWind);
	AddFunc(pEngine, "SetWind", FnSetWind);
	AddFunc(pEngine, "SetSkyFade", FnSetSkyFade);
	AddFunc(pEngine, "SetSkyColor", FnSetSkyColor);
	AddFunc(pEngine, "GetSkyColor", FnGetSkyColor);
	AddFunc(pEngine, "GetTemperature", FnGetTemperature);
	AddFunc(pEngine, "SetTemperature", FnSetTemperature);
	AddFunc(pEngine, "LaunchLightning", FnLaunchLightning);
	AddFunc(pEngine, "LaunchVolcano", FnLaunchVolcano);
	AddFunc(pEngine, "LaunchEarthquake", FnLaunchEarthquake);
	AddFunc(pEngine, "ShakeFree", FnShakeFree);
	AddFunc(pEngine, "ShakeObjects", FnShakeObjects);
	AddFunc(pEngine, "DigFree", FnDigFree);
	AddFunc(pEngine, "FreeRect", FnFreeRect);
	AddFunc(pEngine, "DigFreeRect", FnDigFreeRect);
	AddFunc(pEngine, "CastPXS", FnCastPXS);
	AddFunc(pEngine, "CastObjects", FnCastObjects);
	AddFunc(pEngine, "Hostile", FnHostile);
	AddFunc(pEngine, "SetHostility", FnSetHostility);
	AddFunc(pEngine, "PlaceVegetation", FnPlaceVegetation);
	AddFunc(pEngine, "PlaceAnimal", FnPlaceAnimal);
	AddFunc(pEngine, "GameOver", FnGameOver);
	AddFunc(pEngine, "C4Id", FnC4Id);
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
	AddFunc(pEngine, "GetCaptain", FnGetCaptain);
	AddFunc(pEngine, "SetCursor", FnSetCursor);
	AddFunc(pEngine, "SetViewCursor", FnSetViewCursor);
	AddFunc(pEngine, "SelectCrew", FnSelectCrew);
	AddFunc(pEngine, "GetSelectCount", FnGetSelectCount);
	AddFunc(pEngine, "SetCrewStatus", FnSetCrewStatus, false);
	AddFunc(pEngine, "SetPosition", FnSetPosition);
	AddFunc(pEngine, "ExtractLiquid", FnExtractLiquid);
	AddFunc(pEngine, "GetMaterial", FnGetMaterial);
	AddFunc(pEngine, "GetTexture", FnGetTexture);
	AddFunc(pEngine, "GetMaterialCount", FnGetMaterialCount);
	AddFunc(pEngine, "GBackSolid", FnGBackSolid);
	AddFunc(pEngine, "GBackSemiSolid", FnGBackSemiSolid);
	AddFunc(pEngine, "GBackLiquid", FnGBackLiquid);
	AddFunc(pEngine, "GBackSky", FnGBackSky);
	AddFunc(pEngine, "Material", FnMaterial);
	AddFunc(pEngine, "BlastObjects", FnBlastObjects);
	AddFunc(pEngine, "BlastObject", FnBlastObject);
	AddFunc(pEngine, "BlastFree", FnBlastFree);
	AddFunc(pEngine, "InsertMaterial", FnInsertMaterial);
	AddFunc(pEngine, "DrawVolcanoBranch", FnDrawVolcanoBranch, false);
	AddFunc(pEngine, "FlameConsumeMaterial", FnFlameConsumeMaterial, false);
	AddFunc(pEngine, "LandscapeWidth", FnLandscapeWidth);
	AddFunc(pEngine, "LandscapeHeight", FnLandscapeHeight);
	AddFunc(pEngine, "Resort", FnResort);
	AddFunc(pEngine, "CreateMenu", FnCreateMenu);
	AddFunc(pEngine, "SelectMenuItem", FnSelectMenuItem);
	AddFunc(pEngine, "SetMenuDecoration", FnSetMenuDecoration);
	AddFunc(pEngine, "SetMenuTextProgress", FnSetMenuTextProgress);
	AddFunc(pEngine, "SetSeason", FnSetSeason);
	AddFunc(pEngine, "GetSeason", FnGetSeason);
	AddFunc(pEngine, "SetClimate", FnSetClimate);
	AddFunc(pEngine, "GetClimate", FnGetClimate);
	AddFunc(pEngine, "Distance", FnDistance);
	AddFunc(pEngine, "ObjectDistance", FnObjectDistance);
	AddFunc(pEngine, "GetValue", FnGetValue);
	AddFunc(pEngine, "GetRank", FnGetRank);
	AddFunc(pEngine, "Value", FnValue);
	AddFunc(pEngine, "Angle", FnAngle);
	AddFunc(pEngine, "DoHomebaseMaterial", FnDoHomebaseMaterial);
	AddFunc(pEngine, "DoHomebaseProduction", FnDoHomebaseProduction);
	AddFunc(pEngine, "GainMissionAccess", FnGainMissionAccess);
	AddFunc(pEngine, "SetPhysical", FnSetPhysical);
	AddFunc(pEngine, "TrainPhysical", FnTrainPhysical);
	AddFunc(pEngine, "GetPhysical", FnGetPhysical);
	AddFunc(pEngine, "ResetPhysical", FnResetPhysical);
	AddFunc(pEngine, "SetTransferZone", FnSetTransferZone);
	AddFunc(pEngine, "IsNetwork", FnIsNetwork);
	AddFunc(pEngine, "GetLeague", FnGetLeague);
	AddFunc(pEngine, "TestMessageBoard", FnTestMessageBoard, false);
	AddFunc(pEngine, "CallMessageBoard", FnCallMessageBoard, false);
	AddFunc(pEngine, "AbortMessageBoard", FnAbortMessageBoard, false);
	AddFunc(pEngine, "OnMessageBoardAnswer", FnOnMessageBoardAnswer, false);
	AddFunc(pEngine, "ScriptCounter", FnScriptCounter);
	AddFunc(pEngine, "SetMass", FnSetMass);
	AddFunc(pEngine, "GetColor", FnGetColor);
	AddFunc(pEngine, "SetColor", FnSetColor);
	AddFunc(pEngine, "SetFoW", FnSetFoW);
	AddFunc(pEngine, "SetPlrViewRange", FnSetPlrViewRange);
	AddFunc(pEngine, "SetMaxPlayer", FnSetMaxPlayer);
	AddFunc(pEngine, "SetPicture", FnSetPicture);
	AddFunc(pEngine, "Buy", FnBuy);
	AddFunc(pEngine, "Sell", FnSell);
	AddFunc(pEngine, "GetProcedure", FnGetProcedure);
	AddFunc(pEngine, "GetChar", FnGetChar);
	AddFunc(pEngine, "ActivateGameGoalMenu", FnActivateGameGoalMenu);
	AddFunc(pEngine, "SetGraphics", FnSetGraphics);
	AddFunc(pEngine, "Object", FnObject);
	AddFunc(pEngine, "ObjectNumber", FnObjectNumber);
	AddFunc(pEngine, "ShowInfo", FnShowInfo);
	AddFunc(pEngine, "GetTime", FnGetTime);
	AddFunc(pEngine, "GetSystemTime", FnGetSystemTime, false);
	AddFunc(pEngine, "SetVisibility", FnSetVisibility);
	AddFunc(pEngine, "GetVisibility", FnGetVisibility);
	AddFunc(pEngine, "SetClrModulation", FnSetClrModulation);
	AddFunc(pEngine, "GetClrModulation", FnGetClrModulation);
	AddFunc(pEngine, "GetMissionAccess", FnGetMissionAccess);
	AddFunc(pEngine, "CloseMenu", FnCloseMenu);
	AddFunc(pEngine, "GetMenuSelection", FnGetMenuSelection);
	AddFunc(pEngine, "ResortObjects", FnResortObjects);
	AddFunc(pEngine, "ResortObject", FnResortObject);
	AddFunc(pEngine, "GetDefBottom", FnGetDefBottom);
	AddFunc(pEngine, "SetMaterialColor", FnSetMaterialColor);
	AddFunc(pEngine, "GetMaterialColor", FnGetMaterialColor);
	AddFunc(pEngine, "MaterialName", FnMaterialName);
	AddFunc(pEngine, "SetMenuSize", FnSetMenuSize);
	AddFunc(pEngine, "GetNeededMatStr", FnGetNeededMatStr);
	AddFunc(pEngine, "GetCrewEnabled", FnGetCrewEnabled);
	AddFunc(pEngine, "SetCrewEnabled", FnSetCrewEnabled);
	AddFunc(pEngine, "UnselectCrew", FnUnselectCrew);
	AddFunc(pEngine, "DrawMap", FnDrawMap);
	AddFunc(pEngine, "DrawDefMap", FnDrawDefMap);
	AddFunc(pEngine, "DrawModLandscape", FnDrawModLandscape, false);
	AddFunc(pEngine, "CreateParticle", FnCreateParticle);
	AddFunc(pEngine, "CastParticles", FnCastParticles);
	AddFunc(pEngine, "CastBackParticles", FnCastBackParticles);
	AddFunc(pEngine, "PushParticles", FnPushParticles);
	AddFunc(pEngine, "ClearParticles", FnClearParticles);
	AddFunc(pEngine, "IsNewgfx", FnIsNewgfx, false);
	AddFunc(pEngine, "SetSkyAdjust", FnSetSkyAdjust);
	AddFunc(pEngine, "SetMatAdjust", FnSetMatAdjust);
	AddFunc(pEngine, "GetSkyAdjust", FnGetSkyAdjust);
	AddFunc(pEngine, "GetMatAdjust", FnGetMatAdjust);
	AddFunc(pEngine, "SetSkyParallax", FnSetSkyParallax);
	AddFunc(pEngine, "DoCrewExp", FnDoCrewExp);
	AddFunc(pEngine, "ReloadDef", FnReloadDef);
	AddFunc(pEngine, "ReloadParticle", FnReloadParticle);
	AddFunc(pEngine, "SetGamma", FnSetGamma);
	AddFunc(pEngine, "ResetGamma", FnResetGamma);
	AddFunc(pEngine, "FrameCounter", FnFrameCounter);
	AddFunc(pEngine, "SetLandscapePixel", FnSetLandscapePixel);
	AddFunc(pEngine, "SetObjectOrder", FnSetObjectOrder);
	AddFunc(pEngine, "SetColorDw", FnSetColorDw);
	AddFunc(pEngine, "GetColorDw", FnGetColorDw);
	AddFunc(pEngine, "GetPlrColorDw", FnGetPlrColorDw);
	AddFunc(pEngine, "DrawMaterialQuad", FnDrawMaterialQuad);
	AddFunc(pEngine, "FightWith", FnFightWith);
	AddFunc(pEngine, "SetFilmView", FnSetFilmView);
	AddFunc(pEngine, "ClearMenuItems", FnClearMenuItems);
	AddFunc(pEngine, "GetObjectLayer", FnGetObjectLayer, false);
	AddFunc(pEngine, "SetObjectLayer", FnSetObjectLayer, false);
	AddFunc(pEngine, "SetShape", FnSetShape);
	AddFunc(pEngine, "AddMsgBoardCmd", FnAddMsgBoardCmd);
	AddFunc(pEngine, "SetGameSpeed", FnSetGameSpeed, false);
	AddFunc(pEngine, "DrawMatChunks", FnDrawMatChunks, false);
	AddFunc(pEngine, "GetPathLength", FnGetPathLength);
	AddFunc(pEngine, "SetTextureIndex", FnSetTextureIndex, false);
  AddFunc(pEngine, "RemoveUnusedTexMapEntries", FnRemoveUnusedTexMapEntries, false);
	AddFunc(pEngine, "SetObjDrawTransform", FnSetObjDrawTransform);
	AddFunc(pEngine, "SetObjDrawTransform2", FnSetObjDrawTransform2, false);
	AddFunc(pEngine, "SetPortrait", FnSetPortrait);
	AddFunc(pEngine, "LoadScenarioSection", FnLoadScenarioSection, false);
	AddFunc(pEngine, "SetObjectStatus", FnSetObjectStatus, false);
	AddFunc(pEngine, "GetObjectStatus", FnGetObjectStatus, false);
	AddFunc(pEngine, "AdjustWalkRotation", FnAdjustWalkRotation, false);
	AddFunc(pEngine, "FxFireStart", FnFxFireStart, false);
	AddFunc(pEngine, "FxFireTimer", FnFxFireTimer, false);
	AddFunc(pEngine, "FxFireStop", FnFxFireStop, false);
	AddFunc(pEngine, "FxFireInfo", FnFxFireInfo, false);
	AddFunc(pEngine, "RemoveEffect", FnRemoveEffect);
	AddFunc(pEngine, "ChangeEffect", FnChangeEffect);
	AddFunc(pEngine, "ModulateColor", FnModulateColor);
	AddFunc(pEngine, "WildcardMatch", FnWildcardMatch);
	AddFunc(pEngine, "GetContact", FnGetContact);
	AddFunc(pEngine, "SetObjectBlitMode", FnSetObjectBlitMode);
	AddFunc(pEngine, "GetObjectBlitMode", FnGetObjectBlitMode);
	AddFunc(pEngine, "SetViewOffset", FnSetViewOffset);
	AddFunc(pEngine, "SetPreSend", FnSetPreSend, false);
	AddFunc(pEngine, "GetPlayerID", FnGetPlayerID, false);
	AddFunc(pEngine, "GetPlayerTeam", FnGetPlayerTeam);
	AddFunc(pEngine, "SetPlayerTeam", FnSetPlayerTeam);
	AddFunc(pEngine, "GetTeamConfig", FnGetTeamConfig);
	AddFunc(pEngine, "GetTeamName", FnGetTeamName);
	AddFunc(pEngine, "GetTeamColor", FnGetTeamColor);
	AddFunc(pEngine, "GetTeamByIndex", FnGetTeamByIndex);
	AddFunc(pEngine, "GetTeamCount", FnGetTeamCount);
	AddFunc(pEngine, "InitScenarioPlayer", FnInitScenarioPlayer, false);
	AddFunc(pEngine, PSF_OnOwnerRemoved, FnOnOwnerRemoved, false);
	AddFunc(pEngine, "SetScoreboardData", FnSetScoreboardData, false);
	AddFunc(pEngine, "GetScoreboardString", FnGetScoreboardString, false);
	AddFunc(pEngine, "GetScoreboardData", FnGetScoreboardData, false);
	AddFunc(pEngine, "DoScoreboardShow", FnDoScoreboardShow, false);
	AddFunc(pEngine, "SortScoreboard", FnSortScoreboard, false);
	AddFunc(pEngine, "AddEvaluationData", FnAddEvaluationData, false);
	AddFunc(pEngine, "GetUnusedOverlayID", FnGetUnusedOverlayID, false);
	AddFunc(pEngine, "FatalError", FnFatalError, false);
	AddFunc(pEngine, "ExtractMaterialAmount", FnExtractMaterialAmount);
	AddFunc(pEngine, "GetEffectCount", FnGetEffectCount);
	AddFunc(pEngine, "PlayVideo", FnPlayVideo);
	AddFunc(pEngine, "StartCallTrace", FnStartCallTrace);
	AddFunc(pEngine, "StartScriptProfiler", FnStartScriptProfiler);
	AddFunc(pEngine, "StopScriptProfiler", FnStopScriptProfiler);
	AddFunc(pEngine, "CustomMessage", FnCustomMessage);
	AddFunc(pEngine, "PauseGame", FnPauseGame);
	AddFunc(pEngine, "ExecuteCommand", FnExecuteCommand);
	AddFunc(pEngine, "LocateFunc", FnLocateFunc);
	AddFunc(pEngine, "PathFree", FnPathFree);
	AddFunc(pEngine, "SetNextMission", FnSetNextMission);
	//FIXME new C4AulDefCastFunc(pEngine, "ScoreboardCol", C4V_C4ID, C4V_Int);
	new C4AulDefCastFunc(pEngine, "CastInt", C4V_Any, C4V_Int);
	new C4AulDefCastFunc(pEngine, "CastBool", C4V_Any, C4V_Bool);
	new C4AulDefCastFunc(pEngine, "CastAny", C4V_Any, C4V_Any);
	}

C4ScriptConstDef C4ScriptConstMap[]={
	{ "C4D_All"                ,C4V_Int,          C4D_All},
	{ "C4D_StaticBack"         ,C4V_Int,          C4D_StaticBack},
	{ "C4D_Structure"          ,C4V_Int,          C4D_Structure},
	{ "C4D_Vehicle"            ,C4V_Int,          C4D_Vehicle},
	{ "C4D_Living"             ,C4V_Int,          C4D_Living},
	{ "C4D_Object"             ,C4V_Int,          C4D_Object},
	{ "C4D_Goal"               ,C4V_Int,          C4D_Goal},
	{ "C4D_Environment"        ,C4V_Int,          C4D_Environment},
	{ "C4D_Knowledge"          ,C4V_Int,          C4D_SelectKnowledge},
	{ "C4D_Magic"              ,C4V_Int,          C4D_Magic},
	{ "C4D_Rule"               ,C4V_Int,          C4D_Rule},
	{ "C4D_Background"         ,C4V_Int,          C4D_Background},
	{ "C4D_Parallax"           ,C4V_Int,          C4D_Parallax},
	{ "C4D_MouseSelect"        ,C4V_Int,          C4D_MouseSelect},
	{ "C4D_Foreground"         ,C4V_Int,          C4D_Foreground},
	{ "C4D_MouseIgnore"        ,C4V_Int,          C4D_MouseIgnore},
	{ "C4D_IgnoreFoW"          ,C4V_Int,          C4D_IgnoreFoW},

	{ "C4D_GrabGet"            ,C4V_Int,          C4D_Grab_Get},
	{ "C4D_GrabPut"            ,C4V_Int,          C4D_Grab_Put},

	{ "C4D_LinePower"          ,C4V_Int,          C4D_Line_Power},
	{ "C4D_LineSource"         ,C4V_Int,          C4D_Line_Source},
	{ "C4D_LineDrain"          ,C4V_Int,          C4D_Line_Drain},
	{ "C4D_LineLightning"      ,C4V_Int,          C4D_Line_Lightning},
	{ "C4D_LineVolcano"        ,C4V_Int,          C4D_Line_Volcano},
	{ "C4D_LineRope"           ,C4V_Int,          C4D_Line_Rope},
	{ "C4D_LineColored"        ,C4V_Int,          C4D_Line_Colored},
	{ "C4D_LineVertex"         ,C4V_Int,          C4D_Line_Vertex},

	{ "C4D_PowerInput"         ,C4V_Int,          C4D_Power_Input},
	{ "C4D_PowerOutput"        ,C4V_Int,          C4D_Power_Output},
	{ "C4D_LiquidInput"        ,C4V_Int,          C4D_Liquid_Input},
	{ "C4D_LiquidOutput"       ,C4V_Int,          C4D_Liquid_Output},
	{ "C4D_PowerGenerator"     ,C4V_Int,          C4D_Power_Generator},
	{ "C4D_PowerConsumer"      ,C4V_Int,          C4D_Power_Consumer},
	{ "C4D_LiquidPump"         ,C4V_Int,          C4D_Liquid_Pump},
	//{ "C4D_ConnectRope"	       ,C4V_Int,          C4D_Connect_Rope},
	{ "C4D_EnergyHolder"       ,C4V_Int,          C4D_EnergyHolder},

	{ "C4V_Any"                ,C4V_Int,          C4V_Any},
	{ "C4V_Int"                ,C4V_Int,          C4V_Int},
	{ "C4V_Bool"               ,C4V_Int,          C4V_Bool},
	{ "C4V_C4Object"           ,C4V_Int,          C4V_C4Object},
	{ "C4V_String"             ,C4V_Int,          C4V_String},
	{ "C4V_Array"              ,C4V_Int,          C4V_Array},
	{ "C4V_PropList"           ,C4V_Int,          C4V_PropList},

	{ "COMD_None"              ,C4V_Int,          COMD_None},
	{ "COMD_Stop"              ,C4V_Int,          COMD_Stop},
	{ "COMD_Up"                ,C4V_Int,          COMD_Up},
	{ "COMD_UpRight"           ,C4V_Int,          COMD_UpRight},
	{ "COMD_Right"             ,C4V_Int,          COMD_Right},
	{ "COMD_DownRight"         ,C4V_Int,          COMD_DownRight},
	{ "COMD_Down"              ,C4V_Int,          COMD_Down},
	{ "COMD_DownLeft"          ,C4V_Int,          COMD_DownLeft},
	{ "COMD_Left"              ,C4V_Int,          COMD_Left},
	{ "COMD_UpLeft"            ,C4V_Int,          COMD_UpLeft},

	{ "DIR_Left"               ,C4V_Int,          DIR_Left},
	{ "DIR_Right"              ,C4V_Int,          DIR_Right},

  { "CON_CursorLeft"         ,C4V_Int,          CON_CursorLeft},
  { "CON_CursorToggle"       ,C4V_Int,          CON_CursorToggle},
  { "CON_CursorRight"        ,C4V_Int,          CON_CursorRight},
  { "CON_Throw"              ,C4V_Int,          CON_Throw},
  { "CON_Up"                 ,C4V_Int,          CON_Up},
  { "CON_Dig"                ,C4V_Int,          CON_Dig},
  { "CON_Left"               ,C4V_Int,          CON_Left},
  { "CON_Down"               ,C4V_Int,          CON_Down},
  { "CON_Right"              ,C4V_Int,          CON_Right},
  { "CON_Menu"               ,C4V_Int,          CON_Menu},
  { "CON_Special"            ,C4V_Int,          CON_Special},
  { "CON_Special2"           ,C4V_Int,          CON_Special2},

	{ "OCF_Construct"          ,C4V_Int,          OCF_Construct},
	{ "OCF_Grab"               ,C4V_Int,          OCF_Grab},
	{ "OCF_Collectible"        ,C4V_Int,          OCF_Carryable},
	{ "OCF_OnFire"             ,C4V_Int,          OCF_OnFire},
	{ "OCF_HitSpeed1"          ,C4V_Int,          OCF_HitSpeed1},
	{ "OCF_Fullcon"            ,C4V_Int,          OCF_FullCon},
	{ "OCF_Inflammable"        ,C4V_Int,          OCF_Inflammable},
	{ "OCF_Chop"               ,C4V_Int,          OCF_Chop},
	{ "OCF_Rotate"             ,C4V_Int,          OCF_Rotate},
	{ "OCF_Exclusive"          ,C4V_Int,          OCF_Exclusive},
	{ "OCF_Entrance"           ,C4V_Int,          OCF_Entrance},
	{ "OCF_HitSpeed2"          ,C4V_Int,          OCF_HitSpeed2},
	{ "OCF_HitSpeed3"          ,C4V_Int,          OCF_HitSpeed3},
	{ "OCF_Collection"         ,C4V_Int,          OCF_Collection},
	{ "OCF_Living"             ,C4V_Int,          OCF_Living},
	{ "OCF_HitSpeed4"          ,C4V_Int,          OCF_HitSpeed4},
	{ "OCF_FightReady"         ,C4V_Int,          OCF_FightReady},
	{ "OCF_LineConstruct"      ,C4V_Int,          OCF_LineConstruct},
	{ "OCF_Prey"               ,C4V_Int,          OCF_Prey},
	{ "OCF_AttractLightning"   ,C4V_Int,          OCF_AttractLightning},
	{ "OCF_NotContained"       ,C4V_Int,          OCF_NotContained},
	{ "OCF_CrewMember"         ,C4V_Int,          OCF_CrewMember},
	{ "OCF_Edible"             ,C4V_Int,          OCF_Edible},
	{ "OCF_InLiquid"           ,C4V_Int,          OCF_InLiquid},
	{ "OCF_InSolid"            ,C4V_Int,          OCF_InSolid},
	{ "OCF_InFree"             ,C4V_Int,          OCF_InFree},
	{ "OCF_Available"          ,C4V_Int,          OCF_Available},
	{ "OCF_PowerConsumer"      ,C4V_Int,          OCF_PowerConsumer},
	{ "OCF_PowerSupply"        ,C4V_Int,          OCF_PowerSupply},
	{ "OCF_Container"          ,C4V_Int,          OCF_Container},
	{ "OCF_Alive"              ,C4V_Int,          (int) OCF_Alive},

	{ "VIS_All"                ,C4V_Int,          VIS_All},
	{ "VIS_None"               ,C4V_Int,          VIS_None},
	{ "VIS_Owner"              ,C4V_Int,          VIS_Owner},
	{ "VIS_Allies"             ,C4V_Int,          VIS_Allies},
	{ "VIS_Enemies"            ,C4V_Int,          VIS_Enemies},
	{ "VIS_Local"              ,C4V_Int,          VIS_Local},
	{ "VIS_God"                ,C4V_Int,          VIS_God},
	{ "VIS_LayerToggle"        ,C4V_Int,          VIS_LayerToggle},
	{ "VIS_OverlayOnly"        ,C4V_Int,          VIS_OverlayOnly},

	{ "C4X_Ver1"               ,C4V_Int,          C4XVER1},
	{ "C4X_Ver2"               ,C4V_Int,          C4XVER2},
	{ "C4X_Ver3"               ,C4V_Int,          C4XVER3},
	{ "C4X_Ver4"               ,C4V_Int,          C4XVER4},
	{ "C4X_VerBuild"           ,C4V_Int,          C4XVERBUILD},

	{ "SkyPar_Keep"            ,C4V_Int,          SkyPar_KEEP},

	{ "C4MN_Style_Normal"      ,C4V_Int,          C4MN_Style_Normal},
	{ "C4MN_Style_Context"     ,C4V_Int,          C4MN_Style_Context},
	{ "C4MN_Style_Info"        ,C4V_Int,          C4MN_Style_Info},
	{ "C4MN_Style_Dialog"      ,C4V_Int,          C4MN_Style_Dialog},
	{ "C4MN_Style_EqualItemHeight",C4V_Int,       C4MN_Style_EqualItemHeight},

	{ "C4MN_Extra_None"        ,C4V_Int,          C4MN_Extra_None},
	{ "C4MN_Extra_Components"  ,C4V_Int,          C4MN_Extra_Components},
	{ "C4MN_Extra_Value"       ,C4V_Int,          C4MN_Extra_Value},
	{ "C4MN_Extra_MagicValue"  ,C4V_Int,          C4MN_Extra_MagicValue},
	{ "C4MN_Extra_Info"        ,C4V_Int,          C4MN_Extra_Info},
	{ "C4MN_Extra_ComponentsMagic",C4V_Int,      C4MN_Extra_ComponentsMagic},
	{ "C4MN_Extra_LiveMagicValue" ,C4V_Int,      C4MN_Extra_LiveMagicValue},
	{ "C4MN_Extra_ComponentsLiveMagic",C4V_Int,  C4MN_Extra_ComponentsLiveMagic},

	{ "C4MN_Add_ImgRank"       ,C4V_Int,          C4MN_Add_ImgRank},
	{ "C4MN_Add_ImgIndexed"    ,C4V_Int,          C4MN_Add_ImgIndexed},
	{ "C4MN_Add_ImgObjRank"    ,C4V_Int,          C4MN_Add_ImgObjRank},
	{ "C4MN_Add_ImgObject"     ,C4V_Int,          C4MN_Add_ImgObject},
	{ "C4MN_Add_ImgTextSpec"   ,C4V_Int,          C4MN_Add_ImgTextSpec},
	{ "C4MN_Add_ImgColor"      ,C4V_Int,          C4MN_Add_ImgColor},
	{ "C4MN_Add_PassValue"     ,C4V_Int,          C4MN_Add_PassValue},
	{ "C4MN_Add_ForceCount"    ,C4V_Int,          C4MN_Add_ForceCount},
	{ "C4MN_Add_ForceNoDesc"   ,C4V_Int,          C4MN_Add_ForceNoDesc},

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
	{ "FX_Call_DmgChop"           ,C4V_Int,      C4FxCall_DmgChop           }, // damage through chopping
	{ "FX_Call_Energy"            ,C4V_Int,      32                         }, // bitmask for generic energy loss
	{ "FX_Call_EngScript"         ,C4V_Int,      C4FxCall_EngScript         }, // energy loss through script call
	{ "FX_Call_EngBlast"          ,C4V_Int,      C4FxCall_EngBlast          }, // energy loss through blast
	{ "FX_Call_EngObjHit"         ,C4V_Int,      C4FxCall_EngObjHit         }, // energy loss through object hitting the living
	{ "FX_Call_EngFire"           ,C4V_Int,      C4FxCall_EngFire           }, // energy loss through fire
	{ "FX_Call_EngBaseRefresh"    ,C4V_Int,      C4FxCall_EngBaseRefresh    }, // energy reload in base (also by base object, but that's normally not called)
	{ "FX_Call_EngAsphyxiation"   ,C4V_Int,      C4FxCall_EngAsphyxiation   }, // energy loss through asphyxiaction
	{ "FX_Call_EngCorrosion"      ,C4V_Int,      C4FxCall_EngCorrosion      }, // energy loss through corrosion (acid)
	{ "FX_Call_EngStruct"         ,C4V_Int,      C4FxCall_EngStruct         }, // regular structure energy loss (normally not called)
	{ "FX_Call_EngGetPunched"     ,C4V_Int,      C4FxCall_EngGetPunched     }, // energy loss during fighting

	{ "GFXOV_MODE_None"           ,C4V_Int,      C4GraphicsOverlay::MODE_None },    // gfx overlay modes
	{ "GFXOV_MODE_Base"           ,C4V_Int,      C4GraphicsOverlay::MODE_Base },    //
	{ "GFXOV_MODE_Action"         ,C4V_Int,      C4GraphicsOverlay::MODE_Action },  //
	{ "GFXOV_MODE_Picture"        ,C4V_Int,      C4GraphicsOverlay::MODE_Picture }, //
	{ "GFXOV_MODE_IngamePicture"  ,C4V_Int,      C4GraphicsOverlay::MODE_IngamePicture }, //
	{ "GFXOV_MODE_Object"         ,C4V_Int,      C4GraphicsOverlay::MODE_Object }, //
	{ "GFXOV_MODE_ExtraGraphics"  ,C4V_Int,      C4GraphicsOverlay::MODE_ExtraGraphics }, //
	{ "GFX_Overlay"               ,C4V_Int,      1},                                // default overlay index
	{ "GFXOV_Clothing"            ,C4V_Int,      1000},                             // overlay indices for clothes on Clonks, etc.
	{ "GFXOV_Tools"               ,C4V_Int,      2000},                             // overlay indices for tools, weapons, etc.
	{ "GFXOV_ProcessTarget"       ,C4V_Int,      3000},                             // overlay indices for objects processed by a Clonk
	{ "GFXOV_Misc"                ,C4V_Int,      5000},                             // overlay indices for other stuff
	{ "GFXOV_UI"                  ,C4V_Int,      6000},                             // overlay indices for user interface
	{ "GFX_BLIT_Additive"         ,C4V_Int,      C4GFXBLIT_ADDITIVE},               // blit modes
	{ "GFX_BLIT_Mod2"             ,C4V_Int,      C4GFXBLIT_MOD2},                   //
	{ "GFX_BLIT_ClrSfc_OwnClr"    ,C4V_Int,      C4GFXBLIT_CLRSFC_OWNCLR},          //
	{ "GFX_BLIT_ClrSfc_Mod2"      ,C4V_Int,      C4GFXBLIT_CLRSFC_MOD2},            //
	{ "GFX_BLIT_Custom"           ,C4V_Int,      C4GFXBLIT_CUSTOM},                 //
	{ "GFX_BLIT_Parent"           ,C4V_Int,      C4GFXBLIT_PARENT},                 //

	{ "NO_OWNER"                  ,C4V_Int,      NO_OWNER                   }, // invalid player number

	// contact attachment
	{ "CNAT_None"                 ,C4V_Int,      CNAT_None                  },
	{ "CNAT_Left"                 ,C4V_Int,      CNAT_Left                  },
	{ "CNAT_Right"                ,C4V_Int,      CNAT_Right                 },
	{ "CNAT_Top"                  ,C4V_Int,      CNAT_Top                   },
	{ "CNAT_Bottom"               ,C4V_Int,      CNAT_Bottom                },
	{ "CNAT_Center"               ,C4V_Int,      CNAT_Center                },
	{ "CNAT_MultiAttach"          ,C4V_Int,      CNAT_MultiAttach           },
	{ "CNAT_NoCollision"          ,C4V_Int,      CNAT_NoCollision           },

	// vertex data
	{ "VTX_X"                     ,C4V_Int,      VTX_X                      },
	{ "VTX_Y"                     ,C4V_Int,      VTX_Y                      },
	{ "VTX_CNAT"                  ,C4V_Int,      VTX_CNAT                   },
	{ "VTX_Friction"              ,C4V_Int,      VTX_Friction               },

	// vertex set mode
	{ "VTX_SetPermanent"          ,C4V_Int,      VTX_SetPermanent           },
	{ "VTX_SetPermanentUpd"       ,C4V_Int,      VTX_SetPermanentUpd        },

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

	{ "C4OS_DELETED"              ,C4V_Int,      C4OS_DELETED               },
	{ "C4OS_NORMAL"               ,C4V_Int,      C4OS_NORMAL                },
	{ "C4OS_INACTIVE"             ,C4V_Int,      C4OS_INACTIVE              },

	{ "C4MSGCMDR_Escaped"         ,C4V_Int,      C4MessageBoardCommand::C4MSGCMDR_Escaped },
	{ "C4MSGCMDR_Plain"           ,C4V_Int,      C4MessageBoardCommand::C4MSGCMDR_Plain },
	{ "C4MSGCMDR_Identifier"      ,C4V_Int,      C4MessageBoardCommand::C4MSGCMDR_Identifier },

	{ "BASEFUNC_Default"          ,C4V_Int,			BASEFUNC_Default						},
	{ "BASEFUNC_AutoSellContents" ,C4V_Int,			BASEFUNC_AutoSellContents		},
	{ "BASEFUNC_RegenerateEnergy" ,C4V_Int,			BASEFUNC_RegenerateEnergy		},
	{ "BASEFUNC_Buy"              ,C4V_Int,			BASEFUNC_Buy								},
	{ "BASEFUNC_Sell"             ,C4V_Int,			BASEFUNC_Sell								},
	{ "BASEFUNC_RejectEntrance"   ,C4V_Int,			BASEFUNC_RejectEntrance			},
	{ "BASEFUNC_Extinguish"       ,C4V_Int,			BASEFUNC_Extinguish					},

	{ "C4FO_Not"                  ,C4V_Int,			C4FO_Not						},
	{ "C4FO_And"                  ,C4V_Int,			C4FO_And						},
	{ "C4FO_Or"                   ,C4V_Int,			C4FO_Or							},
	{ "C4FO_Exclude"              ,C4V_Int,			C4FO_Exclude				},
	{ "C4FO_InRect"               ,C4V_Int,			C4FO_InRect					},
	{ "C4FO_AtPoint"              ,C4V_Int,     C4FO_AtPoint 				},
	{ "C4FO_AtRect"               ,C4V_Int,     C4FO_AtRect 				},
	{ "C4FO_OnLine"							  ,C4V_Int,     C4FO_OnLine					},
	{ "C4FO_Distance"             ,C4V_Int,     C4FO_Distance				},
	{ "C4FO_ID"                   ,C4V_Int,     C4FO_ID							},
	{ "C4FO_OCF"                  ,C4V_Int,     C4FO_OCF						},
	{ "C4FO_Category"             ,C4V_Int,     C4FO_Category				},
	{ "C4FO_Action"               ,C4V_Int,     C4FO_Action					},
	{ "C4FO_ActionTarget"         ,C4V_Int,     C4FO_ActionTarget 	},
	{ "C4FO_Container"            ,C4V_Int,     C4FO_Container			},
	{ "C4FO_AnyContainer"         ,C4V_Int,     C4FO_AnyContainer 	},
	{ "C4FO_Owner"                ,C4V_Int,     C4FO_Owner					},
	{ "C4FO_Func"                 ,C4V_Int,     C4FO_Func						},
	{ "C4FO_Layer"                ,C4V_Int,     C4FO_Layer					},

	{ "C4SO_Reverse"              ,C4V_Int,     C4SO_Reverse				},
	{ "C4SO_Multiple"             ,C4V_Int,     C4SO_Multiple				},
	{ "C4SO_Distance"             ,C4V_Int,     C4SO_Distance				},
	{ "C4SO_Random"               ,C4V_Int,     C4SO_Random					},
	{ "C4SO_Speed"                ,C4V_Int,     C4SO_Speed					},
	{ "C4SO_Mass"                 ,C4V_Int,     C4SO_Mass						},
	{ "C4SO_Value"                ,C4V_Int,     C4SO_Value					},
	{ "C4SO_Func"		              ,C4V_Int,     C4SO_Func						},

	{ "PHYS_Current"              ,C4V_Int,     PHYS_Current				},
	{ "PHYS_Permanent"            ,C4V_Int,     PHYS_Permanent			},
	{ "PHYS_Temporary"            ,C4V_Int,     PHYS_Temporary			},
	{ "PHYS_StackTemporary"       ,C4V_Int,     PHYS_StackTemporary },

	{ "C4CMD_Base"                ,C4V_Int,     C4CMD_Mode_Base },
	{ "C4CMD_SilentBase"          ,C4V_Int,     C4CMD_Mode_SilentBase },
	{ "C4CMD_Sub"                 ,C4V_Int,     C4CMD_Mode_Sub },
	{ "C4CMD_SilentSub"           ,C4V_Int,     C4CMD_Mode_SilentSub },

	{ "C4CMD_MoveTo_NoPosAdjust"  ,C4V_Int,     C4CMD_MoveTo_NoPosAdjust },
	{ "C4CMD_MoveTo_PushTarget"   ,C4V_Int,     C4CMD_MoveTo_PushTarget },
	{ "C4CMD_Enter_PushTarget"    ,C4V_Int,     C4CMD_Enter_PushTarget },

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

	{ NULL, C4V_Any, 0} };

#define MkFnC4V (C4Value (*)(C4AulContext *cthr, C4Value*, C4Value*, C4Value*, C4Value*, C4Value*,\
                                                 C4Value*, C4Value*, C4Value*, C4Value*, C4Value*))

C4ScriptFnDef C4ScriptFnMap[]={

  { "goto",									1	,C4V_Any			,{ C4V_Int		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,0 ,                                   Fn_goto },
  { "this",									1	,C4V_C4Object	,{ C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,0 ,                                   Fn_this },
  { "Equal",								1	,C4V_Any			,{ C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,MkFnC4V FnEqual_C4V ,                   0 },
  { "Var",									1	,C4V_Any			,{ C4V_Int		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,MkFnC4V FnVar_C4V ,                   0 },
  { "AssignVar",						0	,C4V_Any			,{ C4V_Int		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,MkFnC4V FnSetVar_C4V ,                0 },
  { "SetVar",								1	,C4V_Any			,{ C4V_Int		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,MkFnC4V FnSetVar_C4V ,                0 },
  { "IncVar",								1	,C4V_Int			,{ C4V_Int		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,MkFnC4V FnIncVar_C4V ,                0 },
  { "DecVar",								1	,C4V_Int			,{ C4V_Int		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,MkFnC4V FnDecVar_C4V ,                0 },
  { "SetGlobal",						1	,C4V_Any			,{ C4V_Int		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,MkFnC4V FnSetGlobal_C4V ,             0 },
  { "Global",								1	,C4V_Any			,{ C4V_Int		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,MkFnC4V FnGlobal_C4V ,                0 },
  { "SetLocal",							1	,C4V_Any			,{ C4V_Int		,C4V_Any		,C4V_C4Object,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,MkFnC4V FnSetLocal_C4V ,              0 },
  { "Local",								1	,C4V_Any			,{ C4V_Int		,C4V_C4Object,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,MkFnC4V FnLocal_C4V ,                 0 },
  { "SetProperty",					1	,C4V_Any			,{ C4V_String	,C4V_Any		,C4V_PropList,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,MkFnC4V FnSetProperty_C4V ,           0 },
  { "GetProperty",					1	,C4V_Any			,{ C4V_String	,C4V_PropList,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,MkFnC4V FnGetProperty_C4V ,           0 },
  { "Explode",							1	,C4V_Bool			,{ C4V_Int		,C4V_C4Object,C4V_PropList,C4V_String	,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,0 ,                                   FnExplode },
  { "Incinerate",						1	,C4V_Bool			,{ C4V_C4Object,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,0 ,                                   FnIncinerate },
	{ "IncinerateLandscape",  1	,C4V_Bool			,{ C4V_Int    ,C4V_Int		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,0 ,                                   FnIncinerateLandscape },
  { "Extinguish",						1	,C4V_Bool			,{ C4V_C4Object,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,0 ,                                   FnExtinguish },
  { "GrabContents",					1	,C4V_Bool			,{ C4V_C4Object,C4V_C4Object,C4V_Any	,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,0 ,																		FnGrabContents },
  { "Punch",								1	,C4V_Bool			,{ C4V_C4Object,C4V_Int		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,0 ,                                   FnPunch },
  { "Kill",									1	,C4V_Bool			,{ C4V_C4Object,C4V_Bool	,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,0 ,                                   FnKill },
  { "Fling",								1	,C4V_Bool			,{ C4V_C4Object,C4V_Int		,C4V_Int		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,0 ,                                   FnFling },
  { "Jump",									1	,C4V_Bool			,{ C4V_C4Object,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,0 ,                                   FnJump },
  { "ChangeDef",						1	,C4V_Bool			,{ C4V_PropList,C4V_C4Object,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,0 ,                                   FnChangeDef },
  { "Exit",									1	,C4V_Bool			,{ C4V_C4Object,C4V_Int		,C4V_Int		,C4V_Int		,C4V_Int		,C4V_Int		,C4V_Int		,C4V_Any		,C4V_Any		,C4V_Any}  ,0 ,                                   FnExit },
  { "Enter",								1	,C4V_Bool			,{ C4V_C4Object,C4V_C4Object,C4V_Any	,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,0 ,                                   FnEnter },
  { "Collect",							1	,C4V_Bool			,{ C4V_C4Object,C4V_C4Object,C4V_Any	,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,0 ,                                   FnCollect },
  { "Split2Components",			1	,C4V_Bool			,{ C4V_C4Object,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,0 ,                                   FnSplit2Components },
  { "PlayerObjectCommand",	1	,C4V_Bool			,{ C4V_Int     ,C4V_String,C4V_C4Object,C4V_Any		,C4V_Int		,C4V_C4Object,C4V_Any		,C4V_Int		,C4V_Any		,C4V_Any}  ,0 ,                                   FnPlayerObjectCommand },
  { "SetCommand",						1	,C4V_Bool			,{ C4V_C4Object,C4V_String,C4V_C4Object,C4V_Any		,C4V_Int		,C4V_C4Object,C4V_Any		,C4V_Int		,C4V_Any		,C4V_Any}  ,0 ,                                   FnSetCommand },
  { "AddCommand",						1	,C4V_Bool			,{ C4V_C4Object,C4V_String,C4V_C4Object,C4V_Any		,C4V_Int		,C4V_C4Object,C4V_Int		,C4V_Any		,C4V_Int		,C4V_Int}  ,0 ,                                   FnAddCommand },
  { "AppendCommand",				1	,C4V_Bool			,{ C4V_C4Object,C4V_String,C4V_C4Object,C4V_Any		,C4V_Int		,C4V_C4Object,C4V_Int		,C4V_Any		,C4V_Int		,C4V_Int}  ,0 ,                                   FnAppendCommand },
  { "GetCommand",						1	,C4V_Any			,{ C4V_C4Object,C4V_Int		,C4V_Int		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,0 ,																		FnGetCommand },
//{ "FindConstructionSite",	0	,C4V_Bool			,{ C4V_C4ID		,C4V_Int		,C4V_Int		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,MkFnC4V FnFindConstructionSite ,    0 },
  { "PathFree2",						1	,C4V_Bool			,{ C4V_pC4Value,C4V_pC4Value,C4V_Int		,C4V_Int		,C4V_Any		,C4V_Any		,C4V_Any	,C4V_Any		,C4V_Any		,C4V_Any}  ,MkFnC4V FnPathFree2_C4V ,                  0 },
  { "DeathAnnounce",				1	,C4V_Bool			,{ C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,0 ,                                   FnDeathAnnounce },
  { "FindObject",						1	,C4V_C4Object	,{ C4V_PropList,C4V_Int		,C4V_Int		,C4V_Int		,C4V_Int		,C4V_Int		,C4V_String	,C4V_C4Object,C4V_Any		,C4V_C4Object}  ,0 ,                            FnFindObject },
  { "FindObject2",					1	,C4V_C4Object	,{ C4V_Array	,C4V_Array	,C4V_Array	,C4V_Array	,C4V_Array	,C4V_Array	,C4V_Array	,C4V_Array	,C4V_Array	,C4V_Array},0 ,																		FnFindObject2 },
  { "FindObjects",					1	,C4V_Array		,{ C4V_Array	,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,0 ,																		FnFindObjects },
  { "ObjectCount",					1	,C4V_Int			,{ C4V_PropList,C4V_Int		,C4V_Int		,C4V_Int		,C4V_Int		,C4V_Int		,C4V_String	,C4V_C4Object,C4V_Any		,C4V_Int},0 ,																		FnObjectCount },
  { "ObjectCount2",					1	,C4V_Int			,{ C4V_Array	,C4V_Array	,C4V_Array	,C4V_Array	,C4V_Array	,C4V_Array	,C4V_Array	,C4V_Array	,C4V_Array	,C4V_Array},0 ,																		FnObjectCount2 },
  { "ObjectCall",						1	,C4V_Any			,{ C4V_C4Object,C4V_String,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,MkFnC4V FnObjectCall_C4V ,            0 },
  { "ProtectedCall",				1	,C4V_Any			,{ C4V_C4Object,C4V_String,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,MkFnC4V FnProtectedCall_C4V ,         0 },
  { "PrivateCall",					1	,C4V_Any			,{ C4V_C4Object,C4V_String,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,MkFnC4V FnPrivateCall_C4V ,           0 },
  { "GameCall",							1	,C4V_Any			,{ C4V_String	,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,MkFnC4V FnGameCall_C4V ,              0 },
  { "GameCallEx",						1	,C4V_Any			,{ C4V_String	,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,MkFnC4V FnGameCallEx_C4V ,            0 },
  { "DefinitionCall",				1	,C4V_Any			,{ C4V_PropList,C4V_String	,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,MkFnC4V FnDefinitionCall_C4V ,        0 },
  { "Call",									0	,C4V_Any			,{ C4V_String	,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,MkFnC4V FnCall_C4V ,                  0 },
  { "GetPlrKnowledge",			1	,C4V_Int			,{ C4V_Int		,C4V_PropList,C4V_Int		,C4V_Int		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,MkFnC4V FnGetPlrKnowledge_C4V ,       0 },
  { "GetPlrMagic",					1	,C4V_Int			,{ C4V_Int		,C4V_PropList,C4V_Int		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,MkFnC4V FnGetPlrMagic_C4V ,           0 },
  { "GetComponent",					1	,C4V_Int			,{ C4V_PropList,C4V_Int		,C4V_C4Object,C4V_PropList,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,MkFnC4V FnGetComponent_C4V ,          0 },
	{ "PlayerMessage",				1	,C4V_Int			,{ C4V_Int		,C4V_String	,C4V_C4Object,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,MkFnC4V &FnPlayerMessage_C4V,					0 },
	{ "Message",							1	,C4V_Bool			,{ C4V_String	,C4V_C4Object,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,MkFnC4V &FnMessage_C4V,								0 },
	{ "AddMessage",						1	,C4V_Bool			,{ C4V_String	,C4V_C4Object,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,MkFnC4V &FnAddMessage_C4V,						0 },
	{ "PlrMessage",						1	,C4V_Bool			,{ C4V_String	,C4V_Int		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,MkFnC4V &FnPlrMessage_C4V,						0 },
	{ "Log",									1	,C4V_Bool			,{ C4V_String	,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,MkFnC4V &FnLog_C4V,										0 },
	{ "DebugLog",							1	,C4V_Bool			,{ C4V_String	,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,MkFnC4V &FnDebugLog_C4V,							0 },
	{ "Format",								1	,C4V_String		,{ C4V_String	,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,MkFnC4V &FnFormat_C4V,								0 },
	{ "EditCursor",						1	,C4V_C4Object	,{ C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,0 ,                                   FnEditCursor },
	{ "AddMenuItem",					1	,C4V_Bool			,{ C4V_String	,C4V_String	,C4V_PropList,C4V_C4Object,C4V_Int		,C4V_Any		,C4V_String	,C4V_Int		,C4V_Any		,C4V_Any}  ,0 ,														        FnAddMenuItem },
	{ "SetSolidMask",					1	,C4V_Bool			,{ C4V_Int		,C4V_Int		,C4V_Int		,C4V_Int		,C4V_Int		,C4V_Int		,C4V_C4Object,C4V_Any		,C4V_Any		,C4V_Any}  ,0 ,                                   FnSetSolidMask },
	{ "SetGravity",						1	,C4V_Bool			,{ C4V_Int		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,0 ,                                   FnSetGravity },
	{ "GetGravity",						1	,C4V_Int			,{ C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,0 ,                                   FnGetGravity },
	{ "GetHomebaseMaterial",	1	,C4V_Int			,{ C4V_Int		,C4V_PropList,C4V_Int		,C4V_Int		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,MkFnC4V FnGetHomebaseMaterial_C4V ,   0 },
	{ "GetHomebaseProduction",	1	,C4V_Int			,{ C4V_Int		,C4V_PropList,C4V_Int		,C4V_Int		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}  ,MkFnC4V FnGetHomebaseProduction_C4V ,   0 },

	{ "Set",									1	,C4V_Any			,{ C4V_pC4Value,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}	 ,MkFnC4V FnSet,                        0 },
	{ "Inc",									1	,C4V_Any			,{ C4V_pC4Value,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}	 ,MkFnC4V FnInc,                        0 },
	{ "Dec",									1	,C4V_Any			,{ C4V_pC4Value,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}	 ,MkFnC4V FnDec,                        0 },
	{ "IsRef",								1	,C4V_Bool			,{ C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}	 ,MkFnC4V FnIsRef,                      0 },
	{ "GetType",							1	,C4V_Int			,{ C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}	 ,MkFnC4V FnGetType,                    0 },

	{ "CreateArray",					1	,C4V_Array		,{ C4V_Int		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}	 ,0,                                    FnCreateArray },
	{ "GetLength",						1	,C4V_Int			,{ C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}	 ,0,                                    FnGetLength },
	{ "GetIndexOf",						1	,C4V_Int			,{ C4V_Any		,C4V_Array	,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}	 ,0,                                    FnGetIndexOf },
	{ "SetLength",						1	,C4V_Bool			,{ C4V_pC4Value,C4V_Int		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}	 ,0,                                    FnSetLength },


	{ "GetDefCoreVal",				1	,C4V_Any			,{ C4V_String	,C4V_String	,C4V_PropList,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}	 ,MkFnC4V FnGetDefCoreVal,              0 },
	{ "GetObjectVal",					1	,C4V_Any			,{ C4V_String	,C4V_String ,C4V_C4Object,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}	 ,MkFnC4V FnGetObjectVal,               0 },
	{ "GetObjectInfoCoreVal",	1	,C4V_Any			,{ C4V_String	,C4V_String	,C4V_C4Object,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}	 ,MkFnC4V FnGetObjectInfoCoreVal,       0 },
	{ "GetScenarioVal",				1	,C4V_Any			,{ C4V_String	,C4V_String	,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}	 ,MkFnC4V FnGetScenarioVal,             0 },
//{ "SetScenarioVal",				1	,C4V_Bool			,{ C4V_String	,C4V_String	,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}	 ,MkFnC4V FnSetScenarioVal,             0 },
	{ "GetPlayerVal",					1	,C4V_Any			,{ C4V_String	,C4V_String	,C4V_Int		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}	 ,MkFnC4V FnGetPlayerVal,               0 },
	{ "GetPlayerInfoCoreVal",	1	,C4V_Any			,{ C4V_String	,C4V_String	,C4V_Int		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}	 ,MkFnC4V FnGetPlayerInfoCoreVal,       0 },
	{ "GetMaterialVal",				1	,C4V_Any			,{ C4V_String	,C4V_String	,C4V_Int		,C4V_Int		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}	 ,MkFnC4V FnGetMaterialVal,             0 },

	{ "SetPlrExtraData",			1	,C4V_Any			,{ C4V_Int		,C4V_String	,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}	 ,MkFnC4V FnSetPlrExtraData,            0 },
	{ "GetPlrExtraData",			1	,C4V_Any			,{ C4V_Int		,C4V_String	,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}	 ,MkFnC4V FnGetPlrExtraData,            0 },
	{ "SetCrewExtraData",			1	,C4V_Any			,{ C4V_C4Object,C4V_String,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}	 ,MkFnC4V FnSetCrewExtraData,           0 },
	{ "GetCrewExtraData",			1	,C4V_Any			,{ C4V_C4Object,C4V_String,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}	 ,MkFnC4V FnGetCrewExtraData,           0 },
	{ "SimFlight",						1	,C4V_Bool			,{ C4V_pC4Value,C4V_pC4Value,C4V_pC4Value,C4V_pC4Value,C4V_Int,C4V_Int		,C4V_Int		,C4V_Int		,C4V_Any		,C4V_Any}	 ,MkFnC4V FnSimFlight,                  0 },
	{ "GetPortrait",          1	,C4V_Any			,{ C4V_C4Object,C4V_Bool	,C4V_Bool		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}	 ,MkFnC4V FnGetPortrait,                0 },
	{ "AddEffect",            1	,C4V_Int			,{ C4V_String	,C4V_C4Object,C4V_Int		,C4V_Int		,C4V_C4Object,C4V_PropList,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}	 ,MkFnC4V FnAddEffect_C4V,              0 },
	{ "GetEffect",            1	,C4V_Any			,{ C4V_String	,C4V_C4Object,C4V_Int		,C4V_Int		,C4V_Int		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}	 ,MkFnC4V FnGetEffect_C4V,              0 },
	{ "CheckEffect",          1	,C4V_Int			,{ C4V_String	,C4V_C4Object,C4V_Int		,C4V_Int	  ,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}	 ,MkFnC4V FnCheckEffect_C4V,            0 },
	{ "EffectCall",           1	,C4V_Any			,{ C4V_C4Object,C4V_Int		,C4V_String	,C4V_Any    ,C4V_Any  	,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any    ,C4V_Any}	 ,MkFnC4V FnEffectCall_C4V,             0 },
	{ "EffectVar",            1	,C4V_pC4Value	,{ C4V_Int		,C4V_C4Object,C4V_Int		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}	 ,MkFnC4V FnEffectVar_C4V,              0 },

	{ "eval",									1	,C4V_Any			,{ C4V_String	,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}	 ,MkFnC4V FnEval,                       0 },

	{ "VarN",									1	,C4V_Any			,{ C4V_String	,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}	 ,MkFnC4V FnVarN,                       0 },
	{ "LocalN",								1	,C4V_Any			,{ C4V_String	,C4V_C4Object,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}	 ,MkFnC4V FnLocalN,                     0 },
	{ "GlobalN",							1	,C4V_Any			,{ C4V_String	,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}	 ,MkFnC4V FnGlobalN,                    0 },

	{ NULL,										0	,C4V_Any			,{ C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any		,C4V_Any}	 ,0,																	 	0 }

  };
