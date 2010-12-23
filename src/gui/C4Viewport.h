/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001, 2005, 2010  Sven Eberhardt
 * Copyright (c) 2005-2006, 2008-2010  Günther Brammer
 * Copyright (c) 2006  Armin Burgmeier
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

/* A viewport to each player */

#ifndef INC_C4Viewport
#define INC_C4Viewport

#include <C4Region.h>

#include <C4Shape.h>

class C4Viewport
{
	friend class C4MouseControl;
public:
	C4Viewport();
	~C4Viewport();
	C4RegionList Regions;
	// "landscape" coordinates
	float ViewX,ViewY;
	int32_t ViewOffsX, ViewOffsY;
	// "display" coordinates
	int32_t ViewWdt,ViewHgt;
	int32_t BorderLeft, BorderTop, BorderRight, BorderBottom;
	int32_t DrawX,DrawY;
	// facets used for last drawing operations
	C4TargetFacet last_game_draw_cgo, last_gui_draw_cgo;
	// factor between "landscape" and "display"
	bool fIsNoOwnerViewport; // this viewport is found for searches of NO_OWNER-viewports; even if it has a player assigned (for network obs)

	float GetZoom() { return Zoom; }
	void Default();
	void Clear();
	void Execute();
	void ClearPointers(C4Object *pObj);
	void SetOutputSize(int32_t iDrawX, int32_t iDrawY, int32_t iOutX, int32_t iOutY, int32_t iOutWdt, int32_t iOutHgt);
	void UpdateViewPosition(); // update view position: Clip properly; update border variables
	void InitZoom();
	void ChangeZoom(float by_factor);
	void SetZoom(float to_zoom, bool direct=false);
	void SetZoomLimits(float to_min_zoom, float to_max_zoom);
	float GetZoomByViewRange(int32_t size_x, int32_t size_y) const; // set zoom such that the supplied size is visible in the viewport
	float GetZoomLimitMin() const { return ZoomLimitMin; }
	float GetZoomLimitMax() const { return ZoomLimitMax; }
	float GetZoomTarget() const { return ZoomTarget; }
	bool Init(int32_t iPlayer, bool fSetTempOnly);
	bool Init(CStdWindow * pParent, CStdApp * pApp, int32_t iPlayer);
#ifdef _WIN32
	bool DropFiles(HANDLE hDrop);
#endif
	bool TogglePlayerLock();
	void NextPlayer();
	C4Rect GetOutputRect() { return C4Rect(OutX, OutY, ViewWdt, ViewHgt); }
	bool IsViewportMenu(class C4Menu *pMenu);
	C4Viewport *GetNext() { return Next; }
	int32_t GetPlayer() { return Player; }
	void CenterPosition();
protected:
	float Zoom;
	float ZoomTarget;
	float ZoomLimitMin,ZoomLimitMax;
	int32_t Player;
	bool PlayerLock;
	int32_t OutX,OutY;
	bool ResetMenuPositions;
	C4RegionList *SetRegions;
	C4Viewport *Next;
	class C4ViewportWindow * pWindow;
	CClrModAddMap ClrModMap; // color modulation map for viewport drawing
	void DrawPlayerFogOfWar(C4TargetFacet &cgo);
	void DrawPlayerStartup(C4TargetFacet &cgo);
	void Draw(C4TargetFacet &cgo, bool fDrawOverlay);
	void DrawOverlay(C4TargetFacet &cgo, const ZoomData &GameZoom);
	void DrawMenu(C4TargetFacet &cgo);
	void DrawPlayerInfo(C4TargetFacet &cgo);
	void BlitOutput();
	void AdjustPosition();
public:
	C4ViewportWindow* GetWindow() {return pWindow;}
	bool UpdateOutputSize();
	bool ViewPositionByScrollBars();
	bool ScrollBarsByViewPosition();
#ifdef _WIN32
	friend LRESULT APIENTRY ViewportWinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif
	friend class C4ViewportWindow;
	friend class C4ViewportList;
	friend class C4GraphicsSystem;
	friend class C4Video;
};

class C4ViewportList {
public:
	C4ViewportList();
	~C4ViewportList();
	void Clear();
	void ClearPointers(C4Object *pObj);
	void Execute(bool DrawBackground);
	void SortViewportsByPlayerControl();
	void RecalculateViewports();
	bool CreateViewport(int32_t iPlayer, bool fSilent=false);
	bool CloseViewport(int32_t iPlayer, bool fSilent);
	int32_t GetViewportCount();
	C4Viewport* GetViewport(int32_t iPlayer, C4Viewport* pPrev = NULL);
	C4Viewport* GetFirstViewport() { return FirstViewport; }
	bool CloseViewport(C4Viewport * cvp);
#ifdef _WIN32
	C4Viewport* GetViewport(HWND hwnd);
#endif
	int32_t GetAudibility(int32_t iX, int32_t iY, int32_t *iPan, int32_t iAudibilityRadius=0);
	bool ViewportNextPlayer();

	bool FreeScroll(C4Vec2D vScrollBy); // key callback: Scroll ownerless viewport by some offset
	bool ViewportZoomOut();
	bool ViewportZoomIn();
protected:
	void MouseMoveToViewport(int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam);
	void DrawFullscreenBackground();
	C4Viewport *FirstViewport;
	C4Facet ViewportArea;
	C4RectList BackgroundAreas; // rectangles covering background without viewports in fullscreen
	friend class C4GUI::Screen;
	friend class C4GraphicsSystem;
};

extern C4ViewportList Viewports;

#endif
