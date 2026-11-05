sonic_wpa_supp_path = $(top_srcdir)/../wpasupplicant/sonic-wpa-supplicant

INCLUDES =  -I $(top_srcdir)/paccfg -I $(top_srcdir)/pacoper -I $(top_srcdir)/authmgr/common -I $(top_srcdir)/authmgr/mapping/include -I $(top_srcdir)/fpinfra/inc -I $(top_srcdir)/authmgr/mapping/auth_mgr_sid -I $(top_srcdir)/authmgr/protocol/include -I $(sonic_wpa_supp_path)/src/common -I $(sonic_wpa_supp_path)/src/utils -I $(sonic_wpa_supp_path)/src/radius -I $(top_srcdir)/mab/mapping/include

lib_LTLIBRARIES = libauthmgr.la 

if DEBUG
DBGFLAGS = -ggdb -DDEBUG
else
DBGFLAGS = -g -DNDEBUG
endif

AM_CPPFLAGS = -save-temps -Wall -Wno-pointer-sign -Wno-pointer-sign -Wno-unused-but-set-variable -Wno-address -Wno-array-bounds -Wno-sequence-point -Wno-switch -Wno-uninitialized -Wno-unused-result -Wno-aggressive-loop-optimizations -Wno-sizeof-pointer-memaccess -Wno-unused-local-typedefs -Wno-unused-value -Wno-format-truncation -g  -Werror $(SONIC_COMMON_CFLAGS) -DCONFIG_CTRL_IFACE -DCONFIG_CTRL_IFACE_UNIX -DCONFIG_SONIC_HOSTAPD

libauthmgr_la_SOURCES = $(top_srcdir)/authmgr/protocol/auth_mgr_db.c $(top_srcdir)/authmgr/mapping/auth_mgr_cnfgr.c $(top_srcdir)/authmgr/mapping/auth_mgr_cfg.c $(top_srcdir)/authmgr/mapping/auth_mgr_api.c $(top_srcdir)/authmgr/mapping/auth_mgr_control.c $(top_srcdir)/authmgr/mapping/auth_mgr_client.c $(top_srcdir)/authmgr/mapping/auth_mgr_ih.c $(top_srcdir)/authmgr/mapping/auth_mgr_debug.c $(top_srcdir)/authmgr/mapping/auth_mgr_sid/auth_mgr_sid.c $(top_srcdir)/authmgr/mapping/auth_mgr_dot1x.c $(top_srcdir)/authmgr/mapping/auth_mgr_mab.c $(top_srcdir)/authmgr/mapping/auth_mgr_socket.c $(top_srcdir)/authmgr/protocol/auth_mgr_sm.c $(top_srcdir)/authmgr/protocol/auth_mgr_mac_db.c $(top_srcdir)/authmgr/protocol/auth_mgr_radius.c $(top_srcdir)/authmgr/protocol/auth_mgr_timer.c $(top_srcdir)/authmgr/protocol/auth_mgr_utils.c $(top_srcdir)/authmgr/protocol/auth_mgr_vlan.c $(top_srcdir)/authmgr/protocol/auth_mgr_vlan_db.c $(top_srcdir)/authmgr/protocol/auth_mgr_txrx.c $(sonic_wpa_supp_path)/src/common/wpa_ctrl.c $(sonic_wpa_supp_path)/src/utils/os_unix.c

libauthmgr_la_LIBADD = -L$(top_srcdir)/fpinfra/ -lfpinfra -L$(top_srcdir)/paccfg/ -lpaccfg -L$(top_srcdir)/pacoper/ -lpacoper $(SONIC_COMMON_LDFLAGS)

