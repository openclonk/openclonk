#include <C4Include.h>
#include <C4PropList.h>
#include <C4Game.h>

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
	}

void C4PropList::AssignRemoval()
	{
	while(FirstRef) FirstRef->Set(0);
	Game.ClearPointers(this);
	}

C4PropList::C4PropList():
	FirstRef(NULL), prototype(0)
	{
	}

C4PropList::~C4PropList()
	{
	assert(!FirstRef);
	while (FirstRef) FirstRef->Set(0);
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
bool C4PropList::GetProperty(C4String * k, C4Value & to)
	{
	if (Properties.Has(k))
		{
		to.SetRef(&Properties.Get(k).Value);
		return true;
		}
	if (prototype)
		{
		return prototype->GetProperty(k, to);
		}
	return false;
	}

void C4PropList::SetProperty(C4String * k, C4Value & to)
	{
	if (Properties.Has(k))
		{
		Properties.Get(k).Value = to;
		}
	else
		{
		C4Property p = { k, to };
		Properties.Add(p);
		}
	if (k == Strings.P[P_PROTOTYPE])
		prototype = to.getPropList();
	}
