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
#include "C4Rect.h"
#include "C4Config.h"
#include "C4Application.h"

#ifndef USE_CONSOLE

// MSVC doesn't define M_PI in math.h unless requested
#ifdef  _MSC_VER
#define _USE_MATH_DEFINES
#endif  /* _MSC_VER */

#include <stdio.h>
#include <math.h>
#include <limits.h>

C4DrawGLShader::C4DrawGLShader(Type shader_type)
{
	GLint gl_type;
	switch(shader_type)
	{
	case FRAGMENT: gl_type = GL_FRAGMENT_SHADER_ARB; break;
	case VERTEX: gl_type = GL_VERTEX_SHADER_ARB; break;
	case GEOMETRY: gl_type = GL_GEOMETRY_SHADER_ARB; break;
	default: assert(false); break;
	}

	Shader = glCreateShaderObjectARB(gl_type);
	if(!Shader) throw C4DrawGLError(FormatString("Failed to create shader")); // TODO: custom error class?
}

C4DrawGLShader::~C4DrawGLShader()
{
	glDeleteObjectARB(Shader);
}

void C4DrawGLShader::Load(const char* code)
{
	glShaderSourceARB(Shader, 1, &code, NULL);
	glCompileShaderARB(Shader);

	GLint compile_status;
	glGetObjectParameterivARB(Shader, GL_OBJECT_COMPILE_STATUS_ARB, &compile_status);
	if(compile_status != GL_TRUE)
	{
		const char* shader_type_str;
		switch(GetType())
		{
		case VERTEX: shader_type_str = "vertex"; break;
		case FRAGMENT: shader_type_str = "fragment"; break;
		case GEOMETRY: shader_type_str = "geometry"; break;
		default: assert(false); break;
		}

		GLint length;
		glGetObjectParameterivARB(Shader, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
		if(length > 0)
		{
			std::vector<char> error_message(length);
			glGetInfoLogARB(Shader, length, NULL, &error_message[0]);
			throw C4DrawGLError(FormatString("Failed to compile %s shader: %s", shader_type_str, &error_message[0]));
		}
		else
		{
			throw C4DrawGLError(FormatString("Failed to compile %s shader", shader_type_str));
		}
	}
}

StdMeshMaterialShader::Type C4DrawGLShader::GetType() const
{
	GLint shader_type;
	glGetObjectParameterivARB(Shader, GL_OBJECT_SUBTYPE_ARB, &shader_type);

	switch(shader_type)
	{
	case GL_FRAGMENT_SHADER_ARB: return FRAGMENT;
	case GL_VERTEX_SHADER_ARB: return VERTEX;
	case GL_GEOMETRY_SHADER_ARB: return GEOMETRY;
	default: assert(false); return static_cast<StdMeshMaterialShader::Type>(-1);
	}
}

C4DrawGLProgram::C4DrawGLProgram(const C4DrawGLShader* fragment_shader, const C4DrawGLShader* vertex_shader, const C4DrawGLShader* geometry_shader)
{
	Program = glCreateProgramObjectARB();
	if(fragment_shader != NULL)
		glAttachObjectARB(Program, fragment_shader->Shader);
	if(vertex_shader != NULL)
		glAttachObjectARB(Program, vertex_shader->Shader);
	if(geometry_shader != NULL)
		glAttachObjectARB(Program, geometry_shader->Shader);
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
			throw C4DrawGLError(FormatString("Failed to link program: %s", &error_message[0]));
		}
		else
		{
			glDeleteObjectARB(Program);
			throw C4DrawGLError(StdStrBuf("Failed to link program"));
		}
	}
}

