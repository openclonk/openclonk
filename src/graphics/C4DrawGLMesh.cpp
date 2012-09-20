
#include "C4Include.h"
#include <C4DrawGL.h>

#include "C4Surface.h"
#include "StdMesh.h"

inline void SetTexCombine(GLenum combine, StdMeshMaterialTextureUnit::BlendOpExType blendop)
{
	switch (blendop)
	{
	case StdMeshMaterialTextureUnit::BOX_Source1:
	case StdMeshMaterialTextureUnit::BOX_Source2:
		glTexEnvi(GL_TEXTURE_ENV, combine, GL_REPLACE);
		break;
	case StdMeshMaterialTextureUnit::BOX_Modulate:
	case StdMeshMaterialTextureUnit::BOX_ModulateX2:
	case StdMeshMaterialTextureUnit::BOX_ModulateX4:
		glTexEnvi(GL_TEXTURE_ENV, combine, GL_MODULATE);
		break;
	case StdMeshMaterialTextureUnit::BOX_Add:
		glTexEnvi(GL_TEXTURE_ENV, combine, GL_ADD);
		break;
	case StdMeshMaterialTextureUnit::BOX_AddSigned:
		glTexEnvi(GL_TEXTURE_ENV, combine, GL_ADD_SIGNED);
		break;
	case StdMeshMaterialTextureUnit::BOX_AddSmooth:
		// b+c-b*c == a*c + b*(1-c) for a==1.
		glTexEnvi(GL_TEXTURE_ENV, combine, GL_INTERPOLATE);
		break;
	case StdMeshMaterialTextureUnit::BOX_Subtract:
		glTexEnvi(GL_TEXTURE_ENV, combine, GL_SUBTRACT);
		break;
	case StdMeshMaterialTextureUnit::BOX_BlendDiffuseAlpha:
	case StdMeshMaterialTextureUnit::BOX_BlendTextureAlpha:
	case StdMeshMaterialTextureUnit::BOX_BlendCurrentAlpha:
	case StdMeshMaterialTextureUnit::BOX_BlendManual:
		glTexEnvi(GL_TEXTURE_ENV, combine, GL_INTERPOLATE);
		break;
	case StdMeshMaterialTextureUnit::BOX_Dotproduct:
		glTexEnvi(GL_TEXTURE_ENV, combine, GL_DOT3_RGB);
		break;
	case StdMeshMaterialTextureUnit::BOX_BlendDiffuseColor:
		glTexEnvi(GL_TEXTURE_ENV, combine, GL_INTERPOLATE);
		break;
	}
}

inline void SetTexScale(GLenum scale, StdMeshMaterialTextureUnit::BlendOpExType blendop)
{
	switch (blendop)
	{
	case StdMeshMaterialTextureUnit::BOX_Source1:
	case StdMeshMaterialTextureUnit::BOX_Source2:
	case StdMeshMaterialTextureUnit::BOX_Modulate:
		glTexEnvf(GL_TEXTURE_ENV, scale, 1.0f);
		break;
	case StdMeshMaterialTextureUnit::BOX_ModulateX2:
		glTexEnvf(GL_TEXTURE_ENV, scale, 2.0f);
		break;
	case StdMeshMaterialTextureUnit::BOX_ModulateX4:
		glTexEnvf(GL_TEXTURE_ENV, scale, 4.0f);
		break;
	case StdMeshMaterialTextureUnit::BOX_Add:
	case StdMeshMaterialTextureUnit::BOX_AddSigned:
	case StdMeshMaterialTextureUnit::BOX_AddSmooth:
	case StdMeshMaterialTextureUnit::BOX_Subtract:
	case StdMeshMaterialTextureUnit::BOX_BlendDiffuseAlpha:
	case StdMeshMaterialTextureUnit::BOX_BlendTextureAlpha:
	case StdMeshMaterialTextureUnit::BOX_BlendCurrentAlpha:
	case StdMeshMaterialTextureUnit::BOX_BlendManual:
	case StdMeshMaterialTextureUnit::BOX_Dotproduct:
	case StdMeshMaterialTextureUnit::BOX_BlendDiffuseColor:
		glTexEnvf(GL_TEXTURE_ENV, scale, 1.0f);
		break;
	}
}

