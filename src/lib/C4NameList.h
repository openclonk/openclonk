/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001, 2005  Sven Eberhardt
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

/* A static list of strings and integer values, i.e. for material amounts */

#ifndef INC_C4NameList
#define INC_C4NameList

const int C4MaxNameList = 10;

#include "C4Constants.h"
#include "C4InputValidation.h"

#include <cstring>
class C4NameList
	{
	public:
		C4NameList();
	public:
		char Name[C4MaxNameList][C4MaxName+1];
		int32_t Count[C4MaxNameList];
	public:
		void Clear();
		bool Add(const char *szName, int32_t iCount=0);
		bool Set(const char *szName, int32_t iCount);
		bool Read(const char *szSource, int32_t iDefValue=0);
		bool Write(char *szTarget, bool fValues=true);
	public:
		bool IsEmpty();
		bool operator==(const C4NameList& rhs)
			{	return !std::memcmp((const uint8_t*)this,(const uint8_t*)&rhs,sizeof(C4NameList)); }
		void CompileFunc(StdCompiler *pComp, bool fValues = true);
	};

#endif
