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

StdStrBuf FnStringFormat(C4AulContext *cthr, const char *szFormatPar, C4Value * Par0, C4Value * Par1, C4Value * Par2, C4Value * Par3,
                                C4Value * Par4, C4Value * Par5, C4Value * Par6, C4Value * Par7, C4Value * Par8, C4Value * Par9)
{
	C4Value * Par[11];
	Par[0]=Par0; Par[1]=Par1; Par[2]=Par2; Par[3]=Par3; Par[4]=Par4;
	Par[5]=Par5; Par[6]=Par6; Par[7]=Par7; Par[8]=Par8; Par[9]=Par9;
	Par[10] = 0;
	int cPar=0;

	StdStrBuf StringBuf("", false);
	const char * cpFormat = szFormatPar;
	const char * cpType;
	char szField[MaxFnStringParLen+1];
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
			SCopy(cpFormat,szField,cpType-cpFormat+1);
			// Insert field by type
			switch (*cpType)
			{
				// number
			case 'd': case 'x': case 'X':
			{
				if (!Par[cPar]) throw new C4AulExecError(cthr->Obj, "format placeholder without parameter");
				StringBuf.AppendFormat(szField, Par[cPar++]->getInt());
				cpFormat+=SLen(szField);
				break;
			}
			// character
			case 'c':
			{
				if (!Par[cPar]) throw new C4AulExecError(cthr->Obj, "format placeholder without parameter");
				StringBuf.AppendCharacter(Par[cPar++]->getInt());
				cpFormat+=SLen(szField);
				break;
			}
			// C4ID
			case 'i':
			{
				if (!Par[cPar]) throw new C4AulExecError(cthr->Obj, "format placeholder without parameter");
				C4ID id = Par[cPar++]->getC4ID();
				StringBuf.Append(id.ToString());
				cpFormat+=SLen(szField);
				break;
			}
			// C4Value
			case 'v':
			{
				if (!Par[cPar]) throw new C4AulExecError(cthr->Obj, "format placeholder without parameter");
				StringBuf.Append(static_cast<const StdStrBuf&>(Par[cPar++]->GetDataString(10)));
				cpFormat+=SLen(szField);
				break;
			}
			// String
			case 's':
			{
				// get string
				if (!Par[cPar]) throw new C4AulExecError(cthr->Obj, "format placeholder without parameter");
				const char *szStr = "(null)";
				if (Par[cPar]->GetData())
				{
					C4String * pStr = Par[cPar++]->getStr();
					if (!pStr) throw new C4AulExecError(cthr->Obj, "string format placeholder without string");
					szStr = pStr->GetCStr();
				}
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

//=============================== C4Script Functions ====================================

static C4Object *Fn_this(C4AulContext *cthr)
{
	return cthr->Obj;
}

static C4PropList * FnCreatePropList(C4AulContext *cthr, C4PropList * prototype)
{
	return C4PropList::New(prototype);
}

static C4Value FnGetProperty_C4V(C4AulContext *cthr, C4Value * key_C4V, C4Value * pObj_C4V)
{
	C4PropList * pObj = pObj_C4V->_getPropList();
	if (!pObj) pObj=cthr->Obj;
	if (!pObj) pObj=cthr->Def;
	if (!pObj) return C4VNull;
	C4String * key = key_C4V->_getStr();
	if (!key) return C4VNull;
	C4Value r;
	pObj->GetPropertyByS(key, &r);
	return r;
}

static C4Value FnSetProperty_C4V(C4AulContext *cthr, C4Value * key_C4V, C4Value * to, C4Value * pObj_C4V)
{
	C4PropList * pObj = pObj_C4V->_getPropList();
	if (!pObj) pObj=cthr->Obj;
	if (!pObj) pObj=cthr->Def;
	if (!pObj) return C4VFalse;
	C4String * key = key_C4V->_getStr();
	if (!key) return C4VFalse;
	if (pObj->IsFrozen())
		throw new C4AulExecError(cthr->Obj, "proplist write: proplist is readonly");
	pObj->SetPropertyByS(key, *to);
	return C4VTrue;
}

static C4Value FnResetProperty_C4V(C4AulContext *cthr, C4Value * key_C4V, C4Value * pObj_C4V)
{
	C4PropList * pObj = pObj_C4V->_getPropList();
	if (!pObj) pObj=cthr->Obj;
	if (!pObj) pObj=cthr->Def;
	if (!pObj) return C4VFalse;
	C4String * key = key_C4V->_getStr();
	if (!key) return C4VFalse;
	if (!pObj->HasProperty(key)) return C4VFalse;
	if (pObj->IsFrozen())
		throw new C4AulExecError(cthr->Obj, "proplist write: proplist is readonly");
	pObj->ResetProperty(key);
	return C4VTrue;
}

static C4Value FnLog_C4V(C4AulContext *cthr, C4Value *szMessage, C4Value * iPar0, C4Value * iPar1, C4Value * iPar2, C4Value * iPar3, C4Value * iPar4, C4Value * iPar5, C4Value * iPar6, C4Value * iPar7, C4Value * iPar8)
{
	Log(FnStringFormat(cthr, FnStringPar(szMessage->getStr()),iPar0,iPar1,iPar2,iPar3,iPar4,iPar5,iPar6,iPar7,iPar8).getData());
	return C4VBool(true);
}

static C4Value FnDebugLog_C4V(C4AulContext *cthr, C4Value *szMessage, C4Value * iPar0, C4Value * iPar1, C4Value * iPar2, C4Value * iPar3, C4Value * iPar4, C4Value * iPar5, C4Value * iPar6, C4Value * iPar7, C4Value * iPar8)
{
	DebugLog(FnStringFormat(cthr, FnStringPar(szMessage->getStr()),iPar0,iPar1,iPar2,iPar3,iPar4,iPar5,iPar6,iPar7,iPar8).getData());
	return C4VBool(true);
}

static C4Value FnFormat_C4V(C4AulContext *cthr, C4Value *szFormat, C4Value * iPar0, C4Value * iPar1, C4Value * iPar2, C4Value * iPar3, C4Value * iPar4, C4Value * iPar5, C4Value * iPar6, C4Value * iPar7, C4Value * iPar8)
{
	return C4VString(FnStringFormat(cthr, FnStringPar(szFormat->getStr()),iPar0,iPar1,iPar2,iPar3,iPar4,iPar5,iPar6,iPar7,iPar8));
}

static C4ID FnC4Id(C4AulContext *cthr, C4String *szID)
{
	return(C4ID(FnStringPar(szID)));
}

static long FnAbs(C4AulContext *cthr, long iVal)
{
	return Abs(iVal);
}

static long FnSin(C4AulContext *cthr, long iAngle, long iRadius, long iPrec)
{
	if (!iPrec) iPrec = 1;
	// Precalculate the modulo operation so the C4Fixed argument to Sin does not overflow
	iAngle %= 360 * iPrec;
	// Let itofix and fixtoi handle the division and multiplication because that can handle higher ranges
	return fixtoi(Sin(itofix(iAngle, iPrec)), iRadius);
}

static long FnCos(C4AulContext *cthr, long iAngle, long iRadius, long iPrec)
{
	if (!iPrec) iPrec = 1;
	iAngle %= 360 * iPrec;
	return fixtoi(Cos(itofix(iAngle, iPrec)), iRadius);
}

static long FnSqrt(C4AulContext *cthr, long iValue)
{
	if (iValue<0) return 0;
	long iSqrt = long(sqrt(double(iValue)));
	if (iSqrt * iSqrt < iValue) iSqrt++;
	if (iSqrt * iSqrt > iValue) iSqrt--;
	return iSqrt;
}

static long FnAngle(C4AulContext *cthr, long iX1, long iY1, long iX2, long iY2, long iPrec)
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

static long FnArcSin(C4AulContext *cthr, long iVal, long iRadius)
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

static long FnArcCos(C4AulContext *cthr, long iVal, long iRadius)
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

static long FnMin(C4AulContext *cthr, long iVal1, long iVal2)
{
	return Min(iVal1,iVal2);
}

static long FnMax(C4AulContext *cthr, long iVal1, long iVal2)
{
	return Max(iVal1,iVal2);
}

static long FnDistance(C4AulContext *cthr, long iX1, long iY1, long iX2, long iY2)
{
	return Distance(iX1,iY1,iX2,iY2);
}

static long FnBoundBy(C4AulContext *cthr, long iVal, long iRange1, long iRange2)
{
	return BoundBy(iVal,iRange1,iRange2);
}

static bool FnInside(C4AulContext *cthr, long iVal, long iRange1, long iRange2)
{
	return Inside(iVal,iRange1,iRange2);
}

static long FnRandom(C4AulContext *cthr, long iRange)
{
	return Random(iRange);
}

static long FnAsyncRandom(C4AulContext *cthr, long iRange)
{
	return SafeRandom(iRange);
}

static C4Value FnGetType(C4AulContext *cthr, C4Value* Value)
{
	return C4VInt(Value->GetType());
}

static C4ValueArray * FnCreateArray(C4AulContext *cthr, int iSize)
{
	return new C4ValueArray(iSize);
}

static C4Value FnGetLength(C4AulContext *cthr, C4Value *pPars)
{
	// support GetLength() etc.
	if (!pPars[0]) return C4VNull;
	C4ValueArray * pArray = pPars->getArray();
	if (pArray)
		return C4VInt(pArray->GetSize());
	C4String * pStr = pPars->getStr();
	if (pStr)
		return C4VInt(GetCharacterCount(pStr->GetData().getData()));
	throw new C4AulExecError(cthr->Obj, "func \"GetLength\" par 0 cannot be converted to string or array");
}

static C4Value FnGetIndexOf(C4AulContext *cthr, C4Value *pPars)
{
	// find first occurance of first parameter in array
	// support GetIndexOf(x, 0)
	if (!pPars[1]) return C4VInt(-1);
	// if the second param is nonzero, it must be an array
	const C4ValueArray * pArray = pPars[1].getArray();
	if (!pArray)
		throw new C4AulExecError(cthr->Obj, "func \"GetIndexOf\" par 1 cannot be converted to array");
	int32_t iSize = pArray->GetSize();
	for (int32_t i = 0; i<iSize; ++i)
		if (pPars[0] == pArray->GetItem(i))
			// element found
			return C4VInt(i);
	// element not found
	return C4VInt(-1);
}

static C4Void FnSetLength(C4AulContext *cthr, C4ValueArray *pArray, int iNewSize)
{
	// safety
	if (iNewSize<0 || iNewSize > C4ValueArray::MaxSize)
		throw new C4AulExecError(cthr->Obj, FormatString("SetLength: invalid array size (%d)", iNewSize).getData());

	// set new size
	pArray->SetSize(iNewSize);
	return C4Void();
}

static Nillable<long> FnGetChar(C4AulContext* cthr, C4String *pString, long iIndex)
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

static C4Value FnEval(C4AulContext *cthr, C4Value *strScript_C4V)
{
	// execute script in the same object
	enum C4AulScript::Strict Strict = C4AulScript::MAXSTRICT;
	if (cthr->Obj)
		return cthr->Obj->Def->Script.DirectExec(cthr->Obj, FnStringPar(strScript_C4V->getStr()), "eval", true, Strict);
	else if (cthr->Def)
		return cthr->Def->Script.DirectExec(0, FnStringPar(strScript_C4V->getStr()), "eval", true, Strict);
	else
		return ::GameScript.DirectExec(0, FnStringPar(strScript_C4V->getStr()), "eval", true, Strict);
}

static bool FnLocateFunc(C4AulContext *cthr, C4String *funcname, C4Object *pObj, C4ID idDef)
{
	// safety
	if (!funcname || !funcname->GetCStr())
	{
		Log("No func name");
		return false;
	}
	// determine script context
	C4AulScript *pCheckScript;
	if (pObj)
	{
		pCheckScript = &pObj->Def->Script;
	}
	else if (idDef)
	{
		C4Def *pDef = C4Id2Def(idDef);
		if (!pDef) { Log("Invalid or unloaded def"); return false; }
		pCheckScript = &pDef->Script;
	}
	else
	{
		if (!cthr || !cthr->Caller || !cthr->Caller->Func || !cthr->Caller->Func->Owner)
		{
			Log("No valid script context");
			return false;
		}
		pCheckScript = cthr->Caller->Func->Owner;
	}
	// get function by name
	C4AulFunc *pFunc = pCheckScript->GetFuncRecursive(funcname->GetCStr());
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
				LogF("%s%s (engine)", szPrefix, pFunc->Name);
			}
			else if (!pSFunc->pOrgScript)
			{
				LogF("%s%s (no owner)", szPrefix, pSFunc->Name);
			}
			else
			{
				int32_t iLine = SGetLine(pSFunc->pOrgScript->GetScript(), pSFunc->Script);
				LogF("%s%s (%s:%d)", szPrefix, pFunc->Name, pSFunc->pOrgScript->ScriptName.getData(), (int)iLine);
			}
			// next func in overload chain
			pFunc = pSFunc ? pSFunc->OwnerOverloaded : NULL;
			szPrefix = "overloads ";
		}
	}
	return true;
}

