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

#include "C4Include.h"
#include "script/C4Value.h"

#include "script/C4AulExec.h"
#include "object/C4Def.h"
#include "object/C4DefList.h"
#include "script/C4StringTable.h"
#include "script/C4ValueArray.h"
#include "game/C4Game.h"
#include "game/C4GameScript.h"
#include "object/C4GameObjects.h"
#include "object/C4Object.h"
#include "lib/C4Log.h"
#include "script/C4Effect.h"

const C4Value C4VNull;

const char* GetC4VName(const C4V_Type Type)
{
	switch (Type)
	{
	case C4V_Nil:
		return "nil";
	case C4V_Int:
		return "int";
	case C4V_Bool:
		return "bool";
	case C4V_String:
		return "string";
	case C4V_Array:
		return "array";
	case C4V_PropList:
		return "proplist";
	case C4V_Any:
		return "any";
	case C4V_Object:
		return "object";
	case C4V_Def:
		return "def";
	case C4V_Effect:
		return "effect";
	case C4V_Function:
		return "function";
	default:
		return "!Fehler!";
	}
}

C4Value::C4Value(C4PropListStatic * p): C4Value(static_cast<C4PropList *>(p)) {}
C4Value::C4Value(C4Def * p): C4Value(static_cast<C4PropList *>(p)) {}
C4Value::C4Value(C4Object * p): C4Value(static_cast<C4PropList *>(p)) {}
C4Value::C4Value(C4Effect * p): C4Value(static_cast<C4PropList *>(p)) {}

C4Object * C4Value::getObj() const
{
	return CheckConversion(C4V_Object) ? Data.PropList->GetObject() : nullptr;
}

C4Object * C4Value::_getObj() const
{
	return Data.PropList ? Data.PropList->GetObject() : nullptr;
}

C4Def * C4Value::getDef() const
{
	return CheckConversion(C4V_Def) ? Data.PropList->GetDef() : nullptr;
}

C4Def * C4Value::_getDef() const
{
	return Data.PropList ? Data.PropList->GetDef() : nullptr;
}

C4Value C4VObj(C4Object *pObj) { return C4Value(static_cast<C4PropList*>(pObj)); }

bool C4Value::FnCnvObject() const
{
	// try casting
	if (Data.PropList->GetObject()) return true;
	return false;
}

bool C4Value::FnCnvDef() const
{
	// try casting
	if (Data.PropList->GetDef()) return true;
	return false;
}

bool C4Value::FnCnvEffect() const
{
	// try casting
	if (Data.PropList->GetEffect()) return true;
	return false;
}

bool C4Value::WarnAboutConversion(C4V_Type Type, C4V_Type vtToType)
{
	switch (vtToType)
	{
	case C4V_Nil:      return Type != C4V_Nil && Type != C4V_Any;
	case C4V_Int:      return Type != C4V_Int && Type != C4V_Nil && Type != C4V_Bool && Type != C4V_Any;
	case C4V_Bool:     return false;
	case C4V_PropList: return Type != C4V_PropList && Type != C4V_Effect && Type != C4V_Def && Type != C4V_Object && Type != C4V_Nil && Type != C4V_Any;
	case C4V_String:   return Type != C4V_String && Type != C4V_Nil && Type != C4V_Any;
	case C4V_Array:    return Type != C4V_Array && Type != C4V_Nil && Type != C4V_Any;
	case C4V_Function: return Type != C4V_Function && Type != C4V_Nil && Type != C4V_Any;
	case C4V_Any:      return false;
	case C4V_Def:      return Type != C4V_Def && Type != C4V_Object && Type != C4V_PropList && Type != C4V_Nil && Type != C4V_Any;
	case C4V_Object:   return Type != C4V_Object && Type != C4V_PropList && Type != C4V_Nil && Type != C4V_Any;
	case C4V_Effect:   return Type != C4V_Effect && Type != C4V_PropList && Type != C4V_Nil && Type != C4V_Any;
	default: assert(!"C4Value::ConvertTo: impossible conversion target"); return false;
	}
}

