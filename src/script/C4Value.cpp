/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001, 2005-2006  Sven Eberhardt
 * Copyright (c) 2001-2002, 2004-2006  Peter Wortmann
 * Copyright (c) 2006-2009  Günther Brammer
 * Copyright (c) 2007  Matthes Bender
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
#include <C4StringTable.h>
#include <C4ValueList.h>

#include <C4Game.h>
#include <C4GameObjects.h>
#include <C4Object.h>
#include <C4Log.h>

const C4NullValue C4VNull = C4NullValue();
const C4Value C4VTrue = C4VBool(true);
const C4Value C4VFalse = C4VBool(false);

const char* GetC4VName(const C4V_Type Type)
{
	switch (Type)
	{
	case C4V_Any:
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
	default:
		return "!Fehler!";
	}
}

char GetC4VID(const C4V_Type Type)
{
	switch (Type)
	{
	case C4V_Any:
		return 'A';
	case C4V_Int:
		return 'i';
	case C4V_Bool:
		return 'b';
	case C4V_PropList:
		return 'p';
	case C4V_C4Object:
	case C4V_C4ObjectEnum:
		return 'O';
	case C4V_String:
		return 's';
	case C4V_C4DefEnum:
		return 'D';
	case C4V_Array:
		return 'a';
	default:
		assert(false);
	}
	return ' ';
}

C4V_Type GetC4VFromID(const char C4VID)
{
	switch (C4VID)
	{
	case 'A':
		return C4V_Any;
	case 'i':
		return C4V_Int;
	case 'b':
		return C4V_Bool;
	case 'o':
		return C4V_C4Object;
	case 's':
		return C4V_String;
	case 'O':
		return C4V_C4ObjectEnum;
	case 'D':
		return C4V_C4DefEnum;
	case 'a':
		return C4V_Array;
	case 'p':
		return C4V_PropList;
	}
	return C4V_Any;
}

const char* C4Value::GetTypeInfo()
{
	return GetC4VName(GetType());
}

bool C4Value::FnCnvObject() const
{
	// try casting
	if (dynamic_cast<C4Object *>(Data.PropList)) return true;
	return false;
}
// Type conversion table
#define CnvOK        C4VCnvFn::CnvOK, false           // allow conversion by same value
#define CnvOK0       C4VCnvFn::CnvOK0, true
#define CnvError     C4VCnvFn::CnvError, true
#define CnvObject    C4VCnvFn::CnvObject, false

C4VCnvFn C4Value::C4ScriptCnvMap[C4V_Last+1][C4V_Last+1] =
{
	{ // C4V_Any - is always 0, convertible to everything
		{ CnvOK   }, // any        same
		{ CnvOK   }, // int
		{ CnvOK   }, // Bool
		{ CnvOK   }, // PropList
		{ CnvOK   }, // C4Object
		{ CnvOK   }, // String
		{ CnvOK   }, // Array
	},
	{ // C4V_Int
		{ CnvOK   }, // any
		{ CnvOK   }, // int        same
		{ CnvOK   }, // Bool
		{ CnvOK0  }, // PropList   only if 0
		{ CnvOK0  }, // C4Object   only if 0
		{ CnvOK0  }, // String     only if 0
		{ CnvOK0  }, // Array      only if 0
	},
	{ // C4V_Bool
		{ CnvOK   }, // any
		{ CnvOK   }, // int        might be used
		{ CnvOK   }, // Bool       same
		{ CnvError  }, // PropList   NEVER!
		{ CnvError  }, // C4Object   NEVER!
		{ CnvError  }, // String     NEVER!
		{ CnvError  }, // Array      NEVER!
	},
	{ // C4V_PropList
		{ CnvOK   }, // any
		{ CnvError  }, // int        NEVER!
		{ CnvOK   }, // Bool
		{ CnvOK   }, // PropList   same
		{ CnvObject }, // C4Object
		{ CnvError  }, // String     NEVER!
		{ CnvError  }, // Array      NEVER!
	},
	{ // C4V_Object
		{ CnvOK   }, // any
		{ CnvError  }, // int        NEVER!
		{ CnvOK   }, // Bool
		{ CnvOK   }, // PropList
		{ CnvOK   }, // C4Object   same
		{ CnvError  }, // String     NEVER!
		{ CnvError  }, // Array      NEVER!
	},
	{ // C4V_String
		{ CnvOK   }, // any
		{ CnvError  }, // int        NEVER!
		{ CnvOK   }, // Bool
		{ CnvError  }, // PropList   NEVER!
		{ CnvError  }, // C4Object   NEVER!
		{ CnvOK   }, // String     same
		{ CnvError  }, // Array      NEVER!
	},
	{ // C4V_Array
		{ CnvOK   }, // any
		{ CnvError  }, // int        NEVER!
		{ CnvOK   }, // Bool
		{ CnvError  }, // PropList   NEVER!
		{ CnvError  }, // C4Object   NEVER!
		{ CnvError  }, // String     NEVER!
		{ CnvOK   }, // Array      same
	}
};

