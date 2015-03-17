/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2013, The OpenClonk Team and contributors
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
#include <C4DrawGL.h>

#include <C4Surface.h>
#include <C4Window.h>
#include <C4FoWRegion.h>
#include "C4Rect.h"
#include "C4Config.h"
#include <C4App.h>

#ifndef USE_CONSOLE

// MSVC doesn't define M_PI in math.h unless requested
#ifdef  _MSC_VER
#define _USE_MATH_DEFINES
#endif  /* _MSC_VER */

#include <stdio.h>
#include <math.h>

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

#ifdef GLDEBUGPROCARB_USERPARAM_IS_CONST
#define USERPARAM_CONST const
#else
#define USERPARAM_CONST
#endif

	void GLAPIENTRY OpenGLDebugProc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, USERPARAM_CONST void* userParam)
	{
		const char *msg_source = MsgSourceToStr(source);
		const char *msg_type = MsgTypeToStr(type);
		const char *msg_severity = MsgSeverityToStr(severity);

		DebugLogF("  gl: %s severity %s %s: %s", msg_severity, msg_source, msg_type, message);
#ifdef USE_WIN32_WINDOWS
		if (IsDebuggerPresent() && severity == GL_DEBUG_SEVERITY_HIGH_ARB)
			BREAKPOINT_HERE;
#endif
	}
}

#undef USERPARAM_CONST

CStdGL::CStdGL():
		pMainCtx(0)
{
	Default();
	byByteCnt=4;
	// global ptr
	pGL = this;
	lines_tex = 0;
}

CStdGL::~CStdGL()
{
	Clear();
	pGL=NULL;
}

void CStdGL::Clear()
{
	NoPrimaryClipper();
	//if (pTexMgr) pTexMgr->IntUnlock(); // cannot do this here or we can't preserve textures across GL reinitialization as required when changing multisampling
	InvalidateDeviceObjects();
	NoPrimaryClipper();
	RenderTarget = NULL;
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
	pMainCtx=0;
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
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// Set clipping plane to -1000 and 1000 so that large meshes are not
	// clipped away.
	//glOrtho((GLdouble) iX, (GLdouble) (iX+iWdt), (GLdouble) (iY+iHgt), (GLdouble) iY, -1000.0f, 1000.0f);
	gluOrtho2D((GLdouble) clipRect.x, (GLdouble) (clipRect.x + clipRect.Wdt), (GLdouble) (clipRect.y + clipRect.Hgt), (GLdouble) clipRect.y);
	//gluOrtho2D((GLdouble) 0, (GLdouble) xRes, (GLdouble) yRes, (GLdouble) yRes-iHgt);
	return true;
}

