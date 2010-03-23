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

C4Value::~C4Value()
{
	// resolve all C4Values referencing this Value
	while(FirstRef)
		FirstRef->Set(*this);

	// delete contents
	DelDataRef(Data, Type, GetNextRef(), GetBaseArray(), Type == C4V_PropListRef ? PropListRefKey : NULL);
}

C4Value &C4Value::operator = (const C4Value &nValue)
{
	// set referenced value
	GetRefVal().Set(nValue.GetRefVal());

	return *this;
}

void C4Value::AddDataRef()
{
	assert(Type != C4V_Any || !Data);
	switch (Type)
	{
		case C4V_Ref: Data.Ref->AddRef(this); break;
		case C4V_PropListRef: Data.PropList->IncElementRef(); PropListRefKey->IncRef(); break;
		case C4V_Array: Data.Array = Data.Array->IncRef(); break;
		case C4V_String: Data.Str->IncRef(); break;
		case C4V_C4Object:
		case C4V_PropList:
		Data.PropList->AddRef(this);
#ifdef _DEBUG
		// check if the object actually exists
		if(Data.PropList->GetPropListNumbered() && !::Objects.ObjectNumber(Data.PropList))
			{ LogF("Warning: using wild object ptr %p!", static_cast<void*>(Data.PropList)); }
		else if(!Data.PropList->Status)
			{ LogF("Warning: using ptr on deleted object %p (%s)!", static_cast<void*>(Data.PropList), Data.PropList->GetName()); }
#endif
		break;
		default: break;
	}
}

void C4Value::DelDataRef(C4V_Data Data, C4V_Type Type, C4Value * pNextRef, C4ValueArray * pBaseArray, C4String * Key)
{
	// clean up
	switch (Type)
	{
		case C4V_Ref:
		// Save because AddDataRef does not set this flag
		HasBaseArray = false;
		Data.Ref->DelRef(this, pNextRef, pBaseArray);
		break;
		case C4V_PropListRef: Data.PropList->DecElementRef(); Key->DecRef(); break;
		case C4V_C4Object: case C4V_PropList: Data.PropList->DelRef(this, pNextRef); break;
		case C4V_Array: Data.Array->DecRef(); break;
		case C4V_String: Data.Str->DecRef(); break;
		default: break;
	}
}

void C4Value::Set(C4V_Data nData, C4V_Type nType, C4String * nPropListRefKey)
{
	assert(nType != C4V_Any || !nData);
	assert(nType != C4V_PropListRef || nPropListRefKey);
	// Do not add this to the same linked list twice.
	if (Data == nData && Type == nType) return;

	C4V_Data oData = Data;
	C4V_Type oType = Type;
	C4Value * oNextRef = NextRef;
	C4ValueArray * oBaseArray = BaseArray;
	C4String * oKey = PropListRefKey;
	bool oHasBaseArray = HasBaseArray;

	// change
	PropListRefKey = nPropListRefKey; // This will be overwritten if the union is needed for something else
	Data = nData;
	Type = nData || IsNullableType(nType) ? nType : C4V_Any;

	// hold
	AddDataRef();

	// clean up
	DelDataRef(oData, oType,
	           oHasBaseArray ? NULL : oNextRef,
	           oHasBaseArray ? oBaseArray : NULL,
	           oType == C4V_PropListRef ? oKey : NULL);
}

void C4Value::Set0()
{
	C4V_Data oData = Data;
	C4V_Type oType = Type;

	// change
	Data = 0;
	Type = C4V_Any;

	// clean up (save even if Data was 0 before)
	DelDataRef(oData, oType,
	           HasBaseArray ? NULL : NextRef,
	           HasBaseArray ? BaseArray : NULL,
	           oType == C4V_PropListRef ? PropListRefKey : NULL);
}

void C4Value::SetPropListRef(C4PropList * PropList, C4String * Key)
{
	assert(PropList);
	assert(Key);
	C4V_Data d; d.PropList = PropList;
	Set(d, C4V_PropListRef, Key);
}

void C4Value::Move(C4Value *nValue)
{
	nValue->Set(*this);

	// change references
	for(C4Value *pVal = FirstRef; pVal; pVal = pVal->GetNextRef())
		pVal->Data.Ref = nValue;

	// copy ref list
	assert(!nValue->FirstRef);
	nValue->FirstRef = FirstRef;

	// delete usself
	FirstRef = NULL;
	Set0();
}

