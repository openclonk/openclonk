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

#include "C4Include.h"
#include "script/C4AulCompiler.h"

#include "script/C4Aul.h"
#include "script/C4AulScriptFunc.h"

static int GetStackValue(C4AulBCCType eType, intptr_t X)
{
	switch (eType)
	{
	case AB_INT:
	case AB_BOOL:
	case AB_STRING:
	case AB_CPROPLIST:
	case AB_CARRAY:
	case AB_CFUNCTION:
	case AB_NIL:
	case AB_LOCALN:
	case AB_GLOBALN:
	case AB_DUP:
	case AB_DUP_CONTEXT:
	case AB_THIS:
		return 1;

	case AB_Pow:
	case AB_Div:
	case AB_Mul:
	case AB_Mod:
	case AB_Sub:
	case AB_Sum:
	case AB_LeftShift:
	case AB_RightShift:
	case AB_LessThan:
	case AB_LessThanEqual:
	case AB_GreaterThan:
	case AB_GreaterThanEqual:
	case AB_Equal:
	case AB_NotEqual:
	case AB_BitAnd:
	case AB_BitXOr:
	case AB_BitOr:
	case AB_PROP_SET:
	case AB_ARRAYA:
	case AB_CONDN:
	case AB_COND:
	case AB_POP_TO:
	case AB_RETURN:
		// JUMPAND/JUMPOR/JUMPNNIL are special: They either jump over instructions adding one to the stack
		// or decrement the stack. Thus, for stack counting purposes, they decrement.
	case AB_JUMPAND:
	case AB_JUMPOR:
	case AB_JUMPNNIL:
		return -1;

	case AB_FUNC:
		return -reinterpret_cast<C4AulFunc *>(X)->GetParCount() + 1;

	case AB_CALL:
	case AB_CALLFS:
		return -C4AUL_MAX_Par;

	case AB_STACK_SET:
	case AB_LOCALN_SET:
	case AB_PROP:
	case AB_GLOBALN_SET:
	case AB_Inc:
	case AB_Dec:
	case AB_BitNot:
	case AB_Not:
	case AB_Neg:
	case AB_PAR:
	case AB_FOREACH_NEXT:
	case AB_ERR:
	case AB_EOFN:
	case AB_JUMP:
	case AB_DEBUG:
		return 0;

	case AB_STACK:
		return X;

	case AB_NEW_ARRAY:
		return -X + 1;

	case AB_NEW_PROPLIST:
		return -X * 2 + 1;

	case AB_ARRAYA_SET:
	case AB_ARRAY_SLICE:
		return -2;

	case AB_ARRAY_SLICE_SET:
		return -3;
	}
	assert(0 && "GetStackValue: unexpected bytecode not handled");
	return 0;
}

int C4AulCompiler::AddVarAccess(const char * TokenSPos, C4AulBCCType eType, intptr_t varnum)
{
	return AddBCC(TokenSPos, eType, 1 + varnum - (stack_height + Fn->VarNamed.iSize));
}

