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
#include "C4Include.h"
#include "script/C4ValueArray.h"
#include <algorithm>

#include "script/C4Aul.h"
#include "object/C4FindObject.h"

C4ValueArray::C4ValueArray()
		: pData(nullptr), iSize(0), iCapacity(0), constant(false)
{
}

C4ValueArray::C4ValueArray(int32_t inSize)
		: pData(nullptr), iSize(0), iCapacity(0), constant(false)
{
	SetSize(inSize);
}

C4ValueArray::C4ValueArray(const C4ValueArray &ValueArray2)
		: pData(nullptr), iSize(0), iCapacity(0), constant(false)
{
	SetSize(ValueArray2.GetSize());
	for (int32_t i = 0; i < iSize; i++)
		pData[i].Set(ValueArray2.GetItem(i));
}

C4ValueArray::~C4ValueArray()
{
	delete[] pData; pData = nullptr;
	iSize = iCapacity = 0;
}

C4ValueArray &C4ValueArray::operator =(const C4ValueArray& ValueArray2)
{
	this->SetSize(ValueArray2.GetSize());
	for (int32_t i = 0; i < iSize; i++)
		pData[i].Set(ValueArray2.GetItem(i));
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
	assert(!constant);
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
			pData[i].SetPropList(reinterpret_cast<C4PropList *>(pPos[i]));
		delete [] pPos;
	}
	else
		// Be sure to use stable sort, as otherweise the algorithm isn't garantueed
		// to produce identical results on all platforms!
		std::stable_sort(pData, pData+iSize, C4SortObjectSTL(rSort));
}

struct C4ValueArraySortStringscomp
{
	bool operator ()(const C4Value &v1, const C4Value &v2)
	{
		if (v1.getStr() && v2.getStr())
			return std::strcmp(v1._getStr()->GetCStr(), v2._getStr()->GetCStr()) < 0;
		return v2.getStr() != nullptr;
	}
};

void C4ValueArray::SortStrings()
{
	assert(!constant);
	std::stable_sort(pData, pData+iSize, C4ValueArraySortStringscomp());
}

struct C4ValueArraySortcomp
{
	bool operator ()(const C4Value &v1, const C4Value &v2)
	{
		// sort by whatever type the values have
		if (v1.getStr() && v2.getStr()) return v1._getStr()->GetData() < v2._getStr()->GetData();
		if (v1.CheckConversion(C4V_Int) && v2.CheckConversion(C4V_Int)) return v1._getInt() < v2._getInt();
		return false;
	}
};

void C4ValueArray::Sort(bool descending)
{
	assert(!constant);
	// sort by whatever type the values have
	std::stable_sort(pData, pData+iSize, C4ValueArraySortcomp());
	if (descending) std::reverse(pData, pData+iSize);
}

struct C4ValueArraySortPropertycomp
{
	C4String *prop_name; C4ValueArraySortcomp value_sort;
	C4ValueArraySortPropertycomp(C4String *prop_name) : prop_name(prop_name) {}
	bool operator ()(const C4Value &v1, const C4Value &v2)
	{
		C4Value p1,p2;
		if (!v1._getPropList()->GetPropertyByS(prop_name, &p1)) p1.Set0();
		if (!v2._getPropList()->GetPropertyByS(prop_name, &p2)) p2.Set0();
		return value_sort(p1,p2);
	}
};

bool C4ValueArray::SortByProperty(C4String *prop_name, bool descending)
{
	assert(!constant);
	// expect this to be an array of proplists and sort by given property
	// make sure we're all proplists before
	for (int32_t i=0; i<iSize; ++i)
		if (!pData[i].getPropList())
			return false;
	// now sort
	std::stable_sort(pData, pData+iSize, C4ValueArraySortPropertycomp(prop_name));
	if (descending) std::reverse(pData, pData+iSize);
	return true;
}

