/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001, 2004, 2007-2008  Sven Eberhardt
 * Copyright (c) 2001, 2009  Peter Wortmann
 * Copyright (c) 2006-2009, 2011  Günther Brammer
 * Copyright (c) 2007  Matthes Bender
 * Copyright (c) 2009  Nicolas Hake
 * Copyright (c) 2010  Benjamin Herr
 * Copyright (c) 2010  Martin Plicht
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
// C4Aul script engine CP conversion

#include <C4Include.h>
#include <C4Aul.h>
#include <C4AulExec.h>
#include <C4AulDebug.h>

#include <C4Config.h>
#include <C4Def.h>
#include <C4Log.h>
#include <C4Components.h>
#include <C4Application.h>
#include <C4LangStringTable.h>

C4AulError::C4AulError(): shown(false) {}

void C4AulError::show()
{
	shown = true;
	// simply log error message
	if (sMessage)
		DebugLog(sMessage.getData());
}

C4AulFunc::C4AulFunc(C4AulScript *pOwner, const char *pName, bool bAtEnd):
		Name(pName ? Strings.RegString(pName) : 0),
		MapNext(NULL),
		LinkedTo (NULL),
		OverloadedBy (NULL)
{
	// reg2list (at end or at the beginning)
	Owner = pOwner;
	if (bAtEnd)
	{
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
	}
	else
	{
		if ((Next = Owner->Func0))
		{
			Next->Prev = this;
			Owner->Func0 = this;
		}
		else
		{
			Owner->Func0 = this;
			Owner->FuncL = this;
		}
		Prev = NULL;
	}
	// add to global lookuptable with this name
	if (GetName())
		Owner->Engine->FuncLookUp.Add(this, bAtEnd);
}


