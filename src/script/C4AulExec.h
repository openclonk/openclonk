/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001, 2006-2007  Sven Eberhardt
 * Copyright (c) 2006, 2009-2010  Peter Wortmann
 * Copyright (c) 2007, 2009-2011  Günther Brammer
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

#ifndef C4AULEXEC_H
#define C4AULEXEC_H

#include <C4Aul.h>

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
	time_t tDirectExecStart, tDirectExecTotal; // profiler time for DirectExec
	C4AulScript *pProfiledScript;

	C4AulScriptContext Contexts[MAX_CONTEXT_STACK];
	C4Value Values[MAX_VALUE_STACK];

public:
	C4Value Exec(C4AulScriptFunc *pSFunc, C4Object *pObj, C4Value pPars[], bool fPassErrors, bool fTemporaryScript = false);
	C4Value Exec(C4AulBCC *pCPos, bool fPassErrors);

	void StartTrace();
	void StartProfiling(C4AulScript *pScript); // resets profling times and starts recording the times
	void StopProfiling(); // stop the profiler and displays results
	void AbortProfiling() { fProfiling=false; }
	inline void StartDirectExec() { if (fProfiling) tDirectExecStart = GetTime(); }
	inline void StopDirectExec() { if (fProfiling) tDirectExecTotal += GetTime() - tDirectExecStart; }

	int GetContextDepth() const { return pCurCtx - Contexts + 1; }
	C4AulScriptContext *GetContext(int iLevel) { return iLevel >= 0 && iLevel < GetContextDepth() ? Contexts + iLevel : NULL; }
	void LogCallStack();

private:
	void PushContext(const C4AulScriptContext &rContext);
	void PopContext();

	void CheckOverflow(int iCnt)
	{
		if (pCurVal - Values >= MAX_VALUE_STACK - iCnt)
			throw new C4AulExecError(pCurCtx->Obj, "value stack overflow, probably due to too deep recursion");
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
		if (!pPar1->ConvertTo(Type1))
			throw new C4AulExecError(pCurCtx->Obj,
			                         FormatString("operator \"%s\" left side got %s, but expected %s",
			                                      opname, pPar1->GetTypeInfo(), GetC4VName(Type1)).getData());
		if (!pPar2->ConvertTo(Type2))
			throw new C4AulExecError(pCurCtx->Obj,
			                         FormatString("operator \"%s\" right side got %s, but expected %s",
			                                      opname, pPar2->GetTypeInfo(), GetC4VName(Type2)).getData());
	}
	ALWAYS_INLINE void CheckOpPar(C4V_Type Type1, const char * opname)
	{
		// Typecheck parameter
		if (!pCurVal->ConvertTo(Type1))
			throw new C4AulExecError(pCurCtx->Obj,
			                         FormatString("operator \"%s\": got %s, but expected %s",
			                                      opname, pCurVal->GetTypeInfo(), GetC4VName(Type1)).getData());
	}

	C4V_Type CheckArrayAccess(C4Value *pStructure, C4Value *pIndex)
	{
		if (pStructure->ConvertToNoNil(C4V_Array))
		{
			if (!pIndex->ConvertTo(C4V_Int))
				throw new C4AulExecError(pCurCtx->Obj, FormatString("array access: index of type %s, but expected int", pIndex->GetTypeName()).getData());
			return C4V_Array;
		}
		else if (pStructure->ConvertToNoNil(C4V_PropList))
		{
			if (!pIndex->ConvertToNoNil(C4V_String))
				throw new C4AulExecError(pCurCtx->Obj, FormatString("proplist access: index of type %s, but expected string", pIndex->GetTypeName()).getData());
			return C4V_PropList;
		}
		else
			throw new C4AulExecError(pCurCtx->Obj, FormatString("can't access %s as array or proplist", pStructure->GetTypeName()).getData());
	}
	C4AulBCC *Call(C4AulFunc *pFunc, C4Value *pReturn, C4Value *pPars, C4Object *pObj = NULL, C4Def *pDef = NULL);
};

extern C4AulExec AulExec;

#endif // C4AULEXEC_H
