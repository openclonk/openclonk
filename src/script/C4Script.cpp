/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2004, 2006-2007, 2010  Sven Eberhardt
 * Copyright (c) 2005-2011  Günther Brammer
 * Copyright (c) 2005-2006  Peter Wortmann
 * Copyright (c) 2009  Nicolas Hake
 * Copyright (c) 2010  Benjamin Herr
 * Copyright (c) 2010  Armin Burgmeier
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

/* Functions mapped by C4Script */

#include <C4Include.h>
#include <C4AulDefFunc.h>

#include <C4AulExec.h>
#include <C4Random.h>
#include <C4Version.h>

//========================== Some Support Functions =======================================

StdStrBuf FnStringFormat(C4PropList * _this, C4String *szFormatPar, C4Value * Pars, int ParCount)
{
	int cPar=0;

	StdStrBuf StringBuf("", false);
	const char * cpFormat = FnStringPar(szFormatPar);
	const char * cpType;
	char szField[20];
	while (*cpFormat)
	{
		// Copy normal stuff
		while (*cpFormat && (*cpFormat!='%'))
			StringBuf.AppendChar(*cpFormat++);
		// Field
		if (*cpFormat=='%')
		{
			// Scan field type
			for (cpType=cpFormat+1; *cpType && (*cpType=='.' || Inside(*cpType,'0','9')); cpType++) {}
			// Copy field
			SCopy(cpFormat,szField,Min<unsigned int>(sizeof szField - 1, cpType - cpFormat + 1));
			// Insert field by type
			switch (*cpType)
			{
				// number
			case 'd': case 'x': case 'X':
			{
				if (cPar >= ParCount) throw new C4AulExecError("format placeholder without parameter");
				StringBuf.AppendFormat(szField, Pars[cPar++].getInt());
				cpFormat+=SLen(szField);
				break;
			}
			// character
			case 'c':
			{
				if (cPar >= ParCount) throw new C4AulExecError("format placeholder without parameter");
				StringBuf.AppendCharacter(Pars[cPar++].getInt());
				cpFormat+=SLen(szField);
				break;
			}
			// C4ID
			case 'i':
			{
				if (cPar >= ParCount) throw new C4AulExecError("format placeholder without parameter");
				C4ID id = Pars[cPar++].getC4ID();
				StringBuf.Append(id.ToString());
				cpFormat+=SLen(szField);
				break;
			}
			// C4Value
			case 'v':
			{
				if (cPar >= ParCount) throw new C4AulExecError("format placeholder without parameter");
				StringBuf.Append(static_cast<const StdStrBuf&>(Pars[cPar++].GetDataString(10)));
				cpFormat+=SLen(szField);
				break;
			}
			// String
			case 's':
			{
				// get string
				if (cPar >= ParCount) throw new C4AulExecError("format placeholder without parameter");
				const char *szStr = "(null)";
				if (Pars[cPar].GetData())
				{
					C4String * pStr = Pars[cPar].getStr();
					if (!pStr) throw new C4AulExecError("string format placeholder without string");
					szStr = pStr->GetCStr();
				}
				++cPar;
				StringBuf.AppendFormat(szField, szStr);
				cpFormat+=SLen(szField);
				break;
			}
			case '%':
				StringBuf.AppendChar('%');
				cpFormat+=SLen(szField);
				break;
				// Undefined / Empty
			default:
				StringBuf.AppendChar('%');
				cpFormat++;
				break;
			}
		}
	}
	return StringBuf;
}

bool C4ValueToMatrix(C4Value& value, StdMeshMatrix* matrix)
{
	//if(value.GetType() != C4V_Array) return false;
	const C4ValueArray* array = value.getArray();
	if (!array) return false;
	return C4ValueToMatrix(*array, matrix);
}

bool C4ValueToMatrix(const C4ValueArray& array, StdMeshMatrix* matrix)
{
	if (array.GetSize() != 12) return false;

	StdMeshMatrix& trans = *matrix;
	trans(0,0) = array[0].getInt()/1000.0f;
	trans(0,1) = array[1].getInt()/1000.0f;
	trans(0,2) = array[2].getInt()/1000.0f;
	trans(0,3) = array[3].getInt()/1000.0f;
	trans(1,0) = array[4].getInt()/1000.0f;
	trans(1,1) = array[5].getInt()/1000.0f;
	trans(1,2) = array[6].getInt()/1000.0f;
	trans(1,3) = array[7].getInt()/1000.0f;
	trans(2,0) = array[8].getInt()/1000.0f;
	trans(2,1) = array[9].getInt()/1000.0f;
	trans(2,2) = array[10].getInt()/1000.0f;
	trans(2,3) = array[11].getInt()/1000.0f;

	return true;
}

