#
# Copyright (c) 2019-2023 NVIDIA CORPORATION & AFFILIATES.
# Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
#############################################################################
# Mellanox
#
# implementation of new platform api
#############################################################################


try:
    import os
    import io
    import re
    import sys
    import glob
    import tempfile
    import subprocess
    from sonic_py_common import device_info
    from sonic_py_common.general import check_output_pipe
    if sys.version_info[0] > 2:
        import configparser
    else:
        import ConfigParser as configparser

    from shutil import copyfile

    from sonic_platform_base.component_base import ComponentBase,           \
                                                    FW_AUTO_INSTALLED,      \
                                                    FW_AUTO_UPDATED,        \
                                                    FW_AUTO_SCHEDULED,      \
                                                    FW_AUTO_ERR_BOOT_TYPE,  \
                                                    FW_AUTO_ERR_IMAGE,      \
                                                    FW_AUTO_ERR_UNKNOWN

except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class MPFAManager(object):
    MPFA_EXTENSION = '.mpfa'

    MPFA_EXTRACT_COMMAND = ['tar', 'xzf', '', '-C', '']
    MPFA_CLEANUP_COMMAND = ['rm', '-rf', '']

    def __init__(self, mpfa_path):
        self.__mpfa_path = mpfa_path
        self.__contents_path = None
        self.__metadata = None

    def __enter__(self):
        self.extract()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.cleanup()

    def __validate_path(self, mpfa_path):
        if not os.path.isfile(mpfa_path):
            raise RuntimeError("MPFA doesn't exist: path={}".format(mpfa_path))

        name, ext = os.path.splitext(mpfa_path)
        if ext != self.MPFA_EXTENSION:
            raise RuntimeError("MPFA doesn't have valid extension: path={}".format(mpfa_path))

    def __extract_contents(self, mpfa_path):
        contents_path = tempfile.mkdtemp(prefix='mpfa-')

        self.MPFA_EXTRACT_COMMAND[2] = mpfa_path
        self.MPFA_EXTRACT_COMMAND[4] = contents_path
        subprocess.check_call(self.MPFA_EXTRACT_COMMAND, universal_newlines=True)

        self.__contents_path = contents_path

    def __parse_metadata(self, contents_path):
        metadata_path = os.path.join(contents_path, 'metadata.ini')

        if not os.path.isfile(metadata_path):
            raise RuntimeError("MPFA metadata doesn't exist: path={}".format(metadata_path))

        cp = configparser.ConfigParser()
        with io.open(metadata_path, 'r') as metadata_ini:
            cp.readfp(metadata_ini)

        self.__metadata = cp

    def extract(self):
        if self.is_extracted():
            return

        self.__validate_path(self.__mpfa_path)
        self.__extract_contents(self.__mpfa_path)
        self.__parse_metadata(self.__contents_path)

    def cleanup(self):
        if os.path.exists(self.__contents_path):
            self.MPFA_CLEANUP_COMMAND[2] = self.__contents_path
            subprocess.check_call(self.MPFA_CLEANUP_COMMAND, universal_newlines=True)

        self.__contents_path = None
        self.__metadata = None

    def get_path(self):
        return self.__contents_path

    def get_metadata(self):
        return self.__metadata

    def is_extracted(self):
        return self.__contents_path is not None and os.path.exists(self.__contents_path)


