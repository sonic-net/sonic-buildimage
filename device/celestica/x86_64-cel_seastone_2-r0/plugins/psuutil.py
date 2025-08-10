try:
    import sys
    import subprocess
    from sonic_psu.psu_base import PsuBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


PSU_SENSOR_STAT_CMD = 'ipmitool raw 0x3a 0x0c 0x00 0x2 0x60'


class PsuUtil(PsuBase):
    """Platform-specific PSUutil class"""

    sensor_info_bit = {
        1: {
            'presence': 4,
            'status': 2
        },
        2: {
            'presence': 5,
            'status': 3
        }
    }

    def __init__(self):
        PsuBase.__init__(self)

    def run_command(self, command):
        proc = subprocess.Popen(
            command,
            shell=True,
            universal_newlines=True,
            stdout=subprocess.PIPE)
        (out, err) = proc.communicate()

        if proc.returncode != 0:
            sys.exit(proc.returncode)

        return out

    def get_num_psus(self):
        """
        Retrieves the number of PSUs available on the device
        :return: An integer, the number of PSUs available on the device
        """
        return len(self.sensor_info_bit)

    def get_psu_status(self, index):
        """
        Retrieves the oprational status of power supply unit (PSU)
         definedby 1-based index <index>
        :param index: An integer,
         1-based index of the PSU of which to query status
        :return: Boolean,
         True if PSU is operating properly, False if PSU is faulty
        """
        out = self.run_command(PSU_SENSOR_STAT_CMD)
        return True if (
            int(out, 16) >> self.sensor_info_bit[index]['status'] & 1
        ) == 1 else False

    def get_psu_presence(self, index):
        """
        Retrieves the presence status of power supply unit (PSU) defined
         by 1-based index <index>
        :param index: An integer,
         1-based index of the PSU of which to query status
        :return: Boolean, True if PSU is plugged, False if not
        """

        out = self.run_command(PSU_SENSOR_STAT_CMD)
        return True if (
            int(out, 16) >> self.sensor_info_bit[index]['presence'] & 1
        ) == 0 else False