bool CStdGL::PrepareRendering(C4Surface * sfcToSurface)
{
	// call from gfx thread only!
	if (!pApp || !pApp->AssertMainThread()) return false;
	// not ready?
	if (!Active)
		//if (!RestoreDeviceObjects())
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

CStdGLCtx *CStdGL::CreateContext(C4Window * pWindow, C4AbstractApp *pApp)
{
	DebugLog("  gl: Create Context...");
	// safety
	if (!pWindow) return NULL;

	// create it
	CStdGLCtx *pCtx = new CStdGLCtx();
	bool first_ctx = !pMainCtx;
	if (first_ctx) pMainCtx = pCtx;
	bool success = pCtx->Init(pWindow, pApp);
	if (Config.Graphics.DebugOpenGL && glDebugMessageCallbackARB)
	{
		glDebugMessageCallbackARB(&OpenGLDebugProc, nullptr);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
	}
	// First context: Log some information about hardware/drivers
	// Must log after context creation to get valid results
	if (first_ctx)
	{
		const char *gl_vendor = reinterpret_cast<const char *>(glGetString(GL_VENDOR));
		const char *gl_renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
		const char *gl_version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
		LogF("GL %s on %s (%s)", gl_version ? gl_version : "", gl_renderer ? gl_renderer : "", gl_vendor ? gl_vendor : "");
		if (Config.Graphics.DebugOpenGL)
		{
			// Dump extension list
			if (glGetStringi)
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

		// Check which workarounds we have to apply
		{
			// If we have less than 2048 uniform components available, we
			// need to upload bone matrices in a different way
			GLint count;
			glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &count);
			Workarounds.LowMaxVertexUniformCount = count < 2048;
		}
	}
	if (!success)
	{
		delete pCtx; Error("  gl: Error creating secondary context!"); return NULL;
	}
	// creation selected the new context - switch back to previous context
	RenderTarget = NULL;
	pCurrCtx = NULL;
	// done
	return pCtx;
}

#ifdef USE_WIN32_WINDOWS
CStdGLCtx *CStdGL::CreateContext(HWND hWindow, C4AbstractApp *pApp)
{
	// safety
	if (!hWindow) return NULL;
	// create it
	CStdGLCtx *pCtx = new CStdGLCtx();
	if (!pCtx->Init(NULL, pApp, hWindow))
	{
		delete pCtx; Error("  gl: Error creating secondary context!"); return NULL;
	}
	if (Config.Graphics.DebugOpenGL && glDebugMessageCallbackARB)
	{
		glDebugMessageCallbackARB(&OpenGLDebugProc, nullptr);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
	}
	if (!pMainCtx)
	{
		pMainCtx = pCtx;
	}
	else
	{
		// creation selected the new context - switch back to previous context
		RenderTarget = NULL;
		pCurrCtx = NULL;
	}
	// done
	return pCtx;
}
#endif

bool CStdGL::CreatePrimarySurfaces(unsigned int, unsigned int, int iColorDepth, unsigned int)
{
	// store options
	bool ok = RestoreDeviceObjects();

	// - AMD GPUs have supported OpenGL 2.1 since 2007
	// - nVidia GPUs have supported OpenGL 2.1 since 2005
	// - Intel integrated GPUs have supported OpenGL 2.1 since Clarkdale (maybe earlier).
	// And we've already been using features from OpenGL 2.1. Nobody has complained yet.
	// So checking for 2.1 support should be fine.
	if (!GLEW_VERSION_2_1)
	{
		return Error("  gl: OpenGL Version 2.1 or higher required. A better graphics driver will probably help.");
	}
	return ok;
}

void CStdGL::SetupMultiBlt(C4ShaderCall& call, const C4BltTransform* pTransform, GLuint baseTex, GLuint overlayTex, GLuint normalTex, DWORD dwOverlayModClr)
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

	if(baseTex != 0)
	{
		call.AllocTexUnit(C4SSU_BaseTex, GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, baseTex);
	}

	if(overlayTex != 0)
	{
		call.AllocTexUnit(C4SSU_OverlayTex, GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, overlayTex);

		const float fOverlayModClr[4] = {
			((dwOverlayModClr >> 16) & 0xff) / 255.0f,
			((dwOverlayModClr >>  8) & 0xff) / 255.0f,
			((dwOverlayModClr      ) & 0xff) / 255.0f,
			((dwOverlayModClr >> 24) & 0xff) / 255.0f
		};

		call.SetUniform4fv(C4SSU_OverlayClr, 1, fOverlayModClr);
	}

	if(pFoW != NULL && normalTex != 0)
	{
		call.AllocTexUnit(C4SSU_NormalTex, GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, normalTex);
	}

	if(pFoW != NULL)
	{
		const C4Rect OutRect = GetOutRect();
		const C4Rect ClipRect = GetClipRect();
		const FLOAT_RECT vpRect = pFoW->getViewportRegion();

		// Dynamic Light
		call.AllocTexUnit(C4SSU_LightTex, GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, pFoW->getSurface()->textures[0].texName);

		float lightTransform[6];
		pFoW->GetFragTransform(ClipRect, OutRect, lightTransform);
		call.SetUniformMatrix2x3fv(C4SSU_LightTransform, 1, lightTransform);

		// Ambient Light
		call.AllocTexUnit(C4SSU_AmbientTex, GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, pFoW->getFoW()->Ambient.Tex);
		call.SetUniform1f(C4SSU_AmbientBrightness, pFoW->getFoW()->Ambient.GetBrightness());

		float ambientTransform[6];
		pFoW->getFoW()->Ambient.GetFragTransform(vpRect, ClipRect, OutRect, ambientTransform);
		call.SetUniformMatrix2x3fv(C4SSU_AmbientTransform, 1, ambientTransform);
	}

	// Apply zoom and transform
	glPushMatrix();
	glTranslatef(ZoomX, ZoomY, 0.0f);
	// Scale Z as well so that we don't distort normals.
	glScalef(Zoom, Zoom, Zoom);
	glTranslatef(-ZoomX, -ZoomY, 0.0f);

	if(pTransform)
	{
		// Decompose scale factors and scale Z accordingly to X and Y, again to avoid distorting normals
		// We could instead work around this by using the projection matrix, but then for object rotations (SetR)
		// the normals would not be correct.
		const float sx = sqrt(pTransform->mat[0]*pTransform->mat[0] + pTransform->mat[1]*pTransform->mat[1]);
		const float sy = sqrt(pTransform->mat[3]*pTransform->mat[3] + pTransform->mat[4]*pTransform->mat[4]);
		const float sz = sqrt(sx * sy);
		const GLfloat transform[16] = { pTransform->mat[0], pTransform->mat[3], 0, pTransform->mat[6], pTransform->mat[1], pTransform->mat[4], 0, pTransform->mat[7], 0, 0, sz, 0, pTransform->mat[2], pTransform->mat[5], 0, pTransform->mat[8] };
		glMultMatrixf(transform);
	}
}

