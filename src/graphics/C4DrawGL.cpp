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

/* OpenGL implementation of NewGfx */

#include "C4Include.h"
#include "C4ForbidLibraryCompilation.h"
#include "graphics/C4DrawGL.h"

#include "game/C4Application.h"
#include "graphics/C4Surface.h"
#include "landscape/fow/C4FoWRegion.h"
#include "lib/C4Rect.h"
#include "lib/StdColors.h"
#include "platform/C4Window.h"

#ifndef USE_CONSOLE

namespace
{
	const char *MsgSourceToStr(GLenum source)
	{
		switch (source)
		{
		case GL_DEBUG_SOURCE_API_ARB: return "API";
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB: return "window system";
		case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB: return "shader compiler";
		case GL_DEBUG_SOURCE_THIRD_PARTY_ARB: return "third party";
		case GL_DEBUG_SOURCE_APPLICATION_ARB: return "application";
		case GL_DEBUG_SOURCE_OTHER_ARB: return "other";
		default: return "<unknown>";
		}
	}

	const char *MsgTypeToStr(GLenum type)
	{
		switch (type)
		{
		case GL_DEBUG_TYPE_ERROR_ARB: return "error";
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB: return "deprecation warning";
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB: return "undefined behavior warning";
		case GL_DEBUG_TYPE_PORTABILITY_ARB: return "portability warning";
		case GL_DEBUG_TYPE_PERFORMANCE_ARB: return "performance warning";
		case GL_DEBUG_TYPE_OTHER_ARB: return "other message";
		default: return "unknown message";
		}
	}

	const char *MsgSeverityToStr(GLenum severity)
	{
		switch (severity)
		{
		case GL_DEBUG_SEVERITY_HIGH_ARB: return "high";
		case GL_DEBUG_SEVERITY_MEDIUM_ARB: return "medium";
		case GL_DEBUG_SEVERITY_LOW_ARB: return "low";
#ifdef GL_DEBUG_SEVERITY_NOTIFICATION
		case GL_DEBUG_SEVERITY_NOTIFICATION: return "notification";
#endif
		default: return "<unknown>";
		}
	}

	void GLAPIENTRY OpenGLDebugProc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* userParam)
	{
		const char *msg_source = MsgSourceToStr(source);
		const char *msg_type = MsgTypeToStr(type);
		const char *msg_severity = MsgSeverityToStr(severity);

		LogSilentF("  gl: %s severity %s %s: %s", msg_severity, msg_source, msg_type, message);
#ifdef USE_WIN32_WINDOWS
		if (IsDebuggerPresent() && severity == GL_DEBUG_SEVERITY_HIGH_ARB)
			BREAKPOINT_HERE;
#endif
	}
}

CStdGL::CStdGL():
	NextVAOID(VAOIDs.end())
{
	GenericVBOs[0] = 0;
	Default();
	// global ptr
	pGL = this;
	lines_tex = 0;
}

CStdGL::~CStdGL()
{
	Clear();
	pGL=nullptr;
}

void CStdGL::Clear()
{
	NoPrimaryClipper();
	// cannot unlock TexMgr here or we can't preserve textures across GL reinitialization as required when changing multisampling
	InvalidateDeviceObjects();
	NoPrimaryClipper();
	RenderTarget = nullptr;
	// Clear all shaders
	SpriteShader.Clear();
	SpriteShaderMod2.Clear();
	SpriteShaderBase.Clear();
	SpriteShaderBaseMod2.Clear();
	SpriteShaderBaseOverlay.Clear();
	SpriteShaderBaseOverlayMod2.Clear();
	SpriteShaderLight.Clear();
	SpriteShaderLightMod2.Clear();
	SpriteShaderLightBase.Clear();
	SpriteShaderLightBaseMod2.Clear();
	SpriteShaderLightBaseOverlay.Clear();
	SpriteShaderLightBaseOverlayMod2.Clear();
	SpriteShaderLightBaseNormal.Clear();
	SpriteShaderLightBaseNormalMod2.Clear();
	SpriteShaderLightBaseNormalOverlay.Clear();
	SpriteShaderLightBaseNormalOverlayMod2.Clear();
	// clear context
	if (pCurrCtx) pCurrCtx->Deselect();
	pMainCtx=nullptr;
	C4Draw::Clear();
}

