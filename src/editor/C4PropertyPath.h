/*
* OpenClonk, http://www.openclonk.org
*
* Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
* Copyright (c) 2013, The OpenClonk Team and contributors
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

/* Generation of scripts to access values in objects and effects */

#ifndef INC_C4PropertyPath
#define INC_C4PropertyPath

#include "C4Include.h" // needed for automoc
#include "script/C4Value.h"

// Path to a property, like e.g. Object(123).foo.bar[456].baz
// Used to allow proper synchronization of property setting
class C4PropertyPath
{
	// TODO: For now just storing the path. May want to keep the path info later to allow validation/updating of values
	StdCopyStrBuf get_path, argument, set_path;
	StdCopyStrBuf root;

public:
	enum PathType
	{
		PPT_Root = 0,
		PPT_Property = 1,
		PPT_Index = 2,
		PPT_SetFunction = 3,
		PPT_GlobalSetFunction = 4,
		PPT_RootSetFunction = 5,
	} get_path_type, set_path_type;

public:
	C4PropertyPath() {}
	C4PropertyPath(C4PropList *target);
	C4PropertyPath(C4Effect *fx, C4Object *target_obj);
	C4PropertyPath(const char *path) : get_path(path), root(path), get_path_type(PPT_Root), set_path_type(PPT_Root) {}
	C4PropertyPath(const C4PropertyPath &parent, int32_t elem_index);
	C4PropertyPath(const C4PropertyPath &parent, const char *child_property);
	void SetSetPath(const C4PropertyPath &parent, const char *child_property, PathType path_type);
	void Clear() { get_path.Clear(); set_path.Clear(); }
	const char *GetGetPath() const { return get_path.getData(); }
	const char *GetSetPath() const { return set_path ? set_path.getData() : get_path.getData(); }
	const char *GetRoot() const { return root.getData(); } // Parent-most path (usually the object)
	bool IsEmpty() const { return get_path.getLength() <= 0; }

	C4Value ResolveValue() const;
	C4Value ResolveRoot() const;
	void SetProperty(const char *set_string) const;
	void SetProperty(const C4Value &to_val, const C4PropListStatic *ignore_reference_parent = nullptr) const;
	void DoCall(const char *call_string) const; // Perform a script call where %s is replaced by the current path

	bool operator ==(const C4PropertyPath &v) const { return get_path == v.get_path; }
};

// Collection of paths and their last values
class C4PropertyCollection
{
public:
	struct Entry
	{
		C4PropertyPath path;
		C4Value value;
		StdCopyStrBuf name;

		Entry(const C4PropertyPath &path, const C4Value &value, const char *name)
			: path(path), value(value), name(name) {}
	};

private:
	std::vector<Entry> entries;
	std::unordered_set<void *> checked_values;

	bool CollectPropList(C4PropList *p, const C4PropertyPath &path, C4PropertyName prop, const C4Value &val, const char *base_name);
	void CollectPropLists(C4ValueArray *target, const C4PropertyPath &target_path, C4PropertyName prop, const C4Value &val, const char *base_name);
	void CollectPropLists(C4PropList *target, const C4PropertyPath &target_path, C4PropertyName prop, const C4Value &val, const char *base_name);

public:
	// Reset
	void Clear();

	// Iterates over all game objects and effects to collect proplists that have the given property set to the given value
	void CollectPropLists(C4PropertyName prop, const C4Value &val);

	const std::vector<Entry> &GetEntries() const { return entries; }
};

#endif // INC_C4PropertyPath
