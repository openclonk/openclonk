/*
 * OpenClonk, http://www.openclonk.org
 *
 * This file is ineligible for copyright and therefore in the public domain,
 * because it does not reach the required threshold of originality.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license text
 * for the above reference.
 */

/* Runs all available tests. */

#include <gtest/gtest.h>

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
