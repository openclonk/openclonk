/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2006  Sven Eberhardt
 * Copyright (c) 2005-2007  Günther Brammer
 * Copyright (c) 2006  Julian Raschke
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

/* OpenGL implementation of NewGfx, the context */

#include "C4Include.h"
#include <Standard.h>
#include <StdGL.h>
#include <StdSurface2.h>
#include <StdWindow.h>

#ifdef USE_GL

void CStdGLCtx::SelectCommon()
	{
	pGL->pCurrCtx = this;
	// update size
	UpdateSize();
	// assign size
	pGL->lpPrimary->Wdt=cx; pGL->lpPrimary->Hgt=cy;
	// set some default states
	glDisable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glShadeModel(GL_FLAT);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	//glEnable(GL_LINE_SMOOTH);
	//glHint(GL_LINE_SMOOTH_HINT, GL_FASTEST);
	//glEnable(GL_POINT_SMOOTH);
	}

#ifdef _WIN32
CStdGLCtx::CStdGLCtx(): hrc(0), pWindow(0), hDC(0), cx(0), cy(0) { }

void CStdGLCtx::Clear()
	{
	if (hrc)
		{
		Deselect();
		wglDeleteContext(hrc); hrc=0;
		}
	if (hDC)
		{
		ReleaseDC(pWindow ? pWindow->hWindow : hWindow, hDC);
		hDC=0;
		}
	pWindow = 0; cx = cy = 0; hWindow = NULL;
	}

bool CStdGLCtx::Init(CStdWindow * pWindow, CStdApp *pApp, HWND hWindow)
	{
	// safety
	if (!pGL) return false;

	// store window
	this->pWindow = pWindow;
	// default HWND
	if (pWindow) hWindow = pWindow->hWindow; else this->hWindow = hWindow;

	// get DC
	hDC=GetDC(hWindow);
	if (!hDC) return !!pGL->Error("  gl: Error getting DC");

	PIXELFORMATDESCRIPTOR &rPfd = pApp->GetPFD();
	if (!pGL->iPixelFormat)
		{
		// pixel format
		memset(&rPfd, 0, sizeof(PIXELFORMATDESCRIPTOR)) ;
		rPfd.nSize      = sizeof(PIXELFORMATDESCRIPTOR);
		rPfd.nVersion   = 1 ;
		rPfd.dwFlags    = PFD_DOUBLEBUFFER | /*(pGL->fFullscreen ? PFD_SWAP_EXCHANGE : 0) |*/
	                        PFD_SUPPORT_OPENGL |
	                        PFD_DRAW_TO_WINDOW ;
		rPfd.iPixelType = PFD_TYPE_RGBA;
		rPfd.cColorBits = pGL->iClrDpt;
		rPfd.cDepthBits = 0;
		rPfd.iLayerType = PFD_MAIN_PLANE;
		pGL->iPixelFormat = ChoosePixelFormat(hDC, &rPfd);
		if (!pGL->iPixelFormat) return !!pGL->Error("  gl: Error getting pixel format");
		}
	if (!SetPixelFormat (hDC, pGL->iPixelFormat, &rPfd)) pGL->Error("  gl: Error setting pixel format");

	// create context
	hrc = wglCreateContext(hDC); if (!hrc) return !!pGL->Error("  gl: Error creating gl context");
	//if (this != &pGL->MainCtx) wglCopyContext(pGL->MainCtx.hrc, hrc, GL_ALL_ATTRIB_BITS);

	// share textures
	wglMakeCurrent(NULL, NULL); pGL->pCurrCtx=NULL;
	if (this != &pGL->MainCtx)
		{
		if (!wglShareLists(pGL->MainCtx.hrc, hrc)) pGL->Error("  gl: Textures for secondary context not available");
		return true;
		}

	// select
	if (!Select()) return !!pGL->Error("  gl: Unable to select context");

	// init extensions
	GLenum err = glewInit();
	if (GLEW_OK != err)
		{
		// Problem: glewInit failed, something is seriously wrong.
		return pGL->Error(reinterpret_cast<const char*>(glewGetErrorString(err)));
		}
	// success
	return true;
	}

bool CStdGLCtx::Select(bool verbose)
	{
	// safety
	if (!pGL || !hrc) return false; if (!pGL->lpPrimary) return false;
	// make context current
	if (!wglMakeCurrent (hDC, hrc)) return false;
	SelectCommon();
	// update clipper - might have been done by UpdateSize
	// however, the wrong size might have been assumed
	if (!pGL->UpdateClipper()) return false;
	// success
	return true;
	}

