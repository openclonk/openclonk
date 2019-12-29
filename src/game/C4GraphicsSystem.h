/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2016, The OpenClonk Team and contributors
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

/* Operates viewports, message board and draws the game */

#ifndef INC_C4GraphicsSystem
#define INC_C4GraphicsSystem

#include "gui/C4MessageBoard.h"
#include "gui/C4UpperBoard.h"

class C4GraphicsSystem
{
public:
	C4GraphicsSystem();
	~C4GraphicsSystem();
	std::unique_ptr<C4MessageBoard> MessageBoard;
	C4UpperBoard UpperBoard;
	int32_t iRedrawBackground;
	bool ShowHelp;
	bool ShowVertices;
	bool ShowAction;
	bool ShowCommand;
	bool ShowEntrance;
	bool ShowPathfinder;
	bool ShowNetstatus;
	int Show8BitSurface; // 0 normal, 1 foreground mats, 2 background mats
	bool ShowLights;
	bool ShowMenuInfo;
	C4LoaderScreen *pLoaderScreen;
	void Default();
	void Clear();
	bool StartDrawing();
	void FinishDrawing();
	void Execute();
	void FlashMessage(const char *message);
	void FlashMessageOnOff(const char *description, bool switch_on);
	void DeactivateDebugOutput();
	bool Init();
	bool InitLoaderScreen(const char *image_name);
	void EnableLoaderDrawing(); // reset black screen loader flag
	bool SaveScreenshotKey(bool save_all) { return SaveScreenshot(save_all, 2.0f); } // keyboard callback for creating screenshot. create at default zoom.
	bool SaveScreenshot(bool save_all, float zoom_factor_all);
	bool DoSaveScreenshot(bool save_all, const char *filename, float zoom_factor_all);
	inline void InvalidateBg() { iRedrawBackground=2; }
	inline void OverwriteBg() { InvalidateBg(); }

private:
	char FlashMessageText[C4MaxTitle+1];
	int32_t FlashMessageTime;
	int32_t FlashMessageX;
	int32_t FlashMessageY;
	void DrawHelp();
	void DrawFlashMessage();
	void DrawHoldMessages();
	void ClearFullscreenBackground();

	C4TimeMilliseconds lastFrame;

public:
	bool ToggleShow8BitSurface();
	bool ToggleShowNetStatus();
	bool ToggleShowVertices();
	bool ToggleShowAction();
	bool ToggleShowHelp();
	friend class C4FullScreen;
};

extern C4GraphicsSystem GraphicsSystem;
#endif
