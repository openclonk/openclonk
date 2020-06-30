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

/* OpenGL implementation of NewGfx, the context */

#include "C4Include.h"
#include "C4ForbidLibraryCompilation.h"
#include "graphics/C4DrawGL.h"

#include "platform/C4App.h"
#include "platform/C4Window.h"

#ifndef USE_CONSOLE

#if defined(USE_WGL) || defined(USE_SDL_MAINLOOP)
static const int REQUESTED_GL_CTX_MAJOR = 3;
static const int REQUESTED_GL_CTX_MINOR = 2;
#endif

std::list<CStdGLCtx*> CStdGLCtx::contexts;

void CStdGLCtx::SelectCommon()
{
	pGL->pCurrCtx = this;
	// set some default states
	glDisable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	// Delete pending VAOs
	std::vector<GLuint> toBeDeleted;
	if (!VAOsToBeDeleted.empty())
	{
		for (unsigned int i : VAOsToBeDeleted)
		{
			if (i < hVAOs.size() && hVAOs[i] != 0)
			{
				toBeDeleted.push_back(hVAOs[i]);
				hVAOs[i] = 0;
			}
		}

		glDeleteVertexArrays(toBeDeleted.size(), &toBeDeleted[0]);
		VAOsToBeDeleted.clear();
	}
}

#ifdef USE_WGL

#include <epoxy/wgl.h>

static PIXELFORMATDESCRIPTOR pfd;  // desired pixel format
static HGLRC hrc = nullptr;

// Enumerate available pixel formats. Choose the best pixel format in
// terms of color and depth buffer bits and then return all formats with
// different multisampling settings. If there are more then one, then choose
// the one with highest depth buffer size and lowest stencil and auxiliary
// buffer sizes since we don't use them in Clonk.
static std::vector<int> EnumeratePixelFormats(HDC hdc)
{
	std::vector<int> result;
	if(!epoxy_has_wgl_extension(hdc, "WGL_ARB_pixel_format")) return result;

	int n_formats;
	int attributes = WGL_NUMBER_PIXEL_FORMATS_ARB;
	if(!wglGetPixelFormatAttribivARB(hdc, 0, 0, 1, &attributes, &n_formats)) return result;

	for(int i = 1; i < n_formats+1; ++i)
	{
		int new_attributes[] = { WGL_DRAW_TO_WINDOW_ARB, WGL_SUPPORT_OPENGL_ARB, WGL_DOUBLE_BUFFER_ARB, WGL_COLOR_BITS_ARB, WGL_DEPTH_BITS_ARB, WGL_STENCIL_BITS_ARB, WGL_AUX_BUFFERS_ARB, WGL_SAMPLE_BUFFERS_ARB, WGL_SAMPLES_ARB, WGL_PIXEL_TYPE_ARB };
		const unsigned int nnew_attributes = sizeof(new_attributes)/sizeof(int);

		int new_results[nnew_attributes];
		if(!wglGetPixelFormatAttribivARB(hdc, i, 0, nnew_attributes, new_attributes, new_results)) continue;
		if(!new_results[0] || !new_results[1] || !new_results[2]) continue;
		if(new_results[3] < 16 || new_results[4] < 16) continue; // require at least 16 bits per pixel in color and depth

		// For no MS we do find a pixel format with depth 32 on my (ck's) computer,
		// however, when choosing it then texturing does not work anymore. I am not
		// exactly sure what the cause of that is, so let's not choose that one for now.
		if(new_results[4] > 24) continue;

		// ensure that we get an RGB buffer. otherwise we might end up with a float buffer, which messes up gamma.
		if (new_results[9] != WGL_TYPE_RGBA_ARB) 
			continue;

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
	for(int i : vec)
	{
		int attributes[] = { WGL_SAMPLE_BUFFERS_ARB, WGL_SAMPLES_ARB };
		const unsigned int n_attributes = 2;
		int results[2];
		if(!wglGetPixelFormatAttribivARB(hDC, i, 0, n_attributes, attributes, results)) continue;

		if( (samples == 0 && results[0] == 0) ||
		    (samples > 0 && results[0] == 1 && results[1] == samples))
		{
			return i;
		}
	}

	return 0;
}

class WinAPIError : public std::runtime_error
{
public:
	typedef DWORD error_code;

	WinAPIError() : WinAPIError(GetLastError()) {}
	WinAPIError(error_code err) : std::runtime_error(format_error(err)) {}

private:
	static std::string format_error(error_code err)
	{
		LPWSTR buffer = nullptr;
		FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
			nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);
		StdStrBuf str(buffer);
		LocalFree(buffer);
		return std::string(str.getData(), str.getLength());
	}
};

