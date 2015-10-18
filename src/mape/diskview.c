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
#include <string.h>

#include <gdk/gdkkeysyms.h>

#include "mape/group.h"
#include "mape/fileicon.h"
#include "mape/configfile.h"
#include "mape/mattexview.h"
#include "mape/editview.h"
#include "mape/diskview.h"

static gboolean mape_disk_view_find_iter(MapeDiskView* disk_view,
                                         GtkTreeIter* child,
                                         GtkTreeIter* parent,
                                         const gchar* file)
{
	gboolean result;
	gchar* filename;

	result = gtk_tree_model_iter_children(
		disk_view->tree_store,
		child,
		parent
	);

	while(result == TRUE)
	{
		gtk_tree_model_get(
			disk_view->tree_store,
			child,
			MAPE_DISK_VIEW_COLUMN_FILE,
			&filename,
			-1
		);

		if(g_strcasecmp(filename, file) == 0)
		{
			g_free(filename);
			return TRUE;
		}
		
		g_free(filename);
		result = gtk_tree_model_iter_next(disk_view->tree_store, child);
	}
	
	return FALSE;
}

static gchar* mape_disk_view_get_file_path(MapeDiskView* disk_view,
                                           GtkTreeIter* file)
{
	gboolean result;
	GtkTreeIter parent;
	GtkTreeIter child;
	gchar* cur_path;
	gchar* component;
	gchar* temp_path;

	child = *file;
	
	gtk_tree_model_get(
		disk_view->tree_store,
		file,
		MAPE_DISK_VIEW_COLUMN_FILE,
		&cur_path,
		-1
	);

	for(;;)
	{
		result = gtk_tree_model_iter_parent(
			disk_view->tree_store,
			&parent,
			&child
		);
		
		if(result == FALSE)
			break;
		
		gtk_tree_model_get(
			disk_view->tree_store,
			&parent,
			MAPE_DISK_VIEW_COLUMN_FILE,
			&component,
			-1
		);

#ifdef G_OS_WIN32
		temp_path = g_build_filename(component, cur_path, NULL);
#else
		/* Add leading "/" to filename for absolute path */
		temp_path = g_build_filename("/", component, cur_path, NULL);
#endif
		g_free(component); g_free(cur_path);
		cur_path = temp_path;

		child = parent;
	}

	return cur_path;
}

static gboolean mape_disk_view_load_materials(MapeDiskView* disk_view,
                                              GtkTreeIter* material_iter,
                                              GError** error)
{
	GtkTreeIter parent_iter;
	GtkTreeIter new_parent;
	gboolean has_parent;
	gboolean result;
	gchar* mat_path;

	MapeGroup* group;
	MapeGroup* parent_group;
	MapeGroup* overloaded_group;

	MapeTextureMap* texture_map;
	gboolean overload_materials;
	gboolean overload_textures;

	/* Open Material.ocg group */
	gtk_tree_model_get(
		disk_view->tree_store,
		material_iter,
		MAPE_DISK_VIEW_COLUMN_GROUP,
		&group,
		-1
	);

	has_parent = gtk_tree_model_iter_parent(
		disk_view->tree_store,
		&parent_iter,
		material_iter
	);
		
	if(has_parent == TRUE)
	{
		gtk_tree_model_get(
			disk_view->tree_store,
			&parent_iter,
			MAPE_DISK_VIEW_COLUMN_GROUP,
			&parent_group,
			-1
		);
			
		g_assert(parent_group != NULL);
	}
	else
	{
		parent_group = disk_view->group_top;
	}
		
	if(group == NULL)
	{
		group = mape_group_open_child(
			parent_group,
			"Material.ocg",
			error
		);

		if(group == NULL) return FALSE;

		gtk_tree_store_set(
			GTK_TREE_STORE(disk_view->tree_store),
			material_iter,
			MAPE_DISK_VIEW_COLUMN_GROUP,
			group,
			-1
		);
	}

	/* Load texture map. Note that this effectively only reads the
	 * OverloadMaterials and OverloadTextures flags, because the indices
	 * cannot be assigned yet, because materials and textures have not
	 * been loaded. However, materials and textures can only be loaded
	 * once we know whether we are overloading or not. */
	texture_map = mape_texture_map_new();
	if(mape_texture_map_load_map(texture_map, group, error) == FALSE)
	{
		g_object_unref(texture_map);
		return FALSE;
	}

	overload_materials = mape_texture_map_get_overload_materials(texture_map);
	overload_textures = mape_texture_map_get_overload_textures(texture_map);

	overloaded_group = NULL;
	if(overload_materials || overload_textures)
	{
		/* Look for overloaded Material.ocg */
		has_parent = gtk_tree_model_iter_parent(
			disk_view->tree_store,
			&parent_iter,
			material_iter
		);

		for(;;)
		{
			has_parent = gtk_tree_model_iter_parent(
				disk_view->tree_store,
				&new_parent,
				&parent_iter
			);
		
			if(has_parent == FALSE)
				break;

			parent_iter = new_parent;
		
			gtk_tree_model_get(
				disk_view->tree_store,
				&parent_iter,
				MAPE_DISK_VIEW_COLUMN_GROUP,
				&overloaded_group,
				-1
			);
		
			if(mape_group_has_entry(overloaded_group, "Material.ocg") ==
				 TRUE)
			{
				/* TODO: Check if the group is already open!
					 (mape_disk_view_find_iter). */
				overloaded_group = mape_group_open_child(
					overloaded_group,
					"Material.ocg",
					error
				);

				if(overloaded_group == NULL)
				{
					g_object_unref(texture_map);
					return FALSE;
				}

				break;
			}
		}
	}

	mat_path = mape_disk_view_get_file_path(disk_view, material_iter);
	mape_config_file_set_entry(
		disk_view->config,
		"material_path",
		mat_path
	);
	g_free(mat_path);

	result = mape_mat_tex_view_reload(
		disk_view->mat_tex,
		texture_map,
		group,
		overload_materials,
		overload_textures,
		overloaded_group,
		error
	);

	g_object_unref(texture_map);
	mape_mapgen_set_root_group(parent_group);

#if 0
	if(overloaded_group != NULL)
		mape_group_destroy(overloaded_group);
#endif

	mape_edit_view_reload(disk_view->edit_view);
	
	return result;
}

