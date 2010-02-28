/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001, 2004  Sven Eberhardt
 * Copyright (c) 2001-2002, 2006  Peter Wortmann
 * Copyright (c) 2006-2008  Günther Brammer
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

// class declarations
class C4Value;
class C4Object;
class C4PropList;
class C4String;
class C4ValueArray;

// C4Value type
enum C4V_Type
{
	C4V_Any=0,				// no type
	C4V_Int=1,				// Integer
	C4V_Bool=2,				// Boolean
	C4V_PropList=3,
	C4V_C4Object=4,		// Pointer on Object
	C4V_String=5,			// String
	C4V_Array=6,			// pointer on array of values
	C4V_pC4Value=7,		// reference on a value (variable)

	C4V_C4ObjectEnum=9 // enumerated object
};

#define C4V_Last (int) C4V_pC4Value

const char* GetC4VName(const C4V_Type Type);
char GetC4VID(const C4V_Type Type);
C4V_Type GetC4VFromID(char C4VID);

union C4V_Data {
	intptr_t Int;
	C4Object * Obj;
	C4PropList * PropList;
	C4String * Str;
	C4Value * Ref;
	C4ValueArray * Array;
	// cheat a little - assume that all members have the same length
	operator void * () { return Ref; }
	operator const void * () const { return Ref; }
	bool operator== (C4V_Data b) { return Ref == b.Ref; }
	C4V_Data& operator= (C4Value * p) { Ref = p; return *this; }
};
// converter function, used in converter table
struct C4VCnvFn
	{
	bool (*Function) (C4Value*, C4V_Type); // function to be called; returns whether possible
	bool Warn;
	};

template <typename T> struct C4ValueConv;

class C4Value
{
public:

	C4Value() : NextRef(NULL), FirstRef(NULL), Type(C4V_Any), HasBaseArray(false) { Data.Ref = 0; }

	C4Value(const C4Value &nValue) : Data(nValue.Data), NextRef(NULL), FirstRef(NULL), Type(nValue.Type), HasBaseArray(false)
		{ AddDataRef(); }

	explicit C4Value(bool data): NextRef(NULL), FirstRef(NULL), Type(C4V_Bool), HasBaseArray(false)
		{ Data.Int = data; AddDataRef(); }
	explicit C4Value(int32_t data): NextRef(NULL), FirstRef(NULL), Type(C4V_Int), HasBaseArray(false)
		{ Data.Int = data; AddDataRef(); }
	explicit C4Value(C4Object *pObj): NextRef(NULL), FirstRef(NULL), Type(pObj ? C4V_C4Object : C4V_Any), HasBaseArray(false)
		{ Data.Obj = pObj; AddDataRef(); }
	explicit C4Value(C4String *pStr): NextRef(NULL), FirstRef(NULL), Type(pStr ? C4V_String : C4V_Any), HasBaseArray(false)
		{ Data.Str = pStr; AddDataRef(); }
	explicit C4Value(C4ValueArray *pArray): NextRef(NULL), FirstRef(NULL), Type(pArray ? C4V_Array : C4V_Any), HasBaseArray(false)
		{ Data.Array = pArray; AddDataRef(); }
	explicit C4Value(C4PropList *p): NextRef(NULL), FirstRef(NULL), Type(p ? C4V_PropList : C4V_Any), HasBaseArray(false)
		{ Data.PropList = p; AddDataRef(); }
	explicit C4Value(C4Value *pVal): NextRef(NULL), FirstRef(NULL), Type(pVal ? C4V_pC4Value : C4V_Any), HasBaseArray(false)
		{ Data.Ref = pVal; AddDataRef(); }

	C4Value& operator = (const C4Value& nValue);

	~C4Value();

	// Checked getters
	int32_t getInt() { return ConvertTo(C4V_Int) ? Data.Int : 0; }
	int32_t getIntOrID() { Deref(); if (Type == C4V_Int || Type == C4V_Bool /* FIXME || Type == C4V_C4ID*/) return Data.Int; else return 0; }
	bool getBool() { return ConvertTo(C4V_Bool) ? !! Data : 0; }
	C4ID getC4ID();
	C4Object * getObj() { return ConvertTo(C4V_C4Object) ? Data.Obj : NULL; }
	C4PropList * getPropList() { return ConvertTo(C4V_PropList) ? Data.PropList : NULL; }
	C4String * getStr() { return ConvertTo(C4V_String) ? Data.Str : NULL; }
	C4ValueArray * getArray() { return ConvertTo(C4V_Array) ? Data.Array : NULL; }
	C4Value * getRef() { return ConvertTo(C4V_pC4Value) ? Data.Ref : NULL; }

	// Unchecked getters
	int32_t _getInt() const { return Data.Int; }
	bool _getBool() const { return !! Data.Int; }
	C4Object *_getObj() const { return Data.Obj; }
	C4String *_getStr() const { return Data.Str; }
	C4ValueArray *_getArray() const { return Data.Array; }
	C4PropList *_getPropList() const { return Data.PropList; }
	C4Value *_getRef() { return Data.Ref; }
	long _getRaw() const { return Data.Int; }

