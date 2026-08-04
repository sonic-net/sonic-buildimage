from sonic_platform_base.sonic_thermal_control.thermal_manager_base import ThermalManagerBase
from .thermal_actions import *
from .thermal_conditions import *
from .thermal_infos import *


class ThermalManager(ThermalManagerBase):
    THERMAL_ALGORITHM_CONTROL_PATH = '/var/run/hw-management/config/suspend'

    @classmethod
    def start_thermal_control_algorithm(cls):
        """
        Start thermal control algorithm

        Returns:
            bool: True if set success, False if fail.
        """
        cls._control_thermal_control_algorithm(False)

    @classmethod
    def stop_thermal_control_algorithm(cls):
        """
        Stop thermal control algorithm

        Returns:
            bool: True if set success, False if fail.
        """
        cls._control_thermal_control_algorithm(True)

    @classmethod
    def _control_thermal_control_algorithm(cls, suspend):
        """
        Control thermal control algorithm

        Args:
            suspend: Bool, indicate suspend the algorithm or not

        Returns:
            bool: True if set success, False if fail.
        """
        status = True
        write_value = 1 if suspend else 0
        try:
            with open(cls.THERMAL_ALGORITHM_CONTROL_PATH, 'w') as control_file:
                control_file.write(str(write_value))
        except (ValueError, IOError):
            status = False

        return status
