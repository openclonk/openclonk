/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2013-2015, The OpenClonk Team and contributors
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
#include <C4Object.h>
#include <C4DrawGL.h>
#include <C4FoWRegion.h>
#include <SHA1.h>

#include "StdMesh.h"
#include "graphics/C4GraphicsResource.h"

#ifndef USE_CONSOLE

namespace
{
	////////////////////////////////////////////
	// Shader code generation
	// This translates the fixed function instructions in a material script
	// to an equivalent fragment shader. The generated code can certainly
	// be optimized more.
	////////////////////////////////////////////
	StdStrBuf TextureUnitSourceToCode(int index, StdMeshMaterialTextureUnit::BlendOpSourceType source, const float manualColor[3], float manualAlpha)
	{
		switch(source)
		{
		case StdMeshMaterialTextureUnit::BOS_Current: return StdStrBuf("currentColor");
		case StdMeshMaterialTextureUnit::BOS_Texture: return FormatString("texture2D(oc_Texture%d, texcoord)", index);
		case StdMeshMaterialTextureUnit::BOS_Diffuse: return StdStrBuf("diffuse");
		case StdMeshMaterialTextureUnit::BOS_Specular: return StdStrBuf("diffuse"); // TODO: Should be specular
		case StdMeshMaterialTextureUnit::BOS_PlayerColor: return StdStrBuf("vec4(oc_PlayerColor, 1.0)");
		case StdMeshMaterialTextureUnit::BOS_Manual: return FormatString("vec4(%f, %f, %f, %f)", manualColor[0], manualColor[1], manualColor[2], manualAlpha);
		default: assert(false); return StdStrBuf("vec4(0.0, 0.0, 0.0, 0.0)");
		}
	}

