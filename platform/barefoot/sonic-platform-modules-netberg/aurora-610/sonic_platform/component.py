#!/usr/bin/env python

try:
    import os
    import re
    import logging
    from subprocess import call, Popen, PIPE
    from sonic_platform_base.component_base import ComponentBase

except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


OS_SYSTEM_SUCCESS = 0

CPLD_INFO_PATH = "/sys/bus/i2c/devices/0-0077/info"
BIOS_VER_PATH = "/sys/class/dmi/id/bios_version"
BIOS_CS_PATH = "/sys/bus/i2c/devices/0-0077/bios_cs"

CPLD_INDEX = 0
BMC_INDEX = 1
MAIN_BIOS_INDEX = 2
BACKUP_BIOS_INDEX = 3


COMPONENT_NAME_LIST = [
    "CPLD",
    "BMC",
    "Main BIOS",
    "Backup BIOS",
]

COMPONENT_DESC_LIST = [
    "Platform management and LED",
    "Baseboard management controller",
    "Main BIOS",
    "Backup BIOS",
]


FW_INSTALL_CMD_LIST = [
    ["/usr/share/sonic/platform/plugins/cpld", "-b", "1", "-s", "0x60", ""],
    ["/usr/share/sonic/platform/plugins/Yafuflash", "-cd", "", "-img-select", "3", "-non-interactive"],
    ["/usr/share/sonic/platform/plugins/afulnx_64", "", "/B", "/P", "/N", "/K"],
    ["/usr/share/sonic/platform/plugins/afulnx_64", "", "/B", "/P", "/N", "/K"],
]

BIOS_ID_MAPPING_TABLE = {
    0: MAIN_BIOS_INDEX,
    1: BACKUP_BIOS_INDEX
}


