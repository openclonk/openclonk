/* Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de

Permission to use, copy, modify, and/or distribute this software for any
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2005  Günther Brammer
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

/* A mouse handling wrapper to DirectInput */

#ifdef USE_DIRECTX

#define DIRECTINPUT_VERSION 0x0700

#include <Standard.h>
#include <DInputX.h>

#include <windows.h>
#include <windowsx.h>

#include <stdarg.h>
#include <mmsystem.h>
#include <stdio.h>
#include <dinput.h>

// DIMOFS_X and co are defined in dinput.h with FIELD_OFFSET
// and gcc>3.2 does not like that in constant expressions.
#ifdef __GNUC__
#include <stddef.h>
#undef FIELD_OFFSET
#define FIELD_OFFSET(t,f) offsetof(t,f)
#endif


bool Log(const char *szMessage);

void HResultErrorString(long hresult, char *tstr)
  {
  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL,
                hresult,
                LANG_SYSTEM_DEFAULT,
                tstr,255,
                NULL);
  }

#define DINPUT_BUFFERSIZE 16

int DIRangeX1, DIRangeY1, DIRangeX2, DIRangeY2;
int MouseX, MouseY, MouseB0, MouseB1, MouseB2;
long MouseStatus=0;
int g_dxFuzz=0;
int g_dyFuzz=0;
int g_iSensitivity=0;

LPDIRECTINPUT g_pdi = NULL;
LPDIRECTINPUTDEVICE g_pMouse = NULL;
HANDLE g_hevtMouse = NULL;

void DirectInputSyncAcquire(BOOL fActive);

BOOL InitDirectInput(HINSTANCE g_hinst, HWND hwnd, int resx, int resy)
	{
  HRESULT hr;
  hr = DirectInputCreate(g_hinst, DIRECTINPUT_VERSION, /*(void **)*/ &g_pdi, NULL);
  if (FAILED(hr)) { Log("DirectInputCreate failure"); return FALSE; }

  hr = g_pdi->CreateDevice(GUID_SysMouse, &g_pMouse, NULL);
  if (FAILED(hr)) { Log("CreateDevice(SysMouse) failure"); return FALSE; }

  hr = g_pMouse->SetDataFormat(&c_dfDIMouse);
  if (FAILED(hr)) { Log("SetDataFormat(SysMouse, dfDIMouse) failure"); return FALSE; }

  hr = g_pMouse->SetCooperativeLevel(hwnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
  if (FAILED(hr)) { Log("SetCooperativeLevel(SysMouse) failure"); return FALSE; }

  g_hevtMouse = CreateEvent(0, 0, 0, 0);
  if (g_hevtMouse == NULL) { Log("CreateEvent failure"); return FALSE; }

  hr = g_pMouse->SetEventNotification(g_hevtMouse);
  if (FAILED(hr)) { Log("SetEventNotification(SysMouse)"); return FALSE; }

  DIPROPDWORD dipdw =
      {
				{ sizeof(DIPROPDWORD), sizeof(DIPROPHEADER), 0, DIPH_DEVICE, },
        DINPUT_BUFFERSIZE,
      };

  hr = g_pMouse->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph);
  if (FAILED(hr)) { Log("Set buffer size(SysMouse)"); return FALSE; }

  SetMouseRange(0,0,resx-1,resy-1);
  CenterMouse();
  MouseB0=MouseB1=MouseB2=0;

  DirectInputSyncAcquire(TRUE);

  return TRUE;
	}

void DeInitDirectInput()
	{
  if (g_pdi)      g_pdi   ->Release(), g_pdi    = NULL;
  if (g_pMouse)   g_pMouse->Release(), g_pMouse = NULL;
  if (g_hevtMouse) CloseHandle(g_hevtMouse), g_hevtMouse = NULL;
	}

void ClipMouse2Range()
  {
  if (MouseX < DIRangeX1) MouseX = DIRangeX1;
  if (MouseX > DIRangeX2) MouseX = DIRangeX2;
  if (MouseY < DIRangeY1) MouseY = DIRangeY1;
  if (MouseY > DIRangeY2) MouseY = DIRangeY2;
  }

void DIMouseUpdatePosition(int dx, int dy)
  {
  dx += g_dxFuzz; g_dxFuzz = 0;
  dy += g_dyFuzz; g_dyFuzz = 0;
  switch (g_iSensitivity)
    {
    case 1: dx *= 2; dy *= 2; break;
    case -1: g_dxFuzz = dx % 2; g_dyFuzz = dy % 2; dx /= 2; dy /= 2; break;
    }
  MouseX += dx; MouseY += dy;
  ClipMouse2Range();
  }

void SetMouseRange(int x1, int y1, int x2, int y2)
  {
  DIRangeX1=x1; DIRangeY1=y1;
  DIRangeX2=x2; DIRangeY2=y2;
  ClipMouse2Range();
  }

void CenterMouse()
  {
  MouseX=(DIRangeX2-DIRangeX1)/2;
  MouseY=(DIRangeY2-DIRangeY1)/2;
  ClipMouse2Range();
  }

void DirectInputSyncAcquire(BOOL fActive)
  {
  if (g_pMouse)
    if (fActive)
      {
			if (g_pMouse->Acquire()==DI_OK) Log("DirectInput: sync acquired");
			else Log("DirectInput: sync acquire failure");
			}
    else
      { g_pMouse->Unacquire(); Log("DirectInput: sync released"); }
  }


void DirectInputCritical()
  {
  // Test for mouse input
  if (WaitForSingleObject(g_hevtMouse,0)!=WAIT_OBJECT_0) return;
  // Retrieve mouse input
  while (TRUE)
    {
    DIDEVICEOBJECTDATA od;
    DWORD dwElements = 1;
    HRESULT hr = g_pMouse->GetDeviceData( sizeof(DIDEVICEOBJECTDATA), &od, &dwElements, 0);
    MouseStatus=hr;
    if (hr == DIERR_INPUTLOST)
      { DirectInputSyncAcquire(TRUE); return; }
    // Unable to read data or no data available
    if (FAILED(hr) || dwElements == 0) return;
    switch (od.dwOfs)
      {
      // Mouse horizontal motion
      case DIMOFS_X: DIMouseUpdatePosition(od.dwData, 0); break;
      // Mouse vertical motion
      case DIMOFS_Y: DIMouseUpdatePosition(0, od.dwData); break;
      // Button 0 pressed or released
      case DIMOFS_BUTTON0: MouseB0 = (od.dwData & 0x80); break;
      // Button 1 pressed or released
      case DIMOFS_BUTTON1: MouseB1 = (od.dwData & 0x80); break;
      // Button 1 pressed or released
      case DIMOFS_BUTTON2: MouseB2 = (od.dwData & 0x80); break;
      }
    }
  }

#endif //USE_DIRECTX
