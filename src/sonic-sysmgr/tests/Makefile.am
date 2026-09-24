AUTOMAKE_OPTIONS = subdir-objects
INCLUDES = -I $(top_srcdir) -I $(top_srcdir)/rebootbackend -I$(top_srcdir)/build/gen -I$(top_srcdir)/build/gen/github.com/openconfig/gnoi -I ../../sonic-swss-common/common

TESTS = tests tests_asan tests_tsan tests_usan

noinst_PROGRAMS = tests tests_asan tests_tsan tests_usan

if DEBUG
DBGFLAGS = -ggdb -DDEBUG
else
DBGFLAGS = -g -DNDEBUG
endif

CFLAGS_GTEST =
LDADD_GTEST = -lgtest -lgtest_main -lgmock -lgmock_main
CFLAGS_COVERAGE = --coverage -fprofile-arcs -ftest-coverage
LDADD_COVERAGE = -lgcov
CFLAGS_ASAN = -fsanitize=address
CFLAGS_TSAN = -fsanitize=thread
CFLAGS_USAN = -fsanitize=undefined

testsdir = /usr/include

tests_HEADERS = /usr/include/gmock/gmock.h

tests_SOURCES = rebootbe_test.cpp \
                $(top_srcdir)/rebootbackend/rebootbe.cpp \
                reboot_thread_test.cpp \
                $(top_srcdir)/rebootbackend/reboot_thread.cpp \
                test_main.cpp

tests_CFLAGS = $(DBGFLAGS) $(AM_CFLAGS) $(CFLAGS_COMMON) $(CFLAGS_GTEST) $(CFLAGS_COVERAGE) $(CFLAGS_SAI)
tests_CPPFLAGS = $(DBGFLAGS) $(AM_CFLAGS) $(CFLAGS_COMMON) $(CFLAGS_GTEST) $(CFLAGS_COVERAGE) $(CFLAGS_SAI)
tests_LDADD = $(LDADD_GTEST) $(LDADD_COVERAGE) -lswsscommon -lpthread \
		-lprotobuf $(top_srcdir)/build/gen/librebootgnoi.la

tests_asan_SOURCES = $(tests_SOURCES)
tests_asan_CFLAGS = $(DBGFLAGS) $(AM_CFLAGS) $(CFLAGS_COMMON) $(CFLAGS_GTEST) $(CFLAGS_ASAN) $(CFLAGS_SAI)
tests_asan_CPPFLAGS = $(DBGFLAGS) $(AM_CFLAGS) $(CFLAGS_COMMON) $(CFLAGS_GTEST) $(CFLAGS_ASAN) $(CFLAGS_SAI)
tests_asan_LDFLAGS = $(CFLAGS_ASAN)
tests_asan_LDADD = $(LDADD_GTEST) -lswsscommon -lpthread -lprotobuf \
			$(top_srcdir)/build/gen/librebootgnoi.la


tests_tsan_SOURCES = $(tests_SOURCES)
tests_tsan_CFLAGS = $(DBGFLAGS) $(AM_CFLAGS) $(CFLAGS_COMMON) $(CFLAGS_GTEST) $(CFLAGS_TSAN) $(CFLAGS_SAI)
tests_tsan_CPPFLAGS = $(DBGFLAGS) $(AM_CFLAGS) $(CFLAGS_COMMON) $(CFLAGS_GTEST) $(CFLAGS_TSAN) $(CFLAGS_SAI)
tests_tsan_LDFLAGS = $(CFLAGS_TSAN)
tests_tsan_LDADD = $(LDADD_GTEST) -lswsscommon -lpthread -lprotobuf \
			$(top_srcdir)/build/gen/librebootgnoi.la

tests_usan_SOURCES = $(tests_SOURCES)
tests_usan_CFLAGS = $(DBGFLAGS) $(AM_CFLAGS) $(CFLAGS_COMMON) $(CFLAGS_GTEST) $(CFLAGS_USAN) $(CFLAGS_SAI)
tests_usan_CPPFLAGS = $(DBGFLAGS) $(AM_CFLAGS) $(CFLAGS_COMMON) $(CFLAGS_GTEST) $(CFLAGS_USAN) $(CFLAGS_SAI)
tests_usan_LDFLAGS = $(CFLAGS_USAN)
tests_usan_LDADD = $(LDADD_GTEST) -lswsscommon -lpthread -lprotobuf \
			$(top_srcdir)/build/gen/librebootgnoi.la
