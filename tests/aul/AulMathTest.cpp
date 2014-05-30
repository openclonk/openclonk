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

#include "C4Include.h"
#include "AulTest.h"

#include "script/C4Aul.h"

class AulMathTest : public AulTest
{};

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
	EXPECT_EQ(C4Value(true), RunExpr("2147483647 + 1 + 1 == 2147483647 + 2"));
	EXPECT_EQ(C4Value(true), RunExpr("-2147483648 - 1 - 1 == -2147483648 - 2"));
}
