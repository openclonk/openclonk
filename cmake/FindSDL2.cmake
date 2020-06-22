# OpenClonk, http://www.openclonk.org
#
# Copyright (c) 2016, The OpenClonk Team and contributors
#
# Distributed under the terms of the ISC license; see accompanying file
# "COPYING" for details.
#
# "Clonk" is a registered trademark of Matthes Bender, used with permission.
# See accompanying file "TRADEMARK" for details.
#
# To redistribute this file separately, substitute the full license texts
# for the above references.

# Locate SDL2.
# This module defines
#  SDL2_INCLUDE_DIRS - a list of directories that need to be added to the include path
#  SDL2_LIBRARIES - a list of libraries to link against to use SDL2
#  SDL2_FOUND - if false, SDL2 cannot be used

find_path(SDL2_INCLUDE_DIR SDL.h 
	HINTS 
	$ENV{SDL2DIR}
	PATH_SUFFIXES SDL2 include
)
mark_as_advanced(SDL2_INCLUDE_DIR)

find_library(SDL2_LIBRARY 
	SDL2 
	HINTS 
	$ENV{SDL2DIR}
	PATH_SUFFIXES lib
)
mark_as_advanced(SDL2_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SDL2 REQUIRED_VARS SDL2_LIBRARY SDL2_INCLUDE_DIR)

if (SDL2_FOUND)
	set(SDL2_LIBRARIES "${SDL2_LIBRARY}")
	set(SDL2_INCLUDE_DIRS "${SDL2_INCLUDE_DIR}")
endif()
