/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2016, The OpenClonk Team and contributors
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
#include "C4ForbidLibraryCompilation.h"
#include "lib/StdMeshMaterial.h"

#include "lib/StdMeshUpdate.h"
#include "graphics/C4DrawGL.h"

#ifdef WITH_GLIB
#include <glib.h>
#endif

namespace
{
	// String <-> Enum assocation
	template<typename EnumType>
	struct Enumerator
	{
		const char* Name;
		EnumType Value;
	};

	// Define a name for a sequence of enums
	template<int Num, typename EnumType>
	struct EnumeratorShortcut
	{
		const char* Name;
		EnumType Values[Num];
	};

	const Enumerator<StdMeshMaterialShaderParameter::Auto> ShaderParameterAutoEnumerators[] =
	{
		{ nullptr, static_cast<StdMeshMaterialShaderParameter::Auto>(0) }
	};

	const Enumerator<StdMeshMaterialTextureUnit::TexAddressModeType> TexAddressModeEnumerators[] =
	{
		{ "wrap", StdMeshMaterialTextureUnit::AM_Wrap },
		{ "clamp", StdMeshMaterialTextureUnit::AM_Clamp },
		{ "mirror", StdMeshMaterialTextureUnit::AM_Mirror },
		{ "border", StdMeshMaterialTextureUnit::AM_Border },
		{ nullptr, static_cast<StdMeshMaterialTextureUnit::TexAddressModeType>(0) }
	};

	const Enumerator<StdMeshMaterialTextureUnit::FilteringType> FilteringEnumerators[] =
	{
		{ "none", StdMeshMaterialTextureUnit::F_None },
		{ "point", StdMeshMaterialTextureUnit::F_Point },
		{ "linear", StdMeshMaterialTextureUnit::F_Linear },
		{ "anisotropic", StdMeshMaterialTextureUnit::F_Anisotropic },
		{ nullptr, static_cast<StdMeshMaterialTextureUnit::FilteringType>(0) }
	};

	const EnumeratorShortcut<3, StdMeshMaterialTextureUnit::FilteringType> FilteringShortcuts[] =
	{
		{ "none", { StdMeshMaterialTextureUnit::F_Point, StdMeshMaterialTextureUnit::F_Point, StdMeshMaterialTextureUnit::F_None } },
		{ "bilinear", { StdMeshMaterialTextureUnit::F_Linear, StdMeshMaterialTextureUnit::F_Linear, StdMeshMaterialTextureUnit::F_Point } },
		{ "trilinear", { StdMeshMaterialTextureUnit::F_Linear, StdMeshMaterialTextureUnit::F_Linear, StdMeshMaterialTextureUnit::F_Linear } },
		{ "anisotropic", { StdMeshMaterialTextureUnit::F_Anisotropic, StdMeshMaterialTextureUnit::F_Anisotropic, StdMeshMaterialTextureUnit::F_Linear } },
		{ nullptr, { static_cast<StdMeshMaterialTextureUnit::FilteringType>(0), static_cast<StdMeshMaterialTextureUnit::FilteringType>(0), static_cast<StdMeshMaterialTextureUnit::FilteringType>(0) } }
	};

	const Enumerator<StdMeshMaterialTextureUnit::BlendOpType> BlendOpEnumerators[] =
	{
		{ "replace", StdMeshMaterialTextureUnit::BO_Replace },
		{ "add", StdMeshMaterialTextureUnit::BO_Add },
		{ "modulate", StdMeshMaterialTextureUnit::BO_Modulate },
		{ "alpha_blend", StdMeshMaterialTextureUnit::BO_AlphaBlend },
		{ nullptr, static_cast<StdMeshMaterialTextureUnit::BlendOpType>(0) }
	};

	const Enumerator<StdMeshMaterialTextureUnit::BlendOpExType> BlendOpExEnumerators[] =
	{
		{ "source1", StdMeshMaterialTextureUnit::BOX_Source1 },
		{ "source2", StdMeshMaterialTextureUnit::BOX_Source2 },
		{ "modulate", StdMeshMaterialTextureUnit::BOX_Modulate },
		{ "modulate_x2", StdMeshMaterialTextureUnit::BOX_ModulateX2 },
		{ "modulate_x4", StdMeshMaterialTextureUnit::BOX_ModulateX4 },
		{ "add", StdMeshMaterialTextureUnit::BOX_Add },
		{ "add_signed", StdMeshMaterialTextureUnit::BOX_AddSigned },
		{ "add_smooth", StdMeshMaterialTextureUnit::BOX_AddSmooth },
		{ "subtract", StdMeshMaterialTextureUnit::BOX_Subtract },
		{ "blend_diffuse_alpha", StdMeshMaterialTextureUnit::BOX_BlendDiffuseAlpha },
		{ "blend_texture_alpha", StdMeshMaterialTextureUnit::BOX_BlendTextureAlpha },
		{ "blend_current_alpha", StdMeshMaterialTextureUnit::BOX_BlendCurrentAlpha },
		{ "blend_manual", StdMeshMaterialTextureUnit::BOX_BlendManual },
		{ "dotproduct", StdMeshMaterialTextureUnit::BOX_Dotproduct },
		{ "blend_diffuse_colour", StdMeshMaterialTextureUnit::BOX_BlendDiffuseColor },
		{ nullptr, static_cast<StdMeshMaterialTextureUnit::BlendOpExType>(0) }
	};

	const Enumerator<StdMeshMaterialTextureUnit::BlendOpSourceType> BlendOpSourceEnumerators[] =
	{
		{ "src_current", StdMeshMaterialTextureUnit::BOS_Current },
		{ "src_texture", StdMeshMaterialTextureUnit::BOS_Texture },
		{ "src_diffuse", StdMeshMaterialTextureUnit::BOS_Diffuse },
		{ "src_specular", StdMeshMaterialTextureUnit::BOS_Specular },
		{ "src_player_color", StdMeshMaterialTextureUnit::BOS_PlayerColor },
		{ "src_player_colour", StdMeshMaterialTextureUnit::BOS_PlayerColor },
		{ "src_manual", StdMeshMaterialTextureUnit::BOS_Manual },
		{ nullptr, static_cast<StdMeshMaterialTextureUnit::BlendOpSourceType>(0) }
	};

	const Enumerator<StdMeshMaterialTextureUnit::Transformation::XFormType> XFormTypeEnumerators[] =
	{
		{ "scroll_x", StdMeshMaterialTextureUnit::Transformation::XF_SCROLL_X },
		{ "scroll_y", StdMeshMaterialTextureUnit::Transformation::XF_SCROLL_Y },
		{ "rotate", StdMeshMaterialTextureUnit::Transformation::XF_ROTATE },
		{ "scale_x", StdMeshMaterialTextureUnit::Transformation::XF_SCALE_X },
		{ "scale_y", StdMeshMaterialTextureUnit::Transformation::XF_SCALE_Y },
		{ nullptr, static_cast<StdMeshMaterialTextureUnit::Transformation::XFormType>(0) }
	};

	const Enumerator<StdMeshMaterialTextureUnit::Transformation::WaveType> WaveTypeEnumerators[] =
	{
		{ "sine", StdMeshMaterialTextureUnit::Transformation::W_SINE },
		{ "triangle", StdMeshMaterialTextureUnit::Transformation::W_TRIANGLE },
		{ "square", StdMeshMaterialTextureUnit::Transformation::W_SQUARE },
		{ "sawtooth", StdMeshMaterialTextureUnit::Transformation::W_SAWTOOTH },
		{ "inverse_sawtooth", StdMeshMaterialTextureUnit::Transformation::W_INVERSE_SAWTOOTH },
		{ nullptr, static_cast<StdMeshMaterialTextureUnit::Transformation::WaveType>(0) }
	};

	const Enumerator<StdMeshMaterialPass::CullHardwareType> CullHardwareEnumerators[] =
	{
		{ "clockwise", StdMeshMaterialPass::CH_Clockwise },
		{ "anticlockwise", StdMeshMaterialPass::CH_CounterClockwise },
		{ "none", StdMeshMaterialPass::CH_None },
		{ nullptr, static_cast<StdMeshMaterialPass::CullHardwareType>(0) }
	};

