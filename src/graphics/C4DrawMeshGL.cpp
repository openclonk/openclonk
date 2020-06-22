/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2013-2016, The OpenClonk Team and contributors
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

/* OpenGL implementation of Mesh Rendering */

#include "C4Include.h"
#include "C4ForbidLibraryCompilation.h"

#include "graphics/C4DrawGL.h"
#include "graphics/C4GraphicsResource.h"
#include "landscape/fow/C4FoWRegion.h"
#include "lib/SHA1.h"
#include "lib/StdMesh.h"
#include "object/C4Object.h"
#include "object/C4MeshDenumerator.h"

#include <clocale>

#ifndef USE_CONSOLE

namespace
{
	template<int category>
	class ScopedLocale
	{
		// We need to make a copy of the return value of setlocale, because
		// it's using TLS
		std::string saved_loc;
	public:
		explicit ScopedLocale(const char *locale)
		{
			const char *old_loc = setlocale(category, locale);
			if (old_loc == nullptr)
				throw std::invalid_argument("Argument to setlocale was invalid");
			saved_loc = old_loc;
		}
		~ScopedLocale()
		{
			setlocale(category, saved_loc.c_str());
		}
	};

	////////////////////////////////////////////
	// Shader code generation
	// This translates the fixed function instructions in a material script
	// to an equivalent fragment shader. The generated code can certainly
	// be optimized more.
	////////////////////////////////////////////
	StdStrBuf Texture2DToCode(int index, bool hasTextureAnimation)
	{
		if (hasTextureAnimation) return FormatString("texture(oc_Texture%d, (oc_TextureMatrix%d * vec4(texcoord, 0.0, 1.0)).xy)", index, index);
		return FormatString("texture(oc_Texture%d, texcoord)", index);
	}

	StdStrBuf TextureUnitSourceToCode(int index, StdMeshMaterialTextureUnit::BlendOpSourceType source, const float manualColor[3], float manualAlpha, bool hasTextureAnimation)
	{
		switch(source)
		{
		case StdMeshMaterialTextureUnit::BOS_Current: return StdStrBuf("currentColor");
		case StdMeshMaterialTextureUnit::BOS_Texture: return Texture2DToCode(index, hasTextureAnimation);
		case StdMeshMaterialTextureUnit::BOS_Diffuse: return StdStrBuf("diffuse");
		case StdMeshMaterialTextureUnit::BOS_Specular: return StdStrBuf("diffuse"); // TODO: Should be specular
		case StdMeshMaterialTextureUnit::BOS_PlayerColor: return StdStrBuf("vec4(oc_PlayerColor, 1.0)");
		case StdMeshMaterialTextureUnit::BOS_Manual: return FormatString("vec4(%f, %f, %f, %f)", manualColor[0], manualColor[1], manualColor[2], manualAlpha);
		default: assert(false); return StdStrBuf("vec4(0.0, 0.0, 0.0, 0.0)");
		}
	}

	StdStrBuf TextureUnitBlendToCode(int index, StdMeshMaterialTextureUnit::BlendOpExType blend_type, const char* source1, const char* source2, float manualFactor, bool hasTextureAnimation)
	{
		switch(blend_type)
		{
		case StdMeshMaterialTextureUnit::BOX_Source1: return StdStrBuf(source1);
		case StdMeshMaterialTextureUnit::BOX_Source2: return StdStrBuf(source2);
		case StdMeshMaterialTextureUnit::BOX_Modulate: return FormatString("%s * %s", source1, source2);
		case StdMeshMaterialTextureUnit::BOX_ModulateX2: return FormatString("2.0 * %s * %s", source1, source2);
		case StdMeshMaterialTextureUnit::BOX_ModulateX4: return FormatString("4.0 * %s * %s", source1, source2);
		case StdMeshMaterialTextureUnit::BOX_Add: return FormatString("%s + %s", source1, source2);
		case StdMeshMaterialTextureUnit::BOX_AddSigned: return FormatString("%s + %s - 0.5", source1, source2);
		case StdMeshMaterialTextureUnit::BOX_AddSmooth: return FormatString("%s + %s - %s*%s", source1, source2, source1, source2);
		case StdMeshMaterialTextureUnit::BOX_Subtract: return FormatString("%s - %s", source1, source2);
		case StdMeshMaterialTextureUnit::BOX_BlendDiffuseAlpha: return FormatString("diffuse.a * %s + (1.0 - diffuse.a) * %s", source1, source2);
		case StdMeshMaterialTextureUnit::BOX_BlendTextureAlpha: return FormatString("%s.a * %s + (1.0 - %s.a) * %s", Texture2DToCode(index, hasTextureAnimation).getData(), source1, Texture2DToCode(index, hasTextureAnimation).getData(), source2);
		case StdMeshMaterialTextureUnit::BOX_BlendCurrentAlpha: return FormatString("currentColor.a * %s + (1.0 - currentColor.a) * %s", source1, source2);
		case StdMeshMaterialTextureUnit::BOX_BlendManual: return FormatString("%f * %s + (1.0 - %f) * %s", manualFactor, source1, manualFactor, source2);
		case StdMeshMaterialTextureUnit::BOX_Dotproduct: return FormatString("vec3(4.0 * dot(%s - 0.5, %s - 0.5), 4.0 * dot(%s - 0.5, %s - 0.5), 4.0 * dot(%s - 0.5, %s - 0.5))", source1, source2, source1, source2, source1, source2); // TODO: Needs special handling for the case of alpha
		case StdMeshMaterialTextureUnit::BOX_BlendDiffuseColor: return FormatString("diffuse.rgb * %s + (1.0 - diffuse.rgb) * %s", source1, source2);
		default: assert(false); return StdStrBuf(source1);
		}
	}

	StdStrBuf TextureUnitToCode(int index, const StdMeshMaterialTextureUnit& texunit)
	{
		ScopedLocale<LC_NUMERIC> scoped_c_locale("C");
		const bool hasTextureAnimation = texunit.HasTexCoordAnimation();

		StdStrBuf color_source1 = FormatString("%s.rgb", TextureUnitSourceToCode(index, texunit.ColorOpSources[0], texunit.ColorOpManualColor1, texunit.AlphaOpManualAlpha1, hasTextureAnimation).getData());
		StdStrBuf color_source2 = FormatString("%s.rgb", TextureUnitSourceToCode(index, texunit.ColorOpSources[1], texunit.ColorOpManualColor2, texunit.AlphaOpManualAlpha2, hasTextureAnimation).getData());
		StdStrBuf alpha_source1 = FormatString("%s.a", TextureUnitSourceToCode(index, texunit.AlphaOpSources[0], texunit.ColorOpManualColor1, texunit.AlphaOpManualAlpha1, hasTextureAnimation).getData());
		StdStrBuf alpha_source2 = FormatString("%s.a", TextureUnitSourceToCode(index, texunit.AlphaOpSources[1], texunit.ColorOpManualColor2, texunit.AlphaOpManualAlpha2, hasTextureAnimation).getData());

		return FormatString("currentColor = vec4(%s, %s);\n", TextureUnitBlendToCode(index, texunit.ColorOpEx, color_source1.getData(), color_source2.getData(), texunit.ColorOpManualFactor, hasTextureAnimation).getData(), TextureUnitBlendToCode(index, texunit.AlphaOpEx, alpha_source1.getData(), alpha_source2.getData(), texunit.AlphaOpManualFactor, hasTextureAnimation).getData());
	}