C4DrawGLProgram::~C4DrawGLProgram()
{
	glDeleteObjectARB(Program);
}

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
	int iWdt=Min(iClipX2, RenderTarget->Wdt-1)-iClipX1+1;
	int iHgt=Min(iClipY2, RenderTarget->Hgt-1)-iClipY1+1;
	int iX=iClipX1; if (iX<0) { iWdt+=iX; iX=0; }
	int iY=iClipY1; if (iY<0) { iHgt+=iY; iY=0; }

	if (iWdt<=0 || iHgt<=0)
	{
		ClipAll=true;
		return true;
	}
	ClipAll=false;
	// set it
	glViewport(iX, RenderTarget->Hgt-iY-iHgt, iWdt, iHgt);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// Set clipping plane to -1000 and 1000 so that large meshes are not
	// clipped away.
	//glOrtho((GLdouble) iX, (GLdouble) (iX+iWdt), (GLdouble) (iY+iHgt), (GLdouble) iY, -1000.0f, 1000.0f);
	gluOrtho2D((GLdouble) iX, (GLdouble) (iX+iWdt), (GLdouble) (iY+iHgt), (GLdouble) iY);
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
	if (!pMainCtx) pMainCtx = pCtx;
	if (!pCtx->Init(pWindow, pApp))
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

bool CStdGL::CreatePrimarySurfaces(bool, unsigned int, unsigned int, int iColorDepth, unsigned int)
{
	// store options
	return RestoreDeviceObjects();
}

void CStdGL::DrawQuadDw(C4Surface * sfcTarget, float *ipVtx, DWORD dwClr1, DWORD dwClr2, DWORD dwClr3, DWORD dwClr4)
{
	// prepare rendering to target
	if (!PrepareRendering(sfcTarget)) return;
	// apply global modulation
	ClrByCurrentBlitMod(dwClr1);
	ClrByCurrentBlitMod(dwClr2);
	ClrByCurrentBlitMod(dwClr3);
	ClrByCurrentBlitMod(dwClr4);
	// apply modulation map
	if (fUseClrModMap)
	{
		ModulateClr(dwClr1, pClrModMap->GetModAt(int(ipVtx[0]), int(ipVtx[1])));
		ModulateClr(dwClr2, pClrModMap->GetModAt(int(ipVtx[2]), int(ipVtx[3])));
		ModulateClr(dwClr3, pClrModMap->GetModAt(int(ipVtx[4]), int(ipVtx[5])));
		ModulateClr(dwClr4, pClrModMap->GetModAt(int(ipVtx[6]), int(ipVtx[7])));
	}
	glShadeModel((dwClr1 == dwClr2 && dwClr1 == dwClr3 && dwClr1 == dwClr4) ? GL_FLAT : GL_SMOOTH);
	// set blitting state
	int iAdditive = dwBlitMode & C4GFXBLIT_ADDITIVE;
	glBlendFunc(GL_SRC_ALPHA, iAdditive ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA);
	// draw two triangles
	glInterleavedArrays(GL_V2F, sizeof(float)*2, ipVtx);
	GLubyte colors[4][4];
	DwTo4UB(dwClr1,colors[0]);
	DwTo4UB(dwClr2,colors[1]);
	DwTo4UB(dwClr3,colors[2]);
	DwTo4UB(dwClr4,colors[3]);
	glColorPointer(4,GL_UNSIGNED_BYTE,0,colors);
	glEnableClientState(GL_COLOR_ARRAY);
	glDrawArrays(GL_POLYGON, 0, 4);
	glDisableClientState(GL_COLOR_ARRAY);
	glShadeModel(GL_FLAT);
}

#ifdef _MSC_VER
#ifdef _M_X64
# include <emmintrin.h>
#endif
static inline long int lrintf(float f)
{
#ifdef _M_X64
	return _mm_cvtt_ss2si(_mm_load_ps1(&f));
#else
	long int i;
	__asm
	{
		fld f
		fistp i
	};
	return i;
#endif
}
#endif

