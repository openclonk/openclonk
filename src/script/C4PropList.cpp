/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2004, Peter Wortmann
 * Copyright (c) 2007, Günther Brammer
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
#include "script/C4PropList.h"

#include "control/C4Record.h"
#include "object/C4GameObjects.h"
#include "script/C4Aul.h"

void C4PropList::AddRef(C4Value *pRef)
{
#ifdef _DEBUG
	C4Value * pVal = FirstRef;
	while (pVal)
	{
		assert(pVal != pRef);
		pVal = pVal->NextRef;
	}
#endif
	pRef->NextRef = FirstRef;
	FirstRef = pRef;
}

void C4PropList::DelRef(const C4Value * pRef, C4Value * pNextRef)
{
	assert(FirstRef);
	// References to objects never have HasBaseArray set
	if (pRef == FirstRef)
	{
		FirstRef = pNextRef;
		if (pNextRef) return;
	}
	else
	{
		C4Value *pPrev = FirstRef;
		while (pPrev->NextRef != pRef)
		{
			pPrev = pPrev->NextRef;
			assert(pPrev);
		}
		pPrev->NextRef = pNextRef;
		return;
	}
	// Only pure script proplists are garbage collected here, host proplists
	// like definitions and effects have their own memory management.
	if (Delete()) delete this;
}

C4PropList * C4PropList::New(C4PropList * prototype)
{
	C4PropList * r = new C4PropListScript(prototype);
	return r;
}

C4PropListStatic * C4PropList::NewStatic(C4PropList * prototype, const C4PropListStatic * parent, C4String * key)
{
	return new C4PropListStatic(prototype, parent, key);
}

C4PropList *C4PropListNumbered::GetByNumber(int32_t iNumber)
{
	return PropLists.Get(iNumber);
}

bool C4PropListNumbered::CheckPropList(C4PropList *pObj)
{
	if (!pObj) return false;
	C4PropListNumbered * const * p = PropLists.First();
	while (p)
	{
		if (*p == pObj)
			return true;
		p = PropLists.Next(p);
	}
	return false;
}

void C4PropListNumbered::SetEnumerationIndex(int32_t iMaxObjectNumber)
{
	// update object enumeration index now, because calls like OnSynchronized might create objects
	EnumerationIndex = std::max(EnumerationIndex, iMaxObjectNumber);
}

void C4PropListNumbered::ResetEnumerationIndex()
{
	assert(!PropLists.GetSize());
	EnumerationIndex = 0;
}

void C4PropListNumbered::ShelveNumberedPropLists()
{
	// unnumber all proplists and put them on the shelve. To be used on remaining objects before a savegame load.
	assert(ShelvedPropLists.empty());
	ShelvedPropLists.reserve(PropLists.GetSize());
	C4PropListNumbered *const* p_next = PropLists.First(), *const* p;
	while ((p = p_next))
	{
		p_next = PropLists.Next(p);
		C4PropListNumbered *pl = *p;
		if (pl->Number != -1)
		{
			pl->ClearNumber();
			ShelvedPropLists.push_back(pl);
		}
	}
}

void C4PropListNumbered::UnshelveNumberedPropLists()
{
	// re-insert shelved proplists into main list and give them a number
	for (auto & ShelvedPropList : ShelvedPropLists)
		ShelvedPropList->AcquireNumber();
	ShelvedPropLists.clear();
}

void C4PropListNumbered::ClearShelve()
{
	// cleanup shelve - used in game clear, un unsuccessful section load, etc.
	ShelvedPropLists.clear();
}

void C4PropListNumbered::ClearNumberedPropLists()
{
	// empty all proplists to ensure safe deletion of proplists with circular references
	// note that this the call to Clear() might delete some prop lists. So it is assumed that
	// PropLists does not shrink its table as the number of values goes down
	C4PropListNumbered *const* p_next = PropLists.First(), *const* p;
	while ((p = p_next))
	{
		p_next = PropLists.Next(p);
		// check *p since it might have been deleted by clearing the previous prop list
		if (*p) (*p)->Clear();
	}
}

C4PropListNumbered::C4PropListNumbered(C4PropList * prototype): C4PropList(prototype)
{
}