static gboolean mape_disk_view_load(MapeDiskView* disk_view,
                                    GtkTreeIter* parent_iter,
                                    GError** error)
{
	GtkTreeIter child_iter;
	GtkTreeIter group_iter;
	GtkTreeIter temp_iter;

	MapeGroup* child_group;
	MapeGroup* parent_group;

	MapeFileIcon* icon;
	MapeFileIconType icon_type;
	const char* fileext;
	char* filename;
	char* utf8_file;
	gboolean deplevel;

	if(parent_iter != NULL)
	{
		gtk_tree_model_get(
			disk_view->tree_store,
			parent_iter,
			MAPE_DISK_VIEW_COLUMN_GROUP,
			&parent_group,
			-1
		);

		/* Group is already open */
		if(parent_group != NULL)
			return TRUE;

		/* Get parent group */
		deplevel = gtk_tree_model_iter_parent(
			disk_view->tree_store,
			&group_iter,
			parent_iter
		);

		if(deplevel == TRUE)
		{	
			gtk_tree_model_get(
				disk_view->tree_store,
				&group_iter,
				MAPE_DISK_VIEW_COLUMN_GROUP,
				&parent_group,
				-1
			);

			g_assert(parent_group != NULL);
		}
		else
		{
			/* Parent is at top-level */
			parent_group = disk_view->group_top;
		}
	}
	else
	{
		parent_group = NULL;
	}

	if(parent_group != NULL)
	{
		gtk_tree_model_get(
			disk_view->tree_store,
			parent_iter,
			MAPE_DISK_VIEW_COLUMN_FILE,
			&utf8_file,
			-1
		);

#ifdef G_OS_WIN32
		filename = g_convert(
			utf8_file,
			-1,
			"LATIN1",
			"UTF-8",
			NULL,
			NULL,
			NULL
		);
#else
		filename = g_filename_from_utf8(
			utf8_file,
			-1,
			NULL,
			NULL,
			NULL
		);
#endif

		g_free(utf8_file);

		child_group = mape_group_open_child(
			parent_group,
			filename,
			error
		);

		g_free(filename);

		if(child_group == NULL)
			return FALSE;

		/* Store child group in parent */
		gtk_tree_store_set(
			GTK_TREE_STORE(disk_view->tree_store),
			parent_iter,
			MAPE_DISK_VIEW_COLUMN_GROUP,
			(gpointer)child_group,
			-1
		);
	}
	else
	{
		/* Group is already open */
		if(disk_view->group_top != NULL)
			return TRUE;

		disk_view->group_top = mape_group_new();
		if(!mape_group_open(disk_view->group_top, "/", error))
		{
			g_object_unref(disk_view->group_top);
			disk_view->group_top = NULL;
			return FALSE;
		}

		child_group = disk_view->group_top;
	}	

	while( (filename = mape_group_get_next_entry(child_group)) != NULL)
	{
		/* Check if this entry is a directory (we are hiding files). */
		if(mape_group_is_child_folder(child_group, filename) == FALSE)
		{
			g_free(filename);
			continue;
		}

#ifdef G_OS_WIN32
		utf8_file = g_convert(
			filename,
		       	-1,
		       	"UTF-8",
		       	"LATIN1",
		       	NULL,
		       	NULL,
		       	NULL
		);
#else
		utf8_file = g_filename_to_utf8(
			filename,
			-1,
			NULL,
			NULL,
			NULL
		);
#endif

		g_free(filename);
		filename = utf8_file;

		/* Invalid file name */
		if(filename == NULL)
			continue;

		gtk_tree_store_append(
			GTK_TREE_STORE(disk_view->tree_store),
			&child_iter,
			parent_iter
		);

		/* Create temporary entry to show the expander arrow */
		if(g_strcasecmp(filename, "Material.ocg") != 0)
		{
			gtk_tree_store_append(
				GTK_TREE_STORE(disk_view->tree_store),
				&temp_iter,
				&child_iter
			);
		}

		fileext = strrchr(filename, '.');
		icon_type = MAPE_FILE_ICON_FOLDER;

		if(fileext != NULL)
		{
			if(g_strcasecmp(fileext, ".ocd") == 0)
				icon_type = MAPE_FILE_ICON_C4OBJECT;
			else if(g_strcasecmp(fileext, ".ocf") == 0)
				icon_type = MAPE_FILE_ICON_C4FOLDER;
			else if(g_strcasecmp(fileext, ".ocg") == 0)
				icon_type = MAPE_FILE_ICON_C4GROUP;
			else if(g_strcasecmp(fileext, ".ocs") == 0)
				icon_type = MAPE_FILE_ICON_C4SCENARIO;
		}

		if(mape_group_is_drive_container(child_group))
			icon_type = MAPE_FILE_ICON_DRIVE;

		icon = mape_file_icon_set_lookup(
			disk_view->icon_set,
			icon_type
		);

		gtk_tree_store_set(
			GTK_TREE_STORE(disk_view->tree_store),
			&child_iter,
			MAPE_DISK_VIEW_COLUMN_ICON,
			mape_file_icon_get(icon),
			MAPE_DISK_VIEW_COLUMN_FILE,
			filename,
			MAPE_DISK_VIEW_COLUMN_GROUP,
			NULL,/*(gpointer)child_group,*/
			-1
		);

		gtk_tree_model_ref_node(
			disk_view->tree_store,
			&child_iter
		);

		g_free(filename);
	}
	
	/* TODO: Close group if no content */

	return TRUE;
}

