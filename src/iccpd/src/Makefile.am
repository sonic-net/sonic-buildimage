SUBDIRS = mclagdctl

INCLUDES = -I$(top_srcdir)/include -I/usr/include/libnl3

bin_PROGRAMS = iccpd

if DEBUG
DBGFLAGS = -ggdb -DDEBUG
else
DBGFLAGS = -g -DNDEBUG
endif

iccpd_SOURCES = \
            app_csm.c cmd_option.c iccp_cli.c iccp_cmd_show.c iccp_cmd.c \
	    iccp_csm.c iccp_ifm.c iccp_main.c logger.c \
	    port.c scheduler.c system.c iccp_consistency_check.c \
	    mlacp_link_handler.c \
	    mlacp_sync_prepare.c mlacp_sync_update.c\
	    mlacp_fsm.c \
	    iccp_netlink.c \
	    iccp_netlink_utils.c \
            openbsd_tree.c
iccpd_CFLAGS = $(DBGFLAGS) $(AM_CFLAGS) $(CFLAGS_COMMON)
iccpd_LDADD = -lnl-genl-3 -lnl-route-3 -lnl-3 -lpthread

check_PROGRAMS = iccp_netlink_utils_test
TESTS = $(check_PROGRAMS)
iccp_netlink_utils_test_SOURCES = tests/iccp_netlink_utils_test.c iccp_netlink_utils.c
iccp_netlink_utils_test_CFLAGS = -g $(AM_CFLAGS) $(CFLAGS_COMMON)
