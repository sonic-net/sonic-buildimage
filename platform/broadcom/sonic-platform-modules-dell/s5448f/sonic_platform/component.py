#!/usr/bin/env python3
"""
########################################################################
# DELLEMC S5448F
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Components' (e.g., BIOS, CPLD, FPGA, BMC etc.) available in
# the platform
#
########################################################################
"""

try:
    import json
    import os
    import re
    import subprocess
    import tarfile
    import tempfile
    from sonic_platform_base.component_base import ComponentBase
    import sonic_platform.hwaccess as hwaccess
    from sonic_py_common.general import check_output_pipe
except ImportError as err:
    raise ImportError(str(err) + "- required module not found")


def get_bios_version():
    """ Returns BIOS Version """
    bios_ver = ''
    try:
        bios_ver = subprocess.check_output(['dmidecode', '-s', 'system-version']).strip()
        if type(bios_ver) == bytes:
            bios_ver = bios_ver.decode()
    except (FileNotFoundError, subprocess.CalledProcessError):
        pass
    return bios_ver

def get_fpga_version():
    """ Returns FPGA Version """
    val = hwaccess.pci_get_value('/sys/bus/pci/devices/0000:04:00.0/resource0', 0)
    return '{}.{}'.format((val >> 8) & 0xff, val & 0xff)

def get_bmc_version():
    """ Returns BMC Version """
    bmc_ver = ''
    try:
        bmc_ver = subprocess.check_output(
            "ipmitool mc info | awk '/Firmware Revision/ { print $NF }'",
            shell=True, text=True).strip()
    except (FileNotFoundError, subprocess.CalledProcessError):
        pass
    return bmc_ver

def get_cpld_version(bus, i2caddr):
    """ Returns CPLD Major.Minor Version at Byte1.Byte0 @ given i2c address """
    major = hwaccess.i2c_get(bus, i2caddr, 1)
    minor = hwaccess.i2c_get(bus, i2caddr, 0)
    if major != -1 and minor != -1:
        return '{}.{}'.format(major, minor)
    return ''

def get_cpld0_version():
    """ Returns System CPLD Version """
    return get_cpld_version(601, 0x31)

def get_cpld1_version():
    """ Returns Secondary CPLD1 Version """
    return get_cpld_version(600, 0x30)

def get_cpld2_version():
    """ Returns Secondary CPLD2 Version """
    return get_cpld_version(600, 0x31)

def get_cpld3_version():
    """ Returns Secondary CPLD3 Version """
    return get_cpld_version(600, 0x32)

def _get_pcie_fw_version():
    """ Returns PCIe Firmware Version """
    pcie_ver = ''
    try:
        cmd_out =  subprocess.check_output(['bcmcmd', "dsh -c \"pciephy fwinfo\""], text=True).strip()
        result = re.search(r'(?<=PCIe FW loader version:).*', cmd_out)
        if result is not None:
            pcie_ver = result.group(0).strip()
    except (FileNotFoundError, subprocess.CalledProcessError):
        pass

    return pcie_ver


