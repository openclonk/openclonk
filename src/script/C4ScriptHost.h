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

const int32_t C4SCR_MaxIDLen = 100,
                               C4SCR_MaxDesc  = 256;


// generic script host for objects
class C4ScriptHost : public C4AulScript
{
public:
	C4ScriptHost();
	~C4ScriptHost();
	bool Delete() { return true; }
public:
	void Clear();
	bool Load(C4Group &hGroup, const char *szFilename,
	          const char *szLanguage/*=NULL*/, C4Def *pDef/*=NULL*/, class C4LangStringTable *pLocalTable);
	C4Value Call(const char *szFunction, C4Object *pObj=0, C4AulParSet *pPars=0, bool fPrivateCall=false, bool fPassError=false);
protected:
	void SetError(const char *szMessage);
	void MakeScript();
	bool ReloadScript(const char *szPath, const char *szLanguage);
	C4ComponentHost ComponentHost;
};


// script host for defs
class C4DefScriptHost : public C4ScriptHost
{
public:
	C4DefScriptHost() : C4ScriptHost() { SFn_CalcValue = SFn_SellTo = SFn_ControlTransfer = NULL; }
	void Clear() { SFn_CalcValue = SFn_SellTo = SFn_ControlTransfer = NULL; C4ScriptHost::Clear(); }

	bool Delete() { return false; } // do NOT delete this - it's just a class member!
protected:
	void AfterLink(); // get common funcs
public:
	C4AulScriptFunc *SFn_CalcValue; // get object value
	C4AulScriptFunc *SFn_SellTo; // player par(0) sold the object
	C4AulScriptFunc *SFn_ControlTransfer; // object par(0) tries to get to par(1)/par(2)
};


// script host for scenario scripts
class C4GameScriptHost : public C4ScriptHost
{
public:
	C4GameScriptHost();
	~C4GameScriptHost();
	bool Delete() { return false; } // do NOT delete this - it's a global!
	C4Value GRBroadcast(const char *szFunction, C4AulParSet *pPars = 0, bool fPassError=false, bool fRejectTest=false);  // call function in scenario script and all goals/rules/environment objects

	// Global script data
	int32_t Counter;
	bool Go;
	bool Execute(int);
	void Clear() { Counter = 0; Go = false; C4ScriptHost::Clear(); }

	// Compile scenario script data
	void CompileFunc(StdCompiler *pComp);

};

extern C4GameScriptHost GameScript;

#endif