	const Enumerator<StdMeshMaterialPass::SceneBlendType> SceneBlendEnumerators[] =
	{
		{ "one", StdMeshMaterialPass::SB_One },
		{ "zero", StdMeshMaterialPass::SB_Zero },
		{ "dest_colour", StdMeshMaterialPass::SB_DestColor },
		{ "src_colour", StdMeshMaterialPass::SB_SrcColor },
		{ "one_minus_dest_colour", StdMeshMaterialPass::SB_OneMinusDestColor },
		{ "one_minus_src_colour", StdMeshMaterialPass::SB_OneMinusSrcColor },
		{ "dest_alpha", StdMeshMaterialPass::SB_DestAlpha },
		{ "src_alpha", StdMeshMaterialPass::SB_SrcAlpha },
		{ "one_minus_dest_alpha", StdMeshMaterialPass::SB_OneMinusDestAlpha },
		{ "one_minus_src_alpha", StdMeshMaterialPass::SB_OneMinusSrcAlpha },
		{ nullptr, static_cast<StdMeshMaterialPass::SceneBlendType>(0) }
	};

	const EnumeratorShortcut<2, StdMeshMaterialPass::SceneBlendType> SceneBlendShortcuts[] =
	{
		{ "add", { StdMeshMaterialPass::SB_One, StdMeshMaterialPass::SB_One } },
		{ "modulate", { StdMeshMaterialPass::SB_DestColor, StdMeshMaterialPass::SB_Zero } },
		{ "colour_blend", { StdMeshMaterialPass::SB_SrcColor, StdMeshMaterialPass::SB_OneMinusSrcColor } },
		{ "alpha_blend", { StdMeshMaterialPass::SB_SrcAlpha, StdMeshMaterialPass::SB_OneMinusSrcAlpha } },
		{ nullptr, { static_cast<StdMeshMaterialPass::SceneBlendType>(0), static_cast<StdMeshMaterialPass::SceneBlendType>(0) } }
	};

	const Enumerator<StdMeshMaterialPass::DepthFunctionType> DepthFunctionEnumerators[] =
	{
		{ "always_fail", StdMeshMaterialPass::DF_AlwaysFail },
		{ "always_pass", StdMeshMaterialPass::DF_AlwaysPass },
		{ "less", StdMeshMaterialPass::DF_Less },
		{ "less_equal", StdMeshMaterialPass::DF_LessEqual },
		{ "equal", StdMeshMaterialPass::DF_Equal },
		{ "not_equal", StdMeshMaterialPass::DF_NotEqual },
		{ "greater_equal", StdMeshMaterialPass::DF_GreaterEqual },
		{ "greater", StdMeshMaterialPass::DF_Greater },
		{ nullptr, static_cast<StdMeshMaterialPass::DepthFunctionType>(0) }
	};
}

StdMeshMaterialError::StdMeshMaterialError(const StdStrBuf& message, const char* file, unsigned int line)
{
	Buf.Format("%s:%u: %s", file, line, message.getData());
}

enum Token
{
	TOKEN_IDTF,
	TOKEN_BRACE_OPEN,
	TOKEN_BRACE_CLOSE,
	TOKEN_COLON,
	TOKEN_EOF
};

class StdMeshMaterialParserCtx
{
public:
	StdMeshMaterialParserCtx(StdMeshMatManager& manager, const char* mat_script, const char* filename, StdMeshMaterialLoader& loader);

	void SkipWhitespace();
	Token Peek(StdStrBuf& name);
	Token Advance(StdStrBuf& name);
	Token AdvanceNonEOF(StdStrBuf& name);
	Token AdvanceRequired(StdStrBuf& name, Token expect);
	Token AdvanceRequired(StdStrBuf& name, Token expect1, Token expect2);
	int AdvanceInt();
	bool AdvanceIntOptional(int& value);
	float AdvanceFloat();
	bool AdvanceFloatOptional(float& value);
	void AdvanceColor(bool with_alpha, float Color[4]);
	bool AdvanceBoolean();
	template<typename EnumType> EnumType AdvanceEnum(const Enumerator<EnumType>* enumerators);
	template<int Num, typename EnumType> void AdvanceEnums(const Enumerator<EnumType>* enumerators, EnumType enums[Num]);
	template<int Num, typename EnumType> void AdvanceEnums(const Enumerator<EnumType>* enumerators, const EnumeratorShortcut<Num, EnumType>* shortcuts, EnumType enums[Num]);
	void Error(const StdStrBuf& message);
	void ErrorUnexpectedIdentifier(const StdStrBuf& identifier);
	void WarningNotSupported(const char* identifier);

	// Current parsing data
	unsigned int Line;
	const char* Script;

	StdMeshMatManager& Manager;
	StdCopyStrBuf FileName;
	StdMeshMaterialLoader& Loader;
};

class StdMeshMaterialSubLoader
{
public:
	StdMeshMaterialSubLoader();

	template<typename SubT> void Load(StdMeshMaterialParserCtx& ctx, std::vector<SubT>& vec);
private:
	unsigned int CurIndex{0u};
};

StdMeshMaterialParserCtx::StdMeshMaterialParserCtx(StdMeshMatManager& manager, const char* mat_script, const char* filename, StdMeshMaterialLoader& loader):
		Line(1), Script(mat_script), Manager(manager), FileName(filename), Loader(loader)
{
}

void StdMeshMaterialParserCtx::SkipWhitespace()
{
	while (isspace(*Script))
	{
		if (*Script == '\n') ++Line;
		++Script;
	}

	if (*Script == '/')
	{
		if (*(Script+1) == '/')
		{
			Script += 2;
			while (*Script != '\n' && *Script != '\0')
				++Script;
			SkipWhitespace();
		}
		else if (*Script == '*')
		{
			for (Script += 2; *Script != '\0'; ++Script)
				if (*Script == '*' && *(Script+1) == '/')
					break;

			if (*Script == '*')
			{
				Script += 2;
				SkipWhitespace();
			}
		}
	}
}

Token StdMeshMaterialParserCtx::Peek(StdStrBuf& name)
{
	SkipWhitespace();

	const char* before = Script;
	Token tok = Advance(name);
	Script = before;
	return tok;
}

Token StdMeshMaterialParserCtx::Advance(StdStrBuf& name)
{
	SkipWhitespace();

	switch (*Script)
	{
	case '\0':
		name.Clear();
		return TOKEN_EOF;
	case '{':
		++Script;
		name = "{";
		return TOKEN_BRACE_OPEN;
	case '}':
		++Script;
		name = "}";
		return TOKEN_BRACE_CLOSE;
	case ':':
		++Script;
		name = ":";
		return TOKEN_COLON;
	default:
		const char* begin = Script;
		// Advance to next whitespace
		do { ++Script; }
		while (!isspace(*Script) && *Script != '{' && *Script != '}' && *Script != ':');
		name.Copy(begin, Script - begin);
		return TOKEN_IDTF;
	}
}

Token StdMeshMaterialParserCtx::AdvanceNonEOF(StdStrBuf& name)
{
	Token token = Advance(name);
	if (token == TOKEN_EOF) Error(StdCopyStrBuf("Unexpected end of file"));
	return token;
}

Token StdMeshMaterialParserCtx::AdvanceRequired(StdStrBuf& name, Token expect)
{
	Token token = AdvanceNonEOF(name);
	// TODO: Explain what was actually expected
	if (token != expect) Error(StdCopyStrBuf("'") + name + "' unexpected");
	return token;
}

Token StdMeshMaterialParserCtx::AdvanceRequired(StdStrBuf& name, Token expect1, Token expect2)
{
	Token token = AdvanceNonEOF(name);
	// TODO: Explain what was actually expected
	if (token != expect1 && token != expect2)
		Error(StdStrBuf("'") + name + "' unexpected");
	return token;
}

int StdMeshMaterialParserCtx::AdvanceInt()
{
	StdStrBuf buf;
	AdvanceRequired(buf, TOKEN_IDTF);
	int i;
#ifdef WITH_GLIB
	char* end;
	i = g_ascii_strtoll(buf.getData(), &end, 10);
	if (*end != '\0')
#else
	if (!(std::istringstream(buf.getData()) >> i))
#endif
		Error(StdStrBuf("Integer value expected"));

	return i;
}

bool StdMeshMaterialParserCtx::AdvanceIntOptional(int& value)
{
	StdStrBuf buf;
	Token tok = Peek(buf);

	if (tok == TOKEN_IDTF && isdigit(buf[0]))
	{
		value = AdvanceInt();
		return true;
	}

	return false;
}

