/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001, 2005  Sven Eberhardt
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

/* Lots of handler functions for object action */

#ifndef INC_C4ObjectCom
#define INC_C4ObjectCom

#include <C4Id.h>

void DrawCommandKey(C4Facet &cgo, int32_t iCom,
										BOOL fPressed = FALSE,
										const char *szText = NULL);

void DrawControlKey(C4Facet &cgo, int32_t iControl,
										BOOL fPressed = FALSE,
										const char *szText = NULL);

int32_t Control2Com(int32_t iControl, bool fUp);
int32_t Com2Control(int32_t iCom);
int32_t Coms2ComDir(int32_t iComs);
bool ComDirLike(int32_t iComDir, int32_t iSample);
const char *ComName(int32_t iCom);
int32_t ComOrder(int32_t iCom);
StdStrBuf PlrControlKeyName(int32_t iPlayer, int32_t iControl, bool fShort);

const int32_t ComOrderNum = 24;

BOOL PlayerObjectCommand(int32_t plr, int32_t cmdf, C4Object *pTarget=NULL, int32_t tx=0, int32_t ty=0);

BOOL ObjectActionWalk(C4Object *cObj);
BOOL ObjectActionStand(C4Object *cObj);
BOOL ObjectActionJump(C4Object *cObj, FIXED xdir, FIXED ydir, bool fByCom);
BOOL ObjectActionDive(C4Object *cObj, FIXED xdir, FIXED ydir);
BOOL ObjectActionTumble(C4Object *cObj, int32_t dir, FIXED xdir, FIXED ydir);
BOOL ObjectActionGetPunched(C4Object *cObj, FIXED xdir, FIXED ydir);
BOOL ObjectActionKneel(C4Object *cObj);
BOOL ObjectActionFlat(C4Object *cObj, int32_t dir);
BOOL ObjectActionScale(C4Object *cObj, int32_t dir);
BOOL ObjectActionHangle(C4Object *cObj, int32_t dir);
BOOL ObjectActionThrow(C4Object *cObj, C4Object *pThing=NULL);
BOOL ObjectActionDig(C4Object *cObj);
BOOL ObjectActionBuild(C4Object *cObj, C4Object *pTarget);
BOOL ObjectActionPush(C4Object *cObj, C4Object *pTarget);
BOOL ObjectActionChop(C4Object *cObj, C4Object *pTarget);
BOOL ObjectActionCornerScale(C4Object *cObj);
BOOL ObjectActionFight(C4Object *cObj, C4Object *pTarget);

BOOL ObjectComMovement(C4Object *cObj, int32_t iComDir);
BOOL ObjectComTurn(C4Object *cObj);
BOOL ObjectComStop(C4Object *cObj);
BOOL ObjectComGrab(C4Object *cObj, C4Object *pTarget);
BOOL ObjectComPut(C4Object *cObj, C4Object *pTarget, C4Object *pThing=NULL);
BOOL ObjectComThrow(C4Object *cObj, C4Object *pThing=NULL);
BOOL ObjectComDrop(C4Object *cObj, C4Object *pThing=NULL);
BOOL ObjectComUnGrab(C4Object *cObj);
BOOL ObjectComJump(C4Object *cObj);
BOOL ObjectComLetGo(C4Object *cObj, int32_t xdirf);
BOOL ObjectComUp(C4Object *cObj);
BOOL ObjectComDig(C4Object *cObj);
BOOL ObjectComChop(C4Object *cObj, C4Object *pTarget);
BOOL ObjectComBuild(C4Object *cObj, C4Object *pTarget);
BOOL ObjectComEnter(C4Object *cObj);
BOOL ObjectComDownDouble(C4Object *cObj);
BOOL ObjectComPutTake(C4Object *cObj, C4Object *pTarget, C4Object *pThing=NULL);
BOOL ObjectComTake(C4Object *cObj, C4ID id);
BOOL ObjectComTake(C4Object *cObj); // carlo
BOOL ObjectComTake2(C4Object *cObj); // carlo
BOOL ObjectComPunch(C4Object *cObj, C4Object *pTarget, int32_t iPunch=0);
BOOL ObjectComCancelAttach(C4Object *cObj);
void ObjectComDigDouble(C4Object *cObj);
void ObjectComStopDig(C4Object *cObj);

C4Object *Buy(int32_t iPlr, C4ID id, C4Object *pBase, BOOL fShowErrors=TRUE);
BOOL Sell(int32_t iPlr, C4Object *pObj, BOOL fShowErrors=TRUE);
BOOL Buy2Base(int32_t iPlr, C4Object *pBase, C4ID id, BOOL fShowErrors=TRUE);
BOOL SellFromBase(int32_t iPlr, C4Object *pBase, C4ID id, C4Object *pSellObj);


#endif
