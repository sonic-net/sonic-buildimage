"""Unit tests for leak-sensor definitions and GPIO decoding
(sonic_platform/leak_detect.py).

Self-contained by design: leak_detect.py is loaded directly by path rather than
imported as sonic_platform.leak_detect, because sonic_platform/__init__.py pulls
in sonic_platform_base and its dependencies, which are not needed to test the
decoding. No BMC or GPIO hardware is required.

The gpioget output parser gets the heaviest coverage here: it is the one piece
that has to survive a libgpiod version change, and it is the piece that must
never decode an unknown value as "no leak".
"""

import importlib.util
import os
import re

import pytest

_TESTS_DIR = os.path.dirname(os.path.abspath(__file__))
_COMMON_DIR = os.path.dirname(_TESTS_DIR)
LEAK_DETECT_PATH = os.path.join(_COMMON_DIR, "sonic_platform", "leak_detect.py")

# common/tests -> common -> sonic-platform-modules-nexthop -> aspeed -> platform
_REPO_ROOT = os.path.abspath(os.path.join(_TESTS_DIR, *([os.pardir] * 5)))
UPSTREAM_LIQUID_COOLING_BASE = os.path.join(
    _REPO_ROOT, "src", "sonic-platform-common", "sonic_platform_base",
    "liquid_cooling_base.py")


def _load(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    assert spec is not None and spec.loader is not None, f"cannot load {path}"
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


ld = _load(LEAK_DETECT_PATH, "nexthop_leak_detect_under_test")


# --------------------------------------------------------------------------
# Severity strings must match upstream
# --------------------------------------------------------------------------

def test_severity_strings_match_upstream():
    """leak_detect duplicates LeakSeverity.value so it can stay dependency-free.
    If upstream renames them, thermalctld and bmcctld would compare against
    different strings than we emit."""
    if not os.path.exists(UPSTREAM_LIQUID_COOLING_BASE):
        pytest.skip("sonic-platform-common submodule not checked out")

    with open(UPSTREAM_LIQUID_COOLING_BASE) as f:
        src = f.read()
    values = set(re.findall(r'^\s+[A-Z_]+\s*=\s*"([A-Z_]+)"', src, re.MULTILINE))
    assert ld.SEVERITY_MINOR in values
    assert ld.SEVERITY_CRITICAL in values


# --------------------------------------------------------------------------
# Sensor inventory
# --------------------------------------------------------------------------

def test_two_sensors_one_per_gpio_line():
    """Two GPIO lines are what the BMC can see, so two sensors are exposed."""
    assert ld.SENSOR_KEYS == ("coldplate", "driptray")


def test_driptray_is_the_severe_position():
    """Liquid pooling in the chassis is the severe class; a localized
    cold-plate leak is not."""
    assert ld.SEVERITY_BY_KEY["driptray"] == ld.SEVERITY_CRITICAL
    assert ld.SEVERITY_BY_KEY["coldplate"] == ld.SEVERITY_MINOR


def test_sensor_types_and_locations():
    assert ld.SENSOR_BY_KEY["driptray"]["type"] == "Rope"
    assert ld.SENSOR_BY_KEY["coldplate"]["type"] == "FlexPCB"
    locations = {s["location"] for s in ld.LEAK_SENSORS}
    assert len(locations) == 2, "each sensor needs a distinct location"


def test_names_are_location_led():
    """location is the first token of the name, matching the convention the
    other in-tree BMC platform uses (location = name.split('_', 1)[0])."""
    for s in ld.LEAK_SENSORS:
        assert s["name"].split("_", 1)[0] == s["location"].lower()


def test_each_sensor_maps_to_a_distinct_line():
    lines = [s["line"] for s in ld.LEAK_SENSORS]
    assert len(set(lines)) == len(lines), "two sensors cannot share a GPIO line"
    assert set(lines) == {ld.LINE_DRIPTRAY, ld.LINE_COLDPLATE}


def test_line_for_rejects_unknown_sensor():
    with pytest.raises(KeyError):
        ld.line_for("nosuch")


def test_line_for_returns_the_configured_line():
    assert ld.line_for("driptray") == ld.LINE_DRIPTRAY
    assert ld.line_for("coldplate") == ld.LINE_COLDPLATE


# --------------------------------------------------------------------------
# gpioget output parsing
# --------------------------------------------------------------------------

@pytest.mark.parametrize("out", [
    "1", "1\n", " 1 ", "active", "ACTIVE", "active\n", "true", "high",
    'LEAK_FAULT_0=1', '"LEAK_FAULT_0"=1', '"LEAK_FAULT_0"=active',
    "LEAK_FAULT_0=active\n",
])
def test_parse_asserted_forms(out):
    """libgpiod v2 prints active/inactive by default, 0/1 under --numeric, and
    either a bare value or <name>=<value> depending on version and line count.
    All of those have to decode."""
    assert ld.parse_line_value(out) is True


@pytest.mark.parametrize("out", [
    "0", "0\n", " 0 ", "inactive", "INACTIVE", "false", "low",
    'LEAK_FAULT_0=0', '"LEAK_FAULT_0"=inactive',
])
def test_parse_deasserted_forms(out):
    assert ld.parse_line_value(out) is False


@pytest.mark.parametrize("out", [
    None, "", "   ", "\n", "banana", "LEAK_FAULT_0=banana", "2", "=",
])
def test_unparseable_output_raises(out):
    """The important property: an output we do not understand must raise, never
    quietly decode as no-leak. A silent False here would hide a real leak."""
    with pytest.raises(ld.LeakLineError):
        ld.parse_line_value(out)


def test_is_leaking_follows_polarity():
    """Both lines are active high, so asserted means wet."""
    assert ld.LINE_ACTIVE_HIGH is True
    assert ld.is_leaking("1") is True
    assert ld.is_leaking("0") is False


def test_is_leaking_propagates_parse_failure():
    with pytest.raises(ld.LeakLineError):
        ld.is_leaking("banana")
