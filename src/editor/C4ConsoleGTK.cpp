/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2002, 2005, Sven Eberhardt
 * Copyright (c) 2006-2007, Armin Burgmeier
 * Copyright (c) 2007, Günther Brammer
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

#include <C4Include.h>
#include <C4Console.h>

#include <C4ConsoleGTKDlg.h>
#include <C4Language.h>
#include <C4Aul.h>
#include <C4Application.h>
#include <C4GameSave.h>
#include <C4Game.h>
#include <C4MessageInput.h>
#include <C4Version.h>
#include <C4Language.h>
#include <C4Object.h>
#include <C4Player.h>
#include <C4Landscape.h>
#include <C4GraphicsSystem.h>
#include <C4PlayerList.h>
#include <C4GameControl.h>
#include <C4Texture.h>
#include <C4Viewport.h>

#include <StdFile.h>
#include <StdRegistry.h>

#include <gdk/gdkx.h>
#include <gtk/gtk.h>

using namespace OpenFileFlags;

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
}

class C4ConsoleGUI::State: public C4ConsoleGUI::InternalState<class C4ConsoleGUI>
{
public:

	GtkWidget* txtLog;
	GtkWidget* txtScript;
	GtkWidget* btnPlay;
	GtkWidget* btnHalt;
	GtkWidget* btnModePlay;
	GtkWidget* btnModeEdit;
	GtkWidget* btnModeDraw;

	GtkWidget* menuBar;
	GtkWidget* itemNet;
	GtkWidget* menuNet;

	GtkWidget* menuViewport;
	GtkWidget* menuPlayer;

	GtkWidget* fileOpen;
	GtkWidget* fileOpenWithPlayers;
	GtkWidget* fileSave;
	GtkWidget* fileSaveAs;
	GtkWidget* fileSaveGameAs;
	GtkWidget* fileRecord;
	GtkWidget* fileClose;
	GtkWidget* fileQuit;

	GtkWidget* compObjects;

	GtkWidget* plrJoin;

	GtkWidget* viewNew;

	GtkWidget* helpAbout;

	GtkWidget* statusBar;
	GtkWidget* lblFrame;
	GtkWidget* lblTime;

	GtkWidget* propertydlg;
	GtkWidget* propertydlg_textview;
	GtkAdjustment* propertydlg_vadj;
	double propertydlg_vadj_pos;
	GtkWidget* propertydlg_entry;

	gulong handlerDestroy;
	gulong handlerPlay;
	gulong handlerHalt;
	gulong handlerModePlay;
	gulong handlerModeEdit;
	gulong handlerModeDraw;
	guint handlerPropertyDlgRescrollIdle;

	State(C4ConsoleGUI *console): Super(console)
	{	
		Clear();
	}

	~State()
	{
		// This is just to be sure, it should not be necessary since
		// the widgets will be removed anyway as soon as the state is.
		if(handlerDestroy)
			g_signal_handler_disconnect(GetOwner()->window, handlerDestroy);
		if(handlerPlay)
			g_signal_handler_disconnect(btnPlay, handlerPlay);
		if(handlerHalt)
			g_signal_handler_disconnect(btnHalt, handlerHalt);
		if(handlerModePlay)
			g_signal_handler_disconnect(btnModePlay, handlerModePlay);
		if(handlerModeEdit)
			g_signal_handler_disconnect(btnModeEdit, handlerModeEdit);
		if(handlerModeDraw)
			g_signal_handler_disconnect(btnModeDraw, handlerModeDraw);
		if (handlerPropertyDlgRescrollIdle)
			g_source_remove(handlerPropertyDlgRescrollIdle);
		if (propertydlg)
		{
			C4DevmodeDlg::RemovePage(propertydlg);
			propertydlg = NULL;
		}
	}

	void InitGUI();
	void Clear();

	void DoEnableControls(bool fEnable);

	static void OnDestroy(GtkWidget* window, gpointer data);

	static void OnScriptEntry(GtkWidget* entry, gpointer data);
	static void OnPlay(GtkWidget* button, gpointer data);
	static void OnHalt(GtkWidget* button, gpointer data);
	static void OnModePlay(GtkWidget* button, gpointer data);
	static void OnModeEdit(GtkWidget* button, gpointer data);
	static void OnModeDraw(GtkWidget* button, gpointer data);

	static void OnFileOpen(GtkWidget* item, gpointer data);
	static void OnFileOpenWPlrs(GtkWidget* item, gpointer data);
	static void OnFileSave(GtkWidget* item, gpointer data);
	static void OnFileSaveAs(GtkWidget* item, gpointer data);
	static void OnFileSaveGameAs(GtkWidget* item, gpointer data);
	static void OnFileRecord(GtkWidget* item, gpointer data);
	static void OnFileClose(GtkWidget* item, gpointer data);
	static void OnFileQuit(GtkWidget* item, gpointer data);

	static void OnCompObjects(GtkWidget* item, gpointer data);

	static void OnPlrJoin(GtkWidget* item, gpointer data);
	static void OnPlrQuit(GtkWidget* item, gpointer data);
	static void OnViewNew(GtkWidget* item, gpointer data);
	static void OnViewNewPlr(GtkWidget* item, gpointer data);
	static void OnHelpAbout(GtkWidget* item, gpointer data);

	static void OnNetClient(GtkWidget* item, gpointer data);

	static void OnScriptActivate(GtkWidget* widget, gpointer data);

	static void OnPropertyVadjustmentChanged(GtkAdjustment* adj, gpointer data);
	static gboolean OnPropertyDlgRescrollIdle(gpointer data);
};

class C4ToolsDlg::State: public C4ConsoleGUI::InternalState<class C4ToolsDlg>
{
public:
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

	GtkWidget* fg_materials;
	GtkWidget* fg_textures;
	GtkWidget* bg_materials;
	GtkWidget* bg_textures;

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

	gulong handlerFgMaterials;
	gulong handlerFgTextures;
	gulong handlerBgMaterials;
	gulong handlerBgTextures;
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
	static void OnComboBgMaterial(GtkWidget* widget, gpointer data);
	static void OnComboBgTexture(GtkWidget* widget, gpointer data);
	static void OnGrade(GtkWidget* widget, gpointer data);
	static void OnWindowHide(GtkWidget* widget, gpointer data);
	
	State(C4ToolsDlg* dlg): Super(dlg), hbox(NULL) { }
	bool Open();
	void UpdateToolCtrls();
	void InitMaterialCtrls();
	void UpdatePreview();
	void UpdateLandscapeModeCtrls();
	void UpdateIFTControls();

	~State()
	{
		if (hbox != NULL)
		{
			g_signal_handler_disconnect(G_OBJECT(C4DevmodeDlg::GetWindow()), handlerHide);
			C4DevmodeDlg::RemovePage(hbox);
			hbox = NULL;
		}
	}

	void Clear() {}
	void Default() { GetOwner()->ModeBack = true;  }
};

void C4ConsoleGUI::State::OnScriptActivate(GtkWidget* widget, gpointer data)
{
	const gchar* text = gtk_entry_get_text(GTK_ENTRY(widget));
	if (text && text[0])
		Console.EditCursor.In(text);
}

void C4ConsoleGUI::State::OnPropertyVadjustmentChanged(GtkAdjustment* adj, gpointer data)
{
	C4ConsoleGUI* gui = static_cast<C4ConsoleGUI*>(data);
	State* state = gui->state;

	if (state->propertydlg_vadj_pos != -1.0)
		gtk_adjustment_set_value(adj, state->propertydlg_vadj_pos);
}

gboolean C4ConsoleGUI::State::OnPropertyDlgRescrollIdle(gpointer data)
{
	C4ConsoleGUI* gui = static_cast<C4ConsoleGUI*>(data);
	State* state = gui->state;

	state->propertydlg_vadj_pos = -1.0;
	state->handlerPropertyDlgRescrollIdle = 0;
	return FALSE;
}

C4Window* C4ConsoleGUI::CreateConsoleWindow(C4AbstractApp* pApp)
{
	C4Rect r(0, 0, 400, 350);
	C4Window* retval = C4Window::Init(C4Window::W_Console, pApp, LoadResStr("IDS_CNS_CONSOLE"), &r);
	state->InitGUI();
	UpdateHaltCtrls(true);
	EnableControls(fGameOpen);
	ClearViewportMenu();
	return retval;
}

