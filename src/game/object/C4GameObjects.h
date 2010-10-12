/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001, 2004  Sven Eberhardt
 * Copyright (c) 2006  Peter Wortmann
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
// game object lists

#ifndef INC_C4GameObjects
#define INC_C4GameObjects

#include <C4ObjectList.h>
#include <C4FindObject.h>
#include <C4Sector.h>

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

	unsigned int LastUsedMarker; // last used value for C4Object::Marker

	using C4ObjectList::Add;
	bool Add(C4Object *nObj); // add object
	bool Remove(C4Object *pObj); // clear pointers to object

	C4ObjectList &ObjectsAt(int ix, int iy); // get object list for map pos

	void CrossCheck(); // various collision-checks
	C4Object *AtObject(int ctx, int cty, DWORD &ocf, C4Object *exclude=NULL); // find object at ctx/cty
	void Synchronize(); // network synchronization
	void UpdateSolidMasks();

	virtual C4Object *ObjectPointer(int32_t iNumber); // object pointer by number
	virtual C4PropList *PropListPointer(int32_t iNumber); // object pointer by number
	int32_t ObjectNumber(C4PropList *pObj); // object number by pointer
	C4Object* SafeObjectPointer(int32_t iNumber);

	int Load(C4Group &hGroup, bool fKeepInactive);
	bool Save(const char *szFilename, bool fSaveGame, bool fSaveInactive);
	bool Save(C4Group &hGroup, bool fSaveGame, bool fSaveInactive);

	void UpdateScriptPointers(); // update pointers to C4AulScript *

	void UpdatePos(C4Object *pObj);
	void UpdatePosResort(C4Object *pObj);

	void FixObjectOrder(); // Called after loading: Resort any objects that are out of order
	void ResortUnsorted(); // resort any objects with unsorted-flag set into lists

	void DeleteObjects(bool fDeleteInactive); // delete all objects and links

	bool ValidateOwners();
	bool AssignInfo();
	void AssignPlrViewRange();
	void SyncClearance();
	void ResetAudibility();
	void UpdateTransferZones();
	void SetOCF();
protected:
	C4Set<C4PropListNumbered *> PropLists;
	friend class C4PropListNumbered;
};

extern C4GameObjects Objects;

#endif
