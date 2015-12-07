/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2015, The OpenClonk Team and contributors
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

// Testing C4Aul behaviour.

#include <C4Include.h>
#include <gtest/gtest.h>
#include <iostream>

#include "gamescript/C4Script.h"
#include "script/C4ScriptHost.h"
#include "lib/C4Random.h"
#include "object/C4DefList.h"
#include "script/C4Value.h"

std::ostream &operator<<(std::ostream &os, const C4Value &val)
{
	return os << val.GetDataString().getData();
}

class C4AulTest : public ::testing::Test
{
protected:
	C4Value RunCode(const char *code, bool wrap = true)
	{
		class OnScopeExit
		{
		public:
			~OnScopeExit()
			{
				GameScript.Clear();
				ScriptEngine.Clear();
			}
		} _cleanup;

		InitCoreFunctionMap(&ScriptEngine);
		FixedRandom(0x40490fdb);

		std::string wrapped;
		if (wrap)
		{
			wrapped = "func Main() {\n";
			wrapped += code;
			wrapped += "\n}\n";
		}
		else
		{
			wrapped = code;
		}
		std::string src("<");
		auto test_info = ::testing::UnitTest::GetInstance()->current_test_info();
		src += test_info->test_case_name();
		src += "::";
		src += test_info->name();
		src += ">";

		GameScript.LoadData(src.c_str(), wrapped.c_str(), NULL);
		ScriptEngine.Link(&::Definitions);
		ScriptEngine.GlobalNamed.SetNameList(&ScriptEngine.GlobalNamedNames);
		
		return GameScript.Call("Main", nullptr, true);
	}
	C4Value RunExpr(const char *expr)
	{
		std::string code = "return ";
		code += expr;
		code += ';';
		return RunCode(code.c_str());
	}
};

namespace {
	void _setItems(C4ValueArray *a, int i) {}
	template<class T0, class ...T>
	void _setItems(C4ValueArray *a, int i, T0 &&v, T &&...t)
	{
		a->SetItem(i, v);
		_setItems(a, ++i, std::forward<T>(t)...);
	}

	// Helper functions to create array C4Values inline
	template<class ...T>
	C4Value C4VArray(T &&...v)
	{
		C4ValueArray *a = new C4ValueArray(sizeof...(v));
		_setItems(a, 0, std::forward<T>(v)...);
		return C4VArray(a);
	}
}

namespace {
	void _setItems(C4PropList *p) {}
	template<class T0, class T1, class ...T>
	void _setItems(C4PropList *p, T0 &&k, T1 &&v, T &&...t)
	{
		p->SetPropertyByS(::Strings.RegString(k), v);
		_setItems(p, std::forward<T>(t)...);
	}

	// Helper function to create proplist C4Values inline
	template<class ...T>
	C4Value C4VPropList(T &&...v)
	{
		static_assert(sizeof...(v) % 2 == 0, "Proplist constructor needs even number of arguments");
		C4PropList *p = C4PropList::New();
		_setItems(p, std::forward<T>(v)...);
		return C4VPropList(p);
	}
}

TEST_F(C4AulTest, ValueReturn)
{
	// Make sure primitive value returns work.
	EXPECT_EQ(C4VNull, RunCode("return;"));
	EXPECT_EQ(C4VNull, RunExpr("nil"));
	EXPECT_EQ(C4VTrue, RunExpr("true"));
	EXPECT_EQ(C4VFalse, RunExpr("false"));
	EXPECT_EQ(C4VInt(42), RunExpr("42"));
	EXPECT_EQ(C4VString("Hello World!"), RunExpr("\"Hello World!\""));

	// Make sure array returns work.
	EXPECT_EQ(C4VArray(), RunExpr("[]"));
	EXPECT_EQ(
		C4VArray(C4VInt(0), C4VNull, C4VArray(C4VInt(1)), C4VString("Hi")),
		RunExpr("[0, nil, [1], \"Hi\"]"));

	// Make sure proplist returns work.
	EXPECT_EQ(C4VPropList(), RunExpr("{}"));
	EXPECT_EQ(
		C4VPropList("a", C4VInt(1), "b", C4VArray()),
		RunExpr("{\"a\": 1, \"b\"=[]}"));
}

TEST_F(C4AulTest, Translate)
{
	EXPECT_EQ(C4VString("a"), RunExpr("Translate(\"a\")"));
}

class AulMathTest : public C4AulTest
{
protected:
	static const C4Value C4VINT_MIN;
	static const C4Value C4VINT_MAX;
};
const C4Value AulMathTest::C4VINT_MIN = C4VInt(-2147483647 - 1);
const C4Value AulMathTest::C4VINT_MAX = C4VInt(2147483647);

TEST_F(AulMathTest, Addition)
{
	// Adding 0 to a value should return the same value.
	EXPECT_EQ(C4VInt(0), RunExpr("0 + 0"));
	EXPECT_EQ(C4VINT_MAX, RunExpr("2147483647 + 0"));
	EXPECT_EQ(C4VINT_MAX, RunExpr("0 + 2147483647"));
	EXPECT_EQ(C4VINT_MIN, RunExpr("-2147483648 + 0"));
	EXPECT_EQ(C4VINT_MIN, RunExpr("0 + -2147483648"));

	EXPECT_EQ(C4VInt(1078530011), RunExpr("1078530011 + 0"));
	EXPECT_EQ(C4VInt(1078530011), RunExpr("0 + 1078530011"));
	EXPECT_EQ(C4VInt(1068827891), RunExpr("1068827891 + 0"));
	EXPECT_EQ(C4VInt(1068827891), RunExpr("0 + 1068827891"));
}

TEST_F(AulMathTest, Division)
{
	// Dividing a value by 0 should fail.
	EXPECT_THROW(RunExpr("1 / 0"), C4AulExecError);
	// Dividing C4VINT_MIN by -1 should fail.
	EXPECT_THROW(RunExpr("-2147483648 / -1"), C4AulExecError);
	// Dividing C4VINT_MIN by 1 should return C4VINT_MIN.
	EXPECT_EQ(C4VINT_MIN, RunExpr("-2147483648 / 1"));
}

TEST_F(AulMathTest, Bug1389)
{
	// 0001389: Aul integer overflow results in strange numbers on 64 bit
	// machines

	// "Strange numbers" shall mean numbers that are impossible to express in
	// C4Script as an integer literal. In particular, in these cases, the
	// high dword of the numbers, which is not exposed to C4Script, is not
	// guaranteed to be zero. Therefore, straight comparison of these numbers
	// will result in unexpected results.

	EXPECT_EQ(C4VINT_MIN, RunExpr("2147483647 + 1"));
	EXPECT_EQ(C4VINT_MAX, RunExpr("-2147483648 - 1"));
	// x ± 1 ± 1 is handled differently from x ± 2, yet the result should be
	// the same.
	EXPECT_EQ(C4VTrue, RunExpr("2147483647 + 1 + 1 == 2147483647 + 2"));
	EXPECT_EQ(C4VTrue, RunExpr("-2147483648 - 1 - 1 == -2147483648 - 2"));
}
