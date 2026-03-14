bin_PROGRAMS = mclagdctl

if DEBUG
DBGFLAGS = -ggdb -DDEBUG
else
DBGFLAGS = -g -DNDEBUG
endif

mclagdctl_SOURCES = mclagdctl.c
mclagdctl_CFLAGS = $(DBGFLAGS) $(AM_CFLAGS) $(CFLAGS_COMMON)