void C4ConsoleGUI::State::InitGUI()
{
	// ------------ Play/Pause and Mode ---------------------
	GtkWidget* image_play = gtk_image_new_from_resource("/org/openclonk/engine/Play_Trans.png");
	GtkWidget* image_pause = gtk_image_new_from_resource("/org/openclonk/engine/Halt_Trans.png");

	GtkWidget* image_mode_play = gtk_image_new_from_resource("/org/openclonk/engine/Mouse_Trans.png");
	GtkWidget* image_mode_edit = gtk_image_new_from_resource("/org/openclonk/engine/Cursor_Trans.png");
	GtkWidget* image_mode_draw = gtk_image_new_from_resource("/org/openclonk/engine/Brush_Trans.png");

	btnPlay = GTK_WIDGET(gtk_toggle_tool_button_new());
	btnHalt = GTK_WIDGET(gtk_toggle_tool_button_new());
	btnModePlay = GTK_WIDGET(gtk_toggle_tool_button_new());
	btnModeEdit = GTK_WIDGET(gtk_toggle_tool_button_new());
	btnModeDraw = GTK_WIDGET(gtk_toggle_tool_button_new());

	gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(btnPlay), image_play);
	gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(btnHalt), image_pause);
	gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(btnModePlay), image_mode_play);
	gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(btnModeEdit), image_mode_edit);
	gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(btnModeDraw), image_mode_draw);

	GtkWidget* top_hbox = gtk_toolbar_new();

	gtk_toolbar_insert(GTK_TOOLBAR(top_hbox), GTK_TOOL_ITEM(btnPlay), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(top_hbox), GTK_TOOL_ITEM(btnHalt), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(top_hbox), gtk_separator_tool_item_new(), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(top_hbox), GTK_TOOL_ITEM(btnModePlay), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(top_hbox), GTK_TOOL_ITEM(btnModeEdit), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(top_hbox), GTK_TOOL_ITEM(btnModeDraw), -1);

	GtkToolItem * itm = gtk_tool_item_new();
	gtk_tool_item_set_expand(itm, TRUE);
	lblTime = gtk_label_new("00:00:00 (0 FPS)");
	gtk_container_add(GTK_CONTAINER(itm), lblTime);
	gtk_toolbar_insert(GTK_TOOLBAR(top_hbox), itm, -1);

	itm = gtk_tool_item_new();
	gtk_tool_item_set_expand(itm, TRUE);
	lblFrame = gtk_label_new("Frame: 0");
	gtk_container_add(GTK_CONTAINER(itm), lblFrame);
	gtk_toolbar_insert(GTK_TOOLBAR(top_hbox), itm, -1);

	// ------------ Statusbar ---------------------
	statusBar = gtk_statusbar_new();

	// ------------ Log view and script entry ---------------------
	GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);

	txtLog = gtk_text_view_new();
	txtScript = gtk_entry_new();

	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll), GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(txtLog), false);
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(txtLog), 2);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(txtLog), GTK_WRAP_CHAR);

	gtk_container_add(GTK_CONTAINER(scroll), txtLog);

	// ------------ Menu -------------------
	menuBar = gtk_menu_bar_new();

	GtkWidget* itemFile = gtk_menu_item_new_with_label(LoadResStr("IDS_MNU_FILE"));
	GtkWidget* itemComponents = gtk_menu_item_new_with_label(LoadResStr("IDS_MNU_COMPONENTS"));
	GtkWidget* itemPlayer = gtk_menu_item_new_with_label(LoadResStr("IDS_MNU_PLAYER"));
	GtkWidget* itemViewport = gtk_menu_item_new_with_label(LoadResStr("IDS_MNU_VIEWPORT"));
	GtkWidget* itemHelp = gtk_menu_item_new_with_label("?");

	gtk_menu_shell_append(GTK_MENU_SHELL(menuBar), itemFile);
	gtk_menu_shell_append(GTK_MENU_SHELL(menuBar), itemComponents);
	gtk_menu_shell_append(GTK_MENU_SHELL(menuBar), itemPlayer);
	gtk_menu_shell_append(GTK_MENU_SHELL(menuBar), itemViewport);
	gtk_menu_shell_append(GTK_MENU_SHELL(menuBar), itemHelp);

	GtkWidget* menuFile = gtk_menu_new();
	GtkWidget* menuComponents = gtk_menu_new();
	GtkWidget* menuHelp = gtk_menu_new();

	menuPlayer = gtk_menu_new();
	menuViewport = gtk_menu_new();

	gtk_menu_item_set_submenu(GTK_MENU_ITEM(itemFile), menuFile);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(itemComponents), menuComponents);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(itemPlayer), menuPlayer);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(itemViewport), menuViewport);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(itemHelp), menuHelp);

	fileOpen = gtk_menu_item_new_with_label(LoadResStr("IDS_MNU_OPEN"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menuFile), fileOpen);

	fileOpenWithPlayers = gtk_menu_item_new_with_label(LoadResStr("IDS_MNU_OPENWPLRS"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menuFile), fileOpenWithPlayers);

	gtk_menu_shell_append(GTK_MENU_SHELL(menuFile), GTK_WIDGET(gtk_separator_menu_item_new()));

	fileSave = gtk_menu_item_new_with_label(LoadResStr("IDS_MNU_SAVESCENARIO"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menuFile), fileSave);

	fileSaveAs = gtk_menu_item_new_with_label(LoadResStr("IDS_MNU_SAVESCENARIOAS"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menuFile), fileSaveAs);

	fileSaveGameAs = gtk_menu_item_new_with_label(LoadResStr("IDS_MNU_SAVEGAMEAS"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menuFile), fileSaveGameAs);

	fileRecord = gtk_menu_item_new_with_label(LoadResStr("IDS_MNU_RECORD"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menuFile), fileRecord);

	gtk_menu_shell_append(GTK_MENU_SHELL(menuFile), GTK_WIDGET(gtk_separator_menu_item_new()));

	fileClose = gtk_menu_item_new_with_label(LoadResStr("IDS_MNU_CLOSE"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menuFile), fileClose);

	gtk_menu_shell_append(GTK_MENU_SHELL(menuFile), GTK_WIDGET(gtk_separator_menu_item_new()));

	fileQuit = gtk_menu_item_new_with_label(LoadResStr("IDS_MNU_QUIT"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menuFile), fileQuit);

	compObjects = gtk_menu_item_new_with_label(LoadResStr("IDS_BTN_OBJECTS"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menuComponents), compObjects);

	plrJoin = gtk_menu_item_new_with_label(LoadResStr("IDS_MNU_JOIN"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menuPlayer), plrJoin);

	viewNew = gtk_menu_item_new_with_label(LoadResStr("IDS_MNU_NEW"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menuViewport), viewNew);

	helpAbout = gtk_menu_item_new_with_label(LoadResStr("IDS_MENU_ABOUT"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menuHelp), helpAbout);

	// ------------ Window ---------------------
	GtkWidget* box = gtk_grid_new();
	gtk_orientable_set_orientation (GTK_ORIENTABLE(box), GTK_ORIENTATION_VERTICAL);

	gtk_container_add(GTK_CONTAINER(box), menuBar);
	gtk_container_add(GTK_CONTAINER(box), top_hbox);
	gtk_widget_set_vexpand(scroll, true);
	gtk_widget_set_hexpand(scroll, true);
	gtk_container_add(GTK_CONTAINER(box), scroll);
	gtk_widget_set_margin_top(txtScript, 3);
	gtk_widget_set_margin_bottom(txtScript, 3);
	gtk_container_add(GTK_CONTAINER(box), txtScript);
	gtk_container_add(GTK_CONTAINER(box), statusBar);

	gtk_container_add(GTK_CONTAINER(GetOwner()->window), box);
	gtk_widget_show_all(GTK_WIDGET(GetOwner()->window));

	// ------------ Signals ---------------------
	handlerDestroy = g_signal_connect(G_OBJECT(GetOwner()->window), "destroy", G_CALLBACK(OnDestroy), this);
	handlerPlay = g_signal_connect(G_OBJECT(btnPlay), "toggled", G_CALLBACK(OnPlay), this);
	handlerHalt = g_signal_connect(G_OBJECT(btnHalt), "toggled", G_CALLBACK(OnHalt), this);
	handlerModePlay = g_signal_connect(G_OBJECT(btnModePlay), "toggled", G_CALLBACK(OnModePlay), this);
	handlerModeEdit = g_signal_connect(G_OBJECT(btnModeEdit), "toggled", G_CALLBACK(OnModeEdit), this);
	handlerModeDraw = g_signal_connect(G_OBJECT(btnModeDraw), "toggled", G_CALLBACK(OnModeDraw), this);
	g_signal_connect(G_OBJECT(txtScript), "activate", G_CALLBACK(OnScriptEntry), this);
	g_signal_connect(G_OBJECT(fileOpen), "activate", G_CALLBACK(OnFileOpen), this);
	g_signal_connect(G_OBJECT(fileOpenWithPlayers), "activate", G_CALLBACK(OnFileOpenWPlrs), this);
	g_signal_connect(G_OBJECT(fileSave), "activate", G_CALLBACK(OnFileSave), this);
	g_signal_connect(G_OBJECT(fileSaveAs), "activate", G_CALLBACK(OnFileSaveAs), this);
	g_signal_connect(G_OBJECT(fileSaveGameAs), "activate", G_CALLBACK(OnFileSaveGameAs), this);
	g_signal_connect(G_OBJECT(fileRecord), "activate", G_CALLBACK(OnFileRecord), this);
	g_signal_connect(G_OBJECT(fileClose), "activate", G_CALLBACK(OnFileClose), this);
	g_signal_connect(G_OBJECT(fileQuit), "activate", G_CALLBACK(OnFileQuit), this);
	g_signal_connect(G_OBJECT(compObjects), "activate", G_CALLBACK(OnCompObjects), this);
	g_signal_connect(G_OBJECT(plrJoin), "activate", G_CALLBACK(OnPlrJoin), this);
	g_signal_connect(G_OBJECT(viewNew), "activate", G_CALLBACK(OnViewNew), this);
	g_signal_connect(G_OBJECT(helpAbout), "activate", G_CALLBACK(OnHelpAbout), this);
}