int C4AulCompiler::AddBCC(const char * TokenSPos, C4AulBCCType eType, intptr_t X)
{
	// Track stack size
	stack_height += GetStackValue(eType, X);

	// Use stack operation instead of 0-Any (enable optimization)
	if (eType == AB_NIL)
	{
		eType = AB_STACK;
		X = 1;
	}

	// Join checks only if it's not a jump target
	if (!at_jump_target && Fn->GetLastCode())
	{
		C4AulBCC *pCPos1 = Fn->GetLastCode();

		// Skip noop stack operation
		if (eType == AB_STACK && X == 0)
		{
			return Fn->GetCodePos() - 1;
		}

		// Join together stack operations
		if (eType == AB_STACK && pCPos1->bccType == AB_STACK &&
			(X <= 0 || pCPos1->Par.i >= 0))
		{
			pCPos1->Par.i += X;
			// Empty? Remove it. This relies on the parser not issuing
			// multiple negative stack operations consecutively, as
			// that could result in removing a jump target bytecode.
			if (!pCPos1->Par.i)
				Fn->RemoveLastBCC();
			return Fn->GetCodePos() - 1;
		}

		// Prune unneeded Incs / Decs
		if (eType == AB_STACK && X < 0 && (pCPos1->bccType == AB_Inc || pCPos1->bccType == AB_Dec))
		{
			if (!pCPos1->Par.X)
			{
				pCPos1->bccType = eType;
				pCPos1->Par.i = X;
				return Fn->GetCodePos() - 1;
			}
			else
			{
				// If it was a result modifier, we can safely remove it knowing that it was neither
				// the first chunk nor a jump target. We can therefore apply additional optimizations.
				Fn->RemoveLastBCC();
				pCPos1--;
			}
		}

		// Join STACK_SET + STACK -1 to POP_TO (equivalent)
		if (eType == AB_STACK && X == -1 && pCPos1->bccType == AB_STACK_SET)
		{
			pCPos1->bccType = AB_POP_TO;
			return Fn->GetCodePos() - 1;
		}

		// Join POP_TO + DUP to AB_STACK_SET if both target the same slot
		if (eType == AB_DUP && pCPos1->bccType == AB_POP_TO && X == pCPos1->Par.i + 1)
		{
			pCPos1->bccType = AB_STACK_SET;
			return Fn->GetCodePos() - 1;
		}

		// Reduce some constructs like SUM + INT 1 to INC or DEC
		if ((eType == AB_Sum || eType == AB_Sub) &&
			pCPos1->bccType == AB_INT &&
			(pCPos1->Par.i == 1 || pCPos1->Par.i == -1))
		{
			if ((pCPos1->Par.i > 0) == (eType == AB_Sum))
				pCPos1->bccType = AB_Inc;
			else
				pCPos1->bccType = AB_Dec;
			pCPos1->Par.i = X;
			return Fn->GetCodePos() - 1;
		}

		// Reduce Not + CONDN to COND, Not + COND to CONDN
		if ((eType == AB_CONDN || eType == AB_COND) && pCPos1->bccType == AB_Not)
		{
			pCPos1->bccType = eType == AB_CONDN ? AB_COND : AB_CONDN;
			pCPos1->Par.i = X + 1;
			return Fn->GetCodePos() - 1;
		}

		// Join AB_STRING + AB_ARRAYA to AB_PROP
		if (eType == AB_ARRAYA && pCPos1->bccType == AB_STRING)
		{
			pCPos1->bccType = AB_PROP;
			return Fn->GetCodePos() - 1;
		}

		// Join AB_INT + AB_Neg to AB_INT
		if (eType == AB_Neg && pCPos1->bccType == AB_INT)
		{
			pCPos1->Par.i *= -1;
			return Fn->GetCodePos() - 1;
		}
	}

	// Add
	Fn->AddBCC(eType, X, TokenSPos);

	// Reset jump flag
	at_jump_target = false;

	return Fn->GetCodePos() - 1;
}

void C4AulCompiler::RemoveLastBCC()
{
	// Security: This is unsafe on anything that might get optimized away
	C4AulBCC *pBCC = Fn->GetLastCode();
	assert(pBCC->bccType != AB_STACK && pBCC->bccType != AB_STACK_SET && pBCC->bccType != AB_POP_TO);
	// Correct stack
	stack_height -= GetStackValue(pBCC->bccType, pBCC->Par.X);
	// Remove
	Fn->RemoveLastBCC();
}

