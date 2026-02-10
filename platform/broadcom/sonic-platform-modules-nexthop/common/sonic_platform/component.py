#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

try:
    from sonic_platform_pddf_base.pddf_component import PddfComponent
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Component(PddfComponent):
    """Platform-specific Component class. Used for programmables"""

    def __init__(self, idx, pddf_data, pddf_plugin_data=None):
        PddfComponent.__init__(self, idx, pddf_data, pddf_plugin_data)