	StdStrBuf TextureUnitBlendToCode(int index, StdMeshMaterialTextureUnit::BlendOpExType blend_type, const char* source1, const char* source2, float manualFactor)
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
		case StdMeshMaterialTextureUnit::BOX_BlendTextureAlpha: return FormatString("texture2D(oc_Texture%d, texcoord).a * %s + (1.0 - texture2D(oc_Texture%d, texcoord).a) * %s", index, source1, index, source2);
		case StdMeshMaterialTextureUnit::BOX_BlendCurrentAlpha: return FormatString("currentColor.a * %s + (1.0 - currentColor.a) * %s", source1, source2);
		case StdMeshMaterialTextureUnit::BOX_BlendManual: return FormatString("%f * %s + (1.0 - %f) * %s", manualFactor, source1, manualFactor, source2);
		case StdMeshMaterialTextureUnit::BOX_Dotproduct: return FormatString("vec3(4.0 * dot(%s - 0.5, %s - 0.5), 4.0 * dot(%s - 0.5, %s - 0.5), 4.0 * dot(%s - 0.5, %s - 0.5));", source1, source2, source1, source2, source1, source2); // TODO: Needs special handling for the case of alpha
		case StdMeshMaterialTextureUnit::BOX_BlendDiffuseColor: return FormatString("diffuse.rgb * %s + (1.0 - diffuse.rgb) * %s", source1, source2);
		default: assert(false); return StdStrBuf(source1);
		}
	}

	StdStrBuf TextureUnitToCode(int index, const StdMeshMaterialTextureUnit& texunit)
	{
		StdStrBuf color_source1 = FormatString("%s.rgb", TextureUnitSourceToCode(index, texunit.ColorOpSources[0], texunit.ColorOpManualColor1, texunit.AlphaOpManualAlpha1).getData());
		StdStrBuf color_source2 = FormatString("%s.rgb", TextureUnitSourceToCode(index, texunit.ColorOpSources[1], texunit.ColorOpManualColor2, texunit.AlphaOpManualAlpha2).getData());
		StdStrBuf alpha_source1 = FormatString("%s.a", TextureUnitSourceToCode(index, texunit.AlphaOpSources[0], texunit.ColorOpManualColor1, texunit.AlphaOpManualAlpha1).getData());
		StdStrBuf alpha_source2 = FormatString("%s.a", TextureUnitSourceToCode(index, texunit.AlphaOpSources[1], texunit.ColorOpManualColor2, texunit.AlphaOpManualAlpha2).getData());

		return FormatString("currentColor = vec4(%s, %s);\n", TextureUnitBlendToCode(index, texunit.ColorOpEx, color_source1.getData(), color_source2.getData(), texunit.ColorOpManualFactor).getData(), TextureUnitBlendToCode(index, texunit.AlphaOpEx, alpha_source1.getData(), alpha_source2.getData(), texunit.AlphaOpManualFactor).getData());
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

	StdStrBuf GetVertexShaderCodeForPass(const StdMeshMaterialPass& pass)
	{
		StdStrBuf buf;

		if (!::GraphicsResource.Files.LoadEntryString("ObjectDefaultVS.glsl", &buf))
		{
			// Fall back just in case
			buf.Copy(
				"varying vec3 normalDir;\n"
				"\n"
				"slice(position)\n"
				"{\n"
				"  gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
				"}\n"
				"\n"
				"slice(texcoord)\n"
				"{\n"
				"  texcoord = gl_MultiTexCoord0.xy;\n"
				"}\n"
				"\n"
				"slice(normal)\n"
				"{\n"
				"  normalDir = normalize(gl_NormalMatrix * gl_Normal);\n"
				"}\n"
			);
		}

		if (pGL->Workarounds.LowMaxVertexUniformCount)
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
		for(unsigned int i = 0; i < pass.TextureUnits.size(); ++i)
		{
			const StdMeshMaterialTextureUnit& texunit = pass.TextureUnits[i];
			textureUnitCode.Append(TextureUnitToCode(texIndex, texunit));

			if(texunit.HasTexture())
			{
				textureUnitDeclCode.Append(FormatString("uniform sampler2D oc_Texture%u;\n", texIndex).getData());
				params.AddParameter(FormatString("oc_Texture%u", texIndex).getData(), StdMeshMaterialShaderParameter::INT).GetInt() = texIndex;
				++texIndex;
			}
		}

		return FormatString(
			"%s\n" // Texture units with active textures, only if >0 texture units
			"uniform vec3 oc_PlayerColor;\n" // This needs to be in-sync with the naming in StdMeshMaterialProgram::CompileShader()
			"\n"
			"slice(texture)\n"
			"{\n"
			"  vec4 diffuse = color;\n"
			"  vec4 currentColor = diffuse;\n"
			"  %s\n"
			"  color = currentColor;\n"
			"}\n",
			textureUnitDeclCode.getData(),
			textureUnitCode.getData()
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
			glGetIntegerv(GL_MAX_TEXTURE_UNITS, &max_texture_units);
			assert(max_texture_units >= 1);
			
			unsigned int active_texture_units = 0;
			for(unsigned int k = 0; k < pass.TextureUnits.size(); ++k)
				if(pass.TextureUnits[k].HasTexture())
					++active_texture_units;

			if (active_texture_units > static_cast<unsigned int>(max_texture_units))
				technique.Available = false;

			for (unsigned int k = 0; k < pass.TextureUnits.size(); ++k)
			{
				StdMeshMaterialTextureUnit& texunit = pass.TextureUnits[k];
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

					if (texunit.Filtering[2] == StdMeshMaterialTextureUnit::F_Point ||
					    texunit.Filtering[2] == StdMeshMaterialTextureUnit::F_Linear)
					{
						// If mipmapping is enabled, then autogenerate mipmap data.
						// In OGRE this is deactivated for several OS/graphics card
						// combinations because of known bugs...

						// This does work for me, but requires re-upload of texture data...
						// so the proper way would be to set this prior to the initial
						// upload, which would be the same place where we could also use
						// gluBuild2DMipmaps. GL_GENERATE_MIPMAP is probably still more
						// efficient though.

						// Disabled for now, until we find a better place for this (C4TexRef?)
#if 0
						if (GLEW_VERSION_1_4)
							{ glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE); const_cast<C4TexRef*>(&texunit.GetTexture())->Lock(); const_cast<C4TexRef*>(&texunit.GetTexture())->Unlock(); }
						else
							technique.Available = false;
#else
						// Disable mipmap for now as a workaround.
						texunit.Filtering[2] = StdMeshMaterialTextureUnit::F_None;
#endif
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

					for (unsigned int m = 0; m < texunit.Transformations.size(); ++m)
					{
						StdMeshMaterialTextureUnit::Transformation& trans = texunit.Transformations[m];
						if (trans.TransformType == StdMeshMaterialTextureUnit::Transformation::T_TRANSFORM)
						{
							// transpose so we can directly pass it to glMultMatrixf
							std::swap(trans.Transform.M[ 1], trans.Transform.M[ 4]);
							std::swap(trans.Transform.M[ 2], trans.Transform.M[ 8]);
							std::swap(trans.Transform.M[ 3], trans.Transform.M[12]);
							std::swap(trans.Transform.M[ 6], trans.Transform.M[ 9]);
							std::swap(trans.Transform.M[ 7], trans.Transform.M[13]);
							std::swap(trans.Transform.M[11], trans.Transform.M[14]);
						}
					}
				} // loop over textures
			} // loop over texture units

			// Create fragment and/or vertex shader
			// if a custom shader is not provided.
			// Re-use existing programs if the generated
			// code is the same (determined by SHA1 hash).
			if(!pass.VertexShader.Shader)
			{
				StdStrBuf buf = GetVertexShaderCodeForPass(pass);
				StdStrBuf hash = GetSHA1HexDigest(buf.getData(), buf.getLength());
				pass.VertexShader.Shader = mat_manager.AddShader("auto-generated vertex shader", hash.getData(), "glsl", SMMS_VERTEX, buf.getData(), true);
			}

			if(!pass.FragmentShader.Shader)
			{
				// TODO: Should use shared_params once we introduce them
				StdStrBuf buf = GetFragmentShaderCodeForPass(pass, pass.FragmentShader.Parameters);
				StdStrBuf hash = GetSHA1HexDigest(buf.getData(), buf.getLength());
				pass.FragmentShader.Shader = mat_manager.AddShader("auto-generated fragment shader", hash.getData(), "glsl", SMMS_FRAGMENT, buf.getData(), true);
			}

			// Then, link the program, and resolve parameter locations
			StdStrBuf name(FormatString("%s:%s:%s", mat.Name.getData(), technique.Name.getData(), pass.Name.getData()));
			const StdMeshMaterialProgram* added_program = mat_manager.AddProgram(name.getData(), loader, pass.FragmentShader, pass.VertexShader, pass.GeometryShader);
			if(!added_program)
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
	bool ApplyZoomAndTransform(float ZoomX, float ZoomY, float Zoom, C4BltTransform* pTransform)
	{
		// Apply zoom
		glTranslatef(ZoomX, ZoomY, 0.0f);
		glScalef(Zoom, Zoom, 1.0f);
		glTranslatef(-ZoomX, -ZoomY, 0.0f);

		// Apply transformation
		if (pTransform)
		{
			const GLfloat transform[16] = { pTransform->mat[0], pTransform->mat[3], 0, pTransform->mat[6], pTransform->mat[1], pTransform->mat[4], 0, pTransform->mat[7], 0, 0, 1, 0, pTransform->mat[2], pTransform->mat[5], 0, pTransform->mat[8] };
			glMultMatrixf(transform);

			// Compute parity of the transformation matrix - if parity is swapped then
			// we need to cull front faces instead of back faces.
			const float det = transform[0]*transform[5]*transform[15]
			                  + transform[4]*transform[13]*transform[3]
			                  + transform[12]*transform[1]*transform[7]
			                  - transform[0]*transform[13]*transform[7]
			                  - transform[4]*transform[1]*transform[15]
			                  - transform[12]*transform[5]*transform[3];
			return det > 0;
		}

		return true;
	}

	void SetStandardUniforms(C4ShaderCall& call, DWORD dwModClr, DWORD dwPlayerColor, DWORD dwBlitMode, const C4FoWRegion* pFoW, const C4Rect& clipRect, const C4Rect& outRect)
	{
		// Draw transform
		const float fMod[4] = {
			((dwModClr >> 16) & 0xff) / 255.0f,
			((dwModClr >>  8) & 0xff) / 255.0f,
			((dwModClr      ) & 0xff) / 255.0f,
			((dwModClr >> 24) & 0xff) / 255.0f
		};
		call.SetUniform4fv(C4SSU_ClrMod, 1, fMod);

		// Player color
		const float fPlrClr[3] = {
			((dwPlayerColor >> 16) & 0xff) / 255.0f,
			((dwPlayerColor >>  8) & 0xff) / 255.0f,
			((dwPlayerColor      ) & 0xff) / 255.0f,
		};
		call.SetUniform3fv(C4SSU_OverlayClr, 1, fPlrClr);

		// Dynamic light
		if(pFoW != NULL)
		{
			call.AllocTexUnit(C4SSU_LightTex, GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, pFoW->getSurface()->textures[0].texName);
			float lightTransform[6];
			pFoW->GetFragTransform(clipRect, outRect, lightTransform);
			call.SetUniformMatrix2x3fv(C4SSU_LightTransform, 1, lightTransform);

			call.AllocTexUnit(C4SSU_AmbientTex, GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, pFoW->getFoW()->Ambient.Tex);
			call.SetUniform1f(C4SSU_AmbientBrightness, pFoW->getFoW()->Ambient.GetBrightness());
			float ambientTransform[6];
			pFoW->getFoW()->Ambient.GetFragTransform(pFoW->getViewportRegion(), clipRect, outRect, ambientTransform);
			call.SetUniformMatrix2x3fv(C4SSU_AmbientTransform, 1, ambientTransform);
		}
	}

	bool ResolveAutoParameter(C4ShaderCall& call, StdMeshMaterialShaderParameter& parameter, StdMeshMaterialShaderParameter::Auto value, DWORD dwModClr, DWORD dwPlayerColor, DWORD dwBlitMode, const C4FoWRegion* pFoW, const C4Rect& clipRect)
	{
		// There are no auto parameters implemented yet
		assert(false);
		return false;
	}

	void RenderSubMeshImpl(const StdMeshInstance& mesh_instance, const StdSubMeshInstance& instance, DWORD dwModClr, DWORD dwBlitMode, DWORD dwPlayerColor, const C4FoWRegion* pFoW, const C4Rect& clipRect, const C4Rect& outRect, bool parity)
	{
		const StdMeshMaterial& material = instance.GetMaterial();
		assert(material.BestTechniqueIndex != -1);
		const StdMeshMaterialTechnique& technique = material.Techniques[material.BestTechniqueIndex];

		bool using_shared_vertices = instance.GetSubMesh().GetVertices().empty();
		GLuint vbo = mesh_instance.GetMesh().GetVBO();
		size_t buffer_offset = using_shared_vertices ? 0 : instance.GetSubMesh().GetOffsetInBuffer();

		// Cook the bone transform matrixes into something that OpenGL can use. This could be moved into RenderMeshImpl.
		// Or, even better, we could upload them into a UBO, but Intel doesn't support them prior to Sandy Bridge.
		struct BoneTransform
		{
			float m[3][4];
		};
		std::vector<BoneTransform> bones;
		if (mesh_instance.GetBoneCount() == 0)
		{
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

		// Render each pass
		for (unsigned int i = 0; i < technique.Passes.size(); ++i)
		{
			const StdMeshMaterialPass& pass = technique.Passes[i];

			if(!pass.DepthCheck)
				glDisable(GL_DEPTH_TEST);

			glDepthMask(pass.DepthWrite ? GL_TRUE : GL_FALSE);

			if(pass.AlphaToCoverage)
				glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
			else
				glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);

			// Set material properties
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, pass.Ambient);
			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, pass.Diffuse);
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, pass.Specular);
			glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, pass.Emissive);
			glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, pass.Shininess);

			glFrontFace(parity ? GL_CW : GL_CCW);
			if(mesh_instance.GetCompletion() < 1.0f)
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
			if(!(dwBlitMode & C4GFXBLIT_ADDITIVE))
			{
				if( ((dwModClr >> 24) & 0xff) < 0xff) // && (!(dwBlitMode & C4GFXBLIT_MOD2)) )
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				else
					glBlendFunc(OgreBlendTypeToGL(pass.SceneBlendFactors[0]),
					            OgreBlendTypeToGL(pass.SceneBlendFactors[1]));
			}
			else
			{
				if( ((dwModClr >> 24) & 0xff) < 0xff) // && (!(dwBlitMode & C4GFXBLIT_MOD2)) )
					glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				else
					glBlendFunc(OgreBlendTypeToGL(pass.SceneBlendFactors[0]), GL_ONE);
			}

			glMatrixMode(GL_TEXTURE);

			assert(pass.Program.get() != NULL);

			// Upload all parameters to the shader (keep GL_TEXTURE matrix mode for this)
			int ssc = 0;
			if(dwBlitMode & C4GFXBLIT_MOD2) ssc |= C4SSC_MOD2;
			if(pFoW != NULL) ssc |= C4SSC_LIGHT;
			const C4Shader* shader = pass.Program->Program->GetShader(ssc);
			C4ShaderCall call(shader);
			call.Start();

			// Upload the current bone transformation matrixes (if there are any)
			if (!bones.empty())
				if (pGL->Workarounds.LowMaxVertexUniformCount)
					glUniformMatrix3x4fv(shader->GetUniform(C4SSU_Bones), bones.size(), GL_FALSE, &bones[0].m[0][0]);
				else
					glUniformMatrix4x3fv(shader->GetUniform(C4SSU_Bones), bones.size(), GL_TRUE, &bones[0].m[0][0]);

			// Bind the vertex data of the mesh
