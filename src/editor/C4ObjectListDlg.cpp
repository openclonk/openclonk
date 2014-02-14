/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2007-2009, RedWolf Design GmbH, http://www.clonk.de/
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

/* A window listing all objects in the game */

#include <C4Include.h>
#include <C4ObjectListDlg.h>
#include <C4Console.h>
#include <C4Object.h>
#include <C4Language.h>
#include <C4Game.h>
#include <C4GameObjects.h>


#ifdef WITH_DEVELOPER_MODE
#include <gtk/gtk.h>


/* Some boilerplate GObject defines. 'klass' is used instead of 'class', because 'class' is a C++ keyword */

#define C4_TYPE_LIST                  (c4_list_get_type ())
#define C4_LIST(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), C4_TYPE_LIST, C4List))
#define C4_LIST_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass),  C4_TYPE_LIST, C4ListClass))
#define C4_IS_LIST(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), C4_TYPE_LIST))
#define C4_IS_LIST_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass),  C4_TYPE_LIST))
#define C4_LIST_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj),  C4_TYPE_LIST, C4ListClass))

/* The data columns that we export via the tree model interface */

enum
{
	C4_LIST_COL_OBJECT,
	C4_LIST_N_COLUMNS
};

typedef struct _C4List       C4List;
typedef struct _C4ListClass  C4ListClass;

struct _C4List
{
	GObject parent; /* this MUST be the first member */

	C4ObjectList * data;

	gint stamp; /* A random integer to check if an iter belongs to this model  */
};



/* more boilerplate GObject stuff */

struct _C4ListClass
{
	GObjectClass parent_class;
};

static GObjectClass *parent_class = NULL;

GType c4_list_get_type (void);

// Initialise an instance
static void
c4_list_init (C4List *c4_list)
{
	c4_list->data = &::Objects;

	c4_list->stamp = g_random_int(); /* Random int to check whether iters belong to this model */
}

// destructor
static void
c4_list_finalize (GObject *object)
{
	/* must chain up - finalize parent */
	(* parent_class->finalize) (object);
}


static GtkTreeModelFlags
c4_list_get_flags (GtkTreeModel *tree_model)
{
	g_return_val_if_fail (C4_IS_LIST(tree_model), (GtkTreeModelFlags)0);

	// neither is this a flat list nor do the iters persist changes in the model
	//return GtkTreeModelFlags(GTK_TREE_MODEL_ITERS_PERSIST);
	return GtkTreeModelFlags(0);
}

// converts 'path' into an iterator and stores that in 'iter'
static gboolean
c4_list_get_iter (GtkTreeModel * tree_model, GtkTreeIter * iter, GtkTreePath * path)
{
	gint          *indices, depth;

	g_assert(C4_IS_LIST(tree_model));
	g_assert(path!=NULL);

	C4List * c4_list = C4_LIST(tree_model);

	indices = gtk_tree_path_get_indices(path);
	depth   = gtk_tree_path_get_depth(path);

	iter->stamp      = c4_list->stamp;


	C4ObjectLink * pLnk = C4_LIST(tree_model)->data->First;
	// Skip Contained Objects in the main list
	while (pLnk && pLnk->Obj->Contained) pLnk = pLnk->Next;
	for (int i = 0; i < depth; ++i)
	{
		if (!pLnk)
			return false;
		if (indices[i] < 0)
			return false;
		for (int j = 0; j < indices[i]; ++j)
		{
			pLnk = pLnk->Next;
			// Skip Contained Objects in the main list
			while (i == 0 && pLnk && pLnk->Obj->Contained) pLnk = pLnk->Next;
			if (!pLnk)
				return false;
		}
		iter->user_data  = pLnk;
		iter->user_data2 = pLnk->Obj->Contained;
		pLnk = pLnk->Obj->Contents.First;
	}

	return true;
}

