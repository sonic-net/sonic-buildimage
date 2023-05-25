#######################################################################
#
# Copyright 2019 Broadcom. All rights reserved.
# The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
#
#######################################################################

#
# Makefile for libnss-radius
#

TARGETS = libnss_radius.so.2 cache_radius
COMMON_INCLUDE = nss_radius_common.h
COMMON_SOURCE = nss_radius_common.c
LIBNSS_SOURCE = nss_radius.c $(COMMON_SOURCE)
CACHE_SOURCE = cache_radius.c $(COMMON_SOURCE)


all: $(TARGETS)

libnss_radius.so.2: $(LIBNSS_SOURCE) $(COMMON_INCLUDE)
	$(CC) $(CFLAGS) $(LDFLAGS) -fPIC -Wall -shared -o libnss_radius.so.2 \
		-Wl,-soname,libnss_radius.so.2 -Wl,--version-script=libnss_radius_vs.txt $(LIBNSS_SOURCE)

cache_radius: $(CACHE_SOURCE) $(COMMON_INCLUDE)
	$(CC) $(CFLAGS) $(LDFLAGS) -o cache_radius $(CACHE_SOURCE)

clean:
	-rm -f $(TARGETS)
	-rm -f test_nss_radius test_cache_radius

distclean: clean

test: test_nss_radius.c $(LIBNSS_SOURCE) $(CACHE_SOURCE) \
		$(COMMON_SOURCE) $(COMMON_INCLUDE)
	$(CC) $(CFLAGS) $(LDFLAGS) -g -DTEST_RADIUS_NSS -o test_nss_radius \
		$(LIBNSS_SOURCE) test_nss_radius.c
	$(CC) $(CFLAGS) $(LDFLAGS) -g -DTEST_RADIUS_NSS -o test_cache_radius \
		$(CACHE_SOURCE)


.PHONY: all clean distclean test

