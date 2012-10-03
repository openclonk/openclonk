/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2012  Julius Michaelis
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

// "abstract" type for numeric values
// is pretty much C4Value with int, float, nil or bool enforced.

#ifndef C4_NUMERIC_H
#define C4_NUMERIC_H

#include "C4Value.h"

class C4Numeric {
	friend class C4Value;
	private:
	C4Value val;
	public:
	C4Numeric() : val() {}
	C4Numeric(const C4Numeric &nValue) : val(nValue.val) {}
	C4Numeric(const C4Value &nValue) : val(nValue) { if(!val.CheckConversion(C4V_Numeric)) Set0(); }
	enum Security { unchecked };
	C4Numeric(const C4Value &nValue, Security s) : val(nValue) {}
	explicit C4Numeric(bool data) : val(data) {}
	C4Numeric(int32_t data) : val(data) {}
	C4Numeric(C4Real data) : val(data) {}
	const C4Value& getVal() const { return val; }
	C4Numeric& operator = (const C4Numeric& nValue) { val.Set(nValue.val); return *this; }
	operator C4Real() const { return val.getFloat(); }
#define C4NUMERIC_GETTER(name, type) \
	type name() const { return val.name (); } \
	type _##name() const { return val._##name(); }
	C4NUMERIC_GETTER(getInt,int32_t)
	C4NUMERIC_GETTER(getFloat,C4Real)
	C4NUMERIC_GETTER(getBool,bool)
#undef C4NUMERIC_GETTER
	bool operator ! () const { return !val; }
	inline operator const void* () const { return val; }  // To allow use of C4Value in conditions
	void SetInt(int i) { val.SetInt(i); }
	void SetFloat(C4Real f) { val.SetFloat(f); }
	void SetBool(bool b) { val.SetBool(b); }
	void Set0() { val.Set0(); }
#define C4NUMERIC_OPERATOR(op) \
	inline C4Numeric &operator op##= (const C4Numeric &rhs) { val op##= rhs.val; return *this; } \
	inline C4Numeric operator op (const C4Numeric &rhs) const { C4Value ret = val; ret op##= rhs.val; return C4Numeric(ret); } \
	inline C4Numeric &operator op##= (const C4Real &rhs) { val op##= C4Value(rhs); return *this; } \
	inline C4Numeric operator op (const C4Real &rhs) const { C4Value ret = val; ret op##= C4Value(rhs); return C4Numeric(ret); } \
	inline C4Numeric &operator op##= (const int32_t &rhs) { val op##= C4Value(rhs); return *this; } \
	inline C4Numeric operator op (const int32_t &rhs) const { C4Value ret = val; ret op##= C4Value(rhs); return C4Numeric(ret); } \
	inline C4Numeric &operator op##= (const long &rhs) { val op##= C4Value((int32_t)rhs); return *this; } \
	inline C4Numeric operator op (const long &rhs) const { C4Value ret = val; ret op##= C4Value((int32_t)rhs); return C4Numeric(ret); } 
	C4NUMERIC_OPERATOR(+)
	C4NUMERIC_OPERATOR(-)
	C4NUMERIC_OPERATOR(*)
	C4NUMERIC_OPERATOR(/)
	C4NUMERIC_OPERATOR(%)
#undef C4NUMERIC_OPERATOR

	C4Numeric & operator++() { ++val; return *this; }
	C4Numeric operator++(int) { C4Numeric old = *this; ++val; return old; }
	C4Numeric & operator--() {--val; return *this; }
	C4Numeric operator--(int) { C4Numeric old = *this; --val; return old; }
	C4Numeric operator-() { C4Value val = -this->val; return C4Numeric(val); }
	C4Numeric & operator+() { return *this; }

	C4Numeric Pow(const C4Numeric &rhs) const { return C4Numeric(val.Pow(rhs.val)); }

	// getters
	C4V_Data GetData()    const { return val.GetData(); }
	C4V_Type GetType()    const { return val.GetType(); }

	StdStrBuf GetDataString(int depth = 1) const { return val.GetDataString(); }

};

// inline all these functions to prevent double definition linker errors
#define COMPND(type1, type2) \
	inline bool operator != (const type1& Value1, const type2& Value2) { return !(Value1 == Value2); } \
	inline bool operator == (const type1& V1, const type2& V2) 
#define COMP(type1, type2)\
	bool operator == (const type1&, const type2&); \
	COMPND(type1, type2)
#define BICOMP(type1, type2) \
	bool operator == (const type1&, const type2&); \
	inline bool operator == (const type2& V2, const type1& V1) { return V1 == V2; } \
	inline bool operator != (const type2& V2, const type1& V1) { return !(V1 == V2); } \
	COMPND(type1, type2)
COMP(C4Numeric, C4Numeric) { return V1.getVal() == V2.getVal(); }
BICOMP(C4Numeric, C4Value) { return V1.getVal() == V2; }
BICOMP(C4Numeric, int32_t) { return V1.getInt() == V2; }
BICOMP(C4Numeric, long)    { return V1.getInt() == V2; }
BICOMP(C4Numeric, C4Real)  { return V1.getFloat() == V2; }
BICOMP(C4Numeric, bool) { return V1.getBool() == V2; }
#undef BICOMP
#undef COMPND
#undef COMP

#define RC4NUMERIC_OPERATOR(op) \
	inline int32_t & operator op##= (int32_t & lhs, const C4Numeric & rhs) { return lhs += rhs.getInt(); } \
	inline C4Numeric operator op (const int32_t & lhs, const C4Numeric & rhs) { return C4Numeric(lhs) op##= rhs; } \
	inline long & operator op##= (long & lhs, const C4Numeric & rhs) { return lhs += rhs.getInt(); } \
	inline C4Numeric operator op (const long & lhs, const C4Numeric & rhs) { return C4Numeric((int32_t)lhs) op##= rhs; } \
	inline C4Real & operator op##= (C4Real & lhs, const C4Numeric & rhs) { return lhs += rhs.getFloat(); } \
	inline C4Numeric operator op (const C4Real & lhs, const C4Numeric & rhs) { return C4Numeric(lhs) op##= rhs; }
	RC4NUMERIC_OPERATOR(+)
	RC4NUMERIC_OPERATOR(-)
	RC4NUMERIC_OPERATOR(*)
	RC4NUMERIC_OPERATOR(/)
	RC4NUMERIC_OPERATOR(%)
#undef RC4NUMERIC_OPERATOR

#define BI_INEQ_COMP(t, op) \
	inline bool operator op (const t & v1, const C4Numeric & v2) { if(v2.GetType() == C4V_Float) return C4Real(v1) op v2.getFloat(); return v1 op v2.getInt(); } \
	inline bool operator op (const C4Numeric & v1, const t & v2) { if(v1.GetType() == C4V_Float) return v1.getFloat() op C4Real(v2); return v1.getInt() op v2; }
#define INEQ_COMP(op) \
	inline bool operator op (const C4Numeric & n1, const C4Numeric & n2) { return n1.getVal() op n2.getVal(); } \
	BI_INEQ_COMP(int32_t, op) \
	BI_INEQ_COMP(long, op) \
	BI_INEQ_COMP(C4Real, op)
	INEQ_COMP(<)
	INEQ_COMP(>)
	INEQ_COMP(<=)
	INEQ_COMP(>=)
#undef INEQ_COMP
#undef BI_INEQ_COMP

inline C4Real Sin(const C4Numeric &real) { return Sin(real.getFloat()); }
inline C4Real Cos(const C4Numeric &real) { return Cos(real.getFloat()); }
inline C4Numeric Pow(const C4Numeric &x, const C4Numeric &y) { return x.Pow(y); }
C4Numeric Sqrt(const C4Numeric & v);

#endif // includeguard
