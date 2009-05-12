dnl Copyright (C) 2009 Günther Brammer

AC_DEFUN([AX_PROG_CXX_REFTOTEMP],
[
  AC_LANG_ASSERT([C++])
  AC_CACHE_CHECK([wether the C++ compiler is friendly], [ax_cv_reftotemp], [
    AC_COMPILE_IFELSE([
struct Foo {
    operator Foo & () { return *this; }
};
#if defined(__GXX_EXPERIMENTAL_CXX0X__) || (defined(__GNUC__) && ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)))
void frobnicate(Foo &&) { }
#else
void frobnicate(Foo &) { }
#endif
int main () {
    frobnicate (Foo());
}
], [ax_cv_reftotemp=yes], [ax_cv_reftotemp=no])])
  if test $ax_cv_reftotemp = no; then
    AC_MSG_ERROR([The C++ compiler won't be able to compile Clonk. Try CXX='g++ -std=gnu++0x' or CXX='g++-4.1'.])
  fi[]dnl
])# AX_PROG_CXX_REFTOTEMP
