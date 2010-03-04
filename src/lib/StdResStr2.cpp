/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2003, 2005-2006  Matthes Bender
 * Copyright (c) 2005-2007  Günther Brammer
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
#include <StdResStr2.h>
#include <StdBuf.h>

#include <stdio.h>

class ResTable {
public:
	ResTable(const char * table): Capacity(int(1.10 * SCharCount('\n', table))), Count(0), Entries(new Entry[Capacity]) {
		// reduce the capacity so that there is always an empty entry to mark the end of the search for an nonexistant key
		--Capacity;
		while (Count < Capacity) {
			// search '='
			const char * pos = table;
			const char * equalpos = 0;
			while (*pos && *pos != '\n' && *pos != '\r') {
				if (*pos == '=') equalpos = pos;
				++pos;
			}
			if (equalpos) {
				unsigned int h = Hash(table);
				// Get a pointer to the bucket
				Entry * e = &(Entries[h % Capacity]);
				// Search an empty spot
				int i = 0;
				while (*e) {
#ifdef _DEBUG
					if (e->Hash == h) printf("Hash Collision: %d (\"%.50s\")\nSTRINGTABLE WILL BREAK\n", h, table);
#endif
					e = &(Entries[(h + ++i) % Capacity]);
				}
				// Fill
				e->Hash = h;
				e->Data.CopyUntil(equalpos + 1, *pos);
				// Compile line feeds ("\n" -> 0D0A)
				for (i = 0; i < pos - equalpos; ++i)
				if (e->Data.getMData()[i] == '\\' && e->Data.getMData()[i + 1] == 'n')
					{ e->Data.getMData()[i] = 0x0D; e->Data.getMData()[i + 1] = 0x0A; }
				// Count!
				++Count;
			}
			while (*pos == '\n' || *pos == '\r') ++pos;
			table = pos;
			if (!*table) break;
		}
	}

	~ResTable() {
		delete[] Entries;
	}

	const char * GetEntry(const char * Key) {
		if (!Key) return NULL;
		unsigned int h = Hash(Key);
		Entry * e = &(Entries[h % Capacity]);
		int i = 0;
		while (e->Hash != h && *e) e = &(Entries[(h + ++i) % Capacity]);
		return e->Data.getData();
	}

private:
	struct Entry {
		StdCopyStrBuf Data;
		unsigned int Hash;
		Entry(): Data(), Hash(0) { }
		Entry(const StdStrBuf & Data, int32_t Hash): Data(Data), Hash(Hash) { }
		operator const void * () { return Data; }
	};
	int Capacity;
	int Count;
	Entry * Entries;
	static unsigned int Hash(const char * Key) {
		// Fowler/Noll/Vo hash
		unsigned int h = 2166136261u;
		while (*Key && *Key != '=')
			h = (h ^ *(Key++)) * 16777619;
		return h;
	}
};

static ResTable * Table = 0;

void SetResStrTable(char *pTable)
{
	// Clear any old table
	ClearResStrTable();
	// Create new Table
	Table = new ResTable(pTable);
	delete[] pTable;
}

void ClearResStrTable()
{
	delete Table;
	Table = 0;
}

bool IsResStrTableLoaded() { return Table != 0; }

const char *GetResStr(const char *id, ResTable * Table) {
	if (!Table) return "Language string table not loaded.";
	const char * r = Table->GetEntry(id);
	if (!r) {
		static char strResult[1024];
		// Default
		sprintf(strResult, "[Undefined:%s]", id);
		return strResult;
	}
	// Return string
	return r;
}

const char *LoadResStr(const char *id)
{
	return GetResStr(id, Table);
}

const int ResStrMaxLen = 4096;
static char strResult[ResStrMaxLen + 1];
char *LoadResStrNoAmp(const char *id)
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

char *GetResStr(const char *id, const char *strTable)
{
	const char *pos;
	// Default
	sprintf(strResult, "[Undefined:%s]", id);
	// Compose identifier with operator
	char idExt[256 + 1 + 1]; SCopy(id, idExt, 256); SAppendChar('=', idExt);
	// String table present and id not empty
	if (strTable && id && id[0])
		// Search for identifier with operator
		if ((pos = SSearch(strTable, idExt)))
			// Get string until end of line
			SCopyUntil(pos, strResult, "\r\n", ResStrMaxLen);
	// Return string
	return strResult;
}
