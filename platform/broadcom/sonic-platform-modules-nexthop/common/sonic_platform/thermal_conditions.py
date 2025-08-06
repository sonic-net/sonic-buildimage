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

@thermal_json_object('fan.two.or.fewer.present')
class FanTwoOrFewerPresentCondition(FanCondition):
    """
    Condition if two or fewer fantray fans are present
    """
    def is_match(self, thermal_info_dict: dict) -> bool:
        fan_info = self.get_fan_info(thermal_info_dict)
        return fan_info.get_num_present_fans() <= 2
    
@thermal_json_object('default.operation')
class ThermalControlAlgorithmCondition(FanCondition):
    """
    Default case, will be the complement of all other conditions
    """
    def is_match(self, thermal_info_dict):
        fan_info = self.get_fan_info(thermal_info_dict)
        return fan_info.get_num_present_fans() > 2