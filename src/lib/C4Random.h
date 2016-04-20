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

#include <pcg/pcg_random.hpp>

extern int RandomCount;
extern pcg32 SafeRandom;

void FixedRandom(uint64_t dwSeed);

uint32_t Random(uint32_t iRange);

inline uint32_t SeededRandom(uint64_t iSeed, uint32_t iRange)
{
	if (!iRange) return 0;
	pcg32 rng(iSeed);
	return rng(iRange);
}

#endif // INC_C4Random
