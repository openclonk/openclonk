/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2013, The OpenClonk Team and contributors
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
		if (iRange<=0)
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
		if (iRange<=0) return 0;
		RandomHold = ((uint64_t)RandomHold * 16807) % 2147483647;
		return RandomHold % iRange;
	}
}
