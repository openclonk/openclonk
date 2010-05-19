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

// Constants required for sine approximation
const __m128 C4RealImpl_SSE::cephes_deg2rad = _mm_set_ps1(0.017453292f); // pi/180
const __m128 C4RealImpl_SSE::cephes_FOPI = _mm_set_ps1(1.27323954473516f); // 4/pi
const __m128 C4RealImpl_SSE::cephes_scaling_factors[3] = {
	_mm_set_ps1(0.78515625f), _mm_set_ps1(2.4187564849853515625e-4f), _mm_set_ps1(3.77489497744594108e-8f)
};
const __m128 C4RealImpl_SSE::cephes_appx_coeffs[3] = {
	_mm_set_ps(-1.9515295891e-4f, 2.443315711809948e-5f, 2.443315711809948e-5f, -1.9515295891e-4f),
	_mm_set_ps(8.3321608736e-3f, -1.388731625493765e-3f, -1.388731625493765e-3f, 8.3321608736e-3f),
	_mm_set_ps(-1.6666654611e-1f, 4.166664568298827e-2f, 4.166664568298827e-2f, -1.6666654611e-1f)
};
const __m128 C4RealImpl_SSE::iee754_sign_mask = _mm_set_ps1(-0.0f);

// Branchless approximation of sine and cosine values
C4RealImpl_SSE C4RealImpl_SSE::SinCos(bool cosine) const
{
	// NOTE: Consider storing radians directly instead of degrees to avoid
	// precision loss due to conversion
	__m128 radian = _mm_mul_ps(value, cephes_deg2rad);
#ifndef NDEBUG
	float float_radians; _mm_store_ss(&float_radians, radian);
#endif
	// put rad into all parts of the xmm register
	radian = _mm_shuffle_ps(radian, radian, 0);

	// cephes library sine/cosine implementation
	// This calculates a Taylor approximation of rank 7 with slightly
	// modified coefficients to achieve better precision on the reduced
	// input range -pi/4..pi/4.

	union m128extract {
		float f[4];
		uint32_t i[4];
		__m128 v;
	};

	// Store sign and take absolute value of input
	__m128 sign = _mm_and_ps(radian, iee754_sign_mask);
	radian = _mm_xor_ps(radian, sign);
	m128extract sign_bits;
	sign_bits.v = sign;

	// Select octant of the unit circle
	__m128 scaling = _mm_mul_ps(radian, C4RealImpl_SSE::cephes_FOPI);
	int octant = _mm_cvttss_si32(scaling);
	octant = (octant + 1) & ~1;
	scaling = _mm_cvtsi32_ss(scaling, octant);
	scaling = _mm_shuffle_ps(scaling, scaling, 0);
	uint32_t flip_sign_sine = ((octant & 4) << 29);
	octant &= 3;
	uint32_t flip_sign_cosine = flip_sign_sine ^ ((octant & 2) << 30);
	flip_sign_sine ^= sign_bits.i[0];

	// map input to +-pi/4
	// note that this get more and more imprecise for abs(radian) > 8192
	radian = _mm_sub_ps(radian, _mm_mul_ps(scaling, cephes_scaling_factors[0]));
	radian = _mm_sub_ps(radian, _mm_mul_ps(scaling, cephes_scaling_factors[1]));
	radian = _mm_sub_ps(radian, _mm_mul_ps(scaling, cephes_scaling_factors[2]));

	// run approximation, calculating four octants at once; correct result
	// will be selected later
	__m128 radiansq = _mm_mul_ps(radian, radian);
	__m128 result = cephes_appx_coeffs[0];
	result = _mm_mul_ps(result, radiansq);
	result = _mm_add_ps(result, cephes_appx_coeffs[1]);
	result = _mm_mul_ps(result, radiansq);
	result = _mm_add_ps(result, cephes_appx_coeffs[2]);
	result = _mm_mul_ps(result, radiansq);
	
	radiansq = _mm_shuffle_ps(radiansq, radian, _MM_SHUFFLE(0,0,0,0));
	radiansq = _mm_shuffle_ps(radiansq, radiansq, _MM_SHUFFLE(2,0,0,2));
	result = _mm_mul_ps(result, radiansq);
	
	radiansq = _mm_mul_ps(radiansq, _mm_set_ps(1.0f, -0.5f, -0.5f, 1.0f));
	result = _mm_add_ps(result, radiansq);
	result = _mm_add_ps(result, _mm_set_ps(0.0f, 1.0f, 1.0f, 0.0f));

	// Select correct octant
	m128extract rv;
	rv.v = result;
	int sinidx = octant;
	int cosidx = octant ^ 2;

	// adjust sign
	rv.i[sinidx] ^= flip_sign_sine;
	rv.i[cosidx] ^= flip_sign_cosine;

	// relative error less than 1.1e-6 for input values between -360 deg
	// and 360 deg
	// relative error less than 1.2e-7 between -260 deg and 260 deg.
	// absolute error less than 6.0e-8 between -4.2e7 and 4.2e7 deg.
	assert(float_radians == 0.0f || std::abs((rv.f[sinidx] - std::sinf(float_radians)) / std::sinf(float_radians)) < 1.1e-6f);
	assert(float_radians == 0.0f || std::abs((rv.f[cosidx] - std::cosf(float_radians)) / std::cosf(float_radians)) < 1.1e-6f);

	uint32_t cosine_mask = cosine * ~0;
	int idx = (cosine_mask & cosidx) | (~cosine_mask & sinidx);
	return rv.f[idx];
}