void CStdGL::FillBG(DWORD dwClr)
{
	if (!pCurrCtx) return;
	glClearColor((float)GetRedValue(dwClr)/255.0f, (float)GetGreenValue(dwClr)/255.0f, (float)GetBlueValue(dwClr)/255.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

bool CStdGL::UpdateClipper()
{
	// no render target? do nothing
	if (!RenderTarget || !Active) return true;
	// negative/zero?
	C4Rect clipRect = GetClipRect();
	if (clipRect.Wdt<=0 || clipRect.Hgt<=0)
	{
		ClipAll=true;
		return true;
	}
	ClipAll=false;
	// set it
	glViewport(clipRect.x, RenderTarget->Hgt-clipRect.y-clipRect.Hgt, clipRect.Wdt, clipRect.Hgt);
	ProjectionMatrix = StdProjectionMatrix::Orthographic(clipRect.x, clipRect.x + clipRect.Wdt, clipRect.y + clipRect.Hgt, clipRect.y);
	return true;
}

bool CStdGL::PrepareRendering(C4Surface * sfcToSurface)
{
	// call from gfx thread only!
	if (!pApp || !pApp->AssertMainThread()) return false;
	// not ready?
	if (!Active)
		return false;
	// target?
	if (!sfcToSurface) return false;
	// target locked?
	if (sfcToSurface->Locked) return false;
	// target is already set as render target?
	if (sfcToSurface != RenderTarget)
	{
		// target is a render-target?
		if (!sfcToSurface->IsRenderTarget()) return false;
		// context
		if (sfcToSurface->pCtx && sfcToSurface->pCtx != pCurrCtx)
			if (!sfcToSurface->pCtx->Select()) return false;
		// set target
		RenderTarget=sfcToSurface;
		// new target has different size; needs other clipping rect
		UpdateClipper();
	}
	// done
	return true;
}


bool CStdGL::PrepareSpriteShader(C4Shader& shader, const char* name, int ssc, C4GroupSet* pGroups, const char* const* additionalDefines, const char* const* additionalSlices)
{
	const char* uniformNames[C4SSU_Count + 1];
	uniformNames[C4SSU_ProjectionMatrix] = "projectionMatrix";
	uniformNames[C4SSU_ModelViewMatrix] = "modelviewMatrix";
	uniformNames[C4SSU_NormalMatrix] = "normalMatrix";
	uniformNames[C4SSU_ClrMod] = "clrMod";
	uniformNames[C4SSU_Gamma] = "gamma";
	uniformNames[C4SSU_Resolution] = "resolution";
	uniformNames[C4SSU_BaseTex] = "baseTex";
	uniformNames[C4SSU_OverlayTex] = "overlayTex";
	uniformNames[C4SSU_OverlayClr] = "overlayClr";
	uniformNames[C4SSU_LightTex] = "lightTex";
	uniformNames[C4SSU_LightTransform] = "lightTransform";
	uniformNames[C4SSU_NormalTex] = "normalTex";
	uniformNames[C4SSU_AmbientTex] = "ambientTex";
	uniformNames[C4SSU_AmbientTransform] = "ambientTransform";
	uniformNames[C4SSU_AmbientBrightness] = "ambientBrightness";
	uniformNames[C4SSU_MaterialAmbient] = "materialAmbient"; // unused
	uniformNames[C4SSU_MaterialDiffuse] = "materialDiffuse"; // unused
	uniformNames[C4SSU_MaterialSpecular] = "materialSpecular"; // unused
	uniformNames[C4SSU_MaterialEmission] = "materialEmission"; // unused
	uniformNames[C4SSU_MaterialShininess] = "materialShininess"; // unused
	uniformNames[C4SSU_Bones] = "bones"; // unused
	uniformNames[C4SSU_CullMode] = "cullMode"; // unused
	uniformNames[C4SSU_FrameCounter] = "frameCounter";
	uniformNames[C4SSU_Count] = nullptr;

	const char* attributeNames[C4SSA_Count + 1];
	attributeNames[C4SSA_Position] = "oc_Position";
	attributeNames[C4SSA_Normal] = "oc_Normal"; // unused
	attributeNames[C4SSA_TexCoord] = "oc_TexCoord"; // only used if C4SSC_Base is set
	attributeNames[C4SSA_Color] = "oc_Color";
	attributeNames[C4SSA_BoneIndices0] = "oc_BoneIndices0"; // unused
	attributeNames[C4SSA_BoneIndices1] = "oc_BoneIndices1"; // unused
	attributeNames[C4SSA_BoneWeights0] = "oc_BoneWeights0"; // unused
	attributeNames[C4SSA_BoneWeights1] = "oc_BoneWeights1"; // unused
	attributeNames[C4SSA_Count] = nullptr;

	// Clear previous content
	shader.Clear();
	shader.ClearSlices();

	// Start with #defines
	shader.AddDefine("OPENCLONK");
	shader.AddDefine("OC_SPRITE");
	if (ssc & C4SSC_MOD2) shader.AddDefine("OC_CLRMOD_MOD2");
	if (ssc & C4SSC_NORMAL) shader.AddDefine("OC_WITH_NORMALMAP");
	if (ssc & C4SSC_LIGHT) shader.AddDefine("OC_DYNAMIC_LIGHT");
	if (ssc & C4SSC_BASE) shader.AddDefine("OC_HAVE_BASE");
	if (ssc & C4SSC_OVERLAY) shader.AddDefine("OC_HAVE_OVERLAY");

	if (additionalDefines)
		for (const char* const* define = additionalDefines; *define != nullptr; ++define)
			shader.AddDefine(*define);

	// Then load slices for fragment and vertex shader
	shader.LoadVertexSlices(pGroups, "SpriteVertexShader.glsl");
	shader.LoadFragmentSlices(pGroups, "CommonShader.glsl");
	shader.LoadFragmentSlices(pGroups, "ObjectShader.glsl");

	// Categories for script shaders.
	shader.SetScriptCategories({"Common", "Object"});

	if (additionalSlices)
		for (const char* const* slice = additionalSlices; *slice != nullptr; ++slice)
			shader.LoadFragmentSlices(pGroups, *slice);

	if (!shader.Init(name, uniformNames, attributeNames))
	{
		shader.ClearSlices();
		return false;
	}

	return true;
}

void CStdGL::ObjectLabel(uint32_t identifier, uint32_t name, int32_t length, const char * label)
{
	if (has_khr_debug)
		glObjectLabel(identifier, name, length, label);
}

CStdGLCtx *CStdGL::CreateContext(C4Window * pWindow, C4AbstractApp *pApp)
{
	// safety
	if (!pWindow) return nullptr;

	// create it
	CStdGLCtx *pCtx;
#ifdef WITH_QT_EDITOR
	auto app = dynamic_cast<C4Application*>(pApp);
	if (app->isEditor)
		pCtx = new CStdGLCtxQt();
	else
#endif
	pCtx = new CStdGLCtx();
	bool first_ctx = !pMainCtx;
	if (first_ctx)
	{
		pMainCtx = pCtx;
		LogF("  gl: Create first %scontext...", Config.Graphics.DebugOpenGL ? "debug " : "");
	}
	if (!pCtx->Init(pWindow, pApp))
	{
		Log("  gl: failed to create context.");
		delete pCtx;
		return NULL;
	}
	has_khr_debug = epoxy_has_gl_extension("GL_KHR_debug") || epoxy_gl_version() >= 43;
	if (Config.Graphics.DebugOpenGL && (has_khr_debug || epoxy_has_gl_extension("GL_ARB_debug_output")))
	{
		if (first_ctx) Log("  gl: Setting OpenGLDebugProc callback");
		glDebugMessageCallback(&OpenGLDebugProc, nullptr);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		if (has_khr_debug)
			glEnable(GL_DEBUG_OUTPUT);
	}
	// First context: Log some information about hardware/drivers
	// Must log after context creation to get valid results
	if (first_ctx)
	{
		const auto *gl_vendor = reinterpret_cast<const char *>(glGetString(GL_VENDOR));
		const auto *gl_renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
		const auto *gl_version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
		LogF("GL %s on %s (%s)", gl_version ? gl_version : "", gl_renderer ? gl_renderer : "", gl_vendor ? gl_vendor : "");
		
		if (Config.Graphics.DebugOpenGL)
		{
			// Dump extension list
			if (epoxy_is_desktop_gl() && epoxy_gl_version() >= 30)
			{
				GLint gl_extension_count = 0;
				glGetIntegerv(GL_NUM_EXTENSIONS, &gl_extension_count);
				if (gl_extension_count == 0)
				{
					LogSilentF("No available extensions.");
				}
				else
				{
					LogSilentF("%d available extensions:", gl_extension_count);
					for (GLint i = 0; i < gl_extension_count; ++i)
					{
						const char *gl_extension = (const char*)glGetStringi(GL_EXTENSIONS, i);
						LogSilentF("  %4d: %s", i, gl_extension);
					}
				}
			}
			else
			{
				const char *gl_extensions = reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS));
				LogSilentF("GLExt: %s", gl_extensions ? gl_extensions : "");
			}
		}
	}
	// creation selected the new context - switch back to previous context
	RenderTarget = nullptr;
#ifdef WITH_QT_EDITOR
	// FIXME This is a hackfix for #1813 / #1956. The proper way to fix them would probably be to select a drawing context before invoking C4Player::FinalInit
	if (!app->isEditor)
#endif
	pCurrCtx = nullptr;
	// done
	return pCtx;
}

