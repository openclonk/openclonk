/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
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

/* At static list of C4IDs */

#include "C4Include.h"
#include "object/C4IDList.h"

#include "object/C4Def.h"
#include "object/C4DefList.h"
#include "graphics/C4GraphicsResource.h"
#include "graphics/C4Draw.h"

C4IDListChunk::C4IDListChunk()
{
	// prepare list
	pNext=nullptr;
}

C4IDListChunk::~C4IDListChunk()
{
	// empty the list
	Clear();
}

void C4IDListChunk::Clear()
{
	// kill all chunks
	C4IDListChunk *pChunk=pNext,*pChunk2;
	while (pChunk)
	{
		pChunk2=pChunk->pNext; pChunk->pNext=nullptr;
		delete pChunk; pChunk=pChunk2;
	}
	pNext=nullptr;
}

C4IDList::C4IDList() : C4IDListChunk()
{
	Default();
}

C4IDList::C4IDList(const C4IDList &rCopy): C4IDListChunk()
{
	Default();
	*this = rCopy;
}

C4IDList &C4IDList::operator = (const C4IDList &rCopy)
{
	// clear previous list
	Clear();
	// copy primary
	memcpy(this, &rCopy, sizeof(C4IDList));
	// copy all chunks
	C4IDListChunk *pTo=this;
	for (C4IDListChunk *pFrom=rCopy.pNext; pFrom; pFrom=pFrom->pNext)
	{
		C4IDListChunk *pNew = new C4IDListChunk(*pFrom);
		pTo->pNext=pNew; pTo=pNew;
	}
	// finalize
	pTo->pNext=nullptr;
	return *this;
}

C4IDList::~C4IDList()
{
	// destruction is done in chunk
}

void C4IDList::Clear()
{
	// inherited
	C4IDListChunk::Clear();
	// reset count
	Count=0;
}

bool C4IDList::IsClear() const
{
	return !Count;
}

C4ID C4IDList::GetID(size_t index, int32_t *ipCount) const
{
	// outside list?
	if (!Inside<size_t>(index+1,1u,Count)) return C4ID::None;
	// get chunk to query
	const C4IDListChunk *pQueryChunk=this;
	while (index>=C4IDListChunkSize) { pQueryChunk=pQueryChunk->pNext; index-=C4IDListChunkSize; }
	// query it
	if (ipCount) *ipCount=pQueryChunk->Count[index];
	return pQueryChunk->id[index];
}

int32_t C4IDList::GetCount(size_t index) const
{
	// outside list?
	if (!Inside<size_t>(index+1,1u,Count)) return 0;
	// get chunk to query
	const C4IDListChunk *pQueryChunk=this;
	while (index>=C4IDListChunkSize) { pQueryChunk=pQueryChunk->pNext; index-=C4IDListChunkSize; }
	// query it
	return pQueryChunk->Count[index];
}

bool C4IDList::SetCount(size_t index, int32_t iCount)
{
	// outside list?
	if (!Inside<size_t>(index+1,1u,Count)) return false;
	// get chunk to set in
	C4IDListChunk *pQueryChunk=this;
	while (index>=C4IDListChunkSize) { pQueryChunk=pQueryChunk->pNext; index-=C4IDListChunkSize; }
	// set it
	pQueryChunk->Count[index]=iCount;
	// success
	return true;
}

int32_t C4IDList::GetIDCount(C4ID c_id, int32_t iZeroDefVal) const
{
	// find id
	const C4IDListChunk *pQueryChunk=this;
	size_t cnt=Count,cntl=0;
	while (cnt--)
	{
		if (pQueryChunk->id[cntl]==c_id)
		{
			int32_t iCount=pQueryChunk->Count[cntl];
			return iCount ? iCount : iZeroDefVal;
		}
		if (++cntl==C4IDListChunkSize)
		{
			pQueryChunk=pQueryChunk->pNext;
			cntl=0;
		}
	}
	// none found
	return 0;
}

