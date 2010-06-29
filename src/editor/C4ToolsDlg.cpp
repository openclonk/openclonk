/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2003  Matthes Bender
 * Copyright (c) 2002, 2005-2007  Sven Eberhardt
 * Copyright (c) 2005-2007, 2009  Günther Brammer
 * Copyright (c) 2005, 2007  Peter Wortmann
 * Copyright (c) 2006-2007  Armin Burgmeier
 * Copyright (c) 2009  Nicolas Hake
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

#include <C4Include.h>
#include <C4ToolsDlg.h>
#include <C4Console.h>
#include <C4Application.h>
#include <C4Texture.h>
#include <C4Landscape.h>
#include <C4Game.h>
#include <C4GameControl.h>
#include <StdRegistry.h>
#include <StdD3D.h>
#ifdef USE_GL
#include <StdGL.h>
#endif

#ifdef _WIN32
#include "resource.h"
#endif

#ifdef WITH_DEVELOPER_MODE
# include <C4Language.h>
# include <C4DevmodeDlg.h>

# include <gtk/gtkwindow.h>
# include <gtk/gtkimage.h>
# include <gtk/gtktogglebutton.h>
# include <gtk/gtkvscale.h>
# include <gtk/gtkhbox.h>
# include <gtk/gtkvbox.h>
# include <gtk/gtkcombobox.h>
# include <gtk/gtkstock.h>
# include <gtk/gtk.h>

# include <res/Brush.h>
# include <res/Line.h>
# include <res/Rect.h>
# include <res/Fill.h>
# include <res/Picker.h>

# include <res/Dynamic.h>
# include <res/Static.h>
# include <res/Exact.h>

# include <res/Ift.h>
# include <res/NoIft.h>

namespace
{
	void SelectComboBoxText(GtkComboBox* combobox, const char* text)
	{
		GtkTreeModel* model = gtk_combo_box_get_model(combobox);

		GtkTreeIter iter;
		for (gboolean ret = gtk_tree_model_get_iter_first(model, &iter); ret; ret = gtk_tree_model_iter_next(model, &iter))
		{
			gchar* col_text;
			gtk_tree_model_get(model, &iter, 0, &col_text, -1);

			if (SEqualNoCase(text, col_text))
			{
				g_free(col_text);
				gtk_combo_box_set_active_iter(combobox, &iter);
				return;
			}

			g_free(col_text);
		}
	}

	gboolean RowSeparatorFunc(GtkTreeModel* model, GtkTreeIter* iter, void* user_data)
	{
		gchar* text;
		gtk_tree_model_get(model, iter, 0, &text, -1);

		if (SEqual(text, "------")) { g_free(text); return true; }
		g_free(text);
		return false;
	}

	GtkWidget* CreateImageFromInlinedPixbuf(const guint8* pixbuf_data)
	{
		GdkPixbuf* pixbuf = gdk_pixbuf_new_from_inline(-1, pixbuf_data, false, NULL);
		GtkWidget* image = gtk_image_new_from_pixbuf(pixbuf);
		gdk_pixbuf_unref(pixbuf);
		return image;
	}
}
#endif

#ifdef _WIN32
#include <commctrl.h>
BOOL CALLBACK ToolsDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	int32_t iValue;
	switch (Msg)
	{
		//----------------------------------------------------------------------------------------------
	case WM_CLOSE:
		Console.ToolsDlg.Clear();
		break;
		//----------------------------------------------------------------------------------------------
	case WM_DESTROY:
		StoreWindowPosition(hDlg, "Property", Config.GetSubkeyPath("Console"), false);
		break;
		//----------------------------------------------------------------------------------------------
	case WM_INITDIALOG:
		return true;
		//----------------------------------------------------------------------------------------------
	case WM_PAINT:
		PostMessage(hDlg,WM_USER,0,0); // For user paint
		return false;
		//----------------------------------------------------------------------------------------------
	case WM_USER:
		Console.ToolsDlg.UpdatePreview();
		return true;
		//----------------------------------------------------------------------------------------------
	case WM_VSCROLL:
		switch (LOWORD(wParam))
		{
		case SB_THUMBTRACK: case SB_THUMBPOSITION:
			iValue=HIWORD(wParam);
			Console.ToolsDlg.SetGrade(C4TLS_GradeMax-iValue);
			break;
		case SB_PAGEUP: case SB_PAGEDOWN:
		case SB_LINEUP: case SB_LINEDOWN:
			iValue=SendDlgItemMessage(hDlg,IDC_SLIDERGRADE,TBM_GETPOS,0,0);
			Console.ToolsDlg.SetGrade(C4TLS_GradeMax-iValue);
			break;
		}
		return true;
		//----------------------------------------------------------------------------------------------
	case WM_COMMAND:
		// Evaluate command
		switch (LOWORD(wParam))
		{
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDOK:
			return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_BUTTONMODEDYNAMIC:
			Console.ToolsDlg.SetLandscapeMode(C4LSC_Dynamic);
			return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_BUTTONMODESTATIC:
			Console.ToolsDlg.SetLandscapeMode(C4LSC_Static);
			return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_BUTTONMODEEXACT:
			Console.ToolsDlg.SetLandscapeMode(C4LSC_Exact);
			return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_BUTTONBRUSH:
			Console.ToolsDlg.SetTool(C4TLS_Brush, false);
			return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_BUTTONLINE:
			Console.ToolsDlg.SetTool(C4TLS_Line, false);
			return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_BUTTONRECT:
			Console.ToolsDlg.SetTool(C4TLS_Rect, false);
			return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_BUTTONFILL:
			Console.ToolsDlg.SetTool(C4TLS_Fill, false);
			return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_BUTTONPICKER:
			Console.ToolsDlg.SetTool(C4TLS_Picker, false);
			return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_BUTTONIFT:
			Console.ToolsDlg.SetIFT(true);
			return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_BUTTONNOIFT:
			Console.ToolsDlg.SetIFT(false);
			return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_COMBOMATERIAL:
			switch (HIWORD(wParam))
			{
			case CBN_SELCHANGE:
			{
				char str[100];
				int32_t cursel = SendDlgItemMessage(hDlg,IDC_COMBOMATERIAL,CB_GETCURSEL,0,0);
				SendDlgItemMessage(hDlg,IDC_COMBOMATERIAL,CB_GETLBTEXT,cursel,(LPARAM)str);
				Console.ToolsDlg.SetMaterial(str);
				break;
			}
			}
			return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_COMBOTEXTURE:
			switch (HIWORD(wParam))
			{
			case CBN_SELCHANGE:
			{
				char str[100];
				int32_t cursel = SendDlgItemMessage(hDlg,IDC_COMBOTEXTURE,CB_GETCURSEL,0,0);
				SendDlgItemMessage(hDlg,IDC_COMBOTEXTURE,CB_GETLBTEXT,cursel,(LPARAM)str);
				Console.ToolsDlg.SetTexture(str);
				break;
			}
			}
			return true;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		}
		return false;
		//----------------------------------------------------------------------------------------
	}
	return false;
}
#endif
C4ToolsDlg::C4ToolsDlg()
{
	Default();
#ifdef _WIN32
	hbmBrush=hbmLine=hbmRect=hbmFill=NULL;
	hbmIFT=hbmNoIFT=NULL;
#endif
}

