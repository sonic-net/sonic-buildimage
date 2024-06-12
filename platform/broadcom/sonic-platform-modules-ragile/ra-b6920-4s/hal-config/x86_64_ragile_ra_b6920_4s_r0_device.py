# coding:utf-8

psu_fan_airflow = {
    "intake": ['DPS-1300AB-6 S', 'GW-CRPS1300D'],
}

fanairflow = {
    "intake": ['P2EFAN I-F'],
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

    PSU_FAN_SPEED_MIN = 150
    PSU_FAN_SPEED_MAX = 12000

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

    FRONT_FAN_SPEED_MAX = 11000
    REAR_FAN_SPEED_MAX = 9500
    FAN_SPEED_MIN = 1500


class Description:
    CPLD = "Used for managing IO modules, SFP+ modules and system LEDs"
    BIOS = "Performs initialization of hardware components during booting"
    FPGA = "Platform management controller for on-board temperature monitoring, in-chassis power"


devices = {
    "onie_e2": [
        {
            "name": "ONIE_E2",
            "e2loc": {"loc": "/sys/bus/i2c/devices/1-0056/eeprom", "way": "sysfs"},
        },
    ],
    "psus": [
        {
            "e2loc": {"loc": "/sys/bus/i2c/devices/23-0050/eeprom", "way": "sysfs"},
            "pmbusloc": {"bus": 23, "addr": 0x58, "way": "i2c"},
            "present": {"loc": "/sys/wb_plat/psu/psu1/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "name": "PSU1",
            "psu_display_name": psu_display_name,
            "airflow": psu_fan_airflow,
            "TempStatus": {"bus": 23, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0004},
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/23-0058/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": threshold.PSU_TEMP_MIN,
                "Max": threshold.PSU_TEMP_MAX,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            },
            "FanStatus": {"bus": 23, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0400},
            "FanSpeed": {
                "value": {"loc": "/sys/bus/i2c/devices/23-0058/hwmon/hwmon*/fan1_input", "way": "sysfs"},
                "Min": threshold.PSU_FAN_SPEED_MIN,
                "Max": threshold.PSU_FAN_SPEED_MAX,
                "Unit": Unit.Speed
            },
            "InputsStatus": {"bus": 23, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x2000},
            "InputsType": {"bus": 23, "addr": 0x58, "offset": 0x80, "way": "i2c", 'psutypedecode': psutypedecode},
            "InputsVoltage": {
                'AC': {
                    "value": {"loc": "/sys/bus/i2c/devices/23-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_AC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_AC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                },
                'DC': {
                    "value": {"loc": "/sys/bus/i2c/devices/23-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_DC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_DC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                },
                'other': {
                    "value": {"loc": "/sys/bus/i2c/devices/23-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.ERR_VALUE,
                    "Max": threshold.ERR_VALUE,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                }
            },
            "InputsCurrent":
            {
                "value": {"loc": "/sys/bus/i2c/devices/23-0058/hwmon/hwmon*/curr1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_CURRENT_MIN,
                "Max": threshold.PSU_INPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "InputsPower":
            {
                "value": {"loc": "/sys/bus/i2c/devices/23-0058/hwmon/hwmon*/power1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_POWER_MIN,
                "Max": threshold.PSU_INPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
            "OutputsStatus": {"bus": 23, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x8800},
            "OutputsVoltage":
            {
                "value": {"loc": "/sys/bus/i2c/devices/23-0058/hwmon/hwmon*/in2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_VOLTAGE_MIN,
                "Max": threshold.PSU_OUTPUT_VOLTAGE_MAX,
                "Unit": Unit.Voltage,
                "format": "float(float(%s)/1000)"
            },
            "OutputsCurrent":
            {
                "value": {"loc": "/sys/bus/i2c/devices/23-0058/hwmon/hwmon*/curr2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_CURRENT_MIN,
                "Max": threshold.PSU_OUTPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "OutputsPower":
            {
                "value": {"loc": "/sys/bus/i2c/devices/23-0058/hwmon/hwmon*/power2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_POWER_MIN,
                "Max": threshold.PSU_OUTPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
        },
        {
            "e2loc": {"loc": "/sys/bus/i2c/devices/25-0050/eeprom", "way": "sysfs"},
            "pmbusloc": {"bus": 25, "addr": 0x58, "way": "i2c"},
            "present": {"loc": "/sys/wb_plat/psu/psu2/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "name": "PSU2",
            "psu_display_name": psu_display_name,
            "airflow": psu_fan_airflow,
            "TempStatus": {"bus": 25, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0004},
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/25-0058/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": threshold.PSU_TEMP_MIN,
                "Max": threshold.PSU_TEMP_MAX,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            },
            "FanStatus": {"bus": 25, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0400},
            "FanSpeed": {
                "value": {"loc": "/sys/bus/i2c/devices/25-0058/hwmon/hwmon*/fan1_input", "way": "sysfs"},
                "Min": threshold.PSU_FAN_SPEED_MIN,
                "Max": threshold.PSU_FAN_SPEED_MAX,
                "Unit": Unit.Speed
            },
            "InputsStatus": {"bus": 25, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x2000},
            "InputsType": {"bus": 25, "addr": 0x58, "offset": 0x80, "way": "i2c", 'psutypedecode': psutypedecode},
            "InputsVoltage": {
                'AC': {
                    "value": {"loc": "/sys/bus/i2c/devices/25-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_AC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_AC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                },
                'DC': {
                    "value": {"loc": "/sys/bus/i2c/devices/25-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_DC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_DC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                },
                'other': {
                    "value": {"loc": "/sys/bus/i2c/devices/25-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.ERR_VALUE,
                    "Max": threshold.ERR_VALUE,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                }
            },
            "InputsCurrent":
            {
                "value": {"loc": "/sys/bus/i2c/devices/25-0058/hwmon/hwmon*/curr1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_CURRENT_MIN,
                "Max": threshold.PSU_INPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "InputsPower":
            {
                "value": {"loc": "/sys/bus/i2c/devices/25-0058/hwmon/hwmon*/power1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_POWER_MIN,
                "Max": threshold.PSU_INPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
            "OutputsStatus": {"bus": 25, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x8800},
            "OutputsVoltage":
            {
                "value": {"loc": "/sys/bus/i2c/devices/25-0058/hwmon/hwmon*/in2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_VOLTAGE_MIN,
                "Max": threshold.PSU_OUTPUT_VOLTAGE_MAX,
                "Unit": Unit.Voltage,
                "format": "float(float(%s)/1000)"
            },
            "OutputsCurrent":
            {
                "value": {"loc": "/sys/bus/i2c/devices/25-0058/hwmon/hwmon*/curr2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_CURRENT_MIN,
                "Max": threshold.PSU_OUTPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "OutputsPower":
            {
                "value": {"loc": "/sys/bus/i2c/devices/25-0058/hwmon/hwmon*/power2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_POWER_MIN,
                "Max": threshold.PSU_OUTPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
        },
        {
            "e2loc": {"loc": "/sys/bus/i2c/devices/24-0050/eeprom", "way": "sysfs"},
            "pmbusloc": {"bus": 24, "addr": 0x58, "way": "i2c"},
            "present": {"loc": "/sys/wb_plat/psu/psu3/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "name": "PSU3",
            "psu_display_name": psu_display_name,
            "airflow": psu_fan_airflow,
            "TempStatus": {"bus": 24, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0004},
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/24-0058/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": threshold.PSU_TEMP_MIN,
                "Max": threshold.PSU_TEMP_MAX,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            },
            "FanStatus": {"bus": 24, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0400},
            "FanSpeed": {
                "value": {"loc": "/sys/bus/i2c/devices/24-0058/hwmon/hwmon*/fan1_input", "way": "sysfs"},
                "Min": threshold.PSU_FAN_SPEED_MIN,
                "Max": threshold.PSU_FAN_SPEED_MAX,
                "Unit": Unit.Speed
            },
            "InputsStatus": {"bus": 24, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x2000},
            "InputsType": {"bus": 24, "addr": 0x58, "offset": 0x80, "way": "i2c", 'psutypedecode': psutypedecode},
            "InputsVoltage": {
                'AC': {
                    "value": {"loc": "/sys/bus/i2c/devices/24-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_AC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_AC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                },
                'DC': {
                    "value": {"loc": "/sys/bus/i2c/devices/24-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_DC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_DC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                },
                'other': {
                    "value": {"loc": "/sys/bus/i2c/devices/24-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.ERR_VALUE,
                    "Max": threshold.ERR_VALUE,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                }
            },
            "InputsCurrent":
            {
                "value": {"loc": "/sys/bus/i2c/devices/24-0058/hwmon/hwmon*/curr1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_CURRENT_MIN,
                "Max": threshold.PSU_INPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "InputsPower":
            {
                "value": {"loc": "/sys/bus/i2c/devices/24-0058/hwmon/hwmon*/power1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_POWER_MIN,
                "Max": threshold.PSU_INPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
            "OutputsStatus": {"bus": 24, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x8800},
            "OutputsVoltage":
            {
                "value": {"loc": "/sys/bus/i2c/devices/24-0058/hwmon/hwmon*/in2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_VOLTAGE_MIN,
                "Max": threshold.PSU_OUTPUT_VOLTAGE_MAX,
                "Unit": Unit.Voltage,
                "format": "float(float(%s)/1000)"
            },
            "OutputsCurrent":
            {
                "value": {"loc": "/sys/bus/i2c/devices/24-0058/hwmon/hwmon*/curr2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_CURRENT_MIN,
                "Max": threshold.PSU_OUTPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "OutputsPower":
            {
                "value": {"loc": "/sys/bus/i2c/devices/24-0058/hwmon/hwmon*/power2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_POWER_MIN,
                "Max": threshold.PSU_OUTPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
        },
        {
            "e2loc": {"loc": "/sys/bus/i2c/devices/26-0050/eeprom", "way": "sysfs"},
            "pmbusloc": {"bus": 26, "addr": 0x58, "way": "i2c"},
            "present": {"loc": "/sys/wb_plat/psu/psu4/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "name": "PSU4",
            "psu_display_name": psu_display_name,
            "airflow": psu_fan_airflow,
            "TempStatus": {"bus": 26, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0004},
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/26-0058/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": threshold.PSU_TEMP_MIN,
                "Max": threshold.PSU_TEMP_MAX,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            },
            "FanStatus": {"bus": 26, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0400},
            "FanSpeed": {
                "value": {"loc": "/sys/bus/i2c/devices/26-0058/hwmon/hwmon*/fan1_input", "way": "sysfs"},
                "Min": threshold.PSU_FAN_SPEED_MIN,
                "Max": threshold.PSU_FAN_SPEED_MAX,
                "Unit": Unit.Speed
            },
            "InputsStatus": {"bus": 26, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x2000},
            "InputsType": {"bus": 26, "addr": 0x58, "offset": 0x80, "way": "i2c", 'psutypedecode': psutypedecode},
            "InputsVoltage": {
                'AC': {
                    "value": {"loc": "/sys/bus/i2c/devices/26-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_AC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_AC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                },
                'DC': {
                    "value": {"loc": "/sys/bus/i2c/devices/26-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_DC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_DC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                },
                'other': {
                    "value": {"loc": "/sys/bus/i2c/devices/26-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.ERR_VALUE,
                    "Max": threshold.ERR_VALUE,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                }
            },
            "InputsCurrent":
            {
                "value": {"loc": "/sys/bus/i2c/devices/26-0058/hwmon/hwmon*/curr1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_CURRENT_MIN,
                "Max": threshold.PSU_INPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "InputsPower":
            {
                "value": {"loc": "/sys/bus/i2c/devices/26-0058/hwmon/hwmon*/power1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_POWER_MIN,
                "Max": threshold.PSU_INPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
            "OutputsStatus": {"bus": 26, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x8800},
            "OutputsVoltage":
            {
                "value": {"loc": "/sys/bus/i2c/devices/26-0058/hwmon/hwmon*/in2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_VOLTAGE_MIN,
                "Max": threshold.PSU_OUTPUT_VOLTAGE_MAX,
                "Unit": Unit.Voltage,
                "format": "float(float(%s)/1000)"
            },
            "OutputsCurrent":
            {
                "value": {"loc": "/sys/bus/i2c/devices/26-0058/hwmon/hwmon*/curr2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_CURRENT_MIN,
                "Max": threshold.PSU_OUTPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "OutputsPower":
            {
                "value": {"loc": "/sys/bus/i2c/devices/26-0058/hwmon/hwmon*/power2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_POWER_MIN,
                "Max": threshold.PSU_OUTPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
        },
    ],
    "temps": [
        {
            "name": "BOARD_TEMP",
            "temp_id": "TEMP1",
            "Temperature": {
                "value": [
                    {"loc": "/sys/bus/i2c/devices/32-0048/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                    {"loc": "/sys/bus/i2c/devices/32-004d/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                    {"loc": "/sys/bus/i2c/devices/48-0048/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                    {"loc": "/sys/bus/i2c/devices/48-004d/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                    {"loc": "/sys/bus/i2c/devices/40-0048/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                    {"loc": "/sys/bus/i2c/devices/40-004d/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                    {"loc": "/sys/bus/i2c/devices/16-0048/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                    {"loc": "/sys/bus/i2c/devices/16-004d/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                ],
                "Min": -10000,
                "Low": 0,
                "High": 85000,
                "Max": 90000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
        {
            "name": "CPU_TEMP",
            "temp_id": "TEMP2",
            "Temperature": {
                "value": {"loc": "/sys/bus/platform/devices/coretemp.0/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": 2000,
                "Low": 10000,
                "High": 85000,
                "Max": 100000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
        {
            "name": "INLET_TEMP",
            "temp_id": "TEMP3",
            "Temperature": {
                "value": [
                    {"loc": "/sys/bus/i2c/devices/32-0049/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                    {"loc": "/sys/bus/i2c/devices/48-0049/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                    {"loc": "/sys/bus/i2c/devices/40-0049/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                    {"loc": "/sys/bus/i2c/devices/16-0049/hwmon/hwmon*/temp1_input", "way": "sysfs"}
                ],
                "Min": -10000,
                "Low": 0,
                "High": 50000,
                "Max": 60000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
        {
            "name": "OUTLET_TEMP",
            "temp_id": "TEMP4",
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/28-004b/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": -10000,
                "Low": 0,
                "High": 85000,
                "Max": 90000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
        {
            "name": "SWITCH_TEMP",
            "temp_id": "TEMP5",
            "api_name": "ASIC_TEMP",
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/28-004c/hwmon/hwmon*/temp2_input", "way": "sysfs"},
                "Min": 2000,
                "Low": 10000,
                "High": 100000,
                "Max": 105000,
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
            "name": "BOARD_SYS_LED",
            "led_type": "SYS_LED",
            "led": {"loc": "/dev/cpld1", "offset": 0x21, "len": 1, "way": "devfile"},
            "led_attrs": {
                "green": 0x04, "red": 0x02, "amber": 0x06, "default": 0x04,
                "flash": 0xff, "light": 0xff, "off": 0, "mask": 0xff
            },
        },
        {
            "name": "BOARD_PSU_LED",
            "led_type": "PSU_LED",
            "led": {"loc": "/dev/cpld1", "offset": 0x22, "len": 1, "way": "devfile"},
            "led_attrs": {
                "green": 0x04, "red": 0x02, "amber": 0x06, "default": 0x04,
                "flash": 0xff, "light": 0xff, "off": 0, "mask": 0xff
            },
        },
        {
            "name": "BOARD_FAN_LED",
            "led_type": "FAN_LED",
            "led": {"loc": "/dev/cpld1", "offset": 0x23, "len": 1, "way": "devfile"},
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
            "e2loc": {"loc": "/sys/bus/i2c/devices/63-0050/eeprom", "way": "sysfs"},
            "present": {"loc": "/sys/wb_plat/fan/fan1/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 14, "addr": 0x0d, "offset": 0x3b, "way": "i2c"},
            "led_attrs": {
                "green": 0x04, "red": 0x02, "amber": 0x06, "default": 0x04,
                "flash": 0xff, "light": 0xff, "off": 0xff
            },
            "Rotor": {
                "Rotor1_config": {"name": "Rotor1",
                                          "SpeedMin": threshold.FAN_SPEED_MIN,
                                  "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                                  "Set_speed": {"bus": 14, "addr": 0x0d, "offset": 0x14, "way": "i2c"},
                                  "Running": {"loc": "/sys/wb_plat/fan/fan1/motor0/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                                  "HwAlarm": {"loc": "/sys/wb_plat/fan/fan1/motor0/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                                  "Speed": {
                                      "value": {"loc": "/sys/wb_plat/fan/fan1/motor0/speed", "way": "sysfs"},
                                      "Min": threshold.FAN_SPEED_MIN,
                                      "Max": threshold.FRONT_FAN_SPEED_MAX,
                                      "Unit": Unit.Speed,
                                  },
                                  },
                "Rotor2_config": {"name": "Rotor2",
                                          "SpeedMin": threshold.FAN_SPEED_MIN,
                                          "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                                  "Set_speed": {"bus": 14, "addr": 0x0d, "offset": 0x14, "way": "i2c"},
                                  "Running": {"loc": "/sys/wb_plat/fan/fan1/motor1/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                                  "HwAlarm": {"loc": "/sys/wb_plat/fan/fan1/motor1/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
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
            "e2loc": {"loc": "/sys/bus/i2c/devices/55-0050/eeprom", "way": "sysfs"},
            "present": {"loc": "/sys/wb_plat/fan/fan2/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 13, "addr": 0x0d, "offset": 0x3b, "way": "i2c"},
            "led_attrs": {
                "green": 0x04, "red": 0x02, "amber": 0x06, "default": 0x04,
                "flash": 0xff, "light": 0xff, "off": 0xff
            },
            "Rotor": {
                "Rotor1_config": {"name": "Rotor1",
                                          "SpeedMin": threshold.FAN_SPEED_MIN,
                                  "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                                  "Set_speed": {"bus": 13, "addr": 0x0d, "offset": 0x14, "way": "i2c"},
                                  "Running": {"loc": "/sys/wb_plat/fan/fan2/motor0/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                                  "HwAlarm": {"loc": "/sys/wb_plat/fan/fan2/motor0/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                                  "Speed": {
                                      "value": {"loc": "/sys/wb_plat/fan/fan2/motor0/speed", "way": "sysfs"},
                                      "Min": threshold.FAN_SPEED_MIN,
                                      "Max": threshold.FRONT_FAN_SPEED_MAX,
                                      "Unit": Unit.Speed,
                                  },
                                  },
                "Rotor2_config": {"name": "Rotor2",
                                          "SpeedMin": threshold.FAN_SPEED_MIN,
                                          "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                                  "Set_speed": {"bus": 13, "addr": 0x0d, "offset": 0x14, "way": "i2c"},
                                  "Running": {"loc": "/sys/wb_plat/fan/fan2/motor1/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                                  "HwAlarm": {"loc": "/sys/wb_plat/fan/fan2/motor1/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
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
            "e2loc": {"loc": "/sys/bus/i2c/devices/64-0050/eeprom", "way": "sysfs"},
            "present": {"loc": "/sys/wb_plat/fan/fan3/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 14, "addr": 0x0d, "offset": 0x3c, "way": "i2c"},
            "led_attrs": {
                "green": 0x04, "red": 0x02, "amber": 0x06, "default": 0x04,
                "flash": 0xff, "light": 0xff, "off": 0xff
            },
            "Rotor": {
                "Rotor1_config": {"name": "Rotor1",
                                          "SpeedMin": threshold.FAN_SPEED_MIN,
                                  "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                                  "Set_speed": {"bus": 14, "addr": 0x0d, "offset": 0x15, "way": "i2c"},
                                  "Running": {"loc": "/sys/wb_plat/fan/fan3/motor0/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                                  "HwAlarm": {"loc": "/sys/wb_plat/fan/fan3/motor0/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                                  "Speed": {
                                      "value": {"loc": "/sys/wb_plat/fan/fan3/motor0/speed", "way": "sysfs"},
                                      "Min": threshold.FAN_SPEED_MIN,
                                      "Max": threshold.FRONT_FAN_SPEED_MAX,
                                      "Unit": Unit.Speed,
                                  },
                                  },
                "Rotor2_config": {"name": "Rotor2",
                                          "SpeedMin": threshold.FAN_SPEED_MIN,
                                          "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                                  "Set_speed": {"bus": 14, "addr": 0x0d, "offset": 0x15, "way": "i2c"},
                                  "Running": {"loc": "/sys/wb_plat/fan/fan3/motor1/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                                  "HwAlarm": {"loc": "/sys/wb_plat/fan/fan3/motor1/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
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
            "e2loc": {"loc": "/sys/bus/i2c/devices/56-0050/eeprom", "way": "sysfs"},
            "present": {"loc": "/sys/wb_plat/fan/fan4/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 13, "addr": 0x0d, "offset": 0x3c, "way": "i2c"},
            "led_attrs": {
                "green": 0x04, "red": 0x02, "amber": 0x06, "default": 0x04,
                "flash": 0xff, "light": 0xff, "off": 0xff
            },
            "Rotor": {
                "Rotor1_config": {"name": "Rotor1",
                                          "SpeedMin": threshold.FAN_SPEED_MIN,
                                  "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                                  "Set_speed": {"bus": 13, "addr": 0x0d, "offset": 0x15, "way": "i2c"},
                                  "Running": {"loc": "/sys/wb_plat/fan/fan4/motor0/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                                  "HwAlarm": {"loc": "/sys/wb_plat/fan/fan4/motor0/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                                  "Speed": {
                                      "value": {"loc": "/sys/wb_plat/fan/fan4/motor0/speed", "way": "sysfs"},
                                      "Min": threshold.FAN_SPEED_MIN,
                                      "Max": threshold.FRONT_FAN_SPEED_MAX,
                                      "Unit": Unit.Speed,
                                  },
                                  },
                "Rotor2_config": {"name": "Rotor2",
                                          "SpeedMin": threshold.FAN_SPEED_MIN,
                                          "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                                  "Set_speed": {"bus": 13, "addr": 0x0d, "offset": 0x15, "way": "i2c"},
                                  "Running": {"loc": "/sys/wb_plat/fan/fan4/motor1/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                                  "HwAlarm": {"loc": "/sys/wb_plat/fan/fan4/motor1/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
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
            "e2loc": {"loc": "/sys/bus/i2c/devices/65-0050/eeprom", "way": "sysfs"},
            "present": {"loc": "/sys/wb_plat/fan/fan5/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 14, "addr": 0x0d, "offset": 0x3d, "way": "i2c"},
            "led_attrs": {
                "green": 0x04, "red": 0x02, "amber": 0x06, "default": 0x04,
                "flash": 0xff, "light": 0xff, "off": 0xff
            },
            "Rotor": {
                "Rotor1_config": {"name": "Rotor1",
                                          "SpeedMin": threshold.FAN_SPEED_MIN,
                                  "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                                  "Set_speed": {"bus": 14, "addr": 0x0d, "offset": 0x16, "way": "i2c"},
                                  "Running": {"loc": "/sys/wb_plat/fan/fan5/motor0/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                                  "HwAlarm": {"loc": "/sys/wb_plat/fan/fan5/motor0/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                                  "Speed": {
                                      "value": {"loc": "/sys/wb_plat/fan/fan5/motor0/speed", "way": "sysfs"},
                                      "Min": threshold.FAN_SPEED_MIN,
                                      "Max": threshold.FRONT_FAN_SPEED_MAX,
                                      "Unit": Unit.Speed,
                                  },
                                  },
                "Rotor2_config": {"name": "Rotor2",
                                          "SpeedMin": threshold.FAN_SPEED_MIN,
                                          "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                                  "Set_speed": {"bus": 14, "addr": 0x0d, "offset": 0x16, "way": "i2c"},
                                  "Running": {"loc": "/sys/wb_plat/fan/fan5/motor1/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                                  "HwAlarm": {"loc": "/sys/wb_plat/fan/fan5/motor1/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
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
            "e2loc": {"loc": "/sys/bus/i2c/devices/57-0050/eeprom", "way": "sysfs"},
            "present": {"loc": "/sys/wb_plat/fan/fan6/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 13, "addr": 0x0d, "offset": 0x3d, "way": "i2c"},
            "led_attrs": {
                "green": 0x04, "red": 0x02, "amber": 0x06, "default": 0x04,
                "flash": 0xff, "light": 0xff, "off": 0xff
            },
            "Rotor": {
                "Rotor1_config": {"name": "Rotor1",
                                          "SpeedMin": threshold.FAN_SPEED_MIN,
                                  "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                                  "Set_speed": {"bus": 13, "addr": 0x0d, "offset": 0x16, "way": "i2c"},
                                  "Running": {"loc": "/sys/wb_plat/fan/fan6/motor0/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                                  "HwAlarm": {"loc": "/sys/wb_plat/fan/fan6/motor0/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                                  "Speed": {
                                      "value": {"loc": "/sys/wb_plat/fan/fan6/motor0/speed", "way": "sysfs"},
                                      "Min": threshold.FAN_SPEED_MIN,
                                      "Max": threshold.FRONT_FAN_SPEED_MAX,
                                      "Unit": Unit.Speed,
                                  },
                                  },
                "Rotor2_config": {"name": "Rotor2",
                                          "SpeedMin": threshold.FAN_SPEED_MIN,
                                          "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                                  "Set_speed": {"bus": 13, "addr": 0x0d, "offset": 0x16, "way": "i2c"},
                                  "Running": {"loc": "/sys/wb_plat/fan/fan6/motor1/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                                  "HwAlarm": {"loc": "/sys/wb_plat/fan/fan6/motor1/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                                  "Speed": {
                                      "value": {"loc": "/sys/wb_plat/fan/fan6/motor1/speed", "way": "sysfs"},
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
        },
        {
            "name": "MAC_CPLD_A",
            "cpld_id": "CPLD2",
            "VersionFile": {"loc": "/dev/cpld1", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for base functions",
            "slot": 0,
        },
        {
            "name": "MAC_CPLD_B",
            "cpld_id": "CPLD3",
            "VersionFile": {"loc": "/dev/cpld2", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for base functions",
            "slot": 0,
        },
        {
            "name": "FAN_CPLD_A",
            "cpld_id": "CPLD4",
            "VersionFile": {"loc": "/dev/cpld3", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for system LEDs and FANs",
            "slot": 0,
        },
        {
            "name": "FAN_CPLD_B",
            "cpld_id": "CPLD5",
            "VersionFile": {"loc": "/dev/cpld4", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for system LEDs and FANs",
            "slot": 0,
        },
        {
            "name": "LC1_CPLD_2",
            "cpld_id": "CPLD6",
            "VersionFile": {"loc": "/dev/cpld5", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for SFP+ modules",
            "slot": 1,
        },
        {
            "name": "LC1_CPLD_1",
            "cpld_id": "CPLD7",
            "VersionFile": {"loc": "/dev/cpld6", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for SFP+ modules",
            "slot": 1,
        },
        {
            "name": "LC2_CPLD_2",
            "cpld_id": "CPLD8",
            "VersionFile": {"loc": "/dev/cpld7", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for SFP+ modules",
            "slot": 2,
        },
        {
            "name": "LC2_CPLD_1",
            "cpld_id": "CPLD9",
            "VersionFile": {"loc": "/dev/cpld8", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for SFP+ modules",
            "slot": 2,
        },
        {
            "name": "LC3_CPLD_2",
            "cpld_id": "CPLD10",
            "VersionFile": {"loc": "/dev/cpld9", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for SFP+ modules",
            "slot": 3,
        },
        {
            "name": "LC3_CPLD_1",
            "cpld_id": "CPLD11",
            "VersionFile": {"loc": "/dev/cpld10", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for SFP+ modules",
            "slot": 3,
        },
        {
            "name": "LC4_CPLD_2",
            "cpld_id": "CPLD12",
            "VersionFile": {"loc": "/dev/cpld11", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for SFP+ modules",
            "slot": 4,
        },
        {
            "name": "LC4_CPLD_1",
            "cpld_id": "CPLD13",
            "VersionFile": {"loc": "/dev/cpld12", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for SFP+ modules",
            "slot": 4,
        },
    ],
    "dcdc": [
        {
            "name": "X86_BOARD_P5V_AUX_IN",
            "dcdc_id": "DCDC1",
            "Min": 4721,
            "value": {
                "loc": "/sys/wb_plat/sensor/in1/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 5250,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "X86_BOARD_P1V7_IN",
            "dcdc_id": "DCDC2",
            "Min": 1615,
            "value": {
                "loc": "/sys/wb_plat/sensor/in2/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1792,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "X86_BOARD_TEST1v24",
            "dcdc_id": "DCDC3",
            "Min": 1178,
            "value": {
                "loc": "/sys/wb_plat/sensor/in3/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1302,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "X86_BOARD_P3V3_STBY_IN",
            "dcdc_id": "DCDC4",
            "Min": 3135,
            "value": {
                "loc": "/sys/wb_plat/sensor/in4/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3500,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_BOARD_VDD1v8",
            "dcdc_id": "DCDC5",
            "Min": 1710,
            "value": {
                "loc": "/sys/wb_plat/sensor/in5/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1909,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_BOARD_SW_VDD1v2",
            "dcdc_id": "DCDC6",
            "Min": 1140,
            "value": {
                "loc": "/sys/wb_plat/sensor/in6/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1286,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_BOARD_TEST1v24",
            "dcdc_id": "DCDC7",
            "Min": 1178,
            "value": {
                "loc": "/sys/wb_plat/sensor/in7/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1302,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_BOARD_VDD1v2_MAC",
            "dcdc_id": "DCDC8",
            "Min": 1140,
            "value": {
                "loc": "/sys/wb_plat/sensor/in8/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1260,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_BOARD_VDD_CORE",
            "dcdc_id": "DCDC9",
            "Min": 712,
            "value": {
                "loc": "/sys/wb_plat/sensor/in9/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 945,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_BOARD_VDD3v3_MCU",
            "dcdc_id": "DCDC10",
            "Min": 3135,
            "value": {
                "loc": "/sys/wb_plat/sensor/in10/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3556,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_BOARD_VDD3v3_CLK",
            "dcdc_id": "DCDC11",
            "Min": 3135,
            "value": {
                "loc": "/sys/wb_plat/sensor/in11/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3556,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_BOARD_VDD5V_CLK_MCU",
            "dcdc_id": "DCDC12",
            "Min": 4750,
            "value": {
                "loc": "/sys/wb_plat/sensor/in12/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 5331,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_BOARD_VDD2v5",
            "dcdc_id": "DCDC13",
            "Min": 2375,
            "value": {
                "loc": "/sys/wb_plat/sensor/in13/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 2688,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_BOARD_VDDO",
            "dcdc_id": "DCDC14",
            "Min": 3135,
            "value": {
                "loc": "/sys/wb_plat/sensor/in14/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3542,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_BOARD_BC_PVDD",
            "dcdc_id": "DCDC15",
            "Min": 760,
            "value": {
                "loc": "/sys/wb_plat/sensor/in15/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 867,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_BOARD_VDD3v3",
            "dcdc_id": "DCDC16",
            "Min": 3135,
            "value": {
                "loc": "/sys/wb_plat/sensor/in16/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3542,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_BOARD_ANLGOUT",
            "dcdc_id": "DCDC17",
            "Min": 760,
            "value": {
                "loc": "/sys/wb_plat/sensor/in17/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 856,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT1_VDD3v3_QSFP_1",
            "dcdc_id": "DCDC18",
            "Min": 3135,
            "value": {
                "loc": "/sys/wb_plat/slot/slot1/in1/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3500,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT1_VDD3v3_QSFP_2",
            "dcdc_id": "DCDC19",
            "Min": 3135,
            "value": {
                "loc": "/sys/wb_plat/slot/slot1/in2/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3500,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT1_VDD1v8",
            "dcdc_id": "DCDC20",
            "Min": 1710,
            "value": {
                "loc": "/sys/wb_plat/slot/slot1/in3/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1926,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT1_TEST1v24",
            "dcdc_id": "DCDC21",
            "Min": 1178,
            "value": {
                "loc": "/sys/wb_plat/slot/slot1/in4/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1302,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT1_VDD3v3_CLK",
            "dcdc_id": "DCDC22",
            "Min": 3135,
            "value": {
                "loc": "/sys/wb_plat/slot/slot1/in5/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3556,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT1_VDD5v0",
            "dcdc_id": "DCDC23",
            "Min": 4750,
            "value": {
                "loc": "/sys/wb_plat/slot/slot1/in6/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 5331,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT1_VDD3v7_CLK_MCU",
            "dcdc_id": "DCDC24",
            "Min": 3639,
            "value": {
                "loc": "/sys/wb_plat/slot/slot1/in7/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 4022,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT1_VDD1v2",
            "dcdc_id": "DCDC25",
            "Min": 1140,
            "value": {
                "loc": "/sys/wb_plat/slot/slot1/in8/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1260,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT1_VDD3v3",
            "dcdc_id": "DCDC26",
            "Min": 3135,
            "value": {
                "loc": "/sys/wb_plat/slot/slot1/in9/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3539,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT1_DVDD_0V8",
            "dcdc_id": "DCDC27",
            "Min": 684,
            "value": {
                "loc": "/sys/wb_plat/slot/slot1/in10/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 772,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT1_VDD1V8_CPLD",
            "dcdc_id": "DCDC28",
            "Min": 1710,
            "value": {
                "loc": "/sys/wb_plat/slot/slot1/in11/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1909,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT1_AVDD0V8",
            "dcdc_id": "DCDC29",
            "Min": 776,
            "value": {
                "loc": "/sys/wb_plat/slot/slot1/in12/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 877,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT2_VDD3v3_QSFP_1",
            "dcdc_id": "DCDC30",
            "Min": 3135,
            "value": {
                "loc": "/sys/wb_plat/slot/slot2/in1/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3500,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT2_VDD3v3_QSFP_2",
            "dcdc_id": "DCDC31",
            "Min": 3135,
            "value": {
                "loc": "/sys/wb_plat/slot/slot2/in2/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3500,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT2_VDD1v8",
            "dcdc_id": "DCDC32",
            "Min": 1710,
            "value": {
                "loc": "/sys/wb_plat/slot/slot2/in3/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1926,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT2_TEST1v24",
            "dcdc_id": "DCDC33",
            "Min": 1178,
            "value": {
                "loc": "/sys/wb_plat/slot/slot2/in4/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1302,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT2_VDD3v3_CLK",
            "dcdc_id": "DCDC34",
            "Min": 3135,
            "value": {
                "loc": "/sys/wb_plat/slot/slot2/in5/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3556,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT2_VDD5v0",
            "dcdc_id": "DCDC35",
            "Min": 4750,
            "value": {
                "loc": "/sys/wb_plat/slot/slot2/in6/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 5331,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT2_VDD3v7_CLK_MCU",
            "dcdc_id": "DCDC36",
            "Min": 3639,
            "value": {
                "loc": "/sys/wb_plat/slot/slot2/in7/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 4022,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT2_VDD1v2",
            "dcdc_id": "DCDC37",
            "Min": 1140,
            "value": {
                "loc": "/sys/wb_plat/slot/slot2/in8/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1260,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT2_VDD3v3",
            "dcdc_id": "DCDC38",
            "Min": 3135,
            "value": {
                "loc": "/sys/wb_plat/slot/slot2/in9/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3539,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT2_DVDD_0V8",
            "dcdc_id": "DCDC39",
            "Min": 684,
            "value": {
                "loc": "/sys/wb_plat/slot/slot2/in10/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 772,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT2_VDD1V8_CPLD",
            "dcdc_id": "DCDC40",
            "Min": 1710,
            "value": {
                "loc": "/sys/wb_plat/slot/slot2/in11/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1909,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT2_AVDD0V8",
            "dcdc_id": "DCDC41",
            "Min": 776,
            "value": {
                "loc": "/sys/wb_plat/slot/slot2/in12/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 877,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT3_VDD3v3_QSFP_1",
            "dcdc_id": "DCDC42",
            "Min": 3135,
            "value": {
                "loc": "/sys/wb_plat/slot/slot3/in1/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3500,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT3_VDD3v3_QSFP_2",
            "dcdc_id": "DCDC43",
            "Min": 3135,
            "value": {
                "loc": "/sys/wb_plat/slot/slot3/in2/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3500,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT3_VDD1v8",
            "dcdc_id": "DCDC44",
            "Min": 1710,
            "value": {
                "loc": "/sys/wb_plat/slot/slot3/in3/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1926,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT3_TEST1v24",
            "dcdc_id": "DCDC45",
            "Min": 1178,
            "value": {
                "loc": "/sys/wb_plat/slot/slot3/in4/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1302,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT3_VDD3v3_CLK",
            "dcdc_id": "DCDC46",
            "Min": 3135,
            "value": {
                "loc": "/sys/wb_plat/slot/slot3/in5/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3556,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT3_VDD5v0",
            "dcdc_id": "DCDC47",
            "Min": 4750,
            "value": {
                "loc": "/sys/wb_plat/slot/slot3/in6/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 5331,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT3_VDD3v7_CLK_MCU",
            "dcdc_id": "DCDC48",
            "Min": 3639,
            "value": {
                "loc": "/sys/wb_plat/slot/slot3/in7/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 4022,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT3_VDD1v2",
            "dcdc_id": "DCDC49",
            "Min": 1140,
            "value": {
                "loc": "/sys/wb_plat/slot/slot3/in8/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1260,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT3_VDD3v3",
            "dcdc_id": "DCDC50",
            "Min": 3135,
            "value": {
                "loc": "/sys/wb_plat/slot/slot3/in9/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3539,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT3_DVDD_0V8",
            "dcdc_id": "DCDC51",
            "Min": 684,
            "value": {
                "loc": "/sys/wb_plat/slot/slot3/in10/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 772,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT3_VDD1V8_CPLD",
            "dcdc_id": "DCDC52",
            "Min": 1710,
            "value": {
                "loc": "/sys/wb_plat/slot/slot3/in11/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1909,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT3_AVDD0V8",
            "dcdc_id": "DCDC53",
            "Min": 776,
            "value": {
                "loc": "/sys/wb_plat/slot/slot3/in12/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 877,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT4_VDD3v3_QSFP_1",
            "dcdc_id": "DCDC54",
            "Min": 3135,
            "value": {
                "loc": "/sys/wb_plat/slot/slot4/in1/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3500,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT4_VDD3v3_QSFP_2",
            "dcdc_id": "DCDC55",
            "Min": 3135,
            "value": {
                "loc": "/sys/wb_plat/slot/slot4/in2/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3500,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT4_VDD1v8",
            "dcdc_id": "DCDC56",
            "Min": 1710,
            "value": {
                "loc": "/sys/wb_plat/slot/slot4/in3/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1926,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT4_TEST1v24",
            "dcdc_id": "DCDC57",
            "Min": 1178,
            "value": {
                "loc": "/sys/wb_plat/slot/slot4/in4/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1302,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT4_VDD3v3_CLK",
            "dcdc_id": "DCDC58",
            "Min": 3135,
            "value": {
                "loc": "/sys/wb_plat/slot/slot4/in5/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3556,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT4_VDD5v0",
            "dcdc_id": "DCDC59",
            "Min": 4750,
            "value": {
                "loc": "/sys/wb_plat/slot/slot4/in6/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 5331,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT4_VDD3v7_CLK_MCU",
            "dcdc_id": "DCDC60",
            "Min": 3639,
            "value": {
                "loc": "/sys/wb_plat/slot/slot4/in7/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 4022,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT4_VDD1v2",
            "dcdc_id": "DCDC61",
            "Min": 1140,
            "value": {
                "loc": "/sys/wb_plat/slot/slot4/in8/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1260,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT4_VDD3v3",
            "dcdc_id": "DCDC62",
            "Min": 3135,
            "value": {
                "loc": "/sys/wb_plat/slot/slot4/in9/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3539,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT4_DVDD_0V8",
            "dcdc_id": "DCDC63",
            "Min": 684,
            "value": {
                "loc": "/sys/wb_plat/slot/slot4/in10/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 772,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT4_VDD1V8_CPLD",
            "dcdc_id": "DCDC64",
            "Min": 1710,
            "value": {
                "loc": "/sys/wb_plat/slot/slot4/in11/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1909,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SLOT4_AVDD0V8",
            "dcdc_id": "DCDC65",
            "Min": 776,
            "value": {
                "loc": "/sys/wb_plat/slot/slot4/in12/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 877,
            "format": "float(float(%s)/1000)",
        },
    ],
    "cpu": [
        {
            "name": "cpu",
            "CpuResetCntReg": {"io_addr": 0xb88, "way": "io"},
            "reboot_cause_path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"
        }
    ],
    "sfps": {
        "ver": '1.0',
        "port_index_start": 0,
        "port_num": 128,
        "log_level": 2,
        "eeprom_retry_times": 5,
        "eeprom_retry_break_sec": 0.2,
        "presence_cpld": {
            "dev_id": {
                6: {
                    "offset": {
                        0x10: "1-8",
                        0x11: "9-16",
                    },
                },
                5: {
                    "offset": {
                        0x10: "17-24",
                        0x11: "25-32",
                    },
                },
                8: {
                    "offset": {
                        0x10: "33-40",
                        0x11: "41-48",
                    },
                },
                7: {
                    "offset": {
                        0x10: "49-56",
                        0x11: "57-64",
                    },
                },
                10: {
                    "offset": {
                        0x10: "65-72",
                        0x11: "73-80",
                    },
                },
                9: {
                    "offset": {
                        0x10: "81-88",
                        0x11: "89-96",
                    },
                },
                12: {
                    "offset": {
                        0x10: "97-104",
                        0x11: "105-112",
                    },
                },
                11: {
                    "offset": {
                        0x10: "113-120",
                        0x11: "121-128",
                    },
                },
            },
        },
        "presence_val_is_present": 0,
        "eeprom_path": "/sys/bus/i2c/devices/i2c-%d/%d-0050/eeprom",
        "eeprom_path_key": list(range(71, 199)),
        "optoe_driver_path": "/sys/bus/i2c/devices/i2c-%d/%d-0050/dev_class",
        "optoe_driver_key": list(range(71, 199)),
        "reset_cpld": {
            "dev_id": {
                6: {
                    "offset": {
                        0x14: "1-8",
                        0x15: "9-16",
                    },
                },
                5: {
                    "offset": {
                        0x14: "17-24",
                        0x15: "25-32",
                    },
                },
                8: {
                    "offset": {
                        0x14: "33-40",
                        0x15: "41-48",
                    },
                },
                7: {
                    "offset": {
                        0x14: "49-56",
                        0x15: "57-64",
                    },
                },
                10: {
                    "offset": {
                        0x14: "65-72",
                        0x15: "73-80",
                    },
                },
                9: {
                    "offset": {
                        0x14: "81-88",
                        0x15: "89-96",
                    },
                },
                12: {
                    "offset": {
                        0x14: "97-104",
                        0x15: "105-112",
                    },
                },
                11: {
                    "offset": {
                        0x14: "113-120",
                        0x15: "121-128",
                    },
                },
            }
        },
        "reset_val_is_reset": 0,
    }
}
