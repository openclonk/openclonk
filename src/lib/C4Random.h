/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2013-2016, The OpenClonk Team and contributors
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

/* Network-safe random number generator */

#ifndef INC_C4Random
#define INC_C4Random

#include <inttypes.h>

extern int RandomCount;

// Seeds both the synchronized and the unsynchronized RNGs.
void FixedRandom(uint64_t dwSeed);
// Synchronized RNG.
uint32_t Random(uint32_t iRange);
// Unsynchronized RNG.
uint32_t UnsyncedRandom();
uint32_t UnsyncedRandom(uint32_t range);
// Generates a single random value from a seed.
uint32_t SeededRandom(uint64_t iSeed, uint32_t iRange);

// Internal
void RecordRandom(uint32_t range, uint32_t val);

#endif // INC_C4Random