C4ToolsDlg::~C4ToolsDlg()
{
	Clear();
#ifdef WITH_DEVELOPER_MODE
	if (hbox != NULL)
	{
		g_signal_handler_disconnect(G_OBJECT(C4DevmodeDlg::GetWindow()), handlerHide);
		C4DevmodeDlg::RemovePage(hbox);
		hbox = NULL;
	}
#endif // WITH_DEVELOPER_MODE

	// Unload bitmaps
#ifdef _WIN32
	if (hbmBrush) DeleteObject(hbmBrush);
	if (hbmLine) DeleteObject(hbmLine);
	if (hbmRect) DeleteObject(hbmRect);
	if (hbmFill) DeleteObject(hbmFill);
	if (hbmIFT) DeleteObject(hbmIFT);
	if (hbmNoIFT) DeleteObject(hbmNoIFT);
#endif
}

bool C4ToolsDlg::Open()
{
	// Create dialog window
#ifdef _WIN32
	if (hDialog) return true;
	hDialog = CreateDialog(Application.GetInstance(),
	                       MAKEINTRESOURCE(IDD_TOOLS),
	                       Console.hWindow,
	                       (DLGPROC) ToolsDlgProc);
	if (!hDialog) return false;
	// Set text
	SetWindowText(hDialog,LoadResStr("IDS_DLG_TOOLS"));
	SetDlgItemText(hDialog,IDC_STATICMATERIAL,LoadResStr("IDS_CTL_MATERIAL"));
	SetDlgItemText(hDialog,IDC_STATICTEXTURE,LoadResStr("IDS_CTL_TEXTURE"));
	// Load bitmaps if necessary
	LoadBitmaps();
	// create target ctx for OpenGL rendering
#ifdef USE_GL
	if (lpDDraw && !pGLCtx) pGLCtx = lpDDraw->CreateContext(GetDlgItem(hDialog,IDC_PREVIEW), &Application);
#endif
	// Show window
	RestoreWindowPosition(hDialog, "Property", Config.GetSubkeyPath("Console"));
	SetWindowPos(hDialog,Console.hWindow,0,0,0,0,SWP_NOSIZE | SWP_NOMOVE);
	ShowWindow(hDialog,SW_SHOWNOACTIVATE);
#else
#ifdef WITH_DEVELOPER_MODE
	if (hbox == NULL)
	{
		hbox = gtk_hbox_new(false, 12);
		GtkWidget* vbox = gtk_vbox_new(false, 6);

		GtkWidget* image_brush = CreateImageFromInlinedPixbuf(brush_pixbuf_data);
		GtkWidget* image_line = CreateImageFromInlinedPixbuf(line_pixbuf_data);
		GtkWidget* image_rect = CreateImageFromInlinedPixbuf(rect_pixbuf_data);
		GtkWidget* image_fill = CreateImageFromInlinedPixbuf(fill_pixbuf_data);
		GtkWidget* image_picker = CreateImageFromInlinedPixbuf(picker_pixbuf_data);

		GtkWidget* image_dynamic = CreateImageFromInlinedPixbuf(dynamic_pixbuf_data);
		GtkWidget* image_static = CreateImageFromInlinedPixbuf(static_pixbuf_data);
		GtkWidget* image_exact = CreateImageFromInlinedPixbuf(exact_pixbuf_data);

		GtkWidget* image_ift = CreateImageFromInlinedPixbuf(ift_pixbuf_data);
		GtkWidget* image_no_ift = CreateImageFromInlinedPixbuf(no_ift_pixbuf_data);

		landscape_dynamic = gtk_toggle_button_new();
		landscape_static = gtk_toggle_button_new();
		landscape_exact = gtk_toggle_button_new();

		gtk_container_add(GTK_CONTAINER(landscape_dynamic), image_dynamic);
		gtk_container_add(GTK_CONTAINER(landscape_static), image_static);
		gtk_container_add(GTK_CONTAINER(landscape_exact), image_exact);

		gtk_box_pack_start(GTK_BOX(vbox), landscape_dynamic, false, false, 0);
		gtk_box_pack_start(GTK_BOX(vbox), landscape_static, false, false, 0);
		gtk_box_pack_start(GTK_BOX(vbox), landscape_exact, false, false, 0);

		gtk_box_pack_start(GTK_BOX(hbox), vbox, false, false, 0);
		vbox = gtk_vbox_new(false, 12);
		gtk_box_pack_start(GTK_BOX(hbox), vbox, true, true, 0);
		GtkWidget* local_hbox = gtk_hbox_new(false, 6);

		brush = gtk_toggle_button_new();
		line = gtk_toggle_button_new();
		rect = gtk_toggle_button_new();
		fill = gtk_toggle_button_new();
		picker = gtk_toggle_button_new();

		gtk_container_add(GTK_CONTAINER(brush), image_brush);
		gtk_container_add(GTK_CONTAINER(line), image_line);
		gtk_container_add(GTK_CONTAINER(rect), image_rect);
		gtk_container_add(GTK_CONTAINER(fill), image_fill);
		gtk_container_add(GTK_CONTAINER(picker), image_picker);

		gtk_box_pack_start(GTK_BOX(local_hbox), brush, false, false, 0);
		gtk_box_pack_start(GTK_BOX(local_hbox), line, false, false, 0);
		gtk_box_pack_start(GTK_BOX(local_hbox), rect, false, false, 0);
		gtk_box_pack_start(GTK_BOX(local_hbox), fill, false, false, 0);
		gtk_box_pack_start(GTK_BOX(local_hbox), picker, false, false, 0);

		gtk_box_pack_start(GTK_BOX(vbox), local_hbox, false, false, 0);
		local_hbox = gtk_hbox_new(false, 12);
		gtk_box_pack_start(GTK_BOX(vbox), local_hbox, true, true, 0);

		preview = gtk_image_new();
		gtk_box_pack_start(GTK_BOX(local_hbox), preview, false, false, 0);

		scale = gtk_vscale_new(NULL);
		gtk_box_pack_start(GTK_BOX(local_hbox), scale, false, false, 0);

		vbox = gtk_vbox_new(false, 6);

		ift = gtk_toggle_button_new();
		no_ift = gtk_toggle_button_new();

		gtk_container_add(GTK_CONTAINER(ift), image_ift);
		gtk_container_add(GTK_CONTAINER(no_ift), image_no_ift);

		gtk_box_pack_start(GTK_BOX(vbox), ift, false, false, 0);
		gtk_box_pack_start(GTK_BOX(vbox), no_ift, false, false, 0);

		gtk_box_pack_start(GTK_BOX(local_hbox), vbox, false, false, 0);

		vbox = gtk_vbox_new(false, 6);

		materials = gtk_combo_box_new_text();
		textures = gtk_combo_box_new_text();

		gtk_combo_box_set_row_separator_func(GTK_COMBO_BOX(materials), RowSeparatorFunc, NULL, NULL);
		gtk_combo_box_set_row_separator_func(GTK_COMBO_BOX(textures), RowSeparatorFunc, NULL, NULL);

		gtk_box_pack_start(GTK_BOX(vbox), materials, true, false, 0);
		gtk_box_pack_start(GTK_BOX(vbox), textures, true, false, 0);

		gtk_box_pack_start(GTK_BOX(local_hbox), vbox, true, true, 0); // ???
		gtk_widget_show_all(hbox);

		C4DevmodeDlg::AddPage(hbox, GTK_WINDOW(Console.window), LoadResStr("IDS_DLG_TOOLS"));

		//g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(OnDestroy), this);
		handlerDynamic = g_signal_connect(G_OBJECT(landscape_dynamic), "toggled", G_CALLBACK(OnButtonModeDynamic), this);
		handlerStatic = g_signal_connect(G_OBJECT(landscape_static), "toggled", G_CALLBACK(OnButtonModeStatic), this);
		handlerExact = g_signal_connect(G_OBJECT(landscape_exact), "toggled", G_CALLBACK(OnButtonModeExact), this);
		handlerBrush = g_signal_connect(G_OBJECT(brush), "toggled", G_CALLBACK(OnButtonBrush), this);
		handlerLine = g_signal_connect(G_OBJECT(line), "toggled", G_CALLBACK(OnButtonLine), this);
		handlerRect = g_signal_connect(G_OBJECT(rect), "toggled", G_CALLBACK(OnButtonRect), this);
		handlerFill = g_signal_connect(G_OBJECT(fill), "toggled", G_CALLBACK(OnButtonFill), this);
		handlerPicker = g_signal_connect(G_OBJECT(picker), "toggled", G_CALLBACK(OnButtonPicker), this);
		handlerIft = g_signal_connect(G_OBJECT(ift), "toggled", G_CALLBACK(OnButtonIft), this);
		handlerNoIft = g_signal_connect(G_OBJECT(no_ift), "toggled", G_CALLBACK(OnButtonNoIft), this);
		handlerMaterials = g_signal_connect(G_OBJECT(materials), "changed", G_CALLBACK(OnComboMaterial), this);
		handlerTextures = g_signal_connect(G_OBJECT(textures), "changed", G_CALLBACK(OnComboTexture), this);
		handlerScale = g_signal_connect(G_OBJECT(scale), "value-changed", G_CALLBACK(OnGrade), this);

		handlerHide = g_signal_connect(G_OBJECT(C4DevmodeDlg::GetWindow()), "hide", G_CALLBACK(OnWindowHide), this);
	}

	C4DevmodeDlg::SwitchPage(hbox);

#endif
#endif
	Active = true;
	// Update contols
	InitGradeCtrl();
	UpdateLandscapeModeCtrls();
	UpdateToolCtrls();
	UpdateIFTControls();
	InitMaterialCtrls();
	EnableControls();
	return true;
}

