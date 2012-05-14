/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001, 2007  Sven Eberhardt
 * Copyright (c) 2011-2012  Günther Brammer
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
#include <C4Value.h>
#include <C4AulFunc.h>
#include <C4Aul.h>
#include <C4ScriptHost.h>

C4AulFunc::C4AulFunc(C4AulScript *pOwner, const char *pName):
		iRefCnt(0),
		Name(pName ? Strings.RegString(pName) : 0),
		MapNext(NULL),
		OverloadedBy (NULL)
{
	AppendToScript(pOwner);
	IncRef(); // see C4AulScript::Clear()
}

void C4AulFunc::AppendToScript(C4AulScript * pOwner)
{
	Owner = pOwner;
	if ((Prev = Owner->FuncL))
	{
		Prev->Next = this;
		Owner->FuncL = this;
	}
	else
	{
		Owner->Func0 = this;
		Owner->FuncL = this;
	}
	Next = NULL;
	assert(GetName() || Owner->Temporary);
	// add to global lookuptable with this name
	if (GetName())
		Owner->Engine->FuncLookUp.Add(this, true);
}

void C4AulFunc::RemoveFromScript()
{
	if (Prev) Prev->Next = Next;
	if (Next) Next->Prev = Prev;
	if (Owner->Func0 == this) Owner->Func0 = Next;
	if (Owner->FuncL == this) Owner->FuncL = Prev;
	assert(Owner);
	assert(Owner->Temporary || Name);
	assert(!Owner->GetPropList() || Owner->GetPropList()->GetFunc(Name) != this);
	if (GetName())
		Owner->Engine->FuncLookUp.Remove(this);
	Prev = 0;
	Next = 0;
	Owner = 0;
}

C4AulFunc::~C4AulFunc()
{
	// remove from list
	if (Prev) Prev->Next = Next;
	if (Next) Next->Prev = Prev;
	if (Owner)
	{
		if (Owner->Func0 == this) Owner->Func0 = Next;
		if (Owner->FuncL == this) Owner->FuncL = Prev;
		if (GetName())
			Owner->Engine->FuncLookUp.Remove(this);
		if (Owner->GetPropList() && Name)
		{
			C4Value v;
			Owner->GetPropList()->GetPropertyByS(Name, &v);
			assert(v.getFunction() != this);
		}
	}
}

StdStrBuf C4AulFunc::GetFullName()
{
	StdStrBuf r;
	// "lost" function?
	if (!Owner)
	{
		r.Ref("(unowned) ");
	}
	else if (Owner->GetPropList() && Owner->GetPropList()->IsStatic())
	{
		r.Take(Owner->GetPropList()->IsStatic()->GetDataString());
		r.AppendChar('.');
	}
	else if (Owner == &GameScript)
	{
		r.Ref("Scenario.");
	}
	else if (Owner->Engine == Owner)
	{
		r.Ref("Global.");
	}
	else
	{
		r.Ref("(unknown) ");
	}
	r.Append(Name->GetData());
	return r;
}