static gboolean mape_disk_view_cb_key_press_event(GtkWidget* widget,
                                                  GdkEventKey* event,
                                                  gpointer user_data)
{
	MapeDiskView* disk_view;
	GtkTreePath* path;
	gboolean result;

	disk_view = (MapeDiskView*)user_data;

	if(event->keyval != GDK_KEY_Left && event->keyval != GDK_KEY_Right)
		return FALSE;

	gtk_tree_view_get_cursor(
		GTK_TREE_VIEW(disk_view->view),
		&path,
		NULL
	);

	if(path == NULL) return FALSE;

	switch(event->keyval)
	{
	case GDK_KEY_Left:
		result = gtk_tree_view_row_expanded(
			GTK_TREE_VIEW(disk_view->view),
			path
		);

		if(result == TRUE)
		{
			gtk_tree_view_collapse_row(
				GTK_TREE_VIEW(disk_view->view),
				path
			);
		}
		else
		{
			if(gtk_tree_path_get_depth(path) > 1)
			{
				result = gtk_tree_path_up(path);
				g_assert(result == TRUE);

				gtk_tree_view_set_cursor(
					GTK_TREE_VIEW(disk_view->view),
					path,
					NULL,
					FALSE
				);
			}
		}

		break;
	case GDK_KEY_Right:
		result = gtk_tree_view_row_expanded(
			GTK_TREE_VIEW(disk_view->view),
			path
		);

		if(result == TRUE)
		{
			gtk_tree_path_down(path);

			gtk_tree_view_set_cursor(
				GTK_TREE_VIEW(disk_view->view),
				path,
				NULL,
				FALSE
			);
		}
		else
		{
			gtk_tree_view_expand_row(
				GTK_TREE_VIEW(disk_view->view),
				path,
				FALSE
			);
		}

		break;
	default:
		g_assert_not_reached();
		break;
	}

	gtk_tree_path_free(path);
	return TRUE;
}