void C4ToolsDlg::Default()
{
#ifdef _WIN32
	hDialog=NULL;
#ifdef USE_GL
	pGLCtx = NULL;
#endif
#else
#ifdef WITH_DEVELOPER_MODE
	hbox = NULL;
#endif
#endif
	Active = false;
	Tool = SelectedTool = C4TLS_Brush;
	Grade = C4TLS_GradeDefault;
	ModeIFT = true;
	SCopy("Earth",Material);
	SCopy("earth",Texture);
}

void C4ToolsDlg::Clear()
{
#ifdef _WIN32
#ifdef USE_GL
	delete pGLCtx; pGLCtx = NULL;
#endif
	if (hDialog) DestroyWindow(hDialog); hDialog=NULL;
#else
#ifdef WITH_DEVELOPER_MODE
	//if(hbox != NULL)
	//  C4DevmodeDlg::switch_page(NULL);
#endif
#endif
	Active = false;
}

bool C4ToolsDlg::SetTool(int32_t iTool, bool fTemp)
{
	Tool=iTool;
	if (!fTemp) SelectedTool = Tool;
	UpdateToolCtrls();
	UpdatePreview();
	return true;
}

void C4ToolsDlg::UpdateToolCtrls()
{
#ifdef _WIN32
	SendDlgItemMessage(hDialog,IDC_BUTTONBRUSH,BM_SETSTATE,(Tool==C4TLS_Brush),0);
	UpdateWindow(GetDlgItem(hDialog,IDC_BUTTONBRUSH));
	SendDlgItemMessage(hDialog,IDC_BUTTONLINE,BM_SETSTATE,(Tool==C4TLS_Line),0);
	UpdateWindow(GetDlgItem(hDialog,IDC_BUTTONLINE));
	SendDlgItemMessage(hDialog,IDC_BUTTONRECT,BM_SETSTATE,(Tool==C4TLS_Rect),0);
	UpdateWindow(GetDlgItem(hDialog,IDC_BUTTONRECT));
	SendDlgItemMessage(hDialog,IDC_BUTTONFILL,BM_SETSTATE,(Tool==C4TLS_Fill),0);
	UpdateWindow(GetDlgItem(hDialog,IDC_BUTTONFILL));
	SendDlgItemMessage(hDialog,IDC_BUTTONPICKER,BM_SETSTATE,(Tool==C4TLS_Picker),0);
	UpdateWindow(GetDlgItem(hDialog,IDC_BUTTONPICKER));
#else
#ifdef WITH_DEVELOPER_MODE
	g_signal_handler_block(brush, handlerBrush);
	g_signal_handler_block(line, handlerLine);
	g_signal_handler_block(rect, handlerRect);
	g_signal_handler_block(fill, handlerFill);
	g_signal_handler_block(picker, handlerPicker);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(brush), Tool == C4TLS_Brush);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(line), Tool == C4TLS_Line);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rect), Tool == C4TLS_Rect);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fill), Tool == C4TLS_Fill);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(picker), Tool == C4TLS_Picker);

	g_signal_handler_unblock(brush, handlerBrush);
	g_signal_handler_unblock(line, handlerLine);
	g_signal_handler_unblock(rect, handlerRect);
	g_signal_handler_unblock(fill, handlerFill);
	g_signal_handler_unblock(picker, handlerPicker);
