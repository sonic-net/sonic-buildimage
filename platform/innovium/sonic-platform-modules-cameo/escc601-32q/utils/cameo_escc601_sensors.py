#!/usr/bin/python

from __future__ import print_function
from tabulate import tabulate
import os
import sys
import logging
import json

MAX_FAN_NUM = 5
MAX_PSU_NUM = 2
PSU_LIST = ['PSU1','PSU2'] #0x58, 0x59

THERMAL_SENSOR_LIST = ['NCT7511Y(U73)', 'G781(U94)', 'G781(U34)', 'G781(U4)']

PLATFORM_INSTALL_INFO_FILE  = '/etc/sonic/platform_install.json'
BMC_SYSFILE_PATH            = '/sys/class/hwmon/hwmon2/device/ESCC601_SYS/'
FAN_SYSFILE_PATH            = '/sys/class/hwmon/hwmon2/device/ESCC601_FAN/'
POWER_SYSFILE_PATH          = '/sys/class/hwmon/hwmon2/device/ESCC601_POWER/'
THERMAL_SYSFILE_PATH        = '/sys/class/hwmon/hwmon2/device/ESCC601_THERMAL/'

def get_psu_path():
    """
    get psu path when without BMC control
    """
    psu_path = []
    try:
        with open(PLATFORM_INSTALL_INFO_FILE) as fd:
            install_info = json.load(fd)
            for psu_name in PSU_LIST:
                psu = install_info[1][psu_name]
                psu_path.append(psu['path']+'/')
            return psu_path
    except Exception:
        print("Fail to get psu sysfsfile path")
    
    return psu_path

def get_thermal_sensor_path():
    sensor_path = []
    try:
        with open(PLATFORM_INSTALL_INFO_FILE) as fd:
            install_info = json.load(fd)
            for sensor_name in THERMAL_SENSOR_LIST:
                sensor = install_info[1][sensor_name]
                sensor_path.append(sensor['hwmon_path']+'/')
            return sensor_path
    except Exception:
        print("Fail to get sensor sysfsfile path")
        
    return sensor_path

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

def bmc_is_exist():
    value = ''
    bmc_filePath = BMC_SYSFILE_PATH+'bmc_present'
    if os.path.exists(bmc_filePath):
       value = get_attr_value(bmc_filePath)
       if int(value) == 1:
            return True
       else:
            return False
    else:
       return False

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
        print ("    %s" % line)
    fo.close()
    return retval

def show_sensor_table():

    headers = ['Sensor', 'Temperature', 'High', 'Low', 'Critical High', 'Critical Low']
    table = list()
    temp = list()
    
    if bmc_is_exist():
        sensor_table = [
            ['Right Bottom Front'   , 'temp_r_b_f', 'temp_r_b_f_max', 'temp_r_b_f_min', 'temp_r_b_f_crit', 'temp_r_b_f_lcrit'],
            ['Right Bottom Back'    , 'temp_r_b_b', 'temp_r_b_b_max', 'temp_r_b_b_min', 'temp_r_b_b_crit', 'temp_r_b_b_lcrit'],
            ['Left Bottom Front'    , 'temp_l_b_f', 'temp_l_b_f_max', 'temp_l_b_f_min', 'temp_l_b_f_crit', 'temp_l_b_f_lcrit'],
            ['Left Bottom Back'     , 'temp_l_b_b', 'temp_l_b_b_max', 'temp_l_b_b_min', 'temp_l_b_b_crit', 'temp_l_b_b_lcrit'],
            ['Right Top Front'      , 'temp_r_t_f', 'temp_r_t_f_max', 'temp_r_t_f_min', 'temp_r_t_f_crit', 'temp_r_t_f_lcrit'],
            ['Right Top Back Sensor', 'temp_r_t_b', 'temp_r_t_b_max', 'temp_r_t_b_min', 'temp_r_t_b_crit', 'temp_r_t_b_lcrit'],
            ['Left Top Front Sensor', 'temp_l_t_f', 'temp_l_t_f_max', 'temp_l_t_f_min', 'temp_l_t_f_crit', 'temp_l_t_f_lcrit'],
            ['Left Top Back Sensor' , 'temp_l_t_b', 'temp_l_t_b_max', 'temp_l_t_b_min', 'temp_l_t_b_crit', 'temp_l_t_b_lcrit'],
        ]
    else:
        sensor_path = get_thermal_sensor_path()
        sensor_table = [
            ['Right Bottom Front'   , sensor_path[0]+'temp1_input', sensor_path[0]+'temp1_max', sensor_path[0]+'temp1_min', sensor_path[0]+'temp1_crit', sensor_path[0]+'temp1_lcrit'],
            ['Right Bottom Back'    , sensor_path[0]+'temp2_input', sensor_path[0]+'temp2_max', sensor_path[0]+'temp2_min', sensor_path[0]+'temp2_crit', sensor_path[0]+'temp2_lcrit'],
            ['Left Bottom Front'    , sensor_path[1]+'temp1_input', sensor_path[1]+'temp1_max', sensor_path[1]+'temp1_min', sensor_path[1]+'temp1_crit', sensor_path[1]+'temp1_lcrit'],
            ['Left Bottom Back'     , sensor_path[1]+'temp2_input', sensor_path[1]+'temp2_max', sensor_path[1]+'temp2_min', sensor_path[1]+'temp2_crit', sensor_path[1]+'temp2_lcrit'],
            ['Right Top Front'      , sensor_path[2]+'temp1_input', sensor_path[2]+'temp1_max', sensor_path[2]+'temp1_min', sensor_path[2]+'temp1_crit', sensor_path[2]+'temp1_lcrit'],
            ['Right Top Back Sensor', sensor_path[2]+'temp2_input', sensor_path[2]+'temp2_max', sensor_path[2]+'temp2_min', sensor_path[2]+'temp2_crit', sensor_path[2]+'temp2_lcrit'],
            ['Left Top Front Sensor', sensor_path[3]+'temp1_input', sensor_path[3]+'temp1_max', sensor_path[3]+'temp1_min', sensor_path[3]+'temp1_crit', sensor_path[3]+'temp1_lcrit'],
            ['Left Top Back Sensor' , sensor_path[3]+'temp2_input', sensor_path[3]+'temp2_max', sensor_path[3]+'temp2_min', sensor_path[3]+'temp2_crit', sensor_path[3]+'temp2_lcrit'],
        ]

    for index in range(len(sensor_table)):
        name = sensor_table[index][0]
        for x in range(0, 5):
            if bmc_is_exist():
                sys_path = THERMAL_SYSFILE_PATH + sensor_table[index][x+1]
            else:
                sys_path = sensor_table[index][x+1]
            t = get_attr_value(sys_path)
            if t == 'ERR':
                temp.append('N/A')
            else:
                if t.isdigit():
                    t = int(t)/1000.0
                temp.append('{} C'.format(t))

        table.append([name, temp[0], temp[1], temp[2], temp[3], temp[4]])
        del temp[:]
    
    print(tabulate(table, headers, tablefmt='simple', stralign='right'))
    print('')

