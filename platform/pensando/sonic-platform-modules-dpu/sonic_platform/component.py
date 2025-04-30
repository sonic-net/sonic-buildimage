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
    import sys,os.path
    import json
    import re
    from pathlib import Path
    from .helper import APIHelper
    from sonic_py_common import syslogger
    from sonic_platform_base.component_base import ComponentBase

except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

sys.path.append(str(Path(__file__).parent.absolute()))

CHASSIS_COMPONENT_MAPPING = None
MMC_DATA_PATH = "/sys/class/mmc_host/mmc0/mmc0:0001/{}"
MMC_DEV_PATH = "/dev/mmcblk0"
NOT_AVAILABLE = 'None'
IMAGES_PATH = "/host/images"
SYSLOG_IDENTIFIER = 'dpu-db-utild'
logger_instance = syslogger.SysLogger(SYSLOG_IDENTIFIER)

def log_err(msg, also_print_to_console=False):
    logger_instance.log_error(msg, also_print_to_console)

def parse_re(pattern, buffer, index = 0, alt_val = "N/A"):
        res_list = re.findall(pattern, buffer)
        return res_list[index] if res_list else alt_val

def get_slot_id():
    try:
        from sonic_platform.chassis import Chassis
        chassis = Chassis()
        slot = chassis.get_my_slot()
    except Exception as e:
        log_err("failed to fetch chassis slot id due to {e}")
        slot = -1
    slot_id = str(slot) if slot != -1 else 'UNDEFINED'
    return slot_id

def fetch_version_info(apiHelper):
    try:
        from sonic_installer.bootloader import get_bootloader
        bootloader = get_bootloader()
        curimage = bootloader.get_current_image()
        curimage = curimage.split('-')[-1]
        fwupdate_version_list = []
        try:
            dpu_docker_name = apiHelper.get_dpu_docker_container_name()
            cmd = "/nic/tools/fwupdate -l | grep -v 'FATAL' "
            cmd += "| grep -v 'sh: /dev/mapper: unknown operand'| "
            cmd += "grep -v 'standard metadata magic not found'"
            output = apiHelper.run_docker_cmd(cmd)
            fw_version_data = json.loads(output)
            try:
                fwupdate_version_list.append(fw_version_data['mainfwa']['system_image']['software_version'])
            except:
                fwupdate_version_list.append(NOT_AVAILABLE)
            try:
                fwupdate_version_list.append(fw_version_data['mainfwa']['uboot']['software_version'])
            except:
                fwupdate_version_list.append(NOT_AVAILABLE)
            try:
                fwupdate_version_list.append(fw_version_data['goldfw']['kernel_fit']['software_version'])
            except:
                fwupdate_version_list.append(NOT_AVAILABLE)
            try:
                fwupdate_version_list.append(fw_version_data['goldfw']['uboot']['software_version'])
            except:
                fwupdate_version_list.append(NOT_AVAILABLE)
            try:
                major_id = apiHelper.run_docker_cmd("cpldapp -r 0x0").replace('0x','')
                minor_id = apiHelper.run_docker_cmd("cpldapp -r 0x1e").replace('0x','')
                cpld_version = "{}.{}".format(major_id,minor_id)
                fwupdate_version_list.append(cpld_version)
            except:
                fwupdate_version_list.append(NOT_AVAILABLE)
        except Exception as e:
            log_err(f"Failed to fetch mainfwa/goldfw/cpld version info due to {e}")
            fwupdate_version_list = [NOT_AVAILABLE]*5
        version_info_list = [curimage]
        version_info_list.extend(fwupdate_version_list)
        return version_info_list
    except Exception as e:
        log_err(f"Failed to fetch sonic version info due to {e}")
        return [NOT_AVAILABLE]*7

"""
Chassis                   Module    Component            Version          Description
------------------------  --------  -------------------  ---------------  --------------------------------
Pensando-elba DSS-MTFUJI  N/A       DPUFW-7              20241119.204642  DPU-7 SONiC Image
                                    DPUQSPI_GOLDFW-7     1.68-G-22        DPU-7 GOLDFW
                                    DPUQSPI_GOLDUBOOT-7  1.68-G-22        DPU-7 GOLDUBOOT
                                    DPUQSPI_UBOOTA-7     1.5.0-EXP        DPU-7 UBOOTA
                                    eMMC                 N/A              Internal storage device
"""

