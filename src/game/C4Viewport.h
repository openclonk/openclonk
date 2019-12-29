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

/* A viewport to each player */

#ifndef INC_C4Viewport
#define INC_C4Viewport

#include "graphics/C4FacetEx.h"

class C4ViewportWindow;
class C4FoWRegion;
struct ZoomData;

class C4Viewport
{
	friend class C4MouseControl;
public:
	C4Viewport();
	~C4Viewport();
	// "display" coordinates
	int32_t ViewWdt;
	int32_t ViewHgt;
	// position of landscape border (left,top,right, bottom) in viewport. 0 if there is not border
	float BorderLeft, BorderTop, BorderRight, BorderBottom;
	int32_t DrawX;
	int32_t DrawY;
	// facets used for last drawing operations
	C4TargetFacet last_game_draw_cgo;
	C4TargetFacet last_gui_draw_cgo;
	// factor between "landscape" and "display"
	bool fIsNoOwnerViewport; // this viewport is found for searches of NO_OWNER-viewports; even if it has a player assigned (for network obs)

	float GetZoom() { return Zoom; }
	void SetZoom(float zoom_value);
	float GetGUIZoom() const { return Clamp<float>(float(ViewWdt)/1280, 0.5f, 1.0f); }
	void Execute();
	void ClearPointers(C4Object *obj);
	void SetOutputSize(int32_t draw_x, int32_t draw_y, int32_t out_x, int32_t out_y, int32_t out_wdt, int32_t out_hgt);
	void CalculateZoom();
	void ChangeZoom(float by_factor);
	void SetZoom(float to_zoom, bool direct = false);
	void SetZoomLimits(float to_min_zoom, float to_max_zoom);
	float GetZoomByViewRange(int32_t size_x, int32_t size_y) const; // set zoom such that the supplied size is visible in the viewport
	float GetZoomLimitMin() const { return ZoomLimitMin; }
	float GetZoomLimitMax() const { return ZoomLimitMax; }
	float GetZoomTarget() const { return ZoomTarget; }
	bool Init(int32_t for_player, bool set_temporary_only);
	void DropFile(const char* filename, float x, float y);
	bool TogglePlayerLock();
	bool GetPlayerLock() { return PlayerLock; }
	void NextPlayer();
	C4Rect GetOutputRect() { return C4Rect(OutX, OutY, ViewWdt, ViewHgt); }
	bool IsViewportMenu(class C4Menu *menu);
	C4Viewport *GetNext() { return Next; }
	int32_t GetPlayer() { return Player; }
	void CenterPosition();
	void DisableFoW();
	void EnableFoW();
public: 
	/** Return x-position of upper left corner of viewport in landscape coordinates */
	float GetViewX() { return viewX; }
	/** Return y-position of upper left corner of viewport in landscape coordinates */
	float GetViewY() { return viewY; }
	/** Return x-position of the center of viewport in landscape coordinates */
	float GetViewCenterX() { return viewX + ViewWdt/Zoom/2; }
	/** Return y-position of the center of viewport in landscape coordinates */
	float GetViewCenterY() { return viewY + ViewHgt/Zoom/2; }

	// Convert window coordinates to game coordinates
	float WindowToGameX(int32_t win_x) { return GetViewX() + float(win_x) / Zoom; }
	float WindowToGameY(int32_t win_y) { return GetViewY() + float(win_y) / Zoom; }

	/** Scroll the viewport by x,y */
	void ScrollView(float by_x, float by_y);
	/** Set the view position. */
	void SetViewX(float x);
	void SetViewY(float y);
	/** Set the view offset of the normal viewport center. Used by C4Script function SetViewOffset. */
	void SetViewOffset(int32_t x, int32_t y) { viewOffsX = x; viewOffsY = y; }

private:
	float viewX, viewY;	// current view position in landscape coordinates (upper left corner)
	float targetViewX, targetViewY; // target view position for smooth scrolling
	int32_t viewOffsX, viewOffsY;	// desired view offset in landscape coordinates

	void UpdateBordersX();
	void UpdateBordersY();
protected:
	float Zoom;
	float ZoomTarget;
	float ZoomLimitMin,ZoomLimitMax;
	int32_t ViewportOpenFrame; // Game FrameCounter in which viewport was opened. Used to init zoom during initial fullscreen viewport movement chaos, but not change it later e.g. when other local players join.
	int32_t Player;
	bool PlayerLock;
	int32_t OutX,OutY;
	bool ResetMenuPositions;
	C4Viewport *Next;
	std::unique_ptr<C4ViewportWindow> pWindow;
	std::unique_ptr<C4FoWRegion> pFoW;
	void DrawPlayerStartup(C4TargetFacet &cgo);
	void Draw(C4TargetFacet &cgo, bool draw_game, bool draw_overlay);
	void DrawOverlay(C4TargetFacet &cgo, const ZoomData &GameZoom);
	void DrawMenu(C4TargetFacet &cgo);
	void DrawPlayerInfo(C4TargetFacet &cgo);
	void InitZoom();
	void BlitOutput();
	void AdjustZoomAndPosition();
public:
	float GetZoom() const { return Zoom; }
	void AdjustPosition(bool immediate = false);
	C4ViewportWindow* GetWindow() {return pWindow.get();}
	bool UpdateOutputSize(int32_t new_width = 0, int32_t new_height = 0);
	bool ViewPositionByScrollBars();
	bool ScrollBarsByViewPosition();

#ifdef WITH_QT_EDITOR
	class C4ConsoleQtViewportScrollArea *scrollarea;
#endif

#ifdef _WIN32
	friend LRESULT APIENTRY ViewportWinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif
	friend class C4ViewportWindow;
	friend class C4ViewportList;
	friend class C4GraphicsSystem;
	friend class C4ConsoleQtViewportView;
};

class C4ViewportList {
public:
	C4ViewportList();
	~C4ViewportList();
	void Clear();
	void ClearPointers(C4Object *obj);
	void Execute(bool DrawBackground);
	void SortViewportsByPlayerControl();
	void RecalculateViewports();
	void DisableFoW();
	void EnableFoW();
	bool CreateViewport(int32_t player_nr, bool silent = false);
	bool CloseViewport(int32_t player_nr, bool silent);
	int32_t GetViewportCount();
	C4Viewport* GetViewport(int32_t player_nr, C4Viewport* prev = nullptr);
	C4Viewport* GetFirstViewport() { return FirstViewport; }
	bool CloseViewport(C4Viewport * viewport);
#ifdef USE_WIN32_WINDOWS
	C4Viewport* GetViewport(HWND hwnd);
#endif
	int32_t GetAudibility(int32_t x, int32_t y, int32_t *pan, int32_t audibility_radius = 0, int32_t *out_player = nullptr);
	bool ViewportNextPlayer();

	bool FreeScroll(C4Vec2D scroll_by); // key callback: Scroll ownerless viewport by some offset
	bool ViewportZoomOut();
	bool ViewportZoomIn();
protected:
	void MouseMoveToViewport(int32_t button, int32_t x, int32_t y, DWORD key_param);
	void DrawFullscreenBackground();
	C4Viewport *FirstViewport{nullptr};
	C4Facet ViewportArea;
	C4RectList BackgroundAreas; // rectangles covering background without viewports in fullscreen
	friend class C4GUI::Screen;
	friend class C4GraphicsSystem;
};

extern C4ViewportList Viewports;

#endif