void C4PropListNumbered::AcquireNumber()
{
	// Enumerate object
	do
		Number = ++EnumerationIndex;
	while (PropLists.Get(Number));
	// Add to list
	PropLists.Add(this);
}

void C4PropListNumbered::ClearNumber()
{
	// Make proplist invisible during denumeration process
	if (Number != -1)
	{
		PropLists.Remove(this);
		Number = -1;
	}
}

C4PropListNumbered* C4PropListNumbered::GetPropListNumbered()
{
	return this;
}

void C4PropListNumbered::CompileFunc(StdCompiler *pComp, C4ValueNumbers * numbers)
{
	int32_t n = Number;
	pComp->Value(n);
	pComp->Separator(StdCompiler::SEP_SEP2);
	C4PropList::CompileFunc(pComp, numbers);
	if (pComp->isDeserializer())
	{
		if (PropLists.Get(n))
		{
			pComp->excCorrupt("multiple PropLists with Number %d", n);
			return;
		}
		// Once this proplist has a Number, it has to be in PropLists. See the destructor below.
		Number = n;
		PropLists.Add(this);
	}
}

C4PropListNumbered::~C4PropListNumbered()
{
	if (Number != -1)
		PropLists.Remove(this);
	else
		Log("removing numbered proplist without number");
}

void C4PropListScript::ClearScriptPropLists()
{
	// empty all proplists to ensure safe deletion of proplists with circular references
	// note that this the call to Clear() might delete some prop lists. So it is assumed that
	// PropLists does not shrink its table as the number of values goes down
	// However, some values may be skipped due to table consolidation. Just fix it by iterating over the table until it's empty.
	C4PropListScript *const* p_next, *const* p;
	while ((p_next = PropLists.First()))
	{
		size_t prev_size = PropLists.GetSize();
		while ((p = p_next))
		{
			p_next = PropLists.Next(p);
			// check *p since it might have been deleted by clearing the previous prop list
			if (*p)
			{
				C4Value ref(C4VPropList(*p)); // keep a reference because prop list might delete itself within clearing method otherwise
				(*p)->Clear();
			}
		}
		if (PropLists.GetSize() >= prev_size)
		{
			// Looks like there's a rogue C4Value pointer somewhere.
			// Could just delete the prop list and let ref counting do the job
			// However, it might be better to keep the dead pointer to find the leak in debug mode
#ifdef _DEBUG
			assert(0);
#endif
			break;
		}
	}
}

void C4PropListStatic::RefCompileFunc(StdCompiler *pComp, C4ValueNumbers * numbers) const
{
	assert(!pComp->isDeserializer());
	if (Parent)
	{
		Parent->RefCompileFunc(pComp, numbers);
		pComp->Separator(StdCompiler::SEP_PART);
	}
	if (!ParentKeyName)
		pComp->excCorrupt("C4PropListStatic::RefCompileFunc without ParentKeyName");
	pComp->Value(mkParAdapt(ParentKeyName->GetData(), StdCompiler::RCT_ID));
}

StdStrBuf C4PropListStatic::GetDataString() const
{
	StdStrBuf r;
	if (Parent)
	{
		r.Take(Parent->GetDataString());
		r.AppendChar('.');
	}
	assert(ParentKeyName);
	if (ParentKeyName)
		r.Append(ParentKeyName->GetData());
	return r;
}

const char *C4PropListStatic::GetName() const
{
	const C4String * s = GetPropertyStr(P_Name);
	if (!s) s = ParentKeyName;
	if (!s) return "";
	return s->GetCStr();
}

C4PropList::C4PropList(C4PropList * prototype):
		prototype(prototype)
{
#ifdef _DEBUG
	PropLists.Add(this);
#endif
}

void C4PropList::ThawRecursively()
{
	//thaw self and all owned properties
	Thaw();
	C4PropListStatic *s = IsStatic();
	//if (s) LogF("Thaw: %s", s->GetDataString().getData());
	auto prop_names = GetUnsortedProperties(nullptr, ::ScriptEngine.GetPropList());
	for (auto prop_name : prop_names)
	{
		C4Value child_val;
		GetPropertyByS(prop_name, &child_val);
		//LogF("  %s=%s", prop_name->GetCStr(), child_val.GetDataString(1).getData());
		C4PropList *child_proplist = child_val.getPropList();
		if (child_proplist && child_proplist->IsFrozen())
		{
			child_proplist->ThawRecursively();
		}
	}
}