// Humanreadable debug output
StdStrBuf C4Value::GetDataString(int depth, const C4PropListStatic *ignore_reference_parent) const
{
	// ouput by type info
	switch (GetType())
	{
	case C4V_Int:
		return FormatString("%ld", static_cast<long>(Data.Int));
	case C4V_Bool:
		return StdStrBuf(Data ? "true" : "false");
	case C4V_PropList:
	{
		if (Data.PropList == ScriptEngine.GetPropList())
			return StdStrBuf("Global");
		C4Object * Obj = Data.PropList->GetObject();
		if (Obj == Data.PropList)
			return FormatString("Object(%d)", Obj->Number);
		const C4PropListStatic * Def = Data.PropList->IsStatic();
		if (Def)
			if (!ignore_reference_parent || Def->GetParent() != ignore_reference_parent)
				return Def->GetDataString();
		C4Effect * fx = Data.PropList->GetEffect();
		StdStrBuf DataString;
		DataString = (fx ? "effect {" : "{");
		Data.PropList->AppendDataString(&DataString, ", ", depth, Def && ignore_reference_parent);
		DataString.AppendChar('}');
		return DataString;
	}
	case C4V_String:
		return (Data.Str && Data.Str->GetCStr()) ? FormatString("\"%s\"", Data.Str->GetCStr()) : StdStrBuf("(nullstring)");
	case C4V_Array:
	{
		if (depth <= 0 && Data.Array->GetSize())
		{
			return StdStrBuf("[...]");
		}
		StdStrBuf DataString;
		DataString = "[";
		for (int32_t i = 0; i < Data.Array->GetSize(); i++)
		{
			if (i) DataString.Append(", ");
			DataString.Append(std::move(Data.Array->GetItem(i).GetDataString(depth - 1)));
		}
		DataString.AppendChar(']');
		return DataString;
	}
	case C4V_Function:
		return Data.Fn->GetFullName();
	case C4V_Nil:
		return StdStrBuf("nil");
	default:
		return StdStrBuf("-unknown type- ");
	}
}

// JSON serialization.
// Only plain data values can be serialized. Throws a C4JSONSerializationError
// when encountering values that cannot be represented in JSON or when the
// maximum depth is reached.
StdStrBuf C4Value::ToJSON(int depth, const C4PropListStatic *ignore_reference_parent) const
{
	// ouput by type info
	switch (GetType())
	{
	case C4V_Int:
		return FormatString("%ld", static_cast<long>(Data.Int));
	case C4V_Bool:
		return StdStrBuf(Data ? "true" : "false");
	case C4V_PropList:
	{
		const C4PropListStatic * Def = Data.PropList->IsStatic();
		if (Def)
			if (!ignore_reference_parent || Def->GetParent() != ignore_reference_parent)
				return Def->ToJSON();
		return Data.PropList->ToJSON(depth, Def && ignore_reference_parent);
	}
	case C4V_String:
		if (Data.Str && Data.Str->GetCStr())
		{
			StdStrBuf str = Data.Str->GetData();
			str.EscapeString();
			str.Replace("\n", "\\n");
			return FormatString("\"%s\"", str.getData());
		}
		else
		{
			return StdStrBuf("null");
		}
	case C4V_Array:
	{
		if (depth <= 0 && Data.Array->GetSize())
		{
			throw C4JSONSerializationError("maximum depth reached");
		}
		StdStrBuf DataString;
		DataString = "[";
		for (int32_t i = 0; i < Data.Array->GetSize(); i++)
		{
			if (i) DataString.Append(",");
			DataString.Append(std::move(Data.Array->GetItem(i).ToJSON(depth - 1)));
		}
		DataString.AppendChar(']');
		return DataString;
	}
	case C4V_Function:
		throw C4JSONSerializationError("cannot serialize function");
	case C4V_Nil:
		return StdStrBuf("null");
	default:
		throw C4JSONSerializationError("unknown type");
	}
}

