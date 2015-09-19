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

# Locate GLEW.
# This module defines
#  GLEW_INCLUDE_DIRS - a list of directories that need to be added to the include path
#  GLEW_LIBRARIES - a list of libraries to link against to use GLEW
#  GLEW_DEFINITIONS - a list of compile-time macros that need to be defined to use GLEW
#  GLEW_FOUND - if false, GLEW cannot be used

find_path(GLEW_INCLUDE_DIR GL/glew.h PATH_SUFFIXES)
mark_as_advanced(GLEW_INCLUDE_DIR)

# Read GLEW version from header
if (GLEW_INCLUDE_DIR AND EXISTS "${GLEW_INCLUDE_DIR}/GL/glew.h")
	file(STRINGS "${GLEW_INCLUDE_DIR}/GL/glew.h" glew_version_str REGEX "^VERSION .+")
	string(REGEX REPLACE "^VERSION (.+)" "\\1" GLEW_VERSION_STRING "${glew_version_str}")
	unset(glew_version_str)
endif()

# On OS other than Windows, it doesn't matter whether we confuse the shared
# library and the static one. On Windows, we need to #define GLEW_STATIC if
# (and only if) we're linking against the static library. "glew32" may match
# the static library on MinGW, so we have to test for that explicitly.
find_library(GLEW_STATIC_LIBRARY glew32s)
mark_as_advanced(GLEW_STATIC_LIBRARY)
find_library(GLEW_SHARED_LIBRARY NAMES GLEW glew32)
mark_as_advanced(GLEW_SHARED_LIBRARY)

if (GLEW_SHARED_LIBRARY)
	set(GLEW_LIBRARY "${GLEW_SHARED_LIBRARY}")
	if (WIN32 AND MINGW AND GLEW_SHARED_LIBRARY MATCHES "\\.a$")
		# not actually a shared library
		set(GLEW_DEFINITIONS "-DGLEW_STATIC")
	else()
		set(GLEW_DEFINITIONS)
	endif()
elseif (GLEW_STATIC_LIBRARY)
	set(GLEW_LIBRARY "${GLEW_STATIC_LIBRARY}")
	set(GLEW_DEFINITIONS "-DGLEW_STATIC")
endif()

include(FindPackageHandleStandardArgs)
if (NOT "${CMAKE_VERSION}" VERSION_LESS "2.8.3")
	find_package_handle_standard_args(GLEW VERSION_VAR GLEW_VERSION_STRING REQUIRED_VARS GLEW_LIBRARY GLEW_INCLUDE_DIR)
else()
	find_package_handle_standard_args(GLEW GLEW_LIBRARY GLEW_INCLUDE_DIR)
endif()

if (GLEW_FOUND)
	set(GLEW_LIBRARIES "${GLEW_LIBRARY}")
	set(GLEW_INCLUDE_DIRS "${GLEW_INCLUDE_DIR}")
endif()
