/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2013-2016, The OpenClonk Team and contributors
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

#include "C4Include.h"
#include "lib/C4NameList.h"

void C4NameList::Clear()
{
	InplaceReconstruct(this);
}

bool C4NameList::Set(const char *szName, int32_t iCount)
{
	int32_t cnt;
	// Find existing name, set count
	for (cnt=0; cnt<C4MaxNameList; cnt++)
		if (SEqual(Name[cnt],szName))
		{
			Count[cnt]=iCount;
			return true;
		}
	// Find empty spot, set name and count
	for (cnt=0; cnt<C4MaxNameList; cnt++)
		if (Name[cnt][0]==0)
		{
			SCopy(szName,Name[cnt],C4MaxName);
			Count[cnt]=iCount;
			return true;
		}
	// No empty spots
	return false;
}

bool C4NameList::Add(const char *szName, int32_t iCount)
{
	// Find empty spot, set name and count
	for (int32_t cnt=0; cnt<C4MaxNameList; cnt++)
		if (!Name[cnt][0])
		{
			SCopy(szName,Name[cnt],C4MaxName);
			Count[cnt]=iCount;
			return true;
		}
	// No empty spots
	return false;
}

bool C4NameList::IsEmpty()
{
	for (int32_t cnt=0; cnt<C4MaxNameList; cnt++)
		if (Name[cnt][0])
			return false;
	return true;
}

void C4NameList::CompileFunc(StdCompiler *pComp, bool fValues)
{
	bool deserializing = pComp->isDeserializer();
	for (int32_t cnt=0; cnt<C4MaxNameList; cnt++)
		if (deserializing || Name[cnt][0])
		{
			if (cnt) pComp->Separator(StdCompiler::SEP_SEP2);
			// Name
			pComp->Value(mkDefaultAdapt(mkStringAdapt(Name[cnt], C4MaxName, StdCompiler::RCT_Idtf), ""));
			// Value
			if (fValues)
			{
				pComp->Separator(StdCompiler::SEP_SET);
				pComp->Value(mkDefaultAdapt(Count[cnt], 0));
			}
		}
}
