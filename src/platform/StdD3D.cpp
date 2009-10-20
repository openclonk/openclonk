/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2002, 2004-2007  Sven Eberhardt
 * Copyright (c) 2004-2008  Günther Brammer
 * Copyright (c) 2007  Peter Wortmann
 * Copyright (c) 2008  Matthes Bender
 * Copyright (c) 2009  Nicolas Hake
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

/* Direct3D implementation of NewGfx */

#include "C4Include.h"
#include <Standard.h>

#include <StdD3D.h>
#include <StdD3DShader.h>
#include <StdMarkup.h>
#include <StdWindow.h>

#ifdef USE_DIRECTX

#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>

CStdD3D::CStdD3D(bool fSoftware)
	{
	this->fSoftware = fSoftware;
	Default();
	// global ptr
	pD3D = this;
	}

CStdD3D::~CStdD3D()
	{
	Clear();
	pD3D=NULL;
	}

void CStdD3D::Default()
	{
	CStdDDraw::Default();
	SceneOpen=false;
	lpD3D=NULL;
	lpDevice=NULL;
	pVB=pVBClr=pVBClrTex=NULL;
	ZeroMemory(&sfcBmpInfo, sizeof(sfcBmpInfo));
	bltState[0] = bltState[1] = bltState[2] = 0;
	bltBaseState[0] = bltBaseState[1] = bltBaseState[2] = bltBaseState[3] = 0;
	drawSolidState[0] = drawSolidState[1] = 0;
	pSavedState = 0;
	for (int i=0; i<SHIDX_Size; ++i) pShaders[i]=NULL;
	}

void CStdD3D::Clear()
	{
	NoPrimaryClipper();
	if (pTexMgr) pTexMgr->IntUnlock();
	if (lpDevice)
		{
		EndScene();
		DeleteDeviceObjects();
		}
  if (lpD3D)
    {
		if (lpDevice) { lpDevice->Release(); lpDevice=NULL; }
    lpD3D->Release(); lpD3D=NULL;
    }
	SceneOpen=false;
	CStdDDraw::Clear();
	}


/* Direct3D initialization */

bool CStdD3D::PageFlip(RECT *pSrcRt, RECT *pDstRt, CStdWindow * pWindow)
	{
	// call from gfx thread only!
	if (!pApp || !pApp->AssertMainThread()) return false;
	// safety
	if (!lpDevice) return false;
	// end the scene and present it
  EndScene();
	if (lpDevice->Present(pSrcRt, pDstRt, pWindow ? pWindow->hWindow : 0, NULL) == D3DERR_DEVICELOST)
		{
		if (lpDevice->TestCooperativeLevel() == D3DERR_DEVICELOST) return false;
		if (!RestoreDeviceObjects()) return false;
		lpDevice->Present(NULL, NULL, NULL, NULL);
		}
  return true;
	}


bool CStdD3D::BeginScene()
	{
	// already open?
	if (SceneOpen) return true;
	// not active?
	if (!Active)
		if (!RestoreDeviceObjects())
			return false;
	// do open
	SceneOpen=(lpDevice->BeginScene() == D3D_OK);
	if (!SceneOpen) return false;
	// clear scene
	//lpDevice->Clear( 0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(rand()%256,rand()%256,rand()%256), 1.0f, 0L );
	// set some def values
	// success
	return true;
	}

void CStdD3D::FillBG(DWORD dwClr)
	{
	if (lpDevice) lpDevice->Clear( 0, NULL, D3DCLEAR_TARGET, dwClr, 1.0f, 0L );
	}

void CStdD3D::EndScene()
	{
	// end scene, if open
	if (!SceneOpen) return;
	lpDevice->EndScene();
	SceneOpen=false;
	}

bool CStdD3D::UpdateClipper()
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
	// clipping set to manual?
#ifdef _DEBUG
	// it won't hurt to clip anyway, if we are not debugging manual clipping
	if (DDrawCfg.ClipManually) return true;
#endif
	// bound clipper to surface size
	D3DVIEWPORT9 Clipper;
	Clipper.X=iX; Clipper.Y=iY; Clipper.Width=iWdt; Clipper.Height=iHgt;
	Clipper.MinZ=0.0f; Clipper.MaxZ=1.0f;
	// set it
	lpDevice->SetViewport(&Clipper);
	return true;
	}

bool CStdD3D::PrepareRendering(SURFACE sfcToSurface)
	{
	// call from gfx thread only!
	if (!pApp || !pApp->AssertMainThread()) return false;
	// do not render to inactive fullscreen
	if (!Active && fFullscreen) return false;
	// device?
	if (!lpDevice) return false;
	// target?
	if (!sfcToSurface) return false;
	// target locked?
	if (sfcToSurface->Locked) return false;
	// target is already set as render target?
	if (sfcToSurface != RenderTarget)
		{
		// target is a render-target?
		if (!sfcToSurface->IsRenderTarget()) return false;
		IDirect3DSurface9 *pDest=sfcToSurface->GetSurface();
		if (!pDest) return false;
		// set target
		EndScene();
		if (lpDevice->SetRenderTarget(0, pDest) != D3D_OK)
			{
			pDest->Release();
			return false;
			}
		RenderTarget=sfcToSurface;
		pDest->Release();
		// new target might need new clipping
		UpdateClipper();
		}
	// start scene, if not already done
	BeginScene();
	// done
	return true;
	}

