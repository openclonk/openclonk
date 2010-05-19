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

/* The Clonk engine uses fixed point math for exact object positions.
   This is rather silly. Nowadays we should simply use floats. However,
   I never dared changing the whole thing. */
/* 01-17-2002: I think the whole, ugly fixed-thing has to be revived,
   because floating point calculations are not guaranteed to be network
   safe...however, it can be solved as a data type with operator
   overloading, automatic type conversions, etc now   - Sven2 */
/* After some time with synchronous float use, C4Real is used again to
   work around the problem that different compilers produce different
   floating point code, leading to desyncs between linux and windows
   engines. */

#ifndef INC_C4Real
#define INC_C4Real

// C4RealBase modes:
//   C4REAL_MODE_SOFTWARE:   fixpoint math, all operations emulated by
//                           software
//   C4REAL_MODE_SOFT_FLOAT: floating-point math, all operations emulated by
//                           software
//   C4REAL_MODE_FPU_FLOAT:  floating-point math, all operations run on FPU
//   C4REAL_MODE_SSE_FLOAT:  floating-point math, all operations run on SSE
//                           unit (which is usually the FPU as well, but w/o
//                           extended precision)
#define C4REAL_MODE_SOFTWARE   1
//#define C4REAL_MODE_SOFT_FLOAT 2
#define C4REAL_MODE_FPU_FLOAT  3
#define C4REAL_MODE_SSE_FLOAT  4

#ifndef C4REAL_MODE
#define C4REAL_MODE C4REAL_MODE_SSE_FLOAT
#endif

template<class C4RealImpl>
class C4RealBase
{
	C4RealImpl value;

	friend C4RealBase Sin(const C4RealBase &);
	friend C4RealBase Cos(const C4RealBase &);
public:
	inline C4RealBase(int32_t val = 0) : value(val) { }
	inline C4RealBase(float val) : value(val) {}
	inline C4RealBase(int32_t val, int32_t prec) : value(val) { operator/=(C4RealBase(prec)); }
	// Conversion between different implementations of C4RealBase
	template<class T>
	inline C4RealBase(const C4RealBase<T> &val) : value(static_cast<float>(val)) { }

	// Copy ctor and assignment
	inline C4RealBase(const C4RealBase &rhs) : value(rhs.value) {}
	inline C4RealBase &operator = (const C4RealBase &rhs) { value = rhs.value; return *this; }
	inline C4RealBase &operator = (float rhs) { value = C4RealImpl(rhs); return *this; }
	inline C4RealBase &operator = (int rhs) { value = C4RealImpl(rhs); return *this; }

	// arithmetic operations
#define C4REAL_ARITHMETIC_OPERATOR(op) \
	/* combined arithmetic and assignment ops */ \
	inline C4RealBase &operator op##= (const C4RealBase &rhs) { value op##= rhs.value; return *this; } \
	inline C4RealBase &operator op##= (float rhs) { return *this op##= C4RealBase(rhs); } \
	inline C4RealBase &operator op##= (int rhs) { return *this op##= C4RealBase(rhs); } \
	/* arithmetic operations on copies */ \
	inline C4RealBase operator op (const C4RealBase &rhs) const { C4RealBase nrv(*this); nrv op##= rhs; return nrv; } \
	inline C4RealBase operator op (float rhs) const { C4RealBase nrv(*this); nrv op##= rhs; return nrv; } \
	inline C4RealBase operator op (int rhs) const { C4RealBase nrv(*this); nrv op##= rhs; return nrv; } \
	/* arithmetic operations on copies, right-hand C4Real */ \
	/* friends defined in the class are implicitly inline */ \
	friend C4RealBase operator op (float lhs, const C4RealBase &rhs) { C4RealBase nrv(lhs); nrv op##= rhs; return nrv; } \
	friend C4RealBase operator op (int lhs, const C4RealBase &rhs) { C4RealBase nrv(lhs); nrv op##= rhs; return nrv; }

	C4REAL_ARITHMETIC_OPERATOR(+)
	C4REAL_ARITHMETIC_OPERATOR(-)
	C4REAL_ARITHMETIC_OPERATOR(*)
	C4REAL_ARITHMETIC_OPERATOR(/)
#undef C4REAL_ARITHMETIC_OPERATOR
	
	inline C4RealBase operator + () const { return *this; }
	inline C4RealBase operator - () const
{
		C4RealBase nrv(*this);
		nrv.value = -nrv.value;
		return nrv;
	}

#define C4REAL_COMPARISON_OPERATOR(op) \
	inline bool operator op (const C4RealBase &rhs) const { return value op rhs.value; } \
	inline bool operator op (float rhs) const { return *this op C4RealBase(rhs); } \
	inline bool operator op (int rhs) const { return *this op C4RealBase(rhs); }
	C4REAL_COMPARISON_OPERATOR(<)
	C4REAL_COMPARISON_OPERATOR(<=)
	C4REAL_COMPARISON_OPERATOR(==)
	C4REAL_COMPARISON_OPERATOR(>=)
	C4REAL_COMPARISON_OPERATOR(>)
	C4REAL_COMPARISON_OPERATOR(!=)
#undef C4REAL_COMPARISON_OPERATOR

	// Conversion
	inline operator int() const { return value; }
	inline operator float() const { return value; }

	// Boolean operators
	// Not using safe-bool-idiom here, because we already define conversions
	// to integer, which is why we don't need to worry about unwanted con-
	// versions via operator bool
	inline operator bool() const { return static_cast<bool>(value); }
	inline bool operator !() const { return !operator bool(); }
};

typedef C4RealBase<class C4RealImpl_Fixed> C4Real_Fixed;
typedef C4RealBase<float> C4Real_FPU_Float;
typedef C4RealBase<class C4RealImpl_SSE> C4Real_SSE_Float;

#include "C4RealImpl_Fixed.h"
#include "C4RealImpl_FPU.h"
#include "C4RealImpl_SSE.h"

// *** wrap C4Real to requested C4RealBase instantiation

#if C4REAL_MODE == C4REAL_MODE_SOFTWARE
typedef C4Real_Fixed C4Real;
#elif C4REAL_MODE == C4REAL_MODE_FPU_FLOAT
typedef C4Real_FPU_Float C4Real;
#elif C4REAL_MODE == C4REAL_MODE_SSE_FLOAT
typedef C4Real_SSE_Float C4Real;
#endif

// Instantiate other C4RealBases as well
template class C4RealBase<C4RealImpl_Fixed>;
template class C4RealBase<float>;
template class C4RealBase<C4RealImpl_SSE>;

// conversion
inline float fixtof(const C4Real &x) { return static_cast<float>(x); }
inline C4Real ftofix(float x) { return C4Real(x); }
inline int fixtoi(const C4Real &x) { return static_cast<int>(x); }
inline int fixtoi(const C4Real &x, int32_t prec) { return static_cast<int>(x * prec); }
inline C4Real itofix(int32_t x) { return C4Real(x); }
inline C4Real itofix(int32_t x, int32_t prec) { return C4Real(x, prec); }

// additional functions
inline C4Real C4REAL100(int x) { C4Real nrv(x); nrv /= 100; return nrv; }
inline C4Real C4REAL256(int x) { C4Real nrv(x); nrv /= 256; return nrv; }
inline C4Real C4REAL10(int x) { C4Real nrv(x); nrv /= 10; return nrv; }

// define 0
const C4Real Fix0 = C4Real(0);

// CompileFunc for C4Real
void CompileFunc(C4Real &rValue, StdCompiler *pComp);

#endif //C4REAL_H_INC
