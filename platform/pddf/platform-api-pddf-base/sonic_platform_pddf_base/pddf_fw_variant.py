#############################################################################
# PDDF
#
# Generic firmware-variant selector for PDDF platforms.
#
# A single component slot can be populated by parts from different
# vendors/variants (e.g. an SSD slot holding a Virtium or a Swissbit drive, a
# transceiver cage holding optics from different makers, a dual-sourced PSU).
# Each variant typically needs a *different* firmware image and reports a
# *different* target version.
#
# This module provides a small, dependency-free helper that maps an installed
# part -- identified at runtime by the platform (model string, vendor name,
# part number, running firmware version, ...) -- to the firmware image and
# target version it should use, from a JSON manifest. It is intentionally
# device-class agnostic: the *detection* of the identity is left to the
# platform (smartctl for SSDs, EEPROM/`ethtool -m` for optics, an I2C/PMBus
# read for PSUs, etc.), while the *matching* logic is shared here.
#
# Standard manifest schema
# ------------------------
#   {
#     "variants": [
#       {
#         "variant":        "Virtium",                 # human label (optional)
#         "id_match":       ["VIRTIUM", "VTSM"],       # case-insensitive substrings of the identity
#         "version_prefix": ["0710"],                  # fallback: running-version prefixes
#         "image":          "/usr/sbin/fw/...bin",     # image to flash
#         "version":        "0710-001"                 # target version to report as "available"
#       }
#     ],
#     "default": null                                  # optional entry when nothing matches
#   }
#
# Matching order:
#   1) any "id_match" substring (case-insensitive) found in the identity,
#   2) any "version_prefix" that the running firmware version starts with,
#   3) the manifest "default" (may be null/None).
#
# Backward-compatible aliases (so existing platform manifests keep working):
#   top-level "models"   == "variants"
#   per-entry "model_match" == "id_match"
#   per-entry label "vendor" / "name" == "variant"
#
# Copyright (c) 2026, Hewlett Packard Enterprise Development LP.
# SPDX-License-Identifier: Apache-2.0
#############################################################################

try:
    import os
    import json
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")


# Accepted top-level keys holding the list of variant entries.
_LIST_KEYS = ("variants", "models")
# Accepted per-entry keys holding the identity-match substrings.
_ID_MATCH_KEYS = ("id_match", "model_match")
# Accepted per-entry keys holding the human-readable label.
_LABEL_KEYS = ("variant", "vendor", "name")


def load_manifest_file(path):
    """
    Load and return a manifest dict from a JSON file path.

    Returns {} on any error (missing file, bad JSON, wrong shape) so callers
    can degrade gracefully without raising.
    """
    if not path or not os.path.isfile(path):
        return {}
    try:
        with open(path, "r") as handle:
            data = json.load(handle)
        return data if isinstance(data, dict) else {}
    except Exception:
        return {}


def load_manifest(path_hint, manifest_filename=None):
    """
    Resolve and load a manifest from a hint that may be:
      - a directory containing `manifest_filename`,
      - a direct path to a .json manifest,
      - None / empty (returns {}).

    Returns the manifest dict, or {} when nothing usable is found.
    """
    if not path_hint:
        return {}

    hint = path_hint.rstrip()
    if os.path.isdir(hint):
        if not manifest_filename:
            return {}
        return load_manifest_file(os.path.join(hint, manifest_filename))
    if hint.endswith(".json"):
        return load_manifest_file(hint)
    return {}


def iter_variants(manifest):
    """Return the list of variant entries from a manifest (handles aliases)."""
    if not isinstance(manifest, dict):
        return []
    for key in _LIST_KEYS:
        value = manifest.get(key)
        if isinstance(value, list):
            return value
    return []


def _entry_field(entry, keys, default=None):
    if not isinstance(entry, dict):
        return default
    for key in keys:
        if key in entry and entry[key] is not None:
            return entry[key]
    return default


def variant_label(entry):
    """Human-readable label for a matched entry (variant/vendor/name)."""
    return _entry_field(entry, _LABEL_KEYS, default="")


def variant_image(entry):
    """Firmware image path for a matched entry."""
    return _entry_field(entry, ("image",), default="")


def variant_version(entry):
    """Target ('available') firmware version for a matched entry."""
    return _entry_field(entry, ("version",), default="")


def select_variant(manifest, identity, running_version=""):
    """
    Select the manifest entry for the installed part.

    Args:
        manifest:        manifest dict (from load_manifest*).
        identity:        identity string for the installed part, e.g. the
                         model/vendor/part-number reported by the platform.
        running_version: currently-running firmware version (optional), used
                         only for the version_prefix fallback.

    Matching order: id_match substring -> version_prefix -> manifest 'default'.

    Returns:
        The matched entry dict, or the manifest 'default' (which may be None).
    """
    entries = iter_variants(manifest)
    identity_uc = (identity or "").upper()
    version_uc = (running_version or "").upper()

    # 1) identity substring match
    if identity_uc:
        for entry in entries:
            for needle in _entry_field(entry, _ID_MATCH_KEYS, default=[]) or []:
                if needle and needle.upper() in identity_uc:
                    return entry

    # 2) running-version prefix fallback
    if version_uc:
        for entry in entries:
            for prefix in (entry.get("version_prefix") if isinstance(entry, dict) else None) or []:
                if prefix and version_uc.startswith(prefix.upper()):
                    return entry

    # 3) default (may be None)
    if isinstance(manifest, dict):
        return manifest.get("default")
    return None
