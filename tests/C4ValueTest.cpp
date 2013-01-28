/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2013 Oliver Schneider
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * See isc_license.txt for full license and disclaimer.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 * See clonk_trademark_license.txt for full license.
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