#endif
#endif
}

void C4ToolsDlg::InitMaterialCtrls()
{
	// Materials
#ifdef _WIN32
	SendDlgItemMessage(hDialog,IDC_COMBOMATERIAL,CB_ADDSTRING,0,(LPARAM)C4TLS_MatSky);
	for (int32_t cnt=0; cnt< ::MaterialMap.Num; cnt++)
		SendDlgItemMessage(hDialog,IDC_COMBOMATERIAL,CB_ADDSTRING,0,(LPARAM)::MaterialMap.Map[cnt].Name);
	SendDlgItemMessage(hDialog,IDC_COMBOMATERIAL,CB_SELECTSTRING,0,(LPARAM)Material);
#else
#ifdef WITH_DEVELOPER_MODE
	GtkListStore* list = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(materials)));

	g_signal_handler_block(materials, handlerMaterials);
	gtk_list_store_clear(list);

	gtk_combo_box_append_text(GTK_COMBO_BOX(materials), C4TLS_MatSky);
	for (int32_t cnt = 0; cnt < ::MaterialMap.Num; cnt++)
	{
		gtk_combo_box_append_text(GTK_COMBO_BOX(materials), ::MaterialMap.Map[cnt].Name);
	}
	g_signal_handler_unblock(materials, handlerMaterials);
	SelectComboBoxText(GTK_COMBO_BOX(materials), Material);
#endif
#endif
	// Textures
	UpdateTextures();
}

void C4ToolsDlg::UpdateTextures()
{
	// Refill dlg
#ifdef _WIN32
	SendDlgItemMessage(hDialog,IDC_COMBOTEXTURE,CB_RESETCONTENT,0,(LPARAM)0);
#else
#ifdef WITH_DEVELOPER_MODE
	GtkListStore* list = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(textures)));
	gtk_list_store_clear(list);
#endif
#endif
	// bottom-most: any invalid textures
	bool fAnyEntry = false; int32_t cnt; const char *szTexture;
	if (::Landscape.Mode!=C4LSC_Exact)
		for (cnt=0; (szTexture=::TextureMap.GetTexture(cnt)); cnt++)
		{
			if (!::TextureMap.GetIndex(Material, szTexture, false))
			{
				fAnyEntry = true;
#ifdef _WIN32
				SendDlgItemMessage(hDialog,IDC_COMBOTEXTURE,CB_INSERTSTRING,0,(LPARAM)szTexture);
#else
#ifdef WITH_DEVELOPER_MODE
				gtk_combo_box_prepend_text(GTK_COMBO_BOX(textures), szTexture);
#endif
#endif
			}
		}
	// separator
	if (fAnyEntry)
	{
#ifdef _WIN32
		SendDlgItemMessage(hDialog,IDC_COMBOTEXTURE,CB_INSERTSTRING,0,(LPARAM)"-------");
#else
#ifdef WITH_DEVELOPER_MODE
		gtk_combo_box_prepend_text(GTK_COMBO_BOX(textures), "-------");
#endif
#endif
	}

	// atop: valid textures
	for (cnt=0; (szTexture=::TextureMap.GetTexture(cnt)); cnt++)
	{
		// Current material-texture valid? Always valid for exact mode
		if (::TextureMap.GetIndex(Material,szTexture,false) || ::Landscape.Mode==C4LSC_Exact)
		{
#ifdef _WIN32
			SendDlgItemMessage(hDialog,IDC_COMBOTEXTURE,CB_INSERTSTRING,0,(LPARAM)szTexture);
#else
#ifdef WITH_DEVELOPER_MODE
			gtk_combo_box_prepend_text(GTK_COMBO_BOX(textures), szTexture);
#endif
#endif
		}
	}
	// reselect current
#ifdef _WIN32
	SendDlgItemMessage(hDialog,IDC_COMBOTEXTURE,CB_SELECTSTRING,0,(LPARAM)Texture);
#else
#ifdef WITH_DEVELOPER_MODE
	g_signal_handler_block(textures, handlerTextures);
	SelectComboBoxText(GTK_COMBO_BOX(textures), Texture);
	g_signal_handler_unblock(textures, handlerTextures);
#endif
#endif
}

void C4ToolsDlg::SetMaterial(const char *szMaterial)
{
	SCopy(szMaterial,Material,C4M_MaxName);
	AssertValidTexture();
	EnableControls();
	if (::Landscape.Mode==C4LSC_Static) UpdateTextures();
	UpdatePreview();
}

void C4ToolsDlg::SetTexture(const char *szTexture)
{
	// assert valid (for separator selection)
	if (!::TextureMap.GetTexture(szTexture))
	{
		// ensure correct texture is in dlg
#ifdef _WIN32
		SendDlgItemMessage(hDialog,IDC_COMBOTEXTURE,CB_SELECTSTRING,0,(LPARAM)Texture);
#else
#ifdef WITH_DEVELOPER_MODE
		g_signal_handler_block(textures, handlerTextures);
		SelectComboBoxText(GTK_COMBO_BOX(textures), Texture);
		g_signal_handler_unblock(textures, handlerTextures);
#endif
#endif
		return;
	}
	SCopy(szTexture,Texture,C4M_MaxName);
	UpdatePreview();
}

