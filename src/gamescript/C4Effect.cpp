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

// C4AulFun-based effects assigned to an object
/* Also contains some helper functions for various landscape effects */

#include <C4Include.h>
#include <C4Effect.h>

#include <C4DefList.h>
#include <C4Object.h>
#include <C4Random.h>
#include <C4Game.h>
#include <C4Landscape.h>
#include <C4PXS.h>
#include <C4GameObjects.h>
#include <C4SoundSystem.h>

void C4Effect::AssignCallbackFunctions()
{
	C4PropList *p = GetCallbackScript();
	// compose function names and search them
	char fn[C4AUL_MAX_Identifier+1];
	sprintf(fn, PSF_FxStart,  GetName()); pFnStart  = p->GetFunc(fn);
	sprintf(fn, PSF_FxStop,   GetName()); pFnStop   = p->GetFunc(fn);
	sprintf(fn, PSF_FxTimer,  GetName()); pFnTimer  = p->GetFunc(fn);
	sprintf(fn, PSF_FxEffect, GetName()); pFnEffect = p->GetFunc(fn);
	sprintf(fn, PSF_FxDamage, GetName()); pFnDamage = p->GetFunc(fn);
}

C4PropList * C4Effect::GetCallbackScript()
{
	C4Def *pDef;
	if (CommandTarget)
	{
		// overwrite ID for sync safety in runtime join
		idCommandTarget = CommandTarget->id;
		return CommandTarget;
	}
	else if (idCommandTarget && (pDef=::Definitions.ID2Def(idCommandTarget)))
		return pDef;
	else
		return ::ScriptEngine.GetPropList();
}

C4Effect::C4Effect(C4Object *pForObj, C4String *szName, int32_t iPrio, int32_t iTimerInterval, C4Object *pCmdTarget, C4ID idCmdTarget, const C4Value &rVal1, const C4Value &rVal2, const C4Value &rVal3, const C4Value &rVal4)
{
	// assign values
	iPriority = 0; // effect is not yet valid; some callbacks to other effects are done before
	iInterval = iTimerInterval;
	iTime = 0;
	CommandTarget = pCmdTarget;
	idCommandTarget = idCmdTarget;
	AcquireNumber();
	Register(pForObj, iPrio);
	// Set name and callback functions
	SetProperty(P_Name, C4VString(szName));
}

void C4Effect::Register(C4Object *pForObj, int32_t iPrio)
{
	// get effect target
	C4Effect **ppEffectList = pForObj ? &pForObj->pEffects : &Game.pGlobalEffects;
	C4Effect *pCheck, *pPrev = *ppEffectList;
	if (pPrev && Abs(pPrev->iPriority) < iPrio)
	{
		while ((pCheck = pPrev->pNext))
				if (Abs(pCheck->iPriority) >= iPrio) break; else pPrev = pCheck;
		// insert after previous
		pNext = pPrev->pNext;
		pPrev->pNext = this;
	}
	else
	{
		// insert as first effect
		pNext = *ppEffectList;
		*ppEffectList = this;
	}
}

