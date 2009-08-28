/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2002, 2005  Sven Eberhardt
 * Copyright (c) 2002, 2004-2005, 2007  Peter Wortmann
 * Copyright (c) 2005, 2007  Günther Brammer
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

/* The Clonk engine uses fixed point math for exact object positions.
   This is rather silly. Nowadays we should simply use floats. However,
	 I never dared changing the whole thing. */
/* 01-17-2002: I think the whole, ugly fixed-thing has to be revived,
   because floating point calculations are not guaranteed to be network
   safe...however, it can be solved as a data type with operator
   overloading, automatic type conversions, etc now   - Sven2 */
/* After some time with synchronous float use, C4Fixed is used again to
   work around the problem that different compilers produce different
   floating point code, leading to desyncs between linux and windows
   engines. */

#ifndef FIXED_H_INC
#define FIXED_H_INC

#include <math.h>

#include "StdCompiler.h"
#include "StdAdaptors.h"

// activate to switch to classic fixed-point math
#define USE_FIXED 1
#define inline GNUC_ALWAYS_INLINE

// note: C4Fixed has to be defined even though it isn't used
//       any more. It is used to convert old-format fixed values
//       to the new-format float ones.

#ifdef USE_FIXED
extern long SineTable[9001]; // external table of sine values
#endif

// fixpoint shift
#define FIXED_SHIFT 16
// fixpoint factor
#define FIXED_FPF int32_t(1 << FIXED_SHIFT)

