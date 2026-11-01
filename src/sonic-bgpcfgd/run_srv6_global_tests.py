#!/usr/bin/env python3
"""
Simple test runner for Srv6GlobalMgr tests
"""
import sys
sys.path.insert(0, 'tests')
import swsscommon_test
sys.modules['swsscommon'] = swsscommon_test

from tests.test_srv6 import (
    test_srv6_global_set_encap_source_address,
    test_srv6_global_del_encap_source_address,
    test_srv6_global_invalid_ipv6,
    test_srv6_global_update_encap_source_address
)

def run_tests():
    tests = [
        ("test_srv6_global_set_encap_source_address", test_srv6_global_set_encap_source_address),
        ("test_srv6_global_del_encap_source_address", test_srv6_global_del_encap_source_address),
        ("test_srv6_global_invalid_ipv6", test_srv6_global_invalid_ipv6),
        ("test_srv6_global_update_encap_source_address", test_srv6_global_update_encap_source_address),
    ]

    passed = 0
    failed = 0

    print("=" * 70)
    print("Running Srv6GlobalMgr Unit Tests")
    print("=" * 70)

    for test_name, test_func in tests:
        try:
            print(f"\n Running {test_name}...", end=" ")
            test_func()
            print(" PASSED")
            passed += 1
        except AssertionError as e:
            print(f" FAILED")
            print(f"  Error: {e}")
            failed += 1
        except Exception as e:
            print(f" ERROR")
            print(f"  Exception: {e}")
            failed += 1

    print("\n" + "=" * 70)
    print(f"Test Results: {passed} passed, {failed} failed")
    print("=" * 70)

    return 0 if failed == 0 else 1

if __name__ == "__main__":
    sys.exit(run_tests())
