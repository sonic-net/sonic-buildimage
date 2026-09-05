sonic_wpa_supp_path = $(top_srcdir)/../wpasupplicant/sonic-wpa-supplicant
radius_lib_path = $(top_srcdir)/../wpasupplicant/sonic-wpa-supplicant/build/radius_lib
radius_lib = $(radius_lib_path)/src/radius/libradius.a
utils_lib  = $(radius_lib_path)/src/utils/libutils.a
crypto_lib = $(radius_lib_path)/src/crypto/libcrypto.a

INCLUDES =  -I $(top_srcdir)/fpinfra/inc -I $(top_srcdir)/mab/common -I $(top_srcdir)/mab/mapping/mab_sid -I $(top_srcdir)/mab/mapping/include -I $(top_srcdir)/mab/protocol/include -I $(top_srcdir)/authmgr/common -I $(sonic_wpa_supp_path)/src/utils -I $(sonic_wpa_supp_path)/src/radius 


#bin_PROGRAMS = mabd 
lib_LTLIBRARIES = libmab.la 

if DEBUG
DBGFLAGS = -ggdb -DDEBUG
else
DBGFLAGS = -g -DNDEBUG
endif

AM_CPPFLAGS = -save-temps -Wall -Wno-pointer-sign -Wno-unused-but-set-variable -Wno-address -Wno-array-bounds -Wno-sequence-point -Wno-switch -Wno-uninitialized -Wno-unused-result -Wno-aggressive-loop-optimizations -Wno-sizeof-pointer-memaccess -Wno-unused-local-typedefs -Wno-unused-value -Wno-format-truncation -g  -Werror $(SONIC_COMMON_CFLAGS) -DCONFIG_SONIC_RADIUS

libmab_la_SOURCES = mapping/mab_socket.c mapping/mab_init.c mapping/mab_cfg.c mapping/mab_api.c mapping/mab_client.c mapping/mab_ih.c mapping/mab_debug.c mapping/mab_sid/mab_sid.c mapping/mab_auth_mgr.c protocol/mab_db.c mapping/mab_control.c protocol/mab_mac_db.c protocol/mab_vlan.c  protocol/mab_utils.c protocol/mab_auth.c protocol/mab_local.c protocol/mab_timer.c protocol/mab_radius.c $(sonic_wpa_supp_path)/src/radius/radius_mab.c 


libmab_la_LIBADD = -lpthread -lswsscommon -L$(top_srcdir)/fpinfra/ -lfpinfra $(radius_lib) $(utils_lib) $(crypto_lib) -lrt $(SONIC_COMMON_LDFLAGS)

$(radius_lib) $(utils_lib) $(crypto_lib):
	cd $(sonic_wpa_supp_path)/radius_lib && make

