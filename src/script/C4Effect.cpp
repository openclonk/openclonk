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

// C4AulFun-based effects assigned to an object
/* Also contains some helper functions for various landscape effects */

#include "C4Include.h"
#include "script/C4Effect.h"

#include "game/C4GameScript.h"
#include "script/C4Aul.h"

void C4Effect::AssignCallbackFunctions()
{
	C4PropList *p = GetCallbackScript();
	if (!p) return;
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
	return CommandTarget._getPropList();
}

C4Effect::C4Effect(C4Effect **ppEffectList, C4String *szName, int32_t iPrio, int32_t iTimerInterval, C4PropList *pCmdTarget)
{
	// assign values
	iPriority = 0; // effect is not yet valid; some callbacks to other effects are done before
	iInterval = iTimerInterval;
	iTime = 0;
	CommandTarget.SetPropList(pCmdTarget);
	AcquireNumber();
	Register(ppEffectList, iPrio);
	// Set name and callback functions
	SetProperty(P_Name, C4VString(szName));
}

C4Effect::C4Effect(C4Effect **ppEffectList, C4PropList * prototype, int32_t iPrio, int32_t iTimerInterval):
		C4PropListNumbered(prototype)
{
	// assign values
	iPriority = 0; // effect is not yet valid; some callbacks to other effects are done before
	iInterval = iTimerInterval;
	iTime = 0;
	CommandTarget.Set0();
	AcquireNumber();
	Register(ppEffectList, iPrio);
	SetProperty(P_Name, C4VString(prototype->GetName()));
}

