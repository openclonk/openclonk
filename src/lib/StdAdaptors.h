/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2007  Sven Eberhardt
 * Copyright (c) 2005-2008  Peter Wortmann
 * Copyright (c) 2005-2008  Günther Brammer
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
#ifndef STDADAPTORS_H
#define STDADAPTORS_H

#include "Standard.h"
#include "StdCompiler.h"

// * Wrappers for C4Compiler-types

// Whole-line string, automatic size deduction (C4Compiler-String)
#define toC4CStr(szString) mkStringAdaptMA(szString)
#define toC4CStrBuf(rBuf) mkParAdapt(rBuf, StdCompiler::RCT_All)

// Integer-array with default 0, automatic size deduction
#define toC4CArr(rArr) mkArrayAdaptDM(rArr, 0)
#define toC4CArrU(rArr) mkArrayAdaptDM(rArr, 0u)

// * Null Adaptor
// Doesn't compile anything
struct StdNullAdapt
{
	inline void CompileFunc(StdCompiler *pComp) const { }
	ALLOW_TEMP_TO_REF(StdNullAdapt)
};

// * Defaulting Adaptor
// Sets default if CompileFunc fails with a Exception of type NotFoundException
template <class T, class D>
struct StdDefaultAdapt
{
	T &rValue; const D &rDefault;
	StdDefaultAdapt(T &rValue, const D &rDefault) : rValue(rValue), rDefault(rDefault) { }
	inline void CompileFunc(StdCompiler *pComp) const
	{
		try
		{
#ifdef STDCOMPILER_EXCEPTION_WORKAROUND
			if (!pComp->ValueSafe(rValue))
				rValue = rDefault;
#else
			pComp->Value(rValue);
#endif
		}
		catch (StdCompiler::NotFoundException *pEx)
		{
			rValue = rDefault;
			delete pEx;
		}
	}
	ALLOW_TEMP_TO_REF(StdDefaultAdapt)
};
template <class T, class D>
inline StdDefaultAdapt<T, D> mkDefaultAdapt(T RREF rValue, const D &rDefault) { return StdDefaultAdapt<T, D>(rValue, rDefault); }

// * Naming Adaptor
// Embeds a value into a named section, failsafe
// (use for values that do defaulting themselves - e.g. objects using naming)
template <class T>
struct StdNamingAdapt
{
	T &rValue; const char *szName;
	StdNamingAdapt(T &rValue, const char *szName) : rValue(rValue), szName(szName) { }
	inline void CompileFunc(StdCompiler *pComp) const
	{
		pComp->Name(szName);
		try
		{
			pComp->Value(rValue);
		}
		catch (StdCompiler::Exception *)
		{
			pComp->NameEnd(true);
			throw;
		}
		pComp->NameEnd();
	}
	template <class D> inline bool operator == (const D &nValue) const { return rValue == nValue; }
	template <class D> inline StdNamingAdapt &operator = (const D &nValue) { rValue = nValue; return *this; }
	ALLOW_TEMP_TO_REF(StdNamingAdapt)
};
template <class T>
inline StdNamingAdapt<T> mkNamingAdapt(T RREF rValue, const char *szName) { return StdNamingAdapt<T>(rValue, szName); }

// * Naming Adaptor (defaulting)
// Embeds a value into a named section, sets default on fail
template <class T, class D>
struct StdNamingDefaultAdapt
{
	T &rValue; const char *szName; const D &rDefault; bool fPrefillDefault; bool fStoreDefault;
	StdNamingDefaultAdapt(T &rValue, const char *szName, const D &rDefault, bool fPrefillDefault, bool fStoreDefault) : rValue(rValue), szName(szName), rDefault(rDefault), fPrefillDefault(fPrefillDefault), fStoreDefault(fStoreDefault) { }
	inline void CompileFunc(StdCompiler *pComp) const
	{
		// Default check
		if (pComp->hasNaming() && pComp->isDecompiler() && rValue == rDefault && !fStoreDefault)
		{
			if (pComp->Default(szName)) return;
		}
		try
		{
			// Search named section, set default if not found
			if (pComp->Name(szName))
			{
				if (fPrefillDefault && pComp->isCompiler()) rValue = rDefault; // default prefill if desired
				pComp->Value(mkDefaultAdapt(rValue, rDefault));
			}
			else
				rValue = rDefault;
		}
		catch (StdCompiler::Exception *)
		{
			pComp->NameEnd(true);
			throw;
		}
		// End section
		pComp->NameEnd();
	}
	ALLOW_TEMP_TO_REF(StdNamingDefaultAdapt)
};
template <class T, class D>
inline StdNamingDefaultAdapt<T,D> mkNamingAdapt(T RREF rValue, const char *szName, const D &rDefault, bool fPrefillDefault=false, bool fStoreDefault=false) { return StdNamingDefaultAdapt<T,D>(rValue, szName, rDefault, fPrefillDefault, fStoreDefault); }

