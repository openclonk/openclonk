/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2004-2006, 2010  Peter Wortmann
 * Copyright (c) 2006-2007, 2009-2011  Günther Brammer
 * Copyright (c) 2009  Nicolas Hake
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
// Template helper to export C++ functions to C4Script

#ifndef INC_C4AulDefFunc
#define INC_C4AulDefFunc

#include <C4Object.h>
#include <C4Effects.h>
#include <C4DefList.h>

typedef int32_t t_int;
typedef bool t_bool;
typedef C4ID t_id;
typedef C4Object *t_object;
typedef C4String *t_string;
typedef C4Value t_any;
typedef C4ValueArray *t_array;

inline t_int getPar_int(C4Value *pVal) { return pVal->getInt(); }
inline t_bool getPar_bool(C4Value *pVal) { return pVal->getBool(); }
inline t_id getPar_id(C4Value *pVal) { return pVal->getC4ID(); }
inline t_object getPar_object(C4Value *pVal) { return pVal->getObj(); }
inline t_string getPar_string(C4Value *pVal) { return pVal->getStr(); }
inline t_any getPar_any(C4Value *pVal) { return *pVal; }
inline t_array getPar_array(C4Value *pVal) { return pVal->getArray(); }

#define PAR(type, name) t_##type name = getPar_##type(pPars++)

inline const static char *FnStringPar(C4String *pString)
{
	return pString ? pString->GetCStr() : "";
}
inline C4String *String(const char * str)
{
	return str ? ::Strings.RegString(str) : NULL;
}
StdStrBuf FnStringFormat(C4AulContext *cthr, const char *szFormatPar, C4Value * Par0=0, C4Value * Par1=0, C4Value * Par2=0, C4Value * Par3=0,
                                C4Value * Par4=0, C4Value * Par5=0, C4Value * Par6=0, C4Value * Par7=0, C4Value * Par8=0, C4Value * Par9=0);
enum { MaxFnStringParLen=500 };

template <typename T> struct C4ValueConv;
// Allow parameters to be nil
template<typename T>
class Nillable
{
	bool _nil;
	T _val;
public:
	inline Nillable(const T &value) : _nil(!value && !C4Value::IsNullableType(C4ValueConv<T>::Type())), _val(value) {}
	inline Nillable() : _nil(true), _val(T()) {}
	template <typename T2> inline Nillable(const Nillable<T2> & n2) : _nil(n2._nil), _val(n2._val) {}
	inline Nillable(const Nillable<void> & n2) : _nil(true), _val(T()) {}
	inline bool IsNil() const { return _nil; }
	inline operator T() const { return _val; }
	inline Nillable<T> &operator =(const T &val)
	{
		_val = val;
		_nil = !val && !C4Value::IsNullableType(C4ValueConv<T>::Type());
		return *this;
	}
	inline Nillable<T> &operator =(const Nillable<T> &val)
	{
		_val = val._val;
		_nil = val._nil;
		return *this;
	}
	// Some operators
	inline Nillable<T> &operator ++() { ++_val; return *this; }
	inline T operator ++(int) { T v(_val++); return v; }
	inline Nillable<T> &operator --() { --_val; return *this; }
	inline T operator --(int) { T v(_val--); return v; }
};

template<>
class Nillable<void>
{
public:
	inline Nillable() {}
	inline bool IsNil() const { return true; }
};

// Some functions require object context.
// Using this type instead of C4AulContext guarantees validity of Obj member.
// Don't add any members to this class, or breakage will occur.
class C4AulObjectContext : public C4AulContext {};

// Some functions are callable in definition context only.
// This exception gets thrown if they are called from anywhere else.
class NeedDefinitionContext : public C4AulExecError
{
public:
	NeedDefinitionContext(const char *function) : C4AulExecError(NULL, FormatString("%s: must be called from definition context", function).getData()) {}
};

// Other functions are callable in object context only.
// This exception gets thrown if they are called from anywhere else.
class NeedObjectContext : public C4AulExecError
{
public:
	NeedObjectContext(const char *function) : C4AulExecError(NULL, FormatString("%s: must be called from object context", function).getData()) {}
};

// Then there's functions that don't care, but need either defn or object context.
// This exception gets thrown if those are called from global scripts.
class NeedNonGlobalContext : public C4AulExecError
{
public:
	NeedNonGlobalContext(const char *function) : C4AulExecError(NULL, FormatString("%s: call must not be from global context", function).getData()) {}
};

// return type of functions returning nil
typedef Nillable<void> C4Void;

