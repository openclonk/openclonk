/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2009-2014, The OpenClonk Team and contributors
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
#include <C4AulScriptFunc.h>

#include <C4AulExec.h>
#include <C4ScriptHost.h>

C4AulScriptFunc::C4AulScriptFunc(C4PropListStatic * Parent, C4ScriptHost *pOrgScript, const char *pName, const char *Script):
		C4AulFunc(Parent, pName),
		OwnerOverloaded(NULL),
		ParCount(0),
		Script(Script),
		pOrgScript(pOrgScript),
		tProfileTime(0)
{
	for (int i = 0; i < C4AUL_MAX_Par; i++) ParType[i] = C4V_Any;
	AddBCC(AB_EOFN);
}

C4AulScriptFunc::C4AulScriptFunc(C4PropListStatic * Parent, const C4AulScriptFunc &FromFunc):
		C4AulFunc(Parent, FromFunc.GetName()),
		OwnerOverloaded(NULL),
		ParCount(FromFunc.ParCount),
		Script(FromFunc.Script),
		VarNamed(FromFunc.VarNamed),
		ParNamed(FromFunc.ParNamed),
		pOrgScript(FromFunc.pOrgScript),
		tProfileTime(0)
{
	for (int i = 0; i < C4AUL_MAX_Par; i++)
		ParType[i] = FromFunc.ParType[i];
	AddBCC(AB_EOFN);
}

C4AulScriptFunc::~C4AulScriptFunc()
{
	if (OwnerOverloaded) OwnerOverloaded->DecRef();
	ClearCode();
}

void C4AulScriptFunc::SetOverloaded(C4AulFunc * f)
{
	if (OwnerOverloaded) OwnerOverloaded->DecRef();
	OwnerOverloaded = f;
	if (f) f->IncRef();
}

void C4AulScriptFunc::AddBCC(C4AulBCCType eType, intptr_t X, const char * SPos)
{
	// store chunk
	C4AulBCC bcc;
	bcc.bccType = eType;
	bcc.Par.X = X;
	Code.push_back(bcc);
	PosForCode.push_back(SPos);

	switch (eType)
	{
	case AB_STRING: case AB_CALL: case AB_CALLFS: case AB_LOCALN: case AB_PROP:
	/* case AB_LOCALN_SET/AB_PROP_SET: -- expected to already have a reference upon creation, see MakeSetter */
		bcc.Par.s->IncRef();
		break;
	case AB_CARRAY:
		bcc.Par.a->IncRef();
		break;
	default: break;
	}
}

void C4AulScriptFunc::RemoveLastBCC()
{
	C4AulBCC *pBCC = &Code.back();
	switch (pBCC->bccType)
	{
	case AB_STRING: case AB_CALL: case AB_CALLFS: case AB_LOCALN: case AB_LOCALN_SET: case AB_PROP: case AB_PROP_SET:
		pBCC->Par.s->DecRef();
		break;
	case AB_CARRAY:
		pBCC->Par.a->DecRef();
		break;
	default: break;
	}
	Code.pop_back();
	PosForCode.pop_back();
}

void C4AulScriptFunc::ClearCode()
{
	while(Code.size() > 0)
		RemoveLastBCC();
	// This function is now broken until an AddBCC call
}

int C4AulScriptFunc::GetLineOfCode(C4AulBCC * bcc)
{
	return SGetLine(pOrgScript ? pOrgScript->GetScript() : Script, PosForCode[bcc - &Code[0]]);
}

C4AulBCC * C4AulScriptFunc::GetCode()
{
	assert(!Code.empty());
	return &Code[0];
}

C4Value C4AulScriptFunc::Exec(C4PropList * p, C4Value pPars[], bool fPassErrors)
{
	return AulExec.Exec(this, p, pPars, fPassErrors);
}