	StdStrBuf AlphaTestToCode(const StdMeshMaterialPass& pass)
	{
		ScopedLocale<LC_NUMERIC> scoped_c_locale("C");
		switch (pass.AlphaRejectionFunction)
		{
		case StdMeshMaterialPass::DF_AlwaysPass:
			return StdStrBuf("");
		case StdMeshMaterialPass::DF_AlwaysFail:
			return StdStrBuf("discard;");
		case StdMeshMaterialPass::DF_Less:
			return FormatString("if (!(fragColor.a < %f)) discard;", pass.AlphaRejectionValue);
		case StdMeshMaterialPass::DF_LessEqual:
			return FormatString("if (!(fragColor.a <= %f)) discard;", pass.AlphaRejectionValue);
		case StdMeshMaterialPass::DF_Equal:
			return FormatString("if (!(fragColor.a == %f)) discard;", pass.AlphaRejectionValue);
		case StdMeshMaterialPass::DF_NotEqual:
			return FormatString("if (!(fragColor.a != %f)) discard;", pass.AlphaRejectionValue);
		case StdMeshMaterialPass::DF_Greater:
			return FormatString("if (!(fragColor.a > %f)) discard;", pass.AlphaRejectionValue);
		case StdMeshMaterialPass::DF_GreaterEqual:
			return FormatString("if (!(fragColor.a >= %f)) discard;", pass.AlphaRejectionValue);
		default:
			assert(false);
			return StdStrBuf();
		}
	}

	// Simple helper function
	inline GLenum OgreBlendTypeToGL(StdMeshMaterialPass::SceneBlendType blend)
	{
		switch(blend)
		{
		case StdMeshMaterialPass::SB_One: return GL_ONE;
		case StdMeshMaterialPass::SB_Zero: return GL_ZERO;
		case StdMeshMaterialPass::SB_DestColor: return GL_DST_COLOR;
		case StdMeshMaterialPass::SB_SrcColor: return GL_SRC_COLOR;
		case StdMeshMaterialPass::SB_OneMinusDestColor: return GL_ONE_MINUS_DST_COLOR;
		case StdMeshMaterialPass::SB_OneMinusSrcColor: return GL_ONE_MINUS_SRC_COLOR;
		case StdMeshMaterialPass::SB_DestAlpha: return GL_DST_ALPHA;
		case StdMeshMaterialPass::SB_SrcAlpha: return GL_SRC_ALPHA;
		case StdMeshMaterialPass::SB_OneMinusDestAlpha: return GL_ONE_MINUS_DST_ALPHA;
		case StdMeshMaterialPass::SB_OneMinusSrcAlpha: return GL_ONE_MINUS_SRC_ALPHA;
		default: assert(false); return GL_ZERO;
		}
	}

	StdStrBuf GetVertexShaderCodeForPass(const StdMeshMaterialPass& pass, bool LowMaxVertexUniformCount)
	{
		StdStrBuf buf;

		if (!::GraphicsResource.Files.LoadEntryString("MeshVertexShader.glsl", &buf))
		{
			// Fall back just in case
			buf.Copy(
				"attribute vec3 oc_Position;\n"
				"attribute vec3 oc_Normal;\n"
				"attribute vec2 oc_TexCoord;\n"
				"varying vec3 vtxNormal;\n"
				"varying vec2 texcoord;\n"
				"uniform mat4 projectionMatrix;\n"
				"uniform mat4 modelviewMatrix;\n"
				"uniform mat3 normalMatrix;\n"
				"\n"
				"slice(position)\n"
				"{\n"
				"  gl_Position = projectionMatrix * modelviewMatrix * vec4(oc_Position, 1.0);\n"
				"}\n"
				"\n"
				"slice(texcoord)\n"
				"{\n"
				"  texcoord = oc_TexCoord;\n"
				"}\n"
				"\n"
				"slice(normal)\n"
				"{\n"
				"  vtxNormal = normalize(normalMatrix * oc_Normal);\n"
				"}\n"
			);
		}

		if (pGL->Workarounds.ForceSoftwareTransform)
			buf.Take(StdStrBuf("#define OC_WA_FORCE_SOFTWARE_TRANSFORM\n") + buf);

		if (LowMaxVertexUniformCount)
			return StdStrBuf("#define OC_WA_LOW_MAX_VERTEX_UNIFORM_COMPONENTS\n") + buf;
		else
			return buf;
	}

	// Note this only gets the code which inserts the slices specific for the pass
	// -- other slices are independent from this!
	StdStrBuf GetFragmentShaderCodeForPass(const StdMeshMaterialPass& pass, StdMeshMaterialShaderParameters& params)
	{
		StdStrBuf buf;

		// Produce the fragment shader... first we create one code fragment for each
		// texture unit, and we count the number of active textures, i.e. texture
		// units that actually use a texture.
		unsigned int texIndex = 0;
		StdStrBuf textureUnitCode(""), textureUnitDeclCode("");
		for(const auto & texunit : pass.TextureUnits)
		{
			textureUnitCode.Append(TextureUnitToCode(texIndex, texunit));

			if(texunit.HasTexture())
			{
				textureUnitDeclCode.Append(FormatString("uniform sampler2D oc_Texture%u;\n", texIndex).getData());
				params.AddParameter(FormatString("oc_Texture%u", texIndex).getData(), StdMeshMaterialShaderParameter::INT).GetInt() = texIndex;

				// If the texture unit has texture coordinate transformations,
				// add a corresponding texture matrix uniform.
				if(texunit.HasTexCoordAnimation())
				{
					textureUnitDeclCode.Append(FormatString("uniform mat4 oc_TextureMatrix%u;\n", texIndex).getData());
					params.AddParameter(FormatString("oc_TextureMatrix%u", texIndex).getData(), StdMeshMaterialShaderParameter::AUTO_TEXTURE_MATRIX).GetInt() = texIndex;
				}

				++texIndex;
			}
		}

		return FormatString(
			"%s\n" // Texture units with active textures, only if >0 texture units
			"uniform vec3 oc_PlayerColor;\n" // This needs to be in-sync with the naming in StdMeshMaterialProgram::CompileShader()
			"\n"
			"slice(texture)\n"
			"{\n"
			"  vec4 diffuse = fragColor;\n"
			"  vec4 currentColor = diffuse;\n"
			"  %s\n"
			"  fragColor = currentColor;\n"
			"}\n"
			"\n"
			"slice(finish)\n"
			"{\n"
			"  %s\n"
			"}\n",
			textureUnitDeclCode.getData(),
			textureUnitCode.getData(),
			AlphaTestToCode(pass).getData()
		);
	}

	StdStrBuf GetSHA1HexDigest(const char* text, std::size_t len)
	{
		sha1 ctx;
		ctx.process_bytes(text, len);
		unsigned int digest[5];
		ctx.get_digest(digest);

		return FormatString("%08x%08x%08x%08x%08x", digest[0], digest[1], digest[2], digest[3], digest[4]);
	}
} // anonymous namespace

