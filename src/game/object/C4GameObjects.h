/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001, 2004  Sven Eberhardt
 * Copyright (c) 2006  Peter Wortmann
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
// game object lists

#ifndef INC_C4GameObjects
#define INC_C4GameObjects

#include <C4ObjectList.h>
#include <C4FindObject.h>
#include <C4Sector.h>

class C4ObjResort;

// main object list class
class C4GameObjects : public C4NotifyingObjectList
	{
	public:
		C4GameObjects(); // constructor
		~C4GameObjects(); // destructor
		void Default();
		void Init(int32_t iWidth, int32_t iHeight);
		void Clear(bool fClearInactive); // clear objects
		void Clear() { Clear(true); } // don't use default parameters so we get a correct vtbl entry
		void CompileFunc(StdCompiler *pComp, bool fSkipPlayerObjects = false);

	public:
		C4LSectors Sectors; // section object lists
		C4ObjectList InactiveObjects; // inactive objects (Status=2)
		C4ObjectList BackObjects; // objects in background (C4D_Background)
		C4ObjectList ForeObjects; // objects in foreground (C4D_Foreground)
		C4ObjResort *ResortProc; // current sheduled user resorts

		unsigned int LastUsedMarker; // last used value for C4Object::Marker

		using C4ObjectList::Add;
		bool Add(C4Object *nObj); // add object
		bool Remove(C4Object *pObj); // clear pointers to object

		C4ObjectList &ObjectsAt(int ix, int iy); // get object list for map pos

		void CrossCheck(); // various collision-checks
		C4Object *AtObject(int ctx, int cty, DWORD &ocf, C4Object *exclude=NULL); // find object at ctx/cty
		void Synchronize(); // network synchronization

		C4Object *FindInternal(C4ID id); // find object in first sector
		virtual C4Object *ObjectPointer(int32_t iNumber); // object pointer by number
		virtual C4PropList *PropListPointer(int32_t iNumber); // object pointer by number
		int32_t ObjectNumber(C4PropList *pObj); // object number by pointer
		C4Object* SafeObjectPointer(int32_t iNumber);
		C4Object* Denumerated(C4Object *pObj);
		C4Object* Enumerated(C4Object *pObj);

		C4ObjectList &ObjectsInt(); // return object list containing system objects

	  void PutSolidMasks();
		void RemoveSolidMasks();
		void RecheckSolidMasks();

		int Load(C4Group &hGroup, bool fKeepInactive);
		bool Save(const char *szFilename, bool fSaveGame, bool fSaveInactive);
	  bool Save(C4Group &hGroup, bool fSaveGame, bool fSaveInactive);

		void UpdateScriptPointers(); // update pointers to C4AulScript *

		void UpdatePos(C4Object *pObj);
		void UpdatePosResort(C4Object *pObj);

		bool OrderObjectBefore(C4Object *pObj1, C4Object *pObj2); // order pObj1 before pObj2
		bool OrderObjectAfter(C4Object *pObj1, C4Object *pObj2); // order pObj1 after pObj2
		void FixObjectOrder(); // Called after loading: Resort any objects that are out of order
		void ResortUnsorted(); // resort any objects with unsorted-flag set into lists
		void ExecuteResorts(); // execute custom resort procs

		void DeleteObjects(bool fDeleteInactive); // delete all objects and links

		bool ValidateOwners();
		bool AssignInfo();
		void AssignPlrViewRange();
		void SortByCategory();
		void SyncClearance();
		void ResetAudibility();
		void UpdateTransferZones();
		void SetOCF();
	protected:
		C4Set<C4PropListNumbered *> PropLists;
		friend class C4PropListNumbered;
	};

extern C4GameObjects Objects;

// sheduled resort holder
class C4ObjResort
	{
	public:
		C4ObjResort(); // constructor
		~C4ObjResort(); // destructor

		void Execute(); // do the resort!
		void Sort(C4ObjectLink *pFirst, C4ObjectLink *pLast); // sort list between pFirst and pLast
		void SortObject();        // sort single object within its category

		int Category;							// object category mask to be sorted
		C4AulFunc *OrderFunc;			// function determining new sort order
		C4ObjResort *Next;				// next resort holder
		C4Object *pSortObj, *pObjBefore;	// objects that are swapped if no OrderFunc is given
		bool fSortAfter;          // if set, the sort object is sorted
	};

#endif
