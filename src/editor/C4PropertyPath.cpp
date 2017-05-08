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

#include "C4Include.h"
#include "editor/C4PropertyPath.h"
#include "script/C4Value.h"
#include "object/C4Object.h"
#include "object/C4GameObjects.h"
#include "object/C4DefList.h"
#include "object/C4Def.h"
#include "script/C4Effect.h"
#include "script/C4AulExec.h"
#include "editor/C4Console.h"


/* Property path for property setting synchronization */

C4PropertyPath::C4PropertyPath(C4PropList *target) : get_path_type(PPT_Root), set_path_type(PPT_Root)
{
	// Build string to set target
	if (target)
	{
		// Object target
		C4Object *obj = target->GetObject();
		C4PropListStatic *target_static;
		if (obj)
		{
			get_path.Format("Object(%d)", (int)obj->Number);
			root = get_path;
		}
		else if ((target_static = target->IsStatic()))
		{
			// Global static prop lists: Resolve name
			get_path = target_static->GetDataString();
			root = get_path;
		}
		else
		{
			// Otherwise leave empty. We do not want assignments into temporary values, etc.
		}
	}
}

C4PropertyPath::C4PropertyPath(C4Effect *fx, C4Object *target_obj) : get_path_type(PPT_Root), set_path_type(PPT_Root)
{
	// Effect property path: Represent as GetEffect("name", Object(%d), index) for object effects and GetEffect("name", nil, index) for global effects
	if (!fx) return;
	const char *name = fx->GetName();
	int32_t index = 0;
	for (C4Effect *ofx = target_obj ? target_obj->pEffects : ::ScriptEngine.pGlobalEffects; ofx; ofx = ofx->pNext)
		if (ofx == fx) break; else if (!strcmp(ofx->GetName(), name)) ++index;
	if (target_obj)
	{
		get_path.Format(R"(GetEffect("%s", Object(%d), %d))", name, (int)target_obj->Number, (int)index);
		root.Format("Object(%d)", (int)target_obj->Number);
	}
	else
	{
		get_path.Format(R"(GetEffect("%s", nil, %d))", name, (int)index);
		root = ::Strings.P[P_Global].GetData();
	}
}

C4PropertyPath::C4PropertyPath(const C4PropertyPath &parent, int32_t elem_index) : root(parent.root)
{
	get_path.Format("%s[%d]", parent.GetGetPath(), (int)elem_index);
	get_path_type = set_path_type = PPT_Index;
}

C4PropertyPath::C4PropertyPath(const C4PropertyPath &parent, const char *child_property)
	: get_path_type(PPT_Property), set_path_type(PPT_Property), root(parent.root)
{
	get_path.Format("%s.%s", parent.GetGetPath(), child_property);
}

void C4PropertyPath::SetSetPath(const C4PropertyPath &parent, const char *child_property, C4PropertyPath::PathType path_type)
{
	set_path_type = path_type;
	if (path_type == PPT_Property)
		set_path.Format("%s.%s", parent.GetGetPath(), child_property);
	else if (path_type == PPT_SetFunction)
		set_path.Format("%s->%s", parent.GetGetPath(), child_property);
	else if (path_type == PPT_GlobalSetFunction)
	{
		set_path.Copy(parent.GetGetPath());
		argument.Copy(child_property);
	}
	else if (path_type == PPT_RootSetFunction)
	{
		set_path.Format("%s->%s", parent.GetRoot(), child_property);
	}
	else
	{
		assert(false);
	}
}

void C4PropertyPath::SetProperty(const char *set_string) const
{
	// Compose script to update property
	const char *set_path_c = GetSetPath();
	StdStrBuf script;
	if (set_path_type == PPT_SetFunction || set_path_type == PPT_RootSetFunction)
		script.Format("%s(%s)", set_path_c, set_string);
	else if (set_path_type == PPT_GlobalSetFunction)
		script.Format("%s(%s,%s)", argument.getData(), set_path_c, set_string);
	else
		script.Format("%s=%s", set_path_c, set_string);
	// Execute synced scripted
	::Console.EditCursor.EMControl(CID_Script, new C4ControlScript(script.getData(), 0, false));
}

