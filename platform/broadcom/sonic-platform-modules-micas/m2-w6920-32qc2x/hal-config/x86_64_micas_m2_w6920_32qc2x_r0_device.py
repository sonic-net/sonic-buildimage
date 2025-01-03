# coding:utf-8

psu_fan_airflow = {
    "intake": ['GW-CRPS1300D', 'DPS-1300AB-6 F', 'DPS-1300AB-6 S'],
    "exhaust": ['CRPS1300D3R', 'DPS-1300AB-11 C']
}

fanairflow = {
    "intake": ['M1HFAN IV-F'],
    "exhaust": ['M1HFAN IV-R']
}

psu_display_name = {
    "PA1300I-F": ['GW-CRPS1300D', 'DPS-1300AB-6 F', 'DPS-1300AB-6 S'],
    "PA1300I-R": ['CRPS1300D3R', 'DPS-1300AB-11 C']
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
    PSU_TEMP_MIN = -20 * 1000
    PSU_TEMP_MAX = 60 * 1000

    PSU_FAN_SPEED_MIN = 3000
    PSU_FAN_SPEED_MAX = 30000

    PSU_OUTPUT_VOLTAGE_MIN = 11 * 1000
    PSU_OUTPUT_VOLTAGE_MAX = 13 * 1000

    PSU_AC_INPUT_VOLTAGE_MIN = 200 * 1000
    PSU_AC_INPUT_VOLTAGE_MAX = 240 * 1000

    PSU_DC_INPUT_VOLTAGE_MIN = 190 * 1000
    PSU_DC_INPUT_VOLTAGE_MAX = 290 * 1000

    ERR_VALUE = -9999999

    PSU_OUTPUT_POWER_MIN = 5 * 1000 * 1000
    PSU_OUTPUT_POWER_MAX = 1300 * 1000 * 1000

    PSU_INPUT_POWER_MIN = 5 * 1000 * 1000
    PSU_INPUT_POWER_MAX = 1400* 1000 * 1000

    PSU_OUTPUT_CURRENT_MIN = 1 * 1000
    PSU_OUTPUT_CURRENT_MAX = 107 * 1000

    PSU_INPUT_CURRENT_MIN = 0.05 * 1000
    PSU_INPUT_CURRENT_MAX = 12 * 1000

    FRONT_FAN_SPEED_MAX = 32500
    REAR_FAN_SPEED_MAX = 30500
    FAN_SPEED_MIN = 7248


class Description:
    CPLD = "Used for managing IO modules, SFP+ modules and system LEDs"
    BIOS = "Performs initialization of hardware components during booting"
    FPGA = "Platform management controller for on-board temperature monitoring, in-chassis power"


devices = {
    "onie_e2": [
        {
            "name": "ONIE_E2",
            "e2loc": {"loc": "/sys/bus/i2c/devices/2-0056/eeprom", "way": "sysfs"},
            "airflow": "intake"
        },
    ],
    "psus": [
        {
            "e2loc": {"loc": "/sys/bus/i2c/devices/59-0050/eeprom", "way": "sysfs"},
            "pmbusloc": {"bus": 59, "addr": 0x58, "way": "i2c"},
            "present": {"loc": "/sys/wb_plat/psu/psu1/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "name": "PSU1",
            "psu_display_name": psu_display_name,
            "airflow": psu_fan_airflow,
            "TempStatus": {"bus": 59, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0004},
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-59/59-0058/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": threshold.PSU_TEMP_MIN,
                "Max": threshold.PSU_TEMP_MAX,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            },
            "FanStatus": {"bus": 59, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0400},
            "FanSpeed": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-59/59-0058/hwmon/hwmon*/fan1_input", "way": "sysfs"},
                "Min": threshold.PSU_FAN_SPEED_MIN,
                "Max": threshold.PSU_FAN_SPEED_MAX,
                "Unit": Unit.Speed
            },
            "psu_fan_tolerance": 40,
            "InputsStatus": {"bus": 59, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x2000},
            "InputsType": {"bus": 59, "addr": 0x58, "offset": 0x80, "way": "i2c", 'psutypedecode': psutypedecode},
            "InputsVoltage": {
                'AC': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-59/59-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_AC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_AC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"

                },
                'DC': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-59/59-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_DC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_DC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                },
                'other': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-59/59-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.ERR_VALUE,
                    "Max": threshold.ERR_VALUE,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                }
            },
            "InputsCurrent": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-59/59-0058/hwmon/hwmon*/curr1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_CURRENT_MIN,
                "Max": threshold.PSU_INPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "InputsPower": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-59/59-0058/hwmon/hwmon*/power1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_POWER_MIN,
                "Max": threshold.PSU_INPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
            "OutputsStatus": {"bus": 59, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x8800},
            "OutputsVoltage": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-59/59-0058/hwmon/hwmon*/in2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_VOLTAGE_MIN,
                "Max": threshold.PSU_OUTPUT_VOLTAGE_MAX,
                "Unit": Unit.Voltage,
                "format": "float(float(%s)/1000)"
            },
            "OutputsCurrent": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-59/59-0058/hwmon/hwmon*/curr2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_CURRENT_MIN,
                "Max": threshold.PSU_OUTPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "OutputsPower": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-59/59-0058/hwmon/hwmon*/power2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_POWER_MIN,
                "Max": threshold.PSU_OUTPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
        },
        {
            "e2loc": {"loc": "/sys/bus/i2c/devices/58-0050/eeprom", "way": "sysfs"},
            "pmbusloc": {"bus": 58, "addr": 0x58, "way": "i2c"},
            "present": {"loc": "/sys/wb_plat/psu/psu2/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "name": "PSU2",
            "psu_display_name": psu_display_name,
            "airflow": psu_fan_airflow,
            "TempStatus": {"bus": 58, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0004},
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-58/58-0058/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": threshold.PSU_TEMP_MIN,
                "Max": threshold.PSU_TEMP_MAX,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            },
            "FanStatus": {"bus": 58, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0400},
            "FanSpeed": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-58/58-0058/hwmon/hwmon*/fan1_input", "way": "sysfs"},
                "Min": threshold.PSU_FAN_SPEED_MIN,
                "Max": threshold.PSU_FAN_SPEED_MAX,
                "Unit": Unit.Speed
            },
            "InputsStatus": {"bus": 58, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x2000},
            "InputsType": {"bus": 58, "addr": 0x58, "offset": 0x80, "way": "i2c", 'psutypedecode': psutypedecode},
            "InputsVoltage": {
                'AC': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-58/58-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_AC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_AC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"

                },
                'DC': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-58/58-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_DC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_DC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                },
                'other': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-58/58-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.ERR_VALUE,
                    "Max": threshold.ERR_VALUE,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                }
            },
            "InputsCurrent":
            {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-58/58-0058/hwmon/hwmon*/curr1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_CURRENT_MIN,
                "Max": threshold.PSU_INPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "InputsPower":
            {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-58/58-0058/hwmon/hwmon*/power1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_POWER_MIN,
                "Max": threshold.PSU_INPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
            "OutputsStatus": {"bus": 58, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x8800},
            "OutputsVoltage":
            {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-58/58-0058/hwmon/hwmon*/in2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_VOLTAGE_MIN,
                "Max": threshold.PSU_OUTPUT_VOLTAGE_MAX,
                "Unit": Unit.Voltage,
                "format": "float(float(%s)/1000)"
            },
            "OutputsCurrent":
            {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-58/58-0058/hwmon/hwmon*/curr2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_CURRENT_MIN,
                "Max": threshold.PSU_OUTPUT_CURRENT_MAX,
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "OutputsPower":
            {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-58/58-0058/hwmon/hwmon*/power2_input", "way": "sysfs"},
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
                    {"loc": "/sys/bus/i2c/devices/57-004f/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                    {"loc": "/sys/bus/i2c/devices/57-004e/hwmon/hwmon*/temp1_input", "way": "sysfs"}
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
                    {"loc": "/sys/bus/i2c/devices/56-004b/hwmon/hwmon*/temp1_input", "way": "sysfs"},
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
                "value": [
                    {"loc": "/sys/bus/i2c/devices/53-0048/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                    {"loc": "/sys/bus/i2c/devices/53-0049/hwmon/hwmon*/temp1_input", "way": "sysfs"}
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
            "name": "SWITCH_TEMP",
            "temp_id": "TEMP5",
            "api_name": "ASIC_TEMP",
            "Temperature": {
                "value": [
                    {"loc": "/sys/bus/i2c/devices/56-004c/hwmon/hwmon*/temp2_input", "way": "sysfs"},
                    {"loc": "/sys/bus/i2c/devices/57-004c/hwmon/hwmon*/temp2_input", "way": "sysfs"}
                ],
                "Min": 2000,
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
                "value": {"loc": "/sys/bus/i2c/devices/59-0058/hwmon/hwmon*/temp1_input", "way": "sysfs"},
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
            "temp_id": "TEMP7",
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/58-0058/hwmon/hwmon*/temp1_input", "way": "sysfs"},
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
            "led": {"bus": 26, "addr": 0x2d, "offset": 0x40, "way": "i2c"},
            "led_attrs": {
                "green": 0x06, "red": 0x05, "amber": 0x03, "default": 0x06,
                "flash": 0xff, "light": 0xff, "off": 0, "mask": 0x0f
            },
        },
        {
            "name": "BOARD_FAN_LED",
            "led_type": "FAN_LED",
            "led": {"bus": 26, "addr": 0x2d, "offset": 0x42, "way": "i2c"},
            "led_attrs": {
                "green": 0x06, "red": 0x05, "amber": 0x03, "default": 0x06,
                "flash": 0xff, "light": 0xff, "off": 0, "mask": 0x0f
            },
        },
        {
            "name": "BOARD_PSU_LED",
            "led_type": "PSU_LED",
            "led": {"bus": 26, "addr": 0x2d, "offset": 0x43, "way": "i2c"},
            "led_attrs": {
                "green": 0x06, "red": 0x05, "amber": 0x03, "default": 0x06,
                "flash": 0xff, "light": 0xff, "off": 0, "mask": 0x0f
            },
        },
    ],
    "fans": [
        {
            "name": "FAN1",
            "airflow": fanairflow,
            "e2loc": {'loc': '/sys/bus/i2c/devices/52-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "present": {"loc": "/sys/wb_plat/fan/fan1/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 28, "addr": 0x3d, "offset": 0x41, "way": "i2c"},
            "led_attrs": {
                "green": 0x04, "red": 0x02, "amber": 0x06, "default": 0x04,
                "flash": 0xff, "light": 0xff, "off": 0xff, "mask": 0x07
            },
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                    "Set_speed": {"bus": 28, "addr": 0x3d, "offset": 0x65, "way": "i2c"},
                    "Running": {"loc": "/sys/wb_plat/fan/fan1/motor0/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/wb_plat/fan/fan1/motor0/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "Speed": {
                        "value": {"loc": "/sys/wb_plat/fan/fan1/motor0/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FRONT_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
                "Rotor2_config": {
                    "name": "Rotor2",
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                    "Set_speed": {"bus": 28, "addr": 0x3d, "offset": 0x65, "way": "i2c"},
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
            "e2loc": {'loc': '/sys/bus/i2c/devices/51-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "present": {"loc": "/sys/wb_plat/fan/fan2/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 28, "addr": 0x3d, "offset": 0x40, "way": "i2c"},
            "led_attrs": {
                "green": 0x04, "red": 0x02, "amber": 0x06, "default": 0x04,
                "flash": 0xff, "light": 0xff, "off": 0xff, "mask": 0x07
            },
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                    "Set_speed": {"bus": 28, "addr": 0x3d, "offset": 0x64, "way": "i2c"},
                    "Running": {"loc": "/sys/wb_plat/fan/fan2/motor0/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/wb_plat/fan/fan2/motor0/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "Speed": {
                        "value": {"loc": "/sys/wb_plat/fan/fan2/motor0/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FRONT_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
                "Rotor2_config": {
                    "name": "Rotor2",
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                    "Set_speed": {"bus": 28, "addr": 0x3d, "offset": 0x64, "way": "i2c"},
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
            "e2loc": {'loc': '/sys/bus/i2c/devices/50-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "present": {"loc": "/sys/wb_plat/fan/fan3/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 28, "addr": 0x3d, "offset": 0x3f, "way": "i2c"},
            "led_attrs": {
                "green": 0x04, "red": 0x02, "amber": 0x06, "default": 0x04,
                "flash": 0xff, "light": 0xff, "off": 0xff, "mask": 0x07
            },
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                    "Set_speed": {"bus": 28, "addr": 0x3d, "offset": 0x63, "way": "i2c"},
                    "Running": {"loc": "/sys/wb_plat/fan/fan3/motor0/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/wb_plat/fan/fan3/motor0/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "Speed": {
                        "value": {"loc": "/sys/wb_plat/fan/fan3/motor0/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FRONT_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
                "Rotor2_config": {
                    "name": "Rotor2",
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                    "Set_speed": {"bus": 28, "addr": 0x3d, "offset": 0x63, "way": "i2c"},
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
            "e2loc": {'loc': '/sys/bus/i2c/devices/49-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "present": {"loc": "/sys/wb_plat/fan/fan4/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 28, "addr": 0x3d, "offset": 0x3e, "way": "i2c"},
            "led_attrs": {
                "green": 0x04, "red": 0x02, "amber": 0x06, "default": 0x04,
                "flash": 0xff, "light": 0xff, "off": 0xff, "mask": 0x07
            },
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                    "Set_speed": {"bus": 28, "addr": 0x3d, "offset": 0x62, "way": "i2c"},
                    "Running": {"loc": "/sys/wb_plat/fan/fan4/motor0/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/wb_plat/fan/fan4/motor0/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "Speed": {
                        "value": {"loc": "/sys/wb_plat/fan/fan4/motor0/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FRONT_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
                "Rotor2_config": {
                    "name": "Rotor2",
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                    "Set_speed": {"bus": 28, "addr": 0x3d, "offset": 0x62, "way": "i2c"},
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
            "e2loc": {'loc': '/sys/bus/i2c/devices/48-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "present": {"loc": "/sys/wb_plat/fan/fan5/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 28, "addr": 0x3d, "offset": 0x3d, "way": "i2c"},
            "led_attrs": {
                "green": 0x04, "red": 0x02, "amber": 0x06, "default": 0x04,
                "flash": 0xff, "light": 0xff, "off": 0xff, "mask": 0x07
            },
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                    "Set_speed": {"bus": 28, "addr": 0x3d, "offset": 0x61, "way": "i2c"},
                    "Running": {"loc": "/sys/wb_plat/fan/fan5/motor0/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/wb_plat/fan/fan5/motor0/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "Speed": {
                        "value": {"loc": "/sys/wb_plat/fan/fan5/motor0/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FRONT_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
                "Rotor2_config": {
                    "name": "Rotor2",
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                    "Set_speed": {"bus": 28, "addr": 0x3d, "offset": 0x61, "way": "i2c"},
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
            "e2loc": {'loc': '/sys/bus/i2c/devices/47-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "present": {"loc": "/sys/wb_plat/fan/fan6/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 28, "addr": 0x3d, "offset": 0x3c, "way": "i2c"},
            "led_attrs": {
                "green": 0x04, "red": 0x02, "amber": 0x06, "default": 0x04,
                "flash": 0xff, "light": 0xff, "off": 0xff, "mask": 0x07
            },
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
                    "Set_speed": {"bus": 28, "addr": 0x3d, "offset": 0x60, "way": "i2c"},
                    "Running": {"loc": "/sys/wb_plat/fan/fan6/motor0/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/wb_plat/fan/fan6/motor0/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "Speed": {
                        "value": {"loc": "/sys/wb_plat/fan/fan6/motor0/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FRONT_FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
                "Rotor2_config": {
                    "name": "Rotor2",
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.REAR_FAN_SPEED_MAX,
                    "Set_speed": {"bus": 28, "addr": 0x3d, "offset": 0x60, "way": "i2c"},
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
            "warm": 0,
        },
        {
            "name": "CTRL_CPLD",
            "cpld_id": "CPLD2",
            "VersionFile": {"loc": "/dev/cpld1", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for base functions",
            "slot": 0,
            "warm": 0,
        },
        {
            "name": "MAC_CPLDA",
            "cpld_id": "CPLD3",
            "VersionFile": {"loc": "/dev/cpld2", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for SFP+ modules",
            "slot": 0,
            "warm": 0,
        },
        {
            "name": "MAC_CPLDB",
            "cpld_id": "CPLD4",
            "VersionFile": {"loc": "/dev/cpld3", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for SFP+ modules",
            "slot": 0,
            "warm": 0,
        },
        {
            "name": "FAN_CPLD",
            "cpld_id": "CPLD5",
            "VersionFile": {"loc": "/dev/cpld4", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for fan modules",
            "slot": 0,
            "warm": 0,
        },
        {
            "name": "MAC_FPGA",
            "cpld_id": "CPLD6",
            "VersionFile": {"loc": "/dev/fpga0", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for base functions",
            "slot": 0,
            "format": "little_endian",
            "warm": 1,
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
            "name": "MAC_VCC12V_CON",
            "dcdc_id": "DCDC1",
            "Min": 11400,
            "value": {
                "loc": "/sys/bus/i2c/devices/60-005b/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 12600,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_VDD1.0V_FPGA",
            "dcdc_id": "DCDC2",
            "Min": 950,
            "value": {
                "loc": "/sys/bus/i2c/devices/60-005b/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1050,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_VDD1.8V_FPGA",
            "dcdc_id": "DCDC3",
            "Min": 1710,
            "value": {
                "loc": "/sys/bus/i2c/devices/60-005b/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1890,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_VDD1.2V_FPGA",
            "dcdc_id": "DCDC4",
            "Min": 1130,
            "value": {
                "loc": "/sys/bus/i2c/devices/60-005b/hwmon/hwmon*/in4_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1280,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_VDD3.3V",
            "dcdc_id": "DCDC5",
            "Min": 3135,
            "value": {
                "loc": "/sys/bus/i2c/devices/60-005b/hwmon/hwmon*/in5_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3465,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_DVDD3.3V_1.8V",
            "dcdc_id": "DCDC6",
            "Min": 3135,
            "value": {
                "loc": "/sys/bus/i2c/devices/60-005b/hwmon/hwmon*/in6_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3465,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_AVDD1.8V",
            "dcdc_id": "DCDC7",
            "Min": 1710,
            "value": {
                "loc": "/sys/bus/i2c/devices/60-005b/hwmon/hwmon*/in7_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1890,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_DVDD1.2V",
            "dcdc_id": "DCDC8",
            "Min": 1130,
            "value": {
                "loc": "/sys/bus/i2c/devices/60-005b/hwmon/hwmon*/in8_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1280,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_AVDD1.2V",
            "dcdc_id": "DCDC9",
            "Min": 1130,
            "value": {
                "loc": "/sys/bus/i2c/devices/60-005b/hwmon/hwmon*/in9_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1280,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_VDD3.3V_SFP+",
            "dcdc_id": "DCDC10",
            "Min": 3135,
            "value": {
                "loc": "/sys/bus/i2c/devices/60-005b/hwmon/hwmon*/in10_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3465,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_VDD3.3V_CLK",
            "dcdc_id": "DCDC11",
            "Min": 3135,
            "value": {
                "loc": "/sys/bus/i2c/devices/60-005b/hwmon/hwmon*/in11_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3465,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_VDD5V_VR",
            "dcdc_id": "DCDC12",
            "Min": 4500,
            "value": {
                "loc": "/sys/bus/i2c/devices/60-005b/hwmon/hwmon*/in13_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 5500,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_VDD_CORE",
            "dcdc_id": "DCDC13",
            "Min": 700,
            "value": {
                "loc": "/sys/bus/i2c/devices/60-005b/hwmon/hwmon*/in14_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 980,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_VDD_ANALOG",
            "dcdc_id": "DCDC14",
            "Min": 760,
            "value": {
                "loc": "/sys/bus/i2c/devices/60-005b/hwmon/hwmon*/in15_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 920,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_VDD_ANALOG1",
            "dcdc_id": "DCDC15",
            "Min": 760,
            "value": {
                "loc": "/sys/bus/i2c/devices/60-005b/hwmon/hwmon*/in16_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 920,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "BASE_VDD3.3V",
            "dcdc_id": "DCDC16",
            "Min": 3135,
            "value": {
                "loc": "/sys/bus/i2c/devices/41-005b/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3465,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "BASE_VDD12V",
            "dcdc_id": "DCDC17",
            "Min": 11400,
            "value": {
                "loc": "/sys/bus/i2c/devices/41-005b/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 12600,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "BASE_VDD3.3_STBY",
            "dcdc_id": "DCDC18",
            "Min": 3135,
            "value": {
                "loc": "/sys/bus/i2c/devices/41-005b/hwmon/hwmon*/in4_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3465,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "BASE_SSD1_VDD3.3V",
            "dcdc_id": "DCDC19",
            "Min": 3135,
            "value": {
                "loc": "/sys/bus/i2c/devices/41-005b/hwmon/hwmon*/in5_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3465,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "BASE_PHY_VDD1V0",
            "dcdc_id": "DCDC20",
            "Min": 950,
            "value": {
                "loc": "/sys/bus/i2c/devices/41-005b/hwmon/hwmon*/in6_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1100,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "BASE_OVDD_PHY_M",
            "dcdc_id": "DCDC21",
            "Min": 3135,
            "value": {
                "loc": "/sys/bus/i2c/devices/41-005b/hwmon/hwmon*/in7_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3465,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "CPU_VCCP",
            "dcdc_id": "DCDC22",
            "Min": 468,
            "value": {
                "loc": "/sys/bus/i2c/devices/42-0068/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1364,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "CPU_VNN",
            "dcdc_id": "DCDC23",
            "Min": 585,
            "value": {
                "loc": "/sys/bus/i2c/devices/42-0068/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1364,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "CPU_P1V05",
            "dcdc_id": "DCDC24",
            "Min": 945,
            "value": {
                "loc": "/sys/bus/i2c/devices/42-006e/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1155,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "CPU_VCCRAM",
            "dcdc_id": "DCDC25",
            "Min": 675,
            "value": {
                "loc": "/sys/bus/i2c/devices/42-006e/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1320,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "CPU_P1V2_VDDQ",
            "dcdc_id": "DCDC26",
            "Min": 1080,
            "value": {
                "loc": "/sys/bus/i2c/devices/42-005e/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1320,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_VDD3.3_MON",
            "dcdc_id": "DCDC27",
            "Min": 3040,
            "value": {
                "loc": "/sys/wb_plat/sensor/in3/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3560,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_QSFPDD_VDD3.3V_A",
            "dcdc_id": "DCDC28",
            "Min": 3040,
            "value": {
                "loc": "/sys/wb_plat/sensor/in1/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3560,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_QSFPDD_VDD3.3V_B",
            "dcdc_id": "DCDC29",
            "Min": 3040,
            "value": {
                "loc": "/sys/wb_plat/sensor/in2/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3560,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_VDD3.7V_CLK",
            "dcdc_id": "DCDC30",
            "Min": 3500,
            "value": {
                "loc": "/sys/wb_plat/sensor/in4/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 4000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_VDD5V_USB",
            "dcdc_id": "DCDC31",
            "Min": 4500,
            "value": {
                "loc": "/sys/wb_plat/sensor/in5/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 5500,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "FAN6_VDD12V",
            "dcdc_id": "DCDC32",
            "Min": 11400,
            "value": {
                "loc": "/sys/wb_plat/sensor/in11/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 12600,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "FAN5_VDD12V",
            "dcdc_id": "DCDC33",
            "Min": 11400,
            "value": {
                "loc": "/sys/wb_plat/sensor/in10/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 12600,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "FAN4_VDD12V",
            "dcdc_id": "DCDC34",
            "Min": 11400,
            "value": {
                "loc": "/sys/wb_plat/sensor/in9/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 12600,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "FAN3_VDD12V",
            "dcdc_id": "DCDC35",
            "Min": 11400,
            "value": {
                "loc": "/sys/wb_plat/sensor/in8/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 12600,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "FAN2_VDD12V",
            "dcdc_id": "DCDC36",
            "Min": 11400,
            "value": {
                "loc": "/sys/wb_plat/sensor/in7/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 12600,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "FAN1_VDD12V",
            "dcdc_id": "DCDC37",
            "Min": 11400,
            "value": {
                "loc": "/sys/wb_plat/sensor/in6/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 12600,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "FAN_VDD3.3V",
            "dcdc_id": "DCDC38",
            "Min": 3040,
            "value": {
                "loc": "/sys/wb_plat/sensor/in12/in_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3560,
            "format": "float(float(%s)/1000)",
        },
    ],
    "cpu": [
        {
            "name": "cpu",
            "CpuResetCntReg": {"loc": "/dev/cpld1", "offset": 0x88, "len": 1, "way": "devfile_ascii"},
            "reboot_cause_path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"
        }
    ],
    "cards": [
        {
            "name": "psu1",
            "sn": "123"
        },
        {
            "name": "psu2",
            "sn": "456"
        },
    ],
    "sfps": {
        "ver": '1.0',
        "port_index_start": 0,
        "port_num": 34,
        "log_level": 2,
        "eeprom_retry_times": 5,
        "eeprom_retry_break_sec": 0.2,
        "presence_cpld": {
            "dev_id": {
                2: {
                    "offset": {
                        0x32: "17-24",
                        0x33: "25-28,31,32,29,30",
                        0x46: "34,33",
                    },
                },
                3: {
                    "offset": {
                        0x4E: "3,4,1,2,5-8",
                        0x4F: "9-16",
                    },
                },
            },
        },
        "presence_val_is_present": 0,
        "eeprom_path": "/sys/bus/i2c/devices/i2c-%d/%d-0050/eeprom",
        "eeprom_path_key": [65,66,63,64] + list(range(67, 91)) + [93,94,91,92,96,95],
        "optoe_driver_path": "/sys/bus/i2c/devices/i2c-%d/%d-0050/dev_class",
        "optoe_driver_key": [65,66,63,64] + list(range(67, 91)) + [93,94,91,92,96,95],
        "reset_cpld": {
            "dev_id": {
                2: {
                    "offset": {
                        0x21: "17-24",
                        0x22: "25-28,31,32,29,30",
                    },
                },
                3: {
                    "offset": {
                        0x22: "3,4,1,2,5-8",
                        0x23: "9-16",
                    },
                },
            },
        },
        "reset_val_is_reset": 0,
        "txdis_cpld": {
            "dev_id": {
                2: {
                    "offset": {
                        0x50: "34,33",
                    },
                },
            },
        },
        "txdisable_val_is_on": 1,
    }
}