class ONIEUpdater(object):
    ONIE_FW_UPDATE_CMD_ADD = ['/usr/bin/mlnx-onie-fw-update.sh', 'add', '']
    ONIE_FW_UPDATE_CMD_REMOVE = ['/usr/bin/mlnx-onie-fw-update.sh', 'remove', '']
    ONIE_FW_UPDATE_CMD_UPDATE = ['/usr/bin/mlnx-onie-fw-update.sh', 'update']
    ONIE_FW_UPDATE_CMD_INSTALL = ['/usr/bin/mlnx-onie-fw-update.sh', 'update', '--no-reboot']
    ONIE_FW_UPDATE_CMD_SHOW_PENDING = ['/usr/bin/mlnx-onie-fw-update.sh', 'show-pending']

    ONIE_VERSION_PARSE_PATTERN = '([0-9]{4})\.([0-9]{2})-([0-9]+)\.([0-9]+)\.([0-9]+)-?(dev)?-([0-9]+)'
    ONIE_VERSION_BASE_PARSE_PATTERN = '([0-9]+)\.([0-9]+)\.([0-9]+)'
    ONIE_VERSION_REQUIRED = '5.2.0016'

    ONIE_VERSION_ATTR = 'onie_version'
    ONIE_NO_PENDING_UPDATES_ATTR = 'No pending firmware updates present'

    ONIE_IMAGE_INFO_COMMAND = ['/bin/bash', '', '-q', '-i']

    # Upgrading fireware from ONIE is not supported from the beginning on some platforms, like SN2700.
    # There is a logic to check the ONIE version in order to know whether it is supported.
    # If it is not supported, we will not proceed and print some error message.
    # For SN2201, upgrading fireware from ONIE is supported from day one so we do not need to check it.
    PLATFORM_ALWAYS_SUPPORT_UPGRADE = ['x86_64-nvidia_sn2201-r0']

    BIOS_UPDATE_FILE_EXT_ROM = '.rom'
    BIOS_UPDATE_FILE_EXT_CAB = '.cab'

    def __init__(self):
        self.platform = device_info.get_platform()

    def __add_prefix(self, image_path):
        if image_path.endswith(self.BIOS_UPDATE_FILE_EXT_CAB):
            return image_path;
        elif self.BIOS_UPDATE_FILE_EXT_ROM not in image_path:
            rename_path = "/tmp/00-{}".format(os.path.basename(image_path))
        else:
            rename_path = "/tmp/99-{}".format(os.path.basename(image_path))

        copyfile(image_path, rename_path)

        return rename_path

    def __mount_onie_fs(self):
        fs_mountpoint = '/mnt/onie-fs'
        onie_path = '/lib/onie'

        if os.path.lexists(onie_path) or os.path.exists(fs_mountpoint):
            self.__umount_onie_fs()

        cmd = "fdisk -l | grep 'ONIE boot' | awk '{print $1}'"
        cmd1 = ['fdisk', '-l']
        cmd2 = ['grep', 'ONIE boot']
        cmd3 = ['awk', '{print $1}']
        fs_path = check_output_pipe(cmd1, cmd2, cmd3).rstrip('\n')

        os.mkdir(fs_mountpoint)
        cmd = ["mount", "-n", "-r", "-t", "ext4", fs_path, fs_mountpoint]
        subprocess.check_call(cmd, universal_newlines=True)

        fs_onie_path = os.path.join(fs_mountpoint, 'onie/tools/lib/onie')
        os.symlink(fs_onie_path, onie_path)

        return fs_mountpoint

    def __umount_onie_fs(self):
        fs_mountpoint = '/mnt/onie-fs'
        onie_path = '/lib/onie'

        if os.path.islink(onie_path):
            os.unlink(onie_path)

        if os.path.ismount(fs_mountpoint):
            cmd = ["umount", "-rf", fs_mountpoint]
            subprocess.check_call(cmd, universal_newlines=True)

        if os.path.exists(fs_mountpoint):
            os.rmdir(fs_mountpoint)

    def __stage_update(self, image_path):
        rename_path = self.__add_prefix(image_path)

        self.ONIE_FW_UPDATE_CMD_ADD[2] = rename_path

        try:
            subprocess.check_call(self.ONIE_FW_UPDATE_CMD_ADD, universal_newlines=True)
        except subprocess.CalledProcessError as e:
            raise RuntimeError("Failed to stage firmware update: {}".format(str(e)))

    def __unstage_update(self, image_path):
        rename_path = self.__add_prefix(image_path)

        self.ONIE_FW_UPDATE_CMD_REMOVE[2] = os.path.basename(rename_path)

        try:
            subprocess.check_call(self.ONIE_FW_UPDATE_CMD_REMOVE, universal_newlines=True)
        except subprocess.CalledProcessError as e:
            raise RuntimeError("Failed to unstage firmware update: {}".format(str(e)))

    def __trigger_update(self, allow_reboot):
        if allow_reboot:
            cmd = self.ONIE_FW_UPDATE_CMD_UPDATE
        else:
            cmd = self.ONIE_FW_UPDATE_CMD_INSTALL

        try:
            subprocess.check_call(cmd, universal_newlines=True)
        except subprocess.CalledProcessError as e:
            raise RuntimeError("Failed to trigger firmware update: {}".format(str(e)))

    def __is_update_staged(self, image_path):
        cmd = self.ONIE_FW_UPDATE_CMD_SHOW_PENDING

        try:
            output = subprocess.check_output(cmd,
                                             stderr=subprocess.STDOUT,
                                             universal_newlines=True).rstrip('\n')
        except subprocess.CalledProcessError as e:
            raise RuntimeError("Failed to get pending firmware updates: {}".format(str(e)))

        rename_path = self.__add_prefix(image_path)
        basename = os.path.basename(rename_path)

        for line in output.splitlines():
            if line.startswith(basename):
                return True

        return False

    def parse_onie_version(self, version, is_base=False):
        onie_year = None
        onie_month = None
        onie_major = None
        onie_minor = None
        onie_release = None
        onie_baudrate = None

        if is_base:
            pattern = self.ONIE_VERSION_BASE_PARSE_PATTERN

            m = re.search(pattern, version)
            if not m:
                raise RuntimeError("Failed to parse ONIE version: pattern={}, version={}".format(pattern, version))

            onie_major = m.group(1)
            onie_minor = m.group(2)
            onie_release = m.group(3)

            return onie_year, onie_month, onie_major, onie_minor, onie_release, onie_baudrate

        pattern = self.ONIE_VERSION_PARSE_PATTERN

        m = re.search(pattern, version)
        if not m:
            raise RuntimeError("Failed to parse ONIE version: pattern={}, version={}".format(pattern, version))

        onie_year = m.group(1)
        onie_month = m.group(2)
        onie_major = m.group(3)
        onie_minor = m.group(4)
        onie_release = m.group(5)
        onie_signtype = m.group(6)
        onie_baudrate = m.group(7)

        return onie_year, onie_month, onie_major, onie_minor, onie_release, onie_baudrate

    def get_onie_required_version(self):
        return self.ONIE_VERSION_REQUIRED

    def get_onie_version(self):
        version = None

        try:
            fs_mountpoint = self.__mount_onie_fs()
            machine_conf_path = os.path.join(fs_mountpoint, 'onie/grub/grub-machine.cfg')

            with open(machine_conf_path, 'r') as machine_conf:
                for line in machine_conf:
                    if line.startswith(self.ONIE_VERSION_ATTR):
                        items = line.rstrip('\n').split('=')

                        if len(items) != 2:
                            raise RuntimeError("Failed to parse ONIE info: line={}".format(line))

                        version = items[1]
                        break

            if version is None:
                raise RuntimeError("Failed to parse ONIE version")
        finally:
            self.__umount_onie_fs()

        return version

    def get_onie_firmware_info(self, image_path):
        firmware_info = { }

        try:
            self.__mount_onie_fs()

            self.ONIE_IMAGE_INFO_COMMAND[1] = image_path

            try:
                output = subprocess.check_output(self.ONIE_IMAGE_INFO_COMMAND,
                                                 stderr=subprocess.STDOUT,
                                                 universal_newlines=True).rstrip('\n')
            except subprocess.CalledProcessError as e:
                raise RuntimeError("Failed to get ONIE firmware info: {}".format(str(e)))

            for line in output.splitlines():
                items = line.split('=')

                if len(items) != 2:
                    raise RuntimeError("Failed to parse ONIE firmware info: line={}".format(line))

                firmware_info[items[0]] = items[1]
        finally:
            self.__umount_onie_fs()

        return firmware_info

    def update_firmware(self, image_path, allow_reboot=True):

        try:
            self.__stage_update(image_path)
            self.__trigger_update(allow_reboot)
        except:
            if self.__is_update_staged(image_path):
                self.__unstage_update(image_path)
            raise

    def is_non_onie_firmware_update_supported(self):
        if self.platform in self.PLATFORM_ALWAYS_SUPPORT_UPGRADE:
            return True

        current_version = self.get_onie_version()
        _, _, major1, minor1, release1, _ = self.parse_onie_version(current_version)
        version1 = int("{}{}{}".format(major1, minor1, release1))

        required_version = self.get_onie_required_version()
        _, _, major2, minor2, release2, _ = self.parse_onie_version(required_version, True)
        version2 = int("{}{}{}".format(major2, minor2, release2))

        return version1 >= version2


