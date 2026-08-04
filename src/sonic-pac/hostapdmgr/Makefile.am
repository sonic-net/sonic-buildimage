sonic_wpa_supp_path = $(top_srcdir)/../wpasupplicant/sonic-wpa-supplicant

INCLUDES = -I $(top_srcdir)/hostapdmgr \
           -I $(sonic_wpa_supp_path)/src/common -I $(sonic_wpa_supp_path)/src/utils \
           -I /usr/include/libnl3 \
           -I /usr/include/swss

bin_PROGRAMS = hostapdmgrd

if DEBUG
DBGFLAGS = -ggdb -DDEBUG
else
DBGFLAGS = -g -DNDEBUG
endif

hostapdmgrd_SOURCES = hostapdmgr_main.cpp $(sonic_wpa_supp_path)/src/common/wpa_ctrl.c  \
                                          $(sonic_wpa_supp_path)/src/utils/os_unix.c hostapdmgr.cpp

hostapdmgrd_CPPFLAGS = $(DBGFLAGS) $(AM_CFLAGS) $(SONIC_COMMON_CFLAGS) -DCONFIG_CTRL_IFACE -DCONFIG_CTRL_IFACE_UNIX -DCONFIG_SONIC_HOSTAPD

AM_LDFLAGS = -lswsscommon -lnl-3 -lnl-route-3 -lhiredis -lelf $(LIBNL_LIBS) $(SONIC_COMMON_LDFLAGS)
