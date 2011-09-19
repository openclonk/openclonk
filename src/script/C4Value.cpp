/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2002, 2004-2006  Peter Wortmann
 * Copyright (c) 2001, 2005-2006  Sven Eberhardt
 * Copyright (c) 2006-2011  Günther Brammer
 * Copyright (c) 2007  Matthes Bender
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

#include <C4Include.h>
#include <C4Value.h>

#include <C4AulExec.h>
#include <C4DefList.h>
#include <C4StringTable.h>
#include <C4ValueArray.h>
#include <C4Game.h>
#include <C4GameObjects.h>
#include <C4Object.h>
#include <C4Log.h>

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
	case C4V_C4Object:
		return "object";
	case C4V_String:
		return "string";
	case C4V_Array:
		return "array";
	case C4V_PropList:
		return "proplist";
	case C4V_Any:
		return "any";
	default:
		return "!Fehler!";
	}
}

const char* C4Value::GetTypeInfo()
{
	return GetC4VName(GetType());
}

bool C4Value::FnCnvObject() const
{
	// try casting
	if (Data.PropList->GetObject()) return true;
	return false;
}


bool C4Value::WarnAboutConversion(C4V_Type Type, C4V_Type vtToType)
{
	switch (vtToType)
	{
	case C4V_Nil:      return Type != C4V_Nil && Type != C4V_Any;
	case C4V_Int:      return Type != C4V_Int && Type != C4V_Nil && Type != C4V_Bool && Type != C4V_Any;
	case C4V_Bool:     return false;
	case C4V_PropList: return Type != C4V_PropList && Type != C4V_C4Object && Type != C4V_Nil && Type != C4V_Any;
	case C4V_C4Object: return Type != C4V_C4Object && Type != C4V_PropList && Type != C4V_Nil && Type != C4V_Any;
	case C4V_String:   return Type != C4V_String && Type != C4V_Nil && Type != C4V_Any;
	case C4V_Array:    return Type != C4V_Array && Type != C4V_Nil && Type != C4V_Any;
	case C4V_Any:      return false;
	default: assert(!"C4Value::ConvertTo: impossible conversion target"); return false;
	}
}

