/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2009-2019, The OpenClonk Team and contributors
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

/* Property lists */

#include "script/C4Value.h"
#include "script/C4StringTable.h"

#ifndef C4PROPLIST_H
#define C4PROPLIST_H

/* C4PropList have a somewhat complicated reference counting scheme. You can:
 - Store a C4Proplist* in a C4Value. This is the preferred and often only way.
 - Store a C4Object* from GetObject in a C4Object* or C4PropList* if there is a ClearPointer function for it
   Use a C4ObjectPtr for help with storing the Object in Savegames.
 - Store a C4Def* from GetDef() in a C4Def* or C4PropList*

All PropLists can be destroyed while there are still C4Values referencing them, though
Definitions do not get destroyed during the game. So always check for nullpointers.

The unordered_set Refs is used to change all C4Values referencing the destroyed Proplist to contain nil instead.
Objects are also cleaned up via various ClearPointer functions.
The list is also used as a reference count to remove unused Proplists.
The exception are C4PropListNumbered and C4Def, which have implicit references
from C4GameObjects, C4Object and C4DefList. They have to be destroyed when loosing that reference.*/

class C4Property
{
public:
	C4Property() = default;
	C4Property(C4String *Key, const C4Value &Value) : Key(Key), Value(Value)
	{ assert(Key); Key->IncRef(); }
	C4Property(const C4Property &o) : Key(o.Key), Value(o.Value) { if (Key) Key->IncRef(); }
	C4Property & operator = (const C4Property &o)
	{ if(o.Key) o.Key->IncRef(); if (Key) Key->DecRef(); Key = o.Key; Value = o.Value; return *this; }
	C4Property(C4Property && o) : Key(o.Key), Value(std::move(o.Value)) { o.Key = nullptr; }
	C4Property & operator = (C4Property && o)
	{ if (Key) Key->DecRef(); Key = o.Key; o.Key = nullptr; Value = std::move(o.Value); return *this; }
	~C4Property() { if (Key) Key->DecRef(); }
	void CompileFunc(StdCompiler *pComp, C4ValueNumbers *);
	C4String * Key{nullptr};
	C4Value Value;
	explicit operator bool() const { return Key != nullptr; }
	bool operator < (const C4Property &cmp) const { return strcmp(GetSafeKey(), cmp.GetSafeKey())<0; }
	const char *GetSafeKey() const { if (Key && Key->GetCStr()) return Key->GetCStr(); return ""; } // get key as C string; return "" if undefined. never return nullptr
};

template<>
inline bool C4Set<C4Property>::Equals(const C4Property &a, const C4Property &b)
{
	return a.Key == b.Key;
}

class C4PropListNumbered;
class C4PropList
{
public:
	void Clear() { constant = false; Properties.Clear(); prototype.Set0(); }
	virtual const char *GetName() const;
	virtual void SetName (const char *NewName = nullptr);
	virtual void SetOnFire(bool OnFire) { }

	// These functions return this or a prototype.
	virtual C4Def const * GetDef() const;
	virtual C4Def * GetDef();
	virtual C4Object * GetObject();
	virtual C4Object const * GetObject() const;
	virtual C4Effect * GetEffect();
	virtual C4PropListNumbered * GetPropListNumbered();
	virtual class C4MapScriptLayer * GetMapScriptLayer();
	virtual class C4MapScriptMap * GetMapScriptMap();

	C4PropList * GetPrototype() const { return prototype._getPropList(); }
	void RemoveCyclicPrototypes();

	// saved as a reference to a global constant?
	virtual class C4PropListStatic * IsStatic() { return nullptr; }
	const class C4PropListStatic * IsStatic() const { return const_cast<C4PropList*>(this)->IsStatic(); }
	// saved as a reference to separately saved objects?
	virtual bool IsNumbered() const { return false; }
	// some proplists have references that are not reference-counted
	virtual bool Delete() { return false; }

	// These four operate on properties as seen by script, which can be dynamic
	// or reflect C++ variables
	virtual bool GetPropertyByS(const C4String *k, C4Value *pResult) const;
	virtual C4ValueArray * GetProperties() const;
	// not allowed on frozen proplists
	virtual void SetPropertyByS(C4String * k, const C4Value & to);
	virtual void ResetProperty(C4String * k);