C4PropListStatic *C4PropList::FreezeAndMakeStaticRecursively(std::vector<C4Value>* prop_lists, const C4PropListStatic *parent, C4String * key)
{
	Freeze();
	// Already static?
	C4PropListStatic *this_static = IsStatic();
	if (!this_static)
	{
		// Make self static by creating a copy and replacing all references
		this_static = NewStatic(GetPrototype(), parent, key);
		this_static->Properties.Swap(&Properties); // grab properties
		this_static->Status = Status;
		C4Value holder = C4VPropList(this);
		while (FirstRef && FirstRef->NextRef)
		{
			C4Value *ref = FirstRef;
			if (ref == &holder) ref = ref->NextRef;
			ref->SetPropList(this_static);
		}
		// store reference
		if (prop_lists)
		{
			prop_lists->push_back(C4VPropList(this_static));
		}
		// "this" should be deleted as holder goes out of scope
	}
	// Iterate over sorted list of elements to make static
	// Must iterate over sorted list because the order must be defined, just in case it's a network game
	// and a non-static child proplist is available through different paths it should still get the same name
	auto prop_names = this_static->GetSortedLocalProperties(false);
	for (auto prop_name : prop_names)
	{
		C4Value child_val;
		this_static->GetPropertyByS(prop_name, &child_val);
		C4PropList *child_proplist = child_val.getPropList();
		if (child_proplist)
		{
			// Avoid infinite recursion: Only freeze into unfrozen children and "true" static children
			C4PropListStatic *child_static = child_proplist->IsStatic();
			if (!child_static || (child_static->GetParent() == this_static && child_static->GetParentKeyName() == prop_name))
			{
				child_proplist->FreezeAndMakeStaticRecursively(prop_lists, this_static, prop_name);
			}
		}
	}
	return this_static;
}

void C4PropList::Denumerate(C4ValueNumbers * numbers)
{
	const C4Property * p = Properties.First();
	while (p)
	{
		const_cast<C4Value &>(p->Value).Denumerate(numbers);
		p = Properties.Next(p);
	}
	prototype.Denumerate(numbers);
	RemoveCyclicPrototypes();
}

C4PropList::~C4PropList()
{
	while (FirstRef)
	{
		// Manually kill references so DelRef doesn't destroy us again
		FirstRef->Data = nullptr; FirstRef->Type = C4V_Nil;
		C4Value *ref = FirstRef;
		FirstRef = FirstRef->NextRef;
		ref->NextRef = nullptr;
	}
#ifdef _DEBUG
	assert(PropLists.Has(this));
	PropLists.Remove(this);
#endif
	assert(!C4PropListNumbered::CheckPropList(this));
}

bool C4PropList::operator==(const C4PropList &b) const
{
	// every numbered proplist has a unique number and is only identical to itself
	if (this == &b) return true;
	if (IsNumbered() || b.IsNumbered()) return false;
	if (Properties.GetSize() != b.Properties.GetSize()) return false;
	if (GetDef() != b.GetDef()) return false;
	const C4Property * p = Properties.First();
	while (p)
	{
		const C4Property & bp = b.Properties.Get(p->Key);
		if (!bp) return false;
		if (p->Value != bp.Value) return false;
		p = Properties.Next(p);
	}
	return true;
}

