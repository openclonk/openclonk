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
#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourcebuffer.h>
#include "mape/mapgen.h"
#include "mape/random.h"
#include "mape/preferences.h"
#include "mape/mattexview.h"
#include "mape/editview.h"
#include "mape/statusbar.h"
#include "mape/preview.h"

typedef struct _ThreadData ThreadData;
struct _ThreadData {
	MapeEditView* view;
	gchar* source;
	gchar* file_path;
	MapeMaterialMap* mat_map;
	MapeTextureMap* tex_map;
	guint map_width;
	guint map_height;
};

typedef struct _ThreadResult ThreadResult;
struct _ThreadResult {
	MapeEditView* view;
	GdkPixbuf* pixbuf;
	GError* error;
	guint idle_id;
};

static void
mape_edit_view_thread_result_destroy_func(gpointer data)
{
	ThreadResult* result = (ThreadResult*)data;
	if(result->pixbuf != NULL) g_object_unref(G_OBJECT(result->pixbuf));
	if(result->error != NULL) g_error_free(result->error);
	/* Don't need to g_source_remove(result->idle_id) since this is
	 * only called when the idle handler is removed anyway. */
}

static void mape_edit_view_cb_changed(GtkTextBuffer* buffer,
                                      gpointer user_data)
{
	mape_edit_view_reload((MapeEditView*)user_data);
}

static void mape_edit_view_cb_update(GtkWidget* widget,
                                     GdkEvent* event,
                                     gpointer user_data)
{
	mape_edit_view_reload((MapeEditView*)user_data);
}

static GdkPixbuf* mape_edit_view_render_map(const gchar* source,
                                            const gchar* file_path,
                                            MapeMaterialMap* mat_map,
                                            MapeTextureMap* tex_map,
                                            guint map_width,
                                            guint map_height,
                                            GError** error)
{
	GdkPixbuf* pixbuf;
	gchar* basename;
	gchar* dirname;
	gchar* scriptname;
	const gchar* filename;

	if(mat_map == NULL || tex_map == NULL)
	{
		g_set_error(
			error,
			g_quark_from_static_string("MAPE_EDIT_VIEW_ERROR"),
			MAPE_EDIT_VIEW_ERROR_MISSING_MAPS,
			"Material.ocg is not loaded"
		);

		return NULL;
	}

	if(file_path != NULL)
	{
		basename = g_path_get_basename(file_path);
		filename = basename;

		dirname = g_path_get_dirname(file_path);
		scriptname = g_build_filename(dirname, "Script.c", NULL);
		g_free(dirname);
	}
	else
	{
		basename = NULL;
		filename = "Landscape.txt";
		scriptname = NULL;
	}

	pixbuf = mape_mapgen_render(
		filename,
		source,
		scriptname,
		mat_map,
		tex_map,
		map_width,
		map_height,
		error
	);

	g_free(basename);
	return pixbuf;
}

static gboolean
mape_edit_view_thread_result(gpointer data_)
{
	ThreadResult* result;
	MapeEditView* view;

	result = (ThreadResult*)data_;
	view = result->view;

	if(result->pixbuf == NULL)
	{
		g_assert(result->error != NULL);

		mape_statusbar_set_compile(
			view->statusbar,
			result->error->message
		);
	}
	else
	{
		mape_statusbar_set_compile(
			view->statusbar,
			"Landscape rendered successfully"
		);
	}

	mape_pre_view_update(view->pre_view, result->pixbuf);

	/* Finish thread */
	g_thread_join(view->render_thread);
	view->render_thread = NULL;

	/* Do we have to render again (someone changed
	   the source while rendering)? */
	if(view->rerender == TRUE)
		mape_edit_view_reload(view);

	return FALSE;
}

static gpointer mape_edit_view_thread_entry(gpointer data_)
{
	ThreadData* data;
	ThreadResult* result;
	GError* error = NULL;
	GdkPixbuf* res_buf;

	data = (ThreadData*)data_;

	res_buf = mape_edit_view_render_map(
		data->source,
		data->file_path,
		data->mat_map,
		data->tex_map,
		data->map_width,
		data->map_height,
		&error
	);

	result = g_slice_new(ThreadResult);
	result->view = data->view;
	result->pixbuf = res_buf;
	result->error = error;

	g_free(data->source);
	g_free(data->file_path);
	g_slice_free(ThreadData, data);

	result->idle_id = g_idle_add_full(
		G_PRIORITY_DEFAULT_IDLE,
		mape_edit_view_thread_result,
		result,
		mape_edit_view_thread_result_destroy_func
	);

	/* Note that you can only use this securly with g_thread_join,
	 * otherwise the idle handler might already have been run */
	return result;
}

