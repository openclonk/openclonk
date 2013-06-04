/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2005, 2009  Günther Brammer
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

/* Network-safe random number generator */

#include "C4Include.h"
#include <C4Random.h>
#include <C4Record.h>

int RandomCount = 0;
unsigned int RandomHold = 0;

int Random(int iRange)
{
	if (Config.General.DebugRec)
	{
		// next pseudorandom value
		RandomCount++;
		C4RCRandom rc;
		rc.Cnt=RandomCount;
		rc.Range=iRange;
		if (iRange==0)
			rc.Val=0;
		else
		{
			RandomHold = ((uint64_t)RandomHold * 16807) % 2147483647;
			rc.Val = RandomHold % iRange;
		}
		AddDbgRec(RCT_Random, &rc, sizeof(rc));
		return rc.Val;
	}
	else
	{
		RandomCount++;
		if (iRange==0) return 0;
		RandomHold = ((uint64_t)RandomHold * 16807) % 2147483647;
		return RandomHold % iRange;
	}
}
