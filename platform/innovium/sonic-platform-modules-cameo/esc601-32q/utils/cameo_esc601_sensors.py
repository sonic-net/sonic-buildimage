#!/usr/bin/python

from __future__ import print_function
import os
import sys
import logging
import json


FAN_NUM = 5
MAX_PSU_NUM = 2
PSU_LIST = ['PSU1','PSU2'] #0x58, 0x59

PLATFORM_INSTALL_INFO_FILE  = '/etc/sonic/platform_install.json'
BMC_SYSFILE_PATH            = '/sys/class/hwmon/hwmon2/device/ESC601_BMC/'
FAN_SYSFILE_PATH            = '/sys/class/hwmon/hwmon2/device/ESC601_FAN/'
PSU_SYSFILE_PATH            = '/sys/class/hwmon/hwmon2/device/ESC601_PSU/'

def get_mac_sensor_path():
    mac_sensor_path = []
    try:
        with open(PLATFORM_INSTALL_INFO_FILE) as fd:
            install_info = json.load(fd)
            mac_sensor_path = install_info[1]['MCP3425']['path']
    except Exception:
        print("Fail to get mac sensor sysfsfile path")
        
    return mac_sensor_path
            
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
       if value.find('not') < 0:
            return True
       else:
            return False
    else:
       return False

def is_fan_speed_supported():
    value = ''
    filePath = '/sys/class/hwmon/hwmon2/device/ESC601_SYS/cpld4_version'
    if os.path.exists(filePath):
        value = get_attr_value(filePath)
        if value == 'ERR':
            return False
        if int(value,16) >= 0x02:
            return True
    
    return False

def calc_mac_temp():
    value = ''
    try:
        if bmc_is_exist():
            value = get_attr_value(BMC_SYSFILE_PATH+'bmc_mac_sensor')
            mac_sensor= int(value,16)/1000.0
        else:
            path = get_mac_sensor_path()
            raw = get_attr_value(path+'/iio:device0/in_voltage0_raw')
            scale = get_attr_value(path+'/iio:device0/in_voltage0_scale')
            mac_sensor= int(raw,10)*float(scale)
    except Exception:
        return 'N/A' 

    temp1 = -124.28 * mac_sensor * mac_sensor
    temp2 = -422.03 * mac_sensor
    temp_sensor = 384.62 + temp1 + temp2
    return "%.2f" %(round(temp_sensor,2))

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

def get_fan_alert(number):
    return

def get_fan_inner_rpm(number):
    return

def get_fan_outer_rpm(number):
    return

def sensors_status():
    if not bmc_is_exist():
        return
    sys_path = BMC_SYSFILE_PATH + 'bmc_sersor_1'  
    print_attr_value_lines(sys_path)
    sys_path = BMC_SYSFILE_PATH + 'bmc_sersor_2'
    print_attr_value_lines(sys_path)
    sys_path = BMC_SYSFILE_PATH + 'bmc_sersor_3'
    print_attr_value_lines(sys_path)
    sys_path = BMC_SYSFILE_PATH + 'bmc_sersor_4'
    print_attr_value_lines(sys_path)
    return

def mac_sensors_temp():
    print ('MAC SENSORS TEMP: %s degrees (C)\n'% calc_mac_temp())
    return

def get_voltage():
    return

def fan_status():
    sys_path = FAN_SYSFILE_PATH + 'fan_status'
    print ('FAN STATUS:')
    print_attr_value_lines(sys_path)
    return

def fan_present():
    sys_path = FAN_SYSFILE_PATH + 'fan_present'
    print ('FAN PRESENT:')
    print_attr_value_lines(sys_path)
    return

def fan_power():
    sys_path = FAN_SYSFILE_PATH + 'fan_power'
    print ('FAN POWER:')
    print_attr_value_lines(sys_path)
    return

def fan_speed():
    sys_path = FAN_SYSFILE_PATH + 'fan_speed_rpm'
    print ('FAN SPEED:')
    print_attr_value_lines(sys_path)
    return

def is_psu_present(psu_number):
    sys_path = PSU_SYSFILE_PATH + 'psu_present'
    search_str = "PSU {} is present".format(psu_number)
    if os.path.exists(sys_path):
       value = get_attr_value(sys_path)
       if search_str in value:
            return True
       else:
            return False
    
    return False

