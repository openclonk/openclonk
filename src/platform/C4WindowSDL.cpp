/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2010-2013, The OpenClonk Team and contributors
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

/* A wrapper class to OS dependent event and window interfaces, SDL version */

#include <C4Include.h>
#include <C4Window.h>

#include <C4Application.h>
#include <C4DrawGL.h>
#include <StdFile.h>
#include <StdBuf.h>

#include "C4Version.h"
#include <C4Rect.h>
#include <C4Config.h>

/* C4Window */

C4Window::C4Window ():
		Active(false), pSurface(0)
{
}

C4Window::~C4Window ()
{
	Clear();
}

C4Window * C4Window::Init(WindowKind windowKind, C4AbstractApp * pApp, const char * Title, const C4Rect * size)
{
/*	    SDL_GL_MULTISAMPLEBUFFERS,
	    SDL_GL_MULTISAMPLESAMPLES,*/
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, /*REQUESTED_GL_CTX_MAJOR*/ 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, /*REQUESTED_GL_CTX_MINOR*/ 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, (Config.Graphics.DebugOpenGL ? SDL_GL_CONTEXT_DEBUG_FLAG : 0));
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	uint32_t flags = SDL_WINDOW_OPENGL;
	if (windowKind == W_Fullscreen && size->Wdt == -1)
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	else if (windowKind == W_Fullscreen)
		flags |= SDL_WINDOW_FULLSCREEN;
	window = SDL_CreateWindow(Title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, size->Wdt, size->Hgt, flags);
	if (!window)
	{
		Log(SDL_GetError());
		return 0;
	}
	Active = true;
	SDL_ShowCursor(SDL_DISABLE);
	return this;
}

bool C4Window::ReInit(C4AbstractApp* pApp)
{
	// TODO: How do we enable multisampling with SDL?
	// Maybe re-call SDL_SetVideoMode?
	return false;
}

void C4Window::Clear()
{
	SDL_DestroyWindow(window);
	window = 0;
}

void C4Window::EnumerateMultiSamples(std::vector<int>& samples) const
{
	// TODO: Enumerate multi samples
}

bool C4Window::StorePosition(const char *, const char *, bool) { return true; }

bool C4Window::RestorePosition(const char *, const char *, bool) { return true; }

// Window size is automatically managed by C4AbstractApp's display mode management.
// Just remember the size for others to query.

bool C4Window::GetSize(C4Rect * pRect)
{
	pRect->x = pRect->y = 0;
	SDL_GL_GetDrawableSize(window, &pRect->Wdt, &pRect->Hgt);
	return true;
}

void C4Window::SetSize(unsigned int X, unsigned int Y)
{
	SDL_SetWindowSize(window, X, Y);
}

void C4Window::SetTitle(const char * Title)
{
	SDL_SetWindowTitle(window, Title);
}

void C4Window::RequestUpdate()
{
	// just invoke directly
	PerformUpdate();
}

void C4Window::FlashWindow()
{
}

void C4Window::GrabMouse(bool grab)
{
	SDL_SetWindowGrab(window, grab ? SDL_TRUE : SDL_FALSE);
}
