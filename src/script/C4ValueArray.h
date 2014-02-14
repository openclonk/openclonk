/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2013, The OpenClonk Team and contributors
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

#include "C4Value.h"

#ifndef INC_C4ValueList
#define INC_C4ValueList

// reference counted array of C4Values
class C4ValueArray
{
public:
	enum { MaxSize = 1000000 }; // ye shalt not create arrays larger than that!

	C4ValueArray();
	C4ValueArray(int32_t inSize);
	C4ValueArray(const C4ValueArray &);

	~C4ValueArray();

	C4ValueArray &operator =(const C4ValueArray&);

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

	C4Value operator[](int32_t iElem) const { return GetItem(iElem); }
	C4Value &operator[](int32_t iElem); // interface for the engine, asserts that 0 <= index < MaxSize

	void Reset();
	void SetItem(int32_t iElemNr, const C4Value &Value); // interface for script
	void SetSize(int32_t inSize); // (enlarge only!)

	void Denumerate(C4ValueNumbers *);

	// comparison
	bool operator==(const C4ValueArray&) const;

	// Compilation
	void CompileFunc(class StdCompiler *pComp, C4ValueNumbers *);


	// Add/Remove Reference
	void IncRef() { iRefCnt++; }
	void DecRef() { if (!--iRefCnt) delete this;  }

	// Return sub-array [startIndex, endIndex). Throws C4AulExecError.
	C4ValueArray * GetSlice(int32_t startIndex, int32_t endIndex);
	// Sets sub-array [startIndex, endIndex). Might resize the array.
	void SetSlice(int32_t startIndex, int32_t endIndex, const C4Value &Val);

	void Sort(class C4SortObject &rSort); // assume array of objects and sort by object sorting function
	void SortStrings(); // sort by values as strings
	void Sort(bool descending=false); // sort by values as integers or strings
	bool SortByProperty(C4String *prop_name, bool descending=false); // checks that this is an array of all proplists and sorts by values of given property. return false if an element is not a proplist.
	bool SortByArrayElement(int32_t array_idx, bool descending=false); // checks that this is an array of all arrays and sorts by array elements at index. returns false if an element is not an array or smaller than array_idx+1

private:
	// Reference counter
	unsigned int iRefCnt;
	int32_t iSize, iCapacity;
	C4Value* pData;
};

#endif

