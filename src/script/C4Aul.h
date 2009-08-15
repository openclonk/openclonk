/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2002, 2006-2007  Sven Eberhardt
 * Copyright (c) 2001, 2004, 2006  Peter Wortmann
 * Copyright (c) 2004, 2006-2009  Günther Brammer
 * Copyright (c) 2006  Armin Burgmeier
 * Copyright (c) 2007  Matthes Bender
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
// C4Aul script engine CP conversion
// (cut C4Aul of classes/structs and put everything in namespace C4Aul instead?)
// drop uncompiled scripts when not in developer mode
// -> build string table
// -> clear the string table in UnLink? ReLink won't happen to be called in player mode anyway

#ifndef INC_C4Aul
#define INC_C4Aul

#include <C4AList.h>
#include <C4ValueList.h>
#include <C4ValueMap.h>
#include <C4Id.h>
#include <C4Script.h>
#include <C4StringTable.h>

// debug mode?
#ifdef _DEBUG
#define C4AUL_DEBUG
#endif

// class predefs
class C4AulError;
class C4AulFunc;
class C4AulScriptFunc;
class C4AulDefFunc;
class C4AulScript;
class C4AulScriptEngine;

struct C4AulContext;
struct C4AulBCC;

// consts
#define C4AUL_MAX_String			1024// max string length
#define C4AUL_MAX_Identifier	100	// max length of function identifiers
#define C4AUL_MAX_Par					10	// max number of parameters
#define C4AUL_MAX_Var					10	// max number of func local vars

#define C4AUL_ControlMethod_None 0
#define C4AUL_ControlMethod_Classic 1
#define C4AUL_ControlMethod_JumpAndRun 2
#define C4AUL_ControlMethod_All 3

// generic C4Aul error class
class C4AulError
	{
	protected:
		StdStrBuf sMessage;

	public:
		C4AulError();
		C4AulError(const C4AulError& Error) { sMessage.Copy(Error.sMessage); } // copy - constructor
		virtual ~C4AulError() { } // destructor
		virtual void show(); // present error message
	};

// parse error
class C4AulParseError : public C4AulError
	{
	public:
		C4AulParseError(C4AulScript *pScript, const char *pMsg, const char *pIdtf = NULL, bool Warn = false); // constructor
		C4AulParseError(class C4AulParseState * state, const char *pMsg, const char *pIdtf = NULL, bool Warn = false); // constructor
	};

// execution error
class C4AulExecError : public C4AulError
	{
		C4Object *cObj;
	public:
		C4AulExecError(C4Object *pObj, const char *szError); // constructor
		virtual void show(); // present error message
	};

// function access
enum C4AulAccess
	{
	AA_PRIVATE,
	AA_PROTECTED,
	AA_PUBLIC,
	AA_GLOBAL
	};

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
	C4Value & operator[](int iIdx) { return Par[iIdx]; }
	C4AulParSet * operator&() { return this; }
};

#define Copy2ParSet8(Pars, Vars) Pars[0].Set(Vars##0); Pars[1].Set(Vars##1); Pars[2].Set(Vars##2); Pars[3].Set(Vars##3); Pars[4].Set(Vars##4); Pars[5].Set(Vars##5); Pars[6].Set(Vars##6); Pars[7].Set(Vars##7);
#define Copy2ParSet9(Pars, Vars) Pars[0].Set(Vars##0); Pars[1].Set(Vars##1); Pars[2].Set(Vars##2); Pars[3].Set(Vars##3); Pars[4].Set(Vars##4); Pars[5].Set(Vars##5); Pars[6].Set(Vars##6); Pars[7].Set(Vars##7); Pars[8].Set(Vars##8);
#define Copy2ParSet10(Pars, Vars) Pars[0].Set(Vars##0); Pars[1].Set(Vars##1); Pars[2].Set(Vars##2); Pars[3].Set(Vars##3); Pars[4].Set(Vars##4); Pars[5].Set(Vars##5); Pars[6].Set(Vars##6); Pars[7].Set(Vars##7); Pars[8].Set(Vars##8); Pars[9].Set(Vars##9);