void CStdGL::ResetMultiBlt()
{
	glPopMatrix();
}

void CStdGL::PerformMultiPix(C4Surface* sfcTarget, const C4BltVertex* vertices, unsigned int n_vertices)
{
	// Only direct rendering
	assert(sfcTarget->IsRenderTarget());
	if(!PrepareRendering(sfcTarget)) return;

	// Draw on pixel center:
	glPushMatrix();
	glTranslatef(0.5f, 0.5f, 0.0f);

	// Feed the vertices to the GL
	C4ShaderCall call(GetSpriteShader(false, false, false));
	SetupMultiBlt(call, NULL, 0, 0, 0, 0);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	// This is a workaround. Instead of submitting the whole vertex array to the GL, we do it
	// in batches of 256 vertices. The intel graphics driver on Linux crashes with
	// significantly larger arrays, such as 400. It's not clear to me why, maybe POINT drawing
	// is just not very well tested.
	const unsigned int BATCH_SIZE = 256;
	for(unsigned int i = 0; i < n_vertices; i += BATCH_SIZE)
	{
		glVertexPointer(2, GL_FLOAT, sizeof(C4BltVertex), &vertices[i].ftx);
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(C4BltVertex), &vertices[i].color[0]);
		glDrawArrays(GL_POINTS, 0, std::min(n_vertices - i, BATCH_SIZE));
	}

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glPopMatrix();

	ResetMultiBlt();
}

void CStdGL::PerformMultiLines(C4Surface* sfcTarget, const C4BltVertex* vertices, unsigned int n_vertices, float width)
{
	// Only direct rendering
	assert(sfcTarget->IsRenderTarget());
	if(!PrepareRendering(sfcTarget)) return;

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
	C4ShaderCall call(GetSpriteShader(true, false, false));
	SetupMultiBlt(call, NULL, lines_tex, 0, 0, 0);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glClientActiveTexture(GL_TEXTURE0);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(2, GL_FLOAT, sizeof(C4BltVertex), &tri_vertices[0].tx);
	glVertexPointer(2, GL_FLOAT, sizeof(C4BltVertex), &tri_vertices[0].ftx);
	glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(C4BltVertex), &tri_vertices[0].color[0]);
	glDrawArrays(GL_TRIANGLES, 0, n_vertices * 3);
	delete[] tri_vertices;

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	ResetMultiBlt();
}

void CStdGL::PerformMultiTris(C4Surface* sfcTarget, const C4BltVertex* vertices, unsigned int n_vertices, const C4BltTransform* pTransform, C4TexRef* pTex, C4TexRef* pOverlay, C4TexRef* pNormal, DWORD dwOverlayModClr)
{
	// Only direct rendering
	assert(sfcTarget->IsRenderTarget());
	if(!PrepareRendering(sfcTarget)) return;

	// Feed the vertices to the GL
	C4ShaderCall call(GetSpriteShader(pTex != NULL, pOverlay != NULL, pNormal != NULL));
	SetupMultiBlt(call, pTransform, pTex ? pTex->texName : 0, pOverlay ? pOverlay->texName : 0, pNormal ? pNormal->texName : 0, dwOverlayModClr);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	if(pTex)
	{
		// We use the texture coordinate array in texture0 for
		// the base, overlay and normal textures
		glClientActiveTexture(GL_TEXTURE0);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, sizeof(C4BltVertex), &vertices[0].tx);
	}

	glVertexPointer(2, GL_FLOAT, sizeof(C4BltVertex), &vertices[0].ftx);
	glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(C4BltVertex), &vertices[0].color[0]);
	glDrawArrays(GL_TRIANGLES, 0, n_vertices);

	if(pTex) glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	ResetMultiBlt();
}

