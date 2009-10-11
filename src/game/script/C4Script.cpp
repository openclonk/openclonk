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

	if (!pObj) return false;

	// No recursion, flag check
	if (rEnergyChainChecked.GetLink(pObj)) return false;
	rEnergyChainChecked.Add(pObj, C4ObjectList::stNone);

	// This object needs energy
	if (pObj->Def->LineConnect & C4D_Power_Consumer)
		if (pObj->NeedEnergy)
			return true;

	// Check all power line connected structures
  C4Object *cline; C4ObjectLink *clnk;
  for (clnk=::Objects.First; clnk && (cline=clnk->Obj); clnk=clnk->Next)
		if (cline->Status) if (cline->Def->id==C4ID_PowerLine)
			if (cline->Action.Target==pObj)
				if (CheckEnergyNeedChain(cline->Action.Target2,rEnergyChainChecked))
					return true;

	return false;
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

// Allow parameters to be nil
template<typename T>
class Nillable
{
	bool _nil;
	T _val;
public:
	inline Nillable(const T &value) : _nil(!value && !C4Value::IsNullableType(C4ValueConv<T>::Type())), _val(value) {}
	inline Nillable(const C4NullValue &) : _nil(true), _val(T()) {}
	inline bool IsNil() const { return _nil; }
	inline operator T() const { return _val; }
	inline Nillable<T> &operator =(const T &val)
	{
		_val = val;
		_nil = !val && !C4Value::IsNullableType(C4ValueConv<T>::Type());
		return *this;
	}
	inline Nillable<T> &operator =(const Nillable<T> &val)
	{
		_val = val._val;
		_nil = val._nil;
		return *this;
	}
	// Some operators
	inline Nillable<T> &operator ++() { ++_val; return *this; }
	inline T operator ++(int) { T v(_val++); return v; }
	inline Nillable<T> &operator --() { --_val; return *this; }
	inline T operator --(int) { T v(_val--); return v; }
};
template<typename T>
struct C4ValueConv<Nillable<T> >
{
	inline static Nillable<T> FromC4V(C4Value &v) { if (v.GetType() == C4V_Any) return C4VNull; else return C4ValueConv<T>::FromC4V(v); }
	inline static Nillable<T> _FromC4V(C4Value &v) { if (v.GetType() == C4V_Any) return C4VNull; else return C4ValueConv<T>::_FromC4V(v); }
	inline static C4V_Type Type() { return C4ValueConv<T>::Type(); }
	inline static C4Value ToC4V(const Nillable<T> &v) { if(v.IsNil()) return C4VNull; else return C4ValueConv<T>::ToC4V(v.operator T()); }
};
template<>
class Nillable<void>
{
public:
	inline Nillable(const C4NullValue &) {}
	inline bool IsNil() const { return true; }
};
template<>
struct C4ValueConv<Nillable<void> >
{
	inline static C4V_Type Type() { return C4VNull.GetType(); }
	inline static C4Value ToC4V(const Nillable<void> &) { return C4VNull; }
};

// Some functions require object context.
// Using this type instead of C4AulContext guarantees validity of Obj member.
// Don't add any members to this class, or breakage will occur.
class C4AulObjectContext : public C4AulContext {};

// Some functions are callable in definition context only.
// This exception gets thrown if they are called from anywhere else.
class NeedDefinitionContext : public C4AulExecError
{
public:
	NeedDefinitionContext(const char *function) : C4AulExecError(NULL, FormatString("%s: must be called from definition context", function).getData()) {}
};
// Other functions are callable in object context only.
// This exception gets thrown if they are called from anywhere else.
class NeedObjectContext : public C4AulExecError
{
public:
	NeedObjectContext(const char *function) : C4AulExecError(NULL, FormatString("%s: must be called from object context", function).getData()) {}
};
// Then there's functions that don't care, but need either defn or object context.
// This exception gets thrown if those are called from global scripts.
class NeedNonGlobalContext : public C4AulExecError
{
public:
	NeedNonGlobalContext(const char *function) : C4AulExecError(NULL, FormatString("%s: call must not be from global context", function).getData()) {}
};

// return type of functions returning nil
typedef Nillable<void> C4Void;

//=============================== C4Script Functions ====================================

static C4Object *Fn_this(C4AulContext *cthr)
{
	return cthr->Obj;
}

static C4Void Fn_goto(C4AulContext *cthr, long iCounter)
{
	Game.Script.Counter=iCounter;
	return C4VNull;
}

static bool FnChangeDef(C4AulObjectContext *cthr, C4ID to_id)
{
	return !!cthr->Obj->ChangeDef(to_id);
}

static C4Void FnExplode(C4AulObjectContext *cthr, long iLevel, C4ID idEffect, C4String *szEffect)
{
	cthr->Obj->Explode(iLevel, idEffect, FnStringPar(szEffect));
	return C4VNull;
}

static bool FnIncinerate(C4AulObjectContext *cthr, Nillable<long> causedBy)
{
	long iCausedBy = causedBy;
	if (causedBy.IsNil()) iCausedBy = cthr->Obj->Controller;

	return !!cthr->Obj->Incinerate(iCausedBy);
}

static bool FnIncinerateLandscape(C4AulContext *cthr, long iX, long iY)
{
	if (cthr->Obj) { iX += cthr->Obj->GetX(); iY += cthr->Obj->GetY(); }
	return !!::Landscape.Incinerate(iX, iY);
}

static bool FnExtinguish(C4AulObjectContext *cthr)
{
	// extinguish all fires
	return !!cthr->Obj->Extinguish(0);
}

static C4Void FnSetSolidMask(C4AulObjectContext *cthr, long iX, long iY, long iWdt, long iHgt, long iTX, long iTY)
{
	cthr->Obj->SetSolidMask(iX,iY,iWdt,iHgt,iTX,iTY);
	return C4VNull;
}

static C4Void FnSetGravity(C4AulContext *cthr, long iGravity)
{
	::Landscape.Gravity = itofix(BoundBy<long>(iGravity,-300,300)) / 500;
	return C4VNull;
}

static long FnGetGravity(C4AulContext *cthr)
{
	return fixtoi(::Landscape.Gravity * 500);
}

static C4Void FnDeathAnnounce(C4AulObjectContext *cthr)
{
	const long MaxDeathMsg=7;
	// Check if crew member has an own death message
	*pscOSTR=0;
	char *szMsg;
	if (cthr->Obj->Info)
		if(*(szMsg = cthr->Obj->Info->DeathMessage))
			sprintf(pscOSTR, "%s", szMsg);
	if (!*pscOSTR)
	{
		char idDeathMsg[128+1]; sprintf(idDeathMsg, "IDS_OBJ_DEATH%d", 1 + SafeRandom(MaxDeathMsg));
		sprintf(pscOSTR,LoadResStr(idDeathMsg) ,cthr->Obj->GetName());
	}
	if (!Game.C4S.Head.Film)
		GameMsgObject(pscOSTR,cthr->Obj);

	return C4VNull;
}

static bool FnGrabContents(C4AulObjectContext *cthr, C4Object *from)
{
	if (!from) return false;
	if (cthr->Obj == from) return false;
	cthr->Obj->GrabContents(from);
	return true;
}

static bool FnPunch(C4AulObjectContext *cthr, C4Object *target, long punch)
{
	if (!target) return false;
	return !!ObjectComPunch(cthr->Obj,target,punch);
}

static bool FnKill(C4AulContext *cthr, C4Object *pObj, bool fForced)
{
	if (!pObj) pObj=cthr->Obj; if (!pObj) return false;
	if (!pObj->GetAlive()) return false;
	// Trace kills by player-owned objects
	// Do not trace for NO_OWNER, because that would include e.g. the Suicide-rule
	if (cthr->Obj && ValidPlr(cthr->Obj->Controller)) pObj->UpdatLastEnergyLossCause(cthr->Obj->Controller);
	// Do the kill
	pObj->AssignDeath(!!fForced);
	return true;
}

static C4Void FnFling(C4AulObjectContext *cthr, long iXDir, long iYDir, long iPrec, bool fAddSpeed)
{
	if (!iPrec) iPrec=1;
	cthr->Obj->Fling(itofix(iXDir, iPrec),itofix(iYDir, iPrec),fAddSpeed);
	// unstick from ground, because Fling command may be issued in an Action-callback,
	// where attach-values have already been determined for that frame
	cthr->Obj->Action.t_attach=0;
	return C4VNull;
}

static bool FnJump(C4AulObjectContext *cthr)
{
	return !!ObjectComJump(cthr->Obj);
}

static bool FnEnter(C4AulObjectContext *cthr, C4Object *pTarget)
{
	return !!cthr->Obj->Enter(pTarget);
}

static bool FnExit(C4AulObjectContext *cthr, long tx, long ty, long tr, long txdir, long tydir, long trdir)
{
	tx+=cthr->Obj->GetX();
	ty+=cthr->Obj->GetY();
	ObjectComCancelAttach(cthr->Obj);
	return !!cthr->Obj->Exit(tx,
		ty+cthr->Obj->Shape.y,
		tr,
		itofix(txdir),itofix(tydir),
		itofix(trdir) / 10);
}

static bool FnCollect(C4AulObjectContext *cthr, C4Object *pItem)
{
	// local call / safety
	if (!pItem) return false;
	// check OCF of collector (MaxCarry)
	if (cthr->Obj->OCF & OCF_Collection)
		// collect
		return !!cthr->Obj->Collect(pItem);
	// failure
	return false;
}

static C4Void FnRemoveObject(C4AulObjectContext *cthr, bool fEjectContents)
{
	cthr->Obj->AssignRemoval(fEjectContents);
	return C4VNull;
}

static C4Void FnSetPosition(C4AulObjectContext *cthr, long iX, long iY, bool fCheckBounds)
{
	if (fCheckBounds)
	{
		// BoundsCheck takes ref to int and not to long
		int32_t i_x = iX, i_y = iY;
		cthr->Obj->BoundsCheck(i_x, i_y);
		iX = i_x; iY = i_y;
	}
	cthr->Obj->ForcePosition(iX,iY);
	// update liquid
	cthr->Obj->UpdateInLiquid();
	return C4VNull;
}

static C4Void FnDoCon(C4AulObjectContext *cthr, long iChange) // in percent
{
	cthr->Obj->DoCon(FullCon*iChange/100);
	return C4VNull;
}

static long FnGetCon(C4AulObjectContext *cthr) // in percent
{
	return 100*cthr->Obj->GetCon()/FullCon;
}

static C4Void FnDoEnergy(C4AulObjectContext *cthr, long iChange, bool fExact, Nillable<long> iEngType, Nillable<long> iCausedBy)
{
	if (iEngType.IsNil()) iEngType = C4FxCall_EngScript;
	if (iCausedBy.IsNil())
		//if (cthr->Caller && cthr->Caller->Obj)
		//	iCausedBy = cthr->Caller->Obj->Controller;
		//else
			iCausedBy = NO_OWNER;
	cthr->Obj->DoEnergy(iChange, fExact, iEngType, iCausedBy);
	return C4VNull;
}

static C4Void FnDoBreath(C4AulObjectContext *cthr, long iChange)
{
	cthr->Obj->DoBreath(iChange);
	return C4VNull;
}

static C4Void FnDoDamage(C4AulObjectContext *cthr, long iChange, Nillable<long> iDmgType, Nillable<long> iCausedBy)
{
	if (iDmgType.IsNil()) iDmgType = C4FxCall_DmgScript;
	if (iCausedBy.IsNil())
		//if (cthr->Caller && cthr->Caller->Obj)
		//	iCausedBy = cthr->Caller->Obj->Controller;
		//else
			iCausedBy = NO_OWNER;
	cthr->Obj->DoDamage(iChange,iCausedBy, iDmgType);
	return C4VNull;
}

static bool FnDoMagicEnergy(C4AulObjectContext *cthr, long iChange, bool fAllowPartial)
{
	// Physical modification factor
	iChange *= MagicPhysicalFactor;
	// Maximum load
	if (iChange>0)
		if (cthr->Obj->MagicEnergy+iChange>cthr->Obj->GetPhysical()->Magic)
		{
			if (!fAllowPartial) return false;
			iChange = cthr->Obj->GetPhysical()->Magic - cthr->Obj->MagicEnergy;
			if (!iChange) return false;
			// partial change to max allowed
		}
	// Insufficient load
	if (iChange<0)
		if (cthr->Obj->MagicEnergy+iChange<0)
		{
			if (!fAllowPartial) return false;
			iChange = -cthr->Obj->MagicEnergy;
			if (!iChange) return false;
			// partial change to zero allowed
		}
	// Change energy level
	cthr->Obj->MagicEnergy=BoundBy<long>(cthr->Obj->MagicEnergy+iChange,0,cthr->Obj->GetPhysical()->Magic);
	cthr->Obj->ViewEnergy = C4ViewDelay;
	return true;
}

static long FnGetMagicEnergy(C4AulObjectContext *cthr)
{
	return cthr->Obj->MagicEnergy/MagicPhysicalFactor;
}

enum PhysicalMode
{
	PHYS_Current = 0,
	PHYS_Permanent = 1,
	PHYS_Temporary = 2,
	PHYS_StackTemporary = 3
};

