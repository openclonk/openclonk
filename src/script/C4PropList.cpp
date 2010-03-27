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

#include <C4Include.h>
#include <C4PropList.h>
#include <C4GameObjects.h>
#include <C4Game.h>
#include <C4Object.h>

void C4PropList::AddRef(C4Value *pRef)
	{
	pRef->NextRef = FirstRef;
	FirstRef = pRef;
	}

void C4PropList::DelRef(const C4Value * pRef, C4Value * pNextRef)
	{
	// References to objects never have HasBaseArray set
	if(pRef == FirstRef)
		FirstRef = pNextRef;
	else
		{
		C4Value *pVal = FirstRef;
		while(pVal->NextRef && pVal->NextRef != pRef)
			pVal = pVal->NextRef;
		assert(pVal->NextRef);
		pVal->NextRef = pNextRef;
		}
	if (FirstRef) return;
	if (iElementReferences) return;
	// These classes have their own memory management
	if (dynamic_cast<C4Object *>(this)) return;
	if (dynamic_cast<C4Def *>(this)) return;
	delete this;
	}

void C4PropList::DecElementRef()
	{
	assert(iElementReferences > 0);
	--iElementReferences;
	if (iElementReferences) return;
	if (FirstRef) return;
	// These classes have their own memory management
	if (dynamic_cast<C4Object *>(this)) return;
	if (dynamic_cast<C4Def *>(this)) return;
	delete this;
	}

void C4PropList::AssignRemoval()
	{
	while(FirstRef) FirstRef->Set0();
	Game.ClearPointers(this);
	}

C4PropList * C4PropList::New(C4PropList * prototype)
	{
	C4PropListNumbered * r = new C4PropListNumbered(prototype);
	r->AcquireNumber();
	return r;
	}

C4PropListNumbered::C4PropListNumbered(C4PropList * prototype): C4PropList(prototype), Number(-1)
	{
	}

void C4PropListNumbered::AcquireNumber()
	{
	// Enumerate object
	do
		Number = ++Game.ObjectEnumerationIndex;
	while (::Objects.PropLists.Get(Number));
	::Objects.PropLists.Add(this);
	}

C4PropListNumbered* C4PropListNumbered::GetPropListNumbered()
	{
	return this;
	}

void C4PropListNumbered::CompileFunc(StdCompiler *pComp)
	{
	pComp->Value(mkNamingAdapt(Number, "Number"));
	C4PropList::CompileFunc(pComp);
	if(pComp->isCompiler())
		::Objects.PropLists.Add(this);
	}

C4PropListNumbered::~C4PropListNumbered()
	{
	if(Number != -1)
		::Objects.PropLists.Remove(this);
	else
		Log("removing numbered proplist without number");
	}

C4PropList::C4PropList(C4PropList * prototype):
	Status(1),
	FirstRef(NULL), iElementReferences(0), prototype(prototype)
	{
	if(prototype)
		SetProperty(Strings.P[P_Prototype], C4VPropList(prototype));
	}

void C4PropList::DenumeratePointers()
	{
	const C4Property * p = Properties.First();
	while (p)
		{
		const_cast<C4Value &>(p->Value).DenumeratePointer();
		p = Properties.Next(p);
		}
	C4Value v;
	GetPropertyVal(Strings.P[P_Prototype], v);
	prototype = v.getPropList();
	}

C4PropList::~C4PropList()
	{
	while (FirstRef)
	{
		// Manually kill references so DelRef doesn't destroy us again
		FirstRef->Data = 0; FirstRef->Type = C4V_Any;
		C4Value *ref = FirstRef;
		FirstRef = FirstRef->NextRef;
		ref->NextRef = NULL;
	}
	assert(!iElementReferences);
	assert(!::Objects.ObjectNumber(this));
	}

void C4PropList::CompileFunc(StdCompiler *pComp)
	{
	pComp->Value(mkNamingAdapt(Properties, "Properties"));
	}

template<typename T>
void C4Set<T>::CompileFunc(StdCompiler *pComp)
	{
	bool fNaming = pComp->hasNaming();
	if (pComp->isCompiler())
		{
		// Compiling: Empty previous
		Clear();
		// Read size (binary only)
		uint32_t iSize;
		if(!pComp->hasNaming()) pComp->Value(iSize);
		// Read new
		do
			{
			// No entries left to read?
			if(!fNaming && !iSize--)
				break;
			// Read entries
			try
				{
				T e;
				pComp->Value(e);
				Add(e);
				}
			catch(StdCompiler::NotFoundException *pEx)
				{
				// No value found: Stop reading loop
				delete pEx;
				break;
				}
			}
		while(pComp->Seperator(StdCompiler::SEP_SEP2));
		}
	else
		{
		// Write size (binary only)
		if(!fNaming)
			{
			int32_t iSize = GetSize();
			pComp->Value(iSize);
			}
		// Write all entries
		const T * p = First();
		while (p)
			{
			pComp->Value(*const_cast<T *>(p));
			p = Next(p);
			if (p) pComp->Seperator(StdCompiler::SEP_SEP2);
			}
		}
	}

