#!/usr/bin/python
#
# Silverstone platform sensors. This script get the sensor data from BMC 
# using ipmitool and display them in lm-sensor alike format.
#
# The following data is support:
#  1. Temperature sensors
#  2. PSUs
#  3. Fan Drawers

import sys
import logging
import subprocess

IPMI_SDR_CMD = ['/usr/bin/ipmitool', 'sdr', 'elist']
MAX_NUM_FANS = 4
MAX_NUM_PSUS = 2

SENSOR_NAME = 0
SENSOR_VAL = 4

sensor_dict = {}

def ipmi_sensor_dump(cmd):
    ''' Execute ipmitool command return dump output
        exit if any error occur.
    '''
    global sensor_dict
    sensor_dump = ''

    try:
        sensor_dump = subprocess.check_output(IPMI_SDR_CMD, universal_newlines=True)
    except subprocess.CalledProcessError as e:
        logging.error('Error! Failed to execute: {}'.format(cmd))
        sys.exit(1)

    for line in sensor_dump.splitlines():
        sensor_info = line.split('|')
        sensor_dict[sensor_info[SENSOR_NAME].strip()] = sensor_info[SENSOR_VAL].strip()

    return True

def get_reading_by_name(sensor_name, sdr_elist_dump):
    found = ''

    for line in sdr_elist_dump.splitlines():
        line = line.decode()
        if sensor_name in line:
            found = line.strip()
            break

    if not found:
        logging.error('Cannot find sensor name:' + sensor_name)

    else:
        try:
            found = found.split('|')[4]
        except IndexError:
            logging.error('Cannot get sensor data of:' + sensor_name)

    logging.basicConfig(level=logging.DEBUG)
    return found


def read_temperature_sensors():
    sensor_list = [\
        ('Base_Temp_U5',    'Baseboard Right Temp'),\
        ('Base_Temp_U7',    'Baseboard Left Temp'),\
        ('Switch_Temp_U1',  'ASIC External Front Temp'),\
        ('Switch_Temp_U18', 'ASIC External Rear Temp'),\
        ('Switch_Temp_U28', 'Switchboard Right Temp'),\
        ('Switch_Temp_U29', 'Switchboard Left Temp'),\
        ('CPU_Temp',        'CPU Internal Temp'),\
        ('Switch_U33_Temp', 'IR3595 Chip Temp'),\
        ('Switch_U21_Temp', 'IR3584 Chip Temp'),\
    ]

    output = ''
    sensor_format = '{0:{width}}{1}\n'
    # Find max length of sensor calling name
    max_name_width = max(len(sensor[1]) for sensor in sensor_list)

    output += "Temperature Sensors\n"
    output += "Adapter: IPMI adapter\n"
    for sensor in sensor_list:
        output += sensor_format.format('{}:'.format(sensor[1]),\
                                       sensor_dict[sensor[0]],\
                                       width=str(max_name_width+1))
    output += '\n'
    return output

def read_fan_sensors(num_fans):

    sensor_list = [\
        ('Fan{}_Status', 'Fan Drawer {} Status'),\
        ('Fan{}_Front',  'Fan {} front'),\
        ('Fan{}_Rear',   'Fan {} rear'),\
    ]

    output = ''
    sensor_format = '{0:{width}}{1}\n'
    # Find max length of sensor calling name
    max_name_width = max(len(sensor[1]) for sensor in sensor_list)

    output += "Fan Drawers\n"
    output += "Adapter: IPMI adapter\n"
    for fan_num in range(1, num_fans+1):
        for sensor in sensor_list:
            ipmi_sensor_name = sensor[0].format(fan_num)
            display_sensor_name = sensor[1].format(fan_num)
            output += sensor_format.format('{}:'.format(display_sensor_name),\
                                           sensor_dict[ipmi_sensor_name],\
                                           width=str(max_name_width+1))
    output += '\n'
    return output

def read_psu_sensors(num_psus):

    sensor_list = [\
        ('PSU{}_Status', 'PSU {} Status'),\
        ('PSU{}_Fan',    'PSU {} Fan 1'),\
        ('PSU{}_VIn',    'PSU {} Input Voltage'),\
        ('PSU{}_CIn',    'PSU {} Input Current'),\
        ('PSU{}_PIn',    'PSU {} Input Power'),\
        ('PSU{}_Temp1',  'PSU {} Ambient Temp'),\
        ('PSU{}_Temp2',  'PSU {} Hotspot Temp'),\
        ('PSU{}_VOut',   'PSU {} Output Voltage'),\
        ('PSU{}_COut',   'PSU {} Output Current'),\
        ('PSU{}_POut',   'PSU {} Output Power'),\
    ]

    output = ''
    sensor_format = '{0:{width}}{1}\n'
    # Find max length of sensor calling name
    max_name_width = max(len(sensor[1]) for sensor in sensor_list)

    output += "PSU\n"
    output += "Adapter: IPMI adapter\n"
    for psu_num in range(1, num_psus+1):
        for sensor in sensor_list:
            ipmi_sensor_name = sensor[0].format('L' if psu_num == 1 else 'R')
            display_sensor_name = sensor[1].format(psu_num)
            output += sensor_format.format('{}:'.format(display_sensor_name),\
                                           sensor_dict[ipmi_sensor_name],\
                                           width=str(max_name_width+1))
    output += '\n'
    return output

def main():
    output_string = ''

    if ipmi_sensor_dump(IPMI_SDR_CMD):
        output_string += read_temperature_sensors()
        output_string += read_psu_sensors(MAX_NUM_PSUS)
        output_string += read_fan_sensors(MAX_NUM_FANS)

        print(output_string)


if __name__ == '__main__':
    main()