bool C4ToolsDlg::SetIFT(bool fIFT)
{
	if (fIFT) ModeIFT = 1; else ModeIFT=0;
	UpdateIFTControls();
	UpdatePreview();
	return true;
}

void C4ToolsDlg::SetColorPattern(const char *szMaterial, const char *szTexture)
{
}

void C4ToolsDlg::UpdatePreview()
{
#ifdef _WIN32
	if (!hDialog) return;
#else
#ifdef WITH_DEVELOPER_MODE
	if (!hbox) return;
#endif
#endif

	SURFACE sfcPreview;

	int32_t iPrvWdt,iPrvHgt;

	RECT rect;
#ifdef _WIN32
	GetClientRect(GetDlgItem(hDialog,IDC_PREVIEW),&rect);
#else
	/* TODO: Set size request for image to read size from image's size request? */
	rect.left = 0;
	rect.top = 0;
	rect.bottom = 64;
	rect.right = 64;
#endif

	iPrvWdt=rect.right-rect.left;
	iPrvHgt=rect.bottom-rect.top;

	if (!(sfcPreview=new CSurface(iPrvWdt,iPrvHgt))) return;

	// fill bg
#ifdef _WIN32
	Application.DDraw->DrawBoxDw(sfcPreview,0,0,iPrvWdt-1,iPrvHgt-1,C4RGB(0x80,0x80,0x80));
#endif
	BYTE bCol = 0;
	CPattern Pattern;
	// Sky material: sky as pattern only
	if (SEqual(Material,C4TLS_MatSky))
	{
		Pattern.Set(::Landscape.Sky.Surface, 0);
	}
	// Material-Texture
	else
	{
		bCol=Mat2PixColDefault(::MaterialMap.Get(Material));
		// Get/Create TexMap entry
		BYTE iTex = ::TextureMap.GetIndex(Material, Texture, true);
		if (iTex)
		{
			// Define texture pattern
			const C4TexMapEntry *pTex = ::TextureMap.GetEntry(iTex);
			// Security
			if (pTex)
			{
				// Set drawing pattern
				Pattern = pTex->GetPattern();
			}
		}
	}
#ifdef _WIN32
	if (IsWindowEnabled(GetDlgItem(hDialog,IDC_PREVIEW)))
#else
#ifdef WITH_DEVELOPER_MODE
	if (GTK_WIDGET_SENSITIVE(preview))
#endif
#endif
		Application.DDraw->DrawPatternedCircle( sfcPreview,
		                                        iPrvWdt/2,iPrvHgt/2,
		                                        Grade,
		                                        bCol, Pattern, *::Landscape.GetPal());

#ifdef _WIN32
#ifdef USE_DIRECTX
	if (pD3D)
		pD3D->BlitSurface2Window( sfcPreview,
		                          0,0,iPrvWdt,iPrvHgt,
		                          GetDlgItem(hDialog,IDC_PREVIEW),
		                          rect.left,rect.top,rect.right,rect.bottom);
#endif
#ifdef USE_GL
	if (pGL && pGLCtx)
	{
		if (pGLCtx->Select())
		{
			pGL->Blit(sfcPreview, 0,0,(float)iPrvWdt,(float)iPrvHgt, Application.pWindow->pSurface, rect.left,rect.top, iPrvWdt,iPrvHgt);
			Application.pWindow->pSurface->PageFlip();
		}
	}
#endif
#else
#ifdef WITH_DEVELOPER_MODE
	// TODO: Can we optimize this?
	GdkPixbuf* pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, true, 8, 64, 64);
	guchar* data = gdk_pixbuf_get_pixels(pixbuf);
	sfcPreview->Lock();
	for (int x = 0; x < 64; ++ x) for (int y = 0; y < 64; ++ y)
		{
			DWORD dw = sfcPreview->GetPixDw(x, y, true);
			*data = (dw >> 16) & 0xff; ++ data;
			*data = (dw >> 8 ) & 0xff; ++ data;
			*data = (dw      ) & 0xff; ++ data;
			*data = 0xff - ((dw >> 24) & 0xff); ++ data;
		}

	sfcPreview->Unlock();
	gtk_image_set_from_pixbuf(GTK_IMAGE(preview), pixbuf);
	gdk_pixbuf_unref(pixbuf);
#endif
#endif
	delete sfcPreview;
}

void C4ToolsDlg::InitGradeCtrl()
{
#ifdef _WIN32
	if (!hDialog) return;
	HWND hwndTrack = GetDlgItem(hDialog,IDC_SLIDERGRADE);
	SendMessage(hwndTrack,TBM_SETPAGESIZE,0,(LPARAM)5);
	SendMessage(hwndTrack,TBM_SETLINESIZE,0,(LPARAM)1);
	SendMessage(hwndTrack,TBM_SETRANGE,(WPARAM)false,
	            (LPARAM) MAKELONG(C4TLS_GradeMin,C4TLS_GradeMax));
	SendMessage(hwndTrack,TBM_SETPOS,(WPARAM)true,(LPARAM)C4TLS_GradeMax-Grade);
	UpdateWindow(hwndTrack);
#else
#ifdef WITH_DEVELOPER_MODE
	if (!hbox) return;
	g_signal_handler_block(scale, handlerScale);
	gtk_range_set_increments(GTK_RANGE(scale), 1, 5);
	gtk_range_set_range(GTK_RANGE(scale), C4TLS_GradeMin, C4TLS_GradeMax);
	gtk_scale_set_draw_value(GTK_SCALE(scale), false);
	gtk_range_set_value(GTK_RANGE(scale), C4TLS_GradeMax-Grade);
	g_signal_handler_unblock(scale, handlerScale);
#endif
#endif
}

bool C4ToolsDlg::SetGrade(int32_t iGrade)
{
	Grade = BoundBy(iGrade,C4TLS_GradeMin,C4TLS_GradeMax);
	UpdatePreview();
	return true;
}

bool C4ToolsDlg::ChangeGrade(int32_t iChange)
{
	Grade = BoundBy(Grade+iChange,C4TLS_GradeMin,C4TLS_GradeMax);
	UpdatePreview();
	InitGradeCtrl();
	return true;
}

