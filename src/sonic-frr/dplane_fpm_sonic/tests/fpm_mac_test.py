# SPDX-License-Identifier: GPL-2.0-or-later
import frrtest


class TestFpmMac(frrtest.TestMultiOut):
    program = "./fpm_mac_test"


TestFpmMac.onesimple("fpm_mac_test: PASS")
