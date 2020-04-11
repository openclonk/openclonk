/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2014-2016, The OpenClonk Team and contributors
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
#include "C4ForbidLibraryCompilation.h"
#include "landscape/fow/C4FoWRegion.h"
#include "graphics/C4DrawGL.h"

C4FoWRegion::C4FoWRegion(C4FoW *pFoW, C4Player *pPlayer)
	: pFoW(pFoW)
#ifndef USE_CONSOLE
	, pPlayer(pPlayer)
	, hFrameBufDraw(0), hFrameBufRead(0), hVBO(0), vaoid(0)
#endif
	, Region(0,0,0,0), OldRegion(0,0,0,0)
	, pSurface(new C4Surface), pBackSurface(new C4Surface)
{
	ViewportRegion.left = ViewportRegion.right = ViewportRegion.top = ViewportRegion.bottom = 0.0f;
}

C4FoWRegion::~C4FoWRegion()
{
#ifndef USE_CONSOLE
	if (hFrameBufDraw) {
		glDeleteFramebuffers(1, &hFrameBufDraw);
		glDeleteFramebuffers(1, &hFrameBufRead);
	}

	if (hVBO) {
		glDeleteBuffers(1, &hVBO);
	}

	if (vaoid) {
		pGL->FreeVAOID(vaoid);
	}
#endif
}

#ifndef USE_CONSOLE
bool C4FoWRegion::BindFramebuf(GLuint prev_fb)
{
	// Flip texture
	pSurface.swap(pBackSurface);

	// Can simply reuse old texture?
	if (pSurface->Wdt < Region.Wdt || (pSurface->Hgt / 2) < Region.Hgt)
	{
		// Determine texture size. Round up to next power of two in order to
		// prevent rounding errors, as well as preventing lots of
		// re-allocations when region size changes quickly (think zoom).
		int iWdt = 1, iHgt = 1;
		while (iWdt < Region.Wdt) iWdt *= 2;
		while (iHgt < Region.Hgt) iHgt *= 2;

		// Double the texture size. The second half of the texture
		// will contain the light color information, while the
		// first half contains the brightness and direction information
		iHgt *= 2;

		// Create the new surfaces
		std::unique_ptr<C4Surface> pNewSurface(new C4Surface);
		std::unique_ptr<C4Surface> pNewBackSurface(new C4Surface);
		if (!pNewSurface->Create(iWdt, iHgt))
			return false;
		if (!pNewBackSurface->Create(iWdt, iHgt))
			return false;

		// Copy over old content. This avoids flicker in already
		// explored regions that might get temporarily dark and
		// re-faded-in with the surface swap. New area in the surface
		// is initialized with darkness (black).
		pSurface->Lock();
		pBackSurface->Lock();
		pNewSurface->Lock();
		pNewBackSurface->Lock();

		// Take into account that the texture
		// is split for normals/intensity and colors, and also that
		// OpenGL textures are upside down.
		for (int y = 0; y < iHgt / 2; ++y)
		{
			for (int x = 0; x < iWdt; ++x)
			{
				if (y < pSurface->Hgt / 2 && x < pSurface->Wdt)
				{
					// Normals and intensity
					pNewSurface->SetPixDw(x, pNewSurface->Hgt/2 - y - 1, pSurface->GetPixDw(x, pSurface->Hgt/2 - y - 1, false));
					pNewBackSurface->SetPixDw(x, pNewBackSurface->Hgt/2 - y - 1, pBackSurface->GetPixDw(x, pBackSurface->Hgt/2 - y - 1, false));

					// Color
					pNewSurface->SetPixDw(x, pNewSurface->Hgt/2 - y + iHgt / 2 - 1, pSurface->GetPixDw(x, pSurface->Hgt/2 - y + pSurface->Hgt / 2 - 1, false));
					pNewBackSurface->SetPixDw(x, pNewBackSurface->Hgt/2 - y + iHgt / 2 - 1, pBackSurface->GetPixDw(x, pBackSurface->Hgt/2 - y + pBackSurface->Hgt / 2 - 1, false));
				}
				else
				{
					// Normals and intensity
					pNewSurface->SetPixDw(x, pNewSurface->Hgt/2 - y - 1, 0x000000ff);
					pNewBackSurface->SetPixDw(x, pNewBackSurface->Hgt/2 - y - 1, 0x000000ff);

					// Color
					pNewSurface->SetPixDw(x, pNewSurface->Hgt/2 - y + iHgt / 2 - 1, 0x000000ff);
					pNewBackSurface->SetPixDw(x, pNewBackSurface->Hgt/2 - y + iHgt / 2 - 1, 0x000000ff);
				}
			}
		}

		pSurface = std::move(pNewSurface);
		pBackSurface = std::move(pNewBackSurface);

		pSurface->Unlock();
		pBackSurface->Unlock();
	}

	// Cannot bind empty surface
	if (!pSurface->iTexSize) return false;

	// Generate frame buffer object
	if (!hFrameBufDraw)
	{
		glGenFramebuffers(1, &hFrameBufDraw);
		glGenFramebuffers(1, &hFrameBufRead);
	}

	// Bind current texture to frame buffer
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, hFrameBufDraw);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, hFrameBufRead);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
		GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
		pSurface->texture->texName, 0);
	if (pBackSurface->texture)
		glFramebufferTexture2D(GL_READ_FRAMEBUFFER,
			GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
			pBackSurface->texture->texName, 0);

	// Check status, unbind if something was amiss
	GLenum status1 = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER),
	       status2 = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
	if (status1 != GL_FRAMEBUFFER_COMPLETE ||
	   (pBackSurface && status2 != GL_FRAMEBUFFER_COMPLETE))
	{
		glBindFramebuffer(GL_FRAMEBUFFER, prev_fb);
		return false;
	}
	// Worked!
	return true;
}
#endif