#define VERTEX_OFFSET(field) reinterpret_cast<const uint8_t *>(offsetof(StdMeshVertex, field))
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glTexCoordPointer(2, GL_FLOAT, sizeof(StdMeshVertex), buffer_offset + VERTEX_OFFSET(u));
			glVertexPointer(3, GL_FLOAT, sizeof(StdMeshVertex), buffer_offset + VERTEX_OFFSET(x));
			glNormalPointer(GL_FLOAT, sizeof(StdMeshVertex), buffer_offset + VERTEX_OFFSET(nx));
			for (int attrib_index = 0; attrib_index <= C4Shader::VAI_BoneIndicesMax - C4Shader::VAI_BoneIndices; ++attrib_index)
			{
				glVertexAttribPointer(C4Shader::VAI_BoneWeights + attrib_index, 4, GL_FLOAT, GL_FALSE, sizeof(StdMeshVertex),
					buffer_offset + VERTEX_OFFSET(bone_weight) + sizeof(std::remove_all_extents<decltype(StdMeshVertex::bone_weight)>::type) * 4 * attrib_index);
				glEnableVertexAttribArray(C4Shader::VAI_BoneWeights + attrib_index);
				glVertexAttribPointer(C4Shader::VAI_BoneIndices + attrib_index, 4, GL_SHORT, GL_FALSE, sizeof(StdMeshVertex),
					buffer_offset + VERTEX_OFFSET(bone_index) + sizeof(std::remove_all_extents<decltype(StdMeshVertex::bone_index)>::type) * 4 * attrib_index);
				glEnableVertexAttribArray(C4Shader::VAI_BoneIndices + attrib_index);
			}
