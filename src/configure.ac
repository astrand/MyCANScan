AC_INIT(mycanscan, 10.0)

AC_CONFIG_SRCDIR([canvis/main.c])

AC_PROG_CC
if test "$GCC" = yes; then
    CFLAGS="$CFLAGS -Wall"
fi

AC_PROG_INSTALL
AC_LANG_C
AC_HEADER_STDC
AC_PATH_XTRA
AC_CHECK_TOOL(STRIP, strip, :)

AC_OUTPUT(Makefile)

