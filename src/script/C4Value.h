/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001, 2004  Sven Eberhardt
 * Copyright (c) 2001-2002, 2006, 2010  Peter Wortmann
 * Copyright (c) 2006-2010  Günther Brammer
 * Copyright (c) 2009  Nicolas Hake
 * Copyright (c) 2010  Armin Burgmeier
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
#ifndef INC_C4Value
#define INC_C4Value

#include "C4Id.h"
#include "C4Real.h"
#include "C4StringTable.h"

// C4Value type
enum C4V_Type
{
	C4V_Any,          // nil
	C4V_Int,
	C4V_Float,
	C4V_Bool,
	C4V_PropList,
	C4V_C4Object,
	C4V_String,
	C4V_Array,

	C4V_Numeric,  // any numeric value; marker in operator table

	C4V_C4ObjectEnum, // enumerated object
	C4V_C4DefEnum     // enumerated object
};

#define C4V_Last (int) C4V_Array

const char* GetC4VName(const C4V_Type Type);
char GetC4VID(const C4V_Type Type);
C4V_Type GetC4VFromID(char C4VID);

union C4V_Data
{
	C4Real::StorageType Float;
	intptr_t Int;
	C4Object * Obj;
	C4PropList * PropList;
	C4String * Str;
	C4ValueArray * Array;
	// cheat a little - assume that all members have the same length
	operator void * () { return Obj; }
	operator const void * () const { return Obj; }
	C4V_Data &operator = (void *p) { Obj = reinterpret_cast<C4Object *>(p); return *this; }
};

// converter function, used in converter table
struct C4VCnvFn
{
	enum { CnvOK, CnvOK0, CnvError, CnvObject, CnvF2I, CnvI2F } Function;
	bool Warn;
};

template <typename T> struct C4ValueConv;

class C4Value
{
public:

	C4Value() : Type(C4V_Any), NextRef(NULL) { Data.Obj = 0; }

	C4Value(const C4Value &nValue) : Data(nValue.Data), Type(nValue.Type), NextRef(NULL)
	{ AddDataRef(); }

	explicit C4Value(bool data): Type(C4V_Bool), NextRef(NULL)
	{ Data.Int = data; }
	explicit C4Value(int32_t data): Type(C4V_Int), NextRef(NULL)
	{ Data.Int = data; }
	explicit C4Value(C4Real data): Type(C4V_Float), NextRef(NULL)
	{ Data.Float = data; AddDataRef(); }
	explicit C4Value(C4Object *pObj): Type(pObj ? C4V_C4Object : C4V_Any), NextRef(NULL)
	{ Data.Obj = pObj; AddDataRef(); }
	explicit C4Value(C4String *pStr): Type(pStr ? C4V_String : C4V_Any), NextRef(NULL)
	{ Data.Str = pStr; AddDataRef(); }
	explicit C4Value(C4ValueArray *pArray): Type(pArray ? C4V_Array : C4V_Any), NextRef(NULL)
	{ Data.Array = pArray; AddDataRef(); }
	explicit C4Value(C4PropList *p): Type(p ? C4V_PropList : C4V_Any), NextRef(NULL)
	{ Data.PropList = p; AddDataRef(); }

	C4Value& operator = (const C4Value& nValue) { Set(nValue); return *this; }

	~C4Value() { DelDataRef(Data, Type, NextRef); }

	// Checked getters
	int32_t getInt() const
	{
		if (!ConvertTo(C4V_Int))
			return 0;
		else if (Type == C4V_Float)
			return C4Real(Data.Float);
		else 
			return Data.Int;
	}
	bool getBool() const { return ConvertTo(C4V_Bool) ? !! Data : 0; }
	C4Real getFloat() const
	{
		if (!ConvertTo(C4V_Float))
			return C4Real(0);
		else if (Type == C4V_Float)
			return Data.Float;
		else
			return static_cast<int32_t>(Data.Int);
	}
	C4ID getC4ID() const;
	C4Object * getObj() const { return ConvertTo(C4V_C4Object) ? Data.Obj : NULL; }
	C4PropList * getPropList() const { return ConvertTo(C4V_PropList) ? Data.PropList : NULL; }
	C4String * getStr() const { return ConvertTo(C4V_String) ? Data.Str : NULL; }
	C4ValueArray * getArray() const { return ConvertTo(C4V_Array) ? Data.Array : NULL; }

	// Unchecked getters
	int32_t _getInt() const { return Data.Int; }
	C4Real _getFloat() const { return Data.Float; }
	bool _getBool() const { return !! Data.Int; }
	C4Object *_getObj() const { return Data.Obj; }
	C4String *_getStr() const { return Data.Str; }
	C4ValueArray *_getArray() const { return Data.Array; }
	C4PropList *_getPropList() const { return Data.PropList; }

