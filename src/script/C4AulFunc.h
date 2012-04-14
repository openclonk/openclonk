/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001  Sven Eberhardt
 * Copyright (c) 2004  Peter Wortmann
 * Copyright (c) 2006-2008, 2011-2012  Günther Brammer
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

#ifndef INC_C4AulFunc
#define INC_C4AulFunc

#include <C4StringTable.h>

#define C4AUL_MAX_Par         10  // max number of parameters

// base function class
class C4AulFunc
{
	friend class C4AulScript;
	friend class C4AulScriptEngine;
	friend class C4AulFuncMap;
	friend class C4AulParse;
	friend class C4ScriptHost;

	// Reference counter
	unsigned int iRefCnt;

public:
	C4AulFunc(C4AulScript *pOwner, const char *pName);

	// Add/Remove Reference
	void IncRef() { iRefCnt++; }
	void DecRef() { if (!--iRefCnt) delete this;  }

	C4AulScript *Owner; // owner
	const char * GetName() const { return Name ? Name->GetCStr() : 0; }
	virtual StdStrBuf GetFullName(); // get a fully classified name (C4ID::Name) for debug output

protected:
	C4RefCntPointer<C4String> Name; // function name
	C4AulFunc *Prev, *Next; // linked list members
	C4AulFunc *MapNext; // map member
	void AppendToScript(C4AulScript *);
	void RemoveFromScript();
	virtual ~C4AulFunc();

public:
	C4AulFunc *OverloadedBy; // function by which this one is overloaded

	virtual C4AulScriptFunc *SFunc() { return NULL; } // type check func...

	// Wether this function should be visible to players
	virtual bool GetPublic() { return false; }
	virtual int GetParCount() { return C4AUL_MAX_Par; }
	virtual C4V_Type* GetParType() { return 0; }
	virtual C4V_Type GetRetType() { return C4V_Any; }
	virtual C4Value Exec(C4AulContext *pCallerCtx, C4Value pPars[], bool fPassErrors=false) { return C4Value(); } // execute func (script call)
	virtual C4Value Exec(C4PropList * p = NULL, C4AulParSet *pPars = NULL, bool fPassErrors=false); // execute func (engine call)
	virtual void UnLink() { OverloadedBy = NULL; }
};

#endif