// byte code chunk type
// some special script functions defined hard-coded to reduce the exec context
enum C4AulBCCType
	{
	AB_ARRAYA_R,	// array access
	AB_ARRAYA_V,	// not creating a reference
	AB_VARN_R,		// a named var
	AB_VARN_V,
	AB_PARN_R,		// a named parameter
	AB_PARN_V,
	AB_LOCALN_R,	// a named local
	AB_LOCALN_V,
	AB_GLOBALN_R,	// a named global
	AB_GLOBALN_V,
	AB_PAR_R,			// Par statement
	AB_PAR_V,
	AB_FUNC,		// function

// prefix
	AB_Inc1,	// ++
	AB_Dec1,	// --
	AB_BitNot,	// ~
	AB_Not, 	// !
				// +
	AB_Neg, 	// -

// postfix (whithout second statement)
	AB_Inc1_Postfix,	// ++
	AB_Dec1_Postfix,	// --

// postfix
	AB_Pow, 	// **
	AB_Div, 	// /
	AB_Mul, 	// *
	AB_Mod, 	// %
	AB_Sub, 	// -
	AB_Sum, 	// +
	AB_LeftShift,	// <<
	AB_RightShift,	// >>
	AB_LessThan,	// <
	AB_LessThanEqual,	// <=
	AB_GreaterThan,	// >
	AB_GreaterThanEqual,	// >=
	AB_Equal,	// ==
	AB_NotEqual,	// !=
	AB_SEqual,	// S=, eq
	AB_SNEqual,	// ne
	AB_BitAnd,	// &
	AB_BitXOr,	// ^
	AB_BitOr,	// |
	AB_MulIt,	// *=
	AB_DivIt,	// /=
	AB_ModIt,	// %=
	AB_Inc, 	// +=
	AB_Dec, 	// -=
	AB_AndIt,	// &=
	AB_OrIt,	// |=
	AB_XOrIt,	// ^=
	AB_Set, 	// =

	AB_CALL,		// direct object call
	AB_CALLFS,	// failsafe direct call
	AB_CALLNS,	// direct object call: namespace operator
	AB_STACK,		// push nulls / pop
	AB_INT,			// constant: int
	AB_BOOL,		// constant: bool
	AB_STRING,	// constant: string
	AB_C4ID,		// constant: C4ID
	AB_NIL,		  // constant: nil
	AB_ARRAY,		// semi-constant: array
	AB_PROPLIST,		// create a new proplist
	AB_PROPSET,		// set a property of a proplist
	AB_IVARN,		// initialization of named var
	AB_JUMP,		// jump
	AB_JUMPAND,		// jump if zero, else pop the stack
	AB_JUMPOR,		// jump if not zero, else pop the stack
	AB_CONDN,		// conditional jump (negated, pops stack)
	AB_FOREACH_NEXT, // foreach: next element
	AB_RETURN,	// return statement
	AB_ERR,			// parse error at this position
	AB_EOFN,		// end of function
	AB_EOF,			// end of file
	};


// ** a definition of an operator
// there are two classes of operators, the postfix-operators (+,-,&,...) and the
// prefix-operators (mainly !,~,...).
struct C4ScriptOpDef
	{
	unsigned short Priority;
	const char* Identifier;
	C4AulBCCType Code;
	bool Postfix;
	bool RightAssociative; // right oder left-associative?
	bool NoSecondStatement; // no second statement expected (++/-- postfix)
	C4V_Type RetType; // type returned. ignored by C4V
	C4V_Type Type1;
	C4V_Type Type2;
	};
extern C4ScriptOpDef C4ScriptOpMap[];

// byte code chunk
struct C4AulBCC
	{
	C4AulBCCType bccType;	// chunk type
	union
		{
		int32_t i;
		C4String * s;
		C4AulFunc * f;
		intptr_t X;
		} Par;		// extra info (long for use with amd64)
	const char *SPos;
	};

// call context
struct C4AulContext
	{
	C4Object *Obj;
	C4Def *Def;
	struct C4AulScriptContext *Caller;
	};

// execution context
struct C4AulScriptContext : public C4AulContext
	{
	C4Value *Return;
	C4Value *Pars;
	C4Value *Vars;
	C4AulScriptFunc *Func;
	bool TemporaryScript;
	C4AulBCC *CPos;
	time_t tTime; // initialized only by profiler if active

	int ParCnt() const { return Vars - Pars; }
	void dump(StdStrBuf Dump = StdStrBuf(""));
	};