struct C4ValueArraySortArrayElementcomp
{
	int32_t element_idx; C4ValueArraySortcomp value_sort;
	C4ValueArraySortArrayElementcomp(int32_t element_idx) : element_idx(element_idx) {}
	bool operator ()(const C4Value &v1, const C4Value &v2)
	{
		return value_sort(v1._getArray()->GetItem(element_idx), v2._getArray()->GetItem(element_idx));
	}
};

bool C4ValueArray::SortByArrayElement(int32_t element_idx, bool descending)
{
	assert(element_idx>=0);
	assert(!constant);
	// expect this to be an array of arrays and sort by given element
	// make sure we're all arrays before
	for (int32_t i=0; i<iSize; ++i)
	{
		if (!pData[i].getArray())
			return false;
		if (pData[i]._getArray()->GetSize() <= element_idx)
			return false;
	}
	// now sort
	std::stable_sort(pData, pData+iSize, C4ValueArraySortArrayElementcomp(element_idx));
	if (descending) std::reverse(pData, pData+iSize);
	return true;
}

C4Value &C4ValueArray::operator[](int32_t iElem)
{
	assert(iElem < MaxSize);
	assert(iElem >= 0);
	assert(!constant);
	if (iElem >= iSize && iElem < MaxSize) this->SetSize(iElem + 1);
	// out-of-memory? This might not get caught, but it's better than a segfault
	assert(iElem < iSize);
	// return
	return pData[iElem];
}

void C4ValueArray::SetItem(int32_t iElem, const C4Value &Value)
{
	assert(!constant);
	// enlarge
	if (iElem < -iSize)
		throw C4AulExecError("array access: index out of range");
	else if (iElem < 0)
		iElem = iSize + iElem;
	else if (iElem >= iSize && iElem < MaxSize) this->SetSize(iElem + 1);
	// out-of-memory? This might not get caught, but it's better than a segfault
	if (iElem >= iSize)
		throw C4AulExecError("array access: index too large");
	// set
	pData[iElem]=Value;
}

void C4ValueArray::SetSize(int32_t inSize)
{
	if(inSize == iSize) return;
	assert(!constant);

	// array not larger than allocated memory? Well, just ignore the additional allocated mem then
	if (inSize <= iCapacity)
	{
		// free values in undefined area, do nothing if new is larger than old
		for (int i=inSize; i<iSize; i++) pData[i].Set0();
		iSize=inSize;
		return;
	}

	// bounds check
	if (inSize > MaxSize) return;

	// create new array
	C4Value* pnData = new C4Value [inSize];
	if (!pnData) return;

	// move existing values
	int32_t i;
	for (i=0; i<iSize; i++)
		pnData[i] = pData[i];

	// replace
	delete[] pData;
	pData = pnData;
	iSize = iCapacity = inSize;
}

bool C4ValueArray::operator==(const C4ValueArray& IntList2) const
{
	for (int32_t i=0; i<std::max(iSize, IntList2.GetSize()); i++)
		if (GetItem(i) != IntList2.GetItem(i))
			return false;

	return true;
}

void C4ValueArray::Reset()
{
	delete[] pData; pData = nullptr;
	iSize = iCapacity = 0;
}

void C4ValueArray::Denumerate(C4ValueNumbers * numbers)
{
	for (int32_t i = 0; i < iSize; i++)
		pData[i].Denumerate(numbers);
}

void C4ValueArray::CompileFunc(class StdCompiler *pComp, C4ValueNumbers * numbers)
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
	if (pComp->isDeserializer()) this->SetSize(inSize);
	// Values
	pComp->Value(mkArrayAdaptMap(pData, iSize, C4Value(), mkParAdaptMaker(numbers)));
}

enum { C4VALUEARRAY_DEBUG = 0 };

