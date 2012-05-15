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
#include <C4LangStringTable.h>

C4AulError::C4AulError(): shown(false) {}

void C4AulError::show()
{
	shown = true;
	// simply log error message
	if (sMessage)
		DebugLog(sMessage.getData());
}

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

C4AulDefFunc::C4AulDefFunc(C4AulScript *pOwner, const char *pName, C4ScriptFnDef* pDef):
		C4AulFunc(pOwner, pName) // constructor
{
	Def = pDef;
	Owner->GetPropList()->SetPropertyByS(Name, C4VFunction(this));
}

C4AulDefFunc::~C4AulDefFunc()
{
	assert(!Owner);
}

C4AulScript::C4AulScript()
{
	// not compiled
	State = ASS_NONE;
	IncludesResolved = false;

	// defaults
	Resolving=false;
	Temporary = false;
	LocalNamed.Reset();

	// prepare lists
	Engine = NULL;
	Func0 = FuncL = NULL;
	// prepare include list
	Includes.clear();
	Appends.clear();
}

C4AulScript::~C4AulScript()
{
	// clear
	Clear();
}


void C4ScriptHost::Unreg()
{
	// remove from list
	if (Prev) Prev->Next = Next; else if (Engine) Engine->Child0 = Next;
	if (Next) Next->Prev = Prev; else if (Engine) Engine->ChildL = Prev;
	Prev = Next = NULL;
	Engine = NULL;
}


void C4AulScript::Clear()
{
	// remove includes
	Includes.clear();
	Appends.clear();
	while (Func0)
	{
		C4AulFunc * f = Func0;
		f->RemoveFromScript();
		f->DecRef(); // see C4AulFunc::C4AulFunc
	}
	// reset flags
	State = ASS_NONE;
}


void C4ScriptHost::Reg2List(C4AulScriptEngine *pEngine)
{
	// already regged? (def reloaded)
	if (Engine) return;
	// reg to list
	if ((Engine = pEngine))
	{
		if ((Prev = Engine->ChildL))
			Prev->Next = this;
		else
			Engine->Child0 = this;
		Engine->ChildL = this;
	}
	else
		Prev = NULL;
	Next = NULL;
}

std::string C4AulScript::Translate(const std::string &text) const
{
	try
	{
		if (stringTable)
			return stringTable->Translate(text);
	}
	catch (C4LangStringTable::NoSuchTranslation &)
	{
		// Ignore, soldier on
	}
	if (Engine && Engine != this)
		return Engine->Translate(text);
	throw C4LangStringTable::NoSuchTranslation(text);
}

C4AulScriptFunc::C4AulScriptFunc(C4AulScript *pOwner, C4ScriptHost *pOrgScript, const char *pName, const char *Script):
		C4AulFunc(pOwner, pName),
		CodePos(0),
		Script(Script),
		OwnerOverloaded(NULL),
		ParCount(0),
		pOrgScript(pOrgScript),
		tProfileTime(0)
{
	for (int i = 0; i < C4AUL_MAX_Par; i++) ParType[i] = C4V_Any;
}

C4AulScriptFunc::C4AulScriptFunc(C4AulScript *pOwner, const C4AulScriptFunc &FromFunc):
		C4AulFunc(pOwner, FromFunc.GetName()),
		CodePos(0),
		Script(FromFunc.Script),
		VarNamed(FromFunc.VarNamed),
		ParNamed(FromFunc.ParNamed),
		OwnerOverloaded(NULL),
		ParCount(FromFunc.ParCount),
		pOrgScript(FromFunc.pOrgScript),
		tProfileTime(0)
{
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
		GlobalPropList(0), warnCnt(0), errCnt(0), lineCnt(0)
{
	// /me r b engine
	Engine = this;
	ScriptName.Ref(C4CFN_System);

	GlobalNamedNames.Reset();
	GlobalNamed.Reset();
	GlobalNamed.SetNameList(&GlobalNamedNames);
	GlobalConstNames.Reset();
	GlobalConsts.Reset();
	GlobalConsts.SetNameList(&GlobalConstNames);
	Child0 = ChildL = NULL;
}

C4PropList * C4AulScriptEngine::GetPropList()
{
	if (!GlobalPropList)
	{
		GlobalPropList = C4PropList::NewAnon(NULL, NULL, ::Strings.RegString("Global"));
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
	while (Child0)
		if (Child0->Delete()) delete Child0;
		else Child0->Unreg();
	// clear own stuff
	if (GlobalPropList)
		GlobalPropList->Clear();
	// clear inherited
	C4AulScript::Clear();
	// reset values
	warnCnt = errCnt = lineCnt = 0;
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
	for (C4ScriptHost *s = Child0; s; s = s->Next)
		s->UnLink();
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
	GameScript.ScenPropList.Denumerate(numbers);
	return true;
}

void C4AulScriptEngine::CompileFunc(StdCompiler *pComp, C4ValueNumbers * numbers)
{
	C4ValueMapData GlobalNamedDefault;
	GlobalNamedDefault.SetNameList(&GlobalNamedNames);
	pComp->Value(mkNamingAdapt(mkParAdapt(GlobalNamed, numbers), "StaticVariables", GlobalNamedDefault));
	pComp->Value(mkNamingAdapt(mkParAdapt(*GameScript.ScenPropList._getPropList(), numbers), "Scenario"));
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
				// Insert divider if necessary
				if (!divider)
					functions.push_back(0);
				divider = true;
				// Add function
				functions.push_back(pRef->GetName());
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