class Component(ComponentBase):
    def __init__(self):
        super(Component, self).__init__()
        self.name = None
        self.description = None
        self.image_ext_name = None

    def get_name(self):
        return self.name

    def get_description(self):
        return self.description

    def auto_update_firmware(self, image_path, boot_action):
        """
        Default handling of attempted automatic update for a component of a Mellanox switch.
        Will skip the installation if the boot_action is 'warm' or 'fast' and will call update_firmware()
        if boot_action is fast.
        """

        # Verify image path exists
        if not os.path.exists(image_path):
            # Invalid image path
            return FW_AUTO_ERR_IMAGE

        # boot_type did not match (skip)
        if boot_action != "cold":
            return FW_AUTO_ERR_BOOT_TYPE

        # Install firmware
        if not self.install_firmware(image_path, allow_reboot=False):
            return FW_AUTO_ERR_UNKNOWN

        # Installed pending next reboot
        return FW_AUTO_INSTALLED

    @staticmethod
    def _read_generic_file(filename, len, ignore_errors=False):
        """
        Read a generic file, returns the contents of the file
        """
        result = None

        try:
            with io.open(filename, 'r') as fileobj:
                result = fileobj.read(len)
        except IOError as e:
            if not ignore_errors:
                raise RuntimeError("Failed to read file {} due to {}".format(filename, repr(e)))

        return result

    def _check_file_validity(self, image_path):
        if not os.path.isfile(image_path):
            print("ERROR: File {} doesn't exist or is not a file".format(image_path))
            return False

        name_list = os.path.splitext(image_path)
        if self.image_ext_name is not None:
            if name_list[1] not in self.image_ext_name:
                print("ERROR: Extend name of file {} is wrong. Image for {} should have extend name {}".format(image_path, self.name, self.image_ext_name))
                return False

        return True


