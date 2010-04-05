/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2007  Peter Wortmann
 * Copyright (c) 2005-2007  Sven Eberhardt
 * Copyright (c) 2006-2007  Günther Brammer
 * Copyright (c) 2008  Armin Burgmeier
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
#include <C4Include.h>
#include <C4ValueList.h>
#include <algorithm>

#include <C4Aul.h>
#include <C4FindObject.h>

C4ValueList::C4ValueList()
		: iSize(0), pData(NULL)
{
}

C4ValueList::C4ValueList(int32_t inSize)
		: iSize(0), pData(NULL)
{
	SetSize(inSize);
}

C4ValueList::C4ValueList(const C4ValueList &ValueList2)
		: iSize(0), pData(NULL)
{
	SetSize(ValueList2.GetSize());
	for (int32_t i = 0; i < iSize; i++)
		pData[i].Set(ValueList2.GetItem(i));
}

C4ValueList::~C4ValueList()
{
	delete[] pData; pData = NULL;
	iSize = 0;
}

C4ValueList &C4ValueList::operator =(const C4ValueList& ValueList2)
{
	this->SetSize(ValueList2.GetSize());
	for (int32_t i = 0; i < iSize; i++)
		pData[i].Set(ValueList2.GetItem(i));
	return *this;
}

class C4SortObjectSTL
{
private:
	C4SortObject &rSorter;

public:
	C4SortObjectSTL(C4SortObject &rSorter) : rSorter(rSorter) {}
	bool operator ()(const C4Value &v1, const C4Value &v2) { return rSorter.Compare(v1._getObj(), v2._getObj()) > 0; }
};

class C4SortObjectSTLCache
{
private:
	C4SortObject &rSorter;
	C4Value *pVals;

public:
	C4SortObjectSTLCache(C4SortObject &rSorter, C4Value *pVals) : rSorter(rSorter), pVals(pVals) {}
	bool operator ()(int32_t n1, int32_t n2) { return rSorter.CompareCache(n1, n2, pVals[n1]._getObj(), pVals[n2]._getObj()) > 0; }
};

void C4ValueArray::Sort(class C4SortObject &rSort)
{
	if (rSort.PrepareCache(this))
	{
		// Initialize position array
		intptr_t i, *pPos = new intptr_t[iSize];
		for (i = 0; i < iSize; i++) pPos[i] = i;
		// Sort
		std::stable_sort(pPos, pPos+iSize, C4SortObjectSTLCache(rSort, pData));
		// Save actual object pointers in array (hacky).
		for (i = 0; i < iSize; i++)
			pPos[i] = reinterpret_cast<intptr_t>(pData[pPos[i]]._getObj());
		// Set the values
		for (i = 0; i < iSize; i++)
			pData[i].SetObject(reinterpret_cast<C4Object *>(pPos[i]));
		delete [] pPos;
	}
	else
		// Be sure to use stable sort, as otherweise the algorithm isn't garantueed
		// to produce identical results on all platforms!
		std::stable_sort(pData, pData+iSize, C4SortObjectSTL(rSort));
}

C4Value &C4ValueList::GetItem(int32_t iElem)
{
	if (iElem < -iSize)
		throw new C4AulExecError(NULL,"invalid subscript");
	else if (iElem < 0)
		iElem = iSize + iElem;
	else if (iElem >= iSize && iElem < MaxSize) this->SetSize(iElem + 1);
	// out-of-memory? This might not be catched, but it's better than a segfault
	if (iElem >= iSize)
		throw new C4AulExecError(NULL,"out of memory");
	// return
	return pData[iElem];
}

void C4ValueList::SetItem(int32_t iElemNr, C4Value iValue)
{
	// enlarge
	if (iElemNr < 0) iElemNr = 0;
	if (iElemNr >= iSize && iElemNr < MaxSize) this->SetSize(iElemNr + 1);
	// out-of-memory? This might not be catched, but it's better than a segfault
	if (iElemNr >= iSize)
		throw new C4AulExecError(NULL,"out of memory");
	// set
	pData[iElemNr]=iValue;
}

void C4ValueList::SetSize(int32_t inSize)
{
	// array made smaller? Well, just ignore the additional allocated mem then
	if (inSize<=iSize)
	{
		// free values in undefined area
		for (int i=inSize; i<iSize; i++) pData[i].Set0();
		iSize=inSize;
		return;
	}

	// bounds check
	if (inSize > MaxSize) return;

	// create new array (initialises)
	C4Value* pnData = new C4Value [inSize];
	if (!pnData) return;

	// move existing values
	int32_t i;
	for (i=0; i<iSize; i++)
		pData[i].Move(&pnData[i]);

	// replace
	delete[] pData;
	pData = pnData;
	iSize = inSize;
}

bool C4ValueList::operator==(const C4ValueList& IntList2) const
{
	for (int32_t i=0; i<Max(iSize, IntList2.GetSize()); i++)
		if (GetItem(i) != IntList2.GetItem(i))
			return false;

	return true;
}

void C4ValueList::Reset()
{
	delete[] pData; pData = NULL;
	iSize = 0;
}

