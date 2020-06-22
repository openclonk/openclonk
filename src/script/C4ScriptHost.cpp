/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
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

/* Handles script file components (calls, inheritance, function maps) */

#include "C4Include.h"
#include "script/C4ScriptHost.h"

#include "object/C4Def.h"
#include "script/C4AulCompiler.h"
#include "script/C4AulParse.h"
#include "script/C4AulScriptFunc.h"
#include "script/C4AulScriptFunc.h"
#include "script/C4Effect.h"

/*--- C4ScriptHost ---*/

C4ScriptHost::C4ScriptHost()
{
	Script = nullptr;
	stringTable = nullptr;
	SourceScripts.push_back(this);
	// prepare include list
	IncludesResolved = false;
	Resolving=false;
	Includes.clear();
	Appends.clear();
}
C4ScriptHost::~C4ScriptHost()
{
	Unreg();
	Clear();
}

void C4ScriptHost::Clear()
{
	UnlinkOwnedFunctions();
	C4ComponentHost::Clear();
	ast.reset();
	Script.Clear();
	LocalValues.Clear();
	DeleteOwnedPropLists();
	SourceScripts.clear();
	SourceScripts.push_back(this);
	if (stringTable)
	{
		stringTable->DelRef();
		stringTable = nullptr;
	}
	// remove includes
	Includes.clear();
	Appends.clear();
	// reset flags
	State = ASS_NONE;
	enabledWarnings.clear();
}

void C4ScriptHost::UnlinkOwnedFunctions()
{
	// Remove owned functions from their parents. This solves a problem
	// where overloading a definition would unload the C4ScriptHost, but
	// keep around global functions, which then contained dangling pointers.
	for (const auto &box : ownedFunctions)
	{
		assert(box.GetType() == C4V_Function);
		C4AulScriptFunc *func = box._getFunction()->SFunc();
		C4PropList *parent = func->Parent;
		if (parent == GetPropList())
			continue;
		assert(parent == &::ScriptEngine);
		C4Value v;
		parent->GetPropertyByS(func->Name, &v);
		if (v.getFunction() == func)
		{
			// If the function we're deleting is the top-level function in
			// the inheritance chain, promote the next one in its stead;
			// if there is no overloaded function, remove the property.
			parent->Thaw();
			if (func->OwnerOverloaded)
				parent->SetPropertyByS(func->Name, C4VFunction(func->OwnerOverloaded));
			else
				parent->ResetProperty(func->Name);
		}
		else
		{
			C4AulScriptFunc *func_chain = v.getFunction()->SFunc();
			assert(func_chain != func);
			while (func_chain)
			{
				// Unlink the removed function from the inheritance chain
				if (func_chain->OwnerOverloaded == func)
				{
					func_chain->OwnerOverloaded = func->OwnerOverloaded;
					func->OwnerOverloaded = nullptr; // func_chain now takes care of this.
					func->DecRef(); // decrease rc because func_chain no longer has a reference to func.
					break;
				}
				assert(func_chain->OwnerOverloaded && "Removed function not found in inheritance chain");
				func_chain = func_chain->OwnerOverloaded->SFunc();
			}
		}
	}
	ownedFunctions.clear();
}

void C4ScriptHost::DeleteOwnedPropLists()
{
	// delete all static proplists associated to this script host.
	// Note that just clearing the vector is not enough in case of
	// cyclic references.
	for (C4Value& value: ownedPropLists)
	{
		C4PropList* plist = value.getPropList();
		if (plist)
		{
			if (plist->Delete()) delete plist;
			else plist->Clear();
		}
	}
	ownedPropLists.clear();
}

void C4ScriptHost::Unreg()
{
	// remove from list
	if (Prev) Prev->Next = Next; else if (Engine) Engine->Child0 = Next;
	if (Next) Next->Prev = Prev; else if (Engine) Engine->ChildL = Prev;
	Prev = Next = nullptr;
	Engine = nullptr;
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
		Prev = nullptr;
	Next = nullptr;
}