bool CStdGL::CreateSpriteShader(C4Shader& shader, const char* name, int ssc, C4GroupSet* pGroups)
{
	static const char vertexSlice[] = 
		"  gl_FrontColor = gl_Color;"
		"  gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;";

	const char* uniformNames[C4SSU_Count + 1];
	uniformNames[C4SSU_ClrMod] = "clrMod";
	uniformNames[C4SSU_BaseTex] = "baseTex";
	uniformNames[C4SSU_OverlayTex] = "overlayTex";
	uniformNames[C4SSU_OverlayClr] = "overlayClr";
	uniformNames[C4SSU_LightTex] = "lightTex";
	uniformNames[C4SSU_LightTransform] = "lightTransform";
	uniformNames[C4SSU_NormalTex] = "normalTex";
	uniformNames[C4SSU_AmbientTex] = "ambientTex";
	uniformNames[C4SSU_AmbientTransform] = "ambientTransform";
	uniformNames[C4SSU_AmbientBrightness] = "ambientBrightness";
	uniformNames[C4SSU_Bones] = "bones";
	uniformNames[C4SSU_Count] = NULL;

	// Clear previous content
	shader.Clear();
	shader.ClearSlices();

	shader.AddVertexSlice(C4Shader_Vertex_PositionPos, vertexSlice);

	// Add texture coordinate if we have base texture, overlay, or normal map
	if ( (ssc & (C4SSC_BASE | C4SSC_OVERLAY | C4SSC_NORMAL)) != 0)
		shader.AddTexCoord("texcoord");

	// Then load slices for fragment shader
	shader.AddFragmentSlice(-1, "#define OPENCLONK");
	if (ssc & C4SSC_MOD2) shader.AddFragmentSlice(-1, "#define CLRMOD_MOD2");
	if (ssc & C4SSC_NORMAL) shader.AddFragmentSlice(-1, "#define HAVE_NORMALMAP");
	if (ssc & C4SSC_LIGHT) shader.AddFragmentSlice(-1, "#define HAVE_LIGHT");
	shader.LoadSlices(pGroups, "UtilShader.glsl");
	shader.LoadSlices(pGroups, "ObjectBaseShader.glsl");

	if (ssc & C4SSC_BASE) shader.LoadSlices(pGroups, "SpriteTextureShader.glsl");
	if (ssc & C4SSC_OVERLAY) shader.LoadSlices(pGroups, "SpriteOverlayShader.glsl");

	// In case light is disabled, these shaders use a default light source
	// (typically ambient light everywhere).
	shader.LoadSlices(pGroups, "ObjectLightShader.glsl");
	shader.LoadSlices(pGroups, "LightShader.glsl");
	shader.LoadSlices(pGroups, "AmbientShader.glsl");

	if (!shader.Init(name, uniformNames))
	{
		shader.ClearSlices();
		return false;
	}

	return true;
}

