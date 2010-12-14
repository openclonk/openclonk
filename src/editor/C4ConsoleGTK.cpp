/*
 * OpenClonk, http://www.openclonk.org
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

#include <C4Include.h>
#include <C4Console.h>
#include <C4Application.h>

#include <C4GameSave.h>
#include <C4Game.h>
#include <C4MessageInput.h>
#include <C4UserMessages.h>
#include <C4Version.h>
#include <C4Language.h>
#include <C4Player.h>
#include <C4Landscape.h>
#include <C4GraphicsSystem.h>
#include <C4PlayerList.h>
#include <C4GameControl.h>
#include <C4Texture.h>
#include <C4Viewport.h>

#include <StdFile.h>
#include <StdRegistry.h>

# include <gdk/gdkx.h>
# include <gtk/gtk.h>

# include <res/Play.h>
# include <res/Halt.h>
# include <res/Mouse.h>
# include <res/Cursor.h>
# include <res/Brush.h>
# include <C4Language.h>
# include <C4DevmodeDlg.h>

# include <res/Line.h>
# include <res/Rect.h>
# include <res/Fill.h>
# include <res/Picker.h>

# include <res/Dynamic.h>
# include <res/Static.h>
# include <res/Exact.h>

# include <res/Ift.h>
# include <res/NoIft.h>

using namespace OpenFileFlags;

namespace
{
	/*GtkWidget* CreateImageFromInlinedPixbuf(const guint8* pixbuf_data)
	{
		GdkPixbuf* pixbuf = gdk_pixbuf_new_from_inline(-1, pixbuf_data, false, NULL);
		GtkWidget* image = gtk_image_new_from_pixbuf(pixbuf);
		gdk_pixbuf_unref(pixbuf);
		return image;
	}*/
	
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

class C4ConsoleGUI::State: public C4ConsoleGUI::InternalState<class C4ConsoleGUI>
{
public:
	GdkCursor* cursorDefault;
	GdkCursor* cursorWait;

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
	GtkWidget* fileSaveGame;
	GtkWidget* fileSaveGameAs;
	GtkWidget* fileRecord;
	GtkWidget* fileClose;
	GtkWidget* fileQuit;

	GtkWidget* compScript;
	GtkWidget* compTitle;
	GtkWidget* compInfo;
	GtkWidget* compObjects;

	GtkWidget* plrJoin;

	GtkWidget* viewNew;

	GtkWidget* helpAbout;

	GtkWidget* lblCursor;
	GtkWidget* lblFrame;
	GtkWidget* lblScript;
	GtkWidget* lblTime;

	gulong handlerDestroy;
	gulong handlerPlay;
	gulong handlerHalt;
	gulong handlerModePlay;
	gulong handlerModeEdit;
	gulong handlerModeDraw;

	State(C4ConsoleGUI *console): Super(console)
	{
		cursorDefault = NULL;
		cursorWait = NULL;
		
		Clear();
	}

	~State()
	{
		if(cursorDefault)
			gdk_cursor_unref(cursorDefault);
		if(cursorWait)
			gdk_cursor_unref(cursorWait);

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
	static void OnFileSaveGame(GtkWidget* item, gpointer data);
	static void OnFileSaveGameAs(GtkWidget* item, gpointer data);
	static void OnFileRecord(GtkWidget* item, gpointer data);
	static void OnFileClose(GtkWidget* item, gpointer data);
	static void OnFileQuit(GtkWidget* item, gpointer data);

	static void OnCompObjects(GtkWidget* item, gpointer data);
	static void OnCompScript(GtkWidget* item, gpointer data);
	static void OnCompTitle(GtkWidget* item, gpointer data);
	static void OnCompInfo(GtkWidget* item, gpointer data);

	static void OnPlrJoin(GtkWidget* item, gpointer data);
	static void OnPlrQuit(GtkWidget* item, gpointer data);
	static void OnViewNew(GtkWidget* item, gpointer data);
	static void OnViewNewPlr(GtkWidget* item, gpointer data);
	static void OnHelpAbout(GtkWidget* item, gpointer data);

	static void OnNetClient(GtkWidget* item, gpointer data);
};

class C4PropertyDlg::State: public C4ConsoleGUI::InternalState<class C4PropertyDlg>
{
public:
//    GtkWidget* window;
	GtkWidget* vbox;
	GtkWidget* textview;
	GtkWidget* entry;