	// Template versions
	template <typename T> inline T Get() { return C4ValueConv<T>::FromC4V(*this); }
	template <typename T> inline T _Get() { return C4ValueConv<T>::_FromC4V(*this); }

	bool operator ! () const { return !GetData(); }
	inline operator const void* () const { return GetData()?this:0; }  // To allow use of C4Value in conditions

	void Set(const C4Value &nValue) { if (this != &nValue) Set(nValue.Data, nValue.Type); }

	void SetInt(int i) { C4V_Data d; d.Int = i; Set(d, C4V_Int); }

	void SetBool(bool b) { C4V_Data d; d.Int = b; Set(d, C4V_Bool); }

	void SetObject(C4Object * Obj) { C4V_Data d; d.Obj = Obj; Set(d, C4V_C4Object); }

	void SetString(C4String * Str) { C4V_Data d; d.Str = Str; Set(d, C4V_String); }

	void SetArray(C4ValueArray * Array) { C4V_Data d; d.Array = Array; Set(d, C4V_Array); }

	void SetPropList(C4PropList * PropList) { C4V_Data d; d.PropList = PropList; Set(d, C4V_PropList); }

	void SetRef(C4Value* nValue) { C4V_Data d; d.Ref = nValue; Set(d, C4V_pC4Value); }

	void Set0();

	bool operator == (const C4Value& Value2) const;
	bool operator != (const C4Value& Value2) const;

	// Change and set Type to int in case it was any before (avoids GuessType())
	C4Value & operator += (int32_t by) { GetData().Int += by; GetRefVal().Type=C4V_Int; return *this; }
	C4Value & operator ++ () { GetData().Int ++; GetRefVal().Type=C4V_Int; return *this; }
	C4Value operator ++ (int) { C4Value alt = GetRefVal(); GetData().Int ++; GetRefVal().Type=C4V_Int; return alt; }
	C4Value & operator -- () { GetData().Int --; GetRefVal().Type=C4V_Int; return *this; }
	C4Value & operator -= (int32_t by) { GetData().Int -= by; GetRefVal().Type=C4V_Int; return *this; }
	C4Value operator -- (int) { C4Value alt = GetRefVal(); GetData().Int --; GetRefVal().Type=C4V_Int; return alt; }

	void Move(C4Value *nValue);

	C4Value GetRef() { return C4Value(this); }
	void Deref() { Set(GetRefVal()); }
	bool IsRef() { return Type == C4V_pC4Value; }

	// get data of referenced value
	C4V_Data GetData() const { return GetRefVal().Data; }
	C4V_Data & GetData() { return GetRefVal().Data; }

	// get type of referenced value
	C4V_Type GetType() const { return GetRefVal().Type; }

	// return referenced value
	const C4Value & GetRefVal() const;
	C4Value & GetRefVal();

	// Get the Value at the index. Throws C4AulExecError if not an array
	void GetArrayElement(int32_t index, C4Value & to, struct C4AulContext *pctx = 0, bool noref = false);
	// Set the length of the array. Throws C4AulExecError if not an array
	void SetArrayLength(int32_t size, C4AulContext *cthr);

	const char *GetTypeName() const { return GetC4VName(GetType()); }
	const char *GetTypeInfo();

	void DenumeratePointer();

	StdStrBuf GetDataString();

	inline bool ConvertTo(C4V_Type vtToType) // convert to dest type
		{
		C4VCnvFn Fn = C4ScriptCnvMap[Type][vtToType];
		if (Fn.Function)
			return (*Fn.Function)(this, vtToType);
		return true;
		}

	// Compilation
	void CompileFunc(StdCompiler *pComp);

	static inline bool IsNullableType(C4V_Type Type)
		{ return Type == C4V_Int || Type == C4V_Bool; }

protected:
	// data
	C4V_Data Data;

	// reference-list
	union {
	C4Value * NextRef;
	C4ValueArray * BaseArray;
	};
	C4Value * FirstRef;

	// data type
	C4V_Type Type:8;
	bool HasBaseArray:8;
	// All referenzes to a C4Value form a linked list, so that they can be updated if the C4Value
	// has to move, (array is resized), or goes out of scope (func & f() { var r; return r; })
	// If the reference is in an array, the last c4value in the list has HasBaseArray set, and
	// BaseArray points to the array. This is used to count the references to elements that an
	// array has, and maintain the invariant that an array can only have multiple references to
	// the array OR its elements, but not both. For example, a[0] = a; has to copy the entire
	// old array into its first element. But a[0]=42; GetLength(a); should not copy the array,
	// so the element reference count has to be reset after a[0] is removed from the stack.

	C4Value * GetNextRef() { if (HasBaseArray) return 0; else return NextRef; }
	C4ValueArray * GetBaseArray() { if (HasBaseArray) return BaseArray; else return 0; }

	C4Value(C4V_Data nData, C4V_Type nType): Data(nData), NextRef(NULL), FirstRef(NULL), HasBaseArray(false)
		{ Type = nData || IsNullableType(nType) ? nType : C4V_Any; AddDataRef(); }

