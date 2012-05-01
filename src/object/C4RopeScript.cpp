/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2012  Armin Burgmeier
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

#include <C4Include.h>
#include <C4Rope.h>
#include <C4AulDefFunc.h>

static C4Object* FnGetFront(C4AulContext* Context)
{
	return static_cast<C4Rope*>(Context->Def)->GetFront()->GetObject();
}

static C4Object* FnGetBack(C4AulContext* Context)
{
	return static_cast<C4Rope*>(Context->Def)->GetBack()->GetObject();
}

C4RopeAul::C4RopeAul():
	RopeDef(NULL)
{
}

C4RopeAul::~C4RopeAul()
{
	delete RopeDef;
}

void C4RopeAul::InitFunctionMap(C4AulScriptEngine* pEngine)
{
	delete RopeDef;
	RopeDef = C4PropList::NewScen();
	RopeDef->SetName("C4Rope");

	Reg2List(pEngine);

	::AddFunc(this, "GetFront", FnGetFront);
	::AddFunc(this, "GetBack", FnGetBack);
	RopeDef->Freeze();
}
