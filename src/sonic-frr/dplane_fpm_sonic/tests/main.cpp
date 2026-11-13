// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * googletest entry point for the dplane NHG unit tests. The cases live in
 * test_fpm_nhg_gtest.cpp and run against the real fpm_nhg.c plus a few zebra
 * symbol stubs; no zebra, no dataplane, no device state.
 */
#include "gtest/gtest.h"
#include <syslog.h>

#include "lib/zlog.h"

int main(int argc, char *argv[])
{
	zlog_aux_init("NHG_TEST: ", LOG_DEBUG);
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
