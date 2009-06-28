/* Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de

Permission to use, copy, modify, and/or distribute this software for any
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2002  Sven Eberhardt
 * Copyright (c) 2004  Günther Brammer
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

"Clonk" is a registered trademark of Matthes Bender. */

/* A wrapper to DirectSound - derived from DXSDK samples */
#ifdef USE_DIRECTX
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <dsound.h>

#include <DSoundX.h>

LPDIRECTSOUND lpDS = NULL;

BOOL InitDirectSound(HWND hwnd)
  {
  if (!SUCCEEDED(DirectSoundCreate(NULL, &lpDS, NULL)))
    return FALSE;
  if (!SUCCEEDED(lpDS->SetCooperativeLevel(hwnd,DSSCL_NORMAL)))
    return FALSE;
  return TRUE;
  }

void DeInitDirectSound()
  {
  if (lpDS)
    {
    lpDS->Release();
    lpDS = NULL;
    }
  }

struct CSoundObject
  {
  BYTE *pbWaveData;               // pointer into wave resource (for restore)
  DWORD cbWaveSize;               // size of wave data (for restore)
  int iAlloc;                     // number of buffers.
  int iCurrent;                   // current buffer
  IDirectSoundBuffer* Buffers[1]; // list of buffers
  };

BOOL DSGetWaveResource(BYTE *pvRes, WAVEFORMATEX **ppWaveHeader, BYTE **ppbWaveData,DWORD *pcbWaveSize)
	{
  DWORD *pdw;
  DWORD *pdwEnd;
  DWORD dwRiff;
  DWORD dwType;
  DWORD dwLength;

  if (ppWaveHeader) *ppWaveHeader = NULL;
  if (ppbWaveData)  *ppbWaveData = NULL;
  if (pcbWaveSize)  *pcbWaveSize = 0;

  pdw = (DWORD *)pvRes;
  dwRiff = *pdw++;
  dwLength = *pdw++;
  dwType = *pdw++;

  if (dwRiff != mmioFOURCC('R', 'I', 'F', 'F')) goto exit;
  if (dwType != mmioFOURCC('W', 'A', 'V', 'E')) goto exit;
  pdwEnd = (DWORD *)((BYTE *)pdw + dwLength-4);

  while (pdw < pdwEnd)
		{
    dwType = *pdw++;
    dwLength = *pdw++;
    switch (dwType)
			{
			case mmioFOURCC('f', 'm', 't', ' '):
        if (ppWaveHeader && !*ppWaveHeader)
					{
          if (dwLength < sizeof(WAVEFORMAT)) goto exit;
          *ppWaveHeader = (WAVEFORMATEX *)pdw;
          if ((!ppbWaveData || *ppbWaveData) && (!pcbWaveSize || *pcbWaveSize))
            { return TRUE; }
					}
        break;
			case mmioFOURCC('d', 'a', 't', 'a'):
        if ((ppbWaveData && !*ppbWaveData) || (pcbWaveSize && !*pcbWaveSize))
					{
          if (ppbWaveData) *ppbWaveData = (LPBYTE)pdw;
          if (pcbWaveSize) *pcbWaveSize = dwLength;
          if (!ppWaveHeader || *ppWaveHeader) return TRUE;
					}
        break;
      }
    pdw = (DWORD *)((BYTE *)pdw + ((dwLength+1)&~1));
		}

exit:
  return FALSE;
	}

BOOL DSFillSoundBuffer(IDirectSoundBuffer *pDSB, BYTE *pbWaveData, DWORD cbWaveSize)
	{
  if (pDSB && pbWaveData && cbWaveSize)
    {
    LPVOID pMem1, pMem2;
    DWORD dwSize1, dwSize2;
    if (SUCCEEDED(pDSB->Lock( 0, cbWaveSize, &pMem1, &dwSize1, &pMem2, &dwSize2, 0)))
      {
      CopyMemory(pMem1, pbWaveData, dwSize1);
      if ( 0 != dwSize2 ) CopyMemory(pMem2, pbWaveData+dwSize1, dwSize2);
      pDSB->Unlock( pMem1, dwSize1, pMem2, dwSize2);
      return TRUE;
      }
    }
  return FALSE;
	}

IDirectSoundBuffer *DSLoadSoundBuffer(IDirectSound *pDS, BYTE *bpWaveBuf)
	{
  IDirectSoundBuffer *pDSB = NULL;
  DSBUFFERDESC dsBD = {0};
  BYTE *pbWaveData;
  if (DSGetWaveResource(bpWaveBuf, &dsBD.lpwfxFormat, &pbWaveData, &dsBD.dwBufferBytes ))
    {
    dsBD.dwSize = sizeof(dsBD);
    dsBD.dwFlags = DSBCAPS_STATIC | DSBCAPS_CTRLVOLUME | DSBCAPS_GETCURRENTPOSITION2;
    if (SUCCEEDED(pDS->CreateSoundBuffer(&dsBD, &pDSB, NULL)))
      {
      if (!DSFillSoundBuffer(pDSB, pbWaveData, dsBD.dwBufferBytes))
        { pDSB->Release(); pDSB = NULL; }
      }
    else
      {
      pDSB = NULL;
      }
    }
  return pDSB;
	}

