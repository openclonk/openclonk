/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001  Sven Eberhardt
 * Copyright (c) 2002  Peter Wortmann
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

/* Some old constants and references */

#ifndef INC_C4Physics
#define INC_C4Physics

#include <Fixed.h>

const int StableRange=10;
const int AttachRange=5;
const int CornerRange=AttachRange+2;

#define GravAccel (::Landscape.Gravity)

extern const FIXED FloatAccel;
extern const FIXED HitSpeed1,HitSpeed2,HitSpeed3,HitSpeed4;
extern const FIXED WalkAccel,SwimAccel;
extern const FIXED FloatFriction;
extern const FIXED RotateAccel;

#endif
