AC_INIT([fib],[1.0.0])
AC_CONFIG_SRCDIR([])
AC_CONFIG_AUX_DIR(config)
AM_CONFIG_HEADER(config.h)
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign subdir-objects parallel-tests])
AC_LANG_C
AC_LANG([C++])
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_HEADER_STDC
AM_PATH_PYTHON
AM_PATH_PYTHON3
AX_ADD_AM_MACRO_STATIC([])

AC_ARG_ENABLE(debug,
[  --enable-debug       Compile with debugging flags],
[case "${enableval}" in
        yes) debug=true ;;
        no)  debug=false ;;
        *) AC_MSG_ERROR(bad value ${enableval} for --enable-debug) ;;
esac],[debug=false])
AM_CONDITIONAL(DEBUG, test x$debug = xtrue)
if test x$CONFIGURED_ARCH = xarmhf && test x$CROSS_BUILD_ENVIRON = xy; then
        AM_CONDITIONAL(ARCH64, false)
else
        AM_CONDITIONAL(ARCH64, test `getconf LONG_BIT` = "64")
fi


CFLAGS_COMMON=""
CFLAGS_COMMON+=" -ansi"
CFLAGS_COMMON+=" -fPIC"
CFLAGS_COMMON+=" -std=c++14"
CFLAGS_COMMON+=" -Wall"
CFLAGS_COMMON+=" -Wcast-align"
CFLAGS_COMMON+=" -Wcast-qual"
CFLAGS_COMMON+=" -Wconversion"
CFLAGS_COMMON+=" -Wdisabled-optimization"
CFLAGS_COMMON+=" -Werror"
CFLAGS_COMMON+=" -Wextra"
CFLAGS_COMMON+=" -Wfloat-equal"
CFLAGS_COMMON+=" -Wformat=2"
CFLAGS_COMMON+=" -Wformat-nonliteral"
CFLAGS_COMMON+=" -Wformat-security"
CFLAGS_COMMON+=" -Wformat-y2k"
CFLAGS_COMMON+=" -Wimport"
CFLAGS_COMMON+=" -Winit-self"
CFLAGS_COMMON+=" -Winvalid-pch"
CFLAGS_COMMON+=" -Wlong-long"
CFLAGS_COMMON+=" -Wmissing-field-initializers"
CFLAGS_COMMON+=" -Wmissing-format-attribute"
CFLAGS_COMMON+=" -Wmissing-include-dirs"
CFLAGS_COMMON+=" -Wmissing-noreturn"
CFLAGS_COMMON+=" -Wno-aggregate-return"
CFLAGS_COMMON+=" -Wno-padded"
CFLAGS_COMMON+=" -Wno-switch-enum"
CFLAGS_COMMON+=" -Wno-unused-parameter"
CFLAGS_COMMON+=" -Wpacked"
CFLAGS_COMMON+=" -Wpointer-arith"
CFLAGS_COMMON+=" -Wredundant-decls"
CFLAGS_COMMON+=" -Wshadow"
CFLAGS_COMMON+=" -Wstack-protector"
CFLAGS_COMMON+=" -Wstrict-aliasing=3"
CFLAGS_COMMON+=" -Wswitch"
CFLAGS_COMMON+=" -Wswitch-default"
CFLAGS_COMMON+=" -Wunreachable-code"
CFLAGS_COMMON+=" -Wunused"
CFLAGS_COMMON+=" -Wvariadic-macros"
CFLAGS_COMMON+=" -Wno-write-strings"
CFLAGS_COMMON+=" -Wno-missing-format-attribute"
CFLAGS_COMMON+=" -Wno-long-long"
CFLAGS_COMMON+=" -fstack-protector-strong"

AC_SUBST(CFLAGS_COMMON)

AC_CONFIG_FILES([
    Makefile
])

AC_OUTPUT
