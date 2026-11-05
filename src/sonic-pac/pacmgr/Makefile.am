INCLUDES = -I $(top_srcdir)/pacmgr -I $(top_srcdir)/authmgr/common -I $(top_srcdir)/authmgr/mapping/include
INCLUDES += -I $(top_srcdir)/fpinfra/inc -I $(top_srcdir)/authmgr/mapping/auth_mgr_sid
INCLUDES += -I $(top_srcdir)/authmgr/protocol/include 
INCLUDES += -I $(top_srcdir)/pacoper/ 
bin_PROGRAMS = pacd 

if DEBUG
DBGFLAGS = -ggdb -DDEBUG
else
DBGFLAGS = -g -DNDEBUG
endif

pacd_SOURCES = $(top_srcdir)/pacmgr/pacmgr_main.cpp $(top_srcdir)/pacmgr/pacmgr.cpp
pacd_CPPFLAGS = $(DBGFLAGS) $(AM_CFLAGS) $(CFLAGS_COMMON) $(SONIC_COMMON_CFLAGS)

AM_LDFLAGS = -L$(top_srcdir)/fpinfra/ -lfpinfra
AM_LDFLAGS += -lswsscommon -lnl-3 -lnl-route-3 -lhiredis -L$(top_srcdir)/authmgr/ -lauthmgr
AM_LDFLAGS += -L$(top_srcdir)/paccfg/ -lpaccfg
AM_LDFLAGS += -L$(top_srcdir)/pacoper/ -lpacoper $(SONIC_COMMON_LDFLAGS)