void C4ConsoleGUI::State::Clear()
{
	// Clear widget pointers
	txtLog = NULL;
	txtScript = NULL;
	btnPlay = NULL;
	btnHalt = NULL;
	btnModePlay = NULL;
	btnModeEdit = NULL;
	btnModeDraw = NULL;

	menuBar = NULL;
	itemNet = NULL;
	menuNet = NULL;

	menuViewport = NULL;
	menuPlayer = NULL;

	fileOpen = NULL;
	fileOpenWithPlayers = NULL;
	fileSave = NULL;
	fileSaveAs = NULL;
	fileSaveGameAs = NULL;
	fileRecord = NULL;
	fileClose = NULL;
	fileQuit = NULL;

	compObjects = NULL;

	plrJoin = NULL;

	viewNew = NULL;

	helpAbout = NULL;

	statusBar = NULL;
	lblFrame = NULL;
	lblTime = NULL;

	handlerDestroy = 0;
	handlerPlay = 0;
	handlerHalt = 0;
	handlerModePlay = 0;
	handlerModeEdit = 0;
	handlerModeDraw = 0;
	handlerPropertyDlgRescrollIdle = 0;

	propertydlg = 0;
}

void C4ConsoleGUI::DisplayInfoText(InfoTextType type, StdStrBuf& text)
{		
	if (!Active)
		return;
	GtkWidget* label;
	switch (type)
	{
	case CONSOLE_Cursor:
		gtk_statusbar_pop(GTK_STATUSBAR(state->statusBar), 0);
		gtk_statusbar_push(GTK_STATUSBAR(state->statusBar), 0, text.getData());
		return;
	case CONSOLE_FrameCounter:
		label = state->lblFrame;
		break;
	case CONSOLE_TimeFPS:
		label = state->lblTime;
	}
	gtk_label_set_label(GTK_LABEL(label), text.getData());
}

void C4ConsoleGUI::AddMenuItemForPlayer(C4Player *player, StdStrBuf &player_text)
{
	GtkWidget* menuItem = gtk_menu_item_new_with_label(player_text.getData());
	gtk_menu_shell_append(GTK_MENU_SHELL(state->menuViewport), menuItem);
	g_signal_connect(G_OBJECT(menuItem), "activate", G_CALLBACK(State::OnViewNewPlr), GINT_TO_POINTER(player->Number));
	gtk_widget_show(menuItem);
}

void C4ConsoleGUI::SetCursor(Cursor cursor)
{
	// Seems not to work. Don't know why...
	GdkDisplay * display = gtk_widget_get_display(GTK_WIDGET(window));
	GdkCursor * gdkcursor;

	if (cursor == CURSOR_Wait)
		gdkcursor = gdk_cursor_new_for_display(display, GDK_WATCH);
	else
		gdkcursor = NULL;

	GdkWindow* window_wnd = gtk_widget_get_window(GTK_WIDGET(window));

	gdk_window_set_cursor(window_wnd, gdkcursor);
	gdk_display_flush(display);
	if (cursor)
		g_object_unref (gdkcursor);
}

void C4ConsoleGUI::ClearViewportMenu()
{
	// Don't need to do anything if the GUI is not created
	if(state->menuViewport == NULL) return;

	GList* children = gtk_container_get_children(GTK_CONTAINER(state->menuViewport));
	for (GList* item = children; item != NULL; item = item->next)
	{
		if (item->data != state->viewNew)
			gtk_container_remove(GTK_CONTAINER(state->menuViewport), GTK_WIDGET(item->data));
	}
	g_list_free(children);
}

void C4ConsoleGUI::RecordingEnabled()
{
	gtk_widget_set_sensitive(state->fileRecord, false);
}

void C4ConsoleGUI::ShowAboutWithCopyright(StdStrBuf &copyright)
{
	gtk_show_about_dialog(GTK_WINDOW(window), "name", C4ENGINECAPTION, "version", C4VERSION, "copyright", copyright.getData(), NULL);
}

bool C4ConsoleGUI::UpdateModeCtrls(int iMode)
{
	if (!Active)
		return false;

	// Prevents recursion
	g_signal_handler_block(state->btnModePlay, state->handlerModePlay);
	g_signal_handler_block(state->btnModeEdit, state->handlerModeEdit);
	g_signal_handler_block(state->btnModeDraw, state->handlerModeDraw);

	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(state->btnModePlay), iMode == C4CNS_ModePlay);
	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(state->btnModeEdit), iMode == C4CNS_ModeEdit);
	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(state->btnModeDraw), iMode == C4CNS_ModeDraw);

	g_signal_handler_unblock(state->btnModePlay, state->handlerModePlay);
	g_signal_handler_unblock(state->btnModeEdit, state->handlerModeEdit);
	g_signal_handler_unblock(state->btnModeDraw, state->handlerModeDraw);
	return true;
}

bool C4ConsoleGUI::FileSelect(StdStrBuf *sFilename, const char * szFilter, DWORD dwFlags, bool fSave)
{
	GtkWidget* dialog = gtk_file_chooser_dialog_new(fSave ? "Save file..." : "Load file...", GTK_WINDOW(window), fSave ? GTK_FILE_CHOOSER_ACTION_SAVE : GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, fSave ? GTK_STOCK_SAVE : GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

	// TODO: Set dialog modal?

	if (g_path_is_absolute(sFilename->getData()) )
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), sFilename->getData());
	else if (fSave)
		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), sFilename->getData());

	// Install file filter
	while (*szFilter)
	{
		char pattern[16 + 1];

		GtkFileFilter* filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, szFilter);
		szFilter+=SLen(szFilter)+1;

		while (true)
		{
			SCopyUntil(szFilter, pattern, ';', 16);

			int len = SLen(pattern);
			char last_c = szFilter[len];

			szFilter += (len + 1);

			// Got not all of the filter, try again.
			if (last_c != ';' && last_c != '\0')
				continue;

			gtk_file_filter_add_pattern(filter, pattern);
			if (last_c == '\0') break;
		}

		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	}

	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), (dwFlags & OFN_OVERWRITEPROMPT) != 0);

	// TODO: Not in GTK+ 2.4, we could check GTK+ version at runtime and rely on lazy binding