void CStdGL::SetupMultiBlt(C4ShaderCall& call, const C4BltTransform* pTransform, GLuint baseTex, GLuint overlayTex, GLuint normalTex, DWORD dwOverlayModClr, StdProjectionMatrix* out_modelview)
{
	// Initialize multi blit shader.
	int iAdditive = dwBlitMode & C4GFXBLIT_ADDITIVE;
	glBlendFunc(GL_SRC_ALPHA, iAdditive ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA);

	call.Start();

	// Upload uniforms
	const DWORD dwModClr = BlitModulated ? BlitModulateClr : 0xffffffff;
	const float fMod[4] = {
		((dwModClr >> 16) & 0xff) / 255.0f,
		((dwModClr >>  8) & 0xff) / 255.0f,
		((dwModClr      ) & 0xff) / 255.0f,
		((dwModClr >> 24) & 0xff) / 255.0f
	};

	call.SetUniform4fv(C4SSU_ClrMod, 1, fMod);
	call.SetUniform3fv(C4SSU_Gamma, 1, gammaOut);

	if(baseTex != 0)
	{
		call.AllocTexUnit(C4SSU_BaseTex);
		glBindTexture(GL_TEXTURE_2D, baseTex);
	}

	if(overlayTex != 0)
	{
		call.AllocTexUnit(C4SSU_OverlayTex);
		glBindTexture(GL_TEXTURE_2D, overlayTex);

		const float fOverlayModClr[4] = {
			((dwOverlayModClr >> 16) & 0xff) / 255.0f,
			((dwOverlayModClr >>  8) & 0xff) / 255.0f,
			((dwOverlayModClr      ) & 0xff) / 255.0f,
			((dwOverlayModClr >> 24) & 0xff) / 255.0f
		};

		call.SetUniform4fv(C4SSU_OverlayClr, 1, fOverlayModClr);
	}

	if(pFoW != nullptr && normalTex != 0)
	{
		call.AllocTexUnit(C4SSU_NormalTex);
		glBindTexture(GL_TEXTURE_2D, normalTex);
	}

	if(pFoW != nullptr)
	{
		const C4Rect OutRect = GetOutRect();
		const C4Rect ClipRect = GetClipRect();
		const FLOAT_RECT vpRect = pFoW->getViewportRegion();

		// Dynamic Light
		call.AllocTexUnit(C4SSU_LightTex);
		glBindTexture(GL_TEXTURE_2D, pFoW->getSurfaceName());

		float lightTransform[6];
		pFoW->GetFragTransform(ClipRect, OutRect, lightTransform);
		call.SetUniformMatrix2x3fv(C4SSU_LightTransform, 1, lightTransform);

		// Ambient Light
		call.AllocTexUnit(C4SSU_AmbientTex);
		glBindTexture(GL_TEXTURE_2D, pFoW->getFoW()->Ambient.Tex);
		call.SetUniform1f(C4SSU_AmbientBrightness, pFoW->getFoW()->Ambient.GetBrightness());

		float ambientTransform[6];
		pFoW->getFoW()->Ambient.GetFragTransform(vpRect, ClipRect, OutRect, ambientTransform);
		call.SetUniformMatrix2x3fv(C4SSU_AmbientTransform, 1, ambientTransform);
	}

	call.SetUniform1f(C4SSU_CullMode, 0.0f);

	// The primary reason we use a 4x4 matrix for the modelview matrix is that
	// that the C4BltTransform pTransform parameter can have projection components
	// (see SetObjDrawTransform2). Still, for sprites the situation is a bit
	// unsatisfactory because there's no distinction between modelview and projection
	// components in the BltTransform. Object rotation is part of the BltTransform
	// for sprites, which should be part of the modelview matrix, so that lighting
	// is correct for rotated sprites. This is much more common than projection
	// components in the BltTransform, and therefore we turn the BltTransform into
	// the modelview matrix and not the projection matrix.
	StdProjectionMatrix default_modelview = StdProjectionMatrix::Identity();
	StdProjectionMatrix& modelview = out_modelview ? *out_modelview : default_modelview;

	// Apply zoom and transform
	Translate(modelview, ZoomX, ZoomY, 0.0f);
	// Scale Z as well so that we don't distort normals.
	Scale(modelview, Zoom, Zoom, Zoom);
	Translate(modelview, -ZoomX, -ZoomY, 0.0f);

	if(pTransform)
	{
		float sz = 1.0f;
		if (pFoW != nullptr && normalTex != 0)
		{
			// Decompose scale factors and scale Z accordingly to X and Y, again to avoid distorting normals
			// We could instead work around this by using the projection matrix, but then for object rotations (SetR)
			// the normals would not be correct.
			const float sx = sqrt(pTransform->mat[0]*pTransform->mat[0] + pTransform->mat[1]*pTransform->mat[1]);
			const float sy = sqrt(pTransform->mat[3]*pTransform->mat[3] + pTransform->mat[4]*pTransform->mat[4]);
			sz = sqrt(sx * sy);
		}

		// Multiply modelview matrix with transform
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
		transform(2, 2) = sz;
		transform(2, 3) = 0.0f;
		transform(3, 0) = pTransform->mat[6];
		transform(3, 1) = pTransform->mat[7];
		transform(3, 2) = 0.0f;
		transform(3, 3) = pTransform->mat[8];
		modelview *= transform;
	}

	call.SetUniformMatrix4x4(C4SSU_ProjectionMatrix, ProjectionMatrix);
	call.SetUniformMatrix4x4(C4SSU_ModelViewMatrix, modelview);

	if (pFoW != nullptr && normalTex != 0)
		call.SetUniformMatrix3x3Transpose(C4SSU_NormalMatrix, StdMeshMatrix::Inverse(StdProjectionMatrix::Upper3x4(modelview)));

	scriptUniform.Apply(call);
}

