########################################################################
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Components' (e.g., BIOS, CPLD, FPGA, etc.) available in
# the platform
#
########################################################################

try:
    import sys
    import subprocess
    from sonic_platform_base.component_base import ComponentBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


if sys.version_info[0] < 3:
    import commands as cmd
else:
    import subprocess as cmd


class Component(ComponentBase):
    """platform-specific Component class"""

    CHASSIS_COMPONENTS = [
        ["U-Boot", "Performs initialization during booting"],
        ["ONIE-VERSION", "ONIE - Open Network Install Environment"],
        ["System-CPLD", "System - CPLD"],
    ]

    def __init__(self, component_index):
        self.index = component_index
        self.name = self.CHASSIS_COMPONENTS[self.index][0]
        self.description = self.CHASSIS_COMPONENTS[self.index][1]

    def _get_command_result(self, cmdline):
        try:
            proc = subprocess.Popen(cmdline.split(), stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT)
            stdout = proc.communicate()[0]
            proc.wait()
            result = stdout.rstrip('\n')
        except OSError:
            result = None

        return result


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
            bool: True if component is operating properly, False if not
        """
        return True

    def get_status(self):
        """
        Retrieves the presence of the component

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
        Indicate whether Chassis is replaceable.
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

        if self.index == 0:
            cmdstatus, uboot_version = cmd.getstatusoutput('grep --null-data ^U-Boot /dev/mtd0ro | cut -d" " -f2')
            return uboot_version

        if self.index == 1:
            cmdstatus, onie_version = cmd.getstatusoutput('grep ^onie_version /host/machine.conf | cut -f2 -d"="')
            return onie_version

        if self.index == 2:
            cmdstatus, cpld_version = cmd.getstatusoutput('cat /sys/bus/i2c/devices/0-0066/cpld_version')
            return cpld_version

    def install_firmware(self, image_path):
        """
        Installs firmware to the component

        Args:
            image_path: A string, path to firmware image

        Returns:
            A boolean, True if install was successful, False if not
        """
        return False


    def get_available_firmware_version(self, image_path):
        """
	Fix for issue: 160170:(SONIC-MGMT) platform_tests post /platform/chassis/component/
	returns empty reply.

        Retrieves the available firmware version of the component
	Note: the firmware version will be read from image.
        Args:
            image_path: A string, path to firmware image
        Returns:
            A string containing the available firmware version of the component
        """
        return "N/A"