static bool FnSetPhysical(C4AulObjectContext *cthr, C4String *szPhysical, long iValue, long iMode)
{
	// Get physical offset
	C4PhysicalInfo::Offset off;
	if (!C4PhysicalInfo::GetOffsetByName(FnStringPar(szPhysical), &off)) return false;
	// Set by mode
	switch (static_cast<PhysicalMode>(iMode)) // Cast so compiler may warn when new modes are added but not handled
	{
	// Currently active physical
	case PHYS_Current:
		// Info objects or temporary mode only
		if (!cthr->Obj->PhysicalTemporary) if (!cthr->Obj->Info || Game.Parameters.UseFairCrew) return false;
		// Set physical
		cthr->Obj->GetPhysical()->*off = iValue;
		return true;
	// Permanent physical
	case PHYS_Permanent:
		// Info objects only
		if (!cthr->Obj->Info) return false;
		// In fair crew mode, changing the permanent physicals is only allowed via TrainPhysical
		// Otherwise, stuff like SetPhysical(..., GetPhysical(...)+1, ...) would screw up the crew in fair crew mode
		if (Game.Parameters.UseFairCrew) return false;
		// Set physical
		cthr->Obj->Info->Physical.*off = iValue;
		return true;
	// Temporary physical
	case PHYS_Temporary:
	case PHYS_StackTemporary:
		// Automatically switch to temporary mode
		if (!cthr->Obj->PhysicalTemporary)
			{
			cthr->Obj->TemporaryPhysical = *(cthr->Obj->GetPhysical());
			cthr->Obj->PhysicalTemporary = true;
			}
		// if old value is to be remembered, register the change
		if (iMode == PHYS_StackTemporary)
			cthr->Obj->TemporaryPhysical.RegisterChange(off);
		// Set physical
		cthr->Obj->TemporaryPhysical.*off = iValue;
		return true;
	}
	// Invalid mode
	throw new C4AulExecError(cthr->Obj, FormatString("SetPhysical: invalid physical mode %i", iMode).getData());
}

static bool FnTrainPhysical(C4AulObjectContext *cthr, C4String *szPhysical, long iTrainBy, long iMaxTrain)
{
	// Get physical offset
	C4PhysicalInfo::Offset off;
	if (!C4PhysicalInfo::GetOffsetByName(FnStringPar(szPhysical), &off)) return false;
	// train it
	return cthr->Obj->TrainPhysical(off, iTrainBy, iMaxTrain);
}

static bool FnResetPhysical(C4AulObjectContext *cthr, C4String *sPhysical)
{
	const char *szPhysical = FnStringPar(sPhysical);

	// Reset to permanent physical
	if (!cthr->Obj->PhysicalTemporary) return false;

	// reset specified physical only?
	if (szPhysical && *szPhysical)
	{
		C4PhysicalInfo::Offset off;
		if (!C4PhysicalInfo::GetOffsetByName(szPhysical, &off)) return false;
		if (!cthr->Obj->TemporaryPhysical.ResetPhysical(off)) return false;
		// if other physical changes remain, do not reset complete physicals
		if (cthr->Obj->TemporaryPhysical.HasChanges(cthr->Obj->GetPhysical(true))) return true;
	}

	// actual reset of temp physicals
	cthr->Obj->PhysicalTemporary = false;
	cthr->Obj->TemporaryPhysical.Default();

	return true;
}

static Nillable<long> FnGetPhysical(C4AulContext *cthr, C4String *szPhysical, long iMode)
{
	// Get physical offset
	C4PhysicalInfo::Offset off;
	if (!C4PhysicalInfo::GetOffsetByName(FnStringPar(szPhysical), &off)) return C4VNull;
	// no object context?
	if (!cthr->Obj)
	{
		// def given?
		if (cthr->Def)
		{
			// get def
			// return physical value
			return cthr->Def->Physical.*off;
		}
		return C4VNull;
	}

	assert(cthr->Obj);
	// Get by mode
	switch (static_cast<PhysicalMode>(iMode))
	{
	// Currently active physical
	case PHYS_Current:
		// Get physical
		return cthr->Obj->GetPhysical()->*off;
	// Permanent physical
	case PHYS_Permanent:
		// Info objects only
		if (!cthr->Obj->Info) return C4VNull;
		// In fair crew mode, scripts may not read permanent physical values - fallback to fair def physical instead!
		if (Game.Parameters.UseFairCrew)
			if (cthr->Obj->Info->pDef)
				return cthr->Obj->Info->pDef->GetFairCrewPhysicals()->*off;
			else
				return cthr->Obj->Def->GetFairCrewPhysicals()->*off;
		// Get physical
		return cthr->Obj->Info->Physical.*off;
	// Temporary physical
	case PHYS_Temporary:
		// Info objects only
		if (!cthr->Obj->Info) return C4VNull;
		// Only if in temporary mode
		if (!cthr->Obj->PhysicalTemporary) return C4VNull;
		// Get physical
		return cthr->Obj->TemporaryPhysical.*off;
	}
	// Invalid mode
	throw new C4AulExecError(cthr->Obj, FormatString("GetPhysical: invalid physical mode %i", iMode).getData());
}

static C4Void FnSetEntrance(C4AulObjectContext *cthr, bool e_status)
{
	cthr->Obj->EntranceStatus = e_status;
	return C4VNull;
}


static C4Void FnSetXDir(C4AulObjectContext *cthr, long nxdir, long iPrec)
{
	// precision (default 10.0)
	if (!iPrec) iPrec=10;
	// update xdir
	cthr->Obj->xdir=itofix(nxdir, iPrec);
	// special: negative dirs must be rounded
	//if (nxdir<0) pObj->xdir += FIXED100(-50)/iPrec;
	cthr->Obj->Mobile=1;
	// success
	return C4VNull;
}

static C4Void FnSetRDir(C4AulObjectContext *cthr, long nrdir, long iPrec)
{
	// precision (default 10.0)
	if (!iPrec) iPrec=10;
	// update rdir
	cthr->Obj->rdir=itofix(nrdir, iPrec);
	// special: negative dirs must be rounded
	//if (nrdir<0) pObj->rdir += FIXED100(-50)/iPrec;
	cthr->Obj->Mobile=1;
	// success
	return C4VNull;
}

static C4Void FnSetYDir(C4AulObjectContext *cthr, long nydir, long iPrec)
{
	// precision (default 10.0)
	if (!iPrec) iPrec=10;
	// update ydir
	cthr->Obj->ydir=itofix(nydir, iPrec);
	// special: negative dirs must be rounded
	//if (nydir<0) pObj->ydir += FIXED100(-50)/iPrec;
	cthr->Obj->Mobile=1;
	return C4VNull;
}

static C4Void FnSetR(C4AulObjectContext *cthr, long nr)
{
	// set rotation
	cthr->Obj->SetRotation(nr);
	// success
	return C4VNull;
}

static bool FnSetAction(C4AulObjectContext *cthr, C4String *szAction,
								 C4Object *pTarget, C4Object *pTarget2, bool fDirect)
{
	if (!szAction) return false;
	return !!cthr->Obj->SetActionByName(FnStringPar(szAction),pTarget,pTarget2,
		C4Object::SAC_StartCall | C4Object::SAC_AbortCall,!!fDirect);
}

static bool FnSetBridgeActionData(C4AulObjectContext *cthr, long iBridgeLength, bool fMoveClonk, bool fWall, long iBridgeMaterial)
{
	if (!cthr->Obj->Status) return false;
	// action must be BRIDGE
	if (!cthr->Obj->Action.pActionDef) return false;
	if (cthr->Obj->Action.pActionDef->GetPropertyInt(P_Procedure) != DFA_BRIDGE) return false;
	// set data
	cthr->Obj->Action.SetBridgeData(iBridgeLength, fMoveClonk, fWall, iBridgeMaterial);
	return true;
}

static bool FnSetActionData(C4AulObjectContext *cthr, long iData)
{
	if (!cthr->Obj->Status) return false;
	// bridge: Convert from old style
	if (cthr->Obj->Action.pActionDef && (cthr->Obj->Action.pActionDef->GetPropertyInt(P_Procedure) == DFA_BRIDGE))
		return FnSetBridgeActionData(cthr, 0, false, false, iData);
	// attach: check for valid vertex indices
	if (cthr->Obj->Action.pActionDef && (cthr->Obj->Action.pActionDef->GetPropertyInt(P_Procedure) == DFA_ATTACH)) // Fixed Action.Act check here... matthes
		if (((iData&255) >= C4D_MaxVertex) || ((iData>>8) >= C4D_MaxVertex))
			return false;
	// set data
	cthr->Obj->Action.Data = iData;
	return true;
}

static C4Void FnSetComDir(C4AulObjectContext *cthr, long ncomdir)

static C4Void FnSetComDir(C4AulObjectContext *cthr, long ncomdir)
{
	cthr->Obj->Action.ComDir=ncomdir;
	return C4VNull;
}

static C4Void FnSetDir(C4AulObjectContext *cthr, long ndir)
{
	cthr->Obj->SetDir(ndir);
	return C4VNull;
}

static C4Void FnSetCategory(C4AulObjectContext *cthr, long iCategory)
{
	if (!(iCategory & C4D_SortLimit)) iCategory |= (cthr->Obj->Category & C4D_SortLimit);
	cthr->Obj->SetCategory(iCategory);
	return C4VNull;
}

static C4Void FnSetAlive(C4AulObjectContext *cthr, bool nalv)
{
	cthr->Obj->SetAlive(nalv);
	return C4VNull;
}

static bool FnSetOwner(C4AulObjectContext *cthr, long iOwner)
{
	// Set owner
	return !!cthr->Obj->SetOwner(iOwner);
}

static bool FnSetPhase(C4AulObjectContext *cthr, long iVal)
{
	return !!cthr->Obj->SetPhase(iVal);
}

static bool FnExecuteCommand(C4AulObjectContext *cthr)
{
	return !!cthr->Obj->ExecuteCommand();
}

static C4Value FnSetCommand(C4AulContext *cthr, C4Value *pPars)
{
	PAR(string, szCommand); PAR(object, pTarget); PAR(any, Tx); PAR(int, iTy);
	PAR(object, pTarget2); PAR(any, Data); PAR(int, iRetries);
	// Object
	if (!cthr->Obj)
		throw NeedObjectContext("SetCommand");
	// Command
	if (!szCommand) return C4VFalse;
	long iCommand = CommandByName(FnStringPar(szCommand));
	if (!iCommand) { cthr->Obj->ClearCommands(); return C4VFalse; }
	// Special: convert iData to szText
	C4String *szText=NULL;
	if (iCommand==C4CMD_Call)
		szText=Data.getStr();
	else
		Tx.ConvertTo(C4V_Int);
	// Set
	cthr->Obj->SetCommand(iCommand,pTarget,Tx,iTy,pTarget2,false,Data,iRetries,szText);
	// Success
	return C4VTrue;
}

static C4Value FnAddCommand(C4AulContext *cthr, C4Value *pPars)
{
	PAR(string, szCommand); PAR(object, pTarget); PAR(any, Tx); PAR(int, iTy);
	PAR(object, pTarget2); PAR(int, iUpdateInterval); PAR(any, Data); PAR(int, iRetries); PAR(int, iBaseMode);
	// Object
	if (!cthr->Obj)
		throw NeedObjectContext("AddCommand");
	// Command
	if (!szCommand) return C4VFalse;
	long iCommand = CommandByName(FnStringPar(szCommand));
	if (!iCommand) return C4VFalse;
	// Special: convert iData to szText
	C4String *szText=NULL;
	if (iCommand==C4CMD_Call)
		szText=Data.getStr();
	else
		Tx.ConvertTo(C4V_Int);
	// Add
	return C4VBool(cthr->Obj->AddCommand(iCommand,pTarget,Tx,iTy,iUpdateInterval,pTarget2,true,Data,false,iRetries,szText,iBaseMode));
}

static C4Value FnAppendCommand(C4AulContext *cthr, C4Value *pPars)
{
	PAR(string, szCommand); PAR(object, pTarget); PAR(any, Tx); PAR(int, iTy);
	PAR(object, pTarget2); PAR(int, iUpdateInterval); PAR(any, Data); PAR(int, iRetries); PAR(int, iBaseMode);
	// Object
	if (!cthr->Obj)
		throw NeedObjectContext("AppendCommand");
	// Command
	if (!szCommand) return C4VFalse;
	long iCommand = CommandByName(FnStringPar(szCommand));
	if (!iCommand) return C4VFalse;
	// Special: convert iData to szText
	C4String *szText=NULL;
	if (iCommand==C4CMD_Call)
		szText=Data.getStr();
	else
		Tx.ConvertTo(C4V_Int);
	// Add
	return C4VBool(cthr->Obj->AddCommand(iCommand,pTarget,Tx,iTy,iUpdateInterval,pTarget2,true,Data,true,iRetries,szText,iBaseMode));
}

