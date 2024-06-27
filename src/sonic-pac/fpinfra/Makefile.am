lib_LTLIBRARIES = libfpinfra.la

libfpinfra_la_SOURCES = osapi/osapi.c \
                        osapi/osapi_sem.c \
                        osapi/osapi_support.c \
                        osapi/osapi_util.c \
                        osapi/osapi_time.c \
                        osapi/osapi_rwlock.c \
                        osapi/proc_osapi_msg.c \
                        osapi/proc_osapi_task.c \
                        osapi/init_platform_config.c \
                        osapi/osapi_file.c \
                        util/sll/sll.c \
                        util/apptimer/apptimer.c \
                        util/avl/avl.c \
                        util/avl/tree_api.c \
                        util/avl/avl_util.c \
                        nim/nim_util.c \
                        nim/nim_intf_api.c \
                        nim/nim_intf_map_api.c \
                        nim/nim_startup.c \
                        nim/nim_events.c \
                        nim/nim_trace.c \
                        nim/nim_ifindex.c \
                        nim/nim_outcalls.c \
                        nim/nim_config.c \
                        nim/nim_cnfgr.c \
                        nim/nim_debug.c \
                        nim/nim_data.c \
                        fpnim.cpp \
                        nimsync.cpp \
                        sim/sim.cpp \
                        sysapi/sysapi_hpc.c \
                        sysapi/sysapi.c \
                        sysapi/sysapi_if_net.c \
                        util/md5/md5.c \
                        util/utils/util.c \
                        util/buff/buff.c \
                        fpSonicUtils.cpp
#AM_CPPFLAGS  = -I$(top_srcdir)/inc
DBGFLAGS = -ggdb -DDEBUG
AM_CPPFLAGS  = -Iinc -I $(top_srcdir) -I/usr/include/libnl3 -I/usr/include/swss $(DBGFLAGS) $(SONIC_COMMON_CFLAGS)
AM_LDFLAGS = -lnl-3 -lrt -pthread $(SONIC_COMMON_LDFLAGS) -lelf $(LIBNL_LIBS) -Wl,-Bsymbolic