void C4PropList::CompileFunc(StdCompiler *pComp, C4ValueNumbers * numbers)
{
	bool oldFormat = false;
	// constant proplists are not serialized to savegames, but recreated from the game data instead
	assert(!constant);
	if (pComp->isDeserializer() && pComp->hasNaming())
	{
		// backwards compat to savegames and scenarios before 5.5
		try
		{
			pComp->Value(constant);
			oldFormat = true;
		}
		catch (StdCompiler::NotFoundException *pEx)
		{
			delete pEx;
			pComp->Value(mkParAdapt(prototype, numbers));
		}
	}
	else
		pComp->Value(mkParAdapt(prototype, numbers));
	pComp->Separator(StdCompiler::SEP_SEP2);
	pComp->Value(mkParAdapt(Properties, numbers));
	if (oldFormat)
	{
		if (Properties.Has(&::Strings.P[P_Prototype]))
		{
			prototype = Properties.Get(&::Strings.P[P_Prototype]).Value;
			Properties.Remove(&::Strings.P[P_Prototype]);
		}
	}
}

void C4PropList::RemoveCyclicPrototypes()
{
	// clear any cyclic prototype chains
	// Use prototype.getPropList() instead of GetPrototype() because denumeration might not be completed yet
	for(C4PropList * it = prototype.getPropList(); it; it = it->prototype.getPropList())
		if(it == this)
		{
			prototype.Set0();
		}
}

void CompileNewFunc(C4PropList *&pStruct, StdCompiler *pComp, C4ValueNumbers *rPar)
{
	std::unique_ptr<C4PropList> temp(C4PropList::New()); // exception-safety
	pComp->Value(mkParAdapt(*temp, rPar));
	pStruct = temp.release();
}

template<typename T>
void C4Set<T>::CompileFunc(StdCompiler *pComp, C4ValueNumbers * numbers)
{
	bool fNaming = pComp->hasNaming();
	if (pComp->isDeserializer())
	{
		// Compiling: Empty previous
		Clear();
		// Read size (binary only)
		uint32_t iSize;
		if (!pComp->hasNaming()) pComp->Value(iSize);
		// Read new
		do
		{
			// No entries left to read?
			if (!fNaming && !iSize--)
				break;
			// Read entries
			try
			{
				T e;
				// This could use the same technique StdArrayAdapt uses
				// instead of hardcoding mkParAdapt here
				pComp->Value(mkParAdapt(e, numbers));
				Add(e);
			}
			catch (StdCompiler::NotFoundException *pEx)
			{
				// No value found: Stop reading loop
				delete pEx;
				break;
			}
		}
		while (pComp->Separator(StdCompiler::SEP_SEP));
	}
	else
	{
		// Write size (binary only)
		if (!fNaming)
		{
			int32_t iSize = GetSize();
			pComp->Value(iSize);
		}
		// Write all entries
		const T * p = First();
		while (p)
		{
			pComp->Value(mkParAdapt(*const_cast<T *>(p), numbers));
			p = Next(p);
			if (p) pComp->Separator(StdCompiler::SEP_SEP);
		}
	}
}

void C4Property::CompileFunc(StdCompiler *pComp, C4ValueNumbers * numbers)
{
	StdStrBuf s;
	if (!pComp->isDeserializer())
		s = Key->GetData();
	pComp->Value(s);
	if (pComp->isDeserializer())
	{
		if (Key) Key->DecRef();
		Key = ::Strings.RegString(s);
		Key->IncRef();
	}
	pComp->Separator(StdCompiler::SEP_SET);
	pComp->Value(mkParAdapt(Value, numbers));
}

void C4PropList::AppendDataString(StdStrBuf * out, const char * delim, int depth, bool ignore_reference_parent) const
{
	StdStrBuf & DataString = *out;
	if (depth <= 0 && Properties.GetSize())
	{
		DataString.Append("...");
		return;
	}
	bool has_elements = false;
	// Append prototype
	if (prototype)
	{
		DataString.Append("Prototype = ");
		DataString.Append(prototype.GetDataString(depth - 1, ignore_reference_parent ? IsStatic() : nullptr));
		has_elements = true;
	}
	// Append other properties
	std::list<const C4Property *> sorted_props = Properties.GetSortedListOfElementPointers();
	for (std::list<const C4Property *>::const_iterator p = sorted_props.begin(); p != sorted_props.end(); ++p)
	{
		if (has_elements) DataString.Append(delim);
		DataString.Append((*p)->Key->GetData());
		DataString.Append(" = ");
		DataString.Append((*p)->Value.GetDataString(depth - 1, ignore_reference_parent ? IsStatic() : nullptr));
		has_elements = true;
	}
}

