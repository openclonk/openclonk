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

# Locate Epoxy.
# This module defines
#  Epoxy_INCLUDE_DIRS - a list of directories that need to be added to the include path
#  Epoxy_LIBRARIES - a list of libraries to link against to use Epoxy
#  Epoxy_FOUND - if false, Epoxy cannot be used

find_path(Epoxy_INCLUDE_DIR epoxy/gl.h)
mark_as_advanced(Epoxy_INCLUDE_DIR)
find_library(Epoxy_LIBRARY epoxy)
mark_as_advanced(Epoxy_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Epoxy REQUIRED_VARS Epoxy_LIBRARY Epoxy_INCLUDE_DIR)

if (Epoxy_FOUND)
	set(Epoxy_LIBRARIES "${Epoxy_LIBRARY}")
	set(Epoxy_INCLUDE_DIRS "${Epoxy_INCLUDE_DIR}")
endif()
