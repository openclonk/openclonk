# OpenClonk, http://www.openclonk.org
#
# Copyright (c) 2012-2015, The OpenClonk Team and contributors
#
# Distributed under the terms of the ISC license; see accompanying file
# "COPYING" for details.
#
# "Clonk" is a registered trademark of Matthes Bender, used with permission.
# See accompanying file "TRADEMARK" for details.
#
# To redistribute this file separately, substitute the full license texts
# for the above references.

# - Find libupnp
# Find the libupnp library
# This module defines
#  UPNP_INCLUDE_DIR, where to find upnp.h, etc.
#  UPNP_LIBRARIES, the libraries needed to use libupnp.
#  UPNP_FOUND, If false, do not try to use libupnp.

# TODO: Use pkg-config if available

if(WIN32)
	CHECK_INCLUDE_FILE_CXX(natupnp.h HAVE_NATIVE_NATUPNP)
	if(NOT HAVE_NATIVE_NATUPNP)
		SET(UPNP_INCLUDE_DIR "thirdparty/natupnp")
	else()
		SET(UPNP_INCLUDE_DIR)
	endif()
	SET(UPNP_LIBRARIES)
	SET(UPNP_FOUND TRUE)
	SET(UPNP_STYLE "Win32")
else()
	find_path(UPNP_INCLUDE_DIR NAMES upnp.h PATH_SUFFIXES upnp)
	set(UPNP_NAMES ${UPNP_NAMES} upnp)
	set(THREADUTIL_NAMES ${THREADUTIL_NAMES} threadutil)
	set(IXML_NAMES ${IXML_NAMES} ixml)
	find_library(UPNP_LIBRARY NAMES ${UPNP_NAMES})
	find_library(THREADUTIL_LIBRARY NAMES ${THREADUTIL_NAMES})
	find_library(IXML_LIBRARY NAMES ${IXML_NAMES})

	include(FindPackageHandleStandardArgs)
	FIND_PACKAGE_HANDLE_STANDARD_ARGS(UPNP DEFAULT_MSG UPNP_LIBRARY THREADUTIL_LIBRARY IXML_LIBRARY UPNP_INCLUDE_DIR)

	if(UPNP_FOUND)
		set(UPNP_LIBRARIES ${UPNP_LIBRARY} ${THREADUTIL_LIBRARY} ${IXML_LIBRARY})
		set(UPNP_INCLUDE_DIR ${UPNP_INCLUDE_DIR})
		set(UPNP_STYLE "libupnp")
	endif()
endif()

mark_as_advanced(UPNP_LIBRARY IXML_LIBRARY UPNP_INCLUDE_DIR)
