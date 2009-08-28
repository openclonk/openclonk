/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2006  Matthes Bender
 * Copyright (c) 2002-2003, 2005-2006, 2008  Sven Eberhardt
 * Copyright (c) 2005  Peter Wortmann
 * Copyright (c) 2005-2008  Günther Brammer
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

/* A piece of a DirectDraw surface */

#include <C4Include.h>
#include <C4Facet.h>
#include <C4GraphicsResource.h>

#include <StdD3D.h>

#ifdef WITH_GLIB
#include <glib.h>
#endif

void C4Facet::Default()
	{
	Set(NULL,0,0,0,0);
	}

void C4Facet::Set(SURFACE nsfc, int32_t nx, int32_t ny, int32_t nwdt, int32_t nhgt)
  {
	Surface=nsfc; X=nx; Y=ny; Wdt=nwdt; Hgt=nhgt;
	}

C4Facet::C4Facet()
	{
	Default();
	}

int32_t C4Facet::GetSectionCount()
	{
	if (Hgt==0) return 0;
	return Wdt/Hgt;
	}

C4Facet C4Facet::GetSection(int32_t iSection)
	{
	C4Facet rval;
	rval.Set(Surface,X+Hgt*iSection,Y,Hgt,Hgt);
	return rval;
	}

C4Facet C4Facet::GetPhase(int iPhaseX, int iPhaseY)
	{
	C4Facet fctResult;
	fctResult.Set(Surface,X+Wdt*iPhaseX,Y+Hgt*iPhaseY,Wdt,Hgt);
	return fctResult;
	}

void C4Facet::Draw(SURFACE sfcTarget, float iX, float iY, int32_t iPhaseX, int32_t iPhaseY)
	{
	if (!lpDDraw || !Surface || !sfcTarget || !Wdt || !Hgt) return;

	lpDDraw->Blit(Surface,
								float(X+Wdt*iPhaseX),float(Y+Hgt*iPhaseY),float(Wdt),float(Hgt),
								sfcTarget,
								iX,iY,Wdt,Hgt,true);
	}

void C4Facet::DrawT(SURFACE sfcTarget, int32_t iX, int32_t iY, int32_t iPhaseX, int32_t iPhaseY, C4DrawTransform *pTransform)
	{
	if (!lpDDraw || !Surface || !sfcTarget || !Wdt || !Hgt) return;

	lpDDraw->Blit(Surface,
								float(X+Wdt*iPhaseX),float(Y+Hgt*iPhaseY),float(Wdt),float(Hgt),
								sfcTarget,
								iX,iY,Wdt,Hgt,true,pTransform);
	}

void C4Facet::DrawT(C4Facet &cgo, bool fAspect, int32_t iPhaseX, int32_t iPhaseY, C4DrawTransform *pTransform)
	{
	if (!lpDDraw || !Surface || !cgo.Surface || !Wdt || !Hgt) return;

	// Drawing area
	C4Facet ccgo = cgo;
	// Adjust for fixed aspect ratio
	if (fAspect)
		{
		// By height
		if (100*cgo.Wdt/Wdt<100*cgo.Hgt/Hgt)
			{
			ccgo.Hgt=Hgt*cgo.Wdt/Wdt;
			ccgo.Y+=(cgo.Hgt-ccgo.Hgt)/2;
			}
		// By width
		else if (100*cgo.Hgt/Hgt<100*cgo.Wdt/Wdt)
			{
			ccgo.Wdt=Wdt*cgo.Hgt/Hgt;
			ccgo.X+=(cgo.Wdt-ccgo.Wdt)/2;
			}
		}

	lpDDraw->Blit(Surface,
								float(X+Wdt*iPhaseX),float(Y+Hgt*iPhaseY),float(Wdt),float(Hgt),
								ccgo.Surface,ccgo.X,ccgo.Y,ccgo.Wdt,ccgo.Hgt,
								true,pTransform);
	}

