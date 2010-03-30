/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2010 Armin Burgmeier
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

/* A convenient way to (de)serialize object pointers */

#ifndef INC_C4ObjectPtr
#define INC_C4ObjectPtr

// An enumerable C4Object pointer
class C4ObjectPtr
{
public:
	// For use with mkNamingAdapt because simply 0 becomes 0 (the int)
	// which makes correct template deduction fail. This constant is an
	// enumerated(!) null pointer.
	static const C4ObjectPtr Null;

	C4ObjectPtr() {} // uninitialized

	C4ObjectPtr(C4Object* pObj)
#ifndef NDEBUG
			: fDenumerated(true)
#endif
	{
		data.ptr = pObj;
	}

#if 0
	C4ObjectPtr(const C4ObjectPtr& other)
#ifndef NDEBUG
			: fDenumerated(other.fDenumerated)
#endif
	{
		data = other.data;
	}
#endif

	void CompileFunc(StdCompiler* pComp);
	void EnumeratePointers();
	void DenumeratePointers();

	bool operator!() const { assert(fDenumerated); return !data.ptr; }
	bool operator==(C4Object* other) const { assert(fDenumerated); return data.ptr == other; }
	bool operator==(const C4ObjectPtr& other) const { assert(fDenumerated == other.fDenumerated); return data.ptr == other.data.ptr; }
	bool operator!=(C4Object* other) const { assert(fDenumerated); return data.ptr != other; }
	bool operator!=(const C4ObjectPtr& other) const { assert(fDenumerated == other.fDenumerated); return data.ptr != other.data.ptr; }

	C4ObjectPtr operator=(C4Object* object)
	{
#ifndef NDEBUG
		fDenumerated = true;
#endif
		data.ptr = object;
		return *this;
	}

	C4Object& operator*() const { assert(fDenumerated); return *data.ptr; }
	C4Object* operator->() const { assert(fDenumerated); return data.ptr; }
	operator C4Object*() const { assert(fDenumerated); return data.ptr; }

protected:
#ifndef NDEBUG
	bool fDenumerated;
#endif

	union
	{
		C4Object* ptr;
		intptr_t nptr; // needs to have same size as ptr for operator== to work when enumerated
	} data;
};

#endif
