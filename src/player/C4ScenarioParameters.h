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

/* Structures for object and player info components */

#ifndef INC_C4ScenarioParameters
#define INC_C4ScenarioParameters

// Definition for a custom setting for the scenario
class C4ScenarioParameterDef
{
public:
	// what kind of parameter?
	enum ParameterType
	{
		SPDT_Enum, // only one type so far
	};

	// single option for an enum type parameter
	struct Option
	{
		int32_t Value; // integer value that will be assigned to the script constant
		StdCopyStrBuf Name; // localized name
		StdCopyStrBuf Description; // localized description. to be displayed as hover text for this option.

		void CompileFunc(StdCompiler *pComp);
	};

private:
	StdCopyStrBuf Name; // localized name
	StdCopyStrBuf Description; // localized description. to be displayed as hover text for this parameter input control
	StdCopyStrBuf ID; // Identifier for value storage and script access
	ParameterType Type; // Type of parameter. Always enum.

	std::vector<Option> Options; // possible options to be selected for an enum type
	int32_t Default; // value of option selected by default for an enum type
	int32_t LeagueValue; // if nonzero, option is forced to this value in league games

	StdCopyStrBuf Achievement; // if this parameter is an achievement, this string contains the name of the achievement graphics to be used

public:
	C4ScenarioParameterDef() : Default(0), LeagueValue(0) {} 
	~C4ScenarioParameterDef() {}

	const char *GetName() const { return Name.getData(); }
	const char *GetDescription() const { return Description.getData(); }
	const char *GetID() const { return ID.getData(); }
	ParameterType GetType() const { return Type; }
	int32_t GetDefault() const { return Default; }
	int32_t GetLeagueValue() const { return LeagueValue; }
	const Option *GetOptionByValue(int32_t val) const;
	const Option *GetOptionByIndex(size_t idx) const;

	bool IsAchievement() const { return Achievement.getLength()>0; }
	const char *GetAchievement() const { return Achievement.getData(); }

	void CompileFunc(StdCompiler *pComp);
};

// Definitions of custom parameters that can be set before scenario start
class C4ScenarioParameterDefs
{
	std::vector<C4ScenarioParameterDef> Parameters;

public:
	C4ScenarioParameterDefs() {}
	~C4ScenarioParameterDefs() {}

	void Clear() { Parameters.clear(); }

	const C4ScenarioParameterDef *GetParameterDefByIndex(size_t idx) const;

	bool Load(C4Group &hGroup, class C4LangStringTable *pLang);
	void CompileFunc(StdCompiler *pComp);

	void RegisterScriptConstants(const class C4ScenarioParameters &values); // register constants for all parameters in script engine
};

// Parameter values that correspond to settings offered in C4ScenarioParameterDefs
class C4ScenarioParameters
{
	std::map<StdCopyStrBuf, int32_t> Parameters;

public:
	C4ScenarioParameters() {}
	~C4ScenarioParameters() {}

	void Clear();
	void Merge(const C4ScenarioParameters &other);

	int32_t GetValueByID(const char *id, int32_t default_value) const;
	void SetValue(const char *id, int32_t value, bool only_if_larger);

	void CompileFunc(StdCompiler *pComp);

	static StdStrBuf AddFilename2ID(const char *filename, const char *id);
};



#endif // INC_C4ScenarioParameters
