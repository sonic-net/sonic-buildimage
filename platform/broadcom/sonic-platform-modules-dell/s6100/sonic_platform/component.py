#!/usr/bin/env python

########################################################################
# DELLEMC S6100
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Components' (e.g., BIOS, CPLD, FPGA, etc.) available in
# the platform
#
########################################################################

try:
    import json
    import os
    import re
    import subprocess
    import tarfile
    from sonic_platform_base.component_base import ComponentBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

BIOS_QUERY_VERSION_COMMAND = "dmidecode -s system-version"
SSD_VERSION_COMMAND = "ssdutil -v"
SSD_UPGRADE_SCHEDULE = "/usr/local/bin/ssd_upgrade_schedule"
PCI_VERSION_COMMAND = "lspci -s 0:0.0"
ONIE_VERSION_COMMAND = "/usr/local/bin/onie_version"


class Component(ComponentBase):
    """DellEMC Platform-specific Component class"""

    HWMON_DIR = "/sys/devices/platform/SMF.512/hwmon/"
    HWMON_NODE = os.listdir(HWMON_DIR)[0]
    MAILBOX_DIR = HWMON_DIR + HWMON_NODE

    CHASSIS_COMPONENTS = [
        ["BIOS", ("Performs initialization of hardware components during "
                  "booting")],
        ["FPGA", ("Platform management controller for on-board temperature "
                  "monitoring, in-chassis power, Fan and LED control")],
        ["CPLD", "Used for managing IO modules, SFP+ modules and system LEDs"],
        ["SSD", "Solid State Drive that stores data persistently"],
        ["ONIE", "Open Network Install Environment"]
    ]
    MODULE_COMPONENT = [
        "IOM{}-CPLD",
        "Used for managing QSFP+ modules ({0}-{1})"
    ]

    def __init__(self, component_index=0,
                 is_module=False, iom_index=0, i2c_line=0, dependency=None):

        ComponentBase.__init__(self)
        self.is_module_component = is_module
        self.dependency = dependency

        if self.is_module_component:
            self.index = iom_index
            self.name = self.MODULE_COMPONENT[0].format(self.index)
            self.description = self.MODULE_COMPONENT[1].format(
                               ((self.index - 1) * 16) + 1, self.index * 16)
            self.cpld_version_file = ("/sys/bus/i2c/devices/i2c-{0}/{0}-003e"
                                      "/iom_cpld_vers").format(i2c_line)
        else:
            self.index = component_index
            self.name = self.CHASSIS_COMPONENTS[self.index][0]
            self.description = self.CHASSIS_COMPONENTS[self.index][1]

    def _read_sysfs_file(self, sysfs_file):
        # On successful read, returns the value read from given
        # sysfs_file and on failure returns 'ERR'
        rv = 'ERR'

        if (not os.path.isfile(sysfs_file)):
            return rv

        try:
            with open(sysfs_file, 'r') as fd:
                rv = fd.read()
        except Exception as error:
            rv = 'ERR'

        rv = rv.rstrip('\r\n')
        rv = rv.lstrip(" ")
        return rv

    def _get_command_result(self, cmdline):
        try:
            proc = subprocess.Popen(cmdline.split(), stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT)
            stdout = proc.communicate()[0]
            proc.wait()
            result = stdout.decode('utf-8').rstrip('\n')
        except OSError:
            result = None

        return result

    def _get_cpld_version(self):
        io_resource = "/dev/port"
        CPLD_VERSION_ADDR = 0x100

        fd = os.open(io_resource, os.O_RDONLY)
        if (fd < 0):
            return 'NA'

        if (os.lseek(fd, CPLD_VERSION_ADDR, os.SEEK_SET) != CPLD_VERSION_ADDR):
            os.close(fd)
            return 'NA'

        buf = os.read(fd, 1)
        cpld_version = str(ord(buf))
        os.close(fd)

        return cpld_version

    def _get_iom_cpld_version(self):
        ver_str = self._read_sysfs_file(self.cpld_version_file)
        if (ver_str != 'ERR'):
            if ver_str == 'read error':
                return 'NA'

            ver_str = ver_str.rstrip('\r\n')
            cpld_version = str(int(ver_str.split(':')[1], 16))
            return cpld_version
        else:
            return 'NA'

    def _get_fpga_version(self):
        fpga_ver_file = self.MAILBOX_DIR + '/smf_firmware_ver'
        fpga_ver = self._read_sysfs_file(fpga_ver_file)
        if (fpga_ver != 'ERR'):
            return fpga_ver
        else:
            return 'NA'

    def _get_ssd_version(self):
        rv = 'NA'
        ssd_ver = self._get_command_result(SSD_VERSION_COMMAND)
        if not ssd_ver:
            return rv
        else:
            version = re.search(r'Firmware\s*:(.*)',ssd_ver)
            if version:
                rv = version.group(1).strip()
        return rv

    def _get_available_firmware_version(self, image_path):
        if not os.path.isfile(image_path):
            return False, "ERROR: File not found"

        try:
            updater = tarfile.open(image_path, "r")
        except tarfile.ReadError:
            return False, "ERROR: Unable to extract firmware updater"

        try:
            ver_info_fd = updater.extractfile("fw-component-version")
        except KeyError:
            updater.close()
            return False, "ERROR: Version info not available"

        ver_info = json.load(ver_info_fd)
        ver_info_fd.close()
        updater.close()

        return True, ver_info

    def get_name(self):
        """
        Retrieves the name of the component

        Returns:
            A string containing the name of the component
        """
        return self.name

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

    def get_presence(self):
        """
        Retrieves the presence of the component
        Returns:
            bool: True if  present, False if not
        """
        if self.is_module_component:
            return self.dependency.get_presence()
        else:
            return True

    def get_status(self):
        """
        Retrieves the operational status of the component
        Returns:
            bool: True if component is operating properly, False if not
        """
        if self.is_module_component:
            return self.dependency.get_status()
        else:
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
        if self.is_module_component:    # IOM CPLD
            return self._get_iom_cpld_version()
        else:
            if self.index == 0:         # BIOS
                bios_ver = self._get_command_result(BIOS_QUERY_VERSION_COMMAND)

                if not bios_ver:
                    return 'NA'
                else:
                    return bios_ver

            elif self.index == 1:       # FPGA
                return self._get_fpga_version()
            elif self.index == 2:       # SwitchCard CPLD
                return self._get_cpld_version()
            elif self.index == 3:       #SSD
                return self._get_ssd_version()
            elif self.index == 4:       # ONIE
                try:
                    return subprocess.check_output(ONIE_VERSION_COMMAND, text=True).strip()
                except (FileNotFoundError, subprocess.CalledProcessError):
                    return 'NA'

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
        if self.index == 2:         # SwitchCard CPLD
            valid, version = self._get_available_firmware_version(image_path)
            pci_ver = self._get_command_result(PCI_VERSION_COMMAND)
            if valid:
                if pci_ver:
                    board_ver = re.search(r"\(rev ([0-9]{2})\)$", pci_ver)
                    if board_ver:
                        board_ver = board_ver.group(1).strip()
                        board_type = 'B0' if board_ver == '02' else 'C0'
                        cpld_ver = self._get_cpld_version()
                        avail_ver = version.get(board_type) if board_type == 'B0' else cpld_ver
            else:
                print(version)

        elif self.index == 3:       # SSD
            valid, version = self._get_available_firmware_version(image_path)
            ssd_ver = self._get_command_result(SSD_VERSION_COMMAND)
            if valid:
                if ssd_ver:
                    ssd_model = re.search(r'Device Model\s*:.*(3IE[3]{0,1})', ssd_ver)
                    if ssd_model:
                        ssd_model = ssd_model.group(1).strip()
                        avail_ver = version.get(ssd_model)
            else:
                print(version)

        return avail_ver if avail_ver else "NA"

    def install_firmware(self, image_path):
        """
        Installs firmware to the component

        Args:
            image_path: A string, path to firmware image

        Returns:
            A boolean, True if install was successful, False if not
        """
        return False

    def update_firmware(self,image_path):
       """
         Updates firmware to the componenent

        Args:
            image_path: A string, path to firmware image

        Returns:
            A boolean, True if install was successful, False if not

       """
       return False
