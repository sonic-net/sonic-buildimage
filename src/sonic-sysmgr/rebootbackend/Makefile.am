AUTOMAKE_OPTIONS = subdir-objects
INCLUDES = -I $(top_srcdir) -I/usr/include/dbus-c++-1/ -I$(top_srcdir)/build/gen -I$(top_srcdir)/build/gen/github.com/openconfig/gnoi -I ../../sonic-swss-common/common

bin_PROGRAMS = rebootbackend

if DEBUG
DBGFLAGS = -ggdb -DDEBUG
else
DBGFLAGS = -g
endif

rebootbackend_SOURCES = rebootbackend.cpp rebootbe.cpp interfaces.cpp \
                        reboot_thread.cpp  

rebootbackend_CFLAGS = $(DBGFLAGS) $(AM_CFLAGS) $(CFLAGS_COMMON) $(CFLAGS_ASAN)
rebootbackend_CPPFLAGS = $(DBGFLAGS) $(AM_CFLAGS) $(CFLAGS_COMMON) $(CFLAGS_ASAN)
rebootbackend_LDADD = $(LDFLAGS_ASAN) -lswsscommon -ldbus-c++-1 -lpthread -lprotobuf $(top_srcdir)/build/gen/librebootgnoi.la
