/*
 * OpenClonk, http://www.openclonk.org
 *
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
/* string table: holds all strings used by script engine */

#include <C4Include.h>
#include <C4StringTable.h>

#ifndef BIG_C4INCLUDE
#include <C4Group.h>
#include <C4Components.h>
//#include <C4Aul.h>
#endif

// *** C4Set
template<> template<>
unsigned int C4Set<C4String *>::Hash<const char *>(const char * s)
	{
	// Fowler/Noll/Vo hash
	unsigned int h = 2166136261u;
	while (*s)
		h = (h ^ *(s++)) * 16777619;
	return h;
	}

template<> template<>
bool C4Set<C4String *>::Equals<const char *>(C4String * a, const char * b)
	{
	return a->GetData() == b;
	}

// *** C4String

C4String::C4String(StdStrBuf strString)
	: iRefCnt(0)
	{
	// take string
	Data.Take(strString);
	Hash = C4Set<C4String*>::Hash(Data.getData());
	// reg
	Strings.Set.Add(this);
	}

C4String::~C4String()
	{
	// unreg
	iRefCnt = 1;
	Strings.Set.Remove(this);
	}

void C4String::IncRef()
	{
	++iRefCnt;
	}

void C4String::DecRef()
	{
	--iRefCnt;
	if(iRefCnt <= 0)
		delete this;
	}

// *** C4StringTable

C4StringTable::C4StringTable()
	{
	P[P_PROTOTYPE] = RegString("Prototype");
	for (unsigned int i = 0; i < P_LAST; ++i) P[i]->IncRef();
	}

C4StringTable::~C4StringTable()
	{
	Clear();
	for (unsigned int i = 0; i < P_LAST; ++i) P[i]->DecRef();
	assert(!Set.GetSize());
	}

void C4StringTable::Clear()
	{
	for (unsigned int i = 0; i < Stringstxt.size(); ++i)
		Stringstxt[i]->DecRef();
	Stringstxt.clear();
	}

C4String *C4StringTable::RegString(StdStrBuf String)
	{
	C4String * s = FindString(String.getData());
	if (s)
		return s;
	else
		return new C4String(String);
	}

C4String *C4StringTable::FindString(const char *strString)
	{
	return Set.Get(strString);
	}

C4String *C4StringTable::FindString(C4String *pString)
	{
	for (C4String * const * i = Set.First(); i; i = Set.Next(i))
		if (*i == pString)
			return pString;
	}

C4String *C4StringTable::FindString(int iEnumID)
	{
	if (iEnumID >= 0 && iEnumID < int(Stringstxt.size()))
		return Stringstxt[iEnumID];
	return NULL;
	}

bool C4StringTable::Load(C4Group& ParentGroup)
	{
	Clear();
	// read data
	char *pData;
	if(!ParentGroup.LoadEntry(C4CFN_Strings, &pData, NULL, 1))
		return false;
	// read all strings
	char strBuf[1024 + 1]; // 1024 was the last used value to write the Strings.txt
	for(int i = 0; SCopySegment(pData, i, strBuf, 0x0A, 1024); i++)
		{
		SReplaceChar(strBuf, 0x0D, 0x00);
		// add string to list
		C4String *pnString;
		pnString = RegString(StdStrBuf(strBuf));
		pnString->IncRef();
		Stringstxt.push_back(pnString);
		}
	// delete data
	delete[] pData;
	return true;
	}

C4StringTable Strings;