// * Decompiling Adaptor
// Allows to use const objects if the compiler won't change the targets
template <class T>
struct StdDecompileAdapt
{
	const T &rValue;
	explicit StdDecompileAdapt(const T &rValue) : rValue(rValue) { }
	inline void CompileFunc(StdCompiler *pComp) const
	{
		assert(pComp->isDecompiler());
		pComp->Value(const_cast<T &>(rValue));
	}
	
	// make this work with in combination with StdParameterAdapt
	template<class P>
	inline void CompileFunc(StdCompiler* pComp, const P& par) const
	{
		assert(pComp->isDecompiler());
		pComp->Value(mkParAdapt(const_cast<T &>(rValue), par));
	}

	ALLOW_TEMP_TO_REF(StdDecompileAdapt)
};
template <class T>
inline StdDecompileAdapt<T> mkDecompileAdapt(const T& rValue) { return StdDecompileAdapt<T>(rValue); }

// * Runtime value Adaptor
// Allows the C4ValueSetCompiler to set the value
template <class T>
struct StdRuntimeValueAdapt
{
	T &rValue;
	explicit StdRuntimeValueAdapt(T &rValue) : rValue(rValue) { }
	inline void CompileFunc(StdCompiler *pComp) const
	{
		pComp->setRuntimeWritesAllowed(+1);
		pComp->Value(rValue);
		pComp->setRuntimeWritesAllowed(-1);
	}
	template <class D> inline bool operator == (const D &nValue) const { return rValue == nValue; }
	template <class D> inline StdRuntimeValueAdapt<T> &operator = (const D &nValue) { rValue = nValue; return *this; }
	ALLOW_TEMP_TO_REF(StdRuntimeValueAdapt)
};
template <class T>
inline StdRuntimeValueAdapt<T> mkRuntimeValueAdapt(T RREF rValue) { return StdRuntimeValueAdapt<T>(rValue); }

// * String adaptor
struct StdStringAdapt
{
	char *szString; int iMaxLength; StdCompiler::RawCompileType eRawType;
	StdStringAdapt(char *szString, int iMaxLength, StdCompiler::RawCompileType eRawType = StdCompiler::RCT_Escaped)
			: szString(szString), iMaxLength(iMaxLength), eRawType(eRawType) { }
	inline void CompileFunc(StdCompiler *pComp) const
	{
		pComp->String(szString, iMaxLength, eRawType);
	}
	inline bool operator == (const char *szDefault) const { return SEqual(szString, szDefault); }
	inline StdStringAdapt &operator = (const char *szDefault) { SCopy(szDefault, szString, iMaxLength); return *this; }
	ALLOW_TEMP_TO_REF(StdStringAdapt)
};
inline StdStringAdapt mkStringAdapt(char *szString, int iMaxLength, StdCompiler::RawCompileType eRawType = StdCompiler::RCT_Escaped)
{ return StdStringAdapt(szString, iMaxLength, eRawType); }
#define mkStringAdaptM(szString) mkStringAdapt(szString, (sizeof(szString) / sizeof(*szString)) - 1)
#define mkStringAdaptMA(szString) mkStringAdapt(szString, (sizeof(szString) / sizeof(*szString)) - 1, StdCompiler::RCT_All)
#define mkStringAdaptMI(szString) mkStringAdapt(szString, (sizeof(szString) / sizeof(*szString)) - 1, StdCompiler::RCT_Idtf)
#define mkStringAdaptMIE(szString) mkStringAdapt(szString, (sizeof(szString) / sizeof(*szString)) - 1, StdCompiler::RCT_IdtfAllowEmpty)

// * Raw adaptor
struct StdRawAdapt
{
	void *pData; size_t iSize; StdCompiler::RawCompileType eRawType;
	StdRawAdapt(void *pData, size_t iSize, StdCompiler::RawCompileType eRawType = StdCompiler::RCT_Escaped)
			: pData(pData), iSize(iSize), eRawType(eRawType) { }
	inline void CompileFunc(StdCompiler *pComp) const
	{
		pComp->Raw(pData, iSize, eRawType);
	}
	inline bool operator == (const void *pDefault) const { return !memcmp(pDefault, pData, iSize); }
	inline StdRawAdapt &operator = (const void *pDefault) { memcpy(pData, pDefault, iSize); return *this; }
};
inline StdRawAdapt mkRawAdapt(void *pData, size_t iSize, StdCompiler::RawCompileType eRawType = StdCompiler::RCT_Escaped)
{ return StdRawAdapt(pData, iSize, eRawType); }
#define mkRawAdaptM(X) mkRawAdapt(&X, sizeof(X))

// * Integer Adaptor
// Stores Integer-like datatypes (Enumerations)
template <class T, class int_t = int32_t>
struct StdIntAdapt
{
	T &rValue;
	explicit StdIntAdapt(T &rValue) : rValue(rValue) { }
	inline void CompileFunc(StdCompiler *pComp) const
	{
		// Cast
		int_t iVal = int_t(rValue);
		pComp->Value(iVal);
		rValue = T(iVal);
	}
	// Operators for default checking/setting
	template <class D> inline bool operator == (const D &nValue) const { return rValue == nValue; }
	template <class D> inline StdIntAdapt &operator = (const D &nValue) { rValue = nValue; return *this; }
	ALLOW_TEMP_TO_REF(StdIntAdapt)

};
template <class T> inline StdIntAdapt<T> mkIntAdapt(T &rValue) { return StdIntAdapt<T>(rValue); }
template <class int_t, class T> StdIntAdapt<T, int_t> mkIntAdaptT(T &rValue) { return StdIntAdapt<T, int_t>(rValue); }

