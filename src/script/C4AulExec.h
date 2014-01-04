/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001, 2006-2007, Sven Eberhardt
 * Copyright (c) 2006, Peter Wortmann
 * Copyright (c) 2007, Günther Brammer
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

#ifndef C4AULEXEC_H
#define C4AULEXEC_H

#include <C4Aul.h>
#include "C4TimeMilliseconds.h"

const int MAX_CONTEXT_STACK = 512;
const int MAX_VALUE_STACK = 1024;

/*
 The Stack layout is as follows:
 first parameter
 ...
 last parameter
 first named var
 ...
 last named var
 temporary values
 */

class C4AulExec
{

public:
	C4AulExec()
			: pCurCtx(Contexts - 1), pCurVal(Values - 1), iTraceStart(-1)
	{ }

private:
	C4AulScriptContext *pCurCtx;
	C4Value *pCurVal;

	int iTraceStart;
	bool fProfiling;
	C4TimeMilliseconds tDirectExecStart;
	uint32_t tDirectExecTotal; // profiler time for DirectExec
	C4AulScript *pProfiledScript;

	C4AulScriptContext Contexts[MAX_CONTEXT_STACK];
	C4Value Values[MAX_VALUE_STACK];

public:
	C4Value Exec(C4AulScriptFunc *pSFunc, C4PropList * p, C4Value pPars[], bool fPassErrors);
	C4Value Exec(C4AulBCC *pCPos, bool fPassErrors);

	void StartTrace();
	void StartProfiling(C4AulScript *pScript); // resets profling times and starts recording the times
	void StopProfiling(); // stop the profiler and displays results
	void AbortProfiling() { fProfiling=false; }
	inline void StartDirectExec() { if (fProfiling) tDirectExecStart = C4TimeMilliseconds::Now(); }
	inline void StopDirectExec() { if (fProfiling) tDirectExecTotal += C4TimeMilliseconds::Now() - tDirectExecStart; }

	int GetContextDepth() const { return pCurCtx - Contexts + 1; }
	C4AulScriptContext *GetContext(int iLevel) { return iLevel >= 0 && iLevel < GetContextDepth() ? Contexts + iLevel : NULL; }
	void LogCallStack();
	static C4String *FnTranslate(C4PropList * _this, C4String *text);
	static bool FnLogCallStack(C4PropList * _this);
	void ClearPointers(C4Object *);

private:
	void PushContext(const C4AulScriptContext &rContext);
	void PopContext();

	void CheckOverflow(int iCnt)
	{
		if (pCurVal - Values >= MAX_VALUE_STACK - iCnt)
			throw new C4AulExecError("value stack overflow, probably due to too deep recursion");
	}

	void PushInt(int32_t i)
	{
		CheckOverflow(1);
		(++pCurVal)->SetInt(i);
	}

	void PushBool(bool b)
	{
		CheckOverflow(1);
		(++pCurVal)->SetBool(b);
	}

	void PushString(C4String * Str)
	{
		CheckOverflow(1);
		(++pCurVal)->SetString(Str);
	}

	void PushArray(C4ValueArray * Array)
	{
		CheckOverflow(1);
		(++pCurVal)->SetArray(Array);
	}

	void PushFunction(C4AulFunc * Fn)
	{
		CheckOverflow(1);
		(++pCurVal)->SetFunction(Fn);
	}

	void PushPropList(C4PropList * PropList)
	{
		CheckOverflow(1);
		(++pCurVal)->SetPropList(PropList);
	}

	void PushValue(const C4Value &rVal)
	{
		CheckOverflow(1);
		(++pCurVal)->Set(rVal);
	}

	void PushNullVals(int iCnt)
	{
		CheckOverflow(iCnt);
		pCurVal += iCnt;
	}

	bool PopValue()
	{
		assert (LocalValueStackSize() >= 1);
		(pCurVal--)->Set0();
		return true;
	}

	void PopValues(int n)
	{
		assert (LocalValueStackSize() >= n);
		while (n--)
			(pCurVal--)->Set0();
	}

	void PopValuesUntil(C4Value *pUntilVal)
	{
		assert (pUntilVal >= Values - 1);
		while (pCurVal > pUntilVal)
			(pCurVal--)->Set0();
	}

	int ContextStackSize() const
	{
		return pCurCtx - Contexts + 1;
	}

	int ValueStackSize() const
	{
		return pCurVal - Values + 1;
	}

	int LocalValueStackSize() const
	{
		return ContextStackSize()
		       ? pCurVal - pCurCtx->Vars - pCurCtx->Func->VarNamed.iSize + 1
		       : pCurVal - Values + 1;
	}

	ALWAYS_INLINE void CheckOpPars(C4V_Type Type1, C4V_Type Type2, const char * opname)
	{
		// Get parameters
		C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;

		// Typecheck parameters
		if (!pPar1->CheckParConversion(Type1))
			throw new C4AulExecError(FormatString("operator \"%s\" left side got %s, but expected %s",
			                                      opname, pPar1->GetTypeName(), GetC4VName(Type1)).getData());
		if (!pPar2->CheckParConversion(Type2))
			throw new C4AulExecError(FormatString("operator \"%s\" right side got %s, but expected %s",
			                                      opname, pPar2->GetTypeName(), GetC4VName(Type2)).getData());
	}
	ALWAYS_INLINE void CheckOpPar(C4V_Type Type1, const char * opname)
	{
		// Typecheck parameter
		if (!pCurVal->CheckParConversion(Type1))
			throw new C4AulExecError(FormatString("operator \"%s\": got %s, but expected %s",
			                                      opname, pCurVal->GetTypeName(), GetC4VName(Type1)).getData());
	}

	C4V_Type CheckArrayAccess(C4Value *pStructure, C4Value *pIndex)
	{
		if (pStructure->CheckConversion(C4V_Array))
		{
			if (!pIndex->CheckConversion(C4V_Int))
				throw new C4AulExecError(FormatString("array access: index of type %s, but expected int", pIndex->GetTypeName()).getData());
			return C4V_Array;
		}
		else if (pStructure->CheckConversion(C4V_PropList))
		{
			if (!pIndex->CheckConversion(C4V_String))
				throw new C4AulExecError(FormatString("proplist access: index of type %s, but expected string", pIndex->GetTypeName()).getData());
			return C4V_PropList;
		}
		else
			throw new C4AulExecError(FormatString("can't access %s as array or proplist", pStructure->GetTypeName()).getData());
	}
	C4AulBCC *Call(C4AulFunc *pFunc, C4Value *pReturn, C4Value *pPars, C4PropList * pContext = NULL);
};

extern C4AulExec AulExec;

#endif // C4AULEXEC_H