static gboolean mape_disk_view_cb_button_press_event(GtkWidget* widget,
                                                     GdkEventButton* event,
                                                     gpointer user_data)
{
	MapeDiskView* disk_view;
	GtkTreePath* path;
	GtkTreeIter iter;
	gboolean result;
	gchar* filename;
	GError* error = NULL;
	GtkWidget* error_dialog;

	disk_view = (MapeDiskView*)user_data;

	if(event->type == GDK_2BUTTON_PRESS)
	{
		gtk_tree_view_get_cursor(
			GTK_TREE_VIEW(disk_view->view),
			&path,
			NULL
		);

		if(path == NULL) return FALSE;
		
		gtk_tree_model_get_iter(
			disk_view->tree_store,
			&iter,
			path
		);

		gtk_tree_model_get(
			disk_view->tree_store,
			&iter,
			MAPE_DISK_VIEW_COLUMN_FILE,
			&filename,
			-1
		);

		/* Load Material.ocg */
		if(g_strcasecmp(filename, "Material.ocg") == 0)
		{
			result = mape_disk_view_load_materials(
				disk_view,
				&iter,
				&error
			);
			
			if(result == FALSE)
			{
				error_dialog = gtk_message_dialog_new(
					NULL,
					GTK_DIALOG_MODAL,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_OK,
					"%s",
					error->message
				);

				g_error_free(error);
				gtk_dialog_run(GTK_DIALOG(error_dialog) );
				gtk_widget_destroy(error_dialog);
			}
		}
		else
		{
			result = gtk_tree_view_row_expanded(
				GTK_TREE_VIEW(disk_view->view),
				path
			);

			if(result == TRUE)
			{
				gtk_tree_view_collapse_row(
					GTK_TREE_VIEW(disk_view->view),
					path
				);
			}
			else
			{
				gtk_tree_view_expand_row(
					GTK_TREE_VIEW(disk_view->view),
					path,
					FALSE
				);
			}
		}

		g_free(filename);
		gtk_tree_path_free(path);
		return TRUE;
	}

	return FALSE;
}

static void mape_disk_view_cb_row_expanded(GtkTreeView* treeview,
                                           GtkTreeIter* row,
                                           GtkTreePath* path,
                                           gpointer user_data)
{
	MapeDiskView* disk_view;
	MapeGroup* group;
	GtkTreeIter iter;
	GtkTreeIter temp_iter;
	gboolean result;
	GError* error = NULL;
	GtkWidget* error_dialog;

	disk_view = (MapeDiskView*)user_data;
	result = gtk_tree_model_get_iter(
		disk_view->tree_store,
		&iter,
		path
	);

	g_assert(result == TRUE);

	/* Remove temporary entry if group is not already loaded */
	gtk_tree_model_get(
		disk_view->tree_store,
		&iter,
		MAPE_DISK_VIEW_COLUMN_GROUP,
		&group,
		-1
	);

	if(group == NULL)
	{
		g_assert(
			gtk_tree_model_iter_n_children(
				disk_view->tree_store,
				&iter
			) == 1
		);

		result = gtk_tree_model_iter_children(
			disk_view->tree_store,
			&temp_iter,
			&iter
		);

		g_assert(result == TRUE);

		result = gtk_tree_store_remove(
			GTK_TREE_STORE(disk_view->tree_store),
			&temp_iter
		);

		g_assert(result == FALSE);
	}

	if(mape_disk_view_load(disk_view, &iter, &error) == FALSE)
	{
		error_dialog = gtk_message_dialog_new(
			NULL,
			GTK_DIALOG_MODAL,
			GTK_MESSAGE_ERROR,
			GTK_BUTTONS_OK,
			"%s",
			error->message
		);

		g_error_free(error);
		gtk_dialog_run(GTK_DIALOG(error_dialog) );
		gtk_widget_destroy(error_dialog);

		return;
	}

	if(group == NULL)
	{
		gtk_tree_view_expand_row(
			GTK_TREE_VIEW(disk_view->view),
			path,
			FALSE
		);
	}
}