// * Casting Adaptor
// Does a reinterprete_cast
template <class T, class to_t>
struct StdCastAdapt
{
	T &rValue;
	explicit StdCastAdapt(T &rValue) : rValue(rValue) { }
	inline void CompileFunc(StdCompiler *pComp) const
	{
		// Cast
		assert(sizeof(to_t) == sizeof(T));
		to_t vVal = *reinterpret_cast<to_t *>(&rValue);
		pComp->Value(vVal);
		rValue = *reinterpret_cast<T *>(&vVal);
	}
	// Operators for default checking/setting
	template <class D> inline bool operator == (const D &nValue) const { return rValue == nValue; }
	template <class D> inline StdCastAdapt &operator = (const D &nValue) { rValue = nValue; return *this; }
	ALLOW_TEMP_TO_REF(StdCastAdapt)

};
template <class to_t, class T> StdCastAdapt<T, to_t> mkCastAdapt(T &rValue) { return StdCastAdapt<T, to_t>(rValue); }
template <class T> StdCastAdapt<T, int32_t> mkCastIntAdapt(T &rValue) { return StdCastAdapt<T, int32_t>(rValue); }

// Helper: Identity function class
template <class T>
struct _IdFuncClass
{
	T &operator ()(T &rValue) const { return rValue; }
};

// * Array Adaptor
// Stores a separated list
template <class T, class M = _IdFuncClass<T> >
struct StdArrayAdapt
{
	StdArrayAdapt(T *pArray, int iSize, M map = M())
			: pArray(pArray), iSize(iSize), map(map)
	{ }
	T *pArray; int iSize; M map;
	inline void CompileFunc(StdCompiler *pComp) const
	{
		for (int i = 0; i < iSize; i++)
		{
			if (i) pComp->Separator(StdCompiler::SEP_SEP);
			pComp->Value(map(pArray[i]));
		}
	}
	// Operators for default checking/setting
	inline bool operator == (const T &rDefault) const
	{
		for (int i = 0; i < iSize; i++)
			if (pArray[i] != rDefault)
				return false;
		return true;
	}
	inline StdArrayAdapt &operator = (const T &rDefault)
	{
		for (int i = 0; i < iSize; i++)
			pArray[i] = rDefault;
		return *this;
	}
	inline bool operator == (const T *pDefaults) const
	{
		for (int i = 0; i < iSize; i++)
			if (pArray[i] != pDefaults[i])
				return false;
		return true;
	}
	inline StdArrayAdapt &operator = (const T *pDefaults)
	{
		for (int i = 0; i < iSize; i++)
			pArray[i] = pDefaults[i];
		return *this;
	}
	ALLOW_TEMP_TO_REF(StdArrayAdapt)
};
template <class T>
inline StdArrayAdapt<T> mkArrayAdapt(T *pArray, int iSize) { return StdArrayAdapt<T>(pArray, iSize); }
#define mkArrayAdaptM(A) mkArrayAdapt(A, sizeof(A) / sizeof(*(A)))
template <class T, class M>
inline StdArrayAdapt<T, M> mkArrayAdaptMap(T *pArray, int iSize, M map) { return StdArrayAdapt<T, M>(pArray, iSize, map); }
#define mkArrayAdaptMapM(A, M) mkArrayAdaptMap(A, sizeof(A) / sizeof(*(A)), M)

// * Array Adaptor (defaulting)
// Stores a separated list, sets defaults if a value or separator is missing.
template <class T, class D, class M = _IdFuncClass<T> >
struct StdArrayDefaultAdapt
{
	StdArrayDefaultAdapt(T *pArray, size_t iSize, const D &rDefault, const M &map = M())
			: pArray(pArray), iSize(iSize), rDefault(rDefault), map(map)
	{ }
	T *pArray; size_t iSize; const D &rDefault; const M map;
	inline void CompileFunc(StdCompiler *pComp) const
	{
		size_t i, iWrite = iSize;
		bool fCompiler = pComp->isCompiler();
		// Decompiling: Omit defaults
		if (!fCompiler && pComp->hasNaming())
			while (iWrite > 0 && pArray[iWrite - 1] == rDefault)
				iWrite--;
		// Read/write values
		for (i = 0; i < iWrite; i++)
		{
			// Separator?
			if (i) if (!pComp->Separator(StdCompiler::SEP_SEP)) break;
			// Expect a value. Default if not found.
			pComp->Value(mkDefaultAdapt(map(pArray[i]), rDefault));
		}
		// Fill rest of array
		if (fCompiler)
			for (; i < iSize; i++)
				pArray[i] = rDefault;
	}
	// Additional defaulting (whole array)
	inline bool operator == (const T *pDefaults) const
	{
		for (size_t i = 0; i < iSize; i++)
			if (pArray[i] != pDefaults[i])
				return false;
		return true;
	}
	inline StdArrayDefaultAdapt &operator = (const T *pDefaults)
	{
		for (size_t i = 0; i < iSize; i++)
			pArray[i] = pDefaults[i];
		return *this;
	}
	ALLOW_TEMP_TO_REF(StdArrayDefaultAdapt)
};
template <class T, class D>
inline StdArrayDefaultAdapt<T, D> mkArrayAdapt(T *pArray, size_t iSize, const D &rDefault) { return StdArrayDefaultAdapt<T, D>(pArray, iSize, rDefault); }
#define mkArrayAdaptDM(A, D) mkArrayAdapt(A, sizeof(A) / sizeof(*(A)), D)
template <class T, class D, class M>
inline StdArrayDefaultAdapt<T, D, M> mkArrayAdaptMap(T *pArray, size_t iSize, const D &rDefault, M map) { return StdArrayDefaultAdapt<T, D, M>(pArray, iSize, rDefault, map); }
#define mkArrayAdaptMapDM(A, D, M) mkArrayAdaptMap(A, sizeof(A) / sizeof(*(A)), D, M)