void C4Facet::Draw(C4Facet &cgo, bool fAspect, int32_t iPhaseX, int32_t iPhaseY, bool fTransparent)
	{
	// Valid parameter check
	if (!lpDDraw || !Surface || !cgo.Surface || !Wdt || !Hgt) return;
	// Drawing area
	C4Facet ccgo = cgo;
	// Adjust for fixed aspect ratio
	if (fAspect)
		{
		// By height
		if (100*cgo.Wdt/Wdt<100*cgo.Hgt/Hgt)
			{
			ccgo.Hgt=Hgt*cgo.Wdt/Wdt;
			ccgo.Y+=(cgo.Hgt-ccgo.Hgt)/2;
			}
		// By width
		else if (100*cgo.Hgt/Hgt<100*cgo.Wdt/Wdt)
			{
			ccgo.Wdt=Wdt*cgo.Hgt/Hgt;
			ccgo.X+=(cgo.Wdt-ccgo.Wdt)/2;
			}
		}
	// Blit
	lpDDraw->Blit(Surface,
								float(X+Wdt*iPhaseX),float(Y+Hgt*iPhaseY),float(Wdt),float(Hgt),
								ccgo.Surface,
								ccgo.X,ccgo.Y,ccgo.Wdt,ccgo.Hgt,
								fTransparent);
	}

void C4Facet::DrawFullScreen(C4Facet &cgo)
	{
	// stretched fullscreen blit: make sure right and lower side are cleared, because this may be missed due to stretching
	if (cgo.Wdt > Wdt+2 || cgo.Hgt > Wdt+2)
		{
		lpDDraw->DrawBoxDw(cgo.Surface, cgo.X, cgo.Y+cgo.Hgt-1, cgo.X+cgo.Wdt+2, cgo.Y+cgo.Hgt+2, 0x00000000);
		lpDDraw->DrawBoxDw(cgo.Surface, cgo.X+cgo.Wdt-1, cgo.Y, cgo.X+cgo.Wdt+2, cgo.Y+cgo.Hgt+2, 0x00000000);
		}
	// normal blit OK
	Draw(cgo, false);
	}

void C4Facet::DrawClr(C4Facet &cgo, bool fAspect, DWORD dwClr)
	{
	if (!Surface) return;
	// set ColorByOwner-color
	Surface->SetClr(dwClr);
	// draw
	Draw(cgo, fAspect);
	}

void C4Facet::DrawXClr(SURFACE sfcTarget, int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, DWORD dwClr)
	{
	// set ColorByOwner-color
	Surface->SetClr(dwClr);
	// draw
	DrawX(sfcTarget, iX, iY, iWdt, iHgt);
	}

void C4Facet::DrawValue2Clr(C4Facet &cgo, int32_t iValue1, int32_t iValue2, DWORD dwClr)
	{
	// set ColorByOwner-color
	Surface->SetClr(dwClr);
	// draw
	DrawValue2(cgo, iValue1, iValue2);
	}

void C4Facet::DrawXR(SURFACE sfcTarget, int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, int32_t iSectionX, int32_t iSectionY, int32_t r)
	{
	if (!lpDDraw || !Surface || !sfcTarget || !Wdt || !Hgt) return;
	CBltTransform rot;
	rot.SetRotate(r, (float) (iX+iX+iWdt)/2, (float) (iY+iY+iHgt)/2);
	lpDDraw->Blit(Surface,
								float(X+Wdt*iSectionX),float(Y+Hgt*iSectionY),float(Wdt),float(Hgt),
								sfcTarget,
								iX,iY,iWdt,iHgt,
								true,&rot);
	}

C4Facet C4Facet::TruncateSection(int32_t iAlign)
	{
	C4Facet fctResult; fctResult.Set(Surface,0,0,0,0);
	// Calculate section size
	int32_t iWdt=Wdt,iHgt=Hgt;
	switch (iAlign & C4FCT_Alignment)
		{
		case C4FCT_Left: case C4FCT_Right:
			iWdt=Hgt;
			if (iAlign & C4FCT_Triple) iWdt*=3;
			if (iAlign & C4FCT_Double) iWdt*=2;
			if (iAlign & C4FCT_Half) iWdt/=2;
			break;
		case C4FCT_Top: case C4FCT_Bottom:
			iHgt=Wdt;
			if (iAlign & C4FCT_Triple) iHgt*=3;
			if (iAlign & C4FCT_Double) iHgt*=2;
			if (iAlign & C4FCT_Half) iHgt/=2;
			break;
		}
	// Size safety
	if ((iWdt>Wdt) || (iHgt>Hgt)) return fctResult;
	// Truncate
	switch (iAlign & C4FCT_Alignment)
		{
		case C4FCT_Left: fctResult.Set(Surface,X,Y,iWdt,iHgt); X+=iWdt; Wdt-=iWdt;	break;
		case C4FCT_Right: fctResult.Set(Surface,X+Wdt-iWdt,Y,iWdt,iHgt); Wdt-=iWdt; break;
		case C4FCT_Top: fctResult.Set(Surface,X,Y,iWdt,iHgt); Y+=iHgt; Hgt-=iHgt; break;
		case C4FCT_Bottom: fctResult.Set(Surface,X,Y+Hgt-iHgt,iWdt,iHgt); Hgt-=iHgt; break;
		}
	// Done
	return fctResult;
	}