#undef VERTEX_OFFSET

			for (unsigned int j = 0; j < pass.TextureUnits.size(); ++j)
			{
				const StdMeshMaterialTextureUnit& texunit = pass.TextureUnits[j];
				if (texunit.HasTexture())
				{
					call.AllocTexUnit(-1, GL_TEXTURE_2D);
					const unsigned int Phase = instance.GetTexturePhase(i, j);
					glBindTexture(GL_TEXTURE_2D, texunit.GetTexture(Phase).texName);

					// Setup texture coordinate transform
					glLoadIdentity();
					const double Position = instance.GetTexturePosition(i, j);
					for (unsigned int k = 0; k < texunit.Transformations.size(); ++k)
					{
						const StdMeshMaterialTextureUnit::Transformation& trans = texunit.Transformations[k];
						switch (trans.TransformType)
						{
						case StdMeshMaterialTextureUnit::Transformation::T_SCROLL:
							glTranslatef(trans.Scroll.X, trans.Scroll.Y, 0.0f);
							break;
						case StdMeshMaterialTextureUnit::Transformation::T_SCROLL_ANIM:
							glTranslatef(trans.GetScrollX(Position), trans.GetScrollY(Position), 0.0f);
							break;
						case StdMeshMaterialTextureUnit::Transformation::T_ROTATE:
							glRotatef(trans.Rotate.Angle, 0.0f, 0.0f, 1.0f);
							break;
						case StdMeshMaterialTextureUnit::Transformation::T_ROTATE_ANIM:
							glRotatef(trans.GetRotate(Position), 0.0f, 0.0f, 1.0f);
							break;
						case StdMeshMaterialTextureUnit::Transformation::T_SCALE:
							glScalef(trans.Scale.X, trans.Scale.Y, 1.0f);
							break;
						case StdMeshMaterialTextureUnit::Transformation::T_TRANSFORM:
							glMultMatrixf(trans.Transform.M);
							break;
						case StdMeshMaterialTextureUnit::Transformation::T_WAVE_XFORM:
							switch (trans.WaveXForm.XForm)
							{
							case StdMeshMaterialTextureUnit::Transformation::XF_SCROLL_X:
								glTranslatef(trans.GetWaveXForm(Position), 0.0f, 0.0f);
								break;
							case StdMeshMaterialTextureUnit::Transformation::XF_SCROLL_Y:
								glTranslatef(0.0f, trans.GetWaveXForm(Position), 0.0f);
								break;
							case StdMeshMaterialTextureUnit::Transformation::XF_ROTATE:
								glRotatef(trans.GetWaveXForm(Position), 0.0f, 0.0f, 1.0f);
								break;
							case StdMeshMaterialTextureUnit::Transformation::XF_SCALE_X:
								glScalef(trans.GetWaveXForm(Position), 1.0f, 1.0f);
								break;
							case StdMeshMaterialTextureUnit::Transformation::XF_SCALE_Y:
								glScalef(1.0f, trans.GetWaveXForm(Position), 1.0f);
								break;
							}
							break;
						}
					}
				}
			}

			// Set uniforms and instance parameters
			SetStandardUniforms(call, dwModClr, dwPlayerColor, dwBlitMode, pFoW, clipRect, outRect);
			for(unsigned int i = 0; i < pass.Program->Parameters.size(); ++i)
			{
				const int uniform = pass.Program->Parameters[i].UniformIndex;
				if(!shader->HaveUniform(uniform)) continue; // optimized out

				const StdMeshMaterialShaderParameter* parameter = pass.Program->Parameters[i].Parameter;

				StdMeshMaterialShaderParameter auto_resolved;
				if(parameter->GetType() == StdMeshMaterialShaderParameter::AUTO)
				{
					if(!ResolveAutoParameter(call, auto_resolved, parameter->GetAuto(), dwModClr, dwPlayerColor, dwBlitMode, pFoW, clipRect))
						continue;
					parameter = &auto_resolved;
				}

				switch(parameter->GetType())
				{
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

			glMatrixMode(GL_MODELVIEW);
			size_t vertex_count = 3 * instance.GetNumFaces();
			glDrawElements(GL_TRIANGLES, vertex_count, GL_UNSIGNED_INT, instance.GetFaces());
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			for (int attrib_index = 0; attrib_index <= C4Shader::VAI_BoneIndicesMax - C4Shader::VAI_BoneIndices; ++attrib_index)
			{
				glDisableVertexAttribArray(C4Shader::VAI_BoneIndices + attrib_index);
				glDisableVertexAttribArray(C4Shader::VAI_BoneWeights + attrib_index);
			}
			call.Finish();

			if(!pass.DepthCheck)
				glEnable(GL_DEPTH_TEST);
		}
	}

	void RenderMeshImpl(StdMeshInstance& instance, DWORD dwModClr, DWORD dwBlitMode, DWORD dwPlayerColor, const C4FoWRegion* pFoW, const C4Rect& clipRect, const C4Rect& outRect, bool parity); // Needed by RenderAttachedMesh

	void RenderAttachedMesh(StdMeshInstance::AttachedMesh* attach, DWORD dwModClr, DWORD dwBlitMode, DWORD dwPlayerColor, const C4FoWRegion* pFoW, const C4Rect& clipRect, const C4Rect& outRect, bool parity)
	{
		const StdMeshMatrix& FinalTrans = attach->GetFinalTransformation();

		// Convert matrix to column-major order, add fourth row
		const float attach_trans_gl[16] =
		{
			FinalTrans(0,0), FinalTrans(1,0), FinalTrans(2,0), 0,
			FinalTrans(0,1), FinalTrans(1,1), FinalTrans(2,1), 0,
			FinalTrans(0,2), FinalTrans(1,2), FinalTrans(2,2), 0,
			FinalTrans(0,3), FinalTrans(1,3), FinalTrans(2,3), 1
		};

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
		glPushMatrix();
		glMultMatrixf(attach_trans_gl);
		RenderMeshImpl(*attach->Child, dwModClr, dwBlitMode, dwPlayerColor, pFoW, clipRect, outRect, parity);
		glPopMatrix();
	}

	void RenderMeshImpl(StdMeshInstance& instance, DWORD dwModClr, DWORD dwBlitMode, DWORD dwPlayerColor, const C4FoWRegion* pFoW, const C4Rect& clipRect, const C4Rect& outRect, bool parity)
	{
		const StdMesh& mesh = instance.GetMesh();

		// Render AM_DrawBefore attached meshes
		StdMeshInstance::AttachedMeshIter attach_iter = instance.AttachedMeshesBegin();

		for (; attach_iter != instance.AttachedMeshesEnd() && ((*attach_iter)->GetFlags() & StdMeshInstance::AM_DrawBefore); ++attach_iter)
			RenderAttachedMesh(*attach_iter, dwModClr, dwBlitMode, dwPlayerColor, pFoW, clipRect, outRect, parity);

		GLint modes[2];
		// Check if we should draw in wireframe or normal mode
		if(dwBlitMode & C4GFXBLIT_WIREFRAME)
		{
			// save old mode
			glGetIntegerv(GL_POLYGON_MODE, modes);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}

		// Render each submesh
		for (unsigned int i = 0; i < mesh.GetNumSubMeshes(); ++i)
			RenderSubMeshImpl(instance, instance.GetSubMeshOrdered(i), dwModClr, dwBlitMode, dwPlayerColor, pFoW, clipRect, outRect, parity);

		// reset old mode to prevent rendering errors
		if(dwBlitMode & C4GFXBLIT_WIREFRAME)
		{
			glPolygonMode(GL_FRONT, modes[0]);
			glPolygonMode(GL_BACK, modes[1]);
		}

		// Render non-AM_DrawBefore attached meshes
		for (; attach_iter != instance.AttachedMeshesEnd(); ++attach_iter)
			RenderAttachedMesh(*attach_iter, dwModClr, dwBlitMode, dwPlayerColor, pFoW, clipRect, outRect, parity);
	}
}

