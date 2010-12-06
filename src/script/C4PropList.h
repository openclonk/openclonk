/*
 * Copyright (c) 2009  Günther Brammer <gbrammer@gmx.de>
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

#ifndef C4PROPLIST_H
#define C4PROPLIST_H

#include "C4Value.h"
#include "C4StringTable.h"

class C4Def;

class C4Property
{
public:
	C4Property() : Key(0) {}
	C4Property(C4String *Key, const C4Value &Value) : Key(Key), Value(Value)
	{ assert(Key); Key->IncRef(); assert(Strings.Set.Has(Key)); }
	C4Property(const C4Property &o) : Key(o.Key), Value(o.Value) { if (Key) Key->IncRef(); }
	C4Property & operator = (const C4Property &o)
	{ assert(o.Key); o.Key->IncRef(); if (Key) Key->DecRef(); Key = o.Key; Value = o.Value; return *this; }
	~C4Property() { if (Key) Key->DecRef(); }
	void CompileFunc(StdCompiler *pComp);
	C4String * Key;
	C4Value Value;
	operator const void * () const { return Key; }
	C4Property & operator = (void * p)
	{ assert(!p); if (Key) Key->DecRef(); Key = 0; Value.Set0(); return *this; }
};
class C4PropListNumbered;
class C4PropList
{
public:
	int32_t Status;
	void AddRef(C4Value *pRef);
	void DelRef(const C4Value *pRef, C4Value * pNextRef);
	void Clear() { constant = false; Properties.Clear(); prototype = 0; }
	const char *GetName();
	virtual void SetName (const char *NewName = 0);

	virtual C4Def const * GetDef() const;
	virtual C4Def * GetDef();
	virtual C4Object * GetObject();
	virtual C4PropListNumbered * GetPropListNumbered();
	C4PropList * GetPrototype() const { return prototype; }

	// Whether this proplist should be saved as a reference to a C4Def
	virtual bool IsDef() const { return false; }

	bool GetPropertyByS(C4String *k, C4Value *pResult) const;
	bool GetProperty(C4PropertyName k, C4Value *pResult) const
	{ return GetPropertyByS(Strings.P[k], pResult); }
	C4String * GetPropertyStr(C4PropertyName k) const;
	int32_t GetPropertyInt(C4PropertyName k) const;
	// not allowed on frozen proplists
	void SetPropertyByS(C4String * k, const C4Value & to);
	void SetProperty(C4PropertyName k, const C4Value & to)
	{ SetPropertyByS(Strings.P[k], to); }
	void ResetProperty(C4String * k);

	static C4PropList * New(C4PropList * prototype = 0);
	static C4PropList * NewAnon(C4PropList * prototype = 0);

	// only freeze proplists which are not going to be modified
	void Freeze() { constant = true; }
	bool IsFrozen() const { return constant; }

	virtual void DenumeratePointers();
	virtual ~C4PropList();

	// Every proplist has to be initialized by either Init or CompileFunc.
	void CompileFunc(StdCompiler *pComp);

	bool operator==(const C4PropList &b) const;

protected:
	C4PropList(C4PropList * prototype = 0);
	C4Value *FirstRef; // No-Save	

private:
	C4Set<C4Property> Properties;
	bool constant; // if true, this proplist is not changeable

	C4PropList * prototype;
	friend void CompileNewFunc<C4PropList>(C4PropList *&pStruct, StdCompiler *pComp);
};

class C4PropListNumbered: public C4PropList
{
public:
	int32_t Number;
	C4PropListNumbered(C4PropList * prototype = 0);
	~C4PropListNumbered();
	void CompileFunc(StdCompiler *pComp);
	virtual C4PropListNumbered * GetPropListNumbered();
	void AcquireNumber();
};


#endif // C4PROPLIST_H