// converts 'iter' into a new tree path and returns that.
// Note: This is called by OnObjectRemove with an iter which is not in the
//       list anymore, but with the object and prev still usable.
static GtkTreePath *
c4_list_get_path (GtkTreeModel * tree_model, GtkTreeIter * iter)
{
	g_return_val_if_fail (C4_IS_LIST(tree_model), NULL);
	g_return_val_if_fail (iter != NULL,               NULL);
	g_return_val_if_fail (iter->user_data != NULL,    NULL);

	GtkTreePath  *path = gtk_tree_path_new();

	C4List * c4_list = C4_LIST(tree_model);

	C4Object * pObj = ((C4ObjectLink *) iter->user_data)->Obj;

	int i = 0;
	for (C4ObjectLink * pLnk = ((C4ObjectLink *) iter->user_data)->Prev; pLnk; pLnk = pLnk->Prev)
	{
		// Skip Contained Objects in the main list
		if (pObj->Contained != pLnk->Obj->Contained) continue;
		++i;
	}
	gtk_tree_path_prepend_index(path, i);

	pObj = (C4Object *) iter->user_data2;
	while (pObj)
	{
		i = 0;
		C4ObjectList * pList = c4_list->data;
		if (pObj->Contained)
			pList = &pObj->Contained->Contents;
		for (C4ObjectLink * pLnk = pList->First; pLnk && pLnk->Obj != pObj; pLnk = pLnk->Next)
		{
			// Skip Contained Objects in the main list
			if (pObj->Contained != pLnk->Obj->Contained) continue;
			++i;
		}
		gtk_tree_path_prepend_index(path, i);
		pObj = pObj->Contained;
	}

	return path;
}

// ++iter
static gboolean
c4_list_iter_next (GtkTreeModel * tree_model, GtkTreeIter * iter)
{
	g_return_val_if_fail (C4_IS_LIST (tree_model), false);

	if (iter == NULL || iter->user_data == NULL)
		return false;

	C4ObjectLink * pLnk = (C4ObjectLink *) iter->user_data;

	pLnk = pLnk->Next;

	// Skip Contained Objects in the main list
	if (!(C4Object *)iter->user_data2)
		while (pLnk && pLnk->Obj->Contained)
			pLnk = pLnk->Next;
	if (!pLnk)
		return false;

	iter->user_data = pLnk;
	iter->user_data2 = pLnk->Obj->Contained;

	return true;
}

// Set 'iter' to the first child of 'parent', or the first top-level row if
// 'parent' is 0, or return false if there aren't any children.
static gboolean
c4_list_iter_children (GtkTreeModel * tree_model, GtkTreeIter * iter, GtkTreeIter * parent)
{
	g_return_val_if_fail (parent == NULL || parent->user_data != NULL, false);
	g_return_val_if_fail (C4_IS_LIST (tree_model), false);

	C4List  *c4_list = C4_LIST(tree_model);

	C4ObjectLink * pLnk;
	if (parent)
	{
		C4ObjectList * pList = &((C4ObjectLink *)parent->user_data)->Obj->Contents;
		pLnk = pList->First;
	}
	else
	{
		pLnk = c4_list->data->First;
		// Skip...
		while (pLnk && pLnk->Obj->Contained) pLnk = pLnk->Next;
	}
	if (!pLnk)
		return false;

	/* Set iter to first item in list */
	iter->stamp     = c4_list->stamp;
	iter->user_data = pLnk;
	iter->user_data2 = pLnk->Obj->Contained;

	return true;
}

// Return true if 'parent' has children.
static gboolean
c4_list_iter_has_child (GtkTreeModel *tree_model,
                        GtkTreeIter  *parent)
{
	g_return_val_if_fail (parent == NULL || parent->user_data != NULL, false);
	g_return_val_if_fail (C4_IS_LIST (tree_model), false);

	C4List  *c4_list = C4_LIST(tree_model);

	C4ObjectList * pList = c4_list->data;
	if (parent)
		pList = &((C4ObjectLink *)parent->user_data)->Obj->Contents;
	return pList->First != 0;
}

