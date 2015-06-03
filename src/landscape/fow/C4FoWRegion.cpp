/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2014-2015, The OpenClonk Team and contributors
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

#include "C4Include.h"
#include "C4FoWRegion.h"

bool glCheck() {
	if (int err = glGetError()) {
		LogF("GL error %d: %s", err, gluErrorString(err));
		return false;
	}
	return true;
}

C4FoWRegion::~C4FoWRegion()
{
	Clear();
}

bool C4FoWRegion::BindFramebuf()
{

	// Flip texture
	C4Surface *pSfc = pSurface;
	pSurface = pBackSurface;
	pBackSurface = pSfc;

	// Can simply reuse old texture?
	if (!pSurface || pSurface->Wdt < Region.Wdt || pSurface->Hgt < Region.Hgt)
	{
		// Create texture. Round up to next power of two in order to
		// prevent rounding errors, as well as preventing lots of
		// re-allocations when region size changes quickly (think zoom).
		if (!pSurface)
			pSurface = new C4Surface();
		int iWdt = 1, iHgt = 1;
		while (iWdt < Region.Wdt) iWdt *= 2;
		while (iHgt < Region.Hgt) iHgt *= 2;
		if (!pSurface->Create(iWdt, iHgt))
			return false;
	}

	// Generate frame buffer object
	if (!hFrameBufDraw) {
		glGenFramebuffersEXT(1, &hFrameBufDraw);
		glGenFramebuffersEXT(1, &hFrameBufRead);
	}

	// Bind current texture to frame buffer
	glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, hFrameBufDraw);
	glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, hFrameBufRead);
	glFramebufferTexture2DEXT(GL_DRAW_FRAMEBUFFER_EXT,
		GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D,
		pSurface->textures[0].texName, 0);
	if (pBackSurface)
		glFramebufferTexture2DEXT(GL_READ_FRAMEBUFFER_EXT,
			GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D,
			pBackSurface->textures[0].texName, 0);

	// Check status, unbind if something was amiss
	GLenum status1 = glCheckFramebufferStatusEXT(GL_READ_FRAMEBUFFER_EXT),
		   status2 = glCheckFramebufferStatusEXT(GL_DRAW_FRAMEBUFFER_EXT);
	if (status1 != GL_FRAMEBUFFER_COMPLETE_EXT ||
		(pBackSurface && status2 != GL_FRAMEBUFFER_COMPLETE_EXT) ||
		!glCheck())
	{
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		return false;
	}

	// Worked!
	return true;
}

void C4FoWRegion::Clear()
{
	if (hFrameBufDraw) {
		glDeleteFramebuffersEXT(1, &hFrameBufDraw);
		glDeleteFramebuffersEXT(1, &hFrameBufRead);
	}
	hFrameBufDraw = hFrameBufRead = 0;
	delete pSurface; pSurface = NULL;
	delete pBackSurface; pBackSurface = NULL;
}

void C4FoWRegion::Update(C4Rect r, const FLOAT_RECT& vp)
{
	// Set the new region
	Region = r;
	ViewportRegion = vp;
}

