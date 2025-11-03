#!/usr/bin/python
# -*- coding: UTF-8 -*-
"""
Device config file
"""
try:
    import re
    import os
    import logging
    import syslog
    import time
    import subprocess
    from vendor_sonic_platform.devcfg import Devcfg
except ImportError as error:
    raise ImportError(str(error) + "- required module not found")


def read_file(path):
    '''
    Read file content
    '''
    try:
        with open(path, "r") as temp_read:
            value = temp_read.read().strip('\n')
    except Exception as error:
        logging.error("unable to read %s file , Error: %s" % (path, str(error)))
        value = 'N/A'
    return value


def get_optic_max_temp():
    if not os.path.exists('/tmp/xcvr_power_on/set'):
        return 'N/A'
    value = 'N/A'
    try:
        with open(Devcfg.SFP_MAX_TEMP, 'r') as temp:
            value = temp.read().strip()
            positive_value = value
            if value[0] == '-':
                positive_value = value[1:]
            if not positive_value.isdigit():
                logging.error("read {}".format(value))
                return 'N/A'
            value = float(value) / 1000
    except Exception as error:
        value = 'N/A'
    return value


def get_cpld_version():
    status, result = UtilsHelper.exec_cmd("cat /sys/switch/debug/cpld/bios_cpld | grep 0x0000: | awk '{print $4}'")
    if status != 0:
        log_error("Read bios_cpld failed")
        return None
    return result


def get_mac_temp_code(path):
    value = 'N/A'
    try:
        with open(path, 'r') as temp:
            value = temp.read().strip()
            PositiveValue = value
            if (value[0] == '-'):
                PositiveValue = value[1:]
            if not PositiveValue.isdigit():
                value = 'N/A'
    except Exception as error:
        value = 'N/A'
    return value

def get_mac_temp_bycpld(latest_mac_temp):

    cpld_version = get_cpld_version()
    if cpld_version is None:
        return latest_mac_temp

    cpld_version = int(cpld_version.strip('\n'), 16) & 0x0f
    if cpld_version >= 4:
        temp_code = get_mac_temp_code(Devcfg.MAC_WIDTH_TEMP_REG_DIR)
    else:
        temp_code = get_mac_temp_code(Devcfg.MAC_INNER_TEMP_REG_DIR)

    if temp_code == 'N/A':
        return latest_mac_temp

    if cpld_version >= 4:
        data = (float(temp_code) / 2  - 1)
    else:
        data = (1000000000 / float(temp_code) / 80 - 1)

    temp_val = 356.07 - Devcfg.MAC_INNER_TEMP_FORMULA_COEF * data

    return temp_val

def get_mac_temp(latest_mac_temp):
    temp_val = get_mac_temp_bycpld(latest_mac_temp)
    count = 0
    while count < 5:
        if temp_val < -20 or temp_val > 150:
            time.sleep(0.1)
            temp_val = get_mac_temp_bycpld(latest_mac_temp)
        else:
            break
        count = count + 1
    if count == 5 and (temp_val < -20 or temp_val > 150):
        temp_val = latest_mac_temp
    return temp_val

def get_vr_temp():
    value = 'N/A'
    try:
        tvr_path = Devcfg.SENSOR_IN0_TEMPERATURE
        value = read_file(tvr_path)
        value = int(float(value))
    except Exception as error:
        value = 'N/A'
    return value


def get_mac_temp_validata(latest_mac_temp):
    """
    Only for td3, cpld 1 version validation, if >1 support else not support
    """
    temp = get_mac_temp(latest_mac_temp)
    if temp >= Devcfg.MAC_SHUT:
        temp1 = get_mac_temp(latest_mac_temp)
        if temp1 < Devcfg.MAC_SHUT:
            return round(temp1, 1)
        temp2 = get_mac_temp(latest_mac_temp)
        if temp2 < Devcfg.MAC_SHUT:
            return round(temp2, 1)
        temp3 = get_mac_temp(latest_mac_temp)
        if temp3 < Devcfg.MAC_SHUT:
            return round(temp3, 1)
        return round(max(temp, temp1, temp2, temp3), 1)
    return round(temp, 1)