class C4Fixed
	{
#ifdef USE_FIXED
		friend int fixtoi(const C4Fixed &x);
		friend int fixtoi(const C4Fixed &x, int32_t prec);
		friend C4Fixed itofix(int32_t x);
		friend C4Fixed itofix(int32_t x, int32_t prec);
		friend float fixtof(const C4Fixed &x);
		friend C4Fixed ftofix(float x);
#else
		friend void FIXED_TO_FLOAT(float *pVal);
#endif
		friend inline void CompileFunc(C4Fixed &rValue, StdCompiler *pComp);

	public:
		int32_t val;	// internal value

	public:
		// constructors
		inline C4Fixed () { /*val=0;*/ } // why initialize?
		inline C4Fixed (const C4Fixed &rCpy): val(rCpy.val) { }

		// Conversion must be done by the conversion routines itofix, fixtoi, ftofix and fixtof
		// in order to be backward compatible, so everything is private.
	private:
		explicit inline C4Fixed(int32_t iVal)
			: val (iVal * FIXED_FPF)
		{ }
		explicit inline C4Fixed(int32_t iVal, int32_t iPrec)
			: val( iPrec < FIXED_FPF
							?	iVal * (FIXED_FPF / iPrec) + (iVal * (FIXED_FPF % iPrec)) / iPrec
							: int32_t( int64_t(iVal) * FIXED_FPF / iPrec )
						)
		{ }
		explicit inline C4Fixed(float fVal)
			: val(static_cast<int32_t>(fVal * float(FIXED_FPF)))
		{ }

		// round to int
		int32_t to_int() const
			{
			int32_t r = val;
			// be carefull not to overflow
			r += (val <= 0x7fffffff - FIXED_FPF / 2) * FIXED_FPF / 2;
			// ensure that -x.50 is rounded to -(x+1)
			r -= (r < 0);
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
			r -= (r < 0);
			r >>= FIXED_SHIFT;
			return int32_t(r);
			}
		// convert to floating point value
		float to_float() const
			{
			return float(val) / float(FIXED_FPF);
			}

	public:

		// set integer (allowed for historic reasons)
		inline C4Fixed &operator = (int32_t x) { return *this = C4Fixed(x); }

		// test value
		inline operator bool () const { return !! val; }
		inline bool operator ! () const { return ! val; }

		// arithmetic operations
		inline C4Fixed &operator += (const C4Fixed &fVal2)
			{
			val += fVal2.val;
			return *this;
			}
		inline C4Fixed &operator -= (const C4Fixed &fVal2)
			{
			val -= fVal2.val;
			return *this;
			}
		inline C4Fixed &operator *= (const C4Fixed &fVal2)
			{
			val = int32_t( (int64_t(val) * fVal2.val) / FIXED_FPF );
			return *this;
			}
		inline C4Fixed &operator *= (int32_t iVal2)
			{
			val *= iVal2;
			return *this;
			}
		inline C4Fixed &operator /= (const C4Fixed &fVal2)
			{
			val = int32_t( (int64_t(val) * FIXED_FPF) / fVal2.val );
			return *this;
			}
		inline C4Fixed &operator /= (int32_t iVal2)
			{
			val /= iVal2;
			return *this;
			}
		inline C4Fixed operator - () const
			{
			C4Fixed fr; fr.val=-val; return fr;
			}
		inline C4Fixed operator + () const
			{
			return *this;
			}

		inline bool operator == (const C4Fixed &fVal2) const { return val==fVal2.val; }
		inline bool operator < (const C4Fixed &fVal2) const { return val<fVal2.val; }
		inline bool operator > (const C4Fixed &fVal2) const { return val>fVal2.val; }
		inline bool operator <= (const C4Fixed &fVal2) const { return val<=fVal2.val; }
		inline bool operator >= (const C4Fixed &fVal2) const { return val>=fVal2.val; }
		inline bool operator != (const C4Fixed &fVal2) const { return val!=fVal2.val; }

		// and wrappers
		inline C4Fixed &operator += (int32_t iVal2) { return operator += (C4Fixed(iVal2)); }
		inline C4Fixed &operator -= (int32_t iVal2) { return operator -= (C4Fixed(iVal2)); }

		inline C4Fixed operator + (C4Fixed fVal2) const { return C4Fixed(*this) += fVal2; }
		inline C4Fixed operator - (C4Fixed fVal2) const { return C4Fixed(*this) -= fVal2; }
		inline C4Fixed operator * (C4Fixed fVal2) const { return C4Fixed(*this) *= fVal2; }
		inline C4Fixed operator / (C4Fixed fVal2) const { return C4Fixed(*this) /= fVal2; }

		inline C4Fixed operator + (int32_t iVal2) const { return C4Fixed(*this) += iVal2; }
		inline C4Fixed operator - (int32_t iVal2) const { return C4Fixed(*this) -= iVal2; }
		inline C4Fixed operator * (int32_t iVal2) const { return C4Fixed(*this) *= iVal2; }
		inline C4Fixed operator / (int32_t iVal2) const { return C4Fixed(*this) /= iVal2; }

#if defined(_MSC_VER) && _MSC_VER <= 1200
		inline C4Fixed operator + (int iVal2) const { return operator + (int32_t(iVal2)); }
		inline C4Fixed operator - (int iVal2) const { return operator - (int32_t(iVal2)); }
		inline C4Fixed operator * (int iVal2) const { return operator * (int32_t(iVal2)); }
		inline C4Fixed operator / (int iVal2) const { return operator / (int32_t(iVal2)); }
#endif

		inline bool operator == (int32_t iVal2) const { return operator == (C4Fixed(iVal2)); }
		inline bool operator < (int32_t iVal2) const { return operator < (C4Fixed(iVal2)); }
		inline bool operator > (int32_t iVal2) const { return operator > (C4Fixed(iVal2)); }
		inline bool operator <= (int32_t iVal2) const { return operator <= (C4Fixed(iVal2)); }
		inline bool operator >= (int32_t iVal2) const { return operator >= (C4Fixed(iVal2)); }
		inline bool operator != (int32_t iVal2) const { return operator != (C4Fixed(iVal2)); }

#if defined(_MSC_VER) && _MSC_VER <= 1200
		inline bool operator == (int iVal2) const { return operator == (C4Fixed(int32_t(iVal2))); }
		inline bool operator < (int iVal2) const { return operator < (C4Fixed(int32_t(iVal2))); }
		inline bool operator > (int iVal2) const { return operator > (C4Fixed(int32_t(iVal2))); }
		inline bool operator <= (int iVal2) const { return operator <= (C4Fixed(int32_t(iVal2))); }
		inline bool operator >= (int iVal2) const { return operator >= (C4Fixed(int32_t(iVal2))); }
		inline bool operator != (int iVal2) const { return operator != (C4Fixed(int32_t(iVal2))); }
#endif

#ifdef USE_FIXED
		C4Fixed sin_deg() const
			{
			// adjust angle
			int32_t v=int32_t((int64_t(val)*100)/FIXED_FPF); if (v<0) v=18000-v; v%=36000;
			// get sine
			C4Fixed fr;
			switch (v/9000)
				{
				case 0: fr.val=+SineTable[v];       break;
				case 1: fr.val=+SineTable[18000-v]; break;
				case 2: fr.val=-SineTable[v-18000]; break;
				case 3: fr.val=-SineTable[36000-v]; break;
				}
			return fr;
			}
		C4Fixed cos_deg() const
			{
			// adjust angle
			int32_t v=int32_t((int64_t(val)*100)/FIXED_FPF); if (v<0) v=-v; v%=36000;
			// get cosine
			C4Fixed fr;
			switch (v/9000)
				{
				case 0: fr.val=+SineTable[9000-v]; break;
				case 1: fr.val=-SineTable[v-9000]; break;
				case 2: fr.val=-SineTable[27000-v]; break;
				case 3: fr.val=+SineTable[v-27000]; break;
				}
			return fr;
			}
#endif

	};

