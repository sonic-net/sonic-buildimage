INCLUDES =  -I $(top_srcdir)/pacoper -I $(top_srcdir)/authmgr/common -I $(top_srcdir)/authmgr/mapping/include -I $(top_srcdir)/fpinfra/inc -I $(top_srcdir)/authmgr/protocol/include 

lib_LTLIBRARIES = libpacoper.la 

if DEBUG
DBGFLAGS = -ggdb -DDEBUG
else
DBGFLAGS = -g -DNDEBUG
endif


libpacoper_la_SOURCES = $(top_srcdir)/pacoper/pacoper.cpp

AM_CPPFLAGS = $(DBGFLAGS) $(AM_CFLAGS) $(SONIC_COMMON_CFLAGS) $(CFLAGS_COMMON)
#libipacoper_la_CPPFLAGS = $(DBGFLAGS) $(CFLAGS_COMMON)

libpacoper_la_LIBADD = -lswsscommon -lnl-3 -lnl-route-3 -lhiredis $(SONIC_COMMON_LDFLAGS) -L$(top_srcdir)/fpinfra -lfpinfra