MapeEditView* mape_edit_view_new(MapePreView* pre_view,
                                 MapeStatusbar* statusbar,
                                 GError** error)
{
	MapeEditView* view;
	GtkSourceBuffer* buf;
	GtkSourceLanguage* lang;
	GtkSourceStyleScheme* style;
	GtkWidget* error_dialog;
	GPtrArray* search_dirs;
	const gchar* const* data_dirs;
	const gchar* const* dir;

	view = malloc(sizeof(MapeEditView) );
	view->pre_view = pre_view;
	view->statusbar = statusbar;
	view->file_path = NULL;
	view->encoding = "UTF-8";
	view->render_thread = NULL;
	view->rerender = FALSE;
	view->fixed_seed = FALSE;

	view->view = gtk_source_view_new();
	buf = GTK_SOURCE_BUFFER(
		gtk_text_view_get_buffer(GTK_TEXT_VIEW(view->view) )
	);

	g_signal_connect_after(
		G_OBJECT(buf),
		"changed",
		G_CALLBACK(mape_edit_view_cb_changed),
		view
	);

	g_signal_connect(
		G_OBJECT(pre_view->event_box),
		"button-press-event",
		G_CALLBACK(mape_edit_view_cb_update),
		view
	);

	view->font_desc = pango_font_description_new();
	pango_font_description_set_family(view->font_desc, "monospace");
	gtk_widget_modify_font(view->view, view->font_desc);

	search_dirs = g_ptr_array_new();
#ifdef G_OS_WIN32
	g_ptr_array_add(
		search_dirs,
		g_win32_get_package_installation_subdirectory(NULL, NULL, "mape-syntax")
	);
#endif

	g_ptr_array_add(
		search_dirs,
		g_build_filename(g_get_home_dir(), ".mape-syntax", NULL)
	);

	data_dirs = g_get_system_data_dirs();
	for(dir = data_dirs; *dir != NULL; ++ dir)
		g_ptr_array_add(search_dirs, g_build_filename(*dir, "mape", NULL));
	g_ptr_array_add(search_dirs, NULL);

	view->lang_manager = gtk_source_language_manager_new();
	gtk_source_language_manager_set_search_path(
		view->lang_manager,
		(gchar**)search_dirs->pdata
	);

	view->style_manager = gtk_source_style_scheme_manager_new();
	gtk_source_style_scheme_manager_set_search_path(
	  view->style_manager,
	  (gchar**)search_dirs->pdata
	);

	g_ptr_array_foreach(search_dirs, (GFunc)g_free, NULL);
	g_ptr_array_free(search_dirs, TRUE);

	lang = gtk_source_language_manager_get_language(
		view->lang_manager,
		"c4landscape"
	);

	style = gtk_source_style_scheme_manager_get_scheme(
	  view->style_manager,
	  "c4landscape"
	);

	if(lang == NULL || style == NULL)
	{
		/* TODO: Show location where we search in */
		error_dialog = gtk_message_dialog_new(
			NULL,
			GTK_DIALOG_MODAL,
			GTK_MESSAGE_ERROR,
			GTK_BUTTONS_OK,
			"The syntax highlighting file for Landscape.txt files "
			"could not be located. Perhaps mape has not been "
			"properly installed. Syntax highlighting is disabled."
		);

		gtk_window_set_title(GTK_WINDOW(error_dialog), "Mape");

		gtk_dialog_run(GTK_DIALOG(error_dialog) );
		gtk_widget_destroy(error_dialog);
	}
	else
	{
		gtk_source_buffer_set_language(buf, lang);
		gtk_source_buffer_set_style_scheme(buf, style);
	}

	gtk_widget_show(view->view);
	
	view->window = gtk_scrolled_window_new(NULL, NULL);
	
	gtk_scrolled_window_set_policy(
		GTK_SCROLLED_WINDOW(view->window),
		GTK_POLICY_AUTOMATIC,
		GTK_POLICY_AUTOMATIC
	);
	
	gtk_scrolled_window_set_shadow_type(
		GTK_SCROLLED_WINDOW(view->window),
		GTK_SHADOW_IN
	);
	
	gtk_container_add(
		GTK_CONTAINER(view->window),
		view->view
	);

	gtk_widget_show(view->window);
	return view;
}

