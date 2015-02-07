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

static __test_last_result;

global func TEST(section)
{
	if (!__test_last_result)
		__test_last_result = {
			section: nil,
			ok: 0,
			failed: 0
		};
	if (__test_last_result.section)
	{
		Log("** TEST: %s: [%d/%d ok]", __test_last_result.section, __test_last_result.ok, __test_last_result.ok + __test_last_result.failed);
	}
	__test_last_result.section = section;
	__test_last_result.ok = __test_last_result.failed = 0;
	if (section)
		Log("** TEST: %s", section);
}

global func END_TEST()
{
	TEST(nil);
}

global func EXPECT_EQ(expected, actual, failure_msg)
{
	if (expected != actual)
	{
		++__test_last_result.failed;
		var readable_expected = Format("%v", expected);
		var readable_actual = Format("%v", actual);
		if (GetType(expected) == C4V_C4Object)
			readable_expected = Format("%v /* %s */", expected, expected->GetName());
		if (GetType(actual) == C4V_C4Object)
			readable_actual = Format("%v /* %s */", actual, actual->GetName());
		if (failure_msg)
			Log("*** EXPECTATION %d FAILED: %s (%s != %s)", __test_last_result.failed + __test_last_result.ok, failure_msg, readable_expected, readable_actual);
		else
			Log("*** EXPECTATION %d FAILED: (%s != %s)", __test_last_result.failed + __test_last_result.ok, readable_expected, readable_actual);
	} else {
		++__test_last_result.ok;
	}
}