	// Template versions
	template <typename T> inline T Get() { return C4ValueConv<T>::FromC4V(*this); }
	template <typename T> inline T _Get() { return C4ValueConv<T>::_FromC4V(*this); }

	bool operator ! () const { return !GetData(); }
	inline operator const void* () const { return GetData()?this:0; }  // To allow use of C4Value in conditions

	void Set(const C4Value &nValue) { if (this != &nValue) Set(nValue.Data, nValue.Type); }

	void SetInt(int i) { C4V_Data d; d.Int = i; Set(d, C4V_Int); }
	void SetFloat(C4Real f) { C4V_Data d; d.Float = f; Set(d, C4V_Float); }

	void SetBool(bool b) { C4V_Data d; d.Int = b; Set(d, C4V_Bool); }
	void SetObject(C4Object * Obj) { C4V_Data d; d.Obj = Obj; Set(d, C4V_C4Object); }
	void SetString(C4String * Str) { C4V_Data d; d.Str = Str; Set(d, C4V_String); }
	void SetArray(C4ValueArray * Array) { C4V_Data d; d.Array = Array; Set(d, C4V_Array); }
	void SetPropList(C4PropList * PropList) { C4V_Data d; d.PropList = PropList; Set(d, C4V_PropList); }
	void Set0();

