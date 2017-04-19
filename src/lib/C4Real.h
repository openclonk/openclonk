/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2016, The OpenClonk Team and contributors
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

#ifndef INC_C4Real
#define INC_C4Real

// activate to switch to classic fixed-point math
#define C4REAL_USE_FIXNUM 1
#define inline ALWAYS_INLINE

// gcc 4.6 generates better code for FIXED_EMULATE_64BIT disablen for both
// 32 and 64 bit. It properly recognizes that the 32,32->64 multiplication
// instruction of the x86 is the right choice for the job whereas the more
// complicated FIXED_EMULATE_64BIT version requires multiple multiplications
//#define FIXED_EMULATE_64BIT

// note: C4Fixed has to be defined even though it isn't used
//       any more. It is used to convert old-format fixed values
//       to the new-format float ones.

#ifdef C4REAL_USE_FIXNUM
extern long SineTable[9001]; // external table of sine values
#endif

// fixpoint shift (check 64 bit emulation before changing!)
#define FIXED_SHIFT 16
// fixpoint factor
#define FIXED_FPF int32_t(1 << FIXED_SHIFT)

class C4Fixed
{
#ifdef C4REAL_USE_FIXNUM
	friend int fixtoi(const C4Fixed &x);
	friend int fixtoi(const C4Fixed &x, int32_t prec);
	friend C4Fixed itofix(int32_t x);
	friend C4Fixed itofix(int32_t x, int32_t prec);
	friend float fixtof(const C4Fixed &x);
	friend C4Fixed ftofix(float x);
#else
	friend void FIXED_TO_FLOAT(float *pVal);
#endif
	friend void CompileFunc(C4Fixed &rValue, StdCompiler *pComp);

public:
	int32_t val;  // internal value

public:
	// constructors
	inline C4Fixed () = default;
	inline C4Fixed (const C4Fixed &) = default;