void CStdD3D::PerformBlt(CBltData &rBltData, CTexRef *pTex, DWORD dwModClr, bool fMod2, bool fExact)
{
	if (!lpDevice || !pVB) return;
	if (DDrawCfg.ClipManually && rBltData.pTransform)
		if (!ClipPoly(rBltData)) return;

	// globally modulated blit
	int iAdditive = dwBlitMode & C4GFXBLIT_ADDITIVE;

	assert(rBltData.byNumVertices < sizeof(bltClrVertices)/sizeof(*bltClrVertices));
	// write to vertex buffer
	for (int i = 0; i < rBltData.byNumVertices; ++i)
	{
		bltClrVertices[i].x = rBltData.vtVtx[i].ftx;
		bltClrVertices[i].y = rBltData.vtVtx[i].fty;
		if (rBltData.pTransform)
			rBltData.pTransform->TransformPoint(bltClrVertices[i].x, bltClrVertices[i].y);
		bltClrVertices[i].tu = rBltData.vtVtx[i].ftx;
		bltClrVertices[i].tv = rBltData.vtVtx[i].fty;
		rBltData.TexPos.TransformPoint(bltClrVertices[i].tu, bltClrVertices[i].tv);
		bltClrVertices[i].color = dwModClr;
	}

	bltBaseState[iAdditive + (fMod2 ? C4GFXBLIT_MOD2 : 0)]->Apply();
	lpDevice->SetFVF(D3DFVF_C4CTVERTEX);

	// color mod map by pixel shader?
	CStdD3DShader *pShader = NULL;
	if (fUseClrModMap && HasShaders())
		{
		bool fColoredFoW = pClrModMap->IsColored();
		pShader = pShaders[fMod2 * SHIDX_Mod2 + fColoredFoW * SHIDX_ColoredFoW];
		lpDevice->SetPixelShader(pShader->GetInterface());
		lpDevice->SetTexture(pShader->iInTexIndex, pTex->pTex);
		CSurface * pModSurface = pClrModMap->GetSurface();
		if (pModSurface->ppTex) lpDevice->SetTexture(pShader->iFoWTexIndex, (*(pModSurface->ppTex))->pTex);
		const float mod_proj[4] = { 1.0f/(pClrModMap->GetResolutionX()*pModSurface->iTexSize),
			                        1.0f/(pClrModMap->GetResolutionY()*pModSurface->iTexSize),
									float(pClrModMap->OffX), float(pClrModMap->OffY) };
		lpDevice->SetPixelShaderConstantF(pShader->iFoWTransformIndex, mod_proj, 1);
		lpDevice->SetSamplerState( pShader->iFoWTexIndex, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR  );
		}
	else
		{
		// set texture
		lpDevice->SetTexture(0, pTex->pTex);
		}
	// override linear filter mode for exact blits
	if (fExact)
		{
		lpDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
		lpDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
		}
	// draw!
	lpDevice->DrawPrimitiveUP( D3DPT_TRIANGLEFAN, rBltData.byNumVertices-2, bltClrVertices, sizeof(bltClrVertices[0]) );
	// done, cleanup
	if (pShader)
		{
		lpDevice->SetPixelShader(NULL);
		lpDevice->SetTexture(pShader->iInTexIndex, NULL);
		lpDevice->SetTexture(pShader->iFoWTexIndex, NULL);
		}
	}

// get bit depth from surface format
unsigned int Format2BitDepth(D3DFORMAT format)
	{
	switch (format)
		{
		case D3DFMT_X1R5G5B5:
		case D3DFMT_R5G6B5:
			return 16;

		case D3DFMT_X8R8G8B8:
		case D3DFMT_A8R8G8B8:
			return 32;

		default:
			// unknown
			return 0;
		}
	}

bool CStdD3D::BlitTex2Window(CTexRef *pTexRef, HDC hdcTarget, RECT &rtFrom, RECT &rtTo)
	{
	// lock
	if (!pTexRef->Lock()) return false;
	// get bits
	BYTE *pBits = (BYTE *) pTexRef->texLock.pBits;
	if (!pBits) return false;
	// get size
	int fWdt = rtFrom.right-rtFrom.left;
	int fHgt = rtFrom.bottom-rtFrom.top;
	int tWdt = rtTo.right-rtTo.left;
	int tHgt = rtTo.bottom-rtTo.top;
	// adjust bitmap info
	sfcBmpInfo.bmiHeader.biHeight = sfcBmpInfo.bmiHeader.biWidth = pTexRef->iSize;
	// Same size
	if ((fWdt==tWdt) && (fHgt==tHgt))
		return SetDIBitsToDevice(hdcTarget, rtTo.left, rtTo.top, fWdt, fHgt, rtFrom.left, rtFrom.top, 0, fWdt, pBits, &sfcBmpInfo, DIB_RGB_COLORS) > 0;
	// Stretch
	SetMapMode(hdcTarget,MM_TEXT);
	int i = StretchDIBits(hdcTarget, rtTo.left, rtTo.top, tWdt, tHgt, rtFrom.left, rtFrom.top, fWdt, fHgt, pBits, &sfcBmpInfo, DIB_RGB_COLORS, SRCCOPY);
	if (i == GDI_ERROR) return false;
	return true;
	}

bool CStdD3D::BlitSurface2Window(SURFACE sfcSource,
																	 int fX, int fY, int fWdt, int fHgt,
																	 HWND hWnd,
																	 int tX, int tY, int tWdt, int tHgt)
	{
	bool fOkay = false;
	if (!sfcSource->Lock()) return false;
	HDC hdcTarget=GetDC(hWnd);
	if (hdcTarget)
		{
		// blit textures
		// blit with basesfc?
		bool fBaseSfc=false;
		if (sfcSource->pMainSfc) if (sfcSource->pMainSfc->ppTex) fBaseSfc=true;
		// calc stretch
		float scaleX=(float) tWdt/fWdt;
		float scaleY=(float) tHgt/fHgt;
		// get involved texture offsets
		int iTexSize=sfcSource->iTexSize;
		int iTexX=fX/iTexSize;
		int iTexY=fY/iTexSize;
		int iTexX2=Min((fX+fWdt-1)/iTexSize +1, sfcSource->iTexX);
		int iTexY2=Min((fY+fHgt-1)/iTexSize +1, sfcSource->iTexY);
		CTexRef **ppTex=sfcSource->ppTex+iTexY*sfcSource->iTexX+iTexX;
		// blit from all these textures
		CTexRef **ppTexRow, *pBaseTex=NULL;
		for (int iY=iTexY; iY<iTexY2; ++iY)
			{
			ppTexRow=ppTex;
			for (int iX=iTexX; iX<iTexX2; ++iX)
				{
				// get current blitting offset in texture
				int iBlitX=iTexSize*iX;
				int iBlitY=iTexSize*iY;
				// get new texture source bounds
				RECT fTexBlt;
				fTexBlt.left  = Max(fX-iBlitX, 0);
				fTexBlt.top   = Max(fY-iBlitY, 0);
				fTexBlt.right = Min(fX+fWdt-iBlitX, iTexSize);
				fTexBlt.bottom= Min(fY+fHgt-iBlitY, iTexSize);
				// get new dest bounds
				RECT tTexBlt;
				tTexBlt.left  = (long) ((fTexBlt.left  +iBlitX-fX)*scaleX+tX);
				tTexBlt.top   = (long) ((fTexBlt.top   +iBlitY-fY)*scaleY+tY);
				tTexBlt.right = (long) ((fTexBlt.right +iBlitX-fX)*scaleX+tX);
				tTexBlt.bottom= (long) ((fTexBlt.bottom+iBlitY-fY)*scaleY+tY);
				// is there a base-surface to be blitted first?
				if (fBaseSfc)
					{
					// then get this surface as same offset as from other surface
					// assuming this is only valid as long as there's no texture management,
					// organizing partially used textures together!
					pBaseTex=*(sfcSource->pMainSfc->ppTex+(ppTex-sfcSource->ppTex));
					BlitTex2Window(pBaseTex, hdcTarget, fTexBlt, tTexBlt);
					}
				// blit this texture
				BlitTex2Window(*ppTex, hdcTarget, fTexBlt, tTexBlt);
				// next col
				++ppTex;
				}
			// next row
			ppTex=ppTexRow+sfcSource->iTexX;
			}
		// done, release DC
		ReleaseDC(hWnd,hdcTarget);
		}
	sfcSource->Unlock();
	return fOkay;
	}

