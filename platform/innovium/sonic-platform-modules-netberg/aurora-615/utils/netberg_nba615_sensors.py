#!/usr/bin/python

from __future__ import print_function
import os
import sys
import logging

MAX_FAN_NUM = 4
MAX_PSU_NUM = 2
PSU_LIST = ['PSU1', 'PSU2']  # 0x58, 0x59

BMC_SYSFILE_PATH = '/sys/class/hwmon/hwmon2/device/NBA615_SYS/'
FAN_SYSFILE_PATH = '/sys/class/hwmon/hwmon2/device/NBA615_FAN/'
POWER_SYSFILE_PATH = '/sys/class/hwmon/hwmon2/device/NBA615_POWER/'
THERMAL_SYSFILE_PATH = '/sys/class/hwmon/hwmon2/device/NBA615_THERMAL/'

# Get sysfs attribute
def get_attr_value(attr_path):
    retval = 'ERR'
    if not os.path.isfile(attr_path):
        return retval

    try:
        with open(attr_path, 'r') as fd:
            retval = fd.read()
    except Exception as error:
        logging.error("Unable to open ", attr_path, " file !")

    retval = retval.rstrip('\r\n')
    fd.close()
    return retval


def print_attr_value_lines(sys_path):
    retval = 'ERR'
    if not os.path.isfile(sys_path):
        return retval
    try:
        fo = open(sys_path, "r")
    except Exception as error:
        logging.error("Unable to open ", sys_path, " file !")
    for line in fo.readlines():
        line = line.strip()
        print("    %s" % line)
    fo.close()
    return retval


def show_sensor_table():
    headers = ['Sensor', 'Temperature(C)', 'High(C)',
               'Low(C)', 'Critical High(C)', 'Critical Low(C)']
 
    table = list()
    temp = list()

    sensor_table = [
        ['TH0 (Front Right)    Top', 'temp_th0_t', 'temp_th0_t_max',
         'temp_th0_t_min', 'temp_th0_t_crit', 'temp_th0_t_lcrit'],
        ['TH0 (Front Right) Bottom', 'temp_th0_b', 'temp_th0_b_max',
         'temp_th0_b_min', 'temp_th0_b_crit', 'temp_th0_b_lcrit'],
        ['TH0 (Remote)            ', 'temp_th0_r', 'temp_th0_r_max',
         'temp_th0_r_min', 'temp_th0_r_crit', 'temp_th0_r_lcrit'],
        ['TH1 (Front Left)     Top', 'temp_th1_t', 'temp_th1_t_max',
         'temp_th1_t_min', 'temp_th1_t_crit', 'temp_th1_t_lcrit'],
        ['TH1 (Front Left)  Bottom', 'temp_th1_b', 'temp_th1_b_max',
         'temp_th1_b_min', 'temp_th1_b_crit', 'temp_th1_b_lcrit'],
        ['TH2 (Back)           Top', 'temp_th2_t', 'temp_th2_t_max',
         'temp_th2_t_min', 'temp_th2_t_crit', 'temp_th2_t_lcrit'],
        ['TH2 (Back)        Bottom', 'temp_th2_b', 'temp_th2_b_max',
         'temp_th2_b_min', 'temp_th2_b_crit', 'temp_th2_b_lcrit'],
        ['TH3 (ASIC)           Top', 'temp_th3_t', 'temp_th3_t_max',
         'temp_th3_t_min', 'temp_th3_t_crit', 'temp_th3_t_lcrit'],
        ['TH3 (ASIC)        Bottom', 'temp_th3_b', 'temp_th3_b_max',
         'temp_th3_b_min', 'temp_th3_b_crit', 'temp_th3_b_lcrit'],
    ]

    for index in range(len(sensor_table)):
        name = sensor_table[index][0]
        for x in range(0, 5):
            sys_path = THERMAL_SYSFILE_PATH + sensor_table[index][x+1]
            t = get_attr_value(sys_path)
            if t == 'ERR':
                temp.append('N/A')
            else:
                if t.isdigit():
                    t = int(t)/1000.0
                temp.append('{}'.format(t))

        table.append([name, temp[0], temp[1], temp[2], temp[3], temp[4]])
        del temp[:]
    print(headers)
    print(table)
    print('')


def show_fan_table():
    print(*['Fan', 'Speed', 'Presence', 'Status', 'Power'], sep='\t')

    for index in range(1, MAX_FAN_NUM+1):
        name_front = "FAN{}-Front".format(index)
        name_rear = "FAN{}-Rear".format(index)
        speed_front, speed_rear = fan_speed_dual(index)
        present = fan_present(index)
        status = fan_status(index)
        power = fan_power(index)
        print(*[name_front, speed_front, present, status, power], sep='\t')
        print(*[name_rear, speed_front, present, status, power], sep='\t')

    print('')


