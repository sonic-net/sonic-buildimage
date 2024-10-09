INCLUDES = -I $(top_srcdir) -I/usr/include/dbus-c++-1/ -I $(top_srcdir)/system 

bin_PROGRAMS = rebootbackend

if DEBUG
DBGFLAGS = -ggdb -DDEBUG
else
DBGFLAGS = -g
endif

rebootbackend_SOURCES = rebootbackend.cpp rebootbe.cpp interfaces.cpp \
                        reboot_thread.cpp init_thread.cpp redis_utils.cpp \
                        reboot_common.cpp telemetry_helper.cpp \
                        system/system.pb.cc types/types.pb.cc \
                        common/common.pb.cc container_stop.pb.cc

rebootbackend_CFLAGS = $(DBGFLAGS) $(AM_CFLAGS) $(CFLAGS_COMMON) $(CFLAGS_ASAN)
rebootbackend_CPPFLAGS = $(DBGFLAGS) $(AM_CFLAGS) $(CFLAGS_COMMON) $(CFLAGS_ASAN)
rebootbackend_LDADD = $(LDFLAGS_ASAN) -lswsscommon -ldbus-c++-1 -lpthread -lprotobuf