bool CStdD3D::FindDisplayMode(unsigned int iXRes, unsigned int iYRes, unsigned int iColorDepth, unsigned int iMonitor)
	{
	if(iColorDepth == 32)
		return FindDisplayMode(iXRes, iYRes, D3DFMT_A8R8G8B8, iMonitor) ||
		       FindDisplayMode(iXRes, iYRes, D3DFMT_X8R8G8B8, iMonitor);
	else if(iColorDepth == 16)
		return FindDisplayMode(iXRes, iYRes, D3DFMT_R5G6B5, iMonitor) ||
		       FindDisplayMode(iXRes, iYRes, D3DFMT_A1R5G5B5, iMonitor);
	else
		return false;
	}

bool CStdD3D::FindDisplayMode(unsigned int iXRes, unsigned int iYRes, D3DFORMAT format, unsigned int iMonitor)
	{
	bool fFound=false;
	D3DDISPLAYMODE dmode;
	// enumerate modes
	int iNumModes=lpD3D->GetAdapterModeCount(iMonitor, format);
	for (int i=0; i<iNumModes; i++)
		if (lpD3D->EnumAdapterModes(iMonitor, format, i, &dmode) == D3D_OK)
			{
			// size is OK?
			if (dmode.Width==iXRes && dmode.Height==iYRes)
				{
				// compare with found one
				if (fFound)
					// try getting a mode that is close to 85Hz, rather than taking the one with highest refresh rate
					// (which may set absurd modes on some devices)
					if (Abs<int>(85-dmode.RefreshRate)>Abs<int>(85-dspMode.RefreshRate))
						// the previous one was better
						continue;
				// choose this one
				fFound=true;
				dspMode=dmode;
				}
			}
	return fFound;
	}

bool CStdD3D::SetOutputAdapter(unsigned int iMonitor)
	{
	// get associated monitor rect
	HMONITOR hMon = lpD3D->GetAdapterMonitor(iMonitor);
	if (!hMon) return Error("GetAdapterMonitor error");
	MONITORINFO nfo; ZeroMemory(&nfo, sizeof(nfo));
	nfo.cbSize = sizeof(MONITORINFO);
	if (!GetMonitorInfo(hMon, &nfo)) return Error("GetMonitorInfo error");
	return true;
	}

bool CStdD3D::CreatePrimarySurfaces(bool Fullscreen, unsigned int iXRes, unsigned int iYRes, int iColorDepth, unsigned int iMonitor)
	{
	DebugLog("Init DX");
	DebugLog("  Create Direct3D9...");
	if ((lpD3D=Direct3DCreate9(D3D_SDK_VERSION))==NULL) return Error("  Direct3DCreate9 failure.");
	// set monitor info (Monitor-var and target rect)
	DebugLog("  SetOutput adapter...");
	if (!SetOutputAdapter(iMonitor))
		return Error("  Output adapter failure.");

	ZeroMemory( &d3dpp, sizeof(d3dpp) );
	// set needed surface type
	switch (iColorDepth)
		{
		case 16: dwSurfaceType=D3DFMT_A4R4G4B4; break;
		case 32: dwSurfaceType=D3DFMT_A8R8G8B8; break;
		}

	HRESULT hr;
	HWND hWindow = pApp->pWindow->hWindow;
	DebugLog("  Create Device...");
	if (Fullscreen)
		{
		// fullscreen mode
		// pick a display mode
#ifndef _DEBUG
		if (DDrawCfg.Windowed)
#else
		if (true)
#endif
			{
			// "Windowed fullscreen"
			if (lpD3D->GetAdapterDisplayMode(iMonitor, &dspMode) != D3D_OK)
				if (lpD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &dspMode) != D3D_OK)
					return Error("Could not get current display mode");
			}
		else
			{
			// true fullscreen
			if (!FindDisplayMode(iXRes, iYRes, iColorDepth, iMonitor))
				return Error("Display mode not supported");
			}
		// fill present structure
#ifndef _DEBUG
		if (DDrawCfg.Windowed)
#endif
			{
			SetWindowPos(hWindow, HWND_TOP, /*pApp->MonitorRect.left*/0, /*pApp->MonitorRect.top*/0,
				iXRes,iYRes, SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
			SetWindowLong(hWindow, GWL_STYLE, (WS_VISIBLE | WS_POPUP | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX));
			d3dpp.Windowed = true;
			}
		d3dpp.BackBufferWidth = iXRes;
		d3dpp.BackBufferHeight= iYRes;
		d3dpp.BackBufferFormat= dspMode.Format;
		d3dpp.BackBufferCount = 0;
		d3dpp.SwapEffect			= D3DSWAPEFFECT_FLIP;
		d3dpp.Flags						= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
		// create primary surface
		hr = lpD3D->CreateDevice(iMonitor, fSoftware ? D3DDEVTYPE_REF : D3DDEVTYPE_HAL, hWindow,
                        D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE,
                        &d3dpp, &lpDevice);
		}
	else
		{
		// windowed mode
		// get current display mode
		if (lpD3D->GetAdapterDisplayMode(iMonitor, &dspMode) != D3D_OK)
			if (lpD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &dspMode) != D3D_OK)
				return Error("Could not get current display mode");
		d3dpp.Windowed = true;
		d3dpp.BackBufferCount = 1;
		d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;
		d3dpp.BackBufferWidth = dspMode.Width;
		d3dpp.BackBufferHeight= dspMode.Height;
		d3dpp.BackBufferFormat = dspMode.Format;
		d3dpp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
		// create primary surface
		hr = lpD3D->CreateDevice(iMonitor, fSoftware ? D3DDEVTYPE_REF : D3DDEVTYPE_HAL, hWindow,
                        D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE,
                        &d3dpp, &lpDevice);
		// windowed mode: Try dfault adapter as well
		if (hr != D3D_OK && iMonitor != D3DADAPTER_DEFAULT)
			hr = lpD3D->CreateDevice(D3DADAPTER_DEFAULT, fSoftware ? D3DDEVTYPE_REF : D3DDEVTYPE_HAL, hWindow,
                        D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE,
                        &d3dpp, &lpDevice);
		}
	switch (hr)
		{
		case D3DERR_INVALIDCALL: return Error("CreateDevice: D3DERR_INVALIDCALL");
		case D3DERR_NOTAVAILABLE: return Error("CreateDevice: D3DERR_NOTAVAILABLE");
		case D3DERR_OUTOFVIDEOMEMORY: return Error("CreateDevice: D3DERR_OUTOFVIDEOMEMORY");
		case D3DERR_DRIVERINTERNALERROR: return Error("CreateDevice: D3DERR_DRIVERINTERNALERROR");
		case D3D_OK: break;
		default: return Error("CreateDevice: unknown error");
		}
	// device successfully created?
	if (!lpDevice) return Error("CreateDevice: unreported error");
	// store color depth
	byByteCnt=iColorDepth/8;
	PrimarySrfcFormat=d3dpp.BackBufferFormat;
	// create bmi-header
	ZeroMemory(&sfcBmpInfo, sizeof(sfcBmpInfo));
	sfcBmpInfo.bmiHeader.biSize					= sizeof(BITMAPINFOHEADER);
	sfcBmpInfo.bmiHeader.biWidth				= iXRes;
	sfcBmpInfo.bmiHeader.biHeight				= -int(iYRes);
	sfcBmpInfo.bmiHeader.biPlanes				= 1;
	sfcBmpInfo.bmiHeader.biBitCount			= Format2BitDepth(dspMode.Format);
	sfcBmpInfo.bmiHeader.biCompression  = BI_RGB;
	// initialize device-dependent objects
	if(!InitDeviceObjects())
		{
		// cleanup
		DeleteDeviceObjects();
		// failure
		return false;
		}
	// update monitor rect by new screen size
	if (!SetOutputAdapter(iMonitor)) return false;
	// success!
	return true;
	}

