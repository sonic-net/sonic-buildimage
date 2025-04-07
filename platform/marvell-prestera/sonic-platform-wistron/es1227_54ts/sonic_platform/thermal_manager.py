from sonic_platform_base.sonic_thermal_control.thermal_manager_base import ThermalManagerBase
from .thermal_actions import *
from .thermal_conditions import *
from .thermal_infos import *


class ThermalManager(ThermalManagerBase):
    @classmethod
    def initialize(cls):
        pass
