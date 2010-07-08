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

#ifndef INC_RealImpl_SSE
#define INC_RealImpl_SSE

#ifndef INC_C4Real
#error C4RealImpl_SSE.h must not be included by itself; include C4Real.h instead
#endif

#include <cassert>
#include <xmmintrin.h>

class C4RealImpl_SSE
{
	friend C4Real_SSE_Float Sin(const C4Real_SSE_Float &);
	friend C4Real_SSE_Float Cos(const C4Real_SSE_Float &);
	__m128 value;

	static const __m128 iee754_sign_mask; // -0.0
	static const __m128 cephes_FOPI; // 4/pi
	static const __m128 cephes_deg2rad; // pi/180
	static const __m128 cephes_appx_coeffs[3]; // approximation coefficients
	static const __m128 cephes_scaling_factors[3]; // factors for quick scaling to -pi/4..pi/4
	C4RealImpl_SSE SinCos(bool cosine) const; // approximation of sine and cosine

public:
	inline C4RealImpl_SSE()
		: value(_mm_setzero_ps())
	{}
	inline C4RealImpl_SSE(const C4RealImpl_SSE &rhs)
		: value(rhs.value)
	{}
	inline C4RealImpl_SSE(int32_t iVal)
		: value(_mm_cvtsi32_ss(value, iVal))
	{}
	inline C4RealImpl_SSE(float fVal)
		: value(_mm_set_ss(fVal))
	{}

	operator int () const
	{
		return _mm_cvtss_si32(value);
	}
	operator float () const
	{
		float f;
		_mm_store_ss(&f, value);
		return f;
	}

	// To store C4RealImpl_SSE into unions, we're using an anonymous struct
	// to distinguish types, and an int32_t inside that struct to avoid passing
	// parameters via the x87 stack.
	struct StorageType { int32_t v; };
	friend bool operator==(StorageType lhs, StorageType rhs) { return lhs.v == rhs.v; }
	inline C4RealImpl_SSE(StorageType rhs)
		: value(_mm_load_ss(reinterpret_cast<float*>(&rhs.v)))
	{}
	operator StorageType() const
	{
		StorageType nrv;
		_mm_store_ss(reinterpret_cast<float*>(&nrv.v), value);
		return nrv;
	}

	// Arithmetics
	// We're using _ps intrinsics for everything except division because they
	// have the same latency as their _ss counterparts, but their representa-
	// tion is one byte shorter (0F xx instead of F3 0F xx).
	// DIVPS is about half as fast as DIVSS, so we use the scalar instruction
	// here.
	C4RealImpl_SSE &operator += (const C4RealImpl_SSE &rhs) { value = _mm_add_ps(value, rhs.value); return *this; }
	C4RealImpl_SSE &operator -= (const C4RealImpl_SSE &rhs) { value = _mm_sub_ps(value, rhs.value); return *this; }
	C4RealImpl_SSE &operator *= (const C4RealImpl_SSE &rhs) { value = _mm_mul_ps(value, rhs.value); return *this; }
	C4RealImpl_SSE &operator /= (const C4RealImpl_SSE &rhs) { value = _mm_div_ss(value, rhs.value); return *this; }

	// Negation
	C4RealImpl_SSE operator - () const
	{
		C4RealImpl_SSE nrv;
		nrv -= *this;
		return nrv;
	}

	// Comparison
	// COMISS is faster on newer CPUs than CMPSS, also we get a nice return
	// value from the intrinsic instead of having to store parts of the XMM
	// register to a variable to read.
	bool operator < (const C4RealImpl_SSE &rhs) const { return _mm_comilt_ss(value, rhs.value) != 0; }
	bool operator <= (const C4RealImpl_SSE &rhs) const { return _mm_comile_ss(value, rhs.value) != 0; }
	bool operator == (const C4RealImpl_SSE &rhs) const { return _mm_comieq_ss(value, rhs.value) != 0; }
	bool operator >= (const C4RealImpl_SSE &rhs) const { return _mm_comige_ss(value, rhs.value) != 0; }
	bool operator > (const C4RealImpl_SSE &rhs) const { return _mm_comigt_ss(value, rhs.value) != 0; }
	bool operator != (const C4RealImpl_SSE &rhs) const { return _mm_comineq_ss(value, rhs.value) != 0; }

	operator bool () const { return _mm_comineq_ss(value, _mm_setzero_ps()) != 0; }
	bool operator ! () const { return _mm_comieq_ss(value, _mm_setzero_ps()) != 0; }
};

inline C4Real_SSE_Float Sin(const C4Real_SSE_Float &real)
{
	return C4Real_SSE_Float(static_cast<float>(real.value.SinCos(false)));
}
inline C4Real_SSE_Float Cos(const C4Real_SSE_Float &real)
{
	return C4Real_SSE_Float(static_cast<float>(real.value.SinCos(true)));
}

#endif