inline void SetTexSource(GLenum source, StdMeshMaterialTextureUnit::BlendOpSourceType blendsource)
{
	switch (blendsource)
	{
	case StdMeshMaterialTextureUnit::BOS_Current:
		glTexEnvi(GL_TEXTURE_ENV, source, GL_PREVIOUS);
		break;
	case StdMeshMaterialTextureUnit::BOS_Texture:
		glTexEnvi(GL_TEXTURE_ENV, source, GL_TEXTURE);
		break;
	case StdMeshMaterialTextureUnit::BOS_Diffuse:
	case StdMeshMaterialTextureUnit::BOS_Specular:
		glTexEnvi(GL_TEXTURE_ENV, source, GL_PRIMARY_COLOR);
		break;
	case StdMeshMaterialTextureUnit::BOS_PlayerColor:
		glTexEnvi(GL_TEXTURE_ENV, source, GL_CONSTANT);
		break;
	case StdMeshMaterialTextureUnit::BOS_Manual:
		glTexEnvi(GL_TEXTURE_ENV, source, GL_CONSTANT);
		break;
	}
}

inline void SetTexSource2(GLenum source, StdMeshMaterialTextureUnit::BlendOpExType blendop)
{
	// Set Arg2 for interpolate (Arg0 for BOX_Add_Smooth)
	switch (blendop)
	{
	case StdMeshMaterialTextureUnit::BOX_AddSmooth:
		glTexEnvi(GL_TEXTURE_ENV, source, GL_CONSTANT); // 1.0, Set in SetTexColor
		break;
	case StdMeshMaterialTextureUnit::BOX_BlendDiffuseAlpha:
		glTexEnvi(GL_TEXTURE_ENV, source, GL_PRIMARY_COLOR);
		break;
	case StdMeshMaterialTextureUnit::BOX_BlendTextureAlpha:
		glTexEnvi(GL_TEXTURE_ENV, source, GL_TEXTURE);
		break;
	case StdMeshMaterialTextureUnit::BOX_BlendCurrentAlpha:
		glTexEnvi(GL_TEXTURE_ENV, source, GL_PREVIOUS);
		break;
	case StdMeshMaterialTextureUnit::BOX_BlendManual:
		glTexEnvi(GL_TEXTURE_ENV, source, GL_CONSTANT); // Set in SetTexColor
		break;
	case StdMeshMaterialTextureUnit::BOX_BlendDiffuseColor:
		// difference to BOX_Blend_Diffuse_Alpha is operand, see SetTexOperand2
		glTexEnvi(GL_TEXTURE_ENV, source, GL_PRIMARY_COLOR);
		break;
	default:
		// TODO
		break;
	}
}

inline void SetTexOperand2(GLenum operand, StdMeshMaterialTextureUnit::BlendOpExType blendop)
{
	switch (blendop)
	{
	case StdMeshMaterialTextureUnit::BOX_Add:
	case StdMeshMaterialTextureUnit::BOX_AddSigned:
	case StdMeshMaterialTextureUnit::BOX_AddSmooth:
	case StdMeshMaterialTextureUnit::BOX_Subtract:
		glTexEnvi(GL_TEXTURE_ENV, operand, GL_SRC_COLOR);
		break;
	case StdMeshMaterialTextureUnit::BOX_BlendDiffuseAlpha:
	case StdMeshMaterialTextureUnit::BOX_BlendTextureAlpha:
	case StdMeshMaterialTextureUnit::BOX_BlendCurrentAlpha:
	case StdMeshMaterialTextureUnit::BOX_BlendManual:
		glTexEnvi(GL_TEXTURE_ENV, operand, GL_SRC_ALPHA);
		break;
	case StdMeshMaterialTextureUnit::BOX_Dotproduct:
	case StdMeshMaterialTextureUnit::BOX_BlendDiffuseColor:
		glTexEnvi(GL_TEXTURE_ENV, operand, GL_SRC_COLOR);
		break;
	default:
		// TODO
		break;
	}
}

