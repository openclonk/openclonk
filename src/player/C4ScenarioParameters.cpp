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
#include "player/C4ScenarioParameters.h"
#include "c4group/C4Components.h"
#include "script/C4Aul.h"

// *** C4ScenarioParameters

void C4ScenarioParameterDef::Option::CompileFunc(StdCompiler *pComp)
{
	if (!pComp->Name("Option")) { pComp->NameEnd(); pComp->excNotFound("Option"); }
	pComp->Value(mkNamingAdapt(mkParAdapt(Name, StdCompiler::RCT_All),          "Name",         StdCopyStrBuf()));
	pComp->Value(mkNamingAdapt(mkParAdapt(Description, StdCompiler::RCT_All),   "Description",  StdCopyStrBuf()));
	pComp->Value(mkNamingAdapt(           Value,                                "Value",        0));
	pComp->NameEnd();
}

const C4ScenarioParameterDef::Option *C4ScenarioParameterDef::GetOptionByValue(int32_t val) const
{
	// search option by value
	for (auto i = Options.cbegin(); i != Options.cend(); ++i)
		if (i->Value == val)
			return &*i;
	return nullptr;
}

const C4ScenarioParameterDef::Option *C4ScenarioParameterDef::GetOptionByIndex(size_t idx) const
{
	if (idx >= Options.size()) return nullptr;
	return &Options[idx];
}

void C4ScenarioParameterDef::CompileFunc(StdCompiler *pComp)
{
	if (!pComp->Name("ParameterDef")) { pComp->NameEnd(); pComp->excNotFound("ParameterDef"); }
	pComp->Value(mkNamingAdapt(mkParAdapt(Name, StdCompiler::RCT_All),          "Name",         StdCopyStrBuf()));
	pComp->Value(mkNamingAdapt(mkParAdapt(Description, StdCompiler::RCT_All),   "Description",  StdCopyStrBuf()));
	pComp->Value(mkNamingAdapt(mkParAdapt(ID, StdCompiler::RCT_Idtf),           "ID",           StdCopyStrBuf()));
	StdEnumEntry<ParameterType> ParTypeEntries[] =
	{
		{ "Enumeration", SPDT_Enum },
		{ nullptr, SPDT_Enum }
	};
	pComp->Value(mkNamingAdapt(mkEnumAdaptT<uint8_t>(Type, ParTypeEntries),         "Type",         SPDT_Enum));
	pComp->Value(mkNamingAdapt(Default,                                             "Default",      0));
	pComp->Value(mkNamingAdapt(LeagueValue,                                         "LeagueValue",  0));
	pComp->Value(mkNamingAdapt(mkParAdapt(Achievement, StdCompiler::RCT_Idtf),      "Achievement",  StdCopyStrBuf()));
	pComp->Value(mkNamingAdapt(mkSTLContainerAdapt(Options, StdCompiler::SEP_NONE), "Options"));
	pComp->NameEnd();
}

void C4ScenarioParameterDefs::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkSTLContainerAdapt(Parameters, StdCompiler::SEP_NONE));
}

const C4ScenarioParameterDef *C4ScenarioParameterDefs::GetParameterDefByIndex(size_t idx) const
{
	if (idx >= Parameters.size()) return nullptr;
	return &Parameters[idx];
}

bool C4ScenarioParameterDefs::Load(C4Group &hGroup, C4LangStringTable *pLang)
{
	// Load buffer, localize and parse
	StdStrBuf Buf;
	if (!hGroup.LoadEntryString(C4CFN_ScenarioParameterDefs,&Buf)) return false;
	if (pLang) pLang->ReplaceStrings(Buf);
	if (!CompileFromBuf_LogWarn<StdCompilerINIRead>(*this, Buf, C4CFN_ScenarioParameterDefs))
		{ return false; }
	return true;
}

