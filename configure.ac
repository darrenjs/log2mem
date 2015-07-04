
#========== Initialisation ==========

AC_INIT([log2mem], [1.1], [log2mem@darrenjs.net], [log2mem], [ ] )
AC_PREREQ([2.59])

# This macro causes the environment variables, $host, $build and $target to be
# defined.
AC_CANONICAL_SYSTEM

# Not: the -Wall here is for automake errors; is unrelated to gcc errors
AM_INIT_AUTOMAKE([1.10 no-define -Wall])
AM_PROG_AR

AC_CONFIG_HEADERS([config.h])
AC_PROG_CXX
AC_CONFIG_MACRO_DIR([m4])


# initialise libtool support

LT_PREREQ([2.4.2]) # not 'technically' required, but good practice.

# Note: we initialise libtool with shared-libraries disabled.  This makes it
# easier to quickly use log2mem; the user doesn't need to worry about altering
# the LD_LIBRARY_PATH at runtime to pick up liblog2mem.so
LT_INIT([disable-shared])

#========== Checks for programs ==========

AC_PROG_CXX
AC_PROG_CC
AM_PROG_LIBTOOL
AC_PROG_INSTALL


#========== Check for third party libraries ==========


dnl PKG_CHECK_MODULES(QT4, [QtCore QtGui >= 4.4.0], [
dnl   AC_PATH_PROGS(MOC, [moc-qt4 moc], moc,`eval $PKG_CONFIG --variable=exec_prefix QtCore`/bin)
dnl   AC_PATH_PROG(RCC, rcc, rcc,`eval $PKG_CONFIG --variable=exec_prefix QtCore`/bin)
dnl   AC_PATH_PROGS(UIC, [uic-qt4 uic], uic,`eval $PKG_CONFIG --variable=exec_prefix QtCore`/bin)
dnl ], [
dnl   AC_MSG_ERROR([Qt 4 library not found])
dnl ])


dnl PKG_CHECK_MODULES([QT], [QtCore QtGui Qt3D
dnl ], [
dnl ])

#========== Checks for header files ==========

AC_HEADER_STDC
AC_CHECK_HEADERS([errno.h fcntl.h stdint.h stdio.h stdlib.h string.h time.h unistd.h sys/time.h sys/stat.h sys/syscall.h sys/mman.h sys/stat.h sys/types.h ])

AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE


#========== Check functions ==========


AC_LINK_IFELSE([AC_LANG_SOURCE([
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdlib.h>
int main(void) {
	char *c;
	return __sync_add_and_fetch(c, 1);
}])],[has___sync_add_and_fetch=yes])
if test "x$has___sync_add_and_fetch" = xyes; then
	AC_DEFINE(HAVE___SYNC_ADD_AND_FETCH,1,[Define to 1 if you have the __sync_add_and_fetch gcc builtin function])
	AC_MSG_RESULT(yes)
else
	AC_MSG_RESULT(no)
fi




#========== User options ==========

export DIRS="liblog2mem dumptool test"
extrabuild=
dnl QT option
AC_ARG_ENABLE([gui],
    AS_HELP_STRING([--enable-gui], [Enable QT gui]))

AS_IF([test "x$enable_gui" = "xyes"], [

 DIRS="$DIRS qtgui"
])
#========== Generation ==========

AC_SUBST(DIRS)

# List the files that will be generated. These are mainly makefiles, which
# automake will generate from the corresponding Makefile.am
AC_CONFIG_FILES([
Makefile
liblog2mem/Makefile
dumptool/Makefile
examples/interact/Makefile
qtgui/Makefile
])

# Trigger the generation of our makefiles
AC_OUTPUT


