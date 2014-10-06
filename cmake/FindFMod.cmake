# OpenClonk, http://www.openclonk.org
#
# Copyright (c) 2011-2013, The OpenClonk Team and contributors
#
# Distributed under the terms of the ISC license; see accompanying file
# "COPYING" for details.
#
# "Clonk" is a registered trademark of Matthes Bender, used with permission.
# See accompanying file "TRADEMARK" for details.
#
# To redistribute this file separately, substitute the full license texts
# for the above references.

# - Find FMod
# Find the FMod library
# This module defines
#  FMOD_INCLUDE_DIR, where to find fmod.h, etc.
#  FMOD_LIBRARIES, the libraries needed to use FMod.
#  FMOD_FOUND, If false, do not try to use FMod.

find_path(FMOD_INCLUDE_DIR fmod.h)

if(CMAKE_CL_64)
	if(MSVC)
		set(FMOD_NAMES ${FMOD_NAMES} fmod64vc)
	else()
		set(FMOD_NAMES ${FMOD_NAMES} fmod64)
	endif()
else()
	if(MSVC)
		set(FMOD_NAMES ${FMOD_NAMES} fmodvc)
	else()
		set(FMOD_NAMES ${FMOD_NAMES} fmod)
	endif()
endif()
find_library(FMOD_LIBRARY NAMES ${FMOD_NAMES})

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(FMOD DEFAULT_MSG FMOD_LIBRARY FMOD_INCLUDE_DIR)

if(FMOD_FOUND)
	set(FMOD_LIBRARIES ${FMOD_LIBRARY})
endif()

mark_as_advanced(FMOD_LIBRARY FMOD_INCLUDE_DIR)