void CStdGLCtx::Deselect()
	{
	if (pGL && pGL->pCurrCtx == this)
		{
		wglMakeCurrent(NULL, NULL);
		pGL->pCurrCtx=NULL;
		}
	}

bool CStdGLCtx::UpdateSize()
	{
	// safety
	if (!pWindow && !hWindow) return false;
	// get size
	RECT rt; if (!GetClientRect(pWindow ? pWindow->hWindow : hWindow, &rt)) return false;
	int cx2=rt.right-rt.left, cy2=rt.bottom-rt.top;
	// assign if different
	if (cx!=cx2 || cy!=cy2)
		{
		cx=cx2; cy=cy2;
		if (pGL) pGL->UpdateClipper();
		}
	// success
	return true;
	}

bool CStdGLCtx::PageFlip()
	{
	// flush GL buffer
	glFlush();
	SwapBuffers(hDC);
	return true;
	}

bool CStdGL::SaveDefaultGammaRamp(CStdWindow * pWindow)
	{
	HDC hDC = GetDC(pWindow->hWindow);
	if (hDC)
		{
		if (!GetDeviceGammaRamp(hDC, &DefRamp))
			{
			DefRamp.Default();
			Log("  Error getting default gamma ramp; using standard");
			}
		ReleaseDC(pWindow->hWindow, hDC);
		return true;
		}
	return false;
	}

bool CStdGL::ApplyGammaRamp(D3DGAMMARAMP &ramp, bool fForce)
	{
	if (!MainCtx.hDC || (!Active && !fForce)) return false;
	if (!SetDeviceGammaRamp(MainCtx.hDC, &ramp))
		{
		int i=::GetLastError();
		//Beep(i,i);
		}
	return true;
	}

#elif defined(USE_X11)

//  Xmd.h typedefs bool to CARD8, whereas microsoft windows and Clonk use int
#define bool _BOOL
#include <X11/Xmd.h>
#include <GL/glx.h>
#include <X11/extensions/xf86vmode.h>
#undef bool

CStdGLCtx::CStdGLCtx(): pWindow(0), ctx(0), cx(0), cy(0) { }

void CStdGLCtx::Clear()
	{
	Deselect();
	if (ctx)
		{
		glXDestroyContext(pWindow->dpy, (GLXContext)ctx);
		ctx = 0;
		}
	pWindow = 0;
	cx = cy = 0;
	}

bool CStdGLCtx::Init(CStdWindow * pWindow, CStdApp *)
	{
	// safety
	if (!pGL) return false;
	// store window
	this->pWindow = pWindow;
	// Create Context with sharing (if this is the main context, our ctx will be 0, so no sharing)
	// try direct rendering first
	if (!DDrawCfg.NoAcceleration)
		ctx = glXCreateContext(pWindow->dpy, (XVisualInfo*)pWindow->Info, (GLXContext)pGL->MainCtx.ctx, True);
	// without, rendering will be unacceptable slow, but that's better than nothing at all
	if (!ctx)
		ctx = glXCreateContext(pWindow->dpy, (XVisualInfo*)pWindow->Info, (GLXContext)pGL->MainCtx.ctx, False);
	// No luck at all?
	if (!ctx) return pGL->Error("  gl: Unable to create context");
	if (!Select(true)) return pGL->Error("  gl: Unable to select context");
	// init extensions
	GLenum err = glewInit();
	if (GLEW_OK != err)
		{
		// Problem: glewInit failed, something is seriously wrong.
		return pGL->Error(reinterpret_cast<const char*>(glewGetErrorString(err)));
		}
	return true;
	}

bool CStdGLCtx::Select(bool verbose)
	{
	// safety
	if (!pGL || !ctx)
		{
		if (verbose) pGL->Error("  gl: pGL is zero");
		return false;
		}
	if (!pGL->lpPrimary)
		{
		if (verbose) pGL->Error("  gl: lpPrimary is zero");
		return false;
		}
	// make context current
	if (!pWindow->renderwnd || !glXMakeCurrent(pWindow->dpy, pWindow->renderwnd, (GLXContext)ctx))
		{
		if (verbose) pGL->Error("  gl: glXMakeCurrent failed");
		return false;
		}
	SelectCommon();
	// update clipper - might have been done by UpdateSize
	// however, the wrong size might have been assumed
	if (!pGL->UpdateClipper())
		{
		if (verbose) pGL->Error("  gl: UpdateClipper failed");
		return false;
		}
	// success
	return true;
	}

void CStdGLCtx::Deselect()
	{
	if (pGL && pGL->pCurrCtx == this)
		{
		glXMakeCurrent(pWindow->dpy, None, NULL);
		pGL->pCurrCtx = 0;
		}
	}

