/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001, 2004, 2006, 2009-2010  Peter Wortmann
 * Copyright (c) 2001-2002, 2004, 2006-2007  Sven Eberhardt
 * Copyright (c) 2004, 2006-2010  Günther Brammer
 * Copyright (c) 2006  Armin Burgmeier
 * Copyright (c) 2007  Matthes Bender
 * Copyright (c) 2009  Nicolas Hake
 * Copyright (c) 2010  Martin Plicht
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

#ifndef INC_C4Aul
#define INC_C4Aul

#include <C4ValueMap.h>
#include <C4Id.h>
#include <C4Script.h>
#include <C4StringTable.h>
#include <string>
#include <vector>

// consts
#define C4AUL_MAX_Identifier  100 // max length of function identifiers
#define C4AUL_MAX_Par         10  // max number of parameters

// generic C4Aul error class
class C4AulError
{
protected:
	StdCopyStrBuf sMessage;

public:
	bool shown;
	C4AulError();
	virtual ~C4AulError() { } // destructor
	void show(); // present error message
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
	AB_ARRAYA,  // array or proplist access
	AB_ARRAYA_SET,
	AB_PROP,    // proplist access with static key
	AB_PROP_SET,
	AB_ARRAY_SLICE, // array slicing
	AB_ARRAY_SLICE_SET,
	AB_DUP,     // duplicate value from stack
	AB_STACK_SET, // copy top of stack to stack
	AB_POP_TO,   // pop top of stack to stack
	AB_LOCALN,  // a property of this
	AB_LOCALN_SET,
	AB_GLOBALN, // a named global
	AB_GLOBALN_SET,
	AB_PAR,     // Par statement
	AB_FUNC,    // function

	AB_PARN_CONTEXT,
	AB_VARN_CONTEXT,

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
	AB_JUMPAND, // jump if zero, else pop the stack
	AB_JUMPOR,  // jump if not zero, else pop the stack
	AB_CONDN,   // conditional jump (negated, pops stack)
	AB_COND,    // conditional jump (pops stack)
	AB_FOREACH_NEXT, // foreach: next element
	AB_RETURN,  // return statement
	AB_ERR,     // parse error at this position
	AB_DEBUG,   // debug break
	AB_EOFN,    // end of function
	AB_EOF      // end of file
};

// byte code chunk
struct C4AulBCC
{
	C4AulBCCType bccType; // chunk type
	union
	{
		int32_t i;
		C4String * s;
		C4PropList * p;
		C4ValueArray * a;
		C4AulFunc * f;
		intptr_t X;
	} Par;    // extra info
};

// call context
struct C4AulContext
{
	C4Object *Obj;
	C4PropList *Def;
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

	void dump(StdStrBuf Dump = StdStrBuf(""));
	StdStrBuf ReturnDump(StdStrBuf Dump = StdStrBuf(""));
};

// base function class
class C4AulFunc
{
	friend class C4AulScript;
	friend class C4AulScriptEngine;
	friend class C4AulFuncMap;
	friend class C4AulParseState;
	friend class C4ScriptHost;

public:
	C4AulFunc(C4AulScript *pOwner, const char *pName, bool bAtEnd = true); // constructor
	virtual ~C4AulFunc(); // destructor

	C4AulScript *Owner; // owner
	const char * GetName() const { return Name ? Name->GetCStr() : 0; }
	virtual StdStrBuf GetFullName(); // get a fully classified name (C4ID::Name) for debug output

protected:
	C4RefCntPointer<C4String> Name; // function name
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
	virtual C4Value Exec(C4PropList * p = NULL, C4AulParSet *pPars = NULL, bool fPassErrors=false); // execute func (engine call)
	virtual void UnLink() { OverloadedBy = NULL; }

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
	int CodePos; // code pos

public:
	C4AulAccess Access;
	const char *Script; // script pos
	C4ValueMapNames VarNamed; // list of named vars in this function
	C4ValueMapNames ParNamed; // list of named pars in this function
	int ParCount;
	C4V_Type ParType[C4AUL_MAX_Par]; // parameter types
	C4ScriptHost *pOrgScript; // the orginal script (!= Owner if included or appended)

	C4AulScriptFunc(C4AulScript *pOwner, const char *pName, bool bAtEnd = true);

	void ParseFn(bool fExprOnly = false, C4AulScriptContext* context = NULL);
	virtual void UnLink();

	virtual bool GetPublic() { return true; }
	virtual int GetParCount() { return ParCount; }
	virtual C4V_Type *GetParType() { return ParType; }
	virtual C4V_Type GetRetType() { return C4V_Any; }
	virtual C4Value Exec(C4AulContext *pCallerCtx, C4Value pPars[], bool fPassErrors=false); // execute func (script call, should not happen)
	virtual C4Value Exec(C4PropList * p = NULL, C4AulParSet *pPars = NULL, bool fPassErrors=false); // execute func (engine call)

	void SetError(C4AulContext *ctx, const char *szMessage); // set error message

	void CopyBody(C4AulScriptFunc &FromFunc); // copy script/code, etc from given func

	int GetLineOfCode(C4AulBCC * bcc);
	C4AulBCC * GetCode();
	C4ScriptHost * GetCodeOwner();

	time_t tProfileTime; // internally set by profiler

	friend class C4ScriptHost;
};

// defined function class
class C4AulDefFunc : C4AulFunc
{
public:
	C4ScriptFnDef* Def;