C4Facet C4Facet::Truncate(int32_t iAlign, int32_t iSize)
	{
	C4Facet fctResult; fctResult.Set(Surface,0,0,0,0);
	// Calculate section size
	int32_t iWdt=Wdt,iHgt=Hgt;
	switch (iAlign)
		{
		case C4FCT_Left: case C4FCT_Right: iWdt=iSize;	break;
		case C4FCT_Top: case C4FCT_Bottom: iHgt=iSize;	break;
		}
	// Size safety
	if ((iWdt>Wdt) || (iHgt>Hgt)) return fctResult;
	// Truncate
	switch (iAlign)
		{
		case C4FCT_Left: fctResult.Set(Surface,X,Y,iWdt,iHgt); X+=iWdt; Wdt-=iWdt;	break;
		case C4FCT_Right: fctResult.Set(Surface,X+Wdt-iWdt,Y,iWdt,iHgt); Wdt-=iWdt; break;
		case C4FCT_Top: fctResult.Set(Surface,X,Y,iWdt,iHgt); Y+=iHgt; Hgt-=iHgt; break;
		case C4FCT_Bottom: fctResult.Set(Surface,X,Y+Hgt-iHgt,iWdt,iHgt); Hgt-=iHgt; break;
		}
	// Done
	return fctResult;
	}

void C4Facet::DrawSectionSelect(C4Facet &cgo, int32_t iSelection, int32_t iMaxSelect)
	{
	int32_t sections = cgo.GetSectionCount();
	int32_t idnum = iMaxSelect;
	int32_t firstid = BoundBy<int32_t>(iSelection-sections/2,0,Max<int32_t>(idnum-sections,0));
	C4Facet cgo2;
	for (int32_t cnt=0; (cnt<sections) && (firstid+cnt<idnum); cnt++)
		{
		cgo2 = cgo.GetSection(cnt);
		if (iSelection==firstid+cnt)
			lpDDraw->DrawBox(cgo2.Surface,cgo2.X,cgo2.Y,cgo2.X+cgo2.Wdt-1,cgo2.Y+cgo2.Hgt-1,CRed);
		Draw(cgo2,true,firstid+cnt,0);
    }
	}

void C4Facet::DrawValue(C4Facet &cgo, int32_t iValue, int32_t iSectionX, int32_t iSectionY, int32_t iAlign)
	{
	if (!lpDDraw) return;
	char ostr[25]; sprintf(ostr,"%i",iValue);
	switch (iAlign)
		{
		case C4FCT_Center:
			Draw(cgo, true, iSectionX, iSectionY);
			lpDDraw->TextOut(ostr, ::GraphicsResource.FontRegular, 1.0, cgo.Surface,
			                   cgo.X + cgo.Wdt - 1, cgo.Y + cgo.Hgt - 1, CStdDDraw::DEFAULT_MESSAGE_COLOR, ARight);
			break;
		case C4FCT_Right:
			{
			int32_t textwdt, texthgt;
			::GraphicsResource.FontRegular.GetTextExtent(ostr, textwdt, texthgt, false);
			lpDDraw->TextOut(ostr, ::GraphicsResource.FontRegular, 1.0, cgo.Surface,
			                   cgo.X + cgo.Wdt - 1, cgo.Y, CStdDDraw::DEFAULT_MESSAGE_COLOR, ARight);
			cgo.Set(cgo.Surface, cgo.X + cgo.Wdt - 1 - textwdt - 2 * cgo.Hgt, cgo.Y, 2 * cgo.Hgt, cgo.Hgt);
			Draw(cgo, true, iSectionX, iSectionY);
			break;
			}
		}
	}

