# OpenClonk, http://www.openclonk.org
#
# Copyright (c) 2015, The OpenClonk Team and contributors
#
# Distributed under the terms of the ISC license; see accompanying file
# "COPYING" for details.
#
# "Clonk" is a registered trademark of Matthes Bender, used with permission.
# See accompanying file "TRADEMARK" for details.
#
# To redistribute this file separately, substitute the full license texts
# for the above references.

# Locate GTK3.
# This module defines
#  GTK3_INCLUDE_DIRS - a list of directories that need to be added to the include path
#  GTK3_LIBRARIES - a list of libraries to link against to use GTK3
#  GTK3_LIBRARY_DIRS - a list of library directories where the libraries above can be found
#  GTK3_COMPILE_DEFINITIONS - a list of compiler flags that need to be set to use GTK3
#  GTK3_FOUND - if false, GTK3 cannot be used
#
# FindGTK3 may optionally take a list of components that should also be located
# and added to the library list. The following components are valid:
#  gtksourceview - GtkSourceView 3.0
#  gobject - Glib GObject 2.0
#  gio - Glib GIO 2.0
#  gthread
#  glib - Glib 2.0
#
# If any of these components are requested, the following variables will be
# defined with the same meaning as above:
#  GTK3_<component>_INCLUDE_DIRS
#  GTK3_<component>_LIBRARIES
#  GTK3_<component>_FOUND

if(GTK3_FIND_QUIETLY)
	set(__GTK3_QUIET "QUIET")
else()
	set(__GTK3_QUIET)
endif()

include(FindPackageHandleStandardArgs)

if(NOT GTK3_FIND_COMPONENTS)
	set(GTK3_FIND_COMPONENTS)
endif()

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
	# If pkg-config is available, defer to it
	set(__GTK3_VERSION_CHECK)
	if(GTK3_FIND_VERSION_EXACT)
		set(__GTK3_VERSION_CHECK "=${GTK3_FIND_VERSION}")
	elseif(GTK3_FIND_VERSION)
		set(__GTK3_VERSION_CHECK ">=${GTK3_FIND_VERSION}")
	endif()

	pkg_check_modules(GTK3 ${__GTK3_QUIET} "gtk+-3.0${__GTK3_VERSION_CHECK}")

	macro(__GTK3_HANDLE_COMPONENT __cname __cfullname)
		if(__component STREQUAL "${__cname}")
			pkg_check_modules(GTK3_${__cname} ${__GTK3_QUIET} "${__cfullname}")
			if(GTK3_${__cname}_FOUND)
				if(GTK3_LIBRARY_DIRS)
					list(REMOVE_ITEM GTK3_${__cname}_LIBRARY_DIRS ${GTK3_LIBRARY_DIRS})
				endif()
				if(GTK3_INCLUDE_DIRS)
					list(REMOVE_ITEM GTK3_${__cname}_INCLUDE_DIRS ${GTK3_INCLUDE_DIRS})
				endif()
				if(GTK3_FIND_REQUIRED_${__cname})
					list(APPEND GTK3_LIBRARIES ${GTK3_${__cname}_LIBRARIES})
					list(APPEND GTK3_INCLUDE_DIRS ${GTK3_${__cname}_INCLUDE_DIRS})
				endif()
			endif()
		endif()
	endmacro(__GTK3_HANDLE_COMPONENT)

	foreach(__component ${GTK3_FIND_COMPONENTS})
		__GTK3_HANDLE_COMPONENT(gtksourceview "gtksourceview-3.0")
		__GTK3_HANDLE_COMPONENT(gobject "gobject-2.0")
		__GTK3_HANDLE_COMPONENT(gio "gio-2.0")
		__GTK3_HANDLE_COMPONENT(gthread "gthread-2.0")
		__GTK3_HANDLE_COMPONENT(glib "glib-2.0")
	endforeach()

	set(GTK3_COMPILE_DEFINITIONS ${GTK3_CFLAGS_OTHER})

	FIND_PACKAGE_HANDLE_STANDARD_ARGS(
		GTK3
		FOUND_VAR GTK3_FOUND
		VERSION_VAR GTK3_VERSION
		HANDLE_COMPONENTS
		REQUIRED_VARS
			GTK3_INCLUDE_DIRS GTK3_LIBRARIES
	)