//  gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(dialog), (dwFlags & OFN_HIDEREADONLY) == 0);

	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), (dwFlags & OFN_ALLOWMULTISELECT) != 0);
	gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dialog), true);

	int response;
	while (true)
	{
		response = gtk_dialog_run(GTK_DIALOG(dialog));
		if (response == GTK_RESPONSE_CANCEL || response == GTK_RESPONSE_DELETE_EVENT) break;

		bool error = false;

		// Check for OFN_FILEMUSTEXIST
		if ((dwFlags & OFN_ALLOWMULTISELECT) == 0)
		{
			char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

			if ((dwFlags & OFN_FILEMUSTEXIST) && !g_file_test(filename, G_FILE_TEST_IS_REGULAR))
			{
				Message(FormatString("File \"%s\" does not exist", filename).getData(), false);
				error = true;
			}

			g_free(filename);
		}

		if (!error) break;
	}

	if (response != GTK_RESPONSE_ACCEPT)
	{
		gtk_widget_destroy(dialog);
		return false;
	}

	// Build result string
	if ((dwFlags & OFN_ALLOWMULTISELECT) == 0)
	{
		// Just the file name without multiselect
		char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		sFilename->Copy(filename);
		g_free(filename);
	}
	else
	{
		// Otherwise its the folder followed by the file names,
		// separated by '\0'-bytes
		char* folder = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));

		sFilename->Copy(folder);
		g_free(folder);

		GSList* files = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
		for (GSList* item = files; item != NULL; item = item->next)
		{
			const char* file = static_cast<const char*>(item->data);
			char* basefile = g_path_get_basename(file);

			sFilename->AppendChar('\0');
			sFilename->Append(basefile);

			g_free(basefile);
			g_free(item->data);
		}

		// End of list
		g_slist_free(files);
	}

	gtk_widget_destroy(dialog);
	return true;
}

bool C4ConsoleGUI::Message(const char *message, bool query)
{
	if (!Active) return false;
	GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, query ? (GTK_BUTTONS_OK_CANCEL) : (GTK_BUTTONS_OK), "%s", message);
	int response = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	return response == GTK_RESPONSE_OK;
}

void C4ConsoleGUI::Out(const char *message)
{
	// Append text to log
	if (!window) return;

	GtkTextIter end;
	GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->txtLog));
	gtk_text_buffer_get_end_iter(buffer, &end);

	gtk_text_buffer_insert(buffer, &end, message, -1);
	gtk_text_buffer_insert(buffer, &end, "\n", 1);

	gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(state->txtLog), gtk_text_buffer_get_insert(buffer), 0.0, false, 0.0, 0.0);
}

void C4ConsoleGUI::AddNetMenu()
{
	state->itemNet = gtk_menu_item_new_with_label(LoadResStr("IDS_MNU_NET"));
	state->menuNet = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(state->itemNet), state->menuNet);
	gtk_menu_shell_insert(GTK_MENU_SHELL(state->menuBar), state->itemNet, 4 /*MenuIndexHelp*/);
	gtk_widget_show_all(state->itemNet);
}

void C4ConsoleGUI::AddNetMenuItemForPlayer(int32_t index, StdStrBuf &text)
{
	GtkWidget* item = gtk_menu_item_new_with_label(text.getData());
	gtk_menu_shell_append(GTK_MENU_SHELL(state->menuNet), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(State::OnNetClient), GINT_TO_POINTER(Game.Clients.getLocalID()));
	gtk_widget_show_all(item);
}

void C4ConsoleGUI::ClearNetMenu()
{
	// Don't need to do anything if the GUI is not created
	if(state->menuBar == NULL || state->itemNet == NULL) return;

	gtk_container_remove(GTK_CONTAINER(state->menuBar), state->itemNet);
	state->itemNet = NULL;
}

void C4ConsoleGUI::SetInputFunctions(std::list<const char*>& functions)
{
	// Don't need to do anything if the GUI is not created
	if(state->txtScript == NULL) return;

	GtkEntryCompletion* completion = gtk_entry_get_completion(GTK_ENTRY(state->txtScript));
	if (!completion)
	{
		completion = gtk_entry_completion_new();
		GtkListStore* store = gtk_list_store_new(1, G_TYPE_STRING);
		gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(store));
		g_object_unref(G_OBJECT(store));
		gtk_entry_completion_set_text_column(completion, 0);
		gtk_entry_set_completion(GTK_ENTRY(state->txtScript), completion);
		g_object_unref(G_OBJECT(completion));
	}

	GtkListStore* store = GTK_LIST_STORE(gtk_entry_completion_get_model(completion));
	g_assert(store);
	gtk_list_store_clear(store);

	GtkTreeIter iter;
	for (std::list<const char*>::iterator it(functions.begin()); it != functions.end(); ++it)
	{
		const char* fn = *it;
		if (!fn) continue;
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 0, fn, -1);
	}
}

void C4ConsoleGUI::ClearPlayerMenu()
{
	// Don't need to do anything if the GUI is not created
	if(state->menuPlayer == NULL) return;

	GList* children = gtk_container_get_children(GTK_CONTAINER(state->menuPlayer));
	for (GList* item = children; item != NULL; item = item->next)
	{
		if (item->data != state->plrJoin)
			gtk_container_remove(GTK_CONTAINER(state->menuPlayer), GTK_WIDGET(item->data));
	}
	g_list_free(children);
}

void C4ConsoleGUI::AddKickPlayerMenuItem(C4Player *player, StdStrBuf& player_text, bool enabled)
{
	// TODO: Implement AddMenuItem...
	GtkWidget* menuItem = gtk_menu_item_new_with_label(player_text.getData());
	gtk_menu_shell_append(GTK_MENU_SHELL(state->menuPlayer), menuItem);
	g_signal_connect(G_OBJECT(menuItem), "activate", G_CALLBACK(State::OnPlrQuit), GINT_TO_POINTER(player->Number));
	gtk_widget_show(menuItem);

	gtk_widget_set_sensitive(menuItem, (!::Network.isEnabled() || ::Network.isHost()) && Editing);
}

bool C4ConsoleGUI::ClearLog()
{
	if(state->txtLog == NULL) return false;

	gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->txtLog)), "", 0);
	return true;
}

bool C4ConsoleGUI::DoUpdateHaltCtrls(bool fHalt)
{
	// Prevents recursion
	g_signal_handler_block(state->btnPlay, state->handlerPlay);
	g_signal_handler_block(state->btnHalt, state->handlerHalt);

	//gtk_widget_set_sensitive(btnPlay, fHalt);
	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(state->btnPlay), !fHalt);
	//gtk_widget_set_sensitive(btnHalt, !fHalt);
	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(state->btnHalt), fHalt);

	g_signal_handler_unblock(state->btnPlay, state->handlerPlay);
	g_signal_handler_unblock(state->btnHalt, state->handlerHalt);
	return true;
}

void C4ConsoleGUI::ToolsDlgSelectTexture(C4ToolsDlg *dlg, const char *texture)
{
	C4ToolsDlg::State* state = dlg->state;
	g_signal_handler_block(state->fg_textures, state->handlerFgTextures);
	SelectComboBoxText(GTK_COMBO_BOX(state->fg_textures), texture);
	g_signal_handler_unblock(state->fg_textures, state->handlerFgTextures);
}

void C4ConsoleGUI::ToolsDlgSelectMaterial(C4ToolsDlg *dlg, const char *material)
{
	C4ToolsDlg::State* state = dlg->state;
	g_signal_handler_block(state->fg_materials, state->handlerFgMaterials);
	SelectComboBoxText(GTK_COMBO_BOX(state->fg_materials), material);
	g_signal_handler_unblock(state->fg_materials, state->handlerFgMaterials);
}

void C4ConsoleGUI::ToolsDlgSelectBackTexture(C4ToolsDlg *dlg, const char *texture)
{
	C4ToolsDlg::State* state = dlg->state;
	g_signal_handler_block(state->bg_textures, state->handlerBgTextures);
	SelectComboBoxText(GTK_COMBO_BOX(state->bg_textures), texture);
	g_signal_handler_unblock(state->bg_textures, state->handlerBgTextures);
}

