/*
 * OpenClonk, http://www.openclonk.org
 *
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

#ifndef INC_C4AulFunc
#define INC_C4AulFunc

#ifndef INC_C4Value
#error Include C4Value.h instead of C4AulFunc.h
#endif

#include "script/C4StringTable.h"

#define C4AUL_MAX_Par         10  // max number of parameters

struct C4AulParSet
{
	C4Value Par[C4AUL_MAX_Par];

	C4AulParSet() {} // standard-constructor
	template<class ...T> explicit C4AulParSet(T&& ...pars):
			Par {C4Value(std::forward<T>(pars))...}
	{
	}
	void Copy(const C4Value * Pars, int ParCount)
	{
		for (int i = 0; i < ParCount; ++i)
			Par[i].Set(Pars[i]);
	}
	C4Value & operator[](int iIdx) { return Par[iIdx]; }
	C4AulParSet * operator&() { return this; }
};

// base function class
class C4AulFunc: public C4RefCnt
{
	friend class C4AulScriptEngine;
	friend class C4AulFuncMap;
	friend class C4AulParse;
	friend class C4ScriptHost;
public:
	C4AulFunc(C4PropListStatic * Parent, const char *pName);

	C4PropListStatic * Parent;
	const char * GetName() const { return Name ? Name->GetCStr() : 0; }
	virtual StdStrBuf GetFullName() const; // get a fully classified name (C4ID::Name) for debug output

protected:
	C4RefCntPointer<C4String> Name; // function name
	C4AulFunc *MapNext; // map member
	virtual ~C4AulFunc();

public:
	virtual C4AulScriptFunc *SFunc() { return nullptr; } // type check func...

	// Wether this function should be visible to players
	virtual bool GetPublic() const { return false; }
	virtual int GetParCount() const { return C4AUL_MAX_Par; }
	virtual const C4V_Type* GetParType() const = 0;
	virtual C4V_Type GetRetType() const = 0;
	C4Value Exec(C4PropList * p = nullptr, C4AulParSet *pPars = nullptr, bool fPassErrors=false)
	{
		// Every parameter type allows conversion from nil, so no parameters are always allowed
		if (!pPars)
			return Exec(p, C4AulParSet().Par, fPassErrors);
		if (!CheckParTypes(pPars->Par, fPassErrors)) return C4Value();
		return Exec(p, pPars->Par, fPassErrors);
	}
	virtual C4Value Exec(C4PropList * p, C4Value pPars[], bool fPassErrors=false) = 0;
	bool CheckParTypes(const C4Value pPars[], bool fPassErrors) const;
};

#endif

