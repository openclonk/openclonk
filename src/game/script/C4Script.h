/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001, 2004, 2007  Sven Eberhardt
 * Copyright (c) 2001  Peter Wortmann
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

#ifndef INC_C4Script
#define INC_C4Script

#include <C4Value.h>

const int C4SCR_Access_Public			= 0,
					C4SCR_Access_Protected	= 1,
					C4SCR_Access_Private		= 2;

#define C4SCR_Public    "public"
#define C4SCR_Protected "protected"
#define C4SCR_Private   "private"

// ** a definition of a script constant
struct C4ScriptConstDef
	{
	const char *Identifier; // constant name
	C4V_Type ValType;       // type value
	long Data;               // raw data
	};



// ** a definition of a script-function.
// includes two versions of the Function to be called:
// * the first (C4V) takes 10 parameters
// * the second (C4V2) takes an array of 10 parameters
// only one may be set.
struct C4ScriptFnDef
  {
  const char* Identifier; // the name of the func in the script
	BOOL Public;
	C4V_Type RetType; // type returned. ignored when C4V
	C4V_Type ParType[10];// type of the parameters. error when wrong parameter type.
	C4Value (*FunctionC4V)(C4AulContext *cthr, C4Value*, C4Value*, C4Value*, C4Value*, C4Value*,
	                                           C4Value*, C4Value*, C4Value*, C4Value*, C4Value*);
	C4Value (*FunctionC4V2)(struct C4AulContext *, C4Value *);
  };

extern C4ScriptConstDef C4ScriptConstMap[];
extern C4ScriptFnDef C4ScriptFnMap[];

void InitFunctionMap(C4AulScriptEngine *pEngine); // add functions to engine

/* Engine-Calls */