StdStrBuf C4PropList::ToJSON(int depth, bool ignore_reference_parent) const
{
	if (depth <= 0 && Properties.GetSize())
	{
		throw new C4JSONSerializationError("maximum depth reached");
	}
	StdStrBuf DataString;
	DataString = "{";
	bool has_elements = false;
	// Append prototype
	if (prototype)
	{
		DataString.Append("Prototype:");
		DataString.Append(prototype.ToJSON(depth - 1, ignore_reference_parent ? IsStatic() : nullptr));
		has_elements = true;
	}
	// Append other properties
	std::list<const C4Property *> sorted_props = Properties.GetSortedListOfElementPointers();
	for (std::list<const C4Property *>::const_iterator p = sorted_props.begin(); p != sorted_props.end(); ++p)
	{
		if (has_elements) DataString.Append(",");
		DataString.Append(C4Value((*p)->Key).ToJSON());
		DataString.Append(":");
		DataString.Append((*p)->Value.ToJSON(depth - 1, ignore_reference_parent ? IsStatic() : nullptr));
		has_elements = true;
	}
	DataString.Append("}");
	return DataString;
}

std::vector< C4String * > C4PropList::GetSortedLocalProperties(bool add_prototype) const
{
	// return property list without descending into prototype
	std::list<const C4Property *> sorted_props = Properties.GetSortedListOfElementPointers();
	std::vector< C4String * > result;
	result.reserve(sorted_props.size() + add_prototype);
	if (add_prototype) result.push_back(&::Strings.P[P_Prototype]); // implicit prototype for every prop list
	for (auto p : sorted_props) result.push_back(p->Key);
	return result;
}

std::vector< C4String * > C4PropList::GetSortedLocalProperties(const char *prefix, const C4PropList *ignore_overridden) const
{
	// return property list without descending into prototype
	// ignore properties that have been overridden by proplist given in ignore_overridden or any of its prototypes up to and excluding this
	std::vector< C4String * > result;
	for (const C4Property *pp = Properties.First(); pp; pp = Properties.Next(pp))
		if (pp->Key != &::Strings.P[P_Prototype])
			if (!prefix || pp->Key->GetData().BeginsWith(prefix))
			{
				// Override check
				const C4PropList *check = ignore_overridden;
				bool overridden = false;
				if (check && check != this)
				{
					if (check->HasProperty(pp->Key)) { overridden = true; break; }
					check = check->GetPrototype();
				}
				result.push_back(pp->Key);
			}
	// Sort
	std::sort(result.begin(), result.end(), [](const C4String *a, const C4String *b) -> bool
	{
		return strcmp(a->GetCStr(), b->GetCStr()) < 0;
	});
	return result;
}

std::vector< C4String * > C4PropList::GetUnsortedProperties(const char *prefix, C4PropList *ignore_parent) const
{
	// Return property list with descending into prototype
	// But do not include Prototype property
	std::vector< C4String * > result;
	const C4PropList *p = this;
	do
	{
		for (const C4Property *pp = p->Properties.First(); pp; pp = p->Properties.Next(pp))
			if (pp->Key != &::Strings.P[P_Prototype])
				if (!prefix || pp->Key->GetData().BeginsWith(prefix))
					result.push_back(pp->Key);
		p = p->GetPrototype();
		if (p == ignore_parent) break;
	} while (p);
	return result;
}

std::vector< C4String * > C4PropList::GetSortedProperties(const char *prefix, C4PropList *ignore_parent) const
{
	struct sort_cmp {
		bool operator() (const C4String *a, const C4String *b) const
		{
			return strcmp(a->GetCStr(), b->GetCStr()) < 0;
		}
	};
	// Return property list with descending into prototype
	// But do not include Prototype property
	std::vector< C4String * > result = GetUnsortedProperties(prefix, ignore_parent);
	// Sort and remove duplicates
	std::set< C4String *, sort_cmp > result_set(result.begin(), result.end());
	result.assign(result_set.begin(), result_set.end());
	return result;
}

const char * C4PropList::GetName() const
{
	C4String * s = GetPropertyStr(P_Name);
	if (!s) return "";
	return s->GetCStr();
}

