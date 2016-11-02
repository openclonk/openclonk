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
// game object lists

#ifndef INC_C4GameObjects
#define INC_C4GameObjects

#include "object/C4ObjectList.h"
#include "object/C4FindObject.h"
#include "object/C4Sector.h"

// main object list class
class C4GameObjects : public C4NotifyingObjectList
{
public:
	C4GameObjects(); // constructor
	~C4GameObjects(); // destructor
	void Default();
	void Init(int32_t iWidth, int32_t iHeight);
	void Clear(bool fClearInactive); // clear objects
	// don't use default parameters so we get a correct vtbl entry
	// don't clear internal objects, because they should not be cleared on section load
	void Clear() { Clear(false); }

private:
	uint32_t LastUsedMarker; // last used value for C4Object::Marker

public:
	C4LSectors Sectors; // section object lists
	C4ObjectList InactiveObjects; // inactive objects (Status=2)
	C4ObjectList ForeObjects; // objects in foreground (C4D_Foreground)

	using C4ObjectList::Add;
	bool Add(C4Object *nObj); // add object
	bool Remove(C4Object *pObj); // clear pointers to object

	C4ObjectList &ObjectsAt(int ix, int iy); // get object list for map pos

	void CrossCheck(); // various collision-checks
	C4Object *AtObject(int ctx, int cty, DWORD &ocf, C4Object *exclude=nullptr); // find object at ctx/cty
	void Synchronize(); // network synchronization
	void UpdateSolidMasks();

	C4Object *ObjectPointer(int32_t iNumber); // object pointer by number
	C4Object* SafeObjectPointer(int32_t iNumber);

	int PostLoad(bool fKeepInactive, C4ValueNumbers *);
	void Denumerate(C4ValueNumbers *);

	void UpdateScriptPointers(); // update pointers to C4AulScript *
	C4Value GRBroadcast(const char *szFunction, C4AulParSet *pPars, bool fPassError, bool fRejectTest);  // call function in all goals/rules/environment objects

	void UpdatePos(C4Object *pObj);
	void UpdatePosResort(C4Object *pObj);

	void FixObjectOrder(); // Called after loading: Resort any objects that are out of order
	void ResortUnsorted(); // resort any objects with unsorted-flag set into lists

	void DeleteObjects(bool fDeleteInactive); // delete all objects and links

	bool ValidateOwners();
	bool AssignInfo();
	void AssignLightRange();
	void SyncClearance();
	void ResetAudibility();
	void OnSynchronized();
	void SetOCF();

	uint32_t GetNextMarker(); // Get a new marker. If all markers are exceeded (LastUsedMarker is 0xffffffff), restart marker at 1 and reset all object markers to zero.
};

extern C4GameObjects Objects;

#endif