class ComponentONIE(Component):
    COMPONENT_NAME = 'ONIE'
    COMPONENT_DESCRIPTION = 'ONIE - Open Network Install Environment'

    ONIE_IMAGE_VERSION_ATTR = 'image_version'

    def __init__(self):
        super(ComponentONIE, self).__init__()

        self.name = self.COMPONENT_NAME
        self.description = self.COMPONENT_DESCRIPTION
        self.onie_updater = ONIEUpdater()

    def __install_firmware(self, image_path, allow_reboot=True):
        if not self._check_file_validity(image_path):
            return False

        try:
            print("INFO: Staging {} firmware update with ONIE updater".format(self.name))
            self.onie_updater.update_firmware(image_path, allow_reboot)
        except Exception as e:
            print("ERROR: Failed to update {} firmware: {}".format(self.name, str(e)))
            return False

        return True

    def get_firmware_version(self):
        return self.onie_updater.get_onie_version()

    def get_available_firmware_version(self, image_path):
        firmware_info = self.onie_updater.get_onie_firmware_info(image_path)
        if self.ONIE_IMAGE_VERSION_ATTR not in firmware_info:
            raise RuntimeError("Failed to get {} available firmware version".format(self.name))

        return firmware_info[self.ONIE_IMAGE_VERSION_ATTR]

    def get_firmware_update_notification(self, image_path):
        return "Immediate cold reboot is required to complete {} firmware update".format(self.name)

    def install_firmware(self, image_path, allow_reboot=True):
        return self.__install_firmware(image_path, allow_reboot)

    def update_firmware(self, image_path):
        self.__install_firmware(image_path)