// converter templates
template<typename T>
struct C4ValueConv<Nillable<T> >
{
	inline static Nillable<T> FromC4V(C4Value &v) { if (v.GetType() == C4V_Nil) return C4Void(); else return C4ValueConv<T>::FromC4V(v); }
	inline static Nillable<T> _FromC4V(C4Value &v) { if (v.GetType() == C4V_Nil) return C4Void(); else return C4ValueConv<T>::_FromC4V(v); }
	inline static C4V_Type Type() { return C4ValueConv<T>::Type(); }
	inline static C4Value ToC4V(const Nillable<T> &v) { if (v.IsNil()) return C4Value(); else return C4ValueConv<T>::ToC4V(v.operator T()); }
};
template<>
struct C4ValueConv<Nillable<void> >
{
	inline static C4V_Type Type() { return C4VNull.GetType(); }
	inline static C4Value ToC4V(const Nillable<void> &) { return C4Value(); }
};
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
	inline static C4Value ToC4V(C4ID v) { return C4VPropList(C4Id2Def(v)); }
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
template <> struct C4ValueConv<C4Effect *>
{
	inline static C4V_Type Type() { return C4V_PropList; }
	inline static C4Effect *FromC4V(C4Value &v) { C4PropList * p = v.getPropList(); return p ? p->GetEffect() : 0; }
	inline static C4Effect *_FromC4V(C4Value &v) { C4PropList * p = v._getPropList(); return p ? p->GetEffect() : 0; }
	inline static C4Value ToC4V(C4Effect *v) { return C4VPropList(v); }
};
template <> struct C4ValueConv<C4Def *>
{
	inline static C4V_Type Type() { return C4V_PropList; }
	inline static C4Def *FromC4V(C4Value &v) { C4PropList * p = v.getPropList(); return p ? p->GetDef() : 0; }
	inline static C4Def *_FromC4V(C4Value &v) { C4PropList * p = v._getPropList(); return p ? p->GetDef() : 0; }
	inline static C4Value ToC4V(C4Def *v) { return C4VPropList(v); }
};
template <> struct C4ValueConv<const C4Value &>
{
	inline static C4V_Type Type() { return C4V_Any; }
	inline static const C4Value &FromC4V(C4Value &v) { return v; }
	inline static const C4Value &_FromC4V(C4Value &v) { return v; }
	inline static C4Value ToC4V(const C4Value &v) { return v; }
};
template <> struct C4ValueConv<C4Value>
{
	inline static C4V_Type Type() { return C4V_Any; }
	inline static C4Value FromC4V(C4Value &v) { return v; }
	inline static C4Value _FromC4V(C4Value &v) { return v; }
	inline static C4Value ToC4V(C4Value v) { return v; }
};

// aliases
template <> struct C4ValueConv<long> : public C4ValueConv<int32_t> { };
#if defined(_MSC_VER) && _MSC_VER <= 1100
template <> struct C4ValueConv<int> : public C4ValueConv<int32_t> { };
#endif

// defined function class
class C4AulDefFuncHelper: public C4AulFunc
{
public:
	C4AulDefFuncHelper(C4AulScript *pOwner, const char *pName, bool Public,
	                   C4V_Type pt0 = C4V_Any, C4V_Type pt1 = C4V_Any, C4V_Type pt2 = C4V_Any, C4V_Type pt3 = C4V_Any, C4V_Type pt4 = C4V_Any,
	                   C4V_Type pt5 = C4V_Any, C4V_Type pt6 = C4V_Any, C4V_Type pt7 = C4V_Any, C4V_Type pt8 = C4V_Any, C4V_Type pt9 = C4V_Any):
			C4AulFunc(pOwner, pName),
			Public(Public)
	{
		ParType[0] = pt0;
		ParType[1] = pt1;
		ParType[2] = pt2;
		ParType[3] = pt3;
		ParType[4] = pt4;
		ParType[5] = pt5;
		ParType[6] = pt6;
		ParType[7] = pt7;
		ParType[8] = pt8;
		ParType[9] = pt9;
	}
	virtual C4V_Type* GetParType() { return ParType; }
	virtual bool GetPublic() { return Public; }
protected:
	C4V_Type ParType[10];// type of the parameters
	bool Public;
};

// A macro to create lists with some helper macros
// LIST(2, foo) would create ", foo(0), foo(1)
// TEXT can be a macro
#define LIST(N, TEXT) LIST##N(TEXT)
// The lists are used in a context where a leading comma is needed when the list is not empty
#define LIST0(TEXT)
#define LIST1(TEXT) LIST0(TEXT), TEXT(0)
#define LIST2(TEXT) LIST1(TEXT), TEXT(1)
#define LIST3(TEXT) LIST2(TEXT), TEXT(2)
#define LIST4(TEXT) LIST3(TEXT), TEXT(3)
#define LIST5(TEXT) LIST4(TEXT), TEXT(4)
#define LIST6(TEXT) LIST5(TEXT), TEXT(5)
#define LIST7(TEXT) LIST6(TEXT), TEXT(6)
#define LIST8(TEXT) LIST7(TEXT), TEXT(7)
#define LIST9(TEXT) LIST8(TEXT), TEXT(8)
#define LIST10(TEXT) LIST9(TEXT), TEXT(9)