void CStdGL::PerformMultiPix(C4Surface* sfcTarget, const C4BltVertex* vertices, unsigned int n_vertices, C4ShaderCall* shader_call)
{
	// Draw on pixel center:
	StdProjectionMatrix transform = StdProjectionMatrix::Translate(0.5f, 0.5f, 0.0f);

	// This is a workaround. Instead of submitting the whole vertex array to the GL, we do it
	// in batches of 256 vertices. The intel graphics driver on Linux crashes with
	// significantly larger arrays, such as 400. It's not clear to me why, maybe POINT drawing
	// is just not very well tested.
	const unsigned int BATCH_SIZE = 256;

	// Feed the vertices to the GL
	if (!shader_call)
	{
		C4ShaderCall call(GetSpriteShader(false, false, false));
		SetupMultiBlt(call, nullptr, 0, 0, 0, 0, &transform);
		for(unsigned int i = 0; i < n_vertices; i += BATCH_SIZE)
			PerformMultiBlt(sfcTarget, OP_POINTS, &vertices[i], std::min(n_vertices - i, BATCH_SIZE), false, &call);
	}
	else
	{
		SetupMultiBlt(*shader_call, nullptr, 0, 0, 0, 0, &transform);
		for(unsigned int i = 0; i < n_vertices; i += BATCH_SIZE)
			PerformMultiBlt(sfcTarget, OP_POINTS, &vertices[i], std::min(n_vertices - i, BATCH_SIZE), false, shader_call);
	}
}

