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

/* Lots of handler functions for object action */

#ifndef INC_C4ObjectCom
#define INC_C4ObjectCom

#include "lib/C4Real.h"

bool ComDirLike(int32_t iComDir, int32_t iSample);

bool ObjectActionWalk(C4Object *cObj);
bool ObjectActionStand(C4Object *cObj);
bool ObjectActionJump(C4Object *cObj, C4Real xdir, C4Real ydir, bool fByCom);
bool ObjectActionDive(C4Object *cObj, C4Real xdir, C4Real ydir);
bool ObjectActionTumble(C4Object *cObj, int32_t dir, C4Real xdir, C4Real ydir);
bool ObjectActionGetPunched(C4Object *cObj, C4Real xdir, C4Real ydir);
bool ObjectActionKneel(C4Object *cObj);
bool ObjectActionFlat(C4Object *cObj, int32_t dir);
bool ObjectActionScale(C4Object *cObj, int32_t dir);
bool ObjectActionHangle(C4Object *cObj);
bool ObjectActionThrow(C4Object *cObj, C4Object *pThing=nullptr);
bool ObjectActionDig(C4Object *cObj);
bool ObjectActionPush(C4Object *cObj, C4Object *pTarget);
bool ObjectActionCornerScale(C4Object *cObj);

bool ObjectComStop(C4Object *cObj);
bool ObjectComGrab(C4Object *cObj, C4Object *pTarget);
bool ObjectComPut(C4Object *cObj, C4Object *pTarget, C4Object *pThing=nullptr);
bool ObjectComThrow(C4Object *cObj, C4Object *pThing=nullptr);
bool ObjectComDrop(C4Object *cObj, C4Object *pThing=nullptr);
bool ObjectComUnGrab(C4Object *cObj);
bool ObjectComJump(C4Object *cObj);
bool ObjectComLetGo(C4Object *cObj, int32_t xdirf);
bool ObjectComDig(C4Object *cObj);
bool ObjectComPutTake(C4Object *cObj, C4Object *pTarget, C4Object *pThing=nullptr);
bool ObjectComTake(C4Object *cObj); // carlo
bool ObjectComTake2(C4Object *cObj); // carlo
bool ObjectComPunch(C4Object *cObj, C4Object *pTarget, int32_t iPunch=0);
bool ObjectComCancelAttach(C4Object *cObj);
void ObjectComStopDig(C4Object *cObj);

#endif
