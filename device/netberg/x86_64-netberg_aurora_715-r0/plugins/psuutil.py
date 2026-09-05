#
# psuutil.py
# Platform-specific PSU status interface for SONiC
#


import os.path

try:
    from sonic_psu.psu_base import PsuBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

ATTR_PATH = '/sys/class/hwmon/hwmon2/device/NBA715_POWER/'
MAX_PSUS = 2


class PsuUtil(PsuBase):
    """Platform-specific PSUutil class"""

    def __init__(self):
        PsuBase.__init__(self)

    # Get sysfs attribute

    def get_attr_value(self, path):

        retval = 'ERR'
        if not os.path.isfile(path):
            return retval

        try:
            with open(path, 'r') as file_d:
                retval = file_d.read()
        except IOError as error:
            print("Unable to open ", path, " file !", str(error))

        retval = retval.rstrip('\r\n')
        return retval

    def get_num_psus(self):
        """
        Retrieves the number of PSUs available on the device
        :return: An integer, the number of PSUs available on the device
         """
        return MAX_PSUS

    def get_psu_status(self, index):
        """
        Retrieves the oprational status of power supply unit (PSU) defined
                by index <index>
        :param index: An integer, index of the PSU of which to query status
        :return: Boolean, True if PSU is operating properly, False if PSU is\
        faulty
        """
        status = 0
        attr_file = 'psu{}_good'.format(index)
        status_path = ATTR_PATH + attr_file
        try:
            reg_file = open(status_path, 'r')
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False
        text = reg_file.read()

        if int(text) == 1:
            status = 1

        reg_file.close()

        return status

    def get_psu_presence(self, index):
        """
        Retrieves the presence status of power supply unit (PSU) defined
                by index <index>
        :param index: An integer, index of the PSU of which to query status
        :return: Boolean, True if PSU is plugged, False if not
        """
        status = 0
        attr_file = 'psu{}_prnt'.format(index)
        presence_path = ATTR_PATH + attr_file
        try:
            reg_file = open(presence_path, 'r')
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False
        text = reg_file.read()

        if int(text) == 1:
            status = 1

        reg_file.close()

        return status
