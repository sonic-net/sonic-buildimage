INCLUDES =  -I $(top_srcdir)/mab -I $(top_srcdir)/mab/common -I $(top_srcdir)/mab/mapping/include -I $(top_srcdir)/fpinfra/inc -I $(top_srcdir)/mab/mapping/mab_sid -I $(top_srcdir)/mab/protocol/include -I $(top_srcdir)/authmgr/common -I $(top_srcdir)/../wpasupplicant/sonic-wpa-supplicant/src/radius

bin_PROGRAMS = mabd

if DEBUG
DBGFLAGS = -ggdb -DDEBUG
else
DBGFLAGS = -g -DNDEBUG
endif

mabd_SOURCES = $(top_srcdir)/mabmgr/mabmgr_main.cpp $(top_srcdir)/mabmgr/mabmgr.cpp

mabd_CPPFLAGS = $(DBGFLAGS) $(AM_CFLAGS) $(CFLAGS_COMMON) $(SONIC_COMMON_CFLAGS)

AM_LDFLAGS = -L$(top_srcdir)/fpinfra/ -lfpinfra
AM_LDFLAGS += -lswsscommon -lnl-3 -lnl-route-3 -lhiredis -L$(top_srcdir)/mab/ -lmab $(SONIC_COMMON_LDFLAGS)
