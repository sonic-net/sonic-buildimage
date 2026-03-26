## ------------------------                                 -*- Autoconf -*-
## Python 3 file handling, adapted from:
## Python file handling
## From Andrew Dalke
## Updated by James Henstridge
## ------------------------
# Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
# Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# AM_PATH_PYTHON3([MINIMUM-VERSION], [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
# ---------------------------------------------------------------------------
# Adds support for distributing Python modules and packages.  To
# install modules, copy them to $(pythondir), using the python_PYTHON
# automake variable.  To install a package with the same name as the
# automake package, install to $(pkgpythondir), or use the
# pkgpython_PYTHON automake variable.
#
# The variables $(pyexecdir) and $(pkgpyexecdir) are provided as
# locations to install python extension modules (shared libraries).
# Another macro is required to find the appropriate flags to compile
# extension modules.
#
# If your package is configured with a different prefix to python,
# users will have to add the install directory to the PYTHONPATH
# environment variable, or create a .pth file (see the python
# documentation for details).
#
# If the MINIMUM-VERSION argument is passed, AM_PATH_PYTHON will
# cause an error if the version of python installed on the system
# doesn't meet the requirement.  MINIMUM-VERSION should consist of
# numbers and dots only.
AC_DEFUN([AM_PATH_PYTHON3],
 [
  dnl Find a Python 3 interpreter
  m4_define_default([_AM_PYTHON3_INTERPRETER_LIST],
                    [python3 python3.0 python3.1])

  m4_if([$1],[],[
    dnl No version check is needed.
    # Find any Python 3 interpreter.
    if test -z "$PYTHON3"; then
      AC_PATH_PROGS([PYTHON3], _AM_PYTHON3_INTERPRETER_LIST, :)
    fi
    am_display_PYTHON3=python3
  ], [
    dnl A version check is needed.
    if test -n "$PYTHON3"; then
      # If the user set $PYTHON3, use it and don't search something else.
      AC_MSG_CHECKING([whether $PYTHON3 version >= $1])
      AM_PYTHON3_CHECK_VERSION([$PYTHON3], [$1],
			      [AC_MSG_RESULT(yes)],
			      [AC_MSG_ERROR(too old)])
      am_display_PYTHON3=$PYTHON3
    else
      # Otherwise, try each interpreter until we find one that satisfies
      # VERSION.
      AC_CACHE_CHECK([for a Python3 interpreter with version >= $1],
	[am_cv_pathless_PYTHON3],[
	for am_cv_pathless_PYTHON3 in _AM_PYTHON3_INTERPRETER_LIST none; do
	  test "$am_cv_pathless_PYTHON3" = none && break
	  AM_PYTHON3_CHECK_VERSION([$am_cv_pathless_PYTHON3], [$1], [break])
	done])
      # Set $PYTHON3 to the absolute path of $am_cv_pathless_PYTHON3.
      if test "$am_cv_pathless_PYTHON3" = none; then
	PYTHON3=:
      else
        AC_PATH_PROG([PYTHON3], [$am_cv_pathless_PYTHON3])
      fi
      am_display_PYTHON3=$am_cv_pathless_PYTHON3
    fi
  ])

  if test "$PYTHON3" = :; then
  dnl Run any user-specified action, or abort.
    m4_default([$3], [AC_MSG_ERROR([no suitable Python3 interpreter found])])
  else

  dnl Query Python3 for its version number.  Getting [:3] seems to be
  dnl the best way to do this; it's what "site.py" does in the standard
  dnl library.

  AC_CACHE_CHECK([for $am_display_PYTHON3 version], [am_cv_python3_version],
    [am_cv_python3_version=`$PYTHON3 -c "import sys; print ('%u.%u' % sys.version_info[[:2]])"`])
  AC_SUBST([PYTHON3_VERSION], [$am_cv_python3_version])

  dnl Use the values of $prefix and $exec_prefix for the corresponding
  dnl values of PYTHON3_PREFIX and PYTHON3_EXEC_PREFIX.  These are made
  dnl distinct variables so they can be overridden if need be.  However,
  dnl general consensus is that you shouldn't need this ability.

  AC_SUBST([PYTHON3_PREFIX], ['${prefix}'])
  AC_SUBST([PYTHON3_EXEC_PREFIX], ['${exec_prefix}'])

  dnl At times (like when building shared libraries) you may want
  dnl to know which OS platform Python3 thinks this is.

  AC_CACHE_CHECK([for $am_display_PYTHON3 platform], [am_cv_python3_platform],
    [am_cv_python3_platform=`$PYTHON3 -c "import sys; sys.stdout.write(sys.platform)"`])
  AC_SUBST([PYTHON3_PLATFORM], [$am_cv_python3_platform])

  AC_CACHE_CHECK([for $am_display_PYTHON3 bldlibrary], [am_cv_python3_bldlibrary],
    [am_cv_python3_bldlibrary=`$PYTHON3 -c "import sys; import sysconfig; sys.stdout.write(sysconfig.get_config_var('BLDLIBRARY'))"`])
  AC_SUBST([PYTHON3_BLDLIBRARY], [$am_cv_python3_bldlibrary])

  dnl Set up 4 directories:

  dnl python3dir -- where to install python3 scripts.  This is the
  dnl   site-packages directory, not the python3 standard library
  dnl   directory like in previous automake betas.  This behavior
  dnl   is more consistent with lispdir.m4 for example.
  dnl Query distutils for this directory.  distutils does not exist in
  dnl Python3 1.5, so we fall back to the hardcoded directory if it
  dnl doesn't work.
  AC_CACHE_CHECK([for $am_display_PYTHON3 script directory],
    [am_cv_python3_python3dir],
    [if test "x$prefix" = xNONE
     then
       am_py_prefix=$ac_default_prefix
     else
       am_py_prefix=$prefix
     fi
     am_cv_python3_python3dir=`$PYTHON3 -c "import sys; from distutils import sysconfig; sys.stdout.write(sysconfig.get_python_lib(0,0,prefix='$am_py_prefix'))" 2>/dev/null ||
     echo "$PYTHON3_PREFIX/lib/python$PYTHON3_VERSION/site-packages"`
     case $am_cv_python3_python3dir in
     $am_py_prefix*)
       am__strip_prefix=`echo "$am_py_prefix" | sed 's|.|.|g'`
       am_cv_python3_python3dir=`echo "$am_cv_python3_python3dir" | sed "s,^$am__strip_prefix,$PYTHON3_PREFIX,"`
       ;;
     esac
    ])
  AC_SUBST([python3dir], [$am_cv_python3_python3dir])

  dnl pkgpython3dir -- $PACKAGE directory under python3dir.  Was
  dnl   PYTHON3_SITE_PACKAGE in previous betas, but this naming is
  dnl   more consistent with the rest of automake.

  AC_SUBST([pkgpython3dir], [\${python3dir}/$PACKAGE])

  dnl pyexecdir -- directory for installing python3 extension modules
  dnl   (shared libraries)
  dnl Query distutils for this directory.  distutils does not exist in
  dnl Python3 1.5, so we fall back to the hardcoded directory if it
  dnl doesn't work.
  AC_CACHE_CHECK([for $am_display_PYTHON3 extension module directory],
    [am_cv_python3_pyexecdir],
    [if test "x$exec_prefix" = xNONE
     then
       am_py_exec_prefix=$am_py_prefix
     else
       am_py_exec_prefix=$exec_prefix
     fi
     am_cv_python3_pyexecdir=`$PYTHON3 -c "import sys; from distutils import sysconfig; sys.stdout.write(sysconfig.get_python_lib(1,0,prefix='$am_py_exec_prefix'))" 2>/dev/null ||
     echo "$PYTHON3_EXEC_PREFIX/lib/python$PYTHON3_VERSION/site-packages"`
     case $am_cv_python3_pyexecdir in
     $am_py_exec_prefix*)
       am__strip_prefix=`echo "$am_py_exec_prefix" | sed 's|.|.|g'`
       am_cv_python3_pyexecdir=`echo "$am_cv_python3_pyexecdir" | sed "s,^$am__strip_prefix,$PYTHON3_EXEC_PREFIX,"`
       ;;
     esac
    ])
  AC_SUBST([py3execdir], [$am_cv_python3_pyexecdir])

  dnl pkgpy3execdir -- $(py3execdir)/$(PACKAGE)

  AC_SUBST([pkgpy3execdir], [\${py3execdir}/$PACKAGE])

  dnl Run any user-specified action.
  $2
  fi

])


# AM_PYTHON3_CHECK_VERSION(PROG, VERSION, [ACTION-IF-TRUE], [ACTION-IF-FALSE])
# ---------------------------------------------------------------------------
# Run ACTION-IF-TRUE if the Python3 interpreter PROG has version >= VERSION.
# Run ACTION-IF-FALSE otherwise.
# This test uses sys.hexversion instead of the string equivalent (first
# word of sys.version), in order to cope with versions such as 2.2c1.
# This supports Python3 2.0 or higher. (2.0 was released on October 16, 2000).
AC_DEFUN([AM_PYTHON3_CHECK_VERSION],
 [prog="import sys
# split strings by '.' and convert to numeric.  Append some zeros
# because we need at least 4 digits for the hex conversion.
# map returns an iterator in Python3 3.0 and a list in 2.x
minver = list(map(int, '$2'.split('.'))) + [[0, 0, 0]]
minverhex = 0
# xrange is not present in Python3yy 3.0 and range returns an iterator
for i in list(range(0, 4)): minverhex = (minverhex << 8) + minver[[i]]
sys.exit(sys.hexversion < minverhex)"
  AS_IF([AM_RUN_LOG([$1 -c "$prog"])], [$3], [$4])])
