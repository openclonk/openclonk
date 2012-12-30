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

static C4Void FnRemove(C4Rope* Rope)
{
	Game.Ropes.RemoveRope(Rope);
	return C4Void();
}

static C4Object* FnGetFront(C4Rope* Rope)
{
	return Rope->GetFront()->GetObject();
}

static C4Object* FnGetBack(C4Rope* Rope)
{
	return Rope->GetBack()->GetObject();
}

static C4Void FnSetFront(C4Rope* Rope, C4Object* obj, Nillable<int> x, Nillable<int> y)
{
	Rope->SetFront(obj, x.IsNil() ? Fix0 : itofix(x), y.IsNil() ? Fix0 : itofix(y));
	return C4Void();
}

static C4Void FnSetBack(C4Rope* Rope, C4Object* obj, Nillable<int> x, Nillable<int> y)
{
	Rope->SetBack(obj, x.IsNil() ? Fix0 : itofix(x), y.IsNil() ? Fix0 : itofix(y));
	return C4Void();
}

static C4Void FnSetFrontAutoSegmentation(C4Rope* Rope, int max)
{
	Rope->SetFrontAutoSegmentation(itofix(max));
	return C4Void();
}

static C4Void FnSetBackAutoSegmentation(C4Rope* Rope, int max)
{
	Rope->SetBackAutoSegmentation(itofix(max));
	return C4Void();
}

static C4Void FnSetFrontFixed(C4Rope* Rope, bool fixed)
{
	Rope->SetFrontFixed(fixed);
	return C4Void();
}

static C4Void FnSetBackFixed(C4Rope* Rope, bool fixed)
{
	Rope->SetBackFixed(fixed);
	return C4Void();
}

static C4Void FnPullFront(C4Rope* Rope, int force)
{
	Rope->PullFront(itofix(force));
	return C4Void();
}

static C4Void FnPullBack(C4Rope* Rope, int force)
{
	Rope->PullBack(itofix(force));
	return C4Void();
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
	RopeDef = C4PropList::NewStatic(NULL, NULL, ::Strings.RegString("Rope"));
	RopeDef->SetName("Rope");
	pEngine->RegisterGlobalConstant("Rope", C4VPropList(RopeDef));

	Reg2List(pEngine);

	::AddFunc(this, "Remove", FnRemove);
	::AddFunc(this, "GetFront", FnGetFront);
	::AddFunc(this, "GetBack", FnGetBack);
	::AddFunc(this, "SetFront", FnSetFront);
	::AddFunc(this, "SetBack", FnSetBack);
	::AddFunc(this, "SetFrontAutoSegmentation", FnSetFrontAutoSegmentation);
	::AddFunc(this, "SetBackAutoSegmentation", FnSetBackAutoSegmentation);
	::AddFunc(this, "SetFrontFixed", FnSetFrontFixed);
	::AddFunc(this, "SetBackFixed", FnSetBackFixed);
	::AddFunc(this, "PullFront", FnPullFront);
	::AddFunc(this, "PullBack", FnPullBack);
	RopeDef->Freeze();
}