// base function class
class C4AulFunc
	{
		friend class C4AulScript;
		friend class C4AulScriptEngine;
		friend class C4AulFuncMap;
		friend class C4AulParseState;

	public:
		C4AulFunc(C4AulScript *pOwner, const char *pName, bool bAtEnd = true); // constructor
		virtual ~C4AulFunc(); // destructor

		C4AulScript *Owner; // owner
		char Name[C4AUL_MAX_Identifier]; // function name

	protected:
		C4AulFunc *Prev, *Next; // linked list members
		C4AulFunc *MapNext; // map member
		C4AulFunc *LinkedTo; // points to next linked function; destructor will destroy linked func, too

	public:
		C4AulFunc *OverloadedBy; // function by which this one is overloaded

		virtual C4AulScriptFunc *SFunc() { return NULL; } // type check func...

		// Wether this function should be visible to players
		virtual bool GetPublic() { return false; }
		virtual int GetParCount() { return C4AUL_MAX_Par; }
		virtual C4V_Type* GetParType() { return 0; }
		virtual C4V_Type GetRetType() { return C4V_Any; }
		virtual C4Value Exec(C4AulContext *pCallerCtx, C4Value pPars[], bool fPassErrors=false) { return C4Value(); } // execute func (script call)
		virtual C4Value Exec(C4Object *pObj=NULL, C4AulParSet *pPars = NULL, bool fPassErrors=false); // execute func (engine call)
		virtual void UnLink() { OverloadedBy = NULL; }

		C4AulFunc *GetLocalSFunc(const char *szIdtf); // find script function in own scope
	protected:
		void DestroyLinked(); // destroys linked functions

	};

// script function class
class C4AulScriptFunc : public C4AulFunc
	{
	public:
		C4AulFunc *OwnerOverloaded; // overloaded owner function; if present
		C4AulScriptFunc *SFunc() { return this; } // type check func...
	protected:
		void ParseDesc(); // evaluate desc (i.e. get idImage and Condition

	public:
		C4AulAccess Access;
		StdCopyStrBuf Desc; // full function description block, including image and condition
		StdCopyStrBuf DescText; // short function description text (name of menu entry)
		StdCopyStrBuf DescLong; // secondary function description
		C4ID idImage; // associated image
		int32_t iImagePhase; // Image phase
		C4AulFunc *Condition; // func condition
		int32_t ControlMethod; // 0 = all, 1 = Classic, 2 = Jump+Run
		const char *Script; // script pos
		C4AulBCC *Code; // code pos
		C4ValueMapNames VarNamed; // list of named vars in this function
		C4ValueMapNames ParNamed; // list of named pars in this function
		C4V_Type ParType[C4AUL_MAX_Par]; // parameter types
		bool bReturnRef; // return reference
		C4AulScript *pOrgScript; // the orginal script (!= Owner if included or appended)

		C4AulScriptFunc(C4AulScript *pOwner, const char *pName, bool bAtEnd = true) : C4AulFunc(pOwner, pName, bAtEnd),
			idImage (C4ID_None), iImagePhase(0), Condition(NULL), ControlMethod(C4AUL_ControlMethod_All), OwnerOverloaded(NULL),
			bReturnRef(false), tProfileTime(0)
		{
			for(int i = 0; i < C4AUL_MAX_Par; i++) ParType[i] = C4V_Any;
		} // constructor

		virtual void UnLink();

		virtual bool GetPublic() { return true; }
		virtual C4V_Type *GetParType() { return ParType; }
		virtual C4V_Type GetRetType() { return bReturnRef ? C4V_pC4Value : C4V_Any; }
		virtual C4Value Exec(C4AulContext *pCallerCtx, C4Value pPars[], bool fPassErrors=false); // execute func (script call, should not happen)
		virtual C4Value Exec(C4Object *pObj=NULL, C4AulParSet *pPars = NULL, bool fPassErrors=false); // execute func (engine call)

		void SetError(C4AulContext *ctx, const char *szMessage); // set error message

		void CopyBody(C4AulScriptFunc &FromFunc); // copy script/code, etc from given func

		StdStrBuf GetFullName(); // get a fully classified name (C4ID::Name) for debug output

		time_t tProfileTime; // internally set by profiler

		friend class C4AulScript;
	};


