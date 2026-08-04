#
# psuutil.py
# Platform-specific PSU status interface for SONiC
#


import os.path

try:
    from sonic_psu.psu_base import PsuBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

HWMON_DIR = "/sys/devices/platform/SMF.512/hwmon/"
HWMON_NODE = os.listdir(HWMON_DIR)[0]


class PsuUtil(PsuBase):
    """Platform-specific PSUutil class"""

    def __init__(self):
        PsuBase.__init__(self)

    # Get a mailbox register
    def get_pmc_register(self, reg_name):
        mailbox_dir = HWMON_DIR + HWMON_NODE
        retval = 'ERR'
        mb_reg_file = mailbox_dir+'/' + reg_name
        if (not os.path.isfile(mb_reg_file)):
            logging.error(mb_reg_file + " not found !")
            return retval

        try:
            with open(mb_reg_file, 'r') as fd:
                retval = fd.read()
        except Exception as error:
            logging.error("Unable to open ", mb_reg_file, "file !")

        retval = retval.rstrip('\r\n')
        return retval

    def get_num_psus(self):
        """
        Retrieves the number of PSUs available on the device
        :return: An integer, the number of PSUs available on the device
         """
        Z9100_MAX_PSUS = 2
        return Z9100_MAX_PSUS

    def get_psu_status(self, index):
        """
        Retrieves the oprational status of power supply unit (PSU) defined
                by index <index>
        :param index: An integer, index of the PSU of which to query status
        :return: Boolean, True if PSU is operating properly, False if PSU is\
        faulty
        """
        status = 0
        psu_status = self.get_pmc_register('psu'+str(index)+'_presence')
        if (psu_status != 'ERR'):
            psu_status = int(psu_status, 16)
            # Check for PSU statuse
            if (~psu_status & 0b1000) or (psu_status & 0b0100):
                status = 1

        return status

    def get_psu_presence(self, index):
        """
        Retrieves the presence status of power supply unit (PSU) defined
                by index <index>
        :param index: An integer, index of the PSU of which to query status
        :return: Boolean, True if PSU is plugged, False if not
        """
        status = 0
        psu_presence = self.get_pmc_register('psu'+str(index)+'_presence')
        if (psu_presence != 'ERR'):
            psu_presence = int(psu_presence, 16)
            # Check for PSU presence
            if (~psu_presence & 0b1):
                status = 1

        return status
