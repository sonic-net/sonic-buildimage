try:
    from sonic_platform_pddf_base.pddf_psu import PddfPsu
    from .helper import APIHelper
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

LPC_GETREG_PATH = "/sys/bus/platform/devices/baseboard/getreg"
LPC_PSU_STATUS_REG = '0xa160'
LPC_PSU_POWER_STATUS_OFFSET = 0
LPC_PSU_PRES_STATUS_OFFSET = 2

class Psu(PddfPsu):
    """PDDF Platform-Specific PSU class"""

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        PddfPsu.__init__(self, index, pddf_data, pddf_plugin_data)
        self._api_helper = APIHelper()

    # Provide the functions/variables below for which implementation is to be overwritten
    def get_capacity(self):
        return 550

    def get_type(self):
        return 'AC'

    def get_presence(self):
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        psu_status = int(self._api_helper.lpc_getreg(LPC_GETREG_PATH, LPC_PSU_STATUS_REG), 16)
        presence = psu_status & (1 << (LPC_PSU_PRES_STATUS_OFFSET + 2 - self.psu_index))
        return True if presence == 0 else False

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        psu_status = int(self._api_helper.lpc_getreg(LPC_GETREG_PATH, LPC_PSU_STATUS_REG), 16)
        power_status = psu_status & (1 << (LPC_PSU_POWER_STATUS_OFFSET + 2 - self.psu_index))
        return False if power_status == 0 else True

    def get_revision(self):
        """
        Retrieves the revision of the device
        Returns:
            string: revision of device
        """
        if not self.get_presence():
            return 'N/A'

        if self._api_helper.is_bmc_present():
            cmd = "ipmitool fru list {} | grep 'Product Version'".format(5 - self.psu_index)
            status, output = self._api_helper.get_cmd_output(cmd)
            if status == 0:
                rev = output.split()[-1]
                return rev
        else:
            # Get the revision information from FRU
            cmd1 = "i2cget -y -f {} {} 0x2d b".format(42 + self.psu_index - 1, hex(0x52 + self.psu_index - 1))
            status1, output1 = self._api_helper.get_cmd_output(cmd1)
            cmd2 = "i2cget -y -f {} {} 0x2e b".format(42 + self.psu_index - 1, hex(0x52 + self.psu_index - 1))
            status2, output2 = self._api_helper.get_cmd_output(cmd2)
            if status1 == 0 and status2 == 0:
                rev = bytes.fromhex(output1[2:] + output2[2:]).decode('utf-8')
                return rev
        return 'N/A'
