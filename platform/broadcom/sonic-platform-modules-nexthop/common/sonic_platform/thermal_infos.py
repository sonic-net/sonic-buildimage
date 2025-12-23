#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

from sonic_platform_base.sonic_thermal_control.thermal_info_base import ThermalPolicyInfoBase
from sonic_platform_base.sonic_thermal_control.thermal_json_object import thermal_json_object

# For reference where these items come from
from .chassis import Chassis
from .thermal import Thermal
from .thermal_manager import ThermalManager
from .fan import Fan
from .fan_drawer import FanDrawer
from .psu import Psu

@thermal_json_object('fan_drawer_info')
class FanDrawerInfo(ThermalPolicyInfoBase):
    """Fan information for all fan drawers"""
    INFO_TYPE = 'fan_drawer_info'
    def __init__(self):
        self._fans = []
        self._fan_drawers = []
    
    def collect(self, chassis: Chassis):
        self._fans = chassis.get_all_fans()[:]
        self._fan_drawers = chassis.get_all_fan_drawers()[:]

    def get_fans(self)->list[Fan]:
        return self._fans

    def get_fan_drawers(self)->list[FanDrawer]:
        return self._fan_drawers
    
    def get_num_present_fan_drawers(self):
        return sum([fan_drawer.get_presence() for fan_drawer in self._fan_drawers])
    
@thermal_json_object('thermal_info')
class ThermalInfo(ThermalPolicyInfoBase):
    INFO_TYPE = 'thermal_info'
    def __init__(self):
        self._thermals = []
        self._thermal_manager = None
    
    def collect(self, chassis: Chassis):
        self._thermals = chassis.get_all_thermals()[:]
        for sfp in chassis.get_all_sfps():
            self._thermals.extend(sfp.get_all_thermals())
        self._thermal_manager = chassis.get_thermal_manager()
    
    def get_thermals(self) -> list[Thermal]:
        return self._thermals
    
    def get_thermal_manager(self) -> ThermalManager:
        return self._thermal_manager

@thermal_json_object('psu_info')
class PsuInfo(ThermalPolicyInfoBase):
    """PSUs have separate fans and thermals from the chassis"""
    INFO_TYPE = 'psu_info'
    def __init__(self):
        self._psus = []
    
    def collect(self, chassis: Chassis):
        self._psus = chassis.get_all_psus()[:]
    
    def get_thermals(self)->list[Psu]:
        return self._psus.get_all_thermals()
    
    def get_fans(self)->list[Psu]:
        return self._psus.get_all_fans()
    
    
