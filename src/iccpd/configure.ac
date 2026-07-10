AC_INIT([iccpd],[0.5])
AC_CONFIG_SRCDIR([])
AC_CONFIG_AUX_DIR(config)
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE([foreign])
AC_LANG_C
AC_PROG_CC
AC_PROG_LIBTOOL
AC_HEADER_STDC

AC_ARG_ENABLE(debug,
[  --enable-debug      Compile with debugging flags],
[case "${enableval}" in
	yes) debug=true ;;
	no)  debug=false ;;
	*) AC_MSG_ERROR(bad value ${enableval} for --enable-debug) ;;
esac],[debug=false])
AM_CONDITIONAL(DEBUG, test x$debug = xtrue)

CPPFLAGS="-D_FORTIFY_SOURCE=2"

CFLAGS_COMMON="-Wno-unused-result"

AC_SUBST(CFLAGS_COMMON)

AC_CONFIG_FILES([
    Makefile
    src/Makefile
    src/mclagdctl/Makefile
])

AC_OUTPUT