C4AulDefFunc::C4AulDefFunc(C4AulScript *pOwner, C4ScriptFnDef* pDef):
		C4AulFunc(pOwner, pDef->Identifier), Def(pDef)
{
	Owner->GetPropList()->SetPropertyByS(Name, C4VFunction(this));
}

C4AulDefFunc::~C4AulDefFunc()
{
}

C4Value C4AulDefFunc::Exec(C4PropList * p, C4Value pPars[], bool fPassErrors)
{
	assert(Def->FunctionC4V);
	return Def->FunctionC4V(p, pPars);
}

//=============================== C4Script Functions ====================================

static C4PropList * FnCreatePropList(C4PropList * _this, C4PropList * prototype)
{
	return C4PropList::New(prototype);
}

static C4Value FnGetProperty(C4PropList * _this, C4String * key, C4PropList * pObj)
{
	if (!pObj) pObj = _this;
	if (!pObj) return C4VNull;
	if (!key) return C4VNull;
	C4Value r;
	pObj->GetPropertyByS(key, &r);
	return r;
}

static bool FnSetProperty(C4PropList * _this, C4String * key, const C4Value & to, C4PropList * pObj)
{
	if (!pObj) pObj = _this;
	if (!pObj) return false;
	if (!key) return false;
	if (pObj->IsFrozen())
		throw new C4AulExecError("proplist write: proplist is readonly");
	pObj->SetPropertyByS(key, to);
	return true;
}

static bool FnResetProperty(C4PropList * _this, C4String * key, C4PropList * pObj)
{
	if (!pObj) pObj = _this;
	if (!pObj) return false;
	if (!key) return false;
	if (!pObj->HasProperty(key)) return false;
	if (pObj->IsFrozen())
		throw new C4AulExecError("proplist write: proplist is readonly");
	pObj->ResetProperty(key);
	return true;
}

static C4ValueArray * FnGetProperties(C4PropList * _this, C4PropList * p)
{
	if (!p) p = _this;
	if (!p) throw new NeedNonGlobalContext("GetProperties");
	C4ValueArray * r = p->GetProperties();
	r->SortStrings();
	return r;
}

static C4Value FnCall(C4PropList * _this, C4Value * Pars)
{
	if (!_this) _this = ::ScriptEngine.GetPropList();
	C4AulParSet ParSet(&Pars[1], 9);
	C4AulFunc * fn = Pars[0].getFunction();
	C4String * name;
	if (!fn)
	{
		name = Pars[0].getStr();
		if (name) fn = _this->GetFunc(name);
	}
	if (!fn)
	{
		const char * s = FnStringPar(name);
		if (s[0] == '~')
		{
			fn = _this->GetFunc(&s[1]);
			if (!fn)
				return C4Value();
		}
	}
	if (!fn)
		throw new C4AulExecError(FormatString("Call: no function %s", Pars[0].GetDataString().getData()).getData());
	return fn->Exec(_this, &ParSet, true);
}

static C4Value FnLog(C4PropList * _this, C4Value * Pars)
{
	Log(FnStringFormat(_this, Pars[0].getStr(), &Pars[1], 9).getData());
	return C4VBool(true);
}

static C4Value FnDebugLog(C4PropList * _this, C4Value * Pars)
{
	DebugLog(FnStringFormat(_this, Pars[0].getStr(), &Pars[1], 9).getData());
	return C4VBool(true);
}

static C4Value FnFormat(C4PropList * _this, C4Value * Pars)
{
	return C4VString(FnStringFormat(_this, Pars[0].getStr(), &Pars[1], 9));
}

static C4ID FnC4Id(C4PropList * _this, C4String *szID)
{
	return(C4ID(FnStringPar(szID)));
}

static long FnAbs(C4PropList * _this, long iVal)
{
	return Abs(iVal);
}

static long FnSin(C4PropList * _this, long iAngle, long iRadius, long iPrec)
{
	if (!iPrec) iPrec = 1;
	// Precalculate the modulo operation so the C4Fixed argument to Sin does not overflow
	iAngle %= 360 * iPrec;
	// Let itofix and fixtoi handle the division and multiplication because that can handle higher ranges
	return fixtoi(Sin(itofix(iAngle, iPrec)), iRadius);
}

