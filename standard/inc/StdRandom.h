/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2002, 2004  Sven Eberhardt
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

/* Some wrappers to runtime-library random */

#ifndef INC_STDRANDOM
#define INC_STDRANDOM

#include <time.h>

extern int RandomCount;
extern unsigned int RandomHold;

inline void FixedRandom(DWORD dwSeed)
	{
	RandomHold=dwSeed; // srand
	RandomCount=0;
	}

inline void Randomize()
	{
	FixedRandom((unsigned)time(NULL));
	}

inline int Random(int iRange)
	{
	RandomCount++;
	if (iRange==0) return 0;
	RandomHold = RandomHold * 214013L + 2531011L;
	return (RandomHold >> 16) % iRange;
	}

#endif // INC_STDRANDOM