C4Shader* CStdGL::GetSpriteShader(bool haveBase, bool haveOverlay, bool haveNormal)
{
	int ssc = 0;
	if(dwBlitMode & C4GFXBLIT_MOD2) ssc |= C4SSC_MOD2;
	if(haveBase) ssc |= C4SSC_BASE;
	if(haveBase && haveOverlay) ssc |= C4SSC_OVERLAY;
	if(pFoW != NULL) ssc |= C4SSC_LIGHT;
	if(pFoW != NULL && haveBase && haveNormal) ssc |= C4SSC_NORMAL;
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
	// Create sprite blitting shaders
	if(!CreateSpriteShader(SpriteShader, "sprite", 0, pGroups))
		return false;
	if(!CreateSpriteShader(SpriteShaderMod2, "spriteMod2", C4SSC_MOD2, pGroups))
		return false;
	if(!CreateSpriteShader(SpriteShaderBase, "spriteBase", C4SSC_BASE, pGroups))
		return false;
	if(!CreateSpriteShader(SpriteShaderBaseMod2, "spriteBaseMod2", C4SSC_MOD2 | C4SSC_BASE, pGroups))
		return false;
	if(!CreateSpriteShader(SpriteShaderBaseOverlay, "spriteBaseOverlay", C4SSC_BASE | C4SSC_OVERLAY, pGroups))
		return false;
	if(!CreateSpriteShader(SpriteShaderBaseOverlayMod2, "spriteBaseOverlayMod2", C4SSC_MOD2 | C4SSC_BASE | C4SSC_OVERLAY, pGroups))
		return false;

	if(!CreateSpriteShader(SpriteShaderLight, "spriteLight", C4SSC_LIGHT, pGroups))
		return false;
	if(!CreateSpriteShader(SpriteShaderLightMod2, "spriteLightMod2", C4SSC_LIGHT | C4SSC_MOD2, pGroups))
		return false;
	if(!CreateSpriteShader(SpriteShaderLightBase, "spriteLightBase", C4SSC_LIGHT | C4SSC_BASE, pGroups))
		return false;
	if(!CreateSpriteShader(SpriteShaderLightBaseMod2, "spriteLightBaseMod2", C4SSC_LIGHT | C4SSC_BASE | C4SSC_MOD2, pGroups))
		return false;
	if(!CreateSpriteShader(SpriteShaderLightBaseOverlay, "spriteLightBaseOverlay", C4SSC_LIGHT | C4SSC_BASE | C4SSC_OVERLAY, pGroups))
		return false;
	if(!CreateSpriteShader(SpriteShaderLightBaseOverlayMod2, "spriteLightBaseOverlayMod2", C4SSC_LIGHT | C4SSC_BASE | C4SSC_OVERLAY | C4SSC_MOD2, pGroups))
		return false;
	if(!CreateSpriteShader(SpriteShaderLightBaseNormal, "spriteLightBaseNormal", C4SSC_LIGHT | C4SSC_BASE | C4SSC_NORMAL, pGroups))
		return false;
	if(!CreateSpriteShader(SpriteShaderLightBaseNormalMod2, "spriteLightBaseNormalMod2", C4SSC_LIGHT | C4SSC_BASE | C4SSC_NORMAL | C4SSC_MOD2, pGroups))
		return false;
	if(!CreateSpriteShader(SpriteShaderLightBaseNormalOverlay, "spriteLightBaseNormalOverlay", C4SSC_LIGHT | C4SSC_BASE | C4SSC_OVERLAY | C4SSC_NORMAL, pGroups))
		return false;
	if(!CreateSpriteShader(SpriteShaderLightBaseNormalOverlayMod2, "spriteLightBaseNormalOverlayMod2", C4SSC_LIGHT | C4SSC_BASE | C4SSC_OVERLAY | C4SSC_NORMAL | C4SSC_MOD2, pGroups))
		return false;

	return true;
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
	const char * linedata = byByteCnt == 2 ? "\xff\xf0\xff\xff" : "\xff\xff\xff\x00\xff\xff\xff\xff";
	glTexImage2D(GL_TEXTURE_2D, 0, 4, 1, 2, 0, GL_BGRA, byByteCnt == 2 ? GL_UNSIGNED_SHORT_4_4_4_4_REV : GL_UNSIGNED_INT_8_8_8_8_REV, linedata);

	MaxTexSize = 64;
	GLint s = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &s);
	if (s>0) MaxTexSize = s;

	// restore gamma if active
	if (Active)
		EnableGamma();
	// reset blit states
	dwBlitMode = 0;

	// done
	return Active;
}

bool CStdGL::InvalidateDeviceObjects()
{
	bool fSuccess=true;
	// clear gamma
#ifndef USE_SDL_MAINLOOP
	DisableGamma();
#endif
	// deactivate
	Active=false;
	// invalidate font objects
	// invalidate primary surfaces
	if (lines_tex)
	{
		glDeleteTextures(1, &lines_tex);
		lines_tex = 0;
	}
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
		NULL,
		err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0, NULL );
	LogF("  gl: GetLastError() = %d - %s", err, StdStrBuf(lpMsgBuf).getData());
	LocalFree(lpMsgBuf);
#endif
	LogF("  gl: %s", glGetString(GL_VENDOR));
	LogF("  gl: %s", glGetString(GL_RENDERER));
	LogF("  gl: %s", glGetString(GL_VERSION));
	LogF("  gl: %s", glGetString(GL_EXTENSIONS));
	return r;
}

bool CStdGL::CheckGLError(const char *szAtOp)
{
	GLenum err = glGetError();
	if (!err) return true;

#ifdef USE_WIN32_WINDOWS
	StdStrBuf err_buf(gluErrorUnicodeStringEXT(err));
#else
	// gluErrorString returns latin-1 strings. Our code expects UTF-8, so convert
	// Also for some reason gluErrorString returns const GLubyte* instead of a more
	// reasonable const char *, so cast it - C-style cast required here to match
	// both unsigned and signed char
	StdStrBuf err_buf((const char*)gluErrorString(err));
	err_buf.EnsureUnicode();
#endif

	LogF("GL error with %s: %d - %s", szAtOp, err, err_buf.getData());
	return false;
}

CStdGL *pGL=NULL;

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
	pCurrCtx = NULL;
	iPixelFormat=0;
	sfcFmt=0;
	iClrDpt=0;
	Workarounds.LowMaxVertexUniformCount = false;
}

#endif // USE_CONSOLE