class ComponentSSD(Component):
    COMPONENT_NAME = 'SSD'
    COMPONENT_DESCRIPTION = 'SSD - Solid-State Drive'
    COMPONENT_FIRMWARE_EXTENSION = ['.pkg']

    FIRMWARE_VERSION_ATTR = 'Firmware Version'
    AVAILABLE_FIRMWARE_VERSION_ATTR = 'Available Firmware Version'
    POWER_CYCLE_REQUIRED_ATTR = 'Power Cycle Required'
    UPGRADE_REQUIRED_ATTR = 'Upgrade Required'

    SSD_INFO_COMMAND = ["/usr/bin/mlnx-ssd-fw-update.sh", "-q"]
    SSD_FIRMWARE_INFO_COMMAND = ["/usr/bin/mlnx-ssd-fw-update.sh", "-q", "-i", ""]
    SSD_FIRMWARE_INSTALL_COMMAND = ["/usr/bin/mlnx-ssd-fw-update.sh", "--no-power-cycle", "-y", "-u", "-i", ""]
    SSD_FIRMWARE_UPDATE_COMMAND = ["/usr/bin/mlnx-ssd-fw-update.sh", "-y", "-u", "-i", ""]

    def __init__(self):
        super(ComponentSSD, self).__init__()

        self.name = self.COMPONENT_NAME
        self.description = self.COMPONENT_DESCRIPTION
        self.image_ext_name = self.COMPONENT_FIRMWARE_EXTENSION

    def __install_firmware(self, image_path, allow_reboot=True):
        if not self._check_file_validity(image_path):
            return False

        if allow_reboot:
            self.SSD_FIRMWARE_UPDATE_COMMAND[4] = image_path
            cmd = self.SSD_FIRMWARE_UPDATE_COMMAND
        else:
            self.SSD_FIRMWARE_INSTALL_COMMAND[5] = image_path
            cmd = self.SSD_FIRMWARE_INSTALL_COMMAND

        try:
            print("INFO: Installing {} firmware update".format(self.name))
            subprocess.check_call(cmd, universal_newlines=True)
        except subprocess.CalledProcessError as e:
            print("ERROR: Failed to update {} firmware: {}".format(self.name, str(e)))
            return False

        return True

    def auto_update_firmware(self, image_path, boot_action):
        """
        Handling of attempted automatic update for a SSD of a Mellanox switch.
        Will first check the image_path to determine if a post-install reboot is required,
        then compares it against boot_action to determine whether to proceed with install.
        """

        # Verify image path exists
        if not os.path.exists(image_path):
            # Invalid image path
            return FW_AUTO_ERR_IMAGE

        # Check if post_install reboot is required
        try:
            reboot_required = self.get_firmware_update_notification(image_path) is not None
        except RuntimeError as e:
            return FW_AUTO_ERR_UNKNOWN

        # Update if no reboot needed
        if not reboot_required:
            self.update_firmware(image_path)
            return FW_AUTO_UPDATED

        # boot_type did not match (skip)
        if boot_action != "cold":
            return FW_AUTO_ERR_BOOT_TYPE

        # Schedule if we need a cold boot
        return FW_AUTO_SCHEDULED

    def get_firmware_version(self):
        try:
            output = subprocess.check_output(self.SSD_INFO_COMMAND,
                                             stderr=subprocess.STDOUT,
                                             universal_newlines=True).rstrip('\n')
        except subprocess.CalledProcessError as e:
            raise RuntimeError("Failed to get {} info: {}".format(self.name, str(e)))

        for line in output.splitlines():
            if line.startswith(self.FIRMWARE_VERSION_ATTR):
                return line.split(':')[1].lstrip(' \t')

        raise RuntimeError("Failed to parse {} version".format(self.name))

    def get_available_firmware_version(self, image_path):
        self.SSD_FIRMWARE_INFO_COMMAND[3] = image_path

        try:
            output = subprocess.check_output(self.SSD_FIRMWARE_INFO_COMMAND,
                                             stderr=subprocess.STDOUT,
                                             universal_newlines=True).rstrip('\n')
        except subprocess.CalledProcessError as e:
            raise RuntimeError("Failed to get {} firmware info: {}".format(self.name, str(e)))

        current_firmware_version = None
        available_firmware_version = None
        upgrade_required = None

        for line in output.splitlines():
            if line.startswith(self.FIRMWARE_VERSION_ATTR):
                current_firmware_version = line.split(':')[1].lstrip(' \t')
            if line.startswith(self.AVAILABLE_FIRMWARE_VERSION_ATTR):
                available_firmware_version = line.split(':')[1].lstrip(' \t')
            if line.startswith(self.UPGRADE_REQUIRED_ATTR):
                upgrade_required = line.split(':')[1].lstrip(' \t')

        if upgrade_required is None or upgrade_required not in ['yes', 'no']:
            raise RuntimeError("Failed to parse {} firmware upgrade status".format(self.name))

        if upgrade_required == 'no':
            if current_firmware_version is None:
                raise RuntimeError("Failed to parse {} current firmware version".format(self.name))
            return current_firmware_version

        if available_firmware_version is None:
            raise RuntimeError("Failed to parse {} available firmware version".format(self.name))

        return available_firmware_version

    def get_firmware_update_notification(self, image_path):
        self.SSD_FIRMWARE_INFO_COMMAND[3] = image_path

        try:
            output = subprocess.check_output(self.SSD_FIRMWARE_INFO_COMMAND,
                                             stderr=subprocess.STDOUT,
                                             universal_newlines=True).rstrip('\n')
        except subprocess.CalledProcessError as e:
            raise RuntimeError("Failed to get {} firmware info: {}".format(self.name, str(e)))

        power_cycle_required = None
        upgrade_required = None

        for line in output.splitlines():
            if line.startswith(self.POWER_CYCLE_REQUIRED_ATTR):
                power_cycle_required = line.split(':')[1].lstrip(' \t')
            if line.startswith(self.UPGRADE_REQUIRED_ATTR):
                upgrade_required = line.split(':')[1].lstrip(' \t')

        if upgrade_required is None or upgrade_required not in ['yes', 'no']:
            raise RuntimeError("Failed to parse {} firmware upgrade status".format(self.name))

        if upgrade_required == 'no':
            return None

        if power_cycle_required is None or power_cycle_required not in ['yes', 'no']:
            raise RuntimeError("Failed to parse {} firmware power policy".format(self.name))

        notification = None

        if power_cycle_required == 'yes':
            notification = "Immediate power cycle is required to complete {} firmware update".format(self.name)

        return notification

    def install_firmware(self, image_path, allow_reboot=True):
        return self.__install_firmware(image_path, allow_reboot)

    def update_firmware(self, image_path):
        self.__install_firmware(image_path)