bool CStdGL::PrepareMaterial(StdMeshMatManager& mat_manager, StdMeshMaterialLoader& loader, StdMeshMaterial& mat)
{
	// TODO: If a technique is not available, show an error message what the problem is

	// select context, if not already done
	if (!pCurrCtx) return false;

	for (unsigned int i = 0; i < mat.Techniques.size(); ++i)
	{
		StdMeshMaterialTechnique& technique = mat.Techniques[i];
		technique.Available = true;
		for (unsigned int j = 0; j < technique.Passes.size(); ++j)
		{
			StdMeshMaterialPass& pass = technique.Passes[j];

			GLint max_texture_units;
			glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_texture_units);

			// OpenGL 3.x guarantees at least 16 TIUs. If the above returns
			// less it's probably a driver bug. So just keep going.
			assert(max_texture_units >= 16);
			max_texture_units = std::min<GLint>(max_texture_units, 16);

			unsigned int active_texture_units = 0;
			for(auto & TextureUnit : pass.TextureUnits)
				if(TextureUnit.HasTexture())
					++active_texture_units;

			if (active_texture_units > static_cast<unsigned int>(max_texture_units))
				technique.Available = false;

			for (auto & texunit : pass.TextureUnits)
			{
				for (unsigned int l = 0; l < texunit.GetNumTextures(); ++l)
				{
					const C4TexRef& texture = texunit.GetTexture(l);
					glBindTexture(GL_TEXTURE_2D, texture.texName);
					switch (texunit.TexAddressMode)
					{
					case StdMeshMaterialTextureUnit::AM_Wrap:
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
						break;
					case StdMeshMaterialTextureUnit::AM_Border:
						glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, texunit.TexBorderColor);
						// fallthrough
					case StdMeshMaterialTextureUnit::AM_Clamp:
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
						break;
					case StdMeshMaterialTextureUnit::AM_Mirror:
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
						break;
					}

					switch (texunit.Filtering[0]) // min
					{
					case StdMeshMaterialTextureUnit::F_None:
						technique.Available = false;
						break;
					case StdMeshMaterialTextureUnit::F_Point:
						switch (texunit.Filtering[2]) // mip
						{
						case StdMeshMaterialTextureUnit::F_None:
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
							break;
						case StdMeshMaterialTextureUnit::F_Point:
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
							break;
						case StdMeshMaterialTextureUnit::F_Linear:
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
							break;
						case StdMeshMaterialTextureUnit::F_Anisotropic:
							technique.Available = false; // invalid
							break;
						}
						break;
					case StdMeshMaterialTextureUnit::F_Linear:
						switch (texunit.Filtering[2]) // mip
						{
						case StdMeshMaterialTextureUnit::F_None:
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
							break;
						case StdMeshMaterialTextureUnit::F_Point:
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
							break;
						case StdMeshMaterialTextureUnit::F_Linear:
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
							break;
						case StdMeshMaterialTextureUnit::F_Anisotropic:
							technique.Available = false; // invalid
							break;
						}
						break;
					case StdMeshMaterialTextureUnit::F_Anisotropic:
						// unsupported
						technique.Available = false;
						break;
					}

					switch (texunit.Filtering[1]) // max
					{
					case StdMeshMaterialTextureUnit::F_None:
						technique.Available = false; // invalid
						break;
					case StdMeshMaterialTextureUnit::F_Point:
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
						break;
					case StdMeshMaterialTextureUnit::F_Linear:
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
						break;
					case StdMeshMaterialTextureUnit::F_Anisotropic:
						// unsupported
						technique.Available = false;
						break;
					}
				} // loop over textures
			} // loop over texture units

			// Create fragment and/or vertex shader
			// if a custom shader is not provided.
			// Re-use existing programs if the generated
			// code is the same (determined by SHA1 hash).
			bool custom_shader = true;
			if(!pass.VertexShader.Shader)
			{
				StdStrBuf buf = GetVertexShaderCodeForPass(pass, Workarounds.LowMaxVertexUniformCount);
				StdStrBuf hash = GetSHA1HexDigest(buf.getData(), buf.getLength());
				pass.VertexShader.Shader = mat_manager.AddShader("auto-generated vertex shader", hash.getData(), "glsl", SMMS_VERTEX, buf.getData(), StdMeshMatManager::SMM_AcceptExisting);
				custom_shader = false;
			}

			if(!pass.FragmentShader.Shader)
			{
				// TODO: Should use shared_params once we introduce them
				StdStrBuf buf = GetFragmentShaderCodeForPass(pass, pass.FragmentShader.Parameters);
				StdStrBuf hash = GetSHA1HexDigest(buf.getData(), buf.getLength());
				pass.FragmentShader.Shader = mat_manager.AddShader("auto-generated fragment shader", hash.getData(), "glsl", SMMS_FRAGMENT, buf.getData(), StdMeshMatManager::SMM_AcceptExisting);
			}

			// Then, link the program, and resolve parameter locations
			StdStrBuf name(FormatString("%s:%s:%s", mat.Name.getData(), technique.Name.getData(), pass.Name.getData()));
			const StdMeshMaterialProgram* added_program = mat_manager.AddProgram(name.getData(), loader, pass.FragmentShader, pass.VertexShader, pass.GeometryShader);
			if(!added_program)
			{
				// If the program could not be compiled, try again with the LowMaxVertexUniformCount workaround.
				// See bug #1368.
				if (!custom_shader && !Workarounds.LowMaxVertexUniformCount)
				{
					StdStrBuf buf = GetVertexShaderCodeForPass(pass, true);
					StdStrBuf hash = GetSHA1HexDigest(buf.getData(), buf.getLength());
					pass.VertexShader.Shader = mat_manager.AddShader("auto-generated vertex shader", hash.getData(), "glsl", SMMS_VERTEX, buf.getData(), StdMeshMatManager::SMM_AcceptExisting);

					added_program = mat_manager.AddProgram(name.getData(), loader, pass.FragmentShader, pass.VertexShader, pass.GeometryShader);
					if(added_program)
					{
						// If this actually work, cache the result, so we don't
						// need to fail again next time before trying the workaround.
						Workarounds.LowMaxVertexUniformCount = true;
						Log("  gl: Enabling low max vertex uniform workaround");
					}
				}
			}

			if (!added_program)
			{
				technique.Available = false;
			}
			else
			{
				std::unique_ptr<StdMeshMaterialPass::ProgramInstance> program_instance(new StdMeshMaterialPass::ProgramInstance(added_program, &pass.FragmentShader, &pass.VertexShader, &pass.GeometryShader));
				pass.Program = std::move(program_instance);
			}
		}

		if (technique.Available && mat.BestTechniqueIndex == -1)
			mat.BestTechniqueIndex = i;
	}

	return mat.BestTechniqueIndex != -1;
}