C4Effect * C4Effect::New(C4Object * pForObj, C4String * szName, int32_t iPrio, int32_t iTimerInterval, C4Object * pCmdTarget, C4ID idCmdTarget, const C4Value &rVal1, const C4Value &rVal2, const C4Value &rVal3, const C4Value &rVal4)
{
	C4Effect * pEffect = new C4Effect(pForObj, szName, iPrio, iTimerInterval, pCmdTarget, idCmdTarget, rVal1, rVal2, rVal3, rVal4);
	// ask all effects with higher priority first - except for prio 1 effects, which are considered out of the priority call chain (as per doc)
	bool fRemoveUpper = (iPrio != 1);
	// note that apart from denying the creation of this effect, higher priority effects may also remove themselves
	// or do other things with the effect list
	// (which does not quite make sense, because the effect might be denied by another effect)
	// so the priority is assigned after this call, marking this effect dead before it's definitely valid
	if (fRemoveUpper && pEffect->pNext)
	{
		C4Effect * pEffect2 = pEffect->pNext->Check(pForObj, szName->GetCStr(), iPrio, iTimerInterval, rVal1, rVal2, rVal3, rVal4);
		if (pEffect2)
		{
			// effect denied (iResult = -1), added to an effect (iResult = Number of that effect)
			// or added to an effect that destroyed itself (iResult = -2)
			if (pEffect2 != (C4Effect*)C4Fx_Effect_Deny && pEffect2 != (C4Effect*)C4Fx_Effect_Annul) return pEffect2;
			// effect is still marked dead
			return 0;
		}
	}
	// init effect
	// higher-priority effects must be deactivated temporarily, and then reactivated regarding the new effect
	// higher-level effects should not be inserted during the process of removing or adding a lower-level effect
	// because that would cause a wrong initialization order
	// (hardly ever causing trouble, however...)
	C4Effect *pLastRemovedEffect=NULL;
	if (fRemoveUpper && pEffect->pNext && pEffect->pFnStart)
		pEffect->TempRemoveUpperEffects(pForObj, false, &pLastRemovedEffect);
	// bad things may happen
	if (pForObj && !pForObj->Status) return 0; // this will be invalid!
	pEffect->iPriority = iPrio; // validate effect now
	if (pEffect->pFnStart)
		if (pEffect->pFnStart->Exec(pCmdTarget, &C4AulParSet(C4VObj(pForObj), C4VPropList(pEffect), C4VInt(0), rVal1, rVal2, rVal3, rVal4)).getInt() == C4Fx_Start_Deny)
			// the effect denied to start: assume it hasn't, and mark it dead
			pEffect->SetDead();
	if (fRemoveUpper && pEffect->pNext && pEffect->pFnStart)
		pEffect->TempReaddUpperEffects(pForObj, pLastRemovedEffect);
	if (pForObj && !pForObj->Status) return 0; // this will be invalid!
	// Update OnFire cache
	if (!pEffect->IsDead() && pForObj && WildcardMatch(C4Fx_AnyFire, szName->GetCStr()))
		pForObj->SetOnFire(true);
	return pEffect;
}

C4Effect::C4Effect()
{
	// defaults
	iPriority=iTime=iInterval=0;
	CommandTarget=NULL;
	pNext = NULL;
}

C4Effect::~C4Effect()
{
	// del following effects (not recursively)
	C4Effect *pEffect;
	while ((pEffect = pNext))
	{
		pNext = pEffect->pNext;
		pEffect->pNext = NULL;
		delete pEffect;
	}
}

void C4Effect::Denumerate(C4ValueNumbers * numbers)
{
	// denum in all effects
	C4Effect *pEff = this;
	do
	{
		// command target
		pEff->CommandTarget.DenumeratePointers();
		// assign any callback functions
		pEff->AssignCallbackFunctions();
		pEff->C4PropList::Denumerate(numbers);
	}
	while ((pEff=pEff->pNext));
}

void C4Effect::ClearPointers(C4Object *pObj)
{
	// clear pointers in all effects
	C4Effect *pEff = this;
	do
		// command target lost: effect dead w/o callback
		if (pEff->CommandTarget == pObj)
		{
			pEff->SetDead();
			pEff->CommandTarget=NULL;
		}
	while ((pEff=pEff->pNext));
}

C4Effect *C4Effect::Get(const char *szName, int32_t iIndex, int32_t iMaxPriority)
{
	// safety
	if (!szName) return NULL;
	// check all effects
	C4Effect *pEff = this;
	do
	{
		// skip dead
		if (pEff->IsDead()) continue;
		// skip effects with too high priority
		if (iMaxPriority && pEff->iPriority > iMaxPriority) continue;
		// wildcard compare name
		const char *szEffectName = pEff->GetName();
		if (!SWildcardMatchEx(szEffectName, szName)) continue;
		// effect name matches
		// check index
		if (iIndex--) continue;
		// effect found
		return pEff;
	}
	while ((pEff=pEff->pNext));
	// nothing found
	return NULL;
}

int32_t C4Effect::GetCount(const char *szMask, int32_t iMaxPriority)
{
	// count all matching effects
	int32_t iCnt=0; C4Effect *pEff = this;
	do if (!pEff->IsDead())
			if (!szMask || SWildcardMatchEx(pEff->GetName(), szMask))
				if (!iMaxPriority || pEff->iPriority <= iMaxPriority)
					++iCnt;
	while ((pEff = pEff->pNext));
	// return count
	return iCnt;
}

