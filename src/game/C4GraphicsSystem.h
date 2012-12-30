/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2008  Matthes Bender
 * Copyright (c) 2001, 2005, 2008  Sven Eberhardt
 * Copyright (c) 2005, 2009  Günther Brammer
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
	bool SaveScreenshot(bool fSaveAll);
	bool DoSaveScreenshot(bool fSaveAll, const char *szFilename);
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
