# {C} Copyright 2023 AMD Systems Inc. All rights reserved
################################################################################
# Pensando
#
# Component contains an implementation of SONiC Platform Base API and
# provides the components firmware management function, such as
# EMMC, CPLD, and transceivers
#
################################################################################

try:
    import threading
    import sys,os.path
    import logging
    import time
    import json
    import yaml
    import re
    import subprocess
    from pathlib import Path
    from .helper import APIHelper
    from sonic_platform_base.component_base import ComponentBase

except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

sys.path.append(str(Path(__file__).parent.absolute()))

CHASSIS_COMPONENT_MAPPING = None
MMC_DATA_PATH = "/sys/class/mmc_host/mmc0/mmc0:0001/{}"
MMC_DEV_PATH = "/dev/mmcblk0"

def parse_re(pattern, buffer, index = 0, alt_val = "N/A"):
        res_list = re.findall(pattern, buffer)
        return res_list[index] if res_list else alt_val

class Component(ComponentBase):
    """Platform-specific Component class"""
    @classmethod
    def populate_component_data(cls):
        global CHASSIS_COMPONENT_MAPPING
        from sonic_platform.helper import APIHelper
        apiHelper = APIHelper()

        if CHASSIS_COMPONENT_MAPPING == None:
            CHASSIS_COMPONENT_MAPPING = {}
        else:
            return CHASSIS_COMPONENT_MAPPING

        component_index = 0
        deviceinfo = {}
        deviceinfo["name"] = "eMMC"
        deviceinfo["description"] = "Internal storage device"
        deviceinfo["productid"] = "N/A"
        try:
            firmware_rev = "N/A"
            try:
                firmware_rev_bytes = bytes.fromhex((open(MMC_DATA_PATH.format("fwrev")).read()).replace('0x',''))
                firmware_rev = firmware_rev_bytes.decode("ascii")
            except:
                firmware_rev = "N/A"
                pass
            deviceinfo["firmware version"] = firmware_rev
            deviceinfo["serial number"] = open(MMC_DATA_PATH.format("serial")).read()
            deviceinfo["ffu capable"] = open(MMC_DATA_PATH.format("ffu_capable")).read()
        except:
            deviceinfo["firmware version"] = "N/A"
            deviceinfo["serial number"] = "N/A"

        CHASSIS_COMPONENT_MAPPING[component_index] = deviceinfo
        component_index = component_index + 1

        cpldinfo = {}
        cpldinfo["name"] = "cpld"
        cpldinfo["description"] = "Used for managing dpu"
        cpldinfo["productid"] = "N/A"
        cpldinfo["serial number"] = "N/A"
        try:
            firmware_rev = "N/A"
            try:
                major_id = apiHelper.runCMD("cpldapp -r 0x0").replace('0x','')
                minor_id = apiHelper.runCMD("cpldapp -r 0x1e").replace('0x','')
                firmware_rev = "{}.{}".format(major_id,minor_id)
            except:
                firmware_rev = "N/A"
                pass
            cpldinfo["firmware version"] = firmware_rev
        except:
            cpldinfo["firmware version"] = "N/A"
        CHASSIS_COMPONENT_MAPPING[component_index] = cpldinfo

        if apiHelper.check_xcvrs_present():
            from sonic_platform.sfp import Sfp
            from sonic_platform.chassis import PORT_START, PORT_END

            sfp_name_map = apiHelper.read_port_config()

            for index in range(PORT_START, PORT_END+1):
                sfp = Sfp(index-1, sfp_name_map[index-1])
                transceiver_info = sfp.get_transceiver_info()
                portinfo = {}
                portinfo["name"] = sfp.get_name()
                portinfo["description"] = f"QSFP port - {index}"
                portinfo["firmware version"] = "N/A"
                portinfo["serial number"] = transceiver_info['serial']
                portinfo["vendor name"] = transceiver_info['manufacturer']
                portinfo["vendor part number"] = transceiver_info['model']
                portinfo["vendor revision"] = transceiver_info['vendor_rev']
                CHASSIS_COMPONENT_MAPPING[component_index + index] = portinfo

        return CHASSIS_COMPONENT_MAPPING


    def __init__(self, component_index = 0):
        global CHASSIS_COMPONENT_MAPPING
        self._api_helper = APIHelper()
        if CHASSIS_COMPONENT_MAPPING == None:
            CHASSIS_COMPONENT_MAPPING = Component.populate_component_data()

        ComponentBase.__init__(self)
        self.component_index = component_index

    def get_firmware_version(self):
        """
        Retrieves the firmware version of module
        Returns: string: The firmware versions of the module
        """
        version = CHASSIS_COMPONENT_MAPPING.get(self.component_index,{}).get("firmware version", None)
        return version

    def get_available_firmware_version(self, image_path):
        """
        Retrieves the available firmware version of the component
        Note: the firmware version will be read from image
        Args: image_path: A string, path to firmware image
        Returns: A string containing the available fw  version of component
        """
        return "N/A"

    def get_name(self):
        """
        Retrieves the name of the component
         Returns: A string containing the name of the component
        """
        name = CHASSIS_COMPONENT_MAPPING.get(self.component_index,{}).get("name", None)
        return name

    def get_description(self):
        """
        Retrieves the description of the component
            Returns: A string containing the description of the component
        """
        description = CHASSIS_COMPONENT_MAPPING.get(self.component_index,{}).get("description", None)
        return description

    def update_firmware(self, image_path):
        """
        Updates firmware of the component
        This API performs firmware update: it assumes firmware installation and
        loading in a single call.
        In case platform component requires some extra steps (apart from calling
        Low Level Utility)
        to load the installed firmware (e.g, reboot, power cycle, etc.) -
        this will be done automatically by API
        Args: image_path: A string, path to firmware image
        Raises: RuntimeError: update failed
        """
        return False

    def install_firmware(self, image_path):
        """
        Installs firmware of the component
        In case platform component requires some extra steps (apart from calling
        Low Level Utility)
        to load the installed firmware (e.g, reboot, power cycle, etc.) -
        that has to be done separately
        Args: image_path: A string, path to firmware image
        Raises: RuntimeError: update failed
        """
        if CHASSIS_COMPONENT_MAPPING.get(self.component_index,{}).get("name", None) == "eMMC":
            cmd = "mmc ffu " + image_path + " " + MMC_DEV_PATH
            self._api_helper.runCMD(cmd)
            return True
        return False

    ###################### Device methods ########################
    ##############################################################


    def get_presence(self):
        """
        Retrieves the presence of the FAN
        Returns:
            bool: True if FAN is present, False if not
        """
        return True

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        return 'N/A'

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        return CHASSIS_COMPONENT_MAPPING.get(self.component_index,{}).get("serial number", None)

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return True

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of
        entPhysicalContainedIn is'0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device
            or -1 if cannot determine the position
        """
        return -1

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False
