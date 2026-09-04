#!/usr/bin/env python
# Copyright (c) 2026, Hewlett Packard Enterprise Development LP.
# SPDX-License-Identifier: Apache-2.0
"""
Unit tests for sonic_platform_pddf_base.pddf_fw_variant -- the generic,
device-class-agnostic firmware-variant selector shared by PDDF platforms.
"""

import json
import os
import tempfile

from sonic_platform_pddf_base import pddf_fw_variant as fwv


CANONICAL_MANIFEST = {
    "variants": [
        {
            "variant": "Virtium",
            "id_match": ["VIRTIUM", "VTSM"],
            "version_prefix": ["0710"],
            "image": "/usr/sbin/fw/Virtium_ISP.bin",
            "version": "0710-001",
        },
        {
            "variant": "Swissbit",
            "id_match": ["SWISSBIT", "SFSA"],
            "version_prefix": ["SBR"],
            "image": "/usr/sbin/fw/swissbit_ISP.bin",
            "version": "SBR10021",
        },
    ],
    "default": None,
}

# Same content using the legacy aliases (models / vendor / model_match).
ALIAS_MANIFEST = {
    "models": [
        {
            "vendor": "Virtium",
            "model_match": ["VIRTIUM", "VTSM"],
            "version_prefix": ["0710"],
            "image": "/usr/sbin/fw/Virtium_ISP.bin",
            "version": "0710-001",
        },
    ],
    "default": None,
}


class TestSelectVariant:
    def test_match_by_identity_substring(self):
        e = fwv.select_variant(CANONICAL_MANIFEST, "Virtium VTSM24 200GB", "0710-001")
        assert fwv.variant_label(e) == "Virtium"
        assert fwv.variant_version(e) == "0710-001"
        assert fwv.variant_image(e) == "/usr/sbin/fw/Virtium_ISP.bin"

    def test_match_is_case_insensitive(self):
        e = fwv.select_variant(CANONICAL_MANIFEST, "swissbit sfsa200", "")
        assert fwv.variant_label(e) == "Swissbit"

    def test_version_prefix_fallback_when_model_unknown(self):
        # Identity does not match any id_match; running version 'SBR...' selects Swissbit.
        e = fwv.select_variant(CANONICAL_MANIFEST, "GENERIC SSD", "SBR10009")
        assert fwv.variant_label(e) == "Swissbit"

    def test_identity_takes_priority_over_version(self):
        # Virtium identity but a Swissbit-looking version -> identity wins.
        e = fwv.select_variant(CANONICAL_MANIFEST, "VIRTIUM drive", "SBR9999")
        assert fwv.variant_label(e) == "Virtium"

    def test_no_match_returns_default_none(self):
        assert fwv.select_variant(CANONICAL_MANIFEST, "FOO BAR", "ZZ.1") is None

    def test_default_entry_used_when_present(self):
        manifest = dict(CANONICAL_MANIFEST)
        sentinel = {"variant": "Generic", "image": "/x.bin", "version": "9.9"}
        manifest = {"variants": CANONICAL_MANIFEST["variants"], "default": sentinel}
        assert fwv.select_variant(manifest, "unknown", "unknown") is sentinel

    def test_alias_schema_models_vendor_model_match(self):
        e = fwv.select_variant(ALIAS_MANIFEST, "Virtium VTSM", "")
        assert fwv.variant_label(e) == "Virtium"        # 'vendor' alias
        assert fwv.variant_version(e) == "0710-001"

    def test_empty_manifest(self):
        assert fwv.select_variant({}, "anything", "1.0") is None
        assert fwv.iter_variants({}) == []


class TestManifestLoading:
    def test_load_manifest_file(self):
        with tempfile.TemporaryDirectory() as d:
            path = os.path.join(d, "m.json")
            with open(path, "w") as f:
                json.dump(CANONICAL_MANIFEST, f)
            loaded = fwv.load_manifest_file(path)
            assert fwv.iter_variants(loaded)
            assert fwv.select_variant(loaded, "VTSM", "") is not None

    def test_load_manifest_from_dir(self):
        with tempfile.TemporaryDirectory() as d:
            path = os.path.join(d, "fw_data.json")
            with open(path, "w") as f:
                json.dump(CANONICAL_MANIFEST, f)
            loaded = fwv.load_manifest(d, "fw_data.json")
            assert fwv.iter_variants(loaded)

    def test_load_manifest_missing_returns_empty(self):
        assert fwv.load_manifest_file("/no/such/file.json") == {}
        assert fwv.load_manifest("/no/such/dir", "x.json") == {}
        assert fwv.load_manifest(None) == {}

    def test_load_manifest_bad_json_returns_empty(self):
        with tempfile.TemporaryDirectory() as d:
            path = os.path.join(d, "bad.json")
            with open(path, "w") as f:
                f.write("{ not valid json ")
            assert fwv.load_manifest_file(path) == {}