#ifdef USE_FIXED

// *** wrap FIXED to C4Fixed

#define FIXED C4Fixed

// conversion
inline float fixtof(const C4Fixed &x) { return x.to_float(); }
inline C4Fixed ftofix(float x) { return C4Fixed(x); }
inline int fixtoi(const C4Fixed &x) { return x.to_int(); }
inline int fixtoi(const C4Fixed &x, int32_t prec) { return x.to_int(prec); }
inline C4Fixed itofix(int32_t x) { return C4Fixed(x); }
inline C4Fixed itofix(int32_t x, int32_t prec) { return C4Fixed(x, prec); }

// additional functions
inline FIXED Sin(FIXED fAngle) { return fAngle.sin_deg(); }
inline FIXED Cos(FIXED fAngle) { return fAngle.cos_deg(); }
inline FIXED FIXED100(int x) { return itofix(x, 100); }
//inline FIXED FIXED256(int x) { return itofix(x, 256); }
inline FIXED FIXED256(int x) { C4Fixed r; r.val = x * FIXED_FPF / 256; return r; }
inline FIXED FIXED10(int x) { return itofix(x, 10); }

#else

// *** wrap FIXED to float

#define FIXED float

// fixtoi: use asm fistp, round up
inline int fixtoi(FIXED x)
{
	int e;
#ifdef _MSC_VER
	float y = x;
	_asm {
		or y,1;
		fld y;
		fistp e;
		}
#else
	asm ("or $1, %0" : "+rom" (x));
	asm ("fistp%z0 %0" : "=om" (e) : "t" (x) : "st");
#endif
	return e;
}

// conversion
inline int fixtoi(FIXED x, int32_t prec) { return fixtoi(x*prec); }
inline FIXED itofix(int x) { return static_cast<FIXED>(x); }
inline FIXED itofix(int x, int prec) { return static_cast<FIXED>(x) / prec; }
inline float fixtof(FIXED x) { return x; }
inline FIXED ftofix(float x) { return x; }

// additional functions
inline FIXED Sin(FIXED x) { return float(sin(x * 3.141592f / 180)); }
inline FIXED Cos(FIXED x) { return float(cos(x * 3.141592f / 180)); }
inline FIXED FIXED100(int x) { return float(x) / 100; }
inline FIXED FIXED256(int x) { return float(x) / 256; }
inline FIXED FIXED10(int x) { return float(x) / 10; }

#endif

// define 0
const FIXED Fix0 = itofix(0);

// conversion...
// note: keep out! really dirty casts!
#ifdef USE_FIXED
inline void FLOAT_TO_FIXED(FIXED *pVal)
{
	*pVal = ftofix (*reinterpret_cast<float *>(pVal));
}
#else
inline void FIXED_TO_FLOAT(FIXED *pVal)
{
	*pVal = reinterpret_cast<C4Fixed *>(pVal)->to_float();
}
#endif

#undef inline

// CompileFunc for FIXED
inline void CompileFunc(FIXED &rValue, StdCompiler *pComp)
{
#ifdef USE_FIXED
	char cFormat = 'F';
#else
	char cFormat = 'f';
#endif
	try {
		// Read/write type
		pComp->Character(cFormat);

	} catch(StdCompiler::NotFoundException *pEx) {
		delete pEx;
		// Expect old format if not found
		cFormat = 'F';
	}
	// Read value (as int32_t)
	pComp->Value(mkCastAdapt<int32_t>(rValue));
	// convert, if neccessary
#ifdef USE_FIXED
	if(cFormat == 'f')
		FLOAT_TO_FIXED(&rValue);
#else
	if(cFormat == 'F')
		FIXED_TO_FLOAT(&rValue);
#endif
}

#endif //FIXED_H_INC
