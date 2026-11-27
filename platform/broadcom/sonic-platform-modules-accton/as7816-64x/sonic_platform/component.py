#############################################################################
#
# Component contains an implementation of SONiC Platform Base API and
# provides the components firmware management function
#
#############################################################################

try:
    import os
    import json
    from sonic_platform_base.component_base import ComponentBase
    from sonic_py_common.general import getstatusoutput_noshell
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

CPLD_ADDR_MAPPING = {
    "CPLD1": ['19', '0x60'],
    "CPLD2": ['20', '0x62'],
    "CPLD3": ['21', '0x64'],
    "CPLD4": ['22', '0x66'],
    "CPLD5": ['17', '0x68']
}
SYSFS_PATH = "/sys/bus/i2c/devices/"
BIOS_VERSION_PATH = "/sys/class/dmi/id/bios_version"
COMPONENT_LIST= [
   ("CPLD1", "CPLD 1"),
   ("CPLD2", "CPLD 2"),
   ("CPLD3", "CPLD 3"),
   ("CPLD4", "CPLD 4"),
   ("CPLD5", "CPLD FAN"),
   ("CPLD6", "CPLD CPU"),
   ("BIOS", "Basic Input/Output System")
]

class Component(ComponentBase):
    """Platform-specific Component class"""

    DEVICE_TYPE = "component"

    def __init__(self, component_index=0):
        self.index = component_index
        self.name = self.get_name()

    def __get_bios_version(self):
        # Retrieves the BIOS firmware version
        try:
            with open(BIOS_VERSION_PATH, 'r') as fd:
                bios_version = fd.read()
                return bios_version.strip()
        except Exception as e:
            return None

    def __get_cpld_version(self):
        # Retrieves the CPLD firmware version
        cpld_version = dict()
        for cpld_name in CPLD_ADDR_MAPPING:
            cmd = ["i2cget", "-f", "-y", CPLD_ADDR_MAPPING[cpld_name][0], CPLD_ADDR_MAPPING[cpld_name][1], "0x1"]
            status, value = getstatusoutput_noshell(cmd)
            if not status:
                cpld_version_raw = value.rstrip()
                cpld_version[cpld_name] = "{}".format(int(cpld_version_raw,16))

        return cpld_version

    def __get_cpld_cpu_version(self):
        try:
            cpu_version = dict()
            cmd = ["i2cget", "-f", "-y", "0", "0x65", "0x01"]
            status, output1 = getstatusoutput_noshell(cmd)
            if status == 0 :
                cmd = ["i2cget", "-f", "-y", "0", "0x65", "0xff"]
                status, output2 = getstatusoutput_noshell(cmd)
                if status == 0 :
                    cpu_version = "{}{}{}".format(output1[2:],".",output2[2:])
                else :
                    cpu_version = 'None'
            else :
                cpu_version = 'None'
        except Exception as e:
            cpu_version = 'None'

        return cpu_version

    def get_name(self):
        """
        Retrieves the name of the component
         Returns:
            A string containing the name of the component
        """
        return COMPONENT_LIST[self.index][0]

    def get_description(self):
        """
        Retrieves the description of the component
            Returns:
            A string containing the description of the component
        """
        return COMPONENT_LIST[self.index][1]

    def get_firmware_version(self):
        """
        Retrieves the firmware version of module
        Returns:
            string: The firmware versions of the module
        """
        fw_version = None

        if self.name == "BIOS":
            fw_version = self.__get_bios_version()
        elif "CPLD6" in self.name:
            fw_version =self.__get_cpld_cpu_version()
        elif "CPLD" in self.name:
            cpld_version = self.__get_cpld_version()
            fw_version = cpld_version.get(self.name)

        return fw_version

    def install_firmware(self, image_path):
        """
        Install firmware to module
        Args:
            image_path: A string, path to firmware image
        Returns:
            A boolean, True if install successfully, False if not
        """
        ret, output = getstatusoutput_noshell(["tar", "-C", "/tmp", "-xzf", image_path ] )
        if ret != 0 :
            print("Installation failed because of wrong image package")
            return False

        if  False == os.path.exists("/tmp/install.json") :
            print("Installation failed without jsonfile")
            return False

        input_file = open ('/tmp/install.json')
        json_array = json.load(input_file)
        ret = 1
        for item in json_array:
            if item.get('id')==None or item.get('path')==None:
                continue
            if self.name == item['id'] and item['path'] and item.get('cpu'):
                print( "Find", item['id'], item['path'], item['cpu'] )
                ret, output = getstatusoutput_noshell(["/tmp/run_install.sh", item['id'], item['path'], item['cpu'] ])
                if ret==0:
                    break
            elif self.name == item['id'] and item['path']:
                print( "Find", item['id'], item['path'] )
                ret, output = getstatusoutput_noshell(["/tmp/run_install.sh", item['id'], item['path'] ])
                if ret==0:
                    break

        if ret==0:
            return True
        else :
            return False

    def get_presence(self):
        """
        Retrieves the presence of the device
        Returns:
            bool: True if device is present, False if not
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
        return 'N/A'

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
