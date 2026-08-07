#!/usr/bin/env python_nos

psutypedecode = {
    0x00: 'N/A',
    0x01: 'AC',
    0x02: 'DC',
}

psu_fan_airflow = {
    "intake": ['CRPS2700D2','AP-CA3200F12DT'],
    "exhaust": []
}

fanairflow = {
    "intake": ['FAN80-04H-F'],
    "exhaust": [],
}


s3ip_led_decode = {
    "red flash": 0x07, "red": 0x03, "green flash": 0x05, "green": 0x01,
    "amber flash": 0x06, "amber": 0x02, "off": 0, "mask": 0x0ff
}

class Unit:
    Temperature = "C"
    Voltage = "V"
    Current = "A"
    Power = "W"
    Speed = "RPM"


class threshold:
    PSU_TEMP_MIN = -20 * 1000
    PSU_TEMP_MAX = 65 * 1000

    PSU_FAN_SPEED_MIN = 1000
    PSU_FAN_SPEED_MAX = 37000
    PSU_A_FAN_SPEED_MAX = 39000

    PSU_OUTPUT_VOLTAGE_MIN = 11 * 1000
    PSU_OUTPUT_VOLTAGE_MAX = 13 * 1000

    PSU_AC_INPUT_VOLTAGE_MIN = 90 * 1000
    PSU_AC_INPUT_VOLTAGE_MAX = 264 * 1000

    PSU_DC_INPUT_VOLTAGE_MIN = 180 * 1000
    PSU_DC_INPUT_VOLTAGE_MAX = 320 * 1000

    ERR_VALUE = -9999999

    PSU_OUTPUT_POWER_MIN = 0* 1000 * 1000
    PSU_OUTPUT_POWER_MAX = 3000 * 1000 * 1000

    PSU_INPUT_POWER_MIN = 0 * 1000 * 1000
    PSU_INPUT_POWER_MAX = 3100* 1000 * 1000

    PSU_OUTPUT_CURRENT_MIN = 0 * 1000
    PSU_OUTPUT_CURRENT_MAX = 246 * 1000

    PSU_INPUT_CURRENT_MIN = 0 * 1000
    PSU_INPUT_CURRENT_MAX = 20 * 1000

    FRONT_FAN_SPEED_MAX = 18000
    REAR_FAN_SPEED_MAX = 16000
    FAN_SPEED_MIN = 3200