const C4Value & C4ValueNumbers::GetValue(uint32_t n)
{
	if (n <= LoadedValues.size())
		return LoadedValues[n - 1];
	LogF("ERROR: Value number %d is missing.", n);
	return C4VNull;
}

void C4Value::Denumerate(class C4ValueNumbers * numbers)
{
	switch (Type)
	{
	case C4V_Enum:
		Set(numbers->GetValue(Data.Int)); break;
	case C4V_Array:
		Data.Array->Denumerate(numbers); break;
	case C4V_PropList:
		// objects and effects are denumerated via the main object list
		if (!Data.PropList->IsNumbered() && !Data.PropList->IsStatic())
			Data.PropList->Denumerate(numbers);
		break;
	case C4V_C4ObjectEnum:
		{
			C4PropList *pObj = C4PropListNumbered::GetByNumber(Data.Int);
			if (pObj)
				// set
				SetPropList(pObj);
			else
			{
				// object: invalid value - set to zero
				LogF("ERROR: Object number %d is missing.", int(Data.Int));
				Set0();
			}
		}
	default: break;
	}
}

void C4ValueNumbers::Denumerate()
{
	for (std::vector<C4Value>::iterator i = LoadedValues.begin(); i != LoadedValues.end(); ++i)
		i->Denumerate(this);
}

uint32_t C4ValueNumbers::GetNumberForValue(C4Value * v)
{
	// This is only used for C4Values containing pointers
	// Assume that all pointers have the same size
	if (ValueNumbers.find(v->GetData()) == ValueNumbers.end())
	{
		ValuesToSave.push_back(v);
		ValueNumbers[v->GetData()] = ValuesToSave.size();
		return ValuesToSave.size();
	}
	return ValueNumbers[v->GetData()];
}