bool C4IDList::SetIDCount(C4ID c_id, int32_t iCount, bool fAddNewID)
{
	// find id
	C4IDListChunk *pQueryChunk=this;
	size_t cnt=Count,cntl=0;
	while (cnt--)
	{
		if (pQueryChunk->id[cntl]==c_id)
		{
			pQueryChunk->Count[cntl]=iCount; return true;
		}
		if (++cntl==C4IDListChunkSize)
		{
			pQueryChunk=pQueryChunk->pNext;
			cntl=0;
		}
	}
	// none found: add new
	if (fAddNewID)
	{
		// if end was reached, add new chunk
		if (!pQueryChunk)
		{
			C4IDListChunk *pLast=this;
			while (pLast->pNext) pLast=pLast->pNext;
			pQueryChunk=new C4IDListChunk();
			pLast->pNext=pQueryChunk;
		}
		// set id
		pQueryChunk->id[cntl]=c_id;
		pQueryChunk->Count[cntl]=iCount;
		// count id!
		++Count;
		// success
		return true;
	}
	// failure
	return false;
}

int32_t C4IDList::GetNumberOfIDs() const
{
	return Count;
}

int32_t C4IDList::GetIndex(C4ID c_id) const
{
	// find id in list
	const C4IDListChunk *pQueryChunk=this;
	size_t cnt=Count,cntl=0;
	while (cnt--)
	{
		if (pQueryChunk->id[cntl]==c_id) return Count-cnt-1;
		if (++cntl==C4IDListChunkSize)
		{
			pQueryChunk=pQueryChunk->pNext;
			cntl=0;
		}
	}
	// not found
	return -1;
}

bool C4IDList::IncreaseIDCount(C4ID c_id, bool fAddNewID, int32_t IncreaseBy, bool fRemoveEmpty)
{
	// find id in list
	C4IDListChunk *pQueryChunk=this;
	size_t cnt=Count,cntl=0;
	while (cnt--)
	{
		if (pQueryChunk->id[cntl]==c_id)
		{
			// increase count
			pQueryChunk->Count[cntl]+=IncreaseBy;
			// check count
			if (fRemoveEmpty && !pQueryChunk->Count[cntl]) DeleteItem(Count-cnt-1);
			// success
			return true;
		}
		if (++cntl==C4IDListChunkSize)
		{
			pQueryChunk=pQueryChunk->pNext;
			cntl=0;
		}
	}
	// add desired?
	if (!fAddNewID) return true;
	// add it
	// if end was reached, add new chunk
	if (!pQueryChunk)
	{
		C4IDListChunk *pLast=this;
		while (pLast->pNext) pLast=pLast->pNext;
		pQueryChunk=new C4IDListChunk();
		pLast->pNext=pQueryChunk;
	}
	// set id
	pQueryChunk->id[cntl]=c_id;
	pQueryChunk->Count[cntl]=IncreaseBy;
	++Count;
	// success
	return true;
}


// Access by category-sorted index
C4ID C4IDList::GetID(C4DefList &rDefs, int32_t dwCategory, int32_t index, int32_t *ipCount) const
{
	int32_t cindex=-1;
	C4Def *cDef;
	if (ipCount) *ipCount=0;
	// find id
	const C4IDListChunk *pQueryChunk=this;
	size_t cnt=Count,cntl=0;
	while (cnt--)
	{
		if ((dwCategory==C4D_All) || ( (cDef=rDefs.ID2Def(pQueryChunk->id[cntl])) && (cDef->Category & dwCategory) ) )
		{
			cindex++;
			if (cindex==index) { if (ipCount) *ipCount=pQueryChunk->Count[cntl]; return pQueryChunk->id[cntl]; }
		}
		if (++cntl==C4IDListChunkSize)
		{
			pQueryChunk=pQueryChunk->pNext;
			cntl=0;
		}
	}
	return C4ID::None;
}

int32_t C4IDList::GetCount(C4DefList &rDefs, int32_t dwCategory, int32_t index) const
{
	int32_t cindex=-1;
	C4Def *cDef;
	const C4IDListChunk *pQueryChunk=this;
	size_t cnt=Count,cntl=0;
	while (cnt--)
	{
		if ((dwCategory==C4D_All) || ( (cDef=rDefs.ID2Def(pQueryChunk->id[cntl])) && (cDef->Category & dwCategory) ) )
		{
			cindex++;
			if (cindex==index) return pQueryChunk->Count[cntl];
		}
		if (++cntl==C4IDListChunkSize)
		{
			pQueryChunk=pQueryChunk->pNext;
			cntl=0;
		}
	}
	return 0;
}

