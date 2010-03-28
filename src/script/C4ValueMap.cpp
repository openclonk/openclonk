/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2002, 2004-2006  Peter Wortmann
 * Copyright (c) 2004-2006  Sven Eberhardt
 * Copyright (c) 2005, 2008  Günther Brammer
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
#include "C4Include.h"
#include "C4ValueMap.h"

// *** C4ValueMapData ***

C4ValueMapData::C4ValueMapData()
		: pData(0), pNames(0), bTempNameList(false), pNext(0)
{

}

C4ValueMapData::C4ValueMapData(const C4ValueMapData &DataToCopy)
		: pData(0), pNames(0), bTempNameList(false), pNext(0)
{
	SetNameList(DataToCopy.pNames);
	if (pNames) for (int32_t i = 0; i < pNames->iSize; i++)
			pData[i].Set(DataToCopy.pData[i]);
}

C4ValueMapData &C4ValueMapData::operator = (const C4ValueMapData &DataToCopy)
{
	SetNameList(DataToCopy.pNames);
	if (pNames) for (int32_t i = 0; i < pNames->iSize; i++)
			pData[i].Set(DataToCopy.pData[i]);
	return *this;
}

bool C4ValueMapData::operator == (const C4ValueMapData &Data) const
{
	if (pNames != Data.pNames) return false;
	if (pNames)
		for (int i = 0; i < pNames->iSize; i++)
			if (pData[i] != Data.pData[i])
				return false;
	return true;
}

C4ValueMapData::~C4ValueMapData()
{
	Reset();
}

void C4ValueMapData::Reset()
{
	// unreg from name list (if using one)
	if (pNames) UnRegister();
	pNames = 0;
	// free data
	delete[] pData;
	pData = 0;
}

void C4ValueMapData::ResetContent()
{
	if (pNames)
		// Realloc list (will destroy all data)
		ReAllocList();
	else
	{
		delete[] pData;
		pData = 0;
	}
}

void C4ValueMapData::SetNameList(C4ValueMapNames *pnNames)
{
	if (pNames == pnNames) return;
	if (pNames)
	{
		// save name array from old name list
		char **pOldNames = pNames->pNames;
		int32_t iOldSize = pNames->iSize;

		// unreg from old name list
		// (manually, because Data::UnRegister() would destroy content)
		C4ValueMapNames *pNames = this->pNames;

		pNames->UnRegister(this);

		// register at new name list
		pnNames->Register(this);

		// call OnNameListChanged to copy data and realloc data array
		OnNameListChanged(const_cast<const char **>(pOldNames), iOldSize);

		// delete old names list, if it is temporary
		if (bTempNameList)
			delete pNames;
		bTempNameList = false;

		// ok
	}
	else
	{
		// simply register...
		Register(pnNames);
	}
}

void C4ValueMapData::Register(C4ValueMapNames *pnNames)
{
	// UnReg from old?
	if (pNames) UnRegister();

	if (pnNames) pnNames->Register(this);
	pNames = pnNames;

	// alloc data array
	ReAllocList();
}


void C4ValueMapData::UnRegister()
{
	if (!pNames) return;

	// safe pointer
	C4ValueMapNames *pNames = this->pNames;

	// unreg
	pNames->UnRegister(this);

	// delete name list (if it is temporary)
	if (bTempNameList)
		delete pNames;
	bTempNameList = false;

	// delete data array
	delete[] pData;
	pData = 0;
}

C4ValueMapNames *C4ValueMapData::CreateTempNameList()
{
	// create new list
	C4ValueMapNames *pTempNames = new C4ValueMapNames();

	// register (this func will unreg if necessary, too)
	Register(pTempNames);

	// error?
	if (pNames != pTempNames)
	{
		delete pTempNames;
		return 0;
	}

	// set flag
	bTempNameList = true;

	return pTempNames;
}

void C4ValueMapData::ReAllocList()
{
	if (!pNames)
	{
		Reset();
		return;
	}

	// delete old array
	delete[] pData;

	// create new one
	pData = new C4Value [pNames->iSize] ();

	// ok...
}

