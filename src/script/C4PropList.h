/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2009-2011  Günther Brammer
 * Copyright (c) 2009  Nicolas Hake
 * Copyright (c) 2010  Benjamin Herr
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 */

/* Property lists */

#include "C4Value.h"
#include "C4StringTable.h"

#ifndef C4PROPLIST_H
#define C4PROPLIST_H


class C4Property
{
public:
	C4Property() : Key(0) {}
	C4Property(C4String *Key, const C4Value &Value) : Key(Key), Value(Value)
	{ assert(Key); Key->IncRef(); /*assert(Strings.Set.Has(Key));*/ }
	C4Property(const C4Property &o) : Key(o.Key), Value(o.Value) { if (Key) Key->IncRef(); }
	C4Property & operator = (const C4Property &o)
	{ if(o.Key) o.Key->IncRef(); if (Key) Key->DecRef(); Key = o.Key; Value = o.Value; return *this; }
#ifdef HAVE_RVALUE_REF
	C4Property(C4Property && o) : Key(o.Key), Value(std::move(o.Value)) { o.Key = 0; }
	C4Property & operator = (C4Property && o)
	{ if (Key) Key->DecRef(); Key = o.Key; o.Key = 0; Value = std::move(o.Value); return *this; }
#endif
	~C4Property() { if (Key) Key->DecRef(); }
	void CompileFunc(StdCompiler *pComp, C4ValueNumbers *);
	C4String * Key;
	C4Value Value;
	operator const void * () const { return Key; }
	C4Property & operator = (void * p)
	{ assert(!p); if (Key) Key->DecRef(); Key = 0; Value.Set0(); return *this; }
	bool operator < (const C4Property &cmp) const { return strcmp(GetSafeKey(), cmp.GetSafeKey())<0; }
	const char *GetSafeKey() const { if (Key && Key->GetCStr()) return Key->GetCStr(); return ""; } // get key as C string; return "" if undefined. never return NULL
};
class C4PropListNumbered;
class C4PropList
{
public:
	void AddRef(C4Value *pRef);
	void DelRef(const C4Value *pRef, C4Value * pNextRef);
	void Clear() { constant = false; Properties.Clear(); prototype = 0; }
	const char *GetName() const;
	virtual void SetName (const char *NewName = 0);

	virtual C4Def const * GetDef() const;
	virtual C4Def * GetDef();
	virtual C4Object * GetObject();
	virtual C4Effect * GetEffect();
	virtual C4PropListNumbered * GetPropListNumbered();
	C4PropList * GetPrototype() const { return prototype; }

	// saved as a reference to a global constant?
	virtual class C4PropListStatic * IsStatic() { return NULL; }
	// saved as a reference to separately saved objects?
	virtual bool IsNumbered() const { return false; }
	// some proplists have references that are not reference-counted
	virtual bool Delete() { return false; }

	// These four operate on properties as seen by script, which can be dynamic
	// or reflect C++ variables
	virtual bool GetPropertyByS(C4String *k, C4Value *pResult) const;
	virtual C4ValueArray * GetProperties() const;
	// not allowed on frozen proplists
	virtual void SetPropertyByS(C4String * k, const C4Value & to);
	virtual void ResetProperty(C4String * k);

	// helper functions to get dynamic properties from other parts of the engine
	bool GetProperty(C4PropertyName k, C4Value *pResult) const
	{ return GetPropertyByS(&Strings.P[k], pResult); }
	C4String * GetPropertyStr(C4PropertyName k) const;
	C4AulFunc * GetFunc(C4PropertyName k) const
	{ return GetFunc(&Strings.P[k]); }
	C4AulFunc * GetFunc(C4String * k) const;
	C4AulFunc * GetFunc(const char * k) const;
	C4String * EnumerateOwnFuncs(C4String * prev = 0) const;
	C4Value Call(C4PropertyName k, C4AulParSet *pPars=0)
	{ return Call(&Strings.P[k], pPars); }
	C4Value Call(C4String * k, C4AulParSet *pPars=0);
	C4Value Call(const char * k, C4AulParSet *pPars=0);
	C4Value CallOrThrow(const char * k, C4AulParSet *pPars=0);
	C4PropertyName GetPropertyP(C4PropertyName k) const;
	int32_t GetPropertyInt(C4PropertyName k) const;
	bool HasProperty(C4String * k) { return Properties.Has(k); }
	// not allowed on frozen proplists
	void SetProperty(C4PropertyName k, const C4Value & to)
	{ SetPropertyByS(&Strings.P[k], to); }

