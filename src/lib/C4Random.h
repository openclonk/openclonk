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

/* Network-safe random number generator */

#ifndef INC_C4Random
#define INC_C4Random

extern int RandomCount;

void FixedRandom(DWORD dwSeed);

int Random(int iRange);

inline unsigned int SeededRandom(unsigned int iSeed, unsigned int iRange)
{
	if (!iRange) return 0;
	iSeed = iSeed * 214013L + 2531011L;
	return (iSeed >> 16) % iRange;
}


inline int SafeRandom(int range)
{
	if (!range) return 0;
	return rand()%range;
}

#endif // INC_C4Random
