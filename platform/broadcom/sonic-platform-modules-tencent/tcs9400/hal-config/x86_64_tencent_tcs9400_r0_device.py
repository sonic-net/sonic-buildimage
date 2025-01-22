#!/usr/bin/python3

psu_fan_airflow = {
    "F2B": ["DPS-1300AB-6 F", "GW-CRPS1300D"],
    "B2F": ["DPS-1300AB-11 C", "CRPS1300D3R"]
}

psu_display_name = {
    "PSA1300CRPS-F": ["DPS-1300AB-6 F", "GW-CRPS1300D"],
    "PSA1300CRPS-R": ["DPS-1300AB-11 C", "CRPS1300D3R"]
}

fanairflow = {
    "F2B": ["FAN12K8080-F"],
}

fan_display_name = {
    "FAN12K8080-F": ["FAN12K8080-F"],
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


PSU_NOT_PRESENT_PWM = 100


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

    FRONT_FAN_SPEED_MAX = 13530
    REAR_FAN_SPEED_MAX = 11770
    FAN_SPEED_MIN = 1650


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
            "e2loc": {"loc": "/sys/bus/i2c/devices/79-0050/eeprom", "way": "sysfs"},
            "pmbusloc": {"bus": 79, "addr": 0x58, "way": "i2c"},
            "present": {"loc": "/sys/rg_plat/psu/psu1/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "name": "PSU1",
            "psu_display_name": psu_display_name,
            "airflow": psu_fan_airflow,
            "psu_not_present_pwm": PSU_NOT_PRESENT_PWM,
            "TempStatus": {"bus": 79, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0004},
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-79/79-0058/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": threshold.PSU_TEMP_MIN,
                "Max": threshold.PSU_TEMP_MAX,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            },
            "FanStatus": {"bus": 79, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0400},
            "FanSpeed": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-79/79-0058/hwmon/hwmon*/fan1_input", "way": "sysfs"},
                "Min": threshold.PSU_FAN_SPEED_MIN,
                "Max": threshold.PSU_FAN_SPEED_MAX,
                "Unit": Unit.Speed
            },
            "InputsStatus": {"bus": 79, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x2000},
            "InputsType": {"bus": 79, "addr": 0x58, "offset": 0x80, "way": "i2c", 'psutypedecode': psutypedecode},
            "InputsVoltage": {
                'AC': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-79/79-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_AC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_AC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"

                },
                'DC': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-79/79-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_DC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_DC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                },
                'other': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-79/79-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.ERR_VALUE,
                    "Max": threshold.ERR_VALUE,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                }
            },
            "InputsCurrent":
            {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-79/79-0058/hwmon/hwmon*/curr1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_CURRENT_MIN,
                "Max": threshold.PSU_INPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "InputsPower":
            {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-79/79-0058/hwmon/hwmon*/power1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_POWER_MIN,
                "Max": threshold.PSU_INPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
            "OutputsStatus": {"bus": 79, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x8800},
            "OutputsVoltage":
            {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-79/79-0058/hwmon/hwmon*/in2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_VOLTAGE_MIN,
                "Max": threshold.PSU_OUTPUT_VOLTAGE_MAX,
                "Unit": Unit.Voltage,
                "format": "float(float(%s)/1000)"
            },
            "OutputsCurrent":
            {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-79/79-0058/hwmon/hwmon*/curr2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_CURRENT_MIN,
                "Max": threshold.PSU_OUTPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "OutputsPower":
            {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-79/79-0058/hwmon/hwmon*/power2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_POWER_MIN,
                "Max": threshold.PSU_OUTPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
        },
        {
            "e2loc": {"loc": "/sys/bus/i2c/devices/80-0050/eeprom", "way": "sysfs"},
            "pmbusloc": {"bus": 80, "addr": 0x58, "way": "i2c"},
            "present": {"loc": "/sys/rg_plat/psu/psu2/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "name": "PSU2",
            "psu_display_name": psu_display_name,
            "airflow": psu_fan_airflow,
            "psu_not_present_pwm": PSU_NOT_PRESENT_PWM,
            "TempStatus": {"bus": 80, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0004},
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-80/80-0058/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": threshold.PSU_TEMP_MIN,
                "Max": threshold.PSU_TEMP_MAX,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            },
            "FanStatus": {"bus": 80, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0400},
            "FanSpeed": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-80/80-0058/hwmon/hwmon*/fan1_input", "way": "sysfs"},
                "Min": threshold.PSU_FAN_SPEED_MIN,
                "Max": threshold.PSU_FAN_SPEED_MAX,
                "Unit": Unit.Speed
            },
            "InputsStatus": {"bus": 80, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x2000},
            "InputsType": {"bus": 80, "addr": 0x58, "offset": 0x80, "way": "i2c", 'psutypedecode': psutypedecode},
            "InputsVoltage": {
                'AC': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-80/80-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_AC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_AC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"

                },
                'DC': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-80/80-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_DC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_DC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                },
                'other': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-80/80-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.ERR_VALUE,
                    "Max": threshold.ERR_VALUE,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                }
            },
            "InputsCurrent":
            {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-80/80-0058/hwmon/hwmon*/curr1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_CURRENT_MIN,
                "Max": threshold.PSU_INPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "InputsPower":
            {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-80/80-0058/hwmon/hwmon*/power1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_POWER_MIN,
                "Max": threshold.PSU_INPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
            "OutputsStatus": {"bus": 80, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x8800},
            "OutputsVoltage":
            {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-80/80-0058/hwmon/hwmon*/in2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_VOLTAGE_MIN,
                "Max": threshold.PSU_OUTPUT_VOLTAGE_MAX,
                "Unit": Unit.Voltage,
                "format": "float(float(%s)/1000)"
            },
            "OutputsCurrent":
            {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-80/80-0058/hwmon/hwmon*/curr2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_CURRENT_MIN,
                "Max": threshold.PSU_OUTPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "OutputsPower":
            {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-80/80-0058/hwmon/hwmon*/power2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_POWER_MIN,
                "Max": threshold.PSU_OUTPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
        },
        {
            "e2loc": {"loc": "/sys/bus/i2c/devices/82-0050/eeprom", "way": "sysfs"},
            "pmbusloc": {"bus": 82, "addr": 0x58, "way": "i2c"},
            "present": {"loc": "/sys/rg_plat/psu/psu3/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "name": "PSU3",
            "psu_display_name": psu_display_name,
            "airflow": psu_fan_airflow,
            "psu_not_present_pwm": PSU_NOT_PRESENT_PWM,
            "TempStatus": {"bus": 82, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0004},
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-82/82-0058/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": threshold.PSU_TEMP_MIN,
                "Max": threshold.PSU_TEMP_MAX,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            },
            "FanStatus": {"bus": 82, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0400},
            "FanSpeed": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-82/82-0058/hwmon/hwmon*/fan1_input", "way": "sysfs"},
                "Min": threshold.PSU_FAN_SPEED_MIN,
                "Max": threshold.PSU_FAN_SPEED_MAX,
                "Unit": Unit.Speed
            },
            "InputsStatus": {"bus": 82, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x2000},
            "InputsType": {"bus": 82, "addr": 0x58, "offset": 0x80, "way": "i2c", 'psutypedecode': psutypedecode},
            "InputsVoltage": {
                'AC': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-82/82-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_AC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_AC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"

                },
                'DC': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-82/82-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_DC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_DC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                },
                'other': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-82/82-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.ERR_VALUE,
                    "Max": threshold.ERR_VALUE,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                }
            },
            "InputsCurrent":
            {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-82/82-0058/hwmon/hwmon*/curr1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_CURRENT_MIN,
                "Max": threshold.PSU_INPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "InputsPower":
            {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-82/82-0058/hwmon/hwmon*/power1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_POWER_MIN,
                "Max": threshold.PSU_INPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
            "OutputsStatus": {"bus": 82, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x8800},
            "OutputsVoltage":
            {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-82/82-0058/hwmon/hwmon*/in2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_VOLTAGE_MIN,
                "Max": threshold.PSU_OUTPUT_VOLTAGE_MAX,
                "Unit": Unit.Voltage,
                "format": "float(float(%s)/1000)"
            },
            "OutputsCurrent":
            {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-82/82-0058/hwmon/hwmon*/curr2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_CURRENT_MIN,
                "Max": threshold.PSU_OUTPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "OutputsPower":
            {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-82/82-0058/hwmon/hwmon*/power2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_POWER_MIN,
                "Max": threshold.PSU_OUTPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
        },
        {
            "e2loc": {"loc": "/sys/bus/i2c/devices/81-0050/eeprom", "way": "sysfs"},
            "pmbusloc": {"bus": 81, "addr": 0x58, "way": "i2c"},
            "present": {"loc": "/sys/rg_plat/psu/psu4/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "name": "PSU4",
            "psu_display_name": psu_display_name,
            "airflow": psu_fan_airflow,
            "psu_not_present_pwm": PSU_NOT_PRESENT_PWM,
            "TempStatus": {"bus": 81, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0004},
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-81/81-0058/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": threshold.PSU_TEMP_MIN,
                "Max": threshold.PSU_TEMP_MAX,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            },
            "FanStatus": {"bus": 81, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0400},
            "FanSpeed": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-81/81-0058/hwmon/hwmon*/fan1_input", "way": "sysfs"},
                "Min": threshold.PSU_FAN_SPEED_MIN,
                "Max": threshold.PSU_FAN_SPEED_MAX,
                "Unit": Unit.Speed
            },
            "InputsStatus": {"bus": 81, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x2000},
            "InputsType": {"bus": 81, "addr": 0x58, "offset": 0x80, "way": "i2c", 'psutypedecode': psutypedecode},
            "InputsVoltage": {
                'AC': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-81/81-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_AC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_AC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"

                },
                'DC': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-81/81-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_DC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_DC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                },
                'other': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-81/81-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.ERR_VALUE,
                    "Max": threshold.ERR_VALUE,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                }
            },
            "InputsCurrent":
            {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-81/81-0058/hwmon/hwmon*/curr1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_CURRENT_MIN,
                "Max": threshold.PSU_INPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "InputsPower":
            {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-81/81-0058/hwmon/hwmon*/power1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_POWER_MIN,
                "Max": threshold.PSU_INPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
            "OutputsStatus": {"bus": 81, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x8800},
            "OutputsVoltage":
            {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-81/81-0058/hwmon/hwmon*/in2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_VOLTAGE_MIN,
                "Max": threshold.PSU_OUTPUT_VOLTAGE_MAX,
                "Unit": Unit.Voltage,
                "format": "float(float(%s)/1000)"
            },
            "OutputsCurrent":
            {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-81/81-0058/hwmon/hwmon*/curr2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_CURRENT_MIN,
                "Max": threshold.PSU_OUTPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "OutputsPower":
            {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-81/81-0058/hwmon/hwmon*/power2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_POWER_MIN,
                "Max": threshold.PSU_OUTPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
        },
    ],
    "temps":[
        {
            "name": "CPU_TEMP",
            "temp_id": "TEMP1",
            "api_name": "CPU",
            "Temperature": {
                "value": {"loc": "/sys/bus/platform/devices/coretemp.0/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": 2000,
                "Low": 10000,
                "High": 100000,
                "Max": 102000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
        {
            "name": "INLET_TEMP",
            "temp_id": "TEMP2",
            "api_name": "Inlet",
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/107-004b/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": -10000,
                "Low": 0,
                "High": 40000,
                "Max": 50000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
        {
            "name": "OUTLET_TEMP",
            "temp_id": "TEMP3",
            "api_name": "Outlet",
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/70-004f/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": -10000,
                "Low": 0,
                "High": 70000,
                "Max": 75000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
        {
            "name": "SWITCH_TEMP",
            "temp_id": "TEMP4",
            "api_name": "Switch ASIC",
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/74-0044/hwmon/hwmon*/temp99_input", "way": "sysfs"},
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
            "name": "FRONT_SYS_LED",
            "led_type": "SYS_LED",
            "led": {"bus": 77, "addr": 0x1d, "offset": 0x08, "way": "i2c"},
            "led_attrs": {
                "off":0x00, "red_flash":0x01, "red":0x02,
                "green_flash":0x03, "green":0x04, "yellow_flash":0x05,
                "yellow":0x06, "mask": 0x07
            },
        },
        {
            "name": "FRONT_PSU_LED",
            "led_type": "PSU_LED",
            "led": {"bus": 77, "addr": 0x1d, "offset": 0x09, "way": "i2c"},
            "led_attrs": {
                "off":0x00, "red_flash":0x01, "red":0x02,
                "green_flash":0x03, "green":0x04, "yellow_flash":0x05,
                "yellow":0x06, "mask": 0x07
            },
        },
        {
            "name": "FRONT_FAN_LED",
            "led_type": "FAN_LED",
            "led": {"bus": 77, "addr": 0x1d, "offset": 0x0a, "way": "i2c"},
            "led_attrs": {
                "off":0x00, "red_flash":0x01, "red":0x02,
                "green_flash":0x03, "green":0x04, "yellow_flash":0x05,
                "yellow":0x06, "mask": 0x07
            },
        },
    ],
    "fans": [
        {
            "name": "FAN1",
            "airflow": fanairflow,
            "fan_display_name": fan_display_name,
            "e2loc": {'loc': '/sys/bus/i2c/devices/i2c-90/90-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "present": {"loc":"/sys/rg_plat/fan/fan1/present","way":"sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 87, "addr": 0x0d, "offset": 0x3b, "way": "i2c"},
            "led_attrs": {
                "off":0x00, "red_flash":0x01, "red":0x02,
                "green_flash":0x03, "green":0x04, "yellow_flash":0x05,
                "yellow":0x06, "mask": 0x07
            },
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"bus": 87, "addr": 0x0d, "offset": 0x14, "way": "i2c"},
                    "Running": {"loc":"/sys/rg_plat/fan/fan1/motor0/status","way":"sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc":"/sys/rg_plat/fan/fan1/motor0/status","way":"sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/rg_plat/fan/fan1/motor0/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FRONT_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
                "Rotor2_config": {
                    "name": "Rotor2",
                    "Set_speed": {"bus": 87, "addr": 0x0d, "offset": 0x14, "way": "i2c"},
                    "Running": {"loc":"/sys/rg_plat/fan/fan1/motor1/status","way":"sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc":"/sys/rg_plat/fan/fan1/motor1/status","way":"sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/rg_plat/fan/fan1/motor1/speed", "way": "sysfs"},
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
            "fan_display_name": fan_display_name,
            "e2loc": {'loc': '/sys/bus/i2c/devices/i2c-98/98-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "present": {"loc":"/sys/rg_plat/fan/fan2/present","way":"sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 95, "addr": 0x0d, "offset": 0x3b, "way": "i2c"},
            "led_attrs": {
                "off":0x00, "red_flash":0x01, "red":0x02,
                "green_flash":0x03, "green":0x04, "yellow_flash":0x05,
                "yellow":0x06, "mask": 0x07
            },
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"bus": 95, "addr": 0x0d, "offset": 0x14, "way": "i2c"},
                    "Running": {"loc":"/sys/rg_plat/fan/fan2/motor0/status","way":"sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc":"/sys/rg_plat/fan/fan2/motor0/status","way":"sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/rg_plat/fan/fan2/motor0/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FRONT_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
                "Rotor2_config": {
                    "name": "Rotor2",
                    "Set_speed": {"bus": 95, "addr": 0x0d, "offset": 0x14, "way": "i2c"},
                    "Running": {"loc":"/sys/rg_plat/fan/fan2/motor1/status","way":"sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc":"/sys/rg_plat/fan/fan2/motor1/status","way":"sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/rg_plat/fan/fan2/motor1/speed", "way": "sysfs"},
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
            "fan_display_name": fan_display_name,
            "e2loc": {'loc': '/sys/bus/i2c/devices/i2c-91/91-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "present": {"loc":"/sys/rg_plat/fan/fan3/present","way":"sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 87, "addr": 0x0d, "offset": 0x3c, "way": "i2c"},
            "led_attrs": {
                "off":0x00, "red_flash":0x01, "red":0x02,
                "green_flash":0x03, "green":0x04, "yellow_flash":0x05,
                "yellow":0x06, "mask": 0x07
            },
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"bus": 87, "addr": 0x0d, "offset": 0x15, "way": "i2c"},
                    "Running": {"loc":"/sys/rg_plat/fan/fan3/motor0/status","way":"sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc":"/sys/rg_plat/fan/fan3/motor0/status","way":"sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/rg_plat/fan/fan3/motor0/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FRONT_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
                "Rotor2_config": {
                    "name": "Rotor2",
                    "Set_speed": {"bus": 87, "addr": 0x0d, "offset": 0x15, "way": "i2c"},
                    "Running": {"loc":"/sys/rg_plat/fan/fan3/motor1/status","way":"sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc":"/sys/rg_plat/fan/fan3/motor1/status","way":"sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/rg_plat/fan/fan3/motor1/speed", "way": "sysfs"},
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
            "fan_display_name": fan_display_name,
            "e2loc": {'loc': '/sys/bus/i2c/devices/i2c-99/99-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "present": {"loc":"/sys/rg_plat/fan/fan4/present","way":"sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 95, "addr": 0x0d, "offset": 0x3c, "way": "i2c"},
            "led_attrs": {
                "off":0x00, "red_flash":0x01, "red":0x02,
                "green_flash":0x03, "green":0x04, "yellow_flash":0x05,
                "yellow":0x06, "mask": 0x07
            },
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"bus": 95, "addr": 0x0d, "offset": 0x15, "way": "i2c"},
                    "Running": {"loc":"/sys/rg_plat/fan/fan4/motor0/status","way":"sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc":"/sys/rg_plat/fan/fan4/motor0/status","way":"sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/rg_plat/fan/fan4/motor0/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FRONT_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
                "Rotor2_config": {
                    "name": "Rotor2",
                    "Set_speed": {"bus": 95, "addr": 0x0d, "offset": 0x15, "way": "i2c"},
                    "Running": {"loc":"/sys/rg_plat/fan/fan4/motor1/status","way":"sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc":"/sys/rg_plat/fan/fan4/motor1/status","way":"sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/rg_plat/fan/fan4/motor1/speed", "way": "sysfs"},
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
            "fan_display_name": fan_display_name,
            "e2loc": {'loc': '/sys/bus/i2c/devices/i2c-92/92-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "present": {"loc":"/sys/rg_plat/fan/fan5/present","way":"sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 87, "addr": 0x0d, "offset": 0x3d, "way": "i2c"},
            "led_attrs": {
                "off":0x00, "red_flash":0x01, "red":0x02,
                "green_flash":0x03, "green":0x04, "yellow_flash":0x05,
                "yellow":0x06, "mask": 0x07
            },
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"bus": 87, "addr": 0x0d, "offset": 0x16, "way": "i2c"},
                    "Running": {"loc":"/sys/rg_plat/fan/fan5/motor0/status","way":"sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc":"/sys/rg_plat/fan/fan5/motor0/status","way":"sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/rg_plat/fan/fan5/motor0/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FRONT_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
                "Rotor2_config": {
                    "name": "Rotor2",
                    "Set_speed": {"bus": 87, "addr": 0x0d, "offset": 0x16, "way": "i2c"},
                    "Running": {"loc":"/sys/rg_plat/fan/fan5/motor1/status","way":"sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc":"/sys/rg_plat/fan/fan5/motor1/status","way":"sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/rg_plat/fan/fan5/motor1/speed", "way": "sysfs"},
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
            "fan_display_name": fan_display_name,
            "e2loc": {'loc': '/sys/bus/i2c/devices/i2c-100/100-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "present": {"loc":"/sys/rg_plat/fan/fan6/present","way":"sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 95, "addr": 0x0d, "offset": 0x3d, "way": "i2c"},
            "led_attrs": {
                "off":0x00, "red_flash":0x01, "red":0x02,
                "green_flash":0x03, "green":0x04, "yellow_flash":0x05,
                "yellow":0x06, "mask": 0x07
            },
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"bus": 95, "addr": 0x0d, "offset": 0x16, "way": "i2c"},
                    "Running": {"loc":"/sys/rg_plat/fan/fan6/motor0/status","way":"sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc":"/sys/rg_plat/fan/fan6/motor0/status","way":"sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/rg_plat/fan/fan6/motor0/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FRONT_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
                "Rotor2_config": {
                    "name": "Rotor2",
                    "Set_speed": {"bus": 95, "addr": 0x0d, "offset": 0x16, "way": "i2c"},
                    "Running": {"loc":"/sys/rg_plat/fan/fan6/motor1/status","way":"sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc":"/sys/rg_plat/fan/fan6/motor1/status","way":"sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/rg_plat/fan/fan6/motor1/speed", "way": "sysfs"},
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
            "fan_display_name": fan_display_name,
            "e2loc": {'loc': '/sys/bus/i2c/devices/i2c-93/93-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "present": {"loc":"/sys/rg_plat/fan/fan7/present","way":"sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 87, "addr": 0x0d, "offset": 0x3e, "way": "i2c"},
            "led_attrs": {
                "off":0x00, "red_flash":0x01, "red":0x02,
                "green_flash":0x03, "green":0x04, "yellow_flash":0x05,
                "yellow":0x06, "mask": 0x07
            },
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"bus": 87, "addr": 0x0d, "offset": 0x17, "way": "i2c"},
                    "Running": {"loc":"/sys/rg_plat/fan/fan7/motor0/status","way":"sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc":"/sys/rg_plat/fan/fan7/motor0/status","way":"sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/rg_plat/fan/fan7/motor0/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FRONT_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
                "Rotor2_config": {
                    "name": "Rotor2",
                    "Set_speed": {"bus": 87, "addr": 0x0d, "offset": 0x17, "way": "i2c"},
                    "Running": {"loc":"/sys/rg_plat/fan/fan7/motor1/status","way":"sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc":"/sys/rg_plat/fan/fan7/motor1/status","way":"sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/rg_plat/fan/fan7/motor1/speed", "way": "sysfs"},
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
            "fan_display_name": fan_display_name,
            "e2loc": {'loc': '/sys/bus/i2c/devices/i2c-101/101-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "present": {"loc":"/sys/rg_plat/fan/fan8/present","way":"sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 95, "addr": 0x0d, "offset": 0x3e, "way": "i2c"},
            "led_attrs": {
                "off":0x00, "red_flash":0x01, "red":0x02,
                "green_flash":0x03, "green":0x04, "yellow_flash":0x05,
                "yellow":0x06, "mask": 0x07
            },
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"bus": 95, "addr": 0x0d, "offset": 0x17, "way": "i2c"},
                    "Running": {"loc":"/sys/rg_plat/fan/fan8/motor0/status","way":"sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc":"/sys/rg_plat/fan/fan8/motor0/status","way":"sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/rg_plat/fan/fan8/motor0/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FRONT_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
                "Rotor2_config": {
                    "name": "Rotor2",
                    "Set_speed": {"bus": 95, "addr": 0x0d, "offset": 0x17, "way": "i2c"},
                    "Running": {"loc":"/sys/rg_plat/fan/fan8/motor1/status","way":"sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc":"/sys/rg_plat/fan/fan8/motor1/status","way":"sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/rg_plat/fan/fan8/motor1/speed", "way": "sysfs"},
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
            "name": "BASE_CPLD",
            "cpld_id": "CPLD2",
            "VersionFile": {"loc": "/dev/cpld1", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for base functions",
            "slot": 0,
        },
        {
            "name": "UPORT_CPLD",
            "cpld_id": "CPLD3",
            "VersionFile": {"loc": "/dev/cpld2", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for sff modules",
            "slot": 0,
        },
        {
            "name": "UFCB_CPLD",
            "cpld_id": "CPLD4",
            "VersionFile": {"loc": "/dev/cpld3", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for fan modules",
            "slot": 0,
        },
        {
            "name": "DFCB_CPLD",
            "cpld_id": "CPLD5",
            "VersionFile": {"loc": "/dev/cpld4", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for fan modules",
            "slot": 0,
        },
        {
            "name": "MAC_CPLDA",
            "cpld_id": "CPLD6",
            "VersionFile": {"loc": "/dev/cpld5", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for sff modules",
            "slot": 0,
        },
        {
            "name": "MAC_CPLDB",
            "cpld_id": "CPLD7",
            "VersionFile": {"loc": "/dev/cpld6", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for sff modules",
            "slot": 0,
        },
        {
            "name": "DPORT_CPLD",
            "cpld_id": "CPLD8",
            "VersionFile": {"loc": "/dev/cpld6", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for sff modules",
            "slot": 0,
        },
        {
            "name": "UPORT_FPGA",
            "cpld_id": "CPLD9",
            "VersionFile": {"loc": "/dev/fpga0", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for base functions",
            "slot": 0,
            "format": "little_endian",
        },
        {
            "name": "MAC_FPGA",
            "cpld_id": "CPLD10",
            "VersionFile": {"loc": "/dev/fpga1", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for base functions",
            "slot": 0,
            "format": "little_endian",
        },
        {
            "name": "DPORT_FPGA",
            "cpld_id": "CPLD11",
            "VersionFile": {"loc": "/dev/fpga2", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for base functions",
            "slot": 0,
            "format": "little_endian",
        },
    ],
    "dcdc": [
        {
            "name": "MAC_VDD_ANALOG1",
            "dcdc_id": "DCDC1",
            "Min": 731,
            "value": {
                "loc": "/sys/bus/i2c/devices/73-005b/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 809,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_VDD12V",
            "dcdc_id": "DCDC2",
            "Min": 11400,
            "value": {
                "loc": "/sys/bus/i2c/devices/73-005b/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 12600,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_VDD1.0V_FPGA",
            "dcdc_id": "DCDC3",
            "Min": 969,
            "value": {
                "loc": "/sys/bus/i2c/devices/73-005b/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 1071,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_VDD1.8V_FPGA",
            "dcdc_id": "DCDC4",
            "Min": 1710,
            "value": {
                "loc": "/sys/bus/i2c/devices/73-005b/hwmon/hwmon*/in4_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 1890,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_VDD1.2V_FPGA",
            "dcdc_id": "DCDC5",
            "Min": 1140,
            "value": {
                "loc": "/sys/bus/i2c/devices/73-005b/hwmon/hwmon*/in5_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 1260,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_VDD3.3V",
            "dcdc_id": "DCDC6",
            "Min": 3135,
            "value": {
                "loc": "/sys/bus/i2c/devices/73-005b/hwmon/hwmon*/in6_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 3465,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_SW_VDD1.2V",
            "dcdc_id": "DCDC7",
            "Min": 1140,
            "value": {
                "loc": "/sys/bus/i2c/devices/73-005b/hwmon/hwmon*/in7_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 1260,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_VDD5V_CLK_MCU",
            "dcdc_id": "DCDC8",
            "Min": 4826,
            "value": {
                "loc": "/sys/bus/i2c/devices/73-005b/hwmon/hwmon*/in8_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 5334,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_VDD5V_VR",
            "dcdc_id": "DCDC9",
            "Min": 4826,
            "value": {
                "loc": "/sys/bus/i2c/devices/73-005b/hwmon/hwmon*/in9_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 5334,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_VDD3.3_CLK",
            "dcdc_id": "DCDC10",
            "Min": 3154,
            "value": {
                "loc": "/sys/bus/i2c/devices/73-005b/hwmon/hwmon*/in10_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 3486,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_VDDO1.8V",
            "dcdc_id": "DCDC11",
            "Min": 1719,
            "value": {
                "loc": "/sys/bus/i2c/devices/73-005b/hwmon/hwmon*/in11_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 1901,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_VDDO1.2V",
            "dcdc_id": "DCDC12",
            "Min": 1140,
            "value": {
                "loc": "/sys/bus/i2c/devices/73-005b/hwmon/hwmon*/in12_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 1260,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_VDD_CORE",
            "dcdc_id": "DCDC13",
            "Min": 700,
            "value": {
                "loc": "/sys/bus/i2c/devices/73-005b/hwmon/hwmon*/in13_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 950,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_VDD_ANALOG",
            "dcdc_id": "DCDC14",
            "Min": 731,
            "value": {
                "loc": "/sys/bus/i2c/devices/73-005b/hwmon/hwmon*/in14_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 809,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_VDD1.2V_MAC",
            "dcdc_id": "DCDC15",
            "Min": 1140,
            "value": {
                "loc": "/sys/bus/i2c/devices/73-005b/hwmon/hwmon*/in15_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 1260,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_AVDD1.8V",
            "dcdc_id": "DCDC16",
            "Min": 1710,
            "value": {
                "loc": "/sys/bus/i2c/devices/73-005b/hwmon/hwmon*/in16_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 1890,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "BASE_VDD12V",
            "dcdc_id": "DCDC17",
            "Min": 11400,
            "value": {
                "loc": "/sys/bus/i2c/devices/105-005b/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 12600,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "BASE_SW_VDD1.2V",
            "dcdc_id": "DCDC18",
            "Min": 1140,
            "value": {
                "loc": "/sys/bus/i2c/devices/105-005b/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 1260,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "BASE_VDD2.5V",
            "dcdc_id": "DCDC19",
            "Min": 2365,
            "value": {
                "loc": "/sys/bus/i2c/devices/105-005b/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 2615,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "BASE_VDD3.3V",
            "dcdc_id": "DCDC20",
            "Min": 3116,
            "value": {
                "loc": "/sys/bus/i2c/devices/105-005b/hwmon/hwmon*/in4_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 3444,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "BASE_SSD_VDD3.3V",
            "dcdc_id": "DCDC21",
            "Min": 3135,
            "value": {
                "loc": "/sys/bus/i2c/devices/105-005b/hwmon/hwmon*/in5_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 3465,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "CPU_VCCIN",
            "dcdc_id": "DCDC22",
            "Min": 1600,
            "value": {
                "loc": "/sys/bus/i2c/devices/106-0060/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 1950,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "CPU_P1V05",
            "dcdc_id": "DCDC23",
            "Min": 1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/106-0060/hwmon/hwmon*/in4_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 1100,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "CPU_P1V2_VDDQ",
            "dcdc_id": "DCDC24",
            "Min": 1160,
            "value": {
                "loc": "/sys/bus/i2c/devices/106-006c/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 1260,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "CPU_P2V5_VPP",
            "dcdc_id": "DCDC25",
            "Min": 2113,
            "value": {
                "loc": "/sys/bus/i2c/devices/106-006c/hwmon/hwmon*/in4_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 2447,
            "format": "float(float(%s)/1000) * 1.124",
        },
        {
            "name": "CPU_P3V3_STBY",
            "dcdc_id": "DCDC26",
            "Min": 3135,
            "value": {
                "loc": "/sys/bus/i2c/devices/106-0043/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 3465,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "CPU_P5V_AUX_IN",
            "dcdc_id": "DCDC27",
            "Min": 4250,
            "value": {
                "loc": "/sys/bus/i2c/devices/106-0043/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 5500,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "CPU_P1V7_VCCSCFUSESUS_IN",
            "dcdc_id": "DCDC28",
            "Min": 1615,
            "value": {
                "loc": "/sys/bus/i2c/devices/106-0043/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 1785,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "UPORT_VDD1.0V_FPGA",
            "dcdc_id": "DCDC29",
            "Min": 959,
            "value": {
                "loc": "/sys/bus/i2c/devices/62-005b/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 1061,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "UPORT_VDD1.8V_FPGA",
            "dcdc_id": "DCDC30",
            "Min": 1719,
            "value": {
                "loc": "/sys/bus/i2c/devices/62-005b/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 1901,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "UPORT_VDD1.2V_FPGA",
            "dcdc_id": "DCDC31",
            "Min": 1140,
            "value": {
                "loc": "/sys/bus/i2c/devices/62-005b/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 1260,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "UPORT_VDD3.3V",
            "dcdc_id": "DCDC32",
            "Min": 3200,
            "value": {
                "loc": "/sys/bus/i2c/devices/62-005b/hwmon/hwmon*/in4_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 3600,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "UPORT_QSFP56_VDD3.3V_A",
            "dcdc_id": "DCDC33",
            "Min": 3200,
            "value": {
                "loc": "/sys/bus/i2c/devices/62-005b/hwmon/hwmon*/in5_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 3600,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "UPORT_QSFP56_VDD3.3V_B",
            "dcdc_id": "DCDC34",
            "Min": 3200,
            "value": {
                "loc": "/sys/bus/i2c/devices/62-005b/hwmon/hwmon*/in6_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 3600,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "UPORT_QSFP56_VDD3.3V_C",
            "dcdc_id": "DCDC35",
            "Min": 3200,
            "value": {
                "loc": "/sys/bus/i2c/devices/62-005b/hwmon/hwmon*/in7_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 3600,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "UPORT_QSFP56_VDD3.3V_D",
            "dcdc_id": "DCDC36",
            "Min": 3200,
            "value": {
                "loc": "/sys/bus/i2c/devices/62-005b/hwmon/hwmon*/in8_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 3600,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "UPORT_VDD3.3_MON",
            "dcdc_id": "DCDC37",
            "Min": 3135,
            "value": {
                "loc": "/sys/bus/i2c/devices/62-005b/hwmon/hwmon*/in9_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 3465,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "UPORT_VDD12V",
            "dcdc_id": "DCDC38",
            "Min": 11400,
            "value": {
                "loc": "/sys/bus/i2c/devices/62-005b/hwmon/hwmon*/in10_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 12600,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "DPORT_VDD1.0V_FPGA",
            "dcdc_id": "DCDC39",
            "Min": 959,
            "value": {
                "loc": "/sys/bus/i2c/devices/113-005b/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 1061,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "DPORT_VDD1.8V_FPGA",
            "dcdc_id": "DCDC40",
            "Min": 1719,
            "value": {
                "loc": "/sys/bus/i2c/devices/113-005b/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 1901,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "DPORT_VDD1.2V_FPGA",
            "dcdc_id": "DCDC41",
            "Min": 1140,
            "value": {
                "loc": "/sys/bus/i2c/devices/113-005b/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 1260,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "DPORT_VDD3.3V",
            "dcdc_id": "DCDC42",
            "Min": 3200,
            "value": {
                "loc": "/sys/bus/i2c/devices/113-005b/hwmon/hwmon*/in4_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 3600,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "DPORT_QSFP56_VDD3.3V_A",
            "dcdc_id": "DCDC43",
            "Min": 3200,
            "value": {
                "loc": "/sys/bus/i2c/devices/113-005b/hwmon/hwmon*/in5_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 3600,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "DPORT_QSFP56_VDD3.3V_B",
            "dcdc_id": "DCDC44",
            "Min": 3200,
            "value": {
                "loc": "/sys/bus/i2c/devices/113-005b/hwmon/hwmon*/in6_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 3600,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "DPORT_QSFP56_VDD3.3V_C",
            "dcdc_id": "DCDC45",
            "Min": 3200,
            "value": {
                "loc": "/sys/bus/i2c/devices/113-005b/hwmon/hwmon*/in7_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 3600,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "DPORT_QSFP56_VDD3.3V_D",
            "dcdc_id": "DCDC46",
            "Min": 3200,
            "value": {
                "loc": "/sys/bus/i2c/devices/113-005b/hwmon/hwmon*/in8_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 3600,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "DPORT_VDD3.3_MON",
            "dcdc_id": "DCDC47",
            "Min": 3135,
            "value": {
                "loc": "/sys/bus/i2c/devices/113-005b/hwmon/hwmon*/in9_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 3465,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "DPORT_VDD12V",
            "dcdc_id": "DCDC48",
            "Min": 11400,
            "value": {
                "loc": "/sys/bus/i2c/devices/113-005b/hwmon/hwmon*/in10_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 12600,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_QSFPDD_VDD3.3V_A",
            "dcdc_id": "DCDC49",
            "Min": 3.200,
            "value": {
                "loc": "/sys/rg_plat/sensor/in0/in_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 3.600,
            "format": "%s",
        },
        {
            "name": "MAC_QSFPDD_VDD3.3V_B",
            "dcdc_id": "DCDC50",
            "Min": 3.200,
            "value": {
                "loc": "/sys/rg_plat/sensor/in1/in_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 3.600,
            "format": "%s",
        },
        {
            "name": "MAC_QSFPDD_VDD3.3V_C",
            "dcdc_id": "DCDC51",
            "Min": 3.200,
            "value": {
                "loc": "/sys/rg_plat/sensor/in2/in_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 3.600,
            "format": "%s",
        },
        {
            "name": "MAC_QSFPDD_VDD3.3V_D",
            "dcdc_id": "DCDC52",
            "Min": 3.200,
            "value": {
                "loc": "/sys/rg_plat/sensor/in3/in_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 3.600,
            "format": "%s",
        },
        {
            "name": "MAC_QSFPDD_VDD3.3V_E",
            "dcdc_id": "DCDC53",
            "Min": 3.200,
            "value": {
                "loc": "/sys/rg_plat/sensor/in4/in_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 3.600,
            "format": "%s",
        },
        {
            "name": "MAC_QSFPDD_VDD3.3V_F",
            "dcdc_id": "DCDC54",
            "Min": 3.200,
            "value": {
                "loc": "/sys/rg_plat/sensor/in5/in_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 3.600,
            "format": "%s",
        },
        {
            "name": "MAC_QSFPDD_VDD3.3V_G",
            "dcdc_id": "DCDC55",
            "Min": 3.200,
            "value": {
                "loc": "/sys/rg_plat/sensor/in6/in_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 3.600,
            "format": "%s",
        },
        {
            "name": "MAC_QSFPDD_VDD3.3V_H",
            "dcdc_id": "DCDC56",
            "Min": 3.200,
            "value": {
                "loc": "/sys/rg_plat/sensor/in7/in_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 3.600,
            "format": "%s",
        },
    ],
    "cpu": [
        {
            "name": "cpu",
            "CpuResetCntReg": {"loc": "/dev/cpld1", "offset": 0x10, "len": 1, "way": "devfile_ascii"},
        }
    ],
    "sfps": {
        "port_start": 1,
        "port_end": 127,
    }
}
