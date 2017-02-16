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

class AulDiagnosticsTest : public AulTest
{
protected:
	std::unique_ptr<ErrorHandler> _e;
};

// Some macro helpers to ensure we can call EXPECT_WARNING multiple times
// in the same scope
#define EXPECT_WARNING_DETAIL2(c) struct _e_ScopeGuard_ ## c { \
	std::unique_ptr<ErrorHandler> &_e; \
	_e_ScopeGuard_ ## c(std::unique_ptr<ErrorHandler> &_e) : _e(_e) {} \
	~_e_ScopeGuard_ ## c() { _e.reset(); }\
} _e_ScopeGuard_ ## c{_e};
#define EXPECT_WARNING_DETAIL(c) EXPECT_WARNING_DETAIL2(c)
#define EXPECT_WARNING(id) \
	EXPECT_WARNING_DETAIL(__COUNTER__); /* setup scope guard to clear _e at the end of scope */ \
	if (!_e) _e = std::make_unique<ErrorHandler>(); /* register new error handler if none is set */ \
	(void)C4AulWarningId::id; /* make sure the warning ID exists */ \
	EXPECT_CALL(*_e, OnWarning(::testing::EndsWith("[" #id "]")))

// Tests begin here
TEST_F(AulDiagnosticsTest, arg_type_mismatch)
{
	{
		EXPECT_WARNING(arg_type_mismatch).Times(3);
		RunScript("func Main(string s, object o, array a) { Sin(s); }");
		RunScript("func Main(string s, object o, array a) { Sin(o); }");
		RunScript("func Main(string s, object o, array a) { Sin(a); }");
	}
	{
		EXPECT_WARNING(arg_type_mismatch).Times(0);
		RunScript("func Main(string s, object o, array a) { var x; Sin(x); }");
	}
}

TEST_F(AulDiagnosticsTest, empty_if)
{
	{
		EXPECT_WARNING(empty_if);
		RunCode("if (true);");
	}
	{
		EXPECT_WARNING(empty_if);
		RunCode("if (true) { return; } else;");
	}
	{
		EXPECT_WARNING(empty_if).Times(0);
		RunCode("if (true) {} else {}");
	}
}

TEST_F(AulDiagnosticsTest, variable_shadows_variable)
{
	{
		EXPECT_WARNING(variable_shadows_variable);
		RunScript("func Main(f) { var f; }");
	}
}

TEST_F(AulDiagnosticsTest, DiagnosticsSelection)
{
	{
		EXPECT_WARNING(arg_type_mismatch).Times(2);
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
		EXPECT_WARNING(arg_type_mismatch);
		// Test that disabling a warning doesn't affect any other warnings
		RunScript(R"(
func Main(string s) {
#warning disable redeclaration
	Sin(s);
}
)");
	}
	{
		EXPECT_WARNING(arg_count_mismatch).Times(0);
		EXPECT_WARNING(arg_type_mismatch).Times(0);
		// Test disabling multiple warnings at once
		RunScript(R"(
func Main(string s) {
#warning disable arg_count_mismatch arg_type_mismatch
	Sin(s, s);
}
)");
	}
	{
		EXPECT_WARNING(arg_type_mismatch).Times(0);
		// Test disabling all warnings at once
		RunScript(R"(
func Main(string s) {
#warning disable
	Sin(s);
}
)");
	}
	{
		EXPECT_WARNING(type_name_used_as_par_name).Times(2);
		// Test that disabled-by-default warnings have to be enabled explicitly
		RunScript(R"(
func Main(array) {}
#warning enable type_name_used_as_par_name
func f(array, string) {}
)");
	}
}