void CStdGL::PerformMultiLines(C4Surface* sfcTarget, const C4BltVertex* vertices, unsigned int n_vertices, float width, C4ShaderCall* shader_call)
{
	// In a first step, we transform the lines array to a triangle array, so that we can draw
	// the lines with some thickness.
	// In principle, this step could be easily (and probably much more efficiently) performed
	// by a geometry shader as well, however that would require OpenGL 3.2.
	C4BltVertex* tri_vertices = new C4BltVertex[n_vertices * 3];
	for(unsigned int i = 0; i < n_vertices; i += 2)
	{
		const float x1 = vertices[i].ftx;
		const float y1 = vertices[i].fty;
		const float x2 = vertices[i+1].ftx;
		const float y2 = vertices[i+1].fty;

		float offx = y1 - y2;
		float offy = x2 - x1;
		float l = sqrtf(offx * offx + offy * offy);
		// avoid division by zero
		l += 0.000000005f;
		offx *= width/l;
		offy *= width/l;

		tri_vertices[3*i + 0].ftx = x1 + offx; tri_vertices[3*i + 0].fty = y1 + offy;
		tri_vertices[3*i + 1].ftx = x1 - offx; tri_vertices[3*i + 1].fty = y1 - offy;
		tri_vertices[3*i + 2].ftx = x2 - offx; tri_vertices[3*i + 2].fty = y2 - offy;
		tri_vertices[3*i + 3].ftx = x2 + offx; tri_vertices[3*i + 3].fty = y2 + offy;

		for(int j = 0; j < 4; ++j)
		{
			tri_vertices[3*i + 0].color[j] = vertices[i].color[j];
			tri_vertices[3*i + 1].color[j] = vertices[i].color[j];
			tri_vertices[3*i + 2].color[j] = vertices[i + 1].color[j];
			tri_vertices[3*i + 3].color[j] = vertices[i + 1].color[j];
		}

		tri_vertices[3*i + 0].tx = 0.f; tri_vertices[3*i + 0].ty = 0.f;
		tri_vertices[3*i + 1].tx = 0.f; tri_vertices[3*i + 1].ty = 2.f;
		tri_vertices[3*i + 2].tx = 1.f; tri_vertices[3*i + 2].ty = 2.f;
		tri_vertices[3*i + 3].tx = 1.f; tri_vertices[3*i + 3].ty = 0.f;

		tri_vertices[3*i + 4] = tri_vertices[3*i + 2]; // duplicate vertex
		tri_vertices[3*i + 5] = tri_vertices[3*i + 0]; // duplicate vertex
	}

	// Then, feed the vertices to the GL
	if (!shader_call)
	{
		C4ShaderCall call(GetSpriteShader(true, false, false));
		SetupMultiBlt(call, nullptr, lines_tex, 0, 0, 0, nullptr);
		PerformMultiBlt(sfcTarget, OP_TRIANGLES, tri_vertices, n_vertices * 3, true, &call);
	}
	else
	{
		SetupMultiBlt(*shader_call, nullptr, lines_tex, 0, 0, 0, nullptr);
		PerformMultiBlt(sfcTarget, OP_TRIANGLES, tri_vertices, n_vertices * 3, true, shader_call);
	}

	delete[] tri_vertices;
}

void CStdGL::PerformMultiTris(C4Surface* sfcTarget, const C4BltVertex* vertices, unsigned int n_vertices, const C4BltTransform* pTransform, C4TexRef* pTex, C4TexRef* pOverlay, C4TexRef* pNormal, DWORD dwOverlayModClr, C4ShaderCall* shader_call)
{
	// Feed the vertices to the GL
	if (!shader_call)
	{
		C4ShaderCall call(GetSpriteShader(pTex != nullptr, pOverlay != nullptr, pNormal != nullptr));
		SetupMultiBlt(call, pTransform, pTex ? pTex->texName : 0, pOverlay ? pOverlay->texName : 0, pNormal ? pNormal->texName : 0, dwOverlayModClr, nullptr);
		PerformMultiBlt(sfcTarget, OP_TRIANGLES, vertices, n_vertices, pTex != nullptr, &call);
	}
	else
	{
		SetupMultiBlt(*shader_call, pTransform, pTex ? pTex->texName : 0, pOverlay ? pOverlay->texName : 0, pNormal ? pNormal->texName : 0, dwOverlayModClr, nullptr);
		PerformMultiBlt(sfcTarget, OP_TRIANGLES, vertices, n_vertices, pTex != nullptr, shader_call);
	}
}