def show_fan_table():
    headers = ['Fan', 'Speed', 'Presence', 'Status', 'Power']
    table = []
    for index in range(1, MAX_FAN_NUM+1):
        name_front = "FAN{}-Front".format(index)
        name_rear = "FAN{}-Rear".format(index)
        speed_front, speed_rear = fan_speed_dual(index)
        present = fan_present(index)
        status = fan_status(index)
        power = fan_power(index)
        table.append( [name_front, speed_front, present, status, power] )
        table.append( [name_rear , speed_front, present, status, power] )
    
    print(tabulate(table, headers, tablefmt='simple', stralign='right'))
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
        return 'On'
    elif ret == '0':
        return 'Off'
    else:
        return 'N/A'

def fan_speed_dual(index):
    sys_path = FAN_SYSFILE_PATH + 'fan{}_front_rpm'.format(index)
    front_ret = get_attr_value(sys_path)
    if front_ret == 'ERR':
        front_ret = 'N/A'
    else:
        front_ret = front_ret+'RPM'
    
    sys_path = FAN_SYSFILE_PATH + 'fan{}_rear_rpm'.format(index)
    rear_ret = get_attr_value(sys_path)
    if rear_ret == 'ERR':
        rear_ret = 'N/A'
    else:
        rear_ret = rear_ret+'RPM'
    
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
    headers = ['PSU', 'Presence', 'Power', 'Fan Speed(RPM)', 'Temperature(C)', 'Vin(V)', 'Vout(V)', 'Pin(W)', 'Pout(W)', 'Iin(A)', 'Iout(A)', 'Max Iout(A)']
    table = []
    psu_sysfiles_list = []
    isbmc = bmc_is_exist()
    if isbmc is False:
        PSU_PATH = get_psu_path()
    
    for index in range(0, MAX_PSU_NUM):
        if isbmc:
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
        else:
            psu_sysfiles_list = [
                PSU_PATH[index]+'psu_fan_speed_1', 
                PSU_PATH[index]+'psu_temp_1',
                PSU_PATH[index]+'psu_vin',
                PSU_PATH[index]+'psu_vout',
                PSU_PATH[index]+'psu_pin',
                PSU_PATH[index]+'psu_pout',
                PSU_PATH[index]+'psu_iin',
                PSU_PATH[index]+'psu_iout',
                PSU_PATH[index]+'psu_iout_max'
            ]            
        status_list = get_psu_status(index+1, psu_sysfiles_list)
        table.append(status_list)
    
    print(tabulate(table, headers, tablefmt='simple', stralign='right'))
    print('')

def get_psu_status(index, sysfile_list):
    # result_list: [name, presence, power, fanSpeed(RPM), temperature(C), vin(V), vout(V), pin(W), pout(W), iin(A), iout(A), maxIout(A)]
    name = 'PSU{}'.format(index)
    result_list = [name, 'Not Present', 'N/A', 'N/A', 'N/A', 'N/A', 'N/A', 'N/A', 'N/A', 'N/A', 'N/A', 'N/A']
    result_mutipler = [None, None, None, None, 1000.0, 1000.0, 1000.0, 1000000.0, 1000000.0, 1000.0, 1000.0, 1000.0]
    
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

    if len(sys.argv)<2:
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