int32_t C4FoWRegion::getSurfaceHeight() const
{
	return pSurface->Hgt;
}

int32_t C4FoWRegion::getSurfaceWidth() const
{
	return pSurface->Wdt;
}

#ifndef USE_CONSOLE
GLuint C4FoWRegion::getSurfaceName() const
{
	assert(pSurface->texture);
	if (!pSurface->texture)
		return 0;
	return pSurface->texture->texName;
}
#endif

void C4FoWRegion::Update(C4Rect r, const FLOAT_RECT& vp)
{
	// Set the new region
	Region = r;
	ViewportRegion = vp;
}

bool C4FoWRegion::Render(const C4TargetFacet *pOnScreen)
{
#ifndef USE_CONSOLE
	GLint prev_fb;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fb);

	// Update FoW at interesting location
	pFoW->Update(Region, pPlayer);

	// On screen? No need to set up frame buffer - simply shortcut
	if (pOnScreen)
	{
		pFoW->Render(this, pOnScreen, pPlayer, pGL->GetProjectionMatrix());
		return true;
	}

	// Set up shader. If this one doesn't work, we're really in trouble.
	C4Shader *pShader = pFoW->GetFramebufShader();
	assert(pShader);
	if (!pShader) return false;

	// Create & bind the frame buffer
	pDraw->StorePrimaryClipper();
	if(!BindFramebuf(prev_fb))
	{
		pDraw->RestorePrimaryClipper();
		return false;
	}
	assert(pSurface && hFrameBufDraw);
	if (!pSurface || !hFrameBufDraw)
		return false;

	// Set up a clean context
	glViewport(0, 0, pSurface->Wdt, pSurface->Hgt);
	const StdProjectionMatrix projectionMatrix = StdProjectionMatrix::Orthographic(0.0f, pSurface->Wdt, pSurface->Hgt, 0.0f);

	// Clear texture contents
	assert(pSurface->Hgt % 2 == 0);
	glScissor(0, pSurface->Hgt / 2, pSurface->Wdt, pSurface->Hgt / 2);
	glClearColor(0.0f, 0.5f / 1.5f, 0.5f / 1.5f, 0.0f);
	glEnable(GL_SCISSOR_TEST);
	glClear(GL_COLOR_BUFFER_BIT);

	// clear lower half of texture
	glScissor(0, 0, pSurface->Wdt, pSurface->Hgt / 2);
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_SCISSOR_TEST);

	// Render FoW to frame buffer object
	glBlendFunc(GL_ONE, GL_ONE);
	pFoW->Render(this, nullptr, pPlayer, projectionMatrix);

	// Copy over the old state
	if (OldRegion.Wdt > 0)
	{

		// How much the borders have moved
		int dx0 = Region.x - OldRegion.x,
			dy0 = Region.y - OldRegion.y,
			dx1 = Region.x + Region.Wdt - OldRegion.x - OldRegion.Wdt,
			dy1 = Region.y + Region.Hgt - OldRegion.y - OldRegion.Hgt;

		// Source and target rect coordinates (landscape coordinate system)
		int sx0 = std::max(0, dx0),                  sy0 = std::max(0, dy0),
			sx1 = OldRegion.Wdt - std::max(0, -dx1), sy1 = OldRegion.Hgt - std::max(0, -dy1),
			tx0 = std::max(0, -dx0),                 ty0 = std::max(0, -dy0),
			tx1 = Region.Wdt - std::max(0, dx1),     ty1 = Region.Hgt - std::max(0, dy1);

		// Quad coordinates
		float vtxData[16];
		float* squad = &vtxData[0];
		float* tquad = &vtxData[8];

		squad[0] = float(sx0);
		squad[1] = float(sy0);
		squad[2] = float(sx0);
		squad[3] = float(sy1);
		squad[4] = float(sx1);
		squad[5] = float(sy0);
		squad[6] = float(sx1);
		squad[7] = float(sy1);

		tquad[0] = float(tx0);
		tquad[1] = float(ty0);
		tquad[2] = float(tx0);
		tquad[3] = float(ty1);
		tquad[4] = float(tx1);
		tquad[5] = float(ty0);
		tquad[6] = float(tx1);
		tquad[7] = float(ty1);

		// Transform into texture coordinates
		for (int i = 0; i < 4; i++)
		{
			squad[i*2] = squad[i*2] / pBackSurface->Wdt;
			squad[i*2+1] = 1.0 - squad[i*2+1] / pBackSurface->Hgt;
		}

		// Load coordinates into vertex buffer
		if (hVBO == 0)
		{
			glGenBuffers(1, &hVBO);
			glBindBuffer(GL_ARRAY_BUFFER, hVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vtxData), vtxData, GL_STREAM_DRAW);

			assert(vaoid == 0);
			vaoid = pGL->GenVAOID();
		}
		else
		{
			glBindBuffer(GL_ARRAY_BUFFER, hVBO);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vtxData), vtxData);
		}

		// Copy using shader
		C4ShaderCall Call(pShader);
		Call.Start();
		if (Call.AllocTexUnit(C4FoWFSU_Texture))
			glBindTexture(GL_TEXTURE_2D, pBackSurface->texture->texName);
		Call.SetUniformMatrix4x4(C4FoWFSU_ProjectionMatrix, projectionMatrix);
		glBlendFunc(GL_ONE_MINUS_CONSTANT_COLOR, GL_CONSTANT_COLOR);
		float normalBlend = 1.0f / 4.0f, // Normals change quickly
		      brightBlend = 1.0f / 16.0f; // Intensity more slowly
		glBlendColor(0.0f,normalBlend,normalBlend,brightBlend);

		GLuint vao;
		const bool has_vao = pGL->GetVAO(vaoid, vao);
		glBindVertexArray(vao);
		if (!has_vao)
		{
			glEnableVertexAttribArray(pShader->GetAttribute(C4FoWFSA_Position));
			glEnableVertexAttribArray(pShader->GetAttribute(C4FoWFSA_TexCoord));
			glVertexAttribPointer(pShader->GetAttribute(C4FoWFSA_Position), 2, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<const uint8_t*>(8 * sizeof(float)));
			glVertexAttribPointer(pShader->GetAttribute(C4FoWFSA_TexCoord), 2, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<const uint8_t*>(0));
		}

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		Call.Finish();
	}

	// Done!
	glBindFramebuffer(GL_FRAMEBUFFER, prev_fb);
	pDraw->RestorePrimaryClipper();

	OldRegion = Region;
#endif
	return true;
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
	trans.Scale(1.0f / pSurface->Wdt, 1.0f / pSurface->Hgt);
	// Light surface Y offset
	trans.Translate(0.0f, 1.0f - (float)(lightRect.Hgt) / (float)pSurface->Hgt);

	// Extract matrix
	trans.Get2x3(lightTransform);
}