// TODO: We should add a class, C4MeshRenderer, which contains all the functions
// in this namespace, and avoids passing around so many parameters.
namespace
{
	// Apply Zoom and Transformation to the current matrix stack. Return
	// parity of the transformation.
	bool ApplyZoomAndTransform(float ZoomX, float ZoomY, float Zoom, C4BltTransform* pTransform, StdProjectionMatrix& projection)
	{
		// Apply zoom
		Translate(projection, ZoomX, ZoomY, 0.0f);
		Scale(projection, Zoom, Zoom, 1.0f);
		Translate(projection, -ZoomX, -ZoomY, 0.0f);

		// Apply transformation
		if (pTransform)
		{
			StdProjectionMatrix transform;
			transform(0, 0) = pTransform->mat[0];
			transform(0, 1) = pTransform->mat[1];
			transform(0, 2) = 0.0f;
			transform(0, 3) = pTransform->mat[2];
			transform(1, 0) = pTransform->mat[3];
			transform(1, 1) = pTransform->mat[4];
			transform(1, 2) = 0.0f;
			transform(1, 3) = pTransform->mat[5];
			transform(2, 0) = 0.0f;
			transform(2, 1) = 0.0f;
			transform(2, 2) = 1.0f;
			transform(2, 3) = 0.0f;
			transform(3, 0) = pTransform->mat[6];
			transform(3, 1) = pTransform->mat[7];
			transform(3, 2) = 0.0f;
			transform(3, 3) = pTransform->mat[8];
			projection *= transform;

			// Compute parity of the transformation matrix - if parity is swapped then
			// we need to cull front faces instead of back faces.
			const float det = transform(0,0)*transform(1,1)*transform(3,3)
			                + transform(1,0)*transform(3,1)*transform(0,3)
			                + transform(3,0)*transform(0,1)*transform(1,3)
			                - transform(0,0)*transform(3,1)*transform(1,3)
			                - transform(1,0)*transform(0,1)*transform(3,3)
			                - transform(3,0)*transform(1,1)*transform(0,3);

			return det > 0;
		}

		return true;
	}

	void SetStandardUniforms(C4ShaderCall& call, DWORD dwModClr, DWORD dwPlayerColor, DWORD dwBlitMode, bool cullFace, const C4FoWRegion* pFoW, const C4Rect& clipRect, const C4Rect& outRect)
	{
		// Draw transform
		const float fMod[4] = {
			((dwModClr >> 16) & 0xff) / 255.0f,
			((dwModClr >>  8) & 0xff) / 255.0f,
			((dwModClr      ) & 0xff) / 255.0f,
			((dwModClr >> 24) & 0xff) / 255.0f
		};
		call.SetUniform4fv(C4SSU_ClrMod, 1, fMod);
		call.SetUniform3fv(C4SSU_Gamma, 1, pDraw->gammaOut);
		call.SetUniform2f(C4SSU_Resolution, outRect.Wdt, outRect.Hgt);

		// Player color
		const float fPlrClr[3] = {
			((dwPlayerColor >> 16) & 0xff) / 255.0f,
			((dwPlayerColor >>  8) & 0xff) / 255.0f,
			((dwPlayerColor      ) & 0xff) / 255.0f,
		};
		call.SetUniform3fv(C4SSU_OverlayClr, 1, fPlrClr);

		// Backface culling flag
		call.SetUniform1f(C4SSU_CullMode, cullFace ? 0.0f : 1.0f);

		// Dynamic light
		if(pFoW != nullptr)
		{
			call.AllocTexUnit(C4SSU_LightTex);
			glBindTexture(GL_TEXTURE_2D, pFoW->getSurfaceName());
			float lightTransform[6];
			pFoW->GetFragTransform(clipRect, outRect, lightTransform);
			call.SetUniformMatrix2x3fv(C4SSU_LightTransform, 1, lightTransform);

			call.AllocTexUnit(C4SSU_AmbientTex);
			glBindTexture(GL_TEXTURE_2D, pFoW->getFoW()->Ambient.Tex);
			call.SetUniform1f(C4SSU_AmbientBrightness, pFoW->getFoW()->Ambient.GetBrightness());
			float ambientTransform[6];
			pFoW->getFoW()->Ambient.GetFragTransform(pFoW->getViewportRegion(), clipRect, outRect, ambientTransform);
			call.SetUniformMatrix2x3fv(C4SSU_AmbientTransform, 1, ambientTransform);
		}

		// Current frame counter
		call.SetUniform1i(C4SSU_FrameCounter, ::Game.FrameCounter);
	}

	bool ResolveAutoParameter(C4ShaderCall& call, StdMeshMaterialShaderParameter& parameter, StdMeshMaterialShaderParameter::Auto value, DWORD dwModClr, DWORD dwPlayerColor, DWORD dwBlitMode, const C4FoWRegion* pFoW, const C4Rect& clipRect)
	{
		// There are no auto parameters implemented yet
		assert(false);
		return false;
	}

	StdProjectionMatrix ResolveAutoTextureMatrix(const StdSubMeshInstance& instance, const StdMeshMaterialTechnique& technique, unsigned int passIndex, unsigned int texUnitIndex)
	{
		assert(passIndex < technique.Passes.size());
		const StdMeshMaterialPass& pass = technique.Passes[passIndex];

		assert(texUnitIndex < pass.TextureUnits.size());
		const StdMeshMaterialTextureUnit& texunit = pass.TextureUnits[texUnitIndex];

		StdProjectionMatrix matrix = StdProjectionMatrix::Identity();
		const double Position = instance.GetTexturePosition(passIndex, texUnitIndex);

		for (const auto & trans : texunit.Transformations)
		{
			StdProjectionMatrix temp_matrix;
			switch (trans.TransformType)
			{
			case StdMeshMaterialTextureUnit::Transformation::T_SCROLL:
				Translate(matrix, trans.Scroll.X, trans.Scroll.Y, 0.0f);
				break;
			case StdMeshMaterialTextureUnit::Transformation::T_SCROLL_ANIM:
				Translate(matrix, trans.GetScrollX(Position), trans.GetScrollY(Position), 0.0f);
				break;
			case StdMeshMaterialTextureUnit::Transformation::T_ROTATE:
				Rotate(matrix, trans.Rotate.Angle, 0.0f, 0.0f, 1.0f);
				break;
			case StdMeshMaterialTextureUnit::Transformation::T_ROTATE_ANIM:
				Rotate(matrix, trans.GetRotate(Position), 0.0f, 0.0f, 1.0f);
				break;
			case StdMeshMaterialTextureUnit::Transformation::T_SCALE:
				Scale(matrix, trans.Scale.X, trans.Scale.Y, 1.0f);
				break;
			case StdMeshMaterialTextureUnit::Transformation::T_TRANSFORM:
				for (int i = 0; i < 16; ++i)
					temp_matrix(i / 4, i % 4) = trans.Transform.M[i];
				matrix *= temp_matrix;
				break;
			case StdMeshMaterialTextureUnit::Transformation::T_WAVE_XFORM:
				switch (trans.WaveXForm.XForm)
				{
				case StdMeshMaterialTextureUnit::Transformation::XF_SCROLL_X:
					Translate(matrix, trans.GetWaveXForm(Position), 0.0f, 0.0f);
					break;
				case StdMeshMaterialTextureUnit::Transformation::XF_SCROLL_Y:
					Translate(matrix, 0.0f, trans.GetWaveXForm(Position), 0.0f);
					break;
				case StdMeshMaterialTextureUnit::Transformation::XF_ROTATE:
					Rotate(matrix, trans.GetWaveXForm(Position), 0.0f, 0.0f, 1.0f);
					break;
				case StdMeshMaterialTextureUnit::Transformation::XF_SCALE_X:
					Scale(matrix, trans.GetWaveXForm(Position), 1.0f, 1.0f);
					break;
				case StdMeshMaterialTextureUnit::Transformation::XF_SCALE_Y:
					Scale(matrix, 1.0f, trans.GetWaveXForm(Position), 1.0f);
					break;
				}
				break;
			}
		}

		return matrix;
	}

