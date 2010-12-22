/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2002, 2006-2007  Sven Eberhardt
 * Copyright (c) 2001-2002, 2005-2007  Peter Wortmann
 * Copyright (c) 2006-2009  Günther Brammer
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
// executes script functions

#include <C4Include.h>
#include <C4Aul.h>
#include <C4AulExec.h>
#include <C4AulDebug.h>

#include <C4Object.h>
#include <C4Config.h>
#include <C4GameMessage.h>
#include <C4Game.h>
#include <C4Log.h>
#include <C4Record.h>
#include <algorithm>

C4AulExec AulExec;

C4AulExecError::C4AulExecError(C4Object *pObj, const char *szError) : cObj(pObj)
{
	// direct error message string
	sMessage.Format("ERROR: %s.", szError ? szError : "(no error message)");
}

void C4AulExecError::show()
{
	// log
	C4AulError::show();
	// debug mode object message
	if (Game.DebugMode)
	{
		if (cObj)
			::Messages.New(C4GM_Target,sMessage,cObj,NO_OWNER);
		else
			::Messages.New(C4GM_Global,sMessage,NULL,ANY_OWNER);
	}
}

StdStrBuf C4AulScriptContext::ReturnDump(StdStrBuf Dump)
{
	if (!Func)
		return StdStrBuf("");
	bool fDirectExec = !*Func->Name;
	if (!fDirectExec)
	{
		// Function name
		Dump.Append(Func->Name);
		// Parameters
		Dump.AppendChar('(');
		int iNullPars = 0;
		for (int i = 0; i < Func->GetParCount(); i++)
			if (Pars + i < Vars)
			{
				if (!Pars[i])
					iNullPars++;
				else
				{
					if (i > iNullPars)
						Dump.AppendChar(',');
					// Insert missing null parameters
					while (iNullPars > 0)
					{
						Dump.Append("0,");
						iNullPars--;
					}
					// Insert parameter
					Dump.Append(Pars[i].GetDataString());
				}
			}
		Dump.AppendChar(')');
	}
	else
		Dump.Append(Func->Owner->ScriptName);
	// Context
	if (Obj)
		Dump.AppendFormat(" (obj %s)", C4VObj(Obj).GetDataString().getData());
	else if (Func->Owner->Def != NULL)
		Dump.AppendFormat(" (def %s)", Func->Owner->Def->GetName());
	// Script
	if (!fDirectExec && Func->pOrgScript)
		Dump.AppendFormat(" (%s:%d)",
		                  Func->pOrgScript->ScriptName.getData(),
		                  CPos ? Func->GetLineOfCode(CPos) : SGetLine(Func->pOrgScript->GetScript(), Func->Script));
	// Return it
	return Dump;
}

void C4AulScriptContext::dump(StdStrBuf Dump)
{
	// Log it
	DebugLog(ReturnDump(Dump).getData());
}

C4Value C4AulExec::Exec(C4AulScriptFunc *pSFunc, C4Object *pObj, C4Value *pnPars, bool fPassErrors, bool fTemporaryScript)
{

	// Push parameters
	C4Value *pPars = pCurVal + 1;
	if (pnPars)
		for (int i = 0; i < C4AUL_MAX_Par; i++)
			PushValue(pnPars[i]);

	// Push variables
	C4Value *pVars = pCurVal + 1;
	PushNullVals(pSFunc->VarNamed.iSize);

	// Derive definition context from function owner (legacy)
	C4Def *pDef = pObj ? pObj->Def : pSFunc->Owner->Def;

	// Executing function in right context?
	// This must hold: The scripter might try to access local variables that don't exist!
	assert(!pSFunc->Owner->Def || pDef == pSFunc->Owner->Def);

	// Push a new context
	C4AulScriptContext ctx;
	ctx.tTime = 0;
	ctx.Obj = pObj;
	ctx.Def = pDef;
	ctx.Return = NULL;
	ctx.Pars = pPars;
	ctx.Vars = pVars;
	ctx.Func = pSFunc;
	ctx.TemporaryScript = fTemporaryScript;
	ctx.CPos = NULL;
	ctx.Caller = NULL;
	PushContext(ctx);

	// Execute
	return Exec(pSFunc->GetCode(), fPassErrors);
}

