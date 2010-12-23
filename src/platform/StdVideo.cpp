/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2007  Matthes Bender
 * Copyright (c) 2005, 2009  Günther Brammer
 * Copyright (c) 2006  Sven Eberhardt
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

/* Some functions to help with saving AVI files using Video for Windows */

#include "C4Include.h"
#ifdef _WIN32

#include <StdVideo.h>
#include <StdSurface2.h>

#include <windows.h>

bool AVIOpenOutput(const char *szFilename,
                   PAVIFILE *ppAviFile,
                   PAVISTREAM *ppAviStream,
                   int iWidth, int iHeight)
{

	// Init AVI system
	AVIFileInit();

	// Create avi file
	if ( AVIFileOpen(
	       ppAviFile,
	       szFilename,
	       OF_CREATE | OF_WRITE,
	       NULL) != 0)
	{
		return false;
	}

	// Create stream
	AVISTREAMINFO avi_info;
	RECT frame; frame.left=0; frame.top=0; frame.right=iWidth; frame.bottom=iHeight;
	avi_info.fccType= streamtypeVIDEO;
	avi_info.fccHandler= mmioFOURCC('M','S','V','C');
	avi_info.dwFlags= 0;
	avi_info.dwCaps= 0;
	avi_info.wPriority= 0;
	avi_info.wLanguage= 0;
	avi_info.dwScale= 1;
	avi_info.dwRate= 35;
	avi_info.dwStart= 0;
	avi_info.dwLength= 10; // ??
	avi_info.dwInitialFrames= 0;
	avi_info.dwSuggestedBufferSize= 0;
	avi_info.dwQuality= -1;
	avi_info.dwSampleSize= 0;
	avi_info.rcFrame= frame;
	avi_info.dwEditCount= 0;
	avi_info.dwFormatChangeCount= 0;
	SCopy("MyRecording",avi_info.szName);

	if ( AVIFileCreateStream(
	       *ppAviFile,
	       ppAviStream,
	       &avi_info) != 0)
	{
		return false;
	}

	return true;
}


bool AVICloseOutput(PAVIFILE *ppAviFile,
                    PAVISTREAM *ppAviStream)
{
	if (ppAviStream && *ppAviStream)
		{ AVIStreamRelease(*ppAviStream); *ppAviStream=NULL; }
	if (ppAviFile && *ppAviFile)
		{ AVIFileRelease(*ppAviFile); *ppAviFile=NULL; }
	return true;
}


bool AVIPutFrame(PAVISTREAM pAviStream,
                 long lFrame,
                 void *lpInfo, long lInfoSize,
                 void *lpData, long lDataSize)
{
	long lBytesWritten=0,lSamplesWritten=0;

	AVIStreamSetFormat(
	  pAviStream,
	  lFrame,
	  lpInfo,
	  lInfoSize
	);

	if (AVIStreamWrite(
	      pAviStream,
	      lFrame,
	      1,
	      lpData,
	      lDataSize,
	      AVIIF_KEYFRAME,
	      &lSamplesWritten,
	      &lBytesWritten) != 0) return false;

	return true;
}


bool AVIOpenGrab(const char *szFilename,
                 PAVISTREAM *ppAviStream,
                 PGETFRAME *ppGetFrame,
                 int &rAviLength, int &rFrameWdt, int &rFrameHgt,
                 int &rFrameBitsPerPixel, int &rFramePitch)
{

	// Open avi stream
	if ( AVIStreamOpenFromFile(
	       ppAviStream,
	       szFilename,
	       streamtypeVIDEO,
	       0,
	       OF_READ,
	       NULL) != 0) return false;

	// Get stream info
	AVISTREAMINFO avi_info;
	AVIStreamInfo(*ppAviStream,&avi_info,sizeof(avi_info));
	rAviLength=avi_info.dwLength;

	// Open get frame
	if (!(*ppGetFrame = AVIStreamGetFrameOpen(*ppAviStream,NULL))) return false;

	// Get sample frame
	void *pframe;
	if (!(pframe = AVIStreamGetFrame(*ppGetFrame,0))) return false;

	// Assign sample bmp info
	BITMAPINFOHEADER *sample = (BITMAPINFOHEADER*) pframe;
	rFrameWdt = sample->biWidth;
	rFrameHgt = sample->biHeight;
	rFrameBitsPerPixel = sample->biBitCount;
	rFramePitch = DWordAligned(rFrameWdt*rFrameBitsPerPixel/8);

	return true;
}