void mape_edit_view_destroy(MapeEditView* view)
{
	gpointer data;

	/* Wait for render thread to finish */
	if(view->render_thread != NULL)
	{
		data = g_thread_join(view->render_thread);
		view->render_thread = NULL;

		/* Don't let idle handler run since edit_view is about to be destroyed */
		g_assert(((ThreadResult*)data)->idle_id != 0);
		g_source_remove(((ThreadResult*)data)->idle_id);
	}

	g_object_unref(G_OBJECT(view->lang_manager) );
	g_object_unref(G_OBJECT(view->style_manager));
	g_free(view->file_path);
	free(view);
}

void mape_edit_view_clear(MapeEditView* view)
{
	g_free(view->file_path);
	view->file_path = NULL;

	/* TODO: Undoable action dingsen */
	/* (statische mape_edit_view_set_contents-Call?) */
	gtk_text_buffer_set_text(
		gtk_text_view_get_buffer(GTK_TEXT_VIEW(view->view)),
		"",
		0
	);
	
	gtk_text_buffer_set_modified(
		gtk_text_view_get_buffer(GTK_TEXT_VIEW(view->view)),
		FALSE
	);
}

gboolean mape_edit_view_open(MapeEditView* view,
                             const gchar* filename,
                             GError** error)
{
	gboolean result;
	gchar* contents;
	gchar* conv;
	gchar* utf8_file;
	gchar* new_path;
	gsize length;

	result = g_file_get_contents(filename, &contents, &length, error);
	if(result == FALSE) return FALSE;

	/* Assume UTF-8 */
	result = g_utf8_validate(contents, length, NULL);

	if(result == FALSE)
	{
		/* No UTF-8, try LATIN1 */
		conv = g_convert(
			contents,
			-1,
			"UTF-8",
			"LATIN1",
			NULL,
			NULL,
			NULL
		);

		g_free(contents);
		if(conv == NULL)
		{
			utf8_file = g_filename_to_utf8(
				filename,
				-1,
				NULL,
				NULL,
				NULL
			);

			if(utf8_file == NULL)
				utf8_file = g_strdup("<unknown file name>");

			g_set_error(
				error,
				g_quark_from_static_string(
					"MAPE_EDIT_VIEW_ERROR"
				),
				MAPE_EDIT_VIEW_ERROR_UNKNOWN_ENCODING,
				"Could not read file %s: Either the encoding "
				"is unknown or it is a binary file",
				utf8_file
			);

			g_free(utf8_file);
			return FALSE;
		}

		/* Conversion succeeded */
		contents = conv;
		view->encoding = "LATIN1";
	}
	else
	{
		view->encoding = "UTF-8";
	}

	/* TODO: Verify that filename is absolute and make it absolute if
	   it is not */
	new_path = g_strdup(filename);
	g_free(view->file_path);
	view->file_path = new_path;

	/* TODO: Undoable action dingsen */
	/* (statische mape_edit_view_set_contents-Call?) */
	gtk_text_buffer_set_text(
		gtk_text_view_get_buffer(GTK_TEXT_VIEW(view->view)),
		contents,
		length
	);

	g_free(contents);

	gtk_text_buffer_set_modified(
		gtk_text_view_get_buffer(GTK_TEXT_VIEW(view->view)),
		FALSE
	);

	return TRUE;
}

gboolean mape_edit_view_save(MapeEditView* view,
                             const gchar* filename,
                             GError** error)
{
	GtkTextBuffer* buffer;
	GtkTextIter begin;
	GtkTextIter end;
	gchar* new_path;
	gchar* source;
	gchar* conv;
	gboolean result;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view->view) );
	gtk_text_buffer_get_start_iter(buffer, &begin);
	gtk_text_buffer_get_end_iter(buffer, &end);
	source = gtk_text_buffer_get_text(buffer, &begin, &end, TRUE);

	conv = g_convert(
		source,
		-1,
		view->encoding,
		"UTF-8",
		NULL,
		NULL,
		error
	);

	g_free(source);
	if(conv == NULL) return FALSE;

	result = g_file_set_contents(filename, conv, -1, error);
	g_free(conv);
	if(result == FALSE) return FALSE;
	
	gtk_text_buffer_set_modified(
		gtk_text_view_get_buffer(GTK_TEXT_VIEW(view->view)),
		FALSE
	);

	new_path = g_strdup(filename);
	g_free(view->file_path);
	view->file_path = new_path;

	/* Rerender with new file path --
	 * different Script.c lookup for algo=script overlays */
	mape_edit_view_reload(view);

	return TRUE;
}