bool C4ToolsDlg::PopMaterial()
{
#ifdef _WIN32
	if (!hDialog) return false;
	SetFocus(GetDlgItem(hDialog,IDC_COMBOMATERIAL));
	SendDlgItemMessage(hDialog,IDC_COMBOMATERIAL,CB_SHOWDROPDOWN,true,0);
#else
#ifdef WITH_DEVELOPER_MODE
	if (!hbox) return false;
	gtk_widget_grab_focus(materials);
	gtk_combo_box_popup(GTK_COMBO_BOX(materials));
#endif
#endif
	return true;
}

bool C4ToolsDlg::PopTextures()
{
#ifdef _WIN32
	if (!hDialog) return false;
	SetFocus(GetDlgItem(hDialog,IDC_COMBOTEXTURE));
	SendDlgItemMessage(hDialog,IDC_COMBOTEXTURE,CB_SHOWDROPDOWN,true,0);
#else
#ifdef WITH_DEVELOPER_MODE
	if (!hbox) return false;
	gtk_widget_grab_focus(textures);
	gtk_combo_box_popup(GTK_COMBO_BOX(textures));
#endif
#endif
	return true;
}

void C4ToolsDlg::UpdateIFTControls()
{
#ifdef _WIN32
	if (!hDialog) return;
	SendDlgItemMessage(hDialog,IDC_BUTTONNOIFT,BM_SETSTATE,(ModeIFT==0),0);
	UpdateWindow(GetDlgItem(hDialog,IDC_BUTTONNOIFT));
	SendDlgItemMessage(hDialog,IDC_BUTTONIFT,BM_SETSTATE,(ModeIFT==1),0);
	UpdateWindow(GetDlgItem(hDialog,IDC_BUTTONIFT));
#else
#ifdef WITH_DEVELOPER_MODE
	if (!hbox) return;
	g_signal_handler_block(no_ift, handlerNoIft);
	g_signal_handler_block(ift, handlerIft);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(no_ift), ModeIFT==0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ift), ModeIFT==1);

	g_signal_handler_unblock(no_ift, handlerNoIft);
	g_signal_handler_unblock(ift, handlerIft);
#endif
#endif
}

void C4ToolsDlg::UpdateLandscapeModeCtrls()
{
	int32_t iMode = ::Landscape.Mode;
#ifdef _WIN32
	// Dynamic: enable only if dynamic anyway
	SendDlgItemMessage(hDialog,IDC_BUTTONMODEDYNAMIC,BM_SETSTATE,(iMode==C4LSC_Dynamic),0);
	EnableWindow(GetDlgItem(hDialog,IDC_BUTTONMODEDYNAMIC),(iMode==C4LSC_Dynamic));
	UpdateWindow(GetDlgItem(hDialog,IDC_BUTTONMODEDYNAMIC));
	// Static: enable only if map available
	SendDlgItemMessage(hDialog,IDC_BUTTONMODESTATIC,BM_SETSTATE,(iMode==C4LSC_Static),0);
	EnableWindow(GetDlgItem(hDialog,IDC_BUTTONMODESTATIC),(::Landscape.Map!=NULL));
	UpdateWindow(GetDlgItem(hDialog,IDC_BUTTONMODESTATIC));
	// Exact: enable always
	SendDlgItemMessage(hDialog,IDC_BUTTONMODEEXACT,BM_SETSTATE,(iMode==C4LSC_Exact),0);
	UpdateWindow(GetDlgItem(hDialog,IDC_BUTTONMODEEXACT));
	// Set dialog caption
	SetWindowText(hDialog,LoadResStr(iMode==C4LSC_Dynamic ? "IDS_DLG_DYNAMIC" : iMode==C4LSC_Static ? "IDS_DLG_STATIC" : "IDS_DLG_EXACT"));
#else
#ifdef WITH_DEVELOPER_MODE
	g_signal_handler_block(landscape_dynamic, handlerDynamic);
	g_signal_handler_block(landscape_static, handlerStatic);
	g_signal_handler_block(landscape_exact, handlerExact);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(landscape_dynamic), iMode==C4LSC_Dynamic);
	gtk_widget_set_sensitive(landscape_dynamic, iMode==C4LSC_Dynamic);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(landscape_static), iMode==C4LSC_Static);
	gtk_widget_set_sensitive(landscape_static, ::Landscape.Map!=NULL);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(landscape_exact), iMode==C4LSC_Exact);

	g_signal_handler_unblock(landscape_dynamic, handlerDynamic);
	g_signal_handler_unblock(landscape_static, handlerStatic);
	g_signal_handler_unblock(landscape_exact, handlerExact);

	C4DevmodeDlg::SetTitle(hbox, LoadResStr(iMode==C4LSC_Dynamic ? "IDS_DLG_DYNAMIC" : iMode==C4LSC_Static ? "IDS_DLG_STATIC" : "IDS_DLG_EXACT"));
#endif
#endif
}

bool C4ToolsDlg::SetLandscapeMode(int32_t iMode, bool fThroughControl)
{
	int32_t iLastMode=::Landscape.Mode;
	// Exact to static: confirm data loss warning
	if (iLastMode==C4LSC_Exact)
		if (iMode==C4LSC_Static)
			if (!fThroughControl)
				if (!Console.Message(LoadResStr("IDS_CNS_EXACTTOSTATIC"),true))
					return false;
	// send as control
	if (!fThroughControl)
	{
		::Control.DoInput(CID_EMDrawTool, new C4ControlEMDrawTool(EMDT_SetMode, iMode), CDT_Decide);
		return true;
	}
	// Set landscape mode
	::Landscape.SetMode(iMode);
	// Exact to static: redraw landscape from map
	if (iLastMode==C4LSC_Exact)
		if (iMode==C4LSC_Static)
			::Landscape.MapToLandscape();
	// Assert valid tool
	if (iMode!=C4LSC_Exact)
		if (SelectedTool==C4TLS_Fill)
			SetTool(C4TLS_Brush, false);
	// Update controls
	UpdateLandscapeModeCtrls();
	EnableControls();
	UpdateTextures();
	// Success
	return true;
}

