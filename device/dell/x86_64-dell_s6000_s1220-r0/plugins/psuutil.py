#
# psuutil.py
# Platform-specific PSU status interface for SONiC
#


import os.path

try:
    from sonic_psu.psu_base import PsuBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class PsuUtil(PsuBase):
    """Platform-specific PSUutil class"""

    def __init__(self):
        PsuBase.__init__(self)

    def get_cpld_register(self, reg_name):
        cpld_dir = "/sys/devices/platform/dell-s6000-cpld.0"
        retval = 'ERR'
        reg_file = cpld_dir + '/' + reg_name
        if (not os.path.isfile(reg_file)):
            return retval

        try:
            with open(reg_file, 'r') as fd:
                retval = fd.read()
        except Exception as error:
            logging.error("Unable to open ", reg_file, "file !")

        retval = retval.rstrip('\r\n')
        return retval

    def get_num_psus(self):
        """
        Retrieves the number of PSUs available on the device
        :return: An integer, the number of PSUs available on the device
         """
        S6000_MAX_PSUS = 2
        return S6000_MAX_PSUS

    def get_psu_status(self, index):
        """
        Retrieves the oprational status of power supply unit (PSU) defined
                by index <index>
        :param index: An integer, index of the PSU of which to query status
        :return: Boolean, True if PSU is operating properly, False if PSU is\
        faulty
        """
        status = 0
        psu_status = self.get_cpld_register('psu'+str(index - 1)+'_status')
        if (psu_status != 'ERR'):
            status = int(psu_status, 10)

        presence = self.get_psu_presence(index)

        return (status & presence)

    def get_psu_presence(self, index):
        """
        Retrieves the presence status of power supply unit (PSU) defined
                by index <index>
        :param index: An integer, index of the PSU of which to query status
        :return: Boolean, True if PSU is plugged, False if not
        """
        status = 0
        psu_presence = self.get_cpld_register('psu'+str(index - 1)+'_prs')
        if (psu_presence != 'ERR'):
            status = int(psu_presence, 10)

        return status
