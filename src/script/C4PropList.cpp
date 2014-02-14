/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2004, Peter Wortmann
 * Copyright (c) 2007, Günther Brammer
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

#include <C4Include.h>
#include <C4PropList.h>
#include <C4GameObjects.h>
#include <C4Game.h>
#include <C4Object.h>
#include <C4Record.h>

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
	EnumerationIndex = Max(EnumerationIndex, iMaxObjectNumber);
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
	for (std::vector<C4PropListNumbered *>::iterator i=ShelvedPropLists.begin(); i!=ShelvedPropLists.end(); ++i)
		(*i)->AcquireNumber();
	ShelvedPropLists.clear();
}

void C4PropListNumbered::ClearShelve()
{
	// cleanup shelve - used in game clear, un unsuccessful section load, etc.
	ShelvedPropLists.clear();
}

C4PropListNumbered::C4PropListNumbered(C4PropList * prototype): C4PropList(prototype), Number(-1)
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
	if (pComp->isCompiler())
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

void C4PropListStatic::RefCompileFunc(StdCompiler *pComp, C4ValueNumbers * numbers) const
{
	assert(!pComp->isCompiler());
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

C4PropList::C4PropList(C4PropList * prototype):
		FirstRef(NULL), prototype(prototype),
		constant(false), Status(1)
{
	if (prototype)
		SetProperty(P_Prototype, C4VPropList(prototype));
#ifdef _DEBUG	
	PropLists.Add(this);
#endif
}

void C4PropList::Denumerate(C4ValueNumbers * numbers)
{
	const C4Property * p = Properties.First();
	while (p)
	{
		const_cast<C4Value &>(p->Value).Denumerate(numbers);
		p = Properties.Next(p);
	}
	C4Value v;
	if(GetProperty(P_Prototype, &v))
		prototype = v.getPropList();
}

C4PropList::~C4PropList()
{
	while (FirstRef)
	{
		// Manually kill references so DelRef doesn't destroy us again
		FirstRef->Data = 0; FirstRef->Type = C4V_Nil;
		C4Value *ref = FirstRef;
		FirstRef = FirstRef->NextRef;
		ref->NextRef = NULL;
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
	//if (GetObject() != b.GetObject()) return false;
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
	pComp->Value(constant);
	pComp->Separator(StdCompiler::SEP_SEP2);
	pComp->Value(mkParAdapt(Properties, numbers));
}

void CompileNewFunc(C4PropList *&pStruct, StdCompiler *pComp, C4ValueNumbers * const & rPar)
{
	std::unique_ptr<C4PropList> temp(C4PropList::New()); // exception-safety
	pComp->Value(mkParAdapt(*temp, rPar));
	pStruct = temp.release();
}

template<typename T>
void C4Set<T>::CompileFunc(StdCompiler *pComp, C4ValueNumbers * numbers)
{
	bool fNaming = pComp->hasNaming();
	if (pComp->isCompiler())
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
	if (!pComp->isCompiler())
		s = Key->GetData();
	pComp->Value(s);
	if (pComp->isCompiler())
	{
		if (Key) Key->DecRef();
		Key = ::Strings.RegString(s);
		Key->IncRef();
	}
	pComp->Separator(StdCompiler::SEP_SET);
	pComp->Value(mkParAdapt(Value, numbers));
}

void C4PropList::AppendDataString(StdStrBuf * out, const char * delim, int depth) const
{
	StdStrBuf & DataString = *out;
	if (depth <= 0 && Properties.GetSize())
	{
		DataString.Append("...");
		return;
	}
	std::list<const C4Property *> sorted_props = Properties.GetSortedListOfElementPointers();
	for (std::list<const C4Property *>::const_iterator p = sorted_props.begin(); p != sorted_props.end(); ++p)
	{
		if (p != sorted_props.begin()) DataString.Append(delim);
		DataString.Append((*p)->Key->GetData());
		DataString.Append(" = ");
		DataString.Append((*p)->Value.GetDataString(depth - 1));
	}
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
	if (prototype) return prototype->GetObject();
	return 0;
}

C4Def * C4PropList::GetDef()
{
	if (prototype) return prototype->GetDef();
	return 0;
}

C4Def const * C4PropList::GetDef() const
{
	if (prototype) return prototype->GetDef();
	return 0;
}

class C4MapScriptLayer * C4PropList::GetMapScriptLayer()
{
	if (prototype) return prototype->GetMapScriptLayer();
	return NULL;
}

class C4MapScriptMap * C4PropList::GetMapScriptMap()
{
	if (prototype) return prototype->GetMapScriptMap();
	return NULL;
}

C4PropListNumbered * C4PropList::GetPropListNumbered()
{
	if (prototype) return prototype->GetPropListNumbered();
	return 0;
}

C4Effect * C4PropList::GetEffect()
{
	if (prototype) return prototype->GetEffect();
	return 0;
}


template<> template<>
unsigned int C4Set<C4Property>::Hash<C4String *>(C4String * const & e)
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
	//return e->Hash;
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

bool C4PropList::GetPropertyByS(C4String * k, C4Value *pResult) const
{
	if (Properties.Has(k))
	{
		*pResult = Properties.Get(k).Value;
		return true;
	}
	else if(prototype)
		return prototype->GetPropertyByS(k, pResult);
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
	if (prototype)
	{
		return prototype->GetPropertyStr(n);
	}
	return 0;
}

C4AulFunc * C4PropList::GetFunc(C4String * k) const
{
	assert(k);
	if (Properties.Has(k))
	{
		return Properties.Get(k).Value.getFunction();
	}
	if (prototype)
	{
		return prototype->GetFunc(k);
	}
	return 0;
}

C4AulFunc * C4PropList::GetFunc(const char * s) const
{
	assert(s);
	if (s[0] == '~') ++s;
	C4String * k = Strings.FindString(s);
	// this string is entirely unused
	if (!k)
		return 0;
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
	if (!pFn) return C4Value();
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
	if (prototype)
	{
		return prototype->GetPropertyP(n);
	}
	return P_LAST;
}

int32_t C4PropList::GetPropertyInt(C4PropertyName n) const
{
	C4String * k = &Strings.P[n];
	if (Properties.Has(k))
	{
		return Properties.Get(k).Value.getInt();
	}
	if (prototype)
	{
		return prototype->GetPropertyInt(n);
	}
	return 0;
}

C4PropList *C4PropList::GetPropertyPropList(C4PropertyName n) const
{
	C4String * k = &Strings.P[n];
	if (Properties.Has(k))
	{
		return Properties.Get(k).Value.getPropList();
	}
	if (prototype)
	{
		return prototype->GetPropertyPropList(n);
	}
	return NULL;
}

C4ValueArray * C4PropList::GetProperties() const
{
	C4ValueArray * a;
	int i;
	if (prototype)
	{
		a = prototype->GetProperties();
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
		(*a)[i++] = C4VString(p->Key);
		p = Properties.Next(p);
	}
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
	return 0;
}

void C4PropList::SetPropertyByS(C4String * k, const C4Value & to)
{
	assert(!constant);
	/*assert(Strings.Set.Has(k));*/
	if (k == &Strings.P[P_Prototype] && to.GetType() == C4V_PropList)
	{
		C4PropList * newpt = to.GetData().PropList;
		for(C4PropList * it = newpt; it; it = it->prototype)
			if(it == this)
				throw new C4AulExecError("Trying to create cyclic prototype structure");
		prototype = newpt;
		//return;
	}
	if (Properties.Has(k))
	{
		Properties.Get(k).Value = to;
	}
	else
	{
		//C4Property p(k, to);
		//Properties.Add(p);
#ifdef DEBUGREC_SCRIPT
		if (Config.General.DebugRec)
		{
			// deactivate this debugrec for now, because property orders seem to be out of sync
			// after loading at the moment. might need to invastigate the cause later...
			//if (k->GetCStr()) AddDbgRec(RCT_SetProperty, k->GetCStr(), strlen(k->GetCStr())+1);
		}
#endif
		Properties.Add(C4Property(k, to));
	}
}

void C4PropList::ResetProperty(C4String * k)
{
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

	if (prototype)
	{
		iter = prototype->begin();
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
