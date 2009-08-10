/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001  Sven Eberhardt
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
// an associated list
// used by C4AulScriptEngine, as virtual function table and include/dependency list

#ifndef INC_C4AList
#define INC_C4AList

#define C4AListChunkSize 128 // number of table entries for each chunk

// table entry
struct C4AListEntry
	{
	C4ID Var; void *Val;
	C4AListEntry *next(); // get entry after the given one
	};

// bunch of table entries
struct C4AListChunk
	{
	C4AListEntry Entries[C4AListChunkSize]; // table entries
	void *Stop; // stop entry; should always be NULL
	C4AListChunk *Next; // next chunk
	};

// associative list
class C4AList
	{
	protected:
		C4AListChunk *Table, *CurrC; // first/last table chunk
		int CCount; // number of entries in current chunk
		C4AListEntry *Curr; // next available entry to be used
		void Grow(); // append chunk

	public:
		C4AList(); // constructor
		~C4AList(); // destructor
		void Clear(); // clear the list

		C4AListEntry *push(C4ID Var = 0, void *pVal = NULL); // push var/value pair to end of list
	};

#endif