bool CStdD3D::OnResolutionChanged(unsigned int iXRes, unsigned int iYRes)
	{
	return true;
	}

bool CStdD3D::SetVideoMode(unsigned int iXRes, unsigned int iYRes, unsigned int iColorDepthFIXME, unsigned int iMonitor, bool fFullScreen)
	{
	// note that this should happen in fullscreen mode only!
	// do not mess with real presetn parameters until resolution change worked
	D3DPRESENT_PARAMETERS d3dpp;
	// reinit window for new resolution
	int iColorDepth = byByteCnt*8;
	ZeroMemory( &d3dpp, sizeof(d3dpp) );
	HRESULT hr;
	HWND hWindow = pApp->pWindow->hWindow;
	// pick a display mode
#ifndef _DEBUG
	if (DDrawCfg.Windowed)
#else
	if (true)
#endif
		{
		// "Windowed fullscreen"
		if (lpD3D->GetAdapterDisplayMode(iMonitor, &dspMode) != D3D_OK)
			if (lpD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &dspMode) != D3D_OK)
				return Error("Could not get current display mode");
		}
	else
		{
		// true fullscreen
		if (!FindDisplayMode(iXRes, iYRes, iColorDepth, iMonitor))
			return Error("Display mode not supported");
		}
	// fill present structure
#ifndef _DEBUG
	if (DDrawCfg.Windowed)
#endif
		{
		SetWindowPos(hWindow, HWND_TOP, /*pApp->MonitorRect.left*/0, /*pApp->MonitorRect.top*/0,
			iXRes, iYRes, SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
		SetWindowLong(hWindow, GWL_STYLE, (WS_VISIBLE | WS_POPUP | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX));
		d3dpp.Windowed = true;
		}
	d3dpp.BackBufferWidth = iXRes;
	d3dpp.BackBufferHeight= iYRes;
	d3dpp.BackBufferFormat= dspMode.Format;
	d3dpp.BackBufferCount = 0;
	d3dpp.SwapEffect			= D3DSWAPEFFECT_FLIP;
	d3dpp.Flags						= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
	// reset device to these parameters
	DeleteDeviceObjects();
	hr = lpDevice->Reset(&d3dpp);
	if (hr != D3D_OK)
		{
		// reset failed
		if (InitDeviceObjects())
			RestoreDeviceObjects(); // try restore at old resolution
		switch (hr)
			{
			case D3DERR_INVALIDCALL: return Error("    CreateDevice: D3DERR_INVALIDCALL");
			case D3DERR_NOTAVAILABLE: return Error("    CreateDevice: D3DERR_NOTAVAILABLE");
			case D3DERR_OUTOFVIDEOMEMORY: return Error("    CreateDevice: D3DERR_OUTOFVIDEOMEMORY");
			case D3DERR_DRIVERINTERNALERROR: return Error("    CreateDevice: D3DERR_DRIVERINTERNALERROR");
			default: return Error("    CreateDevice: unknown error");
			}
		}
	// device successfully reset! Reflect changes
	this->d3dpp = d3dpp;
	// store color depth
	byByteCnt=iColorDepth/8;
	PrimarySrfcFormat=d3dpp.BackBufferFormat;
	// update bmi-header
	sfcBmpInfo.bmiHeader.biWidth				= iXRes;
	sfcBmpInfo.bmiHeader.biHeight				= -int(iYRes);
	sfcBmpInfo.bmiHeader.biBitCount			= Format2BitDepth(dspMode.Format);
	// reinit device-dependant objects
	InitDeviceObjects();
	RestoreDeviceObjects();
	// update monitor rect by new screen size
	if (!SetOutputAdapter(iMonitor)) return false;
	// success!
	return true;
	}

