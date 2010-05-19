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

/* Fixed point math extracted from ALLEGRO by Shawn Hargreaves */

#ifndef INC_RealImpl_FPU
#define INC_RealImpl_FPU

#ifndef INC_C4Real
#error C4RealImpl_FPU.h must not be included by itself; include C4Real.h instead
#endif

#ifdef _MSC_VER
#	define _USE_MATH_DEFINES
#endif
#include <cmath>

inline C4Real_FPU_Float Sin(const C4Real_FPU_Float &real)
{
	return C4Real_FPU_Float(std::sin(real.value * static_cast<float>(M_PI) / 180.0f));
}
inline C4Real_FPU_Float Cos(const C4Real_FPU_Float &real)
{
	return C4Real_FPU_Float(std::cos(real.value * static_cast<float>(M_PI) / 180.0f));
}

// Overload to avoid conversion warning
template<>
inline C4Real_FPU_Float::operator bool () const
{
	return value != 0.0f;
}

// C4Real_Fixed rounds up. Why?
#ifdef _M_X64
#include <xmmintrin.h>
#endif
template<>
inline C4Real_FPU_Float::operator int () const
{
	float y = value;
#if defined _M_X64
	*reinterpret_cast<int*>(&y) |= 1;
	return _mm_cvtss_si32(_mm_load_ss(&y));
#elif defined _MSC_VER
	int e;
	_asm
	{
		or y,1;
		fld y;
		fistp e;
	}
	return e;
#elif defined __GNUC__
	int e;
	asm ("or $1, %0" : "+rom" (y));
	asm ("fistp%z0 %0" : "=om" (e) : "t" (y) : "st");
	return e;
#else
#error Unknown processor; implement rounding here
#endif
}

#endif
