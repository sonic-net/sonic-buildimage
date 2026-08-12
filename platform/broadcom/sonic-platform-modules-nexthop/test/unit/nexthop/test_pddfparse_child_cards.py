#!/usr/bin/env python

# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for pddfparse's CHILD_CARDS merge helpers.

These cover the free functions ``merge_fragment``, ``attach_to_parent`` and
``_unique_matching_variant`` -- the boot-critical sensor-renumbering, collision
detection and variant-matching logic -- with plain dicts, no hardware.
"""

import json
import os
import sys
import types
import pytest

# pddfparse lives in the shared PDDF utils dir, not the nexthop package.
_PDDF_UTILS = os.path.abspath(
    os.path.join(os.path.dirname(__file__), "../../../../../pddf/i2c/utils")
)


@pytest.fixture(scope="function", autouse=True)
def pddfparse_module(monkeypatch):
    """Import pddfparse under the mocked deps injected by conftest."""
    # conftest mocks sonic_platform_pddf_base.pddf_fpga_utils but not
    # pddf_platform_hooks; provide a concrete stub so the import resolves.
    hooks = types.ModuleType("sonic_platform_pddf_base.pddf_platform_hooks")

    class ChildCardEepromUnprogrammed(Exception):
        pass

    hooks.ChildCardEepromUnprogrammed = ChildCardEepromUnprogrammed
    monkeypatch.setitem(
        sys.modules, "sonic_platform_pddf_base.pddf_platform_hooks", hooks
    )
    if _PDDF_UTILS not in sys.path:
        sys.path.insert(0, _PDDF_UTILS)
    monkeypatch.delitem(sys.modules, "pddfparse", raising=False)
    import pddfparse

    yield pddfparse


# --- _unique_matching_variant ---------------------------------------------


def test_variant_one_match(pddfparse_module):
    variants = [
        {"match": {"vendor": "A"}, "pddf_json": "a.j2"},
        {"match": {"vendor": "B"}, "pddf_json": "b.j2"},
    ]
    # 'slot' in the record is ignored -- match is a filter, not full equality.
    got = pddfparse_module._unique_matching_variant(variants, {"vendor": "B", "slot": 3})
    assert got["pddf_json"] == "b.j2"


def test_variant_zero_matches(pddfparse_module):
    with pytest.raises(ValueError, match="no variant matched"):
        pddfparse_module._unique_matching_variant(
            [{"match": {"vendor": "A"}}], {"vendor": "Z"}
        )


def test_variant_two_matches(pddfparse_module):
    variants = [{"match": {"vendor": "A"}}, {"match": {"vendor": "A"}}]
    with pytest.raises(ValueError, match="2 variants matched"):
        pddfparse_module._unique_matching_variant(variants, {"vendor": "A"})


def test_variant_empty_or_missing_match(pddfparse_module):
    with pytest.raises(ValueError, match="empty or missing"):
        pddfparse_module._unique_matching_variant([{"match": {}}], {"vendor": "A"})
    with pytest.raises(ValueError, match="empty or missing"):
        pddfparse_module._unique_matching_variant([{"pddf_json": "x"}], {"vendor": "A"})


# --- merge_fragment: sensor renumbering ------------------------------------


def test_sensor_renumber_arithmetic(pddfparse_module):
    data = {"PLATFORM": {"num_temps": 17, "num_voltage_sensors": 5, "num_current_sensors": 2}}
    fragment = {"devices": {
        "TEMP1": {"dev_info": {"device_type": "TEMP_SENSOR"}},
        "VOLTAGE1": {"dev_info": {"device_type": "VOLTAGE_SENSOR"}},
        "CURRENT1": {"dev_info": {"device_type": "CURRENT_SENSOR"}},
    }}
    pddfparse_module.merge_fragment(data, None, {}, fragment)
    assert "TEMP18" in data and "TEMP1" not in data      # 1 + 17
    assert "VOLTAGE6" in data                             # 1 + 5
    assert "CURRENT3" in data                             # 1 + 2


def test_non_sensor_device_kept_as_is(pddfparse_module):
    data = {"PLATFORM": {"num_temps": 9}}
    fragment = {"devices": {"CPLDMUX7": {"dev_info": {"device_type": "CPLDMUX"}}}}
    pddfparse_module.merge_fragment(data, None, {}, fragment)
    assert "CPLDMUX7" in data


def test_sensor_key_not_matching_prefix_raises(pddfparse_module):
    data = {"PLATFORM": {"num_temps": 0}}
    fragment = {"devices": {"TEMPX": {"dev_info": {"device_type": "TEMP_SENSOR"}}}}
    with pytest.raises(ValueError, match="does not match expected"):
        pddfparse_module.merge_fragment(data, None, {}, fragment)


def test_device_name_collision_raises(pddfparse_module):
    data = {"PLATFORM": {"num_temps": 0}, "CPLDMUX1": {"pre": "existing"}}
    fragment = {"devices": {"CPLDMUX1": {"dev_info": {"device_type": "CPLDMUX"}}}}
    with pytest.raises(ValueError, match="collides"):
        pddfparse_module.merge_fragment(data, None, {}, fragment)


# --- merge_fragment: counts / thermals / i2c clients -----------------------


def test_platform_counts_bumped(pddfparse_module):
    data = {"PLATFORM": {"num_temps": 17}}
    pddfparse_module.merge_fragment(
        data, None, {}, {"platform_counts": {"num_temps": 3, "num_new": 2}}
    )
    assert data["PLATFORM"]["num_temps"] == 20
    assert data["PLATFORM"]["num_new"] == 2


def test_thermals_appended(pddfparse_module):
    data = {"PLATFORM": {}}
    platform_json = {"chassis": {"thermals": [{"name": "T1"}]}}
    fragment = {"platform_thermals": [{"name": "T2"}, {"name": "T3"}]}
    pddfparse_module.merge_fragment(data, platform_json, {}, fragment)
    names = [t["name"] for t in platform_json["chassis"]["thermals"]]
    assert names == ["T1", "T2", "T3"]


def test_thermal_name_already_in_platform_json_raises(pddfparse_module):
    # platform.json.base pre-declaring a fragment's thermal is a config bug:
    # platform_counts still bumps num_temps, so dropping the entry would leave
    # the API enumerating more thermals than platform.json lists.
    data = {"PLATFORM": {}}
    platform_json = {"chassis": {"thermals": [{"name": "IBV0_TEMP"}]}}
    fragment = {"platform_counts": {"num_temps": 1},
                "platform_thermals": [{"name": "IBV0_TEMP"}]}
    with pytest.raises(ValueError, match="already declared"):
        pddfparse_module.merge_fragment(data, platform_json, {}, fragment)


def test_two_fragments_same_thermal_name_raises(pddfparse_module):
    # Second slot's fragment emitting slot 0's thermal name (e.g. a fragment
    # that forgot to interpolate {{ slot }}) must not be silently swallowed.
    data = {"PLATFORM": {}}
    platform_json = {"chassis": {"thermals": []}}
    fragment = {"platform_thermals": [{"name": "IBV0_TEMP"}]}
    pddfparse_module.merge_fragment(data, platform_json, {}, fragment)
    with pytest.raises(ValueError, match="already declared"):
        pddfparse_module.merge_fragment(data, platform_json, {}, fragment)


def test_thermals_without_name_key_still_appended(pddfparse_module):
    # Nameless entries carry no identity to collide on -- must not raise.
    data = {"PLATFORM": {}}
    platform_json = {"chassis": {"thermals": [{"no_name": 1}]}}
    fragment = {"platform_thermals": [{"no_name": 2}, {"no_name": 3}]}
    pddfparse_module.merge_fragment(data, platform_json, {}, fragment)
    assert len(platform_json["chassis"]["thermals"]) == 3


def test_thermals_ignored_when_no_platform_json(pddfparse_module):
    data = {"PLATFORM": {}}
    # platform_json is None -> thermals silently skipped, no crash.
    added = pddfparse_module.merge_fragment(
        data, None, {}, {"platform_thermals": [{"name": "T"}]}
    )
    assert added == []


def test_i2c_clients_attached_to_parent(pddfparse_module):
    data = {
        "PLATFORM": {},
        "CPLDMUX0": {"i2c": {"channel": [{"chan": 3, "dev": []}]}},
    }
    fragment = {"devices": {"IBV_DCDC0": {
        "dev_info": {"device_type": "IBV"},
        "i2c": {"topo_info": {"parent_bus": "0x1", "dev_addr": "0x40"}},
    }}}
    card = {"parent": "CPLDMUX0", "parent_chan": 3}
    added = pddfparse_module.merge_fragment(data, None, card, fragment)
    assert added == ["IBV_DCDC0"]
    assert data["CPLDMUX0"]["i2c"]["channel"][0]["dev"] == ["IBV_DCDC0"]


# --- attach_to_parent ------------------------------------------------------


def test_attach_unknown_parent_raises(pddfparse_module):
    with pytest.raises(KeyError, match="not in pddf-device.json"):
        pddfparse_module.attach_to_parent({}, "CPLDMUXX", "1", ["X"])


def test_attach_unknown_channel_raises(pddfparse_module):
    data = {"CPLDMUX0": {"i2c": {"channel": [{"chan": 1, "dev": []}]}}}
    with pytest.raises(KeyError, match="has no channel"):
        pddfparse_module.attach_to_parent(data, "CPLDMUX0", "9", ["X"])


def test_attach_is_idempotent(pddfparse_module):
    data = {"CPLDMUX0": {"i2c": {"channel": [{"chan": 1, "dev": ["A"]}]}}}
    pddfparse_module.attach_to_parent(data, "CPLDMUX0", "1", ["A", "B"])
    assert data["CPLDMUX0"]["i2c"]["channel"][0]["dev"] == ["A", "B"]


# --- _normalize_chassis_thermals_order -------------------------------------


def _bare_parse(pddfparse_module, data, platform_json):
    """A PddfParse with data/_platform_json set but __init__ (file I/O) skipped."""
    obj = pddfparse_module.PddfParse.__new__(pddfparse_module.PddfParse)
    obj.data = data
    obj._platform_json = platform_json
    return obj


def test_normalize_reorders_to_api_enumeration(pddfparse_module):
    # API order = TEMP1, TEMP2 then ASIC_TEMP1, by dev_attr.display_name.
    data = {
        "PLATFORM": {"num_temps": 2, "num_asic_temps": 1},
        "TEMP1": {"dev_attr": {"display_name": "Inlet"}},
        "TEMP2": {"dev_attr": {"display_name": "Outlet"}},
        "ASIC_TEMP1": {"dev_attr": {"display_name": "ASIC Diode"}},
    }
    platform_json = {"chassis": {"thermals": [
        {"name": "ASIC Diode"}, {"name": "Outlet"}, {"name": "Inlet"},
    ]}}
    obj = _bare_parse(pddfparse_module, data, platform_json)
    obj._normalize_chassis_thermals_order()
    names = [t["name"] for t in platform_json["chassis"]["thermals"]]
    assert names == ["Inlet", "Outlet", "ASIC Diode"]


def test_normalize_left_as_is_when_name_absent(pddfparse_module):
    # A pddf display_name missing from platform.json -> reorder skipped (guard).
    data = {
        "PLATFORM": {"num_temps": 1, "num_asic_temps": 0},
        "TEMP1": {"dev_attr": {"display_name": "Inlet"}},
    }
    platform_json = {"chassis": {"thermals": [{"name": "Outlet"}, {"name": "Other"}]}}
    obj = _bare_parse(pddfparse_module, data, platform_json)
    obj._normalize_chassis_thermals_order()
    names = [t["name"] for t in platform_json["chassis"]["thermals"]]
    assert names == ["Outlet", "Other"]  # unchanged


# --- expand_child_cards: skip a bad slot, fail loud on a bad config --------


def _wire_expand(pddfparse_module, monkeypatch, tmp_path, child_cards, decode=None):
    """A PddfParse with expand_child_cards' file I/O redirected to tmp_path.

    platform.json.base is deliberately absent, so _platform_json stays None and
    only pddf-device.json is written.
    """
    base = tmp_path / "pddf-device.json.base"
    base.write_text(json.dumps({
        "PLATFORM": {"num_temps": 0},
        "EEPROM_A": {"i2c": {"topo_info": {}}},
        "CHILD_CARDS": child_cards,
    }))
    for const, value in (
        ("PDDF_DEVICE_JSON_BASE", str(base)),
        ("PDDF_DEVICE_JSON_PATH", str(tmp_path / "pddf-device.json")),
        ("PLATFORM_JSON_BASE", str(tmp_path / "absent-platform.json.base")),
        ("PDDF_DIR", str(tmp_path)),
    ):
        monkeypatch.setattr(pddfparse_module, const, value)
    monkeypatch.setattr(
        pddfparse_module, "_load_hooks",
        lambda: types.SimpleNamespace(
            decode_eeprom=decode or (lambda decoder, blob, slot: {"vendor": "ACME"})
        ),
    )
    obj = _bare_parse(pddfparse_module, {}, None)
    obj.create_subtree = lambda name: 0
    return obj


def _card(**overrides):
    card = {
        "eeprom_device": "EEPROM_A",
        "decoder": "d.v1",
        "slot": 0,
        "variants": [{"match": {"vendor": "ACME"}, "pddf_json": "frag.json"}],
    }
    card.update(overrides)
    return card


def test_expand_skips_slot_whose_eeprom_read_fails(pddfparse_module, monkeypatch, tmp_path):
    # One unreadable slot must not cost us the healthy ones.
    (tmp_path / "frag.json").write_text(json.dumps({
        "platform_counts": {"num_temps": 1},
        "devices": {"TEMP1": {"dev_info": {"device_type": "TEMP_SENSOR"}}},
    }))
    obj = _wire_expand(pddfparse_module, monkeypatch, tmp_path,
                       {"C1": _card(), "C2": _card(slot=1)})
    calls = []

    def _read(eeprom_device):
        calls.append(eeprom_device)
        if len(calls) == 1:
            raise OSError("ENXIO: i2c read failed")
        return b"\x00"

    obj._read_eeprom = _read
    obj.expand_child_cards()

    written = json.loads((tmp_path / "pddf-device.json").read_text())
    assert written["PLATFORM"]["num_temps"] == 1  # C1 skipped, C2 merged
    assert "TEMP1" in written


def test_expand_skips_slot_with_unprogrammed_eeprom(pddfparse_module, monkeypatch, tmp_path):
    # A blank EEPROM is a runtime state (card absent/unprogrammed), not a bug.
    def _decode(decoder, blob, slot):
        raise pddfparse_module.ChildCardEepromUnprogrammed("no FRU records")

    obj = _wire_expand(pddfparse_module, monkeypatch, tmp_path,
                       {"C1": _card()}, decode=_decode)
    obj._read_eeprom = lambda eeprom_device: b"\x00"
    obj.expand_child_cards()

    written = json.loads((tmp_path / "pddf-device.json").read_text())
    assert written["PLATFORM"]["num_temps"] == 0


def test_expand_raises_when_card_entry_missing_key(pddfparse_module, monkeypatch, tmp_path):
    card = _card()
    del card["slot"]
    obj = _wire_expand(pddfparse_module, monkeypatch, tmp_path, {"C1": card})
    obj._read_eeprom = lambda eeprom_device: b"\x00"
    with pytest.raises(pddfparse_module.ChildCardConfigError, match="missing required"):
        obj.expand_child_cards()


def test_expand_raises_on_undeclared_eeprom_device(pddfparse_module, monkeypatch, tmp_path):
    # A typo'd eeprom_device is a pddf-device.json bug: it must not be reported
    # as an EEPROM read failure and degraded to a silently missing card.
    obj = _wire_expand(pddfparse_module, monkeypatch, tmp_path,
                       {"C1": _card(eeprom_device="EEPROM_TYPO")})
    with pytest.raises(pddfparse_module.ChildCardConfigError, match="not defined in"):
        obj.expand_child_cards()