void C4ToolsDlg::EnableControls()
{
	int32_t iLandscapeMode=::Landscape.Mode;
#ifdef _WIN32
	// Set bitmap buttons
	SendDlgItemMessage(hDialog,IDC_BUTTONBRUSH,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)((iLandscapeMode>=C4LSC_Static) ? hbmBrush : hbmBrush2));
	SendDlgItemMessage(hDialog,IDC_BUTTONLINE,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)((iLandscapeMode>=C4LSC_Static) ? hbmLine : hbmLine2));
	SendDlgItemMessage(hDialog,IDC_BUTTONRECT,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)((iLandscapeMode>=C4LSC_Static) ? hbmRect : hbmRect2));
	SendDlgItemMessage(hDialog,IDC_BUTTONFILL,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)((iLandscapeMode>=C4LSC_Exact) ? hbmFill : hbmFill2));
	SendDlgItemMessage(hDialog,IDC_BUTTONPICKER,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)((iLandscapeMode>=C4LSC_Static) ? hbmPicker : hbmPicker2));
	SendDlgItemMessage(hDialog,IDC_BUTTONIFT,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)hbmIFT);
	SendDlgItemMessage(hDialog,IDC_BUTTONNOIFT,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)hbmNoIFT);
	SendDlgItemMessage(hDialog,IDC_BUTTONMODEDYNAMIC,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)hbmDynamic);
	SendDlgItemMessage(hDialog,IDC_BUTTONMODESTATIC,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)hbmStatic);
	SendDlgItemMessage(hDialog,IDC_BUTTONMODEEXACT,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)hbmExact);
	// Enable drawing controls
	EnableWindow(GetDlgItem(hDialog,IDC_BUTTONBRUSH),(iLandscapeMode>=C4LSC_Static));
	EnableWindow(GetDlgItem(hDialog,IDC_BUTTONLINE),(iLandscapeMode>=C4LSC_Static));
	EnableWindow(GetDlgItem(hDialog,IDC_BUTTONRECT),(iLandscapeMode>=C4LSC_Static));
	EnableWindow(GetDlgItem(hDialog,IDC_BUTTONFILL),(iLandscapeMode>=C4LSC_Exact));
	EnableWindow(GetDlgItem(hDialog,IDC_BUTTONPICKER),(iLandscapeMode>=C4LSC_Static));
	EnableWindow(GetDlgItem(hDialog,IDC_BUTTONIFT),(iLandscapeMode>=C4LSC_Static));
	EnableWindow(GetDlgItem(hDialog,IDC_BUTTONNOIFT),(iLandscapeMode>=C4LSC_Static));
	EnableWindow(GetDlgItem(hDialog,IDC_COMBOMATERIAL),(iLandscapeMode>=C4LSC_Static));
	EnableWindow(GetDlgItem(hDialog,IDC_COMBOTEXTURE),(iLandscapeMode>=C4LSC_Static) && !SEqual(Material,C4TLS_MatSky));
	EnableWindow(GetDlgItem(hDialog,IDC_STATICMATERIAL),(iLandscapeMode>=C4LSC_Static));
	EnableWindow(GetDlgItem(hDialog,IDC_STATICTEXTURE),(iLandscapeMode>=C4LSC_Static) && !SEqual(Material,C4TLS_MatSky));
	EnableWindow(GetDlgItem(hDialog,IDC_SLIDERGRADE),(iLandscapeMode>=C4LSC_Static));
	EnableWindow(GetDlgItem(hDialog,IDC_PREVIEW),(iLandscapeMode>=C4LSC_Static));
#else
#ifdef WITH_DEVELOPER_MODE
	gtk_widget_set_sensitive(brush, iLandscapeMode>=C4LSC_Static);
	gtk_widget_set_sensitive(line, iLandscapeMode>=C4LSC_Static);
	gtk_widget_set_sensitive(rect, iLandscapeMode>=C4LSC_Static);
	gtk_widget_set_sensitive(fill, iLandscapeMode>=C4LSC_Exact);
	gtk_widget_set_sensitive(picker, iLandscapeMode>=C4LSC_Static);
	gtk_widget_set_sensitive(ift, iLandscapeMode>=C4LSC_Static);
	gtk_widget_set_sensitive(no_ift, iLandscapeMode>=C4LSC_Static);
	gtk_widget_set_sensitive(materials, (iLandscapeMode>=C4LSC_Static));
	gtk_widget_set_sensitive(textures, iLandscapeMode >= C4LSC_Static && !SEqual(Material,C4TLS_MatSky));
	gtk_widget_set_sensitive(scale, iLandscapeMode>=C4LSC_Static);
	gtk_widget_set_sensitive(preview, iLandscapeMode>=C4LSC_Static);
#endif // WITH_DEVELOPER_MODE
#endif // _WIN32
	UpdatePreview();
}

#ifdef _WIN32
void C4ToolsDlg::LoadBitmaps()
{
	HINSTANCE hInst = Application.GetInstance();
	if (!hbmBrush) hbmBrush=(HBITMAP)LoadBitmap(hInst,MAKEINTRESOURCE(IDB_BRUSH));
	if (!hbmLine) hbmLine=(HBITMAP)LoadBitmap(hInst,MAKEINTRESOURCE(IDB_LINE));
	if (!hbmRect) hbmRect=(HBITMAP)LoadBitmap(hInst,MAKEINTRESOURCE(IDB_RECT));
	if (!hbmFill) hbmFill=(HBITMAP)LoadBitmap(hInst,MAKEINTRESOURCE(IDB_FILL));
	if (!hbmPicker) hbmPicker=(HBITMAP)LoadBitmap(hInst,MAKEINTRESOURCE(IDB_PICKER));
	if (!hbmBrush2) hbmBrush2=(HBITMAP)LoadBitmap(hInst,MAKEINTRESOURCE(IDB_BRUSH2));
	if (!hbmLine2) hbmLine2=(HBITMAP)LoadBitmap(hInst,MAKEINTRESOURCE(IDB_LINE2));
	if (!hbmRect2) hbmRect2=(HBITMAP)LoadBitmap(hInst,MAKEINTRESOURCE(IDB_RECT2));
	if (!hbmFill2) hbmFill2=(HBITMAP)LoadBitmap(hInst,MAKEINTRESOURCE(IDB_FILL2));
	if (!hbmPicker2) hbmPicker2=(HBITMAP)LoadBitmap(hInst,MAKEINTRESOURCE(IDB_PICKER2));
	if (!hbmIFT) hbmIFT=(HBITMAP)LoadBitmap(hInst,MAKEINTRESOURCE(IDB_IFT));
	if (!hbmNoIFT) hbmNoIFT=(HBITMAP)LoadBitmap(hInst,MAKEINTRESOURCE(IDB_NOIFT));
	if (!hbmDynamic) hbmDynamic=(HBITMAP)LoadBitmap(hInst,MAKEINTRESOURCE(IDB_DYNAMIC));
	if (!hbmStatic) hbmStatic=(HBITMAP)LoadBitmap(hInst,MAKEINTRESOURCE(IDB_STATIC));
	if (!hbmExact) hbmExact=(HBITMAP)LoadBitmap(hInst,MAKEINTRESOURCE(IDB_EXACT));
}
#endif
void C4ToolsDlg::AssertValidTexture()
{
	// Static map mode only
	if (::Landscape.Mode!=C4LSC_Static) return;
	// Ignore if sky
	if (SEqual(Material,C4TLS_MatSky)) return;
	// Current material-texture valid
	if (::TextureMap.GetIndex(Material,Texture,false)) return;
	// Find valid material-texture
	const char *szTexture;
	for (int32_t iTexture=0; (szTexture=::TextureMap.GetTexture(iTexture)); iTexture++)
	{
		if (::TextureMap.GetIndex(Material,szTexture,false))
			{ SelectTexture(szTexture); return; }
	}
	// No valid texture found
}