def add_version_info(CHASSIS_COMPONENT_MAPPING, stat):
    [current_version, mainfwa_sw, mainfwa_uboot, goldfw_sw, goldfw_uboot, cpld_version] = stat

    component_index = 0
    slot_id = get_slot_id()

    ##### SONiC version
    deviceinfo = {}
    deviceinfo["name"] = f"DPUFW-{slot_id}"
    deviceinfo["file_id"] = "sonic-pensando"
    deviceinfo["extension"] = "bin"
    deviceinfo["description"] = f"DPU-{slot_id} SONiC Image"
    deviceinfo["productid"] = NOT_AVAILABLE
    deviceinfo["firmware version"] = current_version
    deviceinfo["serial number"] = NOT_AVAILABLE
    CHASSIS_COMPONENT_MAPPING[component_index] = deviceinfo
    component_index = component_index + 1

    ##### GOLDFW version
    deviceinfo = {}
    deviceinfo["name"] = f"DPUQSPI_GOLDFW-{slot_id}"
    deviceinfo["file_id"] = "goldfw"
    deviceinfo["extension"] = "tar"
    deviceinfo["description"] = f"DPU-{slot_id} GOLDFW"
    deviceinfo["productid"] = NOT_AVAILABLE
    deviceinfo["firmware version"] = goldfw_sw
    deviceinfo["serial number"] = NOT_AVAILABLE
    CHASSIS_COMPONENT_MAPPING[component_index] = deviceinfo
    component_index = component_index + 1

    ##### GOLDUBOOT version
    deviceinfo = {}
    deviceinfo["name"] = f"DPUQSPI_GOLDUBOOT-{slot_id}"
    deviceinfo["file_id"] = "golduboot"
    deviceinfo["extension"] = "img"
    deviceinfo["description"] = f"DPU-{slot_id} GOLDUBOOT"
    deviceinfo["productid"] = NOT_AVAILABLE
    deviceinfo["firmware version"] = goldfw_uboot
    deviceinfo["serial number"] = NOT_AVAILABLE
    CHASSIS_COMPONENT_MAPPING[component_index] = deviceinfo
    component_index = component_index + 1

    ##### UBOOTA version
    deviceinfo = {}
    deviceinfo["name"] = f"DPUQSPI_UBOOTA-{slot_id}"
    deviceinfo["file_id"] = "uboota"
    deviceinfo["extension"] = "img"
    deviceinfo["description"] = f"DPU-{slot_id} UBOOTA"
    deviceinfo["productid"] = NOT_AVAILABLE
    deviceinfo["firmware version"] = mainfwa_uboot
    deviceinfo["serial number"] = NOT_AVAILABLE
    CHASSIS_COMPONENT_MAPPING[component_index] = deviceinfo
    component_index = component_index + 1

    return CHASSIS_COMPONENT_MAPPING

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

        stat = fetch_version_info(apiHelper)
        CHASSIS_COMPONENT_MAPPING = add_version_info(CHASSIS_COMPONENT_MAPPING, stat)

        component_index = len(CHASSIS_COMPONENT_MAPPING)
        deviceinfo = {}
        deviceinfo["name"] = "eMMC"
        deviceinfo["description"] = "Internal storage device"
        deviceinfo["productid"] = NOT_AVAILABLE
        try:
            firmware_rev = NOT_AVAILABLE
            try:
                firmware_rev_bytes = bytes.fromhex((open(MMC_DATA_PATH.format("fwrev")).read()).replace('0x',''))
                firmware_rev = firmware_rev_bytes.decode("ascii")
            except:
                firmware_rev = NOT_AVAILABLE
                pass
            deviceinfo["firmware version"] = firmware_rev
            deviceinfo["serial number"] = open(MMC_DATA_PATH.format("serial")).read()
            deviceinfo["ffu capable"] = open(MMC_DATA_PATH.format("ffu_capable")).read()
        except:
            deviceinfo["firmware version"] = NOT_AVAILABLE
            deviceinfo["serial number"] = NOT_AVAILABLE

        CHASSIS_COMPONENT_MAPPING[component_index] = deviceinfo
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

    def _get_uboot_version(self, image_path, file_path):
        try:
            cmd = f'cat {image_path}/{file_path} | grep --text "software_version" | grep --text ","'
            command_output = self._api_helper.runCMD(cmd)
            pattern = r'"software_version":\s*"([\w.\-]+)"'
            match = re.search(pattern, command_output)
            if match:
                version = match.group(1)  # Extract the captured version
                return version
            else:
                return NOT_AVAILABLE
        except:
            return NOT_AVAILABLE

    def _get_goldfw_version(self, image_path, file_path):
        try:
            manifest_path = os.path.join(image_path, "MANIFEST")
            cmd = f'tar -xvf {image_path}/{file_path} -C {image_path} MANIFEST'
            self._api_helper.runCMD(cmd)
            if os.path.exists(manifest_path):
                file = open(manifest_path, "r")
                data = json.load(file)
                file.close()
                self._api_helper.runCMD(f"rm -f {manifest_path}")
                version = data.get('software_version', NOT_AVAILABLE)
                return version
            else:
                return NOT_AVAILABLE
        except:
            return NOT_AVAILABLE

    def get_available_firmware_version(self, image_path):
        """
        Retrieves the available firmware version of the component
        Note: the firmware version will be read from image
        Args: image_path: A string, path to firmware image
        Returns: A string containing the available fw  version of component
        """
        current_version = CHASSIS_COMPONENT_MAPPING.get(self.component_index,{}).get("firmware version", None)
        name = self.get_name()
        if (image_path == None) or (image_path == ""):
            image_path = IMAGES_PATH
        try:
            matching_files = []
            file_id = CHASSIS_COMPONENT_MAPPING.get(self.component_index,{}).get("file_id", None)
            extension = CHASSIS_COMPONENT_MAPPING.get(self.component_index,{}).get("extension", None)
            if (file_id != None) and os.path.exists(image_path) and os.path.isdir(image_path):
                files = os.listdir(image_path)
                pattern = re.compile(rf'^{re.escape(file_id)}_ver_([\w.\-]+)\.{re.escape(extension)}$')
                for file in files:
                    match = pattern.match(file)
                    if match:
                        version = match.group(1)
                        version_from_file = NOT_AVAILABLE
                        if "UBOOT" in name:
                            version_from_file = self._get_uboot_version(image_path, file)
                        if "GOLDFW" in name:
                            version_from_file = self._get_goldfw_version(image_path, file)
                        if version_from_file != NOT_AVAILABLE:
                            version = version_from_file
                        matching_files.append((file, version))
            matching_files.sort(key=lambda x: x[1], reverse=True)
            #### reading version from files
            if matching_files:
                highest_version_file, highest_version = matching_files[0]
                if highest_version > current_version:
                    return highest_version
                else:
                    return current_version
            else:
                return current_version
        except Exception as e:
            return current_version

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
        available_version = self.get_available_firmware_version(None)
        file_id = CHASSIS_COMPONENT_MAPPING.get(self.component_index,{}).get("file_id", None)
        extension = CHASSIS_COMPONENT_MAPPING.get(self.component_index,{}).get("extension", None)
        file_path = f"{image_path}/{file_id}_ver_{available_version}.{extension}"
        if os.path.exists(file_path):
            return self.install_firmware(file_path)
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
        name = self.get_name()
        if name == "eMMC":
            cmd = "mmc ffu " + image_path + " " + MMC_DEV_PATH
            self._api_helper.runCMD(cmd)
            return True
        elif "DPUFW" in name:
            print(f"use sonic-installer cmd for installation of {name}")
            return False
        else:
            file_id = CHASSIS_COMPONENT_MAPPING.get(self.component_index,{}).get("file_id", None)
            extension = CHASSIS_COMPONENT_MAPPING.get(self.component_index,{}).get("extension", None)
            file_name = os.path.basename(image_path)
            pattern = rf"^{re.escape(file_id)}_ver_(.+)\.{re.escape(extension)}$"
            match = re.match(pattern, file_name)
            if not match:
                print(f"file {image_path} is not correct for {name}, check naming convention")
                print(f"naming convention - {file_id}_ver_<version>.{extension}")
                return False
            if "UBOOT" in name:
                try:
                    version_from_file = self._get_uboot_version(os.path.dirname(image_path), os.path.basename(image_path))
                    print(f"installing {image_path} with version {version_from_file} for component {name}")
                    cmd = f"/boot/install_file {file_id} {image_path}"
                    print(cmd)
                    ret = self._api_helper.runCMD(cmd)
                    print(ret)
                    return True
                except Exception as e:
                    print(f"installation failed due to {e}")
                    return False
            if "GOLDFW" in name:
                try:
                    version_from_file = self._get_goldfw_version(os.path.dirname(image_path), os.path.basename(image_path))
                    container_name = self._api_helper.get_dpu_docker_container_name()
                    print(f"installing {image_path} with version {version_from_file} for component {name}")
                    cmd = f"docker cp {image_path} {container_name}:/data/"
                    print(cmd)
                    ret = self._api_helper.runCMD(cmd)
                    cmd = ["docker", "exec", container_name, "/nic/tools/fwupdate", "-i", "goldfw", "-p", f"/data/{file_name}"]
                    print(cmd)
                    status, result = self._api_helper.run_command(cmd)
                    return status
                except Exception as e:
                    print(f"installation failed due to {e}")
                    return False
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
        return NOT_AVAILABLE

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
