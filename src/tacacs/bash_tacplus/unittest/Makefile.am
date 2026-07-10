AUTOMAKE_OPTIONS = subdir-objects

noinst_PROGRAMS = plugin_test
TESTS = plugin_test

# Use a custom CFLAGS override because Trixie will add -Werror=implicit-function-declaration to the default CFLAGS.
CFLAGS = -g -O2 -fstack-protector-strong -fstack-clash-protection -Wformat -Werror=format-security

# disable some warning because UT need test functions not in header file.
CFLAGS_TEST = -Wno-parentheses -Wno-format-security -Wno-implicit-function-declaration -Wno-int-to-pointer-cast
IFLAGS_TEST = -I.. -I../include -I../lib
DBGFLAGS = -DDEBUG -DBASH_PLUGIN_UT

plugin_test_SOURCES = plugin_test.c mock_helper.c ../bash_tacplus.c

plugin_test_CFLAGS = $(DBGFLAGS) $(AM_CFLAGS) $(CFLAGS_COMMON) $(CFLAGS_TEST) $(IFLAGS_TEST)
plugin_test_LDADD = -lc -lcunit
