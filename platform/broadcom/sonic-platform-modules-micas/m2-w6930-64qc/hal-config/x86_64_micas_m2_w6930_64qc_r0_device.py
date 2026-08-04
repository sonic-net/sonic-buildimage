#!/usr/bin/python3

psu_fan_airflow = {
    "intake": ['DPS-1300AB-6 S', 'GW-CRPS1300D'],
    "exhaust": []
}

fanairflow = {
    "intake": ['M2EFAN II-F'],
    "exhaust": [],
}

psu_display_name = {
    "PA1300I-F": ['GW-CRPS1300D', 'DPS-1300AB-6 S'],
}

psutypedecode = {
    0x00: 'N/A',
    0x01: 'AC',
    0x02: 'DC',
}


class Unit:
    Temperature = "C"
    Voltage = "V"
    Current = "A"
    Power = "W"
    Speed = "RPM"


class threshold:
    PSU_TEMP_MIN = -10 * 1000
    PSU_TEMP_MAX = 60 * 1000

    PSU_FAN_SPEED_MIN = 2000
    PSU_FAN_SPEED_MAX = 28000

    PSU_OUTPUT_VOLTAGE_MIN = 11 * 1000
    PSU_OUTPUT_VOLTAGE_MAX = 14 * 1000

    PSU_AC_INPUT_VOLTAGE_MIN = 200 * 1000
    PSU_AC_INPUT_VOLTAGE_MAX = 240 * 1000

    PSU_DC_INPUT_VOLTAGE_MIN = 190 * 1000
    PSU_DC_INPUT_VOLTAGE_MAX = 290 * 1000

    ERR_VALUE = -9999999

    PSU_OUTPUT_POWER_MIN = 10 * 1000 * 1000
    PSU_OUTPUT_POWER_MAX = 1300 * 1000 * 1000

    PSU_INPUT_POWER_MIN = 10 * 1000 * 1000
    PSU_INPUT_POWER_MAX = 1444 * 1000 * 1000

    PSU_OUTPUT_CURRENT_MIN = 2 * 1000
    PSU_OUTPUT_CURRENT_MAX = 107 * 1000

    PSU_INPUT_CURRENT_MIN = 0.2 * 1000
    PSU_INPUT_CURRENT_MAX = 7 * 1000

    FRONT_FAN_SPEED_MAX = 14200
    REAR_FAN_SPEED_MAX = 11200
    FAN_SPEED_MIN = 2000


