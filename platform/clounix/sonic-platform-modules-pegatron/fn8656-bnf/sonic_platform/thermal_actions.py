import os
#import sys
import time

from sonic_platform_base.sonic_thermal_control.thermal_action_base import ThermalPolicyActionBase
from sonic_platform_base.sonic_thermal_control.thermal_json_object import thermal_json_object
from sonic_py_common import logger
#from sonic_platform.fault import Fault
from .helper import APIHelper

SYSLOG_IDENTIFIER = 'thermalctld'
helper_logger = logger.Logger(SYSLOG_IDENTIFIER)

PLATFORM_CAUSE_DIR = "/host/reboot-cause/platform"
#ADDITIONAL_FAULT_CAUSE_FILE = os.path.join(PLATFORM_CAUSE_DIR, "additional_fault_cause")
INT_STATUS_FILE = '/sys/devices/pci0000:00/0000:00:1f.3/i2c-0/i2c-1/i2c-6/6-0074/int_status'
ADDITIONAL_FAULT_CAUSE_FILE = "/usr/share/sonic/platform/api_files/reboot-cause/platform/additional_fault_cause"
THERMAL_OVERLOAD_POSITION_FILE = "/usr/share/sonic/platform/api_files/reboot-cause/platform/thermal_overload_position"

@thermal_json_object('switch.power_cycling')
class SwitchPolicyAction(ThermalPolicyActionBase):
    """
    Base class for thermal action. Once all thermal conditions in a thermal policy are matched,
    all predefined thermal action will be executed.
    """

    def execute(self, thermal_info_dict):
        """
        Take action when thermal condition matches. For example, power cycle the switch.
        :param thermal_info_dict: A dictionary stores all thermal information.
        :return:
        """
        self.__api_helper = APIHelper()
        helper_logger.log_error("Error: thermal overload !!!!!!!!!!!!!!!!!!Please reboot Now!!")
        helper_logger.log_error("Error: thermal overload !!!!!!!!!!!!!!!!!!")
        """
        helper_logger.log_error("recorded the fault cause begin...")
        attr_rv = self.__api_helper.read_one_line_file(INT_STATUS_FILE)
        attr_rv = int(attr_rv, 16)
        REBOOT_FAN_FAULT = ''
        REBOOT_PSU_FAULT = ''
        #0 is fault and 1 is normal
        if((attr_rv & 0x1) == 0):
            REBOOT_FAN_FAULT = 'FAN'
        if((attr_rv & 0x2) == 0):
            REBOOT_PSU_FAULT = '3V3 SYSTEM POWER'

        #fault_status = Fault.get_fault_status()
        # Write a new default reboot cause file for the next reboot check
        if((attr_rv & 0x3) != 0):
            REBOOT_FAULT_CAUSE = ' '.join([REBOOT_FAN_FAULT, REBOOT_PSU_FAULT])
            with open(ADDITIONAL_FAULT_CAUSE_FILE, "w") as additional_fault_cause_file:
                additional_fault_cause_file.write(REBOOT_FAULT_CAUSE)
                additional_fault_cause_file.close()
                os.system('sync')
                os.system('sync')
        helper_logger.log_error("recorded the fault cause...done")
        """
        #wait for all record actions done
        wait_ms =  30
        while wait_ms > 0:
            if os.path.isfile(THERMAL_OVERLOAD_POSITION_FILE):
                thermal_overload_pos = self.__api_helper.read_one_line_file(THERMAL_OVERLOAD_POSITION_FILE)
            if "critical threshold" in thermal_overload_pos:
                break
            time.sleep(1)
            helper_logger.log_error("wait ############for recorded")
            wait_ms = wait_ms - 1
        #system reset.
        os.system('echo 1 > /sys/devices/pci0000:00/0000:00:1f.3/i2c-0/i2c-1/1-0018/sys_rst')