void C4ScenarioParameterDefs::RegisterScriptConstants(const C4ScenarioParameters &values)
{
	// register constants for all parameters in script engine

	// old-style: one constant per parameter
	for (auto i = Parameters.cbegin(); i != Parameters.cend(); ++i)
	{
		StdStrBuf constant_name;
		constant_name.Format("SCENPAR_%s", i->GetID());
		int32_t constant_value = values.GetValueByID(i->GetID(), i->GetDefault());
		::ScriptEngine.RegisterGlobalConstant(constant_name.getData(), C4VInt(constant_value));
	}

	// new-style: all constants in a proplist
	auto scenpar = C4PropList::NewStatic(nullptr, nullptr, &Strings.P[P_SCENPAR]);
	for (auto i = Parameters.cbegin(); i != Parameters.cend(); ++i)
	{
		int32_t constant_value = values.GetValueByID(i->GetID(), i->GetDefault());
		scenpar->SetPropertyByS(Strings.RegString(StdStrBuf(i->GetID())), C4VInt(constant_value));
	}
	scenpar->Freeze();
	::ScriptEngine.RegisterGlobalConstant("SCENPAR", C4Value(scenpar));
}

void C4ScenarioParameters::Clear()
{
	Parameters.clear();
}

void C4ScenarioParameters::Merge(const C4ScenarioParameters &other)
{
	// Merge lists and keep larger value
	for (auto i = other.Parameters.cbegin(); i != other.Parameters.cend(); ++i)
	{
		auto j = Parameters.find(i->first);
		if (j != Parameters.end())
			if (j->second >= i->second)
				continue; // existing value is same or larger - keep old
		// update to new value from other list
		Parameters[i->first] = i->second;
	}
}

int32_t C4ScenarioParameters::GetValueByID(const char *id, int32_t default_value) const
{
	// return map value if name is in map. Otherwise, return default value.
	auto i = Parameters.find(StdStrBuf(id));
	if (i != Parameters.end()) return i->second; else return default_value;
}

void C4ScenarioParameters::SetValue(const char *id, int32_t value, bool only_if_larger)
{
	if (only_if_larger)
	{
		auto i = Parameters.find(StdStrBuf(id));
		if (i != Parameters.end())
			if (i->second >= value)
				// could become smaller. don't set.
				return;
	}
	// just update map
	Parameters[StdCopyStrBuf(id)] = value;
}

void C4ScenarioParameters::CompileFunc(StdCompiler *pComp)
{
	// Unfortunately, StdCompiler cannot save std::map yet
	if (pComp->isDeserializer())
	{
		Parameters.clear();
		if (pComp->hasNaming())
		{
			// load from INI
			size_t name_count = pComp->NameCount();
			for (size_t i=0; i<name_count; ++i)
			{
				int32_t v=0;
				const char *name = pComp->GetNameByIndex(0); // always get name index 0, because names are removed after values have been extracted for them
				StdCopyStrBuf sName(name);
				if (!name) continue;
				pComp->Value(mkNamingAdapt(v, sName.getData(), 0));
				Parameters[sName] = v;
			}
		}
		else
		{
			// load from binary
			int32_t name_count=0;
			pComp->Value(name_count);
			for (int32_t i=0; i<name_count; ++i)
			{
				StdCopyStrBuf name; int32_t v;
				pComp->Value(name);
				pComp->Value(v);
				Parameters[name] = v;
			}
		}
	}
	else
	{
		if (pComp->hasNaming())
		{
			// save to INI
			for (auto i = Parameters.begin(); i != Parameters.end(); ++i)
				pComp->Value(mkNamingAdapt(i->second, i->first.getData()));
		}
		else
		{
			// save to binary
			int32_t name_count=Parameters.size();
			pComp->Value(name_count);
			for (auto i = Parameters.begin(); i != Parameters.end(); ++i)
			{
				pComp->Value(const_cast<StdCopyStrBuf &>(i->first));
				pComp->Value(i->second);
			}
		}
	}
}

StdStrBuf C4ScenarioParameters::AddFilename2ID(const char *filename, const char *id)
{
	// composes an ID string that contains both the relevant part of the filename and the ID
	// we care for .oc* folders only
	StdStrBuf sResult, sSource(filename, true), sPart;
	sSource.ReplaceChar(AltDirectorySeparator, DirectorySeparator);
	size_t idx=0;
	while (sSource.GetSection(idx++, &sPart, DirectorySeparator))
	{
		size_t len = sPart.getLength();
		if (len > 4 && SEqual2NoCase(sPart.getPtr(len - 4), ".oc", 3))
		{
			// .oc* folders separated by underscores
			sResult.Append(sPart.getData(), len - 4);
			sResult.AppendChar('_');
		}
	}
	sResult.Append(id);
	return sResult;
}
