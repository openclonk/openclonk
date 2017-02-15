/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2015-2017, The OpenClonk Team and contributors
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


#include "C4Include.h"
#include "AulTest.h"
#include "ErrorHandler.h"

class AulDiagnosticsTest : public AulTest {};

TEST_F(AulDiagnosticsTest, Warnings)
{
	{
		ErrorHandler errh;
		EXPECT_CALL(errh, OnWarning(::testing::_)).Times(3);
		EXPECT_EQ(C4Value(), RunScript("func Main(string s, object o, array a) { Sin(s); }"));
		EXPECT_EQ(C4Value(), RunScript("func Main(string s, object o, array a) { Sin(o); }"));
		EXPECT_EQ(C4Value(), RunScript("func Main(string s, object o, array a) { Sin(a); }"));
	}
	{
		ErrorHandler errh;
		EXPECT_CALL(errh, OnWarning(::testing::StartsWith("empty controlled statement")));
		RunCode("if (true);");
	}
	{
		ErrorHandler errh;
		EXPECT_CALL(errh, OnWarning(::testing::StartsWith("empty controlled statement")));
		RunCode("if (true) { return; } else;");
	}
	{
		ErrorHandler errh;
		EXPECT_CALL(errh, OnWarning(::testing::StartsWith("empty controlled statement"))).Times(0);
		RunCode("if (true) {} else {}");
	}
	{
		ErrorHandler errh;
		EXPECT_CALL(errh, OnWarning(::testing::EndsWith("[variable_shadows_variable]")));
		RunScript("func Main(f) { var f; }");
	}
}

TEST_F(AulDiagnosticsTest, NoWarnings)
{
	ErrorHandler errh;
	EXPECT_CALL(errh, OnWarning(::testing::_)).Times(0);
	EXPECT_EQ(C4Value(), RunScript("func Main(string s, object o, array a) { var x; Sin(x); }"));
}

TEST_F(AulDiagnosticsTest, DiagnosticsSelection)
{
	{
		ErrorHandler errh;
		EXPECT_CALL(errh, OnWarning(::testing::EndsWith("[arg_type_mismatch]"))).Times(2);
		// Test disabling and re-enabling warnings
		RunScript(R"(
func Main(string s) {
	Sin(s);
#warning disable arg_type_mismatch
	Sin(s);
#warning enable arg_type_mismatch
	Sin(s);
}
)");
	}
	{
		ErrorHandler errh;
		EXPECT_CALL(errh, OnWarning(::testing::EndsWith("[arg_type_mismatch]")));
		// Test that disabling a warning doesn't affect any other warnings
		RunScript(R"(
func Main(string s) {
#warning disable redeclaration
	Sin(s);
}
)");
	}
	{
		ErrorHandler errh;
		EXPECT_CALL(errh, OnWarning(::testing::EndsWith("[arg_count_mismatch]"))).Times(0);
		EXPECT_CALL(errh, OnWarning(::testing::EndsWith("[arg_type_mismatch]"))).Times(0);
		// Test disabling multiple warnings at once
		RunScript(R"(
func Main(string s) {
#warning disable arg_count_mismatch arg_type_mismatch
	Sin(s, s);
}
)");
	}
	{
		ErrorHandler errh;
		EXPECT_CALL(errh, OnWarning(::testing::EndsWith("[arg_type_mismatch]"))).Times(0);
		// Test disabling all warnings at once
		RunScript(R"(
func Main(string s) {
#warning disable
	Sin(s);
}
)");
	}
	{
		ErrorHandler errh;
		EXPECT_CALL(errh, OnWarning(::testing::EndsWith("[type_name_used_as_par_name]"))).Times(2);
		// Test that disabled-by-default warnings have to be enabled explicitly
		RunScript(R"(
func Main(array) {}
#warning enable type_name_used_as_par_name
func f(array, string) {}
)");
	}
}
