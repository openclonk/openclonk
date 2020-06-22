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

static void PrintTo(const StdStrBuf &v, std::ostream *stream)
{
	*stream << testing::PrintToString(v.getData());
}

TEST(C4ValueTest, ToJSON)
{
	// can't use raw strings that contains \" because MSVC 2015's preprocessor fails on those

	// simple values
	EXPECT_EQ(C4Value(42).ToJSON(), "42");
	EXPECT_EQ(C4Value(-42).ToJSON(), "-42");
	EXPECT_EQ(C4Value("foobar").ToJSON(), R"#("foobar")#");
	EXPECT_EQ(C4Value("es\"caping").ToJSON(), "\"es\\\"caping\"");
	EXPECT_EQ(C4Value("es\\caping").ToJSON(), R"#("es\\caping")#");
	EXPECT_EQ(C4Value("new\nline").ToJSON(), R"#("new\nline")#");
	EXPECT_EQ(C4Value(true).ToJSON(), R"#(true)#");
	EXPECT_EQ(C4Value(false).ToJSON(), R"#(false)#");
	EXPECT_EQ(C4Value().ToJSON(), R"#(null)#");

	// proplists
	{
		auto proplist = C4PropList::NewStatic(nullptr, nullptr, nullptr);
		proplist->SetProperty(P_Options, C4Value("options"));
		proplist->SetProperty(P_Min, C4Value(13));
		auto nested = C4PropList::NewStatic(nullptr, nullptr, nullptr);
		nested->SetProperty(P_Description, C4Value(true));
		proplist->SetProperty(P_Storage, C4Value(nested));
		EXPECT_EQ(C4Value(proplist).ToJSON(), R"#({"Min":13,"Options":"options","Storage":{"Description":true}})#");
	}

	{
		auto crazy_key = C4PropList::NewStatic(nullptr, nullptr, nullptr);
		auto key = Strings.RegString("foo\"bar");
		crazy_key->SetPropertyByS(key, C4Value(42));
		EXPECT_EQ(C4Value(crazy_key).ToJSON(), "{\"foo\\\"bar\":42}");
	}

	// arrays
	{
		auto array = new C4ValueArray(3);
		array->SetItem(0, C4Value(1));
		array->SetItem(1, C4Value(2));
		array->SetItem(2, C4Value(3));
		EXPECT_EQ(C4Value(array).ToJSON(), R"#([1,2,3])#");
	}

	{
		auto proplist = C4PropList::NewStatic(nullptr, nullptr, nullptr);
		proplist->SetProperty(P_Options, C4Value(123));
		auto array = new C4ValueArray(1);
		array->SetItem(0, C4Value(proplist));
		EXPECT_EQ(C4Value(array).ToJSON(), R"#([{"Options":123}])#");
	}
}