static long FnCos(C4PropList * _this, long iAngle, long iRadius, long iPrec)
{
	if (!iPrec) iPrec = 1;
	iAngle %= 360 * iPrec;
	return fixtoi(Cos(itofix(iAngle, iPrec)), iRadius);
}

static long FnSqrt(C4PropList * _this, long iValue)
{
	if (iValue<0) return 0;
	long iSqrt = long(sqrt(double(iValue)));
	if (iSqrt * iSqrt < iValue) iSqrt++;
	if (iSqrt * iSqrt > iValue) iSqrt--;
	return iSqrt;
}

static long FnAngle(C4PropList * _this, long iX1, long iY1, long iX2, long iY2, long iPrec)
{
	long iAngle;

	// Standard prec
	if (!iPrec) iPrec = 1;

	long dx=iX2-iX1,dy=iY2-iY1;
	if (!dx)
	{
		if (dy>0) return 180 * iPrec;
		else return 0;
	}
	if (!dy)
	{
		if (dx>0) return 90 * iPrec;
		else return 270 * iPrec;
	}

	iAngle = static_cast<long>(180.0 * iPrec * atan2(static_cast<double>(Abs(dy)), static_cast<double>(Abs(dx))) / M_PI);

	if (iX2>iX1 )
	{
		if (iY2<iY1) iAngle = (90 * iPrec) - iAngle;
		else iAngle = (90 * iPrec) + iAngle;
	}
	else
	{
		if (iY2<iY1) iAngle = (270 * iPrec) + iAngle;
		else iAngle = (270 * iPrec) - iAngle;
	}

	return iAngle;
}

static long FnArcSin(C4PropList * _this, long iVal, long iRadius)
{
	// safety
	if (!iRadius) return 0;
	if (iVal > iRadius) return 0;
	// calc arcsin
	double f1 = iVal;
	f1 = asin(f1/iRadius)*180.0/M_PI;
	// return rounded angle
	return (long) floor(f1+0.5);
}

static long FnArcCos(C4PropList * _this, long iVal, long iRadius)
{
	// safety
	if (!iRadius) return 0;
	if (iVal > iRadius) return 0;
	// calc arccos
	double f1 = iVal;
	f1 = acos(f1/iRadius)*180.0/M_PI;
	// return rounded angle
	return (long) floor(f1+0.5);
}

static long FnMin(C4PropList * _this, long iVal1, long iVal2)
{
	return Min(iVal1,iVal2);
}

static long FnMax(C4PropList * _this, long iVal1, long iVal2)
{
	return Max(iVal1,iVal2);
}

static long FnDistance(C4PropList * _this, long iX1, long iY1, long iX2, long iY2)
{
	return Distance(iX1,iY1,iX2,iY2);
}

static long FnBoundBy(C4PropList * _this, long iVal, long iRange1, long iRange2)
{
	return BoundBy(iVal,iRange1,iRange2);
}

static bool FnInside(C4PropList * _this, long iVal, long iRange1, long iRange2)
{
	return Inside(iVal,iRange1,iRange2);
}

static long FnRandom(C4PropList * _this, long iRange)
{
	return Random(iRange);
}

static long FnAsyncRandom(C4PropList * _this, long iRange)
{
	return SafeRandom(iRange);
}

static int FnGetType(C4PropList * _this, const C4Value & Value)
{
	// dynamic types
	if (Value.CheckConversion(C4V_Object)) return C4V_Object;
	if (Value.CheckConversion(C4V_Def)) return C4V_Def;
	if (Value.CheckConversion(C4V_Effect)) return C4V_Effect;
	// static types
	return Value.GetType();
}

static C4ValueArray * FnCreateArray(C4PropList * _this, int iSize)
{
	return new C4ValueArray(iSize);
}

static int FnGetLength(C4PropList * _this, const C4Value & Par)
{
	// support GetLength() etc.
	C4ValueArray * pArray = Par.getArray();
	if (pArray)
		return pArray->GetSize();
	C4String * pStr = Par.getStr();
	if (pStr)
		return GetCharacterCount(pStr->GetData().getData());
	throw new C4AulExecError("GetLength: parameter 0 cannot be converted to string or array");
}

static int FnGetIndexOf(C4PropList * _this, C4ValueArray * pArray, const C4Value & Needle)
{
	// find first occurance of first parameter in array
	// support GetIndexOf(0, x)
	if (!pArray) return -1;
	int32_t iSize = pArray->GetSize();
	for (int32_t i = 0; i < iSize; ++i)
		if (Needle == pArray->GetItem(i))
			// element found
			return i;
	// element not found
	return -1;
}

