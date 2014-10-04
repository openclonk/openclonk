/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
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

/* OpenGL implementation of Mesh Rendering */

#include "C4Include.h"
#include <C4DrawGL.h>

#include "StdMesh.h"

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
		case StdMeshMaterialTextureUnit::BOS_Texture: return FormatString("texture2D(oc_Textures[%d], texcoord)", index);
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
		case StdMeshMaterialTextureUnit::BOX_BlendTextureAlpha: return FormatString("texture2D(oc_Textures[%d], texcoord).a * %s + (1.0 - texture2D(oc_Textures[%d], texcoord).a) * %s", index, source1, index, source2);
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

		return FormatString("currentColor = clamp(vec4(%s, %s), 0.0, 1.0);", TextureUnitBlendToCode(index, texunit.ColorOpEx, color_source1.getData(), color_source2.getData(), texunit.ColorOpManualFactor).getData(), TextureUnitBlendToCode(index, texunit.AlphaOpEx, alpha_source1.getData(), alpha_source2.getData(), texunit.AlphaOpManualFactor).getData());
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
} // anonymous namespace

class C4DrawMeshGLShader: public StdMeshMaterialPass::Shader
{
public:
	C4DrawMeshGLShader(const StdMeshMaterialPass& pass);
	virtual ~C4DrawMeshGLShader();

	class Error
	{
	public:
		Error(const StdStrBuf& buf): Message(buf) {}
		StdCopyStrBuf Message;
	};

	class ShaderObject
	{
		friend class C4DrawMeshGLShader;
	public:
		ShaderObject(GLint shader_type);
		~ShaderObject();

		void Load(const char* code);
	private:
		GLuint Shader;
	};

	GLuint Program;

	GLint TexturesLocation;
	GLint PlayerColorLocation;
	GLint ColorModulationLocation;
	GLint Mod2Location;
};

C4DrawMeshGLShader::ShaderObject::ShaderObject(GLint shader_type)
{
	Shader = glCreateShaderObjectARB(shader_type);
	if(!Shader) throw Error(StdStrBuf("Failed to create shader"));
}

C4DrawMeshGLShader::ShaderObject::~ShaderObject()
{
	glDeleteObjectARB(Shader);
}

