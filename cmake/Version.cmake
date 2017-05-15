# OpenClonk, http://www.openclonk.org
#
# Copyright (c) 2009-2016, The OpenClonk Team and contributors
#
# Distributed under the terms of the ISC license; see accompanying file
# "COPYING" for details.
#
# "Clonk" is a registered trademark of Matthes Bender, used with permission.
# See accompanying file "TRADEMARK" for details.
#
# To redistribute this file separately, substitute the full license texts
# for the above references.

include(Version.txt)

############################################################################
# Get revision from Git
############################################################################
include(GitGetChangesetID)
git_get_changeset_id(C4REVISION)

############################################################################
# Get year
############################################################################

STRING(TIMESTAMP C4COPYRIGHT_YEAR "%Y")

############################################################################
# Build version strings
############################################################################
SET(C4ENGINEID          "${C4PROJECT_TLD}.${C4PROJECT_DOMAIN}.${C4ENGINENICK}")
set(C4ENGINECAPTION "${C4ENGINENAME}")

set(C4VERSION           "${C4XVER1}.${C4XVER2}")

if(C4VERSIONEXTRA)
	set(C4VERSION "${C4VERSION}-${C4VERSIONEXTRA}")
endif()

if(WIN32)
	set(C4VERSION       "${C4VERSION} win")
elseif(APPLE)
	set(C4VERSION       "${C4VERSION} mac")
elseif(UNIX)
	set(C4VERSION       "${C4VERSION} unix")
else()
	set(C4VERSION       "${C4VERSION} strange")
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/C4Version.h.in ${CMAKE_CURRENT_BINARY_DIR}/C4Version.h ESCAPE_QUOTES)
if(WIN32)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/res/WindowsGamesExplorer.xml.in ${CMAKE_CURRENT_BINARY_DIR}/WindowsGamesExplorer.xml ESCAPE_QUOTES)
endif()
