#!/usr/bin/env python3
"""
Integration test for SRv6 Global Encapsulation Source Address
Tests the complete flow: CONFIG_DB -> bgpcfgd -> FRR commands
"""
import sys
sys.path.insert(0, 'tests')
import swsscommon_test
sys.modules['swsscommon'] = swsscommon_test

from unittest.mock import MagicMock, call
from bgpcfgd.directory import Directory
from bgpcfgd.template import TemplateFabric
from bgpcfgd.managers_srv6 import Srv6GlobalMgr

def test_integration_srv6_global():
    """
    Integration test simulating CONFIG_DB update flow
    """
    print("\n" + "=" * 70)
    print("SRv6 Global Source Address - Integration Test")
    print("=" * 70)

    # Setup
    cfg_mgr = MagicMock()
    common_objs = {
        'directory': Directory(),
        'cfg_mgr':   cfg_mgr,
        'tf':        TemplateFabric(),
        'constants': {},
    }

    global_mgr = Srv6GlobalMgr(common_objs, "CONFIG_DB", "SRV6_GLOBAL")

    # Test 1: Set global encap source address
    print("\n Test 1: Set SRv6 global encap source address")
    print("  CONFIG_DB: SRV6_GLOBAL|default -> encap_source_address: fc00:1::1")

    result = global_mgr.set_handler("default", {'encap_source_address': 'fc00:1::1'})
    assert result == True, "set_handler should return True"

    # Verify FRR commands were generated
    assert cfg_mgr.push_list.called, "FRR commands should be generated"
    frr_cmds = cfg_mgr.push_list.call_args[0][0]

    expected_cmds = [
        "segment-routing",
        "srv6",
        "encapsulation",
        "source-address fc00:1::1"
    ]

    assert frr_cmds == expected_cmds, f"Expected {expected_cmds}, got {frr_cmds}"
    print("  FRR commands generated correctly:")
    for cmd in frr_cmds:
        print(f"     {cmd}")

    # Test 2: Update to a different address
    print("\n Test 2: Update SRv6 global encap source address")
    print("  CONFIG_DB: SRV6_GLOBAL|default -> encap_source_address: fc00:2::2")

    cfg_mgr.reset_mock()
    result = global_mgr.set_handler("default", {'encap_source_address': 'fc00:2::2'})
    assert result == True, "set_handler should return True"

    frr_cmds = cfg_mgr.push_list.call_args[0][0]
    expected_cmds = [
        "segment-routing",
        "srv6",
        "encapsulation",
        "source-address fc00:2::2"
    ]

    assert frr_cmds == expected_cmds, f"Expected {expected_cmds}, got {frr_cmds}"
    print("  FRR commands updated correctly:")
    for cmd in frr_cmds:
        print(f"     {cmd}")

    # Test 3: Delete configuration
    print("\n Test 3: Delete SRv6 global encap source address")
    print("  CONFIG_DB: DEL SRV6_GLOBAL|default")

    cfg_mgr.reset_mock()
    global_mgr.del_handler("default")

    assert cfg_mgr.push_list.called, "FRR commands should be generated"
    frr_cmds = cfg_mgr.push_list.call_args[0][0]

    expected_cmds = [
        "segment-routing",
        "srv6",
        "encapsulation",
        "no source-address"
    ]

    assert frr_cmds == expected_cmds, f"Expected {expected_cmds}, got {frr_cmds}"
    print("  FRR delete commands generated correctly:")
    for cmd in frr_cmds:
        print(f"     {cmd}")

    # Test 4: Invalid IPv6 address
    print("\n Test 4: Reject invalid IPv6 address")
    print("  CONFIG_DB: SRV6_GLOBAL|default -> encap_source_address: invalid-address")

    cfg_mgr.reset_mock()
    result = global_mgr.set_handler("default", {'encap_source_address': 'invalid-address'})
    assert result == False, "set_handler should return False for invalid IPv6"
    assert not cfg_mgr.push_list.called, "FRR commands should not be generated for invalid input"
    print("  Invalid IPv6 address rejected correctly")

    # Test 5: Invalid key
    print("\n Test 5: Reject invalid key (not 'default')")
    print("  CONFIG_DB: SRV6_GLOBAL|invalid_key -> encap_source_address: fc00:1::1")

    cfg_mgr.reset_mock()
    result = global_mgr.set_handler("invalid_key", {'encap_source_address': 'fc00:1::1'})
    assert result == False, "set_handler should return False for invalid key"
    assert not cfg_mgr.push_list.called, "FRR commands should not be generated for invalid key"
    print("  Invalid key rejected correctly")

    print("\n" + "=" * 70)
    print("All integration tests PASSED!")
    print("=" * 70)

    return 0

if __name__ == "__main__":
    try:
        sys.exit(test_integration_srv6_global())
    except AssertionError as e:
        print(f"\n Integration test FAILED: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"\n Integration test ERROR: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
