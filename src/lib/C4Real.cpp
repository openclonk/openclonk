/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2002  Sven Eberhardt
 * Copyright (c) 2005  Peter Wortmann
 * Copyright (c) 2010  Benjamin Herr
 * Copyright (c) 2010  Nicolas Hake
 * Copyright (c) 2010  Günther Brammer
 * Copyright (c) 2012  Julius Michaelis
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

const C4Real::StorageType C4Real::PI = {0x40490fdb};
const C4Real::StorageType C4Real::E = {0x402df854};

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
	if(_mm_comieq_ss(x.value, C4Real::float_hexconst(0x0))) return 0;
	bool sign = _mm_comilt_ss(x.value, C4Real::float_hexconst(0x0));
	if(sign)
	{
		__m128 sig = _mm_or_ps(_mm_and_ps(y.value, C4Real::float_hexconst(0x80000000)), C4Real::float_hexconst(0x4b000000)); // generate (sign?+:-)2^23
		__m128 trunc = _mm_sub_ps(_mm_add_ps(y.value, sig), sig); // truncate bits after comma
		if(_mm_comineq_ss(y.value, trunc)) 
		{
			C4Real nan;
			nan.value = C4Real::float_hexconst(0x7f800001); // signaling nan
			return nan;
		}
	}
	__m128 abs = _mm_and_ps(x.value, C4Real::float_hexconst(0x7fffffff));
	C4Real val;
	val.value = log_ps(abs);
	val *= y;
	val.value = exp_ps(val.value);
	if(sign)
	{
		// smart little hack: concatenate like in operator%=, but one bit more
		__m128 sig = _mm_or_ps(_mm_and_ps(y.value, C4Real::float_hexconst(0x80000000)), C4Real::float_hexconst(0x4b800000));
		__m128 trunc = _mm_sub_ps(_mm_add_ps(y.value, sig), sig);
		if(_mm_comineq_ss(y.value, trunc)) // negate result if power is not even
			val.value = _mm_or_ps(x.value, C4Real::float_hexconst(0x80000000));
	}
	return val;
}

C4Real Sqrt(const C4Real &v)
{
	C4Real nrv;
	nrv.value = _mm_sqrt_ps(v.value);
	return nrv;
}

C4Real Log(const C4Real &v)
{
	C4Real ret;
	ret.value = log_ps(v.value);
	return ret;
}

C4Real Atan2(const C4Real & y, const C4Real & x)
{
	__m128 t0, t1, t3, t4;
	t1 = _mm_and_ps(x.value, C4Real::float_hexconst(0x7fffffff));
    t3 = _mm_and_ps(y.value, C4Real::float_hexconst(0x7fffffff));
    bool axltay = _mm_comilt_ss(t1, t3);
    if(axltay)
    	t3 = _mm_mul_ps(t1, _mm_rcp_ss(t3));
    else
    	t3 = _mm_mul_ps(t3, _mm_rcp_ss(t1));
	t4 = _mm_mul_ps(t3, t3);
    t0 =                                C4Real::float_hexconst(0xbc5cdd30);
    t0 = _mm_add_ps(_mm_mul_ps(t0, t4), C4Real::float_hexconst(0x3d6b6d55));
    t0 = _mm_add_ps(_mm_mul_ps(t0, t4), C4Real::float_hexconst(0xbdf84c31));
    t0 = _mm_add_ps(_mm_mul_ps(t0, t4), C4Real::float_hexconst(0x3e4854c9));
    t0 = _mm_add_ps(_mm_mul_ps(t0, t4), C4Real::float_hexconst(0xbeaa7e45));
    t0 = _mm_add_ps(_mm_mul_ps(t0, t4), C4Real::float_hexconst(0x3f7fffb7));
    t3 = _mm_mul_ps(t0, t3);
    t3 = (axltay) ? _mm_sub_ps(C4Real::float_hexconst(0x3fc90fdb), t3) : t3;
    t3 = _mm_comilt_ss(x.value, C4Real::float_hexconst(0)) ? _mm_sub_ps(C4Real::float_hexconst(0x40490fdb), t3) : t3;
    t3 = _mm_comilt_ss(y.value, C4Real::float_hexconst(0)) ? _mm_xor_ps(t3, C4Real::float_hexconst(0x80000000)) : t3;
    C4Real ret;
    ret.value = _mm_mul_ps(t3, _mm_div_ss(C4Real::float_hexconst(0x43340000), C4Real(C4Real::PI).value));
    return ret;
}
