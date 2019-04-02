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

#include "object/C4FindObject.h"
#include "object/C4ObjectList.h"
#include "object/C4Sector.h"

// Main object list class
class C4GameObjects : public C4NotifyingObjectList
{
public:
	C4GameObjects(); // constructor
	~C4GameObjects() override; // destructor
	void Default() override;
	void Init(int32_t width, int32_t height);
	void Clear(bool fClearInactive); // clear objects
	// Don't use default parameters so we get a correct vtbl entry
	// Don't clear internal objects, because they should not be cleared on section load
	void Clear() override { Clear(false); }

private:
	uint32_t LastUsedMarker; // Last used value for C4Object::Marker

public:
	C4LSectors Sectors; // Section object lists
	C4ObjectList InactiveObjects; // Inactive objects (Status=2)
	C4ObjectList ForeObjects; // Objects in foreground (C4D_Foreground)

	using C4ObjectList::Add;
	bool Add(C4Object *game_object); // Add object
	bool Remove(C4Object *game_object) override; // Clear pointers to object

	void CrossCheck(); // Various collision-checks
	void Synchronize(); // Network synchronization
	void UpdateSolidMasks();

	C4Object *ObjectPointer(int32_t object_number); // Object pointer by number
	C4Object* SafeObjectPointer(int32_t object_number);

	int PostLoad(bool keep_inactive_objects, C4ValueNumbers *);
	void Denumerate(C4ValueNumbers *);

	void UpdateScriptPointers(); // Update pointers to C4AulScript *
	C4Value GRBroadcast(const char *function_name, C4AulParSet *parameters, bool pass_error, bool reject_test);  // Call function in all goals/rules/environment objects

	void UpdatePos(C4Object *game_object);
	void UpdatePosResort(C4Object *game_object);

	void FixObjectOrder(); // Called after loading: Resort any objects that are out of order
	void ResortUnsorted(); // Resort any objects with unsorted-flag set into lists

	void DeleteObjects(bool delete_inactive_objects); // Delete all objects and links

	bool ValidateOwners() override;
	bool AssignInfo() override;
	void AssignLightRange();
	void SyncClearance();
	void ResetAudibility();
	void OnSynchronized();
	void SetOCF();

	uint32_t GetNextMarker(); // Get a new marker. If all markers are exceeded (LastUsedMarker is 0xffffffff), restart marker at 1 and reset all object markers to zero.
};

extern C4GameObjects Objects;

#endif
