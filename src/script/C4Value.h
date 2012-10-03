/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2002, 2006, 2010  Peter Wortmann
 * Copyright (c) 2001, 2004  Sven Eberhardt
 * Copyright (c) 2006-2011  Günther Brammer
 * Copyright (c) 2009  Nicolas Hake
 * Copyright (c) 2010-2011  Armin Burgmeier
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
	C4V_Nil,
	C4V_Int,
	C4V_Float,
	C4V_Bool,
	
	C4V_PropList,
	C4V_String,
	C4V_Array,
	C4V_Function,
	
	C4V_Enum, // enumerated array or proplist
	C4V_C4ObjectEnum, // enumerated object
	
	// for typechecks
	C4V_Any,
	C4V_Numeric,  // any numeric value
	C4V_Object,
	C4V_Def,
	C4V_Effect
};

const char* GetC4VName(const C4V_Type Type);

union C4V_Data
{
	C4Real::StorageType Float; // beware! this is a 4 byte value where all other values are 8 byte on 64bit systems. Take care when setting so bool conversion stays valid
	intptr_t Int;
	void * Ptr;
	C4PropList * PropList;
	C4String * Str;
	C4ValueArray * Array;
	C4AulFunc * Fn;
	// cheat a little - assume that all members have the same length
	operator void * () { return Ptr; }
	operator const void * () const { return Ptr; }
	C4V_Data &operator = (void *p) { assert(!p); Ptr = p; return *this; }
};

class C4Value
{
public:

	C4Value() : NextRef(NULL), Type(C4V_Nil) { Data = 0; }

	C4Value(const C4Value &nValue) : Data(nValue.Data), NextRef(NULL), Type(nValue.Type)
	{ AddDataRef(); }

	explicit C4Value(bool data): NextRef(NULL), Type(C4V_Bool)
	{ Data.Int = data; }
	explicit C4Value(int32_t data):  NextRef(NULL), Type(C4V_Int)
	{ Data.Int = data; }
	explicit C4Value(C4Real data): NextRef(NULL), Type(C4V_Float)
	{ Data.Int = 0; Data.Float = data; AddDataRef(); }
	explicit C4Value(C4Object *pObj);
	explicit C4Value(C4String *pStr): NextRef(NULL), Type(pStr ? C4V_String : C4V_Nil)
	{ Data.Str = pStr; AddDataRef(); }
	explicit C4Value(C4ValueArray *pArray): NextRef(NULL), Type(pArray ? C4V_Array : C4V_Nil)
	{ Data.Array = pArray; AddDataRef(); }
	explicit C4Value(C4AulFunc * pFn): NextRef(NULL), Type(pFn ? C4V_Function : C4V_Nil)
	{ Data.Fn = pFn; AddDataRef(); }
	explicit C4Value(C4PropList *p): NextRef(NULL), Type(p ? C4V_PropList : C4V_Nil)
	{ Data.PropList = p; AddDataRef(); }

	C4Value& operator = (const C4Value& nValue) { Set(nValue); return *this; }

	~C4Value() { DelDataRef(Data, Type, NextRef); }

	// Checked getters
	int32_t getInt() const
	{
		if (!CheckConversion(C4V_Int))
			return 0;
		if (Type == C4V_Float)
			return C4Real(Data.Float);
		return Data.Int;
	}
	C4Real getFloat() const
	{
		if (!CheckConversion(C4V_Float))
			return C4Real(0);
		if (Type != C4V_Float)
			return C4Real(_getInt());
		return C4Real(Data.Float);
	}
	bool getBool() const { return CheckConversion(C4V_Bool) ? !! Data : 0; }
	C4ID getC4ID() const;
	C4Object * getObj() const;
	C4PropList * getPropList() const { return CheckConversion(C4V_PropList) ? Data.PropList : NULL; }
	C4String * getStr() const { return CheckConversion(C4V_String) ? Data.Str : NULL; }
	C4ValueArray * getArray() const { return CheckConversion(C4V_Array) ? Data.Array : NULL; }
	C4AulFunc * getFunction() const { return CheckConversion(C4V_Function) ? Data.Fn : NULL; }

