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

#include <Fixed.h>
#include <C4Id.h>

class C4Facet;
class C4Object;

void DrawCommandKey(C4Facet &cgo, int32_t iCom,
										bool fPressed = false,
										const char *szText = NULL);

void DrawControlKey(C4Facet &cgo, int32_t iControl,
										bool fPressed = false,
										const char *szText = NULL);

int32_t Control2Com(int32_t iControl, bool fUp);
int32_t Com2Control(int32_t iCom);
int32_t Coms2ComDir(int32_t iComs);
bool ComDirLike(int32_t iComDir, int32_t iSample);
const char *ComName(int32_t iCom);
int32_t ComOrder(int32_t iCom);
StdStrBuf PlrControlKeyName(int32_t iPlayer, int32_t iControl, bool fShort);

const int32_t ComOrderNum = 24;

bool PlayerObjectCommand(int32_t plr, int32_t cmdf, C4Object *pTarget=NULL, int32_t tx=0, int32_t ty=0);

bool ObjectActionWalk(C4Object *cObj);
bool ObjectActionStand(C4Object *cObj);
bool ObjectActionJump(C4Object *cObj, FIXED xdir, FIXED ydir, bool fByCom);
bool ObjectActionDive(C4Object *cObj, FIXED xdir, FIXED ydir);
bool ObjectActionTumble(C4Object *cObj, int32_t dir, FIXED xdir, FIXED ydir);
bool ObjectActionGetPunched(C4Object *cObj, FIXED xdir, FIXED ydir);
bool ObjectActionKneel(C4Object *cObj);
bool ObjectActionFlat(C4Object *cObj, int32_t dir);
bool ObjectActionScale(C4Object *cObj, int32_t dir);
bool ObjectActionHangle(C4Object *cObj, int32_t dir);
bool ObjectActionThrow(C4Object *cObj, C4Object *pThing=NULL);
bool ObjectActionDig(C4Object *cObj);
bool ObjectActionBuild(C4Object *cObj, C4Object *pTarget);
bool ObjectActionPush(C4Object *cObj, C4Object *pTarget);
bool ObjectActionChop(C4Object *cObj, C4Object *pTarget);
bool ObjectActionCornerScale(C4Object *cObj);
bool ObjectActionFight(C4Object *cObj, C4Object *pTarget);

bool ObjectComMovement(C4Object *cObj, int32_t iComDir);
bool ObjectComTurn(C4Object *cObj);
bool ObjectComStop(C4Object *cObj);
bool ObjectComGrab(C4Object *cObj, C4Object *pTarget);
bool ObjectComPut(C4Object *cObj, C4Object *pTarget, C4Object *pThing=NULL);
bool ObjectComThrow(C4Object *cObj, C4Object *pThing=NULL);
bool ObjectComDrop(C4Object *cObj, C4Object *pThing=NULL);
bool ObjectComUnGrab(C4Object *cObj);
bool ObjectComJump(C4Object *cObj);
bool ObjectComLetGo(C4Object *cObj, int32_t xdirf);
bool ObjectComUp(C4Object *cObj);
bool ObjectComDig(C4Object *cObj);
bool ObjectComChop(C4Object *cObj, C4Object *pTarget);
bool ObjectComBuild(C4Object *cObj, C4Object *pTarget);
bool ObjectComEnter(C4Object *cObj);
bool ObjectComDownDouble(C4Object *cObj);
bool ObjectComPutTake(C4Object *cObj, C4Object *pTarget, C4Object *pThing=NULL);
bool ObjectComTake(C4Object *cObj, C4ID id);
bool ObjectComTake(C4Object *cObj); // carlo
bool ObjectComTake2(C4Object *cObj); // carlo
bool ObjectComPunch(C4Object *cObj, C4Object *pTarget, int32_t iPunch=0);
bool ObjectComCancelAttach(C4Object *cObj);
void ObjectComDigDouble(C4Object *cObj);
void ObjectComStopDig(C4Object *cObj);

#endif
