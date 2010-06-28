/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2002, 2005-2006  Matthes Bender
 * Copyright (c) 2005, 2007  Sven Eberhardt
 * Copyright (c) 2006  Günther Brammer
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

/* Captures uncompressed AVI from game view */

#include <C4Include.h>
#include <C4Video.h>

#include <C4Viewport.h>
#include <C4Config.h>
#include <C4Application.h>
#include <C4Game.h>
#include <C4Player.h>
#include <C4GraphicsResource.h>
#include <C4GraphicsSystem.h>
#include <C4PlayerList.h>

#ifdef _WIN32
#include <vfw.h>
#include <StdVideo.h>
#endif

C4Video::C4Video()
{
	Default();
}

C4Video::~C4Video()
{
	Clear();
}

void C4Video::Default()
{
	Active=false;
	pAviFile=NULL;
	pAviStream=NULL;
	AviFrame=0;
	AspectRatio=1.333;
	X=0; Y=0;
	Width=768; Height=576;
	Buffer=NULL;
	BufferSize=0;
	InfoSize=0;
	Recording=false;
	Surface=NULL;
	ShowFlash=0;
}

void C4Video::Clear()
{
	Stop();
}

void C4Video::Init(SURFACE sfcSource, int iWidth, int iHeight)
{
	// Activate
	Active=true;
	// Set source surface
	Surface=sfcSource;
	// Set video size
	Width=iWidth; Height=iHeight;
	AspectRatio = (double) iWidth / (double) iHeight;
}

bool C4Video::Enlarge()
{
	if (!Config.Graphics.VideoModule) return false;
	Resize(+16);
	return true;
}

bool C4Video::Reduce()
{
	if (!Config.Graphics.VideoModule) return false;
	Resize(-16);
	return true;
}

void C4Video::Execute() // Record frame, draw status
{
#ifdef _WIN32
	// Not active
	if (!Active) return;
	// Flash time
	if (ShowFlash) ShowFlash--;
	// Record
	if (Recording) RecordFrame();
	// Draw
	Draw();
#endif
}

bool C4Video::Toggle()
{
	if (!Config.Graphics.VideoModule) return false;
	if (!Recording) return(Start());
	return(Stop());
}

bool C4Video::Stop()
{
#ifdef _WIN32
	// Recording: close file
	if (Recording) AVICloseOutput(&pAviFile,&pAviStream);
	// Recording flag
	Recording = false;
	// Clear buffer
	if (Buffer) delete [] Buffer; Buffer=NULL;
#endif
	// Done
	return true;
}

bool C4Video::Start()
{
	// Determine new filename
	char szFilename[_MAX_PATH+1]; int iIndex=0;
	do { sprintf(szFilename,"Video%03d.avi",iIndex++); }
	while (ItemExists(szFilename));
	// Start recording
	return(Start(szFilename));
}

bool C4Video::Start(const char *szFilename)
{
#ifdef _WIN32
	// Stop any recording
	Stop();
	// Open output file
	if (!AVIOpenOutput(szFilename,&pAviFile,&pAviStream,Width,Height))
	{
		Log("AVIOpenOutput failure");
		AVICloseOutput(&pAviFile,&pAviStream);
		return false;
	}
	// Create video buffer
	AviFrame=0;
	InfoSize = sizeof(BITMAPINFOHEADER);
	if (Config.Graphics.BitDepth == 8)
		InfoSize += sizeof(RGBQUAD)*256;
	BufferSize = InfoSize + DWordAligned(Width)*Height * Config.Graphics.BitDepth/8;
	Buffer = new BYTE[BufferSize];
	// Set bitmap info
	BITMAPINFO *pInfo = (BITMAPINFO*) Buffer;
	ZeroMem((BYTE*)pInfo,sizeof(BITMAPINFOHEADER));
	pInfo->bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
	pInfo->bmiHeader.biPlanes=1;
	pInfo->bmiHeader.biWidth=Width;
	pInfo->bmiHeader.biHeight=Height;
	pInfo->bmiHeader.biBitCount=Config.Graphics.BitDepth;
	pInfo->bmiHeader.biCompression=0;
	pInfo->bmiHeader.biSizeImage = DWordAligned(Width)*Height * Config.Graphics.BitDepth/8;
	// Recording flag
	Recording=true;
#endif //_WIN32
	// Success
	return true;
}

void C4Video::Resize(int iChange)
{
	// Not while recording
	if (Recording) return;
	// Resize
	Width = BoundBy( Width+iChange, 56, 800 );
	Height = BoundBy( (int) ((double)Width/AspectRatio), 40, 600 );
	// Adjust position
	AdjustPosition();
	// Show flash
	ShowFlash=50;
}

void C4Video::Draw(C4TargetFacet &cgo)
{
	// Not active
	if (!Active) return;
	// No show
	if (!ShowFlash) return;
	// Draw frame
	Application.DDraw->DrawFrameDw(cgo.Surface, X+cgo.X,Y+cgo.Y,
	                             X+cgo.X+Width-1,Y+cgo.Y+Height-1,
	                             Recording ? C4RGB(0xca, 0, 0) : C4RGB(0xff, 0xff, 0xff));
	// Draw status
	StdStrBuf str;
	if (Recording) str.Format("%dx%d Frame %d",Width,Height,AviFrame);
	else str.Format("%dx%d",Width,Height);
	Application.DDraw->TextOut(str.getData(), ::GraphicsResource.FontRegular, 1.0,cgo.Surface,cgo.X+X+4,cgo.Y+Y+3,Recording ? 0xfaFF0000 : 0xfaFFFFFF);
}