void C4Value::CompileFunc(StdCompiler *pComp, C4ValueNumbers * numbers)
{
	// Type
	bool deserializing = pComp->isDeserializer();
	char cC4VID;
	if (!deserializing)
	{
		assert(Type != C4V_Nil || !Data);
		switch (Type)
		{
		case C4V_Nil:
			cC4VID = 'n'; break;
		case C4V_Int:
			cC4VID = 'i'; break;
		case C4V_Bool:
			cC4VID = 'b'; break;
		case C4V_PropList:
			if (getPropList()->IsStatic())
				cC4VID = 'D';
			else if (getPropList()->IsNumbered())
				cC4VID = 'O';
			else
				cC4VID = 'E';
			break;
		case C4V_Array:
			cC4VID = 'E'; break;
		case C4V_Function:
			cC4VID = 'D'; break;
		case C4V_String:
			cC4VID = 's'; break;
		default:
			assert(false);
		}
	}
	pComp->Character(cC4VID);
	// Data
	int32_t iTmp;
	switch (cC4VID)
	{
	case 'i':
		iTmp = Data.Int;
		pComp->Value(iTmp);
		SetInt(iTmp);
		break;

	case 'b':
		iTmp = Data.Int;
		pComp->Value(iTmp);
		SetBool(!!iTmp);
		break;

	case 'E':
		if (!deserializing)
			iTmp = numbers->GetNumberForValue(this);
		pComp->Value(iTmp);
		if (deserializing)
		{
			Data.Int = iTmp; // must be denumerated later
			Type = C4V_Enum;
		}
		break;

	case 'O':
		if (!deserializing)
			iTmp = getPropList()->GetPropListNumbered()->Number;
		pComp->Value(iTmp);
		if (deserializing)
		{
			Data.Int = iTmp; // must be denumerated later
			Type = C4V_C4ObjectEnum;
		}
		break;

	case 'D':
	{
		if (!pComp->isDeserializer())
		{
			const C4PropList * p = getPropList();
			if (getFunction())
			{
				p = Data.Fn->Parent;
				assert(p);
				assert(p->GetFunc(Data.Fn->GetName()) == Data.Fn);
				assert(p->IsStatic());
			}
			p->IsStatic()->RefCompileFunc(pComp, numbers);
			if (getFunction())
			{
				pComp->Separator(StdCompiler::SEP_PART);
				StdStrBuf s; s.Ref(Data.Fn->GetName());
				pComp->Value(mkParAdapt(s, StdCompiler::RCT_ID));
			}
		}
		else
		{
			StdStrBuf s;
			C4Value temp;
			pComp->Value(mkParAdapt(s, StdCompiler::RCT_ID));
			if (!::ScriptEngine.GetGlobalConstant(s.getData(), &temp))
				pComp->excCorrupt("Cannot find global constant %s", s.getData());
			while(pComp->Separator(StdCompiler::SEP_PART))
			{
				C4PropList * p = temp.getPropList();
				if (!p)
					pComp->excCorrupt("static proplist %s is not a proplist anymore", s.getData());
				pComp->Value(mkParAdapt(s, StdCompiler::RCT_ID));
				C4String * c4s = ::Strings.FindString(s.getData());
				if (!c4s || !p->GetPropertyByS(c4s, &temp))
					pComp->excCorrupt("Cannot find property %s in %s", s.getData(), GetDataString().getData());
			}
			Set(temp);
		}
		break;
	}

	case 's':
	{
		StdStrBuf s;
		if (!deserializing)
			s = Data.Str->GetData();
		pComp->Value(s);
		if (deserializing)
			SetString(::Strings.RegString(s));
		break;
	}

	// FIXME: remove these three once Game.txt were re-saved with current version
	case 'c':
		if (deserializing)
			Set(GameScript.ScenPropList);
		break;

	case 't':
		if (deserializing)
			Set(GameScript.ScenPrototype);
		break;

	case 'g':
		if (deserializing)
			SetPropList(ScriptEngine.GetPropList());
		break;

	case 'n':
	case 'A': // compat with OC 5.1
		if (deserializing)
			Set0();
		// doesn't have a value, so nothing to store
		break;

	default:
		// shouldn't happen
		pComp->excCorrupt("unknown C4Value type tag '%c'", cC4VID);
		break;
	}
}

void C4ValueNumbers::CompileValue(StdCompiler * pComp, C4Value * v)
{
	// Type
	bool deserializing = pComp->isDeserializer();
	char cC4VID;
	switch(v->GetType())
	{
	case C4V_PropList: cC4VID = 'p'; break;
	case C4V_Array:    cC4VID = 'a'; break;
	default: assert(deserializing); break;
	}
	pComp->Character(cC4VID);
	pComp->Separator(StdCompiler::SEP_START);
	switch(cC4VID)
	{
	case 'p':
		{
			C4PropList * p = v->_getPropList();
			pComp->Value(mkParAdapt(mkPtrAdaptNoNull(p), this));
			if (deserializing) v->SetPropList(p);
		}
		break;
	case 'a':
		{
			C4ValueArray * a = v->_getArray();
			pComp->Value(mkParAdapt(mkPtrAdaptNoNull(a), this));
			if (deserializing) v->SetArray(a);
		}
		break;
	default:
		pComp->excCorrupt("Unexpected character '%c'", cC4VID);
		break;
	}
	pComp->Separator(StdCompiler::SEP_END);
}

