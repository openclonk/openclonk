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

/**
 * SECTION:mape-group
 * @title: MapeGroup
 * @short_description: C4Group interface
 * @include: mape/group.h
 * @stability: Unstable
 *
 * #MapeGroup is a simple GObject-based interface to C4Group. It currntly
 * only supports a subset of the C4Group operations, it does not support
 * writing groups for example. It is just enough for what Mape requires.
 **/

#include <string.h> /* for strrchr */

#include "mape/cpp-handles/group-handle.h"
#include "mape/group.h"

#ifdef G_OS_WIN32
#include <windows.h>
#endif

/* Declare private API */
C4GroupHandle*
_mape_group_get_handle(MapeGroup* group); /* shut up gcc */

typedef struct _MapeGroupPrivate MapeGroupPrivate;
struct _MapeGroupPrivate {
  C4GroupHandle* handle;

  /* On Windows, the root directory "/" is interpreted as a directory
   * containing the local drives (C:\, D:\, etc.). handle is set to NULL for
   * a group representing the "/" directory. */
#ifdef G_OS_WIN32
  /* This is 0 if the group is closed, otherwise the currently accessed drive
   * index (as returned by GetLogicalDrives()) minus one. */
  guint drive;
#endif
};

enum {
  PROP_0,

  /* read only */
  PROP_NAME,
  PROP_FULL_NAME
};

#define MAPE_GROUP_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), MAPE_TYPE_GROUP, MapeGroupPrivate))

static GQuark mape_group_error_quark;

G_DEFINE_TYPE(MapeGroup, mape_group, G_TYPE_OBJECT)

/*
 * GObject overrides.
 */

static void
mape_group_init(MapeGroup* group)
{
  MapeGroupPrivate* priv;
  priv = MAPE_GROUP_PRIVATE(group);

  priv->handle = NULL;

#ifdef G_OS_WIN32
  priv->drive = 0;
#endif
}

static void
mape_group_finalize(GObject* object)
{
  MapeGroup* group;
  MapeGroupPrivate* priv;

  group = MAPE_GROUP(object);
  priv = MAPE_GROUP_PRIVATE(group);

  if(priv->handle != NULL)
    c4_group_handle_free(priv->handle);

  G_OBJECT_CLASS(mape_group_parent_class)->finalize(object);
}

static void
mape_group_set_property(GObject* object,
                        guint prop_id,
                        const GValue* value,
                        GParamSpec* pspec)
{
  MapeGroup* group;
  MapeGroupPrivate* priv;

  group = MAPE_GROUP(object);
  priv = MAPE_GROUP_PRIVATE(group);

  switch(prop_id)
  {
    /* we have only readonly properties */
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(value, prop_id, pspec);
    break;
  }
}