void CStdGL::PerformMultiBlt(C4Surface* sfcTarget, DrawOperation op, const C4BltVertex* vertices, unsigned int n_vertices, bool has_tex, C4ShaderCall* shader_call)
{
	// Only direct rendering
	assert(sfcTarget->IsRenderTarget());
	if(!PrepareRendering(sfcTarget)) return;

	// Set resolution. The other uniforms are set in SetupMultiBlt, but the
	// surface size is still unknown there.
	shader_call->SetUniform2f(C4SSU_Resolution, sfcTarget->Wdt, sfcTarget->Hgt);

	// Select a buffer
	const unsigned int vbo_index = CurrentVBO;
	CurrentVBO = (CurrentVBO + 1) % N_GENERIC_VBOS;

	// Upload data into the buffer, resize buffer if necessary
	glBindBuffer(GL_ARRAY_BUFFER, GenericVBOs[vbo_index]);
	if (GenericVBOSizes[vbo_index] < n_vertices)
	{
		GenericVBOSizes[vbo_index] = n_vertices;
		glBufferData(GL_ARRAY_BUFFER, n_vertices * sizeof(C4BltVertex), vertices, GL_STREAM_DRAW);
	}
	else
	{
		glBufferSubData(GL_ARRAY_BUFFER, 0, n_vertices * sizeof(C4BltVertex), vertices);
	}

	// Choose the VAO that corresponds to the chosen VBO. Also, use one
	// that supplies texture coordinates if we have texturing enabled.
	GLuint vao;
	const unsigned int vao_index = vbo_index + (has_tex ? N_GENERIC_VBOS : 0);
	const unsigned int vao_id = GenericVAOs[vao_index];
	const bool has_vao = GetVAO(vao_id, vao);
	glBindVertexArray(vao);
	if (!has_vao)
	{
		// Initialize VAO for this context
		const GLuint position = shader_call->GetAttribute(C4SSA_Position);
		const GLuint color = shader_call->GetAttribute(C4SSA_Color);
		const GLuint texcoord = has_tex ? shader_call->GetAttribute(C4SSA_TexCoord) : 0;

		glEnableVertexAttribArray(position);
		glEnableVertexAttribArray(color);
		if (has_tex)
			glEnableVertexAttribArray(texcoord);


		glVertexAttribPointer(position, 2, GL_FLOAT, GL_FALSE, sizeof(C4BltVertex), reinterpret_cast<const uint8_t*>(offsetof(C4BltVertex, ftx)));
		glVertexAttribPointer(color, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(C4BltVertex), reinterpret_cast<const uint8_t*>(offsetof(C4BltVertex, color)));
		if (has_tex)
			glVertexAttribPointer(texcoord, 2, GL_FLOAT, GL_FALSE, sizeof(C4BltVertex), reinterpret_cast<const uint8_t*>(offsetof(C4BltVertex, tx)));
	}

	switch (op)
	{
	case OP_POINTS:
		glDrawArrays(GL_POINTS, 0, n_vertices);
		break;
	case OP_TRIANGLES:
		glDrawArrays(GL_TRIANGLES, 0, n_vertices);
		break;
	default:
		assert(false);
		break;
	}

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

C4Shader* CStdGL::GetSpriteShader(bool haveBase, bool haveOverlay, bool haveNormal)
{
	int ssc = 0;
	if(dwBlitMode & C4GFXBLIT_MOD2) ssc |= C4SSC_MOD2;
	if(haveBase) ssc |= C4SSC_BASE;
	if(haveBase && haveOverlay) ssc |= C4SSC_OVERLAY;
	if(pFoW != nullptr) ssc |= C4SSC_LIGHT;
	if(pFoW != nullptr && haveBase && haveNormal) ssc |= C4SSC_NORMAL;
	return GetSpriteShader(ssc);
}

C4Shader* CStdGL::GetSpriteShader(int ssc)
{
	C4Shader* shaders[16] = {
		&SpriteShader,
		&SpriteShaderMod2,
		&SpriteShaderBase,
		&SpriteShaderBaseMod2,
		&SpriteShaderBaseOverlay,
		&SpriteShaderBaseOverlayMod2,

		&SpriteShaderLight,
		&SpriteShaderLightMod2,
		&SpriteShaderLightBase,
		&SpriteShaderLightBaseMod2,
		&SpriteShaderLightBaseOverlay,
		&SpriteShaderLightBaseOverlayMod2,
		&SpriteShaderLightBaseNormal,
		&SpriteShaderLightBaseNormalMod2,
		&SpriteShaderLightBaseNormalOverlay,
		&SpriteShaderLightBaseNormalOverlayMod2,
	};

	int index = 0;
	if(ssc & C4SSC_LIGHT) index += 6;

	if(ssc & C4SSC_BASE)
	{
		index += 2;
		if(ssc & C4SSC_OVERLAY)
			index += 2;
		if( (ssc & C4SSC_NORMAL) && (ssc & C4SSC_LIGHT))
			index += 4;
	}

	if(ssc & C4SSC_MOD2)
		index += 1;

	assert(index < 16);
	return shaders[index];
}

bool CStdGL::InitShaders(C4GroupSet* pGroups)
{
	if (!pCurrCtx)
		EnsureMainContextSelected();

	// Create sprite blitting shaders
	if(!PrepareSpriteShader(SpriteShader, "sprite", 0, pGroups, nullptr, nullptr))
		return false;
	if(!PrepareSpriteShader(SpriteShaderMod2, "spriteMod2", C4SSC_MOD2, pGroups, nullptr, nullptr))
		return false;
	if(!PrepareSpriteShader(SpriteShaderBase, "spriteBase", C4SSC_BASE, pGroups, nullptr, nullptr))
		return false;
	if(!PrepareSpriteShader(SpriteShaderBaseMod2, "spriteBaseMod2", C4SSC_MOD2 | C4SSC_BASE, pGroups, nullptr, nullptr))
		return false;
	if(!PrepareSpriteShader(SpriteShaderBaseOverlay, "spriteBaseOverlay", C4SSC_BASE | C4SSC_OVERLAY, pGroups, nullptr, nullptr))
		return false;
	if(!PrepareSpriteShader(SpriteShaderBaseOverlayMod2, "spriteBaseOverlayMod2", C4SSC_MOD2 | C4SSC_BASE | C4SSC_OVERLAY, pGroups, nullptr, nullptr))
		return false;

	if(!PrepareSpriteShader(SpriteShaderLight, "spriteLight", C4SSC_LIGHT, pGroups, nullptr, nullptr))
		return false;
	if(!PrepareSpriteShader(SpriteShaderLightMod2, "spriteLightMod2", C4SSC_LIGHT | C4SSC_MOD2, pGroups, nullptr, nullptr))
		return false;
	if(!PrepareSpriteShader(SpriteShaderLightBase, "spriteLightBase", C4SSC_LIGHT | C4SSC_BASE, pGroups, nullptr, nullptr))
		return false;
	if(!PrepareSpriteShader(SpriteShaderLightBaseMod2, "spriteLightBaseMod2", C4SSC_LIGHT | C4SSC_BASE | C4SSC_MOD2, pGroups, nullptr, nullptr))
		return false;
	if(!PrepareSpriteShader(SpriteShaderLightBaseOverlay, "spriteLightBaseOverlay", C4SSC_LIGHT | C4SSC_BASE | C4SSC_OVERLAY, pGroups, nullptr, nullptr))
		return false;
	if(!PrepareSpriteShader(SpriteShaderLightBaseOverlayMod2, "spriteLightBaseOverlayMod2", C4SSC_LIGHT | C4SSC_BASE | C4SSC_OVERLAY | C4SSC_MOD2, pGroups, nullptr, nullptr))
		return false;
	if(!PrepareSpriteShader(SpriteShaderLightBaseNormal, "spriteLightBaseNormal", C4SSC_LIGHT | C4SSC_BASE | C4SSC_NORMAL, pGroups, nullptr, nullptr))
		return false;
	if(!PrepareSpriteShader(SpriteShaderLightBaseNormalMod2, "spriteLightBaseNormalMod2", C4SSC_LIGHT | C4SSC_BASE | C4SSC_NORMAL | C4SSC_MOD2, pGroups, nullptr, nullptr))
		return false;
	if(!PrepareSpriteShader(SpriteShaderLightBaseNormalOverlay, "spriteLightBaseNormalOverlay", C4SSC_LIGHT | C4SSC_BASE | C4SSC_OVERLAY | C4SSC_NORMAL, pGroups, nullptr, nullptr))
		return false;
	if(!PrepareSpriteShader(SpriteShaderLightBaseNormalOverlayMod2, "spriteLightBaseNormalOverlayMod2", C4SSC_LIGHT | C4SSC_BASE | C4SSC_OVERLAY | C4SSC_NORMAL | C4SSC_MOD2, pGroups, nullptr, nullptr))
		return false;

	return true;
}

bool CStdGL::EnsureMainContextSelected()
{
	return pMainCtx->Select();
}

bool CStdGL::RestoreDeviceObjects()
{
	assert(pMainCtx);
	// delete any previous objects
	InvalidateDeviceObjects();

	// set states
	Active = pMainCtx->Select();
	RenderTarget = pApp->pWindow->pSurface;

	// lines texture
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, &lines_tex);
	glBindTexture(GL_TEXTURE_2D, lines_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	static const char * linedata = "\xff\xff\xff\x00\xff\xff\xff\xff";
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 2, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, linedata);

	MaxTexSize = 64;
	GLint s = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &s);
	if (s>0) MaxTexSize = s;

	// Generic VBOs
	glGenBuffers(N_GENERIC_VBOS, GenericVBOs);
	for (unsigned int i = 0; i < N_GENERIC_VBOS; ++i)
	{
		GenericVBOSizes[i] = GENERIC_VBO_SIZE;
		glBindBuffer(GL_ARRAY_BUFFER, GenericVBOs[i]);
		glBufferData(GL_ARRAY_BUFFER, GenericVBOSizes[i] * sizeof(C4BltVertex), nullptr, GL_STREAM_DRAW);
		GenericVAOs[i] = GenVAOID();
		GenericVAOs[i + N_GENERIC_VBOS] = GenVAOID();
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// reset blit states
	dwBlitMode = 0;

	// done
	return Active;
}

