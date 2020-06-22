/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2010-2016, The OpenClonk Team and contributors
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

#ifndef INC_C4GameScript
#define INC_C4GameScript

#include "script/C4Value.h"

// add functions to engine
void InitGameFunctionMap(C4AulScriptEngine *pEngine);
void InitObjectFunctionMap(C4AulScriptEngine *pEngine);

bool C4ValueToMatrix(C4Value& value, StdMeshMatrix* matrix);
bool C4ValueToMatrix(const C4ValueArray& array, StdMeshMatrix* matrix);

/* Engine-Calls */

#define PSF_Initialize          "~Initialize"
#define PSF_InitializePlayers   "~InitializePlayers"
#define PSF_InitializeAmbience  "~InitializeAmbience"
#define PSF_Construction        "~Construction"
#define PSF_Destruction         "~Destruction"
#define PSF_ContentsDestruction "~ContentsDestruction" // C4Object *pContents
#define PSF_InitializePlayer    "~InitializePlayer" // iPlayer, iX, iY, pBase, iTeam, idExtra
#define PSF_InitializeScriptPlayer    "~InitializeScriptPlayer" // iPlayer, idTeam
#define PSF_PreInitializePlayer "~PreInitializePlayer" // iPlayer
#define PSF_InitializePlayerControl "~InitializePlayerControl" // iPlayer, szControlSet, hasKeyboard, hasMouse, hasGamepad
#define PSF_InitializeMap       "~InitializeMap" // map
#define PSF_InitializeObjects   "~InitializeObjects"
#define PSF_RemovePlayer        "~RemovePlayer" // iPlayer
#define PSF_RelaunchPlayer      "~RelaunchPlayer" // iPlayer, iKilledBy
#define PSF_Time1               "~Time1"
#define PSF_Hit                 "~Hit"
#define PSF_Hit2                "~Hit2"
#define PSF_Hit3                "~Hit3"
#define PSF_Grab                "~Grab" // pObject, fGrab
#define PSF_Grabbed             "~Grabbed"
#define PSF_Get                 "~Get"
#define PSF_Put                 "~Put"
#define PSF_Collection          "~Collection" // pObject, fPut
#define PSF_Collection2         "~Collection2" // pObject
#define PSF_Ejection            "~Ejection" // pObject
#define PSF_Entrance            "~Entrance" // pContainer
#define PSF_Departure           "~Departure" // pContainer
#define PSF_Purchase            "~Purchase" // iPlayer, pBuyObj
#define PSF_Sale                "~Sale" // iPlayer
#define PSF_Damage              "~Damage" // iChange, iCausedBy
#define PSF_Incineration        "~Incineration" // iCausedBy
#define PSF_IncinerationEx      "~IncinerationEx" // iCausedBy
#define PSF_Death               "~Death" // iCausedBy
#define PSF_ActivateEntrance    "~ActivateEntrance" // pByObject
#define PSF_LiftTop             "~LiftTop"
#define PSF_Contact             "~Contact%s"
#define PSF_ControlCommand      "~ControlCommand" // szCommand, pTarget, iTx, iTy
#define PSF_ControlCommandFinished "~ControlCommandFinished" // szCommand, pTarget, iTx, iTy, pTarget2, iData
#define PSF_CatchBlow           "~CatchBlow" // iLevel, pByObject
#define PSF_QueryCatchBlow      "~QueryCatchBlow" // pByObject
#define PSF_Stuck               "~Stuck"
#define PSF_GrabLost            "~GrabLost"
#define PSF_OnLineBreak         "~OnLineBreak" // iCause
#define PSF_OnLineChange        "~OnLineChange" // current_length
#define PSF_ControlTransfer     "~ControlTransfer" // C4Object* pObj, int iTx, int iTy
#define PSF_OnSynchronized       "~OnSynchronized"
#define PSF_CalcValue           "~CalcValue" // C4Object *pInBase, int iForPlayer
#define PSF_CalcDefValue        "~CalcDefValue" // C4Object *pInBase, int iForPlayer
#define PSF_InputCallback       "InputCallback" // const char *szText
#define PSF_MenuQueryCancel     "~MenuQueryCancel" // int iSelection
#define PSF_IsFulfilled         "~IsFulfilled" // int for_plr
#define PSF_AttachTargetLost    "~AttachTargetLost"
#define PSF_CrewSelection       "~CrewSelection" // bool fDeselect
#define PSF_GetObject2Drop      "~GetObject2Drop" // C4Object *pForCollectionOfObj
#define PSF_LeagueGetResult     "~LeagueGetResult" // int iForPlr
#define PSF_FireMode            "~FireMode"
#define PSF_FrameDecoration     "~FrameDecoration%s"
#define PSF_CalcBuyValue        "~CalcBuyValue" // C4ID idItem, int iDefValue
#define PSF_CalcSellValue       "~CalcSellValue" // C4Object *pObj, int iObjValue
#define PSF_OnJoinCrew          "~Recruitment" // int Player
#define PSF_OnRemoveCrew        "~DeRecruitment" // int Player
#define PSF_OnInIncendiaryMaterial "OnInIncendiaryMaterial"
#define PSF_EditCursorSelection   "~EditCursorSelection"
#define PSF_EditCursorDeselection "~EditCursorDeselection" // new_selection
#define PSF_EditCursorMoved     "~EditCursorMoved" // int old_x, int old_y
#define PSF_DigOutObject        "~DigOutObject" // C4Object *obj
#define PSF_OnDugOut            "~DugOut" //C4Object *by_obj
#define PSF_SaveScenarioObjects "~SaveScenarioObjects" // int file_handle, [array duplication_list]
#define PSF_CommandFailure      "~CommandFailure" // string command, pTarget, iTx, iTy, pTarget2, iData
#define PSF_OnCompletionChange  "~OnCompletionChange" // int old_con, int new_con