bool C4ScriptHost::Load(C4Group &hGroup, const char *szFilename,
                        const char *szLanguage, class C4LangStringTable *pLocalTable)
{
	// Base load
	bool fSuccess = C4ComponentHost::Load(hGroup,szFilename,szLanguage);
	// String Table
	if (stringTable != pLocalTable)
	{
		if (stringTable) stringTable->DelRef();
		stringTable = pLocalTable;
		if (stringTable) stringTable->AddRef();
	}
	// set name
	ScriptName.Ref(GetFilePath());
	// preparse script
	MakeScript();
	// Success
	return fSuccess;
}

bool C4ScriptHost::LoadData(const char *szFilename, const char *szData, class C4LangStringTable *pLocalTable)
{
	// String Table
	if (stringTable != pLocalTable)
	{
		if (stringTable) stringTable->DelRef();
		stringTable = pLocalTable;
		if (stringTable) stringTable->AddRef();
	}
	ScriptName.Copy(szFilename);

	StdStrBuf tempScript;
	tempScript.Copy(szData);

	Script.Clear();
	if(stringTable)
		stringTable->ReplaceStrings(tempScript, Script);
	else
		Script.Take(tempScript);

	Preparse();
	return true;
}

void C4ScriptHost::MakeScript()
{
	// clear prev
	Script.Clear();

	// create script
	if (stringTable)
	{
		stringTable->ReplaceStrings(GetDataBuf(), Script);
	}
	else
	{
		Script.Ref(GetDataBuf());
	}

	// preparse script
	Preparse();
}

bool C4ScriptHost::ReloadScript(const char *szPath, const char *szLanguage)
{
	// this?
	if (SEqualNoCase(szPath, GetFilePath()) || (stringTable && SEqualNoCase(szPath, stringTable->GetFilePath())))
	{
		// try reload
		char szParentPath[_MAX_PATH + 1]; C4Group ParentGrp;
		if (GetParentPath(szPath, szParentPath))
			if (ParentGrp.Open(szParentPath))
				if (Load(ParentGrp, nullptr, szLanguage, stringTable))
					return true;
	}
	return false;
}

std::string C4ScriptHost::Translate(const std::string &text) const
{
	if (stringTable)
		return stringTable->Translate(text);
	throw C4LangStringTable::NoSuchTranslation(text);
}

/*--- C4ExtraScriptHost ---*/

C4ExtraScriptHost::C4ExtraScriptHost(C4String *parent_key_name):
		ParserPropList(C4PropList::NewStatic(nullptr, nullptr, parent_key_name))
{
}

C4ExtraScriptHost::~C4ExtraScriptHost()
{
	Clear();
}

void C4ExtraScriptHost::Clear()
{
	C4ScriptHost::Clear();
	ParserPropList._getPropList()->Clear();
}

C4PropListStatic * C4ExtraScriptHost::GetPropList()
{
	return ParserPropList._getPropList()->IsStatic();
}


/*--- C4ScenarioObjectsScriptHost ---*/

C4ScenarioObjectsScriptHost::C4ScenarioObjectsScriptHost() : C4ExtraScriptHost(::Strings.RegString("ScenarioObjects"))
{
	// Note that "ScenarioObjects" is a fake key name under which you cannot access this prop list from script.
	// It's just given to have a proper name when script errors are reported.
}

/*--- C4DefScriptHost ---*/

