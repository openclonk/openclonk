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

/* Component host for CAulScript */

#ifndef INC_C4ScriptHost
#define INC_C4ScriptHost

#include "c4group/C4ComponentHost.h"
#include "script/C4Aul.h"
#include "script/C4AulAST.h"

#include <bitset>
#include <deque>

// aul script state
enum C4AulScriptState
{
	ASS_NONE,       // nothing
	ASS_PREPARSED,  // function list built; CodeSize set
	ASS_LINKED,     // includes and appends resolved
	ASS_PARSED      // byte code generated
};

// generic script host for objects
class C4ScriptHost: public C4ComponentHost
{
public:
	~C4ScriptHost() override;
	virtual bool Delete() { return false; } // do NOT delete this - it's just a class member!

	void Clear();
	virtual bool Load(C4Group &hGroup, const char *szFilename,
	          const char *szLanguage, C4LangStringTable *pLocalTable);
	virtual bool LoadData(const char *szFilename, const char *szData, class C4LangStringTable *pLocalTable);
	void Reg2List(C4AulScriptEngine *pEngine); // reg to linked list
	virtual C4PropListStatic * GetPropList() { return nullptr; }
	const C4PropListStatic *GetPropList() const { return const_cast<C4ScriptHost*>(this)->GetPropList(); }
	const char *GetScript() const { return Script.getData(); }
	bool IsReady() { return State == ASS_PARSED; } // whether script calls may be done
	// Translate a string using the script's lang table
	std::string Translate(const std::string &text) const;
	std::deque<C4ScriptHost *> SourceScripts;
	StdCopyStrBuf ScriptName; // script name

	bool IsWarningEnabled(const char *pos, C4AulWarningId warning) const;

protected:
	C4ScriptHost();
	void Unreg(); // remove from list
	void MakeScript();
	virtual bool ReloadScript(const char *szPath, const char *szLanguage);

	bool Preparse(); // preparse script; return if successfull
	virtual bool Parse(); // parse preparsed script; return if successfull
	virtual void UnLink(); // reset to unlinked state

	void UnlinkOwnedFunctions();
	void DeleteOwnedPropLists();

	void Warn(const char *pMsg, ...) GNUC_FORMAT_ATTRIBUTE_O;

	C4AulScriptEngine *Engine{nullptr}; //owning engine
	C4ScriptHost *Prev{nullptr}, *Next{nullptr}; // tree structure

	std::list<StdCopyStrBuf> Includes; // include list
	std::list<StdCopyStrBuf> Appends; // append list

	virtual void AddEngineFunctions() {}; // add any engine functions specific to this script host
	void CopyPropList(C4Set<C4Property> & from, C4PropListStatic * to);
	bool ResolveIncludes(C4DefList *rDefs); // resolve includes
	bool ResolveAppends(C4DefList *rDefs); // resolve appends
	void DoAppend(C4Def *def);
	bool Resolving; // set while include-resolving, to catch circular includes
	bool IncludesResolved;

	StdStrBuf Script; // script
	C4LangStringTable *stringTable;
	C4Set<C4Property> LocalValues;
	C4AulScriptState State{ASS_NONE}; // script state

	// list of all functions generated from code in this script host
	std::vector<C4Value> ownedFunctions;

	// list of all static proplists that refer to this script host
	// filled in at link time and used to delete all proplists
	// in Clear() even in case of cyclic references.
	std::vector<C4Value> ownedPropLists;

	void EnableWarning(const char *pos, C4AulWarningId warning, bool enable = true);

	friend class C4AulParse;
	friend class C4AulProfiler;
	friend class C4AulScriptEngine;
	friend class C4AulDebug;
	friend class C4AulCompiler;
	friend class C4AulScriptFunc;

private:
	std::map<const char*, std::bitset<(size_t)C4AulWarningId::WarningCount>> enabledWarnings;
	std::unique_ptr<::aul::ast::Script> ast;
};

// script host for System.ocg scripts and scenario section Objects.c
class C4ExtraScriptHost: public C4ScriptHost
{
	C4Value ParserPropList;
public:
	C4ExtraScriptHost(C4String *parent_key_name = nullptr);
	~C4ExtraScriptHost() override;
	void Clear();

	bool Delete() override { return true; }
	C4PropListStatic * GetPropList() override;
};

// script host for scenario section Objects.c
class C4ScenarioObjectsScriptHost : public C4ExtraScriptHost
{
public:
	C4ScenarioObjectsScriptHost();
};

// script host for defs
class C4DefScriptHost: public C4ScriptHost
{
public:
	C4DefScriptHost() : C4ScriptHost() { }

	void SetDef(C4Def *to_def) { Def=to_def; }
	bool Parse() override;
	C4PropListStatic * GetPropList() override;
protected:
	C4Def *Def{nullptr}; // owning def file
};


// script host for scenario scripts
class C4GameScriptHost : public C4ScriptHost
{
public:
	C4GameScriptHost();
	~C4GameScriptHost() override;
	bool Load(C4Group &, const char *, const char *, C4LangStringTable *) override;
	bool LoadData(const char *, const char *, C4LangStringTable *) override;
	void Clear();
	C4PropListStatic * GetPropList() override;
	void Denumerate(C4ValueNumbers * numbers);
	C4Value Call(const char *szFunction, C4AulParSet *pPars=nullptr, bool fPassError=false);
	C4Value ScenPropList;
	C4Value ScenPrototype;
	C4Effect * pScenarioEffects = nullptr;
};

extern C4GameScriptHost GameScript;

#endif