C4Effect* C4Effect::Check(C4Object *pForObj, const char *szCheckEffect, int32_t iPrio, int32_t iTimer, const C4Value &rVal1, const C4Value &rVal2, const C4Value &rVal3, const C4Value &rVal4)
{
	// priority=1: always OK; no callbacks
	if (iPrio == 1) return 0;
	// check this and other effects
	C4Effect *pAddToEffect = NULL; bool fDoTempCallsForAdd = false;
	C4Effect *pLastRemovedEffect=NULL;
	for (C4Effect *pCheck = this; pCheck; pCheck = pCheck->pNext)
	{
		if (!pCheck->IsDead() && pCheck->pFnEffect && pCheck->iPriority >= iPrio)
		{
			int32_t iResult = pCheck->pFnEffect->Exec(pCheck->CommandTarget, &C4AulParSet(C4VString(szCheckEffect), C4VObj(pForObj), C4VPropList(pCheck), rVal1, rVal2, rVal3, rVal4)).getInt();
			if (iResult == C4Fx_Effect_Deny)
				// effect denied
				return (C4Effect*)C4Fx_Effect_Deny;
			// add to other effect
			if (iResult == C4Fx_Effect_Annul || iResult == C4Fx_Effect_AnnulCalls)
			{
				pAddToEffect = pCheck;
				fDoTempCallsForAdd = (iResult == C4Fx_Effect_AnnulCalls);
			}
		}
	}
	// adding to other effect?
	if (pAddToEffect)
	{
		// do temp remove calls if desired
		if (pAddToEffect->pNext && fDoTempCallsForAdd)
			pAddToEffect->TempRemoveUpperEffects(pForObj, false, &pLastRemovedEffect);
		C4Value Par1 = C4VString(szCheckEffect), Par2 = C4VInt(iTimer), Par8;
		int32_t iResult = pAddToEffect->DoCall(pForObj, PSFS_FxAdd, Par1, Par2, rVal1, rVal2, rVal3, rVal4, Par8).getInt();
		// do temp readd calls if desired
		if (pAddToEffect->pNext && fDoTempCallsForAdd)
			pAddToEffect->TempReaddUpperEffects(pForObj, pLastRemovedEffect);
		// effect removed by this call?
		if (iResult == C4Fx_Start_Deny)
		{
			pAddToEffect->Kill(pForObj);
			return (C4Effect*)C4Fx_Effect_Annul;
		}
		else
			// other effect is the target effect number
			return pAddToEffect;
	}
	// added to no effect and not denied
	return 0;
}

void C4Effect::Execute(C4Object *pObj)
{
	// get effect list
	C4Effect **ppEffectList = pObj ? &pObj->pEffects : &Game.pGlobalEffects;
	// execute all effects not marked as dead
	C4Effect *pEffect = this, **ppPrevEffect=ppEffectList;
	do
	{
		// effect dead?
		if (pEffect->IsDead())
		{
			// delete it, then
			C4Effect *pNextEffect = pEffect->pNext;
			pEffect->pNext = NULL;
			delete pEffect;
			// next effect
			*ppPrevEffect = pEffect = pNextEffect;
		}
		else
		{
			// execute effect: time elapsed
			++pEffect->iTime;
			// check timer execution
			if (pEffect->iInterval && !(pEffect->iTime % pEffect->iInterval))
			{
				if (pEffect->pFnTimer)
				{
					if (pEffect->pFnTimer->Exec(pEffect->CommandTarget, &C4AulParSet(C4VObj(pObj), C4VPropList(pEffect), C4VInt(pEffect->iTime))).getInt() == C4Fx_Execute_Kill)
					{
						// safety: this class got deleted!
						if (pObj && !pObj->Status) return;
						// timer function decided to finish it
						pEffect->Kill(pObj);
					}
					// safety: this class got deleted!
					if (pObj && !pObj->Status) return;
				}
				else
					// no timer function: mark dead after time elapsed
					pEffect->Kill(pObj);
			}
			// next effect
			ppPrevEffect = &pEffect->pNext;
			pEffect = pEffect->pNext;
		}
	}
	while (pEffect);
}

