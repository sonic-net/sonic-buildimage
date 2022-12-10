from sonic_platform_base.sonic_thermal_control.thermal_manager_base import ThermalManagerBase
from .common import Common
from .thermal_actions import *
from .thermal_conditions import *
from .thermal_infos import *


class ThermalManager(ThermalManagerBase):
    FSC_ALGORITHM_CMD = ['supervisorctl', '', 'fancontrol']

    @classmethod
    def start_thermal_control_algorithm(cls):
        """
        Start vendor specific thermal control algorithm. The default behavior of this function is a no-op.
        :return:
        """
        return cls._enable_fancontrol_service(True)

    @classmethod
    def stop_thermal_control_algorithm(cls):
        """
        Stop thermal control algorithm
        Returns:
            bool: True if set success, False if fail.
        """
        return cls._enable_fancontrol_service(False)

    @classmethod
    def deinitialize(cls):
        """
        Destroy thermal manager, including any vendor specific cleanup. The default behavior of this function
        is a no-op.
        :return:
        """
        return cls._enable_fancontrol_service(True)

    @classmethod
    def _enable_fancontrol_service(cls, enable):
        """
        Control thermal by fcs algorithm
        Args:
            enable: Bool, indicate enable the algorithm or not
        Returns:
            bool: True if set success, False if fail.
        """
        cls.FSC_ALGORITHM_CMD[1] = 'start' if enable else 'stop'
        return Common().run_command(cls.FSC_ALGORITHM_CMD)
