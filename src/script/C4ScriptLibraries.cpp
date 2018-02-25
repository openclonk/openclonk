/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2018, The OpenClonk Team and contributors
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
#include "script/C4ScriptLibraries.h"

#ifdef _MSC_VER
#	pragma warning(push)
#	pragma warning(disable: 4804)	// 'operation' : unsafe use of type 'bool' in operation
#endif
#include "blake2.h"
#ifdef _MSC_VER
#	pragma warning(pop)
#endif

#include "script/C4Aul.h"
#include "script/C4AulDefFunc.h"

C4ScriptLibrary::C4ScriptLibrary(const char *name) : C4PropListStaticMember(nullptr, nullptr, ::Strings.RegString(name)) {}

void C4ScriptLibrary::RegisterWithEngine(C4AulScriptEngine *engine)
{
	engine->RegisterGlobalConstant(ParentKeyName->GetCStr(), C4VPropList(this));
	CreateFunctions();
	Freeze();
}

// Define USE_Z85 to 0 if you want to use Adobe's Ascii-85 encoding, which has
// easier to understand encode/decode tables but uses characters that require
// escaping in string constants, or to 1 to use ZeroMQ's variant which has a
// more complicated decode table but avoids those characters
#define USE_Z85 1
namespace {
	constexpr const char encoder_table[] = 
#if USE_Z85
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.-:+=^!/*?&<>()[]{}@%$#"
#else
		"!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstu"
#endif
		;
	static_assert(sizeof(encoder_table) == 85 + 1, "encoder table has more than 85 entries");

	constexpr const char decoder_table[] = {
		// Omits codes 0..31 because they're nonprintable and not part of the Base85 alphabets
#if USE_Z85
		'\xff', '\x44', '\xff', '\x54', '\x53', '\x52', '\x48', '\xff',
		'\x4b', '\x4c', '\x46', '\x41', '\xff', '\x3f', '\x3e', '\x45',
		'\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07',
		'\x08', '\x09', '\x40', '\xff', '\x49', '\x42', '\x4a', '\x47',
		'\x51', '\x24', '\x25', '\x26', '\x27', '\x28', '\x29', '\x2a',
		'\x2b', '\x2c', '\x2d', '\x2e', '\x2f', '\x30', '\x31', '\x32',
		'\x33', '\x34', '\x35', '\x36', '\x37', '\x38', '\x39', '\x3a',
		'\x3b', '\x3c', '\x3d', '\x4d', '\xff', '\x4e', '\x43', '\xff',
		'\xff', '\x0a', '\x0b', '\x0c', '\x0d', '\x0e', '\x0f', '\x10',
		'\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17', '\x18',
		'\x19', '\x1a', '\x1b', '\x1c', '\x1d', '\x1e', '\x1f', '\x20',
		'\x21', '\x22', '\x23', '\x4f', '\xff', '\x50', '\xff', '\xff'
#else
		'\xff', '\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06',
		'\x07', '\x08', '\x09', '\x0a', '\x0b', '\x0c', '\x0d', '\x0e',
		'\x0f', '\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16',
		'\x17', '\x18', '\x19', '\x1a', '\x1b', '\x1c', '\x1d', '\x1e',
		'\x1f', '\x20', '\x21', '\x22', '\x23', '\x24', '\x25', '\x26',
		'\x27', '\x28', '\x29', '\x2a', '\x2b', '\x2c', '\x2d', '\x2e',
		'\x2f', '\x30', '\x31', '\x32', '\x33', '\x34', '\x35', '\x36',
		'\x37', '\x38', '\x39', '\x3a', '\x3b', '\x3c', '\x3d', '\x3e',
		'\x3f', '\x40', '\x41', '\x42', '\x43', '\x44', '\x45', '\x46',
		'\x47', '\x48', '\x49', '\x4a', '\x4b', '\x4c', '\x4d', '\x4e',
		'\x4f', '\x50', '\x51', '\x52', '\x53', '\x54', '\xff', '\xff',
		'\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff',
#endif
	};

	std::string b85_encode(const unsigned char *input, size_t length)
	{
		// Input strings need to be padded to be a multiple of 4 bytes in
		// length
		size_t padding = (4 - length % 4) % 4;
		
		// The encoding process adds an overhead of 20%
		size_t output_length = (length + padding) * 5 / 4;
		std::string encoded(output_length, '\0');
		
		int encode_cursor = 0;
		for (size_t i = 0; i < length; i += 4)
		{
			uint32_t value = 0;
			for (int j = 0; j < 4; ++j)
			{
				// Merge one byte
				value *= 256;
				if (i + j < length)
					value += input[i + j];
			}
			// Write encoded data
			for (int j = 4; j >= 0; --j)
			{
				encoded[encode_cursor + j] = encoder_table[value % 85];
				value /= 85;
			}
			encode_cursor += 5;
		}
		// Remove the padding
		encoded.resize(output_length - padding);
		return encoded;
	}
	
	std::string b85_decode(const std::string &input)
	{
		// Input strings need to be padded to a multiple of 5 bytes
		size_t length = input.size();
		size_t padding = (5 - length % 5) % 5;

		size_t output_length = (length + padding) * 4 / 5;
		std::string decoded(output_length, '\0');
		
		int decode_cursor = 0;
		for (size_t i = 0; i < length; i += 5)
		{
			uint32_t value = 0;
			for (int j = 0; j < 5; ++j)
			{
				// Merge one byte
				value *= 85;
				if (i + j < length)
				{
					unsigned char byte = input[i + j];
					if (byte >= 32 && byte <= 127)
						value += decoder_table[byte - 32];
				}
				else
				{
					value += 84;
				}
			}
			// Write decoded data
			for (int j = 3; j >= 0; --j)
			{
				decoded[decode_cursor + j] = static_cast<char>(value & 0xFF);
				value /= 256;
			}
			decode_cursor += 4;
		}
		decoded.resize(output_length - padding);
		return decoded;
	}
}

class C4ScriptLibraryCrypto : public C4ScriptLibrary
{
	static constexpr const char *LIBRARY_NAME = "_Crypto";
	static constexpr const char *FUNC_COMPUTE_HASH = "_ComputeHash";

	// Calculates a hash value over a given input string.
	static C4Value Hash_Dispatch(C4PropList *_this, C4String *input, int32_t output_length)
	{
		constexpr auto MAX_EXPANDED_OUTPUT = BLAKE2B_OUTBYTES * 5 / 4;
		if (output_length < 1 || output_length > MAX_EXPANDED_OUTPUT)
		{
			throw C4AulExecError(strprintf("outputLength must be between 1 and %d", MAX_EXPANDED_OUTPUT).c_str());
		}
		if (!input)
		{
			throw C4AulExecError(strprintf(R"(call to "%s" parameter %d: passed %s, but expected %s)", FUNC_COMPUTE_HASH, 1, GetC4VName(C4V_Nil), GetC4VName(C4V_String)).c_str());
		}

		// Reduce target hash length to account for Base85 encoding
		int32_t raw_output_length = output_length * 4 / 5 + (output_length % 5 != 0);

		const unsigned char *data = (const unsigned char*) input->GetCStr();
		const int data_length = input->GetData().getLength();

		auto hash_output = std::make_unique<unsigned char[]>(raw_output_length);
		if (blake2b(hash_output.get(), raw_output_length, data, data_length, nullptr, 0) != 0)
		{
			throw C4AulExecError("internal error: blake2b call failed");
		}

		std::string encoded = b85_encode(hash_output.get(), raw_output_length);
		// Need to null-terminate here because StdStrBuf doesn't, so then 
		// C4String's GetCStr() doesn't actually return a C string.
		encoded.resize(output_length);
		return C4VString(StdStrBuf(encoded.c_str(), output_length, false));
	}

protected:
	void CreateFunctions() override
	{
		AddFunc(this, FUNC_COMPUTE_HASH, &Hash_Dispatch);
	}

public:
	C4ScriptLibraryCrypto() : C4ScriptLibrary(LIBRARY_NAME) {}
};

void C4ScriptLibrary::InstantiateAllLibraries(C4AulScriptEngine *engine)
{
	C4ScriptLibrary *libraries[] = {
		new C4ScriptLibraryCrypto
	};

	for (auto library : libraries)
	{
		library->RegisterWithEngine(engine);
	}
}