	struct BoneTransform
	{
		float m[3][4];
	};

	std::vector<BoneTransform> CookBoneTransforms(const StdMeshInstance& mesh_instance)
	{
		// Cook the bone transform matrixes into something that OpenGL can use. This could be moved into RenderMeshImpl.
		// Or, even better, we could upload them into a UBO, but Intel doesn't support them prior to Sandy Bridge.

		std::vector<BoneTransform> bones;
		if (mesh_instance.GetBoneCount() == 0)
		{
#pragma clang diagnostic ignored "-Wmissing-braces" 
			// Upload dummy bone so we don't have to do branching in the vertex shader
			static const BoneTransform dummy_bone = {
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f
			};
			bones.push_back(dummy_bone);
		}
		else
		{
			bones.reserve(mesh_instance.GetBoneCount());
			for (size_t bone_index = 0; bone_index < mesh_instance.GetBoneCount(); ++bone_index)
			{
				const StdMeshMatrix &bone = mesh_instance.GetBoneTransform(bone_index);
				BoneTransform cooked_bone = {
					bone(0, 0), bone(0, 1), bone(0, 2), bone(0, 3),
					bone(1, 0), bone(1, 1), bone(1, 2), bone(1, 3),
					bone(2, 0), bone(2, 1), bone(2, 2), bone(2, 3)
				};
				bones.push_back(cooked_bone);
			}
		}
		return bones;
	}

	struct PretransformedMeshVertex
	{
		float nx, ny, nz;
		float x, y, z;
	};

	void PretransformMeshVertex(PretransformedMeshVertex *out, const StdMeshVertex &in, const StdMeshInstance &mesh_instance)
	{
		// If the first bone assignment has a weight of 0, all others are zero
		// as well, or the loader would have overwritten the assignment
		if (in.bone_weight[0] == 0.0f || mesh_instance.GetBoneCount() == 0)
		{
			out->x = in.x;
			out->y = in.y;
			out->z = in.z;
			out->nx = in.nx;
			out->ny = in.ny;
			out->nz = in.nz;
		}
		else
		{
			PretransformedMeshVertex vtx{ 0, 0, 0, 0, 0, 0 };
			for (size_t i = 0; i < StdMeshVertex::MaxBoneWeightCount && in.bone_weight[i] > 0; ++i)
			{
				float weight = in.bone_weight[i];
				const auto &bone = mesh_instance.GetBoneTransform(in.bone_index[i]);
				auto vertex = weight * (bone * in);
				vtx.nx += vertex.nx;
				vtx.ny += vertex.ny;
				vtx.nz += vertex.nz;
				vtx.x += vertex.x;
				vtx.y += vertex.y;
				vtx.z += vertex.z;
			}
			*out = vtx;
		}
	}

