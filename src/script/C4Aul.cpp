/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2016, The OpenClonk Team and contributors
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
// Miscellaneous script engine bits

#include "C4Include.h"
#include "script/C4Aul.h"
#include "script/C4AulExec.h"
#include "script/C4AulDebug.h"
#include "config/C4Config.h"
#include "object/C4Def.h"
#include "script/C4Effect.h"
#include "lib/C4Log.h"
#include "c4group/C4Components.h"
#include "c4group/C4LangStringTable.h"

C4AulError::C4AulError(): shown(false) {}

void C4AulError::show()
{
	shown = true;
	// simply log error message
	if (sMessage)
		DebugLog(sMessage.getData());
}

const char *C4AulError::what() const noexcept
{
	return sMessage ? sMessage.getData() : "(unknown error)";
}

/*--- C4AulScriptEngine ---*/

C4AulScriptEngine::C4AulScriptEngine():
		C4PropListStaticMember(NULL, NULL, ::Strings.RegString("Global")),
		warnCnt(0), errCnt(0), lineCnt(0)
{
	GlobalNamedNames.Reset();
	GlobalNamed.Reset();
	GlobalNamed.SetNameList(&GlobalNamedNames);
	GlobalConstNames.Reset();
	GlobalConsts.Reset();
	GlobalConsts.SetNameList(&GlobalConstNames);
	Child0 = ChildL = NULL;
	RegisterGlobalConstant("Global", C4VPropList(this));
}

C4AulScriptEngine::~C4AulScriptEngine()
{
	Clear();
}

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
	C4PropListStaticMember::Clear();
	// reset values
	warnCnt = errCnt = lineCnt = 0;
	// resetting name lists will reset all data lists, too
	// except not...
	GlobalNamedNames.Reset();
	GlobalConstNames.Reset();
	GlobalConsts.Reset();
	GlobalConsts.SetNameList(&GlobalConstNames);
	RegisterGlobalConstant("Global", C4VPropList(this));
	GlobalNamed.Reset();
	GlobalNamed.SetNameList(&GlobalNamedNames);
	delete pGlobalEffects; pGlobalEffects=NULL;
	UserFiles.clear();
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

void C4AulScriptEngine::Denumerate(C4ValueNumbers * numbers)
{
	GlobalNamed.Denumerate(numbers);
	// runtime data only: don't denumerate consts
	GameScript.Denumerate(numbers);
	C4PropListStaticMember::Denumerate(numbers);
	if (pGlobalEffects) pGlobalEffects->Denumerate(numbers);
}

static void GlobalEffectsMergeCompileFunc(StdCompiler *pComp, C4Effect * & pEffects, const char * name, C4PropList * pForObj, C4ValueNumbers * numbers)
{
	C4Effect *pOldEffect, *pNextOldEffect=pEffects;
	pEffects = NULL;
	try
	{
		pComp->Value(mkParAdapt(mkNamingPtrAdapt(pEffects, name), pForObj, numbers));
	}
	catch (...)
	{
		delete pNextOldEffect;
		throw;
	}
	while ((pOldEffect=pNextOldEffect))
	{
		pNextOldEffect = pOldEffect->pNext;
		pOldEffect->Register(&pEffects, Abs(pOldEffect->iPriority));
	}
}

void C4AulScriptEngine::CompileFunc(StdCompiler *pComp, bool fScenarioSection, C4ValueNumbers * numbers)
{
	if (!fScenarioSection)
	{
		assert(UserFiles.empty()); // user files must not be kept open
		C4ValueMapData GlobalNamedDefault;
		GlobalNamedDefault.SetNameList(&GlobalNamedNames);
		pComp->Value(mkNamingAdapt(mkParAdapt(GlobalNamed, numbers), "StaticVariables", GlobalNamedDefault));
		pComp->Value(mkNamingAdapt(mkParAdapt(*GameScript.ScenPropList._getPropList(), numbers), "Scenario"));
	}
	if (fScenarioSection && pComp->isCompiler())
	{
		// loading scenario section: Merge effects
		// Must keep old effects here even if they're dead, because the LoadScenarioSection call typically came from execution of a global effect
		// and otherwise dead pointers would remain on the stack
		GlobalEffectsMergeCompileFunc(pComp, pGlobalEffects, "Effects", this, numbers);
		GlobalEffectsMergeCompileFunc(pComp, GameScript.pScenarioEffects, "ScenarioEffects", GameScript.ScenPropList._getPropList(), numbers);
	}
	else
	{
		// Otherwise, just compile effects
		pComp->Value(mkParAdapt(mkNamingPtrAdapt(pGlobalEffects, "Effects"), this, numbers));
		pComp->Value(mkParAdapt(mkNamingPtrAdapt(GameScript.pScenarioEffects, "ScenarioEffects"), GameScript.ScenPropList._getPropList(), numbers));
	}
	pComp->Value(mkNamingAdapt(*numbers, "Values"));
}