	// helper functions to get dynamic properties from other parts of the engine
	bool GetProperty(C4PropertyName k, C4Value *pResult) const
	{ return GetPropertyByS(&Strings.P[k], pResult); }
	C4String * GetPropertyStr(C4PropertyName k) const;
	C4ValueArray * GetPropertyArray(C4PropertyName n) const;
	C4AulFunc * GetFunc(C4PropertyName k) const
	{ return GetFunc(&Strings.P[k]); }
	C4AulFunc * GetFunc(C4String * k) const;
	C4AulFunc * GetFunc(const char * k) const;
	C4String * EnumerateOwnFuncs(C4String * prev = nullptr) const;
	C4Value Call(C4PropertyName k, C4AulParSet *pPars=nullptr, bool fPassErrors=false)
	{ return Call(&Strings.P[k], pPars, fPassErrors); }
	C4Value Call(C4String * k, C4AulParSet *pPars=nullptr, bool fPassErrors=false);
	C4Value Call(const char * k, C4AulParSet *pPars=nullptr, bool fPassErrors=false);
	C4PropertyName GetPropertyP(C4PropertyName k) const;
	int32_t GetPropertyBool(C4PropertyName n, bool default_val = false) const;
	int32_t GetPropertyInt(C4PropertyName k, int32_t default_val = 0) const;
	C4PropList *GetPropertyPropList(C4PropertyName k) const;
	bool HasProperty(C4String * k) const { return Properties.Has(k); }
	// not allowed on frozen proplists
	void SetProperty(C4PropertyName k, const C4Value & to)
	{ SetPropertyByS(&Strings.P[k], to); }

	static C4PropList * New(C4PropList * prototype = nullptr);
	static C4PropListStatic * NewStatic(C4PropList * prototype, const C4PropListStatic * parent, C4String * key);

	// only freeze proplists which are not going to be modified
	// FIXME: Only C4PropListStatic get frozen. Optimize accordingly.
	void Freeze() { constant = true; }
	void Thaw() { constant = false; }
	void ThawRecursively();
	bool IsFrozen() const { return constant; }

	// Freeze this and all proplist in properties and ensure they are static proplists
	// If a proplist is not static, replace it with a static proplist and replace all instances
	// Place references to all proplists made static in the given value array
	C4PropListStatic *FreezeAndMakeStaticRecursively(std::vector<C4Value>* prop_lists, const C4PropListStatic *parent = nullptr, C4String * key = nullptr);

	virtual void Denumerate(C4ValueNumbers *);
	virtual ~C4PropList();

	void CompileFunc(StdCompiler *pComp, C4ValueNumbers *);
	void AppendDataString(StdStrBuf * out, const char * delim, int depth = 3, bool ignore_reference_parent = false) const;
	StdStrBuf ToJSON(int depth = 10, bool ignore_reference_parent = false) const;
	std::vector< C4String * > GetSortedLocalProperties(bool add_prototype=true) const;
	std::vector< C4String * > GetSortedLocalProperties(const char *prefix, const C4PropList *ignore_overridden) const;
	std::vector< C4String * > GetUnsortedProperties(const char *prefix, C4PropList *ignore_parent = nullptr) const;
	std::vector< C4String * > GetSortedProperties(const char *prefix, C4PropList *ignore_parent = nullptr) const;

	bool operator==(const C4PropList &b) const;
#ifdef _DEBUG
	static C4Set<C4PropList *> PropLists;
#endif	

protected:
	C4PropList(C4PropList * prototype = nullptr);
	void ClearRefs() { for( C4Value * ref: RefSet{Refs}) ref->Set0(); assert(Refs.empty()); }

private:
	void AddRef(C4Value *pRef);
	void DelRef(C4Value *pRef);
	typedef std::unordered_set<C4Value *> RefSet;
	RefSet Refs;
	C4Set<C4Property> Properties;
	C4Value prototype;
	bool constant{false}; // if true, this proplist is not changeable
	friend class C4Value;
	friend class C4ScriptHost;
public:
	int32_t Status{1};

	class Iterator
	{
	private:
		std::shared_ptr<std::vector<const C4Property*> > properties;
		std::vector<const C4Property*>::iterator iter;
		// needed when constructing the iterator
		// adds a property or overwrites existing property with same name
		void AddProperty(const C4Property * prop);
		void Reserve(size_t additionalAmount);
		// Initializes internal iterator. Needs to be called before actually using the iterator.
		void Init();
	public:
		Iterator() : properties(nullptr) { }