class ComponentBIOS(Component):
    COMPONENT_NAME = 'BIOS'
    COMPONENT_DESCRIPTION = 'BIOS - Basic Input/Output System'
    COMPONENT_FIRMWARE_EXTENSION = ['.rom', '.cab']

    BIOS_VERSION_COMMAND = ['dmidecode', '--oem-string', '1']

    def __init__(self):
        super(ComponentBIOS, self).__init__()

        self.name = self.COMPONENT_NAME
        self.description = self.COMPONENT_DESCRIPTION
        self.image_ext_name = self.COMPONENT_FIRMWARE_EXTENSION
        self.onie_updater = ONIEUpdater()

    def __install_firmware(self, image_path, allow_reboot=True):
        if not self.onie_updater.is_non_onie_firmware_update_supported():
            print("ERROR: ONIE {} or later is required".format(self.onie_updater.get_onie_required_version()))
            return False

        if not self._check_file_validity(image_path):
            return False

        try:
            print("INFO: Staging {} firmware update with ONIE updater".format(self.name))
            self.onie_updater.update_firmware(image_path, allow_reboot)
        except Exception as e:
            print("ERROR: Failed to update {} firmware: {}".format(self.name, str(e)))
            return False

        return True

    def get_firmware_version(self):
        try:
            version = subprocess.check_output(self.BIOS_VERSION_COMMAND,
                                              stderr=subprocess.STDOUT,
                                              universal_newlines=True).rstrip('\n')
        except subprocess.CalledProcessError as e:
            raise RuntimeError("Failed to get {} version: {}".format(self.name, str(e)))

        return version

    def get_available_firmware_version(self, image_path):
        raise NotImplementedError("{} component doesn't support firmware version query".format(self.name))

    def get_firmware_update_notification(self, image_path):
        return "Immediate cold reboot is required to complete {} firmware update".format(self.name)

    def install_firmware(self, image_path, allow_reboot=True):
        return self.__install_firmware(image_path, allow_reboot)

    def update_firmware(self, image_path):
        self.__install_firmware(image_path)


class ComponentBIOSSN2201(Component):
    COMPONENT_NAME = 'BIOS'
    COMPONENT_DESCRIPTION = 'BIOS - Basic Input/Output System'

    BIOS_VERSION_COMMAND = ['dmidecode', '-t0']

    def __init__(self):
        super(ComponentBIOSSN2201, self).__init__()

        self.name = self.COMPONENT_NAME
        self.description = self.COMPONENT_DESCRIPTION

    def get_firmware_version(self):
        try:
            output = subprocess.check_output(self.BIOS_VERSION_COMMAND,
                                             stderr=subprocess.STDOUT,
                                             universal_newlines=True).rstrip('\n')
        except subprocess.CalledProcessError as e:
            raise RuntimeError("Failed to get {} version: {}".format(self.name, str(e)))

        match = re.search('Version: (.*)', output)
        if match:
            version = match.group(1)
        else:
            version = 'Unknown version'

        return version