inline void SetTexColor(const StdMeshMaterialTextureUnit& texunit, DWORD PlayerColor)
{
	float Color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	if (texunit.ColorOpEx == StdMeshMaterialTextureUnit::BOX_AddSmooth)
	{
		Color[0] = Color[1] = Color[2] = 1.0f;
	}
	else if (texunit.ColorOpEx == StdMeshMaterialTextureUnit::BOX_BlendManual)
	{
		// operand of GL_CONSTANT is set to alpha for this blend mode
		// see SetTexOperand2
		Color[3] = texunit.ColorOpManualFactor;
	}
	else if (texunit.ColorOpSources[0] == StdMeshMaterialTextureUnit::BOS_PlayerColor || texunit.ColorOpSources[1] == StdMeshMaterialTextureUnit::BOS_PlayerColor)
	{
		Color[0] = ((PlayerColor >> 16) & 0xff) / 255.0f;
		Color[1] = ((PlayerColor >>  8) & 0xff) / 255.0f;
		Color[2] = ((PlayerColor      ) & 0xff) / 255.0f;
	}
	else if (texunit.ColorOpSources[0] == StdMeshMaterialTextureUnit::BOS_Manual)
	{
		Color[0] = texunit.ColorOpManualColor1[0];
		Color[1] = texunit.ColorOpManualColor1[1];
		Color[2] = texunit.ColorOpManualColor1[2];
	}
	else if (texunit.ColorOpSources[1] == StdMeshMaterialTextureUnit::BOS_Manual)
	{
		Color[0] = texunit.ColorOpManualColor2[0];
		Color[1] = texunit.ColorOpManualColor2[1];
		Color[2] = texunit.ColorOpManualColor2[2];
	}

	if (texunit.AlphaOpEx == StdMeshMaterialTextureUnit::BOX_AddSmooth)
		Color[3] = 1.0f;
	else if (texunit.AlphaOpEx == StdMeshMaterialTextureUnit::BOX_BlendManual)
		Color[3] = texunit.AlphaOpManualFactor;
	else if (texunit.AlphaOpSources[0] == StdMeshMaterialTextureUnit::BOS_PlayerColor || texunit.AlphaOpSources[1] == StdMeshMaterialTextureUnit::BOS_PlayerColor)
		Color[3] = ((PlayerColor >> 24) & 0xff) / 255.0f;
	else if (texunit.AlphaOpSources[0] == StdMeshMaterialTextureUnit::BOS_Manual)
		Color[3] = texunit.AlphaOpManualAlpha1;
	else if (texunit.AlphaOpSources[1] == StdMeshMaterialTextureUnit::BOS_Manual)
		Color[3] = texunit.AlphaOpManualAlpha2;

	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, Color);
}

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

