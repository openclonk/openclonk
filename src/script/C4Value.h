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
#include "C4StringTable.h"

// C4Value type
enum C4V_Type
{
	C4V_Nil=0,         // nil
	C4V_Int=1,
	C4V_Bool=2,
	C4V_PropList=3,
	C4V_C4Object=4,
	C4V_String=5,
	C4V_Array=6,

	C4V_Enum=8, // enumerated array or proplist
	C4V_C4ObjectEnum=9, // enumerated object
	C4V_C4DefEnum=10, // enumerated definition
	C4V_Any=11, // for typechecks
};
// last C4V_Type that doesn't vanish in Denumerate
#define C4V_Last ((int) C4V_Array)
// a C4V_Type >= C4V_FirstPointer and <= C4V_Last is a pointer
#define C4V_FirstPointer C4V_PropList

const char* GetC4VName(const C4V_Type Type);

union C4V_Data
{
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

class C4Value
{
public:

	C4Value() : NextRef(NULL), Type(C4V_Nil) { Data.Obj = 0; }

	C4Value(const C4Value &nValue) : Data(nValue.Data), NextRef(NULL), Type(nValue.Type)
	{ AddDataRef(); }

	explicit C4Value(bool data): NextRef(NULL), Type(C4V_Bool)
	{ Data.Int = data; }
	explicit C4Value(int32_t data):  NextRef(NULL), Type(C4V_Int)
	{ Data.Int = data; }
	explicit C4Value(C4Object *pObj): NextRef(NULL), Type(pObj ? C4V_C4Object : C4V_Nil)
	{ Data.Obj = pObj; AddDataRef(); }
	explicit C4Value(C4String *pStr): NextRef(NULL), Type(pStr ? C4V_String : C4V_Nil)
	{ Data.Str = pStr; AddDataRef(); }
	explicit C4Value(C4ValueArray *pArray): NextRef(NULL), Type(pArray ? C4V_Array : C4V_Nil)
	{ Data.Array = pArray; AddDataRef(); }
	explicit C4Value(C4PropList *p): NextRef(NULL), Type(p ? C4V_PropList : C4V_Nil)
	{ Data.PropList = p; AddDataRef(); }

	C4Value& operator = (const C4Value& nValue) { Set(nValue); return *this; }

	~C4Value() { DelDataRef(Data, Type, NextRef); }

	// Checked getters
	int32_t getInt() const { return ConvertTo(C4V_Int) ? Data.Int : 0; }
	bool getBool() const { return ConvertTo(C4V_Bool) ? !! Data : 0; }
	C4ID getC4ID() const;
	C4Object * getObj() const { return ConvertToNoNil(C4V_C4Object) ? Data.Obj : NULL; }
	C4PropList * getPropList() const { return ConvertToNoNil(C4V_PropList) ? Data.PropList : NULL; }
	C4String * getStr() const { return ConvertToNoNil(C4V_String) ? Data.Str : NULL; }
	C4ValueArray * getArray() const { return ConvertToNoNil(C4V_Array) ? Data.Array : NULL; }

	// Unchecked getters
	int32_t _getInt() const { return Data.Int; }
	bool _getBool() const { return !! Data.Int; }
	C4Object *_getObj() const { return Data.Obj; }
	C4String *_getStr() const { return Data.Str; }
	C4ValueArray *_getArray() const { return Data.Array; }
	C4PropList *_getPropList() const { return Data.PropList; }

	// Template versions

	bool operator ! () const { return !GetData(); }
	inline operator const void* () const { return GetData()?this:0; }  // To allow use of C4Value in conditions

	void Set(const C4Value &nValue) { Set(nValue.Data, nValue.Type); }

	void SetInt(int i) { C4V_Data d; d.Int = i; Set(d, C4V_Int); }
	void SetBool(bool b) { C4V_Data d; d.Int = b; Set(d, C4V_Bool); }
	void SetObject(C4Object * Obj) { C4V_Data d; d.Obj = Obj; Set(d, C4V_C4Object); }
	void SetString(C4String * Str) { C4V_Data d; d.Str = Str; Set(d, C4V_String); }
	void SetArray(C4ValueArray * Array) { C4V_Data d; d.Array = Array; Set(d, C4V_Array); }
	void SetPropList(C4PropList * PropList) { C4V_Data d; d.PropList = PropList; Set(d, C4V_PropList); }
	void Set0();

	bool operator == (const C4Value& Value2) const;
	bool operator != (const C4Value& Value2) const;

	// Change and set Type to int in case it was any before (avoids GuessType())
	C4Value & operator += (int32_t by) { Data.Int += by; Type=C4V_Int; return *this; }
	C4Value & operator -= (int32_t by) { Data.Int -= by; Type=C4V_Int; return *this; }
	C4Value & operator *= (int32_t by) { Data.Int *= by; Type=C4V_Int; return *this; }
	C4Value & operator /= (int32_t by) { Data.Int /= by; Type=C4V_Int; return *this; }
	C4Value & operator %= (int32_t by) { Data.Int %= by; Type=C4V_Int; return *this; }
	C4Value & operator &= (int32_t by) { Data.Int &= by; Type=C4V_Int; return *this; }
	C4Value & operator ^= (int32_t by) { Data.Int ^= by; Type=C4V_Int; return *this; }
	C4Value & operator |= (int32_t by) { Data.Int |= by; Type=C4V_Int; return *this; }
	C4Value & operator ++ ()           { Data.Int++;     Type=C4V_Int; return *this; }
	C4Value operator ++ (int)          { C4Value old = *this; ++(*this); return old; }
	C4Value & operator -- ()           { Data.Int--;     Type=C4V_Int; return *this; }
	C4Value operator -- (int)          { C4Value old = *this; --(*this); return old; }

