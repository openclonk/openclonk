/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
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

/* Functions mapped by C4Script */

#include "C4Include.h"

#include "C4Version.h"
#include "lib/C4Random.h"
#include "script/C4AulExec.h"
#include "script/C4AulDefFunc.h"

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
			for (cpType=cpFormat+1; *cpType && (*cpType == '+' || *cpType == '-' || *cpType == '.' || *cpType == '#' || *cpType == ' ' || Inside(*cpType,'0','9')); cpType++) {}
			// Copy field
			SCopy(cpFormat,szField,std::min<unsigned int>(sizeof szField - 1, cpType - cpFormat + 1));
			// Insert field by type
			switch (*cpType)
			{
				// number
			case 'd': case 'x': case 'X':
			{
				if (cPar >= ParCount) throw C4AulExecError("format placeholder without parameter");
				StringBuf.AppendFormat(szField, Pars[cPar++].getInt());
				cpFormat+=SLen(szField);
				break;
			}
			// character
			case 'c':
			{
				if (cPar >= ParCount) throw C4AulExecError("format placeholder without parameter");
				StringBuf.AppendCharacter(Pars[cPar++].getInt());
				cpFormat+=SLen(szField);
				break;
			}
			// C4ID
			case 'i':
			// C4Value
			case 'v':
			{
				if (cPar >= ParCount) throw C4AulExecError("format placeholder without parameter");
				StringBuf.Append(static_cast<const StdStrBuf&>(Pars[cPar++].GetDataString(10)));
				cpFormat+=SLen(szField);
				break;
			}
			// String
			case 's':
			{
				// get string
				if (cPar >= ParCount) throw C4AulExecError("format placeholder without parameter");
				const char *szStr = "(null)";
				if (Pars[cPar].GetData())
				{
					C4String * pStr = Pars[cPar].getStr();
					if (!pStr) throw C4AulExecError("string format placeholder without string");
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

C4AulDefFunc::C4AulDefFunc(C4PropListStatic * Parent, C4ScriptFnDef* pDef):
		C4AulFunc(Parent, pDef->Identifier), Def(pDef)
{
	Parent->SetPropertyByS(Name, C4VFunction(this));
}

C4AulDefFunc::~C4AulDefFunc() = default;

C4Value C4AulDefFunc::Exec(C4PropList * p, C4Value pPars[], bool fPassErrors)
{
	assert(Def->FunctionC4V);
	return Def->FunctionC4V(p, pPars);
}

//=============================== C4Script Functions ====================================

#define MAKE_AND_RETURN_ARRAY(values) do { \
	C4ValueArray *matrix = new C4ValueArray(sizeof(values) / sizeof(*values)); \
	for (size_t i = 0; i < sizeof(values) / sizeof(*values); ++i) \
		(*matrix)[i] = C4VInt(values[i]); \
	return matrix; \
} while (0)

static C4ValueArray *FnTrans_Identity(C4PropList * _this)
{
	long values[] = 
	{
		1000, 0, 0, 0,
		0, 1000, 0, 0,
		0, 0, 1000, 0
	};
	MAKE_AND_RETURN_ARRAY(values);
}

static C4ValueArray *FnTrans_Translate(C4PropList * _this, long dx, long dy, long dz)
{
	long values[] = 
	{
		1000, 0, 0, dx,
		0, 1000, 0, dy,
		0, 0, 1000, dz
	};
	MAKE_AND_RETURN_ARRAY(values);
}

static C4ValueArray *FnTrans_Scale(C4PropList * _this, long sx, long sy, long sz)
{
	if (sy == 0 && sz == 0)
		sy = sz = sx;
	long values[] = 
	{
		sx, 0, 0, 0,
		0, sy, 0, 0,
		0, 0, sz, 0
	};
	MAKE_AND_RETURN_ARRAY(values);
}

static C4ValueArray *FnTrans_Rotate(C4PropList * _this, long angle, long rx, long ry, long rz)
{
	long c = fixtoi(Cos(itofix(angle, 1)), 1000);
	long s = fixtoi(Sin(itofix(angle, 1)), 1000);

	long sqrt_val = rx * rx + ry * ry + rz * rz;
	long n = long(sqrt(double(sqrt_val)));
	if (n * n < sqrt_val) n++;
	else if (n * n > sqrt_val) n--;
	
	if (n == 0)
	{
		throw C4AulExecError("cannot rotate around a null vector");
	}

	rx = (1000 * rx) / n;
	ry = (1000 * ry) / n;
	rz = (1000 * rz) / n;

	long values[] = 
	{
		rx*rx*(1000-c)/1000000+c, rx*ry*(1000-c)/1000000-rz*s/1000, rx*rz*(1000-c)/1000000+ry*s/1000, 0,
		ry*rx*(1000-c)/1000000+rz*s/1000, ry*ry*(1000-c)/1000000+c, ry*rz*(1000-c)/1000000-rx*s/1000, 0,
		rz*rx*(1000-c)/1000000-ry*s/1000, ry*rz*(1000-c)/1000000+rx*s/1000, rz*rz*(1000-c)/1000000+c, 0
	};
	MAKE_AND_RETURN_ARRAY(values);
}

static C4Value FnTrans_Mul(C4PropList * _this, C4Value *pars)
{
	const int32_t matrixSize = 12;
	long values[] = 
	{
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0
	};

	// Read all parameters
	bool first = true;
	for (int32_t i = 0; i < C4AUL_MAX_Par; i++)
	{
		C4Value Data = *(pars++);
		// No data given?
		if (!Data) break;
		C4ValueArray *factorArray = Data.getArray();
		if (!factorArray || factorArray->GetSize() != matrixSize) continue;

		if (first)
		{
			first = false;
			
			for (int32_t c = 0; c < matrixSize; ++c)
				values[c] = (*factorArray)[c].getInt();
			continue;
		}

		// multiply current matrix with new one
		long values_rhs[matrixSize], values_result[matrixSize];
		for (int32_t c = 0; c < matrixSize; ++c)
			values_rhs[c] = (*factorArray)[c].getInt();

		// matrix multiplication
		values_result[ 0] = values[0]*values_rhs[0]/1000 + values[1]*values_rhs[4]/1000 + values[ 2]*values_rhs[ 8]/1000;
		values_result[ 1] = values[0]*values_rhs[1]/1000 + values[1]*values_rhs[5]/1000 + values[ 2]*values_rhs[ 9]/1000;
		values_result[ 2] = values[0]*values_rhs[2]/1000 + values[1]*values_rhs[6]/1000 + values[ 2]*values_rhs[10]/1000;
		values_result[ 3] = values[0]*values_rhs[3]/1000 + values[1]*values_rhs[7]/1000 + values[ 2]*values_rhs[11]/1000 + values[3];
		values_result[ 4] = values[4]*values_rhs[0]/1000 + values[5]*values_rhs[4]/1000 + values[ 6]*values_rhs[ 8]/1000;
		values_result[ 5] = values[4]*values_rhs[1]/1000 + values[5]*values_rhs[5]/1000 + values[ 6]*values_rhs[ 9]/1000;
		values_result[ 6] = values[4]*values_rhs[2]/1000 + values[5]*values_rhs[6]/1000 + values[ 6]*values_rhs[10]/1000;
		values_result[ 7] = values[4]*values_rhs[3]/1000 + values[5]*values_rhs[7]/1000 + values[ 6]*values_rhs[11]/1000 + values[7];
		values_result[ 8] = values[8]*values_rhs[0]/1000 + values[9]*values_rhs[4]/1000 + values[10]*values_rhs[ 8]/1000;
		values_result[ 9] = values[8]*values_rhs[1]/1000 + values[9]*values_rhs[5]/1000 + values[10]*values_rhs[ 9]/1000;
		values_result[10] = values[8]*values_rhs[2]/1000 + values[9]*values_rhs[6]/1000 + values[10]*values_rhs[10]/1000;
		values_result[11] = values[8]*values_rhs[3]/1000 + values[9]*values_rhs[7]/1000 + values[10]*values_rhs[11]/1000 + values[11];

		for (int32_t c = 0; c < matrixSize; ++c)
			values[c] = values_result[c];
	}

	// unlike in the other Trans_*-functions, we have to put the array into a C4Value manually here
	C4ValueArray *matrix = new C4ValueArray(sizeof(values) / sizeof(*values));
	for (size_t i = 0; i < sizeof(values) / sizeof(*values); ++i)
		(*matrix)[i] = C4VInt(values[i]);
	return C4VArray(matrix);
}

#undef MAKE_AND_RETURN_ARRAY

/* PropLists */

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
		throw C4AulExecError("proplist write: proplist is readonly");
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
		throw C4AulExecError("proplist write: proplist is readonly");
	pObj->ResetProperty(key);
	return true;
}

static C4ValueArray * FnGetProperties(C4PropList * _this, C4PropList * p)
{
	if (!p) p = _this;
	if (!p) throw NeedNonGlobalContext("GetProperties");
	C4ValueArray * r = p->GetProperties();
	r->SortStrings();
	return r;
}

static C4PropList * FnGetPrototype(C4PropList * _this, C4PropList * p)
{
	if (!p) p = _this;
	if (!p) throw NeedNonGlobalContext("GetPrototype");
	return p->GetPrototype();
}

static void FnSetPrototype(C4PropList * _this, C4PropList * prototype, C4PropList * p)
{
	if (!p) p = _this;
	if (!p) throw NeedNonGlobalContext("GetPrototype");
	p->SetProperty(P_Prototype, C4Value(prototype));
}

static C4Value FnCall(C4PropList * _this, C4Value * Pars)
{
	if (!_this) _this = ::ScriptEngine.GetPropList();
	C4AulParSet ParSet;
	ParSet.Copy(&Pars[1], C4AUL_MAX_Par - 1);
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
		throw C4AulExecError(FormatString("Call: no function %s", Pars[0].GetDataString().getData()).getData());
	return fn->Exec(_this, &ParSet, true);
}

static C4String *FnGetName(C4PropList * _this, bool truename)
{
	if (!_this)
		throw NeedNonGlobalContext("GetName");
	else if(truename)
		return _this->IsStatic() ? _this->IsStatic()->GetParentKeyName() : nullptr;
	else
		return String(_this->GetName());
}

/* Effects */

static C4Value FnAddEffect(C4PropList * _this, C4String * szEffect, C4PropList * pTarget,
                           int iPrio, int iTimerInterval, C4PropList * pCmdTarget, C4Def * idCmdTarget,
                           const C4Value & Val1, const C4Value & Val2, const C4Value & Val3, const C4Value & Val4)
{
	// safety
	if (pTarget && !pTarget->Status) return C4Value();
	if (!szEffect || !*szEffect->GetCStr() || !iPrio) return C4Value();
	// create effect
	C4PropList * p = pCmdTarget;
	if (!p) p = idCmdTarget;
	if (!p) p = ::ScriptEngine.GetPropList();
	C4Effect * pEffect = C4Effect::New(pTarget, FnGetEffectsFor(pTarget),
			szEffect, iPrio, iTimerInterval, p, Val1, Val2, Val3, Val4);
	// return effect - may be 0 if the effect has been denied by another effect
	if (!pEffect) return C4Value();
	return C4VPropList(pEffect);
}

static C4Effect * FnCreateEffect(C4PropList * _this, C4PropList * prototype, int iPrio, int iTimerInterval,
                              const C4Value & Val1, const C4Value & Val2, const C4Value & Val3, const C4Value & Val4)
{
	if (!prototype || !(prototype->GetName()[0])) throw C4AulExecError("CreateEffect needs a prototype with a name");
	if (!iPrio) throw C4AulExecError("CreateEffect needs a nonzero priority");
	// create effect
	C4Effect * pEffect = C4Effect::New(_this, FnGetEffectsFor(_this), prototype, iPrio, iTimerInterval,
	                                   Val1, Val2, Val3, Val4);
	// return effect - may be 0 if the effect has been denied by another effect
	return pEffect;
}

static C4Effect * FnGetEffect(C4PropList * _this, C4String *psEffectName, C4PropList *pTarget, int index, int iMaxPriority)
{
	const char *szEffect = FnStringPar(psEffectName);
	// get effects
	C4Effect *pEffect = *FnGetEffectsFor(pTarget);
	if (!pEffect) return nullptr;
	// name/wildcard given: find effect by name and index
	if (szEffect && *szEffect)
		return pEffect->Get(szEffect, index, iMaxPriority);
	return nullptr;
}

static bool FnRemoveEffect(C4PropList * _this, C4String *psEffectName, C4PropList *pTarget, C4Effect * pEffect2, bool fDoNoCalls)
{
	// evaluate parameters
	const char *szEffect = FnStringPar(psEffectName);
	// if the user passed an effect, it can be used straight-away
	C4Effect *pEffect = pEffect2;
	// otherwise, the correct effect will be searched in the target's effects or in the global ones
	if (!pEffect)
	{
		pEffect = *FnGetEffectsFor(pTarget);
		// the object has no effects attached, nothing to look for
		if (!pEffect) return false;
		// name/wildcard given: find effect by name
		if (szEffect && *szEffect)
			pEffect = pEffect->Get(szEffect, 0);
	}

	// neither passed nor found - nothing to remove!
	if (!pEffect) return false;

	// kill it
	if (fDoNoCalls)
		pEffect->SetDead();
	else
		pEffect->Kill();
	// done, success
	return true;
}

static C4Value FnCheckEffect(C4PropList * _this, C4String * psEffectName, C4PropList * pTarget,
                             int iPrio, int iTimerInterval,
                             const C4Value & Val1, const C4Value & Val2, const C4Value & Val3, const C4Value & Val4)
{
	const char *szEffect = FnStringPar(psEffectName);
	// safety
	if (pTarget && !pTarget->Status) return C4Value();
	if (!szEffect || !*szEffect) return C4Value();
	// get effects
	C4Effect *pEffect = *FnGetEffectsFor(pTarget);
	if (!pEffect) return C4Value();
	// let them check
	C4Effect * r = pEffect->Check(szEffect, iPrio, iTimerInterval, Val1, Val2, Val3, Val4);
	if (r == (C4Effect *)C4Fx_Effect_Deny) return C4VInt(C4Fx_Effect_Deny);
	if (r == (C4Effect *)C4Fx_Effect_Annul) return C4VInt(C4Fx_Effect_Annul);
	return C4VPropList(r);
}

static long FnGetEffectCount(C4PropList * _this, C4String *psEffectName, C4PropList *pTarget, long iMaxPriority)
{
	// evaluate parameters
	const char *szEffect = FnStringPar(psEffectName);
	// get effects
	C4Effect *pEffect = *FnGetEffectsFor(pTarget);
	if (!pEffect) return false;
	// count effects
	if (!*szEffect) szEffect = nullptr;
	return pEffect->GetCount(szEffect, iMaxPriority);
}

static C4Value FnEffectCall(C4PropList * _this, C4Value * Pars)
{
	// evaluate parameters
	C4PropList *pTarget = Pars[0].getPropList();
	C4Effect * pEffect = Pars[1].getPropList() ? Pars[1].getPropList()->GetEffect() : nullptr;
	const char *szCallFn = FnStringPar(Pars[2].getStr());
	// safety
	if (pTarget && !pTarget->Status) return C4Value();
	if (!szCallFn || !*szCallFn) return C4Value();
	if (!pEffect) return C4Value();
	// do call
	return pEffect->DoCall(pTarget, szCallFn, Pars[3], Pars[4], Pars[5], Pars[6], Pars[7], Pars[8], Pars[9]);
}

/* Regex */

static const long
	Regex_CaseInsensitive = (1 << 0),
	Regex_FirstOnly       = (1 << 1);

static std::regex_constants::syntax_option_type C4IntToSyntaxOption(long flags)
{
	std::regex_constants::syntax_option_type out = std::regex::ECMAScript;
	if (flags & Regex_CaseInsensitive)
		out |= std::regex::icase;
	return out;
}

static std::regex_constants::match_flag_type C4IntToMatchFlag(long flags)
{
	std::regex_constants::match_flag_type out = std::regex_constants::match_default;
	if (flags & Regex_FirstOnly)
		out |= std::regex_constants::format_first_only;
	return out;
}

static Nillable<C4String *> FnRegexReplace(C4PropList * _this, C4String *source, C4String *regex, C4String *replacement, long flags)
{
	if (!source || !regex || !replacement) return C4Void();
	try
	{
		std::regex re(regex->GetCStr(), C4IntToSyntaxOption(flags));
		std::string out = std::regex_replace(source->GetCStr(), re, replacement->GetCStr(), C4IntToMatchFlag(flags));
		return ::Strings.RegString(out.c_str());
	}
	catch (const std::regex_error& e)
	{
		throw C4AulExecError(FormatString("RegexReplace: %s", e.what()).getData());
	}
}


static Nillable<C4ValueArray *> FnRegexSearch(C4PropList * _this, C4String *source, C4String *regex, long flags)
{
	if (!source || !regex) return C4Void();
	try
	{
		std::regex re(regex->GetCStr(), C4IntToSyntaxOption(flags));
		C4ValueArray *out = new C4ValueArray();
		const auto &data = source->GetData();
		size_t pos = 0;
		std::cmatch m;
		long i = 0;
		// std::regex_iterator would be the better way to do this, but is is broken in libc++ (see LLVM bug #21597).
		while (pos <= data.getLength() && std::regex_search(data.getData() + pos, data.getData() + data.getLength(), m, re))
		{
			int char_pos = GetCharacterCount(std::string(data.getData(), pos + m.position()).c_str());
			(*out)[i++] = C4VInt(char_pos);
			if (flags & Regex_FirstOnly) break;
			pos += m.position() + std::max<size_t>(m.length(), 1);
		}
		return out;
	}
	catch (const std::regex_error& e)
	{
		throw C4AulExecError(FormatString("RegexSearch: %s", e.what()).getData());
	}
}

static Nillable<C4ValueArray *> FnRegexMatch(C4PropList * _this, C4String *source, C4String *regex, long flags)
{
	if (!source || !regex) return C4Void();
	try
	{
		std::regex re(regex->GetCStr(), C4IntToSyntaxOption(flags));
		C4ValueArray *out = new C4ValueArray();
		const auto &data = source->GetData();
		size_t pos = 0;
		std::cmatch m;
		long i = 0;
		while (pos <= data.getLength() && std::regex_search(data.getData() + pos, data.getData() + data.getLength(), m, re))
		{
			C4ValueArray *match = new C4ValueArray(m.size());
			long j = 0;
			for (auto sm : m)
			{
				(*match)[j++] = C4VString(String(sm.str().c_str()));
			}
			(*out)[i++] = C4VArray(match);
			if (flags & Regex_FirstOnly) break;
			pos += m.position() + std::max<size_t>(m.length(), 1);
		}
		return out;
	}
	catch (const std::regex_error& e)
	{
		throw C4AulExecError(FormatString("RegexMatch: %s", e.what()).getData());
	}
}

static Nillable<C4ValueArray *> FnRegexSplit(C4PropList * _this, C4String *source, C4String *regex, long flags)
{
	if (!source || !regex) return C4Void();
	try
	{
		std::regex re(regex->GetCStr(), C4IntToSyntaxOption(flags));
		C4ValueArray *out = new C4ValueArray();
		const auto &data = source->GetData();
		size_t pos = 0;
		std::cmatch m;
		long i = 0;
		while (pos <= data.getLength() && std::regex_search(data.getData() + pos, data.getData() + data.getLength(), m, re))
		{
			// As we're advancing by one character for zero-length matches, always
			// include at least one character here.
			std::string substr(data.getData() + pos, std::max<size_t>(m.position(), 1));
			(*out)[i++] = C4VString(String(substr.c_str()));
			if (flags & Regex_FirstOnly) break;
			pos += m.position() + std::max<size_t>(m.length(), 1);
		}
		if (pos <= data.getLength())
		{
			std::string substr(data.getData() + pos, data.getLength() - pos);
			(*out)[i++] = C4VString(String(substr.c_str()));
		}
		return out;
	}
	catch (const std::regex_error& e)
	{
		throw C4AulExecError(FormatString("RegexSplit: %s", e.what()).getData());
	}
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

// Parse a string into an integer. Returns nil if the conversion fails.
static Nillable<int32_t> FnParseInt(C4PropList *_this, C4String *str)
{
	const char *cstr = str->GetCStr();
	const char *end = nullptr;
	int32_t result = StrToI32(cstr, 10, &end);
	if (end == cstr || *end != '\0') return C4Void();
	return result;
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
	// Standard prec
	if (!iPrec) iPrec = 1;
	return Angle(iX1, iY1, iX2, iY2, iPrec);
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

static std::pair<Nillable<int32_t>, Nillable<int32_t>> minmax(const char *func, const C4Value &a_val, const Nillable<int32_t> &b_opt)
{
	if (a_val.CheckConversion(C4V_Int))
	{
		int32_t a = a_val.getInt();
		int32_t b = b_opt;
		if (a > b)
			std::swap(a, b);
		return std::make_pair(a, b);
	}
	else if (a_val.CheckConversion(C4V_Array))
	{
		const C4ValueArray *a = a_val.getArray();
		if (a->GetSize() == 0)
			return std::make_pair(nullptr, nullptr);
		
		if (!a->GetItem(0).CheckConversion(C4V_Int))
		{
			throw C4AulExecError(FormatString("%s: argument 1 must be int or array-of-int, but element %d of array is of type %s", func, 0, a->GetItem(0).GetTypeName()).getData());
		}
		int32_t min, max;
		min = max = a->GetItem(0).getInt();

		for (int32_t i = 1; i < a->GetSize(); ++i)
		{
			if (!a->GetItem(i).CheckConversion(C4V_Int))
			{
				throw C4AulExecError(FormatString("%s: argument 1 must be int or array-of-int, but element %d of array is of type %s", func, i, a->GetItem(i).GetTypeName()).getData());
			}
			int32_t value = a->GetItem(i).getInt();
			min = std::min(min, value);
			max = std::max(max, value);
		}

		return std::make_pair(min, max);
	}
	else
	{
		throw C4AulExecError(FormatString("%s: argument 1 must be int or array-of-int, but is of type %s", func, a_val.GetTypeName()).getData());
	}
}

static Nillable<int32_t> FnMin(C4PropList * _this, const C4Value &a, Nillable<int32_t> b)
{
	return minmax("Min", a, b).first;
}

static Nillable<int32_t> FnMax(C4PropList * _this, const C4Value &a, Nillable<int32_t> b)
{
	return minmax("Max", a, b).second;
}

static long FnDistance(C4PropList * _this, long iX1, long iY1, long iX2, long iY2)
{
	return Distance(iX1,iY1,iX2,iY2);
}

static long FnBoundBy(C4PropList * _this, long iVal, long iRange1, long iRange2)
{
	return Clamp(iVal,iRange1,iRange2);
}

static bool FnInside(C4PropList * _this, long iVal, long iRange1, long iRange2)
{
	return Inside(iVal,iRange1,iRange2);
}

static long FnRandom(C4PropList * _this, long iRange)
{
	return Random(iRange);
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
	throw C4AulExecError("GetLength: parameter 0 cannot be converted to string or array");
}

static int FnGetIndexOf(C4PropList * _this, C4ValueArray * pArray, const C4Value & Needle)
{
	// find first occurance of first parameter in array
	// support GetIndexOf(0, x)
	if (!pArray) return -1;
	int32_t iSize = pArray->GetSize();
	for (int32_t i = 0; i < iSize; ++i)
		if (Needle.IsIdenticalTo(pArray->GetItem(i)))
			// element found
			return i;
	// element not found
	return -1;
}

static bool FnDeepEqual(C4PropList * _this, const C4Value & v1, const C4Value & v2)
{
	// return if v1==v2 with deep comparison on arrays and proplists
	return v1 == v2;
}

static void FnSetLength(C4PropList * _this, C4ValueArray *pArray, int iNewSize)
{
	// safety
	if (iNewSize<0 || iNewSize > C4ValueArray::MaxSize)
		throw C4AulExecError(FormatString("SetLength: invalid array size (%d)", iNewSize).getData());

	// set new size
	pArray->SetSize(iNewSize);
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

static C4String *FnStringToIdentifier(C4PropList * _this, C4String *pString)
{
	// Change an arbitrary string so that it becomes an identifier
	const char *text = FnStringPar(pString);
	if (!text) return nullptr;
	StdStrBuf result;
	bool had_valid = false, had_invalid = false;
	const char *ptext = text, *t0 = text;
	uint32_t c = GetNextCharacter(&text);
	while (c)
	{
		if (isalnum(c) || c == '_')
		{
			// Starting with a digit? Needs to prepend a character
			if (isdigit(c) && !had_valid)
			{
				result.Append("_");
				had_invalid = true;
			}
			// Valid character: Append to result string if a modification had to be done
			if (had_invalid) result.Append(ptext, text - ptext);
			had_valid = true;
		}
		else
		{
			// Invalid character. Make sure result is created from previous valid characters
			if (!had_invalid)
			{
				result.Copy(t0, ptext - t0);
				had_invalid = true;
			}
		}
		ptext = text;
		c = GetNextCharacter(&text);
	}
	// Make sure no empty string is returned
	if (!had_valid) return ::Strings.RegString("_");
	// Return either modified string or the original if no modifications were needed
	return had_invalid ? ::Strings.RegString(result) : pString;
}

static C4Value Fneval(C4PropList * _this, C4String *strScript, bool dont_pass_errors)
{
	return ::AulExec.DirectExec(_this, FnStringPar(strScript), "eval", !dont_pass_errors);
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
			pFunc = pSFunc ? pSFunc->OwnerOverloaded : nullptr;
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
	          (std::min<long>(iA1+iA2 - ((iA1*iA2)>>8), 255)           << 24); // alpha
	return r;
}

static long FnWildcardMatch(C4PropList * _this, C4String *psString, C4String *psWildcard)
{
	return SWildcardMatchEx(FnStringPar(psString), FnStringPar(psWildcard));
}

static bool FnFatalError(C4PropList * _this, C4String *pErrorMsg)
{
	throw C4AulExecError(FormatString("script: %s", pErrorMsg ? pErrorMsg->GetCStr() : "(no error)").getData());
}

static bool FnStartCallTrace(C4PropList * _this)
{
	AulExec.StartTrace();
	return true;
}

static bool FnStartScriptProfiler(C4PropList * _this, C4Def * pDef)
{
	// get script to profile
	C4ScriptHost *pScript;
	if (pDef)
		pScript = &pDef->Script;
	else
		pScript = nullptr;
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

static Nillable<C4String *> FnReplaceString(C4PropList * _this, C4String *source, C4String *from, C4String *to)
{
	if (!from) return source;
	if (!source) return C4Void();
	const char *szto = to ? to->GetCStr() : "";
	const char *szfrom = from->GetCStr();
	StdStrBuf s(source->GetData(), true);
	if (s.Replace(szfrom, szto))
	{
		return ::Strings.RegString(s.getData());
	}
	else
	{
		return source;
	}
}

static bool FnSortArray(C4PropList * _this, C4ValueArray *pArray, bool descending)
{
	if (!pArray) throw C4AulExecError("SortArray: no array given");
	if (pArray->IsFrozen()) throw C4AulExecError("array sort: array is readonly");
	// sort array by its members
	pArray->Sort(descending);
	return true;
}

static bool FnSortArrayByProperty(C4PropList * _this, C4ValueArray *pArray, C4String *prop_name, bool descending)
{
	if (!pArray) throw C4AulExecError("SortArrayByProperty: no array given");
	if (!prop_name) throw C4AulExecError("SortArrayByProperty: no property name given");
	if (pArray->IsFrozen()) throw C4AulExecError("array sort: array is readonly");
	// sort array by property
	if (!pArray->SortByProperty(prop_name, descending)) throw C4AulExecError("SortArrayByProperty: not all array elements are proplists");
	return true;
}

static bool FnSortArrayByArrayElement(C4PropList * _this, C4ValueArray *pArray, int32_t element_index, bool descending)
{
	if (!pArray) throw C4AulExecError("SortArrayByArrayElement: no array given");
	if (element_index<0) throw C4AulExecError("SortArrayByArrayElement: element index must be >=0");
	if (pArray->IsFrozen()) throw C4AulExecError("array sort: array is readonly");
	// sort array by array element
	if (!pArray->SortByArrayElement(element_index, descending)) throw C4AulExecError("SortArrayByArrayElement: not all array elements are arrays of sufficient length");
	return true;
}

static bool FnFileWrite(C4PropList * _this, int32_t file_handle, C4String *data)
{
	// resolve file handle to user file
	C4AulUserFile *file = ::ScriptEngine.GetUserFile(file_handle);
	if (!file) throw C4AulExecError("FileWrite: invalid file handle");
	// prepare string to write
	if (!data) return false; // write nullptr? No.
	// write it
	file->Write(data->GetCStr(), data->GetData().getLength());
	return true;
}

//=========================== C4Script Function Map ===================================

C4ScriptConstDef C4ScriptConstMap[]=
{
	{ "FX_OK"                     ,C4V_Int,      C4Fx_OK                    }, // generic standard behaviour for all effect callbacks
	{ "FX_Effect_Deny"            ,C4V_Int,      C4Fx_Effect_Deny           }, // delete effect
	{ "FX_Effect_Annul"           ,C4V_Int,      C4Fx_Effect_Annul          }, // delete effect, because it has annulled a countereffect
	{ "FX_Effect_AnnulDoCalls"    ,C4V_Int,      C4Fx_Effect_AnnulCalls     }, // delete effect, because it has annulled a countereffect; temp readd countereffect
	{ "FX_Execute_Kill"           ,C4V_Int,      C4Fx_Execute_Kill          }, // execute callback: Remove effect now
	{ "FX_Stop_Deny"              ,C4V_Int,      C4Fx_Stop_Deny             }, // deny effect removal
	{ "FX_Start_Deny"             ,C4V_Int,      C4Fx_Start_Deny            }, // deny effect start

	{ "FX_Call_Normal"            ,C4V_Int,      C4FxCall_Normal            }, // normal call; effect is being added or removed
	{ "FX_Call_Temp"              ,C4V_Int,      C4FxCall_Temp              }, // temp call; effect is being added or removed in responce to a lower-level effect change
	{ "FX_Call_TempAddForRemoval" ,C4V_Int,      C4FxCall_TempAddForRemoval }, // temp call; effect is being added because it had been temp removed and is now removed forever
	{ "FX_Call_RemoveClear"       ,C4V_Int,      C4FxCall_RemoveClear       }, // effect is being removed because object is being removed
	{ "FX_Call_RemoveDeath"       ,C4V_Int,      C4FxCall_RemoveDeath       }, // effect is being removed because object died - return -1 to avoid removal
	{ "FX_Call_DmgScript"         ,C4V_Int,      C4FxCall_DmgScript         }, // damage through script call
	{ "FX_Call_DmgBlast"          ,C4V_Int,      C4FxCall_DmgBlast          }, // damage through blast
	{ "FX_Call_DmgFire"           ,C4V_Int,      C4FxCall_DmgFire           }, // damage through fire
	{ "FX_Call_DmgChop"           ,C4V_Int,      C4FxCall_DmgChop           }, // damage through chopping
	{ "FX_Call_Energy"            ,C4V_Int,      32                         }, // bitmask for generic energy loss
	{ "FX_Call_EngScript"         ,C4V_Int,      C4FxCall_EngScript         }, // energy loss through script call
	{ "FX_Call_EngBlast"          ,C4V_Int,      C4FxCall_EngBlast          }, // energy loss through blast
	{ "FX_Call_EngObjHit"         ,C4V_Int,      C4FxCall_EngObjHit         }, // energy loss through object hitting the living
	{ "FX_Call_EngFire"           ,C4V_Int,      C4FxCall_EngFire           }, // energy loss through fire
	{ "FX_Call_EngBaseRefresh"    ,C4V_Int,      C4FxCall_EngBaseRefresh    }, // energy reload in base (also by base object, but that's normally not called)
	{ "FX_Call_EngAsphyxiation"   ,C4V_Int,      C4FxCall_EngAsphyxiation   }, // energy loss through asphyxiaction
	{ "FX_Call_EngCorrosion"      ,C4V_Int,      C4FxCall_EngCorrosion      }, // energy loss through corrosion (acid)
	{ "FX_Call_EngGetPunched"     ,C4V_Int,      C4FxCall_EngGetPunched     }, // energy loss from punch

	{ "Regex_CaseInsensitive"     ,C4V_Int,      Regex_CaseInsensitive      },
	{ "Regex_FirstOnly"           ,C4V_Int,      Regex_FirstOnly            },

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

	{ nullptr, C4V_Nil, 0}
};

C4ScriptFnDef C4ScriptFnMap[]=
{
	{ "Call",          true, C4V_Any,    { C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}, FnCall     },
	{ "EffectCall",    true, C4V_Any,    { C4V_Object  ,C4V_PropList,C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}, FnEffectCall    },
	{ "Log",           true, C4V_Bool,   { C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}, FnLog      },
	{ "DebugLog",      true, C4V_Bool,   { C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}, FnDebugLog },
	{ "Format",        true, C4V_String, { C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}, FnFormat   },
	{ "Trans_Mul",     true, C4V_Array,  { C4V_Array   ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}, FnTrans_Mul},

	{ nullptr,            false, C4V_Nil,    { C4V_Nil     ,C4V_Nil     ,C4V_Nil     ,C4V_Nil     ,C4V_Nil     ,C4V_Nil     ,C4V_Nil    ,C4V_Nil    ,C4V_Nil    ,C4V_Nil}, nullptr          }
};

void InitCoreFunctionMap(C4AulScriptEngine *pEngine)
{
	// add all def constants (all Int)
	for (C4ScriptConstDef *pCDef = &C4ScriptConstMap[0]; pCDef->Identifier; pCDef++)
	{
		assert(pCDef->ValType == C4V_Int); // only int supported currently
		pEngine->RegisterGlobalConstant(pCDef->Identifier, C4VInt(pCDef->Data));
	}

	C4PropListStatic * p = pEngine->GetPropList();
	// add all def script funcs
	for (C4ScriptFnDef *pDef = &C4ScriptFnMap[0]; pDef->Identifier; pDef++)
		new C4AulDefFunc(p, pDef);
#define F(f) ::AddFunc(p, #f, Fn##f)
	F(ParseInt);
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

	F(CreateArray);
	F(CreatePropList);
	F(GetProperties);
	F(GetProperty);
	F(SetProperty);
	F(GetPrototype);
	F(SetPrototype);
	F(ResetProperty);
	F(GetName);
	F(AddEffect);
	F(CreateEffect);
	F(CheckEffect);
	F(RemoveEffect);
	F(GetEffect);
	F(GetEffectCount);
	F(RegexReplace);
	F(RegexSearch);
	F(RegexMatch);
	F(RegexSplit);
	F(Distance);
	F(Angle);
	F(GetChar);
	F(GetType);
	F(ModulateColor);
	F(WildcardMatch);
	F(GetLength);
	F(SetLength);
	F(GetIndexOf);
	F(DeepEqual);
	F(FatalError);
	F(StartCallTrace);
	F(StartScriptProfiler);
	F(StopScriptProfiler);
	F(SortArray);
	F(SortArrayByProperty);
	F(SortArrayByArrayElement);
	F(Trans_Identity);
	F(Trans_Translate);
	F(Trans_Scale);
	F(Trans_Rotate);
	F(LocateFunc);
	F(FileWrite);
	F(eval);
	F(StringToIdentifier);
	F(GetConstantNameByValue);
	F(ReplaceString);

	::AddFunc(p, "Translate", C4AulExec::FnTranslate);
	::AddFunc(p, "LogCallStack", C4AulExec::FnLogCallStack);
#undef F
}