static void RenderSubMeshImpl(const StdMeshInstance& mesh_instance, const StdSubMeshInstance& instance, DWORD dwModClr, DWORD dwBlitMode, DWORD dwPlayerColor, bool parity)
{
	const StdMeshMaterial& material = instance.GetMaterial();
	assert(material.BestTechniqueIndex != -1);
	const StdMeshMaterialTechnique& technique = material.Techniques[material.BestTechniqueIndex];
	const StdMeshVertex* vertices = instance.GetVertices().empty() ? &mesh_instance.GetSharedVertices()[0] : &instance.GetVertices()[0];

	// Render each pass
	for (unsigned int i = 0; i < technique.Passes.size(); ++i)
	{
		const StdMeshMaterialPass& pass = technique.Passes[i];

		glDepthMask(pass.DepthWrite ? GL_TRUE : GL_FALSE);

		if(pass.AlphaToCoverage)
			glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
		else
			glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);

		// Apply ClrMod to material
		// TODO: ClrModMap is not taken into account by this; we should just check
		// mesh center.
		// TODO: Or in case we have shaders enabled use the shader... note the
		// clrmodmap texture needs to be the last texture in that case... we should
		// change the index to maxtextures-1 instead of 3.

		const float dwMod[4] = {
			((dwModClr >> 16) & 0xff) / 255.0f,
			((dwModClr >>  8) & 0xff) / 255.0f,
			((dwModClr      ) & 0xff) / 255.0f,
			((dwModClr >> 24) & 0xff) / 255.0f
		};

		if(!(dwBlitMode & C4GFXBLIT_MOD2) && dwModClr == 0xffffffff)
		{
			// Fastpath for the easy case
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, pass.Ambient);
			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, pass.Diffuse);
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, pass.Specular);
			glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, pass.Emissive);
			glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, pass.Shininess);
		}
		else
		{
			float Ambient[4], Diffuse[4], Specular[4], Emissive[4];

			// TODO: We could also consider applying dwmod using an additional
			// texture unit, maybe we can even re-use the one which is reserved for
			// the clrmodmap texture anyway (+adapt the shader).
			if(!(dwBlitMode & C4GFXBLIT_MOD2))
			{
				Ambient[0] = pass.Ambient[0] * dwMod[0];
				Ambient[1] = pass.Ambient[1] * dwMod[1];
				Ambient[2] = pass.Ambient[2] * dwMod[2];
				Ambient[3] = pass.Ambient[3] * dwMod[3];

				Diffuse[0] = pass.Diffuse[0] * dwMod[0];
				Diffuse[1] = pass.Diffuse[1] * dwMod[1];
				Diffuse[2] = pass.Diffuse[2] * dwMod[2];
				Diffuse[3] = pass.Diffuse[3] * dwMod[3];

				Specular[0] = pass.Specular[0] * dwMod[0];
				Specular[1] = pass.Specular[1] * dwMod[1];
				Specular[2] = pass.Specular[2] * dwMod[2];
				Specular[3] = pass.Specular[3] * dwMod[3];

				Emissive[0] = pass.Emissive[0] * dwMod[0];
				Emissive[1] = pass.Emissive[1] * dwMod[1];
				Emissive[2] = pass.Emissive[2] * dwMod[2];
				Emissive[3] = pass.Emissive[3] * dwMod[3];
			}
			else
			{
				// The RGB part for fMod2 drawing is set in the texture unit,
				// since its effect cannot be achieved properly by playing with
				// the material color.
				// TODO: This should go into an additional texture unit.
				Ambient[0] = pass.Ambient[0];
				Ambient[1] = pass.Ambient[1];
				Ambient[2] = pass.Ambient[2];
				Ambient[3] = pass.Ambient[3];

				Diffuse[0] = pass.Diffuse[0];
				Diffuse[1] = pass.Diffuse[1];
				Diffuse[2] = pass.Diffuse[2];
				Diffuse[3] = pass.Diffuse[3];

				Specular[0] = pass.Specular[0];
				Specular[1] = pass.Specular[1];
				Specular[2] = pass.Specular[2];
				Specular[3] = pass.Specular[3];

				Emissive[0] = pass.Emissive[0];
				Emissive[1] = pass.Emissive[1];
				Emissive[2] = pass.Emissive[2];
				Emissive[3] = pass.Emissive[3];
			}

			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, Ambient);
			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, Diffuse);
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, Specular);
			glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, Emissive);
			glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, pass.Shininess);
		}

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
		GLuint have_texture = 0;
		for (unsigned int j = 0; j < pass.TextureUnits.size(); ++j)
		{
			// Note that it is guaranteed that the GL_TEXTUREn
			// constants are contiguous.
			glActiveTexture(GL_TEXTURE0+j);
			glClientActiveTexture(GL_TEXTURE0+j);

			const StdMeshMaterialTextureUnit& texunit = pass.TextureUnits[j];

			glEnable(GL_TEXTURE_2D);
			if (texunit.HasTexture())
			{
				const unsigned int Phase = instance.GetTexturePhase(i, j);
				have_texture = texunit.GetTexture(Phase).texName;
				glBindTexture(GL_TEXTURE_2D, texunit.GetTexture(Phase).texName);
			}
			else
			{
				// We need to bind a valid texture here, even if the texture unit
				// does not access the texture.
				// TODO: Could use StdGL::lines_tex... this function should be a
				// member of StdGL anyway.
				glBindTexture(GL_TEXTURE_2D, have_texture);
			}
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

			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

			bool fMod2 = (dwBlitMode & C4GFXBLIT_MOD2) != 0;

			// Overwrite texcombine and texscale for fMod2 drawing.
			// TODO: Use an additional texture unit or a shader to do this,
			// so that the settings of this texture unit are not lost.

			if(fMod2)
			{
				// Special case RGBA setup for fMod2
				glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
				glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD_SIGNED);
				glTexEnvf(GL_TEXTURE_ENV, GL_ALPHA_SCALE, 1.0f);
				glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE, 2.0f);
				SetTexSource(GL_SOURCE0_RGB, texunit.ColorOpSources[0]); // TODO: Fails for StdMeshMaterialTextureUnit::BOX_Source2
				glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_CONSTANT);
				glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
				glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
				SetTexSource(GL_SOURCE0_ALPHA, texunit.AlphaOpSources[0]);
				glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_CONSTANT);
				glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
				glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
				glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, dwMod);
			}
			else
			{
				// Combine
				SetTexCombine(GL_COMBINE_RGB, texunit.ColorOpEx);
				SetTexCombine(GL_COMBINE_ALPHA, texunit.AlphaOpEx);

				// Scale
				SetTexScale(GL_RGB_SCALE, texunit.ColorOpEx);
				SetTexScale(GL_ALPHA_SCALE, texunit.AlphaOpEx);

				if (texunit.ColorOpEx == StdMeshMaterialTextureUnit::BOX_Source2)
				{
					SetTexSource(GL_SOURCE0_RGB, texunit.ColorOpSources[1]);
					glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
				}
				else
				{
					if (texunit.ColorOpEx == StdMeshMaterialTextureUnit::BOX_AddSmooth)
					{
						// GL_SOURCE0 is GL_CONSTANT to achieve the desired effect with GL_INTERPOLATE
						SetTexSource2(GL_SOURCE0_RGB, texunit.ColorOpEx);
						SetTexSource(GL_SOURCE1_RGB, texunit.ColorOpSources[0]);
						SetTexSource(GL_SOURCE2_RGB, texunit.ColorOpSources[1]);

						SetTexOperand2(GL_OPERAND0_RGB, texunit.ColorOpEx);
						glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
						glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_COLOR);
					}
					else
					{
						SetTexSource(GL_SOURCE0_RGB, texunit.ColorOpSources[0]);
						glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);

						if (texunit.ColorOpEx != StdMeshMaterialTextureUnit::BOX_Source1)
						{
							SetTexSource(GL_SOURCE1_RGB, texunit.ColorOpSources[1]);
							glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
						}

						SetTexSource2(GL_SOURCE2_RGB, texunit.ColorOpEx);
						SetTexOperand2(GL_OPERAND2_RGB, texunit.ColorOpEx);
					}
				}

				if (texunit.AlphaOpEx == StdMeshMaterialTextureUnit::BOX_Source2)
				{
					SetTexSource(GL_SOURCE0_ALPHA, texunit.AlphaOpSources[1]);
					glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
				}
				else
				{
					if (texunit.AlphaOpEx == StdMeshMaterialTextureUnit::BOX_AddSmooth)
					{
						// GL_SOURCE0 is GL_CONSTANT to achieve the desired effect with GL_INTERPOLATE
						SetTexSource2(GL_SOURCE0_ALPHA, texunit.AlphaOpEx);
						SetTexSource(GL_SOURCE1_ALPHA, texunit.AlphaOpSources[0]);
						SetTexSource(GL_SOURCE2_ALPHA, texunit.AlphaOpSources[1]);

						glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
						glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
						glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_ALPHA, GL_SRC_ALPHA);
					}
					else
					{
						SetTexSource(GL_SOURCE0_ALPHA, texunit.AlphaOpSources[0]);
						glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);

						if (texunit.AlphaOpEx != StdMeshMaterialTextureUnit::BOX_Source1)
						{
							SetTexSource(GL_SOURCE1_ALPHA, texunit.AlphaOpSources[1]);
							glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
						}

						SetTexSource2(GL_SOURCE2_ALPHA, texunit.AlphaOpEx);
						glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_ALPHA, GL_SRC_ALPHA);
					}
				}

				SetTexColor(texunit, dwPlayerColor);
			}
		}

		glMatrixMode(GL_MODELVIEW);

		glDrawElements(GL_TRIANGLES, instance.GetNumFaces()*3, GL_UNSIGNED_INT, instance.GetFaces());

		for (unsigned int j = 0; j < pass.TextureUnits.size(); ++j)
		{
			glActiveTexture(GL_TEXTURE0+j);
			glClientActiveTexture(GL_TEXTURE0+j);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			glDisable(GL_TEXTURE_2D);
		}
	}
}

