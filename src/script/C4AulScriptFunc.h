/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2016, The OpenClonk Team and contributors
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

#ifndef C4AULSCRIPTFUNC_H_
#define C4AULSCRIPTFUNC_H_

#include "script/C4Value.h"
#include "script/C4ValueMap.h"

// byte code chunk type
// some special script functions defined hard-coded to reduce the exec context
enum C4AulBCCType : int
{
	AB_ARRAYA,  // array or proplist access
	AB_ARRAYA_SET,
	AB_PROP,    // proplist access with static key
	AB_PROP_SET,
	AB_ARRAY_SLICE, // array slicing
	AB_ARRAY_SLICE_SET,
	AB_DUP,     // duplicate value from stack
	AB_DUP_CONTEXT, // duplicate value from stack of parent function
	AB_STACK_SET, // copy top of stack to stack
	AB_POP_TO,   // pop top of stack to stack
	AB_LOCALN,  // a property of this
	AB_LOCALN_SET,
	AB_GLOBALN, // a named global
	AB_GLOBALN_SET,
	AB_PAR,     // Par statement
	AB_THIS,    // this()
	AB_FUNC,    // function

// prefix
	AB_Inc,  // ++
	AB_Dec,  // --
	AB_BitNot,  // ~
	AB_Not,   // !
	AB_Neg,   // -

// postfix
	AB_Pow,   // **
	AB_Div,   // /
	AB_Mul,   // *
	AB_Mod,   // %
	AB_Sub,   // -
	AB_Sum,   // +
	AB_LeftShift, // <<
	AB_RightShift,  // >>
	AB_LessThan,  // <
	AB_LessThanEqual, // <=
	AB_GreaterThan, // >
	AB_GreaterThanEqual,  // >=
	AB_Equal, // ==
	AB_NotEqual,  // !=
	AB_BitAnd,  // &
	AB_BitXOr,  // ^
	AB_BitOr, // |

	AB_CALL,    // direct object call
	AB_CALLFS,  // failsafe direct call
	AB_STACK,   // push nulls / pop
	AB_INT,     // constant: int
	AB_BOOL,    // constant: bool
	AB_STRING,  // constant: string
	AB_CPROPLIST, // constant: proplist
	AB_CARRAY,  // constant: array
	AB_CFUNCTION, // constant: function
	AB_NIL,     // constant: nil
	AB_NEW_ARRAY,   // semi-constant: array
	AB_NEW_PROPLIST, // create a new proplist
	AB_JUMP,    // jump
	AB_JUMPAND, // jump if convertible to false, else pop the stack
	AB_JUMPOR,  // jump if convertible to true, else pop the stack
	AB_JUMPNNIL, // jump if not nil, else pop the stack
	AB_CONDN,   // conditional jump (negated, pops stack)
	AB_COND,    // conditional jump (pops stack)
	AB_FOREACH_NEXT, // foreach: next element
	AB_RETURN,  // return statement
	AB_ERR,     // parse error at this position
	AB_DEBUG,   // debug break
	AB_EOFN,    // end of function
};