class GLTempContext
{
	HWND wnd;
	HDC dc;
	HGLRC glrc;
public:
	GLTempContext()
	{
		wnd = CreateWindowExW(0, L"STATIC", nullptr, WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
		if (!wnd)
			throw WinAPIError();
		dc = GetDC(wnd);
		auto pfd = PIXELFORMATDESCRIPTOR();
		pfd.nSize = sizeof(pfd);
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.iLayerType = PFD_MAIN_PLANE;
		int format = ChoosePixelFormat(dc, &pfd);
		if (!format ||
			!SetPixelFormat(dc, format, &pfd) ||
			(glrc = wglCreateContext(dc)) == nullptr)
		{
			DWORD err = GetLastError();
			ReleaseDC(wnd, dc);
			DestroyWindow(wnd);
			throw WinAPIError(err);
		}
		if (!wglMakeCurrent(dc, glrc))
		{
			DWORD err = GetLastError();
			wglDeleteContext(glrc);
			ReleaseDC(wnd, dc);
			DestroyWindow(wnd);
			throw WinAPIError(err);
		}
	}
	~GLTempContext()
	{
		if (glrc == wglGetCurrentContext())
			wglMakeCurrent(dc, nullptr);
		wglDeleteContext(glrc);
		ReleaseDC(wnd, dc);
		DestroyWindow(wnd);
	}
};

CStdGLCtx::CStdGLCtx(): this_context(contexts.end()) { }

void CStdGLCtx::Clear(bool multisample_change)
{
	Deselect();
	if (hDC && pWindow)
		ReleaseDC(pWindow->renderwnd, hDC);
	hDC = nullptr;
	pWindow = nullptr;

	if (this_context != contexts.end())
	{
		contexts.erase(this_context);
		this_context = contexts.end();
	}
	if (multisample_change)
	{
		assert(!pGL->pCurrCtx);
		if (hrc)
			wglDeleteContext(hrc);
		hrc = nullptr;
	}
}

bool CStdGLCtx::Init(C4Window * pWindow, C4AbstractApp *pApp)
{
	// safety
	if (!pGL || !pWindow) return false;

	std::unique_ptr<GLTempContext> tempContext;
	if (hrc == nullptr)
	{
		// Create a temporary context to be able to fetch GL extension pointers
		try
		{
			tempContext = std::make_unique<GLTempContext>();
		}
		catch (const WinAPIError &e)
		{
			pGL->Error((std::string("  gl: Unable to create temporary context: ") + e.what()).c_str());
			return false;
		}
	}

	// store window
	this->pWindow = pWindow;

	// get DC
	hDC = GetDC(pWindow->renderwnd);
	if(!hDC)
	{
		pGL->Error("  gl: Error getting DC");
		return false;
	}
	if (hrc)
	{
		SetPixelFormat(hDC, pGL->iPixelFormat, &pfd);
	}
	else
	{
		// Choose a good pixel format.
		int pixel_format;
		if((pixel_format = GetPixelFormatForMS(hDC, Config.Graphics.MultiSampling)) == 0)
			if((pixel_format = GetPixelFormatForMS(hDC, 0)) != 0)
				Config.Graphics.MultiSampling = 0;

		if (!pixel_format)
		{
			pGL->Error("  gl: Error choosing pixel format");
		}
		else
		{
			ZeroMemory(&pfd, sizeof(pfd)); pfd.nSize = sizeof(pfd);
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
				if (epoxy_has_wgl_extension(hDC, "WGL_ARB_create_context"))
				{
					{
						const int attribs[] = {
							WGL_CONTEXT_FLAGS_ARB, Config.Graphics.DebugOpenGL ? WGL_CONTEXT_DEBUG_BIT_ARB : 0,
							WGL_CONTEXT_MAJOR_VERSION_ARB, REQUESTED_GL_CTX_MAJOR,
							WGL_CONTEXT_MINOR_VERSION_ARB, REQUESTED_GL_CTX_MINOR,
							WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
							0
						};

						hrc = wglCreateContextAttribsARB(hDC, nullptr, attribs);
					}

					if (!hrc)
					{
						LogSilentF("  gl: OpenGL %d.%d not available; falling back to 3.1 emergency context.", REQUESTED_GL_CTX_MAJOR, REQUESTED_GL_CTX_MINOR);
						// Some older Intel drivers don't support OpenGL 3.2; we don't use (much?) of
						// that so we'll request a 3.1 context as a fallback.
						const int attribs[] = {
							WGL_CONTEXT_FLAGS_ARB, Config.Graphics.DebugOpenGL ? WGL_CONTEXT_DEBUG_BIT_ARB : 0,
							WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
							WGL_CONTEXT_MINOR_VERSION_ARB, 1,
							0
						};
						pGL->Workarounds.ForceSoftwareTransform = true;
						hrc = wglCreateContextAttribsARB(hDC, nullptr, attribs);
					}
				}
				else
				{
					DebugLog("  gl: wglCreateContextAttribsARB not available; creating default context.");
					hrc = wglCreateContext(hDC);
				}

				if(!hrc)
				{
					pGL->Error("  gl: Error creating gl context");
				}

				pGL->iPixelFormat = pixel_format;
			}
		}
	}
	if (hrc)
	{
		Select();

		this_context = contexts.insert(contexts.end(), this);
		return true;
	}