void C4ValueList::DenumeratePointers()
{
	for (int32_t i = 0; i < iSize; i++)
		pData[i].DenumeratePointer();
}

void C4ValueList::CompileFunc(class StdCompiler *pComp)
{
	int32_t inSize = iSize;
	// Size. Reset if not found.
	try
		{ pComp->Value(inSize); }
	catch (StdCompiler::NotFoundException *pExc)
		{ Reset(); delete pExc; return; }
	// Separator
	pComp->Separator(StdCompiler::SEP_SEP2);
	// Allocate
	if (pComp->isCompiler()) this->SetSize(inSize);
	// Values
	pComp->Value(mkArrayAdapt(pData, iSize, C4Value()));
}

C4ValueArray::C4ValueArray()
		: C4ValueList(), iRefCnt(0), iElementReferences(0)
{
}

C4ValueArray::C4ValueArray(int32_t inSize)
		: C4ValueList(inSize), iRefCnt(0), iElementReferences(0)
{
}

C4ValueArray::C4ValueArray(const C4ValueArray &Array2)
		: C4ValueList(Array2), iRefCnt(1), iElementReferences(0)
{
}

C4ValueArray::~C4ValueArray()
{
}

enum { C4VALUEARRAY_DEBUG = 0 };

C4ValueArray * C4ValueArray::IncElementRef()
{
	if (iRefCnt > 1)
	{
		C4ValueArray * pNew = new C4ValueArray(*this);
		pNew->iElementReferences = 1;
		if (C4VALUEARRAY_DEBUG) printf("%p IncElementRef at %d, %d - Copying %p\n", static_cast<void*>(this), iRefCnt, iElementReferences, static_cast<void*>(pNew));
		--iRefCnt;
		return pNew;
	}
	else
	{
		if (C4VALUEARRAY_DEBUG) printf("%p IncElementRef at %d, %d\n", static_cast<void*>(this), iRefCnt, iElementReferences);
		++iElementReferences;
		return this;
	}
}

void C4ValueArray::DecElementRef()
{
	if (C4VALUEARRAY_DEBUG) printf("%p DecElementRef at %d, %d\n", static_cast<void*>(this), iRefCnt, iElementReferences);
	assert(iElementReferences > 0);
	--iElementReferences;
}

C4ValueArray * C4ValueArray::IncRef()
{
	if (iRefCnt >= 1 && iElementReferences)
	{
		C4ValueArray * pNew = new C4ValueArray(*this);
		if (C4VALUEARRAY_DEBUG) printf("%p IncRef from %d, %d - Copying %p\n", static_cast<void*>(this), iRefCnt, iElementReferences, static_cast<void*>(pNew));
		return pNew;
	}
	if (C4VALUEARRAY_DEBUG) printf("%p IncRef from %d, %d\n", static_cast<void*>(this), iRefCnt, iElementReferences);
	iRefCnt++;
	return this;
}

C4ValueArray * C4ValueArray::GetSlice(int32_t startIndex, int32_t endIndex)
{
	// adjust indices so that the default end index works and that negative numbers count backwards from the end of the string
	if (startIndex > iSize) startIndex = iSize;
	else if (startIndex < -iSize) throw new C4AulExecError(NULL, "Array slice: invalid start index");
	else if (startIndex < 0) startIndex += iSize;

	if (endIndex > iSize) endIndex = iSize;
	else if (endIndex < -iSize) throw new C4AulExecError(NULL, "Array slice: invalid end index");
	else if (endIndex < 0) endIndex += iSize;

	if (startIndex == 0 && endIndex == iSize)
	{
		return IncRef();
	}
	else
	{
		C4ValueArray* NewArray = new C4ValueArray(std::max(0, endIndex - startIndex));
		for (int i = startIndex; i < endIndex; ++i)
			NewArray->pData[i - startIndex] = pData[i];
		return NewArray;
	}
}

C4ValueArray * C4ValueArray::SetLength(int32_t size)
{
	if (iRefCnt > 1)
	{
		C4ValueArray * pNew = (new C4ValueArray(size))->IncRef();
		for (int32_t i = 0; i < size; i++)
			pNew->pData[i].Set(pData[i]);
		if (C4VALUEARRAY_DEBUG) printf("%p SetLength at %d, %d - Copying %p\n", static_cast<void*>(this), iRefCnt, iElementReferences, static_cast<void*>(pNew));
		--iRefCnt;
		return pNew;
	}
	else
	{
		if (C4VALUEARRAY_DEBUG) printf("%p SetLength at %d, %d\n", static_cast<void*>(this), iRefCnt, iElementReferences);
		SetSize(size);
		return this;
	}
}

void C4ValueArray::DecRef()
{
	if (C4VALUEARRAY_DEBUG) printf("%p DecRef from %d, %d%s\n", static_cast<void*>(this), iRefCnt, iElementReferences, iRefCnt == 1 ? " - Deleting" : "");
	assert(iRefCnt);
	if (!--iRefCnt)
	{
		delete this;
	}
}