		const C4Property * operator*() const { return *iter; }
		const C4Property * operator->() const { return *iter; }
		void operator++() { ++iter; };
		void operator++(int) { operator++(); }

		bool operator==(const Iterator & other) const
		{
			if ((properties == nullptr || iter == properties->end()) && (other.properties == nullptr || other.iter == other.properties->end()))
				return true;
			return properties == other.properties && iter == other.iter;
		}

		bool operator!=(const Iterator & other) const
		{
			return !(*this == other);
		}

		friend class C4PropList;
	};

	// do not modify the proplist while iterating over it!
	Iterator begin();
	Iterator end() { return Iterator(); }
};

void CompileNewFunc(C4PropList *&pStruct, StdCompiler *pComp, C4ValueNumbers *rPar);

// Proplists that are created during a game and get saved in a savegame
// Examples: Objects, Effects
class C4PropListNumbered: public C4PropList
{
public:
	int32_t Number{-1};
	~C4PropListNumbered() override;
	void CompileFunc(StdCompiler *pComp, C4ValueNumbers * numbers);
	C4PropListNumbered * GetPropListNumbered() override;
	bool IsNumbered() const override { return true; }

	static C4PropList * GetByNumber(int32_t iNumber); // pointer by number
	static bool CheckPropList(C4PropList *); // sanity check: true when the proplist is in the list and not a stale pointer
	static void SetEnumerationIndex(int32_t iMaxObjectNumber);
	static int32_t GetEnumerationIndex() { return EnumerationIndex; }
	static void ResetEnumerationIndex();
	static void ShelveNumberedPropLists(); // unnumber all proplists and put them on the shelve. To be used on remaining objects before a savegame load.
	static void UnshelveNumberedPropLists(); // re-insert shelved proplists into main list
	static void ClearShelve();
	static void ClearNumberedPropLists(); // empty all properties in numbered prop lists. Used on game clear to ensure prop lists with circular references get cleared.
protected:
	C4PropListNumbered(C4PropList * prototype = nullptr);
	void AcquireNumber(); // acquire a number and add to internal list
	void ClearNumber(); // clear number and remove from internal list

	static C4Set<C4PropListNumbered *> PropLists;
	static std::vector<C4PropListNumbered *> ShelvedPropLists; // temporary storage for existing proplists while a new section loaded
	static int32_t EnumerationIndex;
	friend class C4Game;
	friend class C4GameObjects;
};

// Proplists created by script at runtime
class C4PropListScript: public C4PropList
{
public:
	C4PropListScript(C4PropList * prototype = nullptr) : C4PropList(prototype) { PropLists.Add(this);  }
	~C4PropListScript() override { PropLists.Remove(this); }
	bool Delete() override { return true; }

	static void ClearScriptPropLists(); // empty all properties in script-created prop lists. Used on game clear to ensure prop lists with circular references get cleared.

protected:
	static C4Set<C4PropListScript *> PropLists;
};

// PropLists declared in the game data
// examples: Definitions, local variable initializers
class C4PropListStatic: public C4PropList
{
public:
	C4PropListStatic(C4PropList * prototype, const C4PropListStatic * parent, C4String * key):
		C4PropList(prototype), Parent(parent), ParentKeyName(key) { }
	~C4PropListStatic() override = default;
	bool Delete() override { return true; }
	C4PropListStatic * IsStatic() override { return this; }
	void RefCompileFunc(StdCompiler *pComp, C4ValueNumbers * numbers) const;
	StdStrBuf GetDataString() const;
	const char *GetName() const override;
	const C4PropListStatic * GetParent() const { return Parent; }
	C4String * GetParentKeyName() { return ParentKeyName; }
protected:
	const C4PropListStatic * Parent;
	C4RefCntPointer<C4String> ParentKeyName; // property in parent this proplist was created in
};

// static PropList of which another class owns the pointer
class C4PropListStaticMember : public C4PropListStatic
{
public:
	C4PropListStaticMember(C4PropList * prototype, const C4PropListStatic * parent, C4String * key):
	  C4PropListStatic(prototype, parent, key) {}
	bool Delete() override { return false; }
};

#endif // C4PROPLIST_H