void C4PropertyPath::SetProperty(const C4Value &to_val, const C4PropListStatic *ignore_reference_parent) const
{
	SetProperty(to_val.GetDataString(9999999, ignore_reference_parent).getData());
}

C4Value C4PropertyPath::ResolveValue() const
{
	if (!get_path.getLength()) return C4VNull;
	return AulExec.DirectExec(::ScriptEngine.GetPropList(), get_path.getData(), "resolve property", false, nullptr);
}

C4Value C4PropertyPath::ResolveRoot() const
{
	if (!root.getLength()) return C4VNull;
	return AulExec.DirectExec(::ScriptEngine.GetPropList(), root.getData(), "resolve property root", false, nullptr);
}

void C4PropertyPath::DoCall(const char *call_string) const
{
	// Compose script call
	StdStrBuf script;
	script.Format(call_string, get_path.getData());
	// Execute synced scripted
	::Console.EditCursor.EMControl(CID_Script, new C4ControlScript(script.getData(), 0, false));
}


/* C4PropertyCollection */

void C4PropertyCollection::Clear()
{
	entries.clear();
	checked_values.clear();
}

bool C4PropertyCollection::CollectPropList(C4PropList *p, const C4PropertyPath &path, C4PropertyName prop, const C4Value &val, const char *base_name)
{
	// Collect prop list if it matches the condition
	C4Value cmp;
	if (p->GetProperty(prop, &cmp))
	{
		if (cmp == val)
		{
			// Only stuff set in the game
			if (!p->IsFrozen())
			{
				entries.emplace_back(path, C4VPropList(p), base_name);
				return true;
			}
		}
	}
	return false;
}

void C4PropertyCollection::CollectPropLists(C4ValueArray *target, const C4PropertyPath &target_path, C4PropertyName prop, const C4Value &val, const char *base_name)
{
	// Avoid recursion
	if (checked_values.find(target) != checked_values.end()) return;
	checked_values.insert(target);
	// Check all child elements
	for (int32_t index = 0; index < target->GetSize(); ++index)
	{
		const C4Value &childval = target->GetItem(index);
		switch (childval.GetType())
		{
		case C4V_Array:
			CollectPropLists(childval._getArray(), C4PropertyPath(target_path, index), prop, val, base_name);
			break;
		case C4V_PropList:
			CollectPropLists(childval._getPropList(), C4PropertyPath(target_path, index), prop, val, base_name);
			break;
		default:
			// nada
			break;
		}
	}
}

void C4PropertyCollection::CollectPropLists(C4PropList *target, const C4PropertyPath &target_path, C4PropertyName prop, const C4Value &val, const char *base_name)
{
	// Avoid recursion
	if (checked_values.find(target) != checked_values.end()) return;
	checked_values.insert(target);
	// Check base itself
	if (CollectPropList(target, target_path, prop, val, base_name))
	{
		// No need to check children then
		return;
	}
	// Check all child properties
	for (C4String *key : target->GetSortedLocalProperties(false))
	{
		C4Value childval;
		target->GetPropertyByS(key, &childval);
		switch (childval.GetType())
		{
		case C4V_Array:
			CollectPropLists(childval._getArray(), C4PropertyPath(target_path, key->GetCStr()), prop, val, base_name);
			break;
		case C4V_PropList:
			CollectPropLists(childval._getPropList(), C4PropertyPath(target_path, key->GetCStr()), prop, val, base_name);
			break;
		default:
			// nada
			break;
		}
	}
}

void C4PropertyCollection::CollectPropLists(C4PropertyName prop, const C4Value &val)
{
	Clear();
	// Walk over game content and search for matching prop lists
	// Walk in reverse because prop lists of more permanent objects are often assigned to temporary objects
	for (C4Object *obj : ::Objects.reverse())
	{
		CollectPropLists(obj, C4PropertyPath(obj), prop, val, obj->GetName());
		for (C4Effect *fx = obj->pEffects; fx; fx = fx->pNext)
		{
			CollectPropLists(fx, C4PropertyPath(fx, obj), prop, val, fx->GetName());
		}
	}
	for (C4Effect *fx = ::ScriptEngine.pGlobalEffects; fx; fx = fx->pNext)
	{
		CollectPropLists(fx, C4PropertyPath(fx, nullptr), prop, val, fx->GetName());
	}
}