	// Conversion must be done by the conversion routines itofix, fixtoi, ftofix and fixtof
	// in order to be backward compatible, so everything is private.
private:
	explicit inline C4Fixed(int32_t iVal)
			: val (iVal * FIXED_FPF)
	{ }
	explicit inline C4Fixed(int32_t iVal, int32_t iPrec)
			: val( iPrec < FIXED_FPF
			       ? iVal * (FIXED_FPF / iPrec) + (iVal * (FIXED_FPF % iPrec)) / iPrec
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
		// round towards positive infinity
		r >>= (FIXED_SHIFT - 1);
		r += 1;
		r >>= 1;
		return r;
	}
	int32_t to_int(int32_t prec) const
	{
		int64_t r = val;
		r *= prec;
		r += FIXED_FPF / 2;
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
	explicit operator bool () const { return val != 0; }
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
#ifndef FIXED_EMULATE_64BIT
		val = int32_t( (int64_t(val) * fVal2.val) / FIXED_FPF );
#else
		uint32_t x0 = val & (FIXED_FPF - 1);
		int32_t x1 = val >> FIXED_SHIFT;
		uint32_t y0 = fVal2.val & (FIXED_FPF - 1);
		int32_t y1 = fVal2.val >> FIXED_SHIFT;
		val = int32_t(x0*y0/FIXED_FPF) + int32_t(x0)*y1 + x1*int32_t(y0) + x1*y1*FIXED_FPF;
#endif
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

	inline C4Fixed operator + (const C4Fixed &fVal2) const { return C4Fixed(*this) += fVal2; }
	inline C4Fixed operator - (const C4Fixed &fVal2) const { return C4Fixed(*this) -= fVal2; }
	inline C4Fixed operator * (const C4Fixed &fVal2) const { return C4Fixed(*this) *= fVal2; }
	inline C4Fixed operator / (const C4Fixed &fVal2) const { return C4Fixed(*this) /= fVal2; }

	inline C4Fixed operator + (int32_t iVal2) const { return C4Fixed(*this) += iVal2; }
	inline C4Fixed operator - (int32_t iVal2) const { return C4Fixed(*this) -= iVal2; }
	inline C4Fixed operator * (int32_t iVal2) const { return C4Fixed(*this) *= iVal2; }
	inline C4Fixed operator / (int32_t iVal2) const { return C4Fixed(*this) /= iVal2; }

	inline C4Fixed operator + (float iVal2) const { return C4Fixed(*this) += iVal2; }
	inline C4Fixed operator - (float iVal2) const { return C4Fixed(*this) -= iVal2; }
	inline C4Fixed operator * (float iVal2) const { return C4Fixed(*this) *= iVal2; }
	inline C4Fixed operator / (float iVal2) const { return C4Fixed(*this) /= iVal2; }

	inline bool operator == (int32_t iVal2) const { return operator == (C4Fixed(iVal2)); }
	inline bool operator < (int32_t iVal2) const { return operator < (C4Fixed(iVal2)); }
	inline bool operator > (int32_t iVal2) const { return operator > (C4Fixed(iVal2)); }
	inline bool operator <= (int32_t iVal2) const { return operator <= (C4Fixed(iVal2)); }
	inline bool operator >= (int32_t iVal2) const { return operator >= (C4Fixed(iVal2)); }
	inline bool operator != (int32_t iVal2) const { return operator != (C4Fixed(iVal2)); }

	inline bool operator == (float iVal2) const { return operator == (C4Fixed(iVal2)); }
	inline bool operator < (float iVal2) const { return operator < (C4Fixed(iVal2)); }
	inline bool operator > (float iVal2) const { return operator > (C4Fixed(iVal2)); }
	inline bool operator <= (float iVal2) const { return operator <= (C4Fixed(iVal2)); }
	inline bool operator >= (float iVal2) const { return operator >= (C4Fixed(iVal2)); }
	inline bool operator != (float iVal2) const { return operator != (C4Fixed(iVal2)); }

#ifdef C4REAL_USE_FIXNUM
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

#ifdef C4REAL_USE_FIXNUM

typedef C4Fixed C4Real;

// conversion
inline float fixtof(const C4Fixed &x) { return x.to_float(); }
inline C4Fixed ftofix(float x) { return C4Fixed(x); }
inline int fixtoi(const C4Fixed &x) { return x.to_int(); }
inline int fixtoi(const C4Fixed &x, int32_t prec) { return x.to_int(prec); }
inline C4Fixed itofix(int32_t x) { return C4Fixed(x); }
inline C4Fixed itofix(int32_t x, int32_t prec) { return C4Fixed(x, prec); }

// additional functions
inline C4Real Sin(const C4Real &fAngle) { return fAngle.sin_deg(); }
inline C4Real Cos(const C4Real &fAngle) { return fAngle.cos_deg(); }
inline C4Real C4REAL100(int x) { return itofix(x, 100); }
inline C4Real C4REAL256(int x) { C4Fixed r; r.val = x * FIXED_FPF / 256; return r; }
inline C4Real C4REAL10(int x) { return itofix(x, 10); }

#else

// *** wrap C4Real to float

typedef float C4Real;

// fixtoi: use asm fistp, round up
inline int fixtoi(C4Real x)
{
	int e;
#ifdef _MSC_VER
	float y = x;
	_asm
	{
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
inline int fixtoi(const C4Real &x, int32_t prec) { return fixtoi(x*prec); }
inline C4Real itofix(int x) { return static_cast<C4Real>(x); }
inline C4Real itofix(int x, int prec) { return static_cast<C4Real>(x) / prec; }
inline float fixtof(const C4Real &x) { return x; }
inline C4Real ftofix(float x) { return x; }

// additional functions
inline C4Real Sin(C4Real x) { return float(sin(x * 3.141592f / 180)); }
inline C4Real Cos(C4Real x) { return float(cos(x * 3.141592f / 180)); }
inline C4Real C4REAL100(int x) { return float(x) / 100; }
inline C4Real C4REAL256(int x) { return float(x) / 256; }
inline C4Real C4REAL10(int x) { return float(x) / 10; }

#endif
// define 0
const C4Real Fix0 = itofix(0);
const C4Real Fix1 = itofix(1);

// conversion...
// note: keep out! really dirty casts!
#ifdef C4REAL_USE_FIXNUM
inline void FLOAT_TO_FIXED(C4Real *pVal)
{
	*pVal = ftofix (*reinterpret_cast<float *>(pVal));
}
#else
inline void FIXED_TO_FLOAT(C4Real *pVal)
{
	*pVal = reinterpret_cast<C4Fixed *>(pVal)->to_float();
}
#endif

#undef inline

// CompileFunc for C4Real
void CompileFunc(C4Real &rValue, StdCompiler *pComp);

#endif //FIXED_H_INC
