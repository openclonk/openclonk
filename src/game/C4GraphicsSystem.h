/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2013, The OpenClonk Team and contributors
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

#include <C4FacetEx.h>
#include <C4MessageBoard.h>
#include <C4UpperBoard.h>
#include <C4Video.h>
#include <C4Shape.h>

class C4GraphicsSystem
{
public:
	C4GraphicsSystem();
	~C4GraphicsSystem();
	C4MessageBoard MessageBoard;
	C4UpperBoard UpperBoard;
	int32_t iRedrawBackground;
	bool ShowHelp;
	bool ShowVertices;
	bool ShowAction;
	bool ShowCommand;
	bool ShowEntrance;
	bool ShowPathfinder;
	bool ShowNetstatus;
	bool ShowSolidMask;
	C4Video Video;
	C4LoaderScreen *pLoaderScreen;
	void Default();
	void Clear();
	bool StartDrawing();
	void FinishDrawing();
	void Execute();
	void FlashMessage(const char *szMessage);
	void FlashMessageOnOff(const char *strWhat, bool fOn);
	void DeactivateDebugOutput();
	bool Init();
	bool InitLoaderScreen(const char *szLoaderSpec);
	void EnableLoaderDrawing(); // reset black screen loader flag
	bool SaveScreenshotKey(bool fSaveAll) { return SaveScreenshot(fSaveAll, 2.0f); } // keyboard callback for creating screenshot. create at default zoom.
	bool SaveScreenshot(bool fSaveAll, float fSaveAllZoom);
	bool DoSaveScreenshot(bool fSaveAll, const char *szFilename, float fSaveAllZoom);
	inline void InvalidateBg() { iRedrawBackground=2; }
	inline void OverwriteBg() { InvalidateBg(); }
protected:
	char FlashMessageText[C4MaxTitle+1];
	int32_t FlashMessageTime,FlashMessageX,FlashMessageY;
	void DrawHelp();
	void DrawFlashMessage();
	void DrawHoldMessages();
	void ClearFullscreenBackground();
	int32_t SeekLoaderScreens(C4Group &rFromGrp, const char *szWildcard, int32_t iLoaderCount, char *szDstName, C4Group **ppDestGrp);

public:
	bool ToggleShowSolidMask();
	bool ToggleShowNetStatus();
	bool ToggleShowVertices();
	bool ToggleShowAction();
	bool ToggleShowHelp();
	friend class C4FullScreen;
};

extern C4GraphicsSystem GraphicsSystem;
#endif
