/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001-2002  Sven Eberhardt
 * Copyright (c) 2005  Peter Wortmann
 * Copyright (c) 2006, 2009  Günther Brammer
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
	const char *GetScript() const { return Script.getData(); }
	virtual C4ScriptHost * GetScriptHost() { return this; }
	std::list<C4ScriptHost *> SourceScripts;
protected:
	C4ScriptHost();
	void SetError(const char *szMessage);
	void MakeScript();
	bool ReloadScript(const char *szPath, const char *szLanguage);
	C4ComponentHost ComponentHost;


	void AddBCC(C4AulBCCType eType, intptr_t = 0, const char * SPos = 0); // add byte code chunk and advance
	void RemoveLastBCC();
	void ClearCode();
	bool Preparse(); // preparse script; return if successfull
	virtual bool Parse(); // parse preparsed script; return if successfull
	virtual void UnLink(); // reset to unlinked state
	int GetCodePos() const { return Code.size(); }
	C4AulBCC *GetCodeByPos(int iPos) { return &Code[iPos]; }
	C4AulBCC *GetLastCode() { return LastCode; }


	std::list<C4ID> Includes; // include list
	std::list<C4ID> Appends; // append list

	void CopyPropList(C4Set<C4Property> & from, C4PropListStatic * to);
	bool ResolveIncludes(C4DefList *rDefs); // resolve includes
	bool ResolveAppends(C4DefList *rDefs); // resolve appends
	bool Resolving; // set while include-resolving, to catch circular includes
	bool IncludesResolved;

	StdStrBuf Script; // script
	std::vector<C4AulBCC> Code;
	std::vector<const char *> PosForCode;
	C4AulBCC * LastCode;
	C4ValueMapNames LocalNamed;
	C4Set<C4Property> LocalValues;
	friend class C4AulParse;
	friend class C4AulScriptFunc;
	friend class C4AulDebug;
	friend class C4DirectExecScript;
};

// script host for System.ocg scripts
class C4ExtraScriptHost: public C4ScriptHost
{
	C4Value ParserPropList;
public:
	C4ExtraScriptHost();
	void Clear();

	bool Delete() { return true; }
	virtual C4PropListStatic * GetPropList();
};

// script host for defs
class C4DefScriptHost: public C4ScriptHost
{
public:
	C4DefScriptHost(C4Def * Def) : C4ScriptHost(), Def(Def) { }

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
	void Clear();
	virtual C4PropListStatic * GetPropList();
	C4Value Call(const char *szFunction, C4AulParSet *pPars=0, bool fPassError=false);
	C4Value GRBroadcast(const char *szFunction, C4AulParSet *pPars = 0, bool fPassError=false, bool fRejectTest=false);  // call function in scenario script and all goals/rules/environment objects
	C4Value ScenPropList;
	C4Value ScenPrototype;
};

extern C4GameScriptHost GameScript;

#endif