	C4AulDefFunc(C4AulScript *pOwner, const char *pName, C4ScriptFnDef* pDef);
	~C4AulDefFunc();

	virtual bool GetPublic() { return !!Def->Public; }
	virtual C4V_Type* GetParType() { return Def->ParType; }
	virtual C4V_Type GetRetType() { return Def->RetType; }

	using C4AulFunc::Exec;
	virtual C4Value Exec(C4AulContext *pCallerCtx, C4Value pPars[], bool fPassErrors=false); // execute func (script call)
};

class C4AulFuncMap
{
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
	ASS_ERROR,      // erroneous script
	ASS_NONE,       // nothing
	ASS_PREPARSED,  // function list built; CodeSize set
	ASS_LINKED,     // includes and appends resolved
	ASS_PARSED      // byte code generated
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
public:
	C4AulScript(); // constructor
	virtual ~C4AulScript(); // destructor
	void Clear(); // remove script, byte code and children
	void Reg2List(C4AulScriptEngine *pEngine, C4AulScript *pOwner); // reg to linked list
	void Unreg(); // remove from list
	virtual bool Delete() { return true; } // allow deletion on pure class

	StdCopyStrBuf ScriptName; // script name
	C4ValueMapNames LocalNamed;
	enum Strict { NONSTRICT = 0, STRICT1 = 1, STRICT2 = 2, MAXSTRICT=STRICT2 };
	enum Strict Strict; // new or even newer syntax?
	bool Temporary; // set for DirectExec-scripts; do not parse those

	virtual C4PropList * GetPropList() { return 0; }
	virtual C4ScriptHost * GetScriptHost() { return 0; }
	C4AulFunc *GetFuncRecursive(const char *pIdtf); // search function by identifier, including global funcs
	C4AulScriptFunc *GetSFunc(const char *pIdtf, C4AulAccess AccNeeded, bool fFailSafe = false); // get local sfunc, check access, check '~'-safety
	C4AulScriptFunc *GetSFunc(const char *pIdtf); // get local script function by name

	void AddFunc(const char *pIdtf, C4ScriptFnDef* Def);  // add def def func to table

	C4Value DirectExec(C4Object *pObj, const char *szScript, const char *szContext, bool fPassErrors = false, C4AulScriptContext* context = NULL); // directly parse uncompiled script (WARG! CYCLES!)
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
	friend class C4AulDebug;
	friend class C4ScriptHost;

	// Translate a string using the script's lang table
	std::string Translate(const std::string &text) const;

protected:
	C4LangStringTable *stringTable;	

	C4AulFunc *Func0, *FuncL; // owned functions
	C4AulScriptEngine *Engine; //owning engine
	C4AulScript *Owner, *Prev, *Next, *Child0, *ChildL; // tree structure

	C4AulScriptState State; // script state
	bool Resolving; // set while include-resolving, to catch circular includes

	std::list<C4ID> Includes; // include list
	std::list<C4ID> Appends; // append list

	// internal function used to find overloaded functions
	C4AulFunc *GetOverloadedFunc(C4AulFunc *ByFunc);
	C4AulFunc *GetFunc(const char *pIdtf); // get local function by name

	bool ResolveIncludes(C4DefList *rDefs); // resolve includes
	bool ResolveAppends(C4DefList *rDefs); // resolve appends
	bool IncludesResolved;
	void AppendTo(C4AulScript &Scr, bool bHighPrio); // append to given script
	virtual void UnLink(); // reset to unlinked state
	virtual void AfterLink(); // called after linking is completed; presearch common funcs here
	virtual bool ReloadScript(const char *szPath, const char *szLanguage); // reload given script
	virtual bool Parse();

	C4AulScript *FindFirstNonStrictScript();    // find first script that is not #strict
};

// holds all C4AulScripts
class C4AulScriptEngine : public C4AulScript
{
protected:
	C4AulFuncMap FuncLookUp;
	C4PropList * GlobalPropList;

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
	virtual C4PropList * GetPropList();
	using C4AulScript::ReloadScript;
	bool ReloadScript(const char *szScript, C4DefList *pDefs, const char *szLanguage); // search script and reload + relink, if found
	C4AulFunc * GetFirstFunc(const char * Name)
	{ return FuncLookUp.GetFirstFunc(Name); }
	C4AulFunc * GetFunc(const char * Name, const C4AulScript * Owner, const C4AulFunc * After)
	{ return FuncLookUp.GetFunc(Name, Owner, After); }
	C4AulFunc * GetNextSNFunc(const C4AulFunc * After)
	{ return FuncLookUp.GetNextSNFunc(After); }

	// For the list of functions in the PropertyDlg
	std::list<const char*> GetFunctionNames(C4AulScript *);

	void RegisterGlobalConstant(const char *szName, const C4Value &rValue); // creates a new constants or overwrites an old one
	bool GetGlobalConstant(const char *szName, C4Value *pTargetValue); // check if a constant exists; assign value to pTargetValue if not NULL

	bool Denumerate(C4ValueNumbers *);
	void UnLink(); // called when a script is being reloaded (clears string table)

	// Compile scenario script data (without strings and constants)
	void CompileFunc(StdCompiler *pComp, C4ValueNumbers * numbers);

	friend class C4AulFunc;
	friend class C4AulParseState;
	friend class C4AulDebug;
};

extern C4AulScriptEngine ScriptEngine;
#endif
