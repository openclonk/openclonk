/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2011, The OpenClonk Team and contributors
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

/* Verify correct behavior of UTF-8 handling code. */

#include "C4Include.h"
#include "lib/Standard.h"
#include <gtest/gtest.h>

TEST(UnicodeHandlingTest, AcceptsEmptyString)
{
	// Check acceptance of empty strings.
	// Part 1: Automatic length detection
	EXPECT_TRUE(::IsValidUtf8(""));
	// Part 2: Automatic length detection with trailing garbage
	EXPECT_TRUE(::IsValidUtf8("\0\xFF\xFF\xFF\xFF"));
	// Part 3: Manual length override with trailing garbage
	EXPECT_TRUE(::IsValidUtf8("\xFF\xFF\xFF\xFF", 0));
}

TEST(UnicodeHandlingTest, AcceptsValidSingleByteUtf8)
{
	// Check acceptance of valid UTF-8 single-byte sequences.
	// This test is exhaustive over U+0000..U+007F.
	// Part 1: Automatic length detection
	// Test gc=Lu: General category: Letter, uppercase
	EXPECT_TRUE(::IsValidUtf8("ABCDEFGHIJKLMNOPQRSTUVWXYZ"));
	// Test gc=Ll: General category: Letter, lowercase
	EXPECT_TRUE(::IsValidUtf8("abcdefghijklmnopqrstuvwxyz"));
	// Test gc=Nd: General category: Number, decimal digit
	EXPECT_TRUE(::IsValidUtf8("0123456789"));
	// Test gc=Zs: General category: Separator, space
	EXPECT_TRUE(::IsValidUtf8(" "));
	// Test gc=Po: General category: Punctuation, other
	EXPECT_TRUE(::IsValidUtf8(
		"!"
		"\x22" // U+0022 QUOTATION MARK
		"#%&'*,./:;?@"
		"\x5C" // U+005C REVERSE SOLIDUS (aka BACKSLASH)
		));
	// Test gc=Sc: General category: Symbol, currency
	EXPECT_TRUE(::IsValidUtf8("$"));
	// Test gc=Ps: General category: Punctuation, open
	EXPECT_TRUE(::IsValidUtf8("([{"));
	// Test gc=Pe: General category: Punctuation, close
	EXPECT_TRUE(::IsValidUtf8(")]}"));
	// Test gc=Sm: General category: Symbol, math
	EXPECT_TRUE(::IsValidUtf8("+<=>|~"));
	// Test gc=Pd: General category: Punctuation, dash
	EXPECT_TRUE(::IsValidUtf8("-"));
	// Test gc=Sk: General category: Symbol, modifier
	EXPECT_TRUE(::IsValidUtf8("^"));
	// Test gc=Cc: General category: Other, control
	// NB: This omits U+0000 NULL due to it being the C string terminator
	EXPECT_TRUE(::IsValidUtf8(
		    "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F"
		"\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F"
		"\x7F"));
	
	// Part 2: Interspersed U+0000 NULL characters
	EXPECT_TRUE(::IsValidUtf8("A\0BC\0DEF\0GHIJ\0KLMNO", 20));
	
	// Part 3: Valid UTF-8 with trailing garbage, manual length override
	EXPECT_TRUE(::IsValidUtf8("AAAA\x80\xF0\xFF", 4));
}

TEST(UnicodeHandlingTest, RejectsInvalidSingleByteUtf8)
{
	// Check rejection of invalid UTF-8 single-byte sequences
	// Part 1: Range 0x80..0xBF (orphaned continuation bytes)
	for (int i = 0x80; i <= 0xBF; ++i)
	{
		char buffer[] = { static_cast<char>(i), 0 };
		EXPECT_FALSE(::IsValidUtf8(buffer));
	}
	// Part 2: Range 0xC0..0xF4 (orphaned start bytes)
	for (int i = 0xC0; i <= 0xFF; ++i)
	{
		char buffer[] = { static_cast<char>(i), 0 };
		EXPECT_FALSE(::IsValidUtf8(buffer));
	}
	// Part 3: Range 0xF5..0xFF (invalid bytes)
	for (int i = 0xF5; i <= 0xFF; ++i)
	{
		char buffer[] = { static_cast<char>(i), 0 };
		EXPECT_FALSE(::IsValidUtf8(buffer));
	}
}