	// Unchecked getters
	int32_t _getInt() const { return Data.Int; }
	C4Real _getFloat() const { return Data.Float; }
	bool _getBool() const { return !! Data.Int; }
	C4Object *_getObj() const;
	C4String *_getStr() const { return Data.Str; }
	C4ValueArray *_getArray() const { return Data.Array; }
	C4AulFunc *_getFunction() const { return Data.Fn; }
	C4PropList *_getPropList() const { return Data.PropList; }

	// Template versions

	bool operator ! () const { return !GetData(); }
	inline operator const void* () const { return GetData() ? this : 0; }  // To allow use of C4Value in conditions

	void Set(const C4Value &nValue) { Set(nValue.Data, nValue.Type); }

	void SetInt(int32_t i) { C4V_Data d; d.Int = i; Set(d, C4V_Int); }
	void SetFloat(C4Real f)
	{
		C4V_Data d;
		d.Float = f;
		d.Int = 0; // make sure the upper bits are 0ed on 64 bit systems
		d.Float = f;
		if(d.Int == (int)0x80000000) d.Int = 0; // Don't store -0.0
		Set(d, C4V_Float);
	}
	void SetBool(bool b) { C4V_Data d; d.Int = b; Set(d, C4V_Bool); }
	void SetString(C4String * Str) { C4V_Data d; d.Str = Str; Set(d, C4V_String); }
	void SetArray(C4ValueArray * Array) { C4V_Data d; d.Array = Array; Set(d, C4V_Array); }
	void SetFunction(C4AulFunc * Fn) { C4V_Data d; d.Fn = Fn; Set(d, C4V_Function); }
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
			Data.Int = _getInt() op rhs._getInt(); \
			Type=C4V_Int; \
		} \
		return *this; \
	}
	C4VALUE_ARITHMETIC_OPERATOR(+)
	C4VALUE_ARITHMETIC_OPERATOR(-)
	C4VALUE_ARITHMETIC_OPERATOR(*)
	C4VALUE_ARITHMETIC_OPERATOR(/)
	C4VALUE_ARITHMETIC_OPERATOR(%)
#undef C4VALUE_ARITHMETIC_OPERATOR
#define C4VALUE_COMPARISON_OPERATOR(op) \
	bool operator op (const C4Value & rhs) const \
	{ \
		if(rhs.GetType() != C4V_Float && GetType() != C4V_Float) \
			return _getInt() op rhs._getInt(); \
		else \
			return getFloat() op rhs.getFloat(); \
	}
	C4VALUE_COMPARISON_OPERATOR(<)
	C4VALUE_COMPARISON_OPERATOR(<=)
	C4VALUE_COMPARISON_OPERATOR(>)
	C4VALUE_COMPARISON_OPERATOR(>=)