void C4FoWRegion::Render(const C4TargetFacet *pOnScreen)
{
	// Update FoW at interesting location
	pFoW->Update(Region, pPlayer);

	// On screen? No need to set up frame buffer - simply shortcut
	if (pOnScreen)
	{
		pFoW->Render(this, pOnScreen, pPlayer);
		return;
	}

	// Set up shader. If this one doesn't work, we're really in trouble.
	C4Shader *pShader = pFoW->GetFramebufShader();
	assert(pShader);
	if (!pShader) return;

	// Create & bind the frame buffer
	pDraw->StorePrimaryClipper();
	if(!BindFramebuf())
	{
		pDraw->RestorePrimaryClipper();
		return;
	}
	assert(pSurface && hFrameBufDraw);
	if (!pSurface || !hFrameBufDraw)
		return;

	// Set up a clean context
	glViewport(0, 0, getSurface()->Wdt, getSurface()->Hgt);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0, getSurface()->Wdt, getSurface()->Hgt, 0);

	// Clear texture contents
	glClearColor(0.0f, 0.5f/1.5f, 0.5f/1.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// Render FoW to frame buffer object
	glBlendFunc(GL_ONE, GL_ONE);
	pFoW->Render(this, NULL, pPlayer);

	// Copy over the old state
	if (OldRegion.Wdt > 0) {

		// Set up shader. If this one doesn't work, we're really in trouble.
		C4Shader *pShader = pFoW->GetFramebufShader();
		assert(pShader);

		// How much the borders have moved
		int dx0 = Region.x - OldRegion.x,
			dy0 = Region.y - OldRegion.y,
			dx1 = Region.x + Region.Wdt - OldRegion.x - OldRegion.Wdt,
			dy1 = Region.y + Region.Hgt - OldRegion.y - OldRegion.Hgt;

		// Source and target rect coordinates (landscape coordinate system)
		int sx0 = Max(0, dx0),                  sy0 = Max(0, dy0),
			sx1 = OldRegion.Wdt - Max(0, -dx1), sy1 = OldRegion.Hgt - Max(0, -dy1),
			tx0 = Max(0, -dx0),                 ty0 = Max(0, -dy0),
			tx1 = Region.Wdt - Max(0, dx1),     ty1 = Region.Hgt - Max(0, dy1);

		// Quad coordinates
		float squad[8] = { float(sx0), float(sy0),  float(sx0), float(sy1),
			               float(sx1), float(sy1),  float(sx1), float(sy0) };
		int tquad[8] = { sx0, ty0,  tx0, ty1,  tx1, ty1, tx1, ty0, };

		// Transform into texture coordinates
		for (int i = 0; i < 4; i++) {
			squad[i*2] = squad[i*2] / getBackSurface()->Wdt;
			squad[i*2+1] = 1.0 - squad[i*2+1] / getBackSurface()->Hgt;
		}

		// Copy using shader
		C4ShaderCall Call(pShader);
		Call.Start();
		if (Call.AllocTexUnit(0, GL_TEXTURE_2D))
			glBindTexture(GL_TEXTURE_2D, getBackSurface()->textures[0].texName);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBegin(GL_QUADS);
		for (int i = 0; i < 4; i++)
		{
			glTexCoord2f(squad[i*2], squad[i*2+1]);
			glVertex2i(tquad[i*2], tquad[i*2+1]);
		}
		glEnd();
		Call.Finish();
    }

	// Done!
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	pDraw->RestorePrimaryClipper();
	glCheck();

	OldRegion = Region;

}

void C4FoWRegion::GetFragTransform(const C4Rect& clipRect, const C4Rect& outRect, float lightTransform[6]) const
{
	const C4Rect& lightRect = getRegion();
	const FLOAT_RECT& vpRect = ViewportRegion;

	C4FragTransform trans;
	// Clip offset
	assert(outRect.Hgt >= clipRect.y + clipRect.Hgt);
	trans.Translate(-clipRect.x, -(outRect.Hgt - clipRect.y - clipRect.Hgt));
	// Clip normalization (0,0 -> 1,1)
	trans.Scale(1.0f / clipRect.Wdt, 1.0f / clipRect.Hgt);
	// Viewport/Landscape normalization
	trans.Scale(vpRect.right - vpRect.left, vpRect.bottom - vpRect.top);
	// Offset between viewport and light texture
	trans.Translate(vpRect.left - lightRect.x, vpRect.top - lightRect.y);
	// Light surface normalization
	trans.Scale(1.0f / getSurface()->Wdt, 1.0f / getSurface()->Hgt);
	// Light surface Y offset
	trans.Translate(0.0f, 1.0f - (float)(lightRect.Hgt) / (float)getSurface()->Hgt);

	// Extract matrix
	trans.Get2x3(lightTransform);
}

C4FoWRegion::C4FoWRegion(C4FoW *pFoW, C4Player *pPlayer)
	: pFoW(pFoW)
	, pPlayer(pPlayer)
	, hFrameBufDraw(0), hFrameBufRead(0)
	, Region(0,0,0,0), OldRegion(0,0,0,0)
	, pSurface(NULL), pBackSurface(NULL)
{
	ViewportRegion.left = ViewportRegion.right = ViewportRegion.top = ViewportRegion.bottom = 0.0f;
}
