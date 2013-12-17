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

/* Some old constants and references */

#ifndef INC_C4Physics
#define INC_C4Physics

#include "C4Real.h"

const int StableRange=10;
const int AttachRange=5;
const int CornerRange=AttachRange+2;

#define GravAccel (::Landscape.Gravity)

extern const C4Real HitSpeed1,HitSpeed2,HitSpeed3,HitSpeed4;
extern const C4Real FloatFriction;
extern const C4Real RotateAccel;
extern const C4Real DefaultGravAccel;

#endif
