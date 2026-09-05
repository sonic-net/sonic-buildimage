#############################################################################
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Chassis information which are available in the platform
#
#############################################################################
try:
    import subprocess
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_py_common.logger import Logger
    from sonic_platform.fan import Fan
    from sonic_platform.psu import Psu
    from sonic_platform.component import Component
    from sonic_platform.thermal import Thermal
    from sonic_platform.sfp import Sfp
    from sonic_platform.eeprom import Tlv
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

NUM_FAN_TRAY = 5
NUM_FAN = 2
NUM_PSU = 2
NUM_THERMAL = 4
NUM_SFP = 54
SFP_PORT_START = 0
QSFP_PORT_START = 48
SFP_PORT_END = 47
QSFP_PORT_END = 53
HOST_REBOOT_CAUSE_PATH = "/host/reboot-cause/"
PMON_REBOOT_CAUSE_PATH = "/usr/share/sonic/platform/api_files/reboot-cause/"
REBOOT_CAUSE_FILE = "reboot-cause.txt"
PREV_REBOOT_CAUSE_FILE = "previous-reboot-cause.txt"
COMPONENT_NAME_LIST = ["BIOS"]
HOST_CHK_CMD = ["docker"]


class Chassis(ChassisBase):
    """Platform-specific Chassis class"""

    def __init__(self):
        super(Chassis, self).__init__()

        for fantray_index in range(0, NUM_FAN_TRAY):
            for fan_index in range(0, NUM_FAN):
                fan = Fan(fantray_index, fan_index)
                self._fan_list.append(fan)
        for index in range(0, NUM_PSU):
            psu = Psu(index)
            self._psu_list.append(psu)
        for index in range(0, NUM_THERMAL):
            thermal = Thermal(index)
            self._thermal_list.append(thermal)

        self.PORT_START = SFP_PORT_START
        self.QSFP_PORT_START = QSFP_PORT_START
        self.PORT_END = QSFP_PORT_END
        for index in range(0, NUM_SFP):
            if index in range(self.QSFP_PORT_START, self.QSPORT_END + 1):
                sfp_module = Sfp(index, 'QSFP')
            else:
                sfp_module = Sfp(index, 'SFP')
            self._sfp_list.append(sfp_module)
        self._component_name_list = COMPONENT_NAME_LIST
        self._watchdog = Watchdog()
        self._eeprom = Tlv()
        logger.log_info("Chassis loaded successfully")

    def __is_host(self):
        return subprocess.call(HOST_CHK_CMD) == 0

    def __read_txt_file(self, file_path):
        try:
            with open(file_path, 'r') as fd:
                data = fd.read()
                return data.strip()
        except IOError:
            pass
        return None

    def get_base_mac(self):
        """
        Retrieves the base MAC address for the chassis
        Returns:
            A string containing the MAC address in the format
            'XX:XX:XX:XX:XX:XX'
        """
        return self._eeprom.get_mac()

    def get_serial(self):
        """
        Retrieves the hardware serial number for the chassis
        Returns:
            A string containing the hardware serial number for this chassis.
        """
        return self._eeprom.get_serial()

    def get_system_eeprom_info(self):
        """
        Retrieves the full content of system EEPROM information for the chassis
        Returns:
            A dictionary where keys are the type code defined in
            OCP ONIE TlvInfo EEPROM format and values are their corresponding
            values.
        """
        return self._eeprom.get_eeprom()

    def get_firmware_version(self, component_name):
        """
        Retrieves platform-specific hardware/firmware versions for chassis
        componenets such as BIOS, CPLD, FPGA, etc.
        Args:
            type: A string, component name

        Returns:
            A string containing platform-specific component versions
        """
        self.component = Component(component_name)
        if component_name not in self._component_name_list:
            return None
        return self.component.get_firmware_version()

    def install_component_firmware(self, component_name, image_path):
        """
        Install firmware to module
        Args:
            type: A string, component name.
            image_path: A string, path to firmware image.

        Returns:
            A boolean, True if install successfully, False if not
        """
        self.component = Component(component_name)
        if component_name not in self._component_name_list:
            return False
        return self.component.upgrade_firmware(image_path)

    def get_reboot_cause(self):
        """
        Retrieves the cause of the previous reboot

        Returns:
            A tuple (string, string) where the first element is a string
            containing the cause of the previous reboot. This string must be
            one of the predefined strings in this class. If the first string
            is "REBOOT_CAUSE_HARDWARE_OTHER", the second string can be used
            to pass a description of the reboot cause.
        """
        description = 'None'
        reboot_cause = self.REBOOT_CAUSE_HARDWARE_OTHER

        reboot_cause_path = (HOST_REBOOT_CAUSE_PATH + REBOOT_CAUSE_FILE) if self.__is_host(
        ) else PMON_REBOOT_CAUSE_PATH + REBOOT_CAUSE_FILE
        prev_reboot_cause_path = (HOST_REBOOT_CAUSE_PATH + PREV_REBOOT_CAUSE_FILE) if self.__is_host(
        ) else PMON_REBOOT_CAUSE_PATH + PREV_REBOOT_CAUSE_FILE
        sw_reboot_cause = self.__read_txt_file(
            reboot_cause_path) or "Unknown"
        prev_sw_reboot_cause = self.__read_txt_file(
            prev_reboot_cause_path) or "Unknown"

        if sw_reboot_cause != "Unknown":
            reboot_cause = self.REBOOT_CAUSE_NON_HARDWARE
            description = sw_reboot_cause
        else:
            reboot_cause = self.REBOOT_CAUSE_HARDWARE_OTHER
            description = 'Unknown reason'

        return (reboot_cause, description)