TEST(UnicodeHandlingTest, AcceptsValidMultiByteUtf8)
{
	// Check acceptance of valid UTF-8 multi-byte sequences.
	// Part 1: Generate all valid two-byte sequences
	for (int i = 0x80; i < 0x800; ++i)
	{
		char buffer[] =
		{
			static_cast<char>(0xC0 | (i >> 6)),
			static_cast<char>(0x80 | (i & 0x3F)),
			0
		};
		EXPECT_TRUE(::IsValidUtf8(buffer)) << "Valid UTF-8 character not recognized:" << std::hex
			<< " 0x" << (uint32_t)(uint8_t)buffer[0] << " 0x" << (uint32_t)(uint8_t)buffer[1]
			<< " (0x" << i << ")";
	}
	// Part 2: Generate all valid three-byte sequences
	for (int i = 0x800; i < 0x10000; ++i)
	{
		if (i == 0xD800) i = 0xE000; // Skip invalid surrogate halves
		char buffer[] =
		{
			static_cast<char>(0xE0 | (i >> 12)),
			static_cast<char>(0x80 | ((i >> 6) & 0x3F)),
			static_cast<char>(0x80 | (i & 0x3F)),
			0
		};
		EXPECT_TRUE(::IsValidUtf8(buffer)) << "Valid UTF-8 character not recognized:" << std::hex
			<< " 0x" << (uint32_t)(uint8_t)buffer[0] << " 0x" << (uint32_t)(uint8_t)buffer[1]
			<< " 0x" << (uint32_t)(uint8_t)buffer[2]
			<< " (0x" << i << ")";
	}
	// Part 3: Generate all valid four-byte sequences
	for (int i = 0x10000; i < 0x10FFFF; ++i)
	{
		char buffer[] =
		{
			static_cast<char>(0xF0 | (i >> 18)),
			static_cast<char>(0x80 | ((i >> 12) & 0x3F)),
			static_cast<char>(0x80 | ((i >> 6) & 0x3F)),
			static_cast<char>(0x80 | (i & 0x3F)),
			0
		};
		EXPECT_TRUE(::IsValidUtf8(buffer)) << "Valid UTF-8 character not recognized:" << std::hex
			<< " 0x" << (uint32_t)(uint8_t)buffer[0] << " 0x" << (uint32_t)(uint8_t)buffer[1]
			<< " 0x" << (uint32_t)(uint8_t)buffer[2] << " 0x" << (uint32_t)(uint8_t)buffer[3]
			<< " (0x" << i << ")";
	}
}