	ReleaseDC(pWindow->renderwnd, hDC); hDC = nullptr;
	return false;
}

std::vector<int> CStdGLCtx::EnumerateMultiSamples() const
{
	assert(hrc != 0);
	std::vector<int> result;
	std::vector<int> vec = EnumeratePixelFormats(hDC);
	for(int i : vec)
	{
		int attributes[] = { WGL_SAMPLE_BUFFERS_ARB, WGL_SAMPLES_ARB };
		const unsigned int n_attributes = 2;
		int results[2];
		if(!wglGetPixelFormatAttribivARB(hDC, i, 0, n_attributes, attributes, results)) continue;

		if(results[0] == 1) result.push_back(results[1]);
	}

	return result;
}

bool CStdGLCtx::Select(bool verbose)
{
	// safety
	if (!pGL || !hrc) return false;
	// make context current
	if (!wglMakeCurrent (hDC, hrc))
	{
		pGL->Error("Unable to select context.");
		return false;
	}
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
		wglMakeCurrent(nullptr, nullptr);
		pGL->pCurrCtx=nullptr;
		pGL->RenderTarget=nullptr;
	}
}

bool CStdGLCtx::PageFlip()
{
	// flush GL buffer
	glFlush();
	SwapBuffers(hDC);
	return true;
}

#elif defined(USE_SDL_MAINLOOP)

CStdGLCtx::CStdGLCtx(): pWindow(0), this_context(contexts.end()) { ctx = nullptr; }

void CStdGLCtx::Clear(bool multisample_change)
{
	Deselect();
	if (ctx) SDL_GL_DeleteContext(ctx);
	ctx = 0;
	pWindow = 0;

	if (this_context != contexts.end())
	{
		contexts.erase(this_context);
		this_context = contexts.end();
	}
}

bool CStdGLCtx::Init(C4Window * pWindow, C4AbstractApp *)
{
	// safety
	if (!pGL) return false;
	// store window
	this->pWindow = pWindow;
	ctx = SDL_GL_CreateContext(pWindow->window);
	if (!ctx)
	{
		LogSilentF("  gl: OpenGL %d.%d not available; falling back to 3.1 emergency context.", REQUESTED_GL_CTX_MAJOR, REQUESTED_GL_CTX_MINOR);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
		pGL->Workarounds.ForceSoftwareTransform = true;
		ctx = SDL_GL_CreateContext(pWindow->window);
	}
	if (!ctx)
	{
		return pGL->Error(FormatString("SDL_GL_CreateContext: %s", SDL_GetError()).getData());
	}
	// No luck at all?
	if (!Select(true)) return false;

	this_context = contexts.insert(contexts.end(), this);
	return true;
}

bool CStdGLCtx::Select(bool verbose)
{
	if (SDL_GL_MakeCurrent(pWindow->window, ctx) != 0)
		return pGL->Error(FormatString("SDL_GL_MakeCurrent: %s", SDL_GetError()).getData());
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
	SDL_GL_SwapWindow(pWindow->window);
	return true;
}

#endif // USE_*

#ifdef WITH_QT_EDITOR
#include <QOpenGLWidget>
#include <QOpenGLContext>
#include <QOffscreenSurface>

CStdGLCtxQt::CStdGLCtxQt() { context = nullptr; surface = nullptr; }

void CStdGLCtxQt::Clear(bool multisample_change)
{
	Deselect();

	if (context)
	{
		if (!pWindow->glwidget) delete context;
		delete surface;
	}
	pWindow = nullptr;
}

bool CStdGLCtxQt::Init(C4Window *window, C4AbstractApp *app)
{
	if (!pGL) return false;
	pWindow = window;

	if (!pWindow->glwidget)
	{
		surface = new QOffscreenSurface();
		surface->create();
		context = new QOpenGLContext();
		QOpenGLContext* share_context = QOpenGLContext::globalShareContext();
		if (share_context) context->setShareContext(share_context);
		if (!context->create())
			return false;

		if (!Select(true)) return false;
	}
	else
	{
		// The Qt GL widget has its own context
		context = pWindow->glwidget->context();
	}

	this_context = contexts.insert(contexts.end(), this);
	return true;
}

bool CStdGLCtxQt::Select(bool verbose)
{
	if (!pWindow->glwidget)
	{
		if (!context->makeCurrent(surface))
			return false;
	}
	else
	{
		// done automatically
		/* pWindow->glwidget->makeCurrent(); */
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

void CStdGLCtxQt::Deselect()
{
	if (!pWindow->glwidget)
		context->doneCurrent();
	else
	{
		// done automatically
		/* pWindow->glwidget->doneCurrent(); */
	}
	if (pGL && pGL->pCurrCtx == this)
	{
		pGL->pCurrCtx = nullptr;
		pGL->RenderTarget = nullptr;
	}
}

bool CStdGLCtxQt::PageFlip()
{
	// flush GL buffer
	glFlush();
	if (!pWindow) return false;
	if (!pWindow->glwidget)
		return false;
	return true;
}

#endif

#endif // USE_CONSOLE