void C4Effect::Kill(C4Object *pObj)
{
	// active?
	C4Effect *pLastRemovedEffect=NULL;
	if (IsActive())
		// then temp remove all higher priority effects
		TempRemoveUpperEffects(pObj, false, &pLastRemovedEffect);
	else
		// otherwise: temp reactivate before real removal
		// this happens only if a lower priority effect removes an upper priority effect in its add- or removal-call
		if (pFnStart && iPriority!=1) pFnStart->Exec(CommandTarget, &C4AulParSet(C4VObj(pObj), C4VPropList(this), C4VInt(C4FxCall_TempAddForRemoval)));
	// remove this effect
	int32_t iPrevPrio = iPriority; SetDead();
	if (pFnStop)
		if (pFnStop->Exec(CommandTarget, &C4AulParSet(C4VObj(pObj), C4VPropList(this), C4VInt(C4FxCall_Normal))).getInt() == C4Fx_Stop_Deny)
			// effect denied to be removed: recover
			iPriority = iPrevPrio;
	// reactivate other effects
	TempReaddUpperEffects(pObj, pLastRemovedEffect);
	// Update OnFire cache
	if (pObj && WildcardMatch(C4Fx_AnyFire, GetName()))
		if (!Get(C4Fx_AnyFire))
			pObj->SetOnFire(false);
}

void C4Effect::ClearAll(C4Object *pObj, int32_t iClearFlag)
{
	// simply remove access all effects recursively, and do removal calls
	// this does not regard lower-level effects being added in the removal calls,
	// because this could hang the engine with poorly coded effects
	if (pNext) pNext->ClearAll(pObj, iClearFlag);
	if ((pObj && !pObj->Status) || IsDead()) return;
	int32_t iPrevPrio = iPriority;
	SetDead();
	if (pFnStop)
		if (pFnStop->Exec(CommandTarget, &C4AulParSet(C4VObj(pObj), C4VPropList(this), C4VInt(iClearFlag))).getInt() == C4Fx_Stop_Deny)
		{
			// this stop-callback might have deleted the object and then denied its own removal
			// must not modify self in this case...
			if (pObj && !pObj->Status) return;
			// effect denied to be removed: recover it
			iPriority = iPrevPrio;
		}
	// Update OnFire cache
	if (pObj && WildcardMatch(C4Fx_AnyFire, GetName()) && IsDead())
		if (!Get(C4Fx_AnyFire))
			pObj->SetOnFire(false);
}

void C4Effect::DoDamage(C4Object *pObj, int32_t &riDamage, int32_t iDamageType, int32_t iCausePlr)
{
	// ask all effects for damage adjustments
	C4Effect *pEff = this;
	do
	{
		if (!pEff->IsDead() && pEff->pFnDamage)
			riDamage = pEff->pFnDamage->Exec(pEff->CommandTarget, &C4AulParSet(C4VObj(pObj), C4VPropList(pEff), C4VInt(riDamage), C4VInt(iDamageType), C4VInt(iCausePlr))).getInt();
		if (pObj && !pObj->Status) return;
	}
	while ((pEff = pEff->pNext) && riDamage);
}

C4Value C4Effect::DoCall(C4Object *pObj, const char *szFn, const C4Value &rVal1, const C4Value &rVal2, const C4Value &rVal3, const C4Value &rVal4, const C4Value &rVal5, const C4Value &rVal6, const C4Value &rVal7)
{
	// def script or global only?
	C4PropList *p = GetCallbackScript();
	// compose function name
	char fn[C4AUL_MAX_Identifier+1];
	sprintf(fn, PSF_FxCustom, GetName(), szFn);
	return p->Call(fn, &C4AulParSet(C4VObj(pObj), C4VPropList(this), rVal1, rVal2, rVal3, rVal4, rVal5, rVal6, rVal7));
}

void C4Effect::OnObjectChangedDef(C4Object *pObj)
{
	// safety
	if (!pObj) return;
	// check all effects for reassignment
	C4Effect *pCheck = this;
	while (pCheck)
	{
		if (pCheck->CommandTarget == pObj)
			pCheck->ReAssignCallbackFunctions();
		pCheck = pCheck->pNext;
	}
}