	gulong handlerHide;

	static void OnScriptActivate(GtkWidget* widget, gpointer data);
	static void OnWindowHide(GtkWidget* widget, gpointer data);
//    static void OnDestroy(GtkWidget* widget, gpointer data);

	~State()
	{
		if (vbox != NULL)
		{
			g_signal_handler_disconnect(G_OBJECT(C4DevmodeDlg::GetWindow()), handlerHide);
			C4DevmodeDlg::RemovePage(vbox);
			vbox = NULL;
		}
	}

	State(C4PropertyDlg* dlg): Super(dlg), vbox(NULL) {}

	bool Open();

	void Clear() {}
	void Default() {}
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
	
	State(C4ToolsDlg* dlg): Super(dlg), hbox(NULL) {}
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
	void Default() {}
};

void C4PropertyDlg::State::OnScriptActivate(GtkWidget* widget, gpointer data)
{
	const gchar* text = gtk_entry_get_text(GTK_ENTRY(widget));
	if (text && text[0])
		Console.EditCursor.In(text);
}

void C4PropertyDlg::State::OnWindowHide(GtkWidget* widget, gpointer user_data)
{
	static_cast<C4PropertyDlg*>(user_data)->Active = false;
}

CStdWindow* C4ConsoleGUI::CreateConsoleWindow(CStdApp* pApp)
{
	state->cursorWait = gdk_cursor_new(GDK_WATCH);
	state->cursorDefault = gdk_cursor_new(GDK_ARROW);

	// Calls InitGUI
	CStdWindow* retval = C4ConsoleBase::Init(CStdWindow::W_GuiWindow, pApp, LoadResStr("IDS_CNS_CONSOLE"), NULL, false);
	UpdateHaltCtrls(true);
	EnableControls(fGameOpen);
	ClearViewportMenu();
	return retval;
}

GtkWidget* C4ConsoleGUI::InitGUI()
{
	state->InitGUI();
	return C4ConsoleBase::InitGUI();
}

void C4ConsoleGUI::State::InitGUI()
{
	// ------------ Play/Pause and Mode ---------------------
	GtkWidget* image_play = CreateImageFromInlinedPixbuf(play_pixbuf_data);
	GtkWidget* image_pause = CreateImageFromInlinedPixbuf(halt_pixbuf_data);

	GtkWidget* image_mode_play = CreateImageFromInlinedPixbuf(mouse_pixbuf_data);
	GtkWidget* image_mode_edit = CreateImageFromInlinedPixbuf(cursor_pixbuf_data);
	GtkWidget* image_mode_draw = CreateImageFromInlinedPixbuf(brush_pixbuf_data);

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

	lblCursor = gtk_label_new("");
	GtkToolItem * itmCursor = gtk_tool_item_new();
	gtk_container_add(GTK_CONTAINER(itmCursor), lblCursor);
	gtk_toolbar_insert(GTK_TOOLBAR(top_hbox), itmCursor, -1);

	// ------------ Statusbar ---------------------
	GtkWidget* statusbar = gtk_hbox_new(false, 6);

	GtkWidget* status_frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(status_frame), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(status_frame), statusbar);

	lblFrame = gtk_label_new("Frame: 0");
	lblScript = gtk_label_new("Script: 0");
	lblTime = gtk_label_new("00:00:00 (0 FPS)");

	gtk_misc_set_alignment(GTK_MISC(lblFrame), 0.0, 0.5);
	gtk_misc_set_alignment(GTK_MISC(lblScript), 0.0, 0.5);
	gtk_misc_set_alignment(GTK_MISC(lblTime), 0.0, 0.5);

	GtkWidget* sep1 = gtk_vseparator_new();
	GtkWidget* sep2 = gtk_vseparator_new();

	gtk_box_pack_start(GTK_BOX(statusbar), lblFrame, true, true, 0);
	gtk_box_pack_start(GTK_BOX(statusbar), sep1, false, false, 0);
	gtk_box_pack_start(GTK_BOX(statusbar), lblScript, true, true, 0);
	gtk_box_pack_start(GTK_BOX(statusbar), sep2, false, false, 0);
	gtk_box_pack_start(GTK_BOX(statusbar), lblTime, true, true, 0);