static C4Void FnSetLength(C4PropList * _this, C4ValueArray *pArray, int iNewSize)
{
	// safety
	if (iNewSize<0 || iNewSize > C4ValueArray::MaxSize)
		throw new C4AulExecError(FormatString("SetLength: invalid array size (%d)", iNewSize).getData());

	// set new size
	pArray->SetSize(iNewSize);
	return C4Void();
}

static Nillable<long> FnGetChar(C4PropList * _this, C4String *pString, long iIndex)
{
	const char *szText = FnStringPar(pString);
	if (!szText) return C4Void();
	// C4Strings are UTF-8 encoded, so decode to get the indicated character
	uint32_t c = GetNextCharacter(&szText);
	for (int i = 0; i < iIndex; ++i)
	{
		c = GetNextCharacter(&szText);
		if (!c) return C4Void();
	}
	return c;
}

static C4Value Fneval(C4PropList * _this, C4String *strScript)
{
	// execute script in the same object
	if (Object(_this))
		return Object(_this)->Def->Script.DirectExec(Object(_this), FnStringPar(strScript), "eval", true);
	else if (_this && _this->GetDef())
		return _this->GetDef()->Script.DirectExec(0, FnStringPar(strScript), "eval", true);
	else
		return ::GameScript.DirectExec(0, FnStringPar(strScript), "eval", true);
}

static bool FnLocateFunc(C4PropList * _this, C4String *funcname, C4PropList * p)
{
	// safety
	if (!funcname || !funcname->GetCStr())
	{
		Log("No func name");
		return false;
	}
	if (!p) p = _this;
	// get function by name
	C4AulFunc *pFunc = p->GetFunc(funcname);
	if (!pFunc)
	{
		LogF("Func %s not found", funcname->GetCStr());
	}
	else
	{
		const char *szPrefix = "";
		while (pFunc)
		{
			C4AulScriptFunc *pSFunc = pFunc->SFunc();
			if (!pSFunc)
			{
				LogF("%s%s (engine)", szPrefix, pFunc->GetName());
			}
			else if (!pSFunc->pOrgScript)
			{
				LogF("%s%s (no owner)", szPrefix, pSFunc->GetName());
			}
			else
			{
				int32_t iLine = SGetLine(pSFunc->pOrgScript->GetScript(), pSFunc->Script);
				LogF("%s%s (%s:%d)", szPrefix, pFunc->GetName(), pSFunc->pOrgScript->ScriptName.getData(), (int)iLine);
			}
			// next func in overload chain
			pFunc = pSFunc ? pSFunc->OwnerOverloaded : NULL;
			szPrefix = "overloads ";
		}
	}
	return true;
}

static long FnModulateColor(C4PropList * _this, long iClr1, long iClr2)
{
	DWORD dwClr1 = iClr1;
	DWORD dwClr2 = iClr2;
	// default color
	if (!dwClr1) dwClr1 = 0xffffff;
	// get alpha
	long iA1=dwClr1>>24, iA2=dwClr2>>24;
	// modulate color values; mod alpha upwards
	DWORD r = (((dwClr1     & 0xff) * (dwClr2    &   0xff))    >>  8)   | // blue
	          (((dwClr1>> 8 & 0xff) * (dwClr2>>8 &   0xff)) &   0xff00) | // green
	          (((dwClr1>>16 & 0xff) * (dwClr2>>8 & 0xff00)) & 0xff0000) | // red
	          (Min<long>(iA1+iA2 - ((iA1*iA2)>>8), 255)           << 24); // alpha
	return r;
}

static long FnWildcardMatch(C4PropList * _this, C4String *psString, C4String *psWildcard)
{
	return SWildcardMatchEx(FnStringPar(psString), FnStringPar(psWildcard));
}

static bool FnFatalError(C4PropList * _this, C4String *pErrorMsg)
{
	throw new C4AulExecError(FormatString("script: %s", pErrorMsg ? pErrorMsg->GetCStr() : "(no error)").getData());
}

static bool FnStartCallTrace(C4PropList * _this)
{
	extern void C4AulStartTrace();
	C4AulStartTrace();
	return true;
}

static bool FnStartScriptProfiler(C4PropList * _this, C4ID idScript)
{
	// get script to profile
	C4AulScript *pScript;
	if (idScript)
	{
		C4Def *pDef = C4Id2Def(idScript);
		if (!pDef) return false;
		pScript = &pDef->Script;
	}
	else
		pScript = &::ScriptEngine;
	// profile it
	C4AulProfiler::StartProfiling(pScript);
	return true;
}

