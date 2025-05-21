#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

from sonic_platform_base.sonic_thermal_control.thermal_condition_base import ThermalPolicyConditionBase
from sonic_platform_base.sonic_thermal_control.thermal_json_object import thermal_json_object
from .thermal_infos import FanInfo

class FanCondition(ThermalPolicyConditionBase):
    def get_fan_info(self, thermal_info_dict) -> FanInfo:
        """
        Get fan info from thermal dict to determine
        if a fan condition matches
        """
        return thermal_info_dict.get(FanInfo.INFO_TYPE)
    
@thermal_json_object('fan.always.true')
class FanAlwaysTrueCondition(FanCondition):
    """
    Dummy condition that is always true
    """
    def is_match(self, thermal_info_dict):
        return True