// * Insertion Adaptor
// Compile a value before / after another
template <class T, class I>
struct StdInsertAdapt
{
	StdInsertAdapt(T &rObj, I &rIns, bool fBefore = true)
			: rObj(rObj), rIns(rIns), fBefore(fBefore)
	{ }
	T &rObj; I &rIns; bool fBefore;
	void CompileFunc(StdCompiler *pComp) const
	{
		if (fBefore) pComp->Value(rIns);
		pComp->Value(rObj);
		if (!fBefore) pComp->Value(rIns);
	}
	ALLOW_TEMP_TO_REF(StdInsertAdapt)
};
template <class T, class I>
inline StdInsertAdapt<T, I> mkInsertAdapt(T RREF rObj, I RREF rIns, bool fBefore = true) { return StdInsertAdapt<T,I>(rObj, rIns, fBefore); }

// * Parameter Adaptor
// Specify a second parameter for the CompileFunc
template <class T, class P>
struct StdParameterAdapt
{
	StdParameterAdapt(T &rObj, const P &rPar) : rObj(rObj), Par(rPar) { }
	T &rObj; const P Par;
	void CompileFunc(StdCompiler *pComp) const
	{
		rObj.CompileFunc(pComp, Par);
	}
	// Operators for default checking/setting
	template <class D> inline bool operator == (const D &nValue) const { return rObj == nValue; }
	template <class D> inline StdParameterAdapt &operator = (const D &nValue) { rObj = nValue; return *this; }

	// getting value
	inline T &GetObj() { return rObj; }

	ALLOW_TEMP_TO_REF(StdParameterAdapt)
};
template <class T, class P>
inline StdParameterAdapt<T, P> mkParAdapt(T RREF rObj, const P &rPar) { return StdParameterAdapt<T, P>(rObj, rPar); }

// * Parameter Adaptor 2
// Specify a second and a third parameter for the CompileFunc
template <class T, class P1, class P2>
struct StdParameter2Adapt
{
	StdParameter2Adapt(T &rObj, const P1 &rPar1, const P2 &rPar2) : rObj(rObj), rPar1(rPar1), rPar2(rPar2) { }
	T &rObj; const P1 &rPar1; const P2 &rPar2;
	void CompileFunc(StdCompiler *pComp) const
	{
		rObj.CompileFunc(pComp, rPar1, rPar2);
	}
	// Operators for default checking/setting
	template <class D> inline bool operator == (const D &nValue) const { return rObj == nValue; }
	template <class D> inline StdParameter2Adapt &operator = (const D &nValue) { rObj = nValue; return *this; }
	ALLOW_TEMP_TO_REF(StdParameter2Adapt)
};
template <class T, class P1, class P2>
inline StdParameter2Adapt<T, P1, P2> mkParAdapt(T &rObj, const P1 &rPar1, const P2 &rPar2) { return StdParameter2Adapt<T, P1, P2>(rObj, rPar1, rPar2); }

template <class T>
struct StdBasicPtrAdapt
{
	//typedef T TargetType;

	StdBasicPtrAdapt(T *&rpObj, bool fAllowNull = true, const char *szNaming = "Data")
		: rpObj(rpObj), fAllowNull(fAllowNull), szNaming(szNaming) {}
	T *&rpObj; bool fAllowNull; const char *szNaming;

	// Operators for default checking/setting
	inline bool operator == (const T &nValue) const { return rpObj && *rpObj == nValue; }
	inline StdBasicPtrAdapt &operator = (const T &nValue) { delete rpObj; rpObj = new T(nValue); return *this; }
	inline bool operator == (const T *pValue) const { return rpObj == pValue; }
	inline StdBasicPtrAdapt &operator = (const T *pValue) { delete rpObj; rpObj = pValue; return *this; }
};

// * Store pointer (contents)
// Defaults to null
template <class T>
struct StdPtrAdapt: StdBasicPtrAdapt<T>
{
	StdPtrAdapt(T *&rpObj, bool fAllowNull = true, const char *szNaming = "Data")
			: StdBasicPtrAdapt<T>(rpObj, fAllowNull, szNaming)
	{ }