static bool FnStopScriptProfiler(C4PropList * _this)
{
	C4AulProfiler::StopProfiling();
	return true;
}

static Nillable<C4String *> FnGetConstantNameByValue(C4PropList * _this, int value, Nillable<C4String *> name_prefix, int idx)
{
	C4String *name_prefix_s = name_prefix;
	// find a constant that has the specified value and prefix
	for (int32_t i = 0; i < ::ScriptEngine.GlobalConsts.GetAnzItems(); ++i)
	{
		if (::ScriptEngine.GlobalConsts[i].getInt() == value)
		{
			const char *const_name = ::ScriptEngine.GlobalConstNames.GetItemUnsafe(i);
			if (!name_prefix_s || SEqual2(const_name, name_prefix_s->GetCStr()))
				if (!idx--)
					// indexed constant found. return name minus prefix
					return String(const_name + (name_prefix_s ? name_prefix_s->GetData().getLength() : 0));
		}
	}
	// nothing found (at index)
	return C4Void();
}

//=========================== C4Script Function Map ===================================

C4ScriptConstDef C4ScriptConstMap[]=
{
	{ "C4V_Nil",         C4V_Int, C4V_Nil},
	{ "C4V_Int",         C4V_Int, C4V_Int},
	{ "C4V_Bool",        C4V_Int, C4V_Bool},
	{ "C4V_C4Object",    C4V_Int, C4V_Object},
	{ "C4V_Effect",      C4V_Int, C4V_Effect},
	{ "C4V_Def",         C4V_Int, C4V_Def},
	{ "C4V_String",      C4V_Int, C4V_String},
	{ "C4V_Array",       C4V_Int, C4V_Array},
	{ "C4V_Function",    C4V_Int, C4V_Function},
	{ "C4V_PropList",    C4V_Int, C4V_PropList},

	{ "C4X_Ver1",        C4V_Int, C4XVER1},
	{ "C4X_Ver2",        C4V_Int, C4XVER2},
	{ "C4X_Ver3",        C4V_Int, C4XVER3},
	{ "C4X_Ver4",        C4V_Int, C4XVER4},

	{ NULL, C4V_Nil, 0}
};

C4ScriptFnDef C4ScriptFnMap[]=
{
	{ "Log",           1, C4V_Bool,   { C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}, FnLog      },
	{ "DebugLog",      1, C4V_Bool,   { C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}, FnDebugLog },
	{ "Format",        1, C4V_String, { C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}, FnFormat   },

	{ NULL,            0, C4V_Nil,    { C4V_Nil     ,C4V_Nil     ,C4V_Nil     ,C4V_Nil     ,C4V_Nil     ,C4V_Nil     ,C4V_Nil    ,C4V_Nil    ,C4V_Nil    ,C4V_Nil}, 0          }
};

void InitCoreFunctionMap(C4AulScriptEngine *pEngine)
{
	// add all def constants (all Int)
	for (C4ScriptConstDef *pCDef = &C4ScriptConstMap[0]; pCDef->Identifier; pCDef++)
	{
		assert(pCDef->ValType == C4V_Int); // only int supported currently
		pEngine->RegisterGlobalConstant(pCDef->Identifier, C4VInt(pCDef->Data));
	}

	// add all def script funcs
	for (C4ScriptFnDef *pDef = &C4ScriptFnMap[0]; pDef->Identifier; pDef++)
		new C4AulDefFunc(pEngine, pDef);
#define F(f) AddFunc(pEngine, #f, Fn##f)
	F(Abs);
	F(Min);
	F(Max);
	F(Sin);
	F(Cos);
	F(Sqrt);
	F(ArcSin);
	F(ArcCos);
	F(BoundBy);
	F(Inside);
	F(Random);
	F(AsyncRandom);

	F(CreateArray);
	F(CreatePropList);
	F(GetProperties);
	F(GetProperty);
	F(SetProperty);
	F(ResetProperty);
	F(C4Id);
	F(Distance);
	F(Angle);
	F(GetChar);
	F(GetType);
	F(ModulateColor);
	F(WildcardMatch);
	F(GetLength);
	F(SetLength);
	F(GetIndexOf);
	F(FatalError);
	F(StartCallTrace);
	F(StartScriptProfiler);
	F(StopScriptProfiler);
	F(LocateFunc);

	F(eval);
	F(GetConstantNameByValue);

	AddFunc(pEngine, "Translate", C4AulExec::FnTranslate);
#undef F
}