bool C4Video::AdjustPosition()
{
	// Get source player & viewport
	C4Viewport *pViewport = ::GraphicsSystem.GetFirstViewport();
	if (!pViewport) return false;
	C4Player *pPlr = ::Players.Get(pViewport->GetPlayer());
	if (!pPlr) return false;
	// Set camera position
	X = int32_t(pPlr->ViewX - pViewport->ViewX + pViewport->DrawX - Width/2);
	X = BoundBy( X, 0, pViewport->ViewWdt - Width );
	Y = int32_t(pPlr->ViewY - pViewport->ViewY + pViewport->DrawY - Height/2);
	Y = BoundBy( Y, 0, pViewport->ViewHgt - Height );
	// Success
	return true;
}

#ifdef _WIN32
static void StdBlit(uint8_t *bypSource, int iSourcePitch, int iSrcBufHgt,
                    int iSrcX, int iSrcY, int iSrcWdt, int iSrcHgt,
                    uint8_t *bypTarget, int iTargetPitch, int iTrgBufHgt,
                    int iTrgX, int iTrgY, int iTrgWdt, int iTrgHgt,
                    int iBytesPerPixel=1, bool fFlip=false)
{
	if (!bypSource || !bypTarget) return;
	if (!iTrgWdt || !iTrgHgt) return;
	int xcnt,ycnt,zcnt,sline,tline,fy;
	for (ycnt=0; ycnt<iTrgHgt; ycnt++)
	{
		fy = iSrcHgt * ycnt / iTrgHgt;
		if (iSrcBufHgt>0) sline = ( iSrcBufHgt - 1 - iSrcY - fy ) * iSourcePitch;
		else sline = ( iSrcY + fy ) * iSourcePitch;
		if (iTrgBufHgt>0) tline = ( iTrgBufHgt - 1 - iTrgY - ycnt ) * iTargetPitch;
		else tline = ( iTrgY + ycnt ) * iTargetPitch;
		if (!fFlip)
		{
			for (xcnt=0; xcnt<iTrgWdt; xcnt++)
				for (zcnt=0; zcnt<iBytesPerPixel; zcnt++)
					bypTarget [ tline + (iTrgX + xcnt) * iBytesPerPixel + zcnt]
					= bypSource [ sline + (iSrcX + iSrcWdt * xcnt / iTrgWdt) * iBytesPerPixel + zcnt ];
		}
		else
		{
			for (xcnt=0; xcnt<iTrgWdt; xcnt++)
				for (zcnt=0; zcnt<iBytesPerPixel; zcnt++)
					bypTarget [ tline + (iTrgX + iTrgWdt - 1 -xcnt) * iBytesPerPixel + zcnt]
					= bypSource [ sline + (iSrcX + iSrcWdt * xcnt / iTrgWdt) * iBytesPerPixel + zcnt ];
		}
	}
}

bool C4Video::RecordFrame()
{
	// No buffer
	if (!Buffer) return false;
	// Lock source
	int iPitch,iWidth,iHeight;
	if (!Surface->Lock()) { Log("Video: lock surface failure"); Stop(); return false; }
	iPitch = Surface->PrimarySurfaceLockPitch;
	BYTE *bypBits = Surface->PrimarySurfaceLockBits;
	iWidth = Surface->Wdt; iHeight = Surface->Hgt;
	// Adjust source position
	if (!AdjustPosition()) { Log("Video: player/viewport failure"); Stop(); return false; }
	// Blit screen to buffer
	StdBlit((uint8_t*)bypBits,iPitch,-iHeight,
	        X,Y,Width,Height,
	        (uint8_t*)Buffer+InfoSize,
	        DWordAligned(Width) * (Config.Graphics.BitDepth/8),Height,
	        0,0,Width,Height,
	        Config.Graphics.BitDepth/8);
	// Unlock source
	Surface->Unlock();
	// Write frame to file
	if (!AVIPutFrame(pAviStream,
	                 AviFrame,
	                 Buffer,InfoSize,
	                 Buffer+InfoSize,BufferSize-InfoSize))
		{ Log("AVIPutFrame failure"); Stop(); return false; }
	// Advance frame
	AviFrame++;
	// Show flash
	ShowFlash=1;
	// Success
	return true;
}

void C4Video::Draw()
{
	// Not active
	if (!Active) return;
	// Get viewport
	C4Viewport *pViewport;
	if ( (pViewport = ::GraphicsSystem.GetFirstViewport()) )
	{
		C4TargetFacet cgo;
		cgo.Set(Surface,pViewport->DrawX,pViewport->DrawY,pViewport->ViewWdt,pViewport->ViewHgt,pViewport->ViewX,pViewport->ViewY);
		Draw(cgo);
	}
}
#endif //_WIN32