void C4Value::GetArrayElement(int32_t Index, C4Value & target, C4AulContext *pctx, bool noref)
{
	if (noref)
	{
		const C4Value & Ref = GetRefValConst();
		// No array (and no nullpointer because Data==0 => Type==any)
		if (Ref.Type != C4V_Array)
			throw new C4AulExecError(pctx->Obj, "Array access: array expected");
		// Get the item, but do not resize the array - it might be used more than once
		if (Index < Ref.Data.Array->GetSize())
			target.Set(Ref.Data.Array->GetItem(Index));
		else
			target.Set0();
	}
	else
	{
		C4Value & Ref = GetRefVal();
		// No array (and no nullpointer because Data==0 => Type==any)
		if (Ref.Type != C4V_Array)
			throw new C4AulExecError(pctx->Obj, "Array access: array expected");
		// Is target the first ref?
		if (Index >= Ref.Data.Array->GetSize() || !Ref.Data.Array->GetItem(Index).FirstRef)
		{
			Ref.Data.Array = Ref.Data.Array->IncElementRef();
			target.SetRef(&Ref.Data.Array->GetItem(Index));
			if (target.Type == C4V_Ref)
			{
				assert(!target.NextRef);
				target.BaseArray = Ref.Data.Array;
				target.HasBaseArray = true;
			}
			// else target apparently owned the last reference to the array
		}
		else
		{
			target.SetRef(&Ref.Data.Array->GetItem(Index));
		}
	}
}

void C4Value::SetArrayLength(int32_t size, C4AulContext *cthr)
{
	C4Value & Ref = GetRefVal();
	// No array
	if (Ref.Type != C4V_Array)
		throw new C4AulExecError(cthr->Obj, "SetLength: array expected");
	Ref.Data.Array = Ref.Data.Array->SetLength(size);
}

const C4Value & C4Value::GetRefVal() const
{
	const C4Value* pVal = this;
	while(1)
	{
		if(pVal->Type == C4V_Ref)
			pVal = pVal->Data.Ref;
		else if(pVal->Type == C4V_PropListRef)
			pVal = pVal->Data.PropList->GetRefToPropertyConst(pVal->PropListRefKey);
		else
			return *pVal;
	}
}

C4Value &C4Value::GetRefVal()
{
	C4Value* pVal = this;
	while(1)
	{
		if(pVal->Type == C4V_Ref)
			pVal = pVal->Data.Ref;
		else if(pVal->Type == C4V_PropListRef)
			pVal = pVal->Data.PropList->GetRefToProperty(pVal->PropListRefKey);
		else
			return *pVal;
	}
}

void C4Value::AddRef(C4Value *pRef)
{
	pRef->NextRef = FirstRef;
	FirstRef = pRef;
}

void C4Value::DelRef(const C4Value *pRef, C4Value * pNextRef, C4ValueArray * pBaseArray)
{
	if(pRef == FirstRef)
		FirstRef = pNextRef;
	else
	{
		C4Value* pVal = FirstRef;
		while(pVal->NextRef != pRef)
		{
			// assert that pRef really was in the list
			assert(pVal->NextRef && !pVal->HasBaseArray);
			pVal = pVal->NextRef;
		}
		pVal->NextRef = pNextRef;
		if (pBaseArray)
		{
			pVal->HasBaseArray = true;
			pVal->BaseArray = pBaseArray;
		}
	}
	// Was pRef the last ref to an array element?
	if (pBaseArray && !FirstRef)
	{
		pBaseArray->DecElementRef();
	}
}

const char* GetC4VName(const C4V_Type Type)
{
	switch(Type)
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
	case C4V_Ref:
		return "&";
	default:
		return "!Fehler!";
	}
}

char GetC4VID(const C4V_Type Type)
{
	switch(Type)
	{
	case C4V_Any:
		return 'A';
	case C4V_Int:
		return 'i';
	case C4V_Bool:
		return 'b';
	case C4V_C4Object:
		return 'o';
	case C4V_String:
		return 's';
	case C4V_C4ObjectEnum:
		return 'O';
	case C4V_C4DefEnum:
		return 'D';
	case C4V_Array:
		return 'a';
	case C4V_PropList:
		return 'p';
	default:
		assert(Type != C4V_Ref && Type != C4V_PropListRef);
		assert(false);
	}
	return ' ';
}