static void RenderMeshImpl(StdMeshInstance& instance, DWORD dwModClr, DWORD dwBlitMode, DWORD dwPlayerColor, bool parity); // Needed by RenderAttachedMesh

static void RenderAttachedMesh(StdMeshInstance::AttachedMesh* attach, DWORD dwModClr, DWORD dwBlitMode, DWORD dwPlayerColor, bool parity)
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
		const StdMeshMatrix& own_trans = instance.GetBoneTransform(attach->ParentBone)
		                                 * StdMeshMatrix::Transform(instance.Mesh.GetBone(attach->ParentBone).Transformation);

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

static void RenderMeshImpl(StdMeshInstance& instance, DWORD dwModClr, DWORD dwBlitMode, DWORD dwPlayerColor, bool parity)
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
		RenderSubMeshImpl(instance, instance.GetSubMesh(i), dwModClr, dwBlitMode, dwPlayerColor, parity);

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
		const StdMeshMatrix& own_trans = instance.GetBoneTransform(attached->ChildBone) * StdMeshMatrix::Transform(instance.Mesh.GetBone(attached->ChildBone).Transformation);

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
static bool ApplyZoomAndTransform(float ZoomX, float ZoomY, float Zoom, C4BltTransform* pTransform)
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
		const float rx = -std::min(v1.x,v1.y) / fabs(v2.x - v1.x);
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
