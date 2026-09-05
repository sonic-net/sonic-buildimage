#
# psuutil.py
# Platform-specific PSU status interface for SONiC
#


import os.path
import subprocess
import logging
from sonic_py_common.general import check_output_pipe

try:
    from sonic_psu.psu_base import PsuBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

DEBUG = False


def show_log(txt):
    if DEBUG == True:
        print("[IX2]"+txt)
    return


def exec_cmd(cmd_args, out_file, show):
    cmd = ' '.join(cmd_args) + ' > ' + out_file
    logging.info('Run :'+cmd)
    try:
        with open(out_file, 'w') as f:
            output = subprocess.check_output(cmd_args, stdout=f, universal_newlines=True)
            show_log(cmd + "output:"+str(output))
    except subprocess.CalledProcessError as e:
        logging.info("Failed :"+cmd)
        if show:
            print("Failed :"+cmd + "returncode = {}, err msg: {}".format(e.returncode, e.output))
    return output


def my_log(txt):
    if DEBUG == True:
        print("[QUANTA DBG]: "+txt)
    return


def log_os_system(cmd1_args, cmd2_args, show):
    cmd = ' '.join(cmd1_args) + ' | ' + ' '.join(cmd2_args)
    logging.info('Run :'+cmd)
    status = 1
    output = ""
    try:
        output = check_output_pipe(cmd1_args, cmd2_args)
        my_log(cmd + "output:"+str(output))
    except subprocess.CalledProcessError as e:
        logging.info('Failed :'+cmd)
        if show:
            print("Failed :"+cmd + "returncode = {}, err msg: {}".format(e.returncode, e.output))
    return output


def gpio16_exist():
    ls = log_os_system(["ls", "/sys/class/gpio/"], ["grep", "gpio16"], 0)
    logging.info('mods:'+ls)
    if len(ls) == 0:
        return False


def gpio17_exist():
    ls = log_os_system(["ls", "/sys/class/gpio/"], ["grep", "gpio17"], 0)
    logging.info('mods:'+ls)
    if len(ls) == 0:
        return False


def gpio19_exist():
    ls = log_os_system(["ls", "/sys/class/gpio/"], ["grep", "gpio19"], 0)
    logging.info('mods:'+ls)
    if len(ls) == 0:
        return False


def gpio20_exist():
    ls = log_os_system(["ls", "/sys/class/gpio/"], ["grep", "gpio20"], 0)
    logging.info('mods:'+ls)
    if len(ls) == 0:
        return False


class PsuUtil(PsuBase):
    """Platform-specific PSUutil class"""

    SYSFS_PSU_PRESENT_DIR = ["/sys/class/gpio/gpio16",
                             "/sys/class/gpio/gpio19"]

    SYSFS_PSU_POWERGOOD_DIR = ["/sys/class/gpio/gpio17",
                               "/sys/class/gpio/gpio20"]

    def __init__(self):
        PsuBase.__init__(self)

        if gpio16_exist() == False:
            output = exec_cmd(["echo", "16"], "/sys/class/gpio/export", 1)
            output = exec_cmd(["echo", "in"], "/sys/class/gpio/gpio16/direction", 1)

        if gpio17_exist() == False:
            output = exec_cmd(["echo", "17"], "/sys/class/gpio/export", 1)
            output = exec_cmd(["echo", "in"], "/sys/class/gpio/gpio17/direction", 1)

        if gpio19_exist() == False:
            output = exec_cmd(["echo", "19"], "/sys/class/gpio/export", 1)
            output = exec_cmd(["echo", "in"], "/sys/class/gpio/gpio19/direction", 1)

        if gpio20_exist() == False:
            output = exec_cmd(["echo", "20"], "/sys/class/gpio/export", 1)
            output = exec_cmd(["echo", "in"], "/sys/class/gpio/gpio20/direction", 1)

    # Get sysfs attribute
    def get_attr_value(self, attr_path):

        retval = 'ERR'
        if (not os.path.isfile(attr_path)):
            return retval

        try:
            with open(attr_path, 'r') as fd:
                retval = fd.read()
        except Exception as error:
            logging.error("Unable to open ", attr_path, " file !")

        retval = retval.rstrip('\r\n')
        return retval

    def get_num_psus(self):
        """
        Retrieves the number of PSUs available on the device
        :return: An integer, the number of PSUs available on the device
         """
        MAX_PSUS = 2
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
        attr_file = 'value'
        attr_path = self.SYSFS_PSU_POWERGOOD_DIR[index-1] + '/' + attr_file

        attr_value = self.get_attr_value(attr_path)

        if (attr_value != 'ERR'):
            attr_value = int(attr_value, 16)
            # Check for PSU status
            if (attr_value == 1):
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
        psu_absent = 0
        attr_file = 'value'
        attr_path = self.SYSFS_PSU_PRESENT_DIR[index-1] + '/' + attr_file

        attr_value = self.get_attr_value(attr_path)

        if (attr_value != 'ERR'):
            attr_value = int(attr_value, 16)
            # Check for PSU presence
            if (attr_value == 0):
                status = 1

        return status
