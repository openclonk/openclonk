/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2009  Armin Burgmeier
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de
 *
 * Portions might be copyrighted by other authors who have contributed
 * to OpenClonk.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * See isc_license.txt for full license and disclaimer.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 * See clonk_trademark_license.txt for full license.
 */

#include "C4Include.h"
#include <StdMeshMaterial.h>
#include <StdDDraw2.h>

#include <cctype>
#include <memory>

#ifdef WITH_GLIB
#include <glib.h>
#else
#include <sstream>
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

	const Enumerator<StdMeshMaterialTextureUnit::TexAddressModeType> TexAddressModeEnumerators[] = {
		{ "wrap", StdMeshMaterialTextureUnit::AM_Wrap },
		{ "clamp", StdMeshMaterialTextureUnit::AM_Clamp },
		{ "mirror", StdMeshMaterialTextureUnit::AM_Mirror },
		{ "border", StdMeshMaterialTextureUnit::AM_Border },
		{ NULL }
	};

	const Enumerator<StdMeshMaterialTextureUnit::FilteringType> FilteringEnumerators[] = {
		{ "none", StdMeshMaterialTextureUnit::F_None },
		{ "point", StdMeshMaterialTextureUnit::F_Point },
		{ "linear", StdMeshMaterialTextureUnit::F_Linear },
		{ "anisotropic", StdMeshMaterialTextureUnit::F_Anisotropic },
		{ NULL }
	};

	const EnumeratorShortcut<3, StdMeshMaterialTextureUnit::FilteringType> FilteringShortcuts[] = {
		{ "none", { StdMeshMaterialTextureUnit::F_Point, StdMeshMaterialTextureUnit::F_Point, StdMeshMaterialTextureUnit::F_None } },
		{ "bilinear", { StdMeshMaterialTextureUnit::F_Linear, StdMeshMaterialTextureUnit::F_Linear, StdMeshMaterialTextureUnit::F_Point } },
		{ "trilinear", { StdMeshMaterialTextureUnit::F_Linear, StdMeshMaterialTextureUnit::F_Linear, StdMeshMaterialTextureUnit::F_Linear } },
		{ "anisotropic", { StdMeshMaterialTextureUnit::F_Anisotropic, StdMeshMaterialTextureUnit::F_Anisotropic, StdMeshMaterialTextureUnit::F_Linear } },
		{ NULL }
	};

	const Enumerator<StdMeshMaterialTextureUnit::BlendOpType> BlendOpEnumerators[] = {
		{ "replace", StdMeshMaterialTextureUnit::BO_Replace },
		{ "add", StdMeshMaterialTextureUnit::BO_Add },
		{ "modulate", StdMeshMaterialTextureUnit::BO_Modulate },
		{ "alpha_blend", StdMeshMaterialTextureUnit::BO_AlphaBlend },
		{ NULL }
	};

	const Enumerator<StdMeshMaterialTextureUnit::BlendOpExType> BlendOpExEnumerators[] = {
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
		{ NULL }
	};
	
	const Enumerator<StdMeshMaterialTextureUnit::BlendOpSourceType> BlendOpSourceEnumerators[] = {
		{ "src_current", StdMeshMaterialTextureUnit::BOS_Current },
		{ "src_texture", StdMeshMaterialTextureUnit::BOS_Texture },
		{ "src_diffuse", StdMeshMaterialTextureUnit::BOS_Diffuse },
		{ "src_specular", StdMeshMaterialTextureUnit::BOS_Specular },
		{ "src_player_colour", StdMeshMaterialTextureUnit::BOS_PlayerColor },
		{ "src_manual", StdMeshMaterialTextureUnit::BOS_Manual },
		{ NULL }
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
	StdMeshMaterialParserCtx(const char* mat_script, const char* filename, StdMeshMaterialTextureLoader& tex_loader);

	void SkipWhitespace();
	Token Peek(StdStrBuf& name);
	Token Advance(StdStrBuf& name);
	Token AdvanceNonEOF(StdStrBuf& name);
	Token AdvanceRequired(StdStrBuf& name, Token expect);
	Token AdvanceRequired(StdStrBuf& name, Token expect1, Token expect2);
	float AdvanceFloat();
	bool AdvanceFloatOptional(float& value);
	void AdvanceColor(bool with_alpha, float Color[4]);
	bool AdvanceBoolean();
	template<typename EnumType> EnumType AdvanceEnum(const Enumerator<EnumType>* enumerators);
	template<int Num, typename EnumType> void AdvanceEnums(const Enumerator<EnumType>* enumerators, EnumType enums[Num]);
	template<int Num, typename EnumType> void AdvanceEnums(const Enumerator<EnumType>* enumerators, const EnumeratorShortcut<Num, EnumType>* shortcuts, EnumType enums[Num]);
	void Error(const StdStrBuf& message);
	void ErrorUnexpectedIdentifier(const StdStrBuf& identifier);

	// Current parsing data
	unsigned int Line;
	const char* Script;

	StdCopyStrBuf FileName;
	StdMeshMaterialTextureLoader& TextureLoader;
};