bool C4IDList::SetCount(C4DefList &rDefs, int32_t dwCategory, int32_t index, int32_t iCount)
{
	int32_t cindex=-1;
	C4Def *cDef;
	C4IDListChunk *pQueryChunk=this;
	size_t cnt=Count,cntl=0;
	while (cnt--)
	{
		if ((dwCategory==C4D_All) || ( (cDef=rDefs.ID2Def(pQueryChunk->id[cntl])) && (cDef->Category & dwCategory) ) )
		{
			cindex++;
			if (cindex==index) { pQueryChunk->Count[cntl]=iCount; return true; }
		}
		if (++cntl==C4IDListChunkSize)
		{
			pQueryChunk=pQueryChunk->pNext;
			cntl=0;
		}
	}
	return false;
}

int32_t C4IDList::GetNumberOfIDs(C4DefList &rDefs, int32_t dwCategory) const
{
	int32_t idnum=0;
	C4Def *cDef;
	const C4IDListChunk *pQueryChunk=this;
	size_t cnt=Count,cntl=0;
	while (cnt--)
	{
		if ((dwCategory==C4D_All) || ( (cDef=rDefs.ID2Def(pQueryChunk->id[cntl])) && (cDef->Category & dwCategory) ) )
			idnum++;
		if (++cntl==C4IDListChunkSize)
		{
			pQueryChunk=pQueryChunk->pNext;
			cntl=0;
		}
	}
	return idnum;
}
// IDList merge

bool C4IDList::Add(C4IDList &rList)
{
	C4IDListChunk *pQueryChunk=&rList;
	size_t cnt=rList.Count,cntl=0;
	while (cnt--)
	{
		IncreaseIDCount(pQueryChunk->id[cntl], true, pQueryChunk->Count[cntl]);
		if (++cntl==C4IDListChunkSize)
		{
			pQueryChunk=pQueryChunk->pNext;
			cntl=0;
		}
	}
	return true;
}

bool C4IDList::ConsolidateValids(C4DefList &rDefs, int32_t dwCategory)
{
	bool fIDsRemoved=false;
	C4IDListChunk *pQueryChunk=this;
	size_t cnt=Count,cntl=0;
	C4Def* pDef;
	while (cnt--)
	{
		// ID does not resolve to a valid def or category is specified and def does not match category
		if (!(pDef = rDefs.ID2Def(pQueryChunk->id[cntl])) || (dwCategory && !(pDef->Category & dwCategory)))
		{
			// delete it
			DeleteItem(Count-cnt-1);
			// handle this list index again!
			--cntl;
			// something was done
			fIDsRemoved=true;
		}
		if (++cntl==C4IDListChunkSize)
		{
			pQueryChunk=pQueryChunk->pNext;
			cntl=0;
		}
	}
	return fIDsRemoved;
}

void C4IDList::Draw(C4Facet &cgo, int32_t iSelection,
                    C4DefList &rDefs, DWORD dwCategory,
                    bool fCounts, int32_t iAlign) const
{

	int32_t sections = cgo.GetSectionCount();
	int32_t idnum = GetNumberOfIDs(rDefs,dwCategory);
	int32_t firstid = Clamp<int32_t>(iSelection-sections/2,0,std::max<int32_t>(idnum-sections,0));
	int32_t idcount;
	C4ID c_id;
	C4Facet cgo2;
	char buf[10];
	for (int32_t cnt=0; (cnt<sections) && (c_id=GetID(rDefs,dwCategory,firstid+cnt,&idcount)); cnt++)
	{
		cgo2 = cgo.TruncateSection(iAlign);
		rDefs.Draw(c_id,cgo2,(firstid+cnt==iSelection),0);
		sprintf(buf,"%dx",idcount);
		if (fCounts) pDraw->TextOut(buf, ::GraphicsResource.FontRegular, 1.0, cgo2.Surface,cgo2.X+cgo2.Wdt-1, cgo2.Y + cgo2.Hgt - 1 - ::GraphicsResource.FontRegular.GetLineHeight(),C4Draw::DEFAULT_MESSAGE_COLOR,ARight);
	}

}

