/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2013, The OpenClonk Team and contributors
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
// an effect itself only supplies the callback mechanism for effects assigned to objects
// the effect itself supplies the callback mechanism for creation, destruction, timers
// and overlapped effects
/* Also contains some helper functions for various landscape effects */

#ifndef INC_C4Effects
#define INC_C4Effects

#include <C4Constants.h>
#include <C4ObjectPtr.h>
#include <C4PropList.h>

// callback return values
#define C4Fx_OK                      0 // generic standard behaviour for all effect callbacks

#define C4Fx_Effect_Deny            -1 // delete effect
#define C4Fx_Effect_Annul           -2 // delete effect, because it has annulled a countereffect
#define C4Fx_Effect_AnnulCalls      -3 // delete effect, because it has annulled a countereffect; temp readd countereffect

#define C4Fx_Execute_Kill           -1 // execute callback: Remove effect now

#define C4Fx_Stop_Deny            -1 // deny effect removal
#define C4Fx_Start_Deny           -1 // deny effect start

// parameters for effect callbacks
#define C4FxCall_Normal            0 // normal call; effect is being added or removed
#define C4FxCall_Temp              1 // temp call; effect is being added or removed in responce to a lower-level effect change
#define C4FxCall_TempAddForRemoval 2 // temp call; effect is being added because it had been temp removed and is now removed forever
#define C4FxCall_RemoveClear       3 // effect is being removed because object is being removed
#define C4FxCall_RemoveDeath       4 // effect is being removed because object died - return -1 to avoid removal

// damage-callbacks
#define C4FxCall_DmgScript         0 // damage through script call
#define C4FxCall_DmgBlast          1 // damage through blast
#define C4FxCall_DmgFire           2 // damage through fire
#define C4FxCall_DmgChop           3 // damage through chopping

// energy loss callbacks
#define C4FxCall_EngScript        32 // energy loss through script call
#define C4FxCall_EngBlast         33 // energy loss through blast
#define C4FxCall_EngObjHit        34 // energy loss through object hitting the living
#define C4FxCall_EngFire          35 // energy loss through fire
#define C4FxCall_EngBaseRefresh   36 // energy reload in base (also by base object, but that's normally not called)
#define C4FxCall_EngAsphyxiation  37 // energy loss through asphyxiaction
#define C4FxCall_EngCorrosion     38 // energy loss through corrosion (acid)
#define C4FxCall_EngStruct        39 // regular structure energy loss (normally not called)
#define C4FxCall_EngGetPunched    40 // energy loss from Punch

#define C4Fx_FireParticle1   "Fire"
#define C4Fx_FireParticle2   "Fire2"

// generic object effect
class C4Effect: public C4PropListNumbered
{
public:
	C4ObjectPtr CommandTarget; // target object for script callbacks - if deleted, the effect is removed without callbacks
	C4ID idCommandTarget;     // ID of command target definition

	int32_t iPriority;          // effect priority for sorting into effect list; -1 indicates a dead effect
	int32_t iTime, iInterval;  // effect time; effect callback intervall

	C4Effect *pNext;        // next effect in linked list

protected:
	// presearched callback functions for faster calling
	C4AulFunc *pFnTimer;           // timer function Fx%sTimer
	C4AulFunc *pFnStart, *pFnStop; // init/deinit-functions Fx%sStart, Fx%sStop
	C4AulFunc *pFnEffect;          // callback if other effect tries to register
	C4AulFunc *pFnDamage;          // callback when owned object gets damage

	void AssignCallbackFunctions(); // resolve callback function names