	void CompileFunc(StdCompiler *pComp) const
	{
		StdPtrAdaptCompileFunc(pComp, *this);
	}

	// For use with StdParAdapt
	template<class P>
	void CompileFunc(StdCompiler *pComp, const P& p)
	{
		StdPtrAdaptCompileFunc(pComp, *this, p);
	}

	ALLOW_TEMP_TO_REF(StdPtrAdapt)
};

template <class T, class ContextT>
struct StdContextPtrAdapt: StdBasicPtrAdapt<T>
{
	StdContextPtrAdapt(T *&rpObj, const ContextT& rCtx, bool fAllowNull = true, const char *szNaming = "Data")
		: StdBasicPtrAdapt<T>(rpObj, fAllowNull, szNaming), pCtx(&rCtx)
	{ }

	const ContextT* pCtx;

	void CompileFunc(StdCompiler *pComp) const
	{
		StdPtrAdaptCompileFunc(pComp, *this);
	}

	// For use with StdParAdapt
	template<class P>
	void CompileFunc(StdCompiler *pComp, const P& p)
	{
		StdPtrAdaptCompileFunc(pComp, *this, p);
	}

	ALLOW_TEMP_TO_REF(StdContextPtrAdapt)
};

template <class T>
void StdPtrAdaptCompileFunc(StdCompiler* pComp, const T& adapt)
{
	bool fCompiler = pComp->isCompiler(),
		fNaming = pComp->hasNaming();
	// Compiling? Clear object before
	if(fCompiler) { delete adapt.rpObj; adapt.rpObj = NULL; }
	// Null checks - different with naming support.
	if(adapt.fAllowNull)
		if(fNaming)
		{
			// Null check: just omit when writing
			if(!fCompiler && !adapt.rpObj) return;
			// Set up naming
			if(!pComp->Name(adapt.szNaming)) { assert(fCompiler); pComp->NameEnd(); return; }
		}
		else
		{
			bool fNull = !! adapt.rpObj;
			pComp->Value(fNull);
			// Null? Nothing further to do
			if(fNull) return;
		}
	else if(!fCompiler)
		assert(adapt.rpObj);
	// Compile value
	if(fCompiler)
		StdPtrAdaptCompileNewFunc(adapt, pComp);
	else
		StdPtrAdaptDecompileNewFunc(adapt, pComp);

	// Close naming
	if(adapt.fAllowNull && fNaming) pComp->NameEnd();
}

// TODO: Avoid code duplication with the above function
template <class T, class P>
void StdPtrAdaptCompileFunc(StdCompiler* pComp, const T& adapt, const P& par)
{
	bool fCompiler = pComp->isCompiler(),
		fNaming = pComp->hasNaming();
	// Compiling? Clear object before
	if(fCompiler) { delete adapt.rpObj; adapt.rpObj = NULL; }
	// Null checks - different with naming support.
	if(adapt.fAllowNull)
		if(fNaming)
		{
			// Null check: just omit when writing
			if(!fCompiler && !adapt.rpObj) return;
			// Set up naming
			if(!pComp->Name(adapt.szNaming)) { assert(fCompiler); pComp->NameEnd(); return; }
		}
		else
		{
			bool fNull = !! adapt.rpObj;
			pComp->Value(fNull);
			// Null? Nothing further to do
			if(fNull) return;
		}
	else if(!fCompiler)
		assert(adapt.rpObj);
	// Compile value
	if(fCompiler)
		StdPtrAdaptCompileNewFunc(adapt, pComp, par);
	else
		StdPtrAdaptDecompileNewFunc(adapt, pComp, par);

	// Close naming
	if(adapt.fAllowNull && fNaming) pComp->NameEnd();
}

template <class T>
void StdPtrAdaptCompileNewFunc(const StdPtrAdapt<T>& adapt, StdCompiler* pComp) { CompileNewFunc(adapt.rpObj, pComp); }
template <class T, class ContextT>
void StdPtrAdaptCompileNewFunc(const StdContextPtrAdapt<T, ContextT>& adapt, StdCompiler* pComp) { CompileNewFuncCtx(adapt.rpObj, pComp, *adapt.pCtx); }
template <class T, class P>
void StdPtrAdaptCompileNewFunc(const StdPtrAdapt<T>& adapt, StdCompiler* pComp, const P& par) { CompileNewFunc(adapt.rpObj, pComp, par); }
template <class T, class ContextT, class P>
void StdPtrAdaptCompileNewFunc(const StdContextPtrAdapt<T, ContextT>& adapt, StdCompiler* pComp, const P& par) { CompileNewFuncCtx(adapt.rpObj, pComp, *adapt.pCtx, par); }


