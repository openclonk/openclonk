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
#include "TestLog.h"

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
	src += "::";
	src += std::to_string(test_info->result()->total_part_count());
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
	EXPECT_EQ(C4VInt(-6), RunCode(R"(
var a = [-3, -2, -1, 0, 1, 2, 3], b;
for (var i in a) {
	if (i > 0) break;
	b += i;
}
return b;
)"));
	EXPECT_EQ(C4VInt(0), RunCode(R"(
var a = [-3, -2, -1, 0, 1, 2, 3], b;
for (var i in a) {
	if (i < -1) continue;
	if (i > 1) break;
	b += i;
}
return b;
)"));
	// Test nested loops
	EXPECT_EQ(C4VInt(-6), RunCode(R"(
var a = [[-3, -2], [-1, 0], [1, 2, 3]], b;
for (var i in a) {
	for (var j in i) {
		if (j > 0) break;
		b += j;
	}
}
return b;
)"));
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

TEST_F(AulTest, Vars)
{
	EXPECT_EQ(C4VInt(42), RunCode("var i = 21; i = i + i; return i;"));
	EXPECT_EQ(C4VInt(42), RunCode("var i = -42; i = Abs(i); return i;"));
}

TEST_F(AulTest, ParameterPassing)
{
	EXPECT_EQ(C4VArray(
		C4VInt(1), C4VInt(2), C4VInt(3), C4VInt(4), C4VInt(5),
		C4VInt(6), C4VInt(7), C4VInt(8), C4VInt(9), C4VInt(10)),
		RunCode(R"(
func f(...)
{
	return [Par(0), Par(1), Par(2), Par(3), Par(4), Par(5), Par(6), Par(7), Par(8), Par(9)];
}

func Main()
{
	return f(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
}
)", false));

	EXPECT_EQ(C4VArray(
		C4VInt(1), C4VInt(2), C4VInt(3), C4VInt(4), C4VInt(5),
		C4VInt(6), C4VInt(7), C4VInt(8), C4VInt(9), C4VInt(10)),
		RunCode(R"(
func f(a, b, ...)
{
	return g(b, a, ...);
}

func g(...)
{
	return [Par(0), Par(1), Par(2), Par(3), Par(4), Par(5), Par(6), Par(7), Par(8), Par(9)];
}

func Main()
{
	return f(2, 1, 3, 4, 5, 6, 7, 8, 9, 10);
}
)", false));
}

TEST_F(AulTest, Conditionals)
{
	EXPECT_EQ(C4VInt(1), RunCode("if (true) return 1; else return 2;"));
	EXPECT_EQ(C4VInt(2), RunCode("if (false) return 1; else return 2;"));
}

TEST_F(AulTest, Warnings)
{
	LogMock log;
	EXPECT_CALL(log, DebugLog(testing::StartsWith("WARNING:"))).Times(3);
	EXPECT_EQ(C4Value(), RunCode("func Main(string s, object o, array a) { Sin(s); }", false));
	EXPECT_EQ(C4Value(), RunCode("func Main(string s, object o, array a) { Sin(o); }", false));
	EXPECT_EQ(C4Value(), RunCode("func Main(string s, object o, array a) { Sin(a); }", false));
}

TEST_F(AulTest, NoWarnings)
{
	LogMock log;
	EXPECT_CALL(log, DebugLog(testing::StartsWith("WARNING:"))).Times(0);
	EXPECT_EQ(C4Value(), RunCode("func Main(string s, object o, array a) { var x; Sin(x); }", false));
}