	void PretransformMeshVertices(const StdMeshInstance &mesh_instance, const StdSubMeshInstance& instance, GLuint vbo)
	{
		assert(pGL->Workarounds.ForceSoftwareTransform);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		const auto &original_vertices = mesh_instance.GetSharedVertices().empty() ? instance.GetSubMesh().GetVertices() : mesh_instance.GetSharedVertices();
		const size_t vertex_count = original_vertices.size();

		// Unmapping the buffer may fail for certain reasons, in which case we need to try again.
		do
		{
			glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(PretransformedMeshVertex), nullptr, GL_STREAM_DRAW);
			void *map = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
			PretransformedMeshVertex *buffer = new (map) PretransformedMeshVertex[vertex_count];

			for (size_t i = 0; i < vertex_count; ++i)
			{
				PretransformMeshVertex(&buffer[i], original_vertices[i], mesh_instance);
			}
		} while (glUnmapBuffer(GL_ARRAY_BUFFER) == GL_FALSE);
		// Unbind the buffer so following rendering calls do not use it
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	void RenderSubMeshImpl(const StdProjectionMatrix& projectionMatrix, const StdMeshMatrix& modelviewMatrix, const StdMeshInstance& mesh_instance, const StdSubMeshInstance& instance, DWORD dwModClr, DWORD dwBlitMode, DWORD dwPlayerColor, const C4FoWRegion* pFoW, const C4Rect& clipRect, const C4Rect& outRect, bool parity)
	{
		// Don't render with degenerate matrix
		if (fabs(modelviewMatrix.Determinant()) < 1e-6)
			return;

		const StdMeshMaterial& material = instance.GetMaterial();
		assert(material.BestTechniqueIndex != -1);
		const StdMeshMaterialTechnique& technique = material.Techniques[material.BestTechniqueIndex];

		bool using_shared_vertices = instance.GetSubMesh().GetVertices().empty();
		GLuint vbo = mesh_instance.GetMesh().GetVBO();
		GLuint ibo = mesh_instance.GetIBO();
		unsigned int vaoid = mesh_instance.GetVAOID();
		size_t vertex_buffer_offset = using_shared_vertices ? 0 : instance.GetSubMesh().GetOffsetInVBO();
		size_t index_buffer_offset = instance.GetSubMesh().GetOffsetInIBO(); // note this is constant

		const bool ForceSoftwareTransform = pGL->Workarounds.ForceSoftwareTransform;
		GLuint pretransform_vbo;

		std::vector<BoneTransform> bones;
		if (!ForceSoftwareTransform)
		{
			bones = CookBoneTransforms(mesh_instance);
		}
		else
		{
			glGenBuffers(1, &pretransform_vbo);
			PretransformMeshVertices(mesh_instance, instance, pretransform_vbo);
		}
		// Modelview matrix does not change between passes, so cache it here
		const StdMeshMatrix normalMatrixTranspose = StdMeshMatrix::Inverse(modelviewMatrix);

		// Render each pass
		for (unsigned int i = 0; i < technique.Passes.size(); ++i)
		{
			const StdMeshMaterialPass& pass = technique.Passes[i];

			if (!pass.DepthCheck)
				glDisable(GL_DEPTH_TEST);

			glDepthMask(pass.DepthWrite ? GL_TRUE : GL_FALSE);

			if (pass.AlphaToCoverage)
				glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
			else
				glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);

			glFrontFace(parity ? GL_CW : GL_CCW);
			if (mesh_instance.GetCompletion() < 1.0f)
			{
				// Backfaces might be visible when completion is < 1.0f since front
				// faces might be omitted.
				glDisable(GL_CULL_FACE);
			}
			else
			{
				switch (pass.CullHardware)
				{
				case StdMeshMaterialPass::CH_Clockwise:
					glEnable(GL_CULL_FACE);
					glCullFace(GL_BACK);
					break;
				case StdMeshMaterialPass::CH_CounterClockwise:
					glEnable(GL_CULL_FACE);
					glCullFace(GL_FRONT);
					break;
				case StdMeshMaterialPass::CH_None:
					glDisable(GL_CULL_FACE);
					break;
				}
			}

			// Overwrite blend mode with default alpha blending when alpha in clrmod
			// is <255. This makes sure that normal non-blended meshes can have
			// blending disabled in their material script (which disables expensive
			// face ordering) but when they are made translucent via clrmod
			if (!(dwBlitMode & C4GFXBLIT_ADDITIVE))
			{
				if (((dwModClr >> 24) & 0xff) < 0xff) // && (!(dwBlitMode & C4GFXBLIT_MOD2)) )
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				else
					glBlendFunc(OgreBlendTypeToGL(pass.SceneBlendFactors[0]),
						OgreBlendTypeToGL(pass.SceneBlendFactors[1]));
			}
			else
			{
				if (((dwModClr >> 24) & 0xff) < 0xff) // && (!(dwBlitMode & C4GFXBLIT_MOD2)) )
					glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				else
					glBlendFunc(OgreBlendTypeToGL(pass.SceneBlendFactors[0]), GL_ONE);
			}

			assert(pass.Program.get() != nullptr);

			// Upload all parameters to the shader
			int ssc = 0;
			if (dwBlitMode & C4GFXBLIT_MOD2) ssc |= C4SSC_MOD2;
			if (pFoW != nullptr) ssc |= C4SSC_LIGHT;
			const C4Shader* shader = pass.Program->Program->GetShader(ssc);
			if (!shader) return;
			C4ShaderCall call(shader);
			call.Start();

			// Upload projection, modelview and normal matrices
			call.SetUniformMatrix4x4(C4SSU_ProjectionMatrix, projectionMatrix);
			call.SetUniformMatrix4x4(C4SSU_ModelViewMatrix, modelviewMatrix);
			call.SetUniformMatrix3x3Transpose(C4SSU_NormalMatrix, normalMatrixTranspose);


			// Upload material properties
			call.SetUniform4fv(C4SSU_MaterialAmbient, 1, pass.Ambient);
			call.SetUniform4fv(C4SSU_MaterialDiffuse, 1, pass.Diffuse);
			call.SetUniform4fv(C4SSU_MaterialSpecular, 1, pass.Specular);
			call.SetUniform4fv(C4SSU_MaterialEmission, 1, pass.Emissive);
			call.SetUniform1f(C4SSU_MaterialShininess, pass.Shininess);

			// Upload the current bone transformation matrixes (if there are any)
			if (!ForceSoftwareTransform)
			{
				if (!bones.empty())
				{
					if (pGL->Workarounds.LowMaxVertexUniformCount)
						glUniformMatrix3x4fv(shader->GetUniform(C4SSU_Bones), bones.size(), GL_FALSE, &bones[0].m[0][0]);
					else
						glUniformMatrix4x3fv(shader->GetUniform(C4SSU_Bones), bones.size(), GL_TRUE, &bones[0].m[0][0]);
				}
			}

			GLuint vao;
			const bool has_vao = pGL->GetVAO(vaoid, vao);
			glBindVertexArray(vao);
			if (!has_vao)
			{
				// Bind the vertex data of the mesh
				// Note this relies on the fact that all vertex
				// attributes for all shaders are at the same
				// locations.
				// TODO: And this fails if the mesh changes
				// from a material with texture to one without
				// or vice versa.
				glBindBuffer(GL_ARRAY_BUFFER, vbo);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
#define VERTEX_OFFSET(field) reinterpret_cast<const uint8_t *>(offsetof(StdMeshVertex, field))
				if (shader->GetAttribute(C4SSA_TexCoord) != -1)
					glVertexAttribPointer(shader->GetAttribute(C4SSA_TexCoord), 2, GL_FLOAT, GL_FALSE, sizeof(StdMeshVertex), VERTEX_OFFSET(u));
				if (!ForceSoftwareTransform)
				{
					glVertexAttribPointer(shader->GetAttribute(C4SSA_Position), 3, GL_FLOAT, GL_FALSE, sizeof(StdMeshVertex), VERTEX_OFFSET(x));
					glVertexAttribPointer(shader->GetAttribute(C4SSA_Normal), 3, GL_FLOAT, GL_FALSE, sizeof(StdMeshVertex), VERTEX_OFFSET(nx));
					glEnableVertexAttribArray(shader->GetAttribute(C4SSA_Position));
					glEnableVertexAttribArray(shader->GetAttribute(C4SSA_Normal));

					glVertexAttribPointer(shader->GetAttribute(C4SSA_BoneWeights0), 4, GL_FLOAT, GL_FALSE, sizeof(StdMeshVertex), VERTEX_OFFSET(bone_weight));
					glVertexAttribPointer(shader->GetAttribute(C4SSA_BoneWeights1), 4, GL_FLOAT, GL_FALSE, sizeof(StdMeshVertex), VERTEX_OFFSET(bone_weight) + 4 * sizeof(std::remove_all_extents<decltype(StdMeshVertex::bone_weight)>::type));
					glVertexAttribPointer(shader->GetAttribute(C4SSA_BoneIndices0), 4, GL_SHORT, GL_FALSE, sizeof(StdMeshVertex), VERTEX_OFFSET(bone_index));
					glVertexAttribPointer(shader->GetAttribute(C4SSA_BoneIndices1), 4, GL_SHORT, GL_FALSE, sizeof(StdMeshVertex), VERTEX_OFFSET(bone_index) + 4 * sizeof(std::remove_all_extents<decltype(StdMeshVertex::bone_index)>::type));
					glEnableVertexAttribArray(shader->GetAttribute(C4SSA_BoneWeights0));
					glEnableVertexAttribArray(shader->GetAttribute(C4SSA_BoneWeights1));
					glEnableVertexAttribArray(shader->GetAttribute(C4SSA_BoneIndices0));
					glEnableVertexAttribArray(shader->GetAttribute(C4SSA_BoneIndices1));
				}
				if (shader->GetAttribute(C4SSA_TexCoord) != -1)
					glEnableVertexAttribArray(shader->GetAttribute(C4SSA_TexCoord));

#undef VERTEX_OFFSET
			}

			if (ForceSoftwareTransform)
			{
				glBindBuffer(GL_ARRAY_BUFFER, pretransform_vbo);
#define VERTEX_OFFSET(field) reinterpret_cast<const uint8_t *>(offsetof(PretransformedMeshVertex, field))
				glVertexAttribPointer(shader->GetAttribute(C4SSA_Position), 3, GL_FLOAT, GL_FALSE, sizeof(PretransformedMeshVertex), VERTEX_OFFSET(x));
				glEnableVertexAttribArray(shader->GetAttribute(C4SSA_Position));
				glVertexAttribPointer(shader->GetAttribute(C4SSA_Normal), 3, GL_FLOAT, GL_FALSE, sizeof(PretransformedMeshVertex), VERTEX_OFFSET(nx));
				glEnableVertexAttribArray(shader->GetAttribute(C4SSA_Normal));
#undef VERTEX_OFFSET
				glBindBuffer(GL_ARRAY_BUFFER, 0);
			}

			// Bind textures
			for (unsigned int j = 0; j < pass.TextureUnits.size(); ++j)
			{
				const StdMeshMaterialTextureUnit& texunit = pass.TextureUnits[j];
				if (texunit.HasTexture())
				{
					call.AllocTexUnit(-1);
					const unsigned int Phase = instance.GetTexturePhase(i, j);
					glBindTexture(GL_TEXTURE_2D, texunit.GetTexture(Phase).texName);
				}
			}

			// Set uniforms and instance parameters
			SetStandardUniforms(call, dwModClr, dwPlayerColor, dwBlitMode, pass.CullHardware != StdMeshMaterialPass::CH_None, pFoW, clipRect, outRect);
			for(auto & Parameter : pass.Program->Parameters)
			{
				const int uniform = Parameter.UniformIndex;
				if(!shader->HaveUniform(uniform)) continue; // optimized out

				const StdMeshMaterialShaderParameter* parameter = Parameter.Parameter;

				StdMeshMaterialShaderParameter auto_resolved;
				if(parameter->GetType() == StdMeshMaterialShaderParameter::AUTO)
				{
					if(!ResolveAutoParameter(call, auto_resolved, parameter->GetAuto(), dwModClr, dwPlayerColor, dwBlitMode, pFoW, clipRect))
						continue;
					parameter = &auto_resolved;
				}

				switch(parameter->GetType())
				{
				case StdMeshMaterialShaderParameter::AUTO_TEXTURE_MATRIX:
					call.SetUniformMatrix4x4(uniform, ResolveAutoTextureMatrix(instance, technique, i, parameter->GetInt()));
					break;
				case StdMeshMaterialShaderParameter::INT:
					call.SetUniform1i(uniform, parameter->GetInt());
					break;
				case StdMeshMaterialShaderParameter::FLOAT:
					call.SetUniform1f(uniform, parameter->GetFloat());
					break;
				case StdMeshMaterialShaderParameter::FLOAT2:
					call.SetUniform2fv(uniform, 1, parameter->GetFloatv());
					break;
				case StdMeshMaterialShaderParameter::FLOAT3:
					call.SetUniform3fv(uniform, 1, parameter->GetFloatv());
					break;
				case StdMeshMaterialShaderParameter::FLOAT4:
					call.SetUniform4fv(uniform, 1, parameter->GetFloatv());
					break;
				case StdMeshMaterialShaderParameter::MATRIX_4X4:
					call.SetUniformMatrix4x4fv(uniform, 1, parameter->GetMatrix());
					break;
				default:
					assert(false);
					break;
				}
			}

			pDraw->scriptUniform.Apply(call);

			size_t vertex_count = 3 * instance.GetNumFaces();
			assert (vertex_buffer_offset % sizeof(StdMeshVertex) == 0);
			size_t base_vertex = vertex_buffer_offset / sizeof(StdMeshVertex);
			glDrawElementsBaseVertex(GL_TRIANGLES, vertex_count, GL_UNSIGNED_INT, reinterpret_cast<void*>(index_buffer_offset), base_vertex);
			glBindVertexArray(0);
			call.Finish();

			if(!pass.DepthCheck)
				glEnable(GL_DEPTH_TEST);
		}