void C4Effect::TempRemoveUpperEffects(C4Object *pObj, bool fTempRemoveThis, C4Effect **ppLastRemovedEffect)
{
	if (pObj && !pObj->Status) return; // this will be invalid!
	// priority=1: no callbacks
	if (iPriority == 1) return;
	// remove from high to low priority
	// recursive implementation...
	C4Effect *pEff = pNext;
		while (pEff) if (pEff->IsActive()) break; else pEff = pEff->pNext;
	// temp remove active effects with higher priority
	if (pEff) pEff->TempRemoveUpperEffects(pObj, true, ppLastRemovedEffect);
	// temp remove this
	if (fTempRemoveThis)
	{
		FlipActive();
		// Update OnFire cache
		if (pObj && WildcardMatch(C4Fx_AnyFire, GetName()))
			if (!Get(C4Fx_AnyFire))
				pObj->SetOnFire(false);
		// temp callbacks only for higher priority effects
		if (pFnStop && iPriority!=1) pFnStop->Exec(CommandTarget, &C4AulParSet(C4VObj(pObj), C4VPropList(this), C4VInt(C4FxCall_Temp), C4VBool(true)));
		if (!*ppLastRemovedEffect) *ppLastRemovedEffect = this;
	}
}

void C4Effect::TempReaddUpperEffects(C4Object *pObj, C4Effect *pLastReaddEffect)
{
	// nothing to do? - this will also happen if TempRemoveUpperEffects did nothing due to priority==1
	if (!pLastReaddEffect) return;
	if (pObj && !pObj->Status) return; // this will be invalid!
	// simply activate all following, inactive effects
	for (C4Effect *pEff = pNext; pEff; pEff = pEff->pNext)
	{
		if (pEff->IsInactiveAndNotDead())
		{
			pEff->FlipActive();
			if (pEff->pFnStart && pEff->iPriority!=1) pEff->pFnStart->Exec(pEff->CommandTarget, &C4AulParSet(C4VObj(pObj), C4VPropList(pEff), C4VInt(C4FxCall_Temp)));
			if (pObj && WildcardMatch(C4Fx_AnyFire, pEff->GetName()))
				pObj->SetOnFire(true);
		}
		// done?
		if (pEff == pLastReaddEffect) break;
	}
}

void C4Effect::CompileFunc(StdCompiler *pComp, C4ValueNumbers * numbers)
{
	// read name
	pComp->Separator(StdCompiler::SEP_START); // '('
	// read priority
	pComp->Value(iPriority); pComp->Separator();
	// read time and intervall
	pComp->Value(iTime); pComp->Separator();
	pComp->Value(iInterval); pComp->Separator();
	// read object number
	pComp->Value(CommandTarget); pComp->Separator();
	// read ID
	pComp->Value(idCommandTarget); pComp->Separator();
	// proplist
	C4PropListNumbered::CompileFunc(pComp, numbers);
	pComp->Separator(StdCompiler::SEP_END); // ')'
	// is there a next effect?
	bool fNext = !! pNext;
	if (pComp->hasNaming())
	{
		if (fNext || pComp->isCompiler())
			fNext = pComp->Separator();
	}
	else
		pComp->Value(fNext);
	if (!fNext) return;
	// read next
	pComp->Value(mkParAdapt(mkPtrAdaptNoNull(pNext), numbers));
	// denumeration and callback assignment will be done later
}

void C4Effect::SetPropertyByS(C4String * k, const C4Value & to)
{
	if (k >= &Strings.P[0] && k < &Strings.P[P_LAST])
	{
		switch(k - &Strings.P[0])
		{
			case P_Name:
				if (!to.getStr() || !*to.getStr()->GetCStr())
					throw new C4AulExecError("effect: Name has to be a nonempty string");
				C4PropListNumbered::SetPropertyByS(k, to);
				ReAssignCallbackFunctions();
				return;
			case P_Priority:
				throw new C4AulExecError("effect: Priority is readonly");
			case P_Interval: iInterval = to.getInt(); return;
			case P_CommandTarget:
				throw new C4AulExecError("effect: CommandTarget is readonly");
			case P_Time: iTime = to.getInt(); return;
		}
	}
	C4PropListNumbered::SetPropertyByS(k, to);
}

void C4Effect::ResetProperty(C4String * k)
{
	if (k >= &Strings.P[0] && k < &Strings.P[P_LAST])
	{
		switch(k - &Strings.P[0])
		{
			case P_Name:
				throw new C4AulExecError("effect: Name has to be a nonempty string");
			case P_Priority:
				throw new C4AulExecError("effect: Priority is readonly");
			case P_Interval: iInterval = 0; return;
			case P_CommandTarget:
				throw new C4AulExecError("effect: CommandTarget is readonly");
			case P_Time: iTime = 0; return;
		}
	}
	C4PropListNumbered::ResetProperty(k);
}

