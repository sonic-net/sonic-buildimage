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

    def get_model(self):
        if self.get_presence() and not self.is_psu_fan:
            return "FAN-80G1-F"
        return "N/A"

    def get_serial(self):
        return "N/A"

    def get_psu_presence(self):
        if not self.is_psu_fan:
            return False

        status = 0
        device = "PSU{}".format(self.fans_psu_index)
        output = self.pddf_obj.get_attr_name_output(device, "psu_present")
        if not output:
            return False

        mode = output['mode']
        status = output['status']

        vmap = self.plugin_data['PSU']['psu_present'][mode]['valmap']

        if status.rstrip('\n') in vmap:
            return vmap[status.rstrip('\n')]
        else:
            return False

    def get_presence(self):
        if self.is_psu_fan:
            return self.get_psu_presence()
        return PddfFan.get_presence(self)