void C4ConsoleGUI::ToolsDlgSelectBackMaterial(C4ToolsDlg *dlg, const char *material)
{
	C4ToolsDlg::State* state = dlg->state;
	g_signal_handler_block(state->bg_materials, state->handlerBgMaterials);
	SelectComboBoxText(GTK_COMBO_BOX(state->bg_materials), material);
	g_signal_handler_unblock(state->bg_materials, state->handlerBgMaterials);
}

void C4ConsoleGUI::DoEnableControls(bool fEnable)
{
	state->DoEnableControls(fEnable);
}

void C4ConsoleGUI::State::DoEnableControls(bool fEnable)
{
	// Halt controls
	gtk_widget_set_sensitive(btnPlay, ::Network.isLobbyActive() || fEnable);
	gtk_widget_set_sensitive(btnHalt, ::Network.isLobbyActive() || fEnable);

	// Edit modes
	gtk_widget_set_sensitive(btnModePlay, ::Network.isLobbyActive() || fEnable);
	gtk_widget_set_sensitive(btnModeEdit, ::Network.isLobbyActive() || fEnable);
	gtk_widget_set_sensitive(btnModeDraw, ::Network.isLobbyActive() || fEnable);

	// Console input
	gtk_widget_set_sensitive(txtScript, ::Network.isLobbyActive() || fEnable);

	// File menu
	// C4Network2 will have to handle that cases somehow (TODO: test)
	gtk_widget_set_sensitive(fileRecord, Game.IsRunning && ::Control.IsRuntimeRecordPossible());
	gtk_widget_set_sensitive(fileSaveGameAs, fEnable && ::Players.GetCount());
	gtk_widget_set_sensitive(fileSave, fEnable);
	gtk_widget_set_sensitive(fileSaveAs, fEnable);
	gtk_widget_set_sensitive(fileClose, fEnable);

	// Components menu
	gtk_widget_set_sensitive(compObjects, fEnable && GetOwner()->Editing);
	// Player & viewport menu
	gtk_widget_set_sensitive(plrJoin, fEnable && GetOwner()->Editing);
	gtk_widget_set_sensitive(viewNew, fEnable);
}

bool C4ConsoleGUI::PropertyDlgOpen()
{
	if (state->propertydlg == NULL)
	{
		GtkWidget * vbox = state->propertydlg = gtk_grid_new();
		gtk_orientable_set_orientation(GTK_ORIENTABLE(vbox), GTK_ORIENTATION_VERTICAL);

		GtkWidget* scrolled_wnd = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_wnd), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
		gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_wnd), GTK_SHADOW_IN);
		GtkAdjustment* adj = state->propertydlg_vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolled_wnd));
		state->propertydlg_vadj_pos = -1.0;

		g_signal_connect(G_OBJECT(adj), "changed", G_CALLBACK(State::OnPropertyVadjustmentChanged), this);

		GtkWidget * textview = state->propertydlg_textview = gtk_text_view_new();
		GtkWidget * entry = state->propertydlg_entry = gtk_entry_new();

		gtk_container_add(GTK_CONTAINER(scrolled_wnd), textview);
		gtk_widget_set_vexpand(scrolled_wnd, true);
		gtk_widget_set_hexpand(scrolled_wnd, true);
		gtk_container_add(GTK_CONTAINER(vbox), scrolled_wnd);
		gtk_widget_set_margin_top(entry, 3);
		gtk_container_add(GTK_CONTAINER(vbox), entry);

		gtk_text_view_set_editable(GTK_TEXT_VIEW(textview), false);
		gtk_text_view_set_left_margin(GTK_TEXT_VIEW(textview), 2);
		gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textview), GTK_WRAP_CHAR);
		gtk_widget_set_sensitive(entry, Console.Editing);

		gtk_widget_show_all(vbox);

		C4DevmodeDlg::AddPage(vbox, GTK_WINDOW(Console.window), LoadResStr("IDS_DLG_PROPERTIES"));

		g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(State::OnScriptActivate), this);
	}

	C4DevmodeDlg::SwitchPage(state->propertydlg);
	return true;
}

void C4ConsoleGUI::PropertyDlgClose()
{
}

void C4ConsoleGUI::PropertyDlgUpdate(C4ObjectList &rSelection, bool force_function_update)
{
	if (!state->propertydlg) return;
	if (!C4DevmodeDlg::GetWindow()) return;
	if (!gtk_widget_get_visible(GTK_WIDGET(C4DevmodeDlg::GetWindow()))) return;

	// Remember current scroll position
	if (PropertyDlgObject == rSelection.GetObject())
	{
		state->propertydlg_vadj_pos = gtk_adjustment_get_value(state->propertydlg_vadj);
		state->handlerPropertyDlgRescrollIdle = g_idle_add_full(GTK_TEXT_VIEW_PRIORITY_VALIDATE + 1, State::OnPropertyDlgRescrollIdle, this, NULL);
	}
	else
	{
		state->propertydlg_vadj_pos = -1.0;
		// TODO: Reset idle handler?
	}

	GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->propertydlg_textview));
	gtk_text_buffer_set_text(buffer, rSelection.GetDataString().getData(), -1);

	if (PropertyDlgObject == rSelection.GetObject() && !force_function_update) return;
	PropertyDlgObject = rSelection.GetObject();
	
	std::list<const char *> functions = ::Console.GetScriptSuggestions(PropertyDlgObject, C4Console::MRU_Object);
	GtkEntryCompletion* completion = gtk_entry_get_completion(GTK_ENTRY(state->propertydlg_entry));
	GtkListStore* store;

	// Uncouple list store from completion so that the completion is not
	// notified for every row we are going to insert. This enhances
	// performance significantly.
	if (!completion)
	{
		completion = gtk_entry_completion_new();
		store = gtk_list_store_new(1, G_TYPE_STRING);

		gtk_entry_completion_set_text_column(completion, 0);
		gtk_entry_set_completion(GTK_ENTRY(state->propertydlg_entry), completion);
		g_object_unref(G_OBJECT(completion));
	}
	else
	{
		store = GTK_LIST_STORE(gtk_entry_completion_get_model(completion));
		g_object_ref(G_OBJECT(store));
		gtk_entry_completion_set_model(completion, NULL);
	}

	GtkTreeIter iter;
	gtk_list_store_clear(store);

	for (std::list<const char*>::iterator it(functions.begin()); it != functions.end(); it++)
	{
		const char* fn = *it;
		if (fn)
		{
			gtk_list_store_append(store, &iter);
			gtk_list_store_set(store, &iter, 0, fn, -1);
		}
	}

	// Reassociate list store with completion
	gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(store));
}

bool C4ConsoleGUI::ToolsDlgOpen(C4ToolsDlg *dlg)
{
	return dlg->state->Open();
}

