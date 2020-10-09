/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2010-2016, The OpenClonk Team and contributors
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

/* A convenient way to (de)serialize object pointers */

#include "C4Include.h"
#include "object/C4ObjectPtr.h"

#include "object/C4GameObjects.h"
#include "object/C4Object.h"

#include <limits>

const C4ObjectPtr C4ObjectPtr::Null(nullptr);

void C4ObjectPtr::CompileFunc(StdCompiler* pComp)
{
	// Pointer needs to be denumerated when decompiling
	assert(fDenumerated);

	int32_t nptr = 0;
	if (!pComp->isDeserializer() && data.ptr)
	{
		nptr = data.ptr->Number;
	}
	pComp->Value(nptr);
	if (pComp->isDeserializer())
	{
		data.nptr = nptr;
#ifndef NDEBUG
		// After having read a value the pointer is enumerated
		fDenumerated = false;
#endif
	}
}

void C4ObjectPtr::DenumeratePointers()
{
	assert(!fDenumerated || !data.ptr);

	assert(data.nptr < std::numeric_limits<int32_t>::max());
	data.ptr = ::Objects.ObjectPointer(static_cast<int32_t>(data.nptr));

#ifndef NDEBUG
	fDenumerated = true;
#endif
}

