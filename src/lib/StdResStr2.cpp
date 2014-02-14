/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2010-2013, The OpenClonk Team and contributors
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

/* Load strings from a primitive memory string table */

#include "C4Include.h"
#include "C4Language.h"

const int ResStrMaxLen = 4096;
static char strResult[ResStrMaxLen + 1];
const char *LoadResStrNoAmp(const char *id)
{
	const char * str = LoadResStr(id);
	char * cpd = strResult;
	for (const char * cps = str; *cps; ++cps, ++cpd)
	{
		if (*cps == '&')
			--cpd;
		else
			*cpd = *cps;
	}
	*cpd = 0;
	return strResult;
}