void CStdD3D::PerformPix(SURFACE sfcDest, float tx, float ty, DWORD dwClr)
	{
	// is render target?
	if (!sfcDest->IsRenderTarget())
		// on non-rendertargets, just set using locks
		{
		sfcDest->SetPixDw((int)tx, (int)ty, dwClr);
		return;
		}
	else if(sfcDest->IsLocked())
		{
		DrawPixPrimary(sfcDest, int(tx), int(ty), dwClr);
		return;
		}
	// on a render target, blit as a box (ugh!)
	// set vertex buffer data
	// vertex order:
	// 0=upper left   dwClr1
	// 1=lower left   dwClr3
	// 2=lower right  dwClr4
	// 3=upper right  dwClr2
	float vtx[8];
	vtx[0] = tx     ; vtx[1] = ty;
	vtx[2] = tx     ; vtx[3] = ty+1.0f;
	vtx[4] = tx+1.0f; vtx[5] = ty+1.0f;
	vtx[6] = tx+1.0f; vtx[7] = ty;
	DrawQuadDw(sfcDest, vtx, dwClr, dwClr, dwClr, dwClr);
	}

void CStdD3D::DrawPixPrimary(SURFACE sfcDest, int iX, int iY, DWORD dwClr)
	{
	// Must be render target and locked
	if(!sfcDest->IsRenderTarget() || !sfcDest->IsLocked()) return;
	// set according to pixel format
	BYTE *pBits = sfcDest->PrimarySurfaceLockBits;
	int iPitch = sfcDest->PrimarySurfaceLockPitch;
	DWORD *pPix32; WORD *pPix16;
	switch (sfcDest->dwClrFormat)
		{
		case D3DFMT_X1R5G5B5:
			// 16 bit 5-5-5
			pPix16=(WORD *) (pBits+iY*iPitch+iX*2);
			*pPix16=WORD((dwClr & 0x000000f8) >> 3)
						| WORD((dwClr & 0x0000f800) >> 6)
						| WORD((dwClr & 0x00f80000) >> 9);
			break;

		case D3DFMT_R5G6B5:
			// 16 bit 5-6-5
			pPix16=(WORD *) (pBits+iY*iPitch+iX*2);
			*pPix16=WORD((dwClr & 0x000000f8) >> 3)
						| WORD((dwClr & 0x0000fc00) >> 5)
						| WORD((dwClr & 0x00f80000) >> 8);
			break;

		case D3DFMT_X8R8G8B8:
			// 32 bit
			pPix32=(DWORD *) (pBits+iY*iPitch+iX*4);
			BltAlpha(*pPix32, dwClr);
			break;
		}
	}

void CStdD3D::DrawQuadDw(SURFACE sfcTarget, float *ipVtx, DWORD dwClr1, DWORD dwClr2, DWORD dwClr3, DWORD dwClr4)
	{
	// prepare rendering to target
	if (!PrepareRendering(sfcTarget)) return;
	// no clr fading supported
	if (DDrawCfg.NoBoxFades) NormalizeColors(dwClr1, dwClr2, dwClr3, dwClr4);
	// set blitting state
	int iAdditive = dwBlitMode & C4GFXBLIT_ADDITIVE;
	drawSolidState[iAdditive]->Apply();
	lpDevice->SetRenderState( D3DRS_DESTBLEND,  iAdditive ? D3DBLEND_ONE : D3DBLEND_INVSRCALPHA );
	lpDevice->SetStreamSource(0, pVBClr, 0, sizeof(C4CLRVERTEX));
	lpDevice->SetFVF(D3DFVF_C4CLRVERTEX);
	// 2do: manual clipping?
	// set vertex buffer data
	float fX1 = (float) ipVtx[0] - 0.5f;
	float fY1 = (float) ipVtx[1] - 0.5f;
	float fX2 = (float) ipVtx[2] - 0.5f;
	float fY2 = (float) ipVtx[3] - 0.5f;
	float fX3 = (float) ipVtx[4] - 0.5f;
	float fY3 = (float) ipVtx[5] - 0.5f;
	float fX4 = (float) ipVtx[6] - 0.5f;
	float fY4 = (float) ipVtx[7] - 0.5f;
	// apply color modulation
	if (BlitModulated)
		{
		ModulateClr(dwClr1, BlitModulateClr);
		ModulateClr(dwClr2, BlitModulateClr);
		ModulateClr(dwClr3, BlitModulateClr);
		ModulateClr(dwClr4, BlitModulateClr);
		}
	// set vertices
	clrVertices[0].x = fX1; clrVertices[0].y = fY1; clrVertices[0].color=dwClr1;
	clrVertices[1].x = fX2; clrVertices[1].y = fY2; clrVertices[1].color=dwClr2;
	clrVertices[2].x = fX3; clrVertices[2].y = fY3; clrVertices[2].color=dwClr3;
	clrVertices[3].x = fX3; clrVertices[3].y = fY3; clrVertices[3].color=dwClr3;
	clrVertices[4].x = fX4; clrVertices[4].y = fY4; clrVertices[4].color=dwClr4;
	clrVertices[5].x = fX1; clrVertices[5].y = fY1; clrVertices[5].color=dwClr1;
	if (fUseClrModMap)
		for (int i = 0; i < 6; ++i) ModulateClr(clrVertices[i].color, pClrModMap->GetModAt((int)clrVertices[i].x, (int)clrVertices[i].y));
	// copy into vertex buffer
	VOID* pVertices;
	if( pVBClr->Lock(0, sizeof(clrVertices), &pVertices, 0) != D3D_OK ) return;
	memcpy(pVertices, clrVertices, sizeof(clrVertices));
	pVBClr->Unlock();
	// draw
	lpDevice->DrawPrimitive( D3DPT_TRIANGLELIST, 0, 2 );
	}