// Counts the children 'parent' has.
static gint
c4_list_iter_n_children (GtkTreeModel *tree_model, GtkTreeIter  *parent)
{
	g_return_val_if_fail (C4_IS_LIST (tree_model), -1);
	g_return_val_if_fail (parent == NULL || parent->user_data != NULL, -1);

	C4List  *c4_list = C4_LIST(tree_model);

	int i = 0;
	if (parent)
	{
		C4ObjectList * pList = &((C4ObjectLink *)parent->user_data)->Obj->Contents;
		C4ObjectLink * pLnk = pList->First;
		while (pLnk)
		{
			++i;
			pLnk = pLnk->Next;
		}
	}
	else
	{
		C4ObjectLink * pLnk = c4_list->data->First;
		while (pLnk)
		{
			if (!pLnk->Obj->Contained)
				++i;
			pLnk = pLnk->Next;
		}
	}
	return i;
}

// Sets 'iter' to the 'n'-th child of 'parent'.
static gboolean
c4_list_iter_nth_child (GtkTreeModel * tree_model, GtkTreeIter * iter, GtkTreeIter * parent, gint n)
{
	g_return_val_if_fail (C4_IS_LIST (tree_model), false);
	g_return_val_if_fail (parent == NULL || parent->user_data != NULL, false);

	C4List  *c4_list = C4_LIST(tree_model);

	C4ObjectLink * pLnk;
	if (parent)
	{
		C4ObjectList * pList = &((C4ObjectLink *)parent->user_data)->Obj->Contents;
		pLnk = pList->First;
		for (int i = 0; i < n; ++i)
		{
			if (!pLnk)
				return false;
			pLnk = pLnk->Next;
		}
	}
	else
	{
		pLnk = c4_list->data->First;
		for (int i = 0; i < n; ++i)
		{
			if (!pLnk)
				return false;
			pLnk = pLnk->Next;
			// Skip...
			while (pLnk && pLnk->Obj->Contained) pLnk = pLnk->Next;
		}
	}
	if (!pLnk)
		return false;

	iter->stamp = c4_list->stamp;
	iter->user_data = pLnk;
	iter->user_data2 = pLnk->Obj->Contained;

	return true;
}

// Helper function.
static gboolean c4_list_iter_for_C4Object (GtkTreeModel * tree_model, GtkTreeIter * iter, C4ObjectList * pList, C4Object * pObj)
{
	if (!pObj)
		return false;

	C4List * c4_list = C4_LIST(tree_model);

	for (C4ObjectLink * pLnk = pList->First; pLnk; pLnk = pLnk->Next)
	{
		if (pLnk->Obj == pObj)
		{
			iter->stamp = c4_list->stamp;
			iter->user_data = pLnk;
			iter->user_data2 = pLnk->Obj->Contained;

			return true;
		}
	}

	g_return_val_if_reached(false);
}

// Sets 'iter' to the parent row of 'child'.
static gboolean
c4_list_iter_parent (GtkTreeModel * tree_model, GtkTreeIter * iter, GtkTreeIter * child)
{
	g_return_val_if_fail (C4_IS_LIST (tree_model), false);
	g_return_val_if_fail (child == NULL || child->user_data != NULL, false);

	C4List * c4_list = C4_LIST(tree_model);

	C4Object * pObj = (C4Object *) child->user_data2;

	C4ObjectList * pList = c4_list->data;
	if (pObj->Contained)
		pList = &pObj->Contained->Contents;
	return c4_list_iter_for_C4Object(tree_model, iter, pList, pObj);
}

static C4Object *
c4_list_iter_get_C4Object(GtkTreeModel * tree_model, GtkTreeIter * iter)
{
	g_return_val_if_fail (C4_IS_LIST (tree_model), NULL);
	g_return_val_if_fail (iter != NULL && iter->user_data != NULL, NULL);

	return ((C4ObjectLink *) iter->user_data)->Obj;
}

// How many columns does this model have?
static gint
c4_list_get_n_columns (GtkTreeModel * tree_model)
{
	g_return_val_if_fail (C4_IS_LIST(tree_model), 0);

	return 1;
}

// What sort of data is in the column?
static GType
c4_list_get_column_type (GtkTreeModel * tree_model, gint index)
{
	g_return_val_if_fail (C4_IS_LIST(tree_model), G_TYPE_INVALID);
	g_return_val_if_fail (index < 1 && index >= 0, G_TYPE_INVALID);

	return G_TYPE_POINTER;
}

