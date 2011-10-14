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
	C4ScriptHost();
	~C4ScriptHost();
	bool Delete() { return true; }
public:
	void Clear();
	virtual bool Load(C4Group &hGroup, const char *szFilename,
	          const char *szLanguage, C4LangStringTable *pLocalTable);
	const char *GetScript() const { return Script.getData(); }
	virtual C4ScriptHost * GetScriptHost() { return this; }
protected:
	void SetError(const char *szMessage);
	void MakeScript();
	bool ReloadScript(const char *szPath, const char *szLanguage);
	C4ComponentHost ComponentHost;


	void AddBCC(C4AulBCCType eType, intptr_t = 0, const char * SPos = 0); // add byte code chunk and advance
	void RemoveLastBCC();
	void ClearCode();
	bool Preparse(); // preparse script; return if successfull
	bool Parse(); // parse preparsed script; return if successfull
	int GetCodePos() const { return Code.size(); }
	C4AulBCC *GetCodeByPos(int iPos) { return &Code[iPos]; }
	C4AulBCC *GetLastCode() { return LastCode; }

	StdStrBuf Script; // script
	std::vector<C4AulBCC> Code;
	std::vector<const char *> PosForCode;
	C4AulBCC * LastCode;
	friend class C4AulParseState;
	friend class C4AulScriptFunc;
	friend class C4AulDebug;
};


// script host for defs
class C4DefScriptHost : public C4ScriptHost
{
public:
	C4DefScriptHost(C4Def * Def) : C4ScriptHost(), Def(Def) { }
	C4Value Call(const char *szFunction, C4Object *pObj=0, C4AulParSet *pPars=0, bool fPrivateCall=false, bool fPassError=false);
	void Clear();

	bool Delete() { return false; } // do NOT delete this - it's just a class member!
	virtual bool Load(C4Group &, const char *, const char *, C4LangStringTable *);
	virtual C4PropList * GetPropList();
protected:
	C4Def *Def; // owning def file
	void AfterLink(); // get common funcs
};


// script host for scenario scripts
class C4GameScriptHost : public C4ScriptHost
{
public:
	C4GameScriptHost();
	~C4GameScriptHost();
	bool LoadScenarioScripts(C4Group &hGroup, C4LangStringTable *pLocalTable);
	void Clear();
	void AfterLink();
	virtual bool Delete() { return false; } // do NOT delete this - it's a global!
	virtual C4PropList * GetPropList() { return ScenPrototype; }
	C4Value Call(const char *szFunction, C4AulParSet *pPars=0, bool fPassError=false);
	C4Value GRBroadcast(const char *szFunction, C4AulParSet *pPars = 0, bool fPassError=false, bool fRejectTest=false);  // call function in scenario script and all goals/rules/environment objects
	C4PropList * ScenPropList;
	C4PropList * ScenPrototype;
};

extern C4GameScriptHost GameScript;

#endif
