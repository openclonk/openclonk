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

/* A static list of strings and integer values, i.e. for material amounts */

#ifndef INC_C4NameList
#define INC_C4NameList

const int C4MaxNameList = 10;

#include "config/C4Constants.h"
#include "lib/C4InputValidation.h"

class C4NameList
{
public:
	char Name[C4MaxNameList][C4MaxName + 1] = { { 0 } };
	int32_t Count[C4MaxNameList] = { 0 };
public:
	void Clear();
	bool Add(const char *szName, int32_t iCount=0);
	bool Set(const char *szName, int32_t iCount);
public:
	bool IsEmpty();
	bool operator==(const C4NameList& rhs)
	{ return !std::memcmp((const uint8_t*)this,(const uint8_t*)&rhs,sizeof(C4NameList)); }
	void CompileFunc(StdCompiler *pComp, bool fValues = true);
};

#endif
