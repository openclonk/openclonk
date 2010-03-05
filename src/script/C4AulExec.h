
#ifndef C4AULEXEC_H
#define C4AULEXEC_H

const int MAX_CONTEXT_STACK = 512;
const int MAX_VALUE_STACK = 1024;

class C4AulExec
	{

	public:
		C4AulExec()
			: pCurCtx(Contexts - 1), pCurVal(Values - 1), iTraceStart(-1)
		{ }

	private:
		C4AulScriptContext Contexts[MAX_CONTEXT_STACK];
		C4Value Values[MAX_VALUE_STACK];

		C4AulScriptContext *pCurCtx;
		C4Value *pCurVal;

		int iTraceStart;
		bool fProfiling;
		time_t tDirectExecStart, tDirectExecTotal; // profiler time for DirectExec
		C4AulScript *pProfiledScript;

	public:
		C4Value Exec(C4AulScriptFunc *pSFunc, C4Object *pObj, C4Value pPars[], bool fPassErrors, bool fTemporaryScript = false);
		C4Value Exec(C4AulBCC *pCPos, bool fPassErrors);

		void StartTrace();
		void StartProfiling(C4AulScript *pScript); // resets profling times and starts recording the times
		void StopProfiling(); // stop the profiler and displays results
		void AbortProfiling() { fProfiling=false; }
		inline void StartDirectExec() { if (fProfiling) tDirectExecStart = timeGetTime(); }
		inline void StopDirectExec() { if (fProfiling) tDirectExecTotal += timeGetTime() - tDirectExecStart; }
		
		int GetContextDepth() const { return pCurCtx - Contexts + 1; }
		C4AulScriptContext *GetContext(int iLevel) { return iLevel >= 0 && iLevel < GetContextDepth() ? Contexts + iLevel : NULL; }

	private:
		void PushContext(const C4AulScriptContext &rContext);
		void PopContext();

		void CheckOverflow(int iCnt)
		{
			if(ValueStackSize() + iCnt > MAX_VALUE_STACK)
				throw new C4AulExecError(pCurCtx->Obj, "internal error: value stack overflow!");
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

		void PushValueRef(C4Value &rVal)
		{
			CheckOverflow(1);
			(++pCurVal)->SetRef(&rVal);
		}

		void PushNullVals(int iCnt)
		{
			CheckOverflow(iCnt);
			pCurVal += iCnt;
		}

		bool PopValue()
		{
			if(LocalValueStackSize() < 1)
				throw new C4AulExecError(pCurCtx->Obj, "internal error: value stack underflow!");
			(pCurVal--)->Set0();
			return true;
		}

		void PopValues(int n)
		{
			if(LocalValueStackSize() < n)
				throw new C4AulExecError(pCurCtx->Obj, "internal error: value stack underflow!");
			while(n--)
				(pCurVal--)->Set0();
		}

		void PopValuesUntil(C4Value *pUntilVal)
		{
			if(pUntilVal < Values - 1)
				throw new C4AulExecError(pCurCtx->Obj, "internal error: value stack underflow!");
			while(pCurVal > pUntilVal)
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

		void CheckOpPars(int iOpID)
		{
			// Get parameters
			C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;

			// Typecheck parameters
			if(!pPar1->ConvertTo(C4ScriptOpMap[iOpID].Type1))
				throw new C4AulExecError(pCurCtx->Obj,
					FormatString("operator \"%s\" left side: got \"%s\", but expected \"%s\"!",
						C4ScriptOpMap[iOpID].Identifier, pPar1->GetTypeInfo(), GetC4VName(C4ScriptOpMap[iOpID].Type1)).getData());
			if(!pPar2->ConvertTo(C4ScriptOpMap[iOpID].Type2))
				throw new C4AulExecError(pCurCtx->Obj,
					FormatString("operator \"%s\" right side: got \"%s\", but expected \"%s\"!",
						C4ScriptOpMap[iOpID].Identifier, pPar2->GetTypeInfo(), GetC4VName(C4ScriptOpMap[iOpID].Type2)).getData());
		}
		void CheckOpPar(int iOpID)
		{
			// Typecheck parameter
			if(!pCurVal->ConvertTo(C4ScriptOpMap[iOpID].Type1))
				throw new C4AulExecError(pCurCtx->Obj,
					FormatString("operator \"%s\": got \"%s\", but expected \"%s\"!",
						C4ScriptOpMap[iOpID].Identifier, pCurVal->GetTypeInfo(), GetC4VName(C4ScriptOpMap[iOpID].Type1)).getData());
		}
		C4AulBCC *Call(C4AulFunc *pFunc, C4Value *pReturn, C4Value *pPars, C4Object *pObj = NULL, C4Def *pDef = NULL);
	};

extern C4AulExec AulExec;

#endif // C4AULEXEC_H