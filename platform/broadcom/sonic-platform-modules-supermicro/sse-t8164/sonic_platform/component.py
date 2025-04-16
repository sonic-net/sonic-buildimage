########################################################################
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Components' (e.g., BIOS, CPLD, FPGA, etc.) available in
# the platform
#
########################################################################

try:
    import subprocess
    from sonic_platform_base.component_base import ComponentBase
    from sonic_py_common.general import getstatusoutput_noshell
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")



class Component(ComponentBase):
    """platform-specific Component class"""

    CHASSIS_COMPONENTS = [
        ["BIOS", "BIOS - Basic Input/Output System"],
        ["BMC", "Baseboard Management Controller"],
        ["CPU board CPLD", "CPU board CPLD"],
        ["Switch board CPLD1", "Switch board CPLD1"],
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
            cmd = ['/usr/sbin/dmidecode', '-s', 'bios-version']
            cmdstatus, bios_version = getstatusoutput_noshell(cmd)
            return bios_version

        if self.index == 1:
            cmd = ['ipmitool', 'raw', '0x6', '0x1']
            cmdstatus, output = getstatusoutput_noshell(cmd)
            if cmdstatus == 0:
                fields = output.split()
                version = f"{int(fields[2], 16):02d}.{int(fields[3], 16):02d}.{fields[11]}"
            else:
                version = "N/A"

            return version

        if self.index == 2:
            cmd = ['ipmitool', 'raw', '0x30', '0x89', '0xdb', '0x00', '0x04', '0x02']
            cmdstatus, cpub_cpld_version = getstatusoutput_noshell(cmd)
            return cpub_cpld_version

        if self.index == 3:
            cmd = ['cat', '/sys/class/CPLD1/cpld_device/cpld_version']
            cmdstatus, cpld1_version = getstatusoutput_noshell(cmd)
            return cpld1_version

    def install_firmware(self, image_path):
        """
        Installs firmware to the component

        Args:
            image_path: A string, path to firmware image

        Returns:
            A boolean, True if install was successful, False if not
        """
        return False