	// ------------ Log view and script entry ---------------------
	GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);

//	int scrollbar_spacing = 0;
//	gtk_widget_style_get (widget, "scrollbar-spacing", &scrollBarSpacing, NULL);
//	g_object_set (scroll, "scrollbar-spacing", 0, NULL);

	txtLog = gtk_text_view_new();
	txtScript = gtk_entry_new();

	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(txtLog), false);

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

	fileSaveGame = gtk_menu_item_new_with_label(LoadResStr("IDS_MNU_SAVEGAME"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menuFile), fileSaveGame);

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

	compScript = gtk_menu_item_new_with_label(LoadResStr("IDS_MNU_SCRIPT"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menuComponents), compScript);

	compTitle = gtk_menu_item_new_with_label(LoadResStr("IDS_MNU_TITLE"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menuComponents), compTitle);

	compInfo = gtk_menu_item_new_with_label(LoadResStr("IDS_MNU_INFO"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menuComponents), compInfo);

	plrJoin = gtk_menu_item_new_with_label(LoadResStr("IDS_MNU_JOIN"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menuPlayer), plrJoin);

	viewNew = gtk_menu_item_new_with_label(LoadResStr("IDS_MNU_NEW"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menuViewport), viewNew);

	helpAbout = gtk_menu_item_new_with_label(LoadResStr("IDS_MENU_ABOUT"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menuHelp), helpAbout);

	// ------------ Window ---------------------
	GtkWidget* box = gtk_vbox_new(false, 0);

	gtk_box_pack_start(GTK_BOX(box), menuBar, false, false, 0);
	gtk_box_pack_start(GTK_BOX(box), top_hbox, false, false, 0);
	gtk_box_pack_start(GTK_BOX(box), scroll, true, true, 0);
	gtk_box_pack_start(GTK_BOX(box), txtScript, false, false, 0);
	gtk_box_pack_start(GTK_BOX(box), status_frame, false, false, 0);

	gtk_window_set_default_size(GTK_WINDOW(GetOwner()->window), 320, 320);

	gtk_container_add(GTK_CONTAINER(GetOwner()->window), box);

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
	g_signal_connect(G_OBJECT(fileSaveGame), "activate", G_CALLBACK(OnFileSaveGame), this);
	g_signal_connect(G_OBJECT(fileSaveGameAs), "activate", G_CALLBACK(OnFileSaveGameAs), this);
	g_signal_connect(G_OBJECT(fileRecord), "activate", G_CALLBACK(OnFileRecord), this);
	g_signal_connect(G_OBJECT(fileClose), "activate", G_CALLBACK(OnFileClose), this);
	g_signal_connect(G_OBJECT(fileQuit), "activate", G_CALLBACK(OnFileQuit), this);
	g_signal_connect(G_OBJECT(compObjects), "activate", G_CALLBACK(OnCompObjects), this);
	g_signal_connect(G_OBJECT(compScript), "activate", G_CALLBACK(OnCompScript), this);
	g_signal_connect(G_OBJECT(compTitle), "activate", G_CALLBACK(OnCompTitle), this);
	g_signal_connect(G_OBJECT(compInfo), "activate", G_CALLBACK(OnCompInfo), this);
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
	fileSaveGame = NULL;
	fileSaveGameAs = NULL;
	fileRecord = NULL;
	fileClose = NULL;
	fileQuit = NULL;

	compScript = NULL;
	compTitle = NULL;
	compInfo = NULL;
	compObjects = NULL;

	plrJoin = NULL;

	viewNew = NULL;

	helpAbout = NULL;

	lblCursor = NULL;
	lblFrame = NULL;
	lblScript = NULL;
	lblTime = NULL;

	handlerDestroy = 0;
	handlerPlay = 0;
	handlerHalt = 0;
	handlerModePlay = 0;
	handlerModeEdit = 0;
	handlerModeDraw = 0;
}