void C4DrawMeshGLShader::ShaderObject::Load(const char* code)
{
	glShaderSourceARB(Shader, 1, &code, NULL);
	glCompileShaderARB(Shader);

	GLint compile_status;
	glGetObjectParameterivARB(Shader, GL_OBJECT_COMPILE_STATUS_ARB, &compile_status);
	if(compile_status != GL_TRUE)
	{
		GLint shader_type;
		glGetObjectParameterivARB(Shader, GL_OBJECT_SUBTYPE_ARB, &shader_type);
		const char* shader_type_str;
		switch(shader_type)
		{
		case GL_VERTEX_SHADER_ARB: shader_type_str = "vertex"; break;
		case GL_FRAGMENT_SHADER_ARB: shader_type_str = "fragment"; break;
		//case GL_GEOMETRY_SHADER_ARB: shader_type_str = "geometry"; break;
		default: assert(false); break;
		}

		GLint length;
		glGetObjectParameterivARB(Shader, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
		if(length > 0)
		{
			std::vector<char> error_message(length);
			glGetInfoLogARB(Shader, length, NULL, &error_message[0]);
			throw Error(FormatString("Failed to compile %s shader: %s", shader_type_str, &error_message[0]));
		}
		else
		{
			throw Error(FormatString("Failed to compile %s shader", shader_type_str));
		}
	}
}

C4DrawMeshGLShader::C4DrawMeshGLShader(const StdMeshMaterialPass& pass)
{
	ShaderObject FragmentShader(GL_FRAGMENT_SHADER_ARB);
	ShaderObject VertexShader(GL_VERTEX_SHADER_ARB);

	VertexShader.Load(
		"varying vec4 diffuse;"
		"varying vec2 texcoord;"
		"void main()"
		"{"
		"  vec3 normal = normalize(gl_NormalMatrix * gl_Normal);" // TODO: Do we need to normalize? I think we enable GL_NORMALIZE in cases we have to...
		"  vec3 lightDir = normalize(gl_LightSource[0].position.xyz);" // TODO: Do we need to normalize?
		"  diffuse = clamp(gl_FrontLightModelProduct.sceneColor + gl_FrontLightProduct[0].ambient + gl_FrontLightProduct[0].diffuse * max(0.0, dot(normal, lightDir)), 0.0, 1.0);"
		"  texcoord = gl_MultiTexCoord0.xy;"
		"  gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;"
		"}"
	);

	// Produce the fragment shader... first we create one code fragment for each
	// texture unit, and we count the number of active textures, i.e. texture
	// units that actually use a texture.
	unsigned int texIndex = 0;
	StdStrBuf textureUnitCode("");
	for(unsigned int i = 0; i < pass.TextureUnits.size(); ++i)
	{
		const StdMeshMaterialTextureUnit& texunit = pass.TextureUnits[i];
		textureUnitCode.Append(TextureUnitToCode(texIndex, texunit));

		if(texunit.HasTexture())
			++texIndex;
	}

	FragmentShader.Load(
		FormatString(
			"varying vec4 diffuse;"
			"varying vec2 texcoord;"
			"%s" // Texture units with active textures, only if >0 texture units
			"uniform vec3 oc_PlayerColor;"
			"uniform vec4 oc_ColorModulation;"
			"uniform int oc_Mod2;"
			"void main()"
			"{"
			"  vec4 currentColor = diffuse;"
			"  %s"
			"  if(oc_Mod2 != 0)"
			"    gl_FragColor = clamp(2.0 * currentColor * oc_ColorModulation - 0.5, 0.0, 1.0);"
			"  else"
			"    gl_FragColor = currentColor * oc_ColorModulation;"
			"}",
			((texIndex > 0) ? FormatString("uniform sampler2D oc_Textures[%d];", texIndex).getData() : ""),
			textureUnitCode.getData()
		).getData()
	);

	Program = glCreateProgramObjectARB();
	glAttachObjectARB(Program, VertexShader.Shader);
	glAttachObjectARB(Program, FragmentShader.Shader);
	glLinkProgramARB(Program);

	GLint link_status;
	glGetObjectParameterivARB(Program, GL_OBJECT_LINK_STATUS_ARB, &link_status);
	if(link_status != GL_TRUE)
	{
		GLint length;
		glGetObjectParameterivARB(Program, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
		if(length > 0)
		{
			std::vector<char> error_message(length);
			glGetInfoLogARB(Program, length, NULL, &error_message[0]);
			glDeleteObjectARB(Program);
			throw Error(FormatString("Failed to link program: %s", &error_message[0]));
		}
		else
		{
			glDeleteObjectARB(Program);
			throw Error(StdStrBuf("Failed to link program"));
		}
	}

	TexturesLocation = glGetUniformLocationARB(Program, "oc_Textures");
	PlayerColorLocation = glGetUniformLocationARB(Program, "oc_PlayerColor");
	ColorModulationLocation = glGetUniformLocationARB(Program, "oc_ColorModulation");
	Mod2Location = glGetUniformLocationARB(Program, "oc_Mod2");
}

C4DrawMeshGLShader::~C4DrawMeshGLShader()
{
	glDeleteObjectARB(Program);
}

bool CStdGL::PrepareMaterial(StdMeshMaterial& mat)
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

			try
			{
				assert(pass.Program == NULL);
				pass.Program = new C4DrawMeshGLShader(pass);
			}
			catch(const C4DrawMeshGLShader::Error& error)
			{
				technique.Available = false;
				LogF("Failed to compile shader: %s\n", error.Message.getData());
			}
		}

		if (technique.Available && mat.BestTechniqueIndex == -1)
			mat.BestTechniqueIndex = i;
	}

	return mat.BestTechniqueIndex != -1;
}

namespace
{
	void RenderSubMeshImpl(const StdMeshInstance& mesh_instance, const StdSubMeshInstance& instance, DWORD dwModClr, DWORD dwBlitMode, DWORD dwPlayerColor, bool parity)
	{
		const StdMeshMaterial& material = instance.GetMaterial();
		assert(material.BestTechniqueIndex != -1);
		const StdMeshMaterialTechnique& technique = material.Techniques[material.BestTechniqueIndex];
		const StdMeshVertex* vertices = instance.GetVertices().empty() ? &mesh_instance.GetSharedVertices()[0] : &instance.GetVertices()[0];

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

			// Use two-sided light model so that vertex normals are inverted for lighting calculation on back-facing polygons
			glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);
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

			// TODO: Use vbo if available.

