#!/usr/bin/env python

try:
    import os
    import logging
    from sonic_platform_base.component_base import ComponentBase
    from sonic_platform.inv_const import Common

except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


OS_SYSTEM_SUCCESS = 0

CPLD1_INFO_PATH = Common.I2C_PREFIX+"/0-0077/info"
CPLD2_INFO_PATH = Common.I2C_PREFIX+"/0-0077/info" #info of 2 cpld are combined by inv_cpld under this path
BIOS_VER_PATH   = "/sys/class/dmi/id/bios_version"
BIOS_CS_PATH    = Common.I2C_PREFIX+"/0-0077/bios_cs"

CPLD1_INDEX        = 0
CPLD2_INDEX        = 1
MAIN_BIOS_INDEX    = 2
BACKUP_BIOS_INDEX  = 3


COMPONENT_NAME_LIST = [
    "CPLD1",
    "CPLD2",
    "Main BIOS",
    "Backup BIOS",
]

COMPONENT_DESC_LIST = [
    "platform management and control LED",
    "platform management and control LED",
    "Main Basic Input/Output System",
    "Backup Basic Input/Output System",
]


BIOS_ID_MAPPING_TABLE = {
    0: MAIN_BIOS_INDEX,
    1: BACKUP_BIOS_INDEX
}

class Component(ComponentBase):

    def __get_attr_value(self, attr_path):

        retval = 'ERR'
        if (not os.path.isfile(attr_path)):
            return retval

        try:
            with open(attr_path, 'r') as fd:
                retval = fd.read()
        except Exception as error:
            logging.error("Unable to open ", attr_path, " file !")

        retval = retval.rstrip(' \t\n\r')
        return retval

    def __set_attr_value(self, attr_path, value):
        try:
            with open(attr_path, 'r+') as reg_file:
                reg_file.write(value)
        except IOError as e:
            logging.error("Error: unable to open file: %s" % str(e))
            return False

        return True 

    def __get_current_bios(self):
        current_bios=self.__get_attr_value(BIOS_CS_PATH)
        if current_bios != 'ERR':
            '''
            Get first char to convert to bios ID
            '''
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
        target=""

        if self.index == CPLD1_INDEX:
            path = CPLD1_INFO_PATH
            target="The CPLD version is "
        elif self.index == CPLD2_INDEX:
            path = CPLD2_INFO_PATH
            target="The CPLD2 version is "
        else:
            logging.error("Unable support index %d", self.index)

        if path !=None:
            try:
                with open(path, 'r') as file:
                    ret_str = file.read()
            except Exception as error:
                logging.error("Unable to open file %s", path)
            
            if ret_str!=None:
                start_idx=ret_str.find(target)
                if start_idx > 0:
                    start_idx = start_idx+len(target)
                    offset = ret_str[start_idx:].find('\n')
                    if offset > 0:
                        end_idx=start_idx+offset
                        cpld_version=ret_str[start_idx:end_idx].strip('\n')

                if cpld_version is None:
                    logging.error("Unable to parse cpld info %d", self.index)

        return cpld_version


    def __get_bios_version(self):
        bios_version = None
        current_bios_id=self.__get_current_bios()

        if current_bios_id != None :
            if self.index == BIOS_ID_MAPPING_TABLE[current_bios_id]:
                try:
                    with open(BIOS_VER_PATH, 'r') as file:
                        bios_version = file.read().strip('\n')
                except Exception as error:
                    logging.error("Unable to open file %s", BIOS_VER_PATH)
            else:
                logging.error("Only support bios version of current running BIOS")
                bios_version = "N/A"

        return bios_version

    def __install_cpld_firmware(self,image_path):
        logging.error("[Component][__install_cpld_firmware] Currently not support FW update on platform D6332")
        raise NotImplementedError
        
    def __install_bios_firmware(self,image_path):
        logging.error("[Component][__install_bios_firmware] Currently not support FW update on platform D6332")
        raise NotImplementedError

    __get_version_callback_list = {
        CPLD1_INDEX:__get_cpld_version,
        CPLD2_INDEX:__get_cpld_version,
        MAIN_BIOS_INDEX:__get_bios_version,
        BACKUP_BIOS_INDEX:__get_bios_version,
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
        logging.error("[Component][install_firmware] Currently not support FW update on platform D6332")
        raise NotImplementedError