void C4Property::CompileFunc(StdCompiler *pComp)
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
	pComp->Seperator(StdCompiler::SEP_SET);
	pComp->Value(Value);
	}


const char * C4PropList::GetName()
	{
	C4String * s = GetPropertyStr(P_Name);
	if (!s) return "";
	return s->GetCStr();
	}

void C4PropList::SetName(const char* NewName)
	{
	if(!NewName)
		ResetProperty(Strings.P[P_Name]);
	else
		{
		C4Value v = C4VString(NewName);
		SetProperty(Strings.P[P_Name], v);
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


C4PropListNumbered * C4PropList::GetPropListNumbered()
	{
	if (prototype) return prototype->GetPropListNumbered();
	return 0;
	}


template<> template<>
unsigned int C4Set<C4Property>::Hash<C4String *>(C4String * e)
	{
	return e->Hash;
	}

template<> template<>
bool C4Set<C4Property>::Equals<C4String *>(C4Property a, C4String * b)
	{
	return a.Key == b;
	}

template<> template<>
unsigned int C4Set<C4Property>::Hash<C4Property>(C4Property p)
	{
	return p.Key->Hash;
	}

bool C4PropList::HasProperty(C4String * k) const
	{
	if (Properties.Has(k))
		{
		return true;
		}
	if (prototype)
		{
		return prototype->HasProperty(k);
		}
	return false;
	}

void C4PropList::GetPropertyRef(C4String * k, C4Value & to)
	{
	// The prototype is special
	if (k == Strings.P[P_Prototype])
		{
		to = C4VPropList(prototype);
		return;
		}
	to.SetPropListRef(this, k);
	}

C4Value * C4PropList::GetRefToProperty(C4String * k)
	{
	// The prototype is special
	if (k == Strings.P[P_Prototype])
		{
		return 0;
		}
	if (Properties.Has(k))
		{
		return &Properties.Get(k).Value;
		}
	if (prototype)
		{
		C4Property p(k, *(prototype->GetRefToProperty(k)));
		return &(Properties.Add(p)->Value);
		}
	C4Property p(k, C4VNull);
	return &(Properties.Add(p)->Value);
	}

const C4Value * C4PropList::GetRefToPropertyConst(C4String * k) const
	{
	// The prototype is special
	if (k == Strings.P[P_Prototype])
		{
		return 0;
		}
	if (Properties.Has(k))
		{
		return &Properties.Get(k).Value;
		}
	if (prototype)
		{
		return prototype->GetRefToPropertyConst(k);
		}
	return 0;
	}

bool C4PropList::GetPropertyVal(C4String * k, C4Value & to)
	{
	if (Properties.Has(k))
		{
		to = Properties.Get(k).Value;
		return true;
		}
	if (prototype)
		{
		return prototype->GetPropertyVal(k, to);
		}
	return false;
	}

C4String * C4PropList::GetPropertyStr(C4PropertyName n)
	{
	C4String * k = Strings.P[n];
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

int32_t C4PropList::GetPropertyInt(C4PropertyName n)
	{
	C4String * k = Strings.P[n];
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

void C4PropList::SetProperty(C4String * k, const C4Value & to)
	{
	assert(Strings.Set.Has(k));
	if (k == Strings.P[P_Prototype] && to.GetType() == C4V_PropList)
		{
		prototype = to.GetData().PropList;
		//return;
		}
	if (Properties.Has(k))
		{
		Properties.Get(k).Value = to;
		}
	else
		{
		C4Property p(k, to);
		Properties.Add(p);
		}
	}

void C4PropList::ResetProperty(C4String * k)
	{
	Properties.Remove(k);
	}



template<> template<>
unsigned int C4Set<C4PropListNumbered *>::Hash<int>(int e)
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
unsigned int C4Set<C4PropListNumbered *>::Hash<C4PropList *>(C4PropList * e)
	{
	return Hash(e->GetPropListNumbered()->Number);
	}

template<> template<>
unsigned int C4Set<C4PropListNumbered *>::Hash<C4PropListNumbered *>(C4PropListNumbered * e)
	{
	return Hash(e->Number);
	}

template<> template<>
bool C4Set<C4PropListNumbered *>::Equals<int>(C4PropListNumbered * a, int b)
	{
	return a->Number == b;
	}

template<> template<>
bool C4Set<C4PropListNumbered *>::Equals<C4PropList *>(C4PropListNumbered * a, C4PropList * b)
	{
	return a == b;
	}
