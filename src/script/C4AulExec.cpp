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
// executes script functions

#include "C4Include.h"
#include "script/C4AulExec.h"

#include "control/C4Record.h"
#include "object/C4Def.h"
#include "object/C4Object.h"
#include "script/C4Aul.h"
#include "script/C4AulScriptFunc.h"
#include "script/C4AulDebug.h"
#include "script/C4ScriptHost.h"

C4AulExec AulExec;

C4AulExecError::C4AulExecError(const char *szError)
{
	assert(szError);
	// direct error message string
	sMessage.Copy(szError ? szError : "(no error message)");
}

StdStrBuf C4AulScriptContext::ReturnDump(StdStrBuf Dump)
{
	if (!Func)
		return StdStrBuf("");
	// Context
	if (Obj && Obj->Status)
	{
		C4Value ObjVal(Obj);
		Dump.Append(ObjVal.GetDataString(0));
		Dump.Append("->");
	}
	bool fDirectExec = !Func->GetName();
	if (!fDirectExec)
	{
		// Function name
		Dump.Append(Func->GetName());
		// Parameters
		Dump.AppendChar('(');
		int iNullPars = 0;
		for (int i = 0; i < Func->GetParCount(); i++)
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
		Dump.Append(Func->Parent->GetDataString());
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

void C4AulExec::LogCallStack()
{
	for (C4AulScriptContext *pCtx = pCurCtx; pCtx >= Contexts; pCtx--)
		pCtx->dump(StdStrBuf(" by: "));
}

C4String *C4AulExec::FnTranslate(C4PropList * _this, C4String *text)
{
#define ReturnIfTranslationAvailable(script, key) do \
{ \
	const auto &s = script; \
	const auto &k = key; \
	if (s) \
	{ \
		try \
		{ \
			return ::Strings.RegString(s->Translate(k).c_str()); \
		} \
		catch (C4LangStringTable::NoSuchTranslation &) {} \
	} \
} while(0)

	if (!text || text->GetData().isNull()) return nullptr;
	// Find correct script: translations of the context if possible, containing script as fallback
	if (_this && _this->GetDef())
		ReturnIfTranslationAvailable(&(_this->GetDef()->Script), text->GetCStr());
	ReturnIfTranslationAvailable(AulExec.pCurCtx[0].Func->pOrgScript, text->GetCStr());

	// No translation available, log
	DebugLogF(R"(WARNING: Translate: no translation for string "%s")", text->GetCStr());
	// Trace
	AulExec.LogCallStack();
	return text;
#undef ReturnIfTranslationAvailable
}

bool C4AulExec::FnLogCallStack(C4PropList * _this)
{
	AulExec.LogCallStack();
	return true;
}

void C4AulExec::ClearPointers(C4Object * obj)
{
	for (C4AulScriptContext *pCtx = pCurCtx; pCtx >= Contexts; pCtx--)
	{
		if (pCtx->Obj == obj)
			pCtx->Obj = nullptr;
	}
}

C4Value C4AulExec::Exec(C4AulScriptFunc *pSFunc, C4PropList * p, C4Value *pnPars, bool fPassErrors)
{
	// Save start context
	C4AulScriptContext *pOldCtx = pCurCtx;
	C4Value *pPars = pCurVal + 1;
	try
	{
		// Push parameters
		assert(pnPars);
		for (int i = 0; i < pSFunc->GetParCount(); i++)
			PushValue(pnPars[i]);

		// Push a new context
		C4AulScriptContext ctx;
		ctx.tTime = 0;
		ctx.Obj = p;
		ctx.Return = nullptr;
		ctx.Pars = pPars;
		ctx.Func = pSFunc;
		ctx.CPos = nullptr;
		PushContext(ctx);

		// Execute
		return Exec(pSFunc->GetCode());
	}
	catch (C4AulError &e)
	{
		if (!fPassErrors)
			::ScriptEngine.GetErrorHandler()->OnError(e.what());
		// Unwind stack
		// TODO: The stack dump should be passed to the error handler somehow
		while (pCurCtx > pOldCtx)
		{
			pCurCtx->dump(StdStrBuf(" by: "));
			PopContext();
		}
		PopValuesUntil(pPars - 1);
		// Pass?
		if (fPassErrors)
			throw;
		// Trace
		LogCallStack();
	}

	// Return nothing
	return C4VNull;
}

C4Value C4AulExec::Exec(C4AulBCC *pCPos)
{
	try
	{

		for (;;)
		{

			bool fJump = false;
			switch (pCPos->bccType)
			{
			case AB_INT:
				PushInt(pCPos->Par.i);
				break;

			case AB_BOOL:
				PushBool(!!pCPos->Par.i);
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

			case AB_CFUNCTION:
				PushFunction(pCPos->Par.f);
				break;

			case AB_NIL:
				PushValue(C4VNull);
				break;

			case AB_DUP:
				PushValue(pCurVal[pCPos->Par.i]);
				break;
			case AB_STACK_SET:
				pCurVal[pCPos->Par.i] = pCurVal[0];
				break;
			case AB_POP_TO:
				pCurVal[pCPos->Par.i] = pCurVal[0];
				PopValue();
				break;

			case AB_EOFN:
				throw C4AulExecError("internal error: function didn't return");

			case AB_ERR:
				if (pCPos->Par.s)
					throw C4AulExecError((std::string("syntax error: ") + pCPos->Par.s->GetCStr()).c_str());
				else
					throw C4AulExecError("syntax error: see above for details");

			case AB_DUP_CONTEXT:
				PushValue(AulExec.GetContext(AulExec.GetContextDepth()-2)->Pars[pCPos->Par.i]);
				break;

			case AB_LOCALN:
				if (!pCurCtx->Obj)
					throw C4AulExecError("can't access local variables without this");
				PushNullVals(1);
				pCurCtx->Obj->GetPropertyByS(pCPos->Par.s, pCurVal);
				break;
			case AB_LOCALN_SET:
				if (!pCurCtx->Obj)
					throw C4AulExecError("can't access local variables without this");
				if (pCurCtx->Obj->IsFrozen())
					throw C4AulExecError("local variable: this is readonly");
				pCurCtx->Obj->SetPropertyByS(pCPos->Par.s, pCurVal[0]);
				break;

			case AB_PROP:
				if (!pCurVal->CheckConversion(C4V_PropList))
					throw C4AulExecError(FormatString("proplist access: proplist expected, got %s", pCurVal->GetTypeName()).getData());
				if (!pCurVal->_getPropList()->GetPropertyByS(pCPos->Par.s, pCurVal))
					pCurVal->Set0();
				break;
			case AB_PROP_SET:
			{
				C4Value *pPropList = pCurVal - 1;
				if (!pPropList->CheckConversion(C4V_PropList))
					throw C4AulExecError(FormatString("proplist write: proplist expected, got %s", pPropList->GetTypeName()).getData());
				if (pPropList->_getPropList()->IsFrozen())
					throw C4AulExecError("proplist write: proplist is readonly");
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
				CheckOpPar(C4V_Int, "~");
				pCurVal->SetInt(~pCurVal->_getInt());
				break;
			case AB_Not:  // !
				pCurVal->SetBool(!pCurVal->getBool());
				break;
			case AB_Neg:  // -
				CheckOpPar(C4V_Int, "-");
				pCurVal->SetInt(-pCurVal->_getInt());
				break;
			case AB_Inc: // ++
				CheckOpPar(C4V_Int, "++");
				pCurVal->SetInt(pCurVal->_getInt() + 1);
				break;
			case AB_Dec: // --
				CheckOpPar(C4V_Int, "--");
				pCurVal->SetInt(pCurVal->_getInt() - 1);
				break;
			// postfix
			case AB_Pow:  // **
			{
				CheckOpPars(C4V_Int, C4V_Int, "**");
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetInt(Pow(pPar1->_getInt(), pPar2->_getInt()));
				PopValue();
				break;
			}
			case AB_Div:  // /
			{
				CheckOpPars(C4V_Int, C4V_Int, "/");
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				if (!pPar2->_getInt())
					throw C4AulExecError("division by zero");
				// INT_MIN/-1 cannot be represented in an int and would cause an uncaught exception
				if (pPar1->_getInt()==INT32_MIN && pPar2->_getInt()==-1)
					throw C4AulExecError("division overflow");
				pPar1->SetInt(pPar1->_getInt() / pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_Mul:  // *
			{
				CheckOpPars(C4V_Int, C4V_Int, "*");
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetInt(pPar1->_getInt() * pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_Mod:  // %
			{
				CheckOpPars(C4V_Int, C4V_Int, "%");
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				// INT_MIN%-1 cannot be represented in an int and would cause an uncaught exception
				if (pPar1->_getInt()==INT32_MIN && pPar2->_getInt()==-1)
					throw C4AulExecError("modulo division overflow");
				if (pPar2->_getInt())
					pPar1->SetInt(pPar1->_getInt() % pPar2->_getInt());
				else
					pPar1->Set0();
				PopValue();
				break;
			}
			case AB_Sub:  // -
			{
				CheckOpPars(C4V_Int, C4V_Int, "-");
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetInt(pPar1->_getInt() - pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_Sum:  // +
			{
				CheckOpPars(C4V_Int, C4V_Int, "+");
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetInt(pPar1->_getInt() + pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_LeftShift:  // <<
			{
				CheckOpPars(C4V_Int, C4V_Int, "<<");
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetInt(pPar1->_getInt() << pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_RightShift: // >>
			{
				CheckOpPars(C4V_Int, C4V_Int, ">>");
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetInt(pPar1->_getInt() >> pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_LessThan: // <
			{
				CheckOpPars(C4V_Int, C4V_Int, "<");
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetBool(pPar1->_getInt() < pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_LessThanEqual:  // <=
			{
				CheckOpPars(C4V_Int, C4V_Int, "<=");
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetBool(pPar1->_getInt() <= pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_GreaterThan:  // >
			{
				CheckOpPars(C4V_Int, C4V_Int, ">");
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetBool(pPar1->_getInt() > pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_GreaterThanEqual: // >=
			{
				CheckOpPars(C4V_Int, C4V_Int, ">=");
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetBool(pPar1->_getInt() >= pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_Equal:  // ==
			{
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetBool(pPar1->IsIdenticalTo(*pPar2));
				PopValue();
				break;
			}
			case AB_NotEqual: // !=
			{
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetBool(!pPar1->IsIdenticalTo(*pPar2));
				PopValue();
				break;
			}
			case AB_BitAnd: // &
			{
				CheckOpPars(C4V_Int, C4V_Int, "&");
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetInt(pPar1->_getInt() & pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_BitXOr: // ^
			{
				CheckOpPars(C4V_Int, C4V_Int, "^");
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetInt(pPar1->_getInt() ^ pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_BitOr:  // |
			{
				CheckOpPars(C4V_Int, C4V_Int, "|");
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetInt(pPar1->_getInt() | pPar2->_getInt());
				PopValue();
				break;
			}

			case AB_NEW_ARRAY:
			{
				// Create array
				C4ValueArray *pArray = new C4ValueArray(pCPos->Par.i);

				// Pop values from stack
				for (int i = 0; i < pCPos->Par.i; i++)
					(*pArray)[i] = pCurVal[i - pCPos->Par.i + 1];

				// Push array
				PopValues(pCPos->Par.i);
				PushArray(pArray);

				break;
			}

			case AB_NEW_PROPLIST:
			{
				C4PropList * pPropList = C4PropList::New();

				for (int i = 0; i < pCPos->Par.i; i++)
					pPropList->SetPropertyByS(pCurVal[-2 * i - 1]._getStr(), pCurVal[-2 * i]);

				PopValues(pCPos->Par.i * 2);
				PushPropList(pPropList);
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
					assert(pStruct->GetType() == C4V_PropList);
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
					if (pStruct->_getArray()->IsFrozen())
						throw C4AulExecError("array write: array is readonly");
					pStruct->_getArray()->SetItem(pIndex->_getInt(), *pValue);
				}
				else
				{
					assert(pStruct->GetType() == C4V_PropList);
					C4PropList *pPropList = pStruct->_getPropList();
					if (pPropList->IsFrozen())
						throw C4AulExecError("proplist write: proplist is readonly");
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
				if (!Array.CheckConversion(C4V_Array))
					throw C4AulExecError(FormatString("array slice: can't access %s as an array", Array.GetTypeName()).getData());
				if (!StartIndex.CheckConversion(C4V_Int))
					throw C4AulExecError(FormatString("array slice: start index of type %s, int expected", StartIndex.GetTypeName()).getData());
				if (!EndIndex.CheckConversion(C4V_Int))
					throw C4AulExecError(FormatString("array slice: end index of type %s, int expected", EndIndex.GetTypeName()).getData());

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
				if (!Array.CheckConversion(C4V_Array))
					throw C4AulExecError(FormatString("array slice: can't access %s as an array", Array.GetTypeName()).getData());
				if (!StartIndex.CheckConversion(C4V_Int))
					throw C4AulExecError(FormatString("array slice: start index of type %s, int expected", StartIndex.GetTypeName()).getData());
				if (!EndIndex.CheckConversion(C4V_Int))
					throw C4AulExecError(FormatString("array slice: end index of type %s, int expected", EndIndex.GetTypeName()).getData());

				C4ValueArray *pArray = Array._getArray();
				if (pArray->IsFrozen()) throw C4AulExecError("array write: array is readonly");
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

			case AB_JUMPNNIL: // ??
			{
				if (pCurVal[0].GetType() != C4V_Nil)
				{
					fJump = true;
					pCPos += pCPos->Par.i;
				}
				else
				{
					PopValue();
				}
				break;
			}

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
					LogF("%s%s returned %s", Buf.getData(), pCurCtx->Func->GetName(), pCurVal->GetDataString().getData());
				}

				C4Value *pReturn = pCurCtx->Return;

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
				assert(pCurCtx->Func->GetCode() <= pCPos);
				// Do the call
				C4AulBCC *pJump = Call(pFunc, pPars, pPars, nullptr);
				if (pJump)
				{
					pCPos = pJump;
					fJump = true;
				}
				break;
			}

			case AB_PAR:
				if (!pCurVal->CheckConversion(C4V_Int))
					throw C4AulExecError(FormatString("Par: index of type %s, int expected", pCurVal->GetTypeName()).getData());
				// Push reference to parameter on the stack
				if (pCurVal->_getInt() >= 0 && pCurVal->_getInt() < pCurCtx->Func->GetParCount())
					pCurVal->Set(pCurCtx->Pars[pCurVal->_getInt()]);
				else
					pCurVal->Set0();
				break;

			case AB_THIS:
				if (!pCurCtx->Obj || !pCurCtx->Obj->Status)
					PushNullVals(1);
				else
					PushPropList(pCurCtx->Obj);
				break;

			case AB_FOREACH_NEXT:
			{
				// This should always hold
				assert(pCurVal->CheckConversion(C4V_Int));
				int iItem = pCurVal->_getInt();
				// Check array the first time only
				if (!iItem)
				{
					if (!pCurVal[-1].CheckConversion(C4V_Array))
						throw C4AulExecError(FormatString("for: array expected, but got %s", pCurVal[-1].GetTypeName()).getData());
				}
				C4ValueArray *pArray = pCurVal[-1]._getArray();
				// No more entries?
				if (pCurVal->_getInt() >= pArray->GetSize())
					break;
				// Get next
				pCurVal[pCPos->Par.i] = pArray->GetItem(iItem);
				// Save position
				pCurVal->SetInt(iItem + 1);
				// Jump over next instruction
				pCPos += 2;
				fJump = true;
				break;
			}

			case AB_CALL:
			case AB_CALLFS:
			{

				C4Value *pPars = pCurVal - C4AUL_MAX_Par + 1;
				C4Value *pTargetVal = pCurVal - C4AUL_MAX_Par;

				C4PropList *pDest;
				if (pTargetVal->CheckConversion(C4V_PropList))
				{
					pDest = pTargetVal->_getPropList();
				}
				else
					throw C4AulExecError(FormatString("'->': invalid target type %s, expected proplist", pTargetVal->GetTypeName()).getData());

				// Search function for given context
				C4AulFunc * pFunc = pDest->GetFunc(pCPos->Par.s);
				if (!pFunc && pCPos->bccType == AB_CALLFS)
				{
					PopValuesUntil(pTargetVal);
					pTargetVal->Set0();
					break;
				}

				// Function not found?
				if (!pFunc)
					throw C4AulExecError(FormatString(R"('->': no function "%s" in object "%s")", pCPos->Par.s->GetCStr(), pTargetVal->GetDataString().getData()).getData());

				// Save current position
				pCurCtx->CPos = pCPos;
				assert(pCurCtx->Func->GetCode() <= pCPos);

				// adjust parameter count
				if (pCurVal + 1 - pPars > pFunc->GetParCount())
					PopValues(pCurVal + 1 - pPars - pFunc->GetParCount());
				else
					PushNullVals(pFunc->GetParCount() - (pCurVal + 1 - pPars));

				// Call function
				C4AulBCC *pNewCPos = Call(pFunc, pTargetVal, pPars, pDest);
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
				if (C4AulDebug *pDebug = C4AulDebug::GetDebugger())
					pDebug->DebugStep(pCPos, pCurVal);
#endif
				break;
			}

			// Continue
			if (!fJump)
				pCPos++;
		}

	}
	catch (C4AulError &)
	{
		// Save current position
		assert(pCurCtx->Func->GetCode() <= pCPos);
		pCurCtx->CPos = pCPos;
		throw;
	}
}

C4AulBCC *C4AulExec::Call(C4AulFunc *pFunc, C4Value *pReturn, C4Value *pPars, C4PropList *pContext)
{
	// No object given? Use current context
	if (!pContext)
	{
		assert(pCurCtx >= Contexts);
		pContext = pCurCtx->Obj;
	}

	pFunc->CheckParTypes(pPars, true);

	// Script function?
	C4AulScriptFunc *pSFunc = pFunc->SFunc();
	if (pSFunc)
	{
		// Push a new context
		C4AulScriptContext ctx;
		ctx.Obj = pContext;
		if (ctx.Obj && !ctx.Obj->Status)
			throw C4AulExecError("using removed object");
		ctx.Return = pReturn;
		ctx.Pars = pPars;
		ctx.Func = pSFunc;
		ctx.CPos = nullptr;
		PushContext(ctx);

		// Jump to code
		return pSFunc->GetCode();
	}
	else
	{
		if (pContext && !pContext->Status)
			throw C4AulExecError("using removed object");

		if (DEBUGREC_SCRIPT && Config.General.DebugRec)
		{
			StdStrBuf sCallText;
			if (pContext && pContext->GetObject())
				sCallText.AppendFormat("Object(%d): ", pContext->GetObject()->Number);
			sCallText.Append(pFunc->GetName());
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
						sCallText.Append(R"(")");
						sCallText.Append(s->GetData());
						sCallText.Append(R"(")");
					}
				}
				else
					sCallText.Append(rV.GetDataString());
			}
			sCallText.AppendChar(')');
			sCallText.AppendChar(';');
			AddDbgRec(RCT_AulFunc, sCallText.getData(), sCallText.getLength()+1);
		}

		// Execute
#ifdef _DEBUG
		C4AulScriptContext *pCtx = pCurCtx;
#endif
		if (pReturn > pCurVal)
			PushValue(pFunc->Exec(pContext, pPars, true));
		else
			pReturn->Set(pFunc->Exec(pContext, pPars, true));
#ifdef _DEBUG
		assert(pCtx == pCurCtx);
#endif

		// Remove parameters from stack
		PopValuesUntil(pReturn);

		// Continue
		return nullptr;
	}

}

void C4AulExec::StartTrace()
{
	if (iTraceStart < 0)
		iTraceStart = ContextStackSize();
}

void C4AulExec::StartProfiling(C4ScriptHost *pProfiledScript)
{
	// stop previous profiler run
	if (fProfiling) StopProfiling();
	fProfiling = true;
	// resets profling times and starts recording the times
	this->pProfiledScript = pProfiledScript;
	C4TimeMilliseconds tNow = C4TimeMilliseconds::Now();
	tDirectExecStart = tNow; // in case profiling is started from DirectExec
	tDirectExecTotal = 0;
	for (C4AulScriptContext *pCtx = Contexts; pCtx <= pCurCtx; ++pCtx)
		pCtx->tTime = tNow;
}

void C4AulExec::PushContext(const C4AulScriptContext &rContext)
{
	if (pCurCtx >= Contexts + MAX_CONTEXT_STACK - 1)
		throw C4AulExecError("context stack overflow");
	*++pCurCtx = rContext;
	// Trace?
	if (iTraceStart >= 0)
	{
		StdStrBuf Buf("T");
		Buf.AppendChars('>', ContextStackSize() - iTraceStart);
		pCurCtx->dump(Buf);
	}
	// Profiler: Safe time to measure difference afterwards
	if (fProfiling) pCurCtx->tTime = C4TimeMilliseconds::Now();
}

void C4AulExec::PopContext()
{
	if (pCurCtx < Contexts)
		throw C4AulExecError("internal error: context stack underflow");
	// Profiler adding up times
	if (fProfiling)
	{
		uint32_t dt = C4TimeMilliseconds::Now() - pCurCtx->tTime;
		if (pCurCtx->Func)
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
	pCurCtx--;
}

void C4AulProfiler::StartProfiling(C4ScriptHost *pScript)
{
	AulExec.StartProfiling(pScript);
	if(pScript)
		ResetTimes(pScript->GetPropList());
	else
		ResetTimes();
}

void C4AulProfiler::StopProfiling()
{
	if (!AulExec.IsProfiling()) return;
	AulExec.StopProfiling();
	// collect profiler times
	C4AulProfiler Profiler;
	Profiler.CollectEntry(nullptr, AulExec.tDirectExecTotal);
	if(AulExec.pProfiledScript)
		Profiler.CollectTimes(AulExec.pProfiledScript->GetPropList());
	else
		Profiler.CollectTimes();
	Profiler.Show();
}

void C4AulProfiler::CollectEntry(C4AulScriptFunc *pFunc, uint32_t tProfileTime)
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
	for (auto & e : Times)
	{
		LogF("%05ums\t%s", e.tProfileTime, e.pFunc ? (e.pFunc->GetFullName().getData()) : "Direct exec");
	}
	Log("==============================");
	// done!
}

C4Value C4AulExec::DirectExec(C4PropList *p, const char *szScript, const char *szContext, bool fPassErrors, C4AulScriptContext* context, bool parse_function)
{
	if (DEBUGREC_SCRIPT && Config.General.DebugRec)
	{
		AddDbgRec(RCT_DirectExec, szScript, strlen(szScript)+1);
		int32_t iObjNumber = p && p->GetPropListNumbered() ? p->GetPropListNumbered()->Number : -1;
		AddDbgRec(RCT_DirectExec, &iObjNumber, sizeof(int32_t));
	}
	// profiler
	StartDirectExec();
	C4PropListStatic * script = ::GameScript.GetPropList();
	if (p && p->IsStatic())
		script = p->IsStatic();
	else if (p && p->GetDef())
		script = p->GetDef();
	// Add a new function
	auto pFunc = std::make_unique<C4AulScriptFunc>(script, nullptr, nullptr, szScript);
	// Parse function
	try
	{
		if (parse_function)
		{
			// Expect a full function (e.g. "func foo() { return bar(); }")
			pFunc->ParseDirectExecFunc(&::ScriptEngine, context);
		}
		else
		{
			// Expect a single statement (e.g. "bar()")
			pFunc->ParseDirectExecStatement(&::ScriptEngine, context);
		}
		C4AulParSet Pars;
		C4Value vRetVal(Exec(pFunc.get(), p, Pars.Par, fPassErrors));
		// profiler
		StopDirectExec();
		return vRetVal;
	}
	catch (C4AulError &ex)
	{
		if(fPassErrors)
			throw;
		::ScriptEngine.GetErrorHandler()->OnError(ex.what());
		LogCallStack();
		StopDirectExec();
		return C4VNull;
	}
}

void C4AulProfiler::ResetTimes(C4PropListStatic * p)
{
	// zero all profiler times of owned functions
	C4AulScriptFunc *pSFunc;
	for (C4String *pFn = p->EnumerateOwnFuncs(); pFn; pFn = p->EnumerateOwnFuncs(pFn))
		if ((pSFunc = p->GetFunc(pFn)->SFunc()))
			pSFunc->tProfileTime = 0;
}

void C4AulProfiler::CollectTimes(C4PropListStatic * p)
{
	// collect all profiler times of owned functions
	C4AulScriptFunc *pSFunc;
	for (C4String *pFn = p->EnumerateOwnFuncs(); pFn; pFn = p->EnumerateOwnFuncs(pFn))
		if ((pSFunc = p->GetFunc(pFn)->SFunc()))
			CollectEntry(pSFunc, pSFunc->tProfileTime);
}

void C4AulProfiler::ResetTimes()
{
	// zero all profiler times of owned functions
	ResetTimes(::ScriptEngine.GetPropList());
	// reset sub-scripts
	for (C4ScriptHost *pScript = ::ScriptEngine.Child0; pScript; pScript = pScript->Next)
		ResetTimes(pScript->GetPropList());
}

void C4AulProfiler::CollectTimes()
{
	// collect all profiler times of owned functions
	CollectTimes(::ScriptEngine.GetPropList());
	// collect sub-scripts
	for (C4ScriptHost *pScript = ::ScriptEngine.Child0; pScript; pScript = pScript->Next)
		CollectTimes(pScript->GetPropList());
}