C4Value C4AulExec::Exec(C4AulBCC *pCPos, bool fPassErrors)
{

#ifndef NOAULDEBUG
	// Debugger pointer
	C4AulDebug * const pDebug = ::ScriptEngine.GetDebugger();
	if (pDebug)
		pDebug->DebugStepIn(pCPos);
#endif

	// Save start context
	C4AulScriptContext *pOldCtx = pCurCtx;

	try
	{

		for (;;)
		{

			bool fJump = false;
			switch (pCPos->bccType)
			{
			case AB_INT:
				PushValue(C4VInt(pCPos->Par.i));
				break;

			case AB_BOOL:
				PushValue(C4VBool(!! pCPos->Par.i));
				break;

			case AB_STRING:
				PushString(pCPos->Par.s);
				break;

			case AB_CPROPLIST:
				PushPropList(pCPos->Par.p);
				break;

			case AB_CARRAY:
				PushArray(pCPos->Par.a);
				break;

			case AB_NIL:
				PushValue(C4VNull);
				break;

			case AB_DUP:
				PushValue(pCurVal[-pCPos->Par.i+1]);
				break;

			case AB_EOFN:
				throw new C4AulExecError(pCurCtx->Obj, "internal error: function didn't return");

			case AB_ERR:
				throw new C4AulExecError(pCurCtx->Obj, "syntax error: see previous parser error for details");

			case AB_PARN:
				PushValue(pCurCtx->Pars[pCPos->Par.i]);
				break;
			case AB_PARN_SET:
				pCurCtx->Pars[pCPos->Par.i] = pCurVal[0];
				break;

			case AB_PARN_CONTEXT:
				PushValue(AulExec.GetContext(AulExec.GetContextDepth()-2)->Pars[pCPos->Par.i]);
				break;

			case AB_VARN_CONTEXT:
				PushValue(AulExec.GetContext(AulExec.GetContextDepth()-2)->Vars[pCPos->Par.i]);
				break;

			case AB_VARN:
				PushValue(pCurCtx->Vars[pCPos->Par.i]);
				break;
			case AB_VARN_SET:
				pCurCtx->Vars[pCPos->Par.i] = pCurVal[0];
				break;

			case AB_LOCALN:
				if (!pCurCtx->Obj)
					throw new C4AulExecError(pCurCtx->Obj, "can't access local variables in a definition call");
				PushNullVals(1);
				pCurCtx->Obj->GetPropertyByS(pCPos->Par.s, pCurVal);
				break;
			case AB_LOCALN_SET:
				if (!pCurCtx->Obj)
					throw new C4AulExecError(pCurCtx->Obj, "can't access local variables in a definition call");
				pCurCtx->Obj->SetPropertyByS(pCPos->Par.s, pCurVal[0]);
				break;

			case AB_PROP:
				if (!(pCurVal->ConvertTo(C4V_PropList) && pCurVal->_getPropList()))
					throw new C4AulExecError(pCurCtx->Obj, FormatString("proplist access: proplist expected, got %s", pCurVal->GetTypeName()).getData());
				if (!pCurVal->_getPropList()->GetPropertyByS(pCPos->Par.s, pCurVal))
					pCurVal->Set0();
				break;
			case AB_PROP_SET:
			{
				C4Value *pPropList = pCurVal - 1;
				if (!(pPropList->ConvertTo(C4V_PropList) && pPropList->_getPropList()))
					throw new C4AulExecError(pCurCtx->Obj, FormatString("proplist write: proplist expected, got %s", pPropList->GetTypeName()).getData());
				if (pPropList->_getPropList()->IsFrozen())
					throw new C4AulExecError(pCurCtx->Obj, "proplist write: proplist is readonly");
				pPropList->_getPropList()->SetPropertyByS(pCPos->Par.s, pCurVal[0]);
				pPropList->Set(pCurVal[0]);
				PopValue();
				break;
			}

			case AB_GLOBALN:
				PushValue(*::ScriptEngine.GlobalNamed.GetItem(pCPos->Par.i));
				break;
			case AB_GLOBALN_SET:
				::ScriptEngine.GlobalNamed.GetItem(pCPos->Par.i)->Set(pCurVal[0]);
				break;
				
			// prefix
			case AB_BitNot: // ~
				CheckOpPar(pCPos->Par.i);
				pCurVal->SetInt(~pCurVal->_getInt());
				break;
			case AB_Not:  // !
				CheckOpPar(pCPos->Par.i);
				pCurVal->SetBool(!pCurVal->_getBool());
				break;
			case AB_Neg:  // -
				CheckOpPar(pCPos->Par.i);
				pCurVal->SetInt(-pCurVal->_getInt());
				break;
			case AB_Inc: // ++
				CheckOpPar(pCPos->Par.i);
				(*pCurVal)++;
				break;
			case AB_Dec: // --
				CheckOpPar(pCPos->Par.i);
				(*pCurVal)--;
				break;
				// postfix
			case AB_Pow:  // **
			{
				CheckOpPars(pCPos->Par.i);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetInt(Pow(pPar1->_getInt(), pPar2->_getInt()));
				PopValue();
				break;
			}
			case AB_Div:  // /
			{
				CheckOpPars(pCPos->Par.i);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				if (!pPar2->_getInt())
					throw new C4AulExecError(pCurCtx->Obj, "division by zero");
				pPar1->SetInt(pPar1->_getInt() / pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_Mul:  // *
			{
				CheckOpPars(pCPos->Par.i);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetInt(pPar1->_getInt() * pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_Mod:  // %
			{
				CheckOpPars(pCPos->Par.i);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				if (pPar2->_getInt())
					pPar1->SetInt(pPar1->_getInt() % pPar2->_getInt());
				else
					pPar1->Set0();
				PopValue();
				break;
			}
			case AB_Sub:  // -
			{
				CheckOpPars(pCPos->Par.i);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetInt(pPar1->_getInt() - pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_Sum:  // +
			{
				CheckOpPars(pCPos->Par.i);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetInt(pPar1->_getInt() + pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_LeftShift:  // <<
			{
				CheckOpPars(pCPos->Par.i);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetInt(pPar1->_getInt() << pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_RightShift: // >>
			{
				CheckOpPars(pCPos->Par.i);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetInt(pPar1->_getInt() >> pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_LessThan: // <
			{
				CheckOpPars(pCPos->Par.i);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetBool(pPar1->_getInt() < pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_LessThanEqual:  // <=
			{
				CheckOpPars(pCPos->Par.i);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetBool(pPar1->_getInt() <= pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_GreaterThan:  // >
			{
				CheckOpPars(pCPos->Par.i);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetBool(pPar1->_getInt() > pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_GreaterThanEqual: // >=
			{
				CheckOpPars(pCPos->Par.i);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetBool(pPar1->_getInt() >= pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_Equal:  // ==
			{
				CheckOpPars(pCPos->Par.i);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetBool(*pPar1 == *pPar2);
				PopValue();
				break;
			}
			case AB_NotEqual: // !=
			{
				CheckOpPars(pCPos->Par.i);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetBool(*pPar1 != *pPar2);
				PopValue();
				break;
			}
			case AB_BitAnd: // &
			{
				CheckOpPars(pCPos->Par.i);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetInt(pPar1->_getInt() & pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_BitXOr: // ^
			{
				CheckOpPars(pCPos->Par.i);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetInt(pPar1->_getInt() ^ pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_BitOr:  // |
			{
				CheckOpPars(pCPos->Par.i);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetInt(pPar1->_getInt() | pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_ARRAY:
			{
				// Create array
				C4ValueArray *pArray = new C4ValueArray(pCPos->Par.i);

				// Pop values from stack
				for (int i = 0; i < pCPos->Par.i; i++)
					(*pArray)[i] = pCurVal[i - pCPos->Par.i + 1];

				// Push array
				if (pCPos->Par.i > 0)
				{
					PopValues(pCPos->Par.i - 1);
					pCurVal->SetArray(pArray);
				}
				else
					PushArray(pArray);

				break;
			}

			case AB_PROPLIST:
			{
				PushPropList(C4PropList::New());
				break;
			}
			case AB_IPROPLIST:
			{
				C4Value *pPropSet = pCurVal - 2, *pKey = pCurVal -1, *pValue = pCurVal;
				if (!pPropSet->ConvertTo(C4V_PropList))
					throw new C4AulExecError(pCurCtx->Obj, FormatString("internal error: proplist expected, got %s", pPropSet->GetTypeName()).getData());
				if (!pKey->ConvertTo(C4V_String))
					throw new C4AulExecError(pCurCtx->Obj, FormatString("internal error: string expected, got %s", pPropSet->GetTypeName()).getData());
				pPropSet->_getPropList()->SetPropertyByS(pKey->_getStr(), *pValue);
				PopValues(2);
				break;
			}

			case AB_ARRAYA:
			{
				C4Value *pIndex = pCurVal, *pStruct = pCurVal - 1, *pResult = pCurVal - 1;
				// Typcheck to determine whether it's an array or a proplist
				if(CheckArrayAccess(pStruct, pIndex) == C4V_Array)
				{
					*pResult = pStruct->_getArray()->GetItem(pIndex->_getInt());
				}
				else
				{
					assert(pStruct->GetType() == C4V_PropList || pStruct->GetType() == C4V_C4Object);
					C4PropList *pPropList = pStruct->_getPropList();
					if (!pPropList->GetPropertyByS(pIndex->_getStr(), pResult))
						pResult->Set0();
				}
				// Remove index
				PopValue();
				break;
			}
			case AB_ARRAYA_SET:
			{
				C4Value *pValue = pCurVal, *pIndex = pCurVal - 1, *pStruct = pCurVal - 2, *pResult = pCurVal - 2;
				// Typcheck to determine whether it's an array or a proplist
				if(CheckArrayAccess(pStruct, pIndex) == C4V_Array)
				{
					pStruct->_getArray()->SetItem(pIndex->_getInt(), *pValue);
				}
				else
				{
					assert(pStruct->GetType() == C4V_PropList || pStruct->GetType() == C4V_C4Object);
					C4PropList *pPropList = pStruct->_getPropList();
					if (pPropList->IsFrozen())
						throw new C4AulExecError(pCurCtx->Obj, "proplist write: proplist is readonly");					
					pPropList->SetPropertyByS(pIndex->_getStr(), *pValue);
				}
				// Set result, remove array and index from stack
				*pResult = *pValue;
				PopValues(2);
				break;
			}
			case AB_ARRAY_SLICE:
			{
				C4Value &Array = pCurVal[-2];
				C4Value &StartIndex = pCurVal[-1];
				C4Value &EndIndex = pCurVal[0];

				// Typcheck
				if (!Array.ConvertTo(C4V_Array) || Array.GetType() == C4V_Any)
					throw new C4AulExecError(pCurCtx->Obj, FormatString("array slice: can't access %s as an array", Array.GetTypeName()).getData());
				if (!StartIndex.ConvertTo(C4V_Int))
					throw new C4AulExecError(pCurCtx->Obj, FormatString("array slice: start index of type %s, int expected", StartIndex.GetTypeName()).getData());
				if (!EndIndex.ConvertTo(C4V_Int))
					throw new C4AulExecError(pCurCtx->Obj, FormatString("array slice: end index of type %s, int expected", EndIndex.GetTypeName()).getData());

				Array.SetArray(Array.GetData().Array->GetSlice(StartIndex._getInt(), EndIndex._getInt()));

				// Remove both indices
				PopValues(2);
				break;
			}

			case AB_ARRAY_SLICE_SET:
			{
				C4Value &Array = pCurVal[-3];
				C4Value &StartIndex = pCurVal[-2];
				C4Value &EndIndex = pCurVal[-1];
				C4Value &Value = pCurVal[0];

				// Typcheck
				if (!Array.ConvertTo(C4V_Array) || Array.GetType() == C4V_Any)
					throw new C4AulExecError(pCurCtx->Obj, FormatString("array slice: can't access %s as an array", Array.GetTypeName()).getData());
				if (!StartIndex.ConvertTo(C4V_Int))
					throw new C4AulExecError(pCurCtx->Obj, FormatString("array slice: start index of type %s, int expected", StartIndex.GetTypeName()).getData());
				if (!EndIndex.ConvertTo(C4V_Int))
					throw new C4AulExecError(pCurCtx->Obj, FormatString("array slice: end index of type %s, int expected", EndIndex.GetTypeName()).getData());

				C4ValueArray *pArray = Array._getArray();
				pArray->SetSlice(StartIndex._getInt(), EndIndex._getInt(), Value);

				// Set value as result, remove both indices and first copy of value
				Array = Value;
				PopValues(3);
				break;
			}

			case AB_STACK:
				if (pCPos->Par.i < 0)
					PopValues(-pCPos->Par.i);
				else
					PushNullVals(pCPos->Par.i);
				break;

			case AB_JUMP:
				fJump = true;
				pCPos += pCPos->Par.i;
				break;

			case AB_JUMPAND:
				if (!pCurVal[0])
				{
					fJump = true;
					pCPos += pCPos->Par.i;
				}
				else
				{
					PopValue();
				}
				break;

			case AB_JUMPOR:
				if (!!pCurVal[0])
				{
					fJump = true;
					pCPos += pCPos->Par.i;
				}
				else
				{
					PopValue();
				}
				break;

			case AB_CONDN:
				if (!pCurVal[0])
				{
					fJump = true;
					pCPos += pCPos->Par.i;
				}
				PopValue();
				break;

			case AB_COND:
				if (pCurVal[0])
				{
					fJump = true;
					pCPos += pCPos->Par.i;
				}
				PopValue();
				break;

			case AB_RETURN:
			{
				// Trace
				if (iTraceStart >= 0)
				{
					StdStrBuf Buf("T");
					Buf.AppendChars('>', ContextStackSize() - iTraceStart);
					LogF("%s%s returned %s", Buf.getData(), pCurCtx->Func->Name, pCurVal->GetDataString().getData());
				}

#ifndef NOAULDEBUG
				// Notify debugger
				C4Value *pReturn = pCurCtx->Return;
				if (pDebug)
					pDebug->DebugStepOut(pReturn ? (pCurCtx-1)->CPos + 1 : NULL, pCurCtx, pCurVal);
#endif

				// External call?
				if (!pReturn)
				{
					// Get return value and stop executing.
					C4Value rVal = *pCurVal;
					PopValuesUntil(pCurCtx->Pars - 1);
					PopContext();
					return rVal;
				}

				// Save return value
				if (pCurVal != pReturn)
					pReturn->Set(*pCurVal);

				// Pop context
				PopContext();

				// Clear value stack, except return value
				PopValuesUntil(pReturn);

				// Jump back, continue.
				pCPos = pCurCtx->CPos + 1;
				fJump = true;

				break;
			}

			case AB_FUNC:
			{
				// Get function call data
				C4AulFunc *pFunc = pCPos->Par.f;
				C4Value *pPars = pCurVal - pFunc->GetParCount() + 1;
				// Save current position
				pCurCtx->CPos = pCPos;
				assert(pCurCtx->Func->GetCode() < pCPos);
				// Do the call
				C4AulBCC *pJump = Call(pFunc, pPars, pPars, NULL);
				if (pJump)
				{
					pCPos = pJump;
					fJump = true;
				}
				break;
			}

			case AB_PAR:
				if (!pCurVal->ConvertTo(C4V_Int))
					throw new C4AulExecError(pCurCtx->Obj, FormatString("Par: index of type %s, int expected", pCurVal->GetTypeName()).getData());
				// Push reference to parameter on the stack
				if (pCurVal->_getInt() >= 0 && pCurVal->_getInt() < pCurCtx->ParCnt())
					pCurVal->Set(pCurCtx->Pars[pCurVal->_getInt()]);
				else
					pCurVal->Set0();
				break;

			case AB_FOREACH_NEXT:
			{
				// This should always hold
				assert(pCurVal->ConvertTo(C4V_Int));
				int iItem = pCurVal->_getInt();
				// Check array the first time only
				if (!iItem)
				{
					if (!pCurVal[-1].ConvertTo(C4V_Array))
						throw new C4AulExecError(pCurCtx->Obj, FormatString("for: array expected, but got %s", pCurVal[-1].GetTypeName()).getData());
					if (!pCurVal[-1]._getArray())
						throw new C4AulExecError(pCurCtx->Obj, FormatString("for: array expected, but got nil").getData());
				}
				C4ValueArray *pArray = pCurVal[-1]._getArray();
				// No more entries?
				if (pCurVal->_getInt() >= pArray->GetSize())
					break;
				// Get next
				pCurCtx->Vars[pCPos->Par.i] = pArray->GetItem(iItem);
				// Save position
				pCurVal->SetInt(iItem + 1);
				// Jump over next instruction
				pCPos += 2;
				fJump = true;
				break;
			}

			case AB_IVARN:
				pCurCtx->Vars[pCPos->Par.i] = pCurVal[0];
				PopValue();
				break;

			case AB_CALL:
			case AB_CALLFS:
			{

				C4Value *pPars = pCurVal - C4AUL_MAX_Par + 1;
				C4Value *pTargetVal = pCurVal - C4AUL_MAX_Par;

				// Check for call to null
				if (!*pTargetVal)
					throw new C4AulExecError(pCurCtx->Obj, "object call: target is zero");

				// Get call target - "object" or "id" are allowed
				C4Object *pDestObj; C4Def *pDestDef;
				if (pTargetVal->ConvertTo(C4V_C4Object))
				{
					// object call
					pDestObj = pTargetVal->_getObj();
					pDestDef = pDestObj->Def;
				}
				else if (pTargetVal->ConvertTo(C4V_PropList) && pTargetVal->_getPropList())
				{
					// definition call
					pDestObj = NULL;
					pDestDef = pTargetVal->_getPropList()->GetDef();
					// definition must be known
					if (!pDestDef)
						throw new C4AulExecError(pCurCtx->Obj,
						                         FormatString("definition call: definition for %s not found", pTargetVal->_getPropList()->GetName()).getData());
				}
				else
					throw new C4AulExecError(pCurCtx->Obj,
					                         FormatString("object call: invalid target type %s, expected object or id", pTargetVal->GetTypeName()).getData());

				// Search function for given context
				const char * szFuncName = pCPos->Par.s->GetCStr();
				C4AulFunc * pFunc = pDestDef->Script.GetFuncRecursive(szFuncName);
				if (!pFunc && pCPos->bccType == AB_CALLFS)
				{
					PopValuesUntil(pTargetVal);
					pTargetVal->Set0();
					break;
				}

				// Function not found?
				if (!pFunc)
				{
					if (pDestObj)
						throw new C4AulExecError(pCurCtx->Obj,
						                         FormatString("object call: no function \"%s\" in object \"%s\"", szFuncName, pTargetVal->GetDataString().getData()).getData());
					else
						throw new C4AulExecError(pCurCtx->Obj,
						                         FormatString("definition call: no function \"%s\" in definition \"%s\"", szFuncName, pDestDef->GetName()).getData());
				}

				// Resolve overloads
				while (pFunc->OverloadedBy)
					pFunc = pFunc->OverloadedBy;

				// Save current position
				pCurCtx->CPos = pCPos;
				assert(pCurCtx->Func->GetCode() < pCPos);

				// Call function
				C4AulBCC *pNewCPos = Call(pFunc, pTargetVal, pPars, pDestObj, pDestDef);
				if (pNewCPos)
				{
					// Jump
					pCPos = pNewCPos;
					fJump = true;
				}

				break;
			}

			case AB_DEBUG:
#ifndef NOAULDEBUG
				if (pDebug) pDebug->DebugStep(pCPos);
#endif
				break;

			default:
				assert(false);
			}

			// Continue
			if (!fJump)
				pCPos++;
		}

	}
	catch (C4AulError *e)
	{
		// Show
		if(!e->shown) e->show();
		// Save current position
		assert(pCurCtx->Func->GetCode() < pCPos);
		pCurCtx->CPos = pCPos;
		// Unwind stack
		C4Value *pUntil = NULL;
		while (pCurCtx >= pOldCtx)
		{
			pCurCtx->dump(StdStrBuf(" by: "));
			pUntil = pCurCtx->Pars - 1;
			PopContext();
		}
		if (pUntil)
			PopValuesUntil(pUntil);
		// Pass?
		if (fPassErrors)
			throw;
		// Trace
		for (C4AulScriptContext *pCtx = pCurCtx; pCtx >= Contexts; pCtx--)
			pCtx->dump(StdStrBuf(" by: "));
		delete e;
	}

	// Return nothing
	return C4VNull;
}

C4AulBCC *C4AulExec::Call(C4AulFunc *pFunc, C4Value *pReturn, C4Value *pPars, C4Object *pObj, C4Def *pDef)
{

	// No object given? Use current context
	if (!pObj && !pDef)
	{
		assert(pCurCtx >= Contexts);
		pObj = pCurCtx->Obj;
		pDef = pCurCtx->Def;
	}

	// Convert parameters (typecheck)
	C4V_Type *pTypes = pFunc->GetParType();
	for (int i = 0; i < pFunc->GetParCount(); i++)
		if (!pPars[i].ConvertTo(pTypes[i]))
			throw new C4AulExecError(pCurCtx->Obj,
			                         FormatString("call to \"%s\" parameter %d: passed %s, but expected %s",
			                                      pFunc->Name, i + 1, pPars[i].GetTypeName(), GetC4VName(pTypes[i])
			                                     ).getData());

	// Script function?
	C4AulScriptFunc *pSFunc = pFunc->SFunc();
	if (pSFunc)
	{

		// Push variables
		C4Value *pVars = pCurVal + 1;
		PushNullVals(pSFunc->VarNamed.iSize);

		// Check context
		assert(!pSFunc->Owner->Def || pDef == pSFunc->Owner->Def);

		// Push a new context
		C4AulScriptContext ctx;
		ctx.Obj = pObj;
		ctx.Def = pDef;
		ctx.Caller = pCurCtx;
		ctx.Return = pReturn;
		ctx.Pars = pPars;
		ctx.Vars = pVars;
		ctx.Func = pSFunc;
		ctx.TemporaryScript = false;
		ctx.CPos = NULL;
		PushContext(ctx);

#ifndef NOAULDEBUG
		// Notify debugger
		if (C4AulDebug *pDebug = ::ScriptEngine.GetDebugger())
			pDebug->DebugStepIn(pSFunc->GetCode());
#endif

		// Jump to code
		return pSFunc->GetCode();
	}
	else
	{

		// Create new context
		C4AulContext CallCtx;
		CallCtx.Obj = pObj;
		CallCtx.Def = pDef;
		CallCtx.Caller = pCurCtx;

#ifdef DEBUGREC_SCRIPT
		if (Game.FrameCounter >= DEBUGREC_START_FRAME)
		{
			StdStrBuf sCallText;
			if (pObj)
				sCallText.AppendFormat("Object(%d): ", pObj->Number);
			sCallText.Append(pFunc->Name);
			sCallText.AppendChar('(');
			for (int i=0; i<C4AUL_MAX_Par; ++i)
			{
				if (i) sCallText.AppendChar(',');
				C4Value &rV = pPars[i];
				if (rV.GetType() == C4V_String)
				{
					C4String *s = rV.getStr();
					if (!s)
						sCallText.Append("(Snull)");
					else
					{
						sCallText.Append("\"");
						sCallText.Append(s->GetData());
						sCallText.Append("\"");
					}
				}
				else
					sCallText.Append(rV.GetDataString());
			}
			sCallText.AppendChar(')');
			sCallText.AppendChar(';');
			AddDbgRec(RCT_AulFunc, sCallText.getData(), sCallText.getLength()+1);
		}
#endif

		// Execute
#ifdef _DEBUG
		C4AulScriptContext *pCtx = pCurCtx;
#endif
		if (pReturn > pCurVal)
			PushValue(pFunc->Exec(&CallCtx, pPars, true));
		else
			pReturn->Set(pFunc->Exec(&CallCtx, pPars, true));
#ifdef _DEBUG
		assert(pCtx == pCurCtx);
#endif

#ifndef NOAULDEBUG
		// Notify debugger
		if (C4AulDebug *pDebug = ::ScriptEngine.GetDebugger())
		{
			// Make dummy context
			C4AulScriptContext ctx;
			ctx.Obj = pObj;
			ctx.Def = pDef;
			ctx.Caller = pCurCtx;
			ctx.Return = pReturn;
			ctx.Pars = pPars;
			ctx.Vars = pPars + pFunc->GetParCount();
			ctx.Func = pSFunc;
			ctx.TemporaryScript = false;
			ctx.CPos = NULL;
			pDebug->DebugStepOut(pCurCtx->CPos + 1, pCurCtx, pReturn);
		}
#endif

		// Remove parameters from stack
		PopValuesUntil(pReturn);

		// Continue
		return NULL;
	}

}

void C4AulStartTrace()
{
	AulExec.StartTrace();
}

void C4AulExec::StartTrace()
{
	if (iTraceStart < 0)
		iTraceStart = ContextStackSize();
}

void C4AulExec::StartProfiling(C4AulScript *pProfiledScript)
{
	// stop previous profiler run
	if (fProfiling) AbortProfiling();
	fProfiling = true;
	// resets profling times and starts recording the times
	this->pProfiledScript = pProfiledScript;
	time_t tNow = timeGetTime();
	tDirectExecStart = tNow; // in case profiling is started from DirectExec
	tDirectExecTotal = 0;
	pProfiledScript->ResetProfilerTimes();
	for (C4AulScriptContext *pCtx = Contexts; pCtx <= pCurCtx; ++pCtx)
		pCtx->tTime = tNow;
}

void C4AulExec::StopProfiling()
{
	// stop the profiler and displays results
	if (!fProfiling) return;
	fProfiling = false;
	// collect profiler times
	C4AulProfiler Profiler;
	Profiler.CollectEntry(NULL, tDirectExecTotal);
	pProfiledScript->CollectProfilerTimes(Profiler);
	Profiler.Show();
}

void C4AulExec::PushContext(const C4AulScriptContext &rContext)
{
	if (pCurCtx >= Contexts + MAX_CONTEXT_STACK - 1)
		throw new C4AulExecError(pCurCtx->Obj, "context stack overflow");
	*++pCurCtx = rContext;
	// Trace?
	if (iTraceStart >= 0)
	{
		StdStrBuf Buf("T");
		Buf.AppendChars('>', ContextStackSize() - iTraceStart);
		pCurCtx->dump(Buf);
	}
	// Profiler: Safe time to measure difference afterwards
	if (fProfiling) pCurCtx->tTime = timeGetTime();
}

void C4AulExec::PopContext()
{
	if (pCurCtx < Contexts)
		throw new C4AulExecError(pCurCtx->Obj, "internal error: context stack underflow");
	// Profiler adding up times
	if (fProfiling)
	{
		time_t dt = timeGetTime() - pCurCtx->tTime;
		if (dt && pCurCtx->Func)
			pCurCtx->Func->tProfileTime += dt;
	}
	// Trace done?
	if (iTraceStart >= 0)
	{
		if (ContextStackSize() <= iTraceStart)
		{
			iTraceStart = -1;
		}
	}
	if (pCurCtx->TemporaryScript)
		delete pCurCtx->Func->Owner;
	pCurCtx--;
}

void C4AulProfiler::StartProfiling(C4AulScript *pScript)
{
	AulExec.StartProfiling(pScript);
}

void C4AulProfiler::StopProfiling()
{
	AulExec.StopProfiling();
}

void C4AulProfiler::Abort()
{
	AulExec.AbortProfiling();
}

void C4AulProfiler::CollectEntry(C4AulScriptFunc *pFunc, time_t tProfileTime)
{
	// zero entries are not collected to have a cleaner list
	if (!tProfileTime) return;
	// add entry to list
	Entry e;
	e.pFunc = pFunc;
	e.tProfileTime = tProfileTime;
	Times.push_back(e);
}

void C4AulProfiler::Show()
{
	// sort by time
	std::sort(Times.rbegin(), Times.rend());
	// display them
	Log("Profiler statistics:");
	Log("==============================");
	typedef std::vector<Entry> EntryList;
	for (EntryList::iterator i = Times.begin(); i!=Times.end(); ++i)
	{
		Entry &e = (*i);
		LogF("%05dms\t%s", (int) e.tProfileTime, e.pFunc ? (e.pFunc->GetFullName().getData()) : "Direct exec");
	}
	Log("==============================");
	// done!
}


C4Value C4AulFunc::Exec(C4Object *pObj, C4AulParSet* pPars, bool fPassErrors)
{
	// construct a dummy caller context
	C4AulContext ctx;
	ctx.Obj = pObj;
	ctx.Def = pObj ? pObj->Def : NULL;
	ctx.Caller = NULL;
	// execute
	return Exec(&ctx, pPars ? pPars->Par : C4AulParSet().Par, fPassErrors);
}

C4Value C4AulScriptFunc::Exec(C4AulContext *pCtx, C4Value pPars[], bool fPassErrors)
{
	// handle easiest case first
	if (Owner->State != ASS_PARSED) return C4VNull;

	// execute
	return AulExec.Exec(this, pCtx->Obj, pPars, fPassErrors);

}


C4Value C4AulScriptFunc::Exec(C4Object *pObj, C4AulParSet *pPars, bool fPassErrors)
{

	// handle easiest case first
	if (Owner->State != ASS_PARSED) return C4VNull;

	// execute
	return AulExec.Exec(this, pObj, pPars ? pPars->Par : C4AulParSet().Par, fPassErrors);

}


C4Value C4AulDefFunc::Exec(C4AulContext *pCallerCtx, C4Value pPars[], bool fPassErrors)
{

	// Choose function call format to use
	if (Def->FunctionC4V2 != 0)

		// C4V function
		return Def->FunctionC4V2(pCallerCtx, pPars);

	if (Def->FunctionC4V != 0)

		// C4V function
		return Def->FunctionC4V(pCallerCtx, &pPars[0], &pPars[1], &pPars[2], &pPars[3], &pPars[4], &pPars[5], &pPars[6], &pPars[7], &pPars[8], &pPars[9]);

	// should never happen...
	return C4VNull;

}

C4Value C4AulScript::DirectExec(C4Object *pObj, const char *szScript, const char *szContext, bool fPassErrors, enum Strict Strict, C4AulScriptContext* context)
{
#ifdef DEBUGREC_SCRIPT
	AddDbgRec(RCT_DirectExec, szScript, strlen(szScript)+1);
	int32_t iObjNumber = pObj ? pObj->Number : -1;
	AddDbgRec(RCT_DirectExec, &iObjNumber, sizeof(int32_t));
#endif
	// profiler
	AulExec.StartDirectExec();
	// Create a new temporary script as child of this script
	C4AulScript* pScript = new C4AulScript();
	pScript->Script.Copy(szScript);
	pScript->ScriptName = FormatString("%s in %s", szContext, ScriptName.getData());
	pScript->Strict = Strict;
	pScript->Temporary = true;
	pScript->State = ASS_LINKED;
	pScript->stringTable = stringTable;
	if (pObj)
	{
		pScript->Def = pObj->Def;
		pScript->LocalNamed = pObj->Def->Script.LocalNamed;
	}
	else
	{
		pScript->Def = NULL;
	}
	pScript->ClearCode();
	pScript->Reg2List(Engine, this);
	// Add a new function
	C4AulScriptFunc *pFunc = new C4AulScriptFunc(pScript, "");
	pFunc->Script = pScript->Script.getData();
	pFunc->pOrgScript = pScript;
	// Parse function
	try
	{
		pScript->ParseFn(pFunc, true, context);
	}
	catch (C4AulError *ex)
	{
		ex->show();
		delete ex;
		delete pFunc;
		delete pScript;
		return C4VNull;
	}
	pFunc->CodePos = 1;
	pScript->State = ASS_PARSED;
	// Execute. The TemporaryScript-parameter makes sure the script will be deleted later on.
	C4Value vRetVal(AulExec.Exec(pFunc, pObj, NULL, fPassErrors, true));
	// profiler
	AulExec.StopDirectExec();
	return vRetVal;
}

void C4AulScript::ResetProfilerTimes()
{
	// zero all profiler times of owned functions
	C4AulScriptFunc *pSFunc;
	for (C4AulFunc *pFn = Func0; pFn; pFn = pFn->Next)
		if ((pSFunc = pFn->SFunc()))
			pSFunc->tProfileTime = 0;
	// reset sub-scripts
	for (C4AulScript *pScript = Child0; pScript; pScript = pScript->Next)
		pScript->ResetProfilerTimes();
}

void C4AulScript::CollectProfilerTimes(class C4AulProfiler &rProfiler)
{
	// collect all profiler times of owned functions
	C4AulScriptFunc *pSFunc;
	for (C4AulFunc *pFn = Func0; pFn; pFn = pFn->Next)
		if ((pSFunc = pFn->SFunc()))
			rProfiler.CollectEntry(pSFunc, pSFunc->tProfileTime);
	// collect sub-scripts
	for (C4AulScript *pScript = Child0; pScript; pScript = pScript->Next)
		pScript->CollectProfilerTimes(rProfiler);
}
