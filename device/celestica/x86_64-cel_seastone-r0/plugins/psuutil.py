#
# psuutil.py
# Platform-specific PSU status interface for SONiC
#

import os


try:
    from sonic_psu.psu_base import PsuBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


GPIO_DIR = "/sys/class/gpio"
GPIO_LABEL = "pca9505"
DX010_MAX_PSUS = 2


class PsuUtil(PsuBase):
    """Platform-specific PSUutil class"""

    def __init__(self):
        PsuBase.__init__(self)
        # DX010 PSU pin mapping
        self.dx010_psu_gpio = [
            {'base': self.__get_gpio_base()},
            {'prs': 27, 'status': 22},
            {'prs': 28, 'status': 25}
        ]

    def __read_txt_file(self, file_path):
        try:
            with open(file_path, 'r') as fd:
                data = fd.read()
                return data.strip()
        except IOError:
            pass
        return ""

    def __get_gpio_base(self):
        for r in os.listdir(GPIO_DIR):
            label_path = os.path.join(GPIO_DIR, r, "label")
            if "gpiochip" in r and GPIO_LABEL in self.__read_txt_file(label_path):
                return int(r[8:], 10)
        return 216  # Reserve

    def __get_gpio_value(self, pinnum):
        gpio_base = self.dx010_psu_gpio[0]['base']
        gpio_dir = GPIO_DIR + '/gpio' + str(gpio_base+pinnum)
        gpio_file = gpio_dir + "/value"
        retval = self.__read_txt_file(gpio_file)
        return retval.rstrip('\r\n')

    def get_num_psus(self):
        """
        Retrieves the number of PSUs available on the device
        :return: An integer, the number of PSUs available on the device
         """
        return DX010_MAX_PSUS

    def get_psu_status(self, index):
        """
        Retrieves the oprational status of power supply unit (PSU) defined
                by index <index>
        :param index: An integer, index of the PSU of which to query status
        :return: Boolean, True if PSU is operating properly, False if PSU is\
        faulty
        """
        raw = self.__get_gpio_value(
            self.dx010_psu_gpio[index]['status'])
        return int(raw, 10) == 1

    def get_psu_presence(self, index):
        """
        Retrieves the presence status of power supply unit (PSU) defined
                by index <index>
        :param index: An integer, index of the PSU of which to query status
        :return: Boolean, True if PSU is plugged, False if not
        """
        raw = self.__get_gpio_value(self.dx010_psu_gpio[index]['prs'])
        return int(raw, 10) == 0
