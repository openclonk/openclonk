dnl Copyright (C) 2000-2002 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Bruno Haible.

dnl Modified for Clonk to not do all that weird stuff

AC_DEFUN([_AX_ICONV_LINK],
[
  dnl Some systems have iconv in libc, some have it in libiconv (OSF/1 and
  dnl those with the standalone portable GNU libiconv installed).

  dnl Search for libiconv and define LIBICONV and INCICONV
  dnl accordingly.

  AC_CACHE_CHECK(for iconv, ax_cv_func_iconv, [
    ax_cv_func_iconv="no, consider installing GNU libiconv"
    ax_cv_lib_iconv=no
    LIBICONV=""
    AC_TRY_LINK([#include <stdlib.h>
#include <iconv.h>],
      [iconv_t cd = iconv_open("","");
       iconv(cd,NULL,NULL,NULL,NULL);
       iconv_close(cd);],
      ax_cv_func_iconv=yes)
    if test "$ax_cv_func_iconv" != yes; then
      ax_save_LIBS="$LIBS"
      LIBS="$LIBS -liconv"
      AC_TRY_LINK([#include <stdlib.h>
#include <iconv.h>],
        [iconv_t cd = iconv_open("","");
         iconv(cd,NULL,NULL,NULL,NULL);
         iconv_close(cd);],
        ax_cv_lib_iconv=yes
        ax_cv_func_iconv=yes
        LIBICONV=-liconv)
      LIBS="$ax_save_LIBS"
    fi
  ])
  if test "$ax_cv_func_iconv" = yes; then
    AC_DEFINE(HAVE_ICONV, 1, [Define if you have the iconv() function.])
  fi
  if test "$ax_cv_lib_iconv" = yes; then
    AC_MSG_CHECKING([how to link with libiconv])
    AC_MSG_RESULT([-liconv])
  fi
  AC_SUBST(LIBICONV)
])

AC_DEFUN([AX_ICONV],
[
  _AX_ICONV_LINK
  if test "$ax_cv_func_iconv" = yes; then
    AC_MSG_CHECKING([for iconv declaration])
    AC_CACHE_VAL(ax_cv_proto_iconv, [
      AC_TRY_COMPILE([
#include <stdlib.h>
#include <iconv.h>
extern
#ifdef __cplusplus
"C"
#endif
#if defined(__STDC__) || defined(__cplusplus)
size_t iconv (iconv_t cd, char * *inbuf, size_t *inbytesleft, char * *outbuf, size_t *outbytesleft);
#else
size_t iconv();
#endif
], [], ax_cv_proto_iconv_arg1="", ax_cv_proto_iconv_arg1="const")])
    AC_MSG_RESULT([extern size_t iconv (iconv_t cd, $ax_cv_proto_iconv_arg1 char * * inbuf, size_t * inbytesleft, char * * outbuf, size_t * outbytesleft);])
    AC_DEFINE_UNQUOTED(ICONV_CONST, $ax_cv_proto_iconv_arg1,
      [Define as const if the declaration of iconv() needs const.])
  fi
])