// gets the data for the 'column' in the row at 'iter' and stores that in 'value'.
static void
c4_list_get_value (GtkTreeModel * tree_model, GtkTreeIter * iter, gint column, GValue * value)
{
	g_return_if_fail (C4_IS_LIST (tree_model));
	g_return_if_fail (iter != NULL);
	g_return_if_fail (column == 0);

	C4Object * pObj = ((C4ObjectLink *) iter->user_data)->Obj;
	g_return_if_fail (pObj != NULL);

	g_value_init (value, G_TYPE_POINTER);
	g_value_set_pointer(value, pObj);

//  g_value_set_string(value, pObj->GetName());
}

// Wrapper around g_object_new.
static C4List *
c4_list_new (void)
{
	C4List * list;

	list = (C4List *) g_object_new (C4_TYPE_LIST, NULL);

	g_assert(list != NULL);

	return list;
}

// Called once for the class.
static void
c4_list_class_init (C4ListClass * klass)
{
	GObjectClass * object_class;

	parent_class = (GObjectClass*) g_type_class_peek_parent (klass);
	object_class = (GObjectClass*) klass;

	object_class->finalize = c4_list_finalize;
}

// fill in the GtkTreeModel interface with the functions above.
static void
c4_list_tree_model_init (GtkTreeModelIface *iface)
{
	iface->get_flags       = c4_list_get_flags;
	iface->get_n_columns   = c4_list_get_n_columns;
	iface->get_column_type = c4_list_get_column_type;
	iface->get_iter        = c4_list_get_iter;
	iface->get_path        = c4_list_get_path;
	iface->get_value       = c4_list_get_value;
	iface->iter_next       = c4_list_iter_next;
	iface->iter_children   = c4_list_iter_children;
	iface->iter_has_child  = c4_list_iter_has_child;
	iface->iter_n_children = c4_list_iter_n_children;
	iface->iter_nth_child  = c4_list_iter_nth_child;
	iface->iter_parent     = c4_list_iter_parent;
}

// Return the type, registering it on first call.
GType
c4_list_get_type (void)
{
	static GType c4_list_type = 0;

	if (c4_list_type == 0)
	{
		// Some boilerplate type registration stuff
		static const GTypeInfo c4_list_info =
		{
			sizeof (C4ListClass),                /* class_size */
			NULL,                                /* base_init */
			NULL,                                /* base_finalize */
			(GClassInitFunc) c4_list_class_init, /* class_init */
			NULL,                                /* class finalize */
			NULL,                                /* class_data */
			sizeof (C4List),                     /* instance_size */
			0,                                   /* n_preallocs */
			(GInstanceInitFunc) c4_list_init,    /* instance_init */
			NULL                                 /* value_table */
		};

		c4_list_type = g_type_register_static (G_TYPE_OBJECT, "C4List",
		                                       &c4_list_info, (GTypeFlags)0);

		/* register the GtkTreeModel interface with the type system */
		static const GInterfaceInfo tree_model_info =
		{
			(GInterfaceInitFunc) c4_list_tree_model_init,
			NULL,
			NULL
		};

		g_type_add_interface_static (c4_list_type, GTK_TYPE_TREE_MODEL, &tree_model_info);
	}

	return c4_list_type;
}

