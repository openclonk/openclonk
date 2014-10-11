
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
		pSurface->ppTex[0]->texName, 0);
	if (pBackSurface)
		glFramebufferTexture2DEXT(GL_READ_FRAMEBUFFER_EXT,
			GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D,
			pBackSurface->ppTex[0]->texName, 0);

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

void C4FoWRegion::Update(C4Rect r)
{
	// Set the new region
	Region = r;
}

void C4FoWRegion::Render(const C4TargetFacet *pOnScreen)
{
	// Update FoW at interesting location
	pFoW->Update(Region);

	// On screen? No need to set up frame buffer - simply shortcut
	if (pOnScreen)
	{
		pFoW->Render(this, pOnScreen);
		return;
	}

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

	// Copy over the old state
	if (OldRegion.Wdt > 0) {

		int dx0 = Region.x - OldRegion.x,
			dy0 = Region.y - OldRegion.y,
			dx1 = Region.x + Region.Wdt - OldRegion.x - OldRegion.Wdt,
			dy1 = Region.y + Region.Hgt - OldRegion.y - OldRegion.Hgt;

		/*glBlitFramebufferEXT(
			Max(0, dx0),                  Max(0, -dy1),
			OldRegion.Wdt - Max(0, -dx1), OldRegion.Hgt - Max(0, dy0),
			Max(0, -dx0),                 Max(0, dy1),
			Region.Wdt - Max(0, dx1),     Region.Hgt - Max(0, -dy0),
			GL_COLOR_BUFFER_BIT, GL_LINEAR);
			*/

		int sx0 = Max(0, dx0),
			sy0 = Max(0, dy1),
			sx1 = OldRegion.Wdt - Max(0, -dx1),
			sy1 = OldRegion.Hgt - Max(0, -dy0),
			tx0 = Max(0, -dx0),
			ty0 = Max(0, -dy1),
			tx1 = Region.Wdt - Max(0, dx1),
			ty1 = Region.Hgt - Max(0, dy0);

		glEnable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, getBackSurface()->ppTex[0]->texName);

		glBlendFunc(GL_ONE, GL_ZERO);
		glBegin(GL_QUADS);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glTexCoord2f(float(sx0)/getBackSurface()->Wdt,float(getBackSurface()->Hgt-sy0)/getBackSurface()->Hgt); glVertex2i(tx0, ty0);
		glTexCoord2f(float(sx0)/getBackSurface()->Wdt,float(getBackSurface()->Hgt-sy1)/getBackSurface()->Hgt); glVertex2i(tx0, ty1);
		glTexCoord2f(float(sx1)/getBackSurface()->Wdt,float(getBackSurface()->Hgt-sy1)/getBackSurface()->Hgt); glVertex2i(tx1, ty1);
		glTexCoord2f(float(sx1)/getBackSurface()->Wdt,float(getBackSurface()->Hgt-sy0)/getBackSurface()->Hgt); glVertex2i(tx1, ty0);
		glEnd();
		glDisable(GL_TEXTURE_2D);

		glCheck();

		// Fade out. Note we constantly vary the alpha factor all the time -
		// this is barely visible but makes it a lot less likely that we 
		// hit cases where we add the same thing every time, but still don't
		// converge to the same color due to rounding.
		int iAdd = (Game.FrameCounter/3) % 2;
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(0.0f, 0.0f, 0.0f, 1.0f/16.0f+iAdd*1.0f/256.0f );  
		glBegin(GL_QUADS);
		glVertex2i(0, 0);
		glVertex2i(getSurface()->Wdt, 0);
		glVertex2i(getSurface()->Wdt, getSurface()->Hgt);
		glVertex2i(0, getSurface()->Hgt);
		glEnd();
	}

	// Render FoW to frame buffer object
	glBlendFunc(GL_ONE, GL_ONE);
	pFoW->Render(this);
	
	// Done!
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	pDraw->RestorePrimaryClipper();
	glCheck();

	OldRegion = Region;

}

C4FoWRegion::C4FoWRegion(C4FoW *pFoW, C4Player *pPlayer)
	: pFoW(pFoW)
	, pPlayer(pPlayer)
	, hFrameBufDraw(0), hFrameBufRead(0)
	, Region(0,0,0,0), OldRegion(0,0,0,0)
	, pSurface(NULL), pBackSurface(NULL)
{
}