	C4Effect(C4Object * pForObj, C4String * szName, int32_t iPrio, int32_t iTimerInterval, C4Object * pCmdTarget, C4ID idCmdTarget, const C4Value &rVal1, const C4Value &rVal2, const C4Value &rVal3, const C4Value &rVal4);
	C4Effect(const C4Effect &); // unimplemented, do not use
	C4Effect(); // for the StdCompiler
	friend void CompileNewFunc<C4Effect, C4ValueNumbers *>(C4Effect *&, StdCompiler *, C4ValueNumbers * const &);
public:
	static C4Effect * New(C4Object * pForObj, C4String * szName, int32_t iPrio, int32_t iTimerInterval, C4Object * pCmdTarget, C4ID idCmdTarget, const C4Value &rVal1, const C4Value &rVal2, const C4Value &rVal3, const C4Value &rVal4);
	~C4Effect();                      // dtor - deletes all following effects

	void Register(C4Object *pForObj, int32_t iPrio);  // add into effect list of object or global effect list
	void Denumerate(C4ValueNumbers *); // numbers to object pointers
	void ClearPointers(C4Object *pObj); // clear all pointers to object - may kill some effects w/o callback, because the callback target is lost

	void SetDead() { iPriority=0; }      // mark effect to be removed in next execution cycle
	bool IsDead() { return !iPriority; } // return whether effect is to be removed
	void FlipActive() { iPriority*=-1; } // alters activation status
	bool IsActive() { return iPriority>0; } // returns whether effect is active
	bool IsInactiveAndNotDead() { return iPriority<0; } // as the name says

	C4Effect *Get(const char *szName, int32_t iIndex=0, int32_t iMaxPriority=0);  // get effect by name
	int32_t GetCount(const char *szMask, int32_t iMaxPriority=0); // count effects that match the mask
	C4Effect *Check(C4Object *pForObj, const char *szCheckEffect, int32_t iPrio, int32_t iTimer, const C4Value &rVal1, const C4Value &rVal2, const C4Value &rVal3, const C4Value &rVal4); // do some effect callbacks
	C4PropList * GetCallbackScript(); // get script context for effect callbacks

	void Execute(C4Object *pObj); // execute all effects
	void Kill(C4Object *pObj);    // mark this effect deleted and do approprioate calls
	void ClearAll(C4Object *pObj, int32_t iClearFlag);// kill all effects doing removal calls w/o reagard of inactive effects
	void DoDamage(C4Object *pObj, int32_t &riDamage, int32_t iDamageType, int32_t iCausePlr); // ask all effects for damage

	C4Value DoCall(C4Object *pObj, const char *szFn, const C4Value &rVal1, const C4Value &rVal2, const C4Value &rVal3, const C4Value &rVal4, const C4Value &rVal5, const C4Value &rVal6, const C4Value &rVal7); // custom call

	void ReAssignCallbackFunctions()
	{ AssignCallbackFunctions(); }
	void ReAssignAllCallbackFunctions()
	{
		ReAssignCallbackFunctions();
		if (pNext) pNext->ReAssignAllCallbackFunctions();
	}
	void OnObjectChangedDef(C4Object *pObj);

	void CompileFunc(StdCompiler *pComp, C4ValueNumbers *);
	virtual C4Effect * GetEffect() { return this; }
	virtual void SetPropertyByS(C4String * k, const C4Value & to);
	virtual void ResetProperty(C4String * k);
	virtual bool GetPropertyByS(C4String *k, C4Value *pResult) const;
	virtual C4ValueArray * GetProperties() const;

protected:
	void TempRemoveUpperEffects(C4Object *pObj, bool fTempRemoveThis, C4Effect **ppLastRemovedEffect); // temp remove all effects with higher priority
	void TempReaddUpperEffects(C4Object *pObj, C4Effect *pLastReaddEffect); // temp remove all effects with higher priority
};

// fire effect constants
#define MaxFirePhase        15
#define C4Fx_Fire           "Fire"
#define C4Fx_AnyFire        "*Fire*"
#define C4Fx_Internal       "Int*"
#define C4Fx_FirePriority   100
#define C4Fx_FireTimer      1

// some other hardcoded engine effects
void Splash(int32_t tx, int32_t ty, int32_t amt, C4Object *pByObj);
void Smoke(int32_t tx, int32_t ty, int32_t level, DWORD dwClr=0);

#endif
