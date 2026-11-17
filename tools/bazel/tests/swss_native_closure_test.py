"""Keep ordinary SWSS next-hop classes distinct from the separate libFIB ABI."""

from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from tools.bazel.ci.swss_native_closure import FIB


class FibOwnershipTest(unittest.TestCase):
    def test_swss_next_hop_types_are_not_a_libfib_dependency(self):
        for value in ("NextHopGroupKey", "FdbDest::NEXTHOPGROUP", '"nexthopgroupkey.h"'):
            with self.subTest(value=value):
                self.assertIsNone(FIB.search(value))

    def test_library_owner_header_and_c_abi_require_review(self):
        for value in ("libnexthopgroup.so.0", "-lnexthopgroup", "//src/libraries/sonic-fib:nexthopgroup",
                      "nexthopgroup/c_nexthopgroupfull.h", "fib_frr_set_log_level(3)",
                      "nexthopgroupfull_json_from_c_nhg_multi(group)", "C_NextHopGroupFull"):
            with self.subTest(value=value):
                self.assertIsNotNone(FIB.search(value))


if __name__ == "__main__":
    unittest.main()
