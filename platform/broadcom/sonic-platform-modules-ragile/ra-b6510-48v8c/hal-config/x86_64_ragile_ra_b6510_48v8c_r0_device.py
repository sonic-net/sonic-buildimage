#!/usr/bin/python3

psu_fan_airflow = {
    "intake": ['CSU550AP-3-500', 'DPS-550AB-39 A', 'GW-CRPS550N2C', 'CSU550AP-3-300', 'DPS-550AB-39 B', 'CSU550AP-3'],
    "exhaust": ['CSU550AP-3-501', 'DPS-550AB-40 A', 'GW-CRPS550N2RC']
}

fanairflow = {
    "intake": ['M1HFAN I-F'],
    "exhaust": ['M1HFAN I-R'],
}

psu_display_name = {
    "PA550II-F": ['CSU550AP-3-500', 'DPS-550AB-39 A', 'GW-CRPS550N2C', 'CSU550AP-3-300', 'DPS-550AB-39 B', 'CSU550AP-3'],
    "PA550II-R": ['CSU550AP-3-501', 'DPS-550AB-40 A', 'GW-CRPS550N2RC']
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
    PSU_TEMP_MIN = -20 * 1000
    PSU_TEMP_MAX = 60 * 1000

    PSU_FAN_SPEED_MIN = 2000
    PSU_FAN_SPEED_MAX = 18000

    PSU_OUTPUT_VOLTAGE_MIN = 11 * 1000
    PSU_OUTPUT_VOLTAGE_MAX = 14 * 1000

    PSU_AC_INPUT_VOLTAGE_MIN = 200 * 1000
    PSU_AC_INPUT_VOLTAGE_MAX = 240 * 1000

    PSU_DC_INPUT_VOLTAGE_MIN = 190 * 1000
    PSU_DC_INPUT_VOLTAGE_MAX = 290 * 1000

    ERR_VALUE = -9999999

    PSU_OUTPUT_POWER_MIN = 10 * 1000 * 1000
    PSU_OUTPUT_POWER_MAX = 560 * 1000 * 1000

    PSU_INPUT_POWER_MIN = 10 * 1000 * 1000
    PSU_INPUT_POWER_MAX = 625 * 1000 * 1000

    PSU_OUTPUT_CURRENT_MIN = 1 * 1000
    PSU_OUTPUT_CURRENT_MAX = 45 * 1000

    PSU_INPUT_CURRENT_MIN = 0 * 1000
    PSU_INPUT_CURRENT_MAX = 7 * 1000

    FRONT_FAN_SPEED_MAX = 24000
    REAR_FAN_SPEED_MAX = 22500
    FAN_SPEED_MIN = 5000


class Description:
    CPLD = "Used for managing IO modules, SFP+ modules and system LEDs"
    BIOS = "Performs initialization of hardware components during booting"
    FPGA = "Platform management controller for on-board temperature monitoring, in-chassis power"


devices = {
    "onie_e2": [
        {
            "name": "ONIE_E2",
            "e2loc": {"loc": "/sys/bus/i2c/devices/0-0056/eeprom", "way": "sysfs"},
            "airflow": "intake"
        },
    ],
    "psus": [
        {
            "e2loc": {"loc": "/sys/bus/i2c/devices/24-0050/eeprom", "way": "sysfs"},
            "pmbusloc": {"bus": 24, "addr": 0x58, "way": "i2c"},
            "present": {"loc": "/sys/wb_plat/psu/psu1/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "name": "PSU1",
            "psu_display_name": psu_display_name,
            "airflow": psu_fan_airflow,
            "TempStatus": {"bus": 24, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0004},
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-24/24-0058/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": threshold.PSU_TEMP_MIN,
                "Max": threshold.PSU_TEMP_MAX,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            },
            "FanStatus": {"bus": 24, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0400},
            "FanSpeed": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-24/24-0058/hwmon/hwmon*/fan1_input", "way": "sysfs"},
                "Min": threshold.PSU_FAN_SPEED_MIN,
                "Max": threshold.PSU_FAN_SPEED_MAX,
                "Unit": Unit.Speed
            },
            "psu_fan_tolerance": 40,
            "InputsStatus": {"bus": 24, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x2000},
            "InputsType": {"bus": 24, "addr": 0x58, "offset": 0x80, "way": "i2c", 'psutypedecode': psutypedecode},
            "InputsVoltage": {
                'AC': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-24/24-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_AC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_AC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"

                },
                'DC': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-24/24-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_DC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_DC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                },
                'other': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-24/24-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.ERR_VALUE,
                    "Max": threshold.ERR_VALUE,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                }
            },
            "InputsCurrent": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-24/24-0058/hwmon/hwmon*/curr1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_CURRENT_MIN,
                "Max": threshold.PSU_INPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "InputsPower": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-24/24-0058/hwmon/hwmon*/power1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_POWER_MIN,
                "Max": threshold.PSU_INPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
            "OutputsStatus": {"bus": 24, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x8800},
            "OutputsVoltage": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-24/24-0058/hwmon/hwmon*/in2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_VOLTAGE_MIN,
                "Max": threshold.PSU_OUTPUT_VOLTAGE_MAX,
                "Unit": Unit.Voltage,
                "format": "float(float(%s)/1000)"
            },
            "OutputsCurrent": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-24/24-0058/hwmon/hwmon*/curr2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_CURRENT_MIN,
                "Max": threshold.PSU_OUTPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "OutputsPower": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-24/24-0058/hwmon/hwmon*/power2_input", "way": "sysfs"},
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
                "value": {"loc": "/sys/bus/i2c/devices/i2c-25/25-0058/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": threshold.PSU_TEMP_MIN,
                "Max": threshold.PSU_TEMP_MAX,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            },
            "FanStatus": {"bus": 25, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0400},
            "FanSpeed": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-25/25-0058/hwmon/hwmon*/fan1_input", "way": "sysfs"},
                "Min": threshold.PSU_FAN_SPEED_MIN,
                "Max": threshold.PSU_FAN_SPEED_MAX,
                "Unit": Unit.Speed
            },
            "psu_fan_tolerance": 40,
            "InputsStatus": {"bus": 25, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x2000},
            "InputsType": {"bus": 25, "addr": 0x58, "offset": 0x80, "way": "i2c", 'psutypedecode': psutypedecode},
            "InputsVoltage": {
                'AC': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-25/25-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_AC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_AC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"

                },
                'DC': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-25/25-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_DC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_DC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                },
                'other': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-25/25-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.ERR_VALUE,
                    "Max": threshold.ERR_VALUE,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                }
            },
            "InputsCurrent": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-25/25-0058/hwmon/hwmon*/curr1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_CURRENT_MIN,
                "Max": threshold.PSU_INPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "InputsPower": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-25/25-0058/hwmon/hwmon*/power1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_POWER_MIN,
                "Max": threshold.PSU_INPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
            "OutputsStatus": {"bus": 25, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x8800},
            "OutputsVoltage": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-25/25-0058/hwmon/hwmon*/in2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_VOLTAGE_MIN,
                "Max": threshold.PSU_OUTPUT_VOLTAGE_MAX,
                "Unit": Unit.Voltage,
                "format": "float(float(%s)/1000)"
            },
            "OutputsCurrent": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-25/25-0058/hwmon/hwmon*/curr2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_CURRENT_MIN,
                "Max": threshold.PSU_OUTPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "OutputsPower": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-25/25-0058/hwmon/hwmon*/power2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_POWER_MIN,
                "Max": threshold.PSU_OUTPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
        }
    ],
    "temps": [
        {
            "name": "SWITCH_TEMP",
            "temp_id": "TEMP1",
            "api_name": "ASIC_TEMP",
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/3-0044/hwmon/hwmon*/temp99_input", "way": "sysfs"},
                "Min": -30000,
                "Low": 0,
                "High": 105000,
                "Max": 110000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
        {
            "name": "CPU_TEMP",
            "temp_id": "TEMP2",
            "Temperature": {
                "value": {"loc": "/sys/bus/platform/devices/coretemp.0/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": -15000,
                "Low": 0,
                "High": 100000,
                "Max": 102000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
        {
            "name": "INLET_TEMP",
            "temp_id": "TEMP3",
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/3-0048/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": -30000,
                "Low": 0,
                "High": 55000,
                "Max": 60000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            },
            "fix_value": {
                "fix_type": "config",
                "addend": -3,
            }
        },
        {
            "name": "OUTLET_TEMP",
            "temp_id": "TEMP4",
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/3-004c/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": -30000,
                "Low": 0,
                "High": 75000,
                "Max": 80000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
        {
            "name": "BOARD_TEMP",
            "temp_id": "TEMP5",
            "api_name": "MAC_OUT_TEMP",
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/3-004a/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": -30000,
                "Low": 0,
                "High": 75000,
                "Max": 80000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
        {
            "name": "MAC_IN_TEMP",
            "temp_id": "TEMP6",
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/3-0049/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": -30000,
                "Low": 0,
                "High": 75000,
                "Max": 80000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
        {
            "name": "PSU1_TEMP",
            "temp_id": "TEMP7",
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-24/24-0058/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": -20000,
                "Low": 0,
                "High": 55000,
                "Max": 60000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
        {
            "name": "PSU2_TEMP",
            "temp_id": "TEMP8",
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-25/25-0058/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": -20000,
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
                "Min": -30000,
                "Low": 0,
                "High": 90000,
                "Max": 100000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            },
            "invalid": -10000,
            "error": -9999,
        }
    ],
    "leds": [
        {
            "name": "FRONT_SYS_LED",
            "led_type": "SYS_LED",
            "led": {"bus": 6, "addr": 0x0d, "offset": 0x72, "way": "i2c"},
            "led_attrs": {
                "off": 0x00, "red_flash": 0x01, "red": 0x02,
                "green_flash": 0x03, "green": 0x04, "amber_flash": 0x05,
                "amber": 0x06, "mask": 0x07
            },
        },
        {
            "name": "FRONT_PSU_LED",
            "led_type": "PSU_LED",
            "led": {"bus": 6, "addr": 0x0d, "offset": 0x73, "way": "i2c"},
            "led_attrs": {
                "off": 0x10, "red_flash": 0x11, "red": 0x12,
                "green_flash": 0x13, "green": 0x14, "amber_flash": 0x15,
                "amber": 0x16, "mask": 0x17
            },
        },
        {
            "name": "FRONT_FAN_LED",
            "led_type": "FAN_LED",
            "led": {"bus": 6, "addr": 0x0d, "offset": 0x74, "way": "i2c"},
            "led_attrs": {
                "off": 0x10, "red_flash": 0x11, "red": 0x12,
                "green_flash": 0x13, "green": 0x14, "amber_flash": 0x15,
                "amber": 0x16, "mask": 0x17
            },
        },
    ],
    "fans": [
        {
            "name": "FAN1",
            "airflow": fanairflow,
            "e2loc": {'loc': '/sys/bus/i2c/devices/i2c-16/16-0050/eeprom', 'way': 'sysfs'},
            "present": {"loc": "/sys/wb_plat/fan/fan1/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 2, "addr": 0x0d, "offset": 0x3b, "way": "i2c"},
            "led_attrs": {
                "off": 0x0b, "red_flash": 0x0e, "red": 0x0a,
                "green_flash": 0x0d, "green": 0x09, "amber_flash": 0x07,
                "amber": 0x03, "mask": 0x0f
            },
            "PowerMax": 38.4,
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"bus": 2, "addr": 0x0d, "offset": 0x14, "way": "i2c"},
                    "Running": {"loc": "/sys/wb_plat/fan/fan1/motor1/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/wb_plat/fan/fan1/motor1/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/wb_plat/fan/fan1/motor1/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FRONT_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
                "Rotor2_config": {
                    "name": "Rotor2",
                    "Set_speed": {"bus": 2, "addr": 0x0d, "offset": 0x14, "way": "i2c"},
                    "Running": {"loc": "/sys/wb_plat/fan/fan1/motor0/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/wb_plat/fan/fan1/motor0/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/wb_plat/fan/fan1/motor0/speed", "way": "sysfs"},
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
            "e2loc": {'loc': '/sys/bus/i2c/devices/i2c-17/17-0050/eeprom', 'way': 'sysfs'},
            "present": {"loc": "/sys/wb_plat/fan/fan2/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 2, "addr": 0x0d, "offset": 0x3c, "way": "i2c"},
            "led_attrs": {
                "off": 0x0b, "red_flash": 0x0e, "red": 0x0a,
                "green_flash": 0x0d, "green": 0x09, "amber_flash": 0x07,
                "amber": 0x03, "mask": 0x0f
            },
            "PowerMax": 38.4,
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"bus": 2, "addr": 0x0d, "offset": 0x15, "way": "i2c"},
                    "Running": {"loc": "/sys/wb_plat/fan/fan2/motor1/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/wb_plat/fan/fan2/motor1/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/wb_plat/fan/fan2/motor1/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FRONT_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
                "Rotor2_config": {
                    "name": "Rotor2",
                    "Set_speed": {"bus": 2, "addr": 0x0d, "offset": 0x15, "way": "i2c"},
                    "Running": {"loc": "/sys/wb_plat/fan/fan2/motor0/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/wb_plat/fan/fan2/motor0/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/wb_plat/fan/fan2/motor0/speed", "way": "sysfs"},
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
            "e2loc": {'loc': '/sys/bus/i2c/devices/i2c-18/18-0050/eeprom', 'way': 'sysfs'},
            "present": {"loc": "/sys/wb_plat/fan/fan3/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 2, "addr": 0x0d, "offset": 0x3d, "way": "i2c"},
            "led_attrs": {
                "off": 0x0b, "red_flash": 0x0e, "red": 0x0a,
                "green_flash": 0x0d, "green": 0x09, "amber_flash": 0x07,
                "amber": 0x03, "mask": 0x0f
            },
            "PowerMax": 38.4,
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"bus": 2, "addr": 0x0d, "offset": 0x16, "way": "i2c"},
                    "Running": {"loc": "/sys/wb_plat/fan/fan3/motor1/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/wb_plat/fan/fan3/motor1/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/wb_plat/fan/fan3/motor1/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FRONT_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
                "Rotor2_config": {
                    "name": "Rotor2",
                    "Set_speed": {"bus": 2, "addr": 0x0d, "offset": 0x16, "way": "i2c"},
                    "Running": {"loc": "/sys/wb_plat/fan/fan3/motor0/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/wb_plat/fan/fan3/motor0/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/wb_plat/fan/fan3/motor0/speed", "way": "sysfs"},
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
            "e2loc": {'loc': '/sys/bus/i2c/devices/i2c-19/19-0050/eeprom', 'way': 'sysfs'},
            "present": {"loc": "/sys/wb_plat/fan/fan4/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 2, "addr": 0x0d, "offset": 0x3e, "way": "i2c"},
            "led_attrs": {
                "off": 0x0b, "red_flash": 0x0e, "red": 0x0a,
                "green_flash": 0x0d, "green": 0x09, "amber_flash": 0x07,
                "amber": 0x03, "mask": 0x0f
            },
            "PowerMax": 38.4,
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"bus": 2, "addr": 0x0d, "offset": 0x17, "way": "i2c"},
                    "Running": {"loc": "/sys/wb_plat/fan/fan4/motor1/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/wb_plat/fan/fan4/motor1/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/wb_plat/fan/fan4/motor1/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FRONT_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
                "Rotor2_config": {
                    "name": "Rotor2",
                    "Set_speed": {"bus": 2, "addr": 0x0d, "offset": 0x17, "way": "i2c"},
                    "Running": {"loc": "/sys/wb_plat/fan/fan4/motor0/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/wb_plat/fan/fan4/motor0/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/wb_plat/fan/fan4/motor0/speed", "way": "sysfs"},
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
            "warm": 0,
            
        },
        {
            "name": "CONNECT_CPLD",
            "cpld_id": "CPLD2",
            "VersionFile": {"loc": "/dev/cpld1", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for base functions",
            "slot": 0,
            "warm": 0,
        },
        {
            "name": "CONNECT_CPLD-FAN",
            "cpld_id": "CPLD3",
            "VersionFile": {"loc": "/dev/cpld2", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for fan modules",
            "slot": 0,
            "warm": 0,
        },
        {
            "name": "MAC_CPLD1",
            "cpld_id": "CPLD4",
            "VersionFile": {"loc": "/dev/cpld3", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for sff modules",
            "slot": 0,
            "warm": 0,
        },
        {
            "name": "MAC_CPLD2",
            "cpld_id": "CPLD5",
            "VersionFile": {"loc": "/dev/cpld4", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for sff modules",
            "slot": 0,
            "warm": 0,
        },
        {
            "name": "FPGA",
            "cpld_id": "CPLD6",
            "VersionFile": {"loc": "/dev/fpga0", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for base functions",
            "slot": 0,
            "format": "little_endian",
            "warm": 0,
        },
        {
            "name": "BIOS",
            "cpld_id": "CPLD7",
            "VersionFile": {"cmd": "dmidecode -s bios-version", "way": "cmd"},
            "desc": "Performs initialization of hardware components during booting",
            "slot": 0,
            "type": "str",
            "warm": 0,
        },
    ],
    "dcdc": [
        {
            "name": "Switch_ZSFP1_3v3_C",
            "dcdc_id": "DCDC1",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/7-0040/hwmon/hwmon*/curr1_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "A",
            "Max": 22000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "Switch_QSFP1_3v3_C",
            "dcdc_id": "DCDC2",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/7-0040/hwmon/hwmon*/curr3_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "A",
            "Max": 22000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "Switch_5v0_C",
            "dcdc_id": "DCDC3",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/7-0040/hwmon/hwmon*/curr2_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "A",
            "Max": 1000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "Switch_ZSFP1_3v3_V",
            "dcdc_id": "DCDC4",
            "Min": 2640,
            "value": {
                "loc": "/sys/bus/i2c/devices/7-0040/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3960,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "Switch_QSFP1_3v3_V",
            "dcdc_id": "DCDC5",
            "Min": 2640,
            "value": {
                "loc": "/sys/bus/i2c/devices/7-0040/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3960,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "Switch_5v0_V",
            "dcdc_id": "DCDC6",
            "Min": 4000,
            "value": {
                "loc": "/sys/bus/i2c/devices/7-0040/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 6000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "Switch_1v2_C",
            "dcdc_id": "DCDC7",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/7-0041/hwmon/hwmon*/curr1_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "A",
            "Max": 2000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "Switch_3v3_C",
            "dcdc_id": "DCDC8",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/7-0041/hwmon/hwmon*/curr2_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "A",
            "Max": 1000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "Switch_Cpld_3v3_C",
            "dcdc_id": "DCDC9",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/7-0041/hwmon/hwmon*/curr3_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "A",
            "Max": 2000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "Switch_1v2_V",
            "dcdc_id": "DCDC10",
            "Min": 960,
            "value": {
                "loc": "/sys/bus/i2c/devices/7-0041/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1440,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "Switch_3v3_V",
            "dcdc_id": "DCDC11",
            "Min": 2640,
            "value": {
                "loc": "/sys/bus/i2c/devices/7-0041/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3960,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "Switch_Cpld_3v3_V",
            "dcdc_id": "DCDC12",
            "Min": 2640,
            "value": {
                "loc": "/sys/bus/i2c/devices/7-0041/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3960,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "Con_1v2_C",
            "dcdc_id": "DCDC13",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/7-0042/hwmon/hwmon*/curr1_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "A",
            "Max": 1300,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "Con_3v3_C",
            "dcdc_id": "DCDC14",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/7-0042/hwmon/hwmon*/curr2_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "A",
            "Max": 2800,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "Con_SSD_3v3_C",
            "dcdc_id": "DCDC15",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/7-0042/hwmon/hwmon*/curr3_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "A",
            "Max": 4500,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "Con_1v2_V",
            "dcdc_id": "DCDC16",
            "Min": 960,
            "value": {
                "loc": "/sys/bus/i2c/devices/7-0042/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1440,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "Con_3v3_V",
            "dcdc_id": "DCDC17",
            "Min": 2640,
            "value": {
                "loc": "/sys/bus/i2c/devices/7-0042/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3960,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "Con_SSD_3v3_V",
            "dcdc_id": "DCDC18",
            "Min": 2640,
            "value": {
                "loc": "/sys/bus/i2c/devices/7-0042/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3960,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "Cpu_3v3_C",
            "dcdc_id": "DCDC19",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/7-0043/hwmon/hwmon*/curr1_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "A",
            "Max": 4686,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "Cpu_5v_C",
            "dcdc_id": "DCDC20",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/7-0043/hwmon/hwmon*/curr2_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "A",
            "Max": 2200,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "Cpu_1v7_C",
            "dcdc_id": "DCDC21",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/7-0043/hwmon/hwmon*/curr3_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "A",
            "Max": 2200,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "Cpu_3v3_V",
            "dcdc_id": "DCDC22",
            "Min": 2640,
            "value": {
                "loc": "/sys/bus/i2c/devices/7-0043/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3960,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "Cpu_5v_V",
            "dcdc_id": "DCDC23",
            "Min": 4000,
            "value": {
                "loc": "/sys/bus/i2c/devices/7-0043/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 6000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "Cpu_1v7_V",
            "dcdc_id": "DCDC24",
            "Min": 1360,
            "value": {
                "loc": "/sys/bus/i2c/devices/7-0043/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 2040,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "Cpu_CORE_C",
            "dcdc_id": "DCDC25",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/7-0060/hwmon/hwmon*/curr1_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "A",
            "Max": 47300,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "Cpu_1v05_C",
            "dcdc_id": "DCDC26",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/7-0060/hwmon/hwmon*/curr2_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "A",
            "Max": 15400,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "Cpu_CORE_V",
            "dcdc_id": "DCDC27",
            "Min": 1456,
            "value": {
                "loc": "/sys/bus/i2c/devices/7-0060/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 2184,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "Cpu_1v05_V",
            "dcdc_id": "DCDC28",
            "Min": 840,
            "value": {
                "loc": "/sys/bus/i2c/devices/7-0060/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1260,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "Switch_CORE_C",
            "dcdc_id": "DCDC29",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/7-0064/hwmon/hwmon*/curr2_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "A",
            "Max": 220000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "Switch_ANALOG_C",
            "dcdc_id": "DCDC30",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/7-0064/hwmon/hwmon*/curr3_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "A",
            "Max": 18000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "Switch_CORE_V",
            "dcdc_id": "DCDC31",
            "Min": 600,
            "value": {
                "loc": "/sys/bus/i2c/devices/7-0064/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1200,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "Switch_ANALOG_V",
            "dcdc_id": "DCDC32",
            "Min": 640,
            "value": {
                "loc": "/sys/bus/i2c/devices/7-0064/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 960,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "Cpu_1v2_C",
            "dcdc_id": "DCDC33",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/7-006c/hwmon/hwmon*/curr1_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "A",
            "Max": 9900,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "Cpu_2v23_C",
            "dcdc_id": "DCDC34",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/7-006c/hwmon/hwmon*/curr2_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "A",
            "Max": 2200,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "Cpu_1v2_V",
            "dcdc_id": "DCDC35",
            "Min": 960,
            "value": {
                "loc": "/sys/bus/i2c/devices/7-006c/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1440,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "Cpu_2v23_V",
            "dcdc_id": "DCDC36",
            "Min": 1784,
            "value": {
                "loc": "/sys/bus/i2c/devices/7-006c/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 2676,
            "format": "float(float(%s)/1000)",
        },
    ],
    "cpu": [
        {
            "name": "cpu",
            "reboot_cause_path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"
        }
    ],
    "sfps": {
        "ver": '1.0',
        "port_index_start": 1,
        "port_num": 56,
        "log_level": 2,
        "eeprom_retry_times": 5,
        "eeprom_retry_break_sec": 0.2,
        "presence_cpld": {
            "dev_id": {
                3: {
                    "offset": {
                        0x30: "1-8",
                        0x31: "9-16",
                        0x32: "17-24",
                    },
                },
                4: {
                    "offset": {
                        0x30: "25-32",
                        0x31: "33-40",
                        0x32: "41-48",
                        0x33: "49-56",
                    },
                },
            },
        },
        "presence_val_is_present": 0,
        "eeprom_path": "/sys/bus/i2c/devices/i2c-%d/%d-0050/eeprom",
        "eeprom_path_key": list(range(32, 88)),
        "optoe_driver_path": "/sys/bus/i2c/devices/i2c-%d/%d-0050/dev_class",
        "optoe_driver_key": list(range(32, 88)),
        "txdis_cpld": {
            "dev_id": {
                3: {
                    "offset": {
                        0x60: "1-8",
                        0x61: "9-16",
                        0x62: "17-24",
                    },
                },
                4: {
                    "offset": {
                        0x60: "25-32",
                        0x61: "33-40",
                        0x62: "41-48",
                    },
                },
            },
        },
        "txdisable_val_is_on": 0,
        "reset_cpld": {
            "dev_id": {
                4: {
                    "offset": {
                        0xb9: "49-56",
                    },
                },
            },
        },
        "reset_val_is_reset": 0,
    }
}
