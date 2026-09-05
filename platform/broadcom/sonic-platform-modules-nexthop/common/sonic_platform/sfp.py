#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

try:
    from sonic_platform_pddf_base.pddf_sfp import PddfSfp
    from sonic_platform.thermal import SfpThermal
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Sfp(PddfSfp):
    """
    PDDF Platform-Specific Sfp class
    """

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        PddfSfp.__init__(self, index, pddf_data, pddf_plugin_data)
        self._thermal_list.append(SfpThermal(self, pddf_data))

    # Provide the functions/variables below for which implementation is to be overwritten
    def get_error_description(self):
        try:
            return super().get_error_description()
        except NotImplementedError:
            if not self.get_presence():
                return self.SFP_STATUS_UNPLUGGED
            return self.SFP_STATUS_OK