void C4ObjectListDlg::OnObjectRemove(C4ObjectList * pList, C4ObjectLink * pLnk)
{
	if (!model) return;

	C4List * c4_list = C4_LIST(model);
	C4Object * Contained = pLnk->Obj->Contained;
	GtkTreeIter iter;
	iter.stamp = c4_list->stamp;
	iter.user_data = pLnk;
	iter.user_data2 = Contained;

	// While pLnk is not in the list anymore, with pLnk->Prev and Contained a path can still be made
	GtkTreePath * path = c4_list_get_path(GTK_TREE_MODEL(model), &iter);

	gtk_tree_model_row_deleted(GTK_TREE_MODEL(model), path);
	gtk_tree_path_free(path);

	// Removed from a now empty container?
	if (Contained && !Contained->Contents.First)
	{
		printf("Removed from a now empty container\n");
		GtkTreeIter parent;
		C4ObjectList * pList = c4_list->data;
		if (Contained->Contained)
			pList = &Contained->Contained->Contents;
		c4_list_iter_for_C4Object(GTK_TREE_MODEL(model), &parent, pList, Contained);

		GtkTreePath * path = c4_list_get_path(GTK_TREE_MODEL(model), &parent);

		gtk_tree_model_row_has_child_toggled(GTK_TREE_MODEL(model), path, &parent);
		gtk_tree_path_free(path);
	}


	// Cheat: For the signals it must look as if the object had it's parent removed already
	pLnk->Obj->Contained = 0;
	// if removed from contents, it get's added to main list
	if (pList != c4_list->data)
	{
		printf("Removed from a container\n");
		GtkTreeIter iter;
		C4ObjectList * pList = c4_list->data;
		c4_list_iter_for_C4Object(GTK_TREE_MODEL(model), &iter, pList, pLnk->Obj);

		GtkTreePath * path = c4_list_get_path(GTK_TREE_MODEL(model), &iter);

		gtk_tree_model_row_inserted(GTK_TREE_MODEL(model), path, &iter);
		gtk_tree_path_free(path);
	}

	// End-of-cheat
	pLnk->Obj->Contained = Contained;
}

void C4ObjectListDlg::OnObjectAdded(C4ObjectList * pList, C4ObjectLink * pLnk)
{
	if (!model) return;

	C4List * c4_list = C4_LIST(model);

	// Inserted into a container? Remove from main list
	if (pList != c4_list->data)
	{
		printf("Inserted into a container\n");
		GtkTreePath  *path = gtk_tree_path_new();
		int i = 0;
		C4ObjectList * pList = c4_list->data;
		C4Object * pObj = pLnk->Obj;
		for (C4ObjectLink * pLnk2 = pList->First; pLnk2 && pLnk2->Obj != pObj; pLnk2 = pLnk2->Next)
		{
			// Skip Contained Objects in the main list
			if (pLnk2->Obj->Contained) continue;
			++i;
		}
		gtk_tree_path_prepend_index(path, i);

		gtk_tree_model_row_deleted(GTK_TREE_MODEL(model), path);
		gtk_tree_path_free(path);
	}

	GtkTreeIter iter;
	iter.stamp = c4_list->stamp;
	iter.user_data = pLnk;
	iter.user_data2 = pLnk->Obj->Contained;

	GtkTreePath * path = c4_list_get_path(GTK_TREE_MODEL(model), &iter);

	gtk_tree_model_row_inserted(GTK_TREE_MODEL(model), path, &iter);
	gtk_tree_path_free(path);

	// Inserted into a previously empty container?
	if (pLnk->Obj->Contained &&
	    pLnk->Obj->Contained->Contents.First == pLnk->Obj->Contained->Contents.Last)
	{
		printf("Inserted into a previously empty container\n");
		GtkTreeIter parent;
		c4_list_iter_parent(GTK_TREE_MODEL(model), &parent, &iter);

		GtkTreePath * path = c4_list_get_path(GTK_TREE_MODEL(model), &parent);

		gtk_tree_model_row_has_child_toggled(GTK_TREE_MODEL(model), path, &parent);
		gtk_tree_path_free(path);
	}
}

void C4ObjectListDlg::OnObjectRename(C4ObjectList * pList, C4ObjectLink * pLnk)
{
}

void C4ObjectListDlg::OnDestroy(GtkWidget* widget, C4ObjectListDlg* dlg)
{
	dlg->window = 0;
	dlg->model = 0;
	dlg->treeview = 0;
}

void C4ObjectListDlg::OnRowActivated(GtkTreeView * tree_view, GtkTreePath * path, GtkTreeViewColumn * column, C4ObjectListDlg * dlg)
{
	Console.EditCursor.SetMode(C4CNS_ModeEdit);
	Console.EditCursor.OpenPropTools();
}