void C4ConsoleGUI::DisplayInfoText(InfoTextType type, StdStrBuf& text)
{		
	if (!Active)
		return;
	GtkWidget* label;
	switch (type)
	{
	case CONSOLE_Cursor:
		label = state->lblCursor;
		break;
	case CONSOLE_FrameCounter:
		label = state->lblFrame;
		break;
	case CONSOLE_ScriptCounter:
		label = state->lblScript;
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
	gdk_window_set_cursor(window->window, state->cursorWait);
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

bool C4ConsoleGUI::FileSelect(char *sFilename, int iSize, const char * szFilter, DWORD dwFlags, bool fSave)
{
	GtkWidget* dialog = gtk_file_chooser_dialog_new(fSave ? "Save file..." : "Load file...", GTK_WINDOW(window), fSave ? GTK_FILE_CHOOSER_ACTION_SAVE : GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, fSave ? GTK_STOCK_SAVE : GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

	// TODO: Set dialog modal?

	if (g_path_is_absolute(sFilename) )
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), sFilename);
	else if (fSave)
		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), sFilename);

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

	// TODO: Not in GTK+ 2.4, we could check GTK+ version at runtime and rely on lazy bindung
//  gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), (dwFlags & OFN_OVERWRITEPROMPT) != 0);

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
		SCopy(filename, sFilename, iSize);
		g_free(filename);
	}
	else
	{
		// Otherwise its the folder followed by the file names,
		// separated by '\0'-bytes
		char* folder = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
		int len = SLen(folder);

		if (iSize > 0) SCopy(folder, sFilename, Min(len + 1, iSize));
		iSize -= (len + 1); sFilename += (len + 1);
		g_free(folder);

		GSList* files = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
		for (GSList* item = files; item != NULL; item = item->next)
		{
			const char* file = static_cast<const char*>(item->data);
			char* basefile = g_path_get_basename(file);

			int len = SLen(basefile);
			if (iSize > 0) SCopy(basefile, sFilename, Min(len + 1, iSize));
			iSize -= (len + 1); sFilename += (len + 1);

			g_free(basefile);
			g_free(item->data);
		}

		// End of list
		*sFilename = '\0';
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

bool C4ConsoleGUI::Out(const char *message)
{
	// Append text to log
	if (!window) return true;

	GtkTextIter end;
	GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->txtLog));
	gtk_text_buffer_get_end_iter(buffer, &end);

	gtk_text_buffer_insert(buffer, &end, message, -1);
	gtk_text_buffer_insert(buffer, &end, "\n", 1);

	gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(state->txtLog), gtk_text_buffer_get_insert(buffer), 0.0, false, 0.0, 0.0);
}

void C4ConsoleGUI::UpdateNetMenu(Stage stage)
{
	switch (stage)
	{
	case C4ConsoleGUI::STAGE_Start:
	{
		state->itemNet = gtk_menu_item_new_with_label(LoadResStr("IDS_MNU_NET"));
		state->menuNet = gtk_menu_new();
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(state->itemNet), state->menuNet);
		gtk_menu_shell_insert(GTK_MENU_SHELL(state->menuBar), state->itemNet, Console.MenuIndexHelp);
		break;
	}
	case C4ConsoleGUI::STAGE_Intermediate:
		break;
	case C4ConsoleGUI::STAGE_End:
		gtk_widget_show_all(state->itemNet);
		break;
	}
}

void C4ConsoleGUI::AddNetMenuItemForPlayer(int32_t index, StdStrBuf &text)
{
	GtkWidget* item = gtk_menu_item_new_with_label(text.getData());
	gtk_menu_shell_append(GTK_MENU_SHELL(state->menuNet), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(State::OnNetClient), GINT_TO_POINTER(Game.Clients.getLocalID()));
}

void C4ConsoleGUI::ClearNetMenu(C4ConsoleGUI::Stage stage)
{
	// Don't need to do anything if the GUI is not created
	if(state->menuBar == NULL || state->itemNet == NULL) return;

	switch (stage)
	{
	case C4ConsoleGUI::STAGE_Start:
		gtk_container_remove(GTK_CONTAINER(state->menuBar), state->itemNet);
		state->itemNet = NULL;
		break;
	case C4ConsoleGUI::STAGE_End:
		break;
	}
}

void C4ConsoleGUI::ClearInput()
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

	GtkTreeIter iter;
	GtkListStore* store = GTK_LIST_STORE(gtk_entry_completion_get_model(completion));
	g_assert(store);
	gtk_list_store_clear(store);
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
	g_signal_handler_block(state->textures, state->handlerTextures);
	SelectComboBoxText(GTK_COMBO_BOX(state->textures), texture);
	g_signal_handler_unblock(state->textures, state->handlerTextures);
}

