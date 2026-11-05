AC_INIT([sonic-pac],[1.0.0])
AC_CONFIG_SRCDIR([])
AC_CONFIG_AUX_DIR(config)
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_LANG_C
AC_LANG([C++])
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_HEADER_STDC

AC_CHECK_LIB([hiredis], [redisConnect],,
    AC_MSG_ERROR([libhiredis is not installed.]))

#AC_CHECK_LIB([nl-genl-3], [genl_connect])

AC_ARG_ENABLE(debug,
[  --enable-debug      Compile with debugging flags],
[case "${enableval}" in
	yes) debug=true ;;
	no)  debug=false ;;
	*) AC_MSG_ERROR(bad value ${enableval} for --enable-debug) ;;
esac],[debug=false])
AM_CONDITIONAL(DEBUG, test x$debug = xtrue)

CFLAGS_COMMON="-std=c++11 -Wall -fPIC -Wno-write-strings -I/usr/include/libnl3 -I/usr/include/swss -I/usr/include/security"

CFLAGS_COMMON+=" -Werror"
CFLAGS_COMMON+=" -Wno-reorder"
#CFLAGS_COMMON+=" -Wno-pointer-sign"
CFLAGS_COMMON+=" -Wno-unused-but-set-variable"
CFLAGS_COMMON+=" -Wcast-align"
CFLAGS_COMMON+=" -Wcast-qual"
CFLAGS_COMMON+=" -Wdisabled-optimization"
CFLAGS_COMMON+=" -Wextra"
CFLAGS_COMMON+=" -Wfloat-equal"
CFLAGS_COMMON+=" -Wimport"
CFLAGS_COMMON+=" -Winit-self"
CFLAGS_COMMON+=" -Winvalid-pch"
CFLAGS_COMMON+=" -Wlong-long"
CFLAGS_COMMON+=" -Wmissing-field-initializers"
CFLAGS_COMMON+=" -Wno-aggregate-return"
CFLAGS_COMMON+=" -Wno-padded"
CFLAGS_COMMON+=" -Wno-switch-enum"
CFLAGS_COMMON+=" -Wno-unused-parameter"
CFLAGS_COMMON+=" -Wpacked"
CFLAGS_COMMON+=" -Wpointer-arith"
CFLAGS_COMMON+=" -Wredundant-decls"
CFLAGS_COMMON+=" -Wstack-protector"
CFLAGS_COMMON+=" -Wstrict-aliasing=3"
CFLAGS_COMMON+=" -Wswitch"
CFLAGS_COMMON+=" -Wswitch-default"
CFLAGS_COMMON+=" -Wunreachable-code"
CFLAGS_COMMON+=" -Wunused"
CFLAGS_COMMON+=" -Wvariadic-macros"
CFLAGS_COMMON+=" -Wno-switch-default"
CFLAGS_COMMON+=" -Wno-long-long"
CFLAGS_COMMON+=" -Wno-redundant-decls"
#CFLAGS_COMMON+=" -Wno-misleading-indentation"
CFLAGS_COMMON+=" -save-temps"

AC_SUBST(CFLAGS_COMMON)

AC_CONFIG_FILES([
    fpinfra/Makefile
    paccfg/Makefile
    pacoper/Makefile
    authmgr/Makefile
    pacmgr/Makefile
    hostapdmgr/Makefile
    mab/Makefile
    mabmgr/Makefile
    Makefile
])

AC_OUTPUT
