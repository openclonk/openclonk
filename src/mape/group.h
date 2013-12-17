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

#ifndef INC_MAPE_GROUP_H
#define INC_MAPE_GROUP_H

#include <glib-object.h>

G_BEGIN_DECLS

#define MAPE_TYPE_GROUP                 (mape_group_get_type())
#define MAPE_GROUP(obj)                 (G_TYPE_CHECK_INSTANCE_CAST((obj), MAPE_TYPE_GROUP, MapeGroup))
#define MAPE_GROUP_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST((klass), MAPE_TYPE_GROUP, MapeGroupClass))
#define MAPE_IS_GROUP(obj)              (G_TYPE_CHECK_INSTANCE_TYPE((obj), MAPE_TYPE_GROUP))
#define MAPE_IS_GROUP_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE((klass), MAPE_TYPE_GROUP))
#define MAPE_GROUP_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS((obj), MAPE_TYPE_GROUP, MapeGroupClass))

typedef struct _MapeGroup MapeGroup;
typedef struct _MapeGroupClass MapeGroupClass;

/**
 * MapeGroupError:
 * @MAPE_GROUP_ERROR_OPEN: An error occured when attempting to open the group.
 * @MAPE_GROUP_ERROR_ACCESS: An error occurred when accessing a group element.
 * @MAPE_GROUP_ERROR_READ: An error occured when reading from the group.
 *
 * These errors are from the MAPE_GROUP_ERROR error domain. They can occur
 * when opening, seeking or reading from a group, respectively.
 */
typedef enum _MapeGroupError {
  MAPE_GROUP_ERROR_OPEN,
  MAPE_GROUP_ERROR_ACCESS,
  MAPE_GROUP_ERROR_READ
} MapeGroupError;

/**
 * MapeGroupClass:
 *
 * This structure does not contain any public fields.
 */
struct _MapeGroupClass {
  /*< private >*/
  GObjectClass parent_class;
};

/**
 * MapeGroup:
 *
 * #MapeGroup is an opaque data type. You should only access it via the
 * public API functions.
 */
struct _MapeGroup {
  /*< private >*/
  GObject parent;
};

GType
mape_group_get_type(void) G_GNUC_CONST;

MapeGroup*
mape_group_new(void);

gboolean
mape_group_is_open(MapeGroup* group);

gboolean
mape_group_open(MapeGroup* group,
                const gchar* path,
                GError** error);

MapeGroup*
mape_group_open_child(MapeGroup* group,
                      const gchar* entry,
                      GError** error);

void
mape_group_close(MapeGroup* group);

const gchar*
mape_group_get_name(MapeGroup* group);

gchar*
mape_group_get_full_name(MapeGroup* group);

gboolean
mape_group_has_entry(MapeGroup* group,
                     const gchar* entry);

void
mape_group_rewind(MapeGroup* group);

gchar*
mape_group_get_next_entry(MapeGroup* group);

guchar*
mape_group_load_entry(MapeGroup* group,
                      const gchar* entry,
                      gsize* size,
                      GError** error);

gboolean
mape_group_is_folder(MapeGroup* group);

gboolean
mape_group_is_drive_container(MapeGroup* group);

gboolean
mape_group_is_child_folder(MapeGroup* group,
                           const gchar* child);

G_END_DECLS

#endif /* INC_MAPE_GROUP_H */

/* vim:set et sw=2 ts=2: */
