/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2013, The OpenClonk Team and contributors
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

#include <C4Include.h>
#include "script/C4Aul.h"
#include "script/C4AulExec.h"

#include <gtest/gtest.h>

TEST(DirectExecTest, SanityTests)
{
	C4Value rVal(AulExec.DirectExec(nullptr, "5*8", "unit test script", false, nullptr));
	EXPECT_EQ(rVal, C4Value(5*8));
}

template<typename T>
bool operator==(const C4Set<T>& lhs, const C4Set<T>& rhs)
{
	if (lhs.GetSize() != rhs.GetSize()) return false;
	auto lit = lhs.First();
	auto rit = rhs.First();
	while(!!lit) {
		if (!C4Set<T>::Equals(*lit, *rit)) return false;
		lit = lhs.Next(lit);
		rit = lhs.Next(rit);
	}
	return true;
}

#include "script/C4ScriptHost.h"
class TestHost : public C4ScriptHost
{
public:
	void test_equality(const TestHost& rhs) const
	{
		// C4ScriptHost
		EXPECT_EQ(Includes, rhs.Includes);
		EXPECT_EQ(Appends, rhs.Appends);
		EXPECT_EQ(Script, rhs.Script);
		EXPECT_EQ(Resolving, rhs.Resolving);
		EXPECT_EQ(IncludesResolved, rhs.IncludesResolved);
		EXPECT_EQ(LocalValues, rhs.LocalValues);

		// C4AulScript
		EXPECT_EQ(ScriptName, rhs.ScriptName);
	}
	virtual bool Parse() { ADD_FAILURE() << "tried to call Parse()"; return false; }
	virtual void UnLink() { FAIL() << "tried to call UnLink()"; }
	virtual bool Load(C4Group &hGroup, const char *szFilename,
	          const char *szLanguage, C4LangStringTable *pLocalTable)
			  { ADD_FAILURE() << "tried to call Load()"; return false; }


	virtual void Clear() { FAIL() << "tried to call Clear()";}
	virtual void ResetProfilerTimes() { FAIL() << "tried to call ResetProfilerTimes()"; }
	virtual void CollectProfilerTimes(class C4AulProfiler &rProfiler) { FAIL() << "tried to call CollectProfilerTimes()"; }
	virtual bool ReloadScript(const char *szPath, const char *szLanguage){ ADD_FAILURE() << "tried to call ReloadScript()"; return false; }
	virtual bool ResolveIncludes(C4DefList *rDefs){ ADD_FAILURE() << "tried to call ResolveIncludes()"; return false; }
	virtual bool ResolveAppends(C4DefList *rDefs){ ADD_FAILURE() << "tried to call ResolveAppends()"; return false; }
};

TEST(DirectExecTest, HostUnmodifedByParseTest)
{
	TestHost host;
	TestHost host2;
	host.test_equality(host2);
	char szScript[] = "8*5";
	C4AulScriptFunc *pFunc = new C4AulScriptFunc(host.GetPropList(), nullptr, nullptr, szScript);
	host.test_equality(host2);
	pFunc->ParseDirectExecStatement(&::ScriptEngine);
	host.test_equality(host2);
	delete pFunc;
}
