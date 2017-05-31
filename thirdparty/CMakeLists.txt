cmake_minimum_required (VERSION 3.0)

if (NOT WIN32)
  message("This file builds third-party dependencies of openclonk only on Windows")
  # For other platforms please use the respective package managers
  return()
endif()

include(ExternalProject)

ExternalProject_Add(freetype2
  GIT_REPOSITORY "git://git.savannah.gnu.org/freetype/freetype2.git"
  GIT_TAG "VER-2-7-1"
  CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
             -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
)

ExternalProject_Add(nasm
  URL "http://www.nasm.us/pub/nasm/releasebuilds/2.12.02/win32/nasm-2.12.02-win32.zip"
  URL_HASH SHA256=91b9f784b1286e3de73dabfdc7466c198b96fef7080e00710411119959935809
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/nasm"
  INSTALL_COMMAND ""
)

ExternalProject_Add("libjpeg-turbo"
  DEPENDS nasm
  GIT_REPOSITORY "git://github.com/libjpeg-turbo/libjpeg-turbo.git"
  GIT_TAG "1.5.1"
  CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
             -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
             -DNASM:FILEPATH=${CMAKE_CURRENT_BINARY_DIR}/nasm/nasm
)

ExternalProject_Add(zlib
  GIT_REPOSITORY "git://github.com/madler/zlib.git"
  GIT_TAG "v1.2.11"
  CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
             -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
)

ExternalProject_Add(libpng
  DEPENDS zlib
  GIT_REPOSITORY "git://github.com/glennrp/libpng.git"
  GIT_TAG "v1.6.29"
  CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
             -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
)

ExternalProject_Add(glew
  URL "https://github.com/nigels-com/glew/releases/download/glew-2.0.0/glew-2.0.0-win32.zip"
  URL_HASH SHA256=9c1d00b550df96899dde289cdebfb5eba5b1ae14853ec41eb905c85106fbd3ae
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  BUILD_IN_SOURCE 1
  INSTALL_COMMAND ${CMAKE_COMMAND} "-DINSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}" -P "${CMAKE_CURRENT_SOURCE_DIR}/InstallGLEW.cmake"
)

ExternalProject_Add(OpenAL
  GIT_REPOSITORY "git://github.com/kcat/openal-soft.git"
  GIT_TAG "openal-soft-1.17.2"
  CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
             -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
)

find_package(Git REQUIRED)
ExternalProject_Add(FreeALUT
  DEPENDS OpenAL
  GIT_REPOSITORY "git://github.com/vancegroup/freealut.git"
  GIT_TAG "freealut_1_1_0"
  PATCH_COMMAND ${GIT_EXECUTABLE} apply ${CMAKE_CURRENT_SOURCE_DIR}/FreeALUT.patch
  CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
             -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
             -DOPENAL_LIB_DIR=${CMAKE_INSTALL_PREFIX}/lib
             -DOPENAL_INCLUDE_DIR=${CMAKE_INSTALL_PREFIX}/include
)

ExternalProject_Add(Ogg
  GIT_REPOSITORY "git://github.com/xiph/ogg.git"
  CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
             -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
)

ExternalProject_Add(Vorbis
  GIT_REPOSITORY "git://github.com/xiph/vorbis.git"
  CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
             -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
)