void C4ObjectListDlg::OnSelectionChanged(GtkTreeSelection* selection, C4ObjectListDlg* dlg)
{
	if (dlg->updating_selection) return;
	dlg->updating_selection = true;

	GList* list = gtk_tree_selection_get_selected_rows(selection, NULL);

	Console.EditCursor.GetSelection().Clear();
	for (GList * i = list; i; i = i->next)
	{
		GtkTreePath * path = (GtkTreePath *)i->data;
		GtkTreeIter iter;
		c4_list_get_iter(GTK_TREE_MODEL(dlg->model), &iter, path);
		Console.EditCursor.GetSelection().Add(((C4ObjectLink *)iter.user_data)->Obj, C4ObjectList::stNone);
	}

	g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free (list);

	Console.EditCursor.OnSelectionChanged();
	dlg->updating_selection = false;
}

void C4ObjectListDlg::Update(C4ObjectList &rSelection)
{
	if (updating_selection) return;
	if (!window) return;
	updating_selection = true;

	GtkTreeSelection *selection;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

	gtk_tree_selection_unselect_all(selection);

	for (C4ObjectLink * pLnk = rSelection.First; pLnk; pLnk = pLnk->Next)
	{
		GtkTreeIter iter;
		C4List * c4_list = C4_LIST(model);
		C4ObjectList * pList = c4_list->data;
		if (pLnk->Obj->Contained)
			pList = &pLnk->Obj->Contained->Contents;
		c4_list_iter_for_C4Object(GTK_TREE_MODEL(model), &iter, pList, pLnk->Obj);
		gtk_tree_selection_select_iter(selection, &iter);
	}

	updating_selection = false;
}

C4ObjectListDlg::C4ObjectListDlg():
		window(0),
		treeview(0),
		model(0),
		updating_selection(false)
{
}

C4ObjectListDlg::~C4ObjectListDlg()
{
}

void C4ObjectListDlg::Execute()
{
}

static void name_cell_data_func(GtkTreeViewColumn* column, GtkCellRenderer* renderer, GtkTreeModel* model, GtkTreeIter* iter, gpointer data)
{
	C4Object* object = c4_list_iter_get_C4Object(model, iter);

	g_object_set(G_OBJECT(renderer), "text", object->GetName(), (gpointer)NULL);
}

enum { ICON_SIZE = 24 };

static void icon_cell_data_func(GtkTreeViewColumn* column, GtkCellRenderer* renderer, GtkTreeModel* model, GtkTreeIter* iter, gpointer data)
{
	C4Object* object = c4_list_iter_get_C4Object(model, iter);

	// Icons for objects with ColorByOwner are cached by object, others by Def
	// FIXME: Invalidate cache when objects change color, and redraw.
	gpointer key = object->Def;
	if (object->Def->ColorByOwner) key = object;

	GHashTable* table = static_cast<GHashTable*>(data);
	GdkPixbuf* pixbuf = GDK_PIXBUF(g_hash_table_lookup(table, key));

	if (pixbuf == NULL)
	{
		/* Not yet cached, create from Graphics */
		C4Surface* surface = object->Def->Graphics.Bmp.Bitmap;
		if (object->Def->Graphics.Bmp.BitmapClr) surface = object->Def->Graphics.Bmp.BitmapClr;

		const C4Rect& picture = object->Def->PictureRect;
		pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, true, 8, picture.Wdt, picture.Hgt);
		guchar* pixels = gdk_pixbuf_get_pixels(pixbuf);
		surface->Lock();
		for (int y = 0; y < picture.Hgt; ++ y) for (int x = 0; x < picture.Wdt; ++ x)
			{
				DWORD dw = surface->GetPixDw(picture.x + x, picture.y + y, true);
				*pixels = (dw >> 16) & 0xff; ++ pixels;
				*pixels = (dw >> 8 ) & 0xff; ++ pixels;
				*pixels = (dw      ) & 0xff; ++ pixels;
				*pixels = 0xff - ((dw >> 24) & 0xff); ++ pixels;
			}
		surface->Unlock();

		// Scale down to ICON_SIZE, keeping aspect ratio
		guint dest_width, dest_height;
		if (picture.Wdt >= picture.Hgt)
		{
			double factor = static_cast<double>(picture.Hgt) / static_cast<double>(picture.Wdt);
			dest_width = ICON_SIZE;
			dest_height = dest_width * factor;
		}
		else
		{
			double factor = static_cast<double>(picture.Wdt) / static_cast<double>(picture.Hgt);
			dest_height = ICON_SIZE;
			dest_width = dest_height * factor;
		}

		GdkPixbuf* scaled = gdk_pixbuf_scale_simple(pixbuf, dest_width, dest_height, GDK_INTERP_HYPER);
		g_object_unref(G_OBJECT(pixbuf));
		pixbuf = scaled;

		g_hash_table_insert(table, key, pixbuf);
	}

	g_object_set(G_OBJECT(renderer), "pixbuf", pixbuf, NULL);
}