void AVICloseGrab(PAVISTREAM *ppAviStream,
                  PGETFRAME *ppGetFrame)
{
	if (ppGetFrame && *ppGetFrame)
		{ AVIStreamGetFrameClose(*ppGetFrame); *ppGetFrame=NULL; }
	if (ppAviStream && *ppAviStream)
		{ AVIStreamRelease(*ppAviStream); *ppAviStream=NULL; }
}


// ----------------------------------------

CStdAVIFile::CStdAVIFile()
		: pAVIFile(NULL), pStream(NULL), pGetFrame(NULL), pbmi(NULL), hOutDib(NULL), hBitmap(NULL), hDD(NULL), hDC(NULL), hWnd(NULL),
		pAudioStream(NULL), pAudioData(NULL), iAudioBufferLength(0), pAudioInfo(NULL)
{
	AVIFileInit();
}

CStdAVIFile::~CStdAVIFile()
{
	Clear();
	AVIFileExit();
}

void CStdAVIFile::Clear()
{
	// free any stuff
	CloseAudioStream();
	if (hBitmap) { DeleteObject(hBitmap); hBitmap = NULL; }
	if (hDD) { DrawDibClose(hDD); hDD = NULL; }
	if (hDC) { ReleaseDC(hWnd, hDC); hDC = NULL; }
	if (pGetFrame) { AVIStreamGetFrameClose(pGetFrame); pGetFrame=NULL; }
	if (pStream) { AVIStreamRelease(pStream); pStream = NULL; }
	if (pbmi) { delete [] pbmi; pbmi = NULL; }
	if (pAVIFile) { AVIFileRelease(pAVIFile); pAVIFile = NULL; }
	sFilename.Clear();
}

bool CStdAVIFile::OpenFile(const char *szFilename, HWND hWnd, int32_t iOutBitDepth)
{
	// clear previous
	Clear();
	sFilename.Copy(szFilename);
	// open the AVI file
	if (AVIFileOpen(&pAVIFile, szFilename, OF_READ, NULL))
		return false;
	if (AVIFileGetStream(pAVIFile, &pStream, streamtypeVIDEO, 0))
		return false;
	// get stream information
	AVIStreamInfo(pStream, &StreamInfo, sizeof(AVISTREAMINFO));
	iWdt = StreamInfo.rcFrame.right-StreamInfo.rcFrame.left;
	iHgt = StreamInfo.rcFrame.bottom-StreamInfo.rcFrame.top;
	iFinalFrame = AVIStreamLength(pStream);
	// some safety
	if (iWdt<=0 || iHgt<=0 || iFinalFrame<=0) return false;
	// calculate playback speed
	iTimePerFrame = AVIStreamSampleToTime(pStream,iFinalFrame)/iFinalFrame;
	// init buffer bitmap info
	pbmi = (BITMAPINFO *) new BYTE[sizeof (BITMAPINFO) + 3*sizeof(RGBQUAD)];
	ZeroMemory(pbmi, sizeof (BITMAPINFO) + 3*sizeof(RGBQUAD));
	pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pbmi->bmiHeader.biPlanes = 1;
	pbmi->bmiHeader.biBitCount = iOutBitDepth;
	pbmi->bmiHeader.biWidth = iWdt;
	pbmi->bmiHeader.biHeight = -iHgt;
	pbmi->bmiHeader.biCompression = (iOutBitDepth == 16) ? BI_BITFIELDS : BI_RGB;
	if (iOutBitDepth == 16)
	{
		*(DWORD*)(&(pbmi->bmiColors[2])) = 0x00f;
		*(DWORD*)(&(pbmi->bmiColors[1])) = 0x0f0;
		*(DWORD*)(&(pbmi->bmiColors[0])) = 0xf00;
	}
	hDC = CreateCompatibleDC(NULL);
	if (!hDC) return false;
	hDD = DrawDibOpen();
	if (!hDD) return false;
	hBitmap = CreateDIBSection(hDC, pbmi, DIB_RGB_COLORS, (void**)(&pFrameData), NULL, 0);
	if (!hBitmap) return false;
	SelectObject(hDC, hBitmap);
	// create a GetFrame-object
	pGetFrame=AVIStreamGetFrameOpen(pStream, NULL /*&(pbmi->bmiHeader)*/);
	if (!pGetFrame) return false;
	// done, success!
	return true;
}

bool CStdAVIFile::GetFrameByTime(time_t iTime, int32_t *piFrame)
{
	// safeties
	if (iTime < 0) return false;
	if (!piFrame || !iTimePerFrame) return false;
	// get frame
	int iFrame = *piFrame = int32_t((iTime + (iTimePerFrame/2)) / iTimePerFrame);
	return iFrame < iFinalFrame;
}