CSoundObject *DSndObjCreate(BYTE *bpWaveBuf, int iConcurrent)
	{
  CSoundObject *pSO = NULL;
  LPWAVEFORMATEX pWaveHeader;
  BYTE *pbData;
  DWORD cbData;

  IDirectSound *pDS = lpDS;

  if (DSGetWaveResource(bpWaveBuf, &pWaveHeader, &pbData, &cbData))
    {
		// Minimum one concurrent buffer
    if (iConcurrent < 1) iConcurrent = 1;

		// Allocate sound object + buffer space
    if ((pSO = (CSoundObject *)LocalAlloc(LPTR, sizeof(CSoundObject) +
            (iConcurrent-1) * sizeof(IDirectSoundBuffer *))) != NULL)
      {
      int i;
      pSO->iAlloc = iConcurrent;
      pSO->pbWaveData = pbData;
      pSO->cbWaveSize = cbData;
      pSO->Buffers[0] = DSLoadSoundBuffer(pDS, bpWaveBuf);
			if (!pSO->Buffers[0])
				{
				DSndObjDestroy(pSO); return NULL;
				}
			// Load buffers
      for (i=1; i<pSO->iAlloc; i++)
				{
        if (FAILED(pDS->DuplicateSoundBuffer(pSO->Buffers[0], &pSO->Buffers[i])))
          {
          pSO->Buffers[i] = DSLoadSoundBuffer(pDS, bpWaveBuf);
          if (!pSO->Buffers[i]) { DSndObjDestroy(pSO); pSO = NULL; break; }
          }
				}
      }

    }

	// Return sound object
  return pSO;
	}

void DSndObjDestroy(CSoundObject *pSO)
  {
  if (!pSO) return;

  int i;
  for (i=0; i<pSO->iAlloc; i++)
    {
    if (pSO->Buffers[i])
      {
      pSO->Buffers[i]->Release();
      pSO->Buffers[i] = NULL;
      }
    }

  LocalFree((HANDLE)pSO);
  }

IDirectSoundBuffer *DSndObjGetFreeBuffer(CSoundObject *pSO)
	{
  IDirectSoundBuffer *pDSB;

  if (pSO == NULL) return NULL;

	// Check current buffer
  if (pDSB = pSO->Buffers[pSO->iCurrent])
		{
    HRESULT hres;
    DWORD dwStatus;

    hres = pDSB->GetStatus(&dwStatus);
    if (FAILED(hres)) dwStatus = 0;

		// Buffer is playing
    if ((dwStatus & DSBSTATUS_PLAYING) == DSBSTATUS_PLAYING)
      {
			// More buffers available, try next one
      if (pSO->iAlloc > 1)
        {
        if (++pSO->iCurrent >= pSO->iAlloc) pSO->iCurrent = 0;
        pDSB = pSO->Buffers[pSO->iCurrent];
        hres = pDSB->GetStatus(&dwStatus);
				// Next buffer is non-playing, use this one
        if (SUCCEEDED(hres) && (dwStatus & DSBSTATUS_PLAYING) == DSBSTATUS_PLAYING)
	        {
          pDSB->Stop();
          pDSB->SetCurrentPosition(0);
		      }
        }
			// Only one buffer available
      else
        {
        pDSB = NULL;
        }
      }

		// Buffer was lost
    if (pDSB && (dwStatus & DSBSTATUS_BUFFERLOST))
      {
			// Try restore
      if (FAILED(pDSB->Restore()) || !DSFillSoundBuffer(pDSB, pSO->pbWaveData, pSO->cbWaveSize))
        {
				// Restore failed
        pDSB = NULL;
        }
      }

		}

	// Return current, non-playing buffer, or NULL if failure
	return pDSB;
	}

BOOL DSndObjPlay(CSoundObject *pSO, DWORD dwPlayFlags)
  {
  BOOL result = FALSE;

  if (!pSO)  return FALSE;

  if ( !(dwPlayFlags & DSBPLAY_LOOPING) || (pSO->iAlloc==1) )
    {
		// Get free buffer, play that
    IDirectSoundBuffer *pDSB = DSndObjGetFreeBuffer(pSO);
    if (pDSB)
      result = SUCCEEDED(pDSB->Play(0, 0, dwPlayFlags));
    }

  return result;
  }

BOOL DSndObjPlaying(CSoundObject *pSO)
  {
  BOOL result,fPlaying=FALSE;
  DWORD dwStatus;
  int i;
  if (pSO)
    for (i=0; i<pSO->iAlloc; i++)
      {
      result=pSO->Buffers[i]->GetStatus(&dwStatus);
      if (FAILED(result)) dwStatus=0;
      if ((dwStatus & DSBSTATUS_PLAYING) == DSBSTATUS_PLAYING)
        fPlaying=TRUE;
      }
  return fPlaying;
  }

BOOL DSndObjSetVolume(CSoundObject *pSO, long lVolume)
  {
  BOOL result,fSuccess=TRUE;
  if (pSO)
    for (int i=0; i<pSO->iAlloc; i++)
      {
      result=pSO->Buffers[i]->SetVolume(lVolume);
      if (FAILED(result)) fSuccess=FALSE;
      }
  return fSuccess;
  }

BOOL DSndObjGetVolume(CSoundObject *pSO, long *lpVolume)
  {
  BOOL result,fSuccess=TRUE;
  if (pSO)
    {
    result=pSO->Buffers[0]->GetVolume(lpVolume);
    if (FAILED(result)) fSuccess=FALSE;
    }
  return fSuccess;
  }

BOOL DSndObjStop(CSoundObject *pSO)
  {
  int i;
  if (!pSO) return FALSE;
  for (i=0; i<pSO->iAlloc; i++)
    {
    pSO->Buffers[i]->Stop();
    pSO->Buffers[i]->SetCurrentPosition(0);
    }
  return TRUE;
  }

#endif //USE_DIRECTX