void C4PropList::SetName(const char* NewName)
{
	if (!NewName)
		ResetProperty(&Strings.P[P_Name]);
	else
	{
		SetProperty(P_Name, C4VString(NewName));
	}
}


C4Object * C4PropList::GetObject()
{
	if (GetPrototype()) return GetPrototype()->GetObject();
	return nullptr;
}

C4Object const * C4PropList::GetObject() const
{
	if (GetPrototype()) return GetPrototype()->GetObject();
	return nullptr;
}

C4Def * C4PropList::GetDef()
{
	if (GetPrototype()) return GetPrototype()->GetDef();
	return nullptr;
}

C4Def const * C4PropList::GetDef() const
{
	if (GetPrototype()) return GetPrototype()->GetDef();
	return nullptr;
}

class C4MapScriptLayer * C4PropList::GetMapScriptLayer()
{
	if (GetPrototype()) return GetPrototype()->GetMapScriptLayer();
	return nullptr;
}

class C4MapScriptMap * C4PropList::GetMapScriptMap()
{
	if (GetPrototype()) return GetPrototype()->GetMapScriptMap();
	return nullptr;
}

C4PropListNumbered * C4PropList::GetPropListNumbered()
{
	if (GetPrototype()) return GetPrototype()->GetPropListNumbered();
	return nullptr;
}

C4Effect * C4PropList::GetEffect()
{
	if (GetPrototype()) return GetPrototype()->GetEffect();
	return nullptr;
}

template<> template<>
unsigned int C4Set<C4Property>::Hash<const C4String *>(C4String const * const & e)
{
	assert(e);
	unsigned int hash = 4, tmp;
	hash += ((uintptr_t)e) >> 16;
	tmp   = ((((uintptr_t)e) & 0xffff) << 11) ^ hash;
	hash  = (hash << 16) ^ tmp;
	hash += hash >> 11;
	hash ^= hash << 3;
	hash += hash >> 5;
	hash ^= hash << 4;
	hash += hash >> 17;
	hash ^= hash << 25;
	hash += hash >> 6;
	return hash;
}

template<> template<>
unsigned int C4Set<C4Property>::Hash<C4String *>(C4String * const & e)
{
	return Hash<const C4String *>(e);
}

template<> template<>
bool C4Set<C4Property>::Equals<const C4String *>(C4Property const & a, C4String const * const & b)
{
	return a.Key == b;
}

template<> template<>
bool C4Set<C4Property>::Equals<C4String *>(C4Property const & a, C4String * const & b)
{
	return a.Key == b;
}

template<> template<>
unsigned int C4Set<C4Property>::Hash<C4Property>(C4Property const & p)
{
	return C4Set<C4Property>::Hash(p.Key);
}

bool C4PropList::GetPropertyByS(const C4String * k, C4Value *pResult) const
{
	if (Properties.Has(k))
	{
		*pResult = Properties.Get(k).Value;
		return true;
	}
	else if (k == &Strings.P[P_Prototype])
	{
		*pResult = prototype;
		return true;
	}
	else if(GetPrototype())
		return GetPrototype()->GetPropertyByS(k, pResult);
	else
		return false;
}

C4String * C4PropList::GetPropertyStr(C4PropertyName n) const
{
	C4String * k = &Strings.P[n];
	if (Properties.Has(k))
	{
		return Properties.Get(k).Value.getStr();
	}
	if (GetPrototype())
	{
		return GetPrototype()->GetPropertyStr(n);
	}
	return nullptr;
}

C4ValueArray * C4PropList::GetPropertyArray(C4PropertyName n) const
{
	C4String * k = &Strings.P[n];
	if (Properties.Has(k))
	{
		return Properties.Get(k).Value.getArray();
	}
	if (GetPrototype())
	{
		return GetPrototype()->GetPropertyArray(n);
	}
	return nullptr;
}

C4AulFunc * C4PropList::GetFunc(C4String * k) const
{
	assert(k);
	if (Properties.Has(k))
	{
		return Properties.Get(k).Value.getFunction();
	}
	if (GetPrototype())
	{
		return GetPrototype()->GetFunc(k);
	}
	return nullptr;
}