// Macros which are passed to LIST
#define TYPENAMES(N) typename Par##N##_t
#define PARS(N) Par##N##_t
#define CONV_TYPE(N) C4ValueConv<Par##N##_t>::Type()
#define CONV_FROM_C4V(N) C4ValueConv<Par##N##_t>::_FromC4V(pPars[N])

// N is the number of parameters pFunc needs. Templates can only have a fixed number of arguments,
// so eleven templates are needed
#define TEMPLATE(N)                           \
template <typename RType LIST(N, TYPENAMES)>  \
class C4AulDefFunc##N:                        \
public C4AulDefFuncHelper {                   \
  public:                                     \
/* A pointer to the function which this class wraps */ \
    typedef RType (*Func)(C4AulContext * LIST(N, PARS)); \
    virtual int GetParCount() { return N; }   \
    virtual C4V_Type GetRetType()             \
    { return C4ValueConv<RType>::Type(); }    \
/* Constructor, using the base class to create the ParType array */ \
    C4AulDefFunc##N(C4AulScript *pOwner, const char *pName, Func pFunc, bool Public): \
      C4AulDefFuncHelper(pOwner, pName, Public LIST(N, CONV_TYPE)), pFunc(pFunc) { } \
/* Avoid hiding base class function */        \
    using C4AulFunc::Exec;                    \
/* Extracts the parameters from C4Values and wraps the return value in a C4Value */ \
    virtual C4Value Exec(C4AulContext *pContext, C4Value pPars[], bool fPassErrors=false) \
    { return C4ValueConv<RType>::ToC4V(pFunc(pContext LIST(N, CONV_FROM_C4V))); } \
  protected:                                  \
    Func pFunc;                               \
  };                                          \
template <typename RType LIST(N, TYPENAMES)>  \
class C4AulDefObjectFunc##N:                  \
public C4AulDefFuncHelper {                   \
  public:                                     \
/* A pointer to the function which this class wraps */ \
    typedef RType (*Func)(C4AulObjectContext * LIST(N, PARS)); \
    virtual int GetParCount() { return N; }   \
    virtual C4V_Type GetRetType()             \
    { return C4ValueConv<RType>::Type(); }    \
/* Constructor, using the base class to create the ParType array */ \
    C4AulDefObjectFunc##N(C4AulScript *pOwner, const char *pName, Func pFunc, bool Public): \
      C4AulDefFuncHelper(pOwner, pName, Public LIST(N, CONV_TYPE)), pFunc(pFunc) { } \
/* Avoid hiding base class function */        \
    using C4AulFunc::Exec;                    \
/* Extracts the parameters from C4Values and wraps the return value in a C4Value */ \
    virtual C4Value Exec(C4AulContext *pContext, C4Value pPars[], bool fPassErrors=false) \
    { \
      if (!pContext->Obj) throw new NeedObjectContext(Name); \
      return C4ValueConv<RType>::ToC4V(pFunc(static_cast<C4AulObjectContext*>(pContext) LIST(N, CONV_FROM_C4V))); \
    } \
  protected:                                  \
    Func pFunc;                               \
  };                                          \
template <typename RType LIST(N, TYPENAMES)> \
inline void AddFunc(C4AulScript * pOwner, const char * Name, RType (*pFunc)(C4AulContext * LIST(N, PARS)), bool Public=true) \
  { \
  new C4AulDefFunc##N<RType LIST(N, PARS)>(pOwner, Name, pFunc, Public); \
  } \
template <typename RType LIST(N, TYPENAMES)> \
inline void AddFunc(C4AulScript * pOwner, const char * Name, RType (*pFunc)(C4AulObjectContext * LIST(N, PARS)), bool Public=true) \
  { \
  new C4AulDefObjectFunc##N<RType LIST(N, PARS)>(pOwner, Name, pFunc, Public); \
  }

TEMPLATE(0)
TEMPLATE(1)
TEMPLATE(2)
TEMPLATE(3)
TEMPLATE(4)
TEMPLATE(5)
TEMPLATE(6)
TEMPLATE(7)
TEMPLATE(8)
TEMPLATE(9)
TEMPLATE(10)


#undef LIST
#undef LIST0
#undef LIST1
#undef LIST2
#undef LIST3
#undef LIST4
#undef LIST5
#undef LIST6
#undef LIST7
#undef LIST8
#undef LIST9
#undef LIST10

#undef TYPENAMES
#undef PARS
#undef CONV_TYPE
#undef CONV_FROM_C4V
#undef TEMPLATE

#endif
