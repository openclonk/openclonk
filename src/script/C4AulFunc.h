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

struct C4AulParSet
{
	C4Value Par[C4AUL_MAX_Par];

	C4AulParSet() {} // standard-constructor
	C4AulParSet(const C4Value &par0,             const C4Value &par1 = C4Value(), const C4Value &par2 = C4Value(), const C4Value &par3 = C4Value(), const C4Value &par4 = C4Value(),
	            const C4Value &par5 = C4Value(), const C4Value &par6 = C4Value(), const C4Value &par7 = C4Value(), const C4Value &par8 = C4Value(), const C4Value &par9 = C4Value())
	{
		Par[0].Set(par0); Par[1].Set(par1); Par[2].Set(par2); Par[3].Set(par3); Par[4].Set(par4);
		Par[5].Set(par5); Par[6].Set(par6); Par[7].Set(par7); Par[8].Set(par8); Par[9].Set(par9);
	}
	C4AulParSet(const C4Value * Pars, int ParCount)
	{
		for (int i = 0; i < ParCount; ++i)
			Par[i].Set(Pars[i]);
	}
	C4Value & operator[](int iIdx) { return Par[iIdx]; }
	C4AulParSet * operator&() { return this; }
};

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
	C4AulFunc *MapNext; // map member
	virtual ~C4AulFunc();

public:
	virtual C4AulScriptFunc *SFunc() { return NULL; } // type check func...

	// Wether this function should be visible to players
	virtual bool GetPublic() { return false; }
	virtual int GetParCount() { return C4AUL_MAX_Par; }
	virtual C4V_Type* GetParType() = 0;
	virtual C4V_Type GetRetType() = 0;
	C4Value Exec(C4PropList * p = NULL, C4AulParSet *pPars = NULL, bool fPassErrors=false)
	{
		return Exec(p, pPars->Par, fPassErrors);
	}
	virtual C4Value Exec(C4PropList * p, C4Value pPars[], bool fPassErrors=false) = 0;
};

#endif

