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

/* Component host for CAulScript */

#ifndef INC_C4ScriptHost
#define INC_C4ScriptHost

#include <C4ComponentHost.h>
#include <C4Aul.h>

// generic script host for objects
class C4ScriptHost : public C4AulScript
{
public:
	~C4ScriptHost();
	bool Delete() { return false; } // do NOT delete this - it's just a class member!

	void Clear();
	virtual bool Load(C4Group &hGroup, const char *szFilename,
	          const char *szLanguage, C4LangStringTable *pLocalTable);
	virtual bool LoadData(const char *szFilename, const char *szData, class C4LangStringTable *pLocalTable);
	const char *GetScript() const { return Script.getData(); }
	virtual C4ScriptHost * GetScriptHost() { return this; }
	std::list<C4ScriptHost *> SourceScripts;
protected:
	C4ScriptHost();
	void SetError(const char *szMessage);
	void MakeScript();
	bool ReloadScript(const char *szPath, const char *szLanguage);
	C4ComponentHost ComponentHost;

	bool Preparse(); // preparse script; return if successfull
	virtual bool Parse(); // parse preparsed script; return if successfull
	virtual void UnLink(); // reset to unlinked state


	std::list<StdCopyStrBuf> Includes; // include list
	std::list<StdCopyStrBuf> Appends; // append list

	virtual void AddEngineFunctions() {}; // add any engine functions specific to this script host
	void CopyPropList(C4Set<C4Property> & from, C4PropListStatic * to);
	bool ResolveIncludes(C4DefList *rDefs); // resolve includes
	bool ResolveAppends(C4DefList *rDefs); // resolve appends
	bool Resolving; // set while include-resolving, to catch circular includes
	bool IncludesResolved;

	StdStrBuf Script; // script
	C4ValueMapNames LocalNamed;
	C4Set<C4Property> LocalValues;
	friend class C4AulParse;
	friend class C4AulScriptFunc;
	friend class C4AulDebug;
};

// script host for System.ocg scripts and scenario section Objects.c
class C4ExtraScriptHost: public C4ScriptHost
{
	C4Value ParserPropList;
public:
	C4ExtraScriptHost(C4String *parent_key_name = NULL);
	void Clear();

	bool Delete() { return true; }
	virtual C4PropListStatic * GetPropList();
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
	C4DefScriptHost() : C4ScriptHost(), Def(NULL) { }

	void SetDef(C4Def *to_def) { Def=to_def; }
	virtual bool Parse();
	virtual C4PropListStatic * GetPropList();
protected:
	C4Def *Def; // owning def file
};


// script host for scenario scripts
class C4GameScriptHost : public C4ScriptHost
{
public:
	C4GameScriptHost();
	~C4GameScriptHost();
	virtual bool Load(C4Group &, const char *, const char *, C4LangStringTable *);
	virtual bool LoadData(const char *, const char *, C4LangStringTable *);
	void Clear();
	virtual C4PropListStatic * GetPropList();
	C4Value Call(const char *szFunction, C4AulParSet *pPars=0, bool fPassError=false);
	C4Value GRBroadcast(const char *szFunction, C4AulParSet *pPars = 0, bool fPassError=false, bool fRejectTest=false);  // call function in scenario script and all goals/rules/environment objects
	C4Value ScenPropList;
	C4Value ScenPrototype;
};

extern C4GameScriptHost GameScript;

#endif