// defined function class
class C4AulDefFunc : C4AulFunc
	{
	public:
		C4ScriptFnDef* Def;

		C4AulDefFunc(C4AulScript *pOwner, const char *pName, C4ScriptFnDef* pDef) : C4AulFunc(pOwner, pName) // constructor
			{	Def = pDef; }

		virtual bool GetPublic() { return !!Def->Public; }
		virtual C4V_Type* GetParType() { return Def->ParType; }
		virtual C4V_Type GetRetType() { return Def->RetType; }

		virtual C4Value Exec(C4AulContext *pCallerCtx, C4Value pPars[], bool fPassErrors=false); // execute func (script call)
	};

class C4AulFuncMap {
	public:
		C4AulFuncMap();
		~C4AulFuncMap();
		C4AulFunc * GetFunc(const char * Name, const C4AulScript * Owner, const C4AulFunc * After);
		C4AulFunc * GetFirstFunc(const char * Name);
		C4AulFunc * GetNextSNFunc(const C4AulFunc * After);
	private:
		C4AulFunc ** Funcs;
		int FuncCnt;
		int Capacity;
		static unsigned int Hash(const char * Name);
	protected:
		void Add(C4AulFunc * func, bool bAtEnd = true);
		void Remove(C4AulFunc * func);
	friend class C4AulFunc;
};


// aul script state
enum C4AulScriptState
	{
	ASS_ERROR,			// erroneous script
	ASS_NONE,				// nothing
	ASS_PREPARSED,	// function list built; CodeSize set
	ASS_LINKED,			// includes and appends resolved
	ASS_PARSED			// byte code generated
	};


// script profiler entry
class C4AulProfiler
	{
	private:
		// map entry
		struct Entry
			{
			C4AulScriptFunc *pFunc;
			time_t tProfileTime;

			bool operator < (const Entry &e2) const { return tProfileTime < e2.tProfileTime ; }
			};

		// items
		std::vector<Entry> Times;

	public:
		void CollectEntry(C4AulScriptFunc *pFunc, time_t tProfileTime);
		void Show();

		static void Abort();
		static void StartProfiling(C4AulScript *pScript);
		static void StopProfiling();
	};


// script class
class C4AulScript
	{
// MSVC maybe needs this.
#ifdef _MSC_VER
	friend class C4AulScript;
#endif
	public:
		C4AulScript(); // constructor
		virtual ~C4AulScript(); // destructor
		void Default(); // init
		void Clear(); // remove script, byte code and children
		void Reg2List(C4AulScriptEngine *pEngine, C4AulScript *pOwner); // reg to linked list
		void Unreg(); // remove from list
		virtual bool Delete() { return true; } // allow deletion on pure class

	protected:
		C4AulFunc *Func0, *FuncL; // owned functions
		C4AulScriptEngine *Engine; //owning engine
		C4AulScript *Owner, *Prev, *Next, *Child0, *ChildL; // tree structure

		StdStrBuf Script; // script
		C4AulBCC *Code, *CPos;	// compiled script (/pos)
		C4AulScriptState State; // script state
		int CodeSize; // current number of byte code chunks in Code
		int CodeBufSize; // size of Code buffer
		bool Preparsing; // set while preparse
		bool Resolving;	// set while include-resolving, to catch circular includes

		C4AListEntry *Includes; // include list
		C4AListEntry *Appends; // append list

		// internal function used to find overloaded functions
		C4AulFunc *GetOverloadedFunc(C4AulFunc *ByFunc);
		C4AulFunc *GetFunc(const char *pIdtf); // get local function by name

		void AddBCC(C4AulBCCType eType, intptr_t = 0, const char * SPos = 0); // add byte code chunk and advance
		void ClearCode();
		bool Preparse(); // preparse script; return if successfull
		void ParseFn(C4AulScriptFunc *Fn, bool fExprOnly = false); // parse single script function

		bool Parse(); // parse preparsed script; return if successfull
		void ParseDescs(); // parse function descs

		bool ResolveIncludes(C4DefList *rDefs); // resolve includes
		bool ResolveAppends(C4DefList *rDefs); // resolve appends
		bool IncludesResolved;
		void AppendTo(C4AulScript &Scr, bool bHighPrio); // append to given script
		void UnLink(); // reset to unlinked state
		virtual void AfterLink(); // called after linking is completed; presearch common funcs here
		virtual bool ReloadScript(const char *szPath); // reload given script

		C4AulScript *FindFirstNonStrictScript();		// find first script that is not #strict

		int GetCodePos() const { return CPos - Code; }
		C4AulBCC *GetCodeByPos(int iPos) { return Code + iPos; }

	public:
		StdCopyStrBuf ScriptName; // script name
		C4Def *Def; // owning def file
		C4ValueMapNames LocalNamed;
		enum Strict { NONSTRICT = 0, STRICT1 = 1, STRICT2 = 2, MAXSTRICT=STRICT2 };
		enum Strict Strict; // new or even newer syntax?
		bool Temporary; // set for DirectExec-scripts; do not parse those

		const char *GetScript() const { return Script.getData(); }

		C4AulFunc *GetFuncRecursive(const char *pIdtf); // search function by identifier, including global funcs
		C4AulScriptFunc *GetSFunc(const char *pIdtf, C4AulAccess AccNeeded, bool fFailSafe = false); // get local sfunc, check access, check '~'-safety
		C4AulScriptFunc *GetSFunc(const char *pIdtf); // get local script function by name
		C4AulScriptFunc *GetSFunc(int iIndex, const char *szPattern = NULL); // get local script function by index
		C4AulScriptFunc *GetSFuncWarn(const char *pIdtf, C4AulAccess AccNeeded, const char *WarnStr); // get function; return NULL and warn if not existant

		void AddFunc(const char *pIdtf, C4ScriptFnDef* Def);  // add def def func to table

	public:
		C4Value DirectExec(C4Object *pObj, const char *szScript, const char *szContext, bool fPassErrors = false, enum Strict Strict = MAXSTRICT); // directly parse uncompiled script (WARG! CYCLES!)
		void ResetProfilerTimes(); // zero all profiler times of owned functions
		void CollectProfilerTimes(class C4AulProfiler &rProfiler);

		bool IsReady() { return State == ASS_PARSED; } // whether script calls may be done

		// helper functions
		void Warn(const char *pMsg, const char *pIdtf);

		friend class C4AulParseError;
		friend class C4AulFunc;
		friend class C4AulScriptFunc;
		friend class C4AulScriptEngine;
		friend class C4AulParseState;
	};


