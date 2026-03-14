#
# psuutil.py
# Platform-specific PSU status interface for SONiC
#


import logging
import sys
from sonic_py_common.general import getstatusoutput_noshell


Z9264F_MAX_PSUS = 2
IPMI_PSU_DATA = ["docker", "exec", "-it", "pmon", "ipmitool", "sdr", "list"]
IPMI_PSU_DATA_DOCKER = ["ipmitool", "sdr", "list"]
PSU_PRESENCE = "PSU{0}_state"
# Use this for older firmware
# PSU_PRESENCE="PSU{0}_prsnt"
ipmi_sdr_list = ""


try:
    from sonic_psu.psu_base import PsuBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class PsuUtil(PsuBase):
    """Platform-specific PSUutil class"""

    def __init__(self):
        PsuBase.__init__(self)

    def isDockerEnv(self):
        num_docker = open('/proc/self/cgroup', 'r').read().count(":/docker")
        if num_docker > 0:
            return True
        else:
            return False

    # Fetch a BMC register
    def get_pmc_register(self, reg_name):

        status = 1
        global ipmi_sdr_list
        ipmi_dev_node = "/dev/pmi0"
        ipmi_cmd = IPMI_PSU_DATA
        dockerenv = self.isDockerEnv()
        if dockerenv == True:
            ipmi_cmd = IPMI_PSU_DATA_DOCKER

        status, ipmi_sdr_list = getstatusoutput_noshell(ipmi_cmd)

        if status:
            logging.error('Failed to execute:' + ipmi_sdr_list)
            sys.exit(0)

        for item in ipmi_sdr_list.split("\n"):
            if reg_name in item:
                output = item.strip()

        if not output:
            print('\nFailed to fetch: ' + reg_name + ' sensor ')
            sys.exit(0)

        output = output.split('|')[1]

        logging.basicConfig(level=logging.DEBUG)
        return output

    def get_num_psus(self):
        """
        Retrieves the number of PSUs available on the device
        :return: An integer, the number of PSUs available on the device
         """
        Z9264F_MAX_PSUS = 2
        return Z9264F_MAX_PSUS

    def get_psu_status(self, index):
        """
        Retrieves the oprational status of power supply unit (PSU) defined
                by index <index>
        :param index: An integer, index of the PSU of which to query status
        :return: Boolean, True if PSU is operating properly, False if PSU is\
        faulty
        """
        # Until psu_status is implemented this is hardcoded temporarily

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
        psu_reg_name = PSU_PRESENCE.format(index)
        psu_status = int(self.get_pmc_register(psu_reg_name), 16)
        if (psu_status != 'ERR'):
            # Check for PSU presence
            if (psu_status):
                status = 1
        return status