#undef CnvOK
#undef CnvOK0
#undef CnvError
#undef CnvObject

// Humanreadable debug output
StdStrBuf C4Value::GetDataString() const
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
		// obj exists?
		if (!::Objects.ObjectNumber(Data.PropList))
			return FormatString("%ld", static_cast<long>(Data.Int));
		else if (Data.PropList)
			if (Data.Obj->Status == C4OS_NORMAL)
				return FormatString("%s #%d", Data.PropList->GetName(), Objects.ObjectNumber(Data.PropList));
			else
				return FormatString("{%s #%d}", Data.PropList->GetName(), Objects.ObjectNumber(Data.PropList));
		else
			return StdStrBuf("0"); // (impossible)
	}
	case C4V_String:
		return (Data.Str && Data.Str->GetCStr()) ? FormatString("\"%s\"", Data.Str->GetCStr()) : StdStrBuf("(nullstring)");
	case C4V_Array:
	{
		StdStrBuf DataString;
		DataString = "[";
		for (int32_t i = 0; i < Data.Array->GetSize(); i++)
		{
			if (i) DataString.Append(", ");
			DataString.Append(static_cast<const StdStrBuf &>(Data.Array->GetItem(i).GetDataString()));
		}
		DataString.AppendChar(']');
		return DataString;
	}
	case C4V_Any:
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

void C4Value::DenumeratePointer()
{
	// array?
	if (Type == C4V_Array)
	{
		Data.Array->DenumeratePointers();
		return;
	}
	// object types only
	if (Type != C4V_C4ObjectEnum) return;
	// get obj id, search object
	int iObjID = Data.Int;
	C4PropList *pObj = Objects.PropListPointer(iObjID);
	if (pObj)
		// set
		SetPropList(pObj);
	else
	{
		// object: invalid value - set to zero
		LogF("ERROR: Object number %d is missing.", iObjID);
		Set0();
	}
}

void C4Value::CompileFunc(StdCompiler *pComp)
{
	// Type
	bool fCompiler = pComp->isCompiler();
	if (!fCompiler)
	{
		// Get type
		assert(Type != C4V_Any || !Data);
		char cC4VID = GetC4VID(Type);
		// special case proplists
		if (Type == C4V_PropList && getPropList()->IsDef())
			cC4VID = GetC4VID(C4V_C4DefEnum);
		else if (Type == C4V_PropList && !getPropList()->IsFrozen())
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
			cC4VID = 'A';
		}
		Type = GetC4VFromID(cC4VID);
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
		if (fCompiler || (Type == C4V_PropList && getPropList()->IsFrozen() && !getPropList()->IsDef()))
		{
			assert(Type == C4V_PropList);
			pComp->Separator(StdCompiler::SEP_START2);
			pComp->Value(mkPtrAdapt(Data.PropList, false));
			if (fCompiler) Data.PropList->AddRef(this);
			pComp->Separator(StdCompiler::SEP_END2);
			break;
		}
		assert(!fCompiler);
		C4PropList * p = getPropList();
		if (Type == C4V_PropList && p->IsDef())
			pComp->Value(p->GetDef()->id);
		else
		{
			iTmp = ::Objects.ObjectNumber(getPropList());
			pComp->Value(iTmp);
		}
		break;
	}

	case C4V_C4ObjectEnum:
		assert(fCompiler);
		pComp->Value(iTmp); // must be denumerated later
		Data.Int = iTmp;
		break;

	case C4V_C4DefEnum:
	{
		assert(fCompiler);
		C4ID id;
		pComp->Value(id);
		Data.PropList = Definitions.ID2Def(id);
		Type = C4V_PropList;
		if (!Data.PropList)
		{
			LogF("ERROR: Definition %s is missing.", id.ToString());
			Type = C4V_Any;
		}
		else
		{
			Data.PropList->AddRef(this);
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
			if (pString)
			{
				Data.Str = pString;
				pString->IncRef();
			}
			else
				Type = C4V_Any;
		}
		break;
	}

	case C4V_Array:
		pComp->Separator(StdCompiler::SEP_START2);
		pComp->Value(mkPtrAdapt(Data.Array, false));
		if (fCompiler) Data.Array->IncRef();
		pComp->Separator(StdCompiler::SEP_END2);
		break;

	case C4V_Any:
		assert(!Data);
		// doesn't have a value, so nothing to store
		break;

		// shouldn't happen
	default:
		assert(false);
		break;
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
		switch (Value2.Type)
		{
		case C4V_Int:
		case C4V_Bool:
			return Data == Value2.Data;
		default:
			return false;
		}
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
		if (Type == Value2.Type)
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

C4Value C4VID(C4ID iVal) { return C4Value(::Definitions.ID2Def(iVal)); }
C4ID C4Value::getC4ID() const
{
	C4PropList * p = getPropList();
	if (!p) return C4ID::None;
	C4Def * d = p->GetDef();
	if (!d) return C4ID::None;
	return d->id;
}