#define PSF_Script							"~Script%i"
#define PSF_Initialize					"~Initialize"
#define PSF_Construction				"~Construction"
#define PSF_Destruction					"~Destruction"
#define PSF_ContentsDestruction "~ContentsDestruction" // C4Object *pContents
#define PSF_InitializePlayer		"~InitializePlayer" // iPlayer, iX, iY, pBase, iTeam, idExtra
#define PSF_InitializeScriptPlayer		"~InitializeScriptPlayer" // iPlayer, idTeam
#define PSF_PreInitializePlayer	"~PreInitializePlayer" // iPlayer
#define PSF_RemovePlayer				"~RemovePlayer" // iPlayer
#define PSF_OnGameOver					"~OnGameOver"
#define PSF_Time1								"~Time1"
#define PSF_Hit									"~Hit"
#define PSF_Hit2								"~Hit2"
#define PSF_Hit3								"~Hit3"
#define PSF_Grab								"~Grab"
#define PSF_Grabbed							"~Grabbed"
#define PSF_Get									"~Get"
#define PSF_Put									"~Put"
#define PSF_Collection					"~Collection" // pObject, fPut
#define PSF_Collection2					"~Collection2" // pObject
#define PSF_Ejection						"~Ejection" // pObject
#define PSF_Entrance						"~Entrance" // pContainer
#define PSF_Departure						"~Departure" // pContainer
#define PSF_Completion					"~Completion"
#define PSF_Purchase						"~Purchase" // iPlayer, pBuyObj
#define PSF_Sale								"~Sale" // iPlayer
#define PSF_Damage							"~Damage" // iChange, iCausedBy
#define PSF_Incineration				"~Incineration" // iCausedBy
#define PSF_IncinerationEx			"~IncinerationEx" // iCausedBy
#define PSF_Death								"~Death" // iCausedBy
#define PSF_ActivateEntrance		"~ActivateEntrance" // pByObject
#define PSF_Activate						"~Activate" // pByObject
#define PSF_LiftTop							"~LiftTop"
#define PSF_Control							"~Control%s"	// pByObject
#define PSF_ContainedControl		"~Contained%s"	// pByObject
#define PSF_ControlUpdate       "~ControlUpdate" // pByObject, iComs
#define PSF_ContainedControlUpdate "~ContainedUpdate" // pByObject, iComs
#define PSF_Contact							"~Contact%s"
#define PSF_ControlCommand			"~ControlCommand" // szCommand, pTarget, iTx, iTy
#define PSF_ControlCommandFinished "~ControlCommandFinished" // szCommand, pTarget, iTx, iTy, pTarget2, iData
#define PSF_DeepBreath					"~DeepBreath"
#define PSF_CatchBlow						"~CatchBlow" // iLevel, pByObject
#define PSF_QueryCatchBlow			"~QueryCatchBlow" // pByObject
#define PSF_Stuck								"~Stuck"
#define PSF_RejectCollection		"~RejectCollect" // idObject, pObject
#define PSF_RejectContents			"~RejectContents" // blocks opening of activate/get/contents menus; no parameters
#define PSF_GrabLost						"~GrabLost"
#define PSF_LineBreak						"~LineBreak" // iCause
#define PSF_BuildNeedsMaterial	"~BuildNeedsMaterial" // idMat1, iAmount1, idMat2, iAmount2...
#define PSF_ControlTransfer			"~ControlTransfer" // C4Object* pObj, int iTx, int iTy
#define PSF_UpdateTransferZone	"~UpdateTransferZone"
#define PSF_CalcValue						"~CalcValue" // C4Object *pInBase, int iForPlayer
#define PSF_CalcDefValue				"~CalcDefValue" // C4Object *pInBase, int iForPlayer
#define PSF_SellTo							"~SellTo" // int iByPlr
#define PSF_InputCallback				"InputCallback" // const char *szText
#define PSF_MenuQueryCancel			"~MenuQueryCancel" // int iSelection
#define PSF_IsFulfilled					"~IsFulfilled"
#define PSF_IsFulfilledforPlr   "~IsFulfilledforPlr" // int iCallPlayer
#define PSF_RejectEntrance			"~RejectEntrance" // C4Object *pIntoObj
#define PSF_RejectFight         "~RejectFight" // C4Object* pEnemy
#define PSF_AttachTargetLost    "~AttachTargetLost"
#define PSF_CrewSelection       "~CrewSelection" // BOOL fDeselect, BOOL fCursorOnly
#define PSF_GetObject2Drop      "~GetObject2Drop" // C4Object *pForCollectionOfObj
#define PSF_LeagueGetResult     "~LeagueGetResult" // int iForPlr
#define PSF_MenuSelection       "~OnMenuSelection" // int iItemIndex, C4Object *pMenuObject
#define PSF_OnActionJump        "~OnActionJump" // int iXDir100, iYDir100
#define PSF_CalcBuyValue        "~CalcBuyValue" // C4ID idItem, int iDefValue
#define PSF_CalcSellValue       "~CalcSellValue" // C4Object *pObj, int iObjValue
#define PSF_MouseSelection      "~MouseSelection" // int iByPlr
#define PSF_OnOwnerChanged      "~OnOwnerChanged" // iNewOwner, iOldOwner
#define PSF_OnJoinCrew          "~Recruitment" // int Player
#define PSF_FxStart             "Fx%sStart" // C4Object *pTarget, int iEffectNumber, int iTemp, C4Value vVar1, C4Value vVar2, C4Value vVar3, C4Value vVar4
#define PSF_FxStop              "Fx%sStop" // C4Object *pTarget, int iEffectNumber, int iReason, BOOL fTemp
#define PSF_FxTimer             "Fx%sTimer" // C4Object *pTarget, int iEffectNumber, int iEffectTime
#define PSF_FxEffect            "Fx%sEffect" // C4String *szNewEffect, C4Object *pTarget, int iEffectNumber, int iNewEffectNumber, C4Value vNewEffectVar1, C4Value vNewEffectVar2, C4Value vNewEffectVar3, C4Value vNewEffectVar4
#define PSF_FxDamage            "Fx%sDamage" // C4Object *pTarget, int iEffectNumber, int iDamage, int iCause, int iCausePlayer
#define PSF_FxCustom            "Fx%s%s" // C4Object *pTarget, int iEffectNumber, C4Value vVar1, C4Value vVar2, C4Value vVar3, C4Value vVar4, C4Value vVar5, C4Value vVar6, C4Value vVar7
#define PSF_FireMode            "~FireMode"
#define PSF_FrameDecoration     "~FrameDecoration%s"
#define PSF_GetFairCrewPhysical "~GetFairCrewPhysical" // C4String *szPhysicalName, int iRank, int iPrevPhysical
#define PSF_DoMagicEnergy       "DoMagicEnergy" // int iChange, C4Object *pObj, bool fAllowPartial
#define PSF_GetCustomComponents "~GetCustomComponents" // C4Object *pBuilder
#define PSF_RejectHostilityChange "~RejectHostilityChange" // int iPlr1, int iPlr2, bool fNewHostility
#define PSF_RejectTeamSwitch      "~RejectTeamSwitch" // int iPlr, int idNewTeam
#define PSF_OnHostilityChange    "~OnHostilityChange" // int iPlr1, int iPlr2, bool fNewHostility, bool fOldHostility
#define PSF_OnTeamSwitch         "~OnTeamSwitch" // int iPlr1, int idNewTeam, int idOldTeam
#define PSF_OnOwnerRemoved       "~OnOwnerRemoved"
#define PSF_PlayerControl        "PlayerControl" // int iPlr, int iControl, C4ID idControlExtraData, int x, int y, int iStrength, bool fRepeated, bool fReleased
#define PSF_Definition           "~Definition" // proplist definition

// Fx%s is automatically prefixed
#define PSFS_FxAdd              "Add" // C4Object *pTarget, int iEffectNumber, C4String *szNewEffect, int iNewTimer, C4Value vNewEffectVar1, C4Value vNewEffectVar2, C4Value vNewEffectVar3, C4Value vNewEffectVar4
#define PSFS_FxInfo             "Info" // C4Object *pTarget, int iEffectNumber

// Construction is used instead of Construct intentionally, because this callback
// is not performed in the beginning of the command; some checks are skipped.
// If there will ever be more generic ControlCommand*-callbacks, there should be
// an additional callback for Construct.
#define PSF_ControlCommandAcquire      "~ControlCommandAcquire" // C4Object *pTarget (unused), int iRangeX, int iRangeY, C4Object *pExcludeContainer, C4ID idAcquireDef
#define PSF_ControlCommandConstruction "~ControlCommandConstruction"  // C4Object *pTarget (unused), int iRangeX, int iRangeY, C4Object *pTarget2 (unused), C4ID idConstructDef

#endif
