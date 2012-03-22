/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2002  Sven Eberhardt
 * Copyright (c) 2005  Peter Wortmann
 * Copyright (c) 2010  Benjamin Herr
 * Copyright (c) 2010  Nicolas Hake
 * Copyright (c) 2010  Günther Brammer
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de
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

#include "C4Include.h"
#include "C4Real.h"

#include "StdCompiler.h"

void CompileFunc(C4Real &rValue, StdCompiler *pComp)
{
	// Read value (as float)
	float value;
	if (pComp->isDecompiler())
		value = static_cast<float>(rValue);
	pComp->Value(value);
	if (pComp->isCompiler())
		rValue = value;
}

#define OC_MACHINE_UNKNOWN 0x0
#define OC_MACHINE_X86     0x1
#define OC_MACHINE_X64     0x2
#if defined(_M_X64) || defined(__amd64)
#	define OC_MACHINE OC_MACHINE_X64
#elif defined(_M_IX86) || defined(__i386__)
#	define OC_MACHINE OC_MACHINE_X86
#else
#	define OC_MACHINE OC_MACHINE_UNKNOWN
#endif

namespace
{
#ifdef _MSC_VER
#	pragma warning(push)
#	pragma warning(disable:4305)  // 'identifier' : truncation from 'type1' to 'type2'
#endif
#if OC_MACHINE == OC_MACHINE_X64
#	define USE_SSE2
#endif
#include "simdmath/sse_mathfun.h"
#ifdef _MSC_VER
#	pragma warning(pop)
#endif
	const __m128 deg2rad = _mm_set_ps1(0.017453292f);
}

C4Real Sin(const C4Real &real)
{
	C4Real nrv;
	nrv.value = sin_ps(_mm_mul_ps(real.value, deg2rad));
	return nrv;
}
C4Real Cos(const C4Real &real)
{
	C4Real nrv;
	nrv.value = cos_ps(_mm_mul_ps(real.value, deg2rad));
	return nrv;
}
C4Real Pow(const C4Real &x, const C4Real &y)
{
	C4Real val;
	val.value = log_ps(x.value);
	val *= y;
	val.value = exp_ps(val.value);
	return val;
}
