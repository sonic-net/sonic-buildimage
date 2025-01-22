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
    "F2B": ["FAN24K4056-F", "FAN24K4056S-F"],
    "B2F": ["FAN24K4056S-R"],
}

fan_display_name = {
    "FAN24K4056-F": ["FAN24K4056-F", "FAN24K4056S-F"],
    "FAN24K4056-R": ["FAN24K4056S-R"]
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

    FRONT_FAN_SPEED_MAX = 25000
    REAR_FAN_SPEED_MAX = 22000
    FAN_SPEED_MIN = 2000


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
            "e2loc": {"loc": "/sys/bus/i2c/devices/41-0050/eeprom", "way": "sysfs"},
            "pmbusloc": {"bus": 41, "addr": 0x58, "way": "i2c"},
            "present": {"loc": "/sys/rg_plat/psu/psu1/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "name": "PSU1",
            "psu_display_name": psu_display_name,
            "airflow": psu_fan_airflow,
            "psu_not_present_pwm": PSU_NOT_PRESENT_PWM,
            "TempStatus": {"bus": 41, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0004},
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-41/41-0058/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": threshold.PSU_TEMP_MIN,
                "Max": threshold.PSU_TEMP_MAX,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            },
            "FanStatus": {"bus": 41, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0400},
            "FanSpeed": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-41/41-0058/hwmon/hwmon*/fan1_input", "way": "sysfs"},
                "Min": threshold.PSU_FAN_SPEED_MIN,
                "Max": threshold.PSU_FAN_SPEED_MAX,
                "Unit": Unit.Speed
            },
            "InputsStatus": {"bus": 41, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x2000},
            "InputsType": {"bus": 41, "addr": 0x58, "offset": 0x80, "way": "i2c", 'psutypedecode': psutypedecode},
            "InputsVoltage": {
                'AC': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-41/41-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_AC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_AC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"

                },
                'DC': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-41/41-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_DC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_DC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                },
                'other': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-41/41-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.ERR_VALUE,
                    "Max": threshold.ERR_VALUE,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                }
            },
            "InputsCurrent":
            {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-41/41-0058/hwmon/hwmon*/curr1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_CURRENT_MIN,
                "Max": threshold.PSU_INPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "InputsPower":
            {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-41/41-0058/hwmon/hwmon*/power1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_POWER_MIN,
                "Max": threshold.PSU_INPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
            "OutputsStatus": {"bus": 41, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x8800},
            "OutputsVoltage":
            {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-41/41-0058/hwmon/hwmon*/in2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_VOLTAGE_MIN,
                "Max": threshold.PSU_OUTPUT_VOLTAGE_MAX,
                "Unit": Unit.Voltage,
                "format": "float(float(%s)/1000)"
            },
            "OutputsCurrent":
            {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-41/41-0058/hwmon/hwmon*/curr2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_CURRENT_MIN,
                "Max": threshold.PSU_OUTPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "OutputsPower":
            {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-41/41-0058/hwmon/hwmon*/power2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_POWER_MIN,
                "Max": threshold.PSU_OUTPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
        },
        {
            "e2loc": {"loc": "/sys/bus/i2c/devices/42-0050/eeprom", "way": "sysfs"},
            "pmbusloc": {"bus": 42, "addr": 0x58, "way": "i2c"},
            "present": {"loc": "/sys/rg_plat/psu/psu2/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "name": "PSU2",
            "psu_display_name": psu_display_name,
            "airflow": psu_fan_airflow,
            "psu_not_present_pwm": PSU_NOT_PRESENT_PWM,
            "TempStatus": {"bus": 42, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0004},
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-42/42-0058/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": threshold.PSU_TEMP_MIN,
                "Max": threshold.PSU_TEMP_MAX,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            },
            "FanStatus": {"bus": 42, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0400},
            "FanSpeed": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-42/42-0058/hwmon/hwmon*/fan1_input", "way": "sysfs"},
                "Min": threshold.PSU_FAN_SPEED_MIN,
                "Max": threshold.PSU_FAN_SPEED_MAX,
                "Unit": Unit.Speed
            },
            "InputsStatus": {"bus": 42, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x2000},
            "InputsType": {"bus": 42, "addr": 0x58, "offset": 0x80, "way": "i2c", 'psutypedecode': psutypedecode},
            "InputsVoltage": {
                'AC': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-42/42-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_AC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_AC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"

                },
                'DC': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-42/42-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_DC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_DC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                },
                'other': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-42/42-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.ERR_VALUE,
                    "Max": threshold.ERR_VALUE,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                }
            },
            "InputsCurrent":
            {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-42/42-0058/hwmon/hwmon*/curr1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_CURRENT_MIN,
                "Max": threshold.PSU_INPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "InputsPower":
            {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-42/42-0058/hwmon/hwmon*/power1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_POWER_MIN,
                "Max": threshold.PSU_INPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
            "OutputsStatus": {"bus": 42, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x8800},
            "OutputsVoltage":
            {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-42/42-0058/hwmon/hwmon*/in2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_VOLTAGE_MIN,
                "Max": threshold.PSU_OUTPUT_VOLTAGE_MAX,
                "Unit": Unit.Voltage,
                "format": "float(float(%s)/1000)"
            },
            "OutputsCurrent":
            {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-42/42-0058/hwmon/hwmon*/curr2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_CURRENT_MIN,
                "Max": threshold.PSU_OUTPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "OutputsPower":
            {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-42/42-0058/hwmon/hwmon*/power2_input", "way": "sysfs"},
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
            "api_name": "Board",
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/40-004e/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": -10000,
                "Low": 0,
                "High": 70000,
                "Max": 80000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
        {
            "name": "CPU_TEMP",
            "temp_id": "TEMP2",
            "api_name": "CPU",
            "Temperature": {
                "value": {"loc": "/sys/bus/platform/devices/coretemp.0/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": 2000,
                "Low": 10000,
                "High": 100000,
                "Max": 104000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
        {
            "name": "INLET_TEMP",
            "temp_id": "TEMP3",
            "api_name": "Inlet",
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/40-004f/hwmon/hwmon*/temp1_input", "way": "sysfs"},
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
            "temp_id": "TEMP4",
            "api_name": "Outlet",
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/36-0048/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": -10000,
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
            "api_name": "Switch ASIC",
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/44-0044/hwmon/hwmon*/temp99_input", "way": "sysfs"},
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
            "led": {"bus": 2, "addr": 0x2d, "offset": 0x40, "way": "i2c"},
            "led_attrs": {
                "off":0x00, "red_flash":0x01, "red":0x02,
                "green_flash":0x03, "green":0x04, "yellow_flash":0x05,
                "yellow":0x06, "mask": 0x07
            },
        },
        {
            "name": "FRONT_PSU_LED",
            "led_type": "PSU_LED",
            "led": {"bus": 2, "addr": 0x2d, "offset": 0x43, "way": "i2c"},
            "led_attrs": {
                "off":0x00, "red_flash":0x01, "red":0x02,
                "green_flash":0x03, "green":0x04, "yellow_flash":0x05,
                "yellow":0x06, "mask": 0x07
            },
        },
        {
            "name": "FRONT_FAN_LED",
            "led_type": "FAN_LED",
            "led": {"bus": 2, "addr": 0x2d, "offset": 0x42, "way": "i2c"},
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
            "e2loc": {'loc': '/sys/bus/i2c/devices/i2c-35/35-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "present": {"bus": 4, "addr": 0x3d, "offset": 0x37, "way": "i2c", "mask": 0x20},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 4, "addr": 0x3d, "offset": 0x41, "way": "i2c"},
            "led_attrs": {
                "off":0x00, "red_flash":0x01, "red":0x02,
                "green_flash":0x03, "green":0x04, "yellow_flash":0x05,
                "yellow":0x06, "mask": 0x07
            },
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"bus": 4, "addr": 0x3d, "offset": 0x65, "way": "i2c"},
                    "Running": {"bus": 4, "addr": 0x3d, "offset": 0x38, "way": "i2c", "mask": 0x20, "is_runing": 0x20},
                    "HwAlarm": {"bus": 4, "addr": 0x3d, "offset": 0x38, "way": "i2c", "mask": 0x20, "no_alarm": 0x20},
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
                    "Set_speed": {"bus": 4, "addr": 0x3d, "offset": 0x65, "way": "i2c"},
                    "Running": {"bus": 4, "addr": 0x3d, "offset": 0x39, "way": "i2c", "mask": 0x20, "is_runing": 0x20},
                    "HwAlarm": {"bus": 4, "addr": 0x3d, "offset": 0x39, "way": "i2c", "mask": 0x20, "no_alarm": 0x20},
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
            "e2loc": {'loc': '/sys/bus/i2c/devices/i2c-34/34-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "present": {"bus": 4, "addr": 0x3d, "offset": 0x37, "way": "i2c", "mask": 0x10},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 4, "addr": 0x3d, "offset": 0x40, "way": "i2c"},
            "led_attrs": {
                "off":0x00, "red_flash":0x01, "red":0x02,
                "green_flash":0x03, "green":0x04, "yellow_flash":0x05,
                "yellow":0x06, "mask": 0x07
            },
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"bus": 4, "addr": 0x3d, "offset": 0x64, "way": "i2c"},
                    "Running": {"bus": 4, "addr": 0x3d, "offset": 0x38, "way": "i2c", "mask": 0x10, "is_runing": 0x10},
                    "HwAlarm": {"bus": 4, "addr": 0x3d, "offset": 0x38, "way": "i2c", "mask": 0x10, "no_alarm": 0x10},
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
                    "Set_speed": {"bus": 4, "addr": 0x3d, "offset": 0x64, "way": "i2c"},
                    "Running": {"bus": 4, "addr": 0x3d, "offset": 0x39, "way": "i2c", "mask": 0x10, "is_runing": 0x10},
                    "HwAlarm": {"bus": 4, "addr": 0x3d, "offset": 0x39, "way": "i2c", "mask": 0x10, "no_alarm": 0x10},
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
            "e2loc": {'loc': '/sys/bus/i2c/devices/i2c-33/33-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "present": {"bus": 4, "addr": 0x3d, "offset": 0x37, "way": "i2c", "mask": 0x08},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 4, "addr": 0x3d, "offset": 0x3f, "way": "i2c"},
            "led_attrs": {
                "off":0x00, "red_flash":0x01, "red":0x02,
                "green_flash":0x03, "green":0x04, "yellow_flash":0x05,
                "yellow":0x06, "mask": 0x07
            },
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"bus": 4, "addr": 0x3d, "offset": 0x63, "way": "i2c"},
                    "Running": {"bus": 4, "addr": 0x3d, "offset": 0x38, "way": "i2c", "mask": 0x08, "is_runing": 0x08},
                    "HwAlarm": {"bus": 4, "addr": 0x3d, "offset": 0x38, "way": "i2c", "mask": 0x08, "no_alarm": 0x08},
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
                    "Set_speed": {"bus": 4, "addr": 0x3d, "offset": 0x63, "way": "i2c"},
                    "Running": {"bus": 4, "addr": 0x3d, "offset": 0x39, "way": "i2c", "mask": 0x08, "is_runing": 0x08},
                    "HwAlarm": {"bus": 4, "addr": 0x3d, "offset": 0x39, "way": "i2c", "mask": 0x08, "no_alarm": 0x08},
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
            "e2loc": {'loc': '/sys/bus/i2c/devices/i2c-32/32-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "present": {"bus": 4, "addr": 0x3d, "offset": 0x37, "way": "i2c", "mask": 0x04},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 4, "addr": 0x3d, "offset": 0x3e, "way": "i2c"},
            "led_attrs": {
                "off":0x00, "red_flash":0x01, "red":0x02,
                "green_flash":0x03, "green":0x04, "yellow_flash":0x05,
                "yellow":0x06, "mask": 0x07
            },
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"bus": 4, "addr": 0x3d, "offset": 0x62, "way": "i2c"},
                    "Running": {"bus": 4, "addr": 0x3d, "offset": 0x38, "way": "i2c", "mask": 0x04, "is_runing": 0x04},
                    "HwAlarm": {"bus": 4, "addr": 0x3d, "offset": 0x38, "way": "i2c", "mask": 0x04, "no_alarm": 0x04},
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
                    "Set_speed": {"bus": 4, "addr": 0x3d, "offset": 0x62, "way": "i2c"},
                    "Running": {"bus": 4, "addr": 0x3d, "offset": 0x39, "way": "i2c", "mask": 0x04, "is_runing": 0x04},
                    "HwAlarm": {"bus": 4, "addr": 0x3d, "offset": 0x39, "way": "i2c", "mask": 0x04, "no_alarm": 0x04},
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
            "e2loc": {'loc': '/sys/bus/i2c/devices/i2c-31/31-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "present": {"bus": 4, "addr": 0x3d, "offset": 0x37, "way": "i2c", "mask": 0x02},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 4, "addr": 0x3d, "offset": 0x3d, "way": "i2c"},
            "led_attrs": {
                "off":0x00, "red_flash":0x01, "red":0x02,
                "green_flash":0x03, "green":0x04, "yellow_flash":0x05,
                "yellow":0x06, "mask": 0x07
            },
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"bus": 4, "addr": 0x3d, "offset": 0x61, "way": "i2c"},
                    "Running": {"bus": 4, "addr": 0x3d, "offset": 0x38, "way": "i2c", "mask": 0x02, "is_runing": 0x02},
                    "HwAlarm": {"bus": 4, "addr": 0x3d, "offset": 0x38, "way": "i2c", "mask": 0x02, "no_alarm": 0x02},
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
                    "Set_speed": {"bus": 4, "addr": 0x3d, "offset": 0x61, "way": "i2c"},
                    "Running": {"bus": 4, "addr": 0x3d, "offset": 0x39, "way": "i2c", "mask": 0x02, "is_runing": 0x02},
                    "HwAlarm": {"bus": 4, "addr": 0x3d, "offset": 0x39, "way": "i2c", "mask": 0x02, "no_alarm": 0x02},
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
            "e2loc": {'loc': '/sys/bus/i2c/devices/i2c-30/30-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "present": {"bus": 4, "addr": 0x3d, "offset": 0x37, "way": "i2c", "mask": 0x01},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 4, "addr": 0x3d, "offset": 0x3c, "way": "i2c"},
            "led_attrs": {
                "off":0x00, "red_flash":0x01, "red":0x02,
                "green_flash":0x03, "green":0x04, "yellow_flash":0x05,
                "yellow":0x06, "mask": 0x07
            },
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"bus": 4, "addr": 0x3d, "offset": 0x60, "way": "i2c"},
                    "Running": {"bus": 4, "addr": 0x3d, "offset": 0x38, "way": "i2c", "mask": 0x01, "is_runing": 0x01},
                    "HwAlarm": {"bus": 4, "addr": 0x3d, "offset": 0x38, "way": "i2c", "mask": 0x01, "no_alarm": 0x01},
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
                    "Set_speed": {"bus": 4, "addr": 0x3d, "offset": 0x60, "way": "i2c"},
                    "Running": {"bus": 4, "addr": 0x3d, "offset": 0x39, "way": "i2c", "mask": 0x01, "is_runing": 0x01},
                    "HwAlarm": {"bus": 4, "addr": 0x3d, "offset": 0x39, "way": "i2c", "mask": 0x01, "no_alarm": 0x01},
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
            "name": "MAC_CPLDA",
            "cpld_id": "CPLD3",
            "VersionFile": {"loc": "/dev/cpld4", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for sff modules",
            "slot": 0,
        },
        {
            "name": "MAC_CPLDB",
            "cpld_id": "CPLD4",
            "VersionFile": {"loc": "/dev/cpld5", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for sff modules",
            "slot": 0,
        },
        {
            "name": "FAN_CPLD",
            "cpld_id": "CPLD5",
            "VersionFile": {"loc": "/dev/cpld6", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for fan modules",
            "slot": 0,
        },
        {
            "name": "FPGA",
            "cpld_id": "CPLD6",
            "VersionFile": {"loc": "/dev/fpga0", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for base functions",
            "slot": 0,
            "format": "little_endian",
        },
    ],
    "dcdc": [
        {
            "name": "VDD5V_CLK_MCU",
            "dcdc_id": "DCDC1",
            "Min": 4840,
            "value": {
                "loc": "/sys/bus/i2c/devices/45-005b/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 5345,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "VDD3.3_CLK",
            "dcdc_id": "DCDC2",
            "Min": 3220,
            "value": {
                "loc": "/sys/bus/i2c/devices/45-005b/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 3560,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "VDD1.0V",
            "dcdc_id": "DCDC3",
            "Min": 950,
            "value": {
                "loc": "/sys/bus/i2c/devices/45-005b/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 1049,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "VDD1.8V",
            "dcdc_id": "DCDC4",
            "Min": 1720,
            "value": {
                "loc": "/sys/bus/i2c/devices/45-005b/hwmon/hwmon*/in4_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 1903,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_BOARD_VDD3.3V",
            "dcdc_id": "DCDC5",
            "Min": 3170,
            "value": {
                "loc": "/sys/bus/i2c/devices/45-005b/hwmon/hwmon*/in5_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 3499,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "VDD1.2V",
            "dcdc_id": "DCDC6",
            "Min": 1150,
            "value": {
                "loc": "/sys/bus/i2c/devices/45-005b/hwmon/hwmon*/in6_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 1272,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "VDD_CORE",
            "dcdc_id": "DCDC7",
            "Min": 670,
            "value": {
                "loc": "/sys/bus/i2c/devices/45-005b/hwmon/hwmon*/in7_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 950,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "ANALOG0.75V",
            "dcdc_id": "DCDC8",
            "Min": 700,
            "value": {
                "loc": "/sys/bus/i2c/devices/45-005b/hwmon/hwmon*/in8_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 800,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_VDD1.2V",
            "dcdc_id": "DCDC9",
            "Min": 1140,
            "value": {
                "loc": "/sys/bus/i2c/devices/45-005b/hwmon/hwmon*/in9_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 1259,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "VDDO1.8V",
            "dcdc_id": "DCDC10",
            "Min": 1750,
            "value": {
                "loc": "/sys/bus/i2c/devices/45-005b/hwmon/hwmon*/in10_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 1937,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_ANA1.2V",
            "dcdc_id": "DCDC11",
            "Min": 1150,
            "value": {
                "loc": "/sys/bus/i2c/devices/45-005b/hwmon/hwmon*/in11_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 1276,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MAC_ANA1.8V",
            "dcdc_id": "DCDC12",
            "Min": 1730,
            "value": {
                "loc": "/sys/bus/i2c/devices/45-005b/hwmon/hwmon*/in12_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 1910,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "QSFP56_VDD3.3V_A",
            "dcdc_id": "DCDC13",
            "Min": 3250,
            "value": {
                "loc": "/sys/bus/i2c/devices/45-005b/hwmon/hwmon*/in13_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 3595,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "QSFP56_VDD3.3V_B",
            "dcdc_id": "DCDC14",
            "Min": 3260,
            "value": {
                "loc": "/sys/bus/i2c/devices/45-005b/hwmon/hwmon*/in14_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 3601,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "QSFPDD_VDD3.3V_A",
            "dcdc_id": "DCDC15",
            "Min": 3230,
            "value": {
                "loc": "/sys/bus/i2c/devices/45-005b/hwmon/hwmon*/in15_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 3565,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "QSFPDD_VDD3.3V_B",
            "dcdc_id": "DCDC16",
            "Min": 3220,
            "value": {
                "loc": "/sys/bus/i2c/devices/45-005b/hwmon/hwmon*/in16_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 3564,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "VDD5.0V",
            "dcdc_id": "DCDC17",
            "Min": 4910,
            "value": {
                "loc": "/sys/bus/i2c/devices/24-005b/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 5429,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "CONNECT_BOARD_VDD3.3V",
            "dcdc_id": "DCDC18",
            "Min": 3110,
            "value": {
                "loc": "/sys/bus/i2c/devices/24-005b/hwmon/hwmon*/in4_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 3437,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "VDD12V",
            "dcdc_id": "DCDC19",
            "Min": 11300,
            "value": {
                "loc": "/sys/bus/i2c/devices/24-005b/hwmon/hwmon*/in6_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 12700,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "VDD3.3_STBY",
            "dcdc_id": "DCDC20",
            "Min": 3160,
            "value": {
                "loc": "/sys/bus/i2c/devices/24-005b/hwmon/hwmon*/in7_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 3489,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SSD_VDD3.3V",
            "dcdc_id": "DCDC21",
            "Min": 3140,
            "value": {
                "loc": "/sys/bus/i2c/devices/24-005b/hwmon/hwmon*/in8_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 3475,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "PHY_VDD1V0",
            "dcdc_id": "DCDC22",
            "Min": 950,
            "value": {
                "loc": "/sys/bus/i2c/devices/24-005b/hwmon/hwmon*/in9_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 1050,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "ODD_PHY_M",
            "dcdc_id": "DCDC23",
            "Min": 3116,
            "value": {
                "loc": "/sys/bus/i2c/devices/24-005b/hwmon/hwmon*/in11_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 3444,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "VCCIN",
            "dcdc_id": "DCDC24",
            "Min": 1700,
            "value": {
                "loc": "/sys/bus/i2c/devices/25-0060/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 1879,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "P1V05",
            "dcdc_id": "DCDC25",
            "Min": 1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/25-0060/hwmon/hwmon*/in4_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 1103,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "P1V2_VDDQ",
            "dcdc_id": "DCDC26",
            "Min": 1140,
            "value": {
                "loc": "/sys/bus/i2c/devices/25-006c/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 1258,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "P2V5_VPP",
            "dcdc_id": "DCDC27",
            "Min": 2117,
            "value": {
                "loc": "/sys/bus/i2c/devices/25-006c/hwmon/hwmon*/in4_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 2342,
            "format": "float(float(%s)/1000) * 1.124",
        },
        {
            "name": "P3V3_STBY",
            "dcdc_id": "DCDC28",
            "Min": 3140,
            "value": {
                "loc": "/sys/bus/i2c/devices/25-0043/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 3476,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "P5V_AUX_IN",
            "dcdc_id": "DCDC29",
            "Min": 4730,
            "value": {
                "loc": "/sys/bus/i2c/devices/25-0043/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 5229,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "P1V7_VCCSCFUSESUS_IN",
            "dcdc_id": "DCDC30",
            "Min": 1620,
            "value": {
                "loc": "/sys/bus/i2c/devices/25-0043/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 11,
            "Unit": "V",
            "Max": 1789,
            "format": "float(float(%s)/1000)",
        },
    ],
    "cpu": [
        {
            "name": "cpu",
            "CpuResetCntReg": {"loc": "/dev/cpld1", "offset": 0x88, "len": 1, "way": "devfile_ascii"},
        }
    ],
    "sfps": {
        "port_start": 1,
        "port_end": 32,
    }
}
