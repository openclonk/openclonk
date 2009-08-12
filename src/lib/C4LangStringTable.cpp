/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005, 2007  Sven Eberhardt
 * Copyright (c) 2005, 2007  Günther Brammer
 * Copyright (c) 2009  Armin Burgmeier
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

#include "C4Include.h"
#include "C4LangStringTable.h"

struct C4StringTableEntry
	{
	const char *pName, *pEntry;

	C4StringTableEntry(const char *pName, const char *pEntry)
		: pName(pName), pEntry(pEntry) {}
	};


void C4LangStringTable::ReplaceStrings(const StdStrBuf &rBuf, StdStrBuf &rTarget, const char *szParentFilePath)
	{
	if (!rBuf.getLength())
		{
		return;
		}
	// grab char ptr from buf
	const char *Data = rBuf.getData();

	// string table
	std::vector<C4StringTableEntry> Entries;

	// read string table
	char *pStrTblBuf = NULL;
	if(GetData())
	{
		// copy data
		pStrTblBuf = new char [GetDataSize() + 1];
		SCopy(GetData(), pStrTblBuf, GetDataSize());
		// find entries
		const char *pLine = pStrTblBuf;
		bool found_eq = false;
		for(char *pPos = pStrTblBuf; *pPos; pPos++)
			if(*pPos == '\n' || *pPos == '\r')
			{
				found_eq = false;
				*pPos = '\0'; pLine = pPos + 1;
			}
			else if(*pPos == '=' && !found_eq)
			{
				*pPos = '\0';
				// We found an '=' sign, so parse everything to end of line from now on, ignoring more '=' signs. Bug #2327.
				found_eq = true;
				// add entry
				Entries.push_back(C4StringTableEntry(pLine, pPos + 1));
			}
	}

	// Find Replace Positions
	int iScriptLen = SLen(Data);
	struct RP { const char *Pos, *String; unsigned int Len; RP *Next; } *pRPList = NULL, *pRPListEnd = NULL;
	for(const char *pPos = SSearch(Data, "$"); pPos; pPos = SSearch(pPos, "$"))
	{
		// Get name
		char szStringName[C4MaxName + 1];
		SCopyUntil(pPos, szStringName, '$', C4MaxName); pPos += SLen(szStringName) + 1;
		if(*(pPos-1) != '$') continue;
		// valid?
		//for(const char *pPos2 = szStringName; *pPos2; pPos2++)
		//	if(!IsIdentifier(*pPos2))
		//		break;
		const char *pPos2 = szStringName;
		while (*pPos2)
			if(!IsIdentifier(*(pPos2++)))
				break;
		if(*pPos2) continue;
		// check termination
		// search in string table
		const char *pStrTblEntry = NULL;
		for(unsigned int i = 0; i < Entries.size(); i++)
			if(SEqual(szStringName, Entries[i].pName))
			{
				pStrTblEntry = Entries[i].pEntry; break;
			}
		// found?
		if(!pStrTblEntry)
			{ LogF("%s: string table entry not found: \"%s\"", FilePath[0] ? FilePath : (szParentFilePath ? szParentFilePath : "Unknown"), szStringName); continue; }
		// add new replace-position entry
		RP *pnRP = new RP;
		pnRP->Pos = pPos - SLen(szStringName) - 2;
		pnRP->String = pStrTblEntry;
		pnRP->Len = SLen(szStringName) + 2;
		pnRP->Next = NULL;
		pRPListEnd = (pRPListEnd ? pRPListEnd->Next : pRPList) = pnRP;
		// calculate new script length
		iScriptLen += SLen(pStrTblEntry) - pnRP->Len;
	}
	// Alloc new Buffer
	char *pNewBuf;
	StdStrBuf sNewBuf;
	sNewBuf.SetLength(iScriptLen);
	pNewBuf = sNewBuf.getMData();
	// Copy data
	const char *pRPos = Data; char *pWPos = pNewBuf;
	for(RP *pRPPos = pRPList; pRPPos; pRPPos = pRPPos->Next)
	{
		// copy preceding string data
		SCopy(pRPos, pWPos, pRPPos->Pos - pRPos);
		pWPos += pRPPos->Pos - pRPos;
		// copy string
		SCopyUntil(pRPPos->String, pWPos, '\n');
		SReplaceChar(pWPos, '\r', '\0');
		// advance
		pRPos = pRPPos->Pos + pRPPos->Len;
		pWPos += SLen(pWPos);
	}
	SCopy(pRPos, pWPos);

	while(pRPList)
	{
		RP *pRP = pRPList;
		pRPList = pRP->Next;
		delete pRP;
	}

	// free buffer
	if(pStrTblBuf) delete [] pStrTblBuf;

	// assign this buf
	rTarget.Clear();
	rTarget.Take(sNewBuf);
	}

void C4LangStringTable::ReplaceStrings(StdStrBuf &rBuf)
	{
	ReplaceStrings(rBuf, rBuf, 0);
	}
