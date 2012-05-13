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

static C4Void FnRemove(C4AulContext* Context)
{
	Game.Ropes.RemoveRope(static_cast<C4Rope*>(Context->Def));
	return C4Void();
}

static C4Object* FnGetFront(C4AulContext* Context)
{
	return static_cast<C4Rope*>(Context->Def)->GetFront()->GetObject();
}

static C4Object* FnGetBack(C4AulContext* Context)
{
	return static_cast<C4Rope*>(Context->Def)->GetBack()->GetObject();
}

static C4Void FnSetFront(C4AulContext* Context, C4Object* obj, Nillable<int> x, Nillable<int> y)
{
	static_cast<C4Rope*>(Context->Def)->SetFront(obj, x.IsNil() ? Fix0 : itofix(x), y.IsNil() ? Fix0 : itofix(y));
	return C4Void();
}

static C4Void FnSetBack(C4AulContext* Context, C4Object* obj, Nillable<int> x, Nillable<int> y)
{
	static_cast<C4Rope*>(Context->Def)->SetBack(obj, x.IsNil() ? Fix0 : itofix(x), y.IsNil() ? Fix0 : itofix(y));
	return C4Void();
}

static C4Void FnSetFrontAutoSegmentation(C4AulContext* Context, int max)
{
	static_cast<C4Rope*>(Context->Def)->SetFrontAutoSegmentation(itofix(max));
	return C4Void();
}

static C4Void FnSetBackAutoSegmentation(C4AulContext* Context, int max)
{
	static_cast<C4Rope*>(Context->Def)->SetBackAutoSegmentation(itofix(max));
	return C4Void();
}

static C4Void FnSetFrontFixed(C4AulContext* Context, bool fixed)
{
	static_cast<C4Rope*>(Context->Def)->SetFrontFixed(fixed);
	return C4Void();
}

static C4Void FnSetBackFixed(C4AulContext* Context, bool fixed)
{
	static_cast<C4Rope*>(Context->Def)->SetBackFixed(fixed);
	return C4Void();
}

static C4Void FnPullFront(C4AulContext* Context, int force)
{
	static_cast<C4Rope*>(Context->Def)->PullFront(itofix(force));
	return C4Void();
}

static C4Void FnPullBack(C4AulContext* Context, int force)
{
	static_cast<C4Rope*>(Context->Def)->PullBack(itofix(force));
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
	RopeDef = C4PropList::NewScen();
	RopeDef->SetName("C4Rope");

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
