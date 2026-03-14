###########################################################################
##
## File:        ./Makefile.am
## Versions:    $Id: Makefile.am,v 1.0 2021/08/24 12:04:29 liuh@microsoft.com Exp $
## Created:     2021/08/24
##
###########################################################################

ACLOCAL_AMFLAGS = -I config
AUTOMAKE_OPTIONS = subdir-objects

moduledir = @plugindir@
module_LTLIBRARIES = bash_tacplus.la
bash_tacplus_la_SOURCES = bash_tacplus.h \
bash_tacplus.c
bash_tacplus_la_CFLAGS = $(AM_CFLAGS) -I $(top_srcdir)/libtac/include
bash_tacplus_la_LDFLAGS = -module -avoid-version

EXTRA_DIST = bash_tacplus.spec

MAINTAINERCLEANFILES = Makefile.in config.h.in configure aclocal.m4 \
                       config/config.guess  config/config.sub  config/depcomp \
                       config/install-sh config/ltmain.sh config/missing

pkgconfigdir = $(libdir)/pkgconfig

SUBDIRS = unittest