C4AulFunc * C4PropList::GetFunc(const char * s) const
{
	assert(s);
	if (s[0] == '~') ++s;
	C4String * k = Strings.FindString(s);
	// this string is entirely unused
	if (!k)
		return nullptr;
	return GetFunc(k);
}

C4Value C4PropList::Call(C4String * k, C4AulParSet *Pars, bool fPassErrors)
{
	if (!Status) return C4Value();
	C4AulFunc *pFn = GetFunc(k);
	if (!pFn) return C4Value();
	return pFn->Exec(this, Pars, fPassErrors);
}

C4Value C4PropList::Call(const char * s, C4AulParSet *Pars, bool fPassErrors)
{
	if (!Status) return C4Value();
	assert(s && s[0]);
	C4AulFunc *pFn = GetFunc(s);
	if (!pFn)
	{
		if (s[0] != '~')
		{
			C4AulExecError err(FormatString("Undefined function: %s", s).getData());
			if (fPassErrors)
				throw err;
			::ScriptEngine.GetErrorHandler()->OnError(err.what());
		}
		return C4Value();
	}
	return pFn->Exec(this, Pars, fPassErrors);
}

C4PropertyName C4PropList::GetPropertyP(C4PropertyName n) const
{
	C4String * k = &Strings.P[n];
	if (Properties.Has(k))
	{
		C4String * v = Properties.Get(k).Value.getStr();
		if (v >= &Strings.P[0] && v < &Strings.P[P_LAST])
			return C4PropertyName(v - &Strings.P[0]);
		return P_LAST;
	}
	if (GetPrototype())
	{
		return GetPrototype()->GetPropertyP(n);
	}
	return P_LAST;
}

int32_t C4PropList::GetPropertyBool(C4PropertyName n, bool default_val) const
{
	C4String * k = &Strings.P[n];
	if (Properties.Has(k))
	{
		return Properties.Get(k).Value.getBool();
	}
	if (GetPrototype())
	{
		return GetPrototype()->GetPropertyBool(n, default_val);
	}
	return default_val;
}

int32_t C4PropList::GetPropertyInt(C4PropertyName n, int32_t default_val) const
{
	C4String * k = &Strings.P[n];
	if (Properties.Has(k))
	{
		return Properties.Get(k).Value.getInt();
	}
	if (GetPrototype())
	{
		return GetPrototype()->GetPropertyInt(n, default_val);
	}
	return default_val;
}

C4PropList *C4PropList::GetPropertyPropList(C4PropertyName n) const
{
	C4String * k = &Strings.P[n];
	if (Properties.Has(k))
	{
		return Properties.Get(k).Value.getPropList();
	}
	if (GetPrototype())
	{
		return GetPrototype()->GetPropertyPropList(n);
	}
	return nullptr;
}

C4ValueArray * C4PropList::GetProperties() const
{
	C4ValueArray * a;
	int i = 0;
	const bool hasInheritedProperties = GetPrototype() != nullptr;
	if (hasInheritedProperties)
	{
		a = GetPrototype()->GetProperties();
		i = a->GetSize();
		a->SetSize(i + Properties.GetSize());
	}
	else
	{
		a = new C4ValueArray(Properties.GetSize());
		i = 0;
	}
	const C4Property * p = Properties.First();
	while (p)
	{
		C4String *newPropertyName = p->Key;
		assert(newPropertyName != nullptr && "Proplist key is nullpointer");
		// Do we need to check for duplicate property names?
		bool skipProperty = false;
		if (hasInheritedProperties)
		{
			for (size_t j = 0; j < i; ++j)
			{
				if ((*a)[j].getStr() != newPropertyName) continue;
				skipProperty = true;
				break;
			}
		}
		if (!skipProperty)
		{
			(*a)[i++] = C4VString(newPropertyName);
			assert(((*a)[i - 1].GetType() == C4V_String) && "Proplist key is non-string");
		}
		p = Properties.Next(p);
	}
	// We might have added less properties than initially intended.
	if (hasInheritedProperties)
		a->SetSize(i);
	return a;
}

