/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
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

/* Handles script file components (calls, inheritance, function maps) */

#include <C4Include.h>
#include <C4ScriptHost.h>

#include <C4Def.h>

/*--- C4ScriptHost ---*/

C4ScriptHost::C4ScriptHost():
	// prepare lists
	Prev(NULL), Next(NULL),
	Engine(NULL),
	State(ASS_NONE) // not compiled
{
	Script = NULL;
	stringTable = 0;
	SourceScripts.push_back(this);
	LocalNamed.Reset();
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
	C4ComponentHost::Clear();
	Script.Clear();
	LocalNamed.Reset();
	LocalValues.Clear();
	SourceScripts.clear();
	SourceScripts.push_back(this);
	if (stringTable)
	{
		stringTable->DelRef();
		stringTable = NULL;
	}
	// remove includes
	Includes.clear();
	Appends.clear();
	// reset flags
	State = ASS_NONE;
}

void C4ScriptHost::Unreg()
{
	// remove from list
	if (Prev) Prev->Next = Next; else if (Engine) Engine->Child0 = Next;
	if (Next) Next->Prev = Prev; else if (Engine) Engine->ChildL = Prev;
	Prev = Next = NULL;
	Engine = NULL;
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
				if (Load(ParentGrp, NULL, szLanguage, stringTable))
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
		ParserPropList(C4PropList::NewStatic(NULL, NULL, parent_key_name))
{
}

void C4ExtraScriptHost::Clear()
{
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
	C4PropListScen(const C4PropListStatic * parent, C4String * key): C4PropListStatic(NULL, parent, key)
	{
		C4PropList * proto = C4PropList::NewStatic(ScriptEngine.GetPropList(), this, &::Strings.P[P_Prototype]);
		C4PropListStatic::SetPropertyByS(&::Strings.P[P_Prototype], C4VPropList(proto));
	}
	virtual void SetPropertyByS(C4String * k, const C4Value & to)
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
C4GameScriptHost::~C4GameScriptHost() { }

bool C4GameScriptHost::Load(C4Group & g, const char * f, const char * l, C4LangStringTable * t)
{
	assert(ScriptEngine.GetPropList());
	C4PropListStatic * pScen = new C4PropListScen(NULL, &::Strings.P[P_Scenario]);
	ScenPropList.SetPropList(pScen);
	::ScriptEngine.RegisterGlobalConstant("Scenario", ScenPropList);
	ScenPrototype.SetPropList(pScen->GetPrototype());
	Reg2List(&ScriptEngine);
	return C4ScriptHost::Load(g, f, l, t);
}

bool C4GameScriptHost::LoadData(const char * f, const char * d, C4LangStringTable * t)
{
	assert(ScriptEngine.GetPropList());
	C4PropListStatic * pScen = new C4PropListScen(NULL, &::Strings.P[P_Scenario]);
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
}

C4PropListStatic * C4GameScriptHost::GetPropList()
{
	C4PropList * p = ScenPrototype._getPropList();
	return p ? p->IsStatic() : 0;
}

C4Value C4GameScriptHost::Call(const char *szFunction, C4AulParSet *Pars, bool fPassError)
{
	return ScenPropList._getPropList()->Call(szFunction, Pars, fPassError);
}

C4GameScriptHost GameScript;