void C4IDList::Default()
{
	Clear();
}

// Clear index entry and shift all entries behind down by one.

bool C4IDList::DeleteItem(size_t iIndex)
{
	// invalid index
	if (!Inside<size_t>(iIndex+1,1u,Count)) return false;
	// get chunk to delete of
	size_t index=iIndex;
	C4IDListChunk *pQueryChunk=this;
	while (index>=C4IDListChunkSize) { pQueryChunk=pQueryChunk->pNext; index-=C4IDListChunkSize; }
	// shift down all entries behind
	size_t cnt=--Count-iIndex,cntl=index,cntl2=cntl;
	C4IDListChunk *pNextChunk=pQueryChunk;
	while (cnt--)
	{
		// check for list overlap
		if (++cntl2==C4IDListChunkSize)
		{
			pNextChunk=pQueryChunk->pNext;
			cntl2=0;
		}
		// move down
		pQueryChunk->id[cntl]=pNextChunk->id[cntl2];
		pQueryChunk->Count[cntl]=pNextChunk->Count[cntl2];
		// next item
		pQueryChunk=pNextChunk; cntl=cntl2;
	}
	// done
	return true;
}

bool C4IDList::operator==(const C4IDList& rhs) const
{
	// compare counts
	if (Count != rhs.Count) return false;
	// compare all chunks
	const C4IDListChunk *pChunk1 = this;
	const C4IDListChunk *pChunk2 = &rhs;
	int32_t cnt=Count;
	while (pChunk1 && pChunk2)
	{
		if (memcmp(pChunk1->id, pChunk2->id, sizeof(C4ID)*std::min<int32_t>(cnt, C4IDListChunkSize)) ) return false;
		if (memcmp(pChunk1->Count, pChunk2->Count, sizeof(int32_t)*std::min<int32_t>(cnt, C4IDListChunkSize)) ) return false;
		pChunk1=pChunk1->pNext; pChunk2=pChunk2->pNext;
		cnt-=C4IDListChunkSize;
	}
	// equal!
	return true;
}

void C4IDList::CompileFunc(StdCompiler *pComp, bool fValues)
{
	// Get compiler characteristics
	bool deserializing = pComp->isDeserializer();
	bool fNaming = pComp->hasNaming();
	// Compiling: Clear existing data first
	if (deserializing) Clear();
	// Start
	C4IDListChunk *pChunk = this;
	size_t iNr = 0, iCNr = 0;
	// Without naming: Compile Count
	int32_t iCount = Count;
	if (!fNaming) pComp->Value(iCount);
	Count = iCount;
	// Read
	for (;;)
	{
		// Prepare compiling of single mapping
		if (!deserializing)
		{
			// End of list?
			if (iNr >= Count) break;
			// End of chunk?
			if (iCNr >= C4IDListChunkSize)
			{
				pChunk = pChunk->pNext;
				iCNr = 0;
			}
		}
		else
		{
			// End of list?
			if (!fNaming) if (iNr >= Count) break;
			// End of chunk?
			if (iCNr >= C4IDListChunkSize)
			{
				pChunk = pChunk->pNext = new C4IDListChunk();
				iCNr = 0;
			}
		}
		// Separator (';')
		if (iNr > 0) if (!pComp->Separator(StdCompiler::SEP_SEP2)) break;
		// ID
		pComp->Value(mkDefaultAdapt(pChunk->id[iCNr], C4ID::None));
		// ID not valid? Note that C4ID::None is invalid.
		if (pChunk->id[iCNr] == C4ID::None) break;
		// Value: Skip this part if requested
		if (fValues)
		{
			// Separator ('=')
			if (pComp->Separator(StdCompiler::SEP_SET))
				// Count
				pComp->Value(mkDefaultAdapt(pChunk->Count[iCNr], 0));
		}
		else if (deserializing)
			pChunk->Count[iCNr] = 0;
		// Goto next entry
		iNr++; iCNr++;
		// Save back count
		if (deserializing && fNaming) Count = iNr;
	}
}