	void Set(long nData, C4V_Type nType) { C4V_Data d; d.Int = nData; Set(d, nType); }
	void Set(C4V_Data nData, C4V_Type nType);

	void AddRef(C4Value *pRef);
	void DelRef(const C4Value *pRef, C4Value * pNextRef, C4ValueArray * pBaseArray);

	void AddDataRef();
	void DelDataRef(C4V_Data Data, C4V_Type Type, C4Value * pNextRef, C4ValueArray * pBaseArray);

	static C4VCnvFn C4ScriptCnvMap[C4V_Last+1][C4V_Last+1];
	static bool FnCnvObject(C4Value *Val, C4V_Type toType);

	friend class C4PropList;
	friend class C4AulDefFunc;
	friend C4Value C4VInt(int32_t iVal);
	friend C4Value C4VBool(bool fVal);
};

// converter
inline C4Value C4VInt(int32_t iVal) { C4V_Data d; d.Int = iVal; return C4Value(d, C4V_Int); }
inline C4Value C4VBool(bool fVal) { C4V_Data d; d.Int = fVal; return C4Value(d, C4V_Bool); }
C4Value C4VID(C4ID iVal);
inline C4Value C4VObj(C4Object *pObj) { return C4Value(pObj); }
inline C4Value C4VPropList(C4PropList * p) { return C4Value(p); }
inline C4Value C4VString(C4String *pStr) { return C4Value(pStr); }
inline C4Value C4VArray(C4ValueArray *pArray) { return C4Value(pArray); }
inline C4Value C4VRef(C4Value *pVal) { return pVal->GetRef(); }

C4Value C4VString(StdStrBuf strString);
C4Value C4VString(const char *strString);

// converter templates
template <> struct C4ValueConv<int32_t>
{
	inline static C4V_Type Type() { return C4V_Int; }
	inline static int32_t FromC4V(C4Value &v) { return v.getInt(); }
	inline static int32_t _FromC4V(C4Value &v) { return v._getInt(); }
	inline static C4Value ToC4V(int32_t v) { return C4VInt(v); }
};
template <> struct C4ValueConv<bool>
{
	inline static C4V_Type Type() { return C4V_Bool; }
	inline static bool FromC4V(C4Value &v) { return v.getBool(); }
	inline static bool _FromC4V(C4Value &v) { return v._getBool(); }
	inline static C4Value ToC4V(bool v) { return C4VBool(v); }
};
template <> struct C4ValueConv<C4ID>
{
	inline static C4V_Type Type() { return C4V_PropList; }
	inline static C4ID FromC4V(C4Value &v) { return v.getC4ID(); }
	inline static C4ID _FromC4V(C4Value &v) { return FromC4V(v); }
	inline static C4Value ToC4V(C4ID v) { return C4VID(v); }
};
template <> struct C4ValueConv<C4Object *>
{
	inline static C4V_Type Type() { return C4V_C4Object; }
	inline static C4Object *FromC4V(C4Value &v) { return v.getObj(); }
	inline static C4Object *_FromC4V(C4Value &v) { return v._getObj(); }
	inline static C4Value ToC4V(C4Object *v) { return C4VObj(v); }
};
template <> struct C4ValueConv<C4String *>
{
	inline static C4V_Type Type() { return C4V_String; }
	inline static C4String *FromC4V(C4Value &v) { return v.getStr(); }
	inline static C4String *_FromC4V(C4Value &v) { return v._getStr(); }
	inline static C4Value ToC4V(C4String *v) { return C4VString(v); }
};
template <> struct C4ValueConv<C4ValueArray *>
{
	inline static C4V_Type Type() { return C4V_Array; }
	inline static C4ValueArray *FromC4V(C4Value &v) { return v.getArray(); }
	inline static C4ValueArray *_FromC4V(C4Value &v) { return v._getArray(); }
	inline static C4Value ToC4V(C4ValueArray *v) { return C4VArray(v); }
};
template <> struct C4ValueConv<C4PropList *>
{
	inline static C4V_Type Type() { return C4V_PropList; }
	inline static C4PropList *FromC4V(C4Value &v) { return v.getPropList(); }
	inline static C4PropList *_FromC4V(C4Value &v) { return v._getPropList(); }
	inline static C4Value ToC4V(C4PropList *v) { return C4VPropList(v); }
};
template <> struct C4ValueConv<C4Value *>
{
	inline static C4V_Type Type() { return C4V_pC4Value; }
	inline static C4Value *FromC4V(C4Value &v) { return v.getRef(); }
	inline static C4Value *_FromC4V(C4Value &v) { return v._getRef(); }
	inline static C4Value ToC4V(C4Value *v) { return C4VRef(v); }
};

// aliases
template <> struct C4ValueConv<long> : public C4ValueConv<int32_t> { };
#if defined(_MSC_VER) && _MSC_VER <= 1100
template <> struct C4ValueConv<int> : public C4ValueConv<int32_t> { };
#endif

extern const C4Value C4VFalse, C4VTrue;

// type tag to allow other code to recognize C4VNull at compile time
class C4NullValue : public C4Value {};
extern const C4NullValue C4VNull;

#endif