devices = {
    "onie_e2": [
        {
            "name": "ONIE_E2",
            "e2loc": {"loc": "/sys/bus/i2c/devices/1-0056/eeprom", "way": "sysfs"},
            "airflow": "intake"
        },
    ],
    "psus": [
        {
            "e2loc": {"loc": "/sys/bus/i2c/devices/83-0050/eeprom", "way": "sysfs"},
            "pmbusloc": {"bus": 83, "addr": 0x58, "way": "i2c"},
            "present": {"loc": "/sys/wb_plat/psu/psu1/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "name": "PSU1",
            "psu_display_name": psu_display_name,
            "airflow": psu_fan_airflow,
            "TempStatus": {"bus": 83, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0004},
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-83/83-0058/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": threshold.PSU_TEMP_MIN,
                "Max": threshold.PSU_TEMP_MAX,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            },
            "FanStatus": {"bus": 83, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0400},
            "FanSpeed": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-83/83-0058/hwmon/hwmon*/fan1_input", "way": "sysfs"},
                "Min": threshold.PSU_FAN_SPEED_MIN,
                "Max": threshold.PSU_FAN_SPEED_MAX,
                "Unit": Unit.Speed
            },
            "psu_fan_tolerance": 40,
            "InputsStatus": {"bus": 83, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x2000},
            "InputsType": {"bus": 83, "addr": 0x58, "offset": 0x80, "way": "i2c", 'psutypedecode': psutypedecode},
            "InputsVoltage": {
                'AC': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-83/83-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_AC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_AC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"

                },
                'DC': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-83/83-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_DC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_DC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                },
                'other': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-83/83-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.ERR_VALUE,
                    "Max": threshold.ERR_VALUE,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                }
            },
            "InputsCurrent": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-83/83-0058/hwmon/hwmon*/curr1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_CURRENT_MIN,
                "Max": threshold.PSU_INPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "InputsPower": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-83/83-0058/hwmon/hwmon*/power1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_POWER_MIN,
                "Max": threshold.PSU_INPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
            "OutputsStatus": {"bus": 83, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x8800},
            "OutputsVoltage": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-83/83-0058/hwmon/hwmon*/in2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_VOLTAGE_MIN,
                "Max": threshold.PSU_OUTPUT_VOLTAGE_MAX,
                "Unit": Unit.Voltage,
                "format": "float(float(%s)/1000)"
            },
            "OutputsCurrent": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-83/83-0058/hwmon/hwmon*/curr2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_CURRENT_MIN,
                "Max": threshold.PSU_OUTPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "OutputsPower": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-83/83-0058/hwmon/hwmon*/power2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_POWER_MIN,
                "Max": threshold.PSU_OUTPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
        },
        {
            "e2loc": {"loc": "/sys/bus/i2c/devices/84-0050/eeprom", "way": "sysfs"},
            "pmbusloc": {"bus": 84, "addr": 0x58, "way": "i2c"},
            "present": {"loc": "/sys/wb_plat/psu/psu2/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "name": "PSU2",
            "psu_display_name": psu_display_name,
            "airflow": psu_fan_airflow,
            "TempStatus": {"bus": 84, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0004},
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-84/84-0058/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": threshold.PSU_TEMP_MIN,
                "Max": threshold.PSU_TEMP_MAX,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            },
            "FanStatus": {"bus": 84, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0400},
            "FanSpeed": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-84/84-0058/hwmon/hwmon*/fan1_input", "way": "sysfs"},
                "Min": threshold.PSU_FAN_SPEED_MIN,
                "Max": threshold.PSU_FAN_SPEED_MAX,
                "Unit": Unit.Speed
            },
            "psu_fan_tolerance": 40,
            "InputsStatus": {"bus": 84, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x2000},
            "InputsType": {"bus": 84, "addr": 0x58, "offset": 0x80, "way": "i2c", 'psutypedecode': psutypedecode},
            "InputsVoltage": {
                'AC': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-84/84-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_AC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_AC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"

                },
                'DC': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-84/84-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_DC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_DC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                },
                'other': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-84/84-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.ERR_VALUE,
                    "Max": threshold.ERR_VALUE,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                }
            },
            "InputsCurrent": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-84/84-0058/hwmon/hwmon*/curr1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_CURRENT_MIN,
                "Max": threshold.PSU_INPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "InputsPower": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-84/84-0058/hwmon/hwmon*/power1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_POWER_MIN,
                "Max": threshold.PSU_INPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
            "OutputsStatus": {"bus": 84, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x8800},
            "OutputsVoltage": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-84/84-0058/hwmon/hwmon*/in2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_VOLTAGE_MIN,
                "Max": threshold.PSU_OUTPUT_VOLTAGE_MAX,
                "Unit": Unit.Voltage,
                "format": "float(float(%s)/1000)"
            },
            "OutputsCurrent": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-84/84-0058/hwmon/hwmon*/curr2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_CURRENT_MIN,
                "Max": threshold.PSU_OUTPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "OutputsPower": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-84/84-0058/hwmon/hwmon*/power2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_POWER_MIN,
                "Max": threshold.PSU_OUTPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
        },
        {
            "e2loc": {"loc": "/sys/bus/i2c/devices/86-0050/eeprom", "way": "sysfs"},
            "pmbusloc": {"bus": 86, "addr": 0x58, "way": "i2c"},
            "present": {"loc": "/sys/wb_plat/psu/psu3/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "name": "PSU3",
            "psu_display_name": psu_display_name,
            "airflow": psu_fan_airflow,
            "TempStatus": {"bus": 86, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0004},
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-86/86-0058/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": threshold.PSU_TEMP_MIN,
                "Max": threshold.PSU_TEMP_MAX,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            },
            "FanStatus": {"bus": 86, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0400},
            "FanSpeed": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-86/86-0058/hwmon/hwmon*/fan1_input", "way": "sysfs"},
                "Min": threshold.PSU_FAN_SPEED_MIN,
                "Max": threshold.PSU_FAN_SPEED_MAX,
                "Unit": Unit.Speed
            },
            "psu_fan_tolerance": 40,
            "InputsStatus": {"bus": 86, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x2000},
            "InputsType": {"bus": 86, "addr": 0x58, "offset": 0x80, "way": "i2c", 'psutypedecode': psutypedecode},
            "InputsVoltage": {
                'AC': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-86/86-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_AC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_AC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"

                },
                'DC': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-86/86-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_DC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_DC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                },
                'other': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-86/86-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.ERR_VALUE,
                    "Max": threshold.ERR_VALUE,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                }
            },
            "InputsCurrent": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-86/86-0058/hwmon/hwmon*/curr1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_CURRENT_MIN,
                "Max": threshold.PSU_INPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "InputsPower": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-86/86-0058/hwmon/hwmon*/power1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_POWER_MIN,
                "Max": threshold.PSU_INPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
            "OutputsStatus": {"bus": 86, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x8800},
            "OutputsVoltage": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-86/86-0058/hwmon/hwmon*/in2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_VOLTAGE_MIN,
                "Max": threshold.PSU_OUTPUT_VOLTAGE_MAX,
                "Unit": Unit.Voltage,
                "format": "float(float(%s)/1000)"
            },
            "OutputsCurrent": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-86/86-0058/hwmon/hwmon*/curr2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_CURRENT_MIN,
                "Max": threshold.PSU_OUTPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "OutputsPower": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-86/86-0058/hwmon/hwmon*/power2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_POWER_MIN,
                "Max": threshold.PSU_OUTPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
        },
        {
            "e2loc": {"loc": "/sys/bus/i2c/devices/85-0050/eeprom", "way": "sysfs"},
            "pmbusloc": {"bus": 85, "addr": 0x58, "way": "i2c"},
            "present": {"loc": "/sys/wb_plat/psu/psu4/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "name": "PSU4",
            "psu_display_name": psu_display_name,
            "airflow": psu_fan_airflow,
            "TempStatus": {"bus": 85, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0004},
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-85/85-0058/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": threshold.PSU_TEMP_MIN,
                "Max": threshold.PSU_TEMP_MAX,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            },
            "FanStatus": {"bus": 85, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0400},
            "FanSpeed": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-85/85-0058/hwmon/hwmon*/fan1_input", "way": "sysfs"},
                "Min": threshold.PSU_FAN_SPEED_MIN,
                "Max": threshold.PSU_FAN_SPEED_MAX,
                "Unit": Unit.Speed
            },
            "psu_fan_tolerance": 40,
            "InputsStatus": {"bus": 85, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x2000},
            "InputsType": {"bus": 85, "addr": 0x58, "offset": 0x80, "way": "i2c", 'psutypedecode': psutypedecode},
            "InputsVoltage": {
                'AC': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-85/85-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_AC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_AC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"

                },
                'DC': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-85/85-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_DC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_DC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                },
                'other': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-85/85-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.ERR_VALUE,
                    "Max": threshold.ERR_VALUE,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                }
            },
            "InputsCurrent": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-85/85-0058/hwmon/hwmon*/curr1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_CURRENT_MIN,
                "Max": threshold.PSU_INPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "InputsPower": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-85/85-0058/hwmon/hwmon*/power1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_POWER_MIN,
                "Max": threshold.PSU_INPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
            "OutputsStatus": {"bus": 85, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x8800},
            "OutputsVoltage": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-85/85-0058/hwmon/hwmon*/in2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_VOLTAGE_MIN,
                "Max": threshold.PSU_OUTPUT_VOLTAGE_MAX,
                "Unit": Unit.Voltage,
                "format": "float(float(%s)/1000)"
            },
            "OutputsCurrent": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-85/85-0058/hwmon/hwmon*/curr2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_CURRENT_MIN,
                "Max": threshold.PSU_OUTPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "OutputsPower": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-85/85-0058/hwmon/hwmon*/power2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_POWER_MIN,
                "Max": threshold.PSU_OUTPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
        }
    ],
    "temps": [
        {
            "name": "BOARD_TEMP",
            "temp_id": "TEMP1",
            "api_name": "BOARD_TEMP",
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/118-004f/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": -10000,
                "Low": 0,
                "High": 70000,
                "Max": 75000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
        {
            "name": "CPU_TEMP",
            "temp_id": "TEMP2",
            "api_name": "CPU_TEMP",
            "Temperature": {
                "value": {"loc": "/sys/bus/platform/devices/coretemp.0/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": -15000,
                "Low": 10000,
                "High": 98000,
                "Max": 100000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
        {
            "name": "INLET_TEMP",
            "temp_id": "TEMP3",
            "api_name": "INLET_TEMP",
            "Temperature": {
                "value": [
                    {"loc": "/sys/bus/i2c/devices/79-004b/hwmon/*/temp1_input", "way": "sysfs"},
                ],
                "Min": -30000,
                "Low": 0,
                "High": 40000,
                "Max": 60000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
        {
            "name": "OUTLET_TEMP",
            "temp_id": "TEMP4",
            "api_name": "OUTLET_TEMP",
            "Temperature": {
                "value": [
                    {"loc": "/sys/bus/i2c/devices/93-0048/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                    {"loc": "/sys/bus/i2c/devices/94-0049/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                    {"loc": "/sys/bus/i2c/devices/102-0048/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                    {"loc": "/sys/bus/i2c/devices/103-0049/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                ],
                "Min": -30000,
                "Low": 0,
                "High": 70000,
                "Max": 80000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
        {
            "name": "SWITCH_TEMP",
            "temp_id": "TEMP5",
            "api_name": "ASIC_TEMP",
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/122-0044/hwmon/hwmon*/temp99_input", "way": "sysfs"},
                "Min": -30000,
                "Low": 10000,
                "High": 100000,
                "Max": 105000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
        {
            "name": "PSU1_TEMP",
            "temp_id": "TEMP6",
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-83/83-0058/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": -10000,
                "Low": 0,
                "High": 55000,
                "Max": 60000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
        {
            "name": "PSU2_TEMP",
            "temp_id": "TEMP7",
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-84/84-0058/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": -10000,
                "Low": 0,
                "High": 55000,
                "Max": 60000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
        {
            "name": "PSU3_TEMP",
            "temp_id": "TEMP8",
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-86/86-0058/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": -10000,
                "Low": 0,
                "High": 55000,
                "Max": 60000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
        {
            "name": "PSU4_TEMP",
            "temp_id": "TEMP9",
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-85/85-0058/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": -10000,
                "Low": 0,
                "High": 55000,
                "Max": 60000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
        {
            "name": "SFF_TEMP",
            "Temperature": {
                "value": {"loc": "/tmp/highest_sff_temp", "way": "sysfs", "flock_path": "/tmp/highest_sff_temp"},
                "Min": -15000,
                "Low": 0,
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
            "led": {"loc": "/dev/cpld4", "offset": 0x08, "len": 1, "way": "devfile"},
            "led_attrs": {
                "green": 0x01, "red": 0x02, "amber": 0x03, "default": 0x01,
                "flash": 0xff, "light": 0xff, "off": 0, "mask": 0xff
            },
        },
        {
            "name": "FRONT_PSU_LED",
            "led_type": "PSU_LED",
            "led": {"loc": "/dev/cpld4", "offset": 0x09, "len": 1, "way": "devfile"},
            "led_attrs": {
                "green": 0x04, "red": 0x02, "amber": 0x06, "default": 0x04,
                "flash": 0xff, "light": 0xff, "off": 0, "mask": 0xff
            },
        },
        {
            "name": "FRONT_FAN_LED",
            "led_type": "FAN_LED",
            "led": {"loc": "/dev/cpld4", "offset": 0x0a, "len": 1, "way": "devfile"},
            "led_attrs": {
                "green": 0x04, "red": 0x02, "amber": 0x06, "default": 0x04,
                "flash": 0xff, "light": 0xff, "off": 0, "mask": 0xff
            },
        },
    ],
    "fans": [
        {
            "name": "FAN1",
            "airflow": fanairflow,
            "e2loc": {'loc': '/sys/bus/i2c/devices/i2c-95/95-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "present": {"loc": "/sys/wb_plat/fan/fan1/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 92, "addr": 0x0d, "offset": 0x3b, "way": "i2c"},
            "led_attrs": {
                "green": 0x04, "red": 0x02, "yellow": 0x06, "default": 0x04,
                "flash": 0xff, "light": 0xff, "off": 0, "mask": 0xff
            },
            "PowerMax": 80.64,
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"bus": 92, "addr": 0x0d, "offset": 0x14, "way": "i2c"},
                    "Running": {"loc": "/sys/wb_plat/fan/fan1/motor0/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/wb_plat/fan/fan1/motor0/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/wb_plat/fan/fan1/motor0/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FRONT_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
                "Rotor2_config": {
                    "name": "Rotor2",
                    "Set_speed": {"bus": 92, "addr": 0x0d, "offset": 0x14, "way": "i2c"},
                    "Running": {"loc": "/sys/wb_plat/fan/fan1/motor1/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/wb_plat/fan/fan1/motor1/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/wb_plat/fan/fan1/motor1/speed", "way": "sysfs"},
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
            "e2loc": {'loc': '/sys/bus/i2c/devices/i2c-104/104-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "present": {"loc": "/sys/wb_plat/fan/fan2/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 101, "addr": 0x0d, "offset": 0x3b, "way": "i2c"},
            "led_attrs": {
                "green": 0x04, "red": 0x02, "yellow": 0x06, "default": 0x04,
                "flash": 0xff, "light": 0xff, "off": 0, "mask": 0xff
            },
            "PowerMax": 80.64,
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"bus": 101, "addr": 0x0d, "offset": 0x14, "way": "i2c"},
                    "Running": {"loc": "/sys/wb_plat/fan/fan2/motor0/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/wb_plat/fan/fan2/motor0/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/wb_plat/fan/fan2/motor0/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FRONT_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
                "Rotor2_config": {
                    "name": "Rotor2",
                    "Set_speed": {"bus": 101, "addr": 0x0d, "offset": 0x14, "way": "i2c"},
                    "Running": {"loc": "/sys/wb_plat/fan/fan2/motor1/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/wb_plat/fan/fan2/motor1/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/wb_plat/fan/fan2/motor1/speed", "way": "sysfs"},
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
            "e2loc": {'loc': '/sys/bus/i2c/devices/i2c-96/96-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "present": {"loc": "/sys/wb_plat/fan/fan3/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 92, "addr": 0x0d, "offset": 0x3c, "way": "i2c"},
            "led_attrs": {
                "green": 0x04, "red": 0x02, "yellow": 0x06, "default": 0x04,
                "flash": 0xff, "light": 0xff, "off": 0, "mask": 0xff
            },
            "PowerMax": 80.64,
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"bus": 92, "addr": 0x0d, "offset": 0x15, "way": "i2c"},
                    "Running": {"loc": "/sys/wb_plat/fan/fan3/motor0/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/wb_plat/fan/fan3/motor0/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/wb_plat/fan/fan3/motor0/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FRONT_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
                "Rotor2_config": {
                    "name": "Rotor2",
                    "Set_speed": {"bus": 92, "addr": 0x0d, "offset": 0x15, "way": "i2c"},
                    "Running": {"loc": "/sys/wb_plat/fan/fan3/motor1/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/wb_plat/fan/fan3/motor1/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/wb_plat/fan/fan3/motor1/speed", "way": "sysfs"},
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
            "e2loc": {'loc': '/sys/bus/i2c/devices/i2c-105/105-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "present": {"loc": "/sys/wb_plat/fan/fan4/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 101, "addr": 0x0d, "offset": 0x3c, "way": "i2c"},
            "led_attrs": {
                "green": 0x04, "red": 0x02, "yellow": 0x06, "default": 0x04,
                "flash": 0xff, "light": 0xff, "off": 0, "mask": 0xff
            },
            "PowerMax": 80.64,
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"bus": 101, "addr": 0x0d, "offset": 0x15, "way": "i2c"},
                    "Running": {"loc": "/sys/wb_plat/fan/fan4/motor0/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/wb_plat/fan/fan4/motor0/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                     "Speed": {
                        "value": {"loc": "/sys/wb_plat/fan/fan4/motor0/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FRONT_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
                "Rotor2_config": {
                    "name": "Rotor2",
                    "Set_speed": {"bus": 101, "addr": 0x0d, "offset": 0x15, "way": "i2c"},
                    "Running": {"loc": "/sys/wb_plat/fan/fan4/motor1/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/wb_plat/fan/fan4/motor1/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/wb_plat/fan/fan4/motor1/speed", "way": "sysfs"},
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
            "e2loc": {'loc': '/sys/bus/i2c/devices/i2c-97/97-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "present": {"loc": "/sys/wb_plat/fan/fan5/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 92, "addr": 0x0d, "offset": 0x3d, "way": "i2c"},
            "led_attrs": {
                "green": 0x04, "red": 0x02, "yellow": 0x06, "default": 0x04,
                "flash": 0xff, "light": 0xff, "off": 0, "mask": 0xff
            },
            "PowerMax": 80.64,
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"bus": 92, "addr": 0x0d, "offset": 0x16, "way": "i2c"},
                    "Running": {"loc": "/sys/wb_plat/fan/fan5/motor0/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/wb_plat/fan/fan5/motor0/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/wb_plat/fan/fan5/motor0/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FRONT_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
                "Rotor2_config": {
                    "name": "Rotor2",
                    "Set_speed": {"bus": 92, "addr": 0x0d, "offset": 0x16, "way": "i2c"},
                    "Running": {"loc": "/sys/wb_plat/fan/fan5/motor1/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/wb_plat/fan/fan5/motor1/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/wb_plat/fan/fan5/motor1/speed", "way": "sysfs"},
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
            "e2loc": {'loc': '/sys/bus/i2c/devices/i2c-106/106-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "present": {"loc": "/sys/wb_plat/fan/fan6/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 101, "addr": 0x0d, "offset": 0x3d, "way": "i2c"},
            "led_attrs": {
                "green": 0x04, "red": 0x02, "yellow": 0x06, "default": 0x04,
                "flash": 0xff, "light": 0xff, "off": 0, "mask": 0xff
            },
            "PowerMax": 80.64,
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"bus": 101, "addr": 0x0d, "offset": 0x16, "way": "i2c"},
                    "Running": {"loc": "/sys/wb_plat/fan/fan6/motor0/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/wb_plat/fan/fan6/motor0/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/wb_plat/fan/fan6/motor0/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FRONT_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
                "Rotor2_config": {
                    "name": "Rotor2",
                    "Set_speed": {"bus": 101, "addr": 0x0d, "offset": 0x16, "way": "i2c"},
                    "Running": {"loc": "/sys/wb_plat/fan/fan6/motor1/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/wb_plat/fan/fan6/motor1/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/wb_plat/fan/fan6/motor1/speed", "way": "sysfs"},
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
            "e2loc": {'loc': '/sys/bus/i2c/devices/i2c-98/98-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "present": {"loc": "/sys/wb_plat/fan/fan7/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 92, "addr": 0x0d, "offset": 0x3e, "way": "i2c"},
            "led_attrs": {
                "green": 0x04, "red": 0x02, "yellow": 0x06, "default": 0x04,
                "flash": 0xff, "light": 0xff, "off": 0, "mask": 0xff
            },
            "PowerMax": 80.64,
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"bus": 92, "addr": 0x0d, "offset": 0x17, "way": "i2c"},
                    "Running": {"loc": "/sys/wb_plat/fan/fan7/motor0/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/wb_plat/fan/fan7/motor0/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/wb_plat/fan/fan7/motor0/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FRONT_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
                "Rotor2_config": {
                    "name": "Rotor2",
                    "Set_speed": {"bus": 92, "addr": 0x0d, "offset": 0x17, "way": "i2c"},
                    "Running": {"loc": "/sys/wb_plat/fan/fan7/motor1/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/wb_plat/fan/fan7/motor1/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/wb_plat/fan/fan7/motor1/speed", "way": "sysfs"},
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
            "e2loc": {'loc': '/sys/bus/i2c/devices/i2c-107/107-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "present": {"loc": "/sys/wb_plat/fan/fan8/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 101, "addr": 0x0d, "offset": 0x3e, "way": "i2c"},
            "led_attrs": {
                "green": 0x04, "red": 0x02, "yellow": 0x06, "default": 0x04,
                "flash": 0xff, "light": 0xff, "off": 0, "mask": 0xff
            },
            "PowerMax": 80.64,
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"bus": 101, "addr": 0x0d, "offset": 0x17, "way": "i2c"},
                    "Running": {"loc": "/sys/wb_plat/fan/fan8/motor0/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/wb_plat/fan/fan8/motor0/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/wb_plat/fan/fan8/motor0/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FRONT_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
                "Rotor2_config": {
                    "name": "Rotor2",
                    "Set_speed": {"bus": 101, "addr": 0x0d, "offset": 0x17, "way": "i2c"},
                    "Running": {"loc": "/sys/wb_plat/fan/fan8/motor1/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/wb_plat/fan/fan8/motor1/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/wb_plat/fan/fan8/motor1/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.REAR_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
            },
        },
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
            "name": "CONNECT_CPLD",
            "cpld_id": "CPLD2",
            "VersionFile": {"loc": "/dev/cpld1", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for base functions",
            "slot": 0,
            "warm": 1,
        },
        {
            "name": "MAC_CPLDA",
            "cpld_id": "CPLD3",
            "VersionFile": {"loc": "/dev/cpld4", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for sff modules",
            "slot": 0,
            "warm": 1,
        },
        {
            "name": "MAC_CPLDB",
            "cpld_id": "CPLD4",
            "VersionFile": {"loc": "/dev/cpld5", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for sff modules",
            "slot": 0,
            "warm": 1,
        },
        {
            "name": "PORT_CPLDA",
            "cpld_id": "CPLD5",
            "VersionFile": {"loc": "/dev/cpld6", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for sff modules",
            "slot": 0,
            "warm": 1,
        },
        {
            "name": "PORT_CPLDB",
            "cpld_id": "CPLD6",
            "VersionFile": {"loc": "/dev/cpld7", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for sff modules",
            "slot": 0,
            "warm": 1,
        },
        {
            "name": "FAN_CPLDA",
            "cpld_id": "CPLD7",
            "VersionFile": {"loc": "/dev/cpld8", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for fan modules",
            "slot": 0,
            "warm": 1,
        },
        {
            "name": "FAN_CPLDB",
            "cpld_id": "CPLD8",
            "VersionFile": {"loc": "/dev/cpld9", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for fan modules",
            "slot": 0,
            "warm": 1,
        },
        {
            "name": "MAC_FPGA",
            "cpld_id": "CPLD9",
            "VersionFile": {"loc": "/dev/fpga0", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for base functions",
            "slot": 0,
            "format": "little_endian",
            "warm": 1,
        },
        {
            "name": "PORT_FPGA",
            "cpld_id": "CPLD10",
            "VersionFile": {"loc": "/dev/fpga1", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for base functions",
            "slot": 0,
            "format": "little_endian",
            "warm": 1,
        },
        {
            "name": "BIOS",
            "cpld_id": "CPLD11",
            "VersionFile": {"cmd": "dmidecode -s bios-version", "way": "cmd"},
            "desc": "Performs initialization of hardware components during booting",
            "slot": 0,
            "type": "str",
            "warm": 0,
        },
    ],
     "dcdc": [
        {
            "name": "MAC_VDD3.3V_standby",
            "dcdc_id": "DCDC1",
            "value": {
                "loc": "/sys/bus/i2c/devices/128-005b/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 2805,
            "Max": 3795,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_VDD12V_A",
            "dcdc_id": "DCDC2",
            "value": {
                "loc": "/sys/bus/i2c/devices/128-005b/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 10200,
            "Max": 13800,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_VDD1.0V_FPGA",
            "dcdc_id": "DCDC3",
            "value": {
                "loc": "/sys/bus/i2c/devices/128-005b/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 850,
            "Max": 1150,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_VDD1.8V_FPGA",
            "dcdc_id": "DCDC4",
            "value": {
                "loc": "/sys/bus/i2c/devices/128-005b/hwmon/hwmon*/in4_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 1530,
            "Max": 2070,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_VDD1.2V_FPGA",
            "dcdc_id": "DCDC5",
            "value": {
                "loc": "/sys/bus/i2c/devices/128-005b/hwmon/hwmon*/in5_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 1020,
            "Max": 1380,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_VDD3.3V",
            "dcdc_id": "DCDC6",
            "value": {
                "loc": "/sys/bus/i2c/devices/128-005b/hwmon/hwmon*/in6_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 2805,
            "Max": 3795,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_VDD5V_CLK_MCU",
            "dcdc_id": "DCDC7",
            "value": {
                "loc": "/sys/bus/i2c/devices/128-005b/hwmon/hwmon*/in7_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 4250,
            "Max": 5750,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_VDD3.3V_MAC",
            "dcdc_id": "DCDC8",
            "value": {
                "loc": "/sys/bus/i2c/devices/128-005b/hwmon/hwmon*/in8_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 2805,
            "Max": 3795,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_VDDO1.8V",
            "dcdc_id": "DCDC9",
            "value": {
                "loc": "/sys/bus/i2c/devices/128-005b/hwmon/hwmon*/in9_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 1530,
            "Max": 2070,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_VDDO1.2V",
            "dcdc_id": "DCDC10",
            "value": {
                "loc": "/sys/bus/i2c/devices/128-005b/hwmon/hwmon*/in10_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 1020,
            "Max": 1380,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_VDD_CORE",
            "dcdc_id": "DCDC11",
            "value": {
                "loc": "/sys/bus/i2c/devices/128-005b/hwmon/hwmon*/in11_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 680,
            "Max": 980,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_VDD3.3_CLK",
            "dcdc_id": "DCDC12",
            "value": {
                "loc": "/sys/bus/i2c/devices/128-005b/hwmon/hwmon*/in12_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 2805,
            "Max": 3795,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_VDD_ANALOG",
            "dcdc_id": "DCDC13",
            "value": {
                "loc": "/sys/bus/i2c/devices/128-005b/hwmon/hwmon*/in13_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 655,
            "Max": 885,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_VDD1.2V_MAC_A",
            "dcdc_id": "DCDC14",
            "value": {
                "loc": "/sys/bus/i2c/devices/128-005b/hwmon/hwmon*/in14_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 1020,
            "Max": 1380,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_AVDD1.8V",
            "dcdc_id": "DCDC15",
            "value": {
                "loc": "/sys/bus/i2c/devices/128-005b/hwmon/hwmon*/in15_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 1530,
            "Max": 2070,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_VDD_ANALOG2",
            "dcdc_id": "DCDC16",
            "value": {
                "loc": "/sys/bus/i2c/devices/128-005b/hwmon/hwmon*/in16_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 655,
            "Max": 885,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_VDD12V_B",
            "dcdc_id": "DCDC17",
            "value": {
                "loc": "/sys/bus/i2c/devices/129-005b/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 10200,
            "Max": 13800,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_VDD5.0V",
            "dcdc_id": "DCDC18",
            "value": {
                "loc": "/sys/bus/i2c/devices/129-005b/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 4250,
            "Max": 5750,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_QSFPDD_VDD3.3V_A",
            "dcdc_id": "DCDC19",
            "value": {
                "loc": "/sys/bus/i2c/devices/129-005b/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 2805,
            "Max": 3795,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_QSFPDD_VDD3.3V_B",
            "dcdc_id": "DCDC20",
            "value": {
                "loc": "/sys/bus/i2c/devices/129-005b/hwmon/hwmon*/in4_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 2805,
            "Max": 3795,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_QSFPDD_VDD3.3V_C",
            "dcdc_id": "DCDC21",
            "value": {
                "loc": "/sys/bus/i2c/devices/129-005b/hwmon/hwmon*/in5_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 2805,
            "Max": 3795,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_QSFPDD_VDD3.3V_D",
            "dcdc_id": "DCDC22",
            "value": {
                "loc": "/sys/bus/i2c/devices/129-005b/hwmon/hwmon*/in6_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 2805,
            "Max": 3795,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_QSFPDD_VDD3.3V_E",
            "dcdc_id": "DCDC23",
            "value": {
                "loc": "/sys/bus/i2c/devices/129-005b/hwmon/hwmon*/in7_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 2805,
            "Max": 3795,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_QSFPDD_VDD3.3V_F",
            "dcdc_id": "DCDC24",
            "value": {
                "loc": "/sys/bus/i2c/devices/129-005b/hwmon/hwmon*/in8_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 2805,
            "Max": 3795,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "PORT_VDD1.0V_FPGA",
            "dcdc_id": "DCDC25",
            "value": {
                "loc": "/sys/bus/i2c/devices/130-005b/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 850,
            "Max": 1150,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "PORT_VDD1.8V_FPGA",
            "dcdc_id": "DCDC26",
            "value": {
                "loc": "/sys/bus/i2c/devices/130-005b/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 1530,
            "Max": 2070,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "PORT_VDD1.2V_FPGA",
            "dcdc_id": "DCDC27",
            "value": {
                "loc": "/sys/bus/i2c/devices/130-005b/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 1020,
            "Max": 1380,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "PORT_VDD3.3V",
            "dcdc_id": "DCDC28",
            "value": {
                "loc": "/sys/bus/i2c/devices/130-005b/hwmon/hwmon*/in4_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 2805,
            "Max": 3795,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "PORT_VDD12V",
            "dcdc_id": "DCDC29",
            "value": {
                "loc": "/sys/bus/i2c/devices/130-005b/hwmon/hwmon*/in5_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 10200,
            "Max": 13800,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "PORT_VDD3.3V_standby",
            "dcdc_id": "DCDC30",
            "value": {
                "loc": "/sys/bus/i2c/devices/130-005b/hwmon/hwmon*/in6_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 2805,
            "Max": 3795,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "PORT_QSFPDD_VDD3.3V_A",
            "dcdc_id": "DCDC31",
            "value": {
                "loc": "/sys/bus/i2c/devices/130-005b/hwmon/hwmon*/in7_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 2805,
            "Max": 3795,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "PORT_QSFPDD_VDD3.3V_B",
            "dcdc_id": "DCDC32",
            "value": {
                "loc": "/sys/bus/i2c/devices/130-005b/hwmon/hwmon*/in8_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 2805,
            "Max": 3795,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "PORT_QSFPDD_VDD3.3V_C",
            "dcdc_id": "DCDC33",
            "value": {
                "loc": "/sys/bus/i2c/devices/130-005b/hwmon/hwmon*/in9_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 2805,
            "Max": 3795,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "PORT_QSFPDD_VDD3.3V_D",
            "dcdc_id": "DCDC34",
            "value": {
                "loc": "/sys/bus/i2c/devices/130-005b/hwmon/hwmon*/in10_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 2805,
            "Max": 3795,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "PORT_QSFPDD_VDD3.3V_E",
            "dcdc_id": "DCDC35",
            "value": {
                "loc": "/sys/bus/i2c/devices/130-005b/hwmon/hwmon*/in11_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 2805,
            "Max": 3795,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "PORT_QSFPDD_VDD3.3V_F",
            "dcdc_id": "DCDC36",
            "value": {
                "loc": "/sys/bus/i2c/devices/130-005b/hwmon/hwmon*/in12_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 2805,
            "Max": 3795,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_VDD1.2V_MAC_B",
            "dcdc_id": "DCDC37",
            "value": {
                "loc": "/sys/bus/i2c/devices/131-0067/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 1020,
            "Max": 1380,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "BASE_VDD12V",
            "dcdc_id": "DCDC38",
            "value": {
                "loc": "/sys/bus/i2c/devices/77-005b/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 10830,
            "Max": 13230,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "BASE_SW_VDD1.2V",
            "dcdc_id": "DCDC39",
            "value": {
                "loc": "/sys/bus/i2c/devices/77-005b/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 1083,
            "Max": 1323,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "BASE_VDD2.5V",
            "dcdc_id": "DCDC40",
            "value": {
                "loc": "/sys/bus/i2c/devices/77-005b/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 2247,
            "Max": 2745,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "BASE_VDD3.3V",
            "dcdc_id": "DCDC41",
            "value": {
                "loc": "/sys/bus/i2c/devices/77-005b/hwmon/hwmon*/in4_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 2960,
            "Max": 3616,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "BASE_SSD_VDD3.3V",
            "dcdc_id": "DCDC42",
            "value": {
                "loc": "/sys/bus/i2c/devices/77-005b/hwmon/hwmon*/in5_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 2978,
            "Max": 3638,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "CPU_P1V05_V",
            "dcdc_id": "DCDC43",
            "value": {
                "loc": "/sys/bus/i2c/devices/78-0067/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 882,
            "Max": 1232,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "CPU_VCCIN_V",
            "dcdc_id": "DCDC44",
            "value": {
                "loc": "/sys/bus/i2c/devices/78-0067/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 1368,
            "Max": 2244,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "CPU_VCCD_V",
            "dcdc_id": "DCDC45",
            "value": {
                "loc": "/sys/bus/i2c/devices/78-006c/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 990,
            "Max": 1452,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "CPU_VCCSCSUS_V",
            "dcdc_id": "DCDC46",
            "value": {
                "loc": "/sys/bus/i2c/devices/78-006c/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 855,
            "Max": 1265,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "CPU_P5V_AUX_V",
            "dcdc_id": "DCDC47",
            "value": {
                "loc": "/sys/bus/i2c/devices/78-0043/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 3852,
            "Max": 6347,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "CPU_P3V3_STBY_V",
            "dcdc_id": "DCDC48",
            "value": {
                "loc": "/sys/bus/i2c/devices/78-0043/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 2682,
            "Max": 4004,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "CPU_P1V7_VCCSCFUSESUS_V",
            "dcdc_id": "DCDC49",
            "value": {
                "loc": "/sys/bus/i2c/devices/78-0043/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 1,
            "Min": 1377,
            "Max": 2057,
            "Unit": "V",
            "format": "float(float(%s)/1000)",
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
        "ver": '1.0',
        "port_index_start": 0,
        "port_num": 64,
        "log_level": 2,
        "eeprom_retry_times": 5,
        "eeprom_retry_break_sec": 0.2,
        "presence_cpld": {
            "dev_id": {
                4: {
                    "offset": {
                        0x71: "49-50, 53-54, 57-58, 61-62",
                    },
                },
                5: {
                    "offset": {
                        0x73: "1-2, 5-6, 9-10, 13-14",
                        0x74: "17-18, 21-22, 25-26, 29-30",
                        0x75: "33-34, 37-38, 41-42, 45-46",
                    },
                },
                6: {
                    "offset": {
                        0x72: "3-4, 7-8, 11-12, 15-16",
                        0x73: "20, 23-24, 27-28, 31-32",
                    },
                },
                7: {
                    "offset": {
                        0x73: "19, 35-36, 39-40, 43-44, 47",
                        0x74: "48, 51-52, 55-56, 59-60, 63",
                        0x75: "64",
                    },
                },
            },
        },
        "presence_val_is_present": 0,
        "eeprom_path": "/sys/bus/i2c/devices/i2c-%d/%d-0050/eeprom",
        "eeprom_path_key": list(range(133, 197)),
        "optoe_driver_path": "/sys/bus/i2c/devices/i2c-%d/%d-0050/dev_class",
        "optoe_driver_key": list(range(133, 197)),
        "reset_cpld": {
            "dev_id": {
                4: {
                    "offset": {
                        0x70: "49-50, 53-54, 57-58, 61-62",
                    },
                },
                5: {
                    "offset": {
                        0x70: "1-2, 5-6, 9-10, 13-14",
                        0x71: "17-18, 21-22, 25-26, 29-30",
                        0x72: "33-34, 37-38, 41-42, 45-46",
                    },
                },
                6: {
                    "offset": {
                        0x70: "3-4, 7-8, 11-12, 15-16",
                        0x71: "20, 23-24, 27-28, 31-32",
                    },
                },
                7: {
                    "offset": {
                        0x70: "19, 35-36, 39-40, 43-44, 47",
                        0x71: "48, 51-52, 55-56, 59-60, 63",
                        0x72: "64",
                    },
                },
            },
        },
        "reset_val_is_reset": 0,
    }
}
