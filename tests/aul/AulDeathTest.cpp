/*
* OpenClonk, http://www.openclonk.org
*
* Copyright (c) 2016-2019, The OpenClonk Team and contributors
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

#include <cstdlib>
#include "script/C4Aul.h"

class AulDeathTest : public AulTest
{};

// DEATH_SUCCESS_CODE is arbitrarily chosen such that it's unlikely to be
// returned by failing code.
#define DEATH_SUCCESS_CODE 86
#define EXPECT_NO_DEATH(code) EXPECT_EXIT({code; std::quick_exit(DEATH_SUCCESS_CODE);}, ::testing::ExitedWithCode(DEATH_SUCCESS_CODE), "")

TEST_F(AulDeathTest, NestedFunctions)
{
	// Ensures the engine does not crash when a function is declared
	// inside a proplist inside a function.
	EXPECT_NO_DEATH(RunCode("local a = {b = func () {} };"));
}

TEST_F(AulDeathTest, issue1891)
{
	// 1891: NULL dereference when warning is emitted in eval()
	EXPECT_NO_DEATH(
		try
		{
			RunExpr("eval(\"Sin(\\\"\\\")\")");
		}
		catch (C4AulExecError &)
		{}
	);
}

TEST_F(AulDeathTest, SetLengthWithNil)
{
	// Github #79: NULL dereference when SetLength is called with nil parameter
	EXPECT_NO_DEATH(
		try
		{
			RunExpr("SetLength(nil, 0)");
		}
		catch (C4AulExecError &)
		{}
	);
}
