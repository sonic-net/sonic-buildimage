#
# psuutil.py
# Platform-specific PSU status interface for SONiC
#

import os
import sys
from sonic_py_common.general import getstatusoutput_noshell

SENSORS_CMD = ["docker", "exec", "-i", "pmon", "/usr/bin/sensors"]
DOCKER_SENSORS_CMD = "/usr/bin/sensors"

try:
    from sonic_psu.psu_base import PsuBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class PsuUtil(PsuBase):
    """Platform-specific PSUutil class"""
    _psu_mapping = {
        1 : '0',
        2 : '1'
    }

    def __init__(self):
        PsuBase.__init__(self)

    def isDockerEnv(self):
        num_docker = open('/proc/self/cgroup', 'r').read().count(":/docker")
        if num_docker > 0:
            return True
        else:
            return False

    def remove_nonnumeric(self, text):
        digits='0123456789.'
        return ''.join(c for c in text if c in digits)

    def get_cpld_register(self, reg_name):
        cpld_dir = "/sys/devices/platform/dell-e3224f-cpld.0/"
        retval = 'ERR'
        reg_file = cpld_dir +'/' + reg_name
        if (not os.path.isfile(reg_file)):
            return retval

        try:
            with open(reg_file, 'r') as fd:
                retval = fd.read()
        except Exception as error:
            print("Unable to open ", reg_file, "file !")

        retval = retval.rstrip('\r\n')
        return retval

    def get_num_psus(self):
        """
        Retrieves the number of PSUs available on the device
        :return: An integer, the number of PSUs available on the device
         """
        E3224F_MAX_PSUS = 2
        return E3224F_MAX_PSUS

    def get_psu_status(self, index):
        """
        Retrieves the oprational status of power supply unit (PSU) defined
                by index <index>
        :param index: An integer, index of the PSU of which to query status
        :return: Boolean, True if PSU is operating properly, False if PSU is\
        faulty
        """
        status = 0
        psu_status = self.get_cpld_register('psu'+self._psu_mapping[index]+'_status')
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
        psu_presence = self.get_cpld_register('psu'+self._psu_mapping[index]+'_prs')
        if (psu_presence != 'ERR'):
            status = int(psu_presence, 10)

        return status

    def get_sensor(self):
        dockerenv = self.isDockerEnv()
        if not dockerenv:
            status, cmd_output = getstatusoutput_noshell(SENSORS_CMD)
        else:
            status, cmd_output = getstatusoutput_noshell(DOCKER_SENSORS_CMD)
        if status:
            print('Failed to execute sensors command')
            sys.exit(0)
        return cmd_output

    def get_output_current(self, index):
        cmd_output= self.get_sensor()
        sensor_name = 'dps460-i2c-10' if index == 1 else 'dps460-i2c-11'
        found = False
        for line in cmd_output.splitlines():
            if line.startswith(sensor_name):
                found = True
            if found:
                if 'Output Current' in line :
                    return float(self.remove_nonnumeric(line.split()[2]))
        return 0.0

    def get_output_voltage(self, index):
        cmd_output= self.get_sensor()
        sensor_name = 'dps460-i2c-10' if index == 1 else 'dps460-i2c-11'
        found = False
        for line in cmd_output.splitlines():
            if line.startswith(sensor_name):
                found = True
            if found:
                if 'Output Voltage' in line :
                    return float(self.remove_nonnumeric(line.split()[2]))
        return 0.0

    def get_fan_rpm(self, index, fan_index):
        if fan_index > 1 : return 0.0
        cmd_output= self.get_sensor()
        sensor_name = 'dps460-i2c-10' if index == 1 else 'dps460-i2c-11'
        found = False
        for line in cmd_output.splitlines():
            if line.startswith(sensor_name):
                found = True
            if found:
                if 'Fan RPM' in line :
                    return self.remove_nonnumeric(line.split()[2])
        return 0.0

    def get_output_power(self, index):
        cmd_output= self.get_sensor()
        sensor_name = 'dps460-i2c-10' if index == 1 else 'dps460-i2c-11'
        found = False
        for line in cmd_output.splitlines():
            if line.startswith(sensor_name):
                found = True
            if found:
                if 'Output Power' in line :
                    return float(self.remove_nonnumeric(line.split()[2]))
        return 0.0

    def get_direction(self, index):
        psuid = '0' if index == 1 else '1'
        sysfs_path = '/sys/devices/platform/dell-e3224f-cpld.0/psu' + psuid + '_prs'
        found_psu = int(open(sysfs_path).read())
        if not found_psu : return ''
        bus_no = '10' if index == 1 else '11'
        sysfs_path = "/sys/bus/i2c/devices/" + bus_no + "-0056/eeprom"
        val = (open(sysfs_path, "rb").read())[0xe1:0xe8]
        dir = 'F2B' if 'FORWARD' == val else 'B2F'
        return dir

    def get_serial(self, index):
        psuid = '0' if index == 1 else '1'
        sysfs_path = '/sys/devices/platform/dell-e3224f-cpld.0/psu' + psuid + '_prs'
        found_psu = int(open(sysfs_path).read())
        if not found_psu : return ''
        bus_no = '10' if index == 1 else '11'
        sysfs_path = "/sys/bus/i2c/devices/" + bus_no + "-0056/eeprom"
        val = (open(sysfs_path, "rb").read())[0xc4:0xd9]
        return val

    def get_model(self, index):
        psuid = '0' if index == 1 else '1'
        sysfs_path = '/sys/devices/platform/dell-e3224f-cpld.0/psu' + psuid + '_prs'
        found_psu = int(open(sysfs_path).read())
        if not found_psu : return ''
        bus_no = '10' if index == 1 else '11'
        sysfs_path = "/sys/bus/i2c/devices/" + bus_no + "-0056/eeprom"
        val = (open(sysfs_path, "rb").read())[0x50:0x62]
        return val

    def get_mfr_id(self, index):
        psuid = '0' if index == 1 else '1'
        sysfs_path = '/sys/devices/platform/dell-e3224f-cpld.0/psu' + psuid + '_prs'
        found_psu = int(open(sysfs_path).read())
        return 'DELTA' if  found_psu else  ''
