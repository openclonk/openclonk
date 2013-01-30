/*
 * mape - C4 Landscape.txt editor
 *
 * Copyright (c) 2005-2009 Armin Burgmeier
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

#include "C4Include.h"
#include "C4Group.h"
#include "mape/cpp-handles/group-handle.h"

#define GROUP_TO_HANDLE(group) (reinterpret_cast<C4GroupHandle*>(group))
#define HANDLE_TO_GROUP(handle) (reinterpret_cast<C4Group*>(handle))

extern "C" {

C4GroupHandle* c4_group_handle_new(void)
{
  return GROUP_TO_HANDLE(new C4Group);
}

void c4_group_handle_free(C4GroupHandle* handle)
{
  delete HANDLE_TO_GROUP(handle);
}

const gchar* c4_group_handle_get_error(C4GroupHandle* handle)
{
  return HANDLE_TO_GROUP(handle)->GetError();
}

gboolean c4_group_handle_open(C4GroupHandle* handle, const gchar* path, gboolean create)
{
  return HANDLE_TO_GROUP(handle)->Open(path, create);
}

gboolean c4_group_handle_open_as_child(C4GroupHandle* handle, C4GroupHandle* mother, const gchar* name, gboolean exclusive, gboolean create)
{
  return HANDLE_TO_GROUP(handle)->OpenAsChild(HANDLE_TO_GROUP(mother),
                                              name, exclusive, create);
}

const gchar* c4_group_handle_get_name(C4GroupHandle* handle)
{
  return HANDLE_TO_GROUP(handle)->GetName();
}

gchar* c4_group_handle_get_full_name(C4GroupHandle* handle)
{
  StdStrBuf buf(HANDLE_TO_GROUP(handle)->GetFullName());
  gchar* res = static_cast<gchar*>(g_malloc(buf.getSize()*sizeof(gchar)));
  memcpy(res, buf.getData(), buf.getSize());
  return res;
}

void c4_group_handle_reset_search(C4GroupHandle* handle)
{
  HANDLE_TO_GROUP(handle)->ResetSearch();
}

gboolean c4_group_handle_find_next_entry(C4GroupHandle* handle, const gchar* wildcard, gsize* size, gchar* filename, gboolean start_at_filename)
{
  return HANDLE_TO_GROUP(handle)->FindNextEntry(wildcard, filename, size, start_at_filename);
}

gboolean c4_group_handle_access_next_entry(C4GroupHandle* handle, const gchar* wildcard, gsize* size, gchar* filename, gboolean start_at_filename)
{
  return HANDLE_TO_GROUP(handle)->AccessNextEntry(wildcard, size, filename, start_at_filename);
}

gboolean c4_group_handle_access_entry(C4GroupHandle* handle, const gchar* wildcard, gsize* size, gchar* filename, gboolean needs_to_be_a_group)
{
  return HANDLE_TO_GROUP(handle)->AccessEntry(wildcard, size, filename, needs_to_be_a_group);
}

gsize c4_group_handle_accessed_entry_size(C4GroupHandle* handle)
{
  return HANDLE_TO_GROUP(handle)->AccessedEntrySize();
}

gboolean c4_group_handle_read(C4GroupHandle* handle, gpointer buffer, gsize size)
{
  return HANDLE_TO_GROUP(handle)->Read(buffer, size);
}

C4GroupHandleStatus c4_group_handle_get_status(C4GroupHandle* handle)
{
  int status = HANDLE_TO_GROUP(handle)->GetStatus();
  switch(status)
  {
  case GRPF_Inactive: return C4_GROUP_HANDLE_INACTIVE;
  case GRPF_File: return C4_GROUP_HANDLE_FILE;
  case GRPF_Folder: return C4_GROUP_HANDLE_FOLDER;
  default: g_assert_not_reached(); return C4_GROUP_HANDLE_INACTIVE;
  }
}

} /* extern "C" */
