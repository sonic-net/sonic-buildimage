import json

import pytest


def protection_data(fields=None, ifname="PortChannel0"):
    return {
        "sonic-portchannel:sonic-portchannel": {
            "PORTCHANNEL": {"PORTCHANNEL_LIST": [
                {"name": "PortChannel0", "admin_status": "up"}
            ]}
        },
        "sonic-bridge-port-protection:sonic-bridge-port-protection": {
            "BRIDGE_PORT_PROTECTION": {
                "BRIDGE_PORT_PROTECTION_LIST": [
                    dict({"ifname": ifname}, **(fields or {}))
                ]
            }
        },
    }


@pytest.mark.parametrize("admin_mode", ["AUTO", "PRIMARY", "PROTECTION"])
@pytest.mark.parametrize("revertive", ["true", "false"])
@pytest.mark.parametrize("timer", ["0", "5000", "4294967295"])
def test_valid_policy(yang_model, admin_mode, revertive, timer):
    yang_model.load_data(protection_data({
        "protection_mode": "SOFTWARE",
        "admin_mode": admin_mode,
        "revertive": revertive,
        "wait_to_restore_time_ms": timer,
    }))


def test_defaults(yang_model):
    node = yang_model.ctx.parse_data_mem(
        json.dumps(protection_data()), "json", strict=True, no_state=True,
        json_string_datatypes=True,
    )
    try:
        path = (
            "/sonic-bridge-port-protection:sonic-bridge-port-protection/"
            "BRIDGE_PORT_PROTECTION/BRIDGE_PORT_PROTECTION_LIST"
            "[ifname='PortChannel0']/"
        )
        expected = {
            "protection_mode": "SOFTWARE", "admin_mode": "AUTO",
            "revertive": False, "wait_to_restore_time_ms": 0,
        }
        for field, value in expected.items():
            assert node.find_one(path + field).value() == value
    finally:
        node.free()


@pytest.mark.parametrize("field,value,error", [
    ("protection_mode", "HARDWARE", "Invalid enumeration value"),
    ("protection_mode", "software", "Invalid enumeration value"),
    ("protection_mode", "UNKNOWN", "Invalid enumeration value"),
    ("admin_mode", "auto", "Invalid enumeration value"),
    ("admin_mode", "UNKNOWN", "Invalid enumeration value"),
    ("revertive", "yes", "Invalid boolean value"),
    ("wait_to_restore_time_ms", "-1", "min/max bounds"),
    ("wait_to_restore_time_ms", "4294967296", "min/max bounds"),
    ("wait_to_restore_time_ms", "1.5", "Invalid"),
    ("convergence_mode", "AUTO", "not found"),
    ("t_failover", "100", "not found"),
])
def test_invalid_policy(yang_model, field, value, error):
    yang_model.load_data(protection_data({field: value}), error)


@pytest.mark.parametrize("ifname,error", [
    ("PortChannel1", "Invalid leafref value"),
    ("PortChannel0.100", "Unsatisfied pattern"),
    ("Ethernet0", "Unsatisfied pattern"),
])
def test_interface_reference(yang_model, ifname, error):
    yang_model.load_data(protection_data(ifname=ifname), error)
