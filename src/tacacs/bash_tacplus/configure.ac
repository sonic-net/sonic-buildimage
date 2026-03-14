dnl
dnl File:        configure.in
dnl Revision:    $Id: configure.ac,v 1.0 2021/08/24 12:04:29 liuh@microsoft.com Exp $
dnl Created:     2021/08/24
dnl Author:      Liu Hua <liuh@microsoft.com>
dnl
dnl Process this file with autoconf to produce a configure script
dnl You need autoconf 2.59 or better!
dnl
dnl ---------------------------------------------------------------------------

AC_PREREQ(2.59)
AC_COPYRIGHT([
See the included file: COPYING for copyright information.
])
AC_INIT(bash_tacplus, 1.0.0, [liuh@microsoft.com])

AC_CONFIG_AUX_DIR(config)
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([bash_tacplus.c])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([config])

dnl --------------------------------------------------------------------
dnl Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_ENABLE_SHARED
AC_DISABLE_STATIC
AM_PROG_LIBTOOL

dnl --------------------------------------------------------------------
dnl Checks for libraries.
AC_CHECK_LIB(tac, tac_connect)
AC_CHECK_LIB(tacsupport, parse_config_file)

dnl --------------------------------------------------------------------
dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stdlib.h string.h strings.h sys/socket.h sys/time.h ])
AC_CHECK_HEADER([libtac/libtac.h], [], [AC_MSG_ERROR([TAC libraries missing. ])] )
AC_CHECK_HEADER([libtac/support.h], [], [AC_MSG_ERROR([TAC support libraries missing. ])] )

dnl --------------------------------------------------------------------
dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_TIME

dnl --------------------------------------------------------------------
dnl Checks for library functions.
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([bzero gethostbyname gettimeofday inet_ntoa select socket logwtmp getrandom])

dnl --------------------------------------------------------------------
dnl Switch for plugin module dir
AC_ARG_ENABLE([plugindir], [AS_HELP_STRING([--enable-plugindir],
              [Location to install the pam module ($libdir/security)])],
              [plugindir=$enableval], [plugindir=$libdir/security])
AC_SUBST(plugindir)

dnl --------------------------------------------------------------------
dnl Generate made files
AC_CONFIG_FILES([Makefile
                    unittest/Makefile])
AC_OUTPUT
