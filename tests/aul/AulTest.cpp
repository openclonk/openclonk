/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2015-2016, The OpenClonk Team and contributors
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

#include "AulTest.h"

#include "script/C4ScriptHost.h"
#include "lib/C4Random.h"
#include "object/C4DefList.h"

C4Value AulTest::RunCode(const char *code, bool wrap)
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
	ScriptEngine.Link(NULL);
		
	return GameScript.Call("Main", nullptr, true);
}
C4Value AulTest::RunExpr(const char *expr)
{
	std::string code = "return ";
	code += expr;
	code += ';';
	return RunCode(code.c_str());
}

const C4Value AulTest::C4VINT_MIN = C4VInt(-2147483647 - 1);
const C4Value AulTest::C4VINT_MAX = C4VInt(2147483647);

TEST_F(AulTest, ValueReturn)
{
	// Make sure primitive value returns work.
	EXPECT_EQ(C4VNull, RunCode("return;"));
	EXPECT_EQ(C4VNull, RunExpr("nil"));
	EXPECT_EQ(C4Value(true), RunExpr("true"));
	EXPECT_EQ(C4Value(false), RunExpr("false"));
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

TEST_F(AulTest, Loops)
{
	EXPECT_EQ(C4VInt(5), RunCode("var i = 0; do ++i; while (i < 5); return i;"));
	EXPECT_EQ(C4VInt(5), RunCode("var i = 0; while (i < 5) ++i; return i;"));
	EXPECT_EQ(C4VInt(5), RunCode("for(var i = 0; i < 5; ++i); return i;"));
	EXPECT_EQ(C4VInt(6), RunCode("var i = 0, b; do { b = i++ >= 5; } while (!b); return i;"));
	EXPECT_EQ(C4VInt(6), RunCode("var i = 0, b; while (!b) { b = i++ >= 5; } return i;"));
	EXPECT_EQ(C4Value(), RunCode("var a = [], sum; for(var i in a) sum += i; return sum;"));
	EXPECT_EQ(C4VInt(1), RunCode("var a = [1], sum; for(var i in a) sum += i; return sum;"));
	EXPECT_EQ(C4VInt(6), RunCode("var a = [1,2,3], sum; for(var i in a) sum += i; return sum;"));
}

TEST_F(AulTest, Locals)
{
	EXPECT_EQ(C4VInt(42), RunCode("local i = 42; func Main() { return i; }", false));
	EXPECT_EQ(C4VInt(42), RunCode("local i; func Main() { i = 42; return i; }", false));
	EXPECT_EQ(C4VInt(42), RunCode("func Main() { local i = 42; return i; }", false));
	EXPECT_EQ(C4VInt(42), RunCode("local i = [42]; func Main() { return i[0]; }", false));
	EXPECT_EQ(C4VInt(42), RunCode("local p = { i = 42 }; func Main() { return p.i; }", false));
	EXPECT_EQ(C4VInt(42), RunCode("local p1 = { i = 42 }, p2 = new p1 {}; func Main() { return p2.i; }", false));
}

TEST_F(AulTest, Eval)
{
	EXPECT_EQ(C4VInt(42), RunExpr("eval(\"42\")"));
	EXPECT_EQ(C4VInt(42), RunCode("local i = 42; func Main() { return eval(\"this.i\"); }", false));
	EXPECT_EQ(C4VInt(42), RunCode("local i; func Main() { eval(\"this.i = 42\"); return i; }", false));
}
