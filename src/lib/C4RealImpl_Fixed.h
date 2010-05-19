/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2002, 2005  Sven Eberhardt
 * Copyright (c) 2002, 2004-2005, 2007  Peter Wortmann
 * Copyright (c) 2005, 2007  Günther Brammer
 * Copyright (c) 2010  Nicolas Hake
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

/* Fixed point math extracted from ALLEGRO by Shawn Hargreaves */

#ifndef INC_RealImpl_Fixed
#define INC_RealImpl_Fixed

#ifndef INC_C4Real
#error C4RealImpl_Fixed.h must not be included by itself; include C4Real.h instead
#endif

#define FIXED_EMULATE_64BIT

#include <boost/operators.hpp>
class C4RealImpl_Fixed
	: boost::totally_ordered<C4RealImpl_Fixed,
	  boost::equivalent<C4RealImpl_Fixed
	  > >
{
	// fixpoint shift (check 64 bit emulation before changing!)
	static const int32_t FIXED_SHIFT = 16;
	static const int32_t FIXED_FPF = 1 << FIXED_SHIFT;

	int32_t val; // Internal value
	static long SineTable[9001]; // external table of sine values

public:
	inline C4RealImpl_Fixed() : val(0) {}
	inline C4RealImpl_Fixed(const C4RealImpl_Fixed &rhs) : val(rhs.val) {}
	explicit inline C4RealImpl_Fixed(int32_t iVal) : val (iVal * FIXED_FPF) { }
	explicit inline C4RealImpl_Fixed(int32_t iVal, int32_t iPrec)
		: val( iPrec < FIXED_FPF
		       ? iVal * (FIXED_FPF / iPrec) + (iVal * (FIXED_FPF % iPrec)) / iPrec
		       : int32_t( int64_t(iVal) * FIXED_FPF / iPrec )
		     )
	{ }
	explicit inline C4RealImpl_Fixed(float fVal)
			: val(static_cast<int32_t>(fVal * float(FIXED_FPF)))
	{ }

	int32_t to_int() const
	{
		int32_t r = val;
		// be careful not to overflow
		r += (val <= 0x7fffffff - FIXED_FPF / 2) * FIXED_FPF / 2;
		// ensure that -x.50 is rounded to -(x+1)
		r -= (val < 0);
		r >>= FIXED_SHIFT;
		// round 32767.5 to 32768 (not that anybody cares)
		r += (val > 0x7fffffff - FIXED_FPF / 2);
		return r;
	}
	int32_t to_int(int32_t prec) const
	{
		int64_t r = val;
		r *= prec;
		r += FIXED_FPF / 2;
		r -= (val < 0);
		r >>= FIXED_SHIFT;
		return int32_t(r);
	}

	// convert to floating point value
	float to_float() const
	{
		return float(val) / float(FIXED_FPF);
	}

	// Arithmetics.
	C4RealImpl_Fixed &operator += (const C4RealImpl_Fixed &rhs)
	{
		val += rhs.val;
		return *this;
	}
	C4RealImpl_Fixed &operator -= (const C4RealImpl_Fixed &rhs)
	{
		val -= rhs.val;
		return *this;
	}
	C4RealImpl_Fixed &operator *= (const C4RealImpl_Fixed &rhs)
	{
#ifndef FIXED_EMULATE_64BIT
		val = int32_t( (int64_t(val) * rhs.val) / FIXED_FPF );
#else
		int32_t x0 = val & (FIXED_FPF - 1),
		             x1 = val >> FIXED_SHIFT;
		int32_t y0 = rhs.val & (FIXED_FPF - 1),
		             y1 = rhs.val >> FIXED_SHIFT;
		val = x0*y0/FIXED_FPF + x0*y1 + x1*y0 + x1*y1*FIXED_FPF;
#endif
		return *this;
	}
	C4RealImpl_Fixed &operator /= (const C4RealImpl_Fixed &rhs)
	{
		val = int32_t( (int64_t(val) * FIXED_FPF) / rhs.val );
		return *this;
	}
	C4RealImpl_Fixed operator - ()
	{
		C4RealImpl_Fixed nrv(*this);
		nrv.val = -nrv.val;
		return nrv;
	}

	C4RealImpl_Fixed sin_deg() const
	{
		// adjust angle
		int32_t v=int32_t((int64_t(val)*100)/FIXED_FPF); if (v<0) v=18000-v; v%=36000;
		// get sine
		C4RealImpl_Fixed fr;
		switch (v/9000)
		{
		case 0: fr.val=+SineTable[v];       break;
		case 1: fr.val=+SineTable[18000-v]; break;
		case 2: fr.val=-SineTable[v-18000]; break;
		case 3: fr.val=-SineTable[36000-v]; break;
		}
		return fr;
	}
	C4RealImpl_Fixed cos_deg() const
	{
		// adjust angle
		int32_t v=int32_t((int64_t(val)*100)/FIXED_FPF); if (v<0) v=-v; v%=36000;
		// get cosine
		C4RealImpl_Fixed fr;
		switch (v/9000)
		{
		case 0: fr.val=+SineTable[9000-v]; break;
		case 1: fr.val=-SineTable[v-9000]; break;
		case 2: fr.val=-SineTable[27000-v]; break;
		case 3: fr.val=+SineTable[v-27000]; break;
		}
		return fr;
	}

	// Comparison
	bool operator < (const C4RealImpl_Fixed &rhs) const { return val < rhs.val; }

	operator bool () const { return val != 0; }

	// Conversion
	operator float () const { return to_float(); }
	operator int () const { return to_int(); }
};

// Avoid overflowing
template<>
inline C4RealBase<C4RealImpl_Fixed>::C4RealBase(int32_t val, int32_t prec)
	: value(val, prec)
{}

inline C4Real_Fixed Sin(const C4Real_Fixed &real)
{
	C4Real_Fixed nrv;
	nrv.value = real.value.sin_deg();
	return nrv;
}
inline C4Real_Fixed Cos(const C4Real_Fixed &real)
{
	C4Real_Fixed nrv;
	nrv.value = real.value.cos_deg();
	return nrv;
}

#endif
