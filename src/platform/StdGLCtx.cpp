/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2002, 2005-2006  Sven Eberhardt
 * Copyright (c) 2005-2007, 2009-2010  Günther Brammer
 * Copyright (c) 2006  Julian Raschke
 * Copyright (c) 2010  Benjamin Herr
 * Copyright (c) 2010  Armin Burgmeier
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
#include <StdGL.h>

#include <StdApp.h>
#include <StdSurface2.h>
#include <StdWindow.h>
#include <C4Config.h>

#ifdef USE_GL

void CStdGLCtx::SelectCommon()
{
	pGL->pCurrCtx = this;
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

#include <GL/wglew.h>

// Enumerate available pixel formats. Choose the best pixel format in
// terms of color and depth buffer bits and then return all formats with
// different multisampling settings. If there are more then one, then choose
// the one with highest depth buffer size and lowest stencil and auxiliary
// buffer sizes since we don't use them in Clonk.
static std::vector<int> EnumeratePixelFormats(HDC hdc)
{
	std::vector<int> result;
	if(!wglGetPixelFormatAttribivARB) return result;

	int n_formats;
	int attributes = WGL_NUMBER_PIXEL_FORMATS_ARB;
	if(!wglGetPixelFormatAttribivARB(hdc, 0, 0, 1, &attributes, &n_formats)) return result;

	for(int i = 1; i < n_formats+1; ++i)
	{
		int new_attributes[] = { WGL_DRAW_TO_WINDOW_ARB, WGL_SUPPORT_OPENGL_ARB, WGL_DOUBLE_BUFFER_ARB, WGL_COLOR_BITS_ARB, WGL_DEPTH_BITS_ARB, WGL_STENCIL_BITS_ARB, WGL_AUX_BUFFERS_ARB, WGL_SAMPLE_BUFFERS_ARB, WGL_SAMPLES_ARB };
		const unsigned int nnew_attributes = sizeof(new_attributes)/sizeof(int);

		int new_results[nnew_attributes];
		if(!wglGetPixelFormatAttribivARB(hdc, i, 0, nnew_attributes, new_attributes, new_results)) continue;
		if(!new_results[0] || !new_results[1] || !new_results[2]) continue;
		if(new_results[3] < 16 || new_results[4] < 16) continue; // require at least 16 bits per pixel in color and depth

		// For no MS we do find a pixel format with depth 32 on my (ck's) computer,
		// however, when choosing it then texturing does not work anymore. I am not
		// exactly sure what the cause of that is, so let's not choose that one for now.
		if(new_results[4] > 24) continue;

		// Multisampling with just one sample is equivalent to no-multisampling,
		// so don't include that in the result.
		if(new_results[7] == 1 && new_results[8] == 1)
			continue;

		if(result.empty())
		{
			result.push_back(i);
		}
		else
		{
			int old_attributes[] = { WGL_COLOR_BITS_ARB };
			const unsigned int nold_attributes = sizeof(old_attributes)/sizeof(int);
			int old_results[nold_attributes];

			if(!wglGetPixelFormatAttribivARB(hdc, result[0], 0, nold_attributes, old_attributes, old_results)) continue;

			if(new_results[3] > old_results[0])
			{
				result.clear();
				result.push_back(i);
			}
			else if(new_results[3] == old_results[0])
			{
				unsigned int j;
				for(j = 0; j < result.size(); ++j)
				{
					int equiv_attributes[] = { WGL_DEPTH_BITS_ARB, WGL_STENCIL_BITS_ARB, WGL_AUX_BUFFERS_ARB, WGL_SAMPLE_BUFFERS_ARB, WGL_SAMPLES_ARB };
					const unsigned int nequiv_attributes = sizeof(equiv_attributes)/sizeof(int);
					int equiv_results[nequiv_attributes];
					if(!wglGetPixelFormatAttribivARB(hdc, result[j], 0, nequiv_attributes, equiv_attributes, equiv_results)) continue;

					if(new_results[7] == equiv_results[3] && new_results[8] == equiv_results[4])
					{
						if(new_results[4] > equiv_results[0] || (new_results[4] == equiv_results[0] && (new_results[5] < equiv_results[1] || (new_results[5] == equiv_results[1] && new_results[6] < equiv_results[2]))))
							result[j] = i;
						break;
					}
				}

				if(j == result.size()) result.push_back(i);
			}
		}
	}

	return result;
}

static int GetPixelFormatForMS(HDC hDC, int samples)
{
	std::vector<int> vec = EnumeratePixelFormats(hDC);
	for(unsigned int i = 0; i < vec.size(); ++i)
	{
		int attributes[] = { WGL_SAMPLE_BUFFERS_ARB, WGL_SAMPLES_ARB };
		const unsigned int n_attributes = 2;
		int results[2];
		if(!wglGetPixelFormatAttribivARB(hDC, vec[i], 0, n_attributes, attributes, results)) continue;

		if( (samples == 0 && results[0] == 0) ||
		    (samples > 0 && results[0] == 1 && results[1] == samples))
		{
			return vec[i];
		}
	}

	return 0;
}

// Initialize GLEW. We need to choose a pixel format for this, however we need
// GLEW initialized to enumerate pixel formats. So this creates a temporary
// window with a default pixel format, initializes glew and removes that temp
// window again. Then we can enumerate pixel formats and choose a proper one
// for the main window in CStdGLCtx::Init.
bool CStdGLCtx::InitGlew(HINSTANCE hInst)
{
	static bool glewInitialized = false;
	if(glewInitialized) return true;

	/*WNDCLASSEXW WndClass = {0};
	WndClass.cbSize        = sizeof(WNDCLASSEX);
	WndClass.style         = CS_DBLCLKS;
	WndClass.lpfnWndProc   = DefWindowProcW;
	WndClass.hInstance     = pApp->hInstance;
	WndClass.hbrBackground = (HBRUSH) COLOR_BACKGROUND;
	WndClass.lpszClassName = L"C4OCTest";
	WndClass.hIcon         = NULL;
	WndClass.hIconSm       = NULL;
	if(!RegisterClassExW(&WndClass)) return !!pGL->Error("  gl: Error registered class for temp wnd");
*/
	// Create window
	HWND hWnd = CreateWindowExW  (
	            0,
	            L"STATIC", //C4FullScreenClassName,
	            NULL, //L"C4OCTest", //ADDL(C4ENGINENAME),
	            WS_OVERLAPPEDWINDOW,
	            CW_USEDEFAULT,CW_USEDEFAULT,0,0,
	            NULL,NULL,hInst,NULL);

	if(!hWnd)
	{
		pGL->Error("  gl: Failed to create temporary window to choose pixel format");
	}
	else
	{
		HDC dc = GetDC(hWnd);

		PIXELFORMATDESCRIPTOR pfd;

		// pixel format
		memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR)) ;
		pfd.nSize      = sizeof(PIXELFORMATDESCRIPTOR);
		pfd.nVersion   = 1 ;
		pfd.dwFlags    = PFD_DOUBLEBUFFER | /*(pGL->fFullscreen ? PFD_SWAP_EXCHANGE : 0) |*/
			              PFD_SUPPORT_OPENGL |
			              PFD_DRAW_TO_WINDOW ;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = pGL->iClrDpt;
		pfd.cDepthBits = 0;
		pfd.iLayerType = PFD_MAIN_PLANE;
		int temp_fmt = ChoosePixelFormat(dc, &pfd);

		if(!temp_fmt)
		{
			pGL->Error("  gl: Error choosing temp pixel format");
		}
		else if(!SetPixelFormat(dc, temp_fmt, &pfd))
		{
			pGL->Error("  gl: Error setting temp pixel format");
		}
		else
		{
			HGLRC hrc = wglCreateContext(dc);
			if(!hrc)
			{
				pGL->Error("  gl: Error creating temp context");
			}
			else
			{
				if(!wglMakeCurrent(dc, hrc))
				{
					pGL->Error("  gl: Error making temp context current");
				}
				else
				{
					// init extensions
					GLenum err = glewInit();
					if(err != GLEW_OK)
					{
						// Problem: glewInit failed, something is seriously wrong.
						pGL->Error(reinterpret_cast<const char*>(glewGetErrorString(err)));
					}
					else
					{
						glewInitialized = true;
					}

					wglMakeCurrent(NULL, NULL);
				}

				wglDeleteContext(hrc);
			}
		}

		ReleaseDC(hWnd, dc);
		DestroyWindow(hWnd);
	}

	return glewInitialized;
}