gboolean mape_edit_view_get_modified(MapeEditView* view)
{
	return gtk_text_buffer_get_modified(
		gtk_text_view_get_buffer(GTK_TEXT_VIEW(view->view))
	);
}

void mape_edit_view_undo(MapeEditView* edit_view)
{
	gtk_source_buffer_undo(
		GTK_SOURCE_BUFFER(
			gtk_text_view_get_buffer(GTK_TEXT_VIEW(edit_view->view))
		)
	);
}

void mape_edit_view_redo(MapeEditView* edit_view)
{
	gtk_source_buffer_redo(
		GTK_SOURCE_BUFFER(
			gtk_text_view_get_buffer(GTK_TEXT_VIEW(edit_view->view))
		)
	);
}

void mape_edit_view_apply_preferences(MapeEditView* edit_view,
                                      MapePreferences* preferences)
{
	GtkSourceView* view;
	GtkSourceBuffer* buffer;
	GtkWrapMode wrap_mode;
	
	view = GTK_SOURCE_VIEW(edit_view->view);
	buffer = GTK_SOURCE_BUFFER(
		gtk_text_view_get_buffer(GTK_TEXT_VIEW(view))
	);

	gtk_source_view_set_tab_width(
		view,
		preferences->tab_width
	);

	gtk_source_view_set_insert_spaces_instead_of_tabs(
		view,
		preferences->tab_to_spaces
	);

	gtk_source_view_set_auto_indent(
		view,
		preferences->auto_indentation
	);

	wrap_mode = GTK_WRAP_CHAR;
	if(preferences->text_wrapping == FALSE)
		wrap_mode = GTK_WRAP_NONE;
	gtk_text_view_set_wrap_mode(
		GTK_TEXT_VIEW(view),
		wrap_mode
	);
	
	gtk_source_view_set_show_line_numbers(
		view,
		preferences->line_numbers
	);

	gtk_source_view_set_highlight_current_line(
		view,
		preferences->highlight_line
	);
	
	gtk_source_buffer_set_highlight_matching_brackets(
		buffer,
		preferences->bracket_matching
	);

	edit_view->fixed_seed = preferences->fixed_seed;
	edit_view->random_seed = preferences->random_seed;
	edit_view->map_width = preferences->map_width;
	edit_view->map_height = preferences->map_height;

	/* Rerender with new random settings */
	mape_edit_view_reload(edit_view);
}

void mape_edit_view_reload(MapeEditView* edit_view)
{
	GError* error = NULL;
	ThreadData* data;
	GtkTextBuffer* buffer;
	GtkTextIter begin;
	GtkTextIter end;

	if(edit_view->render_thread == NULL)
	{
		data = g_slice_new(ThreadData);

		buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(edit_view->view) );
		gtk_text_buffer_get_start_iter(buffer, &begin);
		gtk_text_buffer_get_end_iter(buffer, &end);

		/* TODO: We need to ref view so that it is guaranteed to be alive in the 
		 * thread result handler */
		data->view = edit_view;
		data->source = gtk_text_buffer_get_text(buffer, &begin, &end, TRUE);
		data->file_path = g_strdup(edit_view->file_path);

		/* TODO: We need to ref these so noone can delete them while the thread
		 * uses them. */
		data->mat_map = edit_view->pre_view->mat_tex->mat_map,
		data->tex_map = edit_view->pre_view->mat_tex->tex_map,

		data->map_width = edit_view->map_width;
		data->map_height = edit_view->map_height;

		if(edit_view->fixed_seed == TRUE)
			mape_random_seed(edit_view->random_seed);

		mape_statusbar_set_compile(
			edit_view->statusbar,
			"Rendering map..."
		);

		edit_view->rerender = FALSE;

		edit_view->render_thread = g_thread_create(
			mape_edit_view_thread_entry,
			data,
			TRUE,
			&error
		);
		
		if(edit_view->render_thread == NULL)
		{
			mape_statusbar_set_compile(
				edit_view->statusbar,
				error->message
			);

			g_free(data->source);
			g_slice_free(ThreadData, data);

			g_error_free(error);
		}
	}
	else
	{
		/* Rerender when thread finished */
		edit_view->rerender = TRUE;
	}
}