static long FnModulateColor(C4AulContext *cthr, long iClr1, long iClr2)
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

static long FnWildcardMatch(C4AulContext *ctx, C4String *psString, C4String *psWildcard)
{
	return SWildcardMatchEx(FnStringPar(psString), FnStringPar(psWildcard));
}

static bool FnFatalError(C4AulContext *ctx, C4String *pErrorMsg)
{
	throw new C4AulExecError(ctx->Obj, FormatString("script: %s", pErrorMsg ? pErrorMsg->GetCStr() : "(no error)").getData());
}

static bool FnStartCallTrace(C4AulContext *ctx)
{
	extern void C4AulStartTrace();
	C4AulStartTrace();
	return true;
}

static bool FnStartScriptProfiler(C4AulContext *ctx, C4ID idScript)
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

static bool FnStopScriptProfiler(C4AulContext *ctx)
{
	C4AulProfiler::StopProfiling();
	return true;
}

static C4String *FnTranslate(C4AulContext *ctx, C4String *text)
{
	assert(!ctx->Obj || ctx->Def == ctx->Obj->Def);
	if (!text || text->GetData().isNull()) return NULL;
	// Find correct script: containing script unless -> operator used
	C4AulScript *script = NULL;
	if (ctx->Obj == ctx->Caller->Obj && ctx->Def == ctx->Caller->Def)
		script = ctx->Caller->Func->pOrgScript;
	else
		script = &ctx->Def->Script;
	assert(script);
	try
	{
		return ::Strings.RegString(script->Translate(text->GetCStr()).c_str());
	}
	catch (C4LangStringTable::NoSuchTranslation &)
	{
		DebugLogF("WARNING: Translate: no translation for string \"%s\"", text->GetCStr());
		// Trace
		AulExec.LogCallStack();
		return text;
	}
}