	// getters
	C4V_Data GetData()    const { return Data; }
	C4V_Type GetType()    const { return Type; }

	const char *GetTypeName() const { return GetC4VName(GetType()); }
	const char *GetTypeInfo();

	void Denumerate(C4ValueNumbers *);

	StdStrBuf GetDataString(int depth = 1) const;

	ALWAYS_INLINE bool ConvertTo(C4V_Type vtToType) const // convert to dest type
	{
		switch (vtToType)
		{
		case C4V_Nil:      return Type == C4V_Nil || (Type == C4V_Int && !*this);
		case C4V_Int:      return Type == C4V_Int || Type == C4V_Nil || Type == C4V_Bool;
		case C4V_Bool:     return true;
		case C4V_PropList: return Type == C4V_PropList || Type == C4V_C4Object || Type == C4V_Nil || (Type == C4V_Int && !*this);
		case C4V_C4Object: return Type == C4V_C4Object || (Type == C4V_PropList && FnCnvObject()) || Type == C4V_Nil || (Type == C4V_Int && !*this);
		case C4V_String:   return Type == C4V_String || Type == C4V_Nil || (Type == C4V_Int && !*this);
		case C4V_Array:    return Type == C4V_Array || Type == C4V_Nil || (Type == C4V_Int && !*this);
		case C4V_Any:      return true;
		default: assert(!"C4Value::ConvertTo: impossible conversion target"); return false;
		}
	}
	ALWAYS_INLINE bool ConvertToNoNil(C4V_Type vtToType) const // convert to dest type
	{
		switch (vtToType)
		{
		case C4V_Nil:      return Type == C4V_Nil || (Type == C4V_Int && !*this);
		case C4V_Int:      return Type == C4V_Int || Type == C4V_Bool;
		case C4V_Bool:     return Type != C4V_Nil;
		case C4V_PropList: return Type == C4V_PropList || Type == C4V_C4Object;
		case C4V_C4Object: return Type == C4V_C4Object || (Type == C4V_PropList && FnCnvObject());
		case C4V_String:   return Type == C4V_String;
		case C4V_Array:    return Type == C4V_Array;
		case C4V_Any:      return Type != C4V_Nil;
		default: assert(!"C4Value::ConvertTo: impossible conversion target"); return false;
		}
	}
	static bool WarnAboutConversion(C4V_Type Type, C4V_Type vtToType);

	// Compilation
	void CompileFunc(StdCompiler *pComp, C4ValueNumbers *);

	static inline bool IsNullableType(C4V_Type Type)
	{ return Type == C4V_Int || Type == C4V_Bool; }

protected:
	// data
	C4V_Data Data;

	// proplist reference list
	C4Value * NextRef;

	// data type
	C4V_Type Type;

	C4Value(C4V_Data nData, C4V_Type nType): Data(nData), NextRef(NULL)
	{ Type = (nData || IsNullableType(nType) ? nType : C4V_Nil); AddDataRef(); }

	void Set(C4V_Data nData, C4V_Type nType);

	void AddDataRef();
	void DelDataRef(C4V_Data Data, C4V_Type Type, C4Value *pNextRef);

	bool FnCnvObject() const;
	void LogDeletedObjectWarning(C4PropList *);

	friend class C4PropList;
};

// converter
inline C4Value C4VInt(int32_t i) { return C4Value(i); }
inline C4Value C4VBool(bool b) { return C4Value(b); }
inline C4Value C4VObj(C4Object *pObj) { return C4Value(pObj); }
inline C4Value C4VPropList(C4PropList * p) { return C4Value(p); }
inline C4Value C4VString(C4String *pStr) { return C4Value(pStr); }
inline C4Value C4VArray(C4ValueArray *pArray) { return C4Value(pArray); }

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

ALWAYS_INLINE void C4Value::AddDataRef()
{
	assert(Type <= C4V_C4ObjectEnum);
	assert(Type != C4V_Nil || !Data);
	switch (Type)
	{
	case C4V_Array: Data.Array->IncRef(); break;
	case C4V_String: Data.Str->IncRef(); break;
	case C4V_C4Object:
#ifdef _DEBUG
		// check if the object actually exists
		if (!C4PropListNumbered::CheckPropList(Data.PropList))
			{ LogF("Warning: using wild object ptr %p!", static_cast<void*>(Data.Obj)); }
#endif
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
	default: break;
	}
}

ALWAYS_INLINE void C4Value::DelDataRef(C4V_Data Data, C4V_Type Type, C4Value *pNextRef)
{
	assert(Type <= C4V_C4DefEnum);
	assert(Type != C4V_Nil || !Data);
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
	// Do not add this to the same linked list twice.
	if (Data == nData && Type >= C4V_FirstPointer) return;

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
	Data.Obj = 0;
	Type = C4V_Nil;

	// clean up (save even if Data was 0 before)
	DelDataRef(oData, oType, NextRef);
}

#endif

