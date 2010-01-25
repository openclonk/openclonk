/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001  Sven Eberhardt
 * Copyright (c) 2001, 2004, 2006  Peter Wortmann
 * Copyright (c) 2006  Günther Brammer
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

#ifndef INC_C4ValueList
#define INC_C4ValueList

#include "C4Value.h"

class C4ValueList
{
public:
	enum { MaxSize = 1000000 }; // ye shalt not create arrays larger than that!

	C4ValueList();
	C4ValueList(int32_t inSize);
	C4ValueList(const C4ValueList &ValueList2);
	~C4ValueList();

	C4ValueList &operator =(const C4ValueList& ValueList2);

protected:
	int32_t iSize;
	C4Value* pData;

public:
	int32_t GetSize() const { return iSize; }

	const C4Value &GetItem(int32_t iElem) const
	{
		if (-iSize <= iElem && iElem < 0)
			return pData[iSize + iElem];
		else if (0 <= iElem && iElem < iSize)
			return pData[iElem];
		else
			return C4VNull;
	}
	C4Value &GetItem(int32_t iElem);

	C4Value operator[](int32_t iElem) const { return GetItem(iElem); }
	C4Value &operator[](int32_t iElem)	 { return GetItem(iElem); }

	void Reset();
	void SetItem(int32_t iElemNr, C4Value iValue);
	void SetSize(int32_t inSize); // (enlarge only!)

	void DenumeratePointers();

	// comparison
	bool operator==(const C4ValueList& IntList2) const;

	// Compilation
	void CompileFunc(class StdCompiler *pComp);
};

// value list with reference count, used for arrays
class C4ValueArray : public C4ValueList
{
public:
	C4ValueArray();
	C4ValueArray(int32_t inSize);

	~C4ValueArray();

	// Add Reference, return self or new copy if necessary
	C4ValueArray * IncRef();
	C4ValueArray * IncElementRef();
	// Change length, return self or new copy if necessary
	C4ValueArray * SetLength(int32_t size);
	void DecRef();
	void DecElementRef();

	void Sort(class C4SortObject &rSort);
private:
	// Only for IncRef/AddElementRef
	C4ValueArray(const C4ValueArray &Array2);
	// Reference counter
	unsigned int iRefCnt;
	unsigned int iElementReferences;
};

#endif