void CStdGL::SetupMultiBlt(const C4BltTransform* pTransform, GLuint baseTex, GLuint overlayTex, DWORD dwOverlayModClr)
{
	// Initialize multi blit shader. If pTexRef is given, use that as texture.
	int iAdditive = dwBlitMode & C4GFXBLIT_ADDITIVE;
	glBlendFunc(GL_SRC_ALPHA, iAdditive ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA);

	// TODO: The locations could be cached
	GLint fMod2Location = glGetUniformLocationARB(multi_blt_program->Program, "fMod2");
	GLint fUseClrModMapLocation = glGetUniformLocationARB(multi_blt_program->Program, "fUseClrModMap");
	GLint fUseTextureLocation = glGetUniformLocationARB(multi_blt_program->Program, "fUseTexture");
	GLint fUseOverlayLocation = glGetUniformLocationARB(multi_blt_program->Program, "fUseOverlay");
	GLint clrModLocation = glGetUniformLocationARB(multi_blt_program->Program, "clrMod");
	GLint overlayClrModLocation = glGetUniformLocationARB(multi_blt_program->Program, "overlayClrMod");
	GLint clrModMapLocation = glGetUniformLocationARB(multi_blt_program->Program, "ClrModMap");
	GLint textureLocation = glGetUniformLocationARB(multi_blt_program->Program, "Texture");
	GLint overlayLocation = glGetUniformLocationARB(multi_blt_program->Program, "Overlay");

	const int fMod2 = (dwBlitMode & C4GFXBLIT_MOD2) != 0;
	const int fUseClrModMap = this->fUseClrModMap;
	const int fUseTexture = (baseTex != 0);
	const int fUseOverlay = (overlayTex != 0);
	const DWORD dwModClr = BlitModulated ? BlitModulateClr : 0xffffffff;
	const float dwMod[4] = {
		((dwModClr >> 16) & 0xff) / 255.0f,
		((dwModClr >>  8) & 0xff) / 255.0f,
		((dwModClr      ) & 0xff) / 255.0f,
		((dwModClr >> 24) & 0xff) / 255.0f
	};
	const float dwOverlayMod[4] = {
		((dwOverlayModClr >> 16) & 0xff) / 255.0f,
		((dwOverlayModClr >>  8) & 0xff) / 255.0f,
		((dwOverlayModClr      ) & 0xff) / 255.0f,
		((dwOverlayModClr >> 24) & 0xff) / 255.0f
	};

	glUseProgramObjectARB(multi_blt_program->Program);
	glUniform1iARB(fMod2Location, fMod2);
	glUniform1iARB(fUseClrModMapLocation, fUseClrModMap);
	glUniform1iARB(fUseTextureLocation, fUseTexture);
	glUniform1iARB(fUseOverlayLocation, fUseOverlay);
	glUniform4fvARB(clrModLocation, 1, dwMod);

	if(fUseClrModMap)
	{
		glActiveTexture(GL_TEXTURE2);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, pClrModMap->GetSurface()->ppTex[0]->texName);
		glUniform1iARB(clrModMapLocation, 2);

		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		C4Surface * pSurface = pClrModMap->GetSurface();
		glScalef(1.0f/(pClrModMap->GetResolutionX()*(*pSurface->ppTex)->iSizeX), 1.0f/(pClrModMap->GetResolutionY()*(*pSurface->ppTex)->iSizeY), 1.0f);
		glTranslatef(float(-pClrModMap->OffX), float(-pClrModMap->OffY), 0.0f);
		// Zoom and transform are applied in the modelview matrix, which
		// is applied in the vertex shader before passing the viewport
		// position to the fragment shader for clrmodmap lookup
		glMatrixMode(GL_MODELVIEW);
	}

	if(overlayTex != 0)
	{
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, overlayTex);
		glUniform1iARB(overlayLocation, 1);
		glUniform4fvARB(overlayClrModLocation, 1, dwOverlayMod);
	}

	glActiveTexture(GL_TEXTURE0);
	if(baseTex != 0)
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, baseTex);
		glUniform1iARB(textureLocation, 0);
	}

	// Apply zoom and transform
	glPushMatrix();
	glTranslatef(ZoomX, ZoomY, 0.0f);
	glScalef(Zoom, Zoom, 1.0f);
	glTranslatef(-ZoomX, -ZoomY, 0.0f);

	if(pTransform)
	{
		const GLfloat transform[16] = { pTransform->mat[0], pTransform->mat[3], 0, pTransform->mat[6], pTransform->mat[1], pTransform->mat[4], 0, pTransform->mat[7], 0, 0, 1, 0, pTransform->mat[2], pTransform->mat[5], 0, pTransform->mat[8] };
		glMultMatrixf(transform);
	}
}