float StdMeshMaterialParserCtx::AdvanceFloat()
{
	StdStrBuf buf;
	AdvanceRequired(buf, TOKEN_IDTF);
	float f;
#ifdef WITH_GLIB
	char* end;
	f = g_ascii_strtod(buf.getData(), &end);
	if (*end != '\0')
#else
	if (!(std::istringstream(buf.getData()) >> f))
#endif
		Error(StdStrBuf("Floating point value expected"));
	return f;
}

bool StdMeshMaterialParserCtx::AdvanceFloatOptional(float& value)
{
	StdStrBuf buf;
	Token tok = Peek(buf);

	if (tok == TOKEN_IDTF && isdigit(buf[0]))
	{
		value = AdvanceFloat();
		return true;
	}

	return false;
}

void StdMeshMaterialParserCtx::AdvanceColor(bool with_alpha, float Color[4])
{
	Color[0] = AdvanceFloat();
	Color[1] = AdvanceFloat();
	Color[2] = AdvanceFloat();
	if (with_alpha) AdvanceFloatOptional(Color[3]);
}

bool StdMeshMaterialParserCtx::AdvanceBoolean()
{
	StdCopyStrBuf buf;
	AdvanceRequired(buf, TOKEN_IDTF);
	if (buf == "on") return true;
	if (buf == "off") return false;
	Error(StdCopyStrBuf("Expected either 'on' or 'off', but not '") + buf + "'");
	return false; // Never reached
}

template<typename EnumType>
EnumType StdMeshMaterialParserCtx::AdvanceEnum(const Enumerator<EnumType>* enumerators)
{
	StdCopyStrBuf buf;
	AdvanceRequired(buf, TOKEN_IDTF);

	for (const Enumerator<EnumType>* cur = enumerators; cur->Name; ++cur)
		if (buf == cur->Name)
			return cur->Value;

	ErrorUnexpectedIdentifier(buf);
	return EnumType(); // avoid compiler warning
}

template<int Num, typename EnumType>
void StdMeshMaterialParserCtx::AdvanceEnums(const Enumerator<EnumType>* enumerators, EnumType enums[Num])
{
	for (int i = 0; i < Num; ++i)
		enums[i] = AdvanceEnum(enumerators);
}

template<int Num, typename EnumType>
void StdMeshMaterialParserCtx::AdvanceEnums(const Enumerator<EnumType>* enumerators, const EnumeratorShortcut<Num, EnumType>* shortcuts, EnumType enums[Num])
{
	StdCopyStrBuf buf;
	AdvanceRequired(buf, TOKEN_IDTF);

	const Enumerator<EnumType>* cenum;
	const EnumeratorShortcut<Num, EnumType>* cshort;

	for (cenum = enumerators; cenum->Name; ++cenum)
		if (buf == cenum->Name)
			break;
	for (cshort = shortcuts; cshort->Name; ++cshort)
		if (buf == cshort->Name)
			break;

	if (!cenum->Name && !cshort->Name)
	{
		ErrorUnexpectedIdentifier(buf);
	}
	else if (!cenum->Name && cshort->Name)
	{
		for (int i = 0; i < Num; ++i)
			enums[i] = cshort->Values[i];
	}
	else if (cenum->Name && (!cshort->Name || Num == 1))
	{
		enums[0] = cenum->Value;
		for (int i = 1; i < Num; ++i)
			enums[i] = AdvanceEnum(enumerators);
	}
	else
	{
		// Both enumerator and shortcut are possible, determine by look-ahead
		const Enumerator<EnumType>* cenum2 = nullptr;
		Token tok = Peek(buf);
		if (tok == TOKEN_IDTF)
		{
			for (cenum2 = enumerators; cenum2->Name; ++cenum2)
				if (buf == cenum2->Name)
					break;
		}

		if (cenum2 && cenum2->Name)
		{
			// The next item is an enumerator, so load as enumerators
			enums[0] = cenum->Value;
			for (int i = 1; i < Num; ++i)
				enums[i] = AdvanceEnum(enumerators);
		}
		else
		{
			// The next item is something else, so load the shortcut
			for (int i = 0; i < Num; ++i)
				enums[i] = cshort->Values[i];
		}
	}
}

void StdMeshMaterialParserCtx::Error(const StdStrBuf& message)
{
	throw StdMeshMaterialError(message, FileName.getData(), Line);
}

void StdMeshMaterialParserCtx::ErrorUnexpectedIdentifier(const StdStrBuf& identifier)
{
	Error(StdCopyStrBuf("Unexpected identifier: '") + identifier + "'");
}

void StdMeshMaterialParserCtx::WarningNotSupported(const char* identifier)
{
	DebugLogF(R"(%s:%d: Warning: "%s" is not supported!)", FileName.getData(), Line, identifier);
}

StdMeshMaterialSubLoader::StdMeshMaterialSubLoader() = default;

template<typename SubT>
void StdMeshMaterialSubLoader::Load(StdMeshMaterialParserCtx& ctx, std::vector<SubT>& vec)
{
	std::vector<unsigned int> indices;

	StdCopyStrBuf token_name;
	Token tok = ctx.AdvanceRequired(token_name, TOKEN_IDTF, TOKEN_BRACE_OPEN);
	if(tok == TOKEN_BRACE_OPEN)
	{
		// Unnamed section, name by running index
		indices.push_back(CurIndex);
		assert(CurIndex <= vec.size());
		if(CurIndex == vec.size())
		{
			vec.push_back(SubT());
			vec.back().Name.Format("%u", CurIndex);
		}

		++CurIndex;
	}
	else
	{
		unsigned int size_before = indices.size();
		for(unsigned int i = 0; i < vec.size(); ++i)
			if(SWildcardMatchEx(vec[i].Name.getData(), token_name.getData()))
				indices.push_back(i);

		// Only add new SubSection if no wildcard was given
		if(indices.size() == size_before)
		{
			if(std::strchr(token_name.getData(), '*') == nullptr && std::strchr(token_name.getData(), '?') == nullptr)
			{
				indices.push_back(vec.size());
				vec.push_back(SubT());
				vec.back().Name = token_name;
			}
		}

		ctx.AdvanceRequired(token_name, TOKEN_BRACE_OPEN);
	}
	
	if(indices.empty())
	{
		// Section is not used, parse anyway to advance script position
		// This can happen if there is inheritance by a non-matching wildcard
		SubT().Load(ctx);
	}
	else
	{
		// Parse section multiple times in case there is more than one match.
		// Not particularly elegant but working.
		for(unsigned int i = 0; i < indices.size()-1; ++i)
		{
			unsigned int old_line = ctx.Line;
			const char* old_pos = ctx.Script;
			vec[indices[i]].Load(ctx);
			ctx.Line = old_line;
			ctx.Script = old_pos;
		}

		vec[indices.back()].Load(ctx);
	}	
}

void LoadShader(StdMeshMaterialParserCtx& ctx, StdMeshMaterialShaderType type)
{
	StdStrBuf token_name;
	StdStrBuf name, language;
	ctx.AdvanceRequired(name, TOKEN_IDTF);
	ctx.AdvanceRequired(language, TOKEN_IDTF);
	ctx.AdvanceRequired(token_name, TOKEN_BRACE_OPEN);

	Token token;
	StdCopyStrBuf source, code, syntax;
	while ((token = ctx.AdvanceNonEOF(token_name)) == TOKEN_IDTF)
	{
		if(token_name == "source")
		{
			ctx.AdvanceRequired(source, TOKEN_IDTF);
			code = ctx.Loader.LoadShaderCode(source.getData());
			if(code.getLength() == 0)
				ctx.Error(StdCopyStrBuf("Could not load shader code from '") + source + "'");
		}
		else if(token_name == "syntax")
		{
			ctx.AdvanceRequired(syntax, TOKEN_IDTF);
		}
		else
		{
			ctx.ErrorUnexpectedIdentifier(token_name);
		}
	}

	if (token != TOKEN_BRACE_CLOSE)
		ctx.Error(StdCopyStrBuf("'") + token_name.getData() + "' unexpected");

	ctx.Manager.AddShader(source.getData(), name.getData(), language.getData(), type, code.getData(), StdMeshMatManager::SMM_ForceReload);
}

StdMeshMaterialShaderParameter::StdMeshMaterialShaderParameter() = default;