template <class T>
void StdPtrAdaptDecompileNewFunc(const StdPtrAdapt<T>& adapt, StdCompiler* pComp) { pComp->Value(mkDecompileAdapt(*adapt.rpObj)); }
template <class T, class ContextT>
void StdPtrAdaptDecompileNewFunc(const StdContextPtrAdapt<T, ContextT>& adapt, StdCompiler* pComp) { pComp->Value(mkDecompileAdapt(*adapt.rpObj)); }
template <class T, class P>
void StdPtrAdaptDecompileNewFunc(const StdPtrAdapt<T>& adapt, StdCompiler* pComp, const P& par) { pComp->Value(mkParAdapt(mkDecompileAdapt(*adapt.rpObj), par)); }
template <class T, class ContextT, class P>
void StdPtrAdaptDecompileNewFunc(const StdContextPtrAdapt<T, ContextT>& adapt, StdCompiler* pComp, const P& par) { pComp->Value(mkParAdapt(mkDecompileAdapt(*adapt.rpObj), par)); }

template <class T>
inline StdPtrAdapt<T> mkPtrAdapt(T *&rpObj, bool fAllowNull = true) { return StdPtrAdapt<T>(rpObj, fAllowNull); }
template <class T>
inline StdPtrAdapt<T> mkNamingPtrAdapt(T *&rpObj, const char *szNaming) { return StdPtrAdapt<T>(rpObj, true, szNaming); }
template <class T>
inline StdPtrAdapt<T> mkPtrAdaptNoNull(T *&rpObj) { return mkPtrAdapt<T>(rpObj, false); }

template <class T, class ContextT>
inline StdContextPtrAdapt<T, ContextT> mkContextPtrAdapt(T *&rpObj, const ContextT& ctx, bool fAllowNull = true) { return StdContextPtrAdapt<T, ContextT>(rpObj, ctx, fAllowNull); }
template <class T, class ContextT>
inline StdContextPtrAdapt<T, ContextT> mkNamingContextPtrAdapt(T *&rpObj, const ContextT& ctx, const char* szNaming) { return StdContextPtrAdapt<T, ContextT>(rpObj, ctx, true, szNaming); }
template <class T, class ContextT>
inline StdContextPtrAdapt<T, ContextT> mkContextPtrAdaptNoNull(T *&rpObj, const ContextT& ctx) { return mkContextPtrAdapt<T, ContextT>(rpObj, ctx, false); }

// * Adaptor for STL containers
// Writes a comma-separated list for compilers that support it. Otherwise, the size is calculated and safed.
// The defaulting uses the standard STL operators (full match)
template <class C>
struct StdSTLContainerAdapt
{
	StdSTLContainerAdapt(C &rStruct, StdCompiler::Sep eSep = StdCompiler::SEP_SEP)
			: rStruct(rStruct), eSep(eSep) { }
	C &rStruct; const StdCompiler::Sep eSep;
	inline void CompileFunc(StdCompiler *pComp) const
	{
		typedef typename C::value_type T;
		// Get compiler specs
		bool fCompiler = pComp->isCompiler();
		bool fNaming = pComp->hasNaming();
		// Decompiling?
		if (!fCompiler)
		{
			// Write size (binary only)
			if (!fNaming)
			{
				int32_t iSize = rStruct.size();
				pComp->Value(iSize);
			}
			// Write all entries
			for (typename C::const_iterator i = rStruct.begin(); i != rStruct.end(); ++i)
			{
				if (i != rStruct.begin() && eSep) pComp->Separator(eSep);
				pComp->Value(const_cast<T &>(*i));
			}
		}
		else
		{
			// Compiling: Empty previous
			rStruct.clear();
			// Read size (binary only)
			uint32_t iSize;
			if (!fNaming) pComp->Value(iSize);
			// Read new
			do
			{
				// No entries left to read?
				if (!fNaming && !iSize--)
					break;
				// Read entries
				try
				{
					T val;
					pComp->Value(val);
					rStruct.push_back(val);
				}
				catch (StdCompiler::NotFoundException *pEx)
				{
					// No value found: Stop reading loop
					delete pEx;
					break;
				}
			}
			while (!eSep || pComp->Separator(eSep));
		}
	}
	// Operators for default checking/setting
	inline bool operator == (const C &nValue) const { return rStruct == nValue; }
	inline StdSTLContainerAdapt &operator = (const C &nValue) { rStruct = nValue; return *this; }
	ALLOW_TEMP_TO_REF(StdSTLContainerAdapt)
};
template <class C>
inline StdSTLContainerAdapt<C> mkSTLContainerAdapt(C &rTarget, StdCompiler::Sep eSep = StdCompiler::SEP_SEP) { return StdSTLContainerAdapt<C>(rTarget, eSep); }

// Write an integer that is supposed to be small most of the time. The adaptor writes it in
// 7-bit-pieces, bit 8 being a continuation marker: If it's set, more data is following, if not,
// all following bits are 0.
// Code lengths for uint32_t:
// 0x00000000 (0)         - 0x0000007F (127)        : 1 byte
// 0x00000080 (128)       - 0x00003FFF (16383)      : 2 byte
// 0x00004000 (16384)     - 0x001FFFFF (2097151)    : 3 byte
// 0x00200000 (2097152)   - 0x0FFFFFFF (268435456)  : 4 byte
// 0x10000000 (268435456) - 0xFFFFFFFF (4294967295) : 5 byte
// So this sort of packing is always useful when the integer in question is almost impossible to
// grow bigger than 2,097,151.
template <class T>
struct StdIntPackAdapt
{
	StdIntPackAdapt(T &rVal) : rVal(rVal) { }
	T &rVal;

