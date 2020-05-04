/*
 * OpenClonk, http://www.openclonk.org
 *
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
#ifndef INC_C4Value
#define INC_C4Value

#include "object/C4ObjectPtr.h"
#include "script/C4StringTable.h"

// C4Value type
enum C4V_Type
{
	C4V_Nil=0,
	C4V_Int=1,
	C4V_Bool=2,
	C4V_PropList=3,
	C4V_String=4,
	C4V_Array=5,
	C4V_Function=6,

	C4V_Enum=8, // enumerated array or proplist
	C4V_C4ObjectEnum=9, // enumerated object

	// for typechecks
	C4V_Any,
	C4V_Object,
	C4V_Def,
	C4V_Effect,
};
// last C4V_Type that doesn't vanish in Denumerate
#define C4V_Last ((int) C4V_Array)
// a C4V_Type >= C4V_FirstPointer and <= C4V_Last is a pointer
#define C4V_FirstPointer C4V_PropList

const char* GetC4VName(const C4V_Type Type);
template<typename T> class Nillable;

union C4V_Data
{
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

class C4JSONSerializationError : public std::exception
{
	std::string msg;
public:
	C4JSONSerializationError(const std::string& msg) : msg(msg) {}
	const char* what() const noexcept override { return msg.c_str(); }
};

class C4Value
{
public:

	C4Value() { Data = nullptr; }

	C4Value(const C4Value &nValue) : Data(nValue.Data), Type(nValue.Type)
	{ AddDataRef(); }
	C4Value(C4Value && nValue) noexcept;

	explicit C4Value(bool data): Type(C4V_Bool)
	{ Data.Int = data; }
	explicit C4Value(int data):  Type(C4V_Int)
	{ Data.Int = data; }
	explicit C4Value(long data): Type(C4V_Int)
	{ Data.Int = int32_t(data); }
	explicit C4Value(C4PropListStatic *p);
	explicit C4Value(C4Def *p);
	explicit C4Value(C4Object *pObj);
	explicit C4Value(C4Effect *p);
	explicit C4Value(C4String *pStr): Type(pStr ? C4V_String : C4V_Nil)
	{ Data.Str = pStr; AddDataRef(); }
	explicit C4Value(const char * s): Type(s ? C4V_String : C4V_Nil)
	{ Data.Str = s ? ::Strings.RegString(s) : nullptr; AddDataRef(); }
	explicit C4Value(const StdStrBuf & s): Type(s.isNull() ? C4V_Nil : C4V_String)
	{ Data.Str = s.isNull() ? nullptr: ::Strings.RegString(s); AddDataRef(); }
	explicit C4Value(C4ValueArray *pArray): Type(pArray ? C4V_Array : C4V_Nil)
	{ Data.Array = pArray; AddDataRef(); }
	explicit C4Value(C4AulFunc * pFn): Type(pFn ? C4V_Function : C4V_Nil)
	{ Data.Fn = pFn; AddDataRef(); }
	explicit C4Value(C4PropList *p): Type(p ? C4V_PropList : C4V_Nil)
	{ Data.PropList = p; AddDataRef(); }
	C4Value(C4ObjectPtr p): C4Value(p.operator C4Object *()) {}
	template<typename T> C4Value(Nillable<T> v): C4Value(v.IsNil() ? C4Value() : C4Value(v.operator T())) {}

	C4Value& operator = (const C4Value& nValue) { Set(nValue); return *this; }

	~C4Value() { DelDataRef(Data, Type); }

	// Checked getters
	int32_t getInt() const { return CheckConversion(C4V_Int) ? Data.Int : 0; }
	bool getBool() const { return CheckConversion(C4V_Bool) ? !! Data : false; }
	C4Object * getObj() const;
	C4Def * getDef() const;
	C4PropList * getPropList() const { return CheckConversion(C4V_PropList) ? Data.PropList : nullptr; }
	C4String * getStr() const { return CheckConversion(C4V_String) ? Data.Str : nullptr; }
	C4ValueArray * getArray() const { return CheckConversion(C4V_Array) ? Data.Array : nullptr; }
	C4AulFunc * getFunction() const { return CheckConversion(C4V_Function) ? Data.Fn : nullptr; }

	// Unchecked getters
	int32_t _getInt() const { return Data.Int; }
	bool _getBool() const { return !! Data.Int; }
	C4Object *_getObj() const;
	C4Def *_getDef() const;
	C4String *_getStr() const { return Data.Str; }
	C4ValueArray *_getArray() const { return Data.Array; }
	C4AulFunc *_getFunction() const { return Data.Fn; }
	C4PropList *_getPropList() const { return Data.PropList; }

	bool operator ! () const { return !GetData(); }
	inline operator const void* () const { return GetData() ? this : nullptr; }  // To allow use of C4Value in conditions

	void Set(const C4Value &nValue) { Set(nValue.Data, nValue.Type); }

	void SetInt(int32_t i) { C4V_Data d; d.Int = i; Set(d, C4V_Int); }
	void SetBool(bool b) { C4V_Data d; d.Int = b; Set(d, C4V_Bool); }
	void SetString(C4String * Str) { C4V_Data d; d.Str = Str; Set(d, C4V_String); }
	void SetArray(C4ValueArray * Array) { C4V_Data d; d.Array = Array; Set(d, C4V_Array); }
	void SetFunction(C4AulFunc * Fn) { C4V_Data d; d.Fn = Fn; Set(d, C4V_Function); }
	void SetPropList(C4PropList * PropList) { C4V_Data d; d.PropList = PropList; Set(d, C4V_PropList); }
	void SetObjectEnum(int i) { C4V_Data d; d.Int = i; Set(d, C4V_C4ObjectEnum); }
	void Set0();

	bool operator == (const C4Value& Value2) const;
	bool operator != (const C4Value& Value2) const;

	// Identical comparison
	bool IsIdenticalTo(const C4Value &cmp) const { return GetType()==cmp.GetType() && GetData()==cmp.GetData(); }

	// Change and set Type to int in case it was nil or bool before
	// Use with care: These don't handle int32_t overflow
	C4Value & operator += (int32_t by) { Data.Int += by; Type=C4V_Int; return *this; }
	C4Value & operator ++ ()           { Data.Int++;     Type=C4V_Int; return *this; }
	C4Value operator ++ (int)          { C4Value old = *this; ++(*this); return old; }
	C4Value & operator -- ()           { Data.Int--;     Type=C4V_Int; return *this; }
	C4Value operator -- (int)          { C4Value old = *this; --(*this); return old; }

	// getters
	C4V_Data GetData()    const { return Data; }
	C4V_Type GetType()    const { return Type; }
	C4V_Type GetTypeEx()  const; // Return type including types derived from prop list types (such as C4V_Def)

	const char *GetTypeName() const { return GetC4VName(GetType()); }

	void Denumerate(C4ValueNumbers *);

	StdStrBuf GetDataString(int depth = 10, const class C4PropListStatic *ignore_reference_parent = nullptr) const;
	StdStrBuf ToJSON(int depth = 10, const class C4PropListStatic *ignore_reference_parent = nullptr) const;

	ALWAYS_INLINE bool CheckParConversion(C4V_Type vtToType) const // convert to dest type
	{
		switch (vtToType)
		{
		case C4V_Nil:      return Type == C4V_Nil || (Type == C4V_Int && !*this);
		case C4V_Int:      return Type == C4V_Int || Type == C4V_Nil || Type == C4V_Bool;
		case C4V_Bool:     return true;
		case C4V_PropList: return Type == C4V_PropList || Type == C4V_Nil || (Type == C4V_Int && !*this);
		case C4V_String:   return Type == C4V_String || Type == C4V_Nil || (Type == C4V_Int && !*this);
		case C4V_Array:    return Type == C4V_Array || Type == C4V_Nil || (Type == C4V_Int && !*this);
		case C4V_Function: return Type == C4V_Function || Type == C4V_Nil || (Type == C4V_Int && !*this);
		case C4V_Any:      return true;
		case C4V_Object:   return (Type == C4V_PropList && FnCnvObject()) || Type == C4V_Nil || (Type == C4V_Int && !*this);
		case C4V_Def:      return (Type == C4V_PropList && FnCnvDef()) || Type == C4V_Nil || (Type == C4V_Int && !*this);
		case C4V_Effect:   return (Type == C4V_PropList && FnCnvEffect()) || Type == C4V_Nil || (Type == C4V_Int && !*this);
		default: assert(!"C4Value::CheckParConversion: impossible conversion target"); return false;
		}
	}
	ALWAYS_INLINE bool CheckConversion(C4V_Type vtToType) const // convert to dest type
	{
		switch (vtToType)
		{
		case C4V_Nil:      return Type == C4V_Nil;
		case C4V_Int:      return Type == C4V_Nil || Type == C4V_Int || Type == C4V_Bool;
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

	// Compilation
	void CompileFunc(StdCompiler *pComp, C4ValueNumbers *);

	static inline constexpr bool IsNullableType(C4V_Type Type)
	{ return Type == C4V_Int || Type == C4V_Bool; }

private:
	// data
	C4V_Data Data;

	// data type
	C4V_Type Type{C4V_Nil};

	void Set(C4V_Data nData, C4V_Type nType);

	void AddDataRef();
	void DelDataRef(C4V_Data Data, C4V_Type Type);

	bool FnCnvObject() const;
	bool FnCnvDef() const;
	bool FnCnvEffect() const;
	void LogDeletedObjectWarning(C4PropList *);

	// Prevent unintended type conversions
	template<typename T> explicit C4Value(T);

	friend class C4PropList;
};

// converter
inline C4Value C4VInt(int32_t i) { return C4Value(i); }
inline C4Value C4VBool(bool b) { return C4Value(b); }
C4Value C4VObj(C4Object *pObj);
inline C4Value C4VPropList(C4PropList * p) { return C4Value(p); }
inline C4Value C4VString(C4String *pStr) { return C4Value(pStr); }
inline C4Value C4VString(StdStrBuf strString) { return C4Value(strString); }
inline C4Value C4VString(const char *strString) { return C4Value(strString); }
inline C4Value C4VArray(C4ValueArray *pArray) { return C4Value(pArray); }
inline C4Value C4VFunction(C4AulFunc * pFn) { return C4Value(pFn); }

extern const C4Value C4VNull;

// C4Values can contain data structures that have to maintain their
// identity across a save/load. During serialization, these get a number
class C4ValueNumbers
{
public:
	C4ValueNumbers() = default;
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

#include "script/C4ValueArray.h"
#include "script/C4PropList.h"
#include "script/C4AulFunc.h"

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

ALWAYS_INLINE void C4Value::DelDataRef(C4V_Data Data, C4V_Type Type)
{
	assert(Type < C4V_Any);
	assert(Type != C4V_Nil || !Data);
	// clean up
	switch (Type)
	{
	case C4V_PropList: Data.PropList->DelRef(this); break;
	case C4V_String: Data.Str->DecRef(); break;
	case C4V_Array: Data.Array->DecRef(); break;
	case C4V_Function: Data.Fn->DecRef(); break;
	default: break;
	}
}

ALWAYS_INLINE void C4Value::Set(C4V_Data nData, C4V_Type nType)
{
	// Do not add this to the same linked list twice.
	if (Data == nData && Type >= C4V_FirstPointer) return;

	C4V_Data oData = Data;
	C4V_Type oType = Type;

	// change
	Data = nData;
	Type = nData || IsNullableType(nType) ? nType : C4V_Nil;

	// hold new data & clean up old
	AddDataRef();
	DelDataRef(oData, oType);
}

ALWAYS_INLINE void C4Value::Set0()
{
	C4V_Data oData = Data;
	C4V_Type oType = Type;

	// change
	Data = nullptr;
	Type = C4V_Nil;

	// clean up (save even if Data was 0 before)
	DelDataRef(oData, oType);
}

ALWAYS_INLINE C4Value::C4Value(C4Value && nValue) noexcept:
		Data(nValue.Data), Type(nValue.Type)
{
	if (Type == C4V_PropList)
	{
		Data.PropList->AddRef(this);
		Data.PropList->DelRef(&nValue);
	}
	nValue.Type = C4V_Nil; nValue.Data = nullptr;
}

#endif

