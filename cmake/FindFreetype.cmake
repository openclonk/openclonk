# OpenClonk, http://www.openclonk.org
#
# Copyright (c) 2014-2016, The OpenClonk Team and contributors
#
# Distributed under the terms of the ISC license; see accompanying file
# "COPYING" for details.
#
# "Clonk" is a registered trademark of Matthes Bender, used with permission.
# See accompanying file "TRADEMARK" for details.
#
# To redistribute this file separately, substitute the full license texts
# for the above references.

# This module defines
#  FREETYPE_LIBRARIES, the library to link against
#  FREETYPE_FOUND, if false, do not try to link to FREETYPE
#  FREETYPE_INCLUDE_DIRS, where to find headers.
#  FREETYPE_VERSION_STRING, the version of freetype found

# Use pkg-config if possible instead of doing guesswork like the default CMake module does
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
	set(_ft_pkgconfig_args "")
	if(FREETYPE_FIND_QUIET)
		set(_ft_pkgconfig_args "${_ft_pkgconfig_args}QUIET ")
	endif()
	set(_ft_pkgconfig_args "${_ft_pkgconfig_args}freetype2")
	if(FREETYPE_FIND_VERSION)
		if(NOT FREETYPE_FIND_VERSION_EXACT)
			set(_ft_pkgconfig_args "${_ft_pkgconfig_args}>")
		endif()
		set(_ft_pkgconfig_args "${_ft_pkgconfig_args}=${FREETYPE_FIND_VERSION} ")
	endif()
	pkg_check_modules(FREETYPE ${_ft_pkgconfig_args})
endif()

if(NOT FREETYPE_FOUND)
	# Fallback to system FindFreetype
	set(_ft_module_path "${CMAKE_MODULE_PATH}")
	unset(CMAKE_MODULE_PATH)
	include(FindFreetype)
	set(CMAKE_MODULE_PATH "${_ft_module_path}")
endif()