bool C4ToolsDlg::State::Open()
{
	if (hbox == NULL)
	{
		hbox = gtk_grid_new();
		GtkWidget * toolbar = gtk_toolbar_new();

		GtkWidget* image_brush = gtk_image_new_from_resource("/org/openclonk/engine/Brush_Trans.png");
		GtkWidget* image_line = gtk_image_new_from_resource("/org/openclonk/engine/Line_Trans.png");
		GtkWidget* image_rect = gtk_image_new_from_resource("/org/openclonk/engine/Rect_Trans.png");
		GtkWidget* image_fill = gtk_image_new_from_resource("/org/openclonk/engine/Fill_Trans.png");
		GtkWidget* image_picker = gtk_image_new_from_resource("/org/openclonk/engine/Picker_Trans.png");
		GtkWidget* image_dynamic = gtk_image_new_from_resource("/org/openclonk/engine/Dynamic_Trans.png");
		GtkWidget* image_static = gtk_image_new_from_resource("/org/openclonk/engine/Static_Trans.png");
		GtkWidget* image_exact = gtk_image_new_from_resource("/org/openclonk/engine/Exact_Trans.png");

		brush = GTK_WIDGET(gtk_toggle_tool_button_new());
		line = GTK_WIDGET(gtk_toggle_tool_button_new());
		rect = GTK_WIDGET(gtk_toggle_tool_button_new());
		fill = GTK_WIDGET(gtk_toggle_tool_button_new());
		picker = GTK_WIDGET(gtk_toggle_tool_button_new());
		landscape_dynamic = GTK_WIDGET(gtk_toggle_tool_button_new());
		landscape_static = GTK_WIDGET(gtk_toggle_tool_button_new());
		landscape_exact = GTK_WIDGET(gtk_toggle_tool_button_new());

		gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(landscape_dynamic), image_dynamic);
		gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(landscape_static), image_static);
		gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(landscape_exact), image_exact);
		gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(brush), image_brush);
		gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(line), image_line);
		gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(rect), image_rect);
		gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(fill), image_fill);
		gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(picker), image_picker);

		gtk_container_add(GTK_CONTAINER(toolbar), landscape_dynamic);
		gtk_container_add(GTK_CONTAINER(toolbar), landscape_static);
		gtk_container_add(GTK_CONTAINER(toolbar), landscape_exact);
		gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(gtk_separator_tool_item_new()));
		gtk_container_add(GTK_CONTAINER(toolbar), brush);
		gtk_container_add(GTK_CONTAINER(toolbar), line);
		gtk_container_add(GTK_CONTAINER(toolbar), rect);
		gtk_container_add(GTK_CONTAINER(toolbar), fill);
		gtk_container_add(GTK_CONTAINER(toolbar), picker);

		gtk_grid_attach(GTK_GRID(hbox), toolbar, 0, 0, 5, 1);

		preview = gtk_image_new();
		gtk_widget_set_vexpand(preview, true);
		gtk_widget_set_hexpand(preview, true);
		gtk_grid_attach(GTK_GRID(hbox), preview, 0, 1, 1, 1);

		scale = gtk_scale_new(GTK_ORIENTATION_VERTICAL, NULL);
		gtk_widget_set_vexpand(scale, true);
		gtk_grid_attach(GTK_GRID(hbox), scale, 1, 1, 1, 1);

		GtkWidget * grid = gtk_grid_new();
		fg_materials = gtk_combo_box_text_new();
		g_object_set(fg_materials, "margin", 3, NULL);
		fg_textures = gtk_combo_box_text_new();
		g_object_set(fg_textures, "margin", 3, NULL);
		bg_materials = gtk_combo_box_text_new();
		g_object_set(bg_materials, "margin", 3, NULL);
		bg_textures = gtk_combo_box_text_new();
		g_object_set(bg_textures, "margin", 3, NULL);

		// Link the material combo boxes together, but not the texture combo boxes,
		// so that we can sort the texture combo box differently.
		gtk_combo_box_set_model(GTK_COMBO_BOX(bg_materials), gtk_combo_box_get_model(GTK_COMBO_BOX(fg_materials)));

		gtk_combo_box_set_row_separator_func(GTK_COMBO_BOX(fg_materials), RowSeparatorFunc, NULL, NULL);
		gtk_combo_box_set_row_separator_func(GTK_COMBO_BOX(fg_textures), RowSeparatorFunc, NULL, NULL);
		gtk_combo_box_set_row_separator_func(GTK_COMBO_BOX(bg_materials), RowSeparatorFunc, NULL, NULL);
		gtk_combo_box_set_row_separator_func(GTK_COMBO_BOX(bg_textures), RowSeparatorFunc, NULL, NULL);

		gtk_grid_attach(GTK_GRID(grid), gtk_label_new(LoadResStr("IDS_CTL_MATERIAL")), 1, 0, 1, 1);
		gtk_grid_attach(GTK_GRID(grid), gtk_label_new(LoadResStr("IDS_CTL_TEXTURE")), 2, 0, 1, 1);

		GtkWidget* fg_lbl = gtk_label_new(LoadResStr("IDS_CTL_FOREGROUND"));
		gtk_widget_set_halign(fg_lbl, GTK_ALIGN_END);
#if GTK_CHECK_VERSION(3,12,0)
		gtk_widget_set_margin_start(fg_lbl, 3);
#else
		gtk_widget_set_margin_left(fg_lbl, 3);
#endif
		gtk_grid_attach(GTK_GRID(grid), fg_lbl, 0, 1, 1, 1);
		gtk_grid_attach(GTK_GRID(grid), fg_materials, 1, 1, 1, 1);
		gtk_grid_attach(GTK_GRID(grid), fg_textures, 2, 1, 1, 1);

		GtkWidget* bg_lbl = gtk_label_new(LoadResStr("IDS_CTL_BACKGROUND"));
		gtk_widget_set_halign(bg_lbl, GTK_ALIGN_END);
#if GTK_CHECK_VERSION(3,12,0)
		gtk_widget_set_margin_start(bg_lbl, 3);
#else
		gtk_widget_set_margin_left(bg_lbl, 3);
#endif
		gtk_grid_attach(GTK_GRID(grid), bg_lbl, 0, 2, 1, 1);
		gtk_grid_attach(GTK_GRID(grid), bg_materials, 1, 2, 1, 1);
		gtk_grid_attach(GTK_GRID(grid), bg_textures, 2, 2, 1, 1);

		gtk_grid_attach(GTK_GRID(hbox), grid, 2, 1, 1, 1);

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
		handlerFgMaterials = g_signal_connect(G_OBJECT(fg_materials), "changed", G_CALLBACK(OnComboMaterial), this);
		handlerFgTextures = g_signal_connect(G_OBJECT(fg_textures), "changed", G_CALLBACK(OnComboTexture), this);
		handlerBgMaterials = g_signal_connect(G_OBJECT(bg_materials), "changed", G_CALLBACK(OnComboBgMaterial), this);
		handlerBgTextures = g_signal_connect(G_OBJECT(bg_textures), "changed", G_CALLBACK(OnComboBgTexture), this);
		handlerScale = g_signal_connect(G_OBJECT(scale), "value-changed", G_CALLBACK(OnGrade), this);

		handlerHide = g_signal_connect(G_OBJECT(C4DevmodeDlg::GetWindow()), "hide", G_CALLBACK(OnWindowHide), this);
	}

	C4DevmodeDlg::SwitchPage(hbox);
	return true;
}

void C4ConsoleGUI::ToolsDlgInitMaterialCtrls(C4ToolsDlg *dlg)
{
	dlg->state->InitMaterialCtrls();
}

void C4ToolsDlg::State::InitMaterialCtrls()
{
	GtkListStore* list = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(fg_materials)));

	g_signal_handler_block(fg_materials, handlerFgMaterials);
	g_signal_handler_block(bg_materials, handlerBgMaterials);
	gtk_list_store_clear(list);

	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(fg_materials), C4TLS_MatSky);

	for (int32_t cnt = 0; cnt < ::MaterialMap.Num; cnt++)
	{
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(fg_materials), ::MaterialMap.Map[cnt].Name);
	}

	g_signal_handler_unblock(fg_materials, handlerFgMaterials);
	g_signal_handler_unblock(bg_materials, handlerBgMaterials);

	SelectComboBoxText(GTK_COMBO_BOX(fg_materials), GetOwner()->Material);
	SelectComboBoxText(GTK_COMBO_BOX(bg_materials), GetOwner()->BackMaterial);
}

void C4ToolsDlg::UpdateToolCtrls()
{
	state->UpdateToolCtrls();
}

void C4ToolsDlg::State::UpdateToolCtrls()
{
	C4ToolsDlg* dlg = GetOwner();
	g_signal_handler_block(brush, handlerBrush);
	g_signal_handler_block(line, handlerLine);
	g_signal_handler_block(rect, handlerRect);
	g_signal_handler_block(fill, handlerFill);
	g_signal_handler_block(picker, handlerPicker);

	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(brush), dlg->Tool == C4TLS_Brush);
	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(line), dlg->Tool == C4TLS_Line);
	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(rect), dlg->Tool == C4TLS_Rect);
	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(fill), dlg->Tool == C4TLS_Fill);
	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(picker), dlg->Tool == C4TLS_Picker);

	g_signal_handler_unblock(brush, handlerBrush);
	g_signal_handler_unblock(line, handlerLine);
	g_signal_handler_unblock(rect, handlerRect);
	g_signal_handler_unblock(fill, handlerFill);
	g_signal_handler_unblock(picker, handlerPicker);
}