def fan_status(index):
    sys_path = FAN_SYSFILE_PATH + 'fan{}_stat'.format(index)
    ret = get_attr_value(sys_path)
    if ret == '1':
        return 'OK'
    elif ret == '0':
        return 'NG'
    else:
        return 'N/A'


def fan_present(index):
    sys_path = FAN_SYSFILE_PATH + 'fan{}_present'.format(index)
    ret = get_attr_value(sys_path)
    if ret == '1':
        return 'Present'
    elif ret == '0':
        return 'Not Present'
    else:
        return 'N/A'


def fan_power(index):
    sys_path = FAN_SYSFILE_PATH + 'fan{}_power'.format(index)
    ret = get_attr_value(sys_path)
    if ret == '1':
        return 'OK'
    elif ret == '0':
        return 'NG'
    else:
        return 'N/A'


def fan_speed_dual(index):
    sys_path = FAN_SYSFILE_PATH + 'fan{}_front_rpm'.format(index)
    front_ret = get_attr_value(sys_path)
    if front_ret == 'ERR':
        front_ret = 'N/A'

    sys_path = FAN_SYSFILE_PATH + 'fan{}_rear_rpm'.format(index)
    rear_ret = get_attr_value(sys_path)
    if rear_ret == 'ERR':
        rear_ret = 'N/A'

    return (front_ret, rear_ret)


def is_psu_present(psu_number):
    sys_path = POWER_SYSFILE_PATH + 'psu{}_prnt'.format(psu_number)
    if os.path.exists(sys_path):
        value = get_attr_value(sys_path)
        if value == '1':
            return True
        else:
            return False

    return False


def is_psu_power_up(psu_number):
    sys_path = POWER_SYSFILE_PATH + 'psu{}_good'.format(psu_number)
    if os.path.exists(sys_path):
        value = get_attr_value(sys_path)
        if value == '1':
            return True
        else:
            return False

    return False


def show_psu_table():
    headers = ['PSU', 'Presence', 'Power', 'Fan Speed(RPM)', 'Temperature(C)',
               'Vin(V)', 'Vout(V)', 'Pin(W)', 'Pout(W)', 'Iin(A)', 'Iout(A)', 'Max Iout(A)']
    table = []
    psu_sysfiles_list = []

    for index in range(0, MAX_PSU_NUM):
        psu_sysfiles_list = [
            POWER_SYSFILE_PATH+'psu{}_fan_speed'.format(index+1),
            POWER_SYSFILE_PATH+'psu{}_temp'.format(index+1),
            POWER_SYSFILE_PATH+'psu{}_vin'.format(index+1),
            POWER_SYSFILE_PATH+'psu{}_vout'.format(index+1),
            POWER_SYSFILE_PATH+'psu{}_pin'.format(index+1),
            POWER_SYSFILE_PATH+'psu{}_pout'.format(index+1),
            POWER_SYSFILE_PATH+'psu{}_iin'.format(index+1),
            POWER_SYSFILE_PATH+'psu{}_iout'.format(index+1),
            POWER_SYSFILE_PATH+'psu{}_mfr_iout_max'.format(index+1)
        ]
        status_list = get_psu_status(index+1, psu_sysfiles_list)
        table.append(status_list)

    print(headers)
    print(table)
    print('')


def get_psu_status(index, sysfile_list):
    # result_list: [name, presence, power, fanSpeed(RPM), temperature(C), vin(V), vout(V), pin(W), pout(W), iin(A), iout(A), maxIout(A)]
    name = 'PSU{}'.format(index)
    result_list = [name, 'Not Present', 'N/A', 'N/A', 'N/A',
                   'N/A', 'N/A', 'N/A', 'N/A', 'N/A', 'N/A', 'N/A']
    result_mutipler = [None, None, None, None, 1000.0, 1000.0,
                       1000.0, 1000000.0, 1000000.0, 1000.0, 1000.0, 1000.0]

    if is_psu_present(index):
        result_list[1] = 'Present'
    else:
        return result_list

    if is_psu_power_up(index):
        result_list[2] = 'up'
    else:
        result_list[2] = 'down'

    for x in range(0, 9):
        result_list[x+3] = get_attr_value(sysfile_list[x])

    for x in range(0, 12):
        if result_mutipler[x] != None and result_list[x] != 'ERR':
            result_list[x] = int(result_list[x]) / result_mutipler[x]

    return result_list


def main():
    """
    Usage: %(scriptName)s command object

    command:
        fan_status     : display fans status(present/power good)
    """

    if len(sys.argv) < 2:
        print (main.__doc__)

    for arg in sys.argv[1:]:
        if arg == 'fan_status':
            show_fan_table()
        elif arg == 'sensor_status':
            show_sensor_table()
            show_psu_table()

        else:
            print (main.__doc__)


if __name__ == "__main__":
    main()
