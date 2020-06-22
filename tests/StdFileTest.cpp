/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2019, The OpenClonk Team and contributors
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
#include "platform/StdFile.h"

#include <gtest/gtest.h>

TEST(StdFileTest, IsWildcardStringTest)
{
	EXPECT_TRUE(IsWildcardString("ab*cde"));
	EXPECT_TRUE(IsWildcardString("abcd?e"));
	EXPECT_TRUE(IsWildcardString("[abc]de"));
	EXPECT_FALSE(IsWildcardString("foobar"));
}

TEST(StdFileTest, WildcardMatchTest)
{
	EXPECT_TRUE(WildcardMatch("abc*", "abcdefg"));
	EXPECT_FALSE(WildcardMatch("abc*", "Xabcdefg"));
	EXPECT_TRUE(WildcardMatch("a?c*g", "abcdefg"));
	EXPECT_TRUE(WildcardMatch("a[1-9]?", "a5b"));
	EXPECT_TRUE(WildcardMatch("a[abc][A-Z]", "acX"));
	EXPECT_TRUE(WildcardMatch("[[]", "["));
	EXPECT_TRUE(WildcardMatch("[[-]", "-"));
}