void C4Facet::DrawValue2(C4Facet &cgo, int32_t iValue1, int32_t iValue2, int32_t iSectionX, int32_t iSectionY, int32_t iAlign, int32_t *piUsedWidth)
	{
	if (!lpDDraw) return;
	char ostr[25]; sprintf(ostr,"%i/%i",iValue1,iValue2);
	switch (iAlign)
		{
		case C4FCT_Center:
			Draw(cgo, true, iSectionX, iSectionY);
			lpDDraw->TextOut(ostr, ::GraphicsResource.FontRegular, 1.0, cgo.Surface,
			                   cgo.X + cgo.Wdt - 1, cgo.Y + cgo.Hgt - 1, CStdDDraw::DEFAULT_MESSAGE_COLOR, ARight);
			break;
		case C4FCT_Right:
			{
			int32_t textwdt, texthgt;
			::GraphicsResource.FontRegular.GetTextExtent(ostr, textwdt, texthgt, false);
			textwdt += Wdt + 3;
			lpDDraw->TextOut(ostr, ::GraphicsResource.FontRegular, 1.0, cgo.Surface,
			                   cgo.X + cgo.Wdt - 1, cgo.Y, CStdDDraw::DEFAULT_MESSAGE_COLOR, ARight);
			cgo.Set(cgo.Surface, cgo.X + cgo.Wdt - textwdt, cgo.Y, 2 * cgo.Hgt, cgo.Hgt);
			Draw(cgo, true, iSectionX, iSectionY);
			if (piUsedWidth) *piUsedWidth = textwdt;
			}
			break;
		}
	}

void C4Facet::DrawX(SURFACE sfcTarget, int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, int32_t iSectionX, int32_t iSectionY) const
	{
	if (!lpDDraw || !Surface || !sfcTarget || !Wdt || !Hgt) return;
	lpDDraw->Blit(Surface,
								float(X+Wdt*iSectionX),float(Y+Hgt*iSectionY),float(Wdt),float(Hgt),
								sfcTarget,
								iX,iY,iWdt,iHgt,
								true);
	}

void C4Facet::DrawXFloat(SURFACE sfcTarget, float fX, float fY, float fWdt, float fHgt) const
	{
	if (!lpDDraw || !Surface || !sfcTarget || !Wdt || !Hgt || fWdt<=0 || fHgt<=0) return;
	// Since only source coordinates are available as floats for blitting, go inwards into this facet to match blit
	// for closest integer target coordinates
	float zx = fWdt / float(Wdt), zy = fHgt / float(Hgt);
	int32_t iX = (int32_t) ceilf(fX), iY = (int32_t) ceilf(fY), iX2 = (int32_t) floorf(fX+fWdt), iY2 = (int32_t) floorf(fY+fHgt);
	float ox = (-fX+iX)/zx, oy = (-fY+iY)/zy;
	float oxs = (+fX+fWdt-iX2)/zx, oys = (+fY+fHgt-iY2)/zy;
	lpDDraw->Blit(Surface,
								float(X)+ox, float(Y)+oy, float(Wdt)-ox-oxs, float(Hgt)-oy-oys,
								sfcTarget,
								iX,iY,iX2-iX,iY2-iY,
								true);
	zx=(iX2-iX)/(float(Wdt)-ox-oxs);
	zy=(iY2-iY)/(float(Hgt)-oy-oys);
/*	int32_t iX = floorf(fX+0.5)-1, iY = floorf(fY+0.5)-1, iX2 = floorf(fX+fWdt+0.5)-1, iY2 = floorf(fY+fHgt+0.5)-1;
	lpDDraw->Blit(Surface,
								X, Y, Wdt, Hgt,
								sfcTarget,
								iX,iY,iX2-iX+1,iY2-iY+1,
								true);*/
	}

void C4Facet::DrawXT(SURFACE sfcTarget, int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, int32_t iPhaseX, int32_t iPhaseY, C4DrawTransform *pTransform)
	{
	if (!lpDDraw || !Surface || !sfcTarget || !Wdt || !Hgt) return;
	lpDDraw->Blit(Surface,
								float(X+Wdt*iPhaseX),float(Y+Hgt*iPhaseY),float(Wdt),float(Hgt),
								sfcTarget,
								iX,iY,iWdt,iHgt,
								true, pTransform);
	}

void C4Facet::DrawEnergyLevel(int32_t iLevel, int32_t iRange, int32_t iColor)
	{
	if (!lpDDraw) return;
	iLevel = BoundBy<int32_t>(iLevel,0,iRange);
	lpDDraw->DrawBox(Surface,
				X,Y,
				X+Wdt-1,Y+Hgt-1,
				40);
	lpDDraw->DrawBox(Surface,
				X+1,Y+1+(Hgt-2)-(Hgt-2)*iLevel/Max<int32_t>(iRange,1),
				X+Wdt-2,Y+Hgt-2,
				iColor);
	}