StdMeshMaterialParserCtx::StdMeshMaterialParserCtx(const char* mat_script, const char* filename, StdMeshMaterialTextureLoader& tex_loader):
	Line(0), Script(mat_script), FileName(filename), TextureLoader(tex_loader)
{
}

void StdMeshMaterialParserCtx::SkipWhitespace()
{
	while(isspace(*Script))
	{
		if(*Script == '\n') ++Line;
		++Script;
	}
	
	if(*Script == '/')
	{
		if(*(Script+1) == '/')
		{
			Script += 2;
			while(*Script != '\n' && *Script != '\0')
				++Script;
			SkipWhitespace();
		}
		else if(*Script == '*')
		{
			for(Script += 2; *Script != '\0'; ++Script)
				if(*Script == '*' && *(Script+1) == '/')
					break;

			if(*Script == '*')
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

	switch(*Script)
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
		do { ++Script; } while(!isspace(*Script) && *Script != '{' && *Script != '}' && *Script != ':');
		name.Copy(begin, Script - begin);
		return TOKEN_IDTF;
	}
}

Token StdMeshMaterialParserCtx::AdvanceNonEOF(StdStrBuf& name)
{
	Token token = Advance(name);
	if(token == TOKEN_EOF) Error(StdCopyStrBuf("Unexpected end of file"));
	return token;
}

Token StdMeshMaterialParserCtx::AdvanceRequired(StdStrBuf& name, Token expect)
{
	Token token = AdvanceNonEOF(name);
	// TODO: Explain what was actually expected
	if(token != expect) Error(StdCopyStrBuf("'") + name + "' unexpected");
	return token;
}

Token StdMeshMaterialParserCtx::AdvanceRequired(StdStrBuf& name, Token expect1, Token expect2)
{
	Token token = AdvanceNonEOF(name);
	// TODO: Explain what was actually expected
	if(token != expect1 && token != expect2)
		Error(StdStrBuf("'") + name + "' unexpected");
	return token;
}

#ifdef _MSC_VER
static inline float strtof(const char *_Str, char **_EndPtr)
{
	return static_cast<float>(strtod(_Str, _EndPtr));
}
#endif

float StdMeshMaterialParserCtx::AdvanceFloat()
{
	StdStrBuf buf;
	AdvanceRequired(buf, TOKEN_IDTF);
	float f;
#ifdef WITH_GLIB
	char* end;
	f = g_ascii_strtod(buf.getData(), &end);
	if(*end != '\0')
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

	if(tok == TOKEN_IDTF && isdigit(buf[0]))
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
	if(with_alpha) AdvanceFloatOptional(Color[3]);
}

bool StdMeshMaterialParserCtx::AdvanceBoolean()
{
	StdCopyStrBuf buf;
	AdvanceRequired(buf, TOKEN_IDTF);
	if(buf == "on") return true;
	if(buf == "off") return false;
	Error(StdCopyStrBuf("Expected either 'on' or 'off', but not '") + buf + "'");
	return false; // Never reached
}

template<typename EnumType>
EnumType StdMeshMaterialParserCtx::AdvanceEnum(const Enumerator<EnumType>* enumerators)
{
	StdCopyStrBuf buf;
	AdvanceRequired(buf, TOKEN_IDTF);

	for(const Enumerator<EnumType>* cur = enumerators; cur->Name; ++cur)
		if(buf == cur->Name)
			return cur->Value;

	ErrorUnexpectedIdentifier(buf);
	return EnumType(); // avoid compiler warning
}

template<int Num, typename EnumType>
void StdMeshMaterialParserCtx::AdvanceEnums(const Enumerator<EnumType>* enumerators, EnumType enums[Num])
{
	for(int i = 0; i < Num; ++i)
		enums[i] = AdvanceEnum(enumerators);
}

template<int Num, typename EnumType>
void StdMeshMaterialParserCtx::AdvanceEnums(const Enumerator<EnumType>* enumerators, const EnumeratorShortcut<Num, EnumType>* shortcuts, EnumType enums[Num])
{
	StdCopyStrBuf buf;
	AdvanceRequired(buf, TOKEN_IDTF);

	const Enumerator<EnumType>* cenum;
	const EnumeratorShortcut<Num, EnumType>* cshort;

	for(cenum = enumerators; cenum->Name; ++cenum)
		if(buf == cenum->Name)
			break;
	for(cshort = shortcuts; cshort->Name; ++cshort)
		if(buf == cshort->Name)
			break;

	if(!cenum->Name && !cshort->Name)
	{
		ErrorUnexpectedIdentifier(buf);
	}
	else if(!cenum->Name && cshort->Name)
	{
		for(int i = 0; i < Num; ++i)
			enums[i] = cshort->Values[i];
	}
	else if(cenum->Name && (!cshort->Name || Num == 1))
	{
		enums[0] = cenum->Value;
		for(int i = 1; i < Num; ++i)
			enums[i] = AdvanceEnum(enumerators);
	}
	else
	{
		// Both enumerator and shortcut are possible, determine by look-ahead
		const Enumerator<EnumType>* cenum2 = NULL;
		Token tok = Peek(buf);
		if(tok == TOKEN_IDTF)
		{
			for(cenum2 = enumerators; cenum2->Name; ++cenum2)
				if(buf == cenum2->Name)
					break;
		}

		if(cenum2 && cenum2->Name)
		{
			// The next item is an enumerator, so load as enumerators
			enums[0] = cenum->Value;
			for(int i = 1; i < Num; ++i)
				enums[i] = AdvanceEnum(enumerators);
		}
		else
		{
			// The next item is something else, so load the shortcut
			for(int i = 0; i < Num; ++i)
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

StdMeshMaterialTextureUnit::TexRef::TexRef(C4Surface* Surface):
	RefCount(1), Surf(Surface), Tex(*Surface->ppTex[0])
{
	assert(Surface->ppTex != NULL);
}

StdMeshMaterialTextureUnit::TexRef::~TexRef()
{
	assert(RefCount == 0);
	delete Surf;
}

StdMeshMaterialTextureUnit::StdMeshMaterialTextureUnit():
	TexAddressMode(AM_Wrap), ColorOpEx(BOX_Modulate), ColorOpManualFactor(0.0f),
	AlphaOpEx(BOX_Modulate), AlphaOpManualFactor(0.0f), Texture(NULL)
{
	TexBorderColor[0] = TexBorderColor[1] = TexBorderColor[2] = 0.0f; TexBorderColor[3] = 1.0f;
	Filtering[0] = Filtering[1] = F_Linear; Filtering[2] = F_Point;
	ColorOpSources[0] = BOS_Current; ColorOpSources[1] = BOS_Texture;
	AlphaOpSources[0] = BOS_Current; AlphaOpSources[1] = BOS_Texture;
	ColorOpManualColor1[0] = ColorOpManualColor1[1] = ColorOpManualColor1[2] = AlphaOpManualAlpha1 = 0.0f;
	ColorOpManualColor2[0] = ColorOpManualColor2[1] = ColorOpManualColor2[2] = AlphaOpManualAlpha2 = 0.0f;
}

StdMeshMaterialTextureUnit::StdMeshMaterialTextureUnit(const StdMeshMaterialTextureUnit& other):
	TexAddressMode(other.TexAddressMode),
	ColorOpEx(other.ColorOpEx), ColorOpManualFactor(other.ColorOpManualFactor),
	AlphaOpEx(other.AlphaOpEx), AlphaOpManualFactor(other.AlphaOpManualFactor), Texture(other.Texture)
{
	if(Texture)
		++Texture->RefCount;

	for(unsigned int i = 0; i < 4; ++i)
		TexBorderColor[i] = other.TexBorderColor[i];

	for(unsigned int i = 0; i < 3; ++i)
		Filtering[i] = other.Filtering[i];

	ColorOpSources[0] = other.ColorOpSources[0];
	ColorOpSources[1] = other.ColorOpSources[1];
	for(unsigned int i = 0; i < 3; ++i)
	{
		ColorOpManualColor1[i] = other.ColorOpManualColor1[i];
		ColorOpManualColor2[i] = other.ColorOpManualColor2[i];
	}
	
	AlphaOpSources[0] = other.AlphaOpSources[0];
	AlphaOpSources[1] = other.AlphaOpSources[1];
	AlphaOpManualAlpha1 = other.AlphaOpManualAlpha1;
	AlphaOpManualAlpha2 = other.AlphaOpManualAlpha2;
}

StdMeshMaterialTextureUnit::~StdMeshMaterialTextureUnit()
{
	if(Texture && !--Texture->RefCount)
		delete Texture;
}

StdMeshMaterialTextureUnit& StdMeshMaterialTextureUnit::operator=(const StdMeshMaterialTextureUnit& other)
{
	if(this == &other) return *this;
	if(Texture) if(!--Texture->RefCount) delete Texture;
	Texture = other.Texture;
	if(Texture) ++Texture->RefCount;
	TexAddressMode = other.TexAddressMode;
	for(unsigned int i = 0; i < 4; ++i)
		TexBorderColor[i] = other.TexBorderColor[i];
	for(unsigned int i = 0; i < 3; ++i)
		Filtering[i] = other.Filtering[i];
	ColorOpEx = other.ColorOpEx;
	ColorOpSources[0] = other.ColorOpSources[0];
	ColorOpSources[1] = other.ColorOpSources[1];
	ColorOpManualFactor = other.ColorOpManualFactor;
	for(unsigned int i = 0; i < 3; ++i)
	{
		ColorOpManualColor1[i] = other.ColorOpManualColor1[i];
		ColorOpManualColor2[i] = other.ColorOpManualColor2[i];
	}

	AlphaOpEx = other.AlphaOpEx;
	AlphaOpSources[0] = other.AlphaOpSources[0];
	AlphaOpSources[1] = other.AlphaOpSources[1];
	AlphaOpManualFactor = other.AlphaOpManualFactor;
	AlphaOpManualAlpha1 = other.AlphaOpManualAlpha1;
	AlphaOpManualAlpha2 = other.AlphaOpManualAlpha1;

	return *this;
}

void StdMeshMaterialTextureUnit::Load(StdMeshMaterialParserCtx& ctx)
{
	Token token;
	StdCopyStrBuf token_name;
	while((token = ctx.AdvanceNonEOF(token_name)) == TOKEN_IDTF)
	{
		if(token_name == "texture")
		{
			ctx.AdvanceRequired(token_name, TOKEN_IDTF);

			std::auto_ptr<C4Surface> surface(ctx.TextureLoader.LoadTexture(token_name.getData())); // be exception-safe
			if(!surface.get())
				ctx.Error(StdCopyStrBuf("Could not load texture '") + token_name + "'");

			if(surface->Wdt != surface->Hgt)
				ctx.Error(StdCopyStrBuf("Texture '") + token_name + "' is not quadratic");
			if(surface->iTexX > 1 || surface->iTexY > 1)
				ctx.Error(StdCopyStrBuf("Texture '") + token_name + "' is too large");

			Texture = new TexRef(surface.release());
		}
		else if(token_name == "tex_address_mode")
		{
			TexAddressMode = ctx.AdvanceEnum(TexAddressModeEnumerators);
		}
		else if(token_name == "tex_border_colour")
		{
			ctx.AdvanceColor(true, TexBorderColor);
		}
		else if(token_name == "filtering")
		{
			ctx.AdvanceEnums<3, StdMeshMaterialTextureUnit::FilteringType>(FilteringEnumerators, FilteringShortcuts, Filtering);
			if(Filtering[0] == F_None || Filtering[1] == F_None)
				ctx.Error(StdCopyStrBuf("'none' is only valid for the mip filter"));
			if(Filtering[2] == F_Anisotropic)
				ctx.Error(StdCopyStrBuf("'anisotropic' is not a valid mip filter"));
		}
		else if(token_name == "colour_op")
		{
			BlendOpType ColorOp = ctx.AdvanceEnum(BlendOpEnumerators);
			switch(ColorOp)
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
		else if(token_name == "colour_op_ex")
		{
			ColorOpEx = ctx.AdvanceEnum(BlendOpExEnumerators);
			ColorOpSources[0] = ctx.AdvanceEnum(BlendOpSourceEnumerators);
			ColorOpSources[1] = ctx.AdvanceEnum(BlendOpSourceEnumerators);
			if(ColorOpEx == BOX_BlendManual) ColorOpManualFactor = ctx.AdvanceFloat();
			if(ColorOpSources[0] == BOS_Manual) ctx.AdvanceColor(false, ColorOpManualColor1);
			if(ColorOpSources[1] == BOS_Manual) ctx.AdvanceColor(false, ColorOpManualColor2);
		}
		else if(token_name == "alpha_op_ex")
		{
			AlphaOpEx = ctx.AdvanceEnum(BlendOpExEnumerators);
			AlphaOpSources[0] = ctx.AdvanceEnum(BlendOpSourceEnumerators);
			AlphaOpSources[1] = ctx.AdvanceEnum(BlendOpSourceEnumerators);
			if(AlphaOpEx == BOX_BlendManual) AlphaOpManualFactor = ctx.AdvanceFloat();
			if(AlphaOpSources[0] == BOS_Manual) AlphaOpManualAlpha1 = ctx.AdvanceFloat();
			if(AlphaOpSources[1] == BOS_Manual) AlphaOpManualAlpha2 = ctx.AdvanceFloat();
		}
		else
			ctx.ErrorUnexpectedIdentifier(token_name);
	}

	if(token != TOKEN_BRACE_CLOSE)
		ctx.Error(StdCopyStrBuf("'") + token_name.getData() + "' unexpected");
}

StdMeshMaterialPass::StdMeshMaterialPass():
	DepthWrite(true)
{
	Ambient[0]	= Ambient[1]	= Ambient[2]	= 1.0f; Ambient[3]	= 1.0f;
	Diffuse[0]	= Diffuse[1]	= Diffuse[2]	= 1.0f; Diffuse[3]	= 1.0f;
	Specular[0] = Specular[1] = Specular[2] = 0.0f; Specular[3] = 0.0f;
	Emissive[0] = Emissive[1] = Emissive[2] = 0.0f; Emissive[3] = 0.0f;
	Shininess = 0.0f;
}

void StdMeshMaterialPass::Load(StdMeshMaterialParserCtx& ctx)
{
	Token token;
	StdCopyStrBuf token_name;
	while((token = ctx.AdvanceNonEOF(token_name)) == TOKEN_IDTF)
	{
		if(token_name == "texture_unit")
		{
			// TODO: Can there be an optional name?
			ctx.AdvanceRequired(token_name, TOKEN_BRACE_OPEN);
			TextureUnits.push_back(StdMeshMaterialTextureUnit());
			TextureUnits.back().Load(ctx);
		}
		else if(token_name == "ambient")
		{
			ctx.AdvanceColor(true, Ambient);
		}
		else if(token_name == "diffuse")
		{
			ctx.AdvanceColor(true, Diffuse);
		}
		else if(token_name == "specular")
		{
			Specular[0] = ctx.AdvanceFloat();
			Specular[1] = ctx.AdvanceFloat();
			Specular[2] = ctx.AdvanceFloat();

			// The fourth argument is optional, not the fifth:
			float specular3 = ctx.AdvanceFloat();

			float shininess;
			if(ctx.AdvanceFloatOptional(shininess))
			{
				Specular[3] = specular3;
				Shininess = shininess;
			}
			else
			{
				Shininess = specular3;
			}
		}
		else if(token_name == "emissive")
		{
			ctx.AdvanceColor(true, Emissive);
		}
		else if(token_name == "depth_write")
		{
			DepthWrite = ctx.AdvanceBoolean();
		}
		else
			ctx.ErrorUnexpectedIdentifier(token_name);
	}

	if(token != TOKEN_BRACE_CLOSE)
		ctx.Error(StdCopyStrBuf("'") + token_name.getData() + "' unexpected");
}

StdMeshMaterialTechnique::StdMeshMaterialTechnique():
	Available(false)
{
}

void StdMeshMaterialTechnique::Load(StdMeshMaterialParserCtx& ctx)
{
	Token token;
	StdCopyStrBuf token_name;
	while((token = ctx.AdvanceNonEOF(token_name)) == TOKEN_IDTF)
	{
		if(token_name == "pass")
		{
			// TODO: Can there be an optional name?
			ctx.AdvanceRequired(token_name, TOKEN_BRACE_OPEN);
			Passes.push_back(StdMeshMaterialPass());
			Passes.back().Load(ctx);
		}
		else
			ctx.ErrorUnexpectedIdentifier(token_name);
	}

	if(token != TOKEN_BRACE_CLOSE)
		ctx.Error(StdCopyStrBuf("'") + token_name.getData() + "' unexpected");
}

StdMeshMaterial::StdMeshMaterial():
	Line(0), ReceiveShadows(true), BestTechniqueIndex(-1)
{
}

void StdMeshMaterial::Load(StdMeshMaterialParserCtx& ctx)
{
	Token token;
	StdCopyStrBuf token_name;
	while((token = ctx.AdvanceNonEOF(token_name)) == TOKEN_IDTF)
	{
		if(token_name == "technique")
		{
			// TODO: Can there be an optional name?
			ctx.AdvanceRequired(token_name, TOKEN_BRACE_OPEN);
			Techniques.push_back(StdMeshMaterialTechnique());
			Techniques.back().Load(ctx);
		}
		else if(token_name == "receive_shadows")
		{
			ReceiveShadows = ctx.AdvanceBoolean();
		}
		else
			ctx.ErrorUnexpectedIdentifier(token_name);
	}
	
	if(token != TOKEN_BRACE_CLOSE)
		ctx.Error(StdCopyStrBuf("'") + token_name.getData() + "' unexpected");
}

void StdMeshMatManager::Clear()
{
	Materials.clear();
}

void StdMeshMatManager::Parse(const char* mat_script, const char* filename, StdMeshMaterialTextureLoader& tex_loader)
{
	StdMeshMaterialParserCtx ctx(mat_script, filename, tex_loader);

	Token token;
	StdCopyStrBuf token_name;
	while((token = ctx.Advance(token_name)) == TOKEN_IDTF)
	{
		if(token_name == "material")
		{
			// Read name
			StdCopyStrBuf material_name;
			ctx.AdvanceRequired(material_name, TOKEN_IDTF);

			// Check for uniqueness
			std::map<StdCopyStrBuf, StdMeshMaterial>::iterator iter = Materials.find(material_name);
			if(iter != Materials.end())
				ctx.Error(FormatString("Material with name '%s' is already defined in %s:%u", material_name.getData(), iter->second.FileName.getData(), iter->second.Line));

			// Check if there is a parent given
			Token next = ctx.AdvanceRequired(token_name, TOKEN_BRACE_OPEN, TOKEN_COLON);
			// Read parent name, if any
			StdMeshMaterial* parent = NULL;
			if(next == TOKEN_COLON)
			{
				// Note that if there is a parent, then it needs to be loaded
				// already. This currently makes only sense when its defined above
				// in the same material script file or in a parent definition.
				// We could later support material scripts in the System.c4g.
				StdCopyStrBuf parent_name;
				ctx.AdvanceRequired(parent_name, TOKEN_IDTF);
				ctx.AdvanceRequired(token_name, TOKEN_BRACE_OPEN);

				iter = Materials.find(parent_name);
				if(iter == Materials.end())
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
			
			// To Gfxspecific setup of the material; choose working techniques
			if(lpDDraw->PrepareMaterial(mat) && mat.BestTechniqueIndex != -1)
				Materials[material_name] = mat;
			else
				ctx.Error(StdCopyStrBuf("No working technique for material '") + material_name + "'");
		}
		else
			ctx.ErrorUnexpectedIdentifier(token_name);
	}

	if(token != TOKEN_EOF)
		ctx.Error(StdCopyStrBuf("'") + token_name.getData() + "' unexpected");
}

const StdMeshMaterial* StdMeshMatManager::GetMaterial(const char* material_name) const
{
	std::map<StdCopyStrBuf, StdMeshMaterial>::const_iterator iter = Materials.find(StdCopyStrBuf(material_name));
	if(iter == Materials.end()) return NULL;
	return &iter->second;
}