def show_psu_status(path):
    # [model, vin, vout, fan_speed, temperature, pin, pout, iin, iout, max_iout]
    output_format = [
        ('    model: ', '{}', '\n'),
        ('    Input Voltage:  ', '{:+3.2f} V' , '\n'),
        ('    Output Voltage: ', '{:+3.2f} V' , '\n'),
        ('    Fan Speed:      ', '{:3d} RPM'  , '\n'),
        ('    Temperature     ', '{:+3.1f} C' , '\n'),
        ('    Input Power:    ', '{:3.2f} W'  , '\n'),
        ('    Output Power:   ', '{:3.2f} W'  , '\n'),
        ('    Input Current:  ', '{:+3.2f} A' , '\n'),
        ('    Output Current: ', '{:+3.2f} A' , '  '),
        ('(max = '         , '{:+3.2f} A)', '\n')
    ]

    result_list = [0]*10
    if bmc_is_exist():  
        try:
          reg_file = open(path, 'r')
        except IOError as e:
            print( "Error: unable to open file: %s" % str(e))
            return False 
        
        text_lines = reg_file.readlines()
        reg_file.close()
        
        for line in text_lines:
            spline = line.split(' ')
            if "MFR_MODEL" in spline:
                result_list[0] = spline[-1]
            if "VIN" in spline:
                result_list[1] = spline[-1]
            if "VOUT" in spline:
                result_list[2] = spline[-1]
            if "FAN_SPEED" in spline:
                result_list[3] = spline[-1]
            if "TEMP_1" in spline:
                result_list[4] = spline[-1]
            if "PIN" in spline:
                result_list[5] = spline[-1]
            if "POUT" in spline:
                result_list[6] = spline[-1]
            if "IIN" in spline:
                result_list[7] = spline[-1]
            if "IOUT" in spline:
                result_list[8] = spline[-1]
            if "MFR_IOUT_MAX" in spline:
                result_list[9] = spline[-1]                
                
    else:
        result_list[0] = get_attr_value(path+"psu_mfr_model")
        result_list[1] = get_attr_value(path+"psu_vin")
        result_list[2] = get_attr_value(path+"psu_vout")   
        result_list[3] = get_attr_value(path+"psu_fan_speed_1")
        result_list[4] = get_attr_value(path+"psu_temp_1")
        result_list[5] = get_attr_value(path+"psu_pin")
        result_list[6] = get_attr_value(path+"psu_pout")
        result_list[7] = get_attr_value(path+"psu_iin")
        result_list[8] = get_attr_value(path+"psu_iout")
        result_list[9] = get_attr_value(path+"psu_iout_max")
    
    if result_list[1] != 'ERR':
        result_list[1] = int(result_list[1])/1000.0
        
    if result_list[2] != 'ERR':
        result_list[2] = int(result_list[2])/1000.0
    
    if result_list[3] != 'ERR':
        result_list[3] = int(result_list[3]) 
    
    if result_list[4] != 'ERR':
        result_list[4] = int(result_list[4])/1000.0 
    
    if result_list[5] != 'ERR':
        result_list[5] = int(result_list[5])/1000000.0
    
    if result_list[6] != 'ERR':
        result_list[6] = int(result_list[6])/1000000.0
    
    if result_list[7] != 'ERR':
        result_list[7] = int(result_list[7])/1000.0
    
    if result_list[8] != 'ERR':
        result_list[8] = int(result_list[8])/1000.0
    
    if result_list[9] != 'ERR':
        result_list[9] = int(result_list[9])/1000.0
    
    for i in range(len(output_format)):
        print(output_format[i][0], end='')
        if result_list[i] != 'ERR':
            print(output_format[i][1].format(result_list[i]), end=output_format[i][2])
        else:
            print('error')

    print('')
    return
        

def psu_status():

    for x in range(0,MAX_PSU_NUM):
        if is_psu_present(x+1):
            print("PSU{} present".format(x+1))
            if bmc_is_exist():
                show_psu_status(PSU_SYSFILE_PATH + 'psu_module_{}'.format(x+1))
            else:
                psu_path = get_psu_path()
                show_psu_status(psu_path[x])

    return


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
            fan_status()
            fan_present()
            fan_power()
            if is_fan_speed_supported():
                fan_speed()
        elif arg == 'sensor_status':
            sensors_status()
            mac_sensors_temp()
            psu_status()

        else:
            print (main.__doc__)

if __name__ == "__main__":
    main()