void CStdD3D::PerformLine(SURFACE sfcTarget, float x1, float y1, float x2, float y2, DWORD dwClr)
	{
	//dwClr |= 0xf0000000;
	// render target?
	if (sfcTarget->IsRenderTarget())
		if(!sfcTarget->IsLocked())
			{
			// draw as primitive
			if (!PrepareRendering(sfcTarget)) return;
			// set blitting state - use src alpha channel als opacity because some systems cannot antialias otherwise
			int iAdditive = dwBlitMode & C4GFXBLIT_ADDITIVE;
			drawSolidState[iAdditive]->Apply();
			lpDevice->SetRenderState( D3DRS_DESTBLEND,  iAdditive ? D3DBLEND_ONE : D3DBLEND_INVSRCALPHA );
			lpDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			lpDevice->SetRenderState( D3DRS_ANTIALIASEDLINEENABLE, true );
			lpDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, true );
			lpDevice->SetStreamSource(0, pVBClr, 0, sizeof(C4CLRVERTEX));
			lpDevice->SetFVF(D3DFVF_C4CLRVERTEX);
			// set vertex buffer data
			float fX1 = (float) x1;// - 0.5f;
			float fY1 = (float) y1;// - 0.5f;
			float fX2 = (float) x2;// + 0.5f;
			float fY2 = (float) y2;// + 0.5f;
			clrVertices[0].x = fX1; clrVertices[0].y = fY1; clrVertices[0].color=dwClr;
			clrVertices[1].x = fX2; clrVertices[1].y = fY2; clrVertices[1].color=dwClr;
			clrVertices[2].x = fX2; clrVertices[2].y = fY2; clrVertices[2].color=dwClr;
			clrVertices[3].x = fX1; clrVertices[3].y = fY1; clrVertices[3].color=dwClr;
			if (fUseClrModMap)
				for (int i = 0; i < 4; ++i) ModulateClr(clrVertices[i].color, pClrModMap->GetModAt((int)clrVertices[i].x, (int)clrVertices[i].y));
			// copy into vertex buffer
			VOID* pVertices;
			if( pVBClr->Lock(0, sizeof(clrVertices[0])*4, &pVertices, 0) != D3D_OK ) return;
			memcpy(pVertices, clrVertices, sizeof(clrVertices[0])*4);
			pVBClr->Unlock();
			// draw - actually two lines, to ensure that the last pixels are drawn, too...
			lpDevice->DrawPrimitive(D3DPT_LINELIST, 0, 2 );
			}
		else
			{
			if (fUseClrModMap)
				ModulateClr(dwClr, pClrModMap->GetModAt((int)((x1+x2)/2), (int)((y1+y2)/2)));
			if(Abs(x1 - x2) > Abs(y1 - y2))
				{
				// flip line direction
				if(x2 < x1)
					{
					float tmp = x2; x2 = x1; x1 = tmp;
					tmp = y2; y2 = y1; y1 = tmp;
					}
				// round coordinates
				int32_t iX1 = BoundBy<int32_t>(int32_t(x1+.5f),0,sfcTarget->Wdt-1),
				        iX2 = BoundBy<int32_t>(int32_t(x2+.5f),0,sfcTarget->Wdt-1),
				        iY1 = BoundBy<int32_t>(int32_t(y1+.5f),0,sfcTarget->Hgt-1),
				        iY2 = BoundBy<int32_t>(int32_t(y2+.5f),0,sfcTarget->Hgt-1),
				        iDX = iX2 - iX1;
				// single pixel case?
				if(!iDX) { DrawPixPrimary(sfcTarget, iX1, iY1, dwClr); return; }
				// calculate gradient
				uint32_t g = ((uint32_t(Abs(iY2 - iY1)) << 16) / uint32_t(iX2 - iX1)) << 16;
				// alpha divisor
				uint32_t alpha = dwClr >> 24;
				if (alpha == 0) return; // invisible line
				uint32_t div = (uint32_t(1 << 24) / alpha) << 8;
				DWORD dwClrBase = dwClr & 0x00FFFFFF;
				// current position
				uint32_t sp = 0;
				if(y2 > y1)
					for(int32_t iX = 0, iY = 0; ; iX++)
						{
						// draw pixels
						DWORD dwClr1 = dwClrBase + ((alpha + sp / div) << 24),
									dwClr2 = dwClrBase + ((255 - sp / div) << 24);
						DrawPixPrimary(sfcTarget, iX1+iX, iY1+iY,   dwClr1);
						if(sp) DrawPixPrimary(sfcTarget, iX1+iX, iY1+iY+1, dwClr2);
						if(iX * 2 >= iDX) break;
						DrawPixPrimary(sfcTarget, iX2-iX, iY2-iY,	  dwClr1);
						if(sp) DrawPixPrimary(sfcTarget, iX2-iX, iY2-iY-1, dwClr2);
						if(iX * 2 + 1 >= iDX) break;
						// next pixel
						sp += g; if(sp < g) iY++;
						}
				else
					for(int32_t iX = 0, iY = 0; ; iX++)
						{
						// draw pixels
						DWORD dwClr1 = dwClrBase + ((alpha + sp / div) << 24),
									dwClr2 = dwClrBase + ((255 - sp / div) << 24);
						DrawPixPrimary(sfcTarget, iX1+iX, iY1+iY,   dwClr1);
						if(sp) DrawPixPrimary(sfcTarget, iX1+iX, iY1+iY-1, dwClr2);
						if(iX * 2 >= iDX) break;
						DrawPixPrimary(sfcTarget, iX2-iX, iY2-iY,	  dwClr1);
						if(sp) DrawPixPrimary(sfcTarget, iX2-iX, iY2-iY+1, dwClr2);
						if(iX * 2 + 1 >= iDX) break;
						// next pixel
						sp += g; if(sp < g) iY--;
						}
				}
			else
				{
				// flip line direction
				if(y2 < y1)
					{
					float tmp = y2; y2 = y1; y1 = tmp;
					tmp = x2; x2 = x1; x1 = tmp;
					}
				// calculate gradient
				uint32_t g = uint32_t( Abs(x2 - x1) / (y2 - y1) * 4294967296.0f );
				// round coordinates
				int32_t iX1 = BoundBy<int32_t>(int32_t(x1+.5f),0,sfcTarget->Wdt-1),
				        iX2 = BoundBy<int32_t>(int32_t(x2+.5f),0,sfcTarget->Wdt-1),
				        iY1 = BoundBy<int32_t>(int32_t(y1+.5f),0,sfcTarget->Hgt-1),
				        iY2 = BoundBy<int32_t>(int32_t(y2+.5f),0,sfcTarget->Hgt-1),
				        iDY = iY2 - iY1;
				// single pixel case?
				if(!iDY) { DrawPixPrimary(sfcTarget, iX1, iY1, dwClr); return; }
				// alpha divisor
				uint32_t alpha = dwClr >> 24;
				if (alpha == 0) return; // invisible line
				uint32_t div = (uint32_t(1 << 24) / alpha) << 8;
				DWORD dwClrBase = dwClr & 0x00FFFFFF;
				// current position
				uint32_t sp = 0;
				if(x2 > x1)
					for(int32_t iY = 0, iX = 0; ; iY++)
						{
						// draw pixels
						DWORD dwClr1 = dwClrBase + ((alpha + sp / div) << 24),
									dwClr2 = dwClrBase + ((255 - sp / div) << 24);
						DrawPixPrimary(sfcTarget, iX1+iX,   iY1+iY, dwClr1);
						if(sp) DrawPixPrimary(sfcTarget, iX1+iX+1, iY1+iY, dwClr2);
						if(iY * 2 >= iDY) break;
						DrawPixPrimary(sfcTarget, iX2-iX,	  iY2-iY, dwClr1);
						if(sp) DrawPixPrimary(sfcTarget, iX2-iX-1, iY2-iY, dwClr2);
						if(iY * 2 + 1 >= iDY) break;
						// next pixel
						sp += g; if(sp < g) iX++;
						}
				else
					for(int32_t iY = 0, iX = 0; ; iY++)
						{
						// draw pixels
						DWORD dwClr1 = dwClrBase + ((alpha + sp / div) << 24),
									dwClr2 = dwClrBase + ((255 - sp / div) << 24);
						DrawPixPrimary(sfcTarget, iX1+iX,   iY1+iY, dwClr1);
						if(sp) DrawPixPrimary(sfcTarget, iX1+iX-1, iY1+iY, dwClr2);
						if(iY * 2 >= iDY) break;
						DrawPixPrimary(sfcTarget, iX2-iX,	  iY2-iY, dwClr1);
						if(sp) DrawPixPrimary(sfcTarget, iX2-iX+1, iY2-iY, dwClr2);
						if(iY * 2 + 1 >= iDY) break;
						// next pixel
						sp += g; if(sp < g) iX--;
						}
				}
			}
	else /* if(!sfcTarget->IsRenderTarget()) */
		{
		if (!LockSurfaceGlobal(sfcTarget)) return;
		ForLine((int)x1,(int)y1,(int)x2,(int)y2,&DLineSPixDw,dwClr);
		UnLockSurfaceGlobal(sfcTarget);
		}
	}

