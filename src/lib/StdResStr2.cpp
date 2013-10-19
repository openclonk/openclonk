/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2003, 2005-2006  Matthes Bender
 * Copyright (c) 2005-2007  Günther Brammer
 * Copyright (c) 2010  Benjamin Herr
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