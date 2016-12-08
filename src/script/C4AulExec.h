/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001, 2006-2007, Sven Eberhardt
 * Copyright (c) 2006, Peter Wortmann
 * Copyright (c) 2007, Günther Brammer
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

#ifndef C4AULEXEC_H
#define C4AULEXEC_H

#include "script/C4Aul.h"
#include "platform/C4TimeMilliseconds.h"
#include "script/C4AulScriptFunc.h"

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

// execution context
struct C4AulScriptContext
{
	C4PropList *Obj;
	C4Value *Return;
	C4Value *Pars;
	C4AulScriptFunc *Func;
	C4AulBCC *CPos;
	C4TimeMilliseconds tTime; // initialized only by profiler if active

	void dump(StdStrBuf Dump = StdStrBuf(""));
	StdStrBuf ReturnDump(StdStrBuf Dump = StdStrBuf(""));
};

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
	C4ScriptHost *pProfiledScript;

	C4AulScriptContext Contexts[MAX_CONTEXT_STACK];
	C4Value Values[MAX_VALUE_STACK];

	void StartProfiling(C4ScriptHost *pScript); // starts recording the times
	bool IsProfiling() { return fProfiling; }
	void StopProfiling() { fProfiling=false; }
	friend class C4AulProfiler;
public:
	C4Value Exec(C4AulScriptFunc *pSFunc, C4PropList * p, C4Value pPars[], bool fPassErrors);
	C4Value DirectExec(C4PropList *p, const char *szScript, const char *szContext, bool fPassErrors = false, C4AulScriptContext* context = nullptr, bool parse_function = false);

	void StartTrace();
	inline void StartDirectExec() { if (fProfiling) tDirectExecStart = C4TimeMilliseconds::Now(); }
	inline void StopDirectExec() { if (fProfiling) tDirectExecTotal += C4TimeMilliseconds::Now() - tDirectExecStart; }

	int GetContextDepth() const { return pCurCtx - Contexts + 1; }
	C4AulScriptContext *GetContext(int iLevel) { return iLevel >= 0 && iLevel < GetContextDepth() ? Contexts + iLevel : nullptr; }
	void LogCallStack();
	static C4String *FnTranslate(C4PropList * _this, C4String *text);
	static bool FnLogCallStack(C4PropList * _this);
	void ClearPointers(C4Object *);

private:
	C4Value Exec(C4AulBCC *pCPos);
	void PushContext(const C4AulScriptContext &rContext);
	void PopContext();

	void CheckOverflow(int iCnt)
	{
		if (pCurVal - Values >= MAX_VALUE_STACK - iCnt)
			throw C4AulExecError("value stack overflow, probably due to too deep recursion");
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
		       ? pCurVal - pCurCtx->Pars - pCurCtx->Func->GetParCount() - pCurCtx->Func->VarNamed.iSize + 1
		       : pCurVal - Values + 1;
	}

	ALWAYS_INLINE void CheckOpPars(C4V_Type Type1, C4V_Type Type2, const char * opname)
	{
		// Get parameters
		C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;

		// Typecheck parameters
		if (!pPar1->CheckParConversion(Type1))
			throw C4AulExecError(FormatString("operator \"%s\" left side got %s, but expected %s",
			                                      opname, pPar1->GetTypeName(), GetC4VName(Type1)).getData());
		if (!pPar2->CheckParConversion(Type2))
			throw C4AulExecError(FormatString("operator \"%s\" right side got %s, but expected %s",
			                                      opname, pPar2->GetTypeName(), GetC4VName(Type2)).getData());
	}
	ALWAYS_INLINE void CheckOpPar(C4V_Type Type1, const char * opname)
	{
		// Typecheck parameter
		if (!pCurVal->CheckParConversion(Type1))
			throw C4AulExecError(FormatString("operator \"%s\": got %s, but expected %s",
			                                      opname, pCurVal->GetTypeName(), GetC4VName(Type1)).getData());
	}

	C4V_Type CheckArrayAccess(C4Value *pStructure, C4Value *pIndex)
	{
		if (pStructure->CheckConversion(C4V_Array))
		{
			if (!pIndex->CheckConversion(C4V_Int))
				throw C4AulExecError(FormatString("array access: index of type %s, but expected int", pIndex->GetTypeName()).getData());
			return C4V_Array;
		}
		else if (pStructure->CheckConversion(C4V_PropList))
		{
			if (!pIndex->CheckConversion(C4V_String))
				throw C4AulExecError(FormatString("proplist access: index of type %s, but expected string", pIndex->GetTypeName()).getData());
			return C4V_PropList;
		}
		else
			throw C4AulExecError(FormatString("can't access %s as array or proplist", pStructure->GetTypeName()).getData());
	}
	C4AulBCC *Call(C4AulFunc *pFunc, C4Value *pReturn, C4Value *pPars, C4PropList * pContext = nullptr);
};

extern C4AulExec AulExec;

// script profiler entry
class C4AulProfiler
{
private:
	// map entry
	struct Entry
	{
		C4AulScriptFunc *pFunc;
		uint32_t tProfileTime;

		bool operator < (const Entry &e2) const { return tProfileTime < e2.tProfileTime ; }
	};

	// items
	std::vector<Entry> Times;

	void CollectEntry(C4AulScriptFunc *pFunc, uint32_t tProfileTime);
	void CollectTimes(C4PropListStatic * p);
	void CollectTimes();
	static void ResetTimes(C4PropListStatic * p);
	static void ResetTimes();
	void Show();
public:
	static void Abort() { AulExec.StopProfiling(); }
	static void StartProfiling(C4ScriptHost *pScript); // reset times and start collecting new ones
	static void StopProfiling(); // stop the profiler and displays results
};

#endif // C4AULEXEC_H
