/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2013, The OpenClonk Team and contributors
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

#ifndef INC_C4Aul
#define INC_C4Aul

#include <C4Id.h>
#include <C4StringTable.h>
#include <C4Value.h>
#include <C4ValueMap.h>
#include <string>
#include <vector>

// consts
#define C4AUL_MAX_Identifier  100 // max length of function identifiers

// generic C4Aul error class
class C4AulError : public std::exception
{
protected:
	StdCopyStrBuf sMessage;

public:
	bool shown;
	C4AulError();
	virtual ~C4AulError() { } // destructor
	virtual const char *what() const noexcept;
	void show(); // present error message
};

// parse error
class C4AulParseError : public C4AulError
{
public:
	C4AulParseError(C4ScriptHost *pScript, const char *pMsg, const char *pIdtf = NULL, bool Warn = false); // constructor
	C4AulParseError(class C4AulParse * state, const char *pMsg, const char *pIdtf = NULL, bool Warn = false); // constructor
};

// execution error
class C4AulExecError : public C4AulError
{
public:
	C4AulExecError(const char *szError);
};

class C4AulFuncMap
{
public:
	C4AulFuncMap();
	~C4AulFuncMap();
	C4AulFunc * GetFirstFunc(const char * Name);
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

// user text file to which scripts can write using FileWrite().
// actually just writes to an internal buffer
class C4AulUserFile
{	
	StdCopyStrBuf sContents;
	int32_t handle;

public:
	C4AulUserFile(int32_t handle) : handle(handle) {}
	void Write(const char *data, size_t data_length) { sContents.Append(data, data_length); }

	const char *GetFileContents() { return sContents.getData(); }
	StdStrBuf GrabFileContents() { StdStrBuf r; r.Take(sContents); return r; }
	size_t GetFileLength() { return sContents.getLength(); }
	int32_t GetHandle() const { return handle; }
};

// holds all C4AulScripts
class C4AulScriptEngine: public C4PropListStaticMember
{
protected:
	C4AulFuncMap FuncLookUp;
	C4AulFunc * GetFirstFunc(const char * Name)
	{ return FuncLookUp.GetFirstFunc(Name); }
	C4AulFunc * GetNextSNFunc(const C4AulFunc * After)
	{ return FuncLookUp.GetNextSNFunc(After); }
	C4ScriptHost *Child0, *ChildL; // tree structure

	// all open user files
	// user files aren't saved - they are just open temporary e.g. during game saving
	std::list<C4AulUserFile> UserFiles;

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
	void ReLink(C4DefList *rDefs); // unlink, link and parse all scripts
	C4PropListStatic * GetPropList() { return this; }
	bool ReloadScript(const char *szScript, const char *szLanguage); // search script and reload, if found

	// For the list of functions in the PropertyDlg
	std::list<const char*> GetFunctionNames(C4PropList *);

	void RegisterGlobalConstant(const char *szName, const C4Value &rValue); // creates a new constants or overwrites an old one
	bool GetGlobalConstant(const char *szName, C4Value *pTargetValue); // check if a constant exists; assign value to pTargetValue if not NULL

	void Denumerate(C4ValueNumbers *);
	void UnLink(); // called when a script is being reloaded (clears string table)

	// Compile scenario script data (without strings and constants)
	void CompileFunc(StdCompiler *pComp, C4ValueNumbers * numbers);

	// Handle user files
	int32_t CreateUserFile(); // create new file and return handle
	void CloseUserFile(int32_t handle); // close user file given by handle
	C4AulUserFile *GetUserFile(int32_t handle); // get user file given by handle

	friend class C4AulFunc;
	friend class C4AulProfiler;
	friend class C4ScriptHost;
	friend class C4AulParse;
	friend class C4AulDebug;
};

extern C4AulScriptEngine ScriptEngine;
void InitCoreFunctionMap(C4AulScriptEngine *pEngine);

#endif
