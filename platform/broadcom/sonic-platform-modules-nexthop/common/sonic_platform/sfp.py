#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

from nexthop.xcvr_presence_cache import read_cached_presence
from sonic_platform.syslog import SYSLOG_IDENTIFIER_SFP, NhLoggerMixin

try:
    from sonic_platform_pddf_base.pddf_sfp import PddfSfp
    from sonic_platform.thermal import SfpThermal
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Sfp(PddfSfp, NhLoggerMixin):
    """
    PDDF Platform-Specific Sfp class
    """

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        PddfSfp.__init__(self, index, pddf_data, pddf_plugin_data)
        NhLoggerMixin.__init__(self, SYSLOG_IDENTIFIER_SFP)
        self._thermal_list.append(SfpThermal(self, pddf_data))

    def get_presence(self):
        cached_presence = read_cached_presence(
            self.port_index, log_warning=self.log_warning
        )
        if cached_presence is not None:
            return cached_presence
        return super().get_presence()

    def get_error_description(self):
        try:
            return super().get_error_description()
        except NotImplementedError:
            if not self.get_presence():
                return self.SFP_STATUS_UNPLUGGED
            return self.SFP_STATUS_OK
