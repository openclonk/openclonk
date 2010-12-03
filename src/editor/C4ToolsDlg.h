/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001, 2005, 2007  Sven Eberhardt
 * Copyright (c) 2005  Günther Brammer
 * Copyright (c) 2006-2007  Armin Burgmeier
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

/* Drawing tools dialog for landscape editing in console mode */

#ifndef INC_C4ToolsDlg
#define INC_C4ToolsDlg

#ifdef WITH_DEVELOPER_MODE
#include <gtk/gtk.h>
#endif

#include "C4Constants.h"

const int32_t C4TLS_Brush  = 0,
                             C4TLS_Line   = 1,
                                            C4TLS_Rect   = 2,
                                                           C4TLS_Fill   = 3,
                                                                          C4TLS_Picker = 4;

const int32_t C4TLS_GradeMax     = 50,
                                   C4TLS_GradeMin     = 1,
                                                        C4TLS_GradeDefault = 5;

#define C4TLS_MatSky "Sky"

class C4ToolsDlg
{
#ifdef _WIN32
	friend INT_PTR CALLBACK ToolsDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
#endif
public:
	C4ToolsDlg();
	~C4ToolsDlg();
public:
	bool Active;
#ifdef _WIN32
	HWND hDialog;
#ifdef USE_GL
	class CStdGLCtx *pGLCtx; // rendering context for OpenGL
#endif
#else
#ifdef WITH_DEVELOPER_MODE
	GtkWidget* hbox;

	GtkWidget* brush;
	GtkWidget* line;
	GtkWidget* rect;
	GtkWidget* fill;
	GtkWidget* picker;

	GtkWidget* landscape_dynamic;
	GtkWidget* landscape_static;
	GtkWidget* landscape_exact;

	GtkWidget* preview;
	GtkWidget* scale;

	GtkWidget* ift;
	GtkWidget* no_ift;

	GtkWidget* materials;
	GtkWidget* textures;

	gulong handlerBrush;
	gulong handlerLine;
	gulong handlerRect;
	gulong handlerFill;
	gulong handlerPicker;

	gulong handlerDynamic;
	gulong handlerStatic;
	gulong handlerExact;

	gulong handlerIft;
	gulong handlerNoIft;

	gulong handlerMaterials;
	gulong handlerTextures;
	gulong handlerScale;

	gulong handlerHide;

	//static void OnDestroy(GtkWidget* widget, gpointer data);
	static void OnButtonModeDynamic(GtkWidget* widget, gpointer data);
	static void OnButtonModeStatic(GtkWidget* widget, gpointer data);
	static void OnButtonModeExact(GtkWidget* widget, gpointer data);
	static void OnButtonBrush(GtkWidget* widget, gpointer data);
	static void OnButtonLine(GtkWidget* widget, gpointer data);
	static void OnButtonRect(GtkWidget* widget, gpointer data);
	static void OnButtonFill(GtkWidget* widget, gpointer data);
	static void OnButtonPicker(GtkWidget* widget, gpointer data);
	static void OnButtonIft(GtkWidget* widget, gpointer data);
	static void OnButtonNoIft(GtkWidget* widget, gpointer data);
	static void OnComboMaterial(GtkWidget* widget, gpointer data);
	static void OnComboTexture(GtkWidget* widget, gpointer data);
	static void OnGrade(GtkWidget* widget, gpointer data);
	static void OnWindowHide(GtkWidget* widget, gpointer data);
#endif
#endif
	int32_t Tool, SelectedTool;
	int32_t Grade;
	bool ModeIFT;
	char Material[C4M_MaxName+1];
	char Texture[C4M_MaxName+1];
protected:
#ifdef _WIN32
	HBITMAP hbmBrush,hbmBrush2;
	HBITMAP hbmLine,hbmLine2;
	HBITMAP hbmRect,hbmRect2;
	HBITMAP hbmFill,hbmFill2;
	HBITMAP hbmPicker,hbmPicker2;
	HBITMAP hbmIFT;
	HBITMAP hbmNoIFT;
	HBITMAP hbmDynamic;
	HBITMAP hbmStatic;
	HBITMAP hbmExact;
#endif
public:
	void Default();
	void Clear();
	bool PopTextures();
	bool PopMaterial();
	bool ChangeGrade(int32_t iChange);
	void UpdatePreview();
	bool Open();
	bool SetGrade(int32_t iGrade);
	bool SetTool(int32_t iTool, bool fTemp);
	bool ToggleTool() { return !!SetTool((Tool+1)%4, false); }
	bool SetLandscapeMode(int32_t iMode, bool fThroughControl=false);
	bool SetIFT(bool fIFT);
	bool ToggleIFT() { return !!SetIFT(!ModeIFT); }
	bool SelectTexture(const char *szTexture);
	bool SelectMaterial(const char *szMaterial);
	void SetAlternateTool();
	void ResetAlternateTool();
protected:
	void AssertValidTexture();
	void LoadBitmaps();
	void EnableControls();
	void UpdateIFTControls();
	void InitGradeCtrl();
	void InitMaterialCtrls();
	void UpdateToolCtrls();
	void SetTexture(const char *szTexture);
	void SetMaterial(const char *szMaterial);
	void UpdateTextures();
	void SetColorPattern(const char *szMaterial, const char *szTexture);
public:
	void UpdateLandscapeModeCtrls();
};

#endif //INC_C4ToolsDlg
