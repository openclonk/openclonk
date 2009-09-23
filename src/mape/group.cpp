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

#define MAPE_COMPILING_CPP

#include <C4Group.h>
#include <glib.h>

#include "group.h"

#define CPPGROUP(group) ((C4Group*)group->group_handle)

extern "C" {

/* On Windows, / is interpreted as a directory containing the local drives
   (C:\, D:\, etc.). group_handle will be NULL in this case. */

MapeGroup* mape_group_new(const char* path,
                          GError** error)
{
	MapeGroup* group;
	group = (MapeGroup*)malloc(sizeof(MapeGroup) );
	
	group->group_handle = NULL;

#ifdef G_OS_WIN32
	group->drive_idtf = 0;
	if(strcmp(path, "/") == 0)
	{
		//group->drive_idtf = TRUE;
		return group;
	}
#endif

	group->group_handle = new C4Group;
	if(CPPGROUP(group)->Open(path, FALSE) == FALSE)
	{
		g_set_error(
			error,
			g_quark_from_static_string("MAPE_GROUP_ERROR"),
			MAPE_GROUP_ERROR_OPEN,
			"Could not open '%s': %s",
			path,
			CPPGROUP(group)->GetError()
		);

		delete CPPGROUP(group);
		free(group);

		return NULL;
	}

	return group;
}

MapeGroup* mape_group_new_from_parent(MapeGroup* parent,
                                      const char* entry,
                                      GError** error)
{
	MapeGroup* group;
	bool result;

	group = (MapeGroup*)malloc(sizeof(MapeGroup) );

	group->group_handle = new C4Group;
	
#ifdef G_OS_WIN32
	if(parent->group_handle == NULL)
	{
		result = CPPGROUP(group)->Open(entry, FALSE);
	}
	else
#endif
	{
		result = CPPGROUP(group)->OpenAsChild(
			CPPGROUP(parent),
			entry,
			FALSE
		);
	}
	
	if(result == FALSE)
	{
		g_set_error(
			error,
			g_quark_from_static_string("MAPE_GROUP_ERROR"),
			MAPE_GROUP_ERROR_OPEN,
			"%s",
			CPPGROUP(group)->GetError()
		);
		
		delete CPPGROUP(group);
		free(group);

		return NULL;
	}

	return group;
}

void mape_group_destroy(MapeGroup* group)
{
	if(group->group_handle != NULL)
		delete CPPGROUP(group);

	free(group);
}

const char* mape_group_get_name(MapeGroup* group)
{
	g_assert(group->group_handle != NULL);
	return CPPGROUP(group)->GetName();
}

const char* mape_group_get_full_name(MapeGroup* group)
{
	g_assert(group->group_handle != NULL);
	// TODO: Might this corrupt memory? Should we return a copy?
	return CPPGROUP(group)->GetFullName().getData();
}

gboolean mape_group_has_entry(MapeGroup* group,
                              const char* entry)
{
#ifdef G_OS_WIN32
	DWORD chk_drv;
	if(group->group_handle == NULL)
	{
		if(entry[0] == '\0') return FALSE;
		if(entry[1] != ':') return FALSE;

		chk_drv = 1 << (entry[0] - 'A');
		if( (GetLogicalDrives() & chk_drv) != 0)
			return TRUE;
		else
			return FALSE;
	}
#endif

	CPPGROUP(group)->ResetSearch();
	return CPPGROUP(group)->FindEntry(entry) ? TRUE : FALSE;
}

void mape_group_rewind(MapeGroup* group)
{
#ifdef G_OS_WIN32
	if(group->group_handle == NULL)
	{
		group->drive_idtf = 0;
		return;
	}
#endif

	CPPGROUP(group)->ResetSearch();
}

char* mape_group_get_next_entry(MapeGroup* group)
{
	char* buf;
	bool result;

#ifdef G_OS_WIN32
	static const int DRV_C_SUPPORT = 26;
	DWORD drv_c;

	if(group->group_handle == NULL)
	{
		drv_c = GetLogicalDrives();
		
		/* Find next available drive or wait for overflow */
		while( (group->drive_idtf < DRV_C_SUPPORT) &&
		       (~drv_c & (1 << group->drive_idtf)) )
			++ group->drive_idtf;

		if(group->drive_idtf >= DRV_C_SUPPORT) return NULL;

		buf = (char*)malloc(3 * sizeof(char) );
		buf[0] = 'A' + group->drive_idtf;
		buf[1] = ':'; buf[2] = '\0';
		++ group->drive_idtf;
		
		return buf;
	}
#endif
	buf = (char*)malloc(_MAX_PATH);
	result = CPPGROUP(group)->FindNextEntry("*", buf);

	if(result == false)
		free(buf);

	return result ? buf : NULL;
}

gboolean mape_group_is_folder(MapeGroup* group)
{
	g_assert(group->group_handle != NULL);
	int status = CPPGROUP(group)->GetStatus();
	if(status == GRPF_Folder) return TRUE;
	return FALSE;
}

gboolean mape_group_is_child_folder(MapeGroup* group,
                                    const char* child)
{
	gchar* filename;
	const gchar* ext;
	gboolean result;
	
#ifdef G_OS_WIN32
	/* Drives are always folders */
	if(group->group_handle == NULL)
		return TRUE;
#endif

	// Check for C4? extension
	ext = strrchr(child, '.');
	if(ext != NULL)
	{
		if(stricmp(ext, ".c4s") == 0 ||
		   stricmp(ext, ".c4d") == 0 ||
		   stricmp(ext, ".c4f") == 0 ||
		   stricmp(ext, ".c4g") == 0)
		{
			return TRUE;
		}
	}

	// Packed directories are not supported
	if(CPPGROUP(group)->GetStatus() == GRPF_File)
		return FALSE;

	// No C4Group folder: Check for regular directory
	filename = g_build_filename(
		CPPGROUP(group)->GetName(),
		child,
		NULL
	);

	result = g_file_test(
		filename,
		G_FILE_TEST_IS_DIR
	);

	g_free(filename);
	return result;
}

}