void CStdGL::ResetMultiBlt(GLuint baseTex, GLuint overlayTex)
{
	glPopMatrix();
	if(fUseClrModMap) { glActiveTexture(GL_TEXTURE2); glDisable(GL_TEXTURE_2D); }
	if(overlayTex != 0) { glActiveTexture(GL_TEXTURE1); glDisable(GL_TEXTURE_2D); }
	glActiveTexture(GL_TEXTURE0);
	if(baseTex != 0) glDisable(GL_TEXTURE_2D);
	glUseProgramObjectARB(0);
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
	SetupMultiBlt(NULL, 0, 0, 0);

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

	ResetMultiBlt(0, 0);
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
	SetupMultiBlt(NULL, lines_tex, 0, 0);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glClientActiveTexture(GL_TEXTURE0); // lines_tex was loaded in tex0 by SetupMultiBlt
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(2, GL_FLOAT, sizeof(C4BltVertex), &tri_vertices[0].tx);
	glVertexPointer(2, GL_FLOAT, sizeof(C4BltVertex), &tri_vertices[0].ftx);
	glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(C4BltVertex), &tri_vertices[0].color[0]);
	glDrawArrays(GL_TRIANGLES, 0, n_vertices * 3);
	delete[] tri_vertices;

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	ResetMultiBlt(lines_tex, 0);
}

void CStdGL::PerformMultiTris(C4Surface* sfcTarget, const C4BltVertex* vertices, unsigned int n_vertices, const C4BltTransform* pTransform, C4TexRef* pTex, C4TexRef* pOverlay, DWORD dwOverlayModClr)
{
	// Only direct rendering
	assert(sfcTarget->IsRenderTarget());
	if(!PrepareRendering(sfcTarget)) return;

	// Feed the vertices to the GL
	SetupMultiBlt(pTransform, pTex ? pTex->texName : 0, pOverlay ? pOverlay->texName : 0, dwOverlayModClr);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	if(pTex)
	{
		// We use the texture coordinate array in texture0 for
		// both the base and the overlay texture
		glClientActiveTexture(GL_TEXTURE0); // pTex was loaded in tex0 by SetupMultiBlt
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, sizeof(C4BltVertex), &vertices[0].tx);
	}

	glVertexPointer(2, GL_FLOAT, sizeof(C4BltVertex), &vertices[0].ftx);
	glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(C4BltVertex), &vertices[0].color[0]);
	glDrawArrays(GL_TRIANGLES, 0, n_vertices);

	if(pTex) glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	ResetMultiBlt(pTex ? pTex->texName : 0, pOverlay ? pOverlay->texName : 0);
}

