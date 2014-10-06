# - Check if the files can be included
#
# CHECK_INCLUDE_FILES_CXX(INCLUDE VARIABLE)
#
#  INCLUDE  - list of files to include
#  VARIABLE - variable to return result
#
# The following variables may be set before calling this macro to
# modify the way the check is run:
#
#  CMAKE_REQUIRED_FLAGS = string of compile command line flags
#  CMAKE_REQUIRED_DEFINITIONS = list of macros to define (-DFOO=bar)
#  CMAKE_REQUIRED_INCLUDES = list of include directories

#=============================================================================
# Copyright 2003-2012 Kitware, Inc.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# 
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
# 
# * Neither the names of Kitware, Inc., the Insight Software Consortium,
#   nor the names of their contributors may be used to endorse or promote
#   products derived from this software without specific prior written
#   permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#=============================================================================

macro(CHECK_INCLUDE_FILES_CXX INCLUDE VARIABLE)
  if("${VARIABLE}" MATCHES "^${VARIABLE}$")
    set(CMAKE_CONFIGURABLE_FILE_CONTENT "/* */\n")
    if(CMAKE_REQUIRED_INCLUDES)
      set(CHECK_INCLUDE_FILES_INCLUDE_DIRS "-DINCLUDE_DIRECTORIES=${CMAKE_REQUIRED_INCLUDES}")
    else()
      set(CHECK_INCLUDE_FILES_INCLUDE_DIRS)
    endif()
    set(CHECK_INCLUDE_FILES_CONTENT "/* */\n")
    set(MACRO_CHECK_INCLUDE_FILES_FLAGS ${CMAKE_REQUIRED_FLAGS})
    foreach(FILE ${INCLUDE})
      set(CMAKE_CONFIGURABLE_FILE_CONTENT
        "${CMAKE_CONFIGURABLE_FILE_CONTENT}#include <${FILE}>\n")
    endforeach()
    set(CMAKE_CONFIGURABLE_FILE_CONTENT
      "${CMAKE_CONFIGURABLE_FILE_CONTENT}\n\nint main(){return 0;}\n")
    configure_file("${CMAKE_ROOT}/Modules/CMakeConfigurableFile.in"
      "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/CheckIncludeFiles.cxx" @ONLY IMMEDIATE)

    set(_INCLUDE ${INCLUDE}) # remove empty elements
    if("${_INCLUDE}" MATCHES "^([^;]+);.+;([^;]+)$")
      list(LENGTH _INCLUDE _INCLUDE_LEN)
      set(_description "${_INCLUDE_LEN} C++ include files ${CMAKE_MATCH_1}, ..., ${CMAKE_MATCH_2}")
    elseif("${_INCLUDE}" MATCHES "^([^;]+);([^;]+)$")
      set(_description "C++ include files ${CMAKE_MATCH_1}, ${CMAKE_MATCH_2}")
    else()
      set(_description "C++ include file ${_INCLUDE}")
    endif()

    message(STATUS "Looking for ${_description}")
    try_compile(${VARIABLE}
      ${CMAKE_BINARY_DIR}
      ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/CheckIncludeFiles.cxx
      COMPILE_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS}
      CMAKE_FLAGS
      -DCOMPILE_DEFINITIONS:STRING=${MACRO_CHECK_INCLUDE_FILES_FLAGS}
      "${CHECK_INCLUDE_FILES_INCLUDE_DIRS}"
      OUTPUT_VARIABLE OUTPUT)
    if(${VARIABLE})
      message(STATUS "Looking for ${_description} - found")
      set(${VARIABLE} 1 CACHE INTERNAL "Have include ${INCLUDE}")
      file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
        "Determining if C++ files ${INCLUDE} "
        "exist passed with the following output:\n"
        "${OUTPUT}\n\n")
    else()
      message(STATUS "Looking for ${_description} - not found")
      set(${VARIABLE} "" CACHE INTERNAL "Have includes ${INCLUDE}")
      file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
        "Determining if C++ files ${INCLUDE} "
        "exist failed with the following output:\n"
        "${OUTPUT}\nSource:\n${CMAKE_CONFIGURABLE_FILE_CONTENT}\n")
    endif()
  endif()
endmacro()