bool CStdGLCtx::UpdateSize()
	{
	// safety
	if (!pWindow) return false;
	// get size
	Window winDummy;
	unsigned int borderDummy;
	int x, y;
	unsigned int width, height;
	unsigned int depth;
	XGetGeometry(pWindow->dpy, pWindow->renderwnd, &winDummy, &x, &y,
		&width, &height, &borderDummy, &depth);
	// assign if different
	if (cx!=width || cy!=height)
		{
		cx=width; cy=height;
		if (pGL) pGL->UpdateClipper();
		}
	// success
	return true;
	}

bool CStdGLCtx::PageFlip()
	{
	// flush GL buffer
	glFlush();
	if (!pWindow || !pWindow->renderwnd) return false;
	glXSwapBuffers(pWindow->dpy, pWindow->renderwnd);
	return true;
	}

bool CStdGL::ApplyGammaRamp(_D3DGAMMARAMP& ramp, bool fForce) {
	if (!DeviceReady() || (!Active && !fForce)) return false;
	if (pApp->xf86vmode_major_version < 2) return false;
	if (gammasize != 256) return false;
	return XF86VidModeSetGammaRamp(pApp->dpy, DefaultScreen(pApp->dpy), 256,
		ramp.red, ramp.green, ramp.blue);
}

bool CStdGL::SaveDefaultGammaRamp(CStdWindow * pWindow)
	{
	if (pApp->xf86vmode_major_version < 2) return false;
	// Get the Display
	Display * const dpy = pWindow->dpy;
	XF86VidModeGetGammaRampSize(dpy, DefaultScreen(dpy), &gammasize);
	if (gammasize != 256)
		{
		LogF("  Size of GammaRamp is %d, not 256", gammasize);
		}
	else
		{
		// store default gamma
		if (!XF86VidModeGetGammaRamp(pWindow->dpy, DefaultScreen(pWindow->dpy), 256,
				DefRamp.ramp.red, DefRamp.ramp.green, DefRamp.ramp.blue))
			{
			DefRamp.Default();
			Log("  Error getting default gamma ramp; using standard");
			}
		}
	return true;
	}

#elif defined(USE_SDL_MAINLOOP)

CStdGLCtx::CStdGLCtx(): pWindow(0), cx(0), cy(0) { }

void CStdGLCtx::Clear()
{
	pWindow = 0;
	cx = cy = 0;
}

bool CStdGLCtx::Init(CStdWindow * pWindow, CStdApp *)
{
	// safety
	if (!pGL) return false;
	// store window
	this->pWindow = pWindow;
	assert(!DDrawCfg.NoAcceleration);
	// No luck at all?
	if (!Select(true)) return pGL->Error("  gl: Unable to select context");
	// init extensions
	GLenum err = glewInit();
	if (GLEW_OK != err)
		{
		// Problem: glewInit failed, something is seriously wrong.
		return pGL->Error(reinterpret_cast<const char*>(glewGetErrorString(err)));
		}
	return true;
}

bool CStdGLCtx::Select(bool verbose)
{
	SelectCommon();
	// update clipper - might have been done by UpdateSize
	// however, the wrong size might have been assumed
	if (!pGL->UpdateClipper())
		{
		if (verbose) pGL->Error("  gl: UpdateClipper failed");
		return false;
		}
	// success
	return true;
}

void CStdGLCtx::Deselect()
{
	if (pGL && pGL->pCurrCtx == this)
    {
		pGL->pCurrCtx = 0;
    }
}

bool CStdGLCtx::UpdateSize()
{
	// safety
	if (!pWindow) return false;
	// get size
    RECT rc;
    pWindow->GetSize(&rc);
    int width = rc.right - rc.left, height = rc.bottom - rc.top;
	// assign if different
	if (cx!=width || cy!=height)
    {
		cx=width; cy=height;
		if (pGL) pGL->UpdateClipper();
    }
	// success
	return true;
}

bool CStdGLCtx::PageFlip()
{
	// flush GL buffer
	glFlush();
	if (!pWindow) return false;
	SDL_GL_SwapBuffers();
	return true;
}

bool CStdGL::ApplyGammaRamp(_D3DGAMMARAMP& ramp, bool fForce) {
	return SDL_SetGammaRamp(ramp.red, ramp.green, ramp.blue) != -1;
}

bool CStdGL::SaveDefaultGammaRamp(CStdWindow * pWindow) {
	return SDL_GetGammaRamp(DefRamp.ramp.red, DefRamp.ramp.green, DefRamp.ramp.blue) != -1;
}

#endif //USE_X11/USE_SDL_MAINLOOP

#endif // USE_GL
