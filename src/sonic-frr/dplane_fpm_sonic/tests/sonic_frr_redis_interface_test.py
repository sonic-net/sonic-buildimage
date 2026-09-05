# SPDX-License-Identifier: GPL-2.0-or-later
import frrtest


class TestSonicFrrRedisInterface(frrtest.TestMultiOut):
    program = "./sonic_frr_redis_interface_test"


TestSonicFrrRedisInterface.onesimple("sonic_frr_redis_interface_test: PASS")
