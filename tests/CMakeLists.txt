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

cmake_minimum_required (VERSION 2.6.0)
project(openclonk_unittest)

# used by c4script and by the unit tests
set(C4SCRIPT_SOURCES
	../include/c4script/c4script.h
	../src/lib/C4SimpleLog.cpp
	../src/C4Include.cpp
	../src/script/C4ScriptStandalone.cpp
)

if (UNIX)
    list(APPEND CMAKE_PREFIX_PATH /usr/src)
endif()

# GTest
find_path(GTEST_INCLUDE_DIR gtest/gtest.h)
find_path(GTEST_SOURCE_DIR gtest/CMakeLists.txt)

if (GTEST_INCLUDE_DIR AND GTEST_SOURCE_DIR)
    add_subdirectory(${GTEST_SOURCE_DIR}/gtest gtest)
    include_directories(${GTEST_INCLUDE_DIR})
    set(GTEST_LIBRARIES gtest gtest_main)
    AUX_SOURCE_DIRECTORY(. TESTS_SOURCES)
    add_executable(tests ${TESTS_SOURCES} ${C4SCRIPT_SOURCES})
    target_link_libraries(tests ${GTEST_LIBRARIES} libc4script libmisc)
    if(UNIX AND NOT APPLE)
	    target_link_libraries(tests rt)
    endif()
    if(HAVE_PTHREAD)
	    target_link_libraries(tests pthread)
    endif()
else()
    if (NOT GTEST_INCLUDE_DIR)
        message(STATUS "Could NOT find Gtest (missing gtest/gtest.h)")
    endif()
    if(NOT GTEST_SOURCE_DIR)
        message(STATUS "Could NOT find Gtest (gtest/CMakeLists.txt)")
    endif()
endif()
