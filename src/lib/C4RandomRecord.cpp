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

#include "C4Include.h"
#include "lib/C4Random.h"
#include "control/C4Record.h"

void RecordRandom(uint32_t range, uint32_t val)
{
	RandomCount++;
	if (Config.General.DebugRec)
	{
		// next pseudorandom value
		C4RCRandom rc;
		rc.Cnt=RandomCount;
		rc.Range=range;
		rc.Val=val;
		AddDbgRec(RCT_Random, &rc, sizeof(rc));
	}
}
