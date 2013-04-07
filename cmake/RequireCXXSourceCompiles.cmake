# OpenClonk, http://www.openclonk.org
#
# Copyright (c) 2013  Nicolas Hake
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# See isc_license.txt for full license and disclaimer.
#
# "Clonk" is a registered trademark of Matthes Bender.
# See clonk_trademark_license.txt for full license.

include(CheckCXXSourceCompiles)
macro(REQUIRE_CXX_SOURCE_COMPILES _code _var)
	CHECK_CXX_SOURCE_COMPILES("${_code}" ${_var})
	if(NOT ${_var})
		unset(${_var} CACHE)
		MESSAGE(FATAL_ERROR "${_var} is required for this project to build properly. Aborting.")
	endif()
endmacro()