	bool operator == (const C4Value& Value2) const;
	bool operator != (const C4Value& Value2) const;

#define C4VALUE_ARITHMETIC_OPERATOR(op) \
	/* combined arithmetic and assignment op */ \
	C4Value &operator op##= (const C4Value &rhs) \
	{ \
		/* Promote numeric values to float if any operand is float */ \
		if (rhs.GetType() == C4V_Float || GetType() == C4V_Float) \
		{ \
			C4Real lhsf = getFloat(); \
			C4Real rhsf = rhs.getFloat(); \
			SetFloat(lhsf op##= rhsf); \
		} \
		else \
		{ \
			Data.Int op##= rhs.Data.Int; \
			Type=C4V_Int; \
		} \
		return *this; \
	}
	C4VALUE_ARITHMETIC_OPERATOR(+)
	C4VALUE_ARITHMETIC_OPERATOR(-)
	C4VALUE_ARITHMETIC_OPERATOR(*)
	C4VALUE_ARITHMETIC_OPERATOR(/)
#undef C4VALUE_ARITHMETIC_OPERATOR

	C4Value &operator++()
	{
		switch (Type)
		{
		case C4V_Int:
		case C4V_Bool:
			++Data.Int; Type = C4V_Int; break;
		case C4V_Float:
			SetFloat(getFloat() + 1.0f); break;
		default:
			assert(!"Can't increment a non-numeric value");
		}
		return *this;
	}
	C4Value operator++(int)
	{
		C4Value nrv(*this);
		operator++();
		return nrv;
	}
	C4Value &operator--()
	{
		switch (Type)
		{
		case C4V_Int:
		case C4V_Bool:
			--Data.Int; Type = C4V_Int; break;
		case C4V_Float:
			SetFloat(getFloat() - 1.0f); break;
		default:
			assert(!"Can't decrement a non-numeric value");
		}
		return *this;
	}
	C4Value operator--(int)
	{
		C4Value nrv(*this);
		operator--();
		return nrv;
	}
	C4Value Pow(const C4Value &rhs) const
	{
		assert(ConvertTo(C4V_Numeric));
		assert(rhs.ConvertTo(C4V_Numeric));
		C4Value nrv;
		if (Type == C4V_Float || rhs.Type == C4V_Float)
			nrv.SetFloat(::Pow(getFloat(), rhs.getFloat()));
		else
			nrv.SetInt(::Pow(getInt(), rhs.getInt()));
		return nrv;
	}

	// getters
	C4V_Data GetData()    const { return Data; }
	C4V_Type GetType()    const { return Type; }

	const char *GetTypeName() const { return GetC4VName(GetType()); }
	const char *GetTypeInfo();

	void DenumeratePointer();

	StdStrBuf GetDataString() const;

	inline bool ConvertTo(C4V_Type vtToType) const // convert to dest type
	{
		if (vtToType == C4V_Numeric)
			return Type == C4V_Float || Type == C4V_Int || Type == C4V_Bool || Type == C4V_Any;
		assert(vtToType <= C4V_Last);
		if (vtToType > C4V_Last)
			return false;
		switch (C4ScriptCnvMap[Type][vtToType].Function)
		{
		case C4VCnvFn::CnvOK:
		case C4VCnvFn::CnvF2I:
		case C4VCnvFn::CnvI2F:
			return true;
		case C4VCnvFn::CnvOK0: return !*this;
		case C4VCnvFn::CnvError: return false;
		case C4VCnvFn::CnvObject: return FnCnvObject();
		}
		assert(!"C4Value::ConvertTo: Invalid conversion function specified");
		return false;
	}
	inline static bool WarnAboutConversion(C4V_Type vtFromType, C4V_Type vtToType)
	{
		assert(vtFromType <= C4V_Last);
		assert(vtToType <= C4V_Last);
		if (vtFromType > C4V_Last || vtToType > C4V_Last)
			return false;
		return C4ScriptCnvMap[vtFromType][vtToType].Warn;
	}

	// Compilation
	void CompileFunc(StdCompiler *pComp);

	static inline bool IsNullableType(C4V_Type Type)
	{ return Type == C4V_Int || Type == C4V_Float || Type == C4V_Bool; }

protected:
	// data
	C4V_Data Data;

	// proplist reference list
	C4Value * NextRef;

	// data type
	C4V_Type Type;

	C4Value(C4V_Data nData, C4V_Type nType): Data(nData), NextRef(NULL)
	{ Type = (nData || IsNullableType(nType) ? nType : C4V_Any); AddDataRef(); }

	void Set(C4V_Data nData, C4V_Type nType);

	void AddDataRef();
	void DelDataRef(C4V_Data Data, C4V_Type Type, C4Value *pNextRef);

	static C4VCnvFn C4ScriptCnvMap[C4V_Last+1][C4V_Last+1];
	bool FnCnvObject() const;

	friend class C4PropList;
	friend class C4AulDefFunc;
	friend class C4AulExec;
	friend C4Value C4VInt(int32_t iVal);
	friend C4Value C4VBool(bool fVal);
};

// converter
inline C4Value C4VInt(int32_t iVal) { C4V_Data d; d.Int = iVal; return C4Value(d, C4V_Int); }
inline C4Value C4VFloat(C4Real f) { return C4Value(f); }
inline C4Value C4VBool(bool fVal) { C4V_Data d; d.Int = fVal; return C4Value(d, C4V_Bool); }
C4Value C4VID(C4ID iVal);
inline C4Value C4VObj(C4Object *pObj) { return C4Value(pObj); }
inline C4Value C4VPropList(C4PropList * p) { return C4Value(p); }
inline C4Value C4VString(C4String *pStr) { return C4Value(pStr); }
inline C4Value C4VArray(C4ValueArray *pArray) { return C4Value(pArray); }

C4Value C4VString(StdStrBuf strString);
C4Value C4VString(const char *strString);

extern const C4Value C4VFalse, C4VTrue;

// type tag to allow other code to recognize C4VNull at compile time
class C4NullValue : public C4Value {};
extern const C4NullValue C4VNull;

/* These are by far the most often called C4Value functions.
 They also often do redundant checks the compiler should be able to optimize away
 in common situations because the Type of the new value is known. In any case,
 inlining them does speed up the script engine on at least one artificial benchmark. */

#include "C4ValueArray.h"
#include "C4PropList.h"

ALWAYS_INLINE void C4Value::AddDataRef()
{
	assert(Type != C4V_Any || !Data);
	switch (Type)
	{
	case C4V_Array: Data.Array->IncRef(); break;
	case C4V_String: Data.Str->IncRef(); break;
	case C4V_C4Object:
#ifdef _DEBUG
		// check if the object actually exists
		/*if (!::Objects.ObjectNumber(Data.Obj))
			{ LogF("Warning: using wild object ptr %p!", static_cast<void*>(Data.Obj)); }*/
#endif
	case C4V_PropList:
#ifdef _DEBUG
		if (!Data.PropList->Status)
			{ LogF("Warning: using ptr on deleted object %p (%s)!", static_cast<void*>(Data.PropList), Data.PropList->GetName()); }
#endif
		Data.PropList->AddRef(this);
		break;
	default: break;
	}
}

ALWAYS_INLINE void C4Value::DelDataRef(C4V_Data Data, C4V_Type Type, C4Value *pNextRef)
{
	// clean up
	switch (Type)
	{
	case C4V_C4Object: case C4V_PropList: Data.PropList->DelRef(this, pNextRef); break;
	case C4V_Array: Data.Array->DecRef(); break;
	case C4V_String: Data.Str->DecRef(); break;
	default: break;
	}
}

ALWAYS_INLINE void C4Value::Set(C4V_Data nData, C4V_Type nType)
{
	assert(nType != C4V_Any || !nData);
	// Do not add this to the same linked list twice.
	if (Data == nData && Type == nType) return;

	C4V_Data oData = Data;
	C4V_Type oType = Type;
	C4Value * oNextRef = NextRef;

	// change
	Data = nData;
	Type = nData || IsNullableType(nType) ? nType : C4V_Any;

	// hold new data & clean up old
	AddDataRef();
	DelDataRef(oData, oType, oNextRef);
}

ALWAYS_INLINE void C4Value::Set0()
{
	C4V_Data oData = Data;
	C4V_Type oType = Type;

	// change
	Data.Obj = 0;
	Type = C4V_Any;

	// clean up (save even if Data was 0 before)
	DelDataRef(oData, oType, NextRef);
}

#endif