void C4ConsoleGUI::ToolsDlgSelectMaterial(C4ToolsDlg *dlg, const char *material)
{
	C4ToolsDlg::State* state = dlg->state;
	g_signal_handler_block(state->materials, state->handlerMaterials);
	SelectComboBoxText(GTK_COMBO_BOX(state->materials), material);
	g_signal_handler_unblock(state->materials, state->handlerMaterials);
}

void C4ConsoleGUI::PropertyDlgSetFunctions(C4PropertyDlg *dlg, std::vector<char*> &functions)
{
	GtkEntryCompletion* completion = gtk_entry_get_completion(GTK_ENTRY(dlg->state->entry));
	GtkListStore* store;

	// Uncouple list store from completion so that the completion is not
	// notified for every row we are going to insert. This enhances
	// performance significantly.
	if (!completion)
	{
		completion = gtk_entry_completion_new();
		store = gtk_list_store_new(1, G_TYPE_STRING);

		gtk_entry_completion_set_text_column(completion, 0);
		gtk_entry_set_completion(GTK_ENTRY(dlg->state->entry), completion);
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

	for (std::vector<char*>::iterator it(functions.begin()); it != functions.end(); it++)
	{
		char* fn = *it;
		if (fn != C4ConsoleGUI::LIST_DIVIDER)
		{
			gtk_list_store_append(store, &iter);
			gtk_list_store_set(store, &iter, 0, fn, -1);
		}
	}

	// Reassociate list store with completion
	gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(store));
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
	gtk_widget_set_sensitive(fileSaveGame, fEnable && ::Players.GetCount());
	gtk_widget_set_sensitive(fileSaveGameAs, fEnable && ::Players.GetCount());
	gtk_widget_set_sensitive(fileSave, fEnable);
	gtk_widget_set_sensitive(fileSaveAs, fEnable);
	gtk_widget_set_sensitive(fileClose, fEnable);

	// Components menu
	gtk_widget_set_sensitive(compObjects, fEnable && GetOwner()->Editing);
	gtk_widget_set_sensitive(compScript, fEnable && GetOwner()->Editing);
	gtk_widget_set_sensitive(compInfo, fEnable && GetOwner()->Editing);
	gtk_widget_set_sensitive(compTitle, fEnable && GetOwner()->Editing);

	// Player & viewport menu
	gtk_widget_set_sensitive(plrJoin, fEnable && GetOwner()->Editing);
	gtk_widget_set_sensitive(viewNew, fEnable);
}

bool C4ConsoleGUI::PropertyDlgOpen(class C4PropertyDlg* dlg)
{
	return dlg->state->Open();
}

bool C4PropertyDlg::State::Open()
{
	if (vbox == NULL)
	{
		vbox = gtk_vbox_new(false, 6);

		GtkWidget* scrolled_wnd = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_wnd), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_wnd), GTK_SHADOW_IN);

		textview = gtk_text_view_new();
		entry = gtk_entry_new();

		gtk_container_add(GTK_CONTAINER(scrolled_wnd), textview);
		gtk_box_pack_start(GTK_BOX(vbox), scrolled_wnd, true, true, 0);
		gtk_box_pack_start(GTK_BOX(vbox), entry, false, false, 0);

		gtk_text_view_set_editable(GTK_TEXT_VIEW(textview), false);
		gtk_widget_set_sensitive(entry, Console.Editing);

		gtk_widget_show_all(vbox);

		C4DevmodeDlg::AddPage(vbox, GTK_WINDOW(Console.window), LoadResStr("IDS_DLG_PROPERTIES"));

		g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(OnScriptActivate), this);

		handlerHide = g_signal_connect(G_OBJECT(C4DevmodeDlg::GetWindow()), "hide", G_CALLBACK(OnWindowHide), this);
	}

	C4DevmodeDlg::SwitchPage(vbox);
	return true;
}

void C4ConsoleGUI::PropertyDlgUpdate(class C4PropertyDlg* dlg, StdStrBuf &text)
{
	GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(dlg->state->textview));
	gtk_text_buffer_set_text(buffer, text.getData(), -1);
}