void C4ToolsDlg::UpdateTextures()
{
	GtkComboBox* boxes[2] = {
		GTK_COMBO_BOX(state->fg_textures),
		GTK_COMBO_BOX(state->bg_textures)
	};

	const char* materials[2] = {
		Material, BackMaterial
	};

	for (int i = 0; i < 2; ++i)
	{
		// Refill dlg
		GtkComboBox* box = boxes[i];
		const char* material = materials[i];

		GtkListStore* list = GTK_LIST_STORE(gtk_combo_box_get_model(box));
		gtk_list_store_clear(list);

		// bottom-most: any invalid textures
		bool fAnyEntry = false; int32_t cnt; const char *szTexture;
		if (::Landscape.Mode!=C4LSC_Exact)
			for (cnt=0; (szTexture=::TextureMap.GetTexture(cnt)); cnt++)
			{
				if (!::TextureMap.GetIndex(material, szTexture, false))
				{
					fAnyEntry = true;
					gtk_combo_box_text_prepend_text(GTK_COMBO_BOX_TEXT(box), szTexture);
				}
			}
		// separator
		if (fAnyEntry)
		{
			gtk_combo_box_text_prepend_text(GTK_COMBO_BOX_TEXT(box), "-------");
		}

		// atop: valid textures
		for (cnt=0; (szTexture=::TextureMap.GetTexture(cnt)); cnt++)
		{
			// Current material-texture valid? Always valid for exact mode
			if (::TextureMap.GetIndex(material,szTexture,false) || ::Landscape.Mode==C4LSC_Exact)
			{
				gtk_combo_box_text_prepend_text(GTK_COMBO_BOX_TEXT(box), szTexture);
			}
		}
	}

	// reselect current
	g_signal_handler_block(state->fg_textures, state->handlerFgTextures);
	SelectComboBoxText(GTK_COMBO_BOX(state->fg_textures), Texture);
	g_signal_handler_unblock(state->fg_textures, state->handlerFgTextures);

	g_signal_handler_block(state->bg_textures, state->handlerBgTextures);
	SelectComboBoxText(GTK_COMBO_BOX(state->bg_textures), BackTexture);
	g_signal_handler_unblock(state->bg_textures, state->handlerBgTextures);
}

void C4ToolsDlg::NeedPreviewUpdate()
{
	state->UpdatePreview();
}

void C4ToolsDlg::State::UpdatePreview()
{
	if (!hbox) return;
	if (!gtk_widget_is_sensitive(preview)) return;

	C4ToolsDlg* dlg = GetOwner();

	int width = gtk_widget_get_allocated_width(preview);
	int height = gtk_widget_get_allocated_height(preview);
	width = std::min(width, dlg->Grade * 2);
	height = std::min(height, dlg->Grade * 2);

	// fill bg
	C4Pattern Pattern;
	// Sky material: sky as pattern only
	if (SEqual(dlg->Material,C4TLS_MatSky))
	{
		Pattern.Set(::Landscape.Sky.Surface, 0);
	}
	// Material-Texture
	else
	{
		// Get/Create TexMap entry
		BYTE iTex = ::TextureMap.GetIndex(dlg->Material, dlg->Texture, true);
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

	// Copy the texture into a circle in a cairo surface
	// TODO: Apply zoom factor to the circle size
	cairo_surface_t * surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	cairo_surface_flush(surface);
	unsigned char * data = cairo_image_surface_get_data(surface);
	int stride = cairo_image_surface_get_stride(surface);

	int x = width/2, y = height/2, r = dlg->Grade;
	for (int ycnt = -std::min(r, height); ycnt < std::min(r, height - y); ycnt++)
	{
		int lwdt = (int)sqrt(float(r * r - ycnt * ycnt));
		for (int xcnt = std::max(x - lwdt, 0); xcnt < std::min(x + lwdt, width); ++xcnt)
		{
			DWORD * pix = reinterpret_cast<DWORD *>(data + xcnt * 4 + (y + ycnt) * stride);
			*pix = Pattern.PatternClr(xcnt, y + ycnt);
		}
	}
	cairo_surface_mark_dirty(surface);
	gtk_image_set_from_surface(GTK_IMAGE(preview), surface);
	cairo_surface_destroy(surface);
}

void C4ToolsDlg::UpdateLandscapeModeCtrls()
{
	state->UpdateLandscapeModeCtrls();
}

void C4ToolsDlg::State::UpdateLandscapeModeCtrls()
{
	int32_t iMode = ::Landscape.Mode;
	g_signal_handler_block(landscape_dynamic, handlerDynamic);
	g_signal_handler_block(landscape_static, handlerStatic);
	g_signal_handler_block(landscape_exact, handlerExact);

	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(landscape_dynamic), iMode==C4LSC_Dynamic);
	gtk_widget_set_sensitive(landscape_dynamic, iMode==C4LSC_Dynamic);

	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(landscape_static), iMode==C4LSC_Static);
	gtk_widget_set_sensitive(landscape_static, ::Landscape.HasMap());

	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(landscape_exact), iMode==C4LSC_Exact);

	g_signal_handler_unblock(landscape_dynamic, handlerDynamic);
	g_signal_handler_unblock(landscape_static, handlerStatic);
	g_signal_handler_unblock(landscape_exact, handlerExact);

	C4DevmodeDlg::SetTitle(hbox, LoadResStr(iMode==C4LSC_Dynamic ? "IDS_DLG_DYNAMIC" : iMode==C4LSC_Static ? "IDS_DLG_STATIC" : "IDS_DLG_EXACT"));
}

void C4ToolsDlg::UpdateIFTControls()
{
	state->UpdateIFTControls();
}

void C4ToolsDlg::State::UpdateIFTControls()
{
	// No-op since we have background material controls
}

void C4ToolsDlg::InitGradeCtrl()
{
	if (!state->hbox) return;
	g_signal_handler_block(state->scale, state->handlerScale);
	gtk_range_set_increments(GTK_RANGE(state->scale), 1, 5);
	gtk_range_set_range(GTK_RANGE(state->scale), C4TLS_GradeMin, C4TLS_GradeMax);
	gtk_scale_set_draw_value(GTK_SCALE(state->scale), false);
	gtk_range_set_value(GTK_RANGE(state->scale), C4TLS_GradeMax-Grade);
	g_signal_handler_unblock(state->scale, state->handlerScale);
}

bool C4ToolsDlg::PopMaterial()
{
	if (!state->hbox) return false;
	gtk_widget_grab_focus(state->fg_materials);
	gtk_combo_box_popup(GTK_COMBO_BOX(state->fg_materials));
	return true;
}

bool C4ToolsDlg::PopTextures()
{
	if (!state->hbox) return false;
	gtk_widget_grab_focus(state->fg_textures);
	gtk_combo_box_popup(GTK_COMBO_BOX(state->fg_textures));
	return true;
}

void C4ConsoleGUI::ToolsDlgClose()
{
	// nope
}

void C4ConsoleGUI::SetCaptionToFileName(const char* file_name)
{
}

void C4ToolsDlg::EnableControls()
{
	int32_t iLandscapeMode=::Landscape.Mode;
	gtk_widget_set_sensitive(state->brush, iLandscapeMode>=C4LSC_Static);
	gtk_widget_set_sensitive(state->line, iLandscapeMode>=C4LSC_Static);
	gtk_widget_set_sensitive(state->rect, iLandscapeMode>=C4LSC_Static);
	gtk_widget_set_sensitive(state->fill, iLandscapeMode>=C4LSC_Exact);
	gtk_widget_set_sensitive(state->picker, iLandscapeMode>=C4LSC_Static);
	gtk_widget_set_sensitive(state->fg_materials, iLandscapeMode>=C4LSC_Static);
	gtk_widget_set_sensitive(state->fg_textures, iLandscapeMode >= C4LSC_Static && !SEqual(Material,C4TLS_MatSky));
	gtk_widget_set_sensitive(state->bg_materials, iLandscapeMode>=C4LSC_Static && !SEqual(Material,C4TLS_MatSky));
	gtk_widget_set_sensitive(state->bg_textures, iLandscapeMode >= C4LSC_Static && !SEqual(Material,C4TLS_MatSky) && !SEqual(BackMaterial, C4TLS_MatSky));
	gtk_widget_set_sensitive(state->scale, iLandscapeMode>=C4LSC_Static);
	gtk_widget_set_sensitive(state->preview, iLandscapeMode>=C4LSC_Static);
	NeedPreviewUpdate();
}

// GTK+ Callbacks

void C4ConsoleGUI::State::OnDestroy(GtkWidget* window, gpointer data)
{
	// The main window was destroyed, so clear all widget pointers in our
	// state, so that we don't try to use it anymore.
	static_cast<C4ConsoleGUI::State*>(data)->Clear();
}

