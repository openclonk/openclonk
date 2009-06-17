/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2008  Matthes Bender
 * Copyright (c) 2001, 2005, 2008  Sven Eberhardt
 * Copyright (c) 2005  Günther Brammer
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

class C4Game;
class C4LoaderScreen;
extern C4Game Game;

const float C4GFX_ZoomStep = 1.1040895f;

class C4GraphicsSystem
	{
	public:
		C4GraphicsSystem();
		~C4GraphicsSystem();
		C4MessageBoard MessageBoard;
		C4UpperBoard UpperBoard;
		int32_t iRedrawBackground;
		BOOL ShowHelp;
		BOOL ShowVertices;
		BOOL ShowAction;
		BOOL ShowCommand;
		BOOL ShowEntrance;
		BOOL ShowPathfinder;
		BOOL ShowNetstatus;
		BOOL ShowSolidMask;
		BYTE DarkColorTable[256];
		BOOL fSetPalette;
		uint32_t dwGamma[C4MaxGammaRamps*3];		// gamma ramps
		BOOL fSetGamma;			// must gamma ramp be reassigned?
		C4Video Video;
		C4LoaderScreen *pLoaderScreen;
		void Default();
		void Clear();
		bool StartDrawing();
		void FinishDrawing();
		void Execute();
		void FlashMessage(const char *szMessage);
		void FlashMessageOnOff(const char *strWhat, bool fOn);
		void SetDarkColorTable();
		void DeactivateDebugOutput();
		void MouseMove(int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam, class C4Viewport *pVP); // pVP specified for console mode viewports only
		void SetMouseInGUI(bool fInGUI, bool fByMouse);
		void SortViewportsByPlayerControl();
		void ClearPointers(C4Object *pObj);
		//void UpdateMouse(HWND hwnd, int32_t iX, int32_t iY, int32_t iB);
		void RecalculateViewports();
		BOOL Init();
		BOOL InitLoaderScreen(const char *szLoaderSpec, bool fDrawBlackScreenFirst);
		void EnableLoaderDrawing(); // reset black screen loader flag
		bool SaveScreenshot(bool fSaveAll);
		BOOL DoSaveScreenshot(bool fSaveAll, const char *szFilename);
		BOOL SetPalette();
		BOOL CreateViewport(int32_t iPlayer, bool fSilent);
		BOOL CloseViewport(int32_t iPlayer, bool fSilent);
		int32_t GetAudibility(int32_t iX, int32_t iY, int32_t *iPan, int32_t iAudibilityRadius=0);
		int32_t GetViewportCount();
		C4Viewport* GetViewport(int32_t iPlayer);
		C4Viewport* GetFirstViewport() { return FirstViewport; }
		inline void InvalidateBg() { iRedrawBackground=2; }
		inline void OverwriteBg() { InvalidateBg(); }
		void SetGamma(DWORD dwClr1, DWORD dwClr2, DWORD dwClr3, int32_t iRampIndex);	// set gamma ramp
		void ApplyGamma();																				// apply gamma ramp to ddraw
		BOOL CloseViewport(C4Viewport * cvp);
#ifdef _WIN32
		C4Viewport* GetViewport(HWND hwnd);
		BOOL RegisterViewportClass(HINSTANCE hInst);
#endif
	protected:
		C4Viewport *FirstViewport;
		BOOL fViewportClassRegistered;
		C4Facet ViewportArea;
		C4RectList BackgroundAreas; // rectangles covering background without viewports in fullscreen
		char FlashMessageText[C4MaxTitle+1];
		int32_t FlashMessageTime,FlashMessageX,FlashMessageY;
		void DrawHelp();
		void DrawFlashMessage();
		void DrawHoldMessages();
		void DrawFullscreenBackground();
		void ClearFullscreenBackground();
		int32_t SeekLoaderScreens(C4Group &rFromGrp, const char *szWildcard, int32_t iLoaderCount, char *szDstName, C4Group **ppDestGrp);
		void MouseMoveToViewport(int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam);

	public:
		bool ToggleShowSolidMask();
		bool ToggleShowNetStatus();
		bool ToggleShowVertices();
		bool ToggleShowAction();
		bool ViewportNextPlayer();
		bool ToggleShowHelp();

		bool FreeScroll(C4Vec2D vScrollBy); // key callback: Scroll ownerless viewport by some offset
		bool ViewportZoomOut();
		bool ViewportZoomIn();
	};

extern C4GraphicsSystem GraphicsSystem;
#endif