StdMeshMaterialShaderParameter::StdMeshMaterialShaderParameter(Type type):
	type(type)
{
	if(type == MATRIX_4X4)
		matrix = new float[16];
}

StdMeshMaterialShaderParameter::StdMeshMaterialShaderParameter(const StdMeshMaterialShaderParameter& other)
{
	CopyDeep(other);
}

StdMeshMaterialShaderParameter::StdMeshMaterialShaderParameter(StdMeshMaterialShaderParameter &&other)
{
	Move(std::move(other));
}

StdMeshMaterialShaderParameter::~StdMeshMaterialShaderParameter()
{
	if(type == MATRIX_4X4)
		delete[] matrix;
}

StdMeshMaterialShaderParameter& StdMeshMaterialShaderParameter::operator=(const StdMeshMaterialShaderParameter& other)
{
	if(this == &other) return *this;

	if(type == MATRIX_4X4)
		delete[] matrix;

	CopyDeep(other);
	return *this;
}

StdMeshMaterialShaderParameter& StdMeshMaterialShaderParameter::operator=(StdMeshMaterialShaderParameter &&other)
{
	if(this == &other) return *this;

	if(type == MATRIX_4X4)
		delete[] matrix;

	Move(std::move(other));
	return *this;
}

void StdMeshMaterialShaderParameter::SetType(Type type)
{
	StdMeshMaterialShaderParameter other(type);
	Move(std::move(other));
}

void StdMeshMaterialShaderParameter::CopyShallow(const StdMeshMaterialShaderParameter& other)
{
	type = other.type;

	switch(type)
	{
	case AUTO:
		a = other.a;
		break;
	case AUTO_TEXTURE_MATRIX:
	case INT:
		i = other.i;
		break;
	case FLOAT4:
		f[3] = other.f[3];
	case FLOAT3:
		f[2] = other.f[2];
	case FLOAT2:
		f[1] = other.f[1];
	case FLOAT:
		f[0] = other.f[0];
		break;
	case MATRIX_4X4:
		matrix = other.matrix;
		break;
	default:
		assert(false);
		break;
	}
}

void StdMeshMaterialShaderParameter::CopyDeep(const StdMeshMaterialShaderParameter& other)
{
	CopyShallow(other);

	if(type == MATRIX_4X4)
	{
		matrix = new float[16];
		for(int i = 0; i < 16; ++i)
			matrix[i] = other.matrix[i];
	}
}

void StdMeshMaterialShaderParameter::Move(StdMeshMaterialShaderParameter &&other)
{
	CopyShallow(other);
	other.type = FLOAT;
}

StdMeshMaterialShaderParameters::StdMeshMaterialShaderParameters() = default;

StdMeshMaterialShaderParameter StdMeshMaterialShaderParameters::LoadConstParameter(StdMeshMaterialParserCtx& ctx)
{
	StdStrBuf type_name;
	ctx.AdvanceRequired(type_name, TOKEN_IDTF);
	if(type_name == "int")
	{
		StdMeshMaterialShaderParameter param(StdMeshMaterialShaderParameter::INT);
		param.GetInt() = ctx.AdvanceInt();
		return param;
	}
	else if(type_name == "float")
	{
		StdMeshMaterialShaderParameter param(StdMeshMaterialShaderParameter::FLOAT);
		param.GetFloat() = ctx.AdvanceFloat();
		return param;
	}
	else if(type_name == "float2")
	{
		StdMeshMaterialShaderParameter param(StdMeshMaterialShaderParameter::FLOAT2);
		param.GetFloatv()[0] = ctx.AdvanceFloat();
		param.GetFloatv()[1] = ctx.AdvanceFloat();
		return param;
	}
	else if(type_name == "float3")
	{
		StdMeshMaterialShaderParameter param(StdMeshMaterialShaderParameter::FLOAT2);
		param.GetFloatv()[0] = ctx.AdvanceFloat();
		param.GetFloatv()[1] = ctx.AdvanceFloat();
		param.GetFloatv()[2] = ctx.AdvanceFloat();
		return param;
	}
	else if(type_name == "float4")
	{
		StdMeshMaterialShaderParameter param(StdMeshMaterialShaderParameter::FLOAT2);
		param.GetFloatv()[0] = ctx.AdvanceFloat();
		param.GetFloatv()[1] = ctx.AdvanceFloat();
		param.GetFloatv()[2] = ctx.AdvanceFloat();
		param.GetFloatv()[3] = ctx.AdvanceFloat();
		return param;
	}
	else
	{
		ctx.Error(FormatString(R"(Invalid type: "%s")", type_name.getData()));
		return StdMeshMaterialShaderParameter();
	}
}

StdMeshMaterialShaderParameter StdMeshMaterialShaderParameters::LoadAutoParameter(StdMeshMaterialParserCtx& ctx)
{
	StdMeshMaterialShaderParameter param(StdMeshMaterialShaderParameter::AUTO);
	param.GetAuto() = ctx.AdvanceEnum(ShaderParameterAutoEnumerators);
	return param;
}

void StdMeshMaterialShaderParameters::Load(StdMeshMaterialParserCtx& ctx)
{
	StdStrBuf token_name;
	ctx.AdvanceRequired(token_name, TOKEN_BRACE_OPEN);

	Token token;
	while ((token = ctx.AdvanceNonEOF(token_name)) == TOKEN_IDTF)
	{
		if(token_name == "param_named")
		{
			StdStrBuf param_name;
			ctx.AdvanceRequired(param_name, TOKEN_IDTF);
			NamedParameters.push_back(std::make_pair(StdCopyStrBuf(param_name), LoadConstParameter(ctx)));
		}
		else if(token_name == "param_named_auto")
		{
			StdStrBuf param_name;
			ctx.AdvanceRequired(param_name, TOKEN_IDTF);
			NamedParameters.push_back(std::make_pair(StdCopyStrBuf(param_name), LoadAutoParameter(ctx)));
		}
		else
		{
			ctx.ErrorUnexpectedIdentifier(token_name);
		}
	}

	if (token != TOKEN_BRACE_CLOSE)
		ctx.Error(StdCopyStrBuf("'") + token_name.getData() + "' unexpected");
}

StdMeshMaterialShaderParameter& StdMeshMaterialShaderParameters::AddParameter(const char* name, StdMeshMaterialShaderParameter::Type type)
{
	NamedParameters.push_back(std::make_pair(StdCopyStrBuf(name), StdMeshMaterialShaderParameter(type)));
	return NamedParameters.back().second;
}

StdMeshMaterialProgram::StdMeshMaterialProgram(const char* name, const StdMeshMaterialShader* fragment_shader, const StdMeshMaterialShader* vertex_shader, const StdMeshMaterialShader* geometry_shader):
	Name(name), FragmentShader(fragment_shader), VertexShader(vertex_shader), GeometryShader(geometry_shader)
{
	assert(FragmentShader != nullptr);
	assert(VertexShader != nullptr);
	// Geometry shader is optional (and not even implemented at the moment!)
}

bool StdMeshMaterialProgram::AddParameterNames(const StdMeshMaterialShaderParameters& parameters)
{
	// TODO: This is O(n^2) -- not optimal!
	bool added = false;
	for (const auto & NamedParameter : parameters.NamedParameters)
	{
		const std::vector<StdCopyStrBuf>::const_iterator iter = std::find(ParameterNames.begin(), ParameterNames.end(), NamedParameter.first);
		if (iter == ParameterNames.end())
		{
			ParameterNames.push_back(NamedParameter.first);
			added = true;
		}
	}

	return added;
}