	inline T clearUpper(T x) const
	{
		const int CLEARBITS = 8 * sizeof(T) - 7;
		return (x << CLEARBITS) >> CLEARBITS;
	}

	void CompileFunc(StdCompiler *pComp) const
	{
		// simply write for textual compilers
		if (pComp->hasNaming())
		{
			pComp->Value(rVal);
			return;
		}
		T val; uint8_t tmp;
		// writing?
		if (!pComp->isCompiler())
		{
			val = rVal;
			for (;;)
			{
				tmp = uint8_t(clearUpper(val));
				// last byte?
				if (clearUpper(val) == val)
					break;
				// write byte
				tmp ^= 0x80; pComp->Value(tmp);
				// advance
				val >>= 7;
			}
			// write last byte
			pComp->Value(tmp);
		}
		// reading?
		else
		{
			// read first byte
			pComp->Value(tmp);
			val = clearUpper(T(tmp));
			// read remaining bytes
			int i = 7; T data = val;
			while ( uint8_t(data) != tmp )
			{
				// read byte
				pComp->Value(tmp);
				// add to value
				data = clearUpper(T(tmp));
				val = (data << i) | (val & ((1 << i) - 1));
				// next byte
				i+=7;
			}
			// write
			rVal = val;
		}
	}
	template <class D> inline bool operator == (const D &nValue) const { return rVal == nValue; }
	template <class D> inline StdIntPackAdapt &operator = (const D &nValue) { rVal = nValue; return *this; }
	ALLOW_TEMP_TO_REF(StdIntPackAdapt)
};
template <class T>
StdIntPackAdapt<T> mkIntPackAdapt(T &rVal) { return StdIntPackAdapt<T>(rVal); }

template <class T>
struct StdEnumEntry
{
	const char *Name;
	T Val;
};

// Enumeration: For text compilers, write a given name for a value.
// For everything else, just write an integer of given type.
template <class T, class int_t = int32_t>
struct StdEnumAdapt
{
	typedef StdEnumEntry<T> Entry;

	StdEnumAdapt(T &rVal, const Entry *pNames) : rVal(rVal), pNames(pNames) { assert(pNames); }
	T &rVal; const Entry *pNames;

	void CompileFunc(StdCompiler *pComp) const
	{
		// Write as int
		if (!pComp->isVerbose())
		{
			pComp->Value(mkIntAdaptT<int_t>(rVal));
			return;
		}
		// writing?
		if (!pComp->isCompiler())
		{
			// Find value
			const Entry *pName = pNames;
			for (; pName->Name; pName++)
				if (pName->Val == rVal)
				{
					// Put name
					pComp->String(const_cast<char **>(&pName->Name), StdCompiler::RCT_Idtf);
					break;
				}
			// No name found?
			if (!pName->Name)
				// Put value as integer
				pComp->Value(mkIntAdaptT<int_t>(rVal));
		}
		// reading?
		else
		{
			int_t val = 0;
#ifdef STDCOMPILER_EXCEPTION_WORKAROUND
			// Try to read as number
			if (!pComp->ValueSafe(val))
			{
				rVal = T(val);
#else
			// Try to read as number
			try
			{
				pComp->Value(val);
				rVal = T(val);
			}
			catch (StdCompiler::NotFoundException *pEx)
			{
				delete pEx;
#endif
				// Try to read as string
				StdStrBuf Name;
				pComp->Value(mkParAdapt(Name, StdCompiler::RCT_Idtf));
				// Search in name list
				const Entry *pName = pNames;
				for (; pName->Name; pName++)
					if (Name == pName->Name)
					{
						rVal = pName->Val;
						break;
					}
				// Not found? Warn
				if (!pName->Name)
					pComp->Warn("Unknown bit name: %s", Name.getData());
			}
		}
	}

	template <class D> inline bool operator == (const D &nValue) const { return rVal == nValue; }
	template <class D> inline StdEnumAdapt<T, int_t> &operator = (const D &nValue) { rVal = nValue; return *this; }

	ALLOW_TEMP_TO_REF(StdEnumAdapt)
};
template <class T, class int_t>
StdEnumAdapt<T, int_t> mkEnumAdapt(T &rVal, const StdEnumEntry<T> *pNames) { return StdEnumAdapt<T, int_t>(rVal, pNames); }
template <class int_t, class T>
StdEnumAdapt<T, int_t> mkEnumAdaptT(T &rVal, const StdEnumEntry<T> *pNames) { return StdEnumAdapt<T, int_t>(rVal, pNames); }

template <class T>
struct StdBitfieldEntry
{
	const char *Name;
	T Val;
};

// Convert a bitfield into/from something like "foo | bar | baz", where "foo", "bar" and "baz" are given constants.
template <class T>
struct StdBitfieldAdapt
{
	typedef StdBitfieldEntry<T> Entry;

