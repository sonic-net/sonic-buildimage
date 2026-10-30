"""Unit tests for switch_cpu_reboot_cause.py."""

import os
import sys

import pytest

_SCRIPTS_DIR = os.path.join(os.path.dirname(__file__), os.pardir, "scripts")
sys.path.insert(0, os.path.abspath(_SCRIPTS_DIR))

import switch_cpu_reboot_cause as reboot_cause  # noqa: E402


def test_extract_json_ietf_payload():
    gnmi_text = (
        'notification: update { path { elem { name: "reboot-cause" } } '
        'val { json_ietf_val: "{\\"cause\\":\\"User issued reboot\\",'
        '\\"time\\":\\"2026-06-01 12:00:00\\"}" } }'
    )
    data = reboot_cause.extract_json_ietf_payload(gnmi_text)
    assert data["cause"] == "User issued reboot"
    assert data["time"] == "2026-06-01 12:00:00"


def test_extract_json_ietf_payload_missing_marker():
    with pytest.raises(ValueError, match="no json_ietf_val"):
        reboot_cause.extract_json_ietf_payload("no payload here")


def test_format_reboot_cause_orders_known_keys_first():
    text = reboot_cause.format_reboot_cause(
        {
            "extra": "value",
            "cause": "Power Loss",
            "time": "2026-06-01 12:00:00",
            "user": "admin",
        }
    )
    lines = text.splitlines()
    assert lines[0].startswith("Cause")
    assert lines[1].startswith("Time")
    assert lines[-1].startswith("extra")


def test_format_reboot_cause_history_table():
    text = reboot_cause.format_reboot_cause_history(
        {
            "2026-06-02": {"cause": "B", "time": "t2", "user": "u2", "comment": "c2"},
            "2026-06-01": {"cause": "A", "time": "t1", "user": "u1", "comment": "c1"},
        }
    )
    assert "Name" in text
    assert "2026-06-02" in text
    assert text.index("2026-06-02") < text.index("2026-06-01")