			glVertexPointer(3, GL_FLOAT, sizeof(StdMeshVertex), &vertices->x);
			glNormalPointer(GL_FLOAT, sizeof(StdMeshVertex), &vertices->nx);

			glMatrixMode(GL_TEXTURE);

			std::vector<GLint> textures;
			textures.reserve(pass.TextureUnits.size());
			for (unsigned int j = 0; j < pass.TextureUnits.size(); ++j)
			{
				const StdMeshMaterialTextureUnit& texunit = pass.TextureUnits[j];
				const unsigned int texIndex = textures.size();

				if (texunit.HasTexture())
				{
					// Array with texture indices set for passing the textures to the
					// shader -- shader cannot use fixed texture image units before OGL 4.2.
					textures.push_back(texIndex);

					// Note that it is guaranteed that the GL_TEXTUREn
					// constants are contiguous.
					glActiveTexture(GL_TEXTURE0+texIndex);
					glClientActiveTexture(GL_TEXTURE0+texIndex);
					glEnable(GL_TEXTURE_2D);

					const unsigned int Phase = instance.GetTexturePhase(i, j);
					glBindTexture(GL_TEXTURE_2D, texunit.GetTexture(Phase).texName);

					glEnableClientState(GL_TEXTURE_COORD_ARRAY);
					glTexCoordPointer(2, GL_FLOAT, sizeof(StdMeshVertex), &vertices->u);

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

			glMatrixMode(GL_MODELVIEW);

			const float dwMod[4] = {
				((dwModClr >> 16) & 0xff) / 255.0f,
				((dwModClr >>  8) & 0xff) / 255.0f,
				((dwModClr      ) & 0xff) / 255.0f,
				((dwModClr >> 24) & 0xff) / 255.0f
			};
			
			const float dwPlrClr[3] = {
				((dwPlayerColor >> 16) & 0xff) / 255.0f,
				((dwPlayerColor >>  8) & 0xff) / 255.0f,
				((dwPlayerColor      ) & 0xff) / 255.0f
			};

			const int fMod2 = (dwBlitMode & C4GFXBLIT_MOD2) != 0;

			const C4DrawMeshGLShader& shader = static_cast<const C4DrawMeshGLShader&>(*pass.Program);

			glUseProgramObjectARB(shader.Program);
			if(shader.TexturesLocation != -1 && !textures.empty()) glUniform1ivARB(shader.TexturesLocation, textures.size(), &textures[0]);
			if(shader.PlayerColorLocation != -1) glUniform3fvARB(shader.PlayerColorLocation, 1, dwPlrClr);
			if(shader.ColorModulationLocation != -1) glUniform4fvARB(shader.ColorModulationLocation, 1, dwMod);
			if(shader.Mod2Location != -1) glUniform1iARB(shader.Mod2Location, fMod2);
			glDrawElements(GL_TRIANGLES, instance.GetNumFaces()*3, GL_UNSIGNED_INT, instance.GetFaces());

			// Clean-up, re-set default state
			for (unsigned int j = 0; j < textures.size(); ++j)
			{
				glActiveTexture(GL_TEXTURE0+textures[j]);
				glClientActiveTexture(GL_TEXTURE0+textures[j]);
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
				glDisable(GL_TEXTURE_2D);
			}

			if(!pass.DepthCheck)
				glEnable(GL_DEPTH_TEST);
		}
	}

	void RenderMeshImpl(StdMeshInstance& instance, DWORD dwModClr, DWORD dwBlitMode, DWORD dwPlayerColor, bool parity); // Needed by RenderAttachedMesh

	void RenderAttachedMesh(StdMeshInstance::AttachedMesh* attach, DWORD dwModClr, DWORD dwBlitMode, DWORD dwPlayerColor, bool parity)
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

		// TODO: Take attach transform's parity into account
		glPushMatrix();
		glMultMatrixf(attach_trans_gl);
		RenderMeshImpl(*attach->Child, dwModClr, dwBlitMode, dwPlayerColor, parity);
		glPopMatrix();

#if 0
			const StdMeshMatrix& own_trans = attach->Parent->GetBoneTransform(attach->ParentBone)
			                                 * StdMeshMatrix::Transform(attach->Parent->GetMesh().GetBone(attach->ParentBone).Transformation);

