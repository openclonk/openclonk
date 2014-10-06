# - Try to find Iconv 
# Once done this will define 
# 
#  ICONV_FOUND - system has Iconv 
#  ICONV_INCLUDE_DIR - the Iconv include directory 
#  ICONV_LIBRARIES - Link these to use Iconv 
#  ICONV_IS_CONST - the second argument for iconv() is const
# 
# This was mostly borrowed from Strigi.  LyX also has a (quite
# different) FindICONV: the one in LyX does not check
# second_argument_is_const, but seems to have more fleshed-out support
# for WIN32.  There may need to be some merging done.
# Tim Holy, 2008-05-07

include(CheckCXXSourceCompiles)

IF (ICONV_INCLUDE_DIR AND ICONV_LIBRARIES)
  # Already in cache, be silent
  SET(ICONV_FIND_QUIETLY TRUE)
ENDIF (ICONV_INCLUDE_DIR AND ICONV_LIBRARIES)

FIND_PATH(ICONV_INCLUDE_DIR iconv.h
  /Developer/SDKs/MacOSX10.4u.sdk/usr/include/iconv.h
  /Developer/SDKs/MacOSX10.5.sdk/usr/include/iconv.h
  /usr/include
)

FIND_LIBRARY(ICONV_LIBRARIES NAMES iconv libiconv libiconv2 c
  PATHS
  /Developer/SDKs/MacOSX10.4u.sdk/usr/lib
  /Developer/SDKs/MacOSX10.5.sdk/usr/lib
 /usr/lib
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ICONV REQUIRED_VARS ICONV_LIBRARIES ICONV_INCLUDE_DIR)

IF(ICONV_FOUND)
  set(CMAKE_REQUIRED_INCLUDES ${ICONV_INCLUDE_DIR})
  set(CMAKE_REQUIRED_LIBRARIES ${ICONV_LIBRARIES})
  check_cxx_source_compiles("
  #include <iconv.h>
  int main(){
    iconv_t conv = 0;
    const char* in = 0;
    size_t ilen = 0;
    char* out = 0;
    size_t olen = 0;
    iconv(conv, &in, &ilen, &out, &olen);
    return 0;
  }
" ICONV_IS_CONST )
  set(CMAKE_REQUIRED_INCLUDES)
  set(CMAKE_REQUIRED_LIBRARIES)
ENDIF(ICONV_FOUND)

MARK_AS_ADVANCED(
  ICONV_INCLUDE_DIR
  ICONV_LIBRARIES
  ICONV_IS_CONST
)