bool C4ConsoleGUI::ToolsDlgOpen(C4ToolsDlg *dlg)
{
	return dlg->state->Open();
}

bool C4ToolsDlg::State::Open()
{
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
	return true;
}

void C4ConsoleGUI::ToolsDlgInitMaterialCtrls(C4ToolsDlg *dlg)
{
	dlg->state->InitMaterialCtrls();
}

void C4ToolsDlg::State::InitMaterialCtrls()
{
	GtkListStore* list = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(materials)));

	g_signal_handler_block(materials, handlerMaterials);
	gtk_list_store_clear(list);

	gtk_combo_box_append_text(GTK_COMBO_BOX(materials), C4TLS_MatSky);
	for (int32_t cnt = 0; cnt < ::MaterialMap.Num; cnt++)
	{
		gtk_combo_box_append_text(GTK_COMBO_BOX(materials), ::MaterialMap.Map[cnt].Name);
	}
	g_signal_handler_unblock(materials, handlerMaterials);
	SelectComboBoxText(GTK_COMBO_BOX(materials), GetOwner()->Material);
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

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(brush), dlg->Tool == C4TLS_Brush);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(line), dlg->Tool == C4TLS_Line);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rect), dlg->Tool == C4TLS_Rect);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fill), dlg->Tool == C4TLS_Fill);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(picker), dlg->Tool == C4TLS_Picker);

	g_signal_handler_unblock(brush, handlerBrush);
	g_signal_handler_unblock(line, handlerLine);
	g_signal_handler_unblock(rect, handlerRect);
	g_signal_handler_unblock(fill, handlerFill);
	g_signal_handler_unblock(picker, handlerPicker);
}

void C4ToolsDlg::UpdateTextures()
{
	// Refill dlg
	GtkListStore* list = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(state->textures)));
	gtk_list_store_clear(list);
	// bottom-most: any invalid textures
	bool fAnyEntry = false; int32_t cnt; const char *szTexture;
	if (::Landscape.Mode!=C4LSC_Exact)
		for (cnt=0; (szTexture=::TextureMap.GetTexture(cnt)); cnt++)
		{
			if (!::TextureMap.GetIndex(Material, szTexture, false))
			{
				fAnyEntry = true;
				gtk_combo_box_prepend_text(GTK_COMBO_BOX(state->textures), szTexture);
			}
		}
	// separator
	if (fAnyEntry)
	{
		gtk_combo_box_prepend_text(GTK_COMBO_BOX(state->textures), "-------");
	}

	// atop: valid textures
	for (cnt=0; (szTexture=::TextureMap.GetTexture(cnt)); cnt++)
	{
		// Current material-texture valid? Always valid for exact mode
		if (::TextureMap.GetIndex(Material,szTexture,false) || ::Landscape.Mode==C4LSC_Exact)
		{
			gtk_combo_box_prepend_text(GTK_COMBO_BOX(state->textures), szTexture);
		}
	}
	// reselect current
	g_signal_handler_block(state->textures, state->handlerTextures);
	SelectComboBoxText(GTK_COMBO_BOX(state->textures), Texture);
	g_signal_handler_unblock(state->textures, state->handlerTextures);
}

void C4ConsoleGUI::ToolsDlgSetTexture(class C4ToolsDlg *dlg, const char *texture)
{
	C4ToolsDlg::State* state = dlg->state;
	g_signal_handler_block(state->textures, state->handlerTextures);
	SelectComboBoxText(GTK_COMBO_BOX(state->textures), texture);
	g_signal_handler_unblock(state->textures, state->handlerTextures);
}

void C4ToolsDlg::NeedPreviewUpdate()
{
	state->UpdatePreview();
}