bool CStdGL::RestoreDeviceObjects()
{
	assert(pMainCtx);
	// delete any previous objects
	InvalidateDeviceObjects();

	// set states
	Active = pMainCtx->Select();
	RenderTarget = pApp->pWindow->pSurface;

	// TODO: I think this should be updated. We need at least GLSL shaders now, which I think is OpenGL 2.0(?)
	// BGRA Pixel Formats, Multitexturing, Texture Combine Environment Modes
	// Check for GL 1.2 and two functions from 1.3 we need.
	if( !GLEW_VERSION_1_2 ||
		glActiveTexture == NULL ||
		glClientActiveTexture == NULL
	) {
		return Error("  gl: OpenGL Version 1.3 or higher required. A better graphics driver will probably help.");
	}

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

	// The following shaders are used for drawing primitives such as points, lines and sprites.
	// They are used in PerformMultiPix, PerformMultiLines and PerformMultiTris.
	// The fragment shader applies the color modulation, mod2 drawing and the color modulation map
	// on top of the original fragment color.
	// The vertex shader does not do anything special, but it delegates input values to the
	// fragment shader.
	// TODO: It might be more efficient to use separate shaders for pixels, lines and tris.
	const char* vertex_shader_text =
		"varying vec2 texcoord;"
		"varying vec2 pos;"
		"void main()"
		"{"
		"  texcoord = gl_MultiTexCoord0.xy;"
		"  pos = (gl_ModelViewMatrix * gl_Vertex).xy;"
		"  gl_FrontColor = gl_Color;"
		"  gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;"
		"}";
	const char* fragment_shader_text =
		"uniform int fMod2;"
		"uniform int fUseClrModMap;"
		"uniform int fUseTexture;"
		"uniform int fUseOverlay;"
		"uniform vec4 clrMod;"
		"uniform vec4 overlayClrMod;"
		"uniform sampler2D Texture;"
		"uniform sampler2D Overlay;"
		"uniform sampler2D ClrModMap;"
		"varying vec2 texcoord;"
		"varying vec2 pos;"
		"void main()"
		"{"
                // Start with the base color
                "  vec4 primaryColor = gl_Color;"
                // Get texture
		"  if(fUseTexture != 0)"
		"    primaryColor = primaryColor * texture2D(Texture, texcoord);"
                // Get overlay, if any
                "  vec4 overlayColor = vec4(1.0, 1.0, 1.0, 0.0);"
                "  if(fUseOverlay != 0)"
                "    overlayColor = gl_Color * texture2D(Overlay, texcoord);"
                // Mix base with overlay, and apply clrmod (separately for base and overlay)
                "  primaryColor.rgb = overlayColor.a * overlayClrMod.rgb * overlayColor.rgb + (1.0 - overlayColor.a) * clrMod.rgb * primaryColor.rgb;"
		// Add alpha for base and overlay, and use weighted mean of clrmod alpha
                "  primaryColor.a = clamp(primaryColor.a + overlayColor.a, 0.0, 1.0) * (primaryColor.a * clrMod.a + overlayColor.a * overlayClrMod.a) / (primaryColor.a + overlayColor.a);"
                // Add fog of war
		"  vec4 clrModMapClr = vec4(1.0, 1.0, 1.0, 1.0);"
		"  if(fUseClrModMap != 0)"
		"    clrModMapClr = texture2D(ClrModMap, pos);"
                // Final output, depending on blit mode
		"  if(fMod2 != 0)"
		"    gl_FragColor = clamp(2.0 * primaryColor * clrModMapClr - 0.5, 0.0, 1.0);"
		"  else"
		"    gl_FragColor = primaryColor * clrModMapClr;"
		"}";

	C4DrawGLShader vertex_shader(StdMeshMaterialShader::VERTEX);
	vertex_shader.Load(vertex_shader_text);

	C4DrawGLShader fragment_shader(StdMeshMaterialShader::FRAGMENT);
	fragment_shader.Load(fragment_shader_text);

	multi_blt_program.reset(new C4DrawGLProgram(&fragment_shader, &vertex_shader, NULL));

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
	multi_blt_program.reset(NULL);
	// invalidate font objects
	// invalidate primary surfaces
	if (lines_tex)
	{
		glDeleteTextures(1, &lines_tex);
		lines_tex = 0;
	}
	return fSuccess;
}

bool CStdGL::EnsureAnyContext()
{
	// Make sure some context is selected
	if (pCurrCtx) return true;
	if (!pMainCtx) return false;
	return pMainCtx->Select();
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
	Log(szAtOp);
	switch (err)
	{
	case GL_INVALID_ENUM:       Log("GL_INVALID_ENUM"); break;
	case GL_INVALID_VALUE:      Log("GL_INVALID_VALUE"); break;
	case GL_INVALID_OPERATION:  Log("GL_INVALID_OPERATION"); break;
	case GL_STACK_OVERFLOW:     Log("GL_STACK_OVERFLOW"); break;
	case GL_STACK_UNDERFLOW:    Log("GL_STACK_UNDERFLOW"); break;
	case GL_OUT_OF_MEMORY:      Log("GL_OUT_OF_MEMORY"); break;
	default: Log("unknown error"); break;
	}
	return false;
}

CStdGL *pGL=NULL;

#ifdef USE_WIN32_WINDOWS
void CStdGL::TaskOut()
{
	if (pCurrCtx) pCurrCtx->Deselect();
}
#endif

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
}

#endif // USE_CONSOLE
