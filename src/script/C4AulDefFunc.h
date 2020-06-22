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
// Template helper to export C++ functions to C4Script

#ifndef INC_C4AulDefFunc
#define INC_C4AulDefFunc

#include "script/C4Aul.h"
#include "object/C4Def.h"
#include "object/C4DefList.h"
#include "script/C4Effect.h"

inline const static char *FnStringPar(C4String *pString)
{
	return pString ? pString->GetCStr() : "";
}
inline C4String *String(const char * str)
{
	return str ? ::Strings.RegString(str) : nullptr;
}
inline C4Object * Object(C4PropList * _this)
{
	return _this ? _this->GetObject() : nullptr;
}
StdStrBuf FnStringFormat(C4PropList * _this, C4String *szFormatPar, C4Value * Pars, int ParCount);
C4Effect ** FnGetEffectsFor(C4PropList * pTarget);

// Nillable: Allow integer and boolean parameters to be nil
// pointer parameters represent nil via plain nullptr
// other types can use C4Void
class C4Void { };
template <typename T> struct C4ValueConv;
template <typename T>
class Nillable
{
	bool _nil;
	T _val;
public:
	inline Nillable(const T &value) : _nil(!value && !C4Value::IsNullableType(C4ValueConv<T>::Type)), _val(value) {}
	inline Nillable(const Nillable<T> &other) { *this = other; }
	inline Nillable() : _nil(true), _val(T()) {}
	inline Nillable(std::nullptr_t) : _nil(true), _val(T()) {}
	template <typename T2> inline Nillable(const Nillable<T2> & n2) : _nil(n2._nil), _val(n2._val) {}
	inline Nillable(const C4Void &) : _nil(true), _val(T()) {}
	inline bool IsNil() const { return _nil; }
	inline operator T() const { return _val; }
	inline Nillable<T> &operator =(const T &val)
	{
		_val = val;
		_nil = !val && !C4Value::IsNullableType(C4ValueConv<T>::Type);
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

// Other functions are callable in object context only.
// This exception gets thrown if they are called from anywhere else.
class NeedObjectContext : public C4AulExecError
{
public:
	NeedObjectContext(const char *function) : C4AulExecError(FormatString("%s: must be called from object context", function).getData()) {}
};

// Then there's functions that don't care, but need either defn or object context.
// This exception gets thrown if those are called from global scripts.
class NeedNonGlobalContext : public C4AulExecError
{
public:
	NeedNonGlobalContext(const char *function) : C4AulExecError(FormatString("%s: call must not be from global context", function).getData()) {}
};

// For functions taking a C4Object as this, check that they got one
template <typename ThisType> struct ThisImpl;
template <> struct ThisImpl<C4Object>
{
	static C4Object* Conv(C4PropList* _this, C4AulFunc* func)
	{
		C4Object* Obj = _this ? _this->GetObject() : nullptr;
		if (Obj)
			return Obj;
		else
			throw NeedObjectContext(func->GetName());
	}
};
template <> struct ThisImpl<C4PropList>
{
	static C4PropList* Conv(C4PropList* _this, C4AulFunc* func)
	{
		return _this;
	}
};

// Extracts the parameters from C4Values and wraps the return value in a C4Value
template <typename RType, typename ThisType, typename ...ParTypes>
struct ExecImpl
{
	template <std::size_t... Is>
	static C4Value Exec(RType (*pFunc)(ThisType *, ParTypes...), ThisType * _this, C4Value pPars[], std::index_sequence<Is...>)
	{
		return C4Value(pFunc(_this, C4ValueConv<ParTypes>::_FromC4V(pPars[Is])...));
		(void) pPars;
	}
};
template <typename ThisType, typename ...ParTypes>
struct ExecImpl<void, ThisType, ParTypes...>
{
	template <std::size_t... Is>
	static C4Value Exec(void (*pFunc)(ThisType *, ParTypes...), ThisType * _this, C4Value pPars[], std::index_sequence<Is...>)
	{
		pFunc(_this, C4ValueConv<ParTypes>::_FromC4V(pPars[Is])...);
		return C4Value();
		(void) pPars;
	}
};

// converter templates
template <typename T>
struct C4ValueConv<Nillable<T> >
{
	inline static Nillable<T> _FromC4V(C4Value &v) { if (v.GetType() == C4V_Nil) return C4Void(); else return C4ValueConv<T>::_FromC4V(v); }
	static constexpr C4V_Type Type = C4ValueConv<T>::Type;
};
template <> struct C4ValueConv<void>
{
	static constexpr C4V_Type Type = C4V_Nil;
};
template <> struct C4ValueConv<int>
{
	static constexpr C4V_Type Type = C4V_Int;
	inline static int _FromC4V(C4Value &v) { return v._getInt(); }
};
template <> struct C4ValueConv<long>: public C4ValueConv<int> { };
template <> struct C4ValueConv<bool>
{
	static constexpr C4V_Type Type = C4V_Bool;
	inline static bool _FromC4V(C4Value &v) { return v._getBool(); }
};
template <> struct C4ValueConv<C4ID>
{
	static constexpr C4V_Type Type = C4V_PropList;
	inline static C4ID _FromC4V(C4Value &v) { C4Def * def = v.getDef(); return def ? def->id : C4ID::None; }
};
template <> struct C4ValueConv<C4Object *>
{
	static constexpr C4V_Type Type = C4V_Object;
	inline static C4Object *_FromC4V(C4Value &v) { return v._getObj(); }
};
template <> struct C4ValueConv<C4String *>
{
	static constexpr C4V_Type Type = C4V_String;
	inline static C4String *_FromC4V(C4Value &v) { return v._getStr(); }
};
template <> struct C4ValueConv<C4ValueArray *>
{
	static constexpr C4V_Type Type = C4V_Array;
	inline static C4ValueArray *_FromC4V(C4Value &v) { return v._getArray(); }
};
template <> struct C4ValueConv<C4AulFunc *>
{
	static constexpr C4V_Type Type = C4V_Function;
	inline static C4AulFunc *_FromC4V(C4Value &v) { return v._getFunction(); }
};
template <> struct C4ValueConv<C4PropList *>
{
	static constexpr C4V_Type Type = C4V_PropList;
	inline static C4PropList *_FromC4V(C4Value &v) { return v._getPropList(); }
};
template <> struct C4ValueConv<C4Effect *>
{
	static constexpr C4V_Type Type = C4V_Effect;
	inline static C4Effect *_FromC4V(C4Value &v) { C4PropList * p = v._getPropList(); return p ? p->GetEffect() : nullptr; }
};
template <> struct C4ValueConv<C4Def *>
{
	static constexpr C4V_Type Type = C4V_Def;
	inline static C4Def *_FromC4V(C4Value &v) { return v._getDef(); }
};
template <> struct C4ValueConv<const C4Value &>
{
	static constexpr C4V_Type Type = C4V_Any;
	inline static const C4Value &_FromC4V(C4Value &v) { return v; }
};
template <> struct C4ValueConv<C4Value>
{
	static constexpr C4V_Type Type = C4V_Any;
	inline static C4Value _FromC4V(C4Value &v) { return v; }
};

// Wrapper around an ordinary C++ function callable from C4Script
template <typename RType, typename ThisType, typename ...ParTypes>
class C4AulEngineFunc: public C4AulFunc
{
public:
	/* A pointer to the function which this class wraps */
	typedef RType (*Func)(ThisType *, ParTypes...);

	C4AulEngineFunc(C4PropListStatic * Parent, const char *pName, Func pFunc, bool Public):
		C4AulFunc(Parent, pName),
		pFunc(pFunc), ParType {C4ValueConv<ParTypes>::Type...}, Public(Public)
	{
		Parent->SetPropertyByS(Name, C4VFunction(this));
		for(int i = GetParCount(); i < C4AUL_MAX_Par; ++i)
			ParType[i] = C4V_Any;
	}

	int GetParCount() const override
	{
		return sizeof...(ParTypes);
	}

	const C4V_Type* GetParType() const override
	{
		return ParType;
	}

	C4V_Type GetRetType() const override
	{
		return C4ValueConv<RType>::Type;
	}

	bool GetPublic() const override
	{
		return Public;
	}

	C4Value Exec(C4PropList * _this, C4Value pPars[], bool fPassErrors) override
	{
		return ExecImpl<RType, ThisType, ParTypes...>::Exec(pFunc, ThisImpl<ThisType>::Conv(_this, this),
								    pPars, std::index_sequence_for<ParTypes...>{});
	}
protected:
	Func pFunc;
	C4V_Type ParType[C4AUL_MAX_Par];// type of the parameters
	bool Public;
};

template <typename RType, typename ThisType, typename ...ParTypes>
inline void AddFunc(C4PropListStatic * Parent, const char * Name, RType (*pFunc)(ThisType *, ParTypes...), bool Public=true)
{
	new C4AulEngineFunc<RType, ThisType, ParTypes...>(Parent, Name, pFunc, Public);
}

// a definition of a script constant
struct C4ScriptConstDef
{
	const char * Identifier; // constant name
	C4V_Type ValType;       // type value
	long Data;               // raw data
};

// a definition of a function exported to script
struct C4ScriptFnDef
{
	const char * Identifier; // the name of the func in the script
	bool Public;
	C4V_Type RetType; // type returned. ignored when C4V
	C4V_Type ParType[10];// type of the parameters. error when wrong parameter type.
	C4Value (*FunctionC4V)(C4PropList * _this, C4Value *);
};

// defined function class
class C4AulDefFunc : C4AulFunc
{
public:
	C4ScriptFnDef* Def;

	C4AulDefFunc(C4PropListStatic * Parent, C4ScriptFnDef* pDef);
	~C4AulDefFunc() override;

	bool GetPublic() const override { return !!Def->Public; }
	const C4V_Type* GetParType() const override { return Def->ParType; }
	C4V_Type GetRetType() const override { return Def->RetType; }

	C4Value Exec(C4PropList * p, C4Value pPars[], bool fPassErrors=false) override;
};

#endif