		if (ForceSoftwareTransform)
			glDeleteBuffers(1, &pretransform_vbo);
	}

	void RenderMeshImpl(const StdProjectionMatrix& projectionMatrix, const StdMeshMatrix& modelviewMatrix, StdMeshInstance& instance, DWORD dwModClr, DWORD dwBlitMode, DWORD dwPlayerColor, const C4FoWRegion* pFoW, const C4Rect& clipRect, const C4Rect& outRect, bool parity); // Needed by RenderAttachedMesh

	void RenderAttachedMesh(const StdProjectionMatrix& projectionMatrix, const StdMeshMatrix& modelviewMatrix, StdMeshInstance::AttachedMesh* attach, DWORD dwModClr, DWORD dwBlitMode, DWORD dwPlayerColor, const C4FoWRegion* pFoW, const C4Rect& clipRect, const C4Rect& outRect, bool parity)
	{
		const StdMeshMatrix& FinalTrans = attach->GetFinalTransformation();

		// Take the player color from the C4Object, if the attached object is not a definition
		// This is a bit unfortunate because it requires access to C4Object which is otherwise
		// avoided in this code. It could be replaced by virtual function calls to StdMeshDenumerator
		C4MeshDenumerator* denumerator = dynamic_cast<C4MeshDenumerator*>(attach->ChildDenumerator);
		if(denumerator && denumerator->GetObject())
		{
			dwModClr = denumerator->GetObject()->ColorMod;
			dwBlitMode = denumerator->GetObject()->BlitMode;
			dwPlayerColor = denumerator->GetObject()->Color;
		}

		// TODO: Take attach transform's parity into account
		StdMeshMatrix newModelviewMatrix = modelviewMatrix * FinalTrans;
		RenderMeshImpl(projectionMatrix, newModelviewMatrix, *attach->Child, dwModClr, dwBlitMode, dwPlayerColor, pFoW, clipRect, outRect, parity);
	}

	void RenderMeshImpl(const StdProjectionMatrix& projectionMatrix, const StdMeshMatrix& modelviewMatrix, StdMeshInstance& instance, DWORD dwModClr, DWORD dwBlitMode, DWORD dwPlayerColor, const C4FoWRegion* pFoW, const C4Rect& clipRect, const C4Rect& outRect, bool parity)
	{
		const StdMesh& mesh = instance.GetMesh();

		// Render AM_DrawBefore attached meshes
		StdMeshInstance::AttachedMeshIter attach_iter = instance.AttachedMeshesBegin();

		for (; attach_iter != instance.AttachedMeshesEnd() && ((*attach_iter)->GetFlags() & StdMeshInstance::AM_DrawBefore); ++attach_iter)
			RenderAttachedMesh(projectionMatrix, modelviewMatrix, *attach_iter, dwModClr, dwBlitMode, dwPlayerColor, pFoW, clipRect, outRect, parity);

		// Check if we should draw in wireframe or normal mode
		if(dwBlitMode & C4GFXBLIT_WIREFRAME)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		// Render each submesh
		for (unsigned int i = 0; i < mesh.GetNumSubMeshes(); ++i)
			RenderSubMeshImpl(projectionMatrix, modelviewMatrix, instance, instance.GetSubMeshOrdered(i), dwModClr, dwBlitMode, dwPlayerColor, pFoW, clipRect, outRect, parity);

		// reset old mode to prevent rendering errors
		if(dwBlitMode & C4GFXBLIT_WIREFRAME)
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		// Render non-AM_DrawBefore attached meshes
		for (; attach_iter != instance.AttachedMeshesEnd(); ++attach_iter)
			RenderAttachedMesh(projectionMatrix, modelviewMatrix, *attach_iter, dwModClr, dwBlitMode, dwPlayerColor, pFoW, clipRect, outRect, parity);
	}
}

