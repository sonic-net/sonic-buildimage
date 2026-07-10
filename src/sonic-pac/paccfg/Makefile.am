INCLUDES =  -I $(top_srcdir)/fpinfra/inc -I $(top_srcdir)/authmgr/common -I $(top_srcdir)/authmgr/mapping/include  -I $(top_srcdir)/authmgr/protocol/include

lib_LTLIBRARIES = libpaccfg.la

if DEBUG
DBGFLAGS = -ggdb -DDEBUG
else
DBGFLAGS = -g -DNDEBUG
endif

AM_CPPFLAGS = $(DBGFLAGS) $(AM_CFLAGS) $(CFLAGS_COMMON) $(SONIC_COMMON_CFLAGS)

libpaccfg_la_SOURCES = $(top_srcdir)/paccfg/pac_cfg_authmgr.cpp $(top_srcdir)/paccfg/pac_authmgrcfg.cpp $(top_srcdir)/paccfg/pac_cfg_vlan.cpp $(top_srcdir)/paccfg/pac_vlancfg.cpp

libpaccfg_la_LIBADD = -lswsscommon -lnl-3 -lnl-route-3 -lhiredis $(SONIC_COMMON_LDFLAGS)