void C4ObjectListDlg::Open()
{
	// Create Window if necessary
	if (window == NULL)
	{
		// The Windows
		window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

		gtk_window_set_resizable(GTK_WINDOW(window), true);
		gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_UTILITY);
		gtk_window_set_role(GTK_WINDOW(window), "objectlist");
		gtk_window_set_title(GTK_WINDOW(window), "Objects");
		gtk_window_set_default_size(GTK_WINDOW(window), 180, 300);

		gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(Console.window));

		g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(OnDestroy), this);

		// The VBox and Tree
		GtkWidget* vbox = gtk_vbox_new(false, 8);

		GtkWidget* scrolled_wnd = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_wnd), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_wnd), GTK_SHADOW_IN);

		model = G_OBJECT(c4_list_new());

		treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));

		g_object_unref(model); /* destroy store automatically with view */

		g_signal_connect(G_OBJECT(treeview), "row-activated", G_CALLBACK(OnRowActivated), this);

		GtkTreeViewColumn * col = gtk_tree_view_column_new();
		GtkCellRenderer * renderer;
#if 0
		renderer = gtk_cell_renderer_pixbuf_new();
		gtk_tree_view_column_pack_start(col, renderer, false);
		gtk_tree_view_column_set_cell_data_func(col, renderer, icon_cell_data_func, g_hash_table_new_full(NULL, NULL, NULL, (GDestroyNotify)g_object_unref), (GDestroyNotify)g_hash_table_unref);
#endif
		renderer = gtk_cell_renderer_text_new();
		gtk_tree_view_column_pack_start(col, renderer, true);
		gtk_tree_view_column_set_cell_data_func(col, renderer, name_cell_data_func, NULL, NULL);

		gtk_tree_view_column_set_title(col, "Name");
		gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
		gtk_tree_view_append_column(GTK_TREE_VIEW(treeview),col);

		gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), false);
		gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(treeview), true);

		GtkTreeSelection * selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);

		g_signal_connect(G_OBJECT(selection), "changed", G_CALLBACK(OnSelectionChanged), this);

		gtk_container_add(GTK_CONTAINER(scrolled_wnd), treeview);
		gtk_box_pack_start(GTK_BOX(vbox), scrolled_wnd, true, true, 0);

		gtk_container_add(GTK_CONTAINER(window), vbox);

		gtk_widget_show_all(window);
	}
	else
	{
		gtk_window_present_with_time(GTK_WINDOW(window), gtk_get_current_event_time());
	}
}

#else

C4ObjectListDlg::C4ObjectListDlg()
{
}

C4ObjectListDlg::~C4ObjectListDlg()
{
}

void C4ObjectListDlg::Execute()
{
}

void C4ObjectListDlg::Open()
{
}

void C4ObjectListDlg::Update(C4ObjectList &rSelection)
{
}

void C4ObjectListDlg::OnObjectRemove(C4ObjectList * pList, C4ObjectLink * pLnk)
{
}

void C4ObjectListDlg::OnObjectAdded(C4ObjectList * pList, C4ObjectLink * pLnk)
{
}

void C4ObjectListDlg::OnObjectRename(C4ObjectList * pList, C4ObjectLink * pLnk)
{
}

#endif

C4ObjectListChangeListener & ObjectListChangeListener = Console.ObjectListDlg;
