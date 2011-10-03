# - Find DbgHelp
# Find the DbgHelp library
# This module defines
#  DBGHELP_INCLUDE_DIR, where to find dbghelp.h, etc.
#  DBGHELP_LIBRARIES, the libraries needed to use DbgHelp.
#  DBGHELP_FOUND, If false, do not try to use DbgHelp.

#=============================================================================
# OpenClonk, http://www.openclonk.org
#
# Copyright (c) 2011  Nicolas Hake
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# See isc_license.txt for full license and disclaimer.
#
# "Clonk" is a registered trademark of Matthes Bender.
# See clonk_trademark_license.txt for full license.
#=============================================================================

find_path(DBGHELP_INCLUDE_DIR NAMES dbghelp.h)
set(DBGHELP_NAMES ${DBGHELP_NAMES} dbghelp)
find_library(DBGHELP_LIBRARY NAMES ${DBGHELP_NAMES})

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(DBGHELP DEFAULT_MSG DBGHELP_LIBRARY DBGHELP_INCLUDE_DIR)

if(DBGHELP_FOUND)
	set(DBGHELP_LIBRARIES ${DBGHELP_LIBRARY})
endif()

mark_as_advanced(DBGHELP_LIBRARY DBGHELP_INCLUDE_DIR)