bool C4ToolsDlg::SelectTexture(const char *szTexture)
{
#ifdef _WIN32
	SendDlgItemMessage(hDialog,IDC_COMBOTEXTURE,CB_SELECTSTRING,0,(LPARAM)szTexture);
#else
#ifdef WITH_DEVELOPER_MODE
	g_signal_handler_block(textures, handlerTextures);
	SelectComboBoxText(GTK_COMBO_BOX(textures), szTexture);
	g_signal_handler_unblock(textures, handlerTextures);
#endif
#endif
	SetTexture(szTexture);
	return true;
}

bool C4ToolsDlg::SelectMaterial(const char *szMaterial)
{
#ifdef _WIN32
	SendDlgItemMessage(hDialog,IDC_COMBOMATERIAL,CB_SELECTSTRING,0,(LPARAM)szMaterial);
#else
#ifdef WITH_DEVELOPER_MODE
	g_signal_handler_block(materials, handlerMaterials);
	SelectComboBoxText(GTK_COMBO_BOX(materials), szMaterial);
	g_signal_handler_unblock(materials, handlerMaterials);
#endif
#endif
	SetMaterial(szMaterial);
	return true;
}

void C4ToolsDlg::SetAlternateTool()
{
	// alternate tool is the picker in any mode
	SetTool(C4TLS_Picker, true);
}

void C4ToolsDlg::ResetAlternateTool()
{
	// reset tool to selected tool in case alternate tool was set
	SetTool(SelectedTool, true);
}

#ifdef WITH_DEVELOPER_MODE
// GTK+ callbacks
/*void C4ToolsDlg::OnDestroy(GtkWidget* widget, gpointer data)
{
  static_cast<C4ToolsDlg*>(data)->window = NULL;
  static_cast<C4ToolsDlg*>(data)->Active = false;
}*/

void C4ToolsDlg::OnButtonModeDynamic(GtkWidget* widget, gpointer data)
{
	static_cast<C4ToolsDlg*>(data)->SetLandscapeMode(C4LSC_Dynamic);
}

void C4ToolsDlg::OnButtonModeStatic(GtkWidget* widget, gpointer data)
{
	static_cast<C4ToolsDlg*>(data)->SetLandscapeMode(C4LSC_Static);
}

void C4ToolsDlg::OnButtonModeExact(GtkWidget* widget, gpointer data)
{
	static_cast<C4ToolsDlg*>(data)->SetLandscapeMode(C4LSC_Exact);
}

void C4ToolsDlg::OnButtonBrush(GtkWidget* widget, gpointer data)
{
	static_cast<C4ToolsDlg*>(data)->SetTool(C4TLS_Brush, false);
}

void C4ToolsDlg::OnButtonLine(GtkWidget* widget, gpointer data)
{
	static_cast<C4ToolsDlg*>(data)->SetTool(C4TLS_Line, false);
}

void C4ToolsDlg::OnButtonRect(GtkWidget* widget, gpointer data)
{
	static_cast<C4ToolsDlg*>(data)->SetTool(C4TLS_Rect, false);
}

void C4ToolsDlg::OnButtonFill(GtkWidget* widget, gpointer data)
{
	static_cast<C4ToolsDlg*>(data)->SetTool(C4TLS_Fill, false);
}

void C4ToolsDlg::OnButtonPicker(GtkWidget* widget, gpointer data)
{
	static_cast<C4ToolsDlg*>(data)->SetTool(C4TLS_Picker, false);
}

void C4ToolsDlg::OnButtonIft(GtkWidget* widget, gpointer data)
{
	static_cast<C4ToolsDlg*>(data)->SetIFT(true);
}

void C4ToolsDlg::OnButtonNoIft(GtkWidget* widget, gpointer data)
{
	static_cast<C4ToolsDlg*>(data)->SetIFT(false);
}

void C4ToolsDlg::OnComboMaterial(GtkWidget* widget, gpointer data)
{
	gchar* text = gtk_combo_box_get_active_text(GTK_COMBO_BOX(widget));
	static_cast<C4ToolsDlg*>(data)->SetMaterial(text);
	g_free(text);
}

void C4ToolsDlg::OnComboTexture(GtkWidget* widget, gpointer data)
{
	gchar* text = gtk_combo_box_get_active_text(GTK_COMBO_BOX(widget));
	static_cast<C4ToolsDlg*>(data)->SetTexture(text);
	g_free(text);
}

void C4ToolsDlg::OnGrade(GtkWidget* widget, gpointer data)
{
	C4ToolsDlg* dlg = static_cast<C4ToolsDlg*>(data);
	int value = static_cast<int>(gtk_range_get_value(GTK_RANGE(dlg->scale)) + 0.5);
	dlg->SetGrade(C4TLS_GradeMax-value);
}

void C4ToolsDlg::OnWindowHide(GtkWidget* widget, gpointer data)
{
	static_cast<C4ToolsDlg*>(data)->Active = false;
}
#endif
