/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2007  Armin Burgmeier
 * Copyright (c) 2007-2009, RedWolf Design GmbH, http://www.clonk.de
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

/* Common window for drawing and property tool dialogs in console mode */

#include <C4Include.h>
#include <C4DevmodeDlg.h>

#ifdef WITH_DEVELOPER_MODE

#include <gtk/gtk.h>

GtkWidget* C4DevmodeDlg::window = NULL;
GtkWidget* C4DevmodeDlg::notebook = NULL;

int C4DevmodeDlg::x = -1;
int C4DevmodeDlg::y = -1;

namespace
{
	gboolean OnDeleteEvent(GtkWidget* widget, gpointer user_data)
	{
		// Just hide the window, don't destroy it
		C4DevmodeDlg::SwitchPage(NULL);
		return true;
	}
}

void C4DevmodeDlg::OnDestroy(GtkWidget* window, gpointer user_data)
{
	C4DevmodeDlg::window = NULL;
	C4DevmodeDlg::notebook = NULL;
}

void C4DevmodeDlg::AddPage(GtkWidget* widget, GtkWindow* parent, const char* title)
{
	// Create Window if necessary
	if (window == NULL)
	{
		notebook = gtk_notebook_new();
		gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), false);
		gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), false);
		gtk_widget_show(GTK_WIDGET(notebook));

		window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		gtk_container_add(GTK_CONTAINER(window), notebook);

		gtk_window_set_resizable(GTK_WINDOW(window), true);
		gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_UTILITY);
		gtk_window_set_role(GTK_WINDOW(window), "toolbox");

		gtk_window_set_transient_for(GTK_WINDOW(window), parent);
		gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER_ON_PARENT);

		g_signal_connect(G_OBJECT(window), "delete-event", G_CALLBACK(OnDeleteEvent), NULL);
		g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(OnDestroy), NULL);
	}

	// Add page to notebook
	GtkWidget* label = gtk_label_new(title);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), widget, label);
}

void C4DevmodeDlg::RemovePage(GtkWidget* widget)
{
	int page_num = gtk_notebook_page_num(GTK_NOTEBOOK(notebook), widget);
	assert(page_num != -1); // Make sure it is contained

	gtk_notebook_remove_page(GTK_NOTEBOOK(notebook), page_num);

	if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) == 0)
		gtk_widget_destroy(window);
}

void C4DevmodeDlg::SwitchPage(GtkWidget* widget)
{
#if GTK_CHECK_VERSION(2,18,0)
	bool is_visible = gtk_widget_get_visible(GTK_WIDGET(window));
#else
	bool is_visible = GTK_WIDGET_VISIBLE(GTK_WIDGET(window));
#endif

	// Remember window position
	if (window != NULL && is_visible)
		gtk_window_get_position(GTK_WINDOW(window), &x, &y);

	if (widget != NULL)
	{
		assert(window != NULL);

		// Show required page
		int page_num = gtk_notebook_page_num(GTK_NOTEBOOK(notebook), widget);
		assert(page_num != -1); // Make sure it is contained

		gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), page_num);
		gtk_window_set_title(GTK_WINDOW(window), gtk_notebook_get_tab_label_text(GTK_NOTEBOOK(notebook), widget));

		// Show window if not visible
		if (!is_visible)
		{
			gtk_widget_show(window);
			if (x != -1 || y != -1)
				gtk_window_move(GTK_WINDOW(window), x, y);
		}
	}
	else
	{
		if (window != NULL && is_visible)
			gtk_widget_hide(window);
	}
}

void C4DevmodeDlg::SetTitle(GtkWidget* widget, const char* title)
{
	gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(notebook), widget, title);
	int page_num = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
	if (gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page_num) == widget)
		gtk_window_set_title(GTK_WINDOW(window), title);
}

#endif // WITH_DEVELOPER_MODE
