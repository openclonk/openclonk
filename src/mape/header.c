/*
 * mape - C4 Landscape.txt editor
 *
 * Copyright (c) 2005-2009, Armin Burgmeier
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

#include <stdlib.h>

#include "mape/header.h"

static const GtkActionEntry mape_header_file_entries[] = {
	{
		"File",
		NULL,
		"_File",
		NULL,
		NULL,
		NULL
	}, {
		"FileNew",
		GTK_STOCK_NEW,
		"_New",
		"<control>N",
		"Opens a new document",
		NULL
	}, {
		"FileOpen",
		GTK_STOCK_OPEN,
		"_Open",
		"<control>O",
		"Opens an already existing document from disk",
		NULL
	}, {
		"FileSave",
		GTK_STOCK_SAVE,
		"_Save",
		"<control>S",
		"Saves the current document to disk",
		NULL
	}, {
		"FileSaveAs",
		GTK_STOCK_SAVE_AS,
		"Save as",
		"<control><shift>S",
		"Save the current map to another path on disk",
		NULL
	}, {
		"FileQuit",
		GTK_STOCK_QUIT,
		"_Quit",
		"<control>Q",
		"Exit the program",
		NULL
	}
};

static const GtkActionEntry mape_header_edit_entries[] = {
	{
		"Edit",
		NULL,
		"_Edit",
		NULL,
		NULL,
		NULL
	}, {
		"EditUndo",
		GTK_STOCK_UNDO,
		"_Undo",
		"<control>Z",
		"Undo the last action",
		NULL
	}, {
		"EditRedo",
		GTK_STOCK_REDO,
		"_Redo",
		"<control>Y",
		"Redo the last action",
		NULL
	}, {
		"EditPreferences",
		GTK_STOCK_PREFERENCES,
		"Pr_eferences",
		NULL,
		"Configure the application",
		NULL
	}
};

static const GtkActionEntry mape_header_help_entries[] = {
	{
		"Help",
		NULL,
		"_Help",
		NULL,
		NULL,
		NULL
	}, {
		"HelpAbout",
		GTK_STOCK_ABOUT,
		"_About",
		NULL,
		"Shows authors and copyright information",
		NULL
	}
};

static const gchar* mape_header_ui_desc =
	"<ui>"
	" <menubar name='MenuBar'>"
	"  <menu action='File'>"
	"   <menuitem action='FileNew' />"
	"   <menuitem action='FileOpen' />"
	"   <menuitem action='FileSave' />"
	"   <menuitem action='FileSaveAs' />"
	"   <separator />"
	"   <menuitem action='FileQuit' />"
	"  </menu>"
	"  <menu action='Edit'>"
	"   <menuitem action='EditUndo' />"
	"   <menuitem action='EditRedo' />"
	"   <separator />"
	"   <menuitem action='EditPreferences' />"
	"  </menu>"
	"  <menu action='Help'>"
	"   <menuitem action='HelpAbout' />"
	"  </menu>"
	" </menubar>"
	" <toolbar name='ToolBar'>"
	"  <toolitem action='FileNew' />"
	"  <toolitem action='FileOpen' />"
	"  <toolitem action='FileSave' />"
	"  <toolitem action='FileSaveAs' />"
	"  <separator />"
	"  <toolitem action='EditUndo' />"
	"  <toolitem action='EditRedo' />"
	" </toolbar>"
	"</ui>";

MapeHeader* mape_header_new(void)
{
	MapeHeader* header;
	gint result;

	header = malloc(sizeof(MapeHeader) );

	header->group_file = gtk_action_group_new("FileActions");
	gtk_action_group_add_actions(
		header->group_file,
		mape_header_file_entries,
		G_N_ELEMENTS(mape_header_file_entries),
		header
	);
	
	header->group_edit = gtk_action_group_new("EditActions");
	gtk_action_group_add_actions(
		header->group_edit,
		mape_header_edit_entries,
		G_N_ELEMENTS(mape_header_edit_entries),
		header
	);

	header->group_help = gtk_action_group_new("HelpActions");
	gtk_action_group_add_actions(
		header->group_help,
		mape_header_help_entries,
		G_N_ELEMENTS(mape_header_help_entries),
		header
	);

	header->ui_manager = gtk_ui_manager_new();
	gtk_ui_manager_insert_action_group(
		header->ui_manager,
		header->group_file,
		0
	);
	
	gtk_ui_manager_insert_action_group(
		header->ui_manager,
		header->group_edit,
		0
	);

	gtk_ui_manager_insert_action_group(
		header->ui_manager,
		header->group_help,
		0
	);

	result = gtk_ui_manager_add_ui_from_string(
		header->ui_manager,
		mape_header_ui_desc,
		-1,
		NULL
	);

	g_assert(result != 0);

	header->menubar = gtk_ui_manager_get_widget(
		header->ui_manager,
		"/MenuBar"
	);

	header->toolbar = gtk_ui_manager_get_widget(
		header->ui_manager,
		"/ToolBar"
	);

	g_assert(header->menubar != NULL);
	g_assert(header->toolbar != NULL);

	header->accel_group = gtk_ui_manager_get_accel_group(
		header->ui_manager
	);

	g_assert(header->accel_group != NULL);

	header->file_new = gtk_action_group_get_action(
		header->group_file,
		"FileNew"
	);

	header->file_open = gtk_action_group_get_action(
		header->group_file,
		"FileOpen"
	);

	header->file_save = gtk_action_group_get_action(
		header->group_file,
		"FileSave"
	);

	header->file_save_as = gtk_action_group_get_action(
		header->group_file,
		"FileSaveAs"
	);

	header->file_quit = gtk_action_group_get_action(
		header->group_file,
		"FileQuit"
	);
	
	header->edit_undo = gtk_action_group_get_action(
		header->group_edit,
		"EditUndo"
	);
	
	header->edit_redo = gtk_action_group_get_action(
		header->group_edit,
		"EditRedo"
	);
	
	header->edit_preferences = gtk_action_group_get_action(
		header->group_edit,
		"EditPreferences"
	);

	header->help_about = gtk_action_group_get_action(
		header->group_help,
		"HelpAbout"
	);

	return header;
}

void mape_header_destroy(MapeHeader* header)
{
	g_object_unref(G_OBJECT(header->ui_manager) );
	free(header);
}
