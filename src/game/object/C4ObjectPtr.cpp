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

#include <C4Include.h>
#include <C4ObjectPtr.h>
#include <C4Object.h>
#include <C4GameObjects.h>

#include <limits>

namespace
{
	C4ObjectPtr NullEnumerated()
	{
		C4ObjectPtr ptr(NULL);
		ptr.EnumeratePointers();
		return ptr;
	}
}

const C4ObjectPtr C4ObjectPtr::Null(NullEnumerated());

void C4ObjectPtr::CompileFunc(StdCompiler* pComp)
{
	// Pointer needs to be enumerated when decompiling
	assert(pComp->isCompiler() || !fDenumerated);

	assert(data.nptr < std::numeric_limits<int32_t>::max());
	int32_t nptr = static_cast<int32_t>(data.nptr);
	pComp->Value(nptr); // TODO: Use mkIntPackAdapt?
	data.nptr = nptr;

#ifndef NDEBUG
	// After having read a value the pointer is enumerated
	if(pComp->isCompiler()) fDenumerated = false;
#endif
}

void C4ObjectPtr::EnumeratePointers()
{
	assert(fDenumerated);

	data.nptr = ::Objects.ObjectNumber(data.ptr);

#ifndef NDEBUG
	fDenumerated = false;
#endif
}

void C4ObjectPtr::DenumeratePointers()
{
	assert(!fDenumerated);

	assert(data.nptr < std::numeric_limits<int32_t>::max());
	data.ptr = ::Objects.ObjectPointer(static_cast<int32_t>(data.nptr));

#ifndef NDEBUG
	fDenumerated = true;
#endif
}