void C4ValueMapData::OnNameListChanged(const char **pOldNames, int32_t iOldSize)
{
	if (!pNames)
	{
		Reset();
		return;
	}

	// this function does not use ReAllocList because the old values
	// have to be hold.

	// save pointer on old data
	C4Value *pOldData = pData;

	// create new data list
	pData = new C4Value [pNames->iSize] ();

	// (try to) copy data
	int32_t i, j;
	for (i = 0; i < iOldSize; i++)
	{
		//FIXME: This optimization is ugly.
		if (i < pNames->iSize && SEqual(pNames->pNames[i], pOldNames[i]))
		{
			pOldData[i].Move(&pData[i]);
		}
		else for (j = 0; j < pNames->iSize; j++)
			{
				if (SEqual(pNames->pNames[j], pOldNames[i]))
				{
					pOldData[i].Move(&pData[j]);
					break;
				}
			}
	}
	// delete old data array
	delete[] pOldData;
}

C4Value *C4ValueMapData::GetItem(int32_t iNr)
{
	// the list is nothing without name list...
	if (!pNames) return 0;

	if (iNr >= pNames->iSize) return 0;

	return &pData[iNr];
}

C4Value *C4ValueMapData::GetItem(const char *strName)
{
	if (!pNames) return 0;

	int32_t iNr = pNames->GetItemNr(strName);

	if (iNr == -1) return 0;

	return &pData[iNr];
}

int32_t C4ValueMapData::GetAnzItems()
{
	if (!pNames) return 0;
	return pNames->iSize;
}

void C4ValueMapData::DenumeratePointers()
{
	if (!pNames) return;
	for (int32_t i = 0; i < pNames->iSize; i++)
		pData[i].DenumeratePointer();
}

void C4ValueMapData::CompileFunc(StdCompiler *pComp)
{
	bool fCompiler = pComp->isCompiler();
	if (fCompiler) Reset();
	// Compile item count
	int32_t iValueCnt;
	if (!fCompiler) iValueCnt = pNames ? pNames->iSize : 0;
	pComp->Value(mkDefaultAdapt(iValueCnt, 0));
	// nuthing 2do for no items
	if (!iValueCnt) return;
	// Seperator (';')
	pComp->Seperator(StdCompiler::SEP_SEP2);
	// Data
	char **ppNames = !fCompiler ? pNames->pNames : new char * [iValueCnt];
	if (fCompiler) for (int32_t i = 0; i < iValueCnt; i++) ppNames[i] = 0;
	C4Value *pValues = !fCompiler ? pData : new C4Value [iValueCnt];
	// Compile
	try
	{
		for (int32_t i = 0; i < iValueCnt; i++)
		{
			// Seperate
			if (i) pComp->Seperator();
			// Name
			StdStrBuf Name;
			if (!fCompiler) Name.Ref(ppNames[i]);
			pComp->Value(mkParAdapt(Name, StdCompiler::RCT_Idtf));
			if (fCompiler) ppNames[i] = Name.GrabPointer();
			// Seperator ('=')
			pComp->Seperator(StdCompiler::SEP_SET);
			// Value
			pComp->Value(pValues[i]);
		}
	}
	catch (...)
	{
		// make sure no mem is leaked on compiler error in name list
		if (fCompiler)
		{
			for (int32_t i = 0; i < iValueCnt; i++) if (ppNames[i]) free(ppNames[i]);
			delete [] ppNames;
			delete [] pValues;
		}
		throw;
	}
	// Set
	if (fCompiler)
	{
		C4ValueMapNames *pOldNames = pNames;
		// Set
		CreateTempNameList();
		pNames->SetNameArray(const_cast<const char **>(ppNames), iValueCnt);
		for (int32_t i = 0; i < iValueCnt; i++) delete ppNames[i];
		delete [] ppNames; delete [] pData;
		pData = pValues;
		// Assign old name list
		if (pOldNames) SetNameList(pOldNames);
	}
}