#define PSF_CollectStatistics   "CollectStatistics"

// Effect callbacks

#define PSF_FxStart             "Fx%sStart" // C4Object *pTarget, int iEffectNumber, int iTemp, C4Value vVar1, C4Value vVar2, C4Value vVar3, C4Value vVar4
#define PSF_FxStop              "Fx%sStop" // C4Object *pTarget, int iEffectNumber, int iReason, bool fTemp
#define PSF_FxTimer             "Fx%sTimer" // C4Object *pTarget, int iEffectNumber, int iEffectTime
#define PSF_FxEffect            "Fx%sEffect" // C4String *szNewEffect, C4Object *pTarget, int iEffectNumber, int iNewEffectNumber, C4Value vNewEffectVar1, C4Value vNewEffectVar2, C4Value vNewEffectVar3, C4Value vNewEffectVar4
#define PSF_FxDamage            "Fx%sDamage" // C4Object *pTarget, int iEffectNumber, int iDamage, int iCause, int iCausePlayer
#define PSF_FxCustom            "Fx%s%s" // C4Object *pTarget, int iEffectNumber, C4Value vVar1, C4Value vVar2, C4Value vVar3, C4Value vVar4, C4Value vVar5, C4Value vVar6, C4Value vVar7

// Controls

#define PSF_PlayerControl            "PlayerControl" // int iPlr, int iControl, C4ID idControlExtraData, int x, int y, int iStrength, bool fRepeated, bool fReleased
#define PSF_MouseSelection           "~MouseSelection" // int iByPlr
#define PSF_MouseSelectionAlt        "~MouseSelectionAlt" // int iByPlr
#define PSF_MouseDragDrop            "~MouseDragDrop" // int iPlr, C4Object *source, C4Object *target
#define PSF_MouseHover               "~MouseHover" // int iPlr, C4Object* old, C4Object* new, C4Object* drag

// Proplist

#define PSF_Definition               "~Definition" // proplist definition

// Reject* Callbacks

#define PSF_RejectHostilityChange    "~RejectHostilityChange" // int iPlr1, int iPlr2, bool fNewHostility
#define PSF_RejectTeamSwitch         "~RejectTeamSwitch" // int iPlr, int idNewTeam
#define PSF_RejectEntrance           "~RejectEntrance" // C4Object *pIntoObj
#define PSF_RejectCollection         "~RejectCollect" // idObject, pObject
#define PSF_RejectContents           "~RejectContents" // blocks opening of activate/get/contents menus; no parameters

// On* Callbacks

#define PSF_OnGameOver               "~OnGameOver"
#define PSF_MenuSelection            "~OnMenuSelection" // int iItemIndex, C4Object *pMenuObject
#define PSF_OnActionJump             "~OnActionJump" // int iXDir100, iYDir100
#define PSF_OnOwnerChanged           "~OnOwnerChanged" // iNewOwner, iOldOwner
#define PSF_EnergyChange             "~OnEnergyChange" // int iChange, int iCause, int iCausedByPlayer
#define PSF_BreathChange             "~OnBreathChange" // int iChange
#define PSF_OnHostilityChange        "~OnHostilityChange" // int iPlr1, int iPlr2, bool fNewHostility, bool fOldHostility
#define PSF_OnTeamSwitch             "~OnTeamSwitch" // int iPlr1, int idNewTeam, int idOldTeam
#define PSF_OnOwnerRemoved           "~OnOwnerRemoved"
#define PSF_Promotion                "~OnPromotion"
#define PSF_CrewEnabled              "~OnCrewEnabled"
#define PSF_CrewDisabled             "~OnCrewDisabled"
#define PSF_NameChange               "~OnNameChanged" // bool inInfoSection
#define PSF_OnWealthChanged          "~OnWealthChanged" // int iPlr
#define PSF_OnActionChanged          "~OnActionChanged" // string oldaction
#define PSF_OnMaterialChanged        "~OnMaterialChanged" // int newmat, int oldmat

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