void C4ValueNumbers::CompileFunc(StdCompiler * pComp)
{
	bool deserializing = pComp->isDeserializer();
	bool fNaming = pComp->hasNaming();
	if (deserializing)
	{
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
				LoadedValues.push_back(C4Value());
				CompileValue(pComp, &LoadedValues.back());
			}
			catch (StdCompiler::NotFoundException *pEx)
			{
				// No value found: Stop reading loop
				delete pEx;
				break;
			}
		}
		while (pComp->Separator(StdCompiler::SEP_SEP));
	}
	else
	{
		// Note: the list grows during this loop due to nested data structures.
		// Data structures with loops are fine because the beginning of the loop
		// will be found in the map and not saved again.
		// This may still work with the binary compilers due to double-compiling
		if (!fNaming)
		{
			int32_t iSize = ValuesToSave.size();
			pComp->Value(iSize);
		}
		for(std::list<C4Value *>::iterator i = ValuesToSave.begin(); i != ValuesToSave.end(); ++i)
		{
			CompileValue(pComp, *i);
			if (i != ValuesToSave.end()) pComp->Separator(StdCompiler::SEP_SEP);
		}
	}
}

inline bool ComparisonImpl(const C4Value &Value1, const C4Value &Value2)
{
	C4V_Type Type1 = Value1.GetType();
	C4V_Data Data1 = Value1.GetData();
	C4V_Type Type2 = Value2.GetType();
	C4V_Data Data2 = Value2.GetData();
	switch (Type1)
	{
	case C4V_Nil:
		assert(!Data1);
		return Type1 == Type2;
	case C4V_Int:
	case C4V_Bool:
		return (Type2 == C4V_Int || Type2 == C4V_Bool) &&
		       Data1.Int == Data2.Int;
	case C4V_PropList:
		return Type1 == Type2 && *Data1.PropList == *Data2.PropList;
	case C4V_String:
		return Type1 == Type2 && Data1.Str == Data2.Str;
	case C4V_Array:
		return Type1 == Type2 &&
		       (Data1.Array == Data2.Array || *(Data1.Array) == *(Data2.Array));
	case C4V_Function:
		return Type1 == Type2 && Data1.Fn == Data2.Fn;
	default:
		assert(!"Unexpected C4Value type (denumeration missing?)");
		return Data1 == Data2;
	}
}

bool C4Value::operator == (const C4Value& Value2) const
{
	// recursion guard using a linked list of Seen structures on the stack
	// NOT thread-safe
	struct Seen
	{
		Seen *prev;
		const C4Value *left;
		const C4Value *right;
		inline Seen(Seen *prev, const C4Value *left, const C4Value *right):
			prev(prev), left(left), right(right) {}
		inline bool operator == (const Seen& other)
		{
			return left == other.left && right == other.right;
		}
		inline bool recursion(Seen *new_top)
		{
			for (Seen *s = this; s; s = s->prev)
				if (*s == *new_top)
					return true;
			return false;
		}
		inline Seen *first()
		{
			Seen *s = this;
			while (s->prev) s = s->prev;
			return s;
		}
	};
	static Seen *top = nullptr;
	Seen here(top, this, &Value2);
	
	bool recursion = top && top->recursion(&here);
	if (recursion)
	{
		Seen *first = top->first();
		// GetDataString is fine for circular values
		LogF("Caught infinite recursion comparing %s and %s",
			first->left->GetDataString().getData(),
			first->right->GetDataString().getData());
		return false;
	}
	top = &here;
	bool result = ComparisonImpl(*this, Value2);
	top = here.prev;
	return result;
}

bool C4Value::operator != (const C4Value& Value2) const
{
	return !(*this == Value2);
}

C4V_Type C4Value::GetTypeEx() const
{
	// Return type including types derived from prop list types (such as C4V_Def)
	if (Type == C4V_PropList)
	{
		if (FnCnvEffect()) return C4V_Effect;
		if (FnCnvObject()) return C4V_Object;
		if (FnCnvDef()) return C4V_Def;
	}
	return Type;
}

void C4Value::LogDeletedObjectWarning(C4PropList * p)
{
	if (p->GetPropListNumbered())
		LogF("Warning: using deleted object (#%d) (%s)!", p->GetPropListNumbered()->Number, p->GetName());
	else
		LogF("Warning: using deleted proplist %p (%s)!", static_cast<void*>(p), p->GetName());
	AulExec.LogCallStack();
}
