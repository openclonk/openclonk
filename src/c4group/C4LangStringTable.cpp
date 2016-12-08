/*
 * OpenClonk, http://www.openclonk.org
 *
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
// Loads StringTbl* and replaces $..$-strings by localized versions

#include "C4Include.h"

#include <utility>
#include <vector>

#include "c4group/C4LangStringTable.h"
#include "lib/C4InputValidation.h"

C4LangStringTable::C4LangStringTable() : ref_count(1) {}

bool C4LangStringTable::HasTranslation(const std::string &text) const
{
	if (strings.empty())
		PopulateStringTable();
	return strings.find(text) != strings.end();
}

const std::string &C4LangStringTable::Translate(const std::string &text) const
{
	if (strings.empty())
		PopulateStringTable();
	Table::const_iterator it = strings.find(text);
	if (it == strings.end())
	{
		throw NoSuchTranslation(text);
	}
	return it->second;
}

void C4LangStringTable::PopulateStringTable() const
{
	assert(strings.empty());

	strings.clear();
	std::string key, value;

	// read table
	const char *data = GetData();
	if (!data || !*data)
		return;

	enum { PSTS_Key, PSTS_Val } state = PSTS_Key;
	do
	{
		if (state == PSTS_Key)
		{
			if (*data == '=')
			{
				state = PSTS_Val;
			}
			else if (*data == '\0' || *data == '\n' || *data == '\r')
			{
				if (!key.empty() && key[0]!='#')
					LogF("%s: string table entry without a value: \"%s\"", GetFilePath() ? GetFilePath() : "<unknown>", key.c_str());
				key.clear();
			}
			else
			{
				key.push_back(*data);
			}
		}
		else
		{
			if (*data == '\0' || *data == '\n' || *data == '\r')
			{
				strings.insert(std::make_pair(key, value));
				key.clear(); value.clear();
				state = PSTS_Key;
			}
			else
			{
				value.push_back(*data);
			}
		}
	}
	while (*data++);
}

void C4LangStringTable::ReplaceStrings(const StdStrBuf &rBuf, StdStrBuf &rTarget)
{
	if (!rBuf.getLength())
	{
		return;
	}
	// grab char ptr from buf
	const char *Data = rBuf.getData();

	// Find Replace Positions
	int iScriptLen = SLen(Data);
	struct RP { const char *Pos; std::string String; unsigned int Len; RP *Next; } *pRPList = nullptr, *pRPListEnd = nullptr;
	for (const char *pPos = SSearch(Data, "$"); pPos; pPos = SSearch(pPos, "$"))
	{
		// Get name
		char szStringName[C4MaxName + 1];
		SCopyUntil(pPos, szStringName, '$', C4MaxName); pPos += SLen(szStringName) + 1;
		if (*(pPos-1) != '$') continue;
		// valid?
		const char *pPos2 = szStringName;
		while (*pPos2)
			if (!IsIdentifier(*(pPos2++)))
				break;
		if (*pPos2) continue;
		// check termination
		try
		{
			// search in string table
			std::string pStrTblEntry = Translate(szStringName);
			// add new replace-position entry
			RP *pnRP = new RP;
			pnRP->Pos = pPos - SLen(szStringName) - 2;
			pnRP->String = pStrTblEntry;
			pnRP->Len = SLen(szStringName) + 2;
			pnRP->Next = nullptr;
			pRPListEnd = (pRPListEnd ? pRPListEnd->Next : pRPList) = pnRP;
			// calculate new script length
			iScriptLen += pStrTblEntry.size() - pnRP->Len;
		}
		catch (NoSuchTranslation &)
		{
			LogF("%s: string table entry not found: \"%s\"", GetFilePath() ? GetFilePath() : "<unknown>", szStringName);
		}
	}
	// Alloc new Buffer
	char *pNewBuf;
	StdStrBuf sNewBuf;
	sNewBuf.SetLength(iScriptLen);
	pNewBuf = sNewBuf.getMData();
	// Copy data
	const char *pRPos = Data; char *pWPos = pNewBuf;
	for (RP *pRPPos = pRPList; pRPPos; pRPPos = pRPPos->Next)
	{
		// copy preceding string data
		SCopy(pRPos, pWPos, pRPPos->Pos - pRPos);
		pWPos += pRPPos->Pos - pRPos;
		// copy string
		SCopyUntil(pRPPos->String.c_str(), pWPos, '\n');
		SReplaceChar(pWPos, '\r', '\0');
		// advance
		pRPos = pRPPos->Pos + pRPPos->Len;
		pWPos += SLen(pWPos);
	}
	SCopy(pRPos, pWPos);

	while (pRPList)
	{
		RP *pRP = pRPList;
		pRPList = pRP->Next;
		delete pRP;
	}

	// assign this buf
	rTarget.Clear();
	rTarget.Take(std::move(sNewBuf));
}

void C4LangStringTable::ReplaceStrings(StdStrBuf &rBuf)
{
	ReplaceStrings(rBuf, rBuf);
}