devices = {
    "sensor_print_src": "s3ip",

    "dcdc_data_source": [
        {
            "path": "/sys/s3ip/vol_sensor",
            "type": "vol",
            "Unit": Unit.Voltage,
            "read_times": 3,
        },
        {
            "path": "/sys/s3ip/curr_sensor",
            "type": "curr",
            "Unit": Unit.Current,
            "read_times": 3,
        },
    ],
    "temp_data_source": [
        {
            "path": "/sys/s3ip/temp_sensor",
            "type": "temp",
            "Unit": Unit.Temperature,
        },
    ],

    "onie_e2": [
        {
            "name": "ONIE_E2",
            "e2loc": {"loc": "/sys/bus/i2c/devices/1-0056/eeprom", "way": "sysfs"},
            "airflow": "intake"
        },
    ],
    "psus": [
        {
            "e2loc": {"loc": "/sys/bus/i2c/devices/67-0050/eeprom", "way": "sysfs"},
            "psu_sn": {"loc": "/sys/s3ip/psu/psu1/serial_number", "way": "sysfs"},
            "psu_hw": {"loc": "/sys/s3ip/psu/psu1/hardware_version", "way": "sysfs"},
            "psu_pn": {"loc": "/sys/s3ip/psu/psu1/part_number", "way": "sysfs"},
            "psu_vendor": {"loc": "/sys/s3ip/psu/psu1/vendor", "way": "sysfs"},
            "pmbusloc": {"bus": 67, "addr": 0x58, "way": "i2c"},
            "present": {"loc": "/sys/s3ip/psu/psu1/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "name": "PSU1",
            "airflow": psu_fan_airflow,
            "TempStatus": {"bus": 67, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0004},
            "Temperature": {
                "value": {"loc": "/sys/s3ip/psu/psu1/temp1/value", "way": "sysfs"},
                "Min": threshold.PSU_TEMP_MIN,
                "Max": threshold.PSU_TEMP_MAX,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            },
            "FanStatus": {"bus": 67, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0400},
            "FanSpeed": {
                "value": {"loc": "/sys/s3ip/psu/psu1/fan_speed", "way": "sysfs"},
                "Min": threshold.PSU_FAN_SPEED_MIN,
                "Max": threshold.PSU_FAN_SPEED_MAX,
                "Unit": Unit.Speed
            },
            "psu_fan_tolerance": 40,
            "InputsStatus": {"bus": 67, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x2000},
            "InputsType": {"bus": 67, "addr": 0x58, "offset": 0x80, "way": "i2c", 'psutypedecode': psutypedecode},
            "InputsVoltage": {
                'DC': {
                    "value": {"loc": "/sys/s3ip/psu/psu1/in_vol", "way": "sysfs"},
                    "Min": threshold.PSU_DC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_DC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                },
                'other': {
                    "value": {"loc": "/sys/s3ip/psu/psu1/in_vol", "way": "sysfs"},
                    "Min": threshold.ERR_VALUE,
                    "Max": threshold.ERR_VALUE,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                }
            },
            "InputsCurrent": {
                "value": {"loc": "/sys/s3ip/psu/psu1/in_curr", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_CURRENT_MIN,
                "Max": threshold.PSU_INPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "InputsPower": {
                "value": {"loc": "/sys/s3ip/psu/psu1/in_power", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_POWER_MIN,
                "Max": threshold.PSU_INPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
            "OutputsStatus": {"bus": 67, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x8800},
            "OutputsVoltage": {
                "value": {"loc": "/sys/s3ip/psu/psu1/out_vol", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_VOLTAGE_MIN,
                "Max": threshold.PSU_OUTPUT_VOLTAGE_MAX,
                "Unit": Unit.Voltage,
                "format": "float(float(%s)/1000)"
            },
            "OutputsCurrent": {
                "value": {"loc": "/sys/s3ip/psu/psu1/out_curr", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_CURRENT_MIN,
                "Max": threshold.PSU_OUTPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "OutputsPower": {
                "value": {"loc": "/sys/s3ip/psu/psu1/out_power", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_POWER_MIN,
                "Max": threshold.PSU_OUTPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            }
        },
        {
            "e2loc": {"loc": "/sys/bus/i2c/devices/68-0050/eeprom", "way": "sysfs"},
            "psu_sn": {"loc": "/sys/s3ip/psu/psu2/serial_number", "way": "sysfs"},
            "psu_hw": {"loc": "/sys/s3ip/psu/psu2/hardware_version", "way": "sysfs"},
            "psu_pn": {"loc": "/sys/s3ip/psu/psu2/part_number", "way": "sysfs"},
            "psu_vendor": {"loc": "/sys/s3ip/psu/psu2/vendor", "way": "sysfs"},
            "pmbusloc": {"bus": 68, "addr": 0x58, "way": "i2c"},
            "present": {"loc": "/sys/s3ip/psu/psu2/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "name": "PSU2",
            "airflow": psu_fan_airflow,
            "TempStatus": {"bus": 68, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0004},
            "Temperature": {
                "value": {"loc": "/sys/s3ip/psu/psu2/temp1/value", "way": "sysfs"},
                "Min": threshold.PSU_TEMP_MIN,
                "Max": threshold.PSU_TEMP_MAX,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            },
            "FanStatus": {"bus": 68, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0400},
            "FanSpeed": {
                "value": {"loc": "/sys/s3ip/psu/psu2/fan_speed", "way": "sysfs"},
                "Min": threshold.PSU_FAN_SPEED_MIN,
                "Max": threshold.PSU_FAN_SPEED_MAX,
                "Unit": Unit.Speed
            },
            "psu_fan_tolerance": 40,
            "InputsStatus": {"bus": 68, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x2000},
            "InputsType": {"bus": 68, "addr": 0x58, "offset": 0x80, "way": "i2c", 'psutypedecode': psutypedecode},
            "InputsVoltage": {
                'DC': {
                    "value": {"loc": "/sys/s3ip/psu/psu2/in_vol", "way": "sysfs"},
                    "Min": threshold.PSU_DC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_DC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                },
                'other': {
                    "value": {"loc": "/sys/s3ip/psu/psu2/in_vol", "way": "sysfs"},
                    "Min": threshold.ERR_VALUE,
                    "Max": threshold.ERR_VALUE,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                }
            },
            "InputsCurrent": {
                "value": {"loc": "/sys/s3ip/psu/psu2/in_curr", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_CURRENT_MIN,
                "Max": threshold.PSU_INPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "InputsPower": {
                "value": {"loc": "/sys/s3ip/psu/psu2/in_power", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_POWER_MIN,
                "Max": threshold.PSU_INPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
            "OutputsStatus": {"bus": 68, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x8800},
            "OutputsVoltage": {
                "value": {"loc": "/sys/s3ip/psu/psu2/out_vol", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_VOLTAGE_MIN,
                "Max": threshold.PSU_OUTPUT_VOLTAGE_MAX,
                "Unit": Unit.Voltage,
                "format": "float(float(%s)/1000)"
            },
            "OutputsCurrent": {
                "value": {"loc": "/sys/s3ip/psu/psu2/out_curr", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_CURRENT_MIN,
                "Max": threshold.PSU_OUTPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "OutputsPower": {
                "value": {"loc": "/sys/s3ip/psu/psu2/out_power", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_POWER_MIN,
                "Max": threshold.PSU_OUTPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            }
        },
        {
            "e2loc": {"loc": "/sys/bus/i2c/devices/69-0050/eeprom", "way": "sysfs"},
            "psu_sn": {"loc": "/sys/s3ip/psu/psu3/serial_number", "way": "sysfs"},
            "psu_hw": {"loc": "/sys/s3ip/psu/psu3/hardware_version", "way": "sysfs"},
            "psu_pn": {"loc": "/sys/s3ip/psu/psu3/part_number", "way": "sysfs"},
            "psu_vendor": {"loc": "/sys/s3ip/psu/psu3/vendor", "way": "sysfs"},
            "pmbusloc": {"bus": 69, "addr": 0x58, "way": "i2c"},
            "present": {"loc": "/sys/s3ip/psu/psu3/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "name": "PSU3",
            "airflow": psu_fan_airflow,
            "TempStatus": {"bus": 69, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0004},
            "Temperature": {
                "value": {"loc": "/sys/s3ip/psu/psu3/temp1/value", "way": "sysfs"},
                "Min": threshold.PSU_TEMP_MIN,
                "Max": threshold.PSU_TEMP_MAX,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            },
            "FanStatus": {"bus": 69, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0400},
            "FanSpeed": {
                "value": {"loc": "/sys/s3ip/psu/psu3/fan_speed", "way": "sysfs"},
                "Min": threshold.PSU_FAN_SPEED_MIN,
                "Max": threshold.PSU_FAN_SPEED_MAX,
                "Unit": Unit.Speed
            },
            "psu_fan_tolerance": 40,
            "InputsStatus": {"bus": 69, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x2000},
            "InputsType": {"bus": 69, "addr": 0x58, "offset": 0x80, "way": "i2c", 'psutypedecode': psutypedecode},
            "InputsVoltage": {
                'DC': {
                    "value": {"loc": "/sys/s3ip/psu/psu3/in_vol", "way": "sysfs"},
                    "Min": threshold.PSU_DC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_DC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                },
                'other': {
                    "value": {"loc": "/sys/s3ip/psu/psu3/in_vol", "way": "sysfs"},
                    "Min": threshold.ERR_VALUE,
                    "Max": threshold.ERR_VALUE,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                }
            },
            "InputsCurrent": {
                "value": {"loc": "/sys/s3ip/psu/psu3/in_curr", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_CURRENT_MIN,
                "Max": threshold.PSU_INPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "InputsPower": {
                "value": {"loc": "/sys/s3ip/psu/psu3/in_power", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_POWER_MIN,
                "Max": threshold.PSU_INPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
            "OutputsStatus": {"bus": 69, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x8800},
            "OutputsVoltage": {
                "value": {"loc": "/sys/s3ip/psu/psu3/out_vol", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_VOLTAGE_MIN,
                "Max": threshold.PSU_OUTPUT_VOLTAGE_MAX,
                "Unit": Unit.Voltage,
                "format": "float(float(%s)/1000)"
            },
            "OutputsCurrent": {
                "value": {"loc": "/sys/s3ip/psu/psu3/out_curr", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_CURRENT_MIN,
                "Max": threshold.PSU_OUTPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "OutputsPower": {
                "value": {"loc": "/sys/s3ip/psu/psu3/out_power", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_POWER_MIN,
                "Max": threshold.PSU_OUTPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            }
        },
        {
            "e2loc": {"loc": "/sys/bus/i2c/devices/70-0050/eeprom", "way": "sysfs"},
            "psu_sn": {"loc": "/sys/s3ip/psu/psu4/serial_number", "way": "sysfs"},
            "psu_hw": {"loc": "/sys/s3ip/psu/psu4/hardware_version", "way": "sysfs"},
            "psu_pn": {"loc": "/sys/s3ip/psu/psu4/part_number", "way": "sysfs"},
            "psu_vendor": {"loc": "/sys/s3ip/psu/psu4/vendor", "way": "sysfs"},
            "pmbusloc": {"bus": 70, "addr": 0x58, "way": "i2c"},
            "present": {"loc": "/sys/s3ip/psu/psu4/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "name": "PSU4",
            "airflow": psu_fan_airflow,
            "TempStatus": {"bus": 70, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0004},
            "Temperature": {
                "value": {"loc": "/sys/s3ip/psu/psu4/temp1/value", "way": "sysfs"},
                "Min": threshold.PSU_TEMP_MIN,
                "Max": threshold.PSU_TEMP_MAX,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            },
            "FanStatus": {"bus": 70, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0400},
            "FanSpeed": {
                "value": {"loc": "/sys/s3ip/psu/psu4/fan_speed", "way": "sysfs"},
                "Min": threshold.PSU_FAN_SPEED_MIN,
                "Max": threshold.PSU_FAN_SPEED_MAX,
                "Unit": Unit.Speed
            },
            "psu_fan_tolerance": 40,
            "InputsStatus": {"bus": 70, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x2000},
            "InputsType": {"bus": 70, "addr": 0x58, "offset": 0x80, "way": "i2c", 'psutypedecode': psutypedecode},
            "InputsVoltage": {
                'DC': {
                    "value": {"loc": "/sys/s3ip/psu/psu4/in_vol", "way": "sysfs"},
                    "Min": threshold.PSU_DC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_DC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                },
                'other': {
                    "value": {"loc": "/sys/s3ip/psu/psu4/in_vol", "way": "sysfs"},
                    "Min": threshold.ERR_VALUE,
                    "Max": threshold.ERR_VALUE,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                }
            },
            "InputsCurrent": {
                "value": {"loc": "/sys/s3ip/psu/psu4/in_curr", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_CURRENT_MIN,
                "Max": threshold.PSU_INPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "InputsPower": {
                "value": {"loc": "/sys/s3ip/psu/psu4/in_power", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_POWER_MIN,
                "Max": threshold.PSU_INPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
            "OutputsStatus": {"bus": 70, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x8800},
            "OutputsVoltage": {
                "value": {"loc": "/sys/s3ip/psu/psu4/out_vol", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_VOLTAGE_MIN,
                "Max": threshold.PSU_OUTPUT_VOLTAGE_MAX,
                "Unit": Unit.Voltage,
                "format": "float(float(%s)/1000)"
            },
            "OutputsCurrent": {
                "value": {"loc": "/sys/s3ip/psu/psu4/out_curr", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_CURRENT_MIN,
                "Max": threshold.PSU_OUTPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "OutputsPower": {
                "value": {"loc": "/sys/s3ip/psu/psu4/out_power", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_POWER_MIN,
                "Max": threshold.PSU_OUTPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            }
        }
    ],
    "temps": [
        {
            "name": "CPU_TEMP",
            "temp_id": "TEMP1",
            "api_name": "CPU",
            "Temperature": {
                "value": {"loc": "/sys/s3ip/temp_sensor/temp11/value", "way": "sysfs"},
                "Min": -30000,
                "Low": -20000,
                "High": 90000,
                "Max": 95000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
        {
            "name": "INLET_TEMP",
            "temp_id": "TEMP2",
            "api_name": "Inlet",
            "Temperature": {
                "value": {"loc": "/sys/s3ip/temp_sensor/temp2/value", "way": "sysfs"},
                "Min": -30000,
                "Low": -20000,
                "High": 60000,
                "Max": 70000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
        {
            "name": "OUTLET_TEMP",
            "temp_id": "TEMP3",
            "api_name": "Outlet",
            "Temperature": {
                "value": [
                    {"loc": "/sys/s3ip/temp_sensor/temp3/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp4/value", "way": "sysfs"},
                ],
                "Min": -30000,
                "Low": -20000,
                "High": 70000,
                "Max": 75000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
        {
            "name": "SWITCH_TEMP",
            "temp_id": "TEMP4",
            "api_name": "ASIC_TEMP",
            "Temperature": {
                "value": {"loc": "/sys/s3ip/temp_sensor/temp13/value", "way": "sysfs"},
                "Min": -30000,
                "Low": -20000,
                "High": 100000,
                "Max": 105000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            },
            "invalid": -100000,
            "error": -99999,
        },
        {
            "name": "MOS_TEMP",
            "temp_id": "TEMP5",
            "Temperature": {
                "value": [
                    {"loc": "/sys/s3ip/temp_sensor/temp14/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp15/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp16/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp17/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp18/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp19/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp20/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp21/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp22/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp23/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp24/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp25/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp26/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp27/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp28/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp29/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp30/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp31/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp32/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp33/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp34/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp35/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp36/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp37/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp38/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp39/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp40/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp41/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp42/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp43/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp44/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp45/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp46/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp47/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp48/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp49/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp50/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp51/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp52/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp53/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp54/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp55/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp56/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp57/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp58/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp59/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp60/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp61/value", "way": "sysfs"},
                    {"loc": "/sys/s3ip/temp_sensor/temp62/value", "way": "sysfs"},
                ],
                "Min": -30000,
                "Low": -20000,
                "High": 110000,
                "Max": 125000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
        {
            "name": "BOARD_TEMP",
            "temp_id": "TEMP6",
            "Temperature": {
                "value": {"loc": "/sys/s3ip/temp_sensor/temp6/value", "way": "sysfs"},
                "Min": -30000,
                "Low": -20000,
                "High": 95000,
                "Max": 105000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
        {
            "name": "SFF_TEMP",
            "temp_id": "TEMP7",
            "Temperature": {
                "value": {"loc": "/etc/sonic/highest_sff_temp", "way": "sysfs", "flock_path": "/etc/sonic/highest_sff_temp"},
                "Min": -30000,
                "Low": -20000,
                "High": 80000,
                "Max": 100000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
    ],
    "leds": [
        {
            "name": "FRONT_SYS_LED",
            "led_type": "SYS_LED",
            "led": {"loc": "/sys/s3ip/sysled/sys_led_status", "way": "sysfs"},
            "led_attrs": s3ip_led_decode,
        },
        {
            "name": "FRONT_BMC_LED",
            "led_type": "BMC_LED",
            "led": {"loc": "/sys/s3ip/sysled/bmc_led_status", "way": "sysfs"},
            "led_attrs": s3ip_led_decode,
        },
        {
            "name": "FRONT_PSU_LED",
            "led_type": "PSU_LED",
            "led": {"loc": "/sys/s3ip/sysled/psu_led_status", "way": "sysfs"},
            "led_attrs": s3ip_led_decode,
        },
        {
            "name": "FRONT_FAN_LED",
            "led_type": "FAN_LED",
            "led": {"loc": "/sys/s3ip/sysled/fan_led_status", "way": "sysfs"},
            "led_attrs": s3ip_led_decode,
        },
    ],
    "fans": [
        {
            "name": "FAN1",
            "airflow": fanairflow,
            "e2loc": {'loc': '/sys/bus/i2c/devices/77-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "present": {"loc": "/sys/s3ip/fan/fan1/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"loc": "/sys/s3ip/fan/fan1/led_status", "way": "sysfs"},
            "led_attrs": s3ip_led_decode,
            "PowerMax": 168,
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"loc": "/dev/cpld7", "offset": 0x90, "len": 1, "way": "devfile"},
                    "Running": {"loc": "/sys/s3ip/fan/fan1/motor1/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/s3ip/fan/fan1/motor1/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/s3ip/fan/fan1/motor1/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FRONT_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
                "Rotor2_config": {
                    "name": "Rotor2",
                    "Set_speed": {"loc": "/dev/cpld7", "offset": 0x90, "len": 1, "way": "devfile"},
                    "Running": {"loc": "/sys/s3ip/fan/fan1/motor2/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/s3ip/fan/fan1/motor2/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/s3ip/fan/fan1/motor2/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.REAR_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
            },
        },
        {
            "name": "FAN2",
            "airflow": fanairflow,
            "e2loc": {'loc': '/sys/bus/i2c/devices/85-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "present": {"loc": "/sys/s3ip/fan/fan2/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"loc": "/sys/s3ip/fan/fan2/led_status", "way": "sysfs"},
            "led_attrs": s3ip_led_decode,
            "PowerMax": 168,
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"loc": "/dev/cpld8", "offset": 0x90, "len": 1, "way": "devfile"},
                    "Running": {"loc": "/sys/s3ip/fan/fan2/motor1/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/s3ip/fan/fan2/motor1/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/s3ip/fan/fan2/motor1/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FRONT_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
                "Rotor2_config": {
                    "name": "Rotor2",
                    "Set_speed": {"loc": "/dev/cpld8", "offset": 0x90, "len": 1, "way": "devfile"},
                    "Running": {"loc": "/sys/s3ip/fan/fan2/motor2/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/s3ip/fan/fan2/motor2/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/s3ip/fan/fan2/motor2/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.REAR_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
            },
        },
        {
            "name": "FAN3",
            "airflow": fanairflow,
            "e2loc": {'loc': '/sys/bus/i2c/devices/78-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "present": {"loc": "/sys/s3ip/fan/fan3/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"loc": "/sys/s3ip/fan/fan3/led_status", "way": "sysfs"},
            "led_attrs": s3ip_led_decode,
            "PowerMax": 168,
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"loc": "/dev/cpld7", "offset": 0x91, "len": 1, "way": "devfile"},
                    "Running": {"loc": "/sys/s3ip/fan/fan3/motor1/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/s3ip/fan/fan3/motor1/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/s3ip/fan/fan3/motor1/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FRONT_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
                "Rotor2_config": {
                    "name": "Rotor2",
                    "Set_speed": {"loc": "/dev/cpld7", "offset": 0x91, "len": 1, "way": "devfile"},
                    "Running": {"loc": "/sys/s3ip/fan/fan3/motor2/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/s3ip/fan/fan3/motor2/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/s3ip/fan/fan3/motor2/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.REAR_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
            },
        },
        {
            "name": "FAN4",
            "airflow": fanairflow,
            "e2loc": {'loc': '/sys/bus/i2c/devices/86-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "present": {"loc": "/sys/s3ip/fan/fan4/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"loc": "/sys/s3ip/fan/fan4/led_status", "way": "sysfs"},
            "led_attrs": s3ip_led_decode,
            "PowerMax": 168,
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"loc": "/dev/cpld8", "offset": 0x91, "len": 1, "way": "devfile"},
                    "Running": {"loc": "/sys/s3ip/fan/fan4/motor1/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/s3ip/fan/fan4/motor1/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/s3ip/fan/fan4/motor1/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FRONT_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
                "Rotor2_config": {
                    "name": "Rotor2",
                    "Set_speed": {"loc": "/dev/cpld8", "offset": 0x91, "len": 1, "way": "devfile"},
                    "Running": {"loc": "/sys/s3ip/fan/fan4/motor2/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/s3ip/fan/fan4/motor2/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/s3ip/fan/fan4/motor2/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.REAR_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
            },
        },
        {
            "name": "FAN5",
            "airflow": fanairflow,
            "e2loc": {'loc': '/sys/bus/i2c/devices/79-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "present": {"loc": "/sys/s3ip/fan/fan5/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"loc": "/sys/s3ip/fan/fan5/led_status", "way": "sysfs"},
            "led_attrs": s3ip_led_decode,
            "PowerMax": 168,
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"loc": "/dev/cpld7", "offset": 0x92, "len": 1, "way": "devfile"},
                    "Running": {"loc": "/sys/s3ip/fan/fan5/motor1/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/s3ip/fan/fan5/motor1/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/s3ip/fan/fan5/motor1/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FRONT_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
                "Rotor2_config": {
                    "name": "Rotor2",
                    "Set_speed": {"loc": "/dev/cpld7", "offset": 0x92, "len": 1, "way": "devfile"},
                    "Running": {"loc": "/sys/s3ip/fan/fan5/motor2/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/s3ip/fan/fan5/motor2/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/s3ip/fan/fan5/motor2/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.REAR_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
            },
        },
        {
            "name": "FAN6",
            "airflow": fanairflow,
            "e2loc": {'loc': '/sys/bus/i2c/devices/87-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "present": {"loc": "/sys/s3ip/fan/fan6/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"loc": "/sys/s3ip/fan/fan6/led_status", "way": "sysfs"},
            "led_attrs": s3ip_led_decode,
            "PowerMax": 168,
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"loc": "/dev/cpld8", "offset": 0x92, "len": 1, "way": "devfile"},
                    "Running": {"loc": "/sys/s3ip/fan/fan6/motor1/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/s3ip/fan/fan6/motor1/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                     "Speed": {
                        "value": {"loc": "/sys/s3ip/fan/fan6/motor1/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FRONT_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
                "Rotor2_config": {
                    "name": "Rotor2",
                    "Set_speed": {"loc": "/dev/cpld8", "offset": 0x92, "len": 1, "way": "devfile"},
                    "Running": {"loc": "/sys/s3ip/fan/fan6/motor2/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/s3ip/fan/fan6/motor2/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/s3ip/fan/fan6/motor2/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.REAR_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
            },
        },
        {
            "name": "FAN7",
            "airflow": fanairflow,
            "e2loc": {'loc': '/sys/bus/i2c/devices/80-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "present": {"loc": "/sys/s3ip/fan/fan7/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"loc": "/sys/s3ip/fan/fan7/led_status", "way": "sysfs"},
            "led_attrs": s3ip_led_decode,
            "PowerMax": 168,
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"loc": "/dev/cpld7", "offset": 0x93, "len": 1, "way": "devfile"},
                    "Running": {"loc": "/sys/s3ip/fan/fan7/motor1/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/s3ip/fan/fan7/motor1/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/s3ip/fan/fan7/motor1/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FRONT_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
                "Rotor2_config": {
                    "name": "Rotor2",
                    "Set_speed": {"loc": "/dev/cpld7", "offset": 0x93, "len": 1, "way": "devfile"},
                    "Running": {"loc": "/sys/s3ip/fan/fan7/motor2/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/s3ip/fan/fan7/motor2/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/s3ip/fan/fan7/motor2/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.REAR_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
            },
        },
        {
            "name": "FAN8",
            "airflow": fanairflow,
            "e2loc": {'loc': '/sys/bus/i2c/devices/88-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "present": {"loc": "/sys/s3ip/fan/fan8/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"loc": "/sys/s3ip/fan/fan8/led_status", "way": "sysfs"},
            "led_attrs": s3ip_led_decode,
            "PowerMax": 168,
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"loc": "/dev/cpld8", "offset": 0x93, "len": 1, "way": "devfile"},
                    "Running": {"loc": "/sys/s3ip/fan/fan8/motor1/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/s3ip/fan/fan8/motor1/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/s3ip/fan/fan8/motor1/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FRONT_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
                "Rotor2_config": {
                    "name": "Rotor2",
                    "Set_speed": {"loc": "/dev/cpld8", "offset": 0x93, "len": 1, "way": "devfile"},
                    "Running": {"loc": "/sys/s3ip/fan/fan8/motor2/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/s3ip/fan/fan8/motor2/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/s3ip/fan/fan8/motor2/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.REAR_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
            },
        }
    ],
    "cplds": [
        {
            "name": "CPU_CPLD",
            "cpld_id": "CPLD1",
            "VersionFile": {"loc": "/dev/cpld0", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for system power",
            "slot": 0,
            "warm": 1,
        },
        {
            "name": "MGMT_CPLD",
            "cpld_id": "CPLD2",
            "VersionFile": {"loc": "/dev/cpld1", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for sff modules",
            "slot": 0,
            "warm": 1,
        },
        {
            "name": "MAC_CPLDA",
            "cpld_id": "CPLD3",
            "VersionFile": {"loc": "/dev/cpld2", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for sff modules",
            "slot": 0,
            "warm": 1,
        },
        {
            "name": "MAC_CPLDB",
            "cpld_id": "CPLD4",
            "VersionFile": {"loc": "/dev/cpld3", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for sff modules",
            "slot": 0,
            "warm": 1,
        },
        {
            "name": "MAC_CPLDC",
            "cpld_id": "CPLD5",
            "VersionFile": {"loc": "/dev/cpld4", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for sff modules",
            "slot": 0,
            "warm": 1,
        },
        {
            "name": "UPORT_CPLD",
            "cpld_id": "CPLD6",
            "VersionFile": {"loc": "/dev/cpld5", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for sff modules",
            "slot": 0,
            "warm": 1,
        },
        {
            "name": "DPORT_CPLD",
            "cpld_id": "CPLD7",
            "VersionFile": {"loc": "/dev/cpld6", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for sff modules",
            "slot": 0,
            "warm": 1,
        },
        {
            "name": "UFAN_CPLD",
            "cpld_id": "CPLD8",
            "VersionFile": {"loc": "/dev/cpld7", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for fan modules",
            "slot": 0,
            "warm": 1,
        },
        {
            "name": "DFAN_CPLD",
            "cpld_id": "CPLD9",
            "VersionFile": {"loc": "/dev/cpld8", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for fan modules",
            "slot": 0,
            "warm": 1,
        },
        {
            "name": "MAC_CPLDD",
            "cpld_id": "CPLD10",
            "VersionFile": {"loc": "/dev/cpld9", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for sff modules",
            "slot": 0,
            "warm": 1,
        },
        {
            "name": "MAC_FPGA",
            "cpld_id": "CPLD11",
            "VersionFile": {"loc": "/dev/fpga0", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for base functions",
            "slot": 0,
            "format": "little_endian",
            "warm": 1,
        },
        {
            "name": "UPORT_FPGA",
            "cpld_id": "CPLD12",
            "VersionFile": {"loc": "/dev/fpga1", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for base functions",
            "slot": 0,
            "format": "little_endian",
            "warm": 1,
        },
        {
            "name": "DPORT_FPGA",
            "cpld_id": "CPLD13",
            "VersionFile": {"loc": "/dev/fpga2", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for base functions",
            "slot": 0,
            "format": "little_endian",
            "warm": 1,
        },
        {
            "name": "BIOS",
            "cpld_id": "CPLD14",
            "VersionFile": {"cmd": "dmidecode -s bios-version", "way": "cmd"},
            "desc": "Performs initialization of hardware components during booting",
            "slot": 0,
            "type": "str",
            "warm": 0,
        },
    ],
    "cpu": [
        {
            "name": "cpu",
            "CpuResetCntReg": {"loc": "/dev/cpld1", "offset": 0x10, "len": 1, "way": "devfile_ascii"},
            "reboot_cause_path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"
        }
    ],
    "sfps": {
        "ver": '2.0',
        "port_index_start": 1,
        "port_num": 129,
        "log_level": 2,
        "eeprom_retry_times": 5,
        "eeprom_retry_break_sec": 0.2,
        "presence_path": "/sys/s3ip/transceiver/eth%d/present",
        "presence_val_is_present": 1,
        "eeprom_path": "/sys/s3ip/transceiver/eth%d/eeprom",
        "optoe_driver_path": "/sys/bus/i2c/devices/i2c-%d/%d-0050/dev_class",
        "optoe_driver_key": list(range(201, 330)),
        "lpmode_path": "/sys/s3ip/transceiver/eth%d/low_power_mode",
        "reset_path": "/sys/s3ip/transceiver/eth%d/reset",
        "reset_val_is_reset": 0,
        "tx_dis_path": "/sys/s3ip/transceiver/eth%d/tx_disable",
        "rx_los_path": "/sys/s3ip/transceiver/eth%d/rx_los",
        "tx_fault_path": "/sys/s3ip/transceiver/eth%d/tx_fault",
    }
}