class Component(ComponentBase):
    """DellEMC Platform-specific Component class"""

    CHASSIS_COMPONENTS = [
        ['BIOS',
         'Performs initialization of hardware components during booting',
         get_bios_version
        ],

        ['FPGA',
         'Used for managing the system LEDs',
         get_fpga_version
        ],

        ['BMC',
         'Platform management controller for on-board temperature '
         'monitoring, in-chassis power, Fan and LED control',
         get_bmc_version
        ],

        ['System CPLD',
         'Used for managing the CPU power sequence and CPU states',
         get_cpld0_version
        ],

        ['Secondary CPLD 1',
         'Used for managing SFP56/QSFP56 port transceivers (QSFP56-DD 1-4, SFP56-DD 5-18)',
         get_cpld1_version
        ],

        ['Secondary CPLD 2',
         'Used for managing SFP56/QSFP56 port transceivers (SFP56-DD 19-39)',
         get_cpld2_version
        ],

        ['Secondary CPLD 3',
         'Used for managing SFP56/QSFP56 port transceivers (SFP56-DD 40-52, QSFPDD 53-56)',
         get_cpld3_version
        ],
        ['PCIe',
         'ASIC PCIe firmware',
         _get_pcie_fw_version
        ]
    ]

    def __init__(self, component_index=0):
        ComponentBase.__init__(self)
        self.index = component_index
        self.name = self.CHASSIS_COMPONENTS[self.index][0]
        self.description = self.CHASSIS_COMPONENTS[self.index][1]
        self.version = self.CHASSIS_COMPONENTS[self.index][2]()

    @staticmethod
    def _get_available_firmware_version(image_path):
        if not os.path.isfile(image_path):
            return False, "ERROR: File not found"

        with tempfile.TemporaryDirectory() as tmpdir:
            cmd1 = ["sed", "-e", '1,/^exit_marker$/d', image_path]
            cmd2 = ["tar", "-x", "-C", tmpdir, "installer/onie-update.tar.xz"]
            try:
                check_output_pipe(cmd1, cmd2)
            except subprocess.CalledProcessError:
                return False, "ERROR: Unable to extract firmware updater"

            try:
                updater = tarfile.open(os.path.join(tmpdir, "installer/onie-update.tar.xz"), "r")
            except tarfile.ReadError:
                return False, "ERROR: Unable to extract firmware updater"

            try:
                ver_info_fd = updater.extractfile("firmware/fw-component-version")
            except KeyError:
                updater.close()
                return False, "ERROR: Version info not available"

            ver_info = json.load(ver_info_fd)
            ver_info_fd.close()
            updater.close()

            ver_info = ver_info.get("x86_64-dellemc_s5448f-r0")
            if ver_info:
                components = list(ver_info.keys())
                for component in components:
                    if "CPLD" in component and ver_info[component].get('version'):
                        val = ver_info.pop(component)
                        ver = int(val['version'], 16)
                        val['version'] = "{:x}.{:x}".format((ver >> 4) & 0xf, ver & 0xf)
                        ver_info[component.replace("-", " ")] = val

                return True, ver_info
            else:
                return False, "ERROR: Version info not available"

    @staticmethod
    def _stage_firmware_package(image_path):
        stage_msg = None
        cmd = ["onie_stage_fwpkg", "-a", image_path]
        try:
            subprocess.check_output(cmd, stderr=subprocess.STDOUT, text=True)
        except subprocess.CalledProcessError as e:
            if e.returncode != 2:
                return False, e.output.strip()
            else:
                stage_msg = e.output.strip()

        cmd = ["onie_mode_set", "-o", "update"]
        try:
            subprocess.check_output(cmd, stderr=subprocess.STDOUT, text=True)
        except subprocess.CalledProcessError as e:
            return False, e.output.strip()

        if stage_msg:
            return True, stage_msg
        else:
            return True, "INFO: Firmware upgrade staged"

    def get_name(self):
        """
        Retrieves the name of the component
        Returns:
        A string containing the name of the component
        """
        return self.name

    def get_description(self):
        """
        Retrieves the description of the component
        Returns:
        A string containing the description of the component
        """
        return self.description

    def get_firmware_version(self):
        """
        Retrieves the firmware version of the component
        Returns:
            A string containing the firmware version of the component
        """
        if self.version == None:
            self.version = self.CHASSIS_COMPONENTS[self.index][2]()
        return self.version

    @staticmethod
    def install_firmware(image_path):
        """
        Installs firmware to the component
        Args:
        image_path: A string, path to firmware image
        Returns:
        A boolean, True if install was successful, False if not
        """
        del image_path
        return False

    def get_presence(self):
        """
        Retrieves the presence of the component
        Returns:
            bool: True if  present, False if not
        """
        return True

    def get_model(self):
        """
        Retrieves the part number of the component
        Returns:
            string: Part number of component
        """
        return 'NA'

    def get_serial(self):
        """
        Retrieves the serial number of the component
        Returns:
            string: Serial number of component
        """
        return 'NA'

    def get_status(self):
        """
        Retrieves the operational status of the component
        Returns:
            bool: True if component is operating properly, False if not
        """
        return True

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return -1

    def is_replaceable(self):
        """
        Indicate whether component is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False

    def get_available_firmware_version(self, image_path):
        """
        Retrieves the available firmware version of the component

        Note: the firmware version will be read from image

        Args:
            image_path: A string, path to firmware image

        Returns:
            A string containing the available firmware version of the component
        """
        avail_ver = None
        valid, version = self._get_available_firmware_version(image_path)
        if valid:
            avail_ver = version.get(self.name)
            if avail_ver:
                avail_ver = avail_ver.get("version")
        else:
            print(version)

        return avail_ver if avail_ver else "NA"

    def get_firmware_update_notification(self, image_path):
        """
        Retrieves a notification on what should be done in order to complete
        the component firmware update

        Args:
            image_path: A string, path to firmware image

        Returns:
            A string containing the component firmware update notification if required.
            By default 'None' value will be used, which indicates that no actions are required
        """
        valid, version = self._get_available_firmware_version(image_path)
        if valid:
            avail_ver = version.get(self.name)
            if avail_ver:
                avail_ver = avail_ver.get("version")
                if avail_ver and avail_ver != self.get_firmware_version():
                    return "Cold reboot is required to perform firmware upgrade"
        else:
            print(version)

        return None

    def install_firmware(self, image_path):
        """
        Installs firmware to the component

        This API performs firmware installation only: this may/may not be the same as firmware update.
        In case platform component requires some extra steps (apart from calling Low Level Utility)
        to load the installed firmware (e.g, reboot, power cycle, etc.) - this must be done manually by user

        Note: in case immediate actions are required to complete the component firmware update
        (e.g., reboot, power cycle, etc.) - will be done automatically by API and no return value provided

        Args:
            image_path: A string, path to firmware image

        Returns:
            A boolean, True if install was successful, False if not
        """
        valid, version = self._get_available_firmware_version(image_path)
        if valid:
            avail_ver = version.get(self.name)
            if avail_ver:
                avail_ver = avail_ver.get("version")
                if avail_ver and avail_ver != self.get_firmware_version():
                    status, msg = self._stage_firmware_package(image_path)
                    print(msg)
                    if status:
                        return True
                    else:
                        return False

            print("INFO: Firmware version up-to-date")
            return True
        else:
            print(version)

        return False

    def update_firmware(self, image_path):
        """
        Updates firmware of the component

        This API performs firmware update: it assumes firmware installation and loading in a single call.
        In case platform component requires some extra steps (apart from calling Low Level Utility)
        to load the installed firmware (e.g, reboot, power cycle, etc.) - this will be done automatically by API

        Args:
            image_path: A string, path to firmware image

        Returns:
            False if image not found.

        Raises:
            RuntimeError: update failed
        """
        if not os.path.isfile(image_path):
            return False

        valid, version = self._get_available_firmware_version(image_path)
        if valid:
            avail_ver = version.get(self.name)
            if avail_ver:
                avail_ver = avail_ver.get("version")
                if avail_ver and avail_ver != self.get_firmware_version():
                    status, msg = self._stage_firmware_package(image_path)
                    if status:
                        print(msg)
                        subprocess.call("reboot")
                    else:
                        raise RuntimeError(msg)

            print("INFO: Firmware version up-to-date")
            return None
        else:
            raise RuntimeError(version)

    def auto_update_firmware(self, image_path, boot_type):
        """
        Updates firmware of the component

        This API performs firmware update automatically based on boot_type: it assumes firmware installation
        and/or creating a loading task during the reboot, if needed, in a single call.
        In case platform component requires some extra steps (apart from calling Low Level Utility)
        to load the installed firmware (e.g, reboot, power cycle, etc.) - this will be done automatically during the reboot.
        The loading task will be created by API.

        Args:
            image_path: A string, path to firmware image
            boot_type: A string, reboot type following the upgrade
                         - none/fast/warm/cold

        Returns:
            Output: A return code
                return_code: An integer number, status of component firmware auto-update
                    - return code of a positive number indicates successful auto-update
                        - status_installed = 1
                        - status_updated = 2
                        - status_scheduled = 3
                    - return_code of a negative number indicates failed auto-update
                        - status_err_boot_type = -1
                        - status_err_image = -2
                        - status_err_unknown = -3

        Raises:
            RuntimeError: auto-update failure cause
        """
        valid, version = self._get_available_firmware_version(image_path)
        if valid:
            avail_ver = version.get(self.name)
            if avail_ver:
                avail_ver = avail_ver.get("version")
                if avail_ver and avail_ver != self.get_firmware_version():
                    if boot_type != "cold":
                        return -1

                    status, msg = self._stage_firmware_package(image_path)
                    if status:
                        print(msg)
                        return 3
                    else:
                        raise RuntimeError(msg)

            print("INFO: Firmware version up-to-date")
            return 1
        else:
            print(version)
            return -2
