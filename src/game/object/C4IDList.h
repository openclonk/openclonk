/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001-2002, 2005, 2008  Sven Eberhardt
 * Copyright (c) 2005  Günther Brammer
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

/* A dynamic list of C4IDs */

#ifndef INC_C4IDList
#define INC_C4IDList

#include <C4Id.h>

class C4DefList;
class C4Facet;

// note that setting the chunk size for ID-Lists so low looks like an enormous waste
// at first glance - however, due there's an incredibly large number of small ID-Lists
// (99% of the lists have only one to three items!)
const size_t C4IDListChunkSize = 5; // size of id-chunks

class C4IDListChunk
{
public:
	C4ID id[C4IDListChunkSize];
	int32_t Count[C4IDListChunkSize];

	C4IDListChunk *pNext; // next chunk

public:
	C4IDListChunk();  // ctor
	~C4IDListChunk(); // dtor

public:
	void Clear();   // empty chunk and all behind
};

class C4IDList : protected C4IDListChunk
{
public:
	C4IDList();
	C4IDList(const C4IDList &rCopy);    // copy ctor
	C4IDList &operator = (const C4IDList &rCopy); // assignment
	~C4IDList();
	bool operator==(const C4IDList& rhs) const;
	// trick g++
	ALLOW_TEMP_TO_REF(C4IDList)
protected:
	size_t Count;                   // number of IDs in this list
public:
	// General
	void Default();
	void Clear();
	bool IsClear() const;
	// Access by direct index
	C4ID GetID(size_t index, int32_t *ipCount=NULL) const;
	int32_t  GetCount(size_t index) const;
	bool SetCount(size_t index, int32_t iCount);
	// Access by ID
	int32_t  GetIDCount(C4ID c_id, int32_t iZeroDefVal=0) const;
	bool SetIDCount(C4ID c_id, int32_t iCount, bool fAddNewID=false);
	bool IncreaseIDCount(C4ID c_id, bool fAddNewID=true, int32_t IncreaseBy=1, bool fRemoveEmpty=false);
	bool DecreaseIDCount(C4ID c_id, bool fRemoveEmptyID=true)
	{ return IncreaseIDCount(c_id, false, -1, fRemoveEmptyID); }
	int32_t  GetNumberOfIDs() const;
	int32_t GetIndex(C4ID c_id) const;
	// Access by category-sorted index
	C4ID GetID(C4DefList &rDefs, int32_t dwCategory, int32_t index, int32_t *ipCount=NULL) const;
	int32_t  GetCount(C4DefList &rDefs, int32_t dwCategory, int32_t index) const;
	bool SetCount(C4DefList &rDefs, int32_t dwCategory, int32_t index, int32_t iCount);
	int32_t  GetNumberOfIDs(C4DefList &rDefs, int32_t dwCategory) const;
	// IDList merge
	bool Add(C4IDList &rList);
	// Aux
	bool ConsolidateValids(C4DefList &rDefs, int32_t dwCategory = 0);
	// Item operation
	bool DeleteItem(size_t iIndex);
	// Graphics
	void Draw(C4Facet &cgo, int32_t iSelection,
	          C4DefList &rDefs, DWORD dwCategory,
	          bool fCounts=true, int32_t iAlign=0) const;
	// Compiling
	void CompileFunc(StdCompiler *pComp, bool fValues = true);
};

#endif