static void mape_disk_view_close_groups(MapeDiskView* disk_view,
                                        GtkTreeIter* iter)
{
	MapeGroup* group;
	GtkTreeIter child;
	gboolean has_child, has_next;

	gtk_tree_model_get(
		disk_view->tree_store,
		iter,
		MAPE_DISK_VIEW_COLUMN_GROUP,
		&group,
		-1
	);

	has_child = gtk_tree_model_iter_children(
		disk_view->tree_store,
		&child,
		iter
	);

	has_next = gtk_tree_model_iter_next(
		disk_view->tree_store,
		iter
	);

	if(has_child == TRUE)
		mape_disk_view_close_groups(disk_view, &child);

	if(group != NULL)
		g_object_unref(group);

	if(has_next == TRUE)
		mape_disk_view_close_groups(disk_view, iter);
}


MapeDiskView* mape_disk_view_new(MapeFileIconSet* icon_set,
                                 MapeMatTexView* mat_tex,
                                 MapeEditView* edit_view,
                                 MapeConfigFile* config,
                                 GError** error)
{
	MapeDiskView* view;

	view = malloc(sizeof(MapeDiskView) );

	view->group_top = NULL;
	view->icon_set = icon_set;
	view->mat_tex = mat_tex;
	view->edit_view = edit_view;
	view->config = config;

	view->tree_store = GTK_TREE_MODEL(
		gtk_tree_store_new(
			MAPE_DISK_VIEW_COLUMN_COUNT,
			GDK_TYPE_PIXBUF,
			G_TYPE_STRING,
			G_TYPE_POINTER
		)
	);

	view->renderer_icon = gtk_cell_renderer_pixbuf_new();
	view->renderer_file = gtk_cell_renderer_text_new();

	/*view->col_icon = gtk_tree_view_column_new_with_attributes(
		"Icon",
		view->renderer_icon,
		"pixbuf", MAPE_DISK_VIEW_COLUMN_ICON,
		NULL
	);*/

	view->col_file = gtk_tree_view_column_new();

/*	view->col_file = gtk_tree_view_column_new_with_attributes(
		"Filename",
		view->renderer_file,
		"text", MAPE_DISK_VIEW_COLUMN_FILE,
		"pixbuf", MAPE_DISK_VIEW_COLUMN_ICON,
		NULL
	);*/

	gtk_tree_view_column_pack_start(
		view->col_file,
		view->renderer_icon,
		FALSE
	);

	gtk_tree_view_column_pack_start(
		view->col_file,
		view->renderer_file,
		TRUE
	);

	gtk_tree_view_column_set_attributes(
		view->col_file,
		view->renderer_icon,
		"pixbuf", MAPE_DISK_VIEW_COLUMN_ICON,
		NULL
	);

	gtk_tree_view_column_set_attributes(
		view->col_file,
		view->renderer_file,
		"text", MAPE_DISK_VIEW_COLUMN_FILE,
		NULL
	);

	gtk_tree_view_column_set_spacing(
		view->col_file,
		5
	);

	view->view = gtk_tree_view_new_with_model(
		GTK_TREE_MODEL(view->tree_store)
	);

	gtk_widget_add_events(
		view->view,
		GDK_BUTTON_PRESS_MASK |
		GDK_KEY_PRESS_MASK
	);

	gtk_tree_view_append_column(GTK_TREE_VIEW(view->view), view->col_file);

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view->view), FALSE);
	gtk_tree_sortable_set_sort_column_id(
		GTK_TREE_SORTABLE(view->tree_store),
		MAPE_DISK_VIEW_COLUMN_FILE,
		GTK_SORT_ASCENDING
	);

	g_signal_connect(
		G_OBJECT(view->view),
		"row-expanded",
		G_CALLBACK(mape_disk_view_cb_row_expanded),
		view
	);

	g_signal_connect(
		G_OBJECT(view->view),
		"button-press-event",
		G_CALLBACK(mape_disk_view_cb_button_press_event),
		view
	);

	g_signal_connect(
		G_OBJECT(view->view),
		"key-press-event",
		G_CALLBACK(mape_disk_view_cb_key_press_event),
		view
	);

	if(mape_disk_view_load(view, NULL, error) == FALSE)
	{
		/* TODO: Free cell renderers? */
		gtk_widget_destroy(view->view);
		g_object_unref(G_OBJECT(view->tree_store) );

		free(view);
		return NULL;
	}

	gtk_widget_show(view->view);

	view->window = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(
		GTK_CONTAINER(view->window),
		view->view
	);

	gtk_scrolled_window_set_policy(
		GTK_SCROLLED_WINDOW(view->window),
		GTK_POLICY_AUTOMATIC,
		GTK_POLICY_AUTOMATIC
	);

	gtk_scrolled_window_set_shadow_type(
		GTK_SCROLLED_WINDOW(view->window),
		GTK_SHADOW_IN
	);

	gtk_widget_set_size_request(view->window, 150, -1);
	gtk_widget_show(view->window);

	return view;
}

