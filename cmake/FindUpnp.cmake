# - Find libupnp
# Find the libupnp library
# This module defines
#  UPNP_INCLUDE_DIR, where to find upnp.h, etc.
#  UPNP_LIBRARIES, the libraries needed to use libupnp.
#  UPNP_FOUND, If false, do not try to use libupnp.

#=============================================================================
# OpenClonk, http://www.openclonk.org
#
# Copyright (c) 2012  Armin Burgmeier
# Copyright (c) 2012  Nicolas Hake
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# See isc_license.txt for full license and disclaimer.
#
# "Clonk" is a registered trademark of Matthes Bender.
# See clonk_trademark_license.txt for full license.
#=============================================================================

# TODO: Use pkg-config if available

find_path(UPNP_INCLUDE_DIR NAMES upnp.h PATH_SUFFIXES upnp)
set(UPNP_NAMES ${UPNP_NAMES} upnp)
set(IXML_NAMES ${IXML_NAMES} ixml)
find_library(UPNP_LIBRARY NAMES ${UPNP_NAMES})
find_library(IXML_LIBRARY NAMES ${IXML_NAMES})

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(UPNP DEFAULT_MSG UPNP_LIBRARY IXML_LIBRARY UPNP_INCLUDE_DIR)

if(UPNP_FOUND)
	set(UPNP_LIBRARIES ${UPNP_LIBRARY} ${IXML_LIBRARY})
	set(UPNP_INCLUDE_DIR ${UPNP_INCLUDE_DIR})
endif()

mark_as_advanced(UPNP_LIBRARY IXML_LIBRARY UPNP_INCLUDE_DIR)
