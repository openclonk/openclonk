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
#include <C4Aul.h>
#endif

// *** C4String

C4String::C4String(C4StringTable *pnTable)
	: Data(NULL), iRefCnt(0), Hold(false), iEnumID(-1), pTable(NULL)
	{
	// reg
	Reg(pnTable);
	}

C4String::C4String(StdStrBuf strString, C4StringTable *pnTable)
	: iRefCnt(0), Hold(false), iEnumID(-1), pTable(NULL)
	{
	// take string
	Data.Take(strString);
	// reg
	Reg(pnTable);
	}

C4String::~C4String()
	{
	// unreg
	iRefCnt = 1;
	if(pTable) UnReg();
	}

void C4String::IncRef()
	{
	++iRefCnt;
	}

void C4String::DecRef()
	{
	--iRefCnt;

	// delete if ref cnt is 0 and the Hold-Flag isn't set
	if(iRefCnt <= 0 && !Hold)
		delete this;
	}

void C4String::Reg(C4StringTable *pnTable)
	{
	if(pTable) UnReg();

	// add string to tail of table
	Prev = pnTable->Last;
	Next = NULL;

	if(Prev)
		Prev->Next = this;
	else
		pnTable->First = this;
	pnTable->Last = this;

	pTable = pnTable;
	}

void C4String::UnReg()
	{
	if(!pTable) return;

	if(Next)
		Next->Prev = Prev;
	else
		pTable->Last = Prev;
	if(Prev)
		Prev->Next = Next;
	else
		pTable->First = Next;

	pTable = NULL;

	// delete hold flag if table is lost and check for delete
	Hold = false;
	if(iRefCnt <= 0)
		delete this;
	}

// *** C4StringTable

C4StringTable::C4StringTable()
	: First(NULL), Last(NULL)
	{
	}

C4StringTable::~C4StringTable()
	{
	// unreg all remaining strings
	// (hold strings will delete themselves)
	while(First) First->UnReg();
	}

void C4StringTable::Clear()
	{
	bool bContinue;
	do
		{
		bContinue = false;
		// find string to delete / unreg
		for(C4String *pAct = First; pAct; pAct = pAct->Next)
			if(pAct->Hold)
				{
				pAct->UnReg();
				bContinue = true;
				break;
				}
		}
	while(bContinue);
	}

C4String *C4StringTable::RegString(StdStrBuf String)
	{
	C4String * s = FindString(String.getData());
	if (s)
		return s;
	else
		return new C4String(String, this);
	}

C4String *C4StringTable::FindString(const char *strString)
	{
	for(C4String *pAct = First; pAct; pAct = pAct->Next)
		if(SEqual(pAct->Data.getData(), strString))
			return pAct;
	return NULL;
	}

C4String *C4StringTable::FindString(C4String *pString)
	{
	for(C4String *pAct = First; pAct; pAct = pAct->Next)
		if(pAct == pString)
			return pAct;
	return NULL;
	}

C4String *C4StringTable::FindString(int iEnumID)
	{
	for(C4String *pAct = First; pAct; pAct = pAct->Next)
		if(pAct->iEnumID == iEnumID)
			return pAct;
	return NULL;
	}

bool C4StringTable::Load(C4Group& ParentGroup)
	{
	// read data
	char *pData;
	if(!ParentGroup.LoadEntry(C4CFN_Strings, &pData, NULL, 1))
		return false;
	// read all strings
	char strBuf[C4AUL_MAX_String + 1];
	for(int i = 0; SCopySegment(pData, i, strBuf, 0x0A, C4AUL_MAX_String); i++)
		{
		SReplaceChar(strBuf, 0x0D, 0x00);
		// add string to list
		C4String *pnString;
		if(!(pnString = FindString(strBuf)))
			pnString = RegString(StdStrBuf(strBuf));
		pnString->iEnumID = i;
		}
	// delete data
	delete[] pData;
	return true;
	}
