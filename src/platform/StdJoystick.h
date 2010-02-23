/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
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

/* Simple joystick handling with DirectInput 1 */

#ifndef INC_StdJoystick
#define INC_StdJoystick

#include <Standard.h>
#include <windows.h>
#include <mmsystem.h>

const int32_t PAD_Axis_POVx = 6;
const int32_t PAD_Axis_POVy = 7; // virtual axises of the coolie hat

const int CStdGamepad_MaxGamePad = 15, // maximum number of supported gamepads
					CStdGamepad_MaxCalAxis = 6,  // maximum number of calibrated axises
					CStdGamepad_MaxAxis = 8;     // number of axises plus coolie hat axises

class CStdGamePad
	{
	public:
		enum AxisPos { Low, Mid, High, }; // quantized axis positions
	private:
		int id; // gamepad number
		JOYINFOEX joynfo; // WIN32 gamepad info

	public:
		uint32_t dwAxisMin[CStdGamepad_MaxCalAxis], dwAxisMax[CStdGamepad_MaxCalAxis]; // axis ranges - auto calibrated
		bool fAxisCalibrated[CStdGamepad_MaxCalAxis]; // set if an initial value for axis borders has been determined already

		CStdGamePad(int id); // ctor

		void ResetCalibration(); // resets axis min and max
		void SetCalibration(uint32_t *pdwAxisMin, uint32_t *pdwAxisMax, bool *pfAxisCalibrated);
		void GetCalibration(uint32_t *pdwAxisMin, uint32_t *pdwAxisMax, bool *pfAxisCalibrated);

		bool Update(); // read current gamepad data
		uint32_t GetButtons(); // returns bitmask of pressed buttons for last retrieved info
		AxisPos GetAxisPos(int idAxis, int32_t *out_strength=NULL); // return axis extension - mid for error or center position
	};

#endif
