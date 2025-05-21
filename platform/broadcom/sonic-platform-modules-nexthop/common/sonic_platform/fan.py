#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

try:
    from sonic_platform_pddf_base.pddf_fan import PddfFan
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Fan(PddfFan):
    """PDDF Platform-Specific Fan class"""

    def __init__(
        self,
        tray_idx,
        fan_idx=0,
        pddf_data=None,
        pddf_plugin_data=None,
        is_psu_fan=False,
        psu_index=0,
    ):
        # idx is 0-based
        PddfFan.__init__(
            self, tray_idx, fan_idx, pddf_data, pddf_plugin_data, is_psu_fan, psu_index
        )

    # Provide the functions/variables below for which implementation is to be overwritten
