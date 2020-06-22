/*
 * mape - C4 Landscape.txt editor
 *
 * Copyright (c) 2005-2009, Armin Burgmeier
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

#include "object/C4Def.h"
#include "object/C4DefList.h"
#include "lib/StdMeshLoader.h"

/* This is a simple implementation of C4DefList for what is required by
 * mape. We cannot link the full implementation since it would introduce
 * a dependency on C4Game, and therefore the rest of the engine. */

C4Def::C4Def(): Script(), C4PropListStatic(ScriptEngine.GetPropList(), nullptr, nullptr)
{
        Script.SetDef(this);
	assert(ScriptEngine.GetPropList());
	Graphics.pDef = this;

	Next = nullptr;
	Script.Clear();
}

C4Def::~C4Def()
{
	C4PropList::Clear();
	Script.Clear();
}

C4DefList::C4DefList()
{
	FirstDef = nullptr;
}

C4DefList::~C4DefList()
{
	Clear();
}

void C4DefList::Clear()
{
	while(FirstDef)
	{
		C4Def* out = FirstDef;
		FirstDef = FirstDef->Next;
		delete out;
	}
}

C4Def* C4DefList::ID2Def(C4ID id)
{
	C4Def* cdef;
	for(cdef = FirstDef; cdef != nullptr; cdef = cdef->Next)
		if(cdef->id == id)
			return cdef;
	return nullptr;
}

C4Def* C4DefList::GetByName(const StdStrBuf& name)
{
	return ID2Def(C4ID(name));
}

C4Def* C4DefList::GetDef(int iIndex)
{
	int counter = 0;
	for(C4Def* cdef = FirstDef; cdef != nullptr; cdef = cdef->Next)
	{
		if(counter == iIndex) return cdef;
		++counter;
	}

	return nullptr;
}

int C4DefList::GetDefCount()
{
	int counter = 0;
	for(C4Def* cdef = FirstDef; cdef != nullptr; cdef = cdef->Next)
		++counter;
	return counter;
}

bool C4DefList::Add(C4Def* def, bool fOverload)
{
	assert(ID2Def(def->id) == nullptr);

	def->Next = FirstDef;
	FirstDef = def;

	return true;
}

bool C4Def::Load(C4Group& hGroup, StdMeshSkeletonLoader& loader, DWORD dwLoadWhat, const char* szLanguage, C4SoundSystem* pSoundSystem, C4DefGraphicsPtrBackup *)
{
	// Assume ID has been set already!

	// Register with script engine
	::ScriptEngine.RegisterGlobalConstant(id.ToString(), C4VPropList(this));
	ParentKeyName = ::Strings.RegString(id.ToString());
	Script.Reg2List(&::ScriptEngine);

	if(!Script.Load(hGroup, "Script.c", nullptr, nullptr))
	{
		return false;
	}

	return true;
}

C4DefList Definitions;
