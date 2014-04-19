/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001, 2007, Sven Eberhardt
 * Copyright (c) 2011-2013, The OpenClonk Team and contributors
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

#include <C4Include.h>
#include <C4Value.h>
#include <C4AulFunc.h>
#include <C4Aul.h>

C4AulFunc::C4AulFunc(C4AulScript *pOwner, const char *pName):
		iRefCnt(0),
		Name(pName ? Strings.RegString(pName) : 0),
		MapNext(NULL)
{
	Owner = pOwner;
	// add to global lookuptable with this name
	if (GetName())
		::ScriptEngine.FuncLookUp.Add(this);
}

C4AulFunc::~C4AulFunc()
{
	if (GetName())
		::ScriptEngine.FuncLookUp.Remove(this);
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

void C4AulFunc::CheckParTypes(const C4Value pPars[]) const {
	// Convert parameters (typecheck)
	const C4V_Type *pTypes = GetParType();
	int parcount = GetParCount();
	for (int i = 0; i < parcount; i++) {
		if (!pPars[i].CheckParConversion(pTypes[i]))
			throw new C4AulExecError(FormatString("call to \"%s\" parameter %d: passed %s, but expected %s",
			                                      GetName(), i + 1, pPars[i].GetTypeName(), GetC4VName(pTypes[i])
			                                     ).getData());
	}
}