bool CStdAVIFile::GrabFrame(int32_t iFrame, CSurface *sfc) const
{
	// safeties
	if (!pGetFrame || !sfc) return false;
	if (iFrame<0 || iFrame >= iFinalFrame) return false;
	// grab desired frame
	LPBITMAPINFOHEADER lpbi = (LPBITMAPINFOHEADER)AVIStreamGetFrame(pGetFrame, iFrame);
	// calculate actual data position
	BYTE *pImageData = (BYTE *)lpbi + lpbi->biSize + lpbi->biClrUsed*sizeof(RGBQUAD);
	// draw into buffer bitmap
	if (!DrawDibDraw (hDD, hDC, 0, 0, iWdt, iHgt, lpbi, pImageData, 0, 0, iWdt, iHgt, 0)) return false;
	// copy from buffer bitmap into surface - assumes surface is created in the correct size!
	if (!sfc->Lock()) return false;
	if (!sfc->CopyBytes(pFrameData)) return false;
	return !!sfc->Unlock();
}


bool CStdAVIFile::OpenAudioStream()
{
	// close previous
	CloseAudioStream();
	// open new
	if (!pAVIFile) return false;
	if (AVIFileGetStream(pAVIFile, &pAudioStream, streamtypeAUDIO, 0))
		return false;
	// get audio stream format
	if (AVIStreamReadFormat(pAudioStream, AVIStreamStart(pAudioStream), NULL, &iAudioInfoLength)) return false;
	if (iAudioInfoLength<sizeof(WAVEFORMAT)) return false;
	pAudioInfo = (WAVEFORMAT *) new BYTE[iAudioInfoLength];
	if (AVIStreamReadFormat(pAudioStream, AVIStreamStart(pAudioStream), pAudioInfo, &iAudioInfoLength))
		{ delete [] pAudioInfo; pAudioInfo=NULL; return false; }
	// done!
	return true;
}

BYTE *CStdAVIFile::GetAudioStreamData(size_t *piStreamLength)
{
	// returning the complete audio stream at once here - not very efficient, but easy...
	// get stream size
	if (!pAudioInfo) return NULL;
	if (AVIStreamRead(pAudioStream, 0, AVIStreamLength(pAudioStream), NULL, 0, &iAudioDataLength, NULL)) return NULL;
	if (iAudioDataLength<=0) return NULL;
	// make sure current audio data buffer is large enoiugh to hold the data
	// preceding return data with the RIFF+waveformat structure here, so it can be easily loaded by fmod
	uint32_t iHeaderLength = iAudioInfoLength + sizeof(FOURCC) * 4 + 3 * sizeof(uint32_t);
	LONG iReturnDataLength = iAudioDataLength + iHeaderLength;
	if (iAudioBufferLength < iReturnDataLength)
	{
		delete [] pAudioData;
		pAudioData = new BYTE[iAudioBufferLength = iReturnDataLength];
		// build wave file header
		BYTE *pWrite = pAudioData;
		*((FOURCC *)pWrite) = mmioFOURCC('R', 'I', 'F', 'F'); pWrite += sizeof(FOURCC);
		*((uint32_t *)pWrite) = iReturnDataLength - sizeof(FOURCC) - sizeof(uint32_t); pWrite += sizeof(uint32_t);
		*((FOURCC *)pWrite) = mmioFOURCC('W', 'A', 'V', 'E'); pWrite += sizeof(FOURCC);
		*((FOURCC *)pWrite) = mmioFOURCC('f', 'm', 't', ' '); pWrite += sizeof(FOURCC);
		*((uint32_t *)pWrite) = iAudioInfoLength; pWrite += sizeof(uint32_t);
		memcpy(pWrite, pAudioInfo, iAudioInfoLength); pWrite += iAudioInfoLength;
		*((FOURCC *)pWrite) = mmioFOURCC('d', 'a', 't', 'a'); pWrite += sizeof(FOURCC);
		*((uint32_t *)pWrite) = iAudioDataLength;
	}
	// get it
	if (AVIStreamRead(pAudioStream, 0, AVIStreamLength(pAudioStream), pAudioData+iHeaderLength, iAudioDataLength, NULL, NULL)) return NULL;
	// got the data successfully!
	*piStreamLength = iReturnDataLength;
	return pAudioData;
}

void CStdAVIFile::CloseAudioStream()
{
	if (pAudioStream) { AVIStreamRelease(pAudioStream); pAudioStream = NULL; }
	if (pAudioData) { delete [] pAudioData; pAudioData = NULL; }
	if (pAudioInfo) { delete [] pAudioInfo; pAudioInfo = NULL; }
	iAudioBufferLength = 0;
}

#endif // _WIN32
