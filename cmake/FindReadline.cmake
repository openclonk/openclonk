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

# - Find READLINE
# Find the native READLINE includes and library
#
#  READLINE_INCLUDE_DIR - where to find READLINE.h, etc.
#  READLINE_LIBRARIES   - List of libraries when using READLINE.
#  READLINE_FOUND       - True if READLINE found.


IF (READLINE_INCLUDE_DIR)
	# Already in cache, be silent
	SET(READLINE_FIND_QUIETLY TRUE)
ENDIF (READLINE_INCLUDE_DIR)

FIND_PATH(READLINE_INCLUDE_DIR readline.h)

SET(READLINE_NAMES readline libreadline)
FIND_LIBRARY(READLINE_LIBRARY NAMES ${READLINE_NAMES} )

# handle the QUIETLY and REQUIRED arguments and set READLINE_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(READLINE DEFAULT_MSG READLINE_LIBRARY READLINE_INCLUDE_DIR)

IF(READLINE_FOUND)
	SET( READLINE_LIBRARIES ${READLINE_LIBRARY} )
ELSE(READLINE_FOUND)
	SET( READLINE_LIBRARIES )
ENDIF(READLINE_FOUND)
	
MARK_AS_ADVANCED( READLINE_LIBRARY READLINE_INCLUDE_DIR )

