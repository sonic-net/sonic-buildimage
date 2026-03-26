lib_LTLIBRARIES =
bin_PROGRAMS =

if DEBUG
DBGFLAGS = -ggdb -DDEBUG -gdwarf-5
else
DBGFLAGS = -g -DNDEBUG
endif

CLEANFILES =

include src/Makefile.am
include tests/Makefile.am

ACLOCAL_AMFLAGS = -I m4


EXTRA_DIST += scripts/render_schema.py
