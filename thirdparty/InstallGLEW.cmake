#!/usr/bin/env cmake -P

# The pre-built glew binaries for Windows are extracted in a different
# location from where FindGLEW.cmake expects them. This script copies
# them to the right location.

if(NOT WIN32)
  message(FATAL_ERROR "This script is useful only for Windows. Please use the native package manager or the source build for other platforms")
endif()

# set PLATFORM=x64 to use the 64-bit binaries instead of the default 32-bit
if(NOT DEFINED ENV{PLATFORM})
  set(ENV{PLATFORM} Win32)
endif()

file(COPY "bin/Release/$ENV{PLATFORM}/glew32.dll" DESTINATION "${INSTALL_PREFIX}/bin")
file(COPY "include/GL" DESTINATION "${INSTALL_PREFIX}/include")
file(COPY "lib/Release/$ENV{PLATFORM}/glew32.lib" DESTINATION "${INSTALL_PREFIX}/lib")
