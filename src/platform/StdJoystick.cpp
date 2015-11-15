/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2010-2013, The OpenClonk Team and contributors
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

/* Simple joystick handling with DirectInput 1 */

#include "C4Include.h"
#include <StdJoystick.h>

#include <C4windowswrapper.h>
#include <windowsx.h>

uint32_t POV2Position(DWORD dwPOV, bool fVertical)
{
	// POV value is a 360° angle multiplied by 100
	double dAxis;
	// Centered
	if (dwPOV == JOY_POVCENTERED)
		dAxis = 0.0;
	// Angle: convert to linear value -100 to +100
	else
		dAxis = (fVertical ? -cos((dwPOV/100) * M_PI / 180.0) : sin((dwPOV/100) * M_PI / 180.0)) * 100.0;
	// Gamepad configuration wants unsigned and gets 0 to 200
	return (uint32_t) (dAxis + 100.0);
}

CStdGamePad::CStdGamePad(int id) : id(id)
{
	ResetCalibration();
}

void CStdGamePad::ResetCalibration()
{
	// no calibration yet
	for (int i=0; i<CStdGamepad_MaxCalAxis; ++i)
	{
		fAxisCalibrated[i]=false;
	}
}

void CStdGamePad::SetCalibration(uint32_t *pdwAxisMin, uint32_t *pdwAxisMax, bool *pfAxisCalibrated)
{
	// params to calibration
	for (int i=0; i<CStdGamepad_MaxCalAxis; ++i)
	{
		dwAxisMin[i] = pdwAxisMin[i];
		dwAxisMax[i] = pdwAxisMax[i];
		fAxisCalibrated[i] = pfAxisCalibrated[i];
	}
}

void CStdGamePad::GetCalibration(uint32_t *pdwAxisMin, uint32_t *pdwAxisMax, bool *pfAxisCalibrated)
{
	// calibration to params
	for (int i=0; i<CStdGamepad_MaxCalAxis; ++i)
	{
		pdwAxisMin[i] = dwAxisMin[i];
		pdwAxisMax[i] = dwAxisMax[i];
		pfAxisCalibrated[i] = fAxisCalibrated[i];
	}
}

bool CStdGamePad::Update()
{
	joynfo.dwSize=sizeof(joynfo);
	joynfo.dwFlags=JOY_RETURNBUTTONS | JOY_RETURNRAWDATA | JOY_RETURNX | JOY_RETURNY | JOY_RETURNZ | JOY_RETURNR | JOY_RETURNU | JOY_RETURNV | JOY_RETURNPOV;
	return joyGetPosEx(JOYSTICKID1+id,&joynfo) == JOYERR_NOERROR;
}

uint32_t CStdGamePad::GetButtons()
{
	return joynfo.dwButtons;
}

CStdGamePad::AxisPos CStdGamePad::GetAxisPos(int idAxis, int32_t *out_strength)
{
	if (out_strength) *out_strength = 0; // default no strength
	if (idAxis<0 || idAxis>=CStdGamepad_MaxAxis) return Mid; // wrong axis
	// get raw axis data
	if (idAxis<CStdGamepad_MaxCalAxis)
	{
		uint32_t dwPos = (&joynfo.dwXpos)[idAxis];
		// evaluate axis calibration
		if (fAxisCalibrated[idAxis])
		{
			// update it
			dwAxisMin[idAxis] = std::min(dwAxisMin[idAxis], dwPos);
			dwAxisMax[idAxis] = std::max(dwAxisMax[idAxis], dwPos);
			// Calculate center
			DWORD dwCenter = (dwAxisMin[idAxis] + dwAxisMax[idAxis]) / 2;
			// Axis strength
			DWORD dwRange = (dwAxisMax[idAxis] - dwCenter);
			// Trigger range is 20% off center
			DWORD dwThresh = dwRange / 5;
			if (dwPos < dwCenter - dwThresh)
			{
				if (out_strength && dwRange) *out_strength = (dwCenter-dwPos)*100/dwRange;
				return Low;
			}
			if (dwPos > dwCenter + dwThresh)
			{
				if (out_strength && dwRange) *out_strength = (dwPos-dwCenter)*100/dwRange;
				return High;
			}
		}
		else
		{
			// init it
			dwAxisMin[idAxis] = dwAxisMax[idAxis] = dwPos;
			fAxisCalibrated[idAxis] = true;
		}
	}
	else
	{
		// It's a POV head
		DWORD dwPos = POV2Position(joynfo.dwPOV, idAxis==PAD_Axis_POVy);
		if (out_strength) *out_strength = Abs(int32_t(dwPos) - 100);
		if (dwPos > 130) return High; else if (dwPos < 70) return Low;
	}
	return Mid;
}

