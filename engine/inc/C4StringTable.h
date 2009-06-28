/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2002  Peter Wortmann
 * Copyright (c) 2009  Günther Brammer
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

#ifndef C4STRINGTABLE_H

#define C4STRINGTABLE_H

class C4StringTable;

class C4String
	{
	C4String(C4StringTable *pTable);
	C4String(StdStrBuf strString, C4StringTable *pTable);
	friend class C4StringTable;
public:
	virtual ~C4String();

	// increment/decrement reference count on this string
	void IncRef();
	void DecRef();

	const char * GetCStr() const { return Data.getData(); }
	StdStrBuf GetData() const { return Data.getRef(); }
	StdCopyStrBuf Data; // string data
	int iRefCnt; // reference count on string (by C4Value)
	bool Hold;  // string stays hold when RefCnt reaches 0 (for in-script strings)

	int iEnumID;

	C4String *Next, *Prev; // double-linked list

	C4StringTable *pTable; // owning table

	void Reg(C4StringTable *pTable);
	void UnReg();
	};


class C4StringTable
	{
public:
	C4StringTable();
	virtual ~C4StringTable();

	void Clear();

	C4String *RegString(StdStrBuf String);
	C4String *RegString(const char * s) { return RegString(StdStrBuf(s)); }
	C4String *FindString(const char *strString);
	C4String *FindString(C4String *pString);
	C4String *FindString(int iEnumID);

	bool Load(C4Group& ParentGroup);

	C4String *First, *Last; // string list
	};

#endif