C4AulFunc::~C4AulFunc()
{
	// if it's a global: remove the global link!
	if (LinkedTo && Owner)
		if (LinkedTo->Owner == Owner->Engine)
		{
			if (!LinkedTo->OverloadedBy)
				LinkedTo->Owner->GetPropList()->ResetProperty(LinkedTo->Name);
			delete LinkedTo;
		}
	// unlink func
	if (LinkedTo)
	{
		// find prev
		C4AulFunc* pAkt = this;
		while (pAkt->LinkedTo != this) pAkt = pAkt->LinkedTo;
		if (pAkt == LinkedTo)
			pAkt->LinkedTo = NULL;
		else
			pAkt->LinkedTo = LinkedTo;
		LinkedTo = NULL;
	}
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

void C4AulFunc::DestroyLinked()
{
	// delete all functions linked to this one.
	while (LinkedTo)
		delete LinkedTo;
}

StdStrBuf C4AulFunc::GetFullName()
{
	// "lost" function?
	StdStrBuf sOwner;
	if (!Owner)
	{
		sOwner.Ref("(unknown) ");
	}
	else if (Owner->GetPropList() && Owner->GetPropList()->GetDef())
	{
		sOwner.Format("%s.", Owner->GetPropList()->GetDef()->id.ToString());
	}
	else if (Owner->Engine == Owner)
	{
		sOwner.Ref("global ");
	}
	else
	{
		sOwner.Ref("game ");
	}
	StdStrBuf sResult;
	sResult.Format("%s%s", sOwner.getData(), GetName());
	return sResult;
}

C4AulDefFunc::C4AulDefFunc(C4AulScript *pOwner, const char *pName, C4ScriptFnDef* pDef):
		C4AulFunc(pOwner, pName) // constructor
{
	Def = pDef;
	Owner->GetPropList()->SetPropertyByS(Name, C4VFunction(this));
}

C4AulDefFunc::~C4AulDefFunc()
{
	if (!OverloadedBy)
		Owner->GetPropList()->ResetProperty(Name);
}

C4AulScript::C4AulScript()
{
	// not compiled
	State = ASS_NONE;
	IncludesResolved = false;

	// defaults
	Strict = MAXSTRICT;
	Resolving=false;
	Temporary = false;
	LocalNamed.Reset();

	// prepare lists
	Child0 = ChildL = Prev = Next = NULL;
	Owner = Engine = NULL;
	Func0 = FuncL = NULL;
	// prepare include list
	Includes.clear();
	Appends.clear();
}

C4AulScript::~C4AulScript()
{
	// clear
	Clear();
	// unreg
	Unreg();
}


void C4AulScript::Unreg()
{
	// remove from list
	if (Prev) Prev->Next = Next; else if (Owner) Owner->Child0 = Next;
	if (Next) Next->Prev = Prev; else if (Owner) Owner->ChildL = Prev;
	Prev = Next = Owner = NULL;
}


void C4AulScript::Clear()
{
	// remove includes
	Includes.clear();
	Appends.clear();
	// delete child scripts + funcs
	while (Child0) // Child0->Unreg();
			if (Child0->Delete()) delete Child0; else Child0->Unreg();
	while (Func0) delete Func0;
	// reset flags
	State = ASS_NONE;
}


void C4AulScript::Reg2List(C4AulScriptEngine *pEngine, C4AulScript *pOwner)
{
	// already regged? (def reloaded)
	if (Owner) return;
	// reg to list
	Engine = pEngine;
	if ((Owner = pOwner))
	{
		if ((Prev = Owner->ChildL))
			Prev->Next = this;
		else
			Owner->Child0 = this;
		Owner->ChildL = this;
	}
	else
		Prev = NULL;
	Next = NULL;
}


C4AulFunc *C4AulScript::GetOverloadedFunc(C4AulFunc *ByFunc)
{
	assert(ByFunc);
	// search local list
	C4AulFunc *f = ByFunc;
	if (f) f = f->Prev; else f = FuncL;
	while (f)
	{
		if (SEqual(ByFunc->GetName(), f->GetName())) break;
		f = f->Prev;
	}
#ifdef _DEBUG
	C4AulFunc * f2 = Engine ? Engine->GetFunc(ByFunc->GetName(), this, ByFunc) : NULL;
	assert (f == f2);
#endif
	// nothing found? then search owner, if existant
	if (!f && Owner)
	{
		if ((f = Owner->GetFuncRecursive(ByFunc->GetName())))
			// just found  the global link?
			if (ByFunc && f->LinkedTo == ByFunc)
				f = Owner->GetOverloadedFunc(f);
	}
	// return found fn
	return f;
}

C4AulFunc *C4AulScript::GetFuncRecursive(const char *pIdtf)
{
	// search local list
	C4AulFunc *f = GetFunc(pIdtf);
	if (f) return f;
	// nothing found? then search owner, if existant
	else if (Owner) return Owner->GetFuncRecursive(pIdtf);
	return NULL;
}

C4AulFunc *C4AulScript::GetFunc(const char *pIdtf)
{
	C4AulFunc * f = Engine ? Engine->GetFunc(pIdtf, this, NULL) : NULL;
#if 0
	// search func list
	C4AulFunc *f2 = FuncL;
	while (f2)
	{
		if (SEqual(pIdtf, f2->Name)) break;
		f2 = f2->Prev;
	}
	assert (f == f2);
#endif
	return f;
}


C4AulScriptFunc *C4AulScript::GetSFunc(const char *pIdtf, C4AulAccess AccNeeded, bool fFailsafe)
{
	// failsafe call
	if (*pIdtf=='~') { fFailsafe=true; pIdtf++; }

	// get function reference from table
	C4AulScriptFunc *pFn = GetSFunc(pIdtf);

	// undefined function
	if (!pFn)
	{
		// not failsafe?
		if (!fFailsafe)
		{
			// show error
			C4AulParseError err(this, "Undefined function: ", pIdtf);
			err.show();
		}
		return NULL;
	}

	// check access
	if (pFn->Access < AccNeeded)
	{
		// no access? show error
		C4AulParseError err(this, "insufficient access level");
		err.show();
		// don't even break in strict execution, because the caller might be non-strict
		//if (Strict) return NULL;
	}

	// return found function
	return pFn;
}

C4AulScriptFunc *C4AulScript::GetSFunc(const char *pIdtf)
{
	// get func by name; return script func
	if (!pIdtf) return NULL;
	if (!pIdtf[0]) return NULL;
	if (pIdtf[0] == '~') pIdtf++;
	C4AulFunc *f = GetFunc(pIdtf);
	if (!f) return NULL;
	return f->SFunc();
}

std::string C4AulScript::Translate(const std::string &text) const
{
	const C4AulScript *cursor = this;
	while (cursor)
	{
		try
		{
			if (cursor->stringTable)
				return cursor->stringTable->Translate(text);
		}
		catch (C4LangStringTable::NoSuchTranslation &)
		{
			// Ignore, soldier on
		}
		// Walk tree structure upwards
		cursor = cursor->Owner;
	}
	throw C4LangStringTable::NoSuchTranslation(text);
}

C4AulScriptFunc::C4AulScriptFunc(C4AulScript *pOwner, const char *pName, bool bAtEnd):
		C4AulFunc(pOwner, pName, bAtEnd), OwnerOverloaded(NULL), ParCount(0), tProfileTime(0)
{
	for (int i = 0; i < C4AUL_MAX_Par; i++) ParType[i] = C4V_Any;
	if (Owner->GetPropList())
	{
		// Don't overwrite a function that's overloading this one
		if (bAtEnd || !Owner->GetPropList()->HasProperty(Name))
			Owner->GetPropList()->SetPropertyByS(Name, C4VFunction(this));
	}
}

void C4AulScriptFunc::CopyBody(C4AulScriptFunc &FromFunc)
{
	// copy some members, that are set before linking
	Access = FromFunc.Access;
	Script = FromFunc.Script;
	VarNamed = FromFunc.VarNamed;
	ParNamed = FromFunc.ParNamed;
	ParCount = FromFunc.ParCount;
	pOrgScript = FromFunc.pOrgScript;
	for (int i = 0; i < C4AUL_MAX_Par; i++)
		ParType[i] = FromFunc.ParType[i];
}


void C4AulScript::AddFunc(const char *pIdtf, C4ScriptFnDef* Def)
{
	// create def func
	new C4AulDefFunc(this, pIdtf, Def);
}


/*--- C4AulScriptEngine ---*/

C4AulScriptEngine::C4AulScriptEngine():
		GlobalPropList(0), warnCnt(0), errCnt(0), nonStrictCnt(0), lineCnt(0)
{
	// /me r b engine
	Engine = this;
	ScriptName.Ref(C4CFN_System);
	Strict = MAXSTRICT;

	GlobalNamedNames.Reset();
	GlobalNamed.Reset();
	GlobalNamed.SetNameList(&GlobalNamedNames);
	GlobalConstNames.Reset();
	GlobalConsts.Reset();
	GlobalConsts.SetNameList(&GlobalConstNames);
}

C4PropList * C4AulScriptEngine::GetPropList()
{
	if (!GlobalPropList)
	{
		GlobalPropList = C4PropList::NewScen();
		RegisterGlobalConstant("Global", C4VPropList(GlobalPropList));
	}
	return GlobalPropList;
}

C4AulScriptEngine::~C4AulScriptEngine() { Clear(); }

void C4AulScriptEngine::Clear()
{
#ifndef NOAULDEBUG
	// stop debugger
	delete C4AulDebug::GetDebugger();
#endif
	// clear inherited
	C4AulScript::Clear();
	// clear own stuff
	GetPropList()->Clear();
	// reset values
	warnCnt = errCnt = nonStrictCnt = lineCnt = 0;
	// resetting name lists will reset all data lists, too
	// except not...
	GlobalNamedNames.Reset();
	GlobalConstNames.Reset();
	GlobalConsts.Reset();
	GlobalConsts.SetNameList(&GlobalConstNames);
	GlobalNamed.Reset();
	GlobalNamed.SetNameList(&GlobalNamedNames);
}


void C4AulScriptEngine::UnLink()
{
	// unlink scripts
	C4AulScript::UnLink();
	// Do not clear global variables and constants, because they are registered by the
	// preparser or other parts. Note that keeping those fields means that you cannot delete a global
	// variable or constant at runtime by removing it from the script.
	//GlobalNamedNames.Reset();
	//GlobalConstNames.Reset();
}


void C4AulScriptEngine::RegisterGlobalConstant(const char *szName, const C4Value &rValue)
{
	// Register name and set value.
	// AddName returns the index of existing element if the name is assigned already.
	// That is OK, since it will only change the value of the global ("overload" it).
	// A warning would be nice here. However, this warning would show up whenever a script
	// containing globals constants is recompiled.
	GlobalConsts[GlobalConstNames.AddName(szName)] = rValue;
}

bool C4AulScriptEngine::GetGlobalConstant(const char *szName, C4Value *pTargetValue)
{
	// get index of global by name
	int32_t iConstIndex = GlobalConstNames.GetItemNr(szName);
	// not found?
	if (iConstIndex<0) return false;
	// if it's found, assign the value if desired
	if (pTargetValue) *pTargetValue = GlobalConsts[iConstIndex];
	// constant exists
	return true;
}

bool C4AulScriptEngine::Denumerate(C4ValueNumbers * numbers)
{
	GlobalNamed.Denumerate(numbers);
	// runtime data only: don't denumerate consts
	GameScript.ScenPropList->Denumerate(numbers);
	return true;
}

void C4AulScriptEngine::CompileFunc(StdCompiler *pComp, C4ValueNumbers * numbers)
{
	C4ValueMapData GlobalNamedDefault;
	GlobalNamedDefault.SetNameList(&GlobalNamedNames);
	pComp->Value(mkNamingAdapt(mkParAdapt(GlobalNamed, numbers), "StaticVariables", GlobalNamedDefault));
	pComp->Value(mkNamingAdapt(mkParAdapt(*GameScript.ScenPropList, numbers), "Scenario"));
}

std::list<const char*> C4AulScriptEngine::GetFunctionNames(C4AulScript * script)
{
	std::list<const char*> functions;
	for (C4AulFunc *pFn = Func0; pFn; pFn = pFn->Next)
	{
		if (pFn->GetPublic())
		{
			functions.push_back(pFn->GetName());
		}
	}
	// Add object or scenario script functions
	if (script)
	{
		bool divider = false;
		C4AulFunc *f = script->FuncL;
		C4AulScriptFunc *pRef;
		// Scan all functions
		while (f)
		{
			if ((pRef = f->SFunc()))
			{
				// Public functions only
				if (pRef->Access == AA_PUBLIC)
				{
					// Insert divider if necessary
					if (!divider)
						functions.push_back(0);
					divider = true;
					// Add function
					functions.push_back(pRef->GetName());
				}
			}
			f = f->Prev;
		}
	}
	return functions;
}

/*--- C4AulFuncMap ---*/
static const size_t CapacityInc = 1024;

C4AulFuncMap::C4AulFuncMap(): Funcs(new C4AulFunc*[CapacityInc]), FuncCnt(0), Capacity(CapacityInc)
{
	memset(Funcs, 0, sizeof (C4AulFunc *) * Capacity);
}

C4AulFuncMap::~C4AulFuncMap()
{
	delete[] Funcs;
}

unsigned int C4AulFuncMap::Hash(const char * name)
{
	// Fowler/Noll/Vo hash
	unsigned int h = 2166136261u;
	while (*name)
		h = (h ^ *(name++)) * 16777619;
	return h;
}

C4AulFunc * C4AulFuncMap::GetFirstFunc(const char * Name)
{
	if (!Name) return NULL;
	C4AulFunc * Func = Funcs[Hash(Name) % Capacity];
	while (Func && !SEqual(Name, Func->GetName()))
		Func = Func->MapNext;
	return Func;
}

C4AulFunc * C4AulFuncMap::GetNextSNFunc(const C4AulFunc * After)
{
	C4AulFunc * Func = After->MapNext;
	while (Func && !SEqual(After->GetName(), Func->GetName()))
		Func = Func->MapNext;
	return Func;
}

C4AulFunc * C4AulFuncMap::GetFunc(const char * Name, const C4AulScript * Owner, const C4AulFunc * After)
{
	if (!Name) return NULL;
	C4AulFunc * Func = Funcs[Hash(Name) % Capacity];
	if (After)
	{
		while (Func && Func != After)
			Func = Func->MapNext;
		if (Func)
			Func = Func->MapNext;
	}
	while (Func && (Func->Owner != Owner || !SEqual(Name, Func->GetName())))
		Func = Func->MapNext;
	return Func;
}

void C4AulFuncMap::Add(C4AulFunc * func, bool bAtStart)
{
	if (++FuncCnt > Capacity)
	{
		int NCapacity = Capacity + CapacityInc;
		C4AulFunc ** NFuncs = new C4AulFunc*[NCapacity];
		memset(NFuncs, 0, sizeof (C4AulFunc *) * NCapacity);
		for (int i = 0; i < Capacity; ++i)
		{
			while (Funcs[i])
			{
				// Get a pointer to the bucket
				C4AulFunc ** pNFunc = &(NFuncs[Hash(Funcs[i]->GetName()) % NCapacity]);
				// get a pointer to the end of the linked list
				while (*pNFunc) pNFunc = &((*pNFunc)->MapNext);
				// Move the func over
				*pNFunc = Funcs[i];
				// proceed with the next list member
				Funcs[i] = Funcs[i]->MapNext;
				// Terminate the linked list
				(*pNFunc)->MapNext = 0;
			}
		}
		Capacity = NCapacity;
		delete [] Funcs;
		Funcs = NFuncs;
	}
	// Get a pointer to the bucket
	C4AulFunc ** pFunc = &(Funcs[Hash(func->GetName()) % Capacity]);
	if (bAtStart)
	{
		// move the current first to the second position
		func->MapNext = *pFunc;
	}
	else
	{
		// get a pointer to the end of the linked list
		while (*pFunc)
		{
			pFunc = &((*pFunc)->MapNext);
		}
	}
	// Add the func
	*pFunc = func;
}

void C4AulFuncMap::Remove(C4AulFunc * func)
{
	C4AulFunc ** pFunc = &Funcs[Hash(func->GetName()) % Capacity];
	while (*pFunc != func)
	{
		pFunc = &((*pFunc)->MapNext);
		assert(*pFunc); // crash on remove of a not contained func
	}
	*pFunc = (*pFunc)->MapNext;
	--FuncCnt;
}

C4AulScriptEngine ScriptEngine;