void C4Facet::DrawEnergyLevelEx(int32_t iLevel, int32_t iRange, const C4Facet &gfx, int32_t bar_idx)
	{
	// draw energy level using graphics
	if (!lpDDraw || !gfx.Surface) return;
	int32_t h=gfx.Hgt;
	int32_t yBar = Hgt - BoundBy<int32_t>(iLevel,0,iRange) * Hgt / Max<int32_t>(iRange,1);
	int32_t iY = 0, vidx=0;
	C4Facet gfx_draw = gfx;
	bool filled = false;
	while (iY < Hgt)
		{
		int32_t dy = iY % h;
		int32_t dh = (iY >= Hgt-h) ? Hgt - iY : h - dy;
		if (!filled)
			{
			if (iY >= yBar)
				{
				filled = true; // fully filled
				}
			else if (iY + h >= yBar)
				dh = yBar - iY; // partially filled
			}
		if (!vidx && iY && iY + dh > h)
			{
			if (iY < h)
				{
				// had a break within top section of bar; finish top section
				dh = h - iY;
				}
			else
				{
				// top section finished
				++vidx;
				}
			}
		if (iY + dh >= Hgt - h)
			{
			if (iY >= Hgt - h)
				{
				// within bottom section
				vidx = 2;
				dy = iY + h - Hgt;
				}
			else
				{
				// finish middle section
				dh = Hgt - h - iY;
				}
			}
		// draw it; partially if necessary
		gfx_draw.Y = gfx.Y + vidx*h + dy;
		gfx_draw.Hgt = dh;
		gfx_draw.Draw(Surface, X, Y+iY, bar_idx+bar_idx+!filled);
		iY += dh;
		}
	}

void C4Facet::Set(CSurface &rSfc)
	{
	Set(&rSfc,0,0,rSfc.Wdt,rSfc.Hgt);
	}

#ifdef _WIN32
void C4Facet::Draw(HWND hWnd, int32_t iTx, int32_t iTy, int32_t iTWdt, int32_t iTHgt, bool fAspect, int32_t iPhaseX, int32_t iPhaseY)
	{
#if defined(C4ENGINE) && defined(USE_DIRECTX)
	if (!pD3D || !Surface || !hWnd || !Wdt || !Hgt) return;

	if (fAspect)
		{
		int32_t iTx2=iTx,iTy2=iTy,iTWdt2=iTWdt,iTHgt2=iTHgt;
		// Adjust height aspect by width aspect
		if (100*iTWdt/Wdt<100*iTHgt/Hgt)
			{
			iTHgt2=Hgt*iTWdt/Wdt;
			iTy2+=(iTHgt-iTHgt2)/2;
			}
		// Adjust width aspect by height aspect
		else if (100*iTHgt/Hgt<100*iTWdt/Wdt)
			{
			iTWdt2=Wdt*iTHgt/Hgt;
			iTx2+=(iTWdt-iTWdt2)/2;
			}
		// Blit
		pD3D->BlitSurface2Window(Surface,
																X+Wdt*iPhaseX,Y+Hgt*iPhaseY,Wdt,Hgt,
																hWnd,
																iTx2,iTy2,iTWdt2,iTHgt2
																/*true*/);
		}

	else
		{
		// Blit
		pD3D->BlitSurface2Window(Surface,
																X+Wdt*iPhaseX,Y+Hgt*iPhaseY,Wdt,Hgt,
																hWnd,
																iTx,iTy,iTWdt,iTHgt
																/*true*/);
		}
#endif
	}
#endif

void C4Facet::DrawTile(SURFACE sfcTarget, int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt)
	{
	if (!lpDDraw || !Surface || !Wdt || !Hgt) return;
	// Blits whole source surface, not surface facet area
	lpDDraw->BlitSurfaceTile(Surface,sfcTarget,iX,iY,iWdt,iHgt,0,0,true);
	}

void C4Facet::Expand(int32_t iLeft, int32_t iRight, int32_t iTop, int32_t iBottom)
	{
	X-=iLeft; Wdt+=iLeft;
	Wdt+=iRight;
	Y-=iTop; Hgt+=iTop;
	Hgt+=iBottom;
	}

void C4Facet::Wipe()
	{
	if (!lpDDraw || !Surface || !Wdt || !Hgt) return;
	lpDDraw->WipeSurface(Surface);
	}