static void
mape_group_get_property(GObject* object,
                        guint prop_id,
                        GValue* value,
                        GParamSpec* pspec)
{
  MapeGroup* group;
  MapeGroupPrivate* priv;

  group = MAPE_GROUP(object);
  priv = MAPE_GROUP_PRIVATE(group);

  switch(prop_id)
  {
  case PROP_NAME:
    g_value_set_string(value, mape_group_get_name(group));
    break;
  case PROP_FULL_NAME:
    g_value_take_string(value, mape_group_get_full_name(group));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

/*
 * Gype registration.
 */

static void
mape_group_class_init(MapeGroupClass *class)
{
  GObjectClass* object_class;

  object_class = G_OBJECT_CLASS(class);
  mape_group_parent_class = G_OBJECT_CLASS(g_type_class_peek_parent(class));
  g_type_class_add_private(class, sizeof(MapeGroupPrivate));

  object_class->finalize = mape_group_finalize;
  object_class->set_property = mape_group_set_property;
  object_class->get_property = mape_group_get_property;

  mape_group_error_quark = g_quark_from_static_string("MAPE_GROUP_ERROR");

  g_object_class_install_property(
    object_class,
    PROP_NAME,
    g_param_spec_string(
      "name",
      "Name",
      "The name of the group",
      NULL,
      G_PARAM_READABLE
    )
  );

  g_object_class_install_property(
    object_class,
    PROP_FULL_NAME,
    g_param_spec_string(
      "full-name",
      "Full Name",
      "The full path to the group",
      NULL,
      G_PARAM_READABLE
    )
  );
}

/*
 * Public API.
 */

/**
 * mape_group_new:
 *
 * Creates a new group object. The group object is initially closed. Open it
 * with mape_group_open(). To open a child group from an existing group, use
 * mape_group_open_child() instead of this function.
 *
 * Return Value: A new #MapeGroup. Free with g_object_unref().
 **/
MapeGroup*
mape_group_new(void)
{
  return MAPE_GROUP(g_object_new(MAPE_TYPE_GROUP, NULL));
}

/**
 * mape_group_is_open:
 * @group: A #MapeGroup.
 *
 * Returns whether @group is open or not.
 *
 * Returns: Whether @group is open.
 */
gboolean
mape_group_is_open(MapeGroup* group)
{
  MapeGroupPrivate* priv;

  g_return_val_if_fail(MAPE_IS_GROUP(group), FALSE);

  priv = MAPE_GROUP_PRIVATE(group);

  if(priv->handle != NULL) return TRUE;
#ifdef G_OS_WIN32
  if(priv->drive != 0) return TRUE;
#endif
  return FALSE;
}

/**
 * mape_group_open:
 * @group: A #MapeGroup.
 * @path: A path leading to a non-packed C4Group file or a directory on disk.
 * @error: Location to store error information, if any.
 *
 * @group must not be open yet. Upon success, @group has been opened and its
 * content can be read via mape_group_get_next_entry() and
 * mape_group_load_entry(). On error, the function returns %FALSE and @error
 * will be set.
 *
 * Return Value: %TRUE on success, and %FALSE on error.
 **/
gboolean
mape_group_open(MapeGroup* group,
                const gchar* path,
                GError** error)
{
  MapeGroupPrivate* priv;

  g_return_val_if_fail(MAPE_IS_GROUP(group), FALSE);
  g_return_val_if_fail(path != NULL, FALSE);
  g_return_val_if_fail(error == NULL || *error == NULL, FALSE);
  g_return_val_if_fail(mape_group_is_open(group) == FALSE, FALSE);

  priv = MAPE_GROUP_PRIVATE(group);

#ifdef G_OS_WIN32
  if(strcmp(path, "/") == 0)
  {
    priv->drive = 1;
    return TRUE;
  }
#endif

  priv->handle = c4_group_handle_new();
  if(c4_group_handle_open(priv->handle, path, FALSE) == FALSE)
  {
    g_set_error(error, mape_group_error_quark, MAPE_GROUP_ERROR_OPEN,
                "Could not open '%s': %s", path,
                c4_group_handle_get_error(priv->handle));
    c4_group_handle_free(priv->handle);
    priv->handle = NULL;
    return FALSE;
  }

  g_object_notify(G_OBJECT(group), "name");
  g_object_notify(G_OBJECT(group), "full-name");

  return TRUE;
}

/**
 * mape_group_open_child:
 * @group: A #MapeGroup.
 * @entry: A subgroup entry in @group.
 * @error: Location to store error information, if any.
 *
 * This function attempts to open a child group which is contained in @group,
 * named @entry. On success the new, opened group is returned. On failure,
 * the function returns %NULL and @error is set.
 *
 * Returns: A new group to be freed with g_object_unref() when no longer
 * needed, or %NULL on error.
 **/
MapeGroup*
mape_group_open_child(MapeGroup* group,
                      const gchar* entry,
                      GError** error)
{
  MapeGroupPrivate* parent_priv;
  MapeGroup* child;
  MapeGroupPrivate* child_priv;
  C4GroupHandle* child_handle;

  g_return_val_if_fail(MAPE_IS_GROUP(group), NULL);
  g_return_val_if_fail(entry != NULL, NULL);
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  g_return_val_if_fail(mape_group_is_open(group) == TRUE, NULL);

  parent_priv = MAPE_GROUP_PRIVATE(group);
#ifdef G_OS_WIN32
  if(parent_priv->handle == NULL)
  {
    child = mape_group_new();
    if(!mape_group_open(child, entry, error))
    {
      g_object_unref(child);
      return NULL;
    }

    return child;
  }
#endif

  child_handle = c4_group_handle_new();
  if(!c4_group_handle_open_as_child(child_handle, parent_priv->handle,
                                    entry, FALSE, FALSE))
  {
    g_set_error(error, mape_group_error_quark, MAPE_GROUP_ERROR_OPEN,
                "Could not open '%s': %s", entry,
                c4_group_handle_get_error(child_handle));
    c4_group_handle_free(child_handle);
    return NULL;
  }

  child = mape_group_new();
  child_priv = MAPE_GROUP_PRIVATE(child);
  child_priv->handle = child_handle;

  return child;
}

/**
 * mape_group_close:
 * @group: A #MapeGroup.
 *
 * Closes an open group. The group can be reopened afterwards.
 */
void
mape_group_close(MapeGroup* group)
{
  MapeGroupPrivate* priv;

  g_return_if_fail(MAPE_IS_GROUP(group));
  g_return_if_fail(mape_group_is_open(group) == FALSE);

  priv = MAPE_GROUP_PRIVATE(group);

#ifdef G_OS_WIN32
  priv->drive = 0;
#endif

  if(priv->handle != NULL)
  {
    c4_group_handle_free(priv->handle);
    priv->handle = NULL;

    /* Don't notify when only drive was reset, as name and full-name are
     * not specified for these anyway (yet). */
    g_object_notify(G_OBJECT(group), "name");
    g_object_notify(G_OBJECT(group), "full-name");
  }
}

/**
 * mape_group_get_name:
 * @group: An open #MapeGroup.
 *
 * Returns the name of the group.
 *
 * Return Value: The group's name. The name is owned by the group and must not
 * be freed.
 **/
const gchar*
mape_group_get_name(MapeGroup* group)
{
  g_return_val_if_fail(MAPE_IS_GROUP(group), NULL);
  g_return_val_if_fail(mape_group_is_open(group), NULL);

  return c4_group_handle_get_name(MAPE_GROUP_PRIVATE(group)->handle);
}

/**
 * mape_group_get_full_name:
 * @group: An open #MapeGroup.
 *
 * Returns the full path to the group.
 *
 * Return Value: The group path. Free with g_free() when no longer needed.
 **/
gchar*
mape_group_get_full_name(MapeGroup* group)
{
  g_return_val_if_fail(MAPE_IS_GROUP(group), NULL);
  g_return_val_if_fail(mape_group_is_open(group), NULL);

  return c4_group_handle_get_full_name(MAPE_GROUP_PRIVATE(group)->handle);
}

/**
 * mape_group_has_entry:
 * @group: An open #MapeGroup.
 * @entry: The entry name to check.
 *
 * Returns %TRUE if @group contains an entry with the given name, and %FALSE
 * otherwise.
 */
gboolean
mape_group_has_entry(MapeGroup* group,
                     const gchar* entry)
{
  MapeGroupPrivate* priv;
#ifdef G_OS_WIN32
  DWORD chk_drv;
#endif

  g_return_val_if_fail(MAPE_IS_GROUP(group), FALSE);
  g_return_val_if_fail(mape_group_is_open(group), FALSE);
  g_return_val_if_fail(entry != NULL, FALSE);

  priv = MAPE_GROUP_PRIVATE(group);

#ifdef G_OS_WIN32
  if(priv->handle == NULL)
  {
    if(entry[0] == '\0') return FALSE;
    if(entry[1] != ':') return FALSE;

    chk_drv = 1 << (entry[0] - 'A');
    return (GetLogicalDrives() & chk_drv) != 0;
  }
#endif

  c4_group_handle_reset_search(priv->handle);
  return c4_group_handle_find_next_entry(priv->handle, entry,
                                         NULL, NULL, FALSE);
}

/**
 * mape_group_rewind:
 * @group: An open #MapeGroup.
 *
 * Resets the group's internal iterator. The next call to
 * mape_group_get_next_entry() will return the first entry in the group again.
 */
void
mape_group_rewind(MapeGroup* group)
{
  MapeGroupPrivate* priv;

  g_return_if_fail(MAPE_IS_GROUP(group));
  g_return_if_fail(mape_group_is_open(group));

  priv = MAPE_GROUP_PRIVATE(group);

#ifdef G_OS_WIN32
  if(priv->handle == NULL)
  {
    priv->drive = 1;
    return;
  }
#endif

  c4_group_handle_reset_search(priv->handle);
}

/**
 * mape_group_get_next_entry:
 * @group: An open #MapeGroup.
 * @error: Location to store error information, if any.
 *
 * Advances the group's internal iterator by one and returns the name of the
 * entry it now points to. Use mape_group_load_entry() to load its contents
 * into memory, or mape_group_open_child() if you expect the entry to be a
 * subgroup. If there are no more entries in the group, the function returns
 * %NULL.
 *
 * Returns: The name of the next entry, or %NULL if there is no next entry.
 */
gchar*
mape_group_get_next_entry(MapeGroup* group)
{
  MapeGroupPrivate* priv;
  gchar* buf;
#ifdef G_OS_WIN32
  DWORD drv_c;
  guint drive;
#endif

  g_return_val_if_fail(MAPE_IS_GROUP(group), NULL);
  g_return_val_if_fail(mape_group_is_open(group), NULL);

  priv = MAPE_GROUP_PRIVATE(group);

#ifdef G_OS_WIN32
  static const guint DRV_C_SUPPORT = 26;
  if(priv->handle == NULL)
  {
    drv_c = GetLogicalDrives();

    /* Find next available drive or wait for overflow */
    drive = priv->drive - 1;
    while( (drive < DRV_C_SUPPORT) && ((~drv_c & (1 << drive)) != 0))
      ++drive;
    if(drive >= DRV_C_SUPPORT) return NULL;

    buf = g_malloc(3 * sizeof(gchar));
    buf[0] = 'A' + drive;
    buf[1] = ':';
    buf[2] = '\0';
    priv->drive = drive + 2;

    return buf;
  }
#endif

  buf = g_malloc(512 * sizeof(gchar));
  if(!c4_group_handle_find_next_entry(priv->handle, "*", NULL, buf, FALSE))
  {
    g_free(buf);
    buf = NULL;
  }

  return buf;
}

/**
 * mape_group_load_entry:
 * @group: An open #MapeGroup.
 * @entry: The name of the file to open.
 * @size: Location to store the size of the entry in, or %NULL.
 * @error: Location to store error information, if any.
 *
 * Attempts to load the entry with the given name
 * into memory. If @size is non-%NULL, the size of the
 * entry will be stored at the location @size points to. On error, %NULL
 * is returned, @error is set and @size is left untouched.
 *
 * Returns: A pointer pointing to the loaded entry's contents.
 */
guchar*
mape_group_load_entry(MapeGroup* group,
                      const gchar* entry,
                      gsize* size,
                      GError** error)
{
  MapeGroupPrivate* priv;
  gsize s;
  guchar* res;

  g_return_val_if_fail(MAPE_IS_GROUP(group), NULL);
  g_return_val_if_fail(mape_group_is_open(group), NULL);
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);

  priv = MAPE_GROUP_PRIVATE(group);
  if(!c4_group_handle_access_entry(priv->handle, entry, &s, NULL, FALSE))
  {
    g_set_error(error, mape_group_error_quark, MAPE_GROUP_ERROR_ACCESS,
              "%s", c4_group_handle_get_error(priv->handle));
    return NULL;
  }

  res = g_malloc(s);
  if(!c4_group_handle_read(priv->handle, (char*)res, s))
  {
    g_set_error(error, mape_group_error_quark, MAPE_GROUP_ERROR_READ,
                "%s", c4_group_handle_get_error(priv->handle));
    g_free(res);
    return NULL;
  }

  if(size != NULL) *size = s;
  return res;
}

/**
 * mape_group_is_folder:
 * @group: An open #MapeGroup.
 *
 * Returns whether this group is a directory or a packed file.
 *
 * Returns: %TRUE if the group is a directory or %FALSE if it is a file.
 */
gboolean
mape_group_is_folder(MapeGroup* group)
{
  MapeGroupPrivate* priv;
  C4GroupHandleStatus status;

  g_return_val_if_fail(MAPE_IS_GROUP(group), FALSE);
  g_return_val_if_fail(mape_group_is_open(group), FALSE);

  priv = MAPE_GROUP_PRIVATE(group);

#ifdef G_OS_WIN32
  if(priv->handle == NULL)
    return TRUE;
#endif

  status = c4_group_handle_get_status(priv->handle);
  if(status == C4_GROUP_HANDLE_FOLDER) return TRUE;
  return FALSE;
}

/*
 * mape_group_is_drive_container:
 * @group: An open #MapeGroup.
 *
 * Returns whether @group is a proxy folder which contains the available
 * drives on Windows (C:\, D:\, etc.) as subfolders. This is what one gets
 * when opening the root folder "/" on Windows. On Unix, the function always
 * returns %FALSE.
 *
 * Returns: Whether @group contains the available drives.
 */
gboolean
mape_group_is_drive_container(MapeGroup* group)
{
  g_return_val_if_fail(MAPE_IS_GROUP(group), FALSE);
  g_return_val_if_fail(mape_group_is_open(group), FALSE);

#ifdef G_OS_WIN32
  {
    MapeGroupPrivate* priv = MAPE_GROUP_PRIVATE(group);
    if(priv->handle == NULL)
      return TRUE;
  }
#endif

  return FALSE;
}

/*
 * mape_group_is_child_folder:
 * @group: An open #MapeGroup.
 * @child: A child entry.
 *
 * Returns whether the given entry is a subgroup or not. The current
 * implementation only works exactly for directories, not for packed groups.
 * For the latter it makes only a guess upon the extension of the entry.
 *
 * Returns: %TRUE if @child is a subgroup of @group, %FALSE otherwise.
 */
gboolean
mape_group_is_child_folder(MapeGroup* group,
                           const gchar* child)
{
  MapeGroupPrivate* priv;
  const gchar* ext;
  gchar* fullname;
  gchar* filename;
  gboolean result;

  g_return_val_if_fail(MAPE_IS_GROUP(group), FALSE);
  g_return_val_if_fail(mape_group_is_open(group), FALSE);
  g_return_val_if_fail(child != NULL, FALSE);

  priv = MAPE_GROUP_PRIVATE(group);

#ifdef G_OS_WIN32
  /* Drives are always folders */
  if(priv->handle == NULL)
    return TRUE;
#endif

  /* Guess on extension */
  ext = strrchr(child, '.');
  if(ext != NULL)
  {
    if(g_ascii_strcasecmp(ext, ".ocs") == 0 ||
       g_ascii_strcasecmp(ext, ".ocd") == 0 ||
       g_ascii_strcasecmp(ext, ".ocf") == 0 ||
       g_ascii_strcasecmp(ext, ".ocg") == 0)
   {
     return TRUE;
   }
  }

  /* Otherwise assume it's not a subgroup */
  if(c4_group_handle_get_status(priv->handle) != C4_GROUP_HANDLE_FOLDER)
    return FALSE;

  /* It is an open directory - check for regular directory */
  fullname = c4_group_handle_get_full_name(priv->handle);
  filename = g_build_filename(fullname, child, NULL);
  g_free(fullname);

  result = g_file_test(filename, G_FILE_TEST_IS_DIR);
  g_free(filename);

  return result;
}

/* This function is for internal use only */
C4GroupHandle*
_mape_group_get_handle(MapeGroup* group)
{
  g_return_val_if_fail(MAPE_IS_GROUP(group), NULL);
  return MAPE_GROUP_PRIVATE(group)->handle;
}

/* vim:set et sw=2 ts=2: */