C4ValueArray * C4ValueArray::GetSlice(int32_t startIndex, int32_t endIndex)
{
	// adjust indices so that the default end index works and that negative numbers count backwards from the end of the string
	if (startIndex > iSize) startIndex = iSize;
	else if (startIndex < -iSize) throw C4AulExecError("array slice: start index out of range");
	else if (startIndex < 0) startIndex += iSize;

	if (endIndex > iSize) endIndex = iSize; // this also processes the MAX_INT default if no parameter is given in script
	else if (endIndex < -iSize) throw C4AulExecError("array slice: end index out of range");
	else if (endIndex < 0) endIndex += iSize;

	C4ValueArray* NewArray = new C4ValueArray(std::max(0, endIndex - startIndex));
	for (int i = startIndex; i < endIndex; ++i)
		NewArray->pData[i - startIndex] = pData[i];
	return NewArray;
}


void C4ValueArray::SetSlice(int32_t startIndex, int32_t endIndex, const C4Value &Val)
{
	// maximum bounds
	if(startIndex >= MaxSize) throw C4AulExecError("array slice: start index exceeds maximum capacity");

	// index from back
	if(startIndex < 0) startIndex += iSize;
	if(endIndex < 0) endIndex += iSize;

	// ensure relevant bounds
	if(startIndex < 0) throw C4AulExecError("array slice: start index out of range");
	if(endIndex < 0) throw C4AulExecError("array slice: end index out of range");
	if(endIndex < startIndex)
		endIndex = startIndex;

	// setting an array?
	if(Val.GetType() == C4V_Array)
	{
		const C4ValueArray &Other = *Val._getArray(); // Remember that &Other could be equal to this, carefull with modifying pData

		// Calculcate new size
		int32_t iNewEnd = std::min(startIndex + Other.GetSize(), (int32_t)MaxSize);
		int32_t iNewSize = iNewEnd;
		if(endIndex < iSize)
			iNewSize += iSize - endIndex;
		iNewSize = std::min(iNewSize, (int32_t)MaxSize);
		int32_t iOtherSize = Other.GetSize();

		if(iNewSize != iSize)
		{
			int32_t i,j;
			C4Value* pnData = pData;

			if(iNewSize > iCapacity)
			{
				 pnData = new C4Value [iNewSize];

				 // Copy first part of old array
				for(i = 0; i < startIndex && i < iSize; ++i)
					pnData[i] = pData[i];
			}

			// Copy the second slice of the new array
			for(i = iNewEnd, j = endIndex; i < iNewSize; ++i, ++j)
			{
				assert(j < iSize);
				pnData[i] = pData[j];
			}

			// Copy the data
			// Since pnData and pData can be the same, we can not copy with
			//for(i = startIndex, j = 0; i < iNewEnd; ++i, ++j)
			// but have to start from the end of the copied sequence. That works, since j <= i
			for(i = iNewEnd - 1, j = iNewEnd - startIndex - 1; i >= startIndex; --i, --j)
			{
				assert(j < iOtherSize);
				pnData[i] = Other.pData[j];
			}

			// Other values should have been initialized to 0 by new
			if(pData != pnData)
			{
				delete [] pData;
				pData = pnData;
				iCapacity = iSize = iNewSize;
			}
			else
			{
				// "ignore" the now unused part
				for(i = iNewSize; i < iSize; ++i)
					pData[i].Set0();
				iSize = iNewSize;
			}

		} else // slice has the same size as inserted array
			// Copy the data. changing pData does not break because if &Other == this and iNewSize == iSize, nothing happens at all
			for(int32_t i = startIndex, j = 0; j < iOtherSize; i++, j++)
				pData[i] = Other.pData[j];

	} else /* if(Val.GetType() != C4V_Array) */ {
		if(endIndex > MaxSize) endIndex = iSize;

		// Need resize?
		if(endIndex > iSize) SetSize(endIndex);

		// Fill
		for(int32_t i = startIndex; i < endIndex; i++)
			pData[i] = Val;

	}

}
