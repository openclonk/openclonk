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
#include <gtksourceview/gtksourcebuffer.h>

#include "mape/cpp-handles/version-handle.h"
#include "mape/fileicon.h"
#include "mape/configfile.h"
#include "mape/preferencesdialog.h"
#include "mape/header.h"
#include "mape/statusbar.h"
#include "mape/diskview.h"
#include "mape/mattexview.h"
#include "mape/editview.h"
#include "mape/preview.h"
#include "mape/window.h"

static gboolean mape_window_file_load(MapeWindow* wnd,
                                      const gchar* filename)
{
	GError* error = NULL;
	GtkWidget* error_dialog;
	gchar* title;
	gboolean result;

	result = mape_edit_view_open(wnd->edit_view, filename, &error);
		
	if(result == FALSE)
	{
		error_dialog = gtk_message_dialog_new(
			GTK_WINDOW(wnd->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_ERROR,
			GTK_BUTTONS_OK,
			"%s",
			error->message
		);

		gtk_dialog_run(GTK_DIALOG(error_dialog) );

		gtk_widget_destroy(error_dialog);
		g_error_free(error);

		return FALSE;
	}
	else
	{
		title = g_strdup_printf("%s - Mape", filename);
		gtk_window_set_title(GTK_WINDOW(wnd->window), title);
		g_free(title);

		return TRUE;
	}
}

static gboolean mape_window_file_save(MapeWindow* wnd,
                                      const gchar* filename)
{
	GError* error = NULL;
	GtkWidget* error_dialog;
	gchar* title;
	gboolean result;

	result = mape_edit_view_save(wnd->edit_view, filename, &error);
		
	if(result == FALSE)
	{
		error_dialog = gtk_message_dialog_new(
			GTK_WINDOW(wnd->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_ERROR,
			GTK_BUTTONS_OK,
			"%s",
			error->message
		);
			
		gtk_dialog_run(GTK_DIALOG(error_dialog) );
			
		gtk_widget_destroy(error_dialog);
		g_error_free(error);
		
		return FALSE;
	}
	else
	{
		title = g_strdup_printf("%s - Mape", wnd->edit_view->file_path);
		gtk_window_set_title(GTK_WINDOW(wnd->window), title);
		g_free(title);
		
		return TRUE;
	}
}

static GtkWidget* mape_window_create_file_chooser(MapeWindow* wnd,
                                                  GtkFileChooserAction action)
{
	GtkWidget* dialog;
	GtkFileFilter* filter_all;
	GtkFileFilter* filter_landscape;
	GtkFileFilter* filter_map;
	
	filter_all = gtk_file_filter_new();
	gtk_file_filter_set_name(filter_all, "All files");
	gtk_file_filter_add_pattern(filter_all, "*");
	
	filter_landscape = gtk_file_filter_new();
	gtk_file_filter_set_name(filter_landscape, "Dynamic Map Descriptions");
	gtk_file_filter_add_pattern(filter_landscape, "[Ll]andscape.txt");
	gtk_file_filter_add_pattern(filter_landscape, "[Mm]ap.c");

	dialog = gtk_file_chooser_dialog_new(
		action == GTK_FILE_CHOOSER_ACTION_OPEN ?
			"Open landscape file..." : "Save landscape file...",
		GTK_WINDOW(wnd->window),
		action,
		GTK_STOCK_CANCEL,
		GTK_RESPONSE_CANCEL,
		action == GTK_FILE_CHOOSER_ACTION_OPEN ?
			GTK_STOCK_OPEN : GTK_STOCK_SAVE,
		GTK_RESPONSE_OK,
		NULL
	);
	
	gtk_file_chooser_add_filter(
		GTK_FILE_CHOOSER(dialog),
		filter_landscape
	);
	
	gtk_file_chooser_add_filter(
		GTK_FILE_CHOOSER(dialog),
		filter_all
	);

	gtk_file_chooser_set_do_overwrite_confirmation(
		GTK_FILE_CHOOSER(dialog),
		TRUE
	);

	return dialog;
}

static void mape_window_cb_file_save_as(GtkWidget* widget,
                                        gpointer user_data)
{
	MapeWindow* wnd;
	gint response;
	gchar* filename;
	GtkWidget* save_dialog;

	wnd = (MapeWindow*)user_data;
	save_dialog = mape_window_create_file_chooser(
		wnd,
		GTK_FILE_CHOOSER_ACTION_SAVE
	);
	
	if(wnd->edit_view->file_path != NULL)
	{
		gtk_file_chooser_set_filename(
			GTK_FILE_CHOOSER(save_dialog),
			wnd->edit_view->file_path
		);
	}
	else if(wnd->last_path != NULL)
	{
		gtk_file_chooser_set_current_folder(
			GTK_FILE_CHOOSER(save_dialog),
			wnd->last_path
		);
	}
	
	response = gtk_dialog_run(GTK_DIALOG(save_dialog) );
	if(response == GTK_RESPONSE_OK)
	{
		filename = gtk_file_chooser_get_filename(
			GTK_FILE_CHOOSER(save_dialog)
		);
		
		g_free(wnd->last_path);
		wnd->last_path = gtk_file_chooser_get_current_folder(
			GTK_FILE_CHOOSER(save_dialog)
		);

		mape_window_file_save(wnd, filename);

		g_free(filename);
	}

	gtk_widget_destroy(save_dialog);
}

static void mape_window_cb_file_save(GtkWidget* widget,
                                     gpointer user_data)
{
	MapeWindow* wnd;
	wnd = (MapeWindow*)user_data;
	
	if(wnd->edit_view->file_path == NULL)
	{
		mape_window_cb_file_save_as(widget, user_data);
	}
	else
	{
		mape_window_file_save(wnd, wnd->edit_view->file_path);
	}
}

static gboolean mape_window_confirm_close(MapeWindow* wnd)
{
	GtkWidget* confirm_dialog;
	gint response;

	/* No need to confirm if the document has not been modified */
	if(mape_edit_view_get_modified(wnd->edit_view) == FALSE)
		return TRUE;

	confirm_dialog = gtk_message_dialog_new(
		GTK_WINDOW(wnd->window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_MESSAGE_WARNING,
		GTK_BUTTONS_NONE,
		"Save the changes to document before closing?"
	);
	
	gtk_message_dialog_format_secondary_text(
		GTK_MESSAGE_DIALOG(confirm_dialog),
		"Unsaved changes will be permanently lost."
	);
	
	gtk_dialog_add_buttons(
		GTK_DIALOG(confirm_dialog),
		"Close _without saving",
		GTK_RESPONSE_REJECT,
		GTK_STOCK_CANCEL,
		GTK_RESPONSE_CANCEL,
		GTK_STOCK_SAVE,
		GTK_RESPONSE_ACCEPT,
		NULL
	);

	response = gtk_dialog_run(GTK_DIALOG(confirm_dialog) );
	gtk_widget_destroy(confirm_dialog);
	
	switch(response)
	{
	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		return FALSE;
		break;
	case GTK_RESPONSE_REJECT:
		/* Document may be closed */
		return TRUE;
	case GTK_RESPONSE_ACCEPT:
		/* Save before closing */
		mape_window_cb_file_save(wnd->window, wnd);
		return TRUE;
	default:
		g_assert_not_reached();
		return FALSE;
	}
}

static void mape_window_cb_realize(GtkWidget* widget,
                                   gpointer user_data)
{
	MapeWindow* wnd;
	wnd = (MapeWindow*)user_data;

	gtk_paned_set_position(GTK_PANED(wnd->mid_paned), 200);
}

static void mape_window_cb_file_new(GtkWidget* widget,
                                    gpointer user_data)
{
	MapeWindow* wnd;
	wnd = (MapeWindow*)user_data;
	
	if(mape_window_confirm_close(wnd) == TRUE)
	{
		mape_edit_view_clear(wnd->edit_view);
		gtk_window_set_title(GTK_WINDOW(wnd->window), "Mape");
	}
}

static void mape_window_cb_file_open(GtkWidget* widget,
                                     gpointer user_data)
{
	MapeWindow* wnd;
	gint response;
	GtkWidget* open_dialog;
	gchar* filename;

	wnd = (MapeWindow*)user_data;
	if(mape_window_confirm_close(wnd) == FALSE)
		return;

	open_dialog = mape_window_create_file_chooser(
		wnd,
		GTK_FILE_CHOOSER_ACTION_OPEN
	);

	if(wnd->last_path != NULL)
	{
		gtk_file_chooser_set_current_folder(
			GTK_FILE_CHOOSER(open_dialog),
			wnd->last_path
		);
	}

	response = gtk_dialog_run(GTK_DIALOG(open_dialog) );
	if(response == GTK_RESPONSE_OK)
	{
		filename = gtk_file_chooser_get_filename(
			GTK_FILE_CHOOSER(open_dialog)
		);
		
		g_free(wnd->last_path);
		wnd->last_path = gtk_file_chooser_get_current_folder(
			GTK_FILE_CHOOSER(open_dialog)
		);

		mape_window_file_load(wnd, filename);

		g_free(filename);
	}

	gtk_widget_destroy(open_dialog);
}

static void mape_window_cb_file_quit(GtkWidget* widget,
                                     gpointer user_data)
{
	MapeWindow* wnd;
	wnd = (MapeWindow*)user_data;

	if(mape_window_confirm_close(wnd) == TRUE)
		gtk_widget_destroy(wnd->window);
}

static gboolean mape_window_cb_delete_event(GtkWidget* widget,
                                            GdkEvent* event,
                                            gpointer user_data)
{
	MapeWindow* wnd;
	wnd = (MapeWindow*)user_data;

	if(mape_window_confirm_close(wnd) == TRUE)
		return FALSE;
	else
		return TRUE;
}

static void mape_window_cb_edit_undo(GtkAction* action,
                                     gpointer user_data)
{
	MapeWindow* wnd;
	wnd = (MapeWindow*)user_data;

	mape_edit_view_undo(wnd->edit_view);
}

static void mape_window_cb_edit_redo(GtkAction* action,
                                     gpointer user_data)
{
	MapeWindow* wnd;
	wnd = (MapeWindow*)user_data;
	
	mape_edit_view_redo(wnd->edit_view);
}

static void mape_window_cb_edit_can_undo(GObject* object,
                                         GParamSpec* pspec,
                                         gpointer user_data)
{
	MapeWindow* wnd;
	wnd = (MapeWindow*)user_data;
	
	gtk_action_set_sensitive(
		wnd->header->edit_undo,
		gtk_source_buffer_can_undo(GTK_SOURCE_BUFFER(object))
	);
}

static void mape_window_cb_edit_can_redo(GObject* object,
                                         GParamSpec* pspec,
                                         gpointer user_data)
{
	MapeWindow* wnd;
	wnd = (MapeWindow*)user_data;
	
	gtk_action_set_sensitive(
		wnd->header->edit_redo,
		gtk_source_buffer_can_redo(GTK_SOURCE_BUFFER(object))
	);
}

static void mape_window_cb_edit_preferences(GtkAction* action,
                                            gpointer user_data)
{
	MapeWindow* wnd;
	MapePreferencesDialog* dialog;
	gint result;

	wnd = (MapeWindow*)user_data;
	
	dialog = mape_preferences_dialog_new(
		GTK_WINDOW(wnd->window),
		&wnd->preferences
	);

	result = gtk_dialog_run(GTK_DIALOG(dialog->dialog) );
	if(result == GTK_RESPONSE_OK)
	{
		wnd->preferences = mape_preferences_dialog_get(dialog);
		mape_preferences_to_config(&wnd->preferences, wnd->config);

		mape_edit_view_apply_preferences(
			wnd->edit_view,
			&wnd->preferences
		);
		mape_pre_view_apply_preferences(wnd->pre_view, &wnd->preferences);
	}

	mape_preferences_dialog_destroy(dialog);
}

static void mape_window_cb_help_about(GtkAction* action,
                                      gpointer user_data)
{
	MapeWindow* wnd;
	GtkWidget* about_dialog;

	const gchar* authors[] = {
		"Developers:",
		"  Armin Burgmeier <armin@arbur.net>",
		"",
		"Contributors:",
		"  Tyron Madlener <tm@tyron.at>",
		"  Günther Brammer <gbrammer@gmx.de>",
		NULL
	};

	const gchar* artists[] = {
		"Stefan \"KaZoiTeZ\" Ryll",
		NULL
	};

	wnd = (MapeWindow*)user_data;
	about_dialog = gtk_about_dialog_new();

	gtk_about_dialog_set_program_name(
		GTK_ABOUT_DIALOG(about_dialog),
		"Mape"
	);

	gtk_about_dialog_set_version(
		GTK_ABOUT_DIALOG(about_dialog),
		c4_version_get()
	);

	gtk_about_dialog_set_copyright(
		GTK_ABOUT_DIALOG(about_dialog),
		"Copyright \xc2\xa9 2005-2013 Armin Burgmeier"
	);

	gtk_about_dialog_set_comments(
		GTK_ABOUT_DIALOG(about_dialog),
		"A Clonk Landscape.txt editor"
	);

	gtk_about_dialog_set_authors(
		GTK_ABOUT_DIALOG(about_dialog),
		authors
	);

	gtk_about_dialog_set_artists(
		GTK_ABOUT_DIALOG(about_dialog),
		artists
	);

	gtk_about_dialog_set_license(
		GTK_ABOUT_DIALOG(about_dialog),
		"Permission to use, copy, modify, and/or distribute this "
		"software for any purpose with or without fee is hereby "
		"granted, provided that the above copyright notice and this "
		"permission notice appear in all copies.\n\n"

		"THE SOFTWARE IS PROVIDED \"AS IS\" AND THE AUTHOR DISCLAIMS "
		"ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL "
		"IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO "
		"EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, "
		"INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES "
		"WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, "
		"WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER "
		"TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE "
		"USE OR PERFORMANCE OF THIS SOFTWARE."
	);

	gtk_about_dialog_set_wrap_license(GTK_ABOUT_DIALOG(about_dialog), TRUE);

	gtk_dialog_run(GTK_DIALOG(about_dialog) );
	gtk_widget_destroy(about_dialog);
}

MapeWindow* mape_window_new(int argc,
                            char* argv[],
                            GError** error)
{
	MapeWindow* wnd;
	MapeConfigFileEntry* entry;
	gchar* config_path;
	gchar* material_path;
	gchar* current_dir;
	gchar* landscape_path;
	gchar* basename;
	int i;

	wnd = malloc(sizeof(MapeWindow) );

	/* Load preferences */
	config_path = g_build_filename(g_get_home_dir(), ".mape", NULL);
	wnd->config = mape_config_file_new(config_path);
	g_free(config_path);

	current_dir = g_get_current_dir();
	landscape_path = NULL;

	for(i = 1; i < argc; ++ i)
	{
		basename = g_path_get_basename(argv[i]);
		if(g_strcasecmp(basename, "Material.ocg") == 0)
		{
			if(!g_path_is_absolute(argv[i]))
			{
				material_path = g_build_filename(
					current_dir,
					argv[i],
					NULL
				);
			}
			else
			{
				material_path = g_strdup(argv[i]);
			}

			mape_config_file_set_entry(
				wnd->config,
				"material_path",
				material_path
			);

			g_free(material_path);
		}
		else
		{
			g_free(landscape_path);

			if(!g_path_is_absolute(argv[i]))
			{
				landscape_path = g_build_filename(
					current_dir,
					argv[i],
					NULL
				);
			}
			else
			{
				landscape_path = g_strdup(argv[i]);
			}
		}

		g_free(basename);
	}

	g_free(current_dir);
	
	mape_preferences_from_config(&wnd->preferences, wnd->config);

	wnd->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	wnd->icon_set = mape_file_icon_set_new(wnd->window);
	wnd->last_path = NULL;

	wnd->header = mape_header_new();
	wnd->statusbar = mape_statusbar_new();

	wnd->mat_tex_view = mape_mat_tex_view_new(wnd->icon_set, error);
	if(wnd->mat_tex_view == NULL)
		goto mat_tex_error;

	wnd->pre_view = mape_pre_view_new(wnd->mat_tex_view, error);
	if(wnd->pre_view == NULL)
		goto pre_view_error;

	wnd->edit_view = mape_edit_view_new(
		wnd->pre_view,
		wnd->statusbar,
		error
	);
	if(wnd->edit_view == NULL)
		goto edit_view_error;
	
	wnd->disk_view = mape_disk_view_new(
		wnd->icon_set,
		wnd->mat_tex_view,
		wnd->edit_view,
		wnd->config,
		error
	);

	if(wnd->disk_view == NULL)
		goto disk_view_error;

	wnd->menubar = wnd->header->menubar;
	gtk_widget_show(wnd->menubar);

	wnd->toolbar = wnd->header->toolbar;
	gtk_toolbar_set_style(GTK_TOOLBAR(wnd->toolbar), GTK_TOOLBAR_ICONS);
	gtk_widget_show(wnd->toolbar);
	
	wnd->mid_paned = gtk_hpaned_new();
	gtk_paned_pack1(
		GTK_PANED(wnd->mid_paned),
		wnd->disk_view->window,
		TRUE,
		FALSE
	);

	gtk_paned_pack2(
		GTK_PANED(wnd->mid_paned),
		wnd->edit_view->window,
		TRUE,
		FALSE
	);

	gtk_widget_show(wnd->mid_paned);
	
	wnd->bottom_hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(
		GTK_BOX(wnd->bottom_hbox),
		wnd->mat_tex_view->notebook,
		TRUE,
		TRUE,
		0
	);
	
	gtk_box_pack_start(
		GTK_BOX(wnd->bottom_hbox),
		wnd->pre_view->frame,
		FALSE,
		FALSE,
		0
	);
	gtk_widget_show(wnd->bottom_hbox);

	wnd->topbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(
		GTK_BOX(wnd->topbox),
		wnd->menubar,
		FALSE,
		TRUE,
		0
	);

	gtk_box_pack_start(
		GTK_BOX(wnd->topbox),
		wnd->toolbar,
		FALSE,
		TRUE,
		0
	);

	gtk_box_pack_start(
		GTK_BOX(wnd->topbox),
		wnd->mid_paned,
		TRUE,
		TRUE,
		0
	);

	gtk_box_pack_start(
		GTK_BOX(wnd->topbox),
		wnd->bottom_hbox,
		FALSE,
		TRUE,
		0
	);
	
	gtk_box_pack_start(
		GTK_BOX(wnd->topbox),
		wnd->statusbar->bar,
		FALSE,
		TRUE,
		0
	);

	gtk_widget_show(wnd->topbox);

	gtk_container_add(
		GTK_CONTAINER(wnd->window),
		wnd->topbox
	);

	gtk_window_add_accel_group(
		GTK_WINDOW(wnd->window),
		wnd->header->accel_group
	);
	
	g_signal_connect(
		G_OBJECT(wnd->window),
		"realize",
		G_CALLBACK(mape_window_cb_realize),
		wnd
	);
	
	g_signal_connect(
		G_OBJECT(wnd->window),
		"delete-event",
		G_CALLBACK(mape_window_cb_delete_event),
		wnd
	);
	
	gtk_widget_grab_focus(wnd->edit_view->view);
	
	g_signal_connect(
		G_OBJECT(wnd->header->file_new),
		"activate",
		G_CALLBACK(mape_window_cb_file_new),
		wnd
	);
	
	g_signal_connect(
		G_OBJECT(wnd->header->file_open),
		"activate",
		G_CALLBACK(mape_window_cb_file_open),
		wnd
	);
	
	g_signal_connect(
		G_OBJECT(wnd->header->file_save),
		"activate",
		G_CALLBACK(mape_window_cb_file_save),
		wnd
	);
	
	g_signal_connect(
		G_OBJECT(wnd->header->file_save_as),
		"activate",
		G_CALLBACK(mape_window_cb_file_save_as),
		wnd
	);
	
	g_signal_connect(
		G_OBJECT(wnd->header->file_quit),
		"activate",
		G_CALLBACK(mape_window_cb_file_quit),
		wnd
	);
	
	g_signal_connect(
		G_OBJECT(wnd->header->edit_undo),
		"activate",
		G_CALLBACK(mape_window_cb_edit_undo),
		wnd
	);
	
	g_signal_connect(
		G_OBJECT(wnd->header->edit_redo),
		"activate",
		G_CALLBACK(mape_window_cb_edit_redo),
		wnd
	);

	g_signal_connect(
		G_OBJECT(
			gtk_text_view_get_buffer(
				GTK_TEXT_VIEW(wnd->edit_view->view)
			)
		),
		"notify::can-undo",
		G_CALLBACK(mape_window_cb_edit_can_undo),
		wnd
	);
	
	g_signal_connect(
		G_OBJECT(
			gtk_text_view_get_buffer(
				GTK_TEXT_VIEW(wnd->edit_view->view)
			)
		),
		"notify::can-redo",
		G_CALLBACK(mape_window_cb_edit_can_redo),
		wnd
	);

	g_signal_connect(
		G_OBJECT(wnd->header->edit_preferences),
		"activate",
		G_CALLBACK(mape_window_cb_edit_preferences),
		wnd
	);
	
	g_signal_connect(
		G_OBJECT(wnd->header->help_about),
		"activate",
		G_CALLBACK(mape_window_cb_help_about),
		wnd
	);
	
	gtk_action_set_sensitive(wnd->header->edit_undo, FALSE);
	gtk_action_set_sensitive(wnd->header->edit_redo, FALSE);

	mape_edit_view_apply_preferences(wnd->edit_view, &wnd->preferences);
	mape_pre_view_apply_preferences(wnd->pre_view, &wnd->preferences);

	/* Load initial path in disk view, if any. Note this needs to go
	 * after applying preferences in edit_view, so that the map size is
	 * correct in case the newly loaded Material.ocg triggers a map update. */
	entry = mape_config_file_get_entry_by_key(
		wnd->config,
		"material_path"
	);

	if(entry != NULL)
	{
		mape_disk_view_extend_to_path(
			wnd->disk_view,
			mape_config_file_entry_get_value(entry),
			NULL
		);
	}

	gtk_window_set_title(GTK_WINDOW(wnd->window), "Mape");
	gtk_window_set_default_size(GTK_WINDOW(wnd->window), 640, 480);

	/* Load initial landscape */
	if(landscape_path != NULL)
	{
		mape_window_file_load(wnd, landscape_path);
		g_free(landscape_path);
	}

	gtk_widget_show(wnd->window);

	return wnd;

disk_view_error:
	mape_edit_view_destroy(wnd->edit_view);
	
edit_view_error:
	mape_pre_view_destroy(wnd->pre_view);

pre_view_error:
	mape_mat_tex_view_destroy(wnd->mat_tex_view);

mat_tex_error:
	g_free(landscape_path);

	mape_header_destroy(wnd->header);
	mape_statusbar_destroy(wnd->statusbar);

	mape_file_icon_set_destroy(wnd->icon_set);
	gtk_widget_destroy(wnd->window);
	mape_config_file_destroy(wnd->config);
	free(wnd);

	return NULL;
}

void mape_window_destroy(MapeWindow* wnd)
{
	mape_pre_view_destroy(wnd->pre_view);
	mape_edit_view_destroy(wnd->edit_view);
	mape_disk_view_destroy(wnd->disk_view);
	mape_mat_tex_view_destroy(wnd->mat_tex_view);
	mape_header_destroy(wnd->header);
	mape_file_icon_set_destroy(wnd->icon_set);

	mape_config_file_serialise(wnd->config, NULL);
	mape_config_file_destroy(wnd->config);
	g_free(wnd->last_path);
	free(wnd);
}
