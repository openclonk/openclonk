# OpenClonk, http://www.openclonk.org
#
# Copyright (c) 2013, The OpenClonk Team and contributors
#
# Distributed under the terms of the ISC license; see accompanying file
# "COPYING" for details.
#
# "Clonk" is a registered trademark of Matthes Bender, used with permission.
# See accompanying file "TRADEMARK" for details.
#
# To redistribute this file separately, substitute the full license texts
# for the above references.

include(CheckCXXSourceCompiles)
macro(REQUIRE_CXX_SOURCE_COMPILES _code _var)
	CHECK_CXX_SOURCE_COMPILES("${_code}" ${_var})
	if(NOT ${_var})
		unset(${_var} CACHE)
		MESSAGE(FATAL_ERROR "${_var} is required for this project to build properly. Aborting.")
	endif()
endmacro()