#undef C4VALUE_COMPARISON_OPERATOR

	C4Value &operator++()
	{
		switch (Type)
		{
		case C4V_Nil:
		case C4V_Bool:
			Type = C4V_Int; //nobreak
		case C4V_Int:
			Data.Int = _getInt() + 1; break;
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
		case C4V_Nil:
		case C4V_Bool:
			Type = C4V_Int; //nobreak
		case C4V_Int:
			Data.Int = _getInt() - 1; break;
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
	C4Value operator-() const
	{
		C4Value nrv;
		switch (Type)
		{
		case C4V_Nil:
		case C4V_Bool:
		case C4V_Int:
			nrv.Data.Int = -Data.Int;
			nrv.Type = C4V_Int;
			break;
		case C4V_Float:
			nrv.Data.Int = Data.Int ^ 0x80000000;
			if(nrv.Data.Int == (int)0x80000000) nrv.Data.Int = 0;
			nrv.Type = C4V_Float;
			break;
		default:
			assert(!"Can't negate a non-numeric value");
			return *this;
		}
		return nrv;
	}
	C4Value Pow(const C4Value &rhs) const
	{
		assert(CheckConversion(C4V_Numeric));
		assert(CheckConversion(C4V_Numeric));
		C4Value nrv;
		if (Type == C4V_Float || rhs.Type == C4V_Float)
			nrv.SetFloat(::Pow(getFloat(), rhs.getFloat()));
		else
			nrv.SetInt(::Pow(getInt(), rhs.getInt()));
		return nrv;
	}
	inline bool isWeakNil() const {
		return Type == C4V_Nil || ((Type == C4V_Int || Type == C4V_Float || Type == C4V_Bool) && !*this);
	}

	// getters
	C4V_Data GetData()    const { return Data; }
	C4V_Type GetType()    const { return Type; }

	const char *GetTypeName() const { return GetC4VName(GetType()); }

	void Denumerate(C4ValueNumbers *);

	StdStrBuf GetDataString(int depth = 1) const;

	ALWAYS_INLINE bool CheckParConversion(C4V_Type vtToType) const // convert to dest type
	{
		switch (vtToType)
		{
		case C4V_Nil:      return isWeakNil();
		case C4V_Float: case C4V_Numeric:
		case C4V_Int:      return Type == C4V_Int || Type == C4V_Float || Type == C4V_Nil || Type == C4V_Bool;
		case C4V_Bool:     return true;
		case C4V_PropList: return Type == C4V_PropList || isWeakNil();
		case C4V_String:   return Type == C4V_String || isWeakNil();
		case C4V_Array:    return Type == C4V_Array || isWeakNil();
		case C4V_Function: return Type == C4V_Function || isWeakNil();
		case C4V_Any:      return true;
		case C4V_Object:   return (Type == C4V_PropList && FnCnvObject()) || isWeakNil();
		case C4V_Def:      return (Type == C4V_PropList && FnCnvDef()) || isWeakNil();
		case C4V_Effect:   return (Type == C4V_PropList && FnCnvEffect()) || isWeakNil();
		default: assert(!"C4Value::CheckParConversion: impossible conversion target"); return false;
		}
	}
	ALWAYS_INLINE bool CheckConversion(C4V_Type vtToType) const // convert to dest type
	{
		switch (vtToType)
		{
		case C4V_Nil:      return Type == C4V_Nil;
		case C4V_Float: case C4V_Numeric:
		case C4V_Int:      return Type == C4V_Nil || Type == C4V_Int || Type == C4V_Float || Type == C4V_Bool;
		case C4V_Bool:     return true;
		case C4V_PropList: return Type == C4V_PropList;
		case C4V_String:   return Type == C4V_String;
		case C4V_Array:    return Type == C4V_Array;
		case C4V_Function: return Type == C4V_Function;
		case C4V_Any:      return true;
		case C4V_Object:   return Type == C4V_PropList && FnCnvObject();
		case C4V_Def:      return Type == C4V_PropList && FnCnvDef();
		case C4V_Effect:   return Type == C4V_PropList && FnCnvEffect();
		default: assert(!"C4Value::CheckConversion: impossible conversion target"); return false;
		}
	}
	static bool WarnAboutConversion(C4V_Type Type, C4V_Type vtToType);
	inline void NumericConversion(C4V_Type vToType)
	{
		if(Type == vToType) return;
		if(Type != C4V_Int && Type != C4V_Float && Type != C4V_Bool) return;
		switch(vToType)
		{
		case C4V_Bool: SetBool(getBool()); return;
		case C4V_Int: SetInt(getInt()); return;
		case C4V_Float: SetFloat(getFloat()); return;
		default: return;
		}
	}

	// Compilation
	void CompileFunc(StdCompiler *pComp, C4ValueNumbers *);

	static inline bool IsNullableType(C4V_Type Type)
	{ return Type == C4V_Int || Type == C4V_Float || Type == C4V_Bool; }

protected:
	// data
	C4V_Data Data;

	// proplist reference list
	C4Value * NextRef;

	// data type
	C4V_Type Type;


	void Set(C4V_Data nData, C4V_Type nType);

	void AddDataRef();
	void DelDataRef(C4V_Data Data, C4V_Type Type, C4Value *pNextRef);

	bool FnCnvObject() const;
	bool FnCnvDef() const;
	bool FnCnvEffect() const;
	void LogDeletedObjectWarning(C4PropList *);

	friend class C4PropList;
};

// converter
inline C4Value C4VInt(int32_t i) { return C4Value(i); }
inline C4Value C4VBool(bool b) { return C4Value(b); }
inline C4Value C4VFloat(C4Real f) { return C4Value(f); }
C4Value C4VObj(C4Object *pObj);
inline C4Value C4VPropList(C4PropList * p) { return C4Value(p); }
inline C4Value C4VString(C4String *pStr) { return C4Value(pStr); }
inline C4Value C4VArray(C4ValueArray *pArray) { return C4Value(pArray); }
inline C4Value C4VFunction(C4AulFunc * pFn) { return C4Value(pFn); }

C4Value C4VString(StdStrBuf strString);
C4Value C4VString(const char *strString);

#define C4VFalse C4VBool(false)
#define C4VTrue C4VBool(true)

extern const C4Value C4VNull;

// C4Values can contain data structures that have to maintain their
// identity across a save/load. During serialization, these get a number
class C4ValueNumbers
{
public:
	C4ValueNumbers() {}
	uint32_t GetNumberForValue(C4Value * v);
	const C4Value & GetValue(uint32_t);
	void Denumerate();
	void CompileFunc(StdCompiler *);
	void CompileValue(StdCompiler *, C4Value *);
private:
	std::list<C4Value *> ValuesToSave;
	std::vector<C4Value> LoadedValues;
	std::map<void *, uint32_t> ValueNumbers;
};

/* These are by far the most often called C4Value functions.
 They also often do redundant checks the compiler should be able to optimize away
 in common situations because the Type of the new value is known. In any case,
 inlining them does speed up the script engine on at least one artificial benchmark. */

#include "C4ValueArray.h"
#include "C4PropList.h"
#include "C4AulFunc.h"

ALWAYS_INLINE void C4Value::AddDataRef()
{
	assert(Type < C4V_Any);
	assert(Type != C4V_Nil || !Data);
	switch (Type)
	{
	case C4V_PropList:
#ifdef _DEBUG
		assert(C4PropList::PropLists.Has(Data.PropList));
		if (!Data.PropList->Status)
		{
			LogDeletedObjectWarning(Data.PropList);
		}
#endif
		Data.PropList->AddRef(this);
		break;
	case C4V_String: Data.Str->IncRef(); break;
	case C4V_Array: Data.Array->IncRef(); break;
	case C4V_Function: Data.Fn->IncRef(); break;
	default: break;
	}
}

ALWAYS_INLINE void C4Value::DelDataRef(C4V_Data Data, C4V_Type Type, C4Value *pNextRef)
{
	assert(Type < C4V_Any);
	assert(Type != C4V_Nil || !Data);
	// clean up
	switch (Type)
	{
	case C4V_PropList: Data.PropList->DelRef(this, pNextRef); break;
	case C4V_String: Data.Str->DecRef(); break;
	case C4V_Array: Data.Array->DecRef(); break;
	case C4V_Function: Data.Fn->DecRef(); break;
	default: break;
	}
}

ALWAYS_INLINE void C4Value::Set(C4V_Data nData, C4V_Type nType)
{
	// Do not add this to the same linked list twice.
	if (Data == nData && Type == nType) return;

	C4V_Data oData = Data;
	C4V_Type oType = Type;
	C4Value * oNextRef = NextRef;

	// change
	Data = nData;
	Type = nData || IsNullableType(nType) ? nType : C4V_Nil;

	// hold new data & clean up old
	AddDataRef();
	DelDataRef(oData, oType, oNextRef);
}

ALWAYS_INLINE void C4Value::Set0()
{
	C4V_Data oData = Data;
	C4V_Type oType = Type;

	// change
	Data = 0;
	Type = C4V_Nil;

	// clean up (save even if Data was 0 before)
	DelDataRef(oData, oType, NextRef);
}

#endif