TEST(UnicodeHandlingTest, RejectsInvalidMultiByteUtf8)
{
	// Check rejection of invalid UTF-8 multi-byte sequences.
	// Part 1: Overlong sequences
	//  1.1: U+0000 NULL encoding
	EXPECT_FALSE(::IsValidUtf8("\xC0\x80")); // Two-byte representation of U+0000 NULL
	EXPECT_FALSE(::IsValidUtf8("\xE0\x80\x80")); // Three-byte representation of U+0000 NULL
	EXPECT_FALSE(::IsValidUtf8("\xF0\x80\x80\x80")); // Four-byte representation of U+0000 NULL
	EXPECT_FALSE(::IsValidUtf8("\xF8\x80\x80\x80\x80")); // Five-byte representation of U+0000 NULL
	EXPECT_FALSE(::IsValidUtf8("\xFC\x80\x80\x80\x80\x80")); // Six-byte representation of U+0000 NULL
	//  1.2: U+0080 <control> encoding
	EXPECT_FALSE(::IsValidUtf8("\xE0\x82\x80")); // Three-byte representation of U+0080 <control>
	EXPECT_FALSE(::IsValidUtf8("\xF0\x80\x82\x80")); // Four-byte representation of U+0080 <control>
	//  1.3: U+0800 SAMARITAN LETTER ALAF encoding
	EXPECT_FALSE(::IsValidUtf8("\xF0\x80\xA0\x80")); // four-byte representation of U+0800 SAMARITAN LETTER ALAF
	// Part 2: Incorrectly encoded surrogate halves
	for (int i = 0xD800; i <= 0xDFFF; ++i)
	{
		char buffer[] =
		{
			static_cast<char>(0xE0 | (i >> 12)),
			static_cast<char>(0x80 | ((i >> 6) & 0x3F)),
			static_cast<char>(0x80 | (i & 0x3F)),
			0
		};
		EXPECT_FALSE(::IsValidUtf8(buffer)) << "Invalid surrogate half not recognized: " << std::hex
			<< " 0x" << (uint32_t)(uint8_t)buffer[0] << " 0x" << (uint32_t)(uint8_t)buffer[1]
			<< " 0x" << (uint32_t)(uint8_t)buffer[2]
			<< " (0x" << i << ")";
	}
	// Part 3: Sequences encoding codepoints beyond the unicode range
	EXPECT_FALSE(::IsValidUtf8("\xF4\x90\x80\x80")); // Representation of invalid codepoint U+110000
	// Part 4: Incomplete multibyte sequences
	EXPECT_FALSE(::IsValidUtf8("\xC3\xA6", 1)); // U+00E6 LATIN SMALL LETTER AE
	EXPECT_FALSE(::IsValidUtf8("\xE2\x84\x95", 2)); // U+2115 DOUBLE-STRUCK CAPITAL N
	EXPECT_FALSE(::IsValidUtf8("\xE2\x84"));
	EXPECT_FALSE(::IsValidUtf8("\xF0\x9F\x94\x87", 3)); // U+1F507 SPEAKER WITH CANCELLATION STROKE
}

#include "lib/StdBuf.h"

#ifdef _WIN32
TEST(UnicodeHandlingTest, WideStringConversion)
{
	const wchar_t *wide_strings[] = {
		L"\xD835\xDD07\xD835\xDD22\xD835\xDD2F",
		L"\xD835\xDD0E\xD835\xDD29\xD835\xDD1E\xD835\xDD32\xD835\xDD30",
	};
	for (const auto wide_string : wide_strings)
	{
		StdStrBuf wide_string_buf(wide_string);
		EXPECT_STREQ(wide_string, wide_string_buf.GetWideChar()) << "Conversion wchar_t*=>StdStrBuf=>wchar_t* isn't lossless";
	}
}
#endif

#ifdef _WIN32
#include "platform/StdRegistry.h"
TEST(UnicodeHandlingTest, RegistryAccess)
{
	const wchar_t *wide_strings[] = {
		L"\xD835\xDD07\xD835\xDD22\xD835\xDD2F",
		L"\xD835\xDD0E\xD835\xDD29\xD835\xDD1E\xD835\xDD32\xD835\xDD30",
	};

	const char *key = "SOFTWARE\\OpenClonk Project\\OpenClonk\\Testing";
	for (const auto wide_string : wide_strings)
	{
		ASSERT_TRUE(SetRegistryString(key, "WideCharTest", StdStrBuf(wide_string).getData()));
		StdCopyStrBuf buffer;
		ASSERT_TRUE(!(buffer = GetRegistryString(key, "WideCharTest")).isNull());
		EXPECT_STREQ(wide_string, StdStrBuf(buffer).GetWideChar()) << "Registry read-back returned wrong value";
	}
}
#endif