std::list<const char*> C4AulScriptEngine::GetFunctionNames(C4PropList * p)
{
	std::list<const char*> functions;
	std::list<const char*> global_functions;
	auto sort_alpha = [](const char * const &a, const char * const &b) -> bool { return strcmp(a, b) < 0; };
	if (!p) p = GetPropList();
	const C4ValueArray * a = p->GetProperties();
	for (int i = 0; i < a->GetSize(); ++i)
	{
		C4String * key = (*a)[i].getStr();
		if (!key) continue;
		C4AulFunc * f = p->GetFunc(key);
		if (!f) continue;
		if (!f->GetPublic()) continue;
		if (!::ScriptEngine.GetPropList()->HasProperty(key))
			functions.push_back(key->GetCStr());
		else
			global_functions.push_back(key->GetCStr());
	}
	delete a;
	functions.sort(sort_alpha);
	if (!functions.empty() && !global_functions.empty()) functions.push_back(0); // separator
	global_functions.sort(sort_alpha);
	functions.splice(functions.end(), global_functions);
	return functions;
}

int32_t C4AulScriptEngine::CreateUserFile()
{
	// create new file and return handle
	// find empty handle
	int32_t last_handle = 1;
	for (std::list<C4AulUserFile>::const_iterator i = UserFiles.begin(); i != UserFiles.end(); ++i)
		if ((*i).GetHandle() >= last_handle)
			last_handle = (*i).GetHandle()+1;
	// Create new user file
	UserFiles.push_back(C4AulUserFile(last_handle));
	return last_handle;
}

void C4AulScriptEngine::CloseUserFile(int32_t handle)
{
	// close user file given by handle
	for (std::list<C4AulUserFile>::iterator i = UserFiles.begin(); i != UserFiles.end(); ++i)
		if ((*i).GetHandle() == handle)
		{
			UserFiles.erase(i);
			break;
		}
}

C4AulUserFile *C4AulScriptEngine::GetUserFile(int32_t handle)
{
	// get user file given by handle
	for (std::list<C4AulUserFile>::iterator i = UserFiles.begin(); i != UserFiles.end(); ++i)
		if ((*i).GetHandle() == handle)
		{
			return &*i;
		}
	// not found
	return NULL;
}

/*--- C4AulFuncMap ---*/

C4AulFuncMap::C4AulFuncMap(): FuncCnt(0)
{
	memset(Funcs, 0, sizeof (C4AulFunc *) * HashSize);
}

C4AulFuncMap::~C4AulFuncMap()
{
	assert(!FuncCnt);
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
	C4AulFunc * Func = Funcs[Hash(Name) % HashSize];
	while (Func && !SEqual(Name, Func->GetName()))
		Func = Func->MapNext;
	return Func;
}

C4AulFunc * C4AulFuncMap::GetNextSNFunc(const C4AulFunc * After)
{
	C4AulFunc * Func = After->MapNext;
	while (Func && After->GetName() != Func->GetName())
		Func = Func->MapNext;
	return Func;
}

void C4AulFuncMap::Add(C4AulFunc * func)
{
	++FuncCnt;
	// Get a pointer to the bucket
	C4AulFunc ** pFunc = &(Funcs[Hash(func->GetName()) % HashSize]);
	// move the current first to the second position
	func->MapNext = *pFunc;
	// Add the func
	*pFunc = func;
}

void C4AulFuncMap::Remove(C4AulFunc * func)
{
	C4AulFunc ** pFunc = &Funcs[Hash(func->GetName()) % HashSize];
	while (*pFunc != func)
	{
		pFunc = &((*pFunc)->MapNext);
		assert(*pFunc); // crash on remove of a not contained func
	}
	*pFunc = (*pFunc)->MapNext;
	--FuncCnt;
}