else()
	# Find all headers and libraries
	find_path(GTK3_INCLUDE_DIR NAMES gtk/gtk.h PATH_SUFFIXES gtk-3.0)
	find_library(GTK3_LIBRARY NAMES gtk-3.0)
	mark_as_advanced(GTK3_INCLUDE_DIR GTK3_LIBRARY)

	find_path(GTK3_GDK_INCLUDE_DIR NAMES gdk/gdk.h PATH_SUFFIXES gtk-3.0)
	find_library(GTK3_GDK_LIBRARY NAMES gdk-3.0)
	mark_as_advanced(GTK3_GDK_INCLUDE_DIR GTK3_GDK_LIBRARY)

	find_path(GTK3_Glib_INCLUDE_DIR NAMES glib.h PATH_SUFFIXES glib-2.0)
	find_library(GTK3_Glib_LIBRARY NAMES glib-2.0)
	get_filename_component(__GTK3_Glib_LIBRARY_DIR "${GTK3_Glib_LIBRARY}" PATH)
	find_path(GTK3_Glibconfig_INCLUDE_DIR
		NAMES glibconfig.h
		HINTS ${__GTK3_Glib_LIBRARY_DIR}
		PATH_SUFFIXES glib-2.0/include
	)
	mark_as_advanced(GTK3_Glib_INCLUDE_DIR GTK3_Glibconfig_INCLUDE_DIR GTK3_Glib_LIBRARY)

	find_path(GTK3_Atk_INCLUDE_DIR NAMES atk/atk.h PATH_SUFFIXES atk-1.0)
	find_library(GTK3_Atk_LIBRARY NAMES atk-1.0)
	mark_as_advanced(GTK3_Atk_INCLUDE_DIR GTK3_Atk_LIBRARY)

	find_path(GTK3_Cairo_INCLUDE_DIR NAMES cairo.h)
	find_library(GTK3_Cairo_LIBRARY NAMES cairo)
	mark_as_advanced(GTK3_Cairo_INCLUDE_DIR GTK3_Cairo_LIBRARY)

	find_path(GTK3_Pango_INCLUDE_DIR NAMES pango/pango.h PATH_SUFFIXES pango-1.0)
	find_library(GTK3_Pango_LIBRARY NAMES pango-1.0)
    find_library(GTK3_PangoCairo_LIBRARY NAMES pangocairo-1.0)
	mark_as_advanced(GTK3_Pango_INCLUDE_DIR GTK3_Pango_LIBRARY GTK3_PangoCairo_LIBRARY)

	find_path(GTK3_GDKPixbuf_INCLUDE_DIR NAMES gdk-pixbuf/gdk-pixbuf.h PATH_SUFFIXES gdk-pixbuf-2.0)
	find_library(GTK3_GDKPixbuf_LIBRARY NAMES gdk_pixbuf-2.0)
	mark_as_advanced(GTK3_GDKPixbuf_INCLUDE_DIR GTK3_GDKPixbuf_LIBRARY)

	set(__GTK3_REQUIRED_INCLUDE_DIRS
		GTK3_INCLUDE_DIR
		GTK3_GDK_INCLUDE_DIR
		GTK3_Glib_INCLUDE_DIR GTK3_Glibconfig_INCLUDE_DIR
		GTK3_Atk_INCLUDE_DIR
		GTK3_Cairo_INCLUDE_DIR
		GTK3_Pango_INCLUDE_DIR
		GTK3_GDKPixbuf_INCLUDE_DIR
	)
	set(__GTK3_REQUIRED_LIBRARIES
		GTK3_LIBRARY
		GTK3_GDK_LIBRARY
		GTK3_Glib_LIBRARY
		GTK3_Atk_LIBRARY
		GTK3_Cairo_LIBRARY
		GTK3_Pango_LIBRARY GTK3_PangoCairo_LIBRARY
		GTK3_GDKPixbuf_LIBRARY
	)

	set(__GTK3_COMPONENT_INCLUDE_DIRS)
	set(__GTK3_COMPONENT_LIBRARIES)
	macro(__GTK3_HANDLE_COMPONENT __cname __cheader __csuffixes __clibrary)
		if(__component STREQUAL "${__cname}")
			find_path(GTK3_${__cname}_INCLUDE_DIR NAMES ${__cheader} PATH_SUFFIXES ${__csuffixes})
			find_library(GTK3_${__cname}_LIBRARY NAMES ${__clibrary})
			mark_as_advanced(GTK3_${__cname}_INCLUDE_DIR GTK3_${__cname}_LIBRARY)
			if(GTK3_${__cname}_INCLUDE_DIR AND GTK3_${__cname}_LIBRARY)
				set(GTK3_${__cname}_FOUND TRUE)
				set(GTK3_${__cname}_INCLUDE_DIRS ${GTK3_${__cname}_INCLUDE_DIR})
				set(GTK3_${__cname}_LIBRARIES ${GTK3_${__cname}_LIBRARY})
				if(GTK3_FIND_REQUIRED_${__cname})
					list(APPEND __GTK3_REQUIRED_LIBRARIES GTK3_${__cname}_LIBRARY)
					list(APPEND __GTK3_REQUIRED_INCLUDE_DIRS GTK3_${__cname}_INCLUDE_DIR)
				endif()
			endif()
		endif()
	endmacro(__GTK3_HANDLE_COMPONENT)
	foreach(__component ${GTK3_FIND_COMPONENTS})
		__GTK3_HANDLE_COMPONENT(gtksourceview gtksourceview/gtksourceview.h gtksourceview-3.0 gtksourceview-3.0)
		__GTK3_HANDLE_COMPONENT(gobject gobject/gobject.h glib-2.0 gobject-2.0)
		__GTK3_HANDLE_COMPONENT(gio gio/gio.h glib-2.0 gio-2.0)
		__GTK3_HANDLE_COMPONENT(gthread glib/gthread.h glib-2.0 gthread-2.0)
		__GTK3_HANDLE_COMPONENT(glib glib.h glib-2.0 glib-2.0)
	endforeach()

	# Parse version from GTK3 header
	if(GTK3_INCLUDE_DIR)
		file(READ "${GTK3_INCLUDE_DIR}/gtk/gtkversion.h" _gtk_header)
		string(REGEX REPLACE ".*#define GTK_MAJOR_VERSION \\(([0-9]+)\\).*" "\\1" __GTK3_MAJOR_VER "${_gtk_header}")
		string(REGEX REPLACE ".*#define GTK_MINOR_VERSION \\(([0-9]+)\\).*" "\\1" __GTK3_MINOR_VER "${_gtk_header}")
		string(REGEX REPLACE ".*#define GTK_MICRO_VERSION \\(([0-9]+)\\).*" "\\1" __GTK3_MICRO_VER "${_gtk_header}")

		set(GTK3_VERSION "${__GTK3_MAJOR_VER}.${__GTK3_MINOR_VER}.${__GTK3_MICRO_VER}")
		unset(__GTK3_MICRO_VER)
		unset(__GTK3_MINOR_VER)
		unset(__GTK3_MAJOR_VER)
		unset(_gtk_header)
	endif()

	FIND_PACKAGE_HANDLE_STANDARD_ARGS(
		GTK3
		FOUND_VAR GTK3_FOUND
		VERSION_VAR GTK3_VERSION
		HANDLE_COMPONENTS
		REQUIRED_VARS
			${__GTK3_REQUIRED_INCLUDE_DIRS} ${__GTK3_REQUIRED_LIBRARIES}
	)
	if(GTK3_FOUND)
		set(GTK3_INCLUDE_DIRS)
		foreach(__dir ${__GTK3_REQUIRED_INCLUDE_DIRS})
			list(APPEND GTK3_INCLUDE_DIRS "${${__dir}}")
		endforeach()
		set(GTK3_LIBRARIES)
		foreach(__lib ${__GTK3_REQUIRED_LIBRARIES})
			list(APPEND GTK3_LIBRARIES "${${__lib}}")
		endforeach()
		list(REMOVE_DUPLICATES GTK3_LIBRARIES)
		list(REMOVE_DUPLICATES GTK3_INCLUDE_DIRS)
		set(GTK3_COMPILE_DEFINITIONS)
		set(GTK3_LIBRARY_DIRS)
	endif()

	unset(__GTK3_REQUIRED_INCLUDE_DIRS)
	unset(__GTK3_REQUIRED_LIBRARIES)
endif()

if(NOT GTK3_FOUND)
    unset(GTK3_INCLUDE_DIRS)
    unset(GTK3_LIBRARIES)
endif()