class Component(ComponentBase):

    def __get_attr_value(self, attr_path):

        retval = 'ERR'
        if not os.path.isfile(attr_path):
            return retval

        try:
            with open(attr_path, 'r') as fd:
                retval = fd.read()
        except Exception as error:
            logging.error("Unable to open file: %s", attr_path)

        retval = retval.rstrip(' \t\n\r')
        return retval

    def __set_attr_value(self, attr_path, value):
        try:
            with open(attr_path, 'r+') as reg_file:
                reg_file.write(value)
        except IOError as e:
            logging.error("Error: unable to open file: %s", str(e))
            return False

        return True

    def __get_current_bios(self):
        current_bios = self.__get_attr_value(BIOS_CS_PATH)
        if current_bios != 'ERR':
            # Get first char to convert to bios ID
            current_bios = int(current_bios[:1])
        else:
            current_bios = None

        return current_bios

    def __get_cpld_version(self):
        '''
        The info output would be like:
        The CPLD release date is 06/13/2019.
        The PCB  version is 5
        The CPLD version is 1.1
        '''
        cpld_version = None
        ret_str = None
        path = None
        target = "The CPLD version is "

        if self.index == CPLD_INDEX:
            path = CPLD_INFO_PATH
        else:
            logging.error("Unable support index %d", self.index)

        if path is not None:
            try:
                with open(path, 'r') as file:
                    ret_str = file.read()
            except Exception as error:
                logging.error("Unable to open file %s", path)

            if ret_str is not None:
                start_idx = ret_str.find(target)
                if start_idx > 0:
                    start_idx = start_idx+len(target)
                    offset = ret_str[start_idx:].find('\n')
                    if offset > 0:
                        end_idx = start_idx+offset
                        cpld_version = ret_str[start_idx:end_idx].strip('\n')

                if cpld_version is None:
                    logging.error("Unable to parse cpld info %d", self.index)

        return cpld_version

    def __get_bmc_version(self):
        bmc_version = "N/A"

        output = Popen(["ipmitool", "mc", "info"], stdout=PIPE, encoding='utf8')
        ret_str = output.communicate(timeout=5)[0]

        ret_search = re.search("Firmware Revision\s+:\s(.*)\n", ret_str, re.M)

        if ret_search:
            bmc_version = ret_search.group(1)

        return bmc_version

    def __get_bios_version(self):
        bios_version = None
        current_bios_id = self.__get_current_bios()

        if current_bios_id is not None:
            if self.index == BIOS_ID_MAPPING_TABLE[current_bios_id]:
                try:
                    with open(BIOS_VER_PATH, 'r') as file:
                        bios_version = file.read().strip('\n')
                except Exception as error:
                    logging.error("Unable to open file %s", BIOS_VER_PATH)
            else:
                logging.error(
                    "Only support bios version of current running BIOS")
                bios_version = "N/A"

        return bios_version

    def __get_cmd(self, image_path):
        if self.index == 0:
            FW_INSTALL_CMD_LIST[self.index][5] = image_path
        elif self.index == 1:
            FW_INSTALL_CMD_LIST[self.index][2] = image_path
        elif self.index == 2 or self.index == 3:
            FW_INSTALL_CMD_LIST[self.index][1] = image_path
        return FW_INSTALL_CMD_LIST

    def __install_cpld_firmware(self, image_path):
        result = False
        cmd = self.__get_cmd(image_path)

        ret = call(cmd)
        if ret == OS_SYSTEM_SUCCESS:
            result = True

        return result

    def __install_bmc_firmware(self, image_path):
        result = False
        cmd = self.__get_cmd(image_path)

        ret = call(cmd)
        if ret == OS_SYSTEM_SUCCESS:
            result = True
        return result

    def __install_bios_firmware(self, image_path):
        result = False
        temp_bios_id = None
        current_bios_id = self.__get_current_bios()

        if current_bios_id == 0:
            temp_bios_id = 1
        elif current_bios_id == 1:
            temp_bios_id = 0
        else:
            #If current BIOS ID illegal, treat as None
            logging.error("Cannot get correct bios id")
            current_bios_id = None

        if (current_bios_id is not None) and (temp_bios_id is not None):
            ret = True
            if self.index == BIOS_ID_MAPPING_TABLE[current_bios_id]:
                pass
            elif self.index == BIOS_ID_MAPPING_TABLE[temp_bios_id]:
                ret = self.__set_attr_value(BIOS_CS_PATH, str(temp_bios_id))
            else:
                logging.error("Not support BIOS index %d", self.index)

            if ret:
                cmd = self.__get_cmd(image_path)
                ret = call(cmd)
                if ret == OS_SYSTEM_SUCCESS:
                    result = True
                else:
                    logging.error("Bios Firmware upgrade fail")
            else:
                logging.error("Set BIOS ID fail")

            if self.index == BIOS_ID_MAPPING_TABLE[temp_bios_id]:
                # Write back current_bios no matter what
                self.__set_attr_value(BIOS_CS_PATH, str(current_bios_id))

        return result

    __get_version_callback_list = {
        CPLD_INDEX: __get_cpld_version,
        BMC_INDEX: __get_bmc_version,
        MAIN_BIOS_INDEX: __get_bios_version,
        BACKUP_BIOS_INDEX: __get_bios_version,
    }

    __install_firmware_callback_list = {
        CPLD_INDEX: __install_cpld_firmware,
        BMC_INDEX: __install_bmc_firmware,
        MAIN_BIOS_INDEX: __install_bios_firmware,
        BACKUP_BIOS_INDEX: __install_bios_firmware,
    }

    def __init__(self, component_index):
        self.index = component_index

    def get_name(self):
        """
        Retrieves the name of the component
        Returns:
            A string containing the name of the component
        """
        return COMPONENT_NAME_LIST[self.index]

    def get_description(self):
        """
        Retrieves the description of the component
        Returns:
            A string containing the description of the component
        """
        return COMPONENT_DESC_LIST[self.index]

    def get_firmware_version(self):
        """
        Retrieves the firmware version of the component
        Returns:
            A string containing the firmware version of the component
        """
        return self.__get_version_callback_list[self.index](self)

    def install_firmware(self, image_path):
        """
        Installs firmware to the component
        Args:
            image_path: A string, path to firmware image
        Returns:
            A boolean, True if install was successful, False if not
        """
        return self.__install_firmware_callback_list[self.index](self, image_path)
