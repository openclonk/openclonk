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

#include "script/C4Aul.h"

class AulPredefFunctionTest : public AulTest
{};

TEST_F(AulPredefFunctionTest, Min)
{
	EXPECT_EQ(C4VInt(0), RunExpr("Min(0, 1)"));
	EXPECT_EQ(C4VInt(0), RunExpr("Min(1, 0)"));
	
	EXPECT_EQ(C4VInt(-1), RunExpr("Min(-1, 0)"));
	EXPECT_EQ(C4VInt(-1), RunExpr("Min(0, -1)"));

	EXPECT_EQ(C4VINT_MIN, RunExpr("Min(-2147483648, 0)"));
	EXPECT_EQ(C4VINT_MIN, RunExpr("Min(0, -2147483648)"));

	EXPECT_EQ(C4VInt(0), RunExpr("Min(2147483647, 0)"));
	EXPECT_EQ(C4VInt(0), RunExpr("Min(0, 2147483647)"));
}

TEST_F(AulPredefFunctionTest, Max)
{
	EXPECT_EQ(C4VInt(1), RunExpr("Max(0, 1)"));
	EXPECT_EQ(C4VInt(1), RunExpr("Max(1, 0)"));

	EXPECT_EQ(C4VInt(0), RunExpr("Max(-1, 0)"));
	EXPECT_EQ(C4VInt(0), RunExpr("Max(0, -1)"));

	EXPECT_EQ(C4VInt(0), RunExpr("Max(-2147483648, 0)"));
	EXPECT_EQ(C4VInt(0), RunExpr("Max(0, -2147483648)"));

	EXPECT_EQ(C4VINT_MAX, RunExpr("Max(2147483647, 0)"));
	EXPECT_EQ(C4VINT_MAX, RunExpr("Max(0, 2147483647)"));
}