// *** C4ValueMapNames ***

C4ValueMapNames::C4ValueMapNames()
		: pNames(0), iSize(0), pFirst(0)
{

}

C4ValueMapNames::C4ValueMapNames(C4ValueMapNames& NamesToCopy)
		: pNames(0), iSize(0), pFirst(0)
{
	ChangeNameList(const_cast<const char **>(NamesToCopy.pNames), NamesToCopy.iSize);
}

C4ValueMapNames& C4ValueMapNames::operator = (C4ValueMapNames &NamesToCopy)
{
	ChangeNameList(const_cast<const char **>(NamesToCopy.pNames), NamesToCopy.iSize);
	return *this;
}

C4ValueMapNames::~C4ValueMapNames()
{
	Reset();
}

void C4ValueMapNames::Reset()
{
	// unreg all data lists
	while (pFirst) UnRegister(pFirst);
	// free name list
	for (int32_t i = 0; i < iSize; i++)
		delete[] pNames[i];
	delete[] pNames;
	pNames = NULL;
	iSize = 0;
}

void C4ValueMapNames::Register(C4ValueMapData *pData)
{
	// add to begin of list
	pData->pNext = pFirst;
	pFirst = pData;
	// set name list
	pData->pNames = this;
}

void C4ValueMapNames::UnRegister(C4ValueMapData *pData)
{
	// find in list
	C4ValueMapData *pAktData = pFirst, *pLastData = 0;
	while (pAktData && pAktData != pData)
	{
		pLastData = pAktData;
		pAktData = pAktData->pNext;
	}

	if (!pAktData)
		// isn't registred here...
		return;

	// unreg
	if (pLastData)
		pLastData->pNext = pData->pNext;
	else
		pFirst = pData->pNext;
	pData->pNext = 0;

	pData->pNames = 0;
}

void C4ValueMapNames::ChangeNameList(const char **pnNames, int32_t nSize)
{
	// safe old name list
	char **pOldNames = pNames;
	int32_t iOldSize = iSize;


	// create new lists
	pNames = new char *[nSize];

	// copy names
	int32_t i;
	for (i = 0; i < nSize; i++)
	{
		pNames[i] = new char [SLen(pnNames[i]) + 1];
		SCopy(pnNames[i], pNames[i], SLen(pnNames[i]) + 1);
	}

	// set new size
	iSize = nSize;

	// call OnNameListChanged list for all "child" lists
	C4ValueMapData *pAktData = pFirst;
	while (pAktData)
	{
		pAktData->OnNameListChanged(const_cast<const char **>(pOldNames), iOldSize);
		pAktData = pAktData->pNext;
	}

	// delete old list
	for (i = 0; i < iOldSize; i++)
		delete[] pOldNames[i];
	delete[] pOldNames;

	// ok.
}

void C4ValueMapNames::SetNameArray(const char **pnNames, int32_t nSize)
{
	// simply pass it through...
	ChangeNameList(pnNames, nSize);
}

int32_t C4ValueMapNames::AddName(const char *pnName)
{
	// name already existing?
	int32_t iNr;
	if ((iNr=GetItemNr(pnName)) != -1)
		return iNr;

	// create new dummy lists
	const char **pDummyNames = new const char *[iSize + 1];

	// copy all data from old list
	// (danger! if ChangeNameList would now delete them before
	// creating the new list, this would cause cruel errors...)
	int32_t i;
	for (i = 0; i < iSize; i++)
	{
		pDummyNames[i] = pNames[i];
	}
	pDummyNames[i] = pnName;

	// change list
	ChangeNameList(pDummyNames, iSize + 1);

	// delete dummy arrays
	delete[] pDummyNames;

	// return index to new element (simply last element)
	return iSize-1;
}

int32_t C4ValueMapNames::GetItemNr(const char *strName)
{
	for (int32_t i = 0; i < iSize; i++)
		if (SEqual(pNames[i], strName))
			return i;
	return -1;
}
