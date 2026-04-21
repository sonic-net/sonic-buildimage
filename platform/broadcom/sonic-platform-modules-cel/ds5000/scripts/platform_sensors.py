#!/usr/bin/python
#
# DS5000 platform sensors. This script get the sensor data from BMC 
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
MAX_NUM_FANS = 3
MAX_NUM_PSUS = 4

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
    '''
        Search for the match sensor name, return sensor
        reading value and unit, return object epmtry string 
        if search not match.

        The output of sensor dump:
        TEMP_FB_U52      | 00h | ok  |  7.1 | 31 degrees C
        TEMP_FB_U17      | 01h | ok  |  7.1 | 27 degrees C
        TEMP_SW_U52      | 02h | ok  |  7.1 | 30 degrees C
        Fan2_Status      | 07h | ok  | 29.2 | Present
        Fan2_Front       | 0Eh | ok  | 29.2 | 12000 RPM
        Fan2_Rear        | 46h | ok  | 29.2 | 14700 RPM
        PSU2_Status      | 39h | ok  | 10.2 | Presence detected
        PSU2_Fan         | 3Dh | ok  | 10.2 | 16000 RPM
        PSU2_VIn         | 3Ah | ok  | 10.2 | 234.30 Volts
        PSU2_CIn         | 3Bh | ok  | 10.2 | 0.80 Amps
    '''
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
        '12V_ENTRY_LEFT',\
        '12V_ENTRY_RIGHT',\
        'BB_BUSBAR_TEMP',\
        'BB_OUTLET_TEMP',\
        'CPU_TEMP',\
        'DIMM0_TEMP',\
        'DIMM1_TEMP',\
        'TH5_CORE_TEMP',\
        'TH5_REAR_LEFT',\
        'TH5_REAR_RIGHT',\
        'XP0R8V_TEMP',\
        'XP0R9V_0_TEMP',\
        'XP0R9V_1_TEMP',\
        'XP0R75V_0_TEMP',\
        'XP0R75V_1_TEMP',\
        'XP1R2V_0_TEMP',\
        'XP1R2V_1_TEMP',\
        'XP3R3V_E_TEMP',\
        'XP3R3V_W_TEMP'
    ]

    output = ''
    sensor_format = '{0:{width}}{1}\n'
    # Find max length of sensor calling name
    max_name_width = max(len(sensor) for sensor in sensor_list)

    output += "Temperature Sensors\n"
    output += "Adapter: IPMI adapter\n"
    for sensor in sensor_list:
        output += sensor_format.format('{}:'.format(sensor),\
                                       sensor_dict[sensor],\
                                       width=str(max_name_width+1))
    output += '\n'
    return output

def read_fan_sensors(num_fans):

    sensor_list = [\
        ('FAN{}_STATUS', 'Fan Drawer {} Status'),\
        ('Fan{}_Front_Speed',  'Fan {} Front Speed'),\
        ('Fan{}_Rear_Speed',   'Fan {} Rear Speed'),\
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
        ('PSU{}_STATUS', 'PSU {} Status'),\
        ('PSU{}_FanSpeed',    'PSU {} Fan 1 Speed'),\
        ('PSU{}_VoltIn',    'PSU {} Input Voltage'),\
        ('PSU{}_CurrIn',    'PSU {} Input Current'),\
        ('PSU{}_PowerIn',    'PSU {} Input Power'),\
        ('PSU{}_TEMP1',  'PSU {} Temp1'),\
        ('PSU{}_TEMP2',  'PSU {} SR'),\
        ('PSU{}_TEMP3',  'PSU {} PFC'),\
        ('PSU{}_VoltOut',   'PSU {} Output Voltage'),\
        ('PSU{}_CurrOut',   'PSU {} Output Current'),\
        ('PSU{}_PowerOut',   'PSU {} Output Power'),\
    ]

    output = ''
    sensor_format = '{0:{width}}{1}\n'
    # Find max length of sensor calling name
    max_name_width = max(len(sensor[1]) for sensor in sensor_list)

    output += "PSU\n"
    output += "Adapter: IPMI adapter\n"
    for psu_num in range(1, num_psus+1):
        for sensor in sensor_list:
            ipmi_sensor_name = sensor[0].format(psu_num)
            display_sensor_name = sensor[1].format(psu_num)
            value = sensor_dict[ipmi_sensor_name]
            if len(value) <= 0 and sensor[0] == "PSU{}_STATUS":
                value = "Presence not detected"
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
