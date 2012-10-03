/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2012  Julius Michaelis
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

#include "C4Include.h"

#include "C4Numeric.h"

C4Numeric Sqrt(const C4Numeric & nValue)
{
	if (nValue<0) return 0;
	if (nValue.GetType() == C4V_Float)
		return Sqrt(nValue._getFloat());
	else
	{
		long iSqrt = long(sqrt(double(nValue._getInt())));
		if (iSqrt * iSqrt < nValue) iSqrt++;
		if (iSqrt * iSqrt > nValue) iSqrt--;
		return iSqrt;
	}	//return fixtoi(Sqrt(nValue.getFloat()));
}