C4V_Type C4AulCompiler::GetLastRetType(C4AulScriptEngine * Engine, C4V_Type to)
{
	C4V_Type from;
	switch (Fn->GetLastCode()->bccType)
	{
	case AB_INT: from = C4V_Int; break;
	case AB_STRING: from = C4V_String; break;
	case AB_NEW_ARRAY: case AB_CARRAY: case AB_ARRAY_SLICE: from = C4V_Array; break;
	case AB_CFUNCTION: from = C4V_Function; break;
	case AB_NEW_PROPLIST: case AB_CPROPLIST: from = C4V_PropList; break;
	case AB_BOOL: from = C4V_Bool; break;
	case AB_FUNC:
		from = Fn->GetLastCode()->Par.f->GetRetType(); break;
	case AB_CALL: case AB_CALLFS:
		{
			C4String * pName = Fn->GetLastCode()->Par.s;
			C4AulFunc * pFunc2 = Engine->GetFirstFunc(pName->GetCStr());
			bool allwarn = true;
			from = C4V_Any;
			while (pFunc2 && allwarn)
			{
				from = pFunc2->GetRetType();
				if (!C4Value::WarnAboutConversion(from, to))
				{
					allwarn = false;
					from = C4V_Any;
				}
				pFunc2 = Engine->GetNextSNFunc(pFunc2);
			}
			break;
		}
	case AB_Inc: case AB_Dec: case AB_BitNot: case AB_Neg:
	case AB_Pow: case AB_Div: case AB_Mul: case AB_Mod: case AB_Sub: case AB_Sum:
	case AB_LeftShift: case AB_RightShift: case AB_BitAnd: case AB_BitXOr: case AB_BitOr:
		from = C4V_Int; break;
	case AB_Not: case AB_LessThan: case AB_LessThanEqual: case AB_GreaterThan: case AB_GreaterThanEqual:
	case AB_Equal: case AB_NotEqual:
		from = C4V_Bool; break;
	case AB_DUP:
		{
			int pos = Fn->GetLastCode()->Par.i + stack_height - 2 + Fn->VarNamed.iSize + Fn->GetParCount();
			if (pos < Fn->GetParCount())
				from = Fn->GetParType()[pos];
			else
				from = C4V_Any;
			break;
		}
	default:
		from = C4V_Any; break;
	}
	return from;
}

C4AulBCC C4AulCompiler::MakeSetter(const char * SPos, bool fLeaveValue)
{
	C4AulBCC Value = *(Fn->GetLastCode()), Setter = Value;
	// Check type
	switch (Value.bccType)
	{
	case AB_ARRAYA: Setter.bccType = AB_ARRAYA_SET; break;
	case AB_ARRAY_SLICE: Setter.bccType = AB_ARRAY_SLICE_SET; break;
	case AB_DUP:
		Setter.bccType = AB_STACK_SET;
		// the setter additionally has the new value on the stack
		--Setter.Par.i;
		break;
	case AB_STACK_SET: Setter.bccType = AB_STACK_SET; break;
	case AB_LOCALN:
		Setter.bccType = AB_LOCALN_SET;
		break;
	case AB_PROP:
		Setter.bccType = AB_PROP_SET;
		break;
	case AB_GLOBALN: Setter.bccType = AB_GLOBALN_SET; break;
	default:
		throw C4AulParseError(Fn, SPos, "assignment to a constant");
	}
	// If the new value is produced using the old one, the parameters to get the old one need to be duplicated.
	// Otherwise, the setter can just use the parameters originally meant for the getter.
	// All getters push one value, so the parameter count is one more than the values they pop from the stack.
	int iParCount = 1 - GetStackValue(Value.bccType, Value.Par.X);
	if (Value.bccType == AB_STACK_SET)
	{
		// STACK_SET has a side effect, so it can't be simply removed.
		// Discard the unused value the usual way instead.
		if (!fLeaveValue)
			AddBCC(SPos, AB_STACK, -1);
		// The original parameter isn't needed anymore, since in contrast to the other getters
		// it does not indicate a position.
		iParCount = 0;
	}
	else if (!fLeaveValue || iParCount)
	{
		RemoveLastBCC();
		at_jump_target = true; // In case the original BCC was a jump target
	}
	if (fLeaveValue && iParCount)
	{
		for (int i = 0; i < iParCount; i++)
			AddBCC(SPos, AB_DUP, 1 - iParCount);
		// Finally re-add original BCC
		AddBCC(SPos, Value.bccType, Value.Par.X);
	}
	// Done. The returned BCC should be added later once the value to be set was pushed on top.
	assert(iParCount == -GetStackValue(Setter.bccType, Setter.Par.X));
	return Setter;
}

