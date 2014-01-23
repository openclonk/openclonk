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

# - Find DbgHelp
# Find the DbgHelp library
# This module defines
#  DBGHELP_INCLUDE_DIR, where to find dbghelp.h, etc.
#  DBGHELP_LIBRARIES, the libraries needed to use DbgHelp.
#  DBGHELP_FOUND, If false, do not try to use DbgHelp.

find_path(DBGHELP_INCLUDE_DIR NAMES dbghelp.h)
set(DBGHELP_NAMES ${DBGHELP_NAMES} dbghelp)
find_library(DBGHELP_LIBRARY NAMES ${DBGHELP_NAMES})

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(DBGHELP DEFAULT_MSG DBGHELP_LIBRARY DBGHELP_INCLUDE_DIR)

if(DBGHELP_FOUND)
	set(DBGHELP_LIBRARIES ${DBGHELP_LIBRARY})
endif()

mark_as_advanced(DBGHELP_LIBRARY DBGHELP_INCLUDE_DIR)
