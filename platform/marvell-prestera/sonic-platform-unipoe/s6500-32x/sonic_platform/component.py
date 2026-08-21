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
    from sonic_platform_pddf_base import pddfapi
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


if sys.version_info[0] < 3:
    import commands as cmd
else:
    import subprocess as cmd


class Component(ComponentBase):
    """platform-specific Component class"""

    CHASSIS_COMPONENTS = [
        ["CPLD1", "CPLD 1"],
        ["CPLD2", "CPLD 2"]
    ]

    def __init__(self, component_index):
        self.pddf_obj = pddfapi.PddfApi()
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

    def _get_cpld_version(self):
        # Retrieves the CPLD firmware version
        cpld_version = dict()
        if "CPLD1" in self.name:
            cpld_ver = self.pddf_obj.get_attr_name_output("SYSSTATUS", "cpld1_version")
        if "CPLD2" in self.name:
            cpld_ver = self.pddf_obj.get_attr_name_output("SYSSTATUS", "cpld2_version")
        temp_value = cpld_ver['status'].strip()
        ver = int(temp_value, 16)
        version1 = ver >> 4
        version2 = ver & 0b1111
        version = "V%d.%d" % (version1, version2)
        cpld_version[self.name] = version
        return cpld_version

    def get_firmware_version(self):
        """
        Retrieves the firmware version of the component

        Returns:
            A string containing the firmware version of the component
        """

        if "CPLD" in self.name:
            cpld_version = self._get_cpld_version()
            fw_version = cpld_version.get(self.name)
            return fw_version

    def install_firmware(self, image_path):
        """
        Installs firmware to the component

        Args:
            image_path: A string, path to firmware image

        Returns:
            A boolean, True if install was successful, False if not
        """
        return False

