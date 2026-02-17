# Test Results for Issue #23323 Fix

## Issue Details
- **Issue Number**: #23323
- **Title**: Bug: sonic-cli 'show interface Ethernet' gets exception for ipv6 disabled subinterface
- **Root Cause**: Line 79 of `show_interface.j2` doesn't check if `addresses` field exists
- **Error**: `jinja2.exceptions.UndefinedError: 'collections.OrderedDict object' has no attribute 'addresses'`

## Fix Applied
**File**: `src/sonic-mgmt-framework/CLI/renderer/templates/show_interface.j2`  
**Line**: 79

**Before:**
```jinja2
{% if subif["openconfig-if-ip:ipv6"] %}
```

**After:**
```jinja2
{% if subif["openconfig-if-ip:ipv6"] and "addresses" in subif["openconfig-if-ip:ipv6"] %}
```

## Test Results

### ✅ Test 1: Jinja2 Syntax Validation
**Status**: PASSED  
**Result**: Template syntax is valid, no Jinja2 syntax errors

### ✅ Test 2: IPv6 Disabled Scenario (Bug Reproduction)
**Status**: PASSED  
**Scenario**: Interface with IPv6 disabled (no `addresses` field in JSON)  
**Result**: Template renders without errors - Fix is working!

**Test Data:**
```json
{
  "openconfig-if-ip:ipv6": {
    "config": {"enabled": false},
    "state": {"enabled": false}
    // No "addresses" field
  }
}
```

### ✅ Test 3: IPv6 Enabled Scenario (Normal Operation)
**Status**: PASSED  
**Scenario**: Interface with IPv6 enabled (with `addresses` field)  
**Result**: Template renders correctly - No regression!

**Test Data:**
```json
{
  "openconfig-if-ip:ipv6": {
    "config": {"enabled": true},
    "state": {"enabled": true},
    "addresses": {
      "address": [
        {
          "ip": "2001:db8::1",
          "state": {
            "ip": "2001:db8::1",
            "prefix-length": 64
          }
        }
      ]
    }
  }
}
```

### ✅ Test 4: Bug Reproduction with OLD Code
**Status**: PASSED (Bug confirmed)  
**Result**: OLD template code produces `UndefinedError` as expected  
**Conclusion**: Confirms the bug existed before our fix

## Summary

✅ **All tests passed!**
✅ **The fix for issue #23323 is working correctly!**
✅ **Template handles both IPv6 enabled and disabled scenarios!**
✅ **No regressions introduced!**

## Test Environment
- **Date**: 2026-02-17
- **Branch**: fix-issue-23323-ipv6-subinterface
- **Python Version**: 3.x
- **Jinja2**: Installed

## Commits
- **Submodule commit**: b927e10551318d389fe8a7f68f33aee59e9f0f7c
- **Parent commit**: 5b37fac78

## Next Steps
1. ✅ Fix implemented
2. ✅ Tests passed
3. ⏳ Push branch to GitHub
4. ⏳ Create PR to sonic-net/sonic-mgmt-framework
5. ⏳ Create PR to sonic-net/sonic-buildimage