// Humanreadable debug output
StdStrBuf C4Value::GetDataString(int depth) const
{
	// ouput by type info
	switch (GetType())
	{
	case C4V_Int:
		return FormatString("%ld", static_cast<long>(Data.Int));
	case C4V_Bool:
		return StdStrBuf(Data ? "true" : "false");
	case C4V_C4Object:
	case C4V_PropList:
	{
		StdStrBuf DataString;
		DataString = "{";
		if (Data.PropList->GetObject())
		{
			if (Data.Obj->Status == C4OS_NORMAL)
				DataString.AppendFormat("#%d, ", Data.Obj->Number);
			else
				DataString.AppendFormat("(#%d), ", Data.Obj->Number);
		}
		else if (Data.PropList->GetDef())
			DataString.AppendFormat("%s, ", Data.PropList->GetDef()->id.ToString());
		Data.PropList->AppendDataString(&DataString, ", ", depth);
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
	case C4V_Nil:
		return StdStrBuf("nil");
	default:
		return StdStrBuf("-unknown type- ");
	}
}

C4Value C4VString(const char *strString)
{
	// safety
	if (!strString) return C4Value();
	return C4Value(::Strings.RegString(strString));
}

C4Value C4VString(StdStrBuf Str)
{
	// safety
	if (Str.isNull()) return C4Value();
	return C4Value(::Strings.RegString(Str));
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
		if (!Data.PropList->IsNumbered() && !Data.PropList->IsDef())
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
	if (ValueNumbers.find(v->_getObj()) == ValueNumbers.end())
	{
		ValuesToSave.push_back(v);
		ValueNumbers[v->_getObj()] = ValuesToSave.size();
		return ValuesToSave.size();
	}
	return ValueNumbers[v->_getObj()];
}

static char GetC4VID(const C4V_Type Type)
{
	switch (Type)
	{
	case C4V_Nil:
		return 'n';
	case C4V_Int:
		return 'i';
	case C4V_Bool:
		return 'b';
	case C4V_PropList:
	case C4V_Array:
	case C4V_Enum:
		return 'E';
	case C4V_C4Object:
	case C4V_C4ObjectEnum:
		return 'O';
	case C4V_String:
		return 's';
	case C4V_C4DefEnum:
		return 'D';
	default:
		assert(false);
	}
	return ' ';
}

static C4V_Type GetC4VFromID(const char C4VID)
{
	switch (C4VID)
	{
	case 'n':
	case 'A': // compat with OC 5.1
		return C4V_Nil;
	case 'i':
		return C4V_Int;
	case 'b':
		return C4V_Bool;
	case 's':
		return C4V_String;
	case 'O':
		return C4V_C4ObjectEnum;
	case 'D':
		return C4V_C4DefEnum;
	case 'E':
		return C4V_Enum;
	}
	return C4V_Any;
}

void C4Value::CompileFunc(StdCompiler *pComp, C4ValueNumbers * numbers)
{
	// Type
	bool fCompiler = pComp->isCompiler();
	if (!fCompiler)
	{
		// Get type
		assert(Type != C4V_Nil || !Data);
		char cC4VID = GetC4VID(Type);
		// special cases:
		if (Type == C4V_PropList && getPropList()->IsDef())
			cC4VID = GetC4VID(C4V_C4DefEnum);
		else if (Type == C4V_PropList && getPropList()->IsNumbered())
			cC4VID = GetC4VID(C4V_C4ObjectEnum);
		// Write
		pComp->Character(cC4VID);
	}
	else
	{
		// Clear
		Set0();
		// Read type
		char cC4VID;
		try
		{
			pComp->Character(cC4VID);
		}
		catch (StdCompiler::NotFoundException *pExc)
		{
			delete pExc;
			cC4VID = 'n';
		}
		Type = GetC4VFromID(cC4VID);
		if (Type == C4V_Any)
		{
			Type = C4V_Nil;
			pComp->excCorrupt("unknown C4Value type tag '%c'", cC4VID);
		}
	}
	// Data
	int32_t iTmp;
	switch (Type)
	{

		// simple data types: just save
	case C4V_Int:
	case C4V_Bool:

		// these are 32-bit integers
		iTmp = Data.Int;
		pComp->Value(iTmp);
		Data.Int = iTmp;

		break;

		// object: save object number instead
	case C4V_C4Object: case C4V_PropList:
	{
		assert(!fCompiler);
		C4PropList * p = getPropList();
		if (p->IsDef())
			pComp->Value(p->GetDef()->id);
		else if (p->IsNumbered())
		{
			iTmp = getPropList()->GetPropListNumbered()->Number;
			pComp->Value(iTmp);
		}
		else
		{
			iTmp = numbers->GetNumberForValue(this);
			pComp->Value(iTmp);
		}
		break;
	}

	case C4V_C4ObjectEnum: case C4V_Enum:
		assert(fCompiler);
		pComp->Value(iTmp); // must be denumerated later
		Data.Int = iTmp;
		break;

	case C4V_C4DefEnum:
	{
		assert(fCompiler);
		C4ID id;
		pComp->Value(id);
		C4PropList * p = Definitions.ID2Def(id);
		if (!p)
		{
			Set0();
			pComp->Warn("ERROR: Definition %s is missing.", id.ToString());
		}
		else
		{
			SetPropList(p);
		}
		break;
	}

	case C4V_String:
	{
		// search
		StdStrBuf s;
		if (!fCompiler) s = Data.Str->GetData();
		pComp->Value(s);
		if (fCompiler)
		{
			C4String *pString = ::Strings.RegString(s);
			Data.Str = pString;
			if (pString)
			{
				pString->IncRef();
			}
			else
				Type = C4V_Nil;
		}
		break;
	}

	case C4V_Array:
		iTmp = numbers->GetNumberForValue(this);
		pComp->Value(iTmp);
		break;

	case C4V_Nil:
		assert(!Data);
		// doesn't have a value, so nothing to store
		break;

	default:
		// shouldn't happen
		assert(false);
		break;
	}
}

void C4ValueNumbers::CompileValue(StdCompiler * pComp, C4Value * v)
{
	// Type
	bool fCompiler = pComp->isCompiler();
	char cC4VID;
	switch(v->GetType())
	{
	case C4V_PropList: cC4VID = 'p'; break;
	case C4V_Array:    cC4VID = 'a'; break;
	default: assert(fCompiler); break;
	}
	pComp->Character(cC4VID);
	pComp->Separator(StdCompiler::SEP_START);
	switch(cC4VID)
	{
	case 'p':
		{
			C4PropList * p = v->_getPropList();
			pComp->Value(mkParAdapt(mkPtrAdaptNoNull(p), this));
			if (fCompiler) v->SetPropList(p);
		}
		break;
	case 'a':
		{
			C4ValueArray * a = v->_getArray();
			pComp->Value(mkParAdapt(mkPtrAdaptNoNull(a), this));
			if (fCompiler) v->SetArray(a);
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
	bool fCompiler = pComp->isCompiler();
	bool fNaming = pComp->hasNaming();
	if (fCompiler)
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

bool C4Value::operator == (const C4Value& Value2) const
{
	switch (Type)
	{
	case C4V_Any:
		assert(!Data);
		return Value2.Type == Type;
	case C4V_Int:
	case C4V_Bool:
		switch (Value2.Type)
		{
		case C4V_Int:
		case C4V_Bool:
			return Data == Value2.Data;
		default:
			return false;
		}
	case C4V_C4Object: case C4V_PropList:
		if (Value2.Type == C4V_C4Object || Value2.Type == C4V_PropList)
		{
			// Compare as equal if and only if the proplists are indistinguishable
			// If one or both are mutable, they have to be the same
			// otherwise, they have to have the same contents
			if (Data.PropList == Value2.Data.PropList) return true;
			if (!Data.PropList->IsFrozen() || !Value2.Data.PropList->IsFrozen()) return false;
			return (*Data.PropList == *Value2.Data.PropList);
		}
	case C4V_String:
		return Type == Value2.Type && Data.Str == Value2.Data.Str;
	case C4V_Array:
		return Type == Value2.Type && *(Data.Array) == *(Value2.Data.Array);
	default:
		return Data == Value2.Data;
	}
	return GetData() == Value2.GetData();
}

bool C4Value::operator != (const C4Value& Value2) const
{
	return !(*this == Value2);
}

C4ID C4Value::getC4ID() const
{
	C4PropList * p = getPropList();
	if (!p) return C4ID::None;
	C4Def * d = p->GetDef();
	if (!d) return C4ID::None;
	return d->id;
}

void C4Value::LogDeletedObjectWarning(C4PropList * p)
{
	if (p->GetPropListNumbered())
		LogF("Warning: using deleted object (#%d) (%s)!", p->GetPropListNumbered()->Number, p->GetName());
	else
		LogF("Warning: using deleted proplist %p (%s)!", static_cast<void*>(p), p->GetName());
	AulExec.LogCallStack();
}