bool CStdD3D::InitDeviceObjects()
	{
	D3DCAPS9 d3dCaps;
	pD3D->lpDevice->GetDeviceCaps(&d3dCaps);
	MaxTexSize = d3dCaps.MaxTextureWidth;
	bool fSuccess=true;
	// create lpPrimary and lpBack
	lpPrimary=lpBack=new CSurface();
	// Create shaders
	for (int i=0; i<SHIDX_Size; ++i) if (pShaders[i]) { delete pShaders[i]; pShaders[i]=NULL; }
	if (DDrawCfg.Shader)
		{
		if (!InitShaders())
			{
			Error("    Shader initizlization failed - reverting to no-shader-mode.");
			}
		}
	// restore everything
	return fSuccess && RestoreDeviceObjects();
	}

bool CStdD3D::InitShaders()
	{
	// safety?
	if (!lpDevice) return false;
	return false;
	// create all desired shaders. Fail and delete them all if any one fails.
	for (int i=0; i<SHIDX_Size; ++i) pShaders[i] = new CStdD3DShader();
	bool fOK = true;
	for (int i=0; i<SHIDX_Size; ++i)
		{
		if (!pShaders[i]->Init(lpDevice, !!(i&SHIDX_Mod2), !!(i&SHIDX_ColoredFoW))) { fOK = false; break; }
		}
	if (!fOK) for (int i=0; i<SHIDX_Size; ++i) if (pShaders[i]) { delete pShaders[i]; pShaders[i]=NULL; }
	return fOK;
	}

bool CStdD3D::RestoreDeviceObjects()
	{
	// any device?
	if (!lpDevice) return false;
	// delete any previous objects
	InvalidateDeviceObjects();
	// if the device is in a lost state: reset it
	if (lpDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET) lpDevice->Reset(&d3dpp);
	// couldn't reset?
	if (lpDevice->TestCooperativeLevel() != D3D_OK) return false;
	bool fSuccess=true;
	// restore primary/back
	IDirect3DSurface9 *pPrimarySfc;
	if (lpDevice->GetRenderTarget(0, &pPrimarySfc) != D3D_OK)
		{ Error("Could not get primary surface"); fSuccess=false; }
	RenderTarget=lpPrimary;
	lpPrimary->AttachSfc(pPrimarySfc,0,0);
	lpPrimary->dwClrFormat=PrimarySrfcFormat;
	lpPrimary->byBytesPP=Format2BitDepth(PrimarySrfcFormat)/8;
	// create vertex buffer
	if( lpDevice->CreateVertexBuffer(sizeof(bltVertices), 0, D3DFVF_C4VERTEX, D3DPOOL_DEFAULT, &pVB, NULL) != D3D_OK ) return false;
	// fill initial data for vertex buffer
	int i;
	for (i=0; i<6; ++i)
		{
		bltVertices[i].z=0.9f;
		bltVertices[i].rhw=1.0f;
		}
	// create solid color vertex buffer
	if( lpDevice->CreateVertexBuffer(sizeof(clrVertices), 0, D3DFVF_C4CLRVERTEX, D3DPOOL_DEFAULT, &pVBClr, NULL) != D3D_OK ) return false;
	// fill initial data for vertex buffer
	for (i=0; i<6; ++i)
		{
		clrVertices[i].z=0.9f;
		clrVertices[i].rhw=1.0f;
		}
	// create color-texblit vertices
	if( lpDevice->CreateVertexBuffer(sizeof(bltClrVertices), 0, D3DFVF_C4CTVERTEX, D3DPOOL_DEFAULT, &pVBClrTex, NULL) != D3D_OK ) return false;
	// fill initial data for vertex buffer
	for (i=0; i<6; ++i)
		{
		bltClrVertices[i].z=0.9f;
		bltClrVertices[i].rhw=1.0f;
		}
	// create state blocks
	CreateStateBlock(&bltState[0], false, false, false, false, false);
	CreateStateBlock(&bltState[1], true, false, false, false, false);
	CreateStateBlock(&bltState[2], true, false, false, true, false);
	CreateStateBlock(&drawSolidState[0], false, false, false, false, false);
	CreateStateBlock(&drawSolidState[1], false, false, false, true, false);
	CreateStateBlock(&bltBaseState[0], true, false, true, false, false);
	CreateStateBlock(&bltBaseState[1], true, false, true, true, false);
	CreateStateBlock(&bltBaseState[2], true, false, true, false, true);
	CreateStateBlock(&bltBaseState[3], true, false, true, true, true);
	// activate if successful
	Active=fSuccess;
	// restore gamma if active
	if (Active) EnableGamma();
	// reset blit states
	dwBlitMode = 0;
	// done
	return Active;
	}