C4String * C4PropList::EnumerateOwnFuncs(C4String * prev) const
{
	const C4Property * p = prev ? Properties.Next(&Properties.Get(prev)) : Properties.First();
	while (p)
	{
		if (p->Value.getFunction())
			return p->Key;
		p = Properties.Next(p);
	}
	return nullptr;
}

void C4PropList::SetPropertyByS(C4String * k, const C4Value & to)
{
	assert(!constant);
	if (k == &Strings.P[P_Prototype])
	{
		C4PropList * newpt = to.getPropList();
		for(C4PropList * it = newpt; it; it = it->GetPrototype())
			if(it == this)
				throw C4AulExecError("Trying to create cyclic prototype structure");
		prototype.SetPropList(newpt);
	}
	else if (Properties.Has(k))
	{
		Properties.Get(k).Value = to;
	}
	else
	{
		Properties.Add(C4Property(k, to));
	}
}

void C4PropList::ResetProperty(C4String * k)
{
	if (k == &Strings.P[P_Prototype])
		prototype.Set0();
	else
		Properties.Remove(k);
}

void C4PropList::Iterator::Init()
{
	iter = properties->begin();
}

void C4PropList::Iterator::Reserve(size_t additionalAmount)
{
	properties->reserve(properties->size() + additionalAmount);
}

void C4PropList::Iterator::AddProperty(const C4Property * prop)
{
	std::vector<const C4Property*>::size_type i = 0, len = properties->size();
	for(;i < len; ++i)
	{
		const C4Property *oldProperty = (*properties)[i];
		if (oldProperty->Key == prop->Key)
		{
			(*properties)[i] = prop;
			return;
		}
	}
	// not already in vector?
	properties->push_back(prop);
}

C4PropList::Iterator C4PropList::begin()
{
	C4PropList::Iterator iter;

	if (GetPrototype())
	{
		iter = GetPrototype()->begin();
	}
	else
	{
		iter.properties = std::make_shared<std::vector<const C4Property*> >();
	}
	iter.Reserve(Properties.GetSize());

	const C4Property * p = Properties.First();
	while (p)
	{
		iter.AddProperty(p);
		p = Properties.Next(p);
	}

	iter.Init();
	return iter;
}


template<> template<>
unsigned int C4Set<C4PropListNumbered *>::Hash<int>(int const & e)
{
	unsigned int hash = 4, tmp;
	hash += e >> 16;
	tmp   = ((e & 0xffff) << 11) ^ hash;
	hash  = (hash << 16) ^ tmp;
	hash += hash >> 11;
	hash ^= hash << 3;
	hash += hash >> 5;
	hash ^= hash << 4;
	hash += hash >> 17;
	hash ^= hash << 25;
	hash += hash >> 6;
	return hash;
}

template<> template<>
unsigned int C4Set<C4PropListNumbered *>::Hash<C4PropList *>(C4PropList * const & e)
{
	return Hash(e->GetPropListNumbered()->Number);
}

template<> template<>
unsigned int C4Set<C4PropListNumbered *>::Hash<C4PropListNumbered *>(C4PropListNumbered * const & e)
{
	return Hash(e->Number);
}

template<> template<>
bool C4Set<C4PropListNumbered *>::Equals<int>(C4PropListNumbered * const & a, int const & b)
{
	return a->Number == b;
}

template<> template<>
bool C4Set<C4PropListNumbered *>::Equals<C4PropList *>(C4PropListNumbered * const & a, C4PropList * const & b)
{
	return a == b;
}

template<> template<>
unsigned int C4Set<C4PropList *>::Hash<C4PropList *>(C4PropList * const & e)
{
	return C4Set<C4PropListNumbered *>::Hash(static_cast<int>(reinterpret_cast<intptr_t>(e)));
}

template<> template<>
unsigned int C4Set<C4PropListScript *>::Hash<C4PropListScript *>(C4PropListScript * const & e)
{
	// since script prop lists are only put in the set for reference keeping, just hash by pointer
	// but use only some of the more significant bits because 
	uintptr_t hash = reinterpret_cast<uintptr_t>(e);
	return (unsigned int)(hash / 63);
}
