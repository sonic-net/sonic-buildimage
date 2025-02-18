#!/usr/bin/python

import logging, subprocess, sys
from swsscommon import swsscommon
from sonic_platform import platform
from natsort import natsorted

TEMP_INFO_TBL = 'TEMPERATURE_INFO'
PSU_INFO_TBL = 'PSU_INFO'
sensors_file_path = './usr/bin/sensors'
pddf_data = None

db_ctxt = swsscommon.SonicV2Connector(use_unix_socket_path=True)
db_ctxt.connect(db_ctxt.STATE_DB, True)

def read_psu_sensor(chassis):
    output = "PSU:\n"
    value = None
    sensor_format = '{0:{width}}{1} {2}\n'
    pddf_data = chassis.platform_inventory

    sensor_list = [\
        ('PSU {} Status', 'presence', 'detected'),\
        ('PSU {} Fan Speed', '-', 'RPM'),\
        ('PSU {} Input Voltage', 'input_voltage', 'Volts'),\
        ('PSU {} Input Current', 'input_current', 'Amps'),\
        ('PSU {} Input Power', 'input_power', 'Watts'),\
        ('PSU {} Output Voltage', 'voltage', 'Volts'),\
        ('PSU {} Output Current', 'current', 'Amps'),\
        ('PSU {} Output Power', 'power', 'Watts'),\
        ('PSU {} Temperature', 'temp', 'degrees C'),\
    ]
    
    # Find max length of sensor calling name
    max_name_width = max(len(sensor[0]) for sensor in sensor_list)

    for psu_index in range (0, pddf_data['num_psus']):
        psu_num = str(psu_index + 1)
        tbl_key = PSU_INFO_TBL + "|" + "PSU {}".format(psu_num)
        psu_data = db_ctxt.get_all(db_ctxt.STATE_DB, tbl_key)

        if len(psu_data) == 0:
            logging.error("{} table doesn't exist".format(tbl_key))
            continue

        invalid_data = ["N/A", ""]
        psu_data['input_power'] = 0
        input_voltage = psu_data['input_voltage']
        input_current = psu_data['input_current']
        if input_voltage not in invalid_data and input_current not in invalid_data:
            input_power = round(float(input_voltage) * float(input_current), 2)
            psu_data['input_power'] = input_power

        for sensor in sensor_list:
            sensor_name = (sensor[0]).format(psu_num)
            if 'Fan Speed' in sensor_name:
                psu_fan = chassis.get_psu(psu_index).get_fan(0)
                value = psu_fan.get_speed_rpm()
            elif 'Status' in sensor_name:
                value = 'Presence' if psu_data[sensor[1]] == "true" else 'Not Presence'
            else:
                value = 0 if psu_data[sensor[1]] == 'N/A' else psu_data[sensor[1]]
            output += sensor_format.format('{}:'.format(sensor_name),\
                                    value, sensor[2],\
                                    width=str(max_name_width+4))
    print(output)

def read_temperature_sensor(pddf_data):
    output = "Temperature Sensors:\n"
    sensor_format = '{0:{width}}{1} degrees C\n'
    keys = natsorted(db_ctxt.keys(db_ctxt.STATE_DB, TEMP_INFO_TBL+'*'))

    if len(keys) == 0:
        logging.error("TEMPERATURE_INFO table dosn't exist")
        return
        
    # Find max length of sensor calling name
    max_name_width = max(len(key) for key in keys)

    for key in keys:
        temp_info_tbl = db_ctxt.get_all(db_ctxt.STATE_DB, key)
        sensor_name = (key.split('|'))[1]
        if 'temperature' in temp_info_tbl:
            temperature_val = 0 if temp_info_tbl['temperature'] == "N/A" else temp_info_tbl['temperature']
            output += sensor_format.format('{}:'.format(sensor_name),\
                                   temperature_val,\
                                   width=str(max_name_width-12))
    print(output)

def read_fan_sensor(chassis):
    value = None
    sensor_list = ['Fan Drawer {} Status', 'Fan {} Speed'] 
    output = "Fan Drawers:\n"
    sensor_format = '{0:{width}}{1}\n'
    pddf_data = chassis.platform_inventory

    # Find max length of sensor calling name
    max_name_width = max(len(sensor) for sensor in sensor_list)

    for fan_index in range (0, pddf_data['num_fantrays']):
        for sensor_name in sensor_list:
            fan_obj = chassis.get_fan(fan_index)
            if 'Speed' in sensor_name:
                value = str(fan_obj.get_speed_rpm())+' RPM'
            else:
                value = 'Presence' if fan_obj.get_presence() == True else 'Not Presence'
            output += sensor_format.format('{}:'.format(sensor_name.format(str(fan_index+1))),\
                                       value,\
                                       width=str(max_name_width+4))
    print(output)

def read_core_temp():
    output = ""
    coretemp_found = False
    try:
        sensor_dump = subprocess.check_output(sensors_file_path, universal_newlines=True)
        for line in sensor_dump.splitlines():
            if 'coretemp' in line:
                coretemp_found = True
            elif coretemp_found and not line.strip():  # Stop at empty line
                coretemp_found = False
            if coretemp_found:
                output += line + "\n"
        print(output)
    except subprocess.CalledProcessError as e:
        logging.error('Error! Failed to execute: sensors script')

def main():
    try:
        chassis = platform.Platform().get_chassis()
        read_temperature_sensor(chassis.platform_inventory)
        read_psu_sensor(chassis)
        read_fan_sensor(chassis)
        sys.stdout.flush()
        read_core_temp()
    except Exception as err:
        logging.exception(f"Failed to process the sensor data: {err}")

if __name__ == '__main__':
    main()