void CStdGL::PerformMesh(StdMeshInstance &instance, float tx, float ty, float twdt, float thgt, DWORD dwPlayerColor, C4BltTransform* pTransform)
{
	// Field of View for perspective projection, in degrees
	static const float FOV = 60.0f;
	static const float TAN_FOV = tan(FOV / 2.0f / 180.0f * M_PI);

	// Convert OgreToClonk matrix to column-major order
	// TODO: This must be executed after C4Draw::OgreToClonk was
	// initialized - is this guaranteed at this position?
	static const float OgreToClonkGL[16] =
	{
		C4Draw::OgreToClonk(0,0), C4Draw::OgreToClonk(1,0), C4Draw::OgreToClonk(2,0), 0,
		C4Draw::OgreToClonk(0,1), C4Draw::OgreToClonk(1,1), C4Draw::OgreToClonk(2,1), 0,
		C4Draw::OgreToClonk(0,2), C4Draw::OgreToClonk(1,2), C4Draw::OgreToClonk(2,2), 0,
		C4Draw::OgreToClonk(0,3), C4Draw::OgreToClonk(1,3), C4Draw::OgreToClonk(2,3), 1
	};

	static const bool OgreToClonkParity = C4Draw::OgreToClonk.Determinant() > 0.0f;

	const StdMesh& mesh = instance.GetMesh();

	bool parity = OgreToClonkParity;

	// Convert bounding box to clonk coordinate system
	// (TODO: We should cache this, not sure where though)
	// TODO: Note that this does not generally work with an arbitrary transformation this way
	const StdMeshBox& box = mesh.GetBoundingBox();
	StdMeshVector v1, v2;
	v1.x = box.x1; v1.y = box.y1; v1.z = box.z1;
	v2.x = box.x2; v2.y = box.y2; v2.z = box.z2;
	v1 = OgreToClonk * v1; // TODO: Include translation
	v2 = OgreToClonk * v2; // TODO: Include translation

	// Vector from origin of mesh to center of mesh
	const StdMeshVector MeshCenter = (v1 + v2)/2.0f;

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND); // TODO: Shouldn't this always be enabled? - blending does not work for meshes without this though.

	glClientActiveTexture(GL_TEXTURE0); // our only texcoord corresponds to tex0
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	// TODO: We ignore the additive drawing flag for meshes but instead
	// set the blending mode of the corresponding material. I'm not sure
	// how the two could be combined.
	// TODO: Maybe they can be combined using a pixel shader which does
	// ftransform() and then applies colormod, additive and mod2
	// on the result (with alpha blending).
	//int iAdditive = dwBlitMode & C4GFXBLIT_ADDITIVE;
	//glBlendFunc(GL_SRC_ALPHA, iAdditive ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA);

	// Set up projection matrix first. We do transform and Zoom with the
	// projection matrix, so that lighting is applied to the untransformed/unzoomed
	// mesh.
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();

	// Mesh extents
	const float b = fabs(v2.x - v1.x)/2.0f;
	const float h = fabs(v2.y - v1.y)/2.0f;
	const float l = fabs(v2.z - v1.z)/2.0f;

	if (!fUsePerspective)
	{
		// Orthographic projection. The orthographic projection matrix
		// is already loaded in the GL matrix stack.

		if (!ApplyZoomAndTransform(ZoomX, ZoomY, Zoom, pTransform))
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

		glTranslatef(dx, dy, 0.0f);
		glScalef(1.0f, 1.0f, scz);
	}
	else
	{
		// Perspective projection. We need current viewport size.
		const int iWdt=Min(iClipX2, RenderTarget->Wdt-1)-iClipX1+1;
		const int iHgt=Min(iClipY2, RenderTarget->Hgt-1)-iClipY1+1;

		// Get away with orthographic projection matrix currently loaded
		glLoadIdentity();

		// Back to GL device coordinates
		glTranslatef(-1.0f, 1.0f, 0.0f);
		glScalef(2.0f/iWdt, -2.0f/iHgt, 1.0f);

		glTranslatef(-iClipX1, -iClipY1, 0.0f);
		if (!ApplyZoomAndTransform(ZoomX, ZoomY, Zoom, pTransform))
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

		glTranslatef(ttx, tty, 0.0f);
		glScalef(((float)ttwdt)/iWdt, ((float)tthgt)/iHgt, 1.0f);

		// Return to Clonk coordinate frame
		glScalef(iWdt/2.0, -iHgt/2.0, 1.0f);
		glTranslatef(1.0f, -1.0f, 0.0f);

		// Fix for the case when we have a non-square target
		const float ta = twdt / thgt;
		const float ma = b / h;
		if(ta <= 1 && ta/ma <= 1)
			glScalef(std::max(ta, ta/ma), std::max(ta, ta/ma), 1.0f);
		else if(ta >= 1 && ta/ma >= 1)
			glScalef(std::max(1.0f/ta, ma/ta), std::max(1.0f/ta, ma/ta), 1.0f);

		// Apply perspective projection. After this, x and y range from
		// -1 to 1, and this is mapped into tx/ty/twdt/thgt by the above code.
		// Aspect is 1.0f which is accounted for above.
		gluPerspective(FOV, 1.0f, 0.1f, 100.0f);
	}

	// Now set up modelview matrix
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	if (fUsePerspective)
	{
		// Setup camera position so that the mesh with uniform transformation
		// fits well into a square target (without distortion).
		const float EyeR = l + std::max(b/TAN_FOV, h/TAN_FOV);
		const float EyeX = MeshCenter.x;
		const float EyeY = MeshCenter.y;
		const float EyeZ = MeshCenter.z + EyeR;

		// Up vector is unit vector in theta direction
		const float UpX = 0;//-sinEyePhi * sinEyeTheta;
		const float UpY = -1;//-cosEyeTheta;
		const float UpZ = 0;//-cosEyePhi * sinEyeTheta;

		// Fix X axis (???)
		glScalef(-1.0f, 1.0f, 1.0f);
		// center on mesh's bounding box, so that the mesh is really in the center of the viewport
		gluLookAt(EyeX, EyeY, EyeZ, MeshCenter.x, MeshCenter.y, MeshCenter.z, UpX, UpY, UpZ);
	}

	// Apply mesh transformation matrix
	if (MeshTransform)
	{
		// Convert to column-major order
		const float Matrix[16] =
		{
			(*MeshTransform)(0,0), (*MeshTransform)(1,0), (*MeshTransform)(2,0), 0,
			(*MeshTransform)(0,1), (*MeshTransform)(1,1), (*MeshTransform)(2,1), 0,
			(*MeshTransform)(0,2), (*MeshTransform)(1,2), (*MeshTransform)(2,2), 0,
			(*MeshTransform)(0,3), (*MeshTransform)(1,3), (*MeshTransform)(2,3), 1
		};

		const float det = MeshTransform->Determinant();
		if (det < 0) parity = !parity;

		// Renormalize if transformation resizes the mesh
		// for lighting to be correct.
		// TODO: Also needs to check for orthonormality to be correct
		if (det != 1 && det != -1)
			glEnable(GL_NORMALIZE);

		// Apply MeshTransformation (in the Mesh's coordinate system)
		glMultMatrixf(Matrix);
	}

	// Convert from Ogre to Clonk coordinate system
	glMultMatrixf(OgreToClonkGL);

	DWORD dwModClr = BlitModulated ? BlitModulateClr : 0xffffffff;

	const C4Rect clipRect = GetClipRect();
	const C4Rect outRect = GetOutRect();
	RenderMeshImpl(instance, dwModClr, dwBlitMode, dwPlayerColor, pFoW, clipRect, outRect, parity);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glActiveTexture(GL_TEXTURE0); // switch back to default
	glClientActiveTexture(GL_TEXTURE0); // switch back to default
	glDepthMask(GL_TRUE);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

	glDisable(GL_NORMALIZE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
	//glDisable(GL_BLEND);

	// TODO: glScissor, so that we only clear the area the mesh covered.
	glClear(GL_DEPTH_BUFFER_BIT);
}

#endif // USE_CONSOLE
