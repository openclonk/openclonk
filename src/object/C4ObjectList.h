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

/* Dynamic object list */

#ifndef INC_C4ObjectList
#define INC_C4ObjectList

#include "object/C4Id.h"

class C4ObjectLink
{
public:
	C4Object *Obj;
	C4ObjectLink *Prev,*Next;
};

class C4ObjectListChangeListener
{
public:
	virtual void OnObjectRemove(C4ObjectList * pList, C4ObjectLink * pLnk) {};
	virtual void OnObjectAdded(C4ObjectList * pList, C4ObjectLink * pLnk) {};
	virtual void OnObjectRename(C4ObjectList * pList, C4ObjectLink * pLnk) {};
	virtual void OnObjectContainerChanged(C4Object *obj, C4Object *old_container, C4Object *new_container) {};
	virtual ~C4ObjectListChangeListener() { }
};

extern C4ObjectListChangeListener & ObjectListChangeListener;

class C4ObjectList
{
public:
	C4ObjectList();
	C4ObjectList(const C4ObjectList &List);
	virtual ~C4ObjectList();

	C4ObjectLink *First, *Last;
	int Mass;
	std::list<int32_t> *pEnumerated;

	enum SortType { stNone=0, stMain, stContents, stReverse };

	// An iterator which survives if an object is removed from the list
	class iterator
	{
	public:
		~iterator();
		iterator& operator++ (); // prefix ++
		iterator operator++ (int); // postfix ++
		iterator(const iterator & iter);
		C4Object * operator* ();
		bool operator== (const iterator & iter) const;
		bool operator!= (const iterator & iter) const;
		// advance until end reached or link with target Obj found - return false in former case, true in latter
		bool find(C4Object* target);
		// return whether the iterator has reached the end
		bool atEnd() const;
		// advance the iterator position, either going forward or backward depending on the reverse flag
		bool advance();
		// reset the iterator to either the beginning or the end of the list, depending on the reverse flag
		bool reset();

		iterator& operator=(const iterator & iter);
	private:
		iterator(const C4ObjectList & List, const C4ObjectLink * pLink, bool reverse);
		const C4ObjectList & List;
		// instead of pointing to the current link make a copy of it
		C4ObjectLink link;
		iterator * Next;
		bool reverse;
		friend class C4ObjectList;
	};
	iterator begin() const;
	const iterator end() const;

	// Helper object returned by reverse() - allows for iterating the C4ObjectList in reverse order, still using the for (x : list) syntax
	class ReverseView
	{
	private:
		const C4ObjectList& list;
	public:
		ReverseView(const C4ObjectList& list): list(list) {}
		// return an iterator at the end of the list
		iterator begin() const;
		// return an iterator at the position before the first item in the list
		iterator end() const;
	};

	// Return a temporary object allowing for reverse-order iteration of the C4ObjectList
	const ReverseView reverse() const { return ReverseView(*this); }

	virtual void Default();
	virtual void Clear();
	void Sort();
	void Copy(const C4ObjectList &rList);
	void DrawIfCategory(C4TargetFacet &cgo, int iPlayer, uint32_t dwCat, bool fInvert); // draw all objects that match dwCat (or don't match if fInvert)
	void Draw(C4TargetFacet &cgo, int iPlayer, int MinPlane, int MaxPlane); // draw all objects
	void DrawSelectMark(C4TargetFacet &cgo) const;
	void CloseMenus();
	void UpdateGraphics(bool fGraphicsChanged);
	void UpdateFaces(bool bUpdateShape);
	void ClearInfo(C4ObjectInfo *pInfo);

	typedef int SortProc(C4Object *, C4Object *);

	virtual bool Add(C4Object *nObj, SortType eSort, C4ObjectList *pLstSorted = nullptr);
	bool AddSortCustom(C4Object *nObj, SortProc &pSortProc);
	virtual bool Remove(C4Object *pObj);

	virtual bool AssignInfo();
	virtual bool ValidateOwners();
	StdStrBuf GetNameList(C4DefList &rDefs) const;
	bool IsClear() const;
	bool DenumeratePointers();
	bool Write(char *szTarget);
	void CompileFunc(StdCompiler *pComp, C4ValueNumbers * = 0);
	void CompileFunc(StdCompiler *pComp, bool fSkipPlayerObjects, C4ValueNumbers *);
	void Denumerate(C4ValueNumbers *);

	bool IsContained(const C4Object *pObj) const;
	int ClearPointers(C4Object *pObj);
	int ObjectCount(C4ID id=C4ID::None) const;
	int MassCount();
	int ListIDCount(int32_t dwCategory) const;

	C4Object* GetObject(int Index=0) const;
	C4Object* GetFirstObject() const { return First ? First->Obj : nullptr; }
	C4Object* GetLastObject() const { return Last ? Last->Obj : nullptr; }
	C4Object* Find(C4Def * def, int iOwner=ANY_OWNER, DWORD dwOCF=OCF_All);
	C4Object* FindOther(C4ID id, int iOwner=ANY_OWNER);

	const C4ObjectLink* GetLink(const C4Object *pObj) const;
	C4ObjectLink* GetLink(const C4Object *pObj)
	{ return const_cast<C4ObjectLink*>(const_cast<const C4ObjectList*>(this)->GetLink(pObj)); }

	C4ID GetListID(int32_t dwCategory, int Index) const;

	bool ShiftContents(C4Object *pNewFirst); // cycle list so pNewFirst is at front

	void DeleteObjects(); // delete all objects and links

	void UpdateScriptPointers(); // update pointers to C4AulScript *

	bool CheckSort(C4ObjectList *pList); // check that all objects of this list appear in the other list in the same order
	void CheckCategorySort(); // assertwhether sorting by category is done right

protected:
	virtual void InsertLinkBefore(C4ObjectLink *pLink, C4ObjectLink *pBefore);
	virtual void InsertLink(C4ObjectLink *pLink, C4ObjectLink *pAfter);
	virtual void RemoveLink(C4ObjectLink *pLnk);
	mutable iterator * FirstIter;
	iterator * AddIter(iterator * iter) const;
	void RemoveIter(iterator * iter) const;

	friend class iterator;
};

class C4NotifyingObjectList: public C4ObjectList
{
public:
	C4NotifyingObjectList() { }
	C4NotifyingObjectList(const C4NotifyingObjectList &List): C4ObjectList(List) { }
	C4NotifyingObjectList(const C4ObjectList &List): C4ObjectList(List) { }
	virtual ~C4NotifyingObjectList() { }
protected:
	virtual void InsertLinkBefore(C4ObjectLink *pLink, C4ObjectLink *pBefore);
	virtual void InsertLink(C4ObjectLink *pLink, C4ObjectLink *pAfter);
	virtual void RemoveLink(C4ObjectLink *pLnk);
};

// This iterator is used to return objects of same ID and picture as grouped.
// It's taking advantage of the fact that object lists are sorted by ID.
// Used by functions such as C4ObjectList::DrawIDList, or the menu-filling of
// activation/selling menus
class C4ObjectListIterator
{
private:
	C4ObjectList & rList; // iterated list
	C4ObjectList::iterator pCurr; // link to last returned object
	C4ObjectList::iterator pCurrID; // link to head of link group with same ID

	C4ObjectListIterator(const C4ObjectListIterator &rCopy); // no copy ctor
public:
	C4ObjectListIterator(C4ObjectList &rList) : rList(rList), pCurr(rList.end()), pCurrID(rList.begin()) {} // ctor
	C4Object *GetNext(int32_t *piCount); // get next object; return nullptr if end is reached
};

#endif