CStdGLCtx::CStdGLCtx(): pWindow(0), hrc(0), hDC(0) { }

void CStdGLCtx::Clear()
{
	if (hrc)
	{
		Deselect();
		wglDeleteContext(hrc); hrc=0;
	}
	if (hDC)
	{
		ReleaseDC(pWindow ? pWindow->hRenderWindow : hWindow, hDC);
		hDC=0;
	}
	pWindow = 0; hWindow = NULL;
}

bool CStdGLCtx::Init(CStdWindow * pWindow, CStdApp *pApp, HWND hWindow)
{
	// safety
	if (!pGL) return false;

	// Initialize GLEW so that we can choose a pixel format later
	if(!InitGlew(pApp->hInstance)) return false;

	// store window
	this->pWindow = pWindow;
	// default HWND
	if (pWindow)
		hWindow = pWindow->hRenderWindow;
	else
		this->hWindow = hWindow;

	// get DC
	hDC = GetDC(hWindow);
	if(!hDC)
	{
		pGL->Error("  gl: Error getting DC");
	}
	else
	{
		// Choose a good pixel format.
		int pixel_format;
		if((pixel_format = GetPixelFormatForMS(hDC, Config.Graphics.MultiSampling)) == 0)
			if((pixel_format = GetPixelFormatForMS(hDC, 0)) != 0)
				Config.Graphics.MultiSampling = 0;

		if(!pixel_format)
		{
			pGL->Error("  gl: Error choosing pixel format");
		}
		else
		{
			PIXELFORMATDESCRIPTOR pfd;
			if(!DescribePixelFormat(hDC, pixel_format, sizeof(pfd), &pfd))
			{
				pGL->Error("  gl: Error describing chosen pixel format");
			}
			else if(!SetPixelFormat(hDC, pixel_format, &pfd))
			{
				pGL->Error("  gl: Error setting chosen pixel format");
			}
			else
			{
				// create context
				hrc = wglCreateContext(hDC);
				if(!hrc)
				{
					pGL->Error("  gl: Error creating gl context");
				}
				else
				{
					//if (this != &pGL->MainCtx) wglCopyContext(pGL->MainCtx.hrc, hrc, GL_ALL_ATTRIB_BITS);

					// share textures
					bool success = false;
					wglMakeCurrent(NULL, NULL); pGL->pCurrCtx=NULL;
					if (this != pGL->pMainCtx)
					{
						if(!wglShareLists(pGL->pMainCtx->hrc, hrc))
							pGL->Error("  gl: Textures for secondary context not available");
						else
							success = true;
					}
					else
					{
						// select main context
						if (!Select())
							pGL->Error("  gl: Unable to select context");
						else
							success = true;
					}
					
					if(success)
					{
						pGL->iPixelFormat = pixel_format;
						PIXELFORMATDESCRIPTOR &rPfd = pApp->GetPFD();
						rPfd = pfd;
						return true;
					}
				}

				wglDeleteContext(hrc); hrc = NULL;
			}
		}

		ReleaseDC(hWindow, hDC); hDC = NULL;
	}

	return false;
}

