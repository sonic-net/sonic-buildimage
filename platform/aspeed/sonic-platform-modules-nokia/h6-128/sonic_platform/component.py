########################################################################
# Nokia H6-128 BMC
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Components' (e.g., BIOS, U-Boot, CPLD, FPGA, etc.) available in
# the platform
#
########################################################################

try:
    import sys
    import os
    import time
    import subprocess
    import ntpath
    from sonic_platform_base.component_base import ComponentBase
    from sonic_py_common.general import getstatusoutput_noshell, getstatusoutput_noshell_pipe
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


if sys.version_info[0] < 3:
    import commands as cmd
else:
    import subprocess as cmd


class Component(ComponentBase):
    """Nokia platform-specific Component class"""

    CHASSIS_COMPONENTS = [
        ["U-Boot", "Performs initialization during booting"],
    ]

    def __init__(self, chassis_model, component_index):
        self.index = component_index
        self.name = self.CHASSIS_COMPONENTS[self.index][0]
        self.description = self.CHASSIS_COMPONENTS[self.index][1]
        self.chassis_model = chassis_model

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
    def get_chassis_model(self):
        """
        Retrieves the model number of the Fan

        Returns:
            string: Model number of Fan. Use part number for this.
        """
        return self.chassis_model
    
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
        return True

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
            cmdstatus, uboot_version = cmd.getstatusoutput('cat /proc/device-tree/chosen/u-boot,version')
            return uboot_version or "V1.1"
        
    def get_available_firmware_version(self, image_path):
        """
        Retrieves the available firmware version of the component

        Note: the firmware version will be read from image

        Args:
            image_path: A string, path to firmware image

        Returns:
            A string containing the available firmware version of the component
        """
        if image_path:    
            image_name = ntpath.basename(image_path)
            return image_name

        return 'NA'
    
    def install_firmware(self, image_path):
        """
        Installs firmware to the component

        Args:
            image_path: A string, path to firmware image

        Returns:
            A boolean, True if install was successful, False if not
        """
        image_name = ntpath.basename(image_path)
        print(" Nokia H6-128 BMC - install firmware {}".format(image_name))

        # check whether the image file exists
        if not os.path.isfile(image_path):
            print("ERROR: the firmware image {} doesn't exist ".format(image_path))
            return False
        if self.name == "U-Boot":
            ch_model=self.get_chassis_model()
            if(ch_model[8:10]=='AA'):
                success_flag = False
                UBOOT_UPDATE_COMMAND1 = ['sudo', 'flashcp', '-A', image_path, '/dev/mtd0']
                UBOOT_UPDATE_COMMAND2 = ['sudo', 'reboot']
                try:
                    subprocess.check_call(UBOOT_UPDATE_COMMAND1, stderr=subprocess.STDOUT)
                    subprocess.check_call(UBOOT_UPDATE_COMMAND2, stderr=subprocess.STDOUT)
                    success_flag = True
                except subprocess.CalledProcessError as e:
                    print("ERROR: Failed to upgrade U-BOOT: command={}, rc={}",format(e.cmd),format(e.returncode))

                return success_flag

    def update_firmware(self, image_path):
        """
        Updates firmware of the component

        This API performs firmware update: it assumes firmware installation and loading in a single call.
        In case platform component requires some extra steps (apart from calling Low Level Utility)
        to load the installed firmware (e.g, reboot, power cycle, etc.) - this will be done automatically by API

        Args:
            image_path: A string, path to firmware image

        Returns:
            Boolean False if image_path doesn't exist instead of throwing an exception error
            Nothing when the update is successful

        Raises:
            RuntimeError: update failed
        """
        return self.install_firmware(image_path)
