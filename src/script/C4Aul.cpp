/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2013, The OpenClonk Team and contributors
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

C4AulScript::C4AulScript()
{
	// not compiled
	State = ASS_NONE;

	// prepare lists
	Prev = Next = NULL;
	Engine = NULL;
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
	if (Prev) Prev->Next = Next; else if (Engine) Engine->Child0 = Next;
	if (Next) Next->Prev = Prev; else if (Engine) Engine->ChildL = Prev;
	Prev = Next = NULL;
	Engine = NULL;
}


void C4AulScript::Clear()
{
	// reset flags
	State = ASS_NONE;
}


void C4AulScript::Reg2List(C4AulScriptEngine *pEngine)
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
		OwnerOverloaded(NULL),
		ParCount(0),
		Script(Script),
		pOrgScript(pOrgScript),
		tProfileTime(0)
{
	for (int i = 0; i < C4AUL_MAX_Par; i++) ParType[i] = C4V_Any;
	AddBCC(AB_EOFN);
}

C4AulScriptFunc::C4AulScriptFunc(C4AulScript *pOwner, const C4AulScriptFunc &FromFunc):
		C4AulFunc(pOwner, FromFunc.GetName()),
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

/*--- C4AulScriptEngine ---*/

C4AulScriptEngine::C4AulScriptEngine():
		GlobalPropList(C4PropList::NewStatic(NULL, NULL, ::Strings.RegString("Global"))),
		warnCnt(0), errCnt(0), lineCnt(0)
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
	RegisterGlobalConstant("Global", GlobalPropList);
}

C4PropListStatic * C4AulScriptEngine::GetPropList()
{
	return GlobalPropList._getPropList()->IsStatic();
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
	GlobalPropList._getPropList()->Clear();
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
	RegisterGlobalConstant("Global", GlobalPropList);
	GlobalNamed.Reset();
	GlobalNamed.SetNameList(&GlobalNamedNames);
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

bool C4AulScriptEngine::Denumerate(C4ValueNumbers * numbers)
{
	GlobalNamed.Denumerate(numbers);
	// runtime data only: don't denumerate consts
	GameScript.ScenPropList.Denumerate(numbers);
	return true;
}

void C4AulScriptEngine::CompileFunc(StdCompiler *pComp, C4ValueNumbers * numbers)
{
	assert(UserFiles.empty()); // user files must not be kept open
	C4ValueMapData GlobalNamedDefault;
	GlobalNamedDefault.SetNameList(&GlobalNamedNames);
	pComp->Value(mkNamingAdapt(mkParAdapt(GlobalNamed, numbers), "StaticVariables", GlobalNamedDefault));
	pComp->Value(mkNamingAdapt(mkParAdapt(*GameScript.ScenPropList._getPropList(), numbers), "Scenario"));
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

C4AulFunc * C4AulFuncMap::GetFirstFunc(C4String * Name)
{
	if (!Name) return NULL;
	C4AulFunc * Func = Funcs[Hash(Name->GetCStr()) % HashSize];
	while (Func && Name->GetCStr() != Func->GetName())
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