void mape_disk_view_destroy(MapeDiskView* disk_view)
{
	/* Close open groups */
	GtkTreeIter iter;
	if(gtk_tree_model_get_iter_first(disk_view->tree_store, &iter))
		mape_disk_view_close_groups(disk_view, &iter);

	/* TODO: unref cell renderers? */
	/*mape_file_icon_set_destroy(disk_view->icon_set);*/
	g_object_unref(disk_view->group_top);

	g_object_unref(G_OBJECT(disk_view->tree_store) );
	free(disk_view);
}

gboolean mape_disk_view_extend_to_path(MapeDiskView* disk_view,
                                       const gchar* filepath,
                                       GError** error)
{
	gchar** path_components;
	gchar** cur_component;
	gchar* file;
	GtkTreeIter parent;
	GtkTreeIter child;
	gboolean result;
	GtkTreePath* path;
	gboolean got_parent;

	path_components = g_strsplit_set(filepath, "/\\", 0);
	
	/* Begin at top-level: no parent */
	got_parent = FALSE;

	for(cur_component = path_components;
	    *cur_component != NULL;
	    ++ cur_component)
	{
		file = *cur_component;
		if(*file == '\0') continue;
		if(strcmp(file, ".") == 0) continue;

		if(strcmp(file, "..") == 0)
		{
			if(got_parent == FALSE)
			{
				g_set_error(
					error,
					g_quark_from_static_string(
						"MAPE_DISK_VIEW_ERROR"
					),
					MAPE_DISK_VIEW_ERROR_NOENT,
					"%s: Invalid path",
					filepath
				);

				g_strfreev(path_components);
				return FALSE;
			}
			else
			{
				child = parent;

				got_parent = gtk_tree_model_iter_parent(
					GTK_TREE_MODEL(disk_view->tree_store),
					&parent,
					&child
				);
			}
		}
		else
		{
			if(got_parent == FALSE)
			{
				result = mape_disk_view_find_iter(
					disk_view,
					&child,
					NULL,
					file
				);
			}
			else
			{
				result = mape_disk_view_find_iter(
					disk_view,
					&child,
					&parent,
					file
				);
			}
		
			/* File is not reachable in file system */
			if(result == FALSE)
			{
				g_set_error(
					error,
					g_quark_from_static_string(
						"MAPE_DISK_VIEW_ERROR"
					),
					MAPE_DISK_VIEW_ERROR_NOENT,
					"%s: No such file or directory",
					filepath
				);

				g_strfreev(path_components);
				return FALSE;
			}

			if(g_ascii_strcasecmp(file, "Material.ocg") == 0)
			{
				/* Assume end of path */
				result = mape_disk_view_load_materials(
					disk_view,
					&child,
					error
				);

				g_strfreev(path_components);
				return result;
			}
			else
			{
				/* Convert child to path to preserve
				   position while expanding */
				path = gtk_tree_model_get_path(
					disk_view->tree_store,
					&child
				);

				gtk_tree_view_expand_row(
					GTK_TREE_VIEW(disk_view->view),
					path,
					FALSE
				);
			
				/* Child is new parent for next iteration */
				gtk_tree_model_get_iter(
					disk_view->tree_store,
					&parent,
					path
				);
				got_parent = TRUE;

				gtk_tree_path_free(path);
			}
		}
	}

	/* All nodes expanded without opening Material.ocg */
	g_strfreev(path_components);
	return TRUE;
}