			// Draw attached bone
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_LIGHTING);
			glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
			GLUquadric* quad = gluNewQuadric();
			glPushMatrix();
			glTranslatef(own_trans(0,3), own_trans(1,3), own_trans(2,3));
			gluSphere(quad, 1.0f, 4, 4);
			glPopMatrix();
			gluDeleteQuadric(quad);
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_LIGHTING);
#endif
	}

	void RenderMeshImpl(StdMeshInstance& instance, DWORD dwModClr, DWORD dwBlitMode, DWORD dwPlayerColor, bool parity)
	{
		const StdMesh& mesh = instance.GetMesh();

		// Render AM_DrawBefore attached meshes
		StdMeshInstance::AttachedMeshIter attach_iter = instance.AttachedMeshesBegin();

		for (; attach_iter != instance.AttachedMeshesEnd() && ((*attach_iter)->GetFlags() & StdMeshInstance::AM_DrawBefore); ++attach_iter)
			RenderAttachedMesh(*attach_iter, dwModClr, dwBlitMode, dwPlayerColor, parity);

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
			RenderSubMeshImpl(instance, instance.GetSubMeshOrdered(i), dwModClr, dwBlitMode, dwPlayerColor, parity);

		// reset old mode to prevent rendering errors
		if(dwBlitMode & C4GFXBLIT_WIREFRAME)
		{
			glPolygonMode(GL_FRONT, modes[0]);
			glPolygonMode(GL_BACK, modes[1]);
		}

#if 0
		// Draw attached bone
		if (instance.GetAttachParent())
		{
			const StdMeshInstance::AttachedMesh* attached = instance.GetAttachParent();
			const StdMeshMatrix& own_trans = instance.GetBoneTransform(attached->ChildBone) * StdMeshMatrix::Transform(instance.GetMesh().GetBone(attached->ChildBone).Transformation);

			glDisable(GL_DEPTH_TEST);
			glDisable(GL_LIGHTING);
			glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
			GLUquadric* quad = gluNewQuadric();
			glPushMatrix();
			glTranslatef(own_trans(0,3), own_trans(1,3), own_trans(2,3));
			gluSphere(quad, 1.0f, 4, 4);
			glPopMatrix();
			gluDeleteQuadric(quad);
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_LIGHTING);
		}
#endif

		// Render non-AM_DrawBefore attached meshes
		for (; attach_iter != instance.AttachedMeshesEnd(); ++attach_iter)
			RenderAttachedMesh(*attach_iter, dwModClr, dwBlitMode, dwPlayerColor, parity);
	}

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

	glShadeModel(GL_SMOOTH);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_BLEND); // TODO: Shouldn't this always be enabled? - blending does not work for meshes without this though.

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY); // might still be active from a previous (non-mesh-rendering) GL operation
	glClientActiveTexture(GL_TEXTURE0);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY); // same -- we enable this individually for every texture unit in RenderSubMeshImpl

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

	if (!fUsePerspective)
	{
		// Put a light source in front of the object
		const GLfloat light_position[] = { 0.0f, 0.0f, 1.0f, 0.0f };
		glLightfv(GL_LIGHT0, GL_POSITION, light_position);
		glEnable(GL_LIGHT0);
	}
	else
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

		// Apply lighting (light source at camera position)
		const GLfloat light_position[] = { EyeX, EyeY, EyeZ, 1.0f };
		glLightfv(GL_LIGHT0, GL_POSITION, light_position);
		glEnable(GL_LIGHT0);

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

	if(fUseClrModMap)
	{
		float x = tx + twdt/2.0f;
		float y = ty + thgt/2.0f;

		if(pTransform)
			pTransform->TransformPoint(x,y);

		ApplyZoom(x, y);
		DWORD c = pClrModMap->GetModAt(int(x), int(y));
		ModulateClr(dwModClr, c);
	}

	RenderMeshImpl(instance, dwModClr, dwBlitMode, dwPlayerColor, parity);

	glUseProgramObjectARB(0);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glActiveTexture(GL_TEXTURE0); // switch back to default
	glClientActiveTexture(GL_TEXTURE0); // switch back to default
	glDepthMask(GL_TRUE);

	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

	glDisable(GL_NORMALIZE);
	glDisable(GL_LIGHT0);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
	//glDisable(GL_BLEND);
	glShadeModel(GL_FLAT);

	// TODO: glScissor, so that we only clear the area the mesh covered.
	glClear(GL_DEPTH_BUFFER_BIT);
}

#endif // USE_CONSOLE
