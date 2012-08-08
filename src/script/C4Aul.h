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
#include "C4AulFunc.h"
#include <string>
#include <vector>

// consts
#define C4AUL_MAX_Identifier  100 // max length of function identifiers

class C4ScriptHost;

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
	C4AulParseError(class C4AulParse * state, const char *pMsg, const char *pIdtf = NULL, bool Warn = false); // constructor
};

// execution error
class C4AulExecError : public C4AulError
{
public:
	C4AulExecError(const char *szError);
};

// function access
enum C4AulAccess
{
	AA_PRIVATE,
	AA_PROTECTED,
	AA_PUBLIC,
	AA_GLOBAL
};

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
	AB_THIS,    // this()
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
	AB_Identical, // ===
	AB_NotIdentical,  // !==
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

// execution context
struct C4AulScriptContext
{
	C4PropList *Obj;
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

// script function class
class C4AulScriptFunc : public C4AulFunc
{
public:
	C4AulFunc *OwnerOverloaded; // overloaded owner function; if present
	void SetOverloaded(C4AulFunc *);
	C4AulScriptFunc *SFunc() { return this; } // type check func...
protected:
	int CodePos; // code pos

public:
	const char *Script; // script pos
	C4ValueMapNames VarNamed; // list of named vars in this function
	C4ValueMapNames ParNamed; // list of named pars in this function
	int ParCount;
	C4V_Type ParType[C4AUL_MAX_Par]; // parameter types
	void AddPar(const char * Idtf) { ParNamed.AddName(Idtf); ++ParCount; }
	C4ScriptHost *pOrgScript; // the orginal script (!= Owner if included or appended)

	C4AulScriptFunc(C4AulScript *pOwner, C4ScriptHost *pOrgScript, const char *pName, const char *Script);
	C4AulScriptFunc(C4AulScript *pOwner, const C4AulScriptFunc &FromFunc); // copy script/code, etc from given func
	~C4AulScriptFunc();

	void ParseFn(C4AulScriptContext* context = NULL);

	virtual bool GetPublic() { return true; }
	virtual int GetParCount() { return ParCount; }
	virtual C4V_Type *GetParType() { return ParType; }
	virtual C4V_Type GetRetType() { return C4V_Any; }
	virtual C4Value Exec(C4PropList * p, C4Value pPars[], bool fPassErrors=false); // execute func

	int GetLineOfCode(C4AulBCC * bcc);
	C4AulBCC * GetCode();
	C4ScriptHost * GetCodeOwner();

	time_t tProfileTime; // internally set by profiler

	friend class C4AulParse;
	friend class C4ScriptHost;
};

class C4AulFuncMap
{
public:
	C4AulFuncMap();
	~C4AulFuncMap();
	C4AulFunc * GetFirstFunc(C4String * Name);
	C4AulFunc * GetNextSNFunc(const C4AulFunc * After);
private:
	enum { HashSize = 1025 };
	C4AulFunc * Funcs[HashSize];
	int FuncCnt;
	static unsigned int Hash(const char * Name);
protected:
	void Add(C4AulFunc * func);
	void Remove(C4AulFunc * func);
	friend class C4AulFunc;
	friend class C4ScriptHost;
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
	virtual void Clear(); // remove script, byte code and children
	void Reg2List(C4AulScriptEngine *pEngine); // reg to linked list
	void Unreg(); // remove from list
	virtual bool Delete() { return true; } // allow deletion on pure class

	StdCopyStrBuf ScriptName; // script name
	bool Temporary; // set for DirectExec-scripts; do not parse those

	virtual C4PropListStatic * GetPropList() { return 0; }
	virtual C4ScriptHost * GetScriptHost() { return 0; }

	C4Value DirectExec(C4Object *pObj, const char *szScript, const char *szContext, bool fPassErrors = false, C4AulScriptContext* context = NULL); // directly parse uncompiled script (WARG! CYCLES!)
	virtual void ResetProfilerTimes(); // zero all profiler times of owned functions
	virtual void CollectProfilerTimes(class C4AulProfiler &rProfiler);

	bool IsReady() { return State == ASS_PARSED; } // whether script calls may be done

	// helper functions
	void Warn(const char *pMsg, ...) GNUC_FORMAT_ATTRIBUTE_O;

	friend class C4AulParseError;
	friend class C4AulFunc;
	friend class C4AulScriptFunc;
	friend class C4AulScriptEngine;
	friend class C4AulParse;
	friend class C4AulDebug;
	friend class C4ScriptHost;

	// Translate a string using the script's lang table
	std::string Translate(const std::string &text) const;

protected:
	C4LangStringTable *stringTable;	

	C4AulScriptEngine *Engine; //owning engine
	C4AulScript *Prev, *Next; // tree structure

	C4AulScriptState State; // script state

	virtual bool ReloadScript(const char *szPath, const char *szLanguage); // reload given script
	virtual bool Parse();
	virtual bool ResolveIncludes(C4DefList *rDefs);
	virtual bool ResolveAppends(C4DefList *rDefs);
	virtual void UnLink();
};

// holds all C4AulScripts
class C4AulScriptEngine : public C4AulScript
{
protected:
	C4AulFuncMap FuncLookUp;
	C4Value GlobalPropList;
	C4AulScript *Child0, *ChildL; // tree structure

public:
	int warnCnt, errCnt; // number of warnings/errors
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
	virtual C4PropListStatic * GetPropList();
	using C4AulScript::ReloadScript;
	bool ReloadScript(const char *szScript, C4DefList *pDefs, const char *szLanguage); // search script and reload + relink, if found
	C4AulFunc * GetFirstFunc(C4String * Name)
	{ return FuncLookUp.GetFirstFunc(Name); }
	C4AulFunc * GetNextSNFunc(const C4AulFunc * After)
	{ return FuncLookUp.GetNextSNFunc(After); }

	// For the list of functions in the PropertyDlg
	std::list<const char*> GetFunctionNames(C4PropList *);
	void ResetProfilerTimes(); // zero all profiler times of owned functions
	void CollectProfilerTimes(class C4AulProfiler &rProfiler);

	void RegisterGlobalConstant(const char *szName, const C4Value &rValue); // creates a new constants or overwrites an old one
	bool GetGlobalConstant(const char *szName, C4Value *pTargetValue); // check if a constant exists; assign value to pTargetValue if not NULL

	bool Denumerate(C4ValueNumbers *);
	void UnLink(); // called when a script is being reloaded (clears string table)

	// Compile scenario script data (without strings and constants)
	void CompileFunc(StdCompiler *pComp, C4ValueNumbers * numbers);

	friend class C4AulFunc;
	friend class C4ScriptHost;
	friend class C4AulParse;
	friend class C4AulDebug;
	friend class C4AulScript;
};

extern C4AulScriptEngine ScriptEngine;
#endif