void C4ToolsDlg::State::UpdatePreview()
{
	if (!hbox) return;
	C4ToolsDlg* dlg = GetOwner();

	SURFACE sfcPreview;

	int32_t iPrvWdt,iPrvHgt;

	RECT rect;
	/* TODO: Set size request for image to read size from image's size request? */
	rect.left = 0;
	rect.top = 0;
	rect.bottom = 64;
	rect.right = 64;

	iPrvWdt=rect.right-rect.left;
	iPrvHgt=rect.bottom-rect.top;

	if (!(sfcPreview=new CSurface(iPrvWdt,iPrvHgt))) return;

	// fill bg
	BYTE bCol = 0;
	CPattern Pattern;
	// Sky material: sky as pattern only
	if (SEqual(dlg->Material,C4TLS_MatSky))
	{
		Pattern.Set(::Landscape.Sky.Surface, 0);
	}
	// Material-Texture
	else
	{
		bCol=Mat2PixColDefault(::MaterialMap.Get(dlg->Material));
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
#if GTK_CHECK_VERSION(2,18,0)
	if (gtk_widget_is_sensitive(preview))
#else
	if (GTK_WIDGET_SENSITIVE(preview))
#endif
		lpDDraw->DrawPatternedCircle( sfcPreview,
		                                        iPrvWdt/2,iPrvHgt/2,
		                                        dlg->Grade,
		                                        bCol, Pattern, *::Landscape.GetPal());

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
	delete sfcPreview;
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

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(landscape_dynamic), iMode==C4LSC_Dynamic);
	gtk_widget_set_sensitive(landscape_dynamic, iMode==C4LSC_Dynamic);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(landscape_static), iMode==C4LSC_Static);
	gtk_widget_set_sensitive(landscape_static, ::Landscape.Map!=NULL);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(landscape_exact), iMode==C4LSC_Exact);

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
	if (!hbox) return;
	g_signal_handler_block(no_ift, handlerNoIft);
	g_signal_handler_block(ift, handlerIft);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(no_ift), GetOwner()->ModeIFT==0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ift), GetOwner()->ModeIFT==1);

	g_signal_handler_unblock(no_ift, handlerNoIft);
	g_signal_handler_unblock(ift, handlerIft);
}

void C4ConsoleGUI::ToolsDlgSetMaterial(class C4ToolsDlg *dlg, const char *material)
{
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
	gtk_widget_grab_focus(state->materials);
	gtk_combo_box_popup(GTK_COMBO_BOX(state->materials));
}

bool C4ToolsDlg::PopTextures()
{
	if (!state->hbox) return false;
	gtk_widget_grab_focus(state->textures);
	gtk_combo_box_popup(GTK_COMBO_BOX(state->textures));
}

void C4ConsoleGUI::ClearDlg(void* dlg)
{
	// nope
}

void C4ConsoleGUI::SetCaptionToFileName(const char* file_name)
{
}

void C4ConsoleGUI::SetInputFunctions(std::vector<char*>& functions)
{
}

void C4ConsoleGUI::ToolsDlgEnableControls(C4ToolsDlg* dlg)
{
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
	Console.In(gtk_entry_get_text(GTK_ENTRY(state->txtScript)));
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
	Console.FileSave(false);
}

void C4ConsoleGUI::State::OnFileSaveAs(GtkWidget* item, gpointer data)
{
	Console.FileSaveAs(false);
}

void C4ConsoleGUI::State::OnFileSaveGame(GtkWidget* item, gpointer data)
{
	Console.FileSave(true);
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
	Console.EditObjects();
}

void C4ConsoleGUI::State::OnCompScript(GtkWidget* item, gpointer data)
{
	Console.EditScript();
}

void C4ConsoleGUI::State::OnCompTitle(GtkWidget* item, gpointer data)
{
	Console.EditTitle();
}

void C4ConsoleGUI::State::OnCompInfo(GtkWidget* item, gpointer data)
{
	Console.EditInfo();
}

void C4ConsoleGUI::State::OnPlrJoin(GtkWidget* item, gpointer data)
{
	Console.PlayerJoin();
}

void C4ConsoleGUI::State::OnPlrQuit(GtkWidget* item, gpointer data)
{
	::Control.Input.Add(CID_Script, new C4ControlScript(FormatString("EliminatePlayer(%d)", GPOINTER_TO_INT(data)).getData()));
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
	gchar* text = gtk_combo_box_get_active_text(GTK_COMBO_BOX(widget));
	static_cast<C4ToolsDlg::State*>(data)->GetOwner()->SetMaterial(text);
	g_free(text);
}

void C4ToolsDlg::State::OnComboTexture(GtkWidget* widget, gpointer data)
{
	gchar* text = gtk_combo_box_get_active_text(GTK_COMBO_BOX(widget));
	static_cast<C4ToolsDlg::State*>(data)->GetOwner()->SetTexture(text);
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