void CStdGL::PerformMesh(StdMeshInstance &instance, float tx, float ty, float twdt, float thgt, DWORD dwPlayerColor, C4BltTransform* pTransform)
{
	// Field of View for perspective projection, in degrees
	static const float FOV = 60.0f;
	static const float TAN_FOV = tan(FOV / 2.0f / 180.0f * M_PI);

	// Check mesh transformation; abort when it is degenerate.
	bool mesh_transform_parity = false;
	if (MeshTransform)
	{
		const float det = MeshTransform->Determinant();
		if (fabs(det) < 1e-6)
			return;
		else if (det < 0.0f)
			mesh_transform_parity = true;
	}

	const StdMesh& mesh = instance.GetMesh();

	bool parity = false;

	// Convert bounding box to clonk coordinate system
	// (TODO: We should cache this, not sure where though)
	const StdMeshBox& box = mesh.GetBoundingBox();
	StdMeshVector v1, v2;
	v1.x = box.x1; v1.y = box.y1; v1.z = box.z1;
	v2.x = box.x2; v2.y = box.y2; v2.z = box.z2;

	// Vector from origin of mesh to center of mesh
	const StdMeshVector MeshCenter = (v1 + v2)/2.0f;

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND); // TODO: Shouldn't this always be enabled? - blending does not work for meshes without this though.

	// TODO: We ignore the additive drawing flag for meshes but instead
	// set the blending mode of the corresponding material. I'm not sure
	// how the two could be combined.
	// TODO: Maybe they can be combined using a pixel shader which does
	// ftransform() and then applies colormod, additive and mod2
	// on the result (with alpha blending).
	//int iAdditive = dwBlitMode & C4GFXBLIT_ADDITIVE;
	//glBlendFunc(GL_SRC_ALPHA, iAdditive ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA);

	// Mesh extents
	const float b = fabs(v2.x - v1.x)/2.0f;
	const float h = fabs(v2.y - v1.y)/2.0f;
	const float l = fabs(v2.z - v1.z)/2.0f;

	// Set up projection matrix first. We do transform and Zoom with the
	// projection matrix, so that lighting is applied to the untransformed/unzoomed
	// mesh.
	StdProjectionMatrix projectionMatrix;
	if (!fUsePerspective)
	{
		// Load the orthographic projection
		projectionMatrix = ProjectionMatrix;

		if (!ApplyZoomAndTransform(ZoomX, ZoomY, Zoom, pTransform, projectionMatrix))
			parity = !parity;

		// Scale so that the mesh fits in (tx,ty,twdt,thgt)
		const float rx = -std::min(v1.x,v2.x) / fabs(v2.x - v1.x);
		const float ry = -std::min(v1.y,v2.y) / fabs(v2.y - v1.y);
		const float dx = tx + rx*twdt;
		const float dy = ty + ry*thgt;

		// Scale so that Z coordinate is between -1 and 1, otherwise parts of
		// the mesh could be clipped away by the near or far clipping plane.
		// Note that this only works for the projection matrix, otherwise
		// lighting is screwed up.

		// This technique might also enable us not to clear the depth buffer
		// after every mesh rendering - we could simply scale the first mesh
		// of the scene so that it's Z coordinate is between 0 and 1, scale
		// the second mesh that it is between 1 and 2, and so on.
		// This of course requires an orthogonal projection so that the
		// meshes don't look distorted - if we should ever decide to use
		// a perspective projection we need to think of something different.
		// Take also into account that the depth is not linear but linear
		// in the logarithm (if I am not mistaken), so goes as 1/z

		// Don't scale by Z extents since mesh might be transformed
		// by MeshTransformation, so use GetBoundingRadius to be safe.
		// Note this still fails if mesh is scaled in Z direction or
		// there are attached meshes.
		const float scz = 1.0/(mesh.GetBoundingRadius());

		Translate(projectionMatrix, dx, dy, 0.0f);
		Scale(projectionMatrix, 1.0f, 1.0f, scz);
	}
	else
	{
		// Perspective projection. This code transforms the projected
		// 3D model into the target area.
		const C4Rect clipRect = GetClipRect();
		projectionMatrix = StdProjectionMatrix::Identity();

		// Back to GL device coordinates
		Translate(projectionMatrix, -1.0f, 1.0f, 0.0f);
		Scale(projectionMatrix, 2.0f/clipRect.Wdt, -2.0f/clipRect.Hgt, 1.0f);

		Translate(projectionMatrix, -clipRect.x, -clipRect.y, 0.0f);
		if (!ApplyZoomAndTransform(ZoomX, ZoomY, Zoom, pTransform, projectionMatrix))
			parity = !parity;

		// Move to target location and compensate for 1.0f aspect
		float ttx = tx, tty = ty, ttwdt = twdt, tthgt = thgt;
		if(twdt > thgt)
		{
			tty += (thgt-twdt)/2.0;
			tthgt = twdt;
		}
		else
		{
			ttx += (twdt-thgt)/2.0;
			ttwdt = thgt;
		}

		Translate(projectionMatrix, ttx, tty, 0.0f);
		Scale(projectionMatrix, ((float)ttwdt)/clipRect.Wdt, ((float)tthgt)/clipRect.Hgt, 1.0f);

		// Return to Clonk coordinate frame
		Scale(projectionMatrix, clipRect.Wdt/2.0, -clipRect.Hgt/2.0, 1.0f);
		Translate(projectionMatrix, 1.0f, -1.0f, 0.0f);

		// Fix for the case when we have different aspect ratios
		const float ta = twdt / thgt;
		const float ma = b / h;
		if(ta <= 1 && ta/ma <= 1)
			Scale(projectionMatrix, std::max(ta, ta/ma), std::max(ta, ta/ma), 1.0f);
		else if(ta >= 1 && ta/ma >= 1)
			Scale(projectionMatrix, std::max(1.0f/ta, ma/ta), std::max(1.0f/ta, ma/ta), 1.0f);

		// Apply perspective projection. After this, x and y range from
		// -1 to 1, and this is mapped into tx/ty/twdt/thgt by the above code.
		// Aspect is 1.0f which is changed above as well.
		Perspective(projectionMatrix, 1.0f/TAN_FOV, 1.0f, 0.1f, 100.0f);
	}

	// Now set up the modelview matrix
	StdMeshMatrix modelviewMatrix;
	if (fUsePerspective)
	{
		// Setup camera position so that the mesh with uniform transformation
		// fits well into a square target (without distortion).
		const float EyeR = l + std::max(b/TAN_FOV, h/TAN_FOV);
		const StdMeshVector Eye = StdMeshVector::Translate(MeshCenter.x, MeshCenter.y, MeshCenter.z + EyeR);

		// Up vector is unit vector in theta direction
		const StdMeshVector Up = StdMeshVector::Translate(0.0f, -1.0f, 0.0f);

		// Fix X axis (???)
		modelviewMatrix = StdMeshMatrix::Scale(-1.0f, 1.0f, 1.0f);

		// center on mesh's bounding box, so that the mesh is really in the center of the viewport
		modelviewMatrix *= StdMeshMatrix::LookAt(Eye, MeshCenter, Up);
	}
	else
	{
		modelviewMatrix = StdMeshMatrix::Identity();
	}

	// Apply mesh transformation matrix
	if (MeshTransform)
	{
		// Apply MeshTransformation (in the Mesh's coordinate system)
		modelviewMatrix *= *MeshTransform;
		// Keep track of parity
		if (mesh_transform_parity) parity = !parity;
	}

	DWORD dwModClr = BlitModulated ? BlitModulateClr : 0xffffffff;

	const C4Rect clipRect = GetClipRect();
	const C4Rect outRect = GetOutRect();

	RenderMeshImpl(projectionMatrix, modelviewMatrix, instance, dwModClr, dwBlitMode, dwPlayerColor, pFoW, clipRect, outRect, parity);

	// Reset state
	//glActiveTexture(GL_TEXTURE0);
	glDepthMask(GL_TRUE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);

	// TODO: glScissor, so that we only clear the area the mesh covered.
	glClear(GL_DEPTH_BUFFER_BIT);
}

#endif // USE_CONSOLE