bool StdMeshMaterialProgram::CompileShader(StdMeshMaterialLoader& loader, C4Shader& shader, int ssc)
{
	// Add standard slices
	loader.AddShaderSlices(shader, ssc);
	// Add our slices
	shader.AddVertexSlices(VertexShader->GetFilename(), VertexShader->GetCode(), VertexShader->GetFilename());
	shader.AddFragmentSlices(FragmentShader->GetFilename(), FragmentShader->GetCode(), FragmentShader->GetFilename());
	// Construct the list of uniforms
	std::vector<const char*> uniformNames;
	std::vector<const char*> attributeNames;
#ifndef USE_CONSOLE
	uniformNames.resize(C4SSU_Count + ParameterNames.size() + 1);
	uniformNames[C4SSU_ProjectionMatrix] = "projectionMatrix";
	uniformNames[C4SSU_ModelViewMatrix] = "modelviewMatrix";
	uniformNames[C4SSU_NormalMatrix] = "normalMatrix";
	uniformNames[C4SSU_ClrMod] = "clrMod";
	uniformNames[C4SSU_Gamma] = "gamma";
	uniformNames[C4SSU_Resolution] = "resolution";
	uniformNames[C4SSU_BaseTex] = "baseTex"; // unused
	uniformNames[C4SSU_OverlayTex] = "overlayTex"; // unused
	uniformNames[C4SSU_OverlayClr] = "oc_PlayerColor";
	uniformNames[C4SSU_LightTex] = "lightTex";
	uniformNames[C4SSU_LightTransform] = "lightTransform";
	uniformNames[C4SSU_NormalTex] = "normalTex"; // unused
	uniformNames[C4SSU_AmbientTex] = "ambientTex";
	uniformNames[C4SSU_AmbientTransform] = "ambientTransform";
	uniformNames[C4SSU_AmbientBrightness] = "ambientBrightness";
	uniformNames[C4SSU_MaterialAmbient] = "materialAmbient";
	uniformNames[C4SSU_MaterialDiffuse] = "materialDiffuse";
	uniformNames[C4SSU_MaterialSpecular] = "materialSpecular";
	uniformNames[C4SSU_MaterialEmission] = "materialEmission";
	uniformNames[C4SSU_MaterialShininess] = "materialShininess";
	uniformNames[C4SSU_Bones] = "bones";
	uniformNames[C4SSU_CullMode] = "cullMode";
	uniformNames[C4SSU_FrameCounter] = "frameCounter";
	for (unsigned int i = 0; i < ParameterNames.size(); ++i)
		uniformNames[C4SSU_Count + i] = ParameterNames[i].getData();
	uniformNames[C4SSU_Count + ParameterNames.size()] = nullptr;
	attributeNames.resize(C4SSA_Count + 1);
	attributeNames[C4SSA_Position] = "oc_Position";
	attributeNames[C4SSA_Normal] = "oc_Normal";
	attributeNames[C4SSA_TexCoord] = "oc_TexCoord";
	attributeNames[C4SSA_Color] = "oc_Color"; // unused
	attributeNames[C4SSA_BoneIndices0] = "oc_BoneIndices0";
	attributeNames[C4SSA_BoneIndices1] = "oc_BoneIndices1";
	attributeNames[C4SSA_BoneWeights0] = "oc_BoneWeights0";
	attributeNames[C4SSA_BoneWeights1] = "oc_BoneWeights1";
	attributeNames[C4SSA_Count] = nullptr;
#endif
	// Compile the shader
	StdCopyStrBuf name(Name);
#ifndef USE_CONSOLE
	if (ssc != 0) name.Append(":");
	if (ssc & C4SSC_LIGHT) name.Append("Light");
	if (ssc & C4SSC_MOD2) name.Append("Mod2");
#endif
	return shader.Init(name.getData(), &uniformNames[0], &attributeNames[0]);
}

bool StdMeshMaterialProgram::Compile(StdMeshMaterialLoader& loader)
{
#ifndef USE_CONSOLE
	if (!CompileShader(loader, Shader, 0)) return false;
	if (!CompileShader(loader, ShaderMod2, C4SSC_MOD2)) return false;
	if (!CompileShader(loader, ShaderLight, C4SSC_LIGHT)) return false;
	if (!CompileShader(loader, ShaderLightMod2, C4SSC_LIGHT | C4SSC_MOD2)) return false;
#endif
	return true;
}

const C4Shader* StdMeshMaterialProgram::GetShader(int ssc) const
{
#ifndef USE_CONSOLE
	const C4Shader* shaders[4] = {
		&Shader,
		&ShaderMod2,
		&ShaderLight,
		&ShaderLightMod2
	};

	int index = 0;
	if(ssc & C4SSC_MOD2) index += 1;
	if(ssc & C4SSC_LIGHT) index += 2;

	assert(index < 4);
	return shaders[index];
#else
	return nullptr;
#endif
}

int StdMeshMaterialProgram::GetParameterIndex(const char* name) const
{
#ifndef USE_CONSOLE
	std::vector<StdCopyStrBuf>::const_iterator iter = std::find(ParameterNames.begin(), ParameterNames.end(), name);
	if(iter == ParameterNames.end()) return -1;
	return C4SSU_Count + std::distance(ParameterNames.begin(), iter);
#else
	return -1;
#endif
}

double StdMeshMaterialTextureUnit::Transformation::GetWaveXForm(double t) const
{
	assert(TransformType == T_WAVE_XFORM);
	const double val = fmod(WaveXForm.Frequency * t + WaveXForm.Phase, 1.0);
	switch (WaveXForm.Wave)
	{
	case W_SINE: return WaveXForm.Base + WaveXForm.Amplitude*0.5*(1.0 + sin(val * 2.0 * M_PI));
	case W_TRIANGLE: if (val < 0.5) return WaveXForm.Base + WaveXForm.Amplitude*2.0*val; else return WaveXForm.Base + WaveXForm.Amplitude*2.0*(1.0 - val);
	case W_SQUARE: if (val < 0.5) return WaveXForm.Base; else return WaveXForm.Base + WaveXForm.Amplitude;
	case W_SAWTOOTH: return WaveXForm.Base + WaveXForm.Amplitude*val;
	case W_INVERSE_SAWTOOTH: return WaveXForm.Base + WaveXForm.Amplitude*(1.0-val);
	default: assert(false); return 0.0;
	}
}

StdMeshMaterialTextureUnit::Tex::Tex(C4Surface* Surface)
	: RefCount(1), Surf(Surface), Texture(*Surface->texture)
{
}

StdMeshMaterialTextureUnit::Tex::~Tex()
{
	assert(RefCount == 0);
	delete Surf;
}

StdMeshMaterialTextureUnit::TexPtr::TexPtr(C4Surface* Surface)
		: pTex(new Tex(Surface))
{
}

StdMeshMaterialTextureUnit::TexPtr::TexPtr(const TexPtr& other)
		: pTex(other.pTex)
{
	++pTex->RefCount;
}

StdMeshMaterialTextureUnit::TexPtr::~TexPtr()
{
	if(!--pTex->RefCount)
		delete pTex;
}

StdMeshMaterialTextureUnit::TexPtr& StdMeshMaterialTextureUnit::TexPtr::operator=(const TexPtr& other)
{
	if(&other == this) return *this;

	if(!--pTex->RefCount)
		delete pTex;

	pTex = other.pTex;
	++pTex->RefCount;

	return *this;
}

StdMeshMaterialTextureUnit::StdMeshMaterialTextureUnit()
{
	TexBorderColor[0] = TexBorderColor[1] = TexBorderColor[2] = 0.0f; TexBorderColor[3] = 1.0f;
	Filtering[0] = Filtering[1] = F_Linear; Filtering[2] = F_Point;
	ColorOpSources[0] = BOS_Current; ColorOpSources[1] = BOS_Texture;
	AlphaOpSources[0] = BOS_Current; AlphaOpSources[1] = BOS_Texture;
	ColorOpManualColor1[0] = ColorOpManualColor1[1] = ColorOpManualColor1[2] = AlphaOpManualAlpha1 = 0.0f;
	ColorOpManualColor2[0] = ColorOpManualColor2[1] = ColorOpManualColor2[2] = AlphaOpManualAlpha2 = 0.0f;
}

void StdMeshMaterialTextureUnit::LoadTexture(StdMeshMaterialParserCtx& ctx, const char* texname)
{
	std::unique_ptr<C4Surface> surface(ctx.Loader.LoadTexture(texname)); // be exception-safe
	if (!surface.get())
		ctx.Error(StdCopyStrBuf("Could not load texture '") + texname + "'");

	if (surface->Wdt != surface->Hgt)
		ctx.Error(StdCopyStrBuf("Texture '") + texname + "' is not quadratic");

	Textures.emplace_back(surface.release());
}

