/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2013, The OpenClonk Team and contributors
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

/* Some functions to help with loading and saving AVI files using Video for Windows */

#ifndef INC_STDVIDEO
#define INC_STDVIDEO

#ifdef _WIN32
#pragma once

#include <C4windowswrapper.h> // some vfw.h versions do not compile without this
#undef MK_ALT
#include <mmsystem.h>
#include <vfw.h>
#include <io.h>
#include "StdBuf.h"

bool AVIOpenGrab(const char *szFilename,
                 PAVISTREAM *ppAviStream,
                 PGETFRAME *ppGetFrame,
                 int &rAviLength, int &rFrameWdt, int &rFrameHgt,
                 int &rFrameBitsPerPixel, int &rFramePitch);

void AVICloseGrab(PAVISTREAM *ppAviStream,
                  PGETFRAME *ppGetFrame);

bool AVIOpenOutput(const char *szFilename,
                   PAVIFILE *ppAviFile,
                   PAVISTREAM *ppAviStream,
                   int iWidth, int iHeight);

bool AVICloseOutput(PAVIFILE *ppAviFile,
                    PAVISTREAM *ppAviStream);

bool AVIPutFrame(PAVISTREAM pAviStream,
                 long lFrame,
                 void *lpInfo, long lInfoSize,
                 void *lpData, long lDataSize);


// AVI file reading class
class CStdAVIFile
{
private:
	// file data
	StdStrBuf sFilename;
	PAVIFILE pAVIFile;

	// video data
	AVISTREAMINFO StreamInfo;
	PAVISTREAM pStream;
	PGETFRAME pGetFrame;

	// video processing helpers
	BITMAPINFO *pbmi;
	HDRAWDIB hOutDib;
	HBITMAP hBitmap;
	HDRAWDIB hDD;
	HDC hDC;
	BYTE *pFrameData;
	HWND hWnd;

	// audio data
	PAVISTREAM pAudioStream;
	BYTE *pAudioData;
	LONG iAudioDataLength, iAudioBufferLength;
	WAVEFORMAT *pAudioInfo;
	LONG iAudioInfoLength;

	// frame data
	int32_t iFinalFrame, iWdt, iHgt;
	time_t iTimePerFrame; // [ms/frame]

public:
	CStdAVIFile();
	~CStdAVIFile();

	void Clear();
	bool OpenFile(const char *szFilename, HWND hWnd, int32_t iOutBitDepth);

	int32_t GetWdt() const { return iWdt; }
	int32_t GetHgt() const { return iHgt; }

	// get desired frame from time offset to video start - return false if past video length
	bool GetFrameByTime(time_t iTime, int32_t *piFrame);

	// dump RGBa-data for specified frame
	bool GrabFrame(int32_t iFrame, class C4Surface *sfc) const;

	// getting audio data
	bool OpenAudioStream();
	BYTE *GetAudioStreamData(size_t *piStreamLength);
	void CloseAudioStream();
};
#endif //_WIN32

#endif //INC_STDVIDEO