bool C4Facet::GetPhaseNum(int32_t &rX, int32_t &rY)
	{
	// safety
	if (!Surface) return false;
	// newgfx: use locally stored size
	rX=Surface->Wdt/Wdt; rY=Surface->Hgt/Hgt;
	// success
	return true;
	}

void C4DrawTransform::CompileFunc(StdCompiler *pComp)
	{
	bool fCompiler = pComp->isCompiler();
	int i;
	// hacky. StdCompiler doesn't allow floats to be safed directly.
	for(i = 0; i < 6; i++)
		{
		if(i) pComp->Seperator();
		StdStrBuf val;
		if(!fCompiler)
			{
#ifdef WITH_GLIB
			val.SetLength(G_ASCII_DTOSTR_BUF_SIZE);
			// g_ascii_dtostr is locale-independent which printf is not.
			g_ascii_dtostr(val.getMData(), G_ASCII_DTOSTR_BUF_SIZE, mat[i]);
#else
			val.Format("%g", mat[i]);
#endif
			}
		pComp->Value(mkParAdapt(val, StdCompiler::RCT_Idtf));
		if(fCompiler && pComp->hasNaming())
			if(pComp->Seperator(StdCompiler::SEP_PART))
				{
				StdStrBuf val2;
				pComp->Value(mkParAdapt(val2, StdCompiler::RCT_Idtf));
				val.AppendChar('.'); val.Append(val2);
				}
#ifdef WITH_GLIB
		mat[i] = g_ascii_strtod (val.getData(), NULL);
#else
		if(fCompiler) sscanf(val.getData(), "%g", &mat[i]);
#endif
		}
	pComp->Seperator();
	pComp->Value(FlipDir);
	if(!fCompiler && mat[6] == 0 && mat[7] == 0 && mat[8] == 1) return;
	// because of backwards-compatibility, the last row comes after flipdir
	for(i = 6; i < 9; ++i)
		{
		if (!pComp->Seperator())
			{
			mat[i] = (i == 8) ? 1.0f : 0.0f;
			}
		else
			{
			StdStrBuf val; if(!fCompiler) val.Format("%g", mat[i]);
			pComp->Value(mkParAdapt(val, StdCompiler::RCT_Idtf));
			if(fCompiler && pComp->hasNaming())
				if(pComp->Seperator(StdCompiler::SEP_PART))
					{
					StdStrBuf val2;
					pComp->Value(mkParAdapt(val2, StdCompiler::RCT_Idtf));
					val.AppendChar('.'); val.Append(val2);
					}
			if(fCompiler) sscanf(val.getData(), "%g", &mat[i]);
			}
		}
	}

void C4DrawTransform::SetTransformAt(C4DrawTransform &r, float iOffX, float iOffY)
	{
	// Set matrix, so that r*(x,y,1)-(x,y,1)==this*(x+iOffX,y+iOffY,1)-(x+iOffX,y+iOffY,1)
	float A = r.mat[0] + r.mat[6]*iOffX;
	float B = r.mat[1] + r.mat[7]*iOffX;
	float D = r.mat[3] + r.mat[6]*iOffY;
	float E = r.mat[4] + r.mat[7]*iOffY;
	CBltTransform::Set(
		A,        B,        r.mat[2] - A       *iOffX - B       *iOffY + r.mat[8]*iOffX,
		D,        E,        r.mat[5] - D       *iOffX - E       *iOffY + r.mat[8]*iOffY,
		r.mat[6], r.mat[7], r.mat[8] - r.mat[6]*iOffX - r.mat[7]*iOffY);
	}


C4Facet C4Facet::GetFraction(int32_t percentWdt, int32_t percentHgt, int32_t alignX, int32_t alignY)
{
	C4Facet rval;
	// Simple spec for square fractions
	if (percentHgt == 0) percentHgt = percentWdt;
	// Alignment
	int iX = X, iY = Y, iWdt = Max(Wdt*percentWdt/100, 1), iHgt = Max(Hgt*percentHgt/100, 1);
	if (alignX & C4FCT_Right)	iX += Wdt - iWdt;
	if (alignX & C4FCT_Center)	iX += Wdt/2 - iWdt/2;
	if (alignY & C4FCT_Bottom) iY += Hgt - iHgt;
	if (alignY & C4FCT_Center)	iY += Hgt/2 - iHgt/2;
	// Set resulting facet
	rval.Set(Surface, iX, iY, iWdt, iHgt);
	return rval;
}
