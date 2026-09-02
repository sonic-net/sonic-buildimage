#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Unit tests to verify device configuration files exist in all required directories.
"""

import os
import json
import re
import sys

import pytest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "lib"))

from platform_discovery import (
    DEVICE_DIR,
    PLATFORMS_WITH_PHY_CONTEXT,
    discover_platforms,
    build_hwsku_map,
    get_all_platform_paths,
    get_all_hwsku_paths,
)


class TestContextConfigExists:
    """Test that context_config.json exists in all HWSKU directories."""

    @pytest.mark.parametrize(
        "platform_variant,hwsku,hwsku_path",
        get_all_hwsku_paths(),
        ids=[f"{p}/{h}" for p, h, _ in get_all_hwsku_paths()],
    )
    def test_context_config_exists(self, platform_variant, hwsku, hwsku_path):
        """Verify context_config.json exists (as file or symlink) in each HWSKU directory."""
        config_path = os.path.join(hwsku_path, "context_config.json")
        assert os.path.exists(config_path), (
            f"context_config.json missing in {platform_variant}/{hwsku}"
        )

    @pytest.mark.parametrize(
        "platform_variant,hwsku,hwsku_path",
        get_all_hwsku_paths(),
        ids=[f"{p}/{h}" for p, h, _ in get_all_hwsku_paths()],
    )
    def test_context_config_valid_json(self, platform_variant, hwsku, hwsku_path):
        """Verify context_config.json is valid JSON with required structure."""
        config_path = os.path.join(hwsku_path, "context_config.json")
        if not os.path.exists(config_path):
            pytest.skip(f"context_config.json missing in {platform_variant}/{hwsku}")

        with open(config_path, "r") as f:
            config = json.load(f)

        # Verify required structure
        assert "CONTEXTS" in config, "context_config.json must have CONTEXTS key"
        assert isinstance(config["CONTEXTS"], list), "CONTEXTS must be a list"
        assert len(config["CONTEXTS"]) > 0, "CONTEXTS must have at least one entry"

        # Verify each context has required fields
        for ctx in config["CONTEXTS"]:
            assert "guid" in ctx, "Each context must have a guid"
            assert "name" in ctx, "Each context must have a name"
            if platform_variant in PLATFORMS_WITH_PHY_CONTEXT:
                assert "zmq_enable" in ctx, "Each context must have zmq_enable"
                if ctx["guid"] == 1:
                    assert ctx["zmq_enable"] is False, (
                        "PHY/gbsyncd context must keep zmq_enable disabled"
                    )
            assert "zmq_endpoint" in ctx, "Each context must have zmq_endpoint"
            assert "zmq_ntf_endpoint" in ctx, "Each context must have zmq_ntf_endpoint"
            assert "switches" in ctx, "Each context must have switches"


class TestContextConfigContent:
    """Test context_config.json content for specific platforms."""

    @pytest.mark.parametrize(
        "platform_variant",
        PLATFORMS_WITH_PHY_CONTEXT,
        ids=PLATFORMS_WITH_PHY_CONTEXT,
    )
    def test_phy_context_platforms_have_two_contexts(self, platform_variant):
        """Verify platforms with PHY (e.g., 5010) have the PHY context for gbsyncd."""
        hwsku_map = build_hwsku_map(include_metadata=True)
        if platform_variant not in hwsku_map:
            pytest.skip(f"Platform {platform_variant} not found")

        for hwsku_info in hwsku_map[platform_variant]:
            hwsku = hwsku_info["name"]
            is_virtual = hwsku_info["is_virtual"]
            num_ctx = 2

            if is_virtual:
                num_ctx = 1

            config_path = os.path.join(
                DEVICE_DIR, platform_variant, hwsku, "context_config.json"
            )
            with open(config_path, "r") as f:
                config = json.load(f)

            # Should have 2 contexts: main ASIC (guid 0) and PHY (guid 1)
            assert len(config["CONTEXTS"]) == num_ctx, (
                f"{platform_variant}/{hwsku} should have {num_ctx} contexts (main + PHY)"
            )

            guids = [ctx["guid"] for ctx in config["CONTEXTS"]]
            assert 0 in guids, f"{hwsku} must have context with guid 0 (main ASIC)"
            if not is_virtual:
                assert 1 in guids, f"{hwsku} must have context with guid 1 (PHY)"

    def test_single_asic_platforms_have_one_context(self):
        """Verify single-ASIC platforms (non-PHY) have exactly one context."""
        hwsku_map = build_hwsku_map()
        single_asic_platforms = [
            p for p in discover_platforms() if p not in PLATFORMS_WITH_PHY_CONTEXT
        ]

        for platform_variant in single_asic_platforms:
            for hwsku in hwsku_map.get(platform_variant, []):
                config_path = os.path.join(
                    DEVICE_DIR, platform_variant, hwsku, "context_config.json"
                )
                if not os.path.exists(config_path):
                    continue

                with open(config_path, "r") as f:
                    config = json.load(f)

                assert len(config["CONTEXTS"]) == 1, (
                    f"{platform_variant}/{hwsku} should have exactly 1 context"
                )
                assert config["CONTEXTS"][0]["guid"] == 0, (
                    f"{platform_variant}/{hwsku}: Single context should have guid 0"
                )


class TestThermalPolicyTypes:
    """Every "type" referenced by a thermal_policy.json must have a matching
    @thermal_json_object registration of the right kind (info/condition/action)
    in common/sonic_platform, or thermalctld throws "ThermalJsonObject type <x>
    not found" on every init."""

    SONIC_PLATFORM_DIR = os.path.join(
        os.path.dirname(__file__), "..", "..", "common", "sonic_platform"
    )
    REGISTRATION_RE = re.compile(
        r"@thermal_json_object\(\s*['\"]([^'\"]+)['\"]\s*\)"
    )
    # policy category -> module that must hold the registration
    CATEGORY_MODULES = {
        "infos": "thermal_infos.py",
        "conditions": "thermal_conditions.py",
        "actions": "thermal_actions.py",
    }

    @classmethod
    def registered_types(cls, module):
        with open(os.path.join(cls.SONIC_PLATFORM_DIR, module)) as f:
            return set(cls.REGISTRATION_RE.findall(f.read()))

    @staticmethod
    def referenced_types(policy, category):
        refs = set()
        if category == "infos":
            for info in policy.get("info_types", []):
                refs.add(info["type"])
        else:
            for pol in policy.get("policies", []):
                for entry in pol.get(category, []):
                    refs.add(entry["type"])
        return refs

    @pytest.mark.parametrize("category", sorted(CATEGORY_MODULES))
    @pytest.mark.parametrize(
        "platform_variant,platform_path",
        get_all_platform_paths(),
        ids=[p for p, _ in get_all_platform_paths()],
    )
    def test_thermal_policy_types_are_registered(
        self, platform_variant, platform_path, category
    ):
        policy_path = os.path.join(platform_path, "thermal_policy.json")
        if not os.path.exists(policy_path):
            pytest.skip(f"no thermal_policy.json for {platform_variant}")

        with open(policy_path) as f:
            policy = json.load(f)
        module = self.CATEGORY_MODULES[category]
        missing = self.referenced_types(policy, category) - self.registered_types(
            module
        )
        assert not missing, (
            f"{policy_path} {category} reference thermal object types with no "
            f"@thermal_json_object registration in {module}: {sorted(missing)}"
        )

    def test_thermal_policy_files_found(self):
        assert any(
            os.path.exists(os.path.join(path, "thermal_policy.json"))
            for _, path in get_all_platform_paths()
        ), f"no thermal_policy.json found under {DEVICE_DIR}"