void C4ConsoleGUI::State::OnScriptEntry(GtkWidget* entry, gpointer data)
{
	C4ConsoleGUI::State* state = static_cast<C4ConsoleGUI::State*>(data);
	const char * text = gtk_entry_get_text(GTK_ENTRY(state->txtScript));
	::Console.RegisterRecentInput(text, C4Console::MRU_Scenario);
	::Console.In(text);
	::Console.UpdateInputCtrl();
	gtk_editable_select_region(GTK_EDITABLE(state->txtScript), 0, -1);
}

void C4ConsoleGUI::State::OnPlay(GtkWidget* button, gpointer data)
{
	Console.DoPlay();

	// Must update haltctrls even if DoPlay did noting to restore
	// previous settings since GTK may have released this toggle button
	static_cast<C4ConsoleGUI::State*>(data)->GetOwner()->UpdateHaltCtrls(!!Game.HaltCount);
}

void C4ConsoleGUI::State::OnHalt(GtkWidget* button, gpointer data)
{
	Console.DoHalt();

	// Must update haltctrls even if DoPlay did noting to restore
	// previous settings since GTK may have released this toggle button
	static_cast<C4ConsoleGUI::State*>(data)->GetOwner()->UpdateHaltCtrls(!!Game.HaltCount);
}

void C4ConsoleGUI::State::OnModePlay(GtkWidget* button, gpointer data)
{
	Console.EditCursor.SetMode(C4CNS_ModePlay);
}

void C4ConsoleGUI::State::OnModeEdit(GtkWidget* button, gpointer data)
{
	Console.EditCursor.SetMode(C4CNS_ModeEdit);
}

void C4ConsoleGUI::State::OnModeDraw(GtkWidget* button, gpointer data)
{
	Console.EditCursor.SetMode(C4CNS_ModeDraw);
}

void C4ConsoleGUI::State::OnFileOpen(GtkWidget* item, gpointer data)
{
	Console.FileOpen();
}

void C4ConsoleGUI::State::OnFileOpenWPlrs(GtkWidget* item, gpointer data)
{
	Console.FileOpenWPlrs();
}

void C4ConsoleGUI::State::OnFileSave(GtkWidget* item, gpointer data)
{
	Console.FileSave();
}

void C4ConsoleGUI::State::OnFileSaveAs(GtkWidget* item, gpointer data)
{
	Console.FileSaveAs(false);
}

void C4ConsoleGUI::State::OnFileSaveGameAs(GtkWidget* item, gpointer data)
{
	Console.FileSaveAs(true);
}

void C4ConsoleGUI::State::OnFileRecord(GtkWidget* item, gpointer data)
{
	Console.FileRecord();
}

void C4ConsoleGUI::State::OnFileClose(GtkWidget* item, gpointer data)
{
	Console.FileClose();
}

void C4ConsoleGUI::State::OnFileQuit(GtkWidget* item, gpointer data)
{
	Console.FileQuit();
}

void C4ConsoleGUI::State::OnCompObjects(GtkWidget* item, gpointer data)
{
	Console.ObjectListDlg.Open();
}

void C4ConsoleGUI::State::OnPlrJoin(GtkWidget* item, gpointer data)
{
	Console.PlayerJoin();
}

void C4ConsoleGUI::State::OnPlrQuit(GtkWidget* item, gpointer data)
{
	C4Player *plr = ::Players.Get(GPOINTER_TO_INT(data));
	if (!plr) return;
	::Control.Input.Add(CID_PlrAction, C4ControlPlayerAction::Eliminate(plr));	
}

void C4ConsoleGUI::State::OnViewNew(GtkWidget* item, gpointer data)
{
	Console.ViewportNew();
}

void C4ConsoleGUI::State::OnViewNewPlr(GtkWidget* item, gpointer data)
{
	::Viewports.CreateViewport(GPOINTER_TO_INT(data));
}

void C4ConsoleGUI::State::OnHelpAbout(GtkWidget* item, gpointer data)
{
	Console.HelpAbout();
}

void C4ConsoleGUI::State::OnNetClient(GtkWidget* item, gpointer data)
{
	if (!::Control.isCtrlHost()) return;
	Game.Clients.CtrlRemove(Game.Clients.getClientByID(GPOINTER_TO_INT(data)), LoadResStr("IDS_MSG_KICKBYMENU"));
}

void C4ToolsDlg::State::OnButtonModeDynamic(GtkWidget* widget, gpointer data)
{
	static_cast<C4ToolsDlg::State*>(data)->GetOwner()->SetLandscapeMode(C4LSC_Dynamic);
}

void C4ToolsDlg::State::OnButtonModeStatic(GtkWidget* widget, gpointer data)
{
	static_cast<C4ToolsDlg::State*>(data)->GetOwner()->SetLandscapeMode(C4LSC_Static);
}

void C4ToolsDlg::State::OnButtonModeExact(GtkWidget* widget, gpointer data)
{
	static_cast<C4ToolsDlg::State*>(data)->GetOwner()->SetLandscapeMode(C4LSC_Exact);
}

void C4ToolsDlg::State::OnButtonBrush(GtkWidget* widget, gpointer data)
{
	static_cast<C4ToolsDlg::State*>(data)->GetOwner()->SetTool(C4TLS_Brush, false);
}

void C4ToolsDlg::State::OnButtonLine(GtkWidget* widget, gpointer data)
{
	static_cast<C4ToolsDlg::State*>(data)->GetOwner()->SetTool(C4TLS_Line, false);
}

void C4ToolsDlg::State::OnButtonRect(GtkWidget* widget, gpointer data)
{
	static_cast<C4ToolsDlg::State*>(data)->GetOwner()->SetTool(C4TLS_Rect, false);
}

void C4ToolsDlg::State::OnButtonFill(GtkWidget* widget, gpointer data)
{
	static_cast<C4ToolsDlg::State*>(data)->GetOwner()->SetTool(C4TLS_Fill, false);
}

void C4ToolsDlg::State::OnButtonPicker(GtkWidget* widget, gpointer data)
{
	static_cast<C4ToolsDlg::State*>(data)->GetOwner()->SetTool(C4TLS_Picker, false);
}

void C4ToolsDlg::State::OnButtonIft(GtkWidget* widget, gpointer data)
{
	static_cast<C4ToolsDlg::State*>(data)->GetOwner()->SetIFT(true);
}

void C4ToolsDlg::State::OnButtonNoIft(GtkWidget* widget, gpointer data)
{
	static_cast<C4ToolsDlg::State*>(data)->GetOwner()->SetIFT(false);
}

void C4ToolsDlg::State::OnComboMaterial(GtkWidget* widget, gpointer data)
{
	gchar* text = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget));
	static_cast<C4ToolsDlg::State*>(data)->GetOwner()->SetMaterial(text);
	g_free(text);
}

void C4ToolsDlg::State::OnComboTexture(GtkWidget* widget, gpointer data)
{
	gchar* text = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget));
	static_cast<C4ToolsDlg::State*>(data)->GetOwner()->SetTexture(text);
	g_free(text);
}

void C4ToolsDlg::State::OnComboBgMaterial(GtkWidget* widget, gpointer data)
{
	gchar* text = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget));
	static_cast<C4ToolsDlg::State*>(data)->GetOwner()->SetBackMaterial(text);
	g_free(text);
}

void C4ToolsDlg::State::OnComboBgTexture(GtkWidget* widget, gpointer data)
{
	gchar* text = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget));
	static_cast<C4ToolsDlg::State*>(data)->GetOwner()->SetBackTexture(text);
	g_free(text);
}

void C4ToolsDlg::State::OnGrade(GtkWidget* widget, gpointer data)
{
	C4ToolsDlg::State* state = static_cast<C4ToolsDlg::State*>(data);
	int value = static_cast<int>(gtk_range_get_value(GTK_RANGE(state->scale)) + 0.5);
	state->GetOwner()->SetGrade(C4TLS_GradeMax-value);
}

void C4ToolsDlg::State::OnWindowHide(GtkWidget* widget, gpointer data)
{
	static_cast<C4ToolsDlg::State*>(data)->GetOwner()->Active = false;
}

#include "C4ConsoleGUICommon.h"