bool C4Effect::GetPropertyByS(C4String *k, C4Value *pResult) const
{
	if (k >= &Strings.P[0] && k < &Strings.P[P_LAST])
	{
		switch(k - &Strings.P[0])
		{
			case P_Name: return C4PropListNumbered::GetPropertyByS(k, pResult);
			case P_Priority: *pResult = C4VInt(Abs(iPriority)); return true;
			case P_Interval: *pResult = C4VInt(iInterval); return true;
			case P_CommandTarget:
				if (CommandTarget)
					*pResult = C4VObj(CommandTarget);
				else if (idCommandTarget)
					*pResult = C4VPropList(Definitions.ID2Def(idCommandTarget));
				else
					*pResult = C4VNull;
				//*pResult = CommandTarget ? C4VObj(CommandTarget) :
				//           (idCommandTarget ? C4VPropList(Definitions.ID2Def(idCommandTarget)) : C4VNull);
				return true;
			case P_Time: *pResult = C4VInt(iTime); return true;
		}
	}
	return C4PropListNumbered::GetPropertyByS(k, pResult);
}

C4ValueArray * C4Effect::GetProperties() const
{
	C4ValueArray * a = C4PropList::GetProperties();
	int i;
	i = a->GetSize();
	a->SetSize(i + 5);
	(*a)[i++] = C4VString(&::Strings.P[P_Name]);
	(*a)[i++] = C4VString(&::Strings.P[P_Priority]);
	(*a)[i++] = C4VString(&::Strings.P[P_Interval]);
	(*a)[i++] = C4VString(&::Strings.P[P_CommandTarget]);
	(*a)[i++] = C4VString(&::Strings.P[P_Time]);
	return a;
}

// Some other, internal effects -------------------------------------------------------------

static int32_t GetSmokeLevel()
{
	// just use fixed smoke level, smoke uses particles anyway
	return 150;
}

static void BubbleOut(int32_t tx, int32_t ty)
{
	// No bubbles from nowhere
	if (!GBackSemiSolid(tx,ty)) return;
	// User-defined smoke level
	int32_t SmokeLevel = GetSmokeLevel();
	// Enough bubbles out there already
	if (::Objects.ObjectCount(C4ID::Bubble) >= SmokeLevel) return;
	// Create bubble
	Game.CreateObject(C4ID::Bubble,NULL,NO_OWNER,tx,ty);
}

void Splash(int32_t tx, int32_t ty, int32_t amt, C4Object *pByObj)
{
	// Splash only if there is free space above
	if (GBackSemiSolid(tx, ty - 15)) return;
	// get back mat
	int32_t iMat = GBackMat(tx, ty);
	// check liquid
	if (MatValid(iMat))
		if (DensityLiquid(::MaterialMap.Map[iMat].Density) && ::MaterialMap.Map[iMat].Instable)
		{
			int32_t sy = ty;
			while (GBackLiquid(tx, sy) && sy > ty - 20 && sy >= 0) sy--;
			// Splash bubbles and liquid
			for (int32_t cnt=0; cnt<amt; cnt++)
			{
				int32_t bubble_x = tx+Random(16)-8;
				int32_t bubble_y = ty+Random(16)-6;
				BubbleOut(bubble_x,bubble_y);
				if (GBackLiquid(tx,ty) && !GBackSemiSolid(tx, sy))
				{
					C4Real xdir = C4REAL100(Random(151)-75);
					C4Real ydir = C4REAL100(-Random(200));
					::PXS.Create(::Landscape.ExtractMaterial(tx,ty),
					             itofix(tx),itofix(sy),
					             xdir,
					             ydir);
				}
			}
		}
	// Splash sound
	if (amt>=20)
		StartSoundEffect("Splash2",false,100,pByObj);
	else if (amt>1) StartSoundEffect("Splash1",false,100,pByObj);
}

void Smoke(int32_t tx, int32_t ty, int32_t level, DWORD dwClr)
{
	// Use scripted function (global func Smoke) to create smoke
	// Caution: This makes engine internal smoking a synced call.
	C4AulParSet pars(C4VInt(tx), C4VInt(ty), C4VInt(level), C4VInt(dwClr));
	::ScriptEngine.GetPropList()->Call(P_Smoke, &pars);
}
