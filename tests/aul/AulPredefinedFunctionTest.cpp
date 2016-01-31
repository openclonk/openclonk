/*
* OpenClonk, http://www.openclonk.org
*
* Copyright (c) 2016, The OpenClonk Team and contributors
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

// Testing behaviour of some predefined C4Script functions.

#include "C4Include.h"
#include "AulTest.h"
#include "TestLog.h"

#include "script/C4Aul.h"

class AulPredefFunctionTest : public AulTest
{};

using ::testing::StartsWith;
using ::testing::AnyNumber;
using ::testing::_;

TEST_F(AulPredefFunctionTest, Translate)
{
	// Expect the engine to warn when it can't find a translation
	LogMock log;
	EXPECT_CALL(log, DebugLogF(R"(WARNING: Translate: no translation for string "%s")", _));
	EXPECT_CALL(log, DebugLog(StartsWith(" by: "))).Times(AnyNumber()); // ignore stack trace

	EXPECT_EQ(C4VString("a"), RunExpr("Translate(\"a\")"));
}

TEST_F(AulPredefFunctionTest, Min_int_int)
{
	// Test that Min(int, int) returns the smaller of two values.
	EXPECT_EQ(C4VInt(0), RunExpr("Min(0, 1)"));
	EXPECT_EQ(C4VInt(0), RunExpr("Min(1, 0)"));
	
	EXPECT_EQ(C4VInt(-1), RunExpr("Min(-1, 0)"));
	EXPECT_EQ(C4VInt(-1), RunExpr("Min(0, -1)"));

	EXPECT_EQ(C4VINT_MIN, RunExpr("Min(-2147483648, 0)"));
	EXPECT_EQ(C4VINT_MIN, RunExpr("Min(0, -2147483648)"));

	EXPECT_EQ(C4VInt(0), RunExpr("Min(2147483647, 0)"));
	EXPECT_EQ(C4VInt(0), RunExpr("Min(0, 2147483647)"));
}

TEST_F(AulPredefFunctionTest, Min_array)
{
	// Test that Min(array<int>) returns the smallest array item.
	EXPECT_EQ(C4VNull, RunExpr("Min([])"));

	EXPECT_EQ(C4VInt(0), RunExpr("Min([0, 1, 2])"));
	EXPECT_EQ(C4VInt(0), RunExpr("Min([2, 1, 0])"));

	EXPECT_EQ(C4VInt(-1), RunExpr("Min([-1, 0, 1])"));
	EXPECT_EQ(C4VInt(-1), RunExpr("Min([1, 0, -1])"));

	EXPECT_EQ(C4VINT_MIN, RunExpr("Min([-2147483648, 0, 2147483647])"));
	EXPECT_EQ(C4VINT_MIN, RunExpr("Min([2147483647, 0, -2147483648])"));
}

TEST_F(AulPredefFunctionTest, Min_Invalid)
{
	// Test that Min(array) fails if any item is not (convertible to) int.
	EXPECT_THROW(RunExpr("Min([[]])"), C4AulExecError);
	EXPECT_THROW(RunExpr("Min([0, []])"), C4AulExecError);
	EXPECT_THROW(RunExpr("Min([0, 1, {}])"), C4AulExecError);
	EXPECT_THROW(RunExpr(R"(Min([0, ""]))"), C4AulExecError);

	// Test that Min(a, b) fails if a or b are not (convertible to) int.
	EXPECT_THROW(RunExpr("Min({}, 1)"), C4AulExecError);
	EXPECT_THROW(RunExpr("Min(0, {})"), C4AulExecError);
	EXPECT_THROW(RunExpr(R"(Min("", 1))"), C4AulExecError);
	EXPECT_THROW(RunExpr(R"(Min(0, ""))"), C4AulExecError);
}

TEST_F(AulPredefFunctionTest, Max_int_int)
{
	// Test that Max(int, int) returns the larger of two values.
	EXPECT_EQ(C4VInt(1), RunExpr("Max(0, 1)"));
	EXPECT_EQ(C4VInt(1), RunExpr("Max(1, 0)"));

	EXPECT_EQ(C4VInt(0), RunExpr("Max(-1, 0)"));
	EXPECT_EQ(C4VInt(0), RunExpr("Max(0, -1)"));

	EXPECT_EQ(C4VInt(0), RunExpr("Max(-2147483648, 0)"));
	EXPECT_EQ(C4VInt(0), RunExpr("Max(0, -2147483648)"));

	EXPECT_EQ(C4VINT_MAX, RunExpr("Max(2147483647, 0)"));
	EXPECT_EQ(C4VINT_MAX, RunExpr("Max(0, 2147483647)"));
}

TEST_F(AulPredefFunctionTest, Max_array)
{
	// Test that Max(array<int>) returns the largest array item.
	EXPECT_EQ(C4VNull, RunExpr("Max([])"));

	EXPECT_EQ(C4VInt(2), RunExpr("Max([0, 1, 2])"));
	EXPECT_EQ(C4VInt(2), RunExpr("Max([2, 1, 0])"));

	EXPECT_EQ(C4VInt(1), RunExpr("Max([-1, 0, 1])"));
	EXPECT_EQ(C4VInt(1), RunExpr("Max([1, 0, -1])"));

	EXPECT_EQ(C4VINT_MAX, RunExpr("Max([-2147483648, 0, 2147483647])"));
	EXPECT_EQ(C4VINT_MAX, RunExpr("Max([2147483647, 0, -2147483648])"));
}

TEST_F(AulPredefFunctionTest, Max_Invalid)
{
	// Test that Max(array) fails if any item is not (convertible to) int.
	EXPECT_THROW(RunExpr("Max([[]])"), C4AulExecError);
	EXPECT_THROW(RunExpr("Max([0, []])"), C4AulExecError);
	EXPECT_THROW(RunExpr("Max([0, 1, {}])"), C4AulExecError);
	EXPECT_THROW(RunExpr(R"(Max([0, ""]))"), C4AulExecError);

	// Test that Max(a, b) fails if a or b are not (convertible to) int.
	EXPECT_THROW(RunExpr("Max({}, 1)"), C4AulExecError);
	EXPECT_THROW(RunExpr("Max(0, {})"), C4AulExecError);
	EXPECT_THROW(RunExpr(R"(Max("", 1))"), C4AulExecError);
	EXPECT_THROW(RunExpr(R"(Max(0, ""))"), C4AulExecError);
}

TEST_F(AulPredefFunctionTest, Abs)
{
	EXPECT_EQ(C4VInt(0), RunExpr("Abs()"));
	EXPECT_EQ(C4VInt(1), RunExpr("Abs(-1)"));
	EXPECT_EQ(C4VInt(1), RunExpr("Abs(1)"));
	EXPECT_EQ(C4VINT_MIN, RunExpr("Abs(-2147483648)"));
	EXPECT_EQ(C4VINT_MIN, RunExpr("Abs(2147483648)"));
	EXPECT_EQ(C4VINT_MAX, RunExpr("Abs(-2147483647)"));
	EXPECT_EQ(C4VINT_MAX, RunExpr("Abs(2147483647)"));
}

TEST_F(AulPredefFunctionTest, Trivial)
{
	EXPECT_EQ(C4VInt(100), RunExpr("Sin(900,100,10)"));
	EXPECT_EQ(C4VInt(0), RunExpr("Cos(900,100,10)"));
	EXPECT_EQ(C4VInt(4), RunExpr("Sqrt(16)"));
	EXPECT_EQ(C4VInt(141), RunExpr("Distance(0,0,100,100)"));
	EXPECT_EQ(C4VInt(13500), RunExpr("Angle(0,0,100,100,100)"));
	EXPECT_EQ(C4VInt(90), RunExpr("ArcSin(1,1)"));
	EXPECT_EQ(C4VInt(0), RunExpr("ArcCos(1,1)"));
	EXPECT_EQ(C4VInt(2), RunExpr("BoundBy(1,2,3)"));
	EXPECT_EQ(C4Value(false), RunExpr("Inside(1,2,3)"));
	EXPECT_EQ(C4VArray(new C4ValueArray), RunExpr("CreateArray()"));
	EXPECT_EQ(C4VPropList(C4PropList::New(0)), RunExpr("CreatePropList()"));
	EXPECT_EQ(C4VInt(42), RunExpr("GetProperty(\"a\",{a=42})"));
	EXPECT_EQ(C4VInt(42), RunCode("var p = {}; SetProperty(\"a\",42,p); return p.a;"));
	EXPECT_EQ(C4Value(), RunCode("var p = {a=42}; ResetProperty(\"a\",p); return p.a;"));
	EXPECT_EQ(C4VInt('a'), RunExpr("GetChar(\"a\")"));
	EXPECT_EQ(C4VInt(C4V_Int), RunExpr("GetType(42)"));
	EXPECT_EQ(C4VInt(0), RunExpr("ModulateColor()"));
	EXPECT_EQ(C4VInt(1), RunExpr("WildcardMatch(\"a\",\"*\")"));
	EXPECT_EQ(C4VInt(1), RunExpr("GetLength([0])"));
	EXPECT_EQ(C4VNull, RunCode("var a=[0]; SetLength(a,0); return a[0];"));
	EXPECT_EQ(C4VInt(-1), RunExpr("GetIndexOf([42],{})"));
	EXPECT_EQ(C4VInt(0), RunExpr("GetIndexOf([42],42)"));
	EXPECT_EQ(C4Value(true), RunExpr("DeepEqual({a=[42]},{a=[42]})"));
	EXPECT_EQ(C4Value(false), RunExpr("DeepEqual([1,2,3],[3,2,1])"));
	EXPECT_EQ(C4Value(true), RunCode("var a = [3,2,1]; SortArray(a); return DeepEqual([1,2,3], a);"));
	EXPECT_EQ(C4Value(true), RunCode("var a = [{a=2},{a=1}]; SortArrayByProperty(a,\"a\"); return DeepEqual([{a=1},{a=2}], a);"));
	EXPECT_EQ(C4Value(true), RunCode("var a = [[1,2],[3,1]]; SortArrayByArrayElement(a,1); return DeepEqual([[3,1],[1,2]], a);"));
	EXPECT_EQ(C4Value(true), RunExpr("DeepEqual([\"a\",\"b\"], GetProperties({a=1,b=2}))"));
}
