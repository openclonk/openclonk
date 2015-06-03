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
#include "script/C4Value.h"

#include <gtest/gtest.h>

TEST(C4ValueTest, SanityTests)
{
	srand(time(nullptr));
	int rnd = rand();
	// two C4Values constructed with same parameter are equal
	EXPECT_TRUE(C4Value(rnd) == C4Value(rnd));
	EXPECT_EQ(C4Value(rnd), C4Value(rnd));
	EXPECT_FALSE(C4Value(rnd) != C4Value(rnd));
	int rnd2 = rand();
	while(rnd2 == rnd) rnd2 = rand();
	EXPECT_FALSE(C4Value(rnd) == C4Value(rnd2));
	EXPECT_TRUE(C4Value(42));
	EXPECT_FALSE(C4Value(0));
	EXPECT_TRUE(C4Value(true));
	EXPECT_FALSE(C4Value(false));
}