template<class IF> static inline void SafeRelease(IF *&pInterface)
	{
	if (pInterface)
		pInterface->Release();
	pInterface = 0;
	}

bool CStdD3D::InvalidateDeviceObjects()
{
	bool fSuccess=true;
	// clear gamma
	DisableGamma();
	// deactivate
	Active=false;
	// release state blocks
	SafeRelease(bltState[0]);
	SafeRelease(bltState[1]);
	SafeRelease(bltState[2]);
	SafeRelease(drawSolidState[0]);
	SafeRelease(bltBaseState[0]);
	SafeRelease(bltBaseState[1]);
	SafeRelease(drawSolidState[1]);
	SafeRelease(bltBaseState[2]);
	SafeRelease(bltBaseState[3]);
	SafeRelease(pSavedState);
	// release vertex buffer
	SafeRelease(pVBClr);
	SafeRelease(pVB);
	SafeRelease(pVBClrTex);
	// invalidate primary surfaces
	if (lpPrimary) lpPrimary->Clear();
	return fSuccess;
}

bool CStdD3D::DeleteDeviceObjects()
{
	InvalidateDeviceObjects();
	bool fSuccess=true;
	NoPrimaryClipper();
	// del main surfaces
	if (lpPrimary) delete lpPrimary;
	lpPrimary=lpBack=NULL;
	RenderTarget=NULL;
	for (int i=0; i<SHIDX_Size; ++i) if (pShaders[i]) { delete pShaders[i]; pShaders[i]=NULL; }
	return fSuccess;
}

bool CStdD3D::CreateStateBlock(IDirect3DStateBlock9 **pBlock, bool fTransparent, bool fSolid, bool fBaseTex, bool fAdditive, bool fMod2)
	{
	// settings
	if (!DDrawCfg.AdditiveBlts) fAdditive=false;
	// begin capturing
	lpDevice->BeginStateBlock();
	// set states
	lpDevice->SetRenderState( D3DRS_LIGHTING, false );
	lpDevice->SetRenderState( D3DRS_MULTISAMPLEANTIALIAS, false ); // no antialiasing
	lpDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, true );
	lpDevice->SetRenderState( D3DRS_SRCBLEND,   D3DBLEND_SRCALPHA );
	lpDevice->SetRenderState( D3DRS_DESTBLEND,  fAdditive ? D3DBLEND_ONE : D3DBLEND_INVSRCALPHA );
/*	// This isn't necessary, as a>=0 is equal to no alpha testing anyway
	lpDevice->SetRenderState( D3DRS_ALPHATESTENABLE,  true );
	lpDevice->SetRenderState( D3DRS_ALPHAREF,         0x00 );
	lpDevice->SetRenderState( D3DRS_ALPHAFUNC,  D3DCMP_GREATEREQUAL );
*/
	lpDevice->SetRenderState( D3DRS_FILLMODE,   D3DFILL_SOLID );
	lpDevice->SetRenderState( D3DRS_CULLMODE,   D3DCULL_NONE );
	lpDevice->SetRenderState( D3DRS_ZENABLE,          false );
	lpDevice->SetRenderState( D3DRS_STENCILENABLE,    false );
	lpDevice->SetRenderState( D3DRS_CLIPPING,         true );
	lpDevice->SetRenderState( D3DRS_ANTIALIASEDLINEENABLE,    false );
	lpDevice->SetRenderState( D3DRS_CLIPPLANEENABLE,  false );
	lpDevice->SetRenderState( D3DRS_VERTEXBLEND,      false );
	lpDevice->SetRenderState( D3DRS_INDEXEDVERTEXBLENDENABLE, false );
	lpDevice->SetRenderState( D3DRS_FOGENABLE,        false );
	if (!fBaseTex)
		{
		lpDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
		lpDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1 );
		lpDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, fSolid ? D3DTA_DIFFUSE : D3DTA_TEXTURE );
		lpDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, fSolid ? D3DTA_DIFFUSE : D3DTA_TEXTURE );
		}
	else
		{
		lpDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   fMod2 ? D3DTOP_ADDSIGNED2X : D3DTOP_MODULATE );
		lpDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE );
		lpDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		lpDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
		lpDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
		lpDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
		}
	lpDevice->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
	lpDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );
	lpDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, DDrawCfg.PointFiltering ? D3DTEXF_POINT : D3DTEXF_LINEAR );
	lpDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, DDrawCfg.PointFiltering ? D3DTEXF_POINT : D3DTEXF_LINEAR );
	lpDevice->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE );
	lpDevice->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 0 );
	lpDevice->SetTextureStageState( 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE );
	lpDevice->SetSamplerState( 0, D3DSAMP_ADDRESSU , D3DTADDRESS_CLAMP );
	lpDevice->SetSamplerState( 0, D3DSAMP_ADDRESSV , D3DTADDRESS_CLAMP );
	// capture
	return lpDevice->EndStateBlock(pBlock) == D3D_OK;
	}

void CStdD3D::SetTexture()
	{
	}

void CStdD3D::ResetTexture()
	{
	if (Active) lpDevice->SetTexture(0, NULL);
	}

bool CStdD3D::ApplyGammaRamp(D3DGAMMARAMP &ramp, bool fForce)
	{
	if (!lpDevice || (!Active && !fForce)) return false;
	lpDevice->SetGammaRamp(0, D3DSGR_CALIBRATE, &ramp);
	return true;
	}

bool CStdD3D::SaveDefaultGammaRamp(CStdWindow * pWindow)
	{
	if (!lpDevice) return false;
	lpDevice->GetGammaRamp(0, &DefRamp.ramp);
	return true;
	}

void CStdD3D::TaskOut()
	{
	InvalidateDeviceObjects();
	}

void CStdD3D::TaskIn()
	{
	RestoreDeviceObjects();
	}

CStdD3D *pD3D=NULL;

#endif // USE_DIRECTX