// byte code chunk
class C4AulBCC
{
public:
	C4AulBCCType bccType{AB_EOFN}; // chunk type
	union
	{
		intptr_t X;
		int32_t i;
		C4String * s;
		C4PropList * p;
		C4ValueArray * a;
		C4AulFunc * f;
	} Par;    // extra info
	C4AulBCC() = default;
	C4AulBCC(C4AulBCCType bccType, intptr_t X): bccType(bccType), Par{X}
	{
		IncRef();
	}
	C4AulBCC(const C4AulBCC & from): C4AulBCC(from.bccType, from.Par.X) { }
	C4AulBCC & operator = (const C4AulBCC & from)
	{
		DecRef();
		bccType = from.bccType;
		Par = from.Par;
		IncRef();
		return *this;
	}
	C4AulBCC(C4AulBCC && from): bccType(from.bccType), Par(from.Par)
	{
		from.bccType = AB_EOFN;
	}
	C4AulBCC & operator = (C4AulBCC && from)
	{
		DecRef();
		bccType = from.bccType;
		Par = from.Par;
		from.bccType = AB_EOFN;
		return *this;
	}
	~C4AulBCC()
	{
		DecRef();
	}
private:
	void IncRef()
	{
		switch (bccType)
		{
		case AB_ERR:
			if (Par.s)
		case AB_STRING: case AB_CALL: case AB_CALLFS: case AB_LOCALN: case AB_LOCALN_SET: case AB_PROP: case AB_PROP_SET:
			Par.s->IncRef();
			break;
		case AB_CARRAY:
			Par.a->IncRef();
			break;
		default: break;
		}
	}
	void DecRef()
	{
		switch (bccType)
		{
		case AB_ERR:
			if (Par.s)
		case AB_STRING: case AB_CALL: case AB_CALLFS: case AB_LOCALN: case AB_LOCALN_SET: case AB_PROP: case AB_PROP_SET:
			Par.s->DecRef();
			break;
		case AB_CARRAY:
			Par.a->DecRef();
			break;
		default: break;
		}
	}
};

// script function class
class C4AulScriptFunc : public C4AulFunc
{
public:
	C4AulFunc *OwnerOverloaded; // overloaded owner function; if present
	void SetOverloaded(C4AulFunc *);
	C4AulScriptFunc *SFunc() override { return this; } // type check func...
protected:
	void AddBCC(C4AulBCCType eType, intptr_t = 0, const char * SPos = nullptr); // add byte code chunk and advance
	void RemoveLastBCC();
	void ClearCode();
	int GetCodePos() const { return Code.size(); }
	C4AulBCC *GetCodeByPos(int iPos) { return &Code[iPos]; }
	C4AulBCC *GetLastCode() { return Code.empty() ? nullptr : &Code.back(); }
	void DumpByteCode();
	std::vector<C4AulBCC> Code;
	std::vector<const char *> PosForCode;
	int ParCount;
	C4V_Type ParType[C4AUL_MAX_Par]; // parameter types

public:
	const char *Script; // script pos
	C4ValueMapNames VarNamed; // list of named vars in this function
	C4ValueMapNames ParNamed; // list of named pars in this function
	void AddPar(const char * Idtf, C4V_Type type = C4V_Any)
	{
		assert(ParCount < C4AUL_MAX_Par);
		assert(ParCount == ParNamed.iSize);
		ParNamed.AddName(Idtf);
		ParType[ParCount] = type;
		++ParCount;
	}
	C4ScriptHost *pOrgScript; // the orginal script (!= Owner if included or appended)

	C4AulScriptFunc(C4PropListStatic * Parent, C4ScriptHost *pOrgScript, const char *pName, const char *Script);
	C4AulScriptFunc(C4PropListStatic * Parent, const C4AulScriptFunc &FromFunc); // copy script/code, etc from given func
	~C4AulScriptFunc() override;

	void ParseDirectExecFunc(C4AulScriptEngine *Engine, C4AulScriptContext* context = nullptr);
	void ParseDirectExecStatement(C4AulScriptEngine *Engine, C4AulScriptContext* context = nullptr);

	bool GetPublic() const override { return true; }
	int GetParCount() const override { return ParCount; }
	const C4V_Type *GetParType() const override { return ParType; }
	C4V_Type GetRetType() const override { return C4V_Any; }
	C4Value Exec(C4PropList * p, C4Value pPars[], bool fPassErrors=false) override; // execute func

	int GetLineOfCode(C4AulBCC * bcc);
	C4AulBCC * GetCode();

	uint32_t tProfileTime; // internally set by profiler

	friend class C4AulCompiler;
	friend class C4AulParse;
	friend class C4ScriptHost;
};

#endif /* C4AULSCRIPTFUNC_H_ */
