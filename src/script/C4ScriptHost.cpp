/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001-2002, 2005, 2007  Sven Eberhardt
 * Copyright (c) 2003-2005  Peter Wortmann
 * Copyright (c) 2006  Armin Burgmeier
 * Copyright (c) 2006-2007, 2009, 2011  Günther Brammer
 * Copyright (c) 2009  Nicolas Hake
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de
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

/* Handles script file components (calls, inheritance, function maps) */

#include <C4Include.h>
#include <C4ScriptHost.h>

#include <C4ObjectCom.h>
#include <C4Object.h>
#include <C4Game.h>
#include <C4GameObjects.h>

/*--- C4ScriptHost ---*/

C4ScriptHost::C4ScriptHost() { }
C4ScriptHost::~C4ScriptHost() { Clear(); }

void C4ScriptHost::Clear()
{
	C4AulScript::Clear();
	ComponentHost.Clear();
}

bool C4ScriptHost::Load(C4Group &hGroup, const char *szFilename,
                        const char *szLanguage, C4Def *pDef, class C4LangStringTable *pLocalTable)
{
	// Set definition and id
	Def = pDef;
	// Base load
	bool fSuccess = ComponentHost.Load(hGroup,szFilename,szLanguage);
	// String Table
	stringTable = pLocalTable;
	// set name
	ScriptName.Ref(ComponentHost.GetFilePath());
	// preparse script
	MakeScript();
	// Success
	return fSuccess;
}


void C4ScriptHost::MakeScript()
{
	// clear prev
	Script.Clear();

	// create script
	if (stringTable)
	{
		stringTable->ReplaceStrings(ComponentHost.GetDataBuf(), Script);
	}
	else
	{
		Script.Ref(ComponentHost.GetDataBuf());
	}

	// preparse script
	Preparse();
}

C4Value C4ScriptHost::Call(const char *szFunction, C4Object *pObj, C4AulParSet *Pars, bool fPrivateCall, bool fPassError)
{
	// get function
	C4AulScriptFunc *pFn;
	if (!(pFn = GetSFunc(szFunction, AA_PRIVATE))) return C4VNull;
	// Call code
	return pFn->Exec(pObj,Pars, fPassError);
}

bool C4ScriptHost::ReloadScript(const char *szPath, const char *szLanguage)
{
	// this?
	if (SEqualNoCase(szPath, ComponentHost.GetFilePath()) || (stringTable && SEqualNoCase(szPath, stringTable->GetFilePath())))
	{
		// try reload
		char szParentPath[_MAX_PATH + 1]; C4Group ParentGrp;
		if (GetParentPath(szPath, szParentPath))
			if (ParentGrp.Open(szParentPath))
				if (Load(ParentGrp, NULL, szLanguage, NULL, stringTable))
					return true;
	}
	// call for childs
	return C4AulScript::ReloadScript(szPath, szLanguage);
}

void C4ScriptHost::SetError(const char *szMessage)
{

}

/*--- C4DefScriptHost ---*/

void C4DefScriptHost::AfterLink()
{
	C4AulScript::AfterLink();
	// Search cached functions
	SFn_CalcValue       = GetSFunc(PSF_CalcValue      , AA_PROTECTED);
	SFn_SellTo          = GetSFunc(PSF_SellTo         , AA_PROTECTED);
	SFn_ControlTransfer = GetSFunc(PSF_ControlTransfer, AA_PROTECTED);
	if (Def)
	{
		C4AulAccess CallAccess = /*Strict ? AA_PROTECTED : */AA_PRIVATE;
		Def->TimerCall=GetSFuncWarn((const char *) Def->STimerCall, CallAccess, "TimerCall");
	}
}



/*--- C4GameScriptHost ---*/

C4GameScriptHost::C4GameScriptHost(): Counter(0), Go(false) { }
C4GameScriptHost::~C4GameScriptHost() { }

bool C4GameScriptHost::Execute(int iTick10)
{
	if (!Script) return false;
	if (Go && !iTick10)
	{
		return !! Call(FormatString(PSF_Script,Counter++).getData());
	}
	return false;
}

C4Value C4GameScriptHost::GRBroadcast(const char *szFunction, C4AulParSet *pPars, bool fPassError, bool fRejectTest)
{
	// call objects first - scenario script might overwrite hostility, etc...
	C4Value vResult = ::Objects.GRBroadcast(szFunction, pPars, fPassError, fRejectTest);
	// rejection tests abort on first nonzero result
	if (fRejectTest) if (!!vResult) return vResult;
	// scenario script call
	return Call(szFunction, 0, pPars, fPassError);
}

void C4GameScriptHost::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(Go,             "Go",                    false));
	pComp->Value(mkNamingAdapt(Counter,        "Counter",               0));
}

C4GameScriptHost GameScript;