bool C4DefScriptHost::Parse()
{
	bool r = C4ScriptHost::Parse();
	assert(Def);

	// Check category
	if (!Def->GetPlane() && Def->Category & C4D_SortLimit)
	{
		int Plane; bool gotplane = true;
		switch (Def->Category & C4D_SortLimit)
		{
			case C4D_StaticBack: Plane = 100; break;
			case C4D_Structure: Plane = C4Plane_Structure; break;
			case C4D_Vehicle: Plane = 300; break;
			case C4D_Living: Plane = 400; break;
			case C4D_Object: Plane = 500; break;
			case C4D_StaticBack | C4D_Background: Plane = -500; break;
			case C4D_Structure | C4D_Background: Plane = -400; break;
			case C4D_Vehicle | C4D_Background: Plane = -300; break;
			case C4D_Living | C4D_Background: Plane = -200; break;
			case C4D_Object | C4D_Background: Plane = -100; break;
			case C4D_StaticBack | C4D_Foreground: Plane = 1100; break;
			case C4D_Structure | C4D_Foreground: Plane = 1200; break;
			case C4D_Vehicle | C4D_Foreground: Plane = 1300; break;
			case C4D_Living | C4D_Foreground: Plane = 1400; break;
			case C4D_Object | C4D_Foreground: Plane = 1500; break;
			default:
				Warn("Def %s (%s) has invalid category", Def->GetName(), Def->GetDataString().getData());
				gotplane = false;
				break;
		}
		if (gotplane) Def->SetProperty(P_Plane, C4VInt(Plane));
	}
	if (!Def->GetPlane())
	{
		Warn("Def %s (%s) has invalid Plane", Def->GetName(), Def->GetDataString().getData());
		Def->SetProperty(P_Plane, C4VInt(1));
	}
	return r;
}

C4PropListStatic * C4DefScriptHost::GetPropList() { return Def; }

class C4PropListScen: public C4PropListStatic
{
public:
	C4PropListScen(const C4PropListStatic * parent, C4String * key): C4PropListStatic(nullptr, parent, key)
	{
		C4PropList * proto = C4PropList::NewStatic(ScriptEngine.GetPropList(), this, &::Strings.P[P_Prototype]);
		C4PropListStatic::SetPropertyByS(&::Strings.P[P_Prototype], C4VPropList(proto));
	}
	void SetPropertyByS(C4String * k, const C4Value & to) override
	{
		if (k == &Strings.P[P_Prototype])
		{
			DebugLog("ERROR: Attempt to set Scenario.Prototype.");
			return;
		}
		C4PropListStatic::SetPropertyByS(k, to);
	}
};

/*--- C4GameScriptHost ---*/

C4GameScriptHost::C4GameScriptHost(): ScenPrototype(0), ScenPropList(0) { }
C4GameScriptHost::~C4GameScriptHost() = default;

bool C4GameScriptHost::Load(C4Group & g, const char * f, const char * l, C4LangStringTable * t)
{
	assert(ScriptEngine.GetPropList());
	C4PropListStatic * pScen = new C4PropListScen(nullptr, &::Strings.P[P_Scenario]);
	ScenPropList.SetPropList(pScen);
	::ScriptEngine.RegisterGlobalConstant("Scenario", ScenPropList);
	ScenPrototype.SetPropList(pScen->GetPrototype());
	Reg2List(&ScriptEngine);
	return C4ScriptHost::Load(g, f, l, t);
}

bool C4GameScriptHost::LoadData(const char * f, const char * d, C4LangStringTable * t)
{
	assert(ScriptEngine.GetPropList());
	C4PropListStatic * pScen = new C4PropListScen(nullptr, &::Strings.P[P_Scenario]);
	ScenPropList.SetPropList(pScen);
	::ScriptEngine.RegisterGlobalConstant("Scenario", ScenPropList);
	ScenPrototype.SetPropList(pScen->GetPrototype());
	Reg2List(&ScriptEngine);
	return C4ScriptHost::LoadData(f, d, t);
}

void C4GameScriptHost::Clear()
{
	C4ScriptHost::Clear();
	ScenPropList.Set0();
	ScenPrototype.Set0();
	delete pScenarioEffects; pScenarioEffects=nullptr;
}

C4PropListStatic * C4GameScriptHost::GetPropList()
{
	C4PropList * p = ScenPrototype._getPropList();
	return p ? p->IsStatic() : nullptr;
}

void C4GameScriptHost::Denumerate(C4ValueNumbers * numbers)
{
	ScenPropList.Denumerate(numbers);
	if (pScenarioEffects) pScenarioEffects->Denumerate(numbers);
}

C4Value C4GameScriptHost::Call(const char *szFunction, C4AulParSet *Pars, bool fPassError)
{
	return ScenPropList._getPropList()->Call(szFunction, Pars, fPassError);
}

C4GameScriptHost GameScript;