int C4AulCompiler::JumpHere()
{
	// Set flag so the next generated code chunk won't get joined
	at_jump_target = true;
	return Fn->GetCodePos();
}

static bool IsJump(C4AulBCCType t)
{
	return t == AB_JUMP || t == AB_JUMPAND || t == AB_JUMPOR || t == AB_JUMPNNIL || t == AB_CONDN || t == AB_COND;
}

void C4AulCompiler::SetJumpHere(int iJumpOp)
{
	// Set target
	C4AulBCC *pBCC = Fn->GetCodeByPos(iJumpOp);
	assert(IsJump(pBCC->bccType));
	pBCC->Par.i = Fn->GetCodePos() - iJumpOp;
	// Set flag so the next generated code chunk won't get joined
	at_jump_target = true;
}

void C4AulCompiler::SetJump(int iJumpOp, int iWhere)
{
	// Set target
	C4AulBCC *pBCC = Fn->GetCodeByPos(iJumpOp);
	assert(IsJump(pBCC->bccType));
	pBCC->Par.i = iWhere - iJumpOp;
}

void C4AulCompiler::AddJump(const char * SPos, C4AulBCCType eType, int iWhere)
{
	AddBCC(SPos, eType, iWhere - Fn->GetCodePos());
}

void C4AulCompiler::PushLoop()
{
	Loop *pNew = new Loop();
	pNew->StackSize = stack_height;
	pNew->Controls = NULL;
	pNew->Next = active_loops;
	active_loops = pNew;
}

void C4AulCompiler::PopLoop(int ContinueJump)
{
	// Set targets for break/continue
	for (Loop::Control *pCtrl = active_loops->Controls; pCtrl; pCtrl = pCtrl->Next)
		if (pCtrl->Break)
			SetJumpHere(pCtrl->Pos);
		else
			SetJump(pCtrl->Pos, ContinueJump);
	// Delete loop controls
	Loop *pLoop = active_loops;
	while (pLoop->Controls)
	{
		// Unlink
		Loop::Control *pCtrl = pLoop->Controls;
		pLoop->Controls = pCtrl->Next;
		// Delete
		delete pCtrl;
	}
	// Unlink & delete
	active_loops = pLoop->Next;
	delete pLoop;
}

void C4AulCompiler::AddLoopControl(const char * SPos, bool fBreak)
{
	// Insert code
	if (active_loops->StackSize != stack_height)
		AddBCC(SPos, AB_STACK, active_loops->StackSize - stack_height);
	Loop::Control *pNew = new Loop::Control();
	pNew->Break = fBreak;
	pNew->Pos = Fn->GetCodePos();
	pNew->Next = active_loops->Controls;
	active_loops->Controls = pNew;
	AddBCC(SPos, AB_JUMP);
}

void C4AulCompiler::ErrorOut(const char * SPos, C4AulError & e)
{
	// make all jumps that don't have their destination yet jump here
	for (unsigned int i = 0; i < Fn->Code.size(); i++)
	{
		C4AulBCC *pBCC = &Fn->Code[i];
		if (IsJump(pBCC->bccType))
			if (!pBCC->Par.i)
				pBCC->Par.i = Fn->Code.size() - i;
	}
	// add an error chunk
	const char * msg = e.what();
	if (SEqual2(msg, "ERROR: ")) msg += 7;
	AddBCC(SPos, AB_ERR, reinterpret_cast<intptr_t>(::Strings.RegString(msg)));
}
