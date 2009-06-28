/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005  Sven Eberhardt
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
// Loads StringTbl* and replaces $..$-strings by localized versions

#ifndef INC_C4LangStringTable
#define INC_C4LangStringTable

#include "C4ComponentHost.h"

class C4LangStringTable : public C4ComponentHost
	{
	public:
		// do replacement in buffer
		// if any replacement is done, the buffer will be realloced
		void ReplaceStrings(StdStrBuf &rBuf);
		void ReplaceStrings(const StdStrBuf &rBuf, StdStrBuf &rTarget, const char *szParentFilePath = NULL);
	};

#endif // INC_C4LangStringTable