// holds all C4AulScripts
class C4AulScriptEngine : public C4AulScript
	{
	protected:
		C4AList itbl; // include table
		C4AList atbl; // append table
		C4AulFuncMap FuncLookUp;

	public:
		int warnCnt, errCnt; // number of warnings/errors
		int nonStrictCnt; // number of non-strict scripts
		int lineCnt; // line count parsed

		C4ValueMapNames GlobalNamedNames;
		C4ValueMapData GlobalNamed;

		// global constants (such as "static const C4D_Structure = 2;")
		// cannot share var lists, because it's so closely tied to the data lists
		// constants are used by the Parser only, anyway, so it's not
		// necessary to pollute the global var list here
		C4ValueMapNames GlobalConstNames;
		C4ValueMapData GlobalConsts;

		C4AulScriptEngine(); // constructor
		~C4AulScriptEngine(); // destructor
		void Clear(); // clear data
		void Link(C4DefList *rDefs); // link and parse all scripts
		void ReLink(C4DefList *rDefs); // unlink + relink and parse all scripts
		bool ReloadScript(const char *szScript, C4DefList *pDefs); // search script and reload + relink, if found
		C4AulFunc * GetFirstFunc(const char * Name)
			{ return FuncLookUp.GetFirstFunc(Name); }
		C4AulFunc * GetFunc(const char * Name, const C4AulScript * Owner, const C4AulFunc * After)
			{ return FuncLookUp.GetFunc(Name, Owner, After); }
		C4AulFunc * GetNextSNFunc(const C4AulFunc * After)
			{ return FuncLookUp.GetNextSNFunc(After); }

		// For the list of functions in the PropertyDlg
		C4AulFunc * GetFirstFunc() { return Func0; }
		C4AulFunc * GetNextFunc(C4AulFunc * pFunc) { return pFunc->Next; }


		void RegisterGlobalConstant(const char *szName, const C4Value &rValue); // creates a new constants or overwrites an old one
		bool GetGlobalConstant(const char *szName, C4Value *pTargetValue); // check if a constant exists; assign value to pTargetValue if not NULL

		bool DenumerateVariablePointers();
		void UnLink(); // called when a script is being reloaded (clears string table)
		// Compile scenario script data (without strings and constants)
		void CompileFunc(StdCompiler *pComp);

		friend class C4AulFunc;
		friend class C4AulParseState;
	};

extern C4AulScriptEngine ScriptEngine;
#endif