std::vector<int> CStdGLCtx::EnumerateMultiSamples() const
{
	std::vector<int> result;
	std::vector<int> vec = EnumeratePixelFormats(hDC);
	for(unsigned int i = 0; i < vec.size(); ++i)
	{
		int attributes[] = { WGL_SAMPLE_BUFFERS_ARB, WGL_SAMPLES_ARB };
		const unsigned int n_attributes = 2;
		int results[2];
		if(!wglGetPixelFormatAttribivARB(hDC, vec[i], 0, n_attributes, attributes, results)) continue;

		if(results[0] == 1) result.push_back(results[1]);
	}

	return result;
}

bool CStdGLCtx::Select(bool verbose)
{
	// safety
	if (!pGL || !hrc) return false;
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
		pGL->RenderTarget=NULL;
	}
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
	if (!pMainCtx || (!Active && !fForce)) return false;
	if (!SetDeviceGammaRamp(pMainCtx->hDC, &ramp))
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

CStdGLCtx::CStdGLCtx(): pWindow(0), ctx(0) { }

void CStdGLCtx::Clear()
{
	Deselect();
	if (ctx)
	{
		glXDestroyContext(pWindow->dpy, (GLXContext)ctx);
		ctx = 0;
	}
	pWindow = 0;
}

bool CStdGLCtx::Init(CStdWindow * pWindow, CStdApp *)
{
	// safety
	if (!pGL) return false;
	// store window
	this->pWindow = pWindow;
	// Create Context with sharing (if this is the main context, our ctx will be 0, so no sharing)
	// try direct rendering first
	ctx = glXCreateContext(pWindow->dpy, (XVisualInfo*)pWindow->Info, (pGL->pMainCtx != this) ? (GLXContext)pGL->pMainCtx->ctx : 0, True);
	// without, rendering will be unacceptable slow, but that's better than nothing at all
	if (!ctx)
		ctx = glXCreateContext(pWindow->dpy, (XVisualInfo*)pWindow->Info, pGL->pMainCtx ? (GLXContext)pGL->pMainCtx->ctx : 0, False);
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
		pGL->RenderTarget = 0;
	}
}

bool CStdGLCtx::PageFlip()
{
	// flush GL buffer
	glFlush();
	if (!pWindow || !pWindow->renderwnd) return false;
	glXSwapBuffers(pWindow->dpy, pWindow->renderwnd);
	return true;
}

bool CStdGL::ApplyGammaRamp(_D3DGAMMARAMP& ramp, bool fForce)
{
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

CStdGLCtx::CStdGLCtx(): pWindow(0) { }

void CStdGLCtx::Clear()
{
	pWindow = 0;
}

bool CStdGLCtx::Init(CStdWindow * pWindow, CStdApp *)
{
	// safety
	if (!pGL) return false;
	// store window
	this->pWindow = pWindow;
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
		pGL->RenderTarget = 0;
	}
}

bool CStdGLCtx::PageFlip()
{
	// flush GL buffer
	glFlush();
	if (!pWindow) return false;
	SDL_GL_SwapBuffers();
	return true;
}

bool CStdGL::ApplyGammaRamp(_D3DGAMMARAMP& ramp, bool fForce)
{
	return SDL_SetGammaRamp(ramp.red, ramp.green, ramp.blue) != -1;
}

bool CStdGL::SaveDefaultGammaRamp(CStdWindow * pWindow)
{
	return SDL_GetGammaRamp(DefRamp.ramp.red, DefRamp.ramp.green, DefRamp.ramp.blue) != -1;
}

#endif //USE_X11/USE_SDL_MAINLOOP

#endif // USE_GL