	static C4PropList * New(C4PropList * prototype = 0);
	static C4PropListStatic * NewAnon(C4PropList * prototype, const C4PropListStatic * parent, C4String * key);

	// only freeze proplists which are not going to be modified
	// FIXME: Only C4PropListStatic get frozen. Optimize accordingly.
	void Freeze() { constant = true; }
	void Thaw() { constant = false; }
	bool IsFrozen() const { return constant; }

	virtual void Denumerate(C4ValueNumbers *);
	virtual ~C4PropList();

	void CompileFunc(StdCompiler *pComp, C4ValueNumbers *);
	void AppendDataString(StdStrBuf * out, const char * delim, int depth = 3);

	bool operator==(const C4PropList &b) const;
#ifdef _DEBUG
	static C4Set<C4PropList *> PropLists;
#endif	

protected:
	C4PropList(C4PropList * prototype = 0);
	C4Value *FirstRef; // No-Save

private:
	C4Set<C4Property> Properties;
	C4PropList * prototype;
	bool constant; // if true, this proplist is not changeable
	friend class C4ScriptHost;
public:
	int32_t Status;
};

void CompileNewFunc(C4PropList *&pStruct, StdCompiler *pComp, C4ValueNumbers * const & rPar);

// Proplists that are created during a game and get saved in a savegame
// Examples: Objects, Effects
class C4PropListNumbered: public C4PropList
{
public:
	int32_t Number;
	~C4PropListNumbered();
	void CompileFunc(StdCompiler *pComp, C4ValueNumbers * numbers);
	virtual C4PropListNumbered * GetPropListNumbered();
	void AcquireNumber();
	virtual bool IsNumbered() const { return true; }

	static C4PropList * GetByNumber(int32_t iNumber); // pointer by number
	static bool CheckPropList(C4PropList *); // sanity check: true when the proplist is in the list and not a stale pointer
	static void SetEnumerationIndex(int32_t iMaxObjectNumber);
	static int32_t GetEnumerationIndex() { return EnumerationIndex; }
	static void ResetEnumerationIndex();
protected:
	C4PropListNumbered(C4PropList * prototype = 0);

	static C4Set<C4PropListNumbered *> PropLists;
	static int32_t EnumerationIndex;
	friend class C4GameObjects;
	friend class C4Game;
};

// Proplists created by script at runtime
class C4PropListScript: public C4PropList
{
public:
	C4PropListScript(C4PropList * prototype = 0): C4PropList(prototype) { }
	bool Delete() { return true; }
};

// PropLists declared in the game data
// examples: Definitions, local variable initializers
class C4PropListStatic: public C4PropList
{
public:
	C4PropListStatic(C4PropList * prototype, const C4PropListStatic * parent, C4String * key):
		C4PropList(prototype), Parent(parent), ParentKeyName(key) { }
	virtual ~C4PropListStatic() { }
	bool Delete() { return true; }
	virtual C4PropListStatic * IsStatic() { return this; }
	void RefCompileFunc(StdCompiler *pComp, C4ValueNumbers * numbers) const;
	StdStrBuf GetDataString() const;
	const C4PropListStatic * GetParent() { return Parent; }
	const C4String * GetParentKeyName() { return ParentKeyName; }
protected:
	const C4PropListStatic * Parent;
	C4RefCntPointer<C4String> ParentKeyName; // property in parent this proplist was created in
};

#endif // C4PROPLIST_H