bool CStdGL::InvalidateDeviceObjects()
{
	bool fSuccess=true;
	// deactivate
	Active=false;
	// invalidate font objects
	// invalidate primary surfaces
	if (lines_tex)
	{
		glDeleteTextures(1, &lines_tex);
		lines_tex = 0;
	}

	// invalidate generic VBOs
	if (GenericVBOs[0] != 0)
	{
		glDeleteBuffers(N_GENERIC_VBOS, GenericVBOs);
		GenericVBOs[0] = 0;
		CurrentVBO = 0;
		for (unsigned int GenericVAO : GenericVAOs)
			FreeVAOID(GenericVAO);
	}

	// invalidate shaders

	// TODO: We don't do this here because we cannot re-validate them in
	// RestoreDeviceObjects. This should be refactored.

	/*SpriteShader.Clear();
	SpriteShaderMod2.Clear();
	SpriteShaderBase.Clear();
	SpriteShaderBaseMod2.Clear();
	SpriteShaderBaseOverlay.Clear();
	SpriteShaderBaseOverlayMod2.Clear();
	SpriteShaderLight.Clear();
	SpriteShaderLightMod2.Clear();
	SpriteShaderLightBase.Clear();
	SpriteShaderLightBaseMod2.Clear();
	SpriteShaderLightBaseOverlay.Clear();
	SpriteShaderLightBaseOverlayMod2.Clear();
	SpriteShaderLightBaseNormal.Clear();
	SpriteShaderLightBaseNormalMod2.Clear();
	SpriteShaderLightBaseNormalOverlay.Clear();
	SpriteShaderLightBaseNormalOverlayMod2.Clear();*/
	return fSuccess;
}

bool CStdGL::Error(const char *szMsg)
{
#ifdef USE_WIN32_WINDOWS
	DWORD err = GetLastError();
#endif
	bool r = C4Draw::Error(szMsg);
#ifdef USE_WIN32_WINDOWS
	wchar_t * lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0, nullptr );
	LogF("  gl: GetLastError() = %d - %s", err, StdStrBuf(lpMsgBuf).getData());
	LocalFree(lpMsgBuf);
#endif
	LogF("  gl: %s", glGetString(GL_VENDOR));
	LogF("  gl: %s", glGetString(GL_RENDERER));
	LogF("  gl: %s", glGetString(GL_VERSION));
	LogF("  gl: %s", glGetString(GL_EXTENSIONS));
	return r;
}