void StdMeshMaterialTextureUnit::Load(StdMeshMaterialParserCtx& ctx)
{
	Token token;
	StdCopyStrBuf token_name;
	while ((token = ctx.AdvanceNonEOF(token_name)) == TOKEN_IDTF)
	{
		if (token_name == "texture")
		{
			Textures.clear();
			ctx.AdvanceRequired(token_name, TOKEN_IDTF);
			LoadTexture(ctx, token_name.getData());
		}
		else if (token_name == "anim_texture")
		{
			Textures.clear();

			StdCopyStrBuf base_name;
			ctx.AdvanceRequired(base_name, TOKEN_IDTF);

			int num_frames;
			if (ctx.AdvanceIntOptional(num_frames))
			{
				const char* data = base_name.getData();
				const char* sep = strrchr(data, '.');
				for (int i = 0; i < num_frames; ++i)
				{
					StdCopyStrBuf buf;
					if (sep)
						buf.Format("%.*s_%d.%s", (int)(sep - data), data, i, sep+1);
					else
						buf.Format("%s_%d", data, i);

					LoadTexture(ctx, buf.getData());
				}

				Duration = ctx.AdvanceFloat();
			}
			else
			{
				LoadTexture(ctx, base_name.getData());
				while (!ctx.AdvanceFloatOptional(Duration))
				{
					ctx.AdvanceRequired(token_name, TOKEN_IDTF);
					LoadTexture(ctx, token_name.getData());
				}
			}
		}
		else if (token_name == "tex_address_mode")
		{
			TexAddressMode = ctx.AdvanceEnum(TexAddressModeEnumerators);
		}
		else if (token_name == "tex_border_colour")
		{
			ctx.AdvanceColor(true, TexBorderColor);
		}
		else if (token_name == "filtering")
		{
			ctx.AdvanceEnums<3, StdMeshMaterialTextureUnit::FilteringType>(FilteringEnumerators, FilteringShortcuts, Filtering);
			if (Filtering[0] == F_None || Filtering[1] == F_None)
				ctx.Error(StdCopyStrBuf("'none' is only valid for the mip filter"));
			if (Filtering[2] == F_Anisotropic)
				ctx.Error(StdCopyStrBuf("'anisotropic' is not a valid mip filter"));
		}
		else if (token_name == "colour_op")
		{
			BlendOpType ColorOp = ctx.AdvanceEnum(BlendOpEnumerators);
			switch (ColorOp)
			{
			case BO_Replace:
				ColorOpEx = BOX_Source1;
				break;
			case BO_Add:
				ColorOpEx = BOX_Add;
				break;
			case BO_Modulate:
				ColorOpEx = BOX_Modulate;
				break;
			case BO_AlphaBlend:
				ColorOpEx = BOX_BlendTextureAlpha;
				break;
			}

			ColorOpSources[0] = BOS_Texture;
			ColorOpSources[1] = BOS_Current;
		}
		else if (token_name == "colour_op_ex")
		{
			ColorOpEx = ctx.AdvanceEnum(BlendOpExEnumerators);
			ColorOpSources[0] = ctx.AdvanceEnum(BlendOpSourceEnumerators);
			ColorOpSources[1] = ctx.AdvanceEnum(BlendOpSourceEnumerators);
			if (ColorOpEx == BOX_BlendManual) ColorOpManualFactor = ctx.AdvanceFloat();
			if (ColorOpSources[0] == BOS_Manual) ctx.AdvanceColor(false, ColorOpManualColor1);
			if (ColorOpSources[1] == BOS_Manual) ctx.AdvanceColor(false, ColorOpManualColor2);
		}
		else if (token_name == "alpha_op_ex")
		{
			AlphaOpEx = ctx.AdvanceEnum(BlendOpExEnumerators);
			AlphaOpSources[0] = ctx.AdvanceEnum(BlendOpSourceEnumerators);
			AlphaOpSources[1] = ctx.AdvanceEnum(BlendOpSourceEnumerators);
			if (AlphaOpEx == BOX_BlendManual) AlphaOpManualFactor = ctx.AdvanceFloat();
			if (AlphaOpSources[0] == BOS_Manual) AlphaOpManualAlpha1 = ctx.AdvanceFloat();
			if (AlphaOpSources[1] == BOS_Manual) AlphaOpManualAlpha2 = ctx.AdvanceFloat();
		}
		else if (token_name == "scroll")
		{
			Transformation trans;
			trans.TransformType = Transformation::T_SCROLL;
			trans.Scroll.X = ctx.AdvanceFloat();
			trans.Scroll.Y = ctx.AdvanceFloat();
			Transformations.push_back(trans);
		}
		else if (token_name == "scroll_anim")
		{
			Transformation trans;
			trans.TransformType = Transformation::T_SCROLL_ANIM;
			trans.ScrollAnim.XSpeed = ctx.AdvanceFloat();
			trans.ScrollAnim.YSpeed = ctx.AdvanceFloat();
			Transformations.push_back(trans);
		}
		else if (token_name == "rotate")
		{
			Transformation trans;
			trans.TransformType = Transformation::T_ROTATE;
			trans.Rotate.Angle = ctx.AdvanceFloat();
			Transformations.push_back(trans);
		}
		else if (token_name == "rotate_anim")
		{
			Transformation trans;
			trans.TransformType = Transformation::T_ROTATE_ANIM;
			trans.RotateAnim.RevsPerSec = ctx.AdvanceFloat();
			Transformations.push_back(trans);
		}
		else if (token_name == "scale")
		{
			Transformation trans;
			trans.TransformType = Transformation::T_SCALE;
			trans.Scale.X = ctx.AdvanceFloat();
			trans.Scale.Y = ctx.AdvanceFloat();
			Transformations.push_back(trans);
		}
		else if (token_name == "transform")
		{
			Transformation trans;
			trans.TransformType = Transformation::T_TRANSFORM;
			for (float & i : trans.Transform.M)
				i = ctx.AdvanceFloat();
			Transformations.push_back(trans);
		}
		else if (token_name == "wave_xform")
		{
			Transformation trans;
			trans.TransformType = Transformation::T_WAVE_XFORM;
			trans.WaveXForm.XForm = ctx.AdvanceEnum(XFormTypeEnumerators);
			trans.WaveXForm.Wave = ctx.AdvanceEnum(WaveTypeEnumerators);
			trans.WaveXForm.Base = ctx.AdvanceFloat();
			trans.WaveXForm.Frequency = ctx.AdvanceFloat();
			trans.WaveXForm.Phase = ctx.AdvanceFloat();
			trans.WaveXForm.Amplitude = ctx.AdvanceFloat();
			Transformations.push_back(trans);
		}
		else
			ctx.ErrorUnexpectedIdentifier(token_name);
	}

	if (token != TOKEN_BRACE_CLOSE)
		ctx.Error(StdCopyStrBuf("'") + token_name.getData() + "' unexpected");
}

StdMeshMaterialPass::ProgramInstance::ProgramInstance(const StdMeshMaterialProgram* program, const ShaderInstance* fragment_instance, const ShaderInstance* vertex_instance, const ShaderInstance* geometry_instance):
	Program(program)
{
	// Consistency check
	assert(Program->GetFragmentShader() == fragment_instance->Shader);
	assert(Program->GetVertexShader() == vertex_instance->Shader);
	assert(Program->GetGeometryShader() == geometry_instance->Shader);

	// Load instance parameters, i.e. connect parameter values with uniform index
	LoadParameterRefs(fragment_instance);
	LoadParameterRefs(vertex_instance);
	LoadParameterRefs(geometry_instance);
}

void StdMeshMaterialPass::ProgramInstance::LoadParameterRefs(const ShaderInstance* instance)
{
	for(const auto & NamedParameter : instance->Parameters.NamedParameters)
	{
		const int index = Program->GetParameterIndex(NamedParameter.first.getData());
		assert(index != -1);

		const std::vector<ParameterRef>::const_iterator parameter_iter =
			std::find_if(Parameters.begin(), Parameters.end(), [index](const ParameterRef& ref) { return ref.UniformIndex == index; });
		if(parameter_iter != Parameters.end())
		{
			// TODO: Check that the current parameter has the same value as the found one
			continue;
		}
		else
		{
			ParameterRef ref;
			ref.Parameter = &NamedParameter.second;
			ref.UniformIndex = index;
			Parameters.push_back(ref);
		}
	}
}