C4V_Type GetC4VFromID(const char C4VID)
{
	switch(C4VID)
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
	case 'V':
		return C4V_Ref;
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

// converter functions ----------------

static bool FnCnvError(C4Value *Val, C4V_Type toType)
	{
	// deny convert
	return false;
	}

static bool FnCnvDeref(C4Value *Val, C4V_Type toType)
	{
	// resolve reference of Value
	Val->Deref();
	// retry to check convert
	return Val->ConvertTo(toType);
	}

bool C4Value::FnCnvPLR(C4Value *Val, C4V_Type toType)
	{
	// resolve reference of Value
	Val->SetRef(Val->Data.PropList->GetRefToProperty(Val->PropListRefKey));
	// retry to check convert
	return Val->ConvertTo(toType);
	}

static bool FnOk0(C4Value *Val, C4V_Type toType)
	{
	// 0 can be treated as nil, but every other integer can't
	return !*Val;
	}

bool C4Value::FnCnvObject(C4Value *Val, C4V_Type toType)
	{
	// try casting
	if (dynamic_cast<C4Object *>(Val->Data.PropList)) return true;
	return false;
	}

// Type conversion table
#define CnvOK        0, false           // allow conversion by same value
#define CnvOK0       FnOk0, true
#define CnvError     FnCnvError, true
#define CnvDeref     FnCnvDeref, false
#define CnvPLR       FnCnvPLR, false
#define CnvObject    FnCnvObject, false

C4VCnvFn C4Value::C4ScriptCnvMap[C4V_Last+1][C4V_Last+1] = {
	{ // C4V_Any - is always 0, convertible to everything
		{ CnvOK		}, // any        same
		{ CnvOK		}, // int
		{ CnvOK		}, // Bool
		{ CnvOK		}, // PropList
		{ CnvOK		}, // C4Object
		{ CnvOK		}, // String
		{ CnvOK		}, // Array
		{ CnvError	}, // Ref
		{ CnvError	}, // PropListRef
	},
	{ // C4V_Int
		{ CnvOK		}, // any
		{ CnvOK		}, // int        same
		{ CnvOK		}, // Bool
		{ CnvOK0	}, // PropList   only if 0
		{ CnvOK0	}, // C4Object   only if 0
		{ CnvOK0	}, // String     only if 0
		{ CnvOK0	}, // Array      only if 0
		{ CnvError	}, // Ref
		{ CnvError	}, // PropListRef
	},
	{ // C4V_Bool
		{ CnvOK		}, // any
		{ CnvOK		}, // int        might be used
		{ CnvOK		}, // Bool       same
		{ CnvError	}, // PropList   NEVER!
		{ CnvError	}, // C4Object   NEVER!
		{ CnvError	}, // String     NEVER!
		{ CnvError	}, // Array      NEVER!
		{ CnvError	}, // Ref
		{ CnvError	}, // PropListRef
	},
	{ // C4V_PropList
		{ CnvOK		}, // any
		{ CnvError	}, // int        NEVER!
		{ CnvOK		}, // Bool
		{ CnvOK		}, // PropList   same
		{ CnvObject	}, // C4Object
		{ CnvError	}, // String     NEVER!
		{ CnvError	}, // Array      NEVER!
		{ CnvError	}, // Ref   NEVER!
		{ CnvError	}, // PropListRef
	},
	{ // C4V_Object
		{ CnvOK		}, // any
		{ CnvError	}, // int        NEVER!
		{ CnvOK		}, // Bool
		{ CnvOK		}, // PropList
		{ CnvOK		}, // C4Object   same
		{ CnvError	}, // String     NEVER!
		{ CnvError	}, // Array      NEVER!
		{ CnvError	}, // Ref   NEVER!
		{ CnvError	}, // PropListRef
	},
	{ // C4V_String
		{ CnvOK		}, // any
		{ CnvError	}, // int        NEVER!
		{ CnvOK		}, // Bool
		{ CnvError	}, // PropList   NEVER!
		{ CnvError	}, // C4Object   NEVER!
		{ CnvOK		}, // String     same
		{ CnvError	}, // Array      NEVER!
		{ CnvError	}, // Ref   NEVER!
		{ CnvError	}, // PropListRef
	},
	{ // C4V_Array
		{ CnvOK		}, // any
		{ CnvError	}, // int        NEVER!
		{ CnvOK		}, // Bool
		{ CnvError	}, // PropList   NEVER!
		{ CnvError	}, // C4Object   NEVER!
		{ CnvError	}, // String     NEVER!
		{ CnvOK		}, // Array      same
		{ CnvError	}, // Ref   NEVER!
		{ CnvError	}, // PropListRef
	},
	{ // C4V_Ref - resolve reference and retry type check
		{ CnvDeref	}, // any
		{ CnvDeref	}, // int
		{ CnvDeref	}, // Bool
		{ CnvDeref	}, // PropList
		{ CnvDeref	}, // C4Object
		{ CnvDeref	}, // String
		{ CnvDeref	}, // Array
		{ CnvOK		}, // Ref   same
		{ CnvError	}, // PropListRef
	},
	{ // C4V_Ref - resolve reference and retry type check
		{ CnvPLR	}, // any
		{ CnvPLR	}, // int
		{ CnvPLR	}, // Bool
		{ CnvPLR	}, // PropList
		{ CnvPLR	}, // C4Object
		{ CnvPLR	}, // String
		{ CnvPLR	}, // Array
		{ CnvPLR	}, // Ref
		{ CnvOK		}, // PropListRef   same
	},
};

#undef CnvOK
#undef CnvOK0
#undef CnvError
#undef CnvDeref
#undef CnvPLR
#undef CnvObject

// Humanreadable debug output
StdStrBuf C4Value::GetDataString()
{
	if (Type == C4V_Ref)
		return GetRefVal().GetDataString() + "*";

	// ouput by type info
	switch(GetType())
	{
	case C4V_Int:
		return FormatString("%ld", Data.Int);
	case C4V_Bool:
		return StdStrBuf(Data ? "true" : "false");
	case C4V_C4Object:
	case C4V_PropList:
		{
		// obj exists?
		if(!::Objects.ObjectNumber(Data.PropList))
			return FormatString("%ld", Data.Int);
		else
			if (Data.PropList)
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
			for(int32_t i = 0; i < Data.Array->GetSize(); i++)
			{
				if(i) DataString.Append(", ");
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
	if(!strString) return C4Value();
	return C4Value(::Strings.RegString(strString));
}

C4Value C4VString(StdStrBuf Str)
{
	// safety
	if(Str.isNull()) return C4Value();
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
	if(Type != C4V_C4ObjectEnum) return;
	// get obj id, search object
	int iObjID = Data.Int;
	C4PropList *pObj = Objects.PropListPointer(iObjID);
	if(pObj)
		// set
		SetPropList(pObj);
	else
	{
		// object: invalid value - set to zero
		LogF("ERROR: Object number %d is missing.", Data.Int);
		Set0();
	}
}

void C4Value::CompileFunc(StdCompiler *pComp)
	{
	// Type
	bool fCompiler = pComp->isCompiler();
	if(!fCompiler)
		{
		// Get type
		assert(Type != C4V_Any || !Data);
		char cC4VID = GetC4VID(Type);
		// Object reference is saved enumerated
		if(Type == C4V_C4Object)
			cC4VID = GetC4VID(C4V_C4ObjectEnum);
		if(Type == C4V_PropList && getPropList()->GetDef())
			cC4VID = GetC4VID(C4V_C4DefEnum);
		else if(Type == C4V_PropList)
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
		catch(StdCompiler::NotFoundException *pExc)
			{
			delete pExc;
			cC4VID = 'A';
			}
		// old style string
		if (cC4VID == 'S')
			{
			int32_t iTmp;
			pComp->Value(iTmp);
			// search
			C4String *pString = ::Strings.FindString(iTmp);
			if(pString)
				{
				Data.Str = pString;
				pString->IncRef();
				}
			else
				Type = C4V_Any;
			return;
			}
		Type = GetC4VFromID(cC4VID);
		}
	// Data
	int32_t iTmp;
	switch(Type)
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
		if (Type == C4V_PropList && p->GetDef())
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
		pComp->Value(id); // must be denumerated later
		Data.PropList = Definitions.ID2Def(id);
		Type = C4V_PropList;
		if(!Data.PropList)
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

	// string: save string number
	case C4V_String:
		{
		// search
		StdStrBuf s;
		if (!fCompiler) s = Data.Str->GetData();
		pComp->Value(s);
		if(fCompiler)
			{
			C4String *pString = ::Strings.RegString(s);
			if(pString)
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
		pComp->Seperator(StdCompiler::SEP_START2);
		pComp->Value(mkPtrAdapt(Data.Array, false));
		if (fCompiler) Data.Array = Data.Array->IncRef();
		pComp->Seperator(StdCompiler::SEP_END2);
		break;

	case C4V_Any:
		assert(!Data);
		// doesn't have a value, so nothing to store
		break;

	// shouldn't happen
	case C4V_Ref:
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
			return Data == Value2.Data && Type == Value2.Type;
		case C4V_String:
			return Type == Value2.Type && Data.Str == Value2.Data.Str;
		case C4V_Array:
			return Type == Value2.Type && *(Data.Array) == *(Value2.Data.Array);
		default:
			// C4AulExec should have dereferenced both values, no need to implement comparison here
			return Data == Value2.Data;
		}
	return GetData() == Value2.GetData();
	}

bool C4Value::operator != (const C4Value& Value2) const
{
	return !(*this == Value2);
}

C4Value C4VID(C4ID iVal) { return C4Value(::Definitions.ID2Def(iVal)); }
C4ID C4Value::getC4ID()
{
	C4PropList * p = getPropList();
	if(!p) return C4ID::None;
	C4Def * d = p->GetDef();
	if (!d) return C4ID::None;
	return d->id;
}