void C4Effect::Register(C4Effect **ppEffectList, int32_t iPrio)
{
	// get effect target
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

C4Effect * C4Effect::New(C4PropList *pForObj, C4Effect **ppEffectList, C4String * szName, int32_t iPrio, int32_t iTimerInterval, C4PropList * pCmdTarget, const C4Value &rVal1, const C4Value &rVal2, const C4Value &rVal3, const C4Value &rVal4)
{
	C4Effect * pEffect = new C4Effect(ppEffectList, szName, iPrio, iTimerInterval, pCmdTarget);
	return pEffect->Init(pForObj, iPrio, rVal1, rVal2, rVal3, rVal4);
}

C4Effect * C4Effect::New(C4PropList *pForObj, C4Effect **ppEffectList, C4PropList * prototype, int32_t iPrio, int32_t iTimerInterval, const C4Value &rVal1, const C4Value &rVal2, const C4Value &rVal3, const C4Value &rVal4)
{
	C4Effect * pEffect = new C4Effect(ppEffectList, prototype, iPrio, iTimerInterval);
	return pEffect->Init(pForObj, iPrio, rVal1, rVal2, rVal3, rVal4);
}

C4Effect * C4Effect::Init(C4PropList *pForObj, int32_t iPrio, const C4Value &rVal1, const C4Value &rVal2, const C4Value &rVal3, const C4Value &rVal4)
{
	Target = pForObj;
	// ask all effects with higher priority first - except for prio 1 effects, which are considered out of the priority call chain (as per doc)
	bool fRemoveUpper = (iPrio != 1);
	// note that apart from denying the creation of this effect, higher priority effects may also remove themselves
	// or do other things with the effect list
	// (which does not quite make sense, because the effect might be denied by another effect)
	// so the priority is assigned after this call, marking this effect dead before it's definitely valid
	if (fRemoveUpper && pNext)
	{
		C4Effect * pEffect2 = pNext->Check(GetName(), iPrio, iInterval, rVal1, rVal2, rVal3, rVal4);
		if (pEffect2)
		{
			// effect denied (iResult = -1), added to an effect (iResult = Number of that effect)
			// or added to an effect that destroyed itself (iResult = -2)
			if (pEffect2 != (C4Effect*)C4Fx_Effect_Deny && pEffect2 != (C4Effect*)C4Fx_Effect_Annul) return pEffect2;
			// effect is still marked dead
			return nullptr;
		}
	}
	// init effect
	// higher-priority effects must be deactivated temporarily, and then reactivated regarding the new effect
	// higher-level effects should not be inserted during the process of removing or adding a lower-level effect
	// because that would cause a wrong initialization order
	// (hardly ever causing trouble, however...)
	C4Effect *pLastRemovedEffect=nullptr;
	C4AulFunc * pFn;
	if (!GetCallbackScript())
	{
		Call(P_Construction, &C4AulParSet(rVal1, rVal2, rVal3, rVal4)).getInt();
		if (pForObj && !pForObj->Status) return nullptr;
		pFn = GetFunc(P_Start);
	}
	else
		pFn = pFnStart;
	if (fRemoveUpper && pNext && pFn)
		TempRemoveUpperEffects(false, &pLastRemovedEffect);
	// bad things may happen
	if (pForObj && !pForObj->Status) return nullptr; // this will be invalid!
	iPriority = iPrio; // validate effect now
	if (CallStart(0, rVal1, rVal2, rVal3, rVal4) == C4Fx_Start_Deny)
		// the effect denied to start: assume it hasn't, and mark it dead
		SetDead();
	if (fRemoveUpper && pNext && pFn)
		TempReaddUpperEffects(pLastRemovedEffect);
	if (pForObj && !pForObj->Status) return nullptr; // this will be invalid!
	// Update OnFire cache
	if (!IsDead() && pForObj && WildcardMatch(C4Fx_AnyFire, GetName()))
		pForObj->SetOnFire(true);
	return this;
}

C4Effect::C4Effect()
{
	// defaults
	iPriority=iTime=iInterval=0;
	CommandTarget.Set0();
	pNext = nullptr;
}

C4Effect::~C4Effect()
{
	// del following effects (not recursively)
	C4Effect *pEffect;
	while ((pEffect = pNext))
	{
		pNext = pEffect->pNext;
		pEffect->pNext = nullptr;
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
		pEff->CommandTarget.Denumerate(numbers);
		// assign any callback functions
		pEff->AssignCallbackFunctions();
		pEff->C4PropList::Denumerate(numbers);
	}
	while ((pEff=pEff->pNext));
}

void C4Effect::ClearPointers(C4PropList *pObj)
{
	// clear pointers in all effects
	C4Effect *pEff = this;
	do
		// command target lost: effect dead w/o callback
		if (pEff->CommandTarget.getPropList() == pObj)
		{
			pEff->SetDead();
			pEff->CommandTarget.Set0();
		}
	while ((pEff=pEff->pNext));
}

void C4Effect::SetDead()
{
	iPriority = 0;
}

C4Effect *C4Effect::Get(const char *szName, int32_t iIndex, int32_t iMaxPriority)
{
	// safety
	if (!szName) return nullptr;
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
	return nullptr;
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

C4Effect* C4Effect::Check(const char *szCheckEffect, int32_t iPrio, int32_t iTimer, const C4Value &rVal1, const C4Value &rVal2, const C4Value &rVal3, const C4Value &rVal4)
{
	// priority=1: always OK; no callbacks
	if (iPrio == 1) return nullptr;
	// check this and other effects
	C4Effect *pAddToEffect = nullptr; bool fDoTempCallsForAdd = false;
	C4Effect *pLastRemovedEffect=nullptr;
	for (C4Effect *pCheck = this; pCheck; pCheck = pCheck->pNext)
	{
		if (!pCheck->IsDead() && pCheck->iPriority >= iPrio)
		{
			int32_t iResult = pCheck->CallEffect(szCheckEffect, rVal1, rVal2, rVal3, rVal4);
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
			pAddToEffect->TempRemoveUpperEffects(false, &pLastRemovedEffect);
		C4Value Par1 = C4VString(szCheckEffect), Par2 = C4VInt(iTimer), Par8;
		int32_t iResult = pAddToEffect->DoCall(Target, PSFS_FxAdd, Par1, Par2, rVal1, rVal2, rVal3, rVal4, Par8).getInt();
		// do temp readd calls if desired
		if (pAddToEffect->pNext && fDoTempCallsForAdd)
			pAddToEffect->TempReaddUpperEffects(pLastRemovedEffect);
		// effect removed by this call?
		if (iResult == C4Fx_Start_Deny)
		{
			pAddToEffect->Kill();
			return (C4Effect*)C4Fx_Effect_Annul;
		}
		else
			// other effect is the target effect number
			return pAddToEffect;
	}
	// added to no effect and not denied
	return nullptr;
}

void C4Effect::Execute(C4Effect **ppEffectList)
{
	// advance all effect timers first; then do execution
	// this prevents a possible endless loop if timers register into the same effect list with interval 1 while it is being executed
	for (C4Effect *pEffect = *ppEffectList; pEffect; pEffect = pEffect->pNext)
	{
		// ignore dead status; adjusting their time doesn't hurt
		++pEffect->iTime;
	}
	// get effect list
	// execute all effects not marked as dead
	C4Effect *pEffect = *ppEffectList, **ppPrevEffect=ppEffectList;
	while (pEffect)
	{
		// effect dead?
		if (pEffect->IsDead())
		{
			// delete it, then
			C4Effect *pNextEffect = pEffect->pNext;
			pEffect->pNext = nullptr;
			delete pEffect;
			// next effect
			*ppPrevEffect = pEffect = pNextEffect;
		}
		else
		{
			// check timer execution
			if (pEffect->iInterval && !(pEffect->iTime % pEffect->iInterval) && pEffect->iTime)
			{
				if (pEffect->CallTimer(pEffect->iTime) == C4Fx_Execute_Kill)
				{
					// safety: this class got deleted!
					if (pEffect->Target && !pEffect->Target->Status) return;
					// timer function decided to finish it
					pEffect->Kill();
				}
				// safety: this class got deleted!
				if (pEffect->Target && !pEffect->Target->Status) return;
			}
			// next effect
			ppPrevEffect = &pEffect->pNext;
			pEffect = pEffect->pNext;
		}
	}
}

void C4Effect::Kill()
{
	// active?
	C4Effect *pLastRemovedEffect=nullptr;
	if (IsActive())
		// then temp remove all higher priority effects
		TempRemoveUpperEffects(false, &pLastRemovedEffect);
	else
		// otherwise: temp reactivate before real removal
		// this happens only if a lower priority effect removes an upper priority effect in its add- or removal-call
		if (iPriority!=1) CallStart(C4FxCall_TempAddForRemoval, C4Value(), C4Value(), C4Value(), C4Value());
	// remove this effect
	int32_t iPrevPrio = iPriority; SetDead();
	if (CallStop(C4FxCall_Normal, false) == C4Fx_Stop_Deny)
		// effect denied to be removed: recover
		iPriority = iPrevPrio;
	// reactivate other effects
	TempReaddUpperEffects(pLastRemovedEffect);
	// Update OnFire cache
	if (Target && WildcardMatch(C4Fx_AnyFire, GetName()))
		if (!Get(C4Fx_AnyFire))
			Target->SetOnFire(false);
	if (IsDead() && !GetCallbackScript())
		Call(P_Destruction, &C4AulParSet(C4FxCall_Normal));
}

void C4Effect::ClearAll(int32_t iClearFlag)
{
	// simply remove access all effects recursively, and do removal calls
	// this does not regard lower-level effects being added in the removal calls,
	// because this could hang the engine with poorly coded effects
	if (pNext) pNext->ClearAll(iClearFlag);
	if ((Target && !Target->Status) || IsDead()) return;
	int32_t iPrevPrio = iPriority;
	SetDead();
	if (CallStop(iClearFlag, false) == C4Fx_Stop_Deny)
	{
		// this stop-callback might have deleted the object and then denied its own removal
		// must not modify self in this case...
		if (Target && !Target->Status) return;
		// effect denied to be removed: recover it
		iPriority = iPrevPrio;
	}
	// Update OnFire cache
	if (Target && WildcardMatch(C4Fx_AnyFire, GetName()) && IsDead())
		if (!Get(C4Fx_AnyFire))
			Target->SetOnFire(false);
	if (IsDead() && !GetCallbackScript())
		Call(P_Destruction, &C4AulParSet(iClearFlag));
}

void C4Effect::DoDamage(int32_t &riDamage, int32_t iDamageType, int32_t iCausePlr)
{
	// ask all effects for damage adjustments
	C4Effect *pEff = this;
	do
	{
		if (!pEff->IsDead())
			pEff->CallDamage(riDamage, iDamageType, iCausePlr);
		if (Target && !Target->Status) return;
	}
	while ((pEff = pEff->pNext) && riDamage);
}

static C4Object * Obj(C4PropList * p) { return p ? p->GetObject() : nullptr; }

C4Value C4Effect::DoCall(C4PropList *pObj, const char *szFn, const C4Value &rVal1, const C4Value &rVal2, const C4Value &rVal3, const C4Value &rVal4, const C4Value &rVal5, const C4Value &rVal6, const C4Value &rVal7)
{
	C4PropList * p = GetCallbackScript();
	if (!p)
	{
		C4AulFunc * fn = GetFunc(szFn);
		if (fn) return fn->Exec(this, &C4AulParSet(rVal1, rVal2, rVal3, rVal4, rVal5, rVal6, rVal7));
	}
	else
	{
		// old variant
		// compose function name
		C4AulFunc * fn = p->GetFunc(FormatString(PSF_FxCustom, GetName(), szFn).getData());
		if (fn) return fn->Exec(p, &C4AulParSet(Obj(pObj), this, rVal1, rVal2, rVal3, rVal4, rVal5, rVal6, rVal7));
	}
	return C4Value();
}

int C4Effect::CallStart(int temporary, const C4Value &var1, const C4Value &var2, const C4Value &var3, const C4Value &var4)
{
	if (!GetCallbackScript())
		return Call(P_Start, &C4AulParSet(temporary, var1, var2, var3, var4)).getInt();
	if (pFnStart)
		return pFnStart->Exec(GetCallbackScript(), &C4AulParSet(Obj(Target), this, temporary, var1, var2, var3, var4)).getInt();
	return C4Fx_OK;
}
int C4Effect::CallStop(int reason, bool temporary)
{
	if (!GetCallbackScript())
		return Call(P_Stop, &C4AulParSet(reason, temporary)).getInt();
	if (pFnStop)
		return pFnStop->Exec(GetCallbackScript(), &C4AulParSet(Obj(Target), this, reason, temporary)).getInt();
	return C4Fx_OK;
}
int C4Effect::CallTimer(int time)
{
	try
	{
		if (!GetCallbackScript())
			return Call(P_Timer, &C4AulParSet(time), true).getInt();
		if (pFnTimer)
			return pFnTimer->Exec(GetCallbackScript(), &C4AulParSet(Obj(Target), this, time), true).getInt();
	}
	catch (C4AulError &e)
	{
		// Script error: remove the timer.
		// TODO: The error message is printed after the stack trace. No way around that currently.
		::ScriptEngine.GetErrorHandler()->OnError(e.what());
		// => Removing effect { ... }
		DebugLogF(" Removing %s", C4Value(this).GetDataString(3).getData());
	}
	return C4Fx_Execute_Kill;
}
void C4Effect::CallDamage(int32_t & damage, int damagetype, int plr)
{
	if (!GetCallbackScript())
	{
		C4AulFunc *pFn = GetFunc(P_Damage);
		if (pFn)
			damage = pFn->Exec(this, &C4AulParSet(damage, damagetype, plr)).getInt();
	}
	else if (pFnDamage)
		damage = pFnDamage->Exec(GetCallbackScript(), &C4AulParSet(Obj(Target), this, damage, damagetype, plr)).getInt();
}
int C4Effect::CallEffect(const char * effect, const C4Value &var1, const C4Value &var2, const C4Value &var3, const C4Value &var4)
{
	if (!GetCallbackScript())
		return Call(P_Effect, &C4AulParSet(effect, var1, var2, var3, var4)).getInt();
	if (pFnEffect)
		return pFnEffect->Exec(GetCallbackScript(), &C4AulParSet(effect, Obj(Target), this, var1, var2, var3, var4)).getInt();
	return C4Fx_OK;
}

void C4Effect::OnObjectChangedDef(C4PropList *pObj)
{
	// safety
	if (!pObj) return;
	// check all effects for reassignment
	C4Effect *pCheck = this;
	while (pCheck)
	{
		if (pCheck->GetCallbackScript() == pObj)
			pCheck->ReAssignCallbackFunctions();
		pCheck = pCheck->pNext;
	}
}

void C4Effect::TempRemoveUpperEffects(bool fTempRemoveThis, C4Effect **ppLastRemovedEffect)
{
	if (Target && !Target->Status) return; // this will be invalid!
	// priority=1: no callbacks
	if (iPriority == 1) return;
	// remove from high to low priority
	// recursive implementation...
	C4Effect *pEff = pNext;
		while (pEff) if (pEff->IsActive()) break; else pEff = pEff->pNext;
	// temp remove active effects with higher priority
	if (pEff) pEff->TempRemoveUpperEffects(true, ppLastRemovedEffect);
	// temp remove this
	if (fTempRemoveThis)
	{
		FlipActive();
		// Update OnFire cache
		if (Target && WildcardMatch(C4Fx_AnyFire, GetName()))
			if (!Get(C4Fx_AnyFire))
				Target->SetOnFire(false);
		// temp callbacks only for higher priority effects
		if (iPriority!=1) CallStop(C4FxCall_Temp, true);
		if (!*ppLastRemovedEffect) *ppLastRemovedEffect = this;
	}
}

void C4Effect::TempReaddUpperEffects(C4Effect *pLastReaddEffect)
{
	// nothing to do? - this will also happen if TempRemoveUpperEffects did nothing due to priority==1
	if (!pLastReaddEffect) return;
	if (Target && !Target->Status) return; // this will be invalid!
	// simply activate all following, inactive effects
	for (C4Effect *pEff = pNext; pEff; pEff = pEff->pNext)
	{
		if (pEff->IsInactiveAndNotDead())
		{
			pEff->FlipActive();
			if (pEff->iPriority!=1) pEff->CallStart(C4FxCall_Temp, C4Value(), C4Value(), C4Value(), C4Value());
			if (Target && WildcardMatch(C4Fx_AnyFire, pEff->GetName()))
				Target->SetOnFire(true);
		}
		// done?
		if (pEff == pLastReaddEffect) break;
	}
}

void C4Effect::CompileFunc(StdCompiler *pComp, C4PropList * Owner, C4ValueNumbers * numbers)
{
	if (pComp->isDeserializer()) Target = Owner;
	// read name
	pComp->Separator(StdCompiler::SEP_START); // '('
	// read priority
	pComp->Value(iPriority); pComp->Separator();
	// read time and intervall
	pComp->Value(iTime); pComp->Separator();
	pComp->Value(iInterval); pComp->Separator();
	// read object number
	// FIXME: replace with this when savegame compat breaks for other reasons
	// pComp->Value(mkParAdapt(CommandTarget, numbers));
	int32_t nptr = 0;
	if (!pComp->isDeserializer() && CommandTarget.getPropList() && CommandTarget._getPropList()->GetPropListNumbered())
		nptr = CommandTarget._getPropList()->GetPropListNumbered()->Number;
	pComp->Value(nptr);
	if (pComp->isDeserializer())
		CommandTarget.SetObjectEnum(nptr);
	pComp->Separator();
	// read ID
	if (pComp->isSerializer())
	{
		const C4PropListStatic * p = CommandTarget.getPropList() ? CommandTarget._getPropList()->IsStatic() : nullptr;
		if (p)
			p->RefCompileFunc(pComp, numbers);
		else
			pComp->String(const_cast<char*>("None"), 5, StdCompiler::RCT_ID);
	}
	else
	{
		StdStrBuf s;
		pComp->Value(mkParAdapt(s, StdCompiler::RCT_ID));
		// An Object trumps a definition as command target
		if (!nptr)
			if (!::ScriptEngine.GetGlobalConstant(s.getData(), &CommandTarget))
				CommandTarget.Set0();
	}
	pComp->Separator();
	// proplist
	C4PropListNumbered::CompileFunc(pComp, numbers);
	pComp->Separator(StdCompiler::SEP_END); // ')'
	// is there a next effect?
	bool fNext = !! pNext;
	if (pComp->hasNaming())
	{
		if (fNext || pComp->isDeserializer())
			fNext = pComp->Separator();
	}
	else
		pComp->Value(fNext);
	if (!fNext) return;
	// read next
	pComp->Value(mkParAdapt(mkPtrAdaptNoNull(pNext), Owner, numbers));
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
					throw C4AulExecError("effect: Name has to be a nonempty string");
				C4PropListNumbered::SetPropertyByS(k, to);
				ReAssignCallbackFunctions();
				return;
			case P_Priority:
				throw C4AulExecError("effect: Priority is readonly");
			case P_Interval: iInterval = to.getInt(); return;
			case P_CommandTarget:
				throw C4AulExecError("effect: CommandTarget is readonly");
			case P_Target:
				throw C4AulExecError("effect: Target is readonly");
			case P_Time: iTime = to.getInt(); return;
			case P_Prototype:
				throw new C4AulExecError("effect: Prototype is readonly");
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
				throw C4AulExecError("effect: Name has to be a nonempty string");
			case P_Priority:
				throw C4AulExecError("effect: Priority is readonly");
			case P_Interval: iInterval = 0; return;
			case P_CommandTarget:
				throw C4AulExecError("effect: CommandTarget is readonly");
			case P_Target:
				throw C4AulExecError("effect: Target is readonly");
			case P_Time: iTime = 0; return;
			case P_Prototype:
				throw new C4AulExecError("effect: Prototype is readonly");
		}
	}
	C4PropListNumbered::ResetProperty(k);
}

bool C4Effect::GetPropertyByS(const C4String *k, C4Value *pResult) const
{
	if (k >= &Strings.P[0] && k < &Strings.P[P_LAST])
	{
		switch(k - &Strings.P[0])
		{
			case P_Name: return C4PropListNumbered::GetPropertyByS(k, pResult);
			case P_Priority: *pResult = C4VInt(Abs(iPriority)); return true;
			case P_Interval: *pResult = C4VInt(iInterval); return true;
			case P_CommandTarget: *pResult = CommandTarget; return true;
			case P_Target: *pResult = C4Value(Target); return true;
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
	(*a)[i++] = C4VString(&::Strings.P[P_Target]);
	(*a)[i++] = C4VString(&::Strings.P[P_Time]);
	return a;
}
