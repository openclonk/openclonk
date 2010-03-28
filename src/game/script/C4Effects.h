/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001, 2004-2005  Sven Eberhardt
 * Copyright (c) 2005  Peter Wortmann
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

// C4AulFun-based effects assigned to an object
// an effect itself only supplies the callback mechanism for effects assigned to objects
// the effect itself supplies the callback mechanism for creation, destruction, timers
// and overlapped effects
/* Also contains some helper functions for various landscape effects */

#ifndef INC_C4Effects
#define INC_C4Effects

#include <C4Constants.h>
#include <C4ValueList.h>

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
#define C4FxCall_EngGetPunched    40 // energy loss during fighting

// fire drawing modes
#define C4Fx_FireMode_Default      0 // determine mode by category
#define C4Fx_FireMode_LivingVeg    2 // C4D_Living and C4D_StaticBack
#define C4Fx_FireMode_StructVeh    1 // C4D_Structure and C4D_Vehicle
#define C4Fx_FireMode_Object       3 // other (C4D_Object and no bit set (magic))
#define C4Fx_FireMode_Last         3 // largest valid fire mode

#define C4Fx_FireParticle1   "Fire"
#define C4Fx_FireParticle2   "Fire2"

// generic object effect
class C4Effect
{
public:
	char Name[C4MaxDefString+1]; // name of effect
	C4Object *pCommandTarget; // target object for script callbacks - if deleted, the effect is removed without callbacks
	C4ID idCommandTarget;     // ID of command target definition
	int32_t nCommandTarget;       // enumerated ptr for target object (argh, when will this system be changed?)

	int32_t iPriority;          // effect priority for sorting into effect list; -1 indicates a dead effect
	C4ValueList EffectVars; // custom effect variables
	int32_t iTime, iIntervall;  // effect time; effect callback intervall
	int32_t iNumber;            // effect number for addressing

	C4Effect *pNext;        // next effect in linked list

protected:
	// presearched callback functions for faster calling
	C4AulFunc *pFnTimer;           // timer function Fx%sTimer
	C4AulFunc *pFnStart, *pFnStop; // init/deinit-functions Fx%sStart, Fx%sStop
	C4AulFunc *pFnEffect;          // callback if other effect tries to register
	C4AulFunc *pFnDamage;          // callback when owned object gets damage

	void AssignCallbackFunctions(); // resolve callback function names

public:
	C4Effect(C4Object *pForObj, const char *szName, int32_t iPrio, int32_t iTimerIntervall, C4Object *pCmdTarget, C4ID idCmdTarget, C4Value &rVal1, C4Value &rVal2, C4Value &rVal3, C4Value &rVal4, bool fDoCalls, int32_t &riStoredAsNumber); // ctor
	C4Effect(StdCompiler *pComp); // ctor: compile
	~C4Effect();                      // dtor - deletes all following effects

	void EnumeratePointers();  // object pointers to numbers
	void DenumeratePointers(); // numbers to object pointers
	void ClearPointers(C4Object *pObj); // clear all pointers to object - may kill some effects w/o callback, because the callback target is lost

	void SetDead() { iPriority=0; }      // mark effect to be removed in next execution cycle
	bool IsDead() { return !iPriority; } // return whether effect is to be removed
	void FlipActive() { iPriority*=-1; } // alters activation status
	bool IsActive() { return iPriority>0; } // returns whether effect is active
	bool IsInactiveAndNotDead() { return iPriority<0; } // as the name says

	C4Effect *Get(const char *szName, int32_t iIndex=0, int32_t iMaxPriority=0);  // get effect by name
	C4Effect *Get(int32_t iNumber, bool fIncludeDead, int32_t iMaxPriority=0);    // get effect by number
	int32_t GetCount(const char *szMask, int32_t iMaxPriority=0); // count effects that match the mask
	int32_t Check(C4Object *pForObj, const char *szCheckEffect, int32_t iPrio, int32_t iTimer, C4Value &rVal1, C4Value &rVal2, C4Value &rVal3, C4Value &rVal4); // do some effect callbacks
	C4AulScript *GetCallbackScript(); // get script context for effect callbacks

	void Execute(C4Object *pObj); // execute all effects
	void Kill(C4Object *pObj);    // mark this effect deleted and do approprioate calls
	void ClearAll(C4Object *pObj, int32_t iClearFlag);// kill all effects doing removal calls w/o reagard of inactive effects
	void DoDamage(C4Object *pObj, int32_t &riDamage, int32_t iDamageType, int32_t iCausePlr); // ask all effects for damage

	C4Value DoCall(C4Object *pObj, const char *szFn, C4Value &rVal1, C4Value &rVal2, C4Value &rVal3, C4Value &rVal4, C4Value &rVal5, C4Value &rVal6, C4Value &rVal7); // custom call

	void ReAssignCallbackFunctions()
	{ AssignCallbackFunctions(); }
	void ReAssignAllCallbackFunctions()
	{
		ReAssignCallbackFunctions();
		if (pNext) pNext->ReAssignAllCallbackFunctions();
	}
	void OnObjectChangedDef(C4Object *pObj);

	void CompileFunc(StdCompiler *pComp);

protected:
	void TempRemoveUpperEffects(C4Object *pObj, bool fTempRemoveThis, C4Effect **ppLastRemovedEffect); // temp remove all effects with higher priority
	void TempReaddUpperEffects(C4Object *pObj, C4Effect *pLastReaddEffect); // temp remove all effects with higher priority
};

// ctor for StdPtrAdapt
inline void CompileNewFunc(C4Effect *&pRes, StdCompiler *pComp) { pRes = new C4Effect(pComp); }

// fire effect constants
#define MaxFirePhase        15
#define C4Fx_Fire           "Fire"
#define C4Fx_AnyFire        "*Fire*"
#define C4Fx_Internal       "Int*"
#define C4Fx_FirePriority   100
#define C4Fx_FireTimer      1

// fire effect
int32_t FnFxFireStart(C4AulContext *ctx, C4Object *pObj, int32_t iNumber, int32_t iTemp, int32_t iCausedBy, bool fBlasted, C4Object *pIncineratingObject);
int32_t FnFxFireTimer(C4AulContext *ctx, C4Object *pObj, int32_t iNumber, int32_t iTime);
int32_t FnFxFireStop(C4AulContext *ctx, C4Object *pObj, int32_t iNumber, int32_t iReason, bool fTemp);
C4String *FnFxFireInfo(C4AulContext *ctx, C4Object *pObj, int32_t iNumber);
class C4Value &FxFireVarCausedBy(C4Effect *pEffect);

// some other hardcoded engine effects
void Splash(int32_t tx, int32_t ty, int32_t amt, C4Object *pByObj);
void Explosion(int32_t tx, int32_t ty, int32_t level, C4Object *inobj, int32_t iCausedBy, C4Object *pByObj, C4ID idEffect, const char *szEffect);
void Smoke(int32_t tx, int32_t ty, int32_t level, DWORD dwClr=0);
void BubbleOut(int32_t tx, int32_t ty);

#endif
