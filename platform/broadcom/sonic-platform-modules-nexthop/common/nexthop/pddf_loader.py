#!/usr/bin/env python3

"""
PDDF configuration loader with platform agnostic dependencies
"""

import json

PDDF_DEVICE_JSON_PATH = "/usr/share/sonic/platform/pddf/pddf-device.json"


def load_pddf_device_config():
    """Load and parse pddf-device.json configuration. Raises exception on error."""
    with open(PDDF_DEVICE_JSON_PATH, "r") as f:
        config = json.load(f)
    return config