def get_ssd_temp():
    try:
        cmd = 'smartctl -A /dev/sda | grep "Temperature_Celsius"'
        p = os.popen(cmd, "r")
        s = p.read()
        p.close()
        exp = r"(\d+)$"
        match = re.search(exp, s)
        temp = int(match.group(0)) if match is not None else -1
    except Exception as error:
        temp = 'N/A'
    return temp


def find_all_hwmon_paths(name):
    '''
    Find hwmon path by name
    '''
    hw_list = os.listdir(Devcfg.HWMON_DIR)
    hw_list.sort(key=lambda x: int(x[5:]))
    path_list = []

    for node in hw_list:
        hw_name = ''
        hw_dir = Devcfg.HWMON_DIR + ('%s/' % (node))
        try:
            with open(hw_dir + 'name', 'r') as temp_read:
                hw_name = temp_read.read()
        except Exception as error:
            logging.error(str(error))
            return False
        if name in hw_name:
            path_list.append(hw_dir)

    return path_list


def get_spot_temp(sensor_type, sensor_index, spot_index):
    temp = 0

    try:
        if sensor_type == 'max6696':
            max6696_dir = find_all_hwmon_paths("Max6696")
            spot = 'temp_input'
            sysfs_path = max6696_dir[sensor_index]
            temp = read_file(os.path.join(sysfs_path, spot))
            temp = int(float(temp))

        elif sensor_type == 'coretemp':
            core_dir = find_all_hwmon_paths("coretemp")
            spot = 'temp%d_input' % (spot_index + 1)
            sysfs_path = core_dir[sensor_index]
            temp = read_file(os.path.join(sysfs_path, spot))
            temp = int(temp) * Devcfg.TEMP_RATIO

        elif sensor_type == 'i350':
            i350_dir = find_all_hwmon_paths("i350bb")
            spot = 'temp%d_input' % (spot_index + 1)
            sysfs_path = i350_dir[sensor_index]
            temp = read_file(os.path.join(sysfs_path, spot))
            temp = int(temp) * Devcfg.TEMP_RATIO

        elif sensor_type == 'ssd':
            temp = get_ssd_temp()

        elif sensor_type == 'tvr':
            temp = get_vr_temp()

        elif sensor_type == 'mac':
            temp = get_mac_temp_validata()

        elif sensor_type == 'sfp':
            temp = get_optic_max_temp()
    except BaseException as err:
        syslog.syslog(syslog.LOG_ERR, str(err))
        temp = 'N/A'
        return temp
    return temp


class UtilsHelper(object):
    """
    Device config helper
    """

    def __init__(self):
        '''
        if filepath:
            configpath = filepath
        else:
            root_dir = os.path.dirname(os.path.abspath('.'))
            configpath = os.path.join(root_dir, "conf\\config.ini")
        self.cf = configparser.ConfigParser()
        self.cf.read(configpath)
        '''

    @staticmethod
    def read_data_from_file(path_name, file_name):
        """
            funcation: Get data in file
            reutrn value:  data
            data in file
        """
        file_path = path_name + file_name
        data = None

        try:
            with open(file_path, 'r') as fd:
                data = fd.read()
        except Exception as err:
            # value = "io error"
            logging.error(str(err))

        return data

    @staticmethod
    def exec_cmd(args):
        isString = True if isinstance(args, str) else False
        try:
            proc = subprocess.Popen(args, shell=isString, stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT, universal_newlines=True)
        except BaseException as err:
            logging.error(str(err))
            return -1, None

        try:
            out, _ = proc.communicate()
        except BaseException as err:
            proc.kill()
            logging.error(str(err))
            return -1, None
        status = proc.poll()
        return status, out

    @staticmethod
    def get_i2c_adapt_start():
        status, output = UtilsHelper.exec_cmd("cat /sys/class/i2c-adapter/i2c-*/name | grep -v dom | wc -l")
        if status != 0:
            logging.error("Initialize i2c-adapter failed!")
            return 3
        return int(output)