const char* CStdGL::GLErrorString(GLenum code)
{
	switch (code)
	{
	case GL_NO_ERROR: return "No error";
	case GL_INVALID_ENUM: return "An unacceptable value is specified for an enumerated argument";
	case GL_INVALID_VALUE: return "A numeric argument is out of range";
	case GL_INVALID_OPERATION: return "The specified operation is not allowed in the current state";
	case GL_INVALID_FRAMEBUFFER_OPERATION: return "The framebuffer object is not complete";
	case GL_OUT_OF_MEMORY: return "There is not enough memory left to execute the command";
	case GL_STACK_UNDERFLOW: return "An attempt has been made to perform an operation that would cause an internal stack to underflow";
	case GL_STACK_OVERFLOW: return "An attempt has been made to perform an operation that would cause an internal stack to overflow";
	default: assert(false); return "";
	}
}

bool CStdGL::CheckGLError(const char *szAtOp)
{
	GLenum err = glGetError();
	if (!err) return true;

	LogF("GL error with %s: %d - %s", szAtOp, err, GLErrorString(err));
	return false;
}

CStdGL *pGL=nullptr;

bool CStdGL::OnResolutionChanged(unsigned int iXRes, unsigned int iYRes)
{
	// Re-create primary clipper to adapt to new size.
	CreatePrimaryClipper(iXRes, iYRes);
	RestoreDeviceObjects();
	return true;
}

void CStdGL::Default()
{
	C4Draw::Default();
	pCurrCtx = nullptr;
	iPixelFormat=0;
	sfcFmt=0;
	Workarounds.LowMaxVertexUniformCount = false;
	Workarounds.ForceSoftwareTransform = false;
}

unsigned int CStdGL::GenVAOID()
{
	// Generate a new VAO ID. Make them sequential so that the actual
	// VAOs in the context can be simply maintained with a lookup table.
	unsigned int id;
	if (NextVAOID == VAOIDs.begin())
	{
		// Insert at the beginning
		id = 1;
	}
	else
	{
		// Insert at the end, or somewhere in the middle
		std::set<unsigned int>::iterator iter = NextVAOID;
		--iter;

		id = *iter + 1;
	}

	// Actually insert the ID
#ifdef NDEBUG
	std::set<unsigned int>::iterator inserted_iter = VAOIDs.insert(NextVAOID, id);
#else
	std::pair<std::set<unsigned int>::iterator, bool> inserted = VAOIDs.insert(id);
	assert(inserted.second == true);
	std::set<unsigned int>::iterator inserted_iter = inserted.first;
#endif

	// Update next VAO ID: increment iterator until we find a gap
	// in the sequence.
	NextVAOID = inserted_iter;
	unsigned int prev_id = id;
	++NextVAOID;
	while(NextVAOID != VAOIDs.end() && prev_id + 1 == *NextVAOID)
	{
		prev_id = *NextVAOID;
		++NextVAOID;
	}

	return id;
}

void CStdGL::FreeVAOID(unsigned int vaoid)
{
	std::set<unsigned int>::iterator iter = VAOIDs.find(vaoid);
	assert(iter != VAOIDs.end());

	// Delete this VAO in the current context
	if (pCurrCtx)
	{
		if (vaoid < pCurrCtx->hVAOs.size() && pCurrCtx->hVAOs[vaoid] != 0)
		{
			glDeleteVertexArrays(1, &pCurrCtx->hVAOs[vaoid]);
			pCurrCtx->hVAOs[vaoid] = 0;
		}
	}

	// For all other contexts, mark it to be deleted as soon as we select
	// that context. Otherwise we would need to do a lot of context
	// switching at this point.
	for (auto ctx : CStdGLCtx::contexts)
	{
		if (ctx != pCurrCtx && vaoid < ctx->hVAOs.size() && ctx->hVAOs[vaoid] != 0)
			if (std::find(ctx->VAOsToBeDeleted.begin(), ctx->VAOsToBeDeleted.end(), vaoid) == ctx->VAOsToBeDeleted.end())
				ctx->VAOsToBeDeleted.push_back(vaoid);
	}

	// Delete the VAO ID from our list of VAO IDs in use
	// If the Next VAO ID is 1, then no matter what we delete we don't need
	// to update anything. If it is not at the beginning, then move it to the
	// gap we just created if it was at a higher place, to make sure we keep
	// the numbers as sequential as possible.
	unsigned int nextVaoID = 1;
	if (NextVAOID != VAOIDs.begin())
	{
		std::set<unsigned int>::iterator next_vao_iter = NextVAOID;
		--next_vao_iter;
		nextVaoID = *next_vao_iter + 1;
	}

	assert(vaoid != nextVaoID);

	if (vaoid < nextVaoID || iter == NextVAOID)
		NextVAOID = VAOIDs.erase(iter);
	else
		VAOIDs.erase(iter);
}

bool CStdGL::GetVAO(unsigned int vaoid, GLuint& vao)
{
	assert(pCurrCtx != nullptr);

	if (vaoid >= pCurrCtx->hVAOs.size())
	{
		// Resize the VAO array so that all generated VAO IDs fit
		// in it, and not only the one requested in this call.
		// We hope to get away with fewer reallocations this way.
		assert(VAOIDs.find(vaoid) != VAOIDs.end());
		std::set<unsigned int>::iterator iter = VAOIDs.end();
		--iter;

		pCurrCtx->hVAOs.resize(*iter + 1);
	}

	if (pCurrCtx->hVAOs[vaoid] == 0)
	{
		glGenVertexArrays(1, &pCurrCtx->hVAOs[vaoid]);
		vao = pCurrCtx->hVAOs[vaoid];
		return false;
	}

	vao = pCurrCtx->hVAOs[vaoid];
	return true;
}

#endif // USE_CONSOLE