static Nillable<C4String *> FnGetConstantNameByValue(C4AulContext *ctx, int value, Nillable<C4String *> name_prefix, int idx)
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
	{ "C4V_Nil"                ,C4V_Int,          C4V_Nil},
	{ "C4V_Int"                ,C4V_Int,          C4V_Int},
	{ "C4V_Bool"               ,C4V_Int,          C4V_Bool},
	{ "C4V_C4Object"           ,C4V_Int,          C4V_C4Object},
	{ "C4V_String"             ,C4V_Int,          C4V_String},
	{ "C4V_Array"              ,C4V_Int,          C4V_Array},
	{ "C4V_PropList"           ,C4V_Int,          C4V_PropList},

	{ "C4X_Ver1"               ,C4V_Int,          C4XVER1},
	{ "C4X_Ver2"               ,C4V_Int,          C4XVER2},
	{ "C4X_Ver3"               ,C4V_Int,          C4XVER3},
	{ "C4X_Ver4"               ,C4V_Int,          C4XVER4},

	{ NULL, C4V_Nil, 0}
};

#define MkFnC4V (C4Value (*)(C4AulContext *cthr, C4Value*, C4Value*, C4Value*, C4Value*, C4Value*,\
                                                 C4Value*, C4Value*, C4Value*, C4Value*, C4Value*))