StdMeshMaterialPass::StdMeshMaterialPass()
{
	Ambient[0]  = Ambient[1]  = Ambient[2]  = 1.0f; Ambient[3]  = 1.0f;
	Diffuse[0]  = Diffuse[1]  = Diffuse[2]  = 1.0f; Diffuse[3]  = 1.0f;
	Specular[0] = Specular[1] = Specular[2] = 0.0f; Specular[3] = 0.0f;
	Emissive[0] = Emissive[1] = Emissive[2] = 0.0f; Emissive[3] = 0.0f;
	Shininess = 0.0f;
	SceneBlendFactors[0] = SB_One; SceneBlendFactors[1] = SB_Zero;
	AlphaRejectionFunction = DF_AlwaysPass; AlphaRejectionValue = 0.0f;
	AlphaToCoverage = false;
	VertexShader.Shader = FragmentShader.Shader = GeometryShader.Shader = nullptr;
}

void StdMeshMaterialPass::LoadShaderRef(StdMeshMaterialParserCtx& ctx, StdMeshMaterialShaderType type)
{
	StdStrBuf program_name, token;
	ctx.AdvanceRequired(program_name, TOKEN_IDTF);

	ShaderInstance* cur_shader;
	const StdMeshMaterialShader* shader;
	const char* shader_type_name;

	switch(type)
	{
	case SMMS_FRAGMENT:
		cur_shader = &FragmentShader;
		shader = ctx.Manager.GetFragmentShader(program_name.getData());
		shader_type_name = "fragment";
		break;
	case SMMS_VERTEX:
		cur_shader = &VertexShader;
		shader = ctx.Manager.GetVertexShader(program_name.getData());
		shader_type_name = "vertex";
		break;
	case SMMS_GEOMETRY:
		cur_shader = &GeometryShader;
		shader = ctx.Manager.GetGeometryShader(program_name.getData());
		shader_type_name = "geometry";
		break;
	default: // can't happen
		assert(0);
		return;
	}

	if(cur_shader->Shader != nullptr)
		ctx.Error(FormatString("There is already a %s shader in this pass", shader_type_name));
	if(!shader)
		ctx.Error(FormatString("There is no such %s shader with name %s", shader_type_name, program_name.getData()));

	cur_shader->Shader = shader;
	cur_shader->Parameters.Load(ctx);
}

void StdMeshMaterialPass::Load(StdMeshMaterialParserCtx& ctx)
{
	Token token;
	StdCopyStrBuf token_name;
	StdMeshMaterialSubLoader texture_unit_loader;
	while ((token = ctx.AdvanceNonEOF(token_name)) == TOKEN_IDTF)
	{
		if (token_name == "texture_unit")
		{
			texture_unit_loader.Load(ctx, TextureUnits);
		}
		else if (token_name == "ambient")
		{
			ctx.AdvanceColor(true, Ambient);
		}
		else if (token_name == "diffuse")
		{
			ctx.AdvanceColor(true, Diffuse);
		}
		else if (token_name == "specular")
		{
			Specular[0] = ctx.AdvanceFloat();
			Specular[1] = ctx.AdvanceFloat();
			Specular[2] = ctx.AdvanceFloat();

			// The fourth argument is optional, not the fifth:
			float specular3 = ctx.AdvanceFloat();

			float shininess;
			if (ctx.AdvanceFloatOptional(shininess))
			{
				Specular[3] = specular3;
				Shininess = shininess;
			}
			else
			{
				Shininess = specular3;
			}
		}
		else if (token_name == "emissive")
		{
			ctx.AdvanceColor(true, Emissive);
		}
		else if (token_name == "depth_check")
		{
			DepthCheck = ctx.AdvanceBoolean();
		}
		else if (token_name == "depth_write")
		{
			DepthWrite = ctx.AdvanceBoolean();
		}
		else if (token_name == "cull_hardware")
		{
			CullHardware = ctx.AdvanceEnum(CullHardwareEnumerators);
		}
		else if (token_name == "scene_blend")
		{
			ctx.AdvanceEnums<2, StdMeshMaterialPass::SceneBlendType>(SceneBlendEnumerators, SceneBlendShortcuts, SceneBlendFactors);
		}
		else if (token_name == "scene_blend_op")
		{
			StdStrBuf op;
			ctx.AdvanceRequired(op, TOKEN_IDTF);
			ctx.WarningNotSupported(token_name.getData());
		}
		else if (token_name == "alpha_rejection")
		{
			AlphaRejectionFunction = ctx.AdvanceEnum(DepthFunctionEnumerators);
			if (AlphaRejectionFunction != DF_AlwaysFail && AlphaRejectionFunction != DF_AlwaysPass)
				AlphaRejectionValue = ctx.AdvanceFloat() / 255.0f;
		}
		else if (token_name == "alpha_to_coverage")
		{
			AlphaToCoverage = ctx.AdvanceBoolean();
		}
		else if (token_name == "colour_write")
		{
			ctx.AdvanceBoolean();
			ctx.WarningNotSupported("colour_write");
		}
		else if (token_name == "depth_func")
		{
			StdStrBuf func;
			ctx.AdvanceRequired(func, TOKEN_IDTF);
			ctx.WarningNotSupported(token_name.getData());
		}
		else if (token_name == "illumination_stage")
		{
			ctx.WarningNotSupported(token_name.getData());
		}
		else if (token_name == "light_clip_planes")
		{
			ctx.AdvanceBoolean();
			ctx.WarningNotSupported(token_name.getData());
		}
		else if (token_name == "light_scissor")
		{
			ctx.AdvanceBoolean();
			ctx.WarningNotSupported(token_name.getData());
		}
		else if (token_name == "lighting")
		{
			ctx.AdvanceBoolean();
			ctx.WarningNotSupported(token_name.getData());
		}
		else if (token_name == "normalise_normals" || token_name == "normalize_normals")
		{
			ctx.AdvanceBoolean();
			ctx.WarningNotSupported(token_name.getData());
		}
		else if (token_name == "polygon_mode")
		{
			StdStrBuf mode;
			ctx.AdvanceRequired(mode, TOKEN_IDTF);
			ctx.WarningNotSupported(token_name.getData());
		}
		else if (token_name == "shading")
		{
			StdStrBuf shading;
			ctx.AdvanceRequired(shading, TOKEN_IDTF);
			ctx.WarningNotSupported(token_name.getData());
		}
		else if (token_name == "transparent_sorting")
		{
			ctx.AdvanceBoolean();
			ctx.WarningNotSupported(token_name.getData());
		}
		else if (token_name == "vertex_program_ref")
		{
			LoadShaderRef(ctx, SMMS_VERTEX);
		}
		else if (token_name == "fragment_program_ref")
		{
			LoadShaderRef(ctx, SMMS_FRAGMENT);
		}
		else if (token_name == "geometry_program_ref")
		{
			LoadShaderRef(ctx, SMMS_GEOMETRY);
		}
		else
			ctx.ErrorUnexpectedIdentifier(token_name);
	}

	if (token != TOKEN_BRACE_CLOSE)
		ctx.Error(StdCopyStrBuf("'") + token_name.getData() + "' unexpected");
}

StdMeshMaterialTechnique::StdMeshMaterialTechnique() = default;

void StdMeshMaterialTechnique::Load(StdMeshMaterialParserCtx& ctx)
{
	Token token;
	StdCopyStrBuf token_name;
	StdMeshMaterialSubLoader pass_loader;
	while ((token = ctx.AdvanceNonEOF(token_name)) == TOKEN_IDTF)
	{
		if (token_name == "pass")
		{
			pass_loader.Load(ctx, Passes);
		}
		else
			ctx.ErrorUnexpectedIdentifier(token_name);
	}

	if (token != TOKEN_BRACE_CLOSE)
		ctx.Error(StdCopyStrBuf("'") + token_name.getData() + "' unexpected");
}

bool StdMeshMaterialTechnique::IsOpaque() const
{
	// Technique is opaque if one of the passes is opaque (subsequent
	// non-opaque passes will just depend on the opaque value drawn in
	// the previous pass; total result will not depend on original
	// frame buffer value).
	for(const auto & Pass : Passes)
		if(Pass.IsOpaque())
			return true;
	return false;
}

StdMeshMaterial::StdMeshMaterial() = default;

void StdMeshMaterial::Load(StdMeshMaterialParserCtx& ctx)
{
	Token token;
	StdCopyStrBuf token_name;
	StdMeshMaterialSubLoader technique_loader;
	while ((token = ctx.AdvanceNonEOF(token_name)) == TOKEN_IDTF)
	{
		if (token_name == "technique")
		{
			technique_loader.Load(ctx, Techniques);
		}
		else if (token_name == "receive_shadows")
		{
			ReceiveShadows = ctx.AdvanceBoolean();
		}
		else
			ctx.ErrorUnexpectedIdentifier(token_name);
	}

	if (token != TOKEN_BRACE_CLOSE)
		ctx.Error(StdCopyStrBuf("'") + token_name.getData() + "' unexpected");
}