	StdBitfieldAdapt(T &rVal, const Entry *pNames) : rVal(rVal), pNames(pNames) { assert(pNames); }
	T &rVal; const Entry *pNames;

	void CompileFunc(StdCompiler *pComp) const
	{
		// simply write for non-verbose compilers
		if (!pComp->isVerbose())
		{
			pComp->Value(rVal);
			return;
		}
		// writing?
		if (!pComp->isCompiler())
		{
			T val = rVal;
			// Write until value is comsumed
			bool fFirst = true;
			for (const Entry *pName = pNames; val && pName->Name; pName++)
				if ((pName->Val & val) == pName->Val)
				{
					// Put "|"
					if (!fFirst) pComp->Separator(StdCompiler::SEP_VLINE);
					// Put name
					pComp->String(const_cast<char **>(&pName->Name), StdCompiler::RCT_Idtf);
					fFirst = false;
					// Remove bits
					val &= ~pName->Val;
				}
			// Anything left is written as number
			if (val)
			{
				// Put "|"
				if (!fFirst) pComp->Separator(StdCompiler::SEP_VLINE);
				// Put value
				pComp->Value(val);
			}
		}
		// reading?
		else
		{
			T val = 0;
			// Read
			do
			{
#ifdef STDCOMPILER_EXCEPTION_WORKAROUND
				T tmp;
				// Try to read as number
				if (pComp->ValueSafe(tmp))
					val |= tmp;
				else
				{
#else
				// Try to read as number
				try
				{
					T tmp;
					pComp->Value(tmp);
					val |= tmp;
				}
				catch (StdCompiler::NotFoundException *pEx)
				{
					delete pEx;
#endif
					// Try to read as string
					StdStrBuf Name;
					pComp->Value(mkParAdapt(Name, StdCompiler::RCT_Idtf));
					// Search in name list
					const Entry *pName = pNames;
					for (; pName->Name; pName++)
						if (Name == pName->Name)
						{
							val |= pName->Val;
							break;
						}
					// Not found? Warn
					if (!pName->Name)
						pComp->Warn("Unknown bit name: %s", Name.getData());
				}
				// Expect separation
			} while (pComp->Separator(StdCompiler::SEP_VLINE));
			// Write value back
			rVal = val;
		}
	}

	template <class D> inline bool operator == (const D &nValue) const { return rVal == nValue; }
	template <class D> inline StdBitfieldAdapt<T> &operator = (const D &nValue) { rVal = nValue; return *this; }

	ALLOW_TEMP_TO_REF(StdBitfieldAdapt)
};
template <class T>
StdBitfieldAdapt<T> mkBitfieldAdapt(T &rVal, const StdBitfieldEntry<T> *pNames) { return StdBitfieldAdapt<T>(rVal, pNames); }

// * Name count adapter
// For compilers without name support, this just compiles the given value. Otherwise, the count
// of given namings is returned on compiling, and nothing is done for decompiling (The caller
// has to make sure that an appropriate number of namings will be created)
template <class int_t>
struct StdNamingCountAdapt
{
	int_t &iCount; const char *szName;
	StdNamingCountAdapt(int_t &iCount, const char *szName) : iCount(iCount), szName(szName) { }
	inline void CompileFunc(StdCompiler *pComp) const
	{
		if (pComp->hasNaming())
		{
			if (pComp->isCompiler())
				iCount = pComp->NameCount(szName);
		}
		else
			pComp->Value(mkIntPackAdapt(iCount));
	}
	ALLOW_TEMP_TO_REF(StdNamingCountAdapt)
};
template <class int_t>
inline StdNamingCountAdapt<int_t> mkNamingCountAdapt(int_t &iCount, const char *szName) { return StdNamingCountAdapt<int_t>(iCount, szName); }

// * Hex adapter
// Writes raw binary data in hexadecimal
class StdHexAdapt
{
	void *pData; size_t iSize;
public:
	StdHexAdapt(void *pData, size_t iSize) : pData(pData), iSize(iSize) { }
	inline void CompileFunc(StdCompiler *pComp) const
	{
		if (!pComp->isVerbose())
			pComp->Raw(pData, iSize);
		char szData[2+1]; bool fCompiler = pComp->isCompiler();
		for (size_t i = 0; i < iSize; i++)
		{
			uint8_t *pByte = reinterpret_cast<uint8_t *>(pData) + i;
			if (!fCompiler) sprintf(szData, "%02x", *pByte);
			pComp->String(szData, 2, StdCompiler::RCT_Idtf);
			if (fCompiler)
			{
				unsigned int b;
				if (sscanf(szData, "%02x", &b) != 1)
					pComp->excNotFound(i ? "hexadecimal data: bytes missing!" : "hexadecimal data missing!");
				*pByte = b;
			}
		}
	}
	ALLOW_TEMP_TO_REF(StdHexAdapt)
};
inline StdHexAdapt mkHexAdapt(void *pData, size_t iSize) { return StdHexAdapt(pData, iSize); }
template <class T>
inline StdHexAdapt mkHexAdapt(T &rData) { return StdHexAdapt(&rData, sizeof(rData)); }


#endif //STDADAPTORS_H