class ComponentCPLD(Component):
    COMPONENT_NAME = 'CPLD{}'
    COMPONENT_DESCRIPTION = 'CPLD - Complex Programmable Logic Device'
    COMPONENT_FIRMWARE_EXTENSION = ['.vme']

    MST_DEVICE_PATH = '/dev/mst'
    MST_DEVICE_PATTERN = 'mt[0-9]*_pci_cr0'

    CPLD_NUMBER_FILE = '/var/run/hw-management/config/cpld_num'
    CPLD_PART_NUMBER_FILE = '/var/run/hw-management/system/cpld{}_pn'
    CPLD_VERSION_FILE = '/var/run/hw-management/system/cpld{}_version'
    CPLD_VERSION_MINOR_FILE = '/var/run/hw-management/system/cpld{}_version_min'

    CPLD_NUMBER_MAX_LENGTH = 1
    CPLD_PART_NUMBER_MAX_LENGTH = 6
    CPLD_VERSION_MAX_LENGTH = 2
    CPLD_VERSION_MINOR_MAX_LENGTH = 2

    CPLD_PART_NUMBER_DEFAULT = '0'
    CPLD_VERSION_MINOR_DEFAULT = '0'

    CPLD_FIRMWARE_UPDATE_COMMAND = ['cpldupdate', '--dev', '', '--print-progress', '']

    def __init__(self, idx):
        super(ComponentCPLD, self).__init__()

        self.idx = idx
        self.name = self.COMPONENT_NAME.format(self.idx)
        self.description = self.COMPONENT_DESCRIPTION
        self.image_ext_name = self.COMPONENT_FIRMWARE_EXTENSION

    def __get_mst_device(self):
        if not os.path.exists(self.MST_DEVICE_PATH):
            print("ERROR: mst driver is not loaded")
            return None

        pattern = os.path.join(self.MST_DEVICE_PATH, self.MST_DEVICE_PATTERN)

        mst_dev_list = glob.glob(pattern)
        if not mst_dev_list or len(mst_dev_list) != 1:
            devices = str(os.listdir(self.MST_DEVICE_PATH))
            print("ERROR: Failed to get mst device: pattern={}, devices={}".format(pattern, devices))
            return None

        return mst_dev_list[0]

    def _install_firmware(self, image_path):
        if not self._check_file_validity(image_path):
            return False

        mst_dev = self.__get_mst_device()
        if mst_dev is None:
            return False
        self.CPLD_FIRMWARE_UPDATE_COMMAND[2] = mst_dev
        self.CPLD_FIRMWARE_UPDATE_COMMAND[4] = image_path
        cmd = self.CPLD_FIRMWARE_UPDATE_COMMAND

        try:
            print("INFO: Installing {} firmware update: path={}".format(self.name, image_path))
            subprocess.check_call(cmd, universal_newlines=True)
        except subprocess.CalledProcessError as e:
            print("ERROR: Failed to update {} firmware: {}".format(self.name, str(e)))
            return False

        return True

    def auto_update_firmware(self, image_path, boot_action):
        """
        Default handling of attempted automatic update for a component of a Mellanox switch.
        Will skip the installation if the boot_action is 'warm' or 'fast' and will call update_firmware()
        if boot_action is fast.
        """

        # Verify image path exists
        if not os.path.exists(image_path):
            # Invalid image path
            return FW_AUTO_ERR_IMAGE

        # boot_type did not match (skip)
        if boot_action != "cold":
            return FW_AUTO_ERR_BOOT_TYPE

        # Install burn. Error if fail.
        if not self.install_firmware(image_path):
            return FW_AUTO_ERR_UNKNOWN

        # Schedule refresh
        return FW_AUTO_SCHEDULED

    def get_firmware_version(self):
        part_number_file = self.CPLD_PART_NUMBER_FILE.format(self.idx)
        version_file = self.CPLD_VERSION_FILE.format(self.idx)
        version_minor_file = self.CPLD_VERSION_MINOR_FILE.format(self.idx)

        part_number = self._read_generic_file(part_number_file, self.CPLD_PART_NUMBER_MAX_LENGTH, True)
        version = self._read_generic_file(version_file, self.CPLD_VERSION_MAX_LENGTH)
        version_minor = self._read_generic_file(version_minor_file, self.CPLD_VERSION_MINOR_MAX_LENGTH, True)

        if part_number is None:
            part_number = self.CPLD_PART_NUMBER_DEFAULT

        if version_minor is None:
            version_minor = self.CPLD_VERSION_MINOR_DEFAULT

        part_number = part_number.rstrip('\n').zfill(self.CPLD_PART_NUMBER_MAX_LENGTH)
        version = version.rstrip('\n').zfill(self.CPLD_VERSION_MAX_LENGTH)
        version_minor = version_minor.rstrip('\n').zfill(self.CPLD_VERSION_MINOR_MAX_LENGTH)

        return "CPLD{}_REV{}{}".format(part_number, version, version_minor)

    def get_available_firmware_version(self, image_path):
        with MPFAManager(image_path) as mpfa:
            if not mpfa.get_metadata().has_option('version', self.name):
                raise RuntimeError("Failed to get {} available firmware version".format(self.name))

            return mpfa.get_metadata().get('version', self.name)

    def get_firmware_update_notification(self, image_path):
        name, ext = os.path.splitext(os.path.basename(image_path))
        if ext in self.COMPONENT_FIRMWARE_EXTENSION:
            return "Power cycle (with 30 sec delay) or refresh image is required to complete {} firmware update".format(self.name)

        return "Immediate power cycle is required to complete {} firmware update".format(self.name)

    def install_firmware(self, image_path):
        if MPFAManager.MPFA_EXTENSION in image_path:
            with MPFAManager(image_path) as mpfa:
                if not mpfa.get_metadata().has_option('firmware', 'burn'):
                    raise RuntimeError("Failed to get {} burn firmware".format(self.name))

                burn_firmware = mpfa.get_metadata().get('firmware', 'burn')

                print("INFO: Processing {} burn file: firmware install".format(self.name))
                return self._install_firmware(os.path.join(mpfa.get_path(), burn_firmware))
        else:
            return self._install_firmware(image_path)

    def update_firmware(self, image_path):
        with MPFAManager(image_path) as mpfa:
            if not mpfa.get_metadata().has_option('firmware', 'burn'):
                raise RuntimeError("Failed to get {} burn firmware".format(self.name))
            if not mpfa.get_metadata().has_option('firmware', 'refresh'):
                raise RuntimeError("Failed to get {} refresh firmware".format(self.name))

            burn_firmware = mpfa.get_metadata().get('firmware', 'burn')
            refresh_firmware = mpfa.get_metadata().get('firmware', 'refresh')

            print("INFO: Processing {} burn file: firmware install".format(self.name))
            if not self._install_firmware(os.path.join(mpfa.get_path(), burn_firmware)):
                return

            print("INFO: Processing {} refresh file: firmware update".format(self.name))
            self._install_firmware(os.path.join(mpfa.get_path(), refresh_firmware))

    @classmethod
    def get_component_list(cls):
        component_list = [ ]

        cpld_number = cls._read_generic_file(cls.CPLD_NUMBER_FILE, cls.CPLD_NUMBER_MAX_LENGTH)
        cpld_number = cpld_number.rstrip('\n')

        for cpld_idx in range(1, int(cpld_number) + 1):
            component_list.append(cls(cpld_idx))

        return component_list


class ComponentCPLDSN2201(ComponentCPLD):
    CPLD_FIRMWARE_UPDATE_COMMAND = ['cpldupdate', '--gpio', '', '--uncustomized', '--print-progress']

    def _install_firmware(self, image_path):
        self.CPLD_FIRMWARE_UPDATE_COMMAND[2] = image_path

        try:
            print("INFO: Installing {} firmware update: path={}".format(self.name, image_path))
            subprocess.check_call(self.CPLD_FIRMWARE_UPDATE_COMMAND, universal_newlines=True)
        except subprocess.CalledProcessError as e:
            print("ERROR: Failed to update {} firmware: {}".format(self.name, str(e)))
            return False

        return True