void StdMeshMatManager::Clear()
{
	Materials.clear();

	Programs.clear();
	FragmentShaders.clear();
	VertexShaders.clear();
	GeometryShaders.clear();
}

std::set<StdCopyStrBuf> StdMeshMatManager::Parse(const char* mat_script, const char* filename, StdMeshMaterialLoader& loader)
{
	StdMeshMaterialParserCtx ctx(*this, mat_script, filename, loader);

	Token token;
	StdCopyStrBuf token_name;

	std::set<StdCopyStrBuf> loaded_materials;

	while ((token = ctx.Advance(token_name)) == TOKEN_IDTF)
	{
		if (token_name == "material")
		{
			// Read name
			StdCopyStrBuf material_name;
			ctx.AdvanceRequired(material_name, TOKEN_IDTF);

			// Check for uniqueness
			std::map<StdCopyStrBuf, StdMeshMaterial>::iterator iter = Materials.find(material_name);
			if (iter != Materials.end())
				ctx.Error(FormatString("Material with name '%s' is already defined in %s:%u", material_name.getData(), iter->second.FileName.getData(), iter->second.Line));

			// Check if there is a parent given
			Token next = ctx.AdvanceRequired(token_name, TOKEN_BRACE_OPEN, TOKEN_COLON);
			// Read parent name, if any
			StdMeshMaterial* parent = nullptr;
			if (next == TOKEN_COLON)
			{
				// Note that if there is a parent, then it needs to be loaded
				// already. This currently makes only sense when its defined above
				// in the same material script file or in a parent definition.
				// We could later support material scripts in the System.ocg.
				StdCopyStrBuf parent_name;
				ctx.AdvanceRequired(parent_name, TOKEN_IDTF);
				ctx.AdvanceRequired(token_name, TOKEN_BRACE_OPEN);

				iter = Materials.find(parent_name);
				if (iter == Materials.end())
					ctx.Error(StdCopyStrBuf("Parent material '") + parent_name + "' does not exist (or is not yet loaded)");
				parent = &iter->second;
			}

			// Copy properties from parent if one is given, otherwise
			// default-construct the material.
			StdMeshMaterial mat = parent ? StdMeshMaterial(*parent) : StdMeshMaterial();

			// Set/Overwrite source and name
			mat.Name = material_name;
			mat.FileName = ctx.FileName;
			mat.Line = ctx.Line;

			mat.Load(ctx);

			Materials[material_name] = mat;

#ifndef USE_CONSOLE
			// To Gfxspecific setup of the material; choose working techniques
			if (!pDraw->PrepareMaterial(*this, loader, Materials[material_name]))
			{
				Materials.erase(material_name);
				ctx.Error(StdCopyStrBuf("No working technique for material '") + material_name + "'");
			}
#endif
			loaded_materials.insert(material_name);
		}
		else if (token_name == "vertex_program")
		{
			LoadShader(ctx, SMMS_VERTEX);
		}
		else if (token_name == "fragment_program")
		{
			LoadShader(ctx, SMMS_FRAGMENT);
		}
		else if (token_name == "geometry_program")
		{
			LoadShader(ctx, SMMS_GEOMETRY);
		}
		else
			ctx.ErrorUnexpectedIdentifier(token_name);
	}

	if (token != TOKEN_EOF)
		ctx.Error(StdCopyStrBuf("'") + token_name.getData() + "' unexpected");

	return loaded_materials;
}

const StdMeshMaterial* StdMeshMatManager::GetMaterial(const char* material_name) const
{
	std::map<StdCopyStrBuf, StdMeshMaterial>::const_iterator iter = Materials.find(StdCopyStrBuf(material_name));
	if (iter == Materials.end()) return nullptr;
	return &iter->second;
}

StdMeshMatManager::Iterator StdMeshMatManager::Remove(const Iterator& iter, StdMeshMaterialUpdate* update)
{
	if(update) update->Add(&*iter);
	Iterator next_iter = iter;
	++next_iter;
	Materials.erase(iter.iter_);
	return next_iter;
}

void StdMeshMatManager::Remove(const StdStrBuf &name, StdMeshMaterialUpdate *update)
{
	auto it = Materials.find(StdCopyStrBuf(name));
	if (it == Materials.end())
		return;
	if (update) update->Add(&it->second);
	Materials.erase(it);
}

const StdMeshMaterialShader* StdMeshMatManager::GetFragmentShader(const char* name) const
{
	ShaderMap::const_iterator iter = FragmentShaders.find(StdCopyStrBuf(name));
	if(iter == FragmentShaders.end()) return nullptr;
	return iter->second.get();
}

const StdMeshMaterialShader* StdMeshMatManager::GetVertexShader(const char* name) const
{
	ShaderMap::const_iterator iter = VertexShaders.find(StdCopyStrBuf(name));
	if(iter == VertexShaders.end()) return nullptr;
	return iter->second.get();
}

const StdMeshMaterialShader* StdMeshMatManager::GetGeometryShader(const char* name) const
{
	ShaderMap::const_iterator iter = GeometryShaders.find(StdCopyStrBuf(name));
	if(iter == GeometryShaders.end()) return nullptr;
	return iter->second.get();
}

const StdMeshMaterialShader* StdMeshMatManager::AddShader(const char* filename, const char* name, const char* language, StdMeshMaterialShaderType type, const char* text, uint32_t load_flags)
{
	ShaderMap* map = nullptr;
	switch(type)
	{
	case SMMS_FRAGMENT:
		map = &FragmentShaders;
		break;
	case SMMS_VERTEX:
		map = &VertexShaders;
		break;
	case SMMS_GEOMETRY:
		map = &GeometryShaders;
		break;
	}

	StdCopyStrBuf name_buf(name);
	ShaderMap::iterator iter = map->find(name_buf);
	
	if(iter != map->end())
	{
		// Shader exists
		if ((load_flags & SMM_ForceReload) == SMM_ForceReload)
			map->erase(iter);
		else if ((load_flags & SMM_AcceptExisting) == SMM_AcceptExisting)
			return iter->second.get();
		else
			return nullptr;
	}

	std::unique_ptr<StdMeshMaterialShader> shader(new StdMeshMaterialShader(filename, name, language, type, text));
	std::pair<ShaderMap::iterator, bool> inserted = map->insert(std::make_pair(name_buf, std::move(shader)));
	assert(inserted.second == true);
	iter = inserted.first;

	return iter->second.get();
}

const StdMeshMaterialProgram* StdMeshMatManager::AddProgram(const char* name, StdMeshMaterialLoader& loader, const StdMeshMaterialPass::ShaderInstance& fragment_shader, const StdMeshMaterialPass::ShaderInstance& vertex_shader, const StdMeshMaterialPass::ShaderInstance& geometry_shader)
{
	std::tuple<const StdMeshMaterialShader*, const StdMeshMaterialShader*, const StdMeshMaterialShader*> key = std::make_tuple(fragment_shader.Shader, vertex_shader.Shader, geometry_shader.Shader);
	ProgramMap::iterator iter = Programs.find(key);
	if(iter == Programs.end())
	{
		std::unique_ptr<StdMeshMaterialProgram> program(new StdMeshMaterialProgram(name, fragment_shader.Shader, vertex_shader.Shader, geometry_shader.Shader));
		iter = Programs.insert(std::make_pair(key, std::move(program))).first;
	}

	StdMeshMaterialProgram& inserted_program = *iter->second;

	const bool fragment_added = inserted_program.AddParameterNames(fragment_shader.Parameters);
	const bool vertex_added = inserted_program.AddParameterNames(vertex_shader.Parameters);
	const bool geometry_added = inserted_program.AddParameterNames(geometry_shader.Parameters);

	// Re-compile the program (and assign new uniform locations if new
	// parameters were encountered).
	if(!inserted_program.IsCompiled() || fragment_added || vertex_added || geometry_added)
		if(!inserted_program.Compile(loader))
			return nullptr;

	return &inserted_program;
}

StdMeshMatManager MeshMaterialManager;