static C4Value FnGetCommand(C4AulContext *cthr, C4Value *pPars)
	{
	PAR(int, iElement); PAR(int, iCommandNum);

	// safety
	if (!cthr->Obj)
		throw NeedObjectContext("GetCommand");

	C4Command * Command = cthr->Obj->Command;
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

static bool FnFinishCommand(C4AulObjectContext *cthr, bool fSuccess, long iCommandNum)
{
	C4Command * Command = cthr->Obj->Command;
	// Move through list to Command iCommandNum
	while (Command && iCommandNum--) Command = Command->Next;
	// Object has no command or iCommandNum was to high or < 0
	if (!Command) return false;
	if (!fSuccess) ++(Command->Failures);
	else Command->Finished = true;
	return true;
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

static C4String *FnGetAction(C4AulObjectContext *cthr)
{
	if (!cthr->Obj->Action.pActionDef) return String("Idle");
	return String(cthr->Obj->Action.pActionDef->GetName());
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
	if (!pObj) return C4VNull;
	C4String * key = key_C4V->_getStr();
	if(!key) return C4VNull;
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

static C4String *FnGetName(C4AulContext *cthr)
{
	if (!cthr->Obj)
		if (!cthr->Def)
			throw new NeedNonGlobalContext("GetName");
		else
			return String(cthr->Def->GetName());
	else
		return String(cthr->Obj->GetName());
}

static bool FnSetName(C4AulContext *cthr, C4String *pNewName, bool fSetInInfo, bool fMakeValidIfExists)
{
	if (!cthr->Obj)
	{
		if (!cthr->Def)
			throw new NeedNonGlobalContext("SetName");
		else if (fSetInInfo)
			return false;
		// Definition name
		cthr->Def->SetName(FnStringPar(pNewName));
		return true;
	} else {
		// Object name
		if (fSetInInfo)
		{
			// setting name in info
			C4ObjectInfo *pInfo = cthr->Obj->Info;
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
			C4Player *pOwner = ::Players.Get(cthr->Obj->Owner);
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
			cthr->Obj->SetName(); // make sure object uses info name
		}
		else
		{
			if (!pNewName) cthr->Obj->SetName();
			else cthr->Obj->SetName(pNewName->GetCStr());
		}
	}
	return true;
}

static C4String *FnGetDesc(C4AulContext *cthr)
{
	assert(!cthr->Obj || cthr->Def);
	// find def
	if (!cthr->Def)
		throw new NeedNonGlobalContext("GetDesc");
	// return desc
	return String(cthr->Def->GetDesc());
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
	return String(FormatString("<c %x>%s</c>", dwClr&0xffffff, pPlr->GetName()).getData());
}

static long FnGetPlayerType(C4AulContext *cthr, long iPlayer)
{
	C4Player *pPlr = ::Players.Get(iPlayer);
	if (!pPlr) return 0;
	return pPlr->GetType();
}

static C4Object *FnGetActionTarget(C4AulObjectContext *cthr, long target_index)
{
	if (target_index==0) return cthr->Obj->Action.Target;
	if (target_index==1) return cthr->Obj->Action.Target2;
	return NULL;
}

static C4Void FnSetActionTargets(C4AulObjectContext *cthr, C4Object *pTarget1, C4Object *pTarget2)
{
	// set targets
	cthr->Obj->Action.Target=pTarget1;
	cthr->Obj->Action.Target2=pTarget2;
	return C4VNull;
}

static long FnGetDir(C4AulObjectContext *cthr)
{
	return cthr->Obj->Action.Dir;
}

static bool FnGetEntrance(C4AulObjectContext *cthr)
{
  return cthr->Obj->EntranceStatus;
}

static long FnGetPhase(C4AulObjectContext *cthr)
{
	return cthr->Obj->Action.Phase;
}

static long FnGetEnergy(C4AulObjectContext *cthr)
{
	return 100*cthr->Obj->Energy/C4MaxPhysical;
}

static long FnGetBreath(C4AulObjectContext *cthr)
{
	return 100*cthr->Obj->Breath/C4MaxPhysical;
}

static long FnGetMass(C4AulContext *cthr)
{
	if (!cthr->Obj)
		if (!cthr->Def)
			throw new NeedNonGlobalContext("GetMass");
		else
			return cthr->Def->Mass;
	else
		return cthr->Obj->Mass;
}

static long FnGetRDir(C4AulObjectContext *cthr, long iPrec)
{
	if (!iPrec) iPrec = 10;
	return fixtoi(cthr->Obj->rdir, iPrec);
}

static long FnGetXDir(C4AulObjectContext *cthr, long iPrec)
{
	if (!iPrec) iPrec = 10;
	return fixtoi(cthr->Obj->xdir, iPrec);
}

static long FnGetYDir(C4AulObjectContext *cthr, long iPrec)
{
	if (!iPrec) iPrec = 10;
	return fixtoi(cthr->Obj->ydir, iPrec);
}

static long FnGetR(C4AulObjectContext *cthr)
{
	// Adjust range
	long iR = cthr->Obj->r;
	while (iR>180) iR-=360;
	while (iR<-180) iR+=360;
	return iR;
}

static long FnGetComDir(C4AulObjectContext *cthr)
{
	return cthr->Obj->Action.ComDir;
}

static Nillable<long> FnGetX(C4AulContext *cthr)
{
	if (!cthr->Obj) return C4VNull;
	return cthr->Obj->GetX();
}

static long FnGetVertexNum(C4AulObjectContext *cthr)
{
	return cthr->Obj->Shape.VtxNum;
}

enum VertexDataIndex
{
	VTX_X,
	VTX_Y,
	VTX_CNAT,
	VTX_Friction
};
enum VertexUpdateMode
{
	VTX_SetNonpermanent,
	VTX_SetPermanent,
	VTX_SetPermanentUpd
};

static Nillable<long> FnGetVertex(C4AulObjectContext *cthr, long iIndex, long iValueToGet)
{
	if (cthr->Obj->Shape.VtxNum<1) return C4VNull;
	iIndex=Min<long>(iIndex,cthr->Obj->Shape.VtxNum-1);
	switch (static_cast<VertexDataIndex>(iValueToGet))
	{
	case VTX_X: return cthr->Obj->Shape.VtxX[iIndex]; break;
	case VTX_Y: return cthr->Obj->Shape.VtxY[iIndex]; break;
	case VTX_CNAT: return cthr->Obj->Shape.VtxCNAT[iIndex]; break;
	case VTX_Friction: return cthr->Obj->Shape.VtxFriction[iIndex]; break;
	default:
		DebugLog(FormatString("GetVertex: Unknown vertex attribute: %ld", iValueToGet).getData());
		return C4VNull;
		break;
	}
	// impossible mayhem!
	assert(!"FnGetVertex: unreachable code reached");
	return C4VNull;
}

static bool FnSetVertex(C4AulObjectContext *cthr, long iIndex, long iValueToSet, long iValue, long iOwnVertexMode)
{
	// own vertex mode?
	if (iOwnVertexMode)
	{
		// enter own custom vertex mode if not already set
		if (!cthr->Obj->fOwnVertices)
		{
			cthr->Obj->Shape.CreateOwnOriginalCopy(cthr->Obj->Def->Shape);
			cthr->Obj->fOwnVertices = 1;
		}
		// set vertices at end of buffer
		iIndex += C4D_VertexCpyPos;
	}
	// range check
	if (!Inside<long>(iIndex,0,C4D_MaxVertex-1)) return false;
	// set desired value
	switch (static_cast<VertexDataIndex>(iValueToSet))
	{
		case VTX_X: cthr->Obj->Shape.VtxX[iIndex]=iValue; break;
		case VTX_Y: cthr->Obj->Shape.VtxY[iIndex]=iValue; break;
		case VTX_CNAT: cthr->Obj->Shape.VtxCNAT[iIndex]=iValue; break;
		case VTX_Friction: cthr->Obj->Shape.VtxFriction[iIndex]=iValue; break;
		default:
			DebugLogF("SetVertex: Unknown vertex attribute: %ld", iValueToSet);
			return false;
			break;
	}
	// vertex update desired?
	if (iOwnVertexMode==VTX_SetPermanentUpd) cthr->Obj->UpdateShape(true);
	return true;
}

static bool FnAddVertex(C4AulObjectContext *cthr, long iX, long iY)
{
	return !!cthr->Obj->Shape.AddVertex(iX,iY);
}

static bool FnRemoveVertex(C4AulObjectContext *cthr, long iIndex)
{
	return !!cthr->Obj->Shape.RemoveVertex(iIndex);
}

static C4Void FnSetContactDensity(C4AulObjectContext *cthr, long iDensity)
{
	cthr->Obj->Shape.ContactDensity = iDensity;
	return C4VNull;
}

static Nillable<long> FnGetY(C4AulContext *cthr)
{
	if (!cthr->Obj) return C4VNull;
	return cthr->Obj->GetY();
}

static bool FnGetAlive(C4AulObjectContext *cthr)
{
	return cthr->Obj->GetAlive();
}

static long FnGetOwner(C4AulObjectContext *cthr)
{
	return cthr->Obj->Owner;
}

static long FnGetController(C4AulObjectContext *cthr)
{
	return cthr->Obj->Controller;
}

static bool FnSetController(C4AulObjectContext *cthr, long iNewController)
{
	// validate player
	if (iNewController != NO_OWNER && !ValidPlr(iNewController)) return false;
	// Set controller
	cthr->Obj->Controller = iNewController;
	return true;
}

static long FnGetKiller(C4AulObjectContext *cthr)
{
	return cthr->Obj->LastEnergyLossCausePlayer;
}

static bool FnSetKiller(C4AulObjectContext *cthr, long iNewKiller)
{
	// validate player
	if (iNewKiller != NO_OWNER && !ValidPlr(iNewKiller)) return false;
	// set killer as last energy loss cause
	cthr->Obj->LastEnergyLossCausePlayer = iNewKiller;
	return true;
}

static long FnGetCategory(C4AulContext *cthr)
{
	if (!cthr->Obj)
		if (!cthr->Def)
			throw new NeedNonGlobalContext("GetCategory");
		else
			return cthr->Def->Category;
	else
		return cthr->Obj->Category;
}

static long FnGetOCF(C4AulObjectContext *cthr)
{
	return cthr->Obj->OCF;
}

static long FnGetDamage(C4AulObjectContext *cthr)
{
	return cthr->Obj->Damage;
}

static long FnGetValue(C4AulContext *cthr, C4Object *pInBase, long iForPlayer)
{
	if (!cthr->Obj)
		if (!cthr->Def)
			throw new NeedNonGlobalContext("GetValue");
		else
			return cthr->Def->GetValue(pInBase, iForPlayer);
	else
		return cthr->Obj->GetValue(pInBase, iForPlayer);
}

static long FnGetRank(C4AulObjectContext *cthr)
{
	if (!cthr->Obj->Info) return 0;
	return cthr->Obj->Info->Rank;
}

static long FnGetActTime(C4AulObjectContext *cthr)
{
	return cthr->Obj->Action.Time;
}

static C4ID FnGetID(C4AulObjectContext *cthr)
{
	// return id of object
	return cthr->Obj->Def->id;
}

static Nillable<C4ID> FnGetMenu(C4AulObjectContext *cthr)
{
	if (cthr->Obj->Menu && cthr->Obj->Menu->IsActive())
		return C4ID(cthr->Obj->Menu->GetIdentification());
	return C4VNull;
}

static bool FnCreateMenu(C4AulObjectContext *cthr, C4ID iSymbol, C4Object *pCommandObj,
                         long iExtra, C4String *szCaption, long iExtraData,
                         long iStyle, bool fPermanent, C4ID idMenuID)
{
	if (pCommandObj)
		// object menu: Validate object
		if (!pCommandObj->Status) return false;
	// else scenario script callback: No command object OK

	// Create symbol
	C4Def *pDef;
	C4FacetSurface fctSymbol;
	fctSymbol.Create(C4SymbolSize,C4SymbolSize);
	if (pDef = C4Id2Def(iSymbol))	pDef->Draw(fctSymbol);

	// Clear any old menu, init new menu
	if (!cthr->Obj->CloseMenu(false)) return false;
	if (!cthr->Obj->Menu) cthr->Obj->Menu = new C4ObjectMenu; else cthr->Obj->Menu->ClearItems(true);
	cthr->Obj->Menu->Init(fctSymbol,FnStringPar(szCaption),pCommandObj,iExtra,iExtraData,idMenuID ? idMenuID : iSymbol,iStyle,true);

	// Set permanent
	cthr->Obj->Menu->SetPermanent(fPermanent);

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
	PAR(int,		iCount);
	PAR(any,		Parameter);
	PAR(string, szInfoCaption);
	PAR(int,		iExtra);
	PAR(any,		XPar);
	PAR(any,		XPar2);

	if (!cthr->Obj)
		throw new NeedObjectContext("AddMenuItem");
	if (!cthr->Obj->Menu) return C4VFalse;

	char caption[256+1];
	char parameter[256+1];
	char dummy[256+1];
	char command[512+1];
	char command2[512+1];
	char infocaption[C4MaxTitle+1];

	// get needed symbol size
	int iSymbolSize = cthr->Obj->Menu->GetSymbolSize();

	// Check specified def
	C4Def *pDef = C4Id2Def(idItem);
	if (!pDef) pDef=cthr->Obj->Def;

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
		SCopy("nil", parameter);
		break;
	case C4V_Array:
		// Arrays were never allowed, so tell the scripter
		throw new C4AulExecError(cthr->Obj, "array as parameter to AddMenuItem");
	default:
		return C4VBool(false);
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
			pDef->Draw(fctSymbol, false, 0, NULL, XPar.getInt());
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
				if (cthr->Obj->Menu->IsContextMenu())
					{
					// context menu entry: left object gfx
					long C4MN_SymbolSize = cthr->Obj->Menu->GetItemHeight();
					fctSymbol.Create(C4MN_SymbolSize * 2,C4MN_SymbolSize);
					fctSymbol.Wdt = C4MN_SymbolSize;
					pGfxObj->Def->Draw(fctSymbol, false, pGfxObj->Color, pGfxObj);
					// right of it the rank
					fctRank = fctSymbol;
					fctRank.X = C4MN_SymbolSize;
					fctSymbol.Wdt *= 2;
					}
				else
					{
					// regular menu: draw object picture
					fctSymbol.Create(iSymbolSize,iSymbolSize);
					pGfxObj->Def->Draw(fctSymbol, false, pGfxObj->Color, pGfxObj);
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
			if (XPar.GetType() != C4V_C4Object)
				throw new C4AulExecError(cthr->Obj,
					FormatString("call to \"%s\" parameter %d: got \"%s\", but expected \"%s\"!",
					"AddMenuItem", 8, XPar.GetTypeName(), GetC4VName(C4V_C4Object)
					).getData());
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
			pDef->Draw(fctSymbol, false, XPar.getInt());
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
	cthr->Obj->Menu->Add(caption,fctSymbol,command,iCount,NULL,infocaption,idItem,command2,fOwnValue,iValue,fIsSelectable);

	return C4VTrue;
}

static bool FnSelectMenuItem(C4AulObjectContext *cthr, long iItem)
{
	if (!cthr->Obj->Menu) return false;
	return !!cthr->Obj->Menu->SetSelection(iItem, false, true);
}

static bool FnSetMenuDecoration(C4AulObjectContext *cthr, C4ID idNewDeco)
{
	if (!cthr->Obj->Menu) return false;
	C4GUI::FrameDecoration *pNewDeco = new C4GUI::FrameDecoration();
	if (!pNewDeco->SetByDef(idNewDeco))
	{
		delete pNewDeco;
		return false;
	}
	cthr->Obj->Menu->SetFrameDeco(pNewDeco);
	return true;
}

static bool FnSetMenuTextProgress(C4AulObjectContext *cthr, long iNewProgress)
{
	if (!cthr->Obj->Menu) return false;
	return cthr->Obj->Menu->SetTextProgress(iNewProgress, false);
}


// Check / Status

static C4Object *FnContained(C4AulObjectContext *cthr)
{
	return cthr->Obj->Contained;
}

static C4Object *FnContents(C4AulObjectContext *cthr, long index)
{
	// Special: objects attaching to another object
	//          cannot be accessed by FnContents
	C4Object *cobj;
	while	(cobj=cthr->Obj->Contents.GetObject(index++))
		if (cobj->GetProcedure()!=DFA_ATTACH) return cobj;

	return NULL;
}

static bool FnShiftContents(C4AulObjectContext *cthr, bool fShiftBack, C4ID idTarget, bool fDoCalls)
{
	// regular shift
	if (!idTarget) return !!cthr->Obj->ShiftContents(fShiftBack, fDoCalls);
	// check if ID is present within target
	C4Object *pNewFront = cthr->Obj->Contents.Find(idTarget);
	if (!pNewFront) return false;
	// select it
	cthr->Obj->DirectComContents(pNewFront, fDoCalls);
	// done, success
	return true;
}

static C4Object *FnScrollContents(C4AulObjectContext *cthr)
{
	C4Object *pMove = cthr->Obj->Contents.GetObject();
	if (pMove)
	{
		cthr->Obj->Contents.Remove(pMove);
		cthr->Obj->Contents.Add(pMove,C4ObjectList::stNone);
	}

	return cthr->Obj->Contents.GetObject();
}

static long FnContentsCount(C4AulObjectContext *cthr, C4ID id)
{
	return cthr->Obj->Contents.ObjectCount(id);
}

static C4Object *FnFindContents(C4AulObjectContext *cthr, C4ID c_id)
{
	return cthr->Obj->Contents.Find(c_id);
}

static C4Object *FnFindOtherContents(C4AulObjectContext *cthr, C4ID c_id)
{
	return cthr->Obj->Contents.FindOther(c_id);
}

static bool FnActIdle(C4AulObjectContext *cthr)
{
	return !cthr->Obj->Action.pActionDef;
}

static bool FnCheckEnergyNeedChain(C4AulObjectContext *cthr)
{
	C4ObjectList EnergyChainChecked;
	return CheckEnergyNeedChain(cthr->Obj,EnergyChainChecked);
}

static bool FnEnergyCheck(C4AulObjectContext *cthr, long energy)
{
	if (!(Game.Rules & C4RULE_StructuresNeedEnergy)
	    || (cthr->Obj->Energy>=energy)
		|| !(cthr->Obj->Def->LineConnect & C4D_Power_Consumer))
	{
		cthr->Obj->NeedEnergy=0;
		return true;
	}
	cthr->Obj->NeedEnergy=1;
	return false;
}

static bool FnStuck(C4AulObjectContext *cthr)
{
	return !!cthr->Obj->Shape.CheckContact(cthr->Obj->GetX(),cthr->Obj->GetY());
}

static bool FnInLiquid(C4AulObjectContext *cthr)
{
	return cthr->Obj->InLiquid;
}

static bool FnOnFire(C4AulObjectContext *cthr)
{
	if (cthr->Obj->GetOnFire()) return true;
	// check for effect
	if (!cthr->Obj->pEffects) return false;
	return !!cthr->Obj->pEffects->Get(C4Fx_AnyFire);
}

static bool FnComponentAll(C4AulObjectContext *cthr, C4ID c_id)
{
	long cnt;
	C4IDList Components;
	cthr->Obj->Def->GetComponents(&Components, cthr->Obj, cthr->Obj);
	for (cnt=0; Components.GetID(cnt); cnt++)
		if (Components.GetID(cnt)!=c_id)
			if (Components.GetCount(cnt)>0)
				return false;
	return true;
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
		if (cthr->Obj)
			iOwner = cthr->Obj->Controller;
		else
			iOwner = NO_OWNER;

  C4Object *pNewObj = Game.CreateObject(PropList,cthr->Obj,iOwner,iXOffset,iYOffset);

	// Set initial controller to creating controller, so more complicated cause-effect-chains can be traced back to the causing player
	if (pNewObj && cthr->Obj && cthr->Obj->Controller > NO_OWNER) pNewObj->Controller = cthr->Obj->Controller;

	return pNewObj;
  }

static C4Object *FnCreateConstruction(C4AulContext *cthr,
            C4PropList * PropList, long iXOffset, long iYOffset, Nillable<long> owner,
						long iCompletion, bool fTerrain, bool fCheckSite)
  {
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
		if (cthr->Obj)
			iOwner = cthr->Obj->Controller;
		else
			iOwner = NO_OWNER;

	// Create site object
	C4Object *pNewObj = Game.CreateObjectConstruction(PropList,cthr->Obj,iOwner,iXOffset,iYOffset,iCompletion*FullCon/100,fTerrain);

	// Set initial controller to creating controller, so more complicated cause-effect-chains can be traced back to the causing player
	if (pNewObj && cthr->Obj && cthr->Obj->Controller>NO_OWNER) pNewObj->Controller = cthr->Obj->Controller;

	return pNewObj;
  }

static C4Object *FnCreateContents(C4AulObjectContext *cthr, C4PropList * PropList, Nillable<long> iCount)
{
	// default amount parameter
	if (iCount.IsNil()) iCount = 1;
	// create objects
	C4Object *pNewObj = NULL;
	while (iCount-- > 0) pNewObj = cthr->Obj->CreateContents(PropList);
	// controller will automatically be set upon entrance
	// return last created
	return pNewObj;
}

static C4Object *FnComposeContents(C4AulObjectContext *cthr, C4ID c_id)
{
	return cthr->Obj->ComposeContents(c_id);
}

static bool FnFindConstructionSite(C4AulContext *cthr, C4PropList * PropList, C4Value * VarX, C4Value * VarY)
  {
	// Get def																			Old-style implementation (fixed)...
  C4Def *pDef;
  if (!(pDef=PropList->GetDef())) return false;
	// Get thread vars
	if(!cthr->Caller) return false;
  C4Value& V1 = VarX->GetRefVal();
	C4Value& V2 = VarY->GetRefVal();
  // Construction check at starting position
  if (ConstructionCheck(PropList,V1.getInt(),V2.getInt()))
    return true;
  // Search for real
  int32_t v1 = V1.getInt(), v2 = V2.getInt();
  bool result = !!FindConSiteSpot(v1, v2,
                         pDef->Shape.Wdt,pDef->Shape.Hgt,
                         pDef->Category,
                         20);
  V1 = C4VInt(v1); V2 = C4VInt(v2);
  return result;
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

static bool FnMakeCrewMember(C4AulObjectContext *cthr, long iPlayer)
{
	if (!ValidPlr(iPlayer)) return false;
	return !!::Players.Get(iPlayer)->MakeCrewMember(cthr->Obj);
}

static C4Value FnObjectCount(C4AulContext *cthr, C4Value *pPars)
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

static C4Value FnFindObject(C4AulContext *cthr, C4Value *pPars)
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

static bool FnGrabObjectInfo(C4AulObjectContext *cthr, C4Object *pFrom)
{
	// local call, safety
	if (!pFrom) return false;
	// grab info
	return !!cthr->Obj->GrabInfo(pFrom);
}

static bool FnSmoke(C4AulContext *cthr, long tx, long ty, long level, long dwClr)
  {
	if (cthr->Obj) { tx+=cthr->Obj->GetX(); ty+=cthr->Obj->GetY(); }
  Smoke(tx,ty,level,dwClr);
  return true;
  }

static bool FnBubble(C4AulContext *cthr, long tx, long ty)
  {
	if (cthr->Obj) { tx+=cthr->Obj->GetX(); ty+=cthr->Obj->GetY(); }
  BubbleOut(tx,ty);
  return true;
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

static C4Void FnBlastObjects(C4AulContext *cthr, long iX, long iY, long iLevel, C4Object *pInObj, Nillable<long> iCausedBy)
{
	if (iCausedBy.IsNil() && cthr->Obj) iCausedBy = cthr->Obj->Controller;
	Game.BlastObjects(iX,iY,iLevel,pInObj,iCausedBy,cthr->Obj);
	return C4VNull;
}

static bool FnBlastObject(C4AulObjectContext *cthr, long iLevel, Nillable<long> iCausedBy)
{
	if (iCausedBy.IsNil() && cthr->Obj) iCausedBy = cthr->Obj->Controller;
	if (!cthr->Obj->Status) return false;
	cthr->Obj->Blast(iLevel, iCausedBy);
	return true;
}

static C4Void FnBlastFree(C4AulContext *cthr, long iX, long iY, long iLevel, Nillable<long> iCausedBy)
{
	if (iCausedBy.IsNil() && cthr->Obj) iCausedBy = cthr->Obj->Controller;

	if (cthr->Obj)
	{
		iX += cthr->Obj->GetX();
		iY += cthr->Obj->GetY();
	}
	int grade = BoundBy<int>((iLevel/10)-1,1,3);
	::Landscape.BlastFree(iX, iY, iLevel, grade, iCausedBy);
	return C4VNull;
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
	if(iLoop >= 0 && !fMultiple && GetSoundInstance(FnStringPar(szSound), pObj))
		return false;
	// try to play effect
	if(iLoop >= 0)
		StartSoundEffect(FnStringPar(szSound),!!iLoop,iLevel,pObj, iCustomFalloffDistance);
	else
		StopSoundEffect(FnStringPar(szSound),pObj);
	// always return true (network safety!)
	return true;
}

static bool FnMusic(C4AulContext *cthr, C4String *szSongname, bool fLoop)
	{
	// FIXME: Script should not influence the user's configuration
	if (!szSongname)
		{
		Config.Sound.RXMusic=false;
		Application.MusicSystem.Stop();
		}
	else
		{
		Config.Sound.RXMusic=true;
		Application.MusicSystem.Stop();
		if (!Application.MusicSystem.Play(FnStringPar(szSongname), !!fLoop))
			{
			Config.Sound.RXMusic=false;
			return true;
			}
		}
	return true;
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

static bool FnGameOver(C4AulContext *cthr, long iGameOverValue /* provided for future compatibility */)
	{
	return !!Game.DoGameOver();
	}

static bool FnGainMissionAccess(C4AulContext *cthr, C4String *szPassword)
	{
	if (SLen(Config.General.MissionAccess)+SLen(FnStringPar(szPassword))+3>CFG_MaxString) return false;
	SAddModule(Config.General.MissionAccess,FnStringPar(szPassword));
	return true;
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
	bool fSpoken=false;
	if (SCopySegment(FnStringPar(szMessage),1,buf,'$'))
		if (StartSoundEffect(buf,false,100,pObj ? pObj : cthr->Obj))
			fSpoken=true;

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
	bool fSpoken=false;
	if (SCopySegment(FnStringPar(szMessage),1,buf,'$'))
		if (StartSoundEffect(buf,false,100,cthr->Obj))
			fSpoken=true;

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
	bool fSpoken=false;
	if (SCopySegment(FnStringPar(szMessage),1,buf,'$'))
		if (StartSoundEffect(buf,false,100,cthr->Obj))
			fSpoken=true;

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

static bool FnDrawVolcanoBranch(C4AulContext *cthr, long mat, long fx, long fy, long tx, long ty, long size)
	{
	long cx,cx2,cy;
	for (cy=ty; cy<fy; cy++)
		{
		cx=fx+(tx-fx)*(cy-fy)/(ty-fy);
		for (cx2=cx-size/2; cx2<cx+size/2; cx2++)
			SBackPix(cx2,cy,Mat2PixColDefault(mat)+GBackIFT(cx2,cy));
		}
	return true;
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
	if (!Config.Graphics.ShowCommands) Config.Graphics.ShowCommands=true;
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
		if(!C4Id2Def(id)) return false;
		return pPlr->Knowledge.SetIDCount(id,1,true);
		}
  }

static bool FnSetComponent(C4AulObjectContext *cthr, C4ID idComponent, long iCount)
{
	return cthr->Obj->Component.SetIDCount(idComponent,iCount,true);
}

static C4Value FnGetPlrKnowledge_C4V(C4AulContext *cthr, C4Value* iPlr_C4V, C4Value* id_C4V, C4Value* iIndex_C4V, C4Value* dwCategory_C4V)
  {
	long iPlr = iPlr_C4V->getInt();
	C4ID id = id_C4V->getC4ID();
	long iIndex = iIndex_C4V->getInt();
	DWORD dwCategory = dwCategory_C4V->getInt();
  if (!ValidPlr(iPlr)) return C4VBool(false);
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

  if (!ValidPlr(iPlr)) return C4VBool(false);
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

  if (!ValidPlr(iPlr)) return C4VBool(false);
	// Search by id, return available count
	if (id)	return C4VInt(::Players.Get(iPlr)->HomeBaseProduction.GetIDCount(id));
	// Search indexed item of given category, return C4ID
	return C4VID(::Players.Get(iPlr)->HomeBaseProduction.GetID( ::Definitions, dwCategory, iIndex ));
	}

static long FnSetPlrMagic(C4AulContext *cthr, long iPlr, C4ID id, bool fRemove)
  {
	C4Player *pPlr=::Players.Get(iPlr);
  if (!pPlr) return false;
	if (fRemove)
		{
		long iIndex=pPlr->Magic.GetIndex(id);
		if (iIndex<0) return false;
		return pPlr->Magic.DeleteItem(iIndex);
		}
	else
		{
		if(!C4Id2Def(id)) return false;
		return pPlr->Magic.SetIDCount(id,1,true);
		}
  }

static C4Value FnGetPlrMagic_C4V(C4AulContext *cthr, C4Value* iPlr_C4V, C4Value* id_C4V, C4Value* iIndex_C4V)
  {
	long iPlr = iPlr_C4V->getInt();
	C4ID id = id_C4V->getC4ID();
	long iIndex = iIndex_C4V->getInt();

  if (!ValidPlr(iPlr)) return C4VBool(false);
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
  if (!ValidPlr(iPlr)) return false;
  ::Players.Get(iPlr)->Wealth = BoundBy<long>(iValue,0,100000);
	return true;
  }

static long FnDoScore(C4AulContext *cthr, long iPlr, long iChange)
  {
  if (!ValidPlr(iPlr)) return false;
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
  if (!ValidPlr(iPlr)) return false;
  return ::Players.Get(iPlr)->GetHiRankActiveCrew(false);
  }

static C4Object *FnGetCrew(C4AulContext *cthr, long iPlr, long index)
  {
  if (!ValidPlr(iPlr)) return false;
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
  if (!ValidPlr(iPlr)) return false;
  return ::Players.Get(iPlr)->Captain;
  }

static bool FnSetCursor(C4AulContext *cthr, long iPlr, C4Object *pObj, bool fNoSelectMark, bool fNoSelectArrow, bool fNoSelectCrew)
  {
	C4Player *pPlr = ::Players.Get(iPlr);
	if (!pPlr || (pObj && !pObj->Status) || (pObj && pObj->CrewDisabled)) return false;
	pPlr->SetCursor(pObj, !fNoSelectMark, !fNoSelectArrow);
	if (!fNoSelectCrew) pPlr->SelectCrew(pObj, true);
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

static bool FnSelectCrew(C4AulContext *cthr, long iPlr, C4Object *pObj, bool fSelect, bool fNoCursorAdjust)
  {
	C4Player *pPlr = ::Players.Get(iPlr);
	if (!pPlr || !pObj) return false;
	if (fNoCursorAdjust)
		{ if (fSelect) pObj->DoSelect(); else pObj->UnSelect(); }
	else
		pPlr->SelectCrew(pObj,fSelect);
	return true;
  }

static long FnGetSelectCount(C4AulContext *cthr, long iPlr)
  {
  if (!ValidPlr(iPlr)) return false;
  return ::Players.Get(iPlr)->SelectCount;
  }

static bool FnSetCrewStatus(C4AulObjectContext *cthr, long iPlr, bool fInCrew)
{
	// validate player
	C4Player *pPlr = ::Players.Get(iPlr);
	if (!pPlr) return false;
	// set crew status
	return !!pPlr->SetObjectCrewStatus(cthr->Obj, fInCrew);
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
	return C4VNull;
}

static C4Void FnSetTemperature(C4AulContext *cthr, long iTemperature)
{
	::Weather.SetTemperature(iTemperature);
	return C4VNull;
}

static long FnGetTemperature(C4AulContext *cthr)
  {
  return ::Weather.GetTemperature();
  }

static C4Void FnSetSeason(C4AulContext *cthr, long iSeason)
{
	::Weather.SetSeason(iSeason);
	return C4VNull;
}

static long FnGetSeason(C4AulContext *cthr)
  {
  return ::Weather.GetSeason();
  }

static C4Void FnSetClimate(C4AulContext *cthr, long iClimate)
{
	::Weather.SetClimate(iClimate);
	return C4VNull;
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
	return C4VNull;
}

static C4Void FnShakeObjects(C4AulContext *cthr, long x, long y, long rad)
{
	Game.ShakeObjects(x,y,rad);
	return C4VNull;
}

static C4Void FnDigFree(C4AulContext *cthr, long x, long y, long rad, bool fRequest)
{
	::Landscape.DigFree(x,y,rad,fRequest,cthr->Obj);
	return C4VNull;
}

static C4Void FnDigFreeRect(C4AulContext *cthr, long iX, long iY, long iWdt, long iHgt, bool fRequest)
{
	::Landscape.DigFreeRect(iX,iY,iWdt,iHgt,fRequest,cthr->Obj);
	return C4VNull;
}

static C4Void FnFreeRect(C4AulContext *cthr, long iX, long iY, long iWdt, long iHgt, long iFreeDensity)
{
	if (iFreeDensity)
		::Landscape.ClearRectDensity(iX,iY,iWdt,iHgt,iFreeDensity);
	else
		::Landscape.ClearRect(iX,iY,iWdt,iHgt);
	return C4VNull;
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

static long FnSetTransferZone(C4AulObjectContext *cthr, long iX, long iY, long iWdt, long iHgt)
{
	iX+=cthr->Obj->GetX(); iY+=cthr->Obj->GetY();
	return Game.TransferZones.Set(iX,iY,iWdt,iHgt,cthr->Obj);
}

static C4Value FnEqual_C4V(C4AulContext *cthr, C4Value * Val1, C4Value * Val2)
  {
  return C4VBool(Val1->GetData()==Val2->GetData());
  }

static long FnAbs(C4AulContext *cthr, long iVal)
  {
  return Abs(iVal);
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

static long FnObjectNumber(C4AulObjectContext *cthr)
{
	return cthr->Obj->Number;
}

C4Object* FnObject(C4AulContext *cthr, long iNumber)
	{
	return ::Objects.SafeObjectPointer(iNumber);
	}

static long FnShowInfo(C4AulObjectContext *cthr, C4Object *pObj)
{
	if (!pObj) pObj=cthr->Obj; if (!pObj) return false;
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

static long FnRandom(C4AulContext *cthr, long iRange)
  {
  return Random(iRange);
  }

static long FnAsyncRandom(C4AulContext *cthr, long iRange)
  {
  return SafeRandom(iRange);
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

static bool FnResort(C4AulContext *cthr, C4Object *pObj)
	{
  if (!pObj) pObj=cthr->Obj;
	// Resort single object
	if (pObj)
		pObj->Resort();
	// Resort object list
	else
		::Objects.SortByCategory();
	return true;
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
	if (pObj) scr = &pObj->Def->Script; else scr = &Game.Script;
	// exec func
	return !!scr->Call(PSF_InputCallback, pObj, &C4AulParSet(C4VString(FnStringPar(szAnswerString)), C4VInt(iForPlr)));
	}

static long FnScriptCounter(C4AulContext *cthr)
	{
	return Game.Script.Counter;
	}

static C4Void FnSetMass(C4AulObjectContext *cthr, long iValue)
{
	cthr->Obj->OwnMass=iValue-cthr->Obj->Def->Mass;
	cthr->Obj->UpdateMass();
	return C4VNull;
}

static long FnGetColor(C4AulObjectContext *cthr)
{
	return cthr->Obj->Color;
}

static Nillable<long> FnGetPlrColor(C4AulContext *cthr, long iPlr)
  {
	// get player
	C4Player *pPlr = ::Players.Get(iPlr);
	// safety
	if (!pPlr) return C4VNull;
	// return player color
	return pPlr->ColorDw;
  }

static C4Void FnSetColor(C4AulObjectContext *cthr, long iValue)
{
	cthr->Obj->Color=iValue;
	cthr->Obj->UpdateGraphics(false);
	cthr->Obj->UpdateFace(false);
	return C4VNull;
}

static C4Void FnSetFoW(C4AulContext *cthr, bool fEnabled, long iPlr)
{
	// safety
	if (!ValidPlr(iPlr)) return C4VNull;
	// set enabled
	::Players.Get(iPlr)->SetFoW(!!fEnabled);
	// success
	return C4VNull;
}

static C4Void FnSetPlrViewRange(C4AulObjectContext *cthr, long iRange)
{
	// set range
	cthr->Obj->SetPlrViewRange(iRange);
	// success
	return C4VNull;
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

static C4Void FnSetPicture(C4AulObjectContext *cthr, long iX, long iY, long iWdt, long iHgt)
{
	// set new picture rect
	cthr->Obj->PictureRect.Set(iX, iY, iWdt, iHgt);
	// success
	return C4VNull;
}

static C4String *FnGetProcedure(C4AulObjectContext *cthr)
{
	// no action?
	if (!cthr->Obj->Action.pActionDef) return NULL;
	// get proc
	long iProc = cthr->Obj->Action.pActionDef->GetPropertyInt(P_Procedure);
	// NONE?
	if (iProc <= DFA_NONE) return NULL;
	// return procedure name
	return String(ProcedureName[iProc]);
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
	if (iNewSize<0 || iNewSize > C4ValueArray::MaxSize)
		throw new C4AulExecError(cthr->Obj, FormatString("SetLength: invalid array size (%d)", iNewSize).getData());

	// set new size
	pArrayRef.SetArrayLength(iNewSize, cthr);

	// yeah, done!
	return C4VBool(true);
}

static bool FnSetClrModulation(C4AulObjectContext *cthr, Nillable<long> dwClr, long iOverlayID)
{
	uint32_t clr = 0xffffffff;
	if(!dwClr.IsNil()) clr = dwClr;

	// overlay?
	if (iOverlayID)
	{
		C4GraphicsOverlay *pOverlay = cthr->Obj->GetGraphicsOverlay(iOverlayID, false);
		if (!pOverlay)
		{
			DebugLogF("SetClrModulation: Overlay %d not defined for object %d (%s)", (int) iOverlayID, (int) cthr->Obj->Number, cthr->Obj->GetName());
			return false;
		}
		pOverlay->SetClrModulation(clr);
	}
	else
	{
		// set it
		cthr->Obj->ColorMod=clr;
	}
	// success
	return true;
}

static Nillable<long> FnGetClrModulation(C4AulObjectContext *cthr, long iOverlayID)
{
	// overlay?
	if (iOverlayID)
	{
		C4GraphicsOverlay *pOverlay = cthr->Obj->GetGraphicsOverlay(iOverlayID, false);
		if (!pOverlay)
		{
			DebugLogF("GetClrModulation: Overlay %d not defined for object %d (%s)", (int) iOverlayID, (int) cthr->Obj->Number, cthr->Obj->GetName());
			return C4VNull;
		}
		return pOverlay->GetClrModulation();
	}
	else
		// get it
		return cthr->Obj->ColorMod;
}

static bool FnGetMissionAccess(C4AulContext *cthr, C4String *strMissionAccess)
{
	// safety
	if (!strMissionAccess) return false;

	// non-sync mode: warn
	if(::Control.SyncMode())
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

static C4Value FnGetObjectVal(C4AulObjectContext* cthr, C4Value* strEntry_C4V, C4Value* strSection_C4V, C4Value *iEntryNr_C4V)
{
	const char *strEntry = FnStringPar(strEntry_C4V->getStr());
	const char *strSection = FnStringPar(strSection_C4V->getStr());
	if (!*strSection) strSection = NULL;

	long iEntryNr = iEntryNr_C4V->getInt();

	// get value
	return GetValByStdCompiler(strEntry, strSection, iEntryNr, mkNamingAdapt(*cthr->Obj, "Object"));
}

static C4Value FnGetObjectInfoCoreVal(C4AulObjectContext* cthr, C4Value* strEntry_C4V, C4Value* strSection_C4V, C4Value *iEntryNr_C4V)
{
	const char *strEntry = FnStringPar(strEntry_C4V->getStr());
	const char *strSection = FnStringPar(strSection_C4V->getStr());
	if (strSection && !*strSection) strSection = NULL;
	long iEntryNr = iEntryNr_C4V->getInt();

	// get obj info
	C4ObjectInfo* pObjInfo = cthr->Obj->Info;

	if(!pObjInfo) return C4VNull;

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
	return GetValByStdCompiler(strEntry, strSection, iEntryNr, mkNamingAdapt(mkParAdapt(*pPlayer, true), "Player"));
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

static bool FnCloseMenu(C4AulObjectContext *cthr)
{
	return !!cthr->Obj->CloseMenu(true);
}

static Nillable<long> FnGetMenuSelection(C4AulObjectContext *cthr)
{
	if(!cthr->Obj->Menu || !cthr->Obj->Menu->IsActive()) return C4VNull;
	return cthr->Obj->Menu->GetSelection();
}

static bool FnResortObjects(C4AulContext* cthr, C4String *szFunc, long Category)
	{
	// safety
	if (!szFunc) return false;
	if (!cthr->Caller) return false;
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
	return true;
	}

static bool FnResortObject(C4AulObjectContext* cthr, C4String *szFunc)
{
	// safety
	if (!szFunc) return false;
	if (!cthr->Caller) return false;
	// get function
	C4AulFunc *pFn = cthr->Caller->Func->GetLocalSFunc(FnStringPar(szFunc));
	if (!pFn)
		throw new C4AulExecError(cthr->Obj, FormatString("ResortObjects: Resort function %s not found", FnStringPar(szFunc)).getData());
	// create object resort
	C4ObjResort *pObjRes = new C4ObjResort();
	pObjRes->OrderFunc=pFn;
	pObjRes->pSortObj=cthr->Obj;
	// insert into game resort proc list
	pObjRes->Next = ::Objects.ResortProc;
	::Objects.ResortProc = pObjRes;
	// success, so far
	return true;
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

static bool FnCanConcatPictureWith(C4AulObjectContext *pCtx, C4Object *pObj)
{
	// safety
	if (!pCtx->Obj->Status || !pObj) return false;
	// Call the function in the object
	return pCtx->Obj->CanConcatPictureWith(pObj);
}

static bool FnSetGraphics(C4AulObjectContext *pCtx, C4String *pGfxName, C4ID idSrcGfx, long iOverlayID, long iOverlayMode, C4String *pAction, long dwBlitMode, C4Object *pOverlayObject)
{
	// safety
	if (!pCtx->Obj->Status) return false;
	// get def for source graphics
	C4Def *pSrcDef=NULL;
	if (idSrcGfx) if (!(pSrcDef=C4Id2Def(idSrcGfx))) return false;
	// setting overlay?
	if (iOverlayID)
	{
		// any overlays must be positive for now
		if (iOverlayID<0) { Log("SetGraphics: Background overlays not implemented!"); return false; }
		// deleting overlay?
		C4DefGraphics *pGrp;
		if (iOverlayMode == C4GraphicsOverlay::MODE_Object)
		{
			if (!pOverlayObject) return pCtx->Obj->RemoveGraphicsOverlay(iOverlayID);
		}
		else
		{
			if (!pSrcDef) return pCtx->Obj->RemoveGraphicsOverlay(iOverlayID);
			pGrp = pSrcDef->Graphics.Get(FnStringPar(pGfxName));
			if (!pGrp) return false;
		}
		// adding/setting
		C4GraphicsOverlay *pOverlay = pCtx->Obj->GetGraphicsOverlay(iOverlayID, true);
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
		if (!pOverlay->IsValid(pCtx->Obj))
		{
			pCtx->Obj->RemoveGraphicsOverlay(iOverlayID);
			return false;
		}
		// Okay, valid overlay set!
		return true;
	}
	// no overlay: Base graphics
	// set graphics - pSrcDef==NULL defaults to pObj->pDef
	return pCtx->Obj->SetGraphics(FnStringPar(pGfxName), pSrcDef);
}

static long FnGetDefBottom(C4AulContext* cthr)
{
	if (!cthr->Obj)
		if (!cthr->Def)
			throw new NeedNonGlobalContext("GetDefBottom");

	assert(!cthr->Obj || cthr->Obj->Def == cthr->Def);
	return cthr->Def->Shape.y+cthr->Def->Shape.Hgt + (cthr->Obj ? cthr->Obj->GetY() : 0);
}

static C4String *FnMaterialName(C4AulContext* cthr, long iMat)
	{
	// mat valid?
	if (!MatValid(iMat)) return NULL;
	// return mat name
	return String(::MaterialMap.Map[iMat].Name);
	}

static bool FnSetMenuSize(C4AulObjectContext* cthr, long iCols, long iRows)
{
	// get menu
	C4Menu *pMnu=cthr->Obj->Menu;
	if (!pMnu || !pMnu->IsActive()) return false;
	pMnu->SetSize(BoundBy<long>(iCols, 0, 50), BoundBy<long>(iRows, 0, 50));
	return true;
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

static C4Value FnLocalN(C4AulContext* cthr, C4Value* strName_C4V)
{
	if (!cthr->Obj)
		throw new NeedObjectContext("LocalN");

	const char* strName = FnStringPar(strName_C4V->getStr());

	// find variable
	C4Value* pVarN = cthr->Obj->LocalNamed.GetItem(strName);

	if(!pVarN) return C4VNull;

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

static C4Value FnSetCrewExtraData(C4AulContext *cthr, C4Value *strDataName_C4V, C4Value *Data)
{
	if (!cthr->Obj)
		throw NeedObjectContext("SetCrewExtraData");

	const char *strDataName = FnStringPar(strDataName_C4V->getStr());
	// valid crew with info? (for great nullpointer prevention)
	if(!cthr->Obj->Info) return C4Value();
	// do not allow data type C4V_String or C4V_C4Object
	if(Data->GetType() != C4V_Any &&
		Data->GetType() != C4V_Int &&
		Data->GetType() != C4V_Bool) return C4VNull;
	// get pointer on info...
	C4ObjectInfo *pInfo = cthr->Obj->Info;
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

static C4Value FnGetCrewExtraData(C4AulContext *cthr, C4Value *strDataName_C4V)
{
	if (!cthr->Obj)
		throw NeedObjectContext("GetCrewExtraData");

	const char *strDataName = FnStringPar(strDataName_C4V->getStr());
	// valid crew with info?
	if(!cthr->Obj->Info) return C4Value();
	// get pointer on info...
	C4ObjectInfo *pInfo = cthr->Obj->Info;
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

static bool FnGetCrewEnabled(C4AulObjectContext *cctx)
{
	// return status
	return !cctx->Obj->CrewDisabled;
}

static C4Void FnSetCrewEnabled(C4AulObjectContext *cctx, bool fEnabled)
{
	// set status
	cctx->Obj->CrewDisabled=!fEnabled;
	// deselect
	if (!fEnabled)
		{
		cctx->Obj->Select=false;
		C4Player *pOwner;
		if (pOwner=::Players.Get(cctx->Obj->Owner))
			{
			// if viewed player cursor gets deactivated and no new cursor is found, follow the old in target mode
			bool fWasCursorMode = (pOwner->ViewMode == C4PVM_Cursor);
			if (pOwner->Cursor==cctx->Obj)
				pOwner->AdjustCursorCommand();
			if (!pOwner->ViewCursor && !pOwner->Cursor && fWasCursorMode)
				pOwner->SetViewMode(C4PVM_Target, cctx->Obj);
			}
		}
	// success
	return C4VNull;
}

static bool FnUnselectCrew(C4AulContext *cctx, long iPlayer)
	{
	// get player
	C4Player *pPlr=::Players.Get(iPlayer);
	if (!pPlr) return false;
	// unselect crew
	pPlr->UnselectCrew();
	// success
	return true;
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
	return true;
	}

static C4Void FnDoCrewExp(C4AulObjectContext* ctx, long iChange)
	{
	// do exp
	ctx->Obj->DoExperience(iChange);
	// success
	return C4VNull;
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
	return true;
	}

static bool FnResetGamma(C4AulContext* ctx, long iRampIndex)
	{
	::GraphicsSystem.SetGamma(0x000000, 0x808080, 0xffffff, iRampIndex);
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
	if(!Inside(iNewIndex, 0l, 255l)) return false;
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

static bool FnSetObjectOrder(C4AulContext* ctx, C4Object *pObjBeforeOrAfter, C4Object *pSortObj, bool fSortAfter)
	{
	// local call/safety
	if (!pSortObj) pSortObj=ctx->Obj; if (!pSortObj) return false;
	if (!pObjBeforeOrAfter) return false;
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
	return true;
	}

static bool FnDrawMaterialQuad(C4AulContext* ctx, C4String *szMaterial, long iX1, long iY1, long iX2, long iY2, long iX3, long iY3, long iX4, long iY4, bool fSub)
	{
  const char *szMat = FnStringPar(szMaterial);
	return !! ::Landscape.DrawQuad(iX1, iY1, iX2, iY2, iX3, iY3, iX4, iY4, szMat, fSub);
	}

static bool FnFightWith(C4AulObjectContext *ctx, C4Object *pTarget)
{
	// safety
	if (!pTarget) return false;
	C4Object *pClonk = ctx->Obj;
	// check OCF
	if (~(pTarget->OCF & pClonk->OCF) & OCF_FightReady) return false;
	// RejectFight callback
	C4AulParSet parset1(C4VObj(pTarget) );
	C4AulParSet parset2(C4VObj(pClonk) );
	if(pTarget->Call(PSF_RejectFight, &parset1).getBool() ) return false;
	if(pClonk->Call(PSF_RejectFight, &parset2).getBool() ) return false;
	// begin fighting
	ObjectActionFight(pClonk,pTarget);
	ObjectActionFight(pTarget,pClonk);
	// success
	return true;
}

static bool FnSetFilmView(C4AulContext *ctx, long iToPlr)
	{
	// check player
	if (!ValidPlr(iToPlr) && iToPlr != NO_OWNER) return false;
	// real switch in replays only
	if (!::Control.isReplay()) return true;
	// set new target plr
	if (C4Viewport *vp = ::GraphicsSystem.GetFirstViewport()) vp->Init(iToPlr, true);
	// done, always success (sync)
	return true;
	}

static bool FnClearMenuItems(C4AulObjectContext *ctx)
{
	// check menu
	if (!ctx->Obj->Menu) return false;
	// clear the items
	ctx->Obj->Menu->ClearItems(true);
	// success
	return true;
}

static C4Object *FnGetObjectLayer(C4AulObjectContext *ctx)
{
	// get layer object
	return ctx->Obj->pLayer;
}

static C4Void FnSetObjectLayer(C4AulObjectContext *ctx, C4Object *pNewLayer)
{
	// set layer object
	ctx->Obj->pLayer = pNewLayer;
	// set for all contents as well
	for (C4ObjectLink *pLnk=ctx->Obj->Contents.First; pLnk; pLnk=pLnk->Next)
		if ((ctx->Obj=pLnk->Obj) && ctx->Obj->Status)
			ctx->Obj->pLayer = pNewLayer;
	// success
	return C4VNull;
}

static C4Void FnSetShape(C4AulObjectContext *ctx, long iX, long iY, long iWdt, long iHgt)
{
	// update shape
	ctx->Obj->Shape.x = iX;
	ctx->Obj->Shape.y = iY;
	ctx->Obj->Shape.Wdt = iWdt;
	ctx->Obj->Shape.Hgt = iHgt;
	// section list needs refresh
	ctx->Obj->UpdatePos();
	// done, success
	return C4VNull;
}

static bool FnAddMsgBoardCmd(C4AulContext *ctx, C4String *pstrCommand, C4String *pstrScript, long iRestriction)
	{
	// safety
	if(!pstrCommand || !pstrScript) return false;
	// unrestricted commands cannot be set by direct-exec script (like /script).
	if(iRestriction != C4MessageBoardCommand::C4MSGCMDR_Identifier)
		if(!ctx->Caller || !*ctx->Caller->Func->Name)
			return false;
	C4MessageBoardCommand::Restriction eRestriction;
	switch (iRestriction)
		{
		case C4MessageBoardCommand::C4MSGCMDR_Escaped: eRestriction = C4MessageBoardCommand::C4MSGCMDR_Escaped; break;
		case C4MessageBoardCommand::C4MSGCMDR_Plain: eRestriction = C4MessageBoardCommand::C4MSGCMDR_Plain; break;
		case C4MessageBoardCommand::C4MSGCMDR_Identifier: eRestriction = C4MessageBoardCommand::C4MSGCMDR_Identifier; break;
		default: return false;
		}
	// add command
	::MessageInput.AddCommand(FnStringPar(pstrCommand), FnStringPar(pstrScript), eRestriction);
	return true;
	}

static bool FnSetGameSpeed(C4AulContext *ctx, long iSpeed)
	{
	// safety
	if(iSpeed) if(!Inside<long>(iSpeed, 0, 1000)) return false;
	if(!iSpeed) iSpeed = 38;
	// set speed, restart timer
	Application.SetGameTickDelay(1000 / iSpeed);
	return true;
	}

static bool FnSetObjDrawTransform(C4AulObjectContext *ctx, long iA, long iB, long iC, long iD, long iE, long iF, long iOverlayID)
{
	C4DrawTransform *pTransform;
	// overlay?
	if (iOverlayID)
	{
		// set overlay transform
		C4GraphicsOverlay *pOverlay = ctx->Obj->GetGraphicsOverlay(iOverlayID, false);
		if (!pOverlay) return false;
		pTransform = pOverlay->GetTransform();
	}
	else
		{
		// set base transform
		pTransform = ctx->Obj->pDrawTransform;
		// del transform?
		if (!iB && !iC && !iD && !iF && iA==iE && (!iA || iA==1000))
		{
			// identity/0 and no transform defined: nothing to do
			if (!pTransform) return true;
			// transform has no flipdir?
			if (pTransform->FlipDir == 1)
			{
				// kill identity-transform, then
				delete pTransform;
				ctx->Obj->pDrawTransform=NULL;
				return true;
			}
			// flipdir must remain: set identity
			pTransform->Set(1,0,0,0,1,0,0,0,1);
			return true;
		}
		// create draw transform if not already present
		if (!pTransform) pTransform = ctx->Obj->pDrawTransform = new C4DrawTransform();
		}
	// assign values
	pTransform->Set((float) iA/1000, (float) iB/1000, (float) iC/1000, (float) iD/1000, (float) iE/1000, (float) iF/1000, 0, 0, 1);
	// done, success
	return true;
}

static bool FnSetObjDrawTransform2(C4AulObjectContext *ctx, long iA, long iB, long iC, long iD, long iE, long iF, long iG, long iH, long iI, long iOverlayID)
{
	// local call / safety
	C4Object * pObj = ctx->Obj;
	C4DrawTransform *pTransform;
	// overlay?
	if (iOverlayID)
	{
		// set overlay transform
		C4GraphicsOverlay *pOverlay = pObj->GetGraphicsOverlay(iOverlayID, false);
		if (!pOverlay) return false;
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
	return true;
}

#define COPY_C4V_PAR(Var, Par, ParType, Std) \
	Var = (Par && Par->GetType() == ParType \
         ? Par->GetData().Int \
				 : Std)

bool SimFlight(FIXED &x, FIXED &y, FIXED &xdir, FIXED &ydir, int32_t iDensityMin, int32_t iDensityMax, int32_t iIter);

static C4Value FnSimFlight(C4AulContext *ctx, C4Value *pvrX, C4Value *pvrY, C4Value *pvrXDir, C4Value *pvrYDir, C4Value *pviDensityMin, C4Value *pviDensityMax, C4Value *pviIter, C4Value *pviPrec)
{
	// check and copy parameters
	if(!pvrX || !pvrY || !pvrXDir || !pvrYDir) return C4VFalse;

	COPY_C4V_PAR(int iDensityMin, pviDensityMin, C4V_Int, C4M_Solid);
	COPY_C4V_PAR(int iDensityMax, pviDensityMax, C4V_Int, 100);
	COPY_C4V_PAR(int iIter, pviIter, C4V_Int, -1);
	COPY_C4V_PAR(int iPrec, pviPrec, C4V_Int, 10);

	// convert to FIXED
	FIXED x = itofix(pvrX->GetData().Int), y = itofix(pvrY->GetData().Int),
		    xdir = itofix(pvrXDir->GetData().Int, iPrec), ydir = itofix(pvrYDir->GetData().Int, iPrec);

	// simulate
	if(!SimFlight(x, y, xdir, ydir, iDensityMin, iDensityMax, iIter))
		return C4VFalse;

	// write results back
	*pvrX = C4VInt(fixtoi(x)); *pvrY = C4VInt(fixtoi(y));
	*pvrXDir = C4VInt(fixtoi(xdir * iPrec)); *pvrYDir = C4VInt(fixtoi(ydir * iPrec));

	return C4VTrue;
}
#undef COPY_C4V_PAR
static bool FnSetPortrait(C4AulObjectContext *ctx, C4String *pstrPortrait, C4ID idSourceDef, bool fPermanent, bool fCopyGfx)
{
	// safety
	const char *szPortrait;
	if (!pstrPortrait || !*(szPortrait=FnStringPar(pstrPortrait))) return false;
	C4Object *pTarget = ctx->Obj;
	if (!pTarget->Status || !pTarget->Info) return false;
	// special case: clear portrait
	if (SEqual(szPortrait, C4Portrait_None)) return pTarget->Info->ClearPortrait(!!fPermanent);
	// get source def for portrait
	C4Def *pSourceDef;
	if (idSourceDef) pSourceDef = ::Definitions.ID2Def(idSourceDef); else pSourceDef=pTarget->Def;
	if (!pSourceDef) return false;
	// special case: random portrait
	if (SEqual(szPortrait, C4Portrait_Random)) return pTarget->Info->SetRandomPortrait(pSourceDef->id, !!fPermanent, !!fCopyGfx);
	// try to set portrait
	return pTarget->Info->SetPortrait(szPortrait, pSourceDef, !!fPermanent, !!fCopyGfx);
}

static C4Value FnGetPortrait(C4AulContext *ctx, C4Value *pvfGetID, C4Value *pvfGetPermanent)
{
	// get parameters
	C4Object *pObj = ctx->Obj; bool fGetID = pvfGetID->getBool(); bool fGetPermanent = pvfGetPermanent->getBool();
	// check valid object with info section
	if (!pObj)
		throw new NeedObjectContext("GetPortrait");
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
	if (!pstrSection || !*(szSection=FnStringPar(pstrSection))) return false;
	// try to load it
	return Game.LoadScenarioSection(szSection, dwFlags);
	}

static bool FnSetObjectStatus(C4AulObjectContext *ctx, long iNewStatus, bool fClearPointers)
	{
	// local call / safety
	if (!ctx->Obj->Status) return false;
	// no change
	if (ctx->Obj->Status == iNewStatus) return true;
	// set new status
	switch (iNewStatus)
	{
	case C4OS_NORMAL:
		return ctx->Obj->StatusActivate();
	case C4OS_INACTIVE:
		return ctx->Obj->StatusDeactivate(fClearPointers);
	default:
		return false; // status unknown
	}
}

static long FnGetObjectStatus(C4AulObjectContext *ctx)
{
	return ctx->Obj->Status;
}

static bool FnAdjustWalkRotation(C4AulObjectContext *ctx, long iRangeX, long iRangeY, long iSpeed)
{
	// must be rotateable and attached to solid ground
	if (!ctx->Obj->Def->Rotateable || ~ctx->Obj->Action.t_attach&CNAT_Bottom || ctx->Obj->Shape.AttachMat == MNone)
		return false;
	// adjust rotation
	return ctx->Obj->AdjustWalkRotation(iRangeX, iRangeY, iSpeed);
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
	return true;
	}

static bool FnChangeEffect(C4AulContext *ctx, C4String *psEffectName, C4Object *pTarget, long iIndex, C4String *psNewEffectName, long iNewTimer)
	{
	// evaluate parameters
	const char *szEffect = FnStringPar(psEffectName);
	const char *szNewEffect = FnStringPar(psNewEffectName);
	if (!szNewEffect || !*szNewEffect) return false;
	// get effects
	C4Effect *pEffect = pTarget ? pTarget->pEffects : Game.pGlobalEffects;
	if (!pEffect) return false;
	// name/wildcard given: find effect by name and index
	if (szEffect && *szEffect)
		pEffect = pEffect->Get(szEffect, iIndex);
	else
		// otherwise, get by number
		pEffect = pEffect->Get(iIndex, false);
	// effect found?
	if (!pEffect) return false;
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
	return true;
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
	if (!pEffect) return false;
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

static long FnGetContact(C4AulObjectContext *ctx, long iVertex, long dwCheck)
{
	// vertex not specified: check all
	if (iVertex == -1)
	{
		long iResult = 0;
		for (int i=0; i<ctx->Obj->Shape.VtxNum; ++i)
			iResult |= ctx->Obj->Shape.GetVertexContact(i, dwCheck, ctx->Obj->GetX(), ctx->Obj->GetY());
		return iResult;
	}
	// vertex specified: check it
	if (!Inside<long>(iVertex, 0, ctx->Obj->Shape.VtxNum-1)) return 0;
	return ctx->Obj->Shape.GetVertexContact(iVertex, dwCheck, ctx->Obj->GetX(), ctx->Obj->GetY());
}

static long FnSetObjectBlitMode(C4AulObjectContext *ctx, long dwNewBlitMode, long iOverlayID)
{
	// overlay?
	if (iOverlayID)
	{
		C4GraphicsOverlay *pOverlay = ctx->Obj->GetGraphicsOverlay(iOverlayID, false);
		if (!pOverlay)
		{
			DebugLogF("SetObjectBlitMode: Overlay %d not defined for object %d (%s)", (int) iOverlayID, (int) ctx->Obj->Number, ctx->Obj->GetName());
			return false;
		}
		pOverlay->SetBlitMode(dwNewBlitMode);
		return true;
	}
	// get prev blit mode
	DWORD dwPrevMode = ctx->Obj->BlitMode;
	// iNewBlitMode = 0: reset to definition default
	if (!dwNewBlitMode)
		ctx->Obj->BlitMode = ctx->Obj->Def->BlitMode;
	else
		// otherwise, set the desired value
		// also ensure that the custom flag is set
		ctx->Obj->BlitMode = dwNewBlitMode | C4GFXBLIT_CUSTOM;
	// return previous value
	return dwPrevMode;
}

static Nillable<long> FnGetObjectBlitMode(C4AulObjectContext *ctx, long iOverlayID)
{
	// overlay?
	if (iOverlayID)
	{
		C4GraphicsOverlay *pOverlay = ctx->Obj->GetGraphicsOverlay(iOverlayID, false);
		if (!pOverlay)
		{
			DebugLogF("SetObjectBlitMode: Overlay %d not defined for object %d (%s)", (int) iOverlayID, (int) ctx->Obj->Number, ctx->Obj->GetName());
			return C4VNull;
		}
		return pOverlay->GetBlitMode();
	}
	// get blitting mode
	return ctx->Obj->BlitMode;
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

static bool FnOnOwnerRemoved(C4AulObjectContext *cthr)
	{
	// safety
	C4Object *pObj = cthr->Obj;
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
	return C4VNull;
}

static long FnGetUnusedOverlayID(C4AulObjectContext *ctx, long iBaseIndex)
	{
	// safety
	if (!iBaseIndex) return 0;
	// find search first unused index from there on
	int iSearchDir = (iBaseIndex < 0) ? -1 : 1;
	while (ctx->Obj->GetGraphicsOverlay(iBaseIndex, false)) iBaseIndex += iSearchDir;
	return iBaseIndex;
	}

static long FnActivateGameGoalMenu(C4AulContext *ctx, long iPlayer)
	{
	// get target player
	C4Player *pPlr = ::Players.Get(iPlayer);
	if (!pPlr) return false;
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
	if (!dwClr) dwClr = 0xffffffff;
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
template <typename RType LIST(N, TYPENAMES)>  \
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
		virtual C4Value Exec(C4AulContext *pContext, C4Value pPars[], bool fPassErrors=false) \
		{ return C4ValueConv<RType>::ToC4V(pFunc(pContext LIST(N, CONV_FROM_C4V))); } \
	protected:                                  \
		Func pFunc;                               \
	};                                          \
template <typename RType LIST(N, TYPENAMES)>  \
class C4AulDefObjectFunc##N:                  \
public C4AulDefFuncHelper {                   \
	public:                                     \
/* A pointer to the function which this class wraps */ \
		typedef RType (*Func)(C4AulObjectContext * LIST(N, PARS)); \
		virtual int GetParCount() { return N; }   \
		virtual C4V_Type GetRetType()             \
		{ return C4ValueConv<RType>::Type(); }    \
/* Constructor, using the base class to create the ParType array */ \
		C4AulDefObjectFunc##N(C4AulScript *pOwner, const char *pName, Func pFunc, bool Public): \
			C4AulDefFuncHelper(pOwner, pName, Public LIST(N, CONV_TYPE)), pFunc(pFunc) { } \
/* Extracts the parameters from C4Values and wraps the return value in a C4Value */ \
		virtual C4Value Exec(C4AulContext *pContext, C4Value pPars[], bool fPassErrors=false) \
		{ \
			if (!pContext->Obj) throw new NeedObjectContext(Name); \
			return C4ValueConv<RType>::ToC4V(pFunc(static_cast<C4AulObjectContext*>(pContext) LIST(N, CONV_FROM_C4V))); \
		} \
	protected:                                  \
		Func pFunc;                               \
	};                                          \
template <typename RType LIST(N, TYPENAMES)> \
static void AddFunc(C4AulScript * pOwner, const char * Name, RType (*pFunc)(C4AulContext * LIST(N, PARS)), bool Public=true) \
	{ \
	new C4AulDefFunc##N<RType LIST(N, PARS)>(pOwner, Name, pFunc, Public); \
	} \
template <typename RType LIST(N, TYPENAMES)> \
static void AddFunc(C4AulScript * pOwner, const char * Name, RType (*pFunc)(C4AulObjectContext * LIST(N, PARS)), bool Public=true) \
	{ \
	new C4AulDefObjectFunc##N<RType LIST(N, PARS)>(pOwner, Name, pFunc, Public); \
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
	AddFunc(pEngine, "Abs", FnAbs);
	AddFunc(pEngine, "Min", FnMin);
	AddFunc(pEngine, "Max", FnMax);
	AddFunc(pEngine, "Sin", FnSin);
	AddFunc(pEngine, "Cos", FnCos);
	AddFunc(pEngine, "Sqrt", FnSqrt);
	AddFunc(pEngine, "ArcSin", FnArcSin);
	AddFunc(pEngine, "ArcCos", FnArcCos);
	AddFunc(pEngine, "BoundBy", FnBoundBy);
	AddFunc(pEngine, "Inside", FnInside);
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
	AddFunc(pEngine, "Sound", FnSound);
	AddFunc(pEngine, "Music", FnMusic);
	AddFunc(pEngine, "MusicLevel", FnMusicLevel);
	AddFunc(pEngine, "SetPlayList", FnSetPlayList);
	AddFunc(pEngine, "RemoveObject", FnRemoveObject);
	AddFunc(pEngine, "GetActionTarget", FnGetActionTarget);
	AddFunc(pEngine, "SetActionTargets", FnSetActionTargets);
	AddFunc(pEngine, "SetPlrView", FnSetPlrView);
	AddFunc(pEngine, "SetPlrKnowledge", FnSetPlrKnowledge);
	AddFunc(pEngine, "SetPlrMagic", FnSetPlrMagic);
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
	AddFunc(pEngine, "GetTemperature", FnGetTemperature);
	AddFunc(pEngine, "SetTemperature", FnSetTemperature);
	AddFunc(pEngine, "ShakeFree", FnShakeFree);
	AddFunc(pEngine, "ShakeObjects", FnShakeObjects);
	AddFunc(pEngine, "DigFree", FnDigFree);
	AddFunc(pEngine, "FreeRect", FnFreeRect);
	AddFunc(pEngine, "DigFreeRect", FnDigFreeRect);
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
	AddFunc(pEngine, "GetProcedure", FnGetProcedure);
	AddFunc(pEngine, "GetChar", FnGetChar);
	AddFunc(pEngine, "ActivateGameGoalMenu", FnActivateGameGoalMenu);
	AddFunc(pEngine, "CanConcatPictureWith", FnCanConcatPictureWith);
	AddFunc(pEngine, "SetGraphics", FnSetGraphics);
	AddFunc(pEngine, "Object", FnObject);
	AddFunc(pEngine, "ObjectNumber", FnObjectNumber);
	AddFunc(pEngine, "ShowInfo", FnShowInfo);
	AddFunc(pEngine, "GetTime", FnGetTime);
	AddFunc(pEngine, "GetSystemTime", FnGetSystemTime, false);
	AddFunc(pEngine, "SetClrModulation", FnSetClrModulation);
	AddFunc(pEngine, "GetClrModulation", FnGetClrModulation);
	AddFunc(pEngine, "GetMissionAccess", FnGetMissionAccess);
	AddFunc(pEngine, "CloseMenu", FnCloseMenu);
	AddFunc(pEngine, "GetMenuSelection", FnGetMenuSelection);
	AddFunc(pEngine, "ResortObjects", FnResortObjects);
	AddFunc(pEngine, "ResortObject", FnResortObject);
	AddFunc(pEngine, "GetDefBottom", FnGetDefBottom);
	AddFunc(pEngine, "MaterialName", FnMaterialName);
	AddFunc(pEngine, "SetMenuSize", FnSetMenuSize);
	AddFunc(pEngine, "GetNeededMatStr", FnGetNeededMatStr);
	AddFunc(pEngine, "GetCrewEnabled", FnGetCrewEnabled);
	AddFunc(pEngine, "SetCrewEnabled", FnSetCrewEnabled);
	AddFunc(pEngine, "UnselectCrew", FnUnselectCrew);
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
	AddFunc(pEngine, "DoCrewExp", FnDoCrewExp);
	AddFunc(pEngine, "ReloadDef", FnReloadDef);
	AddFunc(pEngine, "ReloadParticle", FnReloadParticle);
	AddFunc(pEngine, "SetGamma", FnSetGamma);
	AddFunc(pEngine, "ResetGamma", FnResetGamma);
	AddFunc(pEngine, "FrameCounter", FnFrameCounter);
	AddFunc(pEngine, "SetLandscapePixel", FnSetLandscapePixel);
	AddFunc(pEngine, "SetObjectOrder", FnSetObjectOrder);
	AddFunc(pEngine, "GetPlrColor", FnGetPlrColor);
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
	AddFunc(pEngine, "HideSettlementScoreInEvaluation", FnHideSettlementScoreInEvaluation, false);
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
	AddFunc(pEngine, "GetPlayerControlState", FnGetPlayerControlState);
	//FIXME new C4AulDefCastFunc(pEngine, "ScoreboardCol", C4V_C4ID, C4V_Int);

	AddFunc(pEngine, "goto", Fn_goto);
	AddFunc(pEngine, "this", Fn_this);
	AddFunc(pEngine, "ChangeDef", FnChangeDef);
	AddFunc(pEngine, "Explode", FnExplode);
	AddFunc(pEngine, "Incinerate", FnIncinerate);
	AddFunc(pEngine, "IncinerateLandscape", FnIncinerateLandscape);
	AddFunc(pEngine, "Extinguish", FnExtinguish);
	AddFunc(pEngine, "GrabContents", FnGrabContents);
	AddFunc(pEngine, "Punch", FnPunch);
	AddFunc(pEngine, "Kill", FnKill);
	AddFunc(pEngine, "Fling", FnFling);
	AddFunc(pEngine, "Jump", FnJump);
	AddFunc(pEngine, "Enter", FnEnter);
	AddFunc(pEngine, "DeathAnnounce", FnDeathAnnounce);
	AddFunc(pEngine, "SetSolidMask", FnSetSolidMask);
	AddFunc(pEngine, "GetGravity", FnGetGravity);
	AddFunc(pEngine, "GetGravity", FnGetGravity);
	AddFunc(pEngine, "Exit", FnExit);
	AddFunc(pEngine, "Collect", FnCollect);
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
	{ "VIS_Select"             ,C4V_Int,          VIS_Select},
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

  { "Equal",                1  ,C4V_Any      ,{ C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V FnEqual_C4V ,                 0 },
  { "SetProperty",          1  ,C4V_Any      ,{ C4V_String  ,C4V_Any     ,C4V_PropList,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V FnSetProperty_C4V ,           0 },
  { "GetProperty",          1  ,C4V_Any      ,{ C4V_String  ,C4V_PropList,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V FnGetProperty_C4V ,           0 },
  { "PlayerObjectCommand",  1  ,C4V_Bool     ,{ C4V_Int     ,C4V_String  ,C4V_C4Object,C4V_Any     ,C4V_Int     ,C4V_C4Object,C4V_Any    ,C4V_Int    ,C4V_Any    ,C4V_Any}  ,0 ,                                   FnPlayerObjectCommand },
  { "SetCommand",           1  ,C4V_Bool     ,{ C4V_String  ,C4V_C4Object,C4V_Any     ,C4V_Int     ,C4V_C4Object,C4V_Any     ,C4V_Int    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,0 ,                                   FnSetCommand },
  { "AddCommand",           1  ,C4V_Bool     ,{ C4V_String  ,C4V_C4Object,C4V_Any     ,C4V_Int     ,C4V_C4Object,C4V_Int     ,C4V_Any    ,C4V_Int    ,C4V_Int    ,C4V_Any}  ,0 ,                                   FnAddCommand },
  { "AppendCommand",        1  ,C4V_Bool     ,{ C4V_String  ,C4V_C4Object,C4V_Any     ,C4V_Int     ,C4V_C4Object,C4V_Int     ,C4V_Any    ,C4V_Int    ,C4V_Int    ,C4V_Any}  ,0 ,                                   FnAppendCommand },
  { "GetCommand",           1  ,C4V_Any      ,{ C4V_Int     ,C4V_Int     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,0 ,                                   FnGetCommand },
  { "PathFree2",            1  ,C4V_Bool     ,{ C4V_pC4Value,C4V_pC4Value,C4V_Int     ,C4V_Int     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V FnPathFree2_C4V ,             0 },
  { "FindObject",           1  ,C4V_C4Object ,{ C4V_Array   ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,0 ,                                   FnFindObject },
  { "FindObjects",          1  ,C4V_Array    ,{ C4V_Array   ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,0 ,                                   FnFindObjects },
  { "ObjectCount",          1  ,C4V_Int      ,{ C4V_Array   ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,0 ,                                   FnObjectCount },
  { "ProtectedCall",        1  ,C4V_Any      ,{ C4V_C4Object,C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V FnProtectedCall_C4V ,         0 },
  { "PrivateCall",          1  ,C4V_Any      ,{ C4V_C4Object,C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V FnPrivateCall_C4V ,           0 },
  { "GameCall",             1  ,C4V_Any      ,{ C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V FnGameCall_C4V ,              0 },
  { "GameCallEx",           1  ,C4V_Any      ,{ C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V FnGameCallEx_C4V ,            0 },
  { "DefinitionCall",       1  ,C4V_Any      ,{ C4V_PropList,C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V FnDefinitionCall_C4V ,        0 },
  { "Call",                 0  ,C4V_Any      ,{ C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V FnCall_C4V ,                  0 },
  { "GetPlrKnowledge",      1  ,C4V_Int      ,{ C4V_Int     ,C4V_PropList,C4V_Int     ,C4V_Int     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V FnGetPlrKnowledge_C4V ,       0 },
  { "GetPlrMagic",          1  ,C4V_Int      ,{ C4V_Int     ,C4V_PropList,C4V_Int     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V FnGetPlrMagic_C4V ,           0 },
  { "GetComponent",         1  ,C4V_Int      ,{ C4V_PropList,C4V_Int     ,C4V_C4Object,C4V_PropList,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V FnGetComponent_C4V ,          0 },
  { "PlayerMessage",        1  ,C4V_Int      ,{ C4V_Int     ,C4V_String  ,C4V_C4Object,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V &FnPlayerMessage_C4V,         0 },
  { "Message",              1  ,C4V_Bool     ,{ C4V_String  ,C4V_C4Object,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V &FnMessage_C4V,               0 },
  { "AddMessage",           1  ,C4V_Bool     ,{ C4V_String  ,C4V_C4Object,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V &FnAddMessage_C4V,            0 },
  { "PlrMessage",           1  ,C4V_Bool     ,{ C4V_String  ,C4V_Int     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V &FnPlrMessage_C4V,            0 },
  { "Log",                  1  ,C4V_Bool     ,{ C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V &FnLog_C4V,                   0 },
  { "DebugLog",             1  ,C4V_Bool     ,{ C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V &FnDebugLog_C4V,              0 },
  { "Format",               1  ,C4V_String   ,{ C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V &FnFormat_C4V,                0 },
  { "EditCursor",           1  ,C4V_C4Object ,{ C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,0 ,                                   FnEditCursor },
  { "AddMenuItem",          1  ,C4V_Bool     ,{ C4V_String  ,C4V_String  ,C4V_PropList,C4V_Int     ,C4V_Any     ,C4V_String  ,C4V_Int    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,0 ,                                   FnAddMenuItem },
  { "GetHomebaseMaterial",  1  ,C4V_Int      ,{ C4V_Int     ,C4V_PropList,C4V_Int     ,C4V_Int     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V FnGetHomebaseMaterial_C4V ,   0 },
  { "GetHomebaseProduction",1  ,C4V_Int      ,{ C4V_Int     ,C4V_PropList,C4V_Int     ,C4V_Int     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V FnGetHomebaseProduction_C4V , 0 },

  { "IsRef",                1  ,C4V_Bool     ,{ C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnIsRef,                     0 },
  { "GetType",              1  ,C4V_Int      ,{ C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnGetType,                   0 },

  { "CreateArray",          1  ,C4V_Array    ,{ C4V_Int     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,0,                                   FnCreateArray },
  { "GetLength",            1  ,C4V_Int      ,{ C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,0,                                   FnGetLength },
  { "GetIndexOf",           1  ,C4V_Int      ,{ C4V_Any     ,C4V_Array   ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,0,                                   FnGetIndexOf },
  { "SetLength",            1  ,C4V_Bool     ,{ C4V_pC4Value,C4V_Int     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,0,                                   FnSetLength },

  { "GetDefCoreVal",        1  ,C4V_Any      ,{ C4V_String  ,C4V_String  ,C4V_Int     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnGetDefCoreVal,             0 },
  { "GetObjectVal",         1  ,C4V_Any      ,{ C4V_String  ,C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnGetObjectVal,              0 },
  { "GetObjectInfoCoreVal", 1  ,C4V_Any      ,{ C4V_String  ,C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnGetObjectInfoCoreVal,      0 },
  { "GetScenarioVal",       1  ,C4V_Any      ,{ C4V_String  ,C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnGetScenarioVal,            0 },
  { "GetPlayerVal",         1  ,C4V_Any      ,{ C4V_String  ,C4V_String  ,C4V_Int     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnGetPlayerVal,              0 },
  { "GetPlayerInfoCoreVal", 1  ,C4V_Any      ,{ C4V_String  ,C4V_String  ,C4V_Int     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnGetPlayerInfoCoreVal,      0 },
  { "GetMaterialVal",       1  ,C4V_Any      ,{ C4V_String  ,C4V_String  ,C4V_Int     ,C4V_Int     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnGetMaterialVal,            0 },

  { "SetPlrExtraData",      1  ,C4V_Any      ,{ C4V_Int     ,C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnSetPlrExtraData,           0 },
  { "GetPlrExtraData",      1  ,C4V_Any      ,{ C4V_Int     ,C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnGetPlrExtraData,           0 },
  { "SetCrewExtraData",     1  ,C4V_Any      ,{ C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnSetCrewExtraData,          0 },
  { "GetCrewExtraData",     1  ,C4V_Any      ,{ C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnGetCrewExtraData,          0 },
  { "SimFlight",            1  ,C4V_Bool     ,{ C4V_pC4Value,C4V_pC4Value,C4V_pC4Value,C4V_pC4Value,C4V_Int     ,C4V_Int     ,C4V_Int    ,C4V_Int    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnSimFlight,                 0 },
  { "GetPortrait",          1  ,C4V_Any      ,{ C4V_C4Object,C4V_Bool    ,C4V_Bool    ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnGetPortrait,               0 },
  { "AddEffect",            1  ,C4V_Int      ,{ C4V_String  ,C4V_C4Object,C4V_Int     ,C4V_Int     ,C4V_C4Object,C4V_PropList,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnAddEffect_C4V,             0 },
  { "GetEffect",            1  ,C4V_Any      ,{ C4V_String  ,C4V_C4Object,C4V_Int     ,C4V_Int     ,C4V_Int     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnGetEffect_C4V,             0 },
  { "CheckEffect",          1  ,C4V_Int      ,{ C4V_String  ,C4V_C4Object,C4V_Int     ,C4V_Int     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnCheckEffect_C4V,           0 },
  { "EffectCall",           1  ,C4V_Any      ,{ C4V_C4Object,C4V_Int     ,C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnEffectCall_C4V,            0 },
  { "EffectVar",            1  ,C4V_pC4Value ,{ C4V_Int     ,C4V_C4Object,C4V_Int     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnEffectVar_C4V,             0 },

  { "eval",                 1  ,C4V_Any      ,{ C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnEval,                      0 },

  { "VarN",                 1  ,C4V_Any      ,{ C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnVarN,                      0 },
  { "LocalN",               1  ,C4V_Any      ,{ C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnLocalN,                    0 },
  { "GlobalN",              1  ,C4V_Any      ,{ C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnGlobalN,                   0 },

  { NULL,                   0  ,C4V_Any      ,{ C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,0,                                   0 }

  };