C4ScriptFnDef C4ScriptFnMap[]=
{

	{ "SetProperty",          1  ,C4V_Any      ,{ C4V_String  ,C4V_Any     ,C4V_PropList,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V FnSetProperty_C4V ,           0 },
	{ "GetProperty",          1  ,C4V_Any      ,{ C4V_String  ,C4V_PropList,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V FnGetProperty_C4V ,           0 },
	{ "ResetProperty",        1  ,C4V_Any      ,{ C4V_String  ,C4V_PropList,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V FnResetProperty_C4V ,         0 },
	{ "Log",                  1  ,C4V_Bool     ,{ C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V &FnLog_C4V,                   0 },
	{ "DebugLog",             1  ,C4V_Bool     ,{ C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V &FnDebugLog_C4V,              0 },
	{ "Format",               1  ,C4V_String   ,{ C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,MkFnC4V &FnFormat_C4V,                0 },

	{ "GetType",              1  ,C4V_Int      ,{ C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnGetType,                   0 },

	{ "GetLength",            1  ,C4V_Int      ,{ C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,0,                                   FnGetLength },
	{ "GetIndexOf",           1  ,C4V_Int      ,{ C4V_Any     ,C4V_Array   ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,0,                                   FnGetIndexOf },

	{ "eval",                 1  ,C4V_Any      ,{ C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnEval,                      0 },

	{ NULL,                   0  ,C4V_Nil      ,{ C4V_Nil     ,C4V_Nil     ,C4V_Nil     ,C4V_Nil     ,C4V_Nil     ,C4V_Nil     ,C4V_Nil    ,C4V_Nil    ,C4V_Nil    ,C4V_Nil}   ,0,                                   0 }

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
		pEngine->AddFunc(pDef->Identifier, pDef);
	AddFunc(pEngine, "Abs", FnAbs);
	AddFunc(pEngine, "Min", FnMin);
	AddFunc(pEngine, "Max", FnMax);
	AddFunc(pEngine, "Sin", FnSin);
	AddFunc(pEngine, "Cos", FnCos);
	AddFunc(pEngine, "Sqrt", FnSqrt);
	AddFunc(pEngine, "ArcSin", FnArcSin);
	AddFunc(pEngine, "ArcCos", FnArcCos);
	AddFunc(pEngine, "BoundBy", FnBoundBy);
	AddFunc(pEngine, "Inside", FnInside);
	AddFunc(pEngine, "Random", FnRandom);
	AddFunc(pEngine, "AsyncRandom", FnAsyncRandom);

	AddFunc(pEngine, "CreateArray", FnCreateArray);
	AddFunc(pEngine, "CreatePropList", FnCreatePropList);
	AddFunc(pEngine, "C4Id", FnC4Id, false);
	AddFunc(pEngine, "Distance", FnDistance);
	AddFunc(pEngine, "Angle", FnAngle);
	AddFunc(pEngine, "GetChar", FnGetChar);
	AddFunc(pEngine, "ModulateColor", FnModulateColor);
	AddFunc(pEngine, "WildcardMatch", FnWildcardMatch);
	AddFunc(pEngine, "FatalError", FnFatalError);
	AddFunc(pEngine, "StartCallTrace", FnStartCallTrace);
	AddFunc(pEngine, "StartScriptProfiler", FnStartScriptProfiler);
	AddFunc(pEngine, "StopScriptProfiler", FnStopScriptProfiler);
	AddFunc(pEngine, "LocateFunc", FnLocateFunc);

	AddFunc(pEngine, "SetLength", FnSetLength);

	AddFunc(pEngine, "this", Fn_this);
	AddFunc(pEngine, "GetConstantNameByValue", FnGetConstantNameByValue, false);

	AddFunc(pEngine, "Translate", FnTranslate);
}
