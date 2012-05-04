/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2010  Nicolas Hake
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

namespace
{
#ifdef _MSC_VER
#	pragma warning(push)
#	pragma warning(disable:4305)  // 'identifier' : truncation from 'type1' to 'type2'
#endif
#ifdef _M_X64
#	define USE_SSE2
#endif
#include "simdmath/sse_mathfun.h"
#ifdef _MSC_VER
#	pragma warning(pop)
#endif
	const __m128 deg2rad = _mm_set_ps1(0.017453292f);
}

C4Real_SSE_Float Sin(const C4Real_SSE_Float &real)
{
	return C4RealImpl_SSE(sin_ps(_mm_mul_ps(real.value.value, deg2rad)));
}
C4Real_SSE_Float Cos(const C4Real_SSE_Float &real)
{
	return C4RealImpl_SSE(cos_ps(_mm_mul_ps(real.value.value, deg2rad)));
}
C4Real_SSE_Float Pow(const C4Real_SSE_Float &x, const C4Real_SSE_Float &y)
{
	C4RealImpl_SSE val;
	val.value = log_ps(x.value.value);
	val *= y.value;
	val.value = exp_ps(val.value);
	return val;
}
