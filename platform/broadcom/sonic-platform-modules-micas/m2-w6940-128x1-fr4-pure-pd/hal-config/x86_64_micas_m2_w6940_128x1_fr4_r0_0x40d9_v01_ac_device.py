#!/usr/bin/python3

# CPO 100 board modified configuration, support 2 AC PSUS

psu_fan_airflow = {
    "intake": ['DLK3000AN12C31', 'CRPS3000CL', 'ECDL3000123', 'ECD26020050'],
    "exhaust": []
}

fanairflow = {
    "intake": ['M2EFAN V-F'],
    "exhaust": [],
}

psu_display_name = {
    "PA3000I-F": ['DLK3000AN12C31', 'CRPS3000CL', 'ECDL3000123'],
    "PD3000I-F": ['ECD26020050']
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
    PSU_TEMP_MIN = -5 * 1000
    PSU_TEMP_MAX = 55 * 1000

    PSU_FAN_SPEED_MIN = 3500
    PSU_FAN_SPEED_MAX = 32950

    PSU_OUTPUT_VOLTAGE_MIN = 11 * 1000
    PSU_OUTPUT_VOLTAGE_MAX = 13 * 1000

    PSU_AC_INPUT_VOLTAGE_MIN = 90 * 1000
    PSU_AC_INPUT_VOLTAGE_MAX = 264 * 1000

    PSU_DC_INPUT_VOLTAGE_MIN = 180 * 1000
    PSU_DC_INPUT_VOLTAGE_MAX = 320 * 1000

    PSU_DC_INPUT_VOLTAGE_MIN_2 = 45 * 1000
    PSU_DC_INPUT_VOLTAGE_MAX_2 = 53 * 1000

    ERR_VALUE = -9999999

    PSU_OUTPUT_POWER_MIN = 0 * 1000 * 1000
    PSU_OUTPUT_POWER_MAX = 3000 * 1000 * 1000

    PSU_INPUT_POWER_MIN = 0 * 1000 * 1000
    PSU_INPUT_POWER_MAX = 3100 * 1000 * 1000

    PSU_OUTPUT_CURRENT_MIN = 0 * 1000
    PSU_OUTPUT_CURRENT_MAX = 246 * 1000

    PSU_DC_OUTPUT_CURRENT_MAX = 250 * 1000

    PSU_INPUT_CURRENT_MIN = 0 * 1000
    PSU_INPUT_CURRENT_MAX = 20 * 1000

    PSU_DC_INPUT_CURRENT_MAX = 80 * 1000

    FRONT_FAN_SPEED_MAX = 11700
    REAR_FAN_SPEED_MAX = 10200
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
            "e2loc": {"loc": "/sys/bus/i2c/devices/11-0050/eeprom", "way": "sysfs"},
            "e2_type": ["wedge_v5", "fru"],
            "pmbusloc": {"bus": 11, "addr": 0x58, "way": "i2c"},
            "present": {"loc": "/sys/s3ip/psu/psu1/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "name": "PSU1",
            "psu_display_name": psu_display_name,
            "psu_display_name": psu_display_name,
            "airflow": psu_fan_airflow,
            "TempStatus": {"bus": 11, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0004},
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-11/11-0058/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": threshold.PSU_TEMP_MIN,
                "Max": threshold.PSU_TEMP_MAX,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            },
            "FanStatus": {"bus": 11, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0400},
            "FanSpeed": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-11/11-0058/hwmon/hwmon*/fan1_input", "way": "sysfs"},
                "Min": threshold.PSU_FAN_SPEED_MIN,
                "Max": threshold.PSU_FAN_SPEED_MAX,
                "Unit": Unit.Speed
            },
            "psu_fan_tolerance": 40,
            "InputsStatus": {"bus": 11, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x2000},
            "InputsType": {"bus": 11, "addr": 0x58, "offset": 0x80, "way": "i2c", 'psutypedecode': psutypedecode},
            "InputsVoltage": {
                'AC': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-11/11-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_AC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_AC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"

                },
                'DC': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-11/11-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": { 
                        "other": threshold.PSU_DC_INPUT_VOLTAGE_MIN,
                        "ECD26020050": threshold.PSU_DC_INPUT_VOLTAGE_MIN_2,
                    },
                    "Max": { 
                        "other": threshold.PSU_DC_INPUT_VOLTAGE_MAX,
                        "ECD26020050": threshold.PSU_DC_INPUT_VOLTAGE_MAX_2,
                    },
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                },
                'other': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-11/11-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.ERR_VALUE,
                    "Max": threshold.ERR_VALUE,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                }
            },
            "InputsCurrent": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-11/11-0058/hwmon/hwmon*/curr1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_CURRENT_MIN,
                "Max": {
                    "other":threshold.PSU_INPUT_CURRENT_MAX,
                    "ECD26020050":threshold.PSU_DC_INPUT_CURRENT_MAX,
                },
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "InputsPower": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-11/11-0058/hwmon/hwmon*/power1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_POWER_MIN,
                "Max": threshold.PSU_INPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
            "OutputsStatus": {"bus": 11, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x8800},
            "OutputsVoltage": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-11/11-0058/hwmon/hwmon*/in2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_VOLTAGE_MIN,
                "Max": threshold.PSU_OUTPUT_VOLTAGE_MAX,
                "Unit": Unit.Voltage,
                "format": "float(float(%s)/1000)"
            },
            "OutputsCurrent": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-11/11-0058/hwmon/hwmon*/curr2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_CURRENT_MIN,
                "Max": {
                    "other":threshold.PSU_OUTPUT_CURRENT_MAX,
                    "ECD26020050":threshold.PSU_DC_OUTPUT_CURRENT_MAX,
                },
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "OutputsPower": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-11/11-0058/hwmon/hwmon*/power2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_POWER_MIN,
                "Max": threshold.PSU_OUTPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
        },
        {
            "e2loc": {"loc": "/sys/bus/i2c/devices/10-0050/eeprom", "way": "sysfs"},
            "e2_type": ["wedge_v5", "fru"],
            "pmbusloc": {"bus": 10, "addr": 0x58, "way": "i2c"},
            "present": {"loc": "/sys/s3ip/psu/psu2/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "name": "PSU2",
            "psu_display_name": psu_display_name,
            "airflow": psu_fan_airflow,
            "TempStatus": {"bus": 10, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0004},
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-10/10-0058/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": threshold.PSU_TEMP_MIN,
                "Max": threshold.PSU_TEMP_MAX,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            },
            "FanStatus": {"bus": 10, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x0400},
            "FanSpeed": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-10/10-0058/hwmon/hwmon*/fan1_input", "way": "sysfs"},
                "Min": threshold.PSU_FAN_SPEED_MIN,
                "Max": threshold.PSU_FAN_SPEED_MAX,
                "Unit": Unit.Speed
            },
            "psu_fan_tolerance": 40,
            "InputsStatus": {"bus":10, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x2000},
            "InputsType": {"bus": 10, "addr": 0x58, "offset": 0x80, "way": "i2c", 'psutypedecode': psutypedecode},
            "InputsVoltage": {
                'AC': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-10/10-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.PSU_AC_INPUT_VOLTAGE_MIN,
                    "Max": threshold.PSU_AC_INPUT_VOLTAGE_MAX,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"

                },
                'DC': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-10/10-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": { 
                        "other": threshold.PSU_DC_INPUT_VOLTAGE_MIN,
                        "ECD26020050": threshold.PSU_DC_INPUT_VOLTAGE_MIN_2,
                    },
                    "Max": { 
                        "other": threshold.PSU_DC_INPUT_VOLTAGE_MAX,
                        "ECD26020050": threshold.PSU_DC_INPUT_VOLTAGE_MAX_2,
                    },
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                },
                'other': {
                    "value": {"loc": "/sys/bus/i2c/devices/i2c-10/10-0058/hwmon/hwmon*/in1_input", "way": "sysfs"},
                    "Min": threshold.ERR_VALUE,
                    "Max": threshold.ERR_VALUE,
                    "Unit": Unit.Voltage,
                    "format": "float(float(%s)/1000)"
                }
            },
            "InputsCurrent": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-10/10-0058/hwmon/hwmon*/curr1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_CURRENT_MIN,
                "Max": {
                    "other":threshold.PSU_INPUT_CURRENT_MAX,
                    "ECD26020050":threshold.PSU_DC_INPUT_CURRENT_MAX,
                },
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "InputsPower": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-10/10-0058/hwmon/hwmon*/power1_input", "way": "sysfs"},
                "Min": threshold.PSU_INPUT_POWER_MIN,
                "Max": threshold.PSU_INPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
            "OutputsStatus": {"bus": 10, "addr": 0x58, "offset": 0x79, "way": "i2cword", "mask": 0x8800},
            "OutputsVoltage": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-10/10-0058/hwmon/hwmon*/in2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_VOLTAGE_MIN,
                "Max": threshold.PSU_OUTPUT_VOLTAGE_MAX,
                "Unit": Unit.Voltage,
                "format": "float(float(%s)/1000)"
            },
            "OutputsCurrent": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-10/10-0058/hwmon/hwmon*/curr2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_CURRENT_MIN,
                "Max": {
                    "other":threshold.PSU_OUTPUT_CURRENT_MAX,
                    "ECD26020050":threshold.PSU_DC_OUTPUT_CURRENT_MAX,
                },
                "Unit": Unit.Current,
                "format": "float(float(%s)/1000)"
            },
            "OutputsPower": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-10/10-0058/hwmon/hwmon*/power2_input", "way": "sysfs"},
                "Min": threshold.PSU_OUTPUT_POWER_MIN,
                "Max": threshold.PSU_OUTPUT_POWER_MAX,
                "Unit": Unit.Power,
                "format": "float(float(%s)/1000000)"
            },
        },
    ],
    "temps": [
        {
            "name": "CPU_TEMP",
            "temp_id": "TEMP1",
            "api_name": "CPU",
            "Temperature": {
                "value": {"loc": "/sys/bus/platform/devices/coretemp.0/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": -30000,
                "Low": -10000,
                "High": 95000,
                "Max": 99000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
        {
            "name": "INLET_TEMP",
            "temp_id": "TEMP2",
            "api_name": "Inlet",
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/35-004d/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": -37000,
                "Low": -27000,
                "High": 50000,
                "Max": 55000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)-1"
            }
        },
        {
            "name": "OUTLET_TEMP",
            "temp_id": "TEMP3",
            "api_name": "Outlet",
            "Temperature": {
                "value": [
                    {"loc": "/sys/bus/i2c/devices/8-0048/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                    {"loc": "/sys/bus/i2c/devices/8-0049/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                    {"loc": "/sys/bus/i2c/devices/9-0048/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                    {"loc": "/sys/bus/i2c/devices/9-0049/hwmon/hwmon*/temp1_input", "way": "sysfs"},
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
            "name": "BOARD_TEMP",
            "temp_id": "TEMP4",
            "api_name": "Board",
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/14-0048/hwmon/*/temp1_input", "way": "sysfs"},
                "Min": -30000,
                "Low": -10000,
                "High": 70000,
                "Max": 75000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
        {
            "name": "SWITCH_TEMP",
            "temp_id": "TEMP5",
            "api_name": "ASIC_TEMP",
            "Temperature": {
                "value": {"loc": "/sys/s3ip/temp_sensor/temp2/value", "way": "sysfs"},
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
            "name": "PSU1_TEMP",
            "temp_id": "TEMP6",
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-11/11-0058/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": -10000,
                "Low": -5000,
                "High": 50000,
                "Max": 55000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
        {
            "name": "PSU2_TEMP",
            "temp_id": "TEMP7",
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/i2c-10/10-0058/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": -10000,
                "Low": -5000,
                "High": 50000,
                "Max": 55000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
        {
            "name": "OE_TEMP",
            "temp_id": "TEMP8",
            "api_name": "OE_TEMP",
            "Temperature": {
                "value": {"loc": "/etc/sonic/highest_oe_temp", "way": "sysfs", "flock_path": "/etc/sonic/highest_oe_temp"},
                "Min": -30000,
                "Low": -20000,
                "High": 95000,
                "Max": 100000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            },
            "invalid": -100000,
            "error": -99999,
        },
        {
            "name": "RLM_TEMP",
            "temp_id": "TEMP9",
            "api_name": "RLM_TEMP",
            "Temperature": {
                "value": {"loc": "/etc/sonic/highest_rlm_temp", "way": "sysfs", "flock_path": "/etc/sonic/highest_rlm_temp"},
                "Min": -40000,
                "Low": -30000,
                "High": 50000,
                "Max": 55000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            },
            "invalid": -100000,
            "error": -99999,
        },
        {
            "name": "MOS_TEMP",
            "temp_id": "TEMP10",
            "api_name": "MOSFET_TEMP",
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/64-0040/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": -30000,
                "Low": -20000,
                "High": 110000,
                "Max": 125000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
        {
            "name": "Tout-Tin",
            "Temperature": {
                "value": {
                    "val_conf_list": [
                        {"loc": "/sys/bus/i2c/devices/8-0048/hwmon/hwmon*/temp1_input", "way": "sysfs", "fail_val":-99999000},
                        {"loc": "/sys/bus/i2c/devices/8-0049/hwmon/hwmon*/temp1_input", "way": "sysfs", "fail_val":-99999000},
                        {"loc": "/sys/bus/i2c/devices/9-0048/hwmon/hwmon*/temp1_input", "way": "sysfs", "fail_val":-99999000},
                        {"loc": "/sys/bus/i2c/devices/9-0049/hwmon/hwmon*/temp1_input", "way": "sysfs", "fail_val":-99999000},
                        {"loc": "/sys/bus/i2c/devices/35-004d/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                    ],
                    "fail_conf_set_list": [
                        {0,1,2,3},
                    ],
                    "format": "float(float(max(%s,%s,%s,%s)-float(%s))/1000)+3"
                },
                #"Min": -10000,
                #"Low": 0,
                "High": 15000,
                "Max": 20000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
    ],
    "leds": [
        {
            "name": "FRONT_SYS_LED",
            "led_type": "SYS_LED",
            "led": {"loc": "/dev/cpld1", "offset": 0x50, "len": 1, "way": "devfile"},
            "led_attrs": {
                "off": 0x00, "green": 0x02, "amber": 0x06, "red": 0x06, "mask": 0x07,
                "flash": 0x01, "amber_flash": 0x05,
            },
            "led_map": {
                0x00: "off", 0x02: "blue", 0x06: "amber", 0x01: "blue/amber alternate flashing",
                0x05: "amber flashing"
            }
        },
        {
            "name": "FRONT_PSU_LED",
            "led_type": "PSU_LED",
            "led": {"loc": "/dev/cpld1", "offset": 0x51, "len": 1, "way": "devfile"},
            "led_attrs": {
                "off": 0x00, "green": 0x02, "amber": 0x06, "red": 0x06, "mask": 0x07
            },
            "led_map": {
                0x00: "off", 0x02: "blue", 0x06: "amber"
            }
        },
        {
            "name": "FRONT_SMB_LED",
            "led_type": "SMB_LED",
            
            "led": {"loc": "/dev/cpld1", "offset": 0x53, "len": 1, "way": "devfile"},
            "led_attrs": {
                "off": 0x00, "green": 0x02, "amber": 0x06, "red": 0x06, "mask": 0x07
            },
            "led_map": {
                0x00: "off", 0x02: "blue", 0x06: "amber"
            }
        },
        {
            "name": "FRONT_FAN_LED",
            "led_type": "FAN_LED",
            "led": {"loc": "/dev/cpld1", "offset": 0x52, "len": 1, "way": "devfile"},
            "led_attrs": {
                "off": 0x00, "green": 0x02, "amber": 0x06, "red": 0x06, "mask": 0x07
            },
            "led_map": {
                0x00: "off", 0x02: "blue", 0x06: "amber"
            }
        },
        {
            "name": "SCM_FRU_LED",
            "led_type": "SMB_LED",
            "led": {"loc": "/dev/cpld1", "offset": 0x54, "len": 1, "way": "devfile"},
            "led_attrs": {
                "off": 0x00, "green": 0x02, "amber": 0x06, "red": 0x06, "mask": 0x07,
                "amber_flash": 0x05,
            },
            "led_map": {
                0x00: "off", 0x02: "blue", 0x06: "amber", 0x05: "amber flashing"
            }
        },
        {
            "name": "SMB_FRU_LED",
            "led_type": "SMB_LED",
            "led": {"loc": "/dev/fpga1", "offset": 0x2c, "len": 1, "way": "devfile"},
            "led_attrs": {
                "off": 0x07, "green": 0x03, "amber": 0x04, "red": 0x04, "mask": 0xf,
                "amber_flash": 0x0c,
            },
            "led_map": {
                0x07: "off", 0x03: "blue", 0x04: "amber", 0x0c: "amber flashing"
            }
        },
    ],
    "fans": [
        {
            "name": "FAN7",
            "airflow": fanairflow,
            "e2loc": {'loc': '/sys/bus/i2c/devices/i2c-43/43-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "e2_type": ["wedge_v5", "fru"],
            "present": {"loc": "/sys/s3ip/fan/fan7/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 44, "addr": 0x0d, "offset": 0x60, "way": "i2c"},
            "led_attrs": {
                "off": 0x00, "green": 0x06, "amber": 0x01, "red": 0x01, "mask": 0x07
            },
            "led_map": {
                0x00: "off", 0x06: "blue", 0x01: "amber"
            },
            "PowerMax": 80.64,
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"bus": 44, "addr": 0x0d, "offset": 0x90, "way": "i2c"},
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
                    "Set_speed": {"bus": 44, "addr": 0x0d, "offset": 0x90, "way": "i2c"},
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
            "name": "FAN5",
            "airflow": fanairflow,
            "e2loc": {'loc': '/sys/bus/i2c/devices/i2c-42/42-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "e2_type": ["wedge_v5", "fru"],
            "present": {"loc": "/sys/s3ip/fan/fan5/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 44, "addr": 0x0d, "offset": 0x61, "way": "i2c"},
            "led_attrs": {
                "off": 0x00, "green": 0x06, "amber": 0x01, "red": 0x01, "mask": 0x07
            },
            "led_map": {
                0x00: "off", 0x06: "blue", 0x01: "amber"
            },
            "PowerMax": 80.64,
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"bus": 44, "addr": 0x0d, "offset": 0x91, "way": "i2c"},
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
                    "Set_speed": {"bus": 44, "addr": 0x0d, "offset": 0x91, "way": "i2c"},
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
            "name": "FAN3",
            "airflow": fanairflow,
            "e2loc": {'loc': '/sys/bus/i2c/devices/i2c-41/41-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "e2_type": ["wedge_v5", "fru"],
            "present": {"loc": "/sys/s3ip/fan/fan3/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 44, "addr": 0x0d, "offset": 0x62, "way": "i2c"},
            "led_attrs": {
                "off": 0x00, "green": 0x06, "amber": 0x01, "red": 0x01, "mask": 0x07
            },
            "led_map": {
                0x00: "off", 0x06: "blue", 0x01: "amber"
            },
            "PowerMax": 80.64,
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"bus": 44, "addr": 0x0d, "offset": 0x92, "way": "i2c"},
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
                    "Set_speed": {"bus": 44, "addr": 0x0d, "offset": 0x92, "way": "i2c"},
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
            "name": "FAN1",
            "airflow": fanairflow,
            "e2loc": {'loc': '/sys/bus/i2c/devices/i2c-40/40-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "e2_type": ["wedge_v5", "fru"],
            "present": {"loc": "/sys/s3ip/fan/fan1/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 44, "addr": 0x0d, "offset": 0x63, "way": "i2c"},
            "led_attrs": {
                "off": 0x00, "green": 0x06, "amber": 0x01, "red": 0x01, "mask": 0x07
            },
            "led_map": {
                0x00: "off", 0x06: "blue", 0x01: "amber"
            },
            "PowerMax": 80.64,
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"bus": 44, "addr": 0x0d, "offset": 0x93, "way": "i2c"},
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
                    "Set_speed": {"bus": 44, "addr": 0x0d, "offset": 0x93, "way": "i2c"},
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
            "name": "FAN8",
            "airflow": fanairflow,
            "e2loc": {'loc': '/sys/bus/i2c/devices/i2c-51/51-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "e2_type": ["wedge_v5", "fru"],
            "present": {"loc": "/sys/s3ip/fan/fan8/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 52, "addr": 0x0d, "offset": 0x60, "way": "i2c"},
            "led_attrs": {
                "off": 0x00, "green": 0x06, "amber": 0x01, "red": 0x01, "mask": 0x07
            },
            "led_map": {
                0x00: "off", 0x06: "blue", 0x01: "amber"
            },
            "PowerMax": 80.64,
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"bus": 52, "addr": 0x0d, "offset": 0x90, "way": "i2c"},
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
                    "Set_speed": {"bus": 52, "addr": 0x0d, "offset": 0x90, "way": "i2c"},
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
        },
        {
            "name": "FAN6",
            "airflow": fanairflow,
            "e2loc": {'loc': '/sys/bus/i2c/devices/i2c-50/50-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "e2_type": ["wedge_v5", "fru"],
            "present": {"loc": "/sys/s3ip/fan/fan6/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 52, "addr": 0x0d, "offset": 0x61, "way": "i2c"},
            "led_attrs": {
                "off": 0x00, "green": 0x06, "amber": 0x01, "red": 0x01, "mask": 0x07
            },
            "led_map": {
                0x00: "off", 0x06: "blue", 0x01: "amber"
            },
            "PowerMax": 80.64,
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"bus": 52, "addr": 0x0d, "offset": 0x91, "way": "i2c"},
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
                    "Set_speed": {"bus": 52, "addr": 0x0d, "offset": 0x91, "way": "i2c"},
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
            "name": "FAN4",
            "airflow": fanairflow,
            "e2loc": {'loc': '/sys/bus/i2c/devices/i2c-49/49-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "e2_type": ["wedge_v5", "fru"],
            "present": {"loc": "/sys/s3ip/fan/fan4/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 52, "addr": 0x0d, "offset": 0x62, "way": "i2c"},
            "led_attrs": {
                "off": 0x00, "green": 0x06, "amber": 0x01, "red": 0x01, "mask": 0x07
            },
            "led_map": {
                0x00: "off", 0x06: "blue", 0x01: "amber"
            },
            "PowerMax": 80.64,
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"bus": 52, "addr": 0x0d, "offset": 0x92, "way": "i2c"},
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
                    "Set_speed": {"bus": 52, "addr": 0x0d, "offset": 0x92, "way": "i2c"},
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
            "name": "FAN2",
            "airflow": fanairflow,
            "e2loc": {'loc': '/sys/bus/i2c/devices/i2c-48/48-0050/eeprom', 'offset': 0, 'len': 256, 'way': 'devfile'},
            "e2_type": ["wedge_v5", "fru"],
            "present": {"loc": "/sys/s3ip/fan/fan2/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FRONT_FAN_SPEED_MAX,
            "led": {"bus": 52, "addr": 0x0d, "offset": 0x63, "way": "i2c"},
            "led_attrs": {
                "off": 0x00, "green": 0x06, "amber": 0x01, "red": 0x01, "mask": 0x07
            },
            "led_map": {
                0x00: "off", 0x06: "blue", 0x01: "amber"
            },
            "PowerMax": 80.64,
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"bus": 52, "addr": 0x0d, "offset": 0x93, "way": "i2c"},
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
                    "Set_speed": {"bus": 52, "addr": 0x0d, "offset": 0x93, "way": "i2c"},
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
    ],
    "cplds": [
        {
            "name": "CPU CPLD",
            "cpld_id": "CPLD1",
            "VersionFile": {"loc": "/dev/cpld0", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for system power",
            "slot": 0,
            "warm": 0,
        },
        {
            "name": "SCM CPLD",
            "cpld_id": "CPLD2",
            "VersionFile": {"loc": "/dev/cpld1", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for base functions",
            "slot": 0,
            "warm": 0,
        },
        {
            "name": "MCB CPLD",
            "cpld_id": "CPLD3",
            "VersionFile": {"loc": "/dev/cpld3", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for base functions",
            "slot": 0,
            "warm": 0,
        },
        {
            "name": "SMB CPLD",
            "cpld_id": "CPLD4",
            "VersionFile": {"loc": "/dev/cpld4", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for sff modules",
            "slot": 0,
            "warm": 0,
        },
        {
            "name": "FCB_T CPLD",
            "cpld_id": "CPLD5",
            "VersionFile": {"loc": "/dev/cpld5", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for fan modules",
            "slot": 0,
            "warm": 0,
        },
        {
            "name": "FCB_B CPLD",
            "cpld_id": "CPLD6",
            "VersionFile": {"loc": "/dev/cpld6", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for fan modules",
            "slot": 0,
            "warm": 0,
        },
        {
            "name": "IOB FPGA",
            "cpld_id": "CPLD7",
            "VersionFile": {"loc": "/dev/fpga0", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for base functions",
            "slot": 0,
            "format": "little_endian",
            "warm": 0,
        },
        {
            "name": "DOM FPGA",
            "cpld_id": "CPLD8",
            "VersionFile": {"loc": "/dev/fpga1", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for sff functions",
            "slot": 0,
            "format": "little_endian",
            "warm": 0,
        },
        {
            "name": "BIOS",
            "cpld_id": "CPLD9",
            "VersionFile": {"cmd": "dmidecode -s bios-version", "way": "cmd"},
            "desc": "Performs initialization of hardware components during booting",
            "slot": 0,
            "type": "str",
            "warm": 0,
        },
    ],
    "dcdc": [
        {
            "name": "SCM_53134O_V1.0_V",
            "dcdc_id": "DCDC1",
            "Min": 900,
            "value": {
                "loc": "/sys/bus/i2c/devices/33-0040/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1100,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "SCM_I210_V3.3_V",
            "dcdc_id": "DCDC2",
            "Min": 2970,
            "value": {
                "loc": "/sys/bus/i2c/devices/33-0040/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 3630,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "SCM_VDD3.3_V",
            "dcdc_id": "DCDC3",
            "Min": 2970,
            "value": {
                "loc": "/sys/bus/i2c/devices/33-0040/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 3630,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "SCM_VDD12.0_V",
            "dcdc_id": "DCDC4",
            "Min": 10800,
            "value": {
                "loc": "/sys/bus/i2c/devices/33-0041/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 13200,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "SCM_OCM_V12.0_V",
            "dcdc_id": "DCDC5",
            "Min": 10800,
            "value": {
                "loc": "/sys/bus/i2c/devices/33-0041/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 13200,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "SCM_SSD_V3.3_V",
            "dcdc_id": "DCDC6",
            "Min": 2970,
            "value": {
                "loc": "/sys/bus/i2c/devices/33-0042/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 3630,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "SCM_BMC_V3.3_V",
            "dcdc_id": "DCDC7",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/33-0042/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 3630,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "SCM_VDD5.0_V",
            "dcdc_id": "DCDC8",
            "Min": 4500,
            "value": {
                "loc": "/sys/bus/i2c/devices/33-0042/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 5500,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_CORE_V",
            "dcdc_id": "DCDC9",
            "Min": 630,
            "value": {
                "loc": "/sys/bus/i2c/devices/64-0040/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 858,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_ANALOG0 V0.9_V",
            "dcdc_id": "DCDC10",
            "Min": 810,
            "value": {
                "loc": "/sys/bus/i2c/devices/65-0040/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 990,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_ANALOG0 V0.75_V",
            "dcdc_id": "DCDC11",
            "Min": 675,
            "value": {
                "loc": "/sys/bus/i2c/devices/65-0040/hwmon/hwmon*/in4_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 825,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_ANALOG1 V0.9_V",
            "dcdc_id": "DCDC12",
            "Min": 810,
            "value": {
                "loc": "/sys/bus/i2c/devices/66-0040/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 990,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_ANALOG1 V0.75_V",
            "dcdc_id": "DCDC13",
            "Min": 675,
            "value": {
                "loc": "/sys/bus/i2c/devices/66-0040/hwmon/hwmon*/in4_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 825,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_PLL0_VDD0.9_V",
            "dcdc_id": "DCDC14",
            "Min": 810,
            "value": {
                "loc": "/sys/bus/i2c/devices/56-0040/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 990,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_PLL1_VDD0.9_V",
            "dcdc_id": "DCDC15",
            "Min": 810,
            "value": {
                "loc": "/sys/bus/i2c/devices/56-0040/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 990,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_PLL2_VDD0.9_V",
            "dcdc_id": "DCDC16",
            "Min": 810,
            "value": {
                "loc": "/sys/bus/i2c/devices/61-0040/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 990,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_PLL3_VDD0.9_V",
            "dcdc_id": "DCDC17",
            "Min": 810,
            "value": {
                "loc": "/sys/bus/i2c/devices/61-0040/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 990,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_VDD_0.8_V",
            "dcdc_id": "DCDC18",
            "Min": 720,
            "value": {
                "loc": "/sys/bus/i2c/devices/58-0040/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 880,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_VDD_1.8_V",
            "dcdc_id": "DCDC19",
            "Min": 1620,
            "value": {
                "loc": "/sys/bus/i2c/devices/60-0040/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1980,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_VDD_1.5_V",
            "dcdc_id": "DCDC20",
            "Min": 1350,
            "value": {
                "loc": "/sys/bus/i2c/devices/58-0040/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1650,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_VDD_1.2_V",
            "dcdc_id": "DCDC21",
            "Min": 1080,
            "value": {
                "loc": "/sys/bus/i2c/devices/58-0040/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1320,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE0_AVDD_RX_1.8_V",
            "dcdc_id": "DCDC22",
            "Min": 1620,
            "value": {
                "loc": "/sys/bus/i2c/devices/67-0070/hwmon/hwmon*/loopa_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1935,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE0_AVDD_RX_1.2_V",
            "dcdc_id": "DCDC23",
            "Min": 1080,
            "value": {
                "loc": "/sys/bus/i2c/devices/67-0070/hwmon/hwmon*/loopb_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1260,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE0_AVDD_TX_1.8_V",
            "dcdc_id": "DCDC24",
            "Min": 1620,
            "value": {
                "loc": "/sys/bus/i2c/devices/67-0068/hwmon/hwmon*/loopa_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1935,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE0_AVDD_TX_1.2_V",
            "dcdc_id": "DCDC25",
            "Min": 1080,
            "value": {
                "loc": "/sys/bus/i2c/devices/67-0068/hwmon/hwmon*/loopb_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1260,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE0_AVDD_TRX_0.75_V",
            "dcdc_id": "DCDC26",
            "Min": 630,
            "value": {
                "loc": "/sys/bus/i2c/devices/67-0060/hwmon/hwmon*/loopa_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 788,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE1_AVDD_RX_1.8_V",
            "dcdc_id": "DCDC27",
            "Min": 1620,
            "value": {
                "loc": "/sys/bus/i2c/devices/68-0070/hwmon/hwmon*/loopa_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1935,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE1_AVDD_RX_1.2_V",
            "dcdc_id": "DCDC28",
            "Min": 1080,
            "value": {
                "loc": "/sys/bus/i2c/devices/68-0070/hwmon/hwmon*/loopb_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1260,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE1_AVDD_TX_1.8_V",
            "dcdc_id": "DCDC29",
            "Min": 1620,
            "value": {
                "loc": "/sys/bus/i2c/devices/68-0068/hwmon/hwmon*/loopa_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1935,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE1_AVDD_TX_1.2_V",
            "dcdc_id": "DCDC30",
            "Min": 1080,
            "value": {
                "loc": "/sys/bus/i2c/devices/68-0068/hwmon/hwmon*/loopb_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1260,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE1_AVDD_TRX_0.75_V",
            "dcdc_id": "DCDC31",
            "Min": 630,
            "value": {
                "loc": "/sys/bus/i2c/devices/68-0060/hwmon/hwmon*/loopa_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 788,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE2_AVDD_RX_1.8_V",
            "dcdc_id": "DCDC32",
            "Min": 1620,
            "value": {
                "loc": "/sys/bus/i2c/devices/68-006e/hwmon/hwmon*/loopa_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1935,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE2_AVDD_RX_1.2_V",
            "dcdc_id": "DCDC33",
            "Min": 1080,
            "value": {
                "loc": "/sys/bus/i2c/devices/68-006e/hwmon/hwmon*/loopb_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1260,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE2_AVDD_TX_1.8_V",
            "dcdc_id": "DCDC34",
            "Min": 1620,
            "value": {
                "loc": "/sys/bus/i2c/devices/68-0066/hwmon/hwmon*/loopa_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1935,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE2_AVDD_TX_1.2_V",
            "dcdc_id": "DCDC35",
            "Min": 1080,
            "value": {
                "loc": "/sys/bus/i2c/devices/68-0066/hwmon/hwmon*/loopb_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1260,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE2_AVDD_TRX_0.75_V",
            "dcdc_id": "DCDC36",
            "Min": 630,
            "value": {
                "loc": "/sys/bus/i2c/devices/68-005e/hwmon/hwmon*/loopa_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 788,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE3_AVDD_RX_1.8_V",
            "dcdc_id": "DCDC37",
            "Min": 1620,
            "value": {
                "loc": "/sys/bus/i2c/devices/69-0070/hwmon/hwmon*/loopa_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1935,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE3_AVDD_RX_1.2_V",
            "dcdc_id": "DCDC38",
            "Min": 1080,
            "value": {
                "loc": "/sys/bus/i2c/devices/69-0070/hwmon/hwmon*/loopb_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1260,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE3_AVDD_TX_1.8_V",
            "dcdc_id": "DCDC39",
            "Min": 1620,
            "value": {
                "loc": "/sys/bus/i2c/devices/69-0068/hwmon/hwmon*/loopa_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1935,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE3_AVDD_TX_1.2_V",
            "dcdc_id": "DCDC40",
            "Min": 1080,
            "value": {
                "loc": "/sys/bus/i2c/devices/69-0068/hwmon/hwmon*/loopb_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1260,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE3_AVDD_TRX_0.75_V",
            "dcdc_id": "DCDC41",
            "Min": 630,
            "value": {
                "loc": "/sys/bus/i2c/devices/69-0060/hwmon/hwmon*/loopa_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 788,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE4_AVDD_RX_1.8_V",
            "dcdc_id": "DCDC42",
            "Min": 1620,
            "value": {
                "loc": "/sys/bus/i2c/devices/69-006e/hwmon/hwmon*/loopa_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1935,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE4_AVDD_RX_1.2_V",
            "dcdc_id": "DCDC43",
            "Min": 1080,
            "value": {
                "loc": "/sys/bus/i2c/devices/69-006e/hwmon/hwmon*/loopb_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1260,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE4_AVDD_TX_1.8_V",
            "dcdc_id": "DCDC44",
            "Min": 1620,
            "value": {
                "loc": "/sys/bus/i2c/devices/69-0066/hwmon/hwmon*/loopa_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1935,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE4_AVDD_TX_1.2_V",
            "dcdc_id": "DCDC45",
            "Min": 1080,
            "value": {
                "loc": "/sys/bus/i2c/devices/69-0066/hwmon/hwmon*/loopb_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1260,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE4_AVDD_TRX_0.75_V",
            "dcdc_id": "DCDC46",
            "Min": 630,
            "value": {
                "loc": "/sys/bus/i2c/devices/69-005e/hwmon/hwmon*/loopa_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 788,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE5_AVDD_RX_1.8_V",
            "dcdc_id": "DCDC47",
            "Min": 1620,
            "value": {
                "loc": "/sys/bus/i2c/devices/70-0070/hwmon/hwmon*/loopa_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1935,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE5_AVDD_RX_1.2_V",
            "dcdc_id": "DCDC48",
            "Min": 1080,
            "value": {
                "loc": "/sys/bus/i2c/devices/70-0070/hwmon/hwmon*/loopb_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1260,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE5_AVDD_TX_1.8_V",
            "dcdc_id": "DCDC49",
            "Min": 1620,
            "value": {
                "loc": "/sys/bus/i2c/devices/70-0068/hwmon/hwmon*/loopa_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1935,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE5_AVDD_TX_1.2_V",
            "dcdc_id": "DCDC50",
            "Min": 1080,
            "value": {
                "loc": "/sys/bus/i2c/devices/70-0068/hwmon/hwmon*/loopb_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1260,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE5_AVDD_TRX_0.75_V",
            "dcdc_id": "DCDC51",
            "Min": 630,
            "value": {
                "loc": "/sys/bus/i2c/devices/70-0060/hwmon/hwmon*/loopa_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 788,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE6_AVDD_RX_1.8_V",
            "dcdc_id": "DCDC52",
            "Min": 1620,
            "value": {
                "loc": "/sys/bus/i2c/devices/70-006e/hwmon/hwmon*/loopa_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1935,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE6_AVDD_RX_1.2_V",
            "dcdc_id": "DCDC53",
            "Min": 1080,
            "value": {
                "loc": "/sys/bus/i2c/devices/70-006e/hwmon/hwmon*/loopb_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1260,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE6_AVDD_TX_1.8_V",
            "dcdc_id": "DCDC54",
            "Min": 1620,
            "value": {
                "loc": "/sys/bus/i2c/devices/70-0066/hwmon/hwmon*/loopa_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1935,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE6_AVDD_TX_1.2_V",
            "dcdc_id": "DCDC55",
            "Min": 1080,
            "value": {
                "loc": "/sys/bus/i2c/devices/70-0066/hwmon/hwmon*/loopb_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1260,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE6_AVDD_TRX_0.75_V",
            "dcdc_id": "DCDC56",
            "Min": 630,
            "value": {
                "loc": "/sys/bus/i2c/devices/70-005e/hwmon/hwmon*/loopa_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 788,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE7_AVDD_RX_1.8_V",
            "dcdc_id": "DCDC57",
            "Min": 1620,
            "value": {
                "loc": "/sys/bus/i2c/devices/67-006e/hwmon/hwmon*/loopa_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1935,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE7_AVDD_RX_1.2_V",
            "dcdc_id": "DCDC58",
            "Min": 1080,
            "value": {
                "loc": "/sys/bus/i2c/devices/67-006e/hwmon/hwmon*/loopb_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1260,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE7_AVDD_TX_1.8_V",
            "dcdc_id": "DCDC59",
            "Min": 1620,
            "value": {
                "loc": "/sys/bus/i2c/devices/67-0066/hwmon/hwmon*/loopa_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1935,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE7_AVDD_TX_1.2_V",
            "dcdc_id": "DCDC60",
            "Min": 1080,
            "value": {
                "loc": "/sys/bus/i2c/devices/67-0066/hwmon/hwmon*/loopb_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1260,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE7_AVDD_TRX_0.75_V",
            "dcdc_id": "DCDC61",
            "Min": 630,
            "value": {
                "loc": "/sys/bus/i2c/devices/67-005e/hwmon/hwmon*/loopa_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 788,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE_HCSL_PLL_VDD2.5_V",
            "dcdc_id": "DCDC62",
            "Min": 2250,
            "value": {
                "loc": "/sys/bus/i2c/devices/59-0040/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 2750,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE_VDD_1.8_V",
            "dcdc_id": "DCDC63",
            "Min": 1620,
            "value": {
                "loc": "/sys/bus/i2c/devices/59-0040/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1980,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "RLML_VDD V3.3_V",
            "dcdc_id": "DCDC64",
            "Min": 1980,
            "value": {
                "loc": "/sys/bus/i2c/devices/71-0068/hwmon/hwmon*/loopa_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 2420,
            "format": "float(float(%s)*1.5/1000)",
        },

        {
            "name": "RLMH_VDD V3.3_V",
            "dcdc_id": "DCDC65",
            "Min": 1980,
            "value": {
                "loc": "/sys/bus/i2c/devices/71-0068/hwmon/hwmon*/loopb_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 2420,
            "format": "float(float(%s)*1.5/1000)",
        },

        {
            "name": "SMB_CLK_VDD3.3_V",
            "dcdc_id": "DCDC66",
            "Min": 2970,
            "value": {
                "loc": "/sys/bus/i2c/devices/59-0040/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 3630,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "SMB_CLK2_VDD1.8_V",
            "dcdc_id": "DCDC67",
            "Min": 1620,
            "value": {
                "loc": "/sys/bus/i2c/devices/56-0041/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1980,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "SMB_CLK_VDD1.8_V",
            "dcdc_id": "DCDC68",
            "Min": 1620,
            "value": {
                "loc": "/sys/bus/i2c/devices/56-0041/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1980,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "SMB_FPGA_VDD1.8_V",
            "dcdc_id": "DCDC69",
            "Min": 1620,
            "value": {
                "loc": "/sys/bus/i2c/devices/62-0040/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1980,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "SMB_VDD_5.0_V",
            "dcdc_id": "DCDC70",
            "Min": 4500,
            "value": {
                "loc": "/sys/bus/i2c/devices/60-0040/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 5500,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "SMB_FPGA_V3.3_V",
            "dcdc_id": "DCDC71",
            "Min": 2970,
            "value": {
                "loc": "/sys/bus/i2c/devices/62-0040/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 3630,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "SMB_FPGA_V1.0_V",
            "dcdc_id": "DCDC72",
            "Min": 900,
            "value": {
                "loc": "/sys/bus/i2c/devices/62-0040/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1100,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "SMB_FPGA_V1.2_V",
            "dcdc_id": "DCDC73",
            "Min": 1080,
            "value": {
                "loc": "/sys/bus/i2c/devices/57-0040/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1320,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "SMB_VDD_3.3_V",
            "dcdc_id": "DCDC74",
            "Min": 2970,
            "value": {
                "loc": "/sys/bus/i2c/devices/63-0040/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 3630,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "SMB_CPLD_VDD_1.8_V",
            "dcdc_id": "DCDC75",
            "Min": 1620,
            "value": {
                "loc": "/sys/bus/i2c/devices/63-0040/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1980,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "SMB_CPLD_VDD_3.3_V",
            "dcdc_id": "DCDC76",
            "Min": 2970,
            "value": {
                "loc": "/sys/bus/i2c/devices/63-0040/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 3630,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MCB_FPGA_VDD1.0_V",
            "dcdc_id": "DCDC77",
            "Min": 900,
            "value": {
                "loc": "/sys/bus/i2c/devices/12-0040/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1100,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MCB_FPGA_VDD1.8_V",
            "dcdc_id": "DCDC78",
            "Min": 1620,
            "value": {
                "loc": "/sys/bus/i2c/devices/12-0040/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1980,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MCB_FPGA_VDD1.2_V",
            "dcdc_id": "DCDC79",
            "Min": 1080,
            "value": {
                "loc": "/sys/bus/i2c/devices/12-0040/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1320,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MCB_CPLD_VDD3.3_V",
            "dcdc_id": "DCDC80",
            "Min": 2970,
            "value": {
                "loc": "/sys/bus/i2c/devices/12-0041/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 3630,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MCB_VDD3.3_V",
            "dcdc_id": "DCDC81",
            "Min": 2970,
            "value": {
                "loc": "/sys/bus/i2c/devices/12-0041/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 3630,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MCB_FPGA_VDD3.3_V",
            "dcdc_id": "DCDC82",
            "Min": 2970,
            "value": {
                "loc": "/sys/bus/i2c/devices/12-0041/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 3630,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MCB_LEDL_VDD3.3_V",
            "dcdc_id": "DCDC208",
            "Min": 2970,
            "value": {
                "loc": "/sys/bus/i2c/devices/91-0042/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 3630,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "MCB_LEDR_VDD3.3_V",
            "dcdc_id": "DCDC209",
            "Min": 2970,
            "value": {
                "loc": "/sys/bus/i2c/devices/91-0042/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 3630,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OCM_P1V05_V",
            "dcdc_id": "DCDC83",
            "Min": 954,
            "value": {
                "loc": "/sys/bus/i2c/devices/5-005f/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1160,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OCM_VCCIN_V",
            "dcdc_id": "DCDC84",
            "Min": 1350,
            "value": {
                "loc": "/sys/bus/i2c/devices/5-005f/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 2200,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OCM_P1V2_VDDQ_V",
            "dcdc_id": "DCDC85",
            "Min": 1120,
            "value": {
                "loc": "/sys/bus/i2c/devices/5-005f/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1280,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OCM_P1V8_V",
            "dcdc_id": "DCDC86",
            "Min": 1690,
            "value": {
                "loc": "/sys/bus/i2c/devices/5-005f/hwmon/hwmon*/in4_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1910,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OCM_P0V6_VTT_V",
            "dcdc_id": "DCDC87",
            "Min": 558,
            "value": {
                "loc": "/sys/bus/i2c/devices/5-005f/hwmon/hwmon*/in5_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 682,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OCM_VNN_PCH_V",
            "dcdc_id": "DCDC88",
            "Min": 540,
            "value": {
                "loc": "/sys/bus/i2c/devices/5-005f/hwmon/hwmon*/in6_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1320,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OCM_VNN_NAC_V",
            "dcdc_id": "DCDC89",
            "Min": 540,
            "value": {
                "loc": "/sys/bus/i2c/devices/5-005f/hwmon/hwmon*/in7_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1320,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OCM_P2V5_VPP_V",
            "dcdc_id": "DCDC90",
            "Min": 2250,
            "value": {
                "loc": "/sys/bus/i2c/devices/5-005f/hwmon/hwmon*/in8_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 2750,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OCM_VCC_ANA_V",
            "dcdc_id": "DCDC91",
            "Min": 900,
            "value": {
                "loc": "/sys/bus/i2c/devices/5-005f/hwmon/hwmon*/in9_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1100,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OCM_P3V3_STBY_V",
            "dcdc_id": "DCDC92",
            "Min": 2970,
            "value": {
                "loc": "/sys/bus/i2c/devices/5-005f/hwmon/hwmon*/in10_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 3630,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OCM_P5V_AUX_V",
            "dcdc_id": "DCDC93",
            "Min": 4000,
            "value": {
                "loc": "/sys/bus/i2c/devices/5-005f/hwmon/hwmon*/in11_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 5750,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OCM_P1V8_AUX_V",
            "dcdc_id": "DCDC94",
            "Min": 1690,
            "value": {
                "loc": "/sys/bus/i2c/devices/5-005f/hwmon/hwmon*/in12_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1910,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OCM_P3V3_AUX_V",
            "dcdc_id": "DCDC95",
            "Min": 2970,
            "value": {
                "loc": "/sys/bus/i2c/devices/5-005f/hwmon/hwmon*/in13_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 3630,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OCM_V1P80_EMMC_V",
            "dcdc_id": "DCDC96",
            "Min": 1690,
            "value": {
                "loc": "/sys/bus/i2c/devices/5-005f/hwmon/hwmon*/in14_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1910,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OCM_V3P3_EMMC_V",
            "dcdc_id": "DCDC97",
            "Min": 3100,
            "value": {
                "loc": "/sys/bus/i2c/devices/5-005f/hwmon/hwmon*/in15_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 3500,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OCM_XDPE_VCCIN_V",
            "dcdc_id": "DCDC98",
            "Min": 1350,
            "value": {
                "loc": "/sys/bus/i2c/devices/5-0070/hwmon/hwmon*/loopa_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 2200,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OCM_XDPE_P1V8_V",
            "dcdc_id": "DCDC99",
            "Min": 1690,
            "value": {
                "loc": "/sys/bus/i2c/devices/5-0070/hwmon/hwmon*/loopb_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1910,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OCM_XDPE_P1V05_V",
            "dcdc_id": "DCDC100",
            "Min": 954,
            "value": {
                "loc": "/sys/bus/i2c/devices/5-006e/hwmon/hwmon*/loopa_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1160,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OCM_XDPE_VNN_PCH_V",
            "dcdc_id": "DCDC101",
            "Min": 540,
            "value": {
                "loc": "/sys/bus/i2c/devices/5-006e/hwmon/hwmon*/loopb_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1320,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OCM_XDPE_VNN_NAC_V",
            "dcdc_id": "DCDC102",
            "Min": 540,
            "value": {
                "loc": "/sys/bus/i2c/devices/5-0068/hwmon/hwmon*/loopa_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1320,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OCM_XDPE_VCC_ANA_V",
            "dcdc_id": "DCDC103",
            "Min": 900,
            "value": {
                "loc": "/sys/bus/i2c/devices/5-0068/hwmon/hwmon*/loopb_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1100,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OCM_XDPE_P1V2_VDDQ_V",
            "dcdc_id": "DCDC104",
            "Min": 1120,
            "value": {
                "loc": "/sys/bus/i2c/devices/5-005e/hwmon/hwmon*/loopa_vout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 1280,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "FAN1_VDD12_V",
            "dcdc_id": "DCDC105",
            "Min": 7000,
            "value": {
                "loc": "/sys/bus/i2c/devices/88-0042/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 13200,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "FAN2_VDD12_V",
            "dcdc_id": "DCDC106",
            "Min": 7000,
            "value": {
                "loc": "/sys/bus/i2c/devices/89-0042/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 13200,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "FAN3_VDD12_V",
            "dcdc_id": "DCDC107",
            "Min": 7000,
            "value": {
                "loc": "/sys/bus/i2c/devices/88-0042/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 13200,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "FAN4_VDD12_V",
            "dcdc_id": "DCDC108",
            "Min": 7000,
            "value": {
                "loc": "/sys/bus/i2c/devices/89-0042/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 13200,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "FAN5_VDD12_V",
            "dcdc_id": "DCDC109",
            "Min": 7000,
            "value": {
                "loc": "/sys/bus/i2c/devices/88-0042/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 13200,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "FAN6_VDD12_V",
            "dcdc_id": "DCDC110",
            "Min": 7000,
            "value": {
                "loc": "/sys/bus/i2c/devices/90-0042/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 13200,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "FAN7_VDD12_V",
            "dcdc_id": "DCDC111",
            "Min": 7000,
            "value": {
                "loc": "/sys/bus/i2c/devices/89-0042/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 13200,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "FAN8_VDD12_V",
            "dcdc_id": "DCDC112",
            "Min": 7000,
            "value": {
                "loc": "/sys/bus/i2c/devices/90-0042/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "V",
            "Max": 13200,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "SCM_53134O_V1.0_C",
            "dcdc_id": "DCDC113",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/33-0040/hwmon/hwmon*/curr1_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 550,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "SCM_I210_V3.3_C",
            "dcdc_id": "DCDC114",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/33-0040/hwmon/hwmon*/curr2_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 550,
            "format": "float(float(%s)/1000)",
        },       

        {
            "name": "SCM_VDD3.3_C",
            "dcdc_id": "DCDC115",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/33-0040/hwmon/hwmon*/curr3_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 2200,
            "format": "float(float(%s)/1000)",
        },   

        {
            "name": "SCM_VDD12.0_C",
            "dcdc_id": "DCDC116",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/33-0041/hwmon/hwmon*/curr1_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 8800,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "SCM_OCM_V12.0_C",
            "dcdc_id": "DCDC117",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/33-0041/hwmon/hwmon*/curr3_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 7700,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "SCM_SSD_V3.3_C",
            "dcdc_id": "DCDC118",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/33-0042/hwmon/hwmon*/curr1_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 1650,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "SCM_BMC_V3.3_C",
            "dcdc_id": "DCDC119",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/33-0042/hwmon/hwmon*/curr2_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 2750,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "SCM_VDD5.0_C",
            "dcdc_id": "DCDC120",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/33-0042/hwmon/hwmon*/curr3_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 550,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_CORE_C",
            "dcdc_id": "DCDC121",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/64-0040/hwmon/hwmon*/curr3_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 909700,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_ANALOG0 V0.9_C",
            "dcdc_id": "DCDC122",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/65-0040/hwmon/hwmon*/curr3_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 77737,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_ANALOG0 V0.75_C",
            "dcdc_id": "DCDC123",
            "Min": -1000,
            "value": {
                "loc": "//sys/bus/i2c/devices/65-0040/hwmon/hwmon*/curr4_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 38826,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_ANALOG1 V0.9_C",
            "dcdc_id": "DCDC124",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/66-0040/hwmon/hwmon*/curr3_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 77737,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_ANALOG1 V0.75_C",
            "dcdc_id": "DCDC125",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/66-0040/hwmon/hwmon*/curr4_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 38826,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_PLL0_VDD0.9_C",
            "dcdc_id": "DCDC126",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/56-0040/hwmon/hwmon*/curr2_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 2570,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_PLL1_VDD0.9_C",
            "dcdc_id": "DCDC127",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/56-0040/hwmon/hwmon*/curr1_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 2570,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MAC_PLL2_VDD0.9_C",
            "dcdc_id": "DCDC128",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/58-0040/hwmon/hwmon*/curr1_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 2570,
            "format": "float(float(%s)/1000)",
        },  

        {
            "name": "MAC_PLL3_VDD0.9_C",
            "dcdc_id": "DCDC129",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/58-0040/hwmon/hwmon*/curr3_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 2570,
            "format": "float(float(%s)/1000)",
        }, 

        {
            "name": "MAC_VDD_0.8_C",
            "dcdc_id": "DCDC130",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/58-0040/hwmon/hwmon*/curr2_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 1500,
            "format": "float(float(%s)/1000)",
        },   

        {
            "name": "MAC_VDD_1.8_C",
            "dcdc_id": "DCDC131",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/60-0040/hwmon/hwmon*/curr3_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 2570,
            "format": "float(float(%s)/1000)",
        },   

        {
            "name": "MAC_VDD_1.5_C",
            "dcdc_id": "DCDC132",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/58-0040/hwmon/hwmon*/curr1_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 1485,
            "format": "float(float(%s)/1000)",
        },  

        {
            "name": "MAC_VDD_1.2_C",
            "dcdc_id": "DCDC133",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/58-0040/hwmon/hwmon*/curr3_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 1600,
            "format": "float(float(%s)/1000)",
        },  


        {
            "name": "OE0_AVDD_RX_1.8_C",
            "dcdc_id": "DCDC134",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/67-0070/hwmon/hwmon*/loopa_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 2000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE0_AVDD_RX_1.2_C",
            "dcdc_id": "DCDC135",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/67-0070/hwmon/hwmon*/loopb_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 5100,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE0_AVDD_TX_1.8_C",
            "dcdc_id": "DCDC136",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/67-0068/hwmon/hwmon*/loopa_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 10000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE0_AVDD_TX_1.2_C",
            "dcdc_id": "DCDC137",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/67-0068/hwmon/hwmon*/loopb_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 9000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE0_AVDD_TRX_0.75_C",
            "dcdc_id": "DCDC138",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/67-0060/hwmon/hwmon*/loopa_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 9500,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE1_AVDD_RX_1.8_C",
            "dcdc_id": "DCDC139",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/68-0070/hwmon/hwmon*/loopa_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 2000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE1_AVDD_RX_1.2_C",
            "dcdc_id": "DCDC140",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/68-0070/hwmon/hwmon*/loopb_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 5100,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE1_AVDD_TX_1.8_C",
            "dcdc_id": "DCDC141",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/68-0068/hwmon/hwmon*/loopa_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 10000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE1_AVDD_TX_1.2_C",
            "dcdc_id": "DCDC142",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/68-0068/hwmon/hwmon*/loopb_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 9000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE1_AVDD_TRX_0.75_C",
            "dcdc_id": "DCDC143",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/68-0060/hwmon/hwmon*/loopa_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 9500,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE2_AVDD_RX_1.8_C",
            "dcdc_id": "DCDC144",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/68-006e/hwmon/hwmon*/loopa_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 2000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE2_AVDD_RX_1.2_C",
            "dcdc_id": "DCDC145",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/68-006e/hwmon/hwmon*/loopb_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 5100,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE2_AVDD_TX_1.8_C",
            "dcdc_id": "DCDC146",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/68-0066/hwmon/hwmon*/loopa_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 10000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE2_AVDD_TX_1.2_C",
            "dcdc_id": "DCDC147",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/68-0066/hwmon/hwmon*/loopb_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 9000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE2_AVDD_TRX_0.75_C",
            "dcdc_id": "DCDC148",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/68-005e/hwmon/hwmon*/loopa_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 9500,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE3_AVDD_RX_1.8_C",
            "dcdc_id": "DCDC149",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/69-0070/hwmon/hwmon*/loopa_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 2000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE3_AVDD_RX_1.2_C",
            "dcdc_id": "DCDC150",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/69-0070/hwmon/hwmon*/loopb_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 5100,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE3_AVDD_TX_1.8_C",
            "dcdc_id": "DCDC151",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/69-0068/hwmon/hwmon*/loopa_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 10000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE3_AVDD_TX_1.2_C",
            "dcdc_id": "DCDC152",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/69-0068/hwmon/hwmon*/loopb_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 9000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE3_AVDD_TRX_0.75_C",
            "dcdc_id": "DCDC153",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/69-0060/hwmon/hwmon*/loopa_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 9500,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE4_AVDD_RX_1.8_C",
            "dcdc_id": "DCDC154",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/69-006e/hwmon/hwmon*/loopa_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 2000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE4_AVDD_RX_1.2_C",
            "dcdc_id": "DCDC155",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/69-006e/hwmon/hwmon*/loopb_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 5100,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE4_AVDD_TX_1.8_C",
            "dcdc_id": "DCDC156",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/69-0066/hwmon/hwmon*/loopa_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 10000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE4_AVDD_TX_1.2_C",
            "dcdc_id": "DCDC157",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/69-0066/hwmon/hwmon*/loopb_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 9000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE4_AVDD_TRX_0.75_C",
            "dcdc_id": "DCDC158",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/69-005e/hwmon/hwmon*/loopa_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 9500,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE5_AVDD_RX_1.8_C",
            "dcdc_id": "DCDC159",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/70-0070/hwmon/hwmon*/loopa_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 2000,
            "format": "float(float(%s)/1000)",
        },


        {
            "name": "OE5_AVDD_RX_1.2_C",
            "dcdc_id": "DCDC160",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/70-0070/hwmon/hwmon*/loopb_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 5100,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE5_AVDD_TX_1.8_C",
            "dcdc_id": "DCDC161",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/70-0068/hwmon/hwmon*/loopa_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 10000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE5_AVDD_TX_1.2_C",
            "dcdc_id": "DCDC162",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/70-0068/hwmon/hwmon*/loopb_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 9000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE5_AVDD_TRX_0.75_C",
            "dcdc_id": "DCDC163",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/70-0060/hwmon/hwmon*/loopa_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 9500,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE6_AVDD_RX_1.8_C",
            "dcdc_id": "DCDC164",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/70-006e/hwmon/hwmon*/loopa_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 2000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE6_AVDD_RX_1.2_C",
            "dcdc_id": "DCDC165",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/70-006e/hwmon/hwmon*/loopb_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 5100,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE6_AVDD_TX_1.8_C",
            "dcdc_id": "DCDC166",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/70-0066/hwmon/hwmon*/loopa_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 10000,
            "format": "float(float(%s)/1000)",
        },


        {
            "name": "OE6_AVDD_TX_1.2_C",
            "dcdc_id": "DCDC167",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/70-0066/hwmon/hwmon*/loopb_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 9000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE6_AVDD_TRX_0.75_C",
            "dcdc_id": "DCDC168",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/70-005e/hwmon/hwmon*/loopa_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 9500,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE7_AVDD_RX_1.8_C",
            "dcdc_id": "DCDC169",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/67-006e/hwmon/hwmon*/loopa_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 2000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE7_AVDD_RX_1.2_C",
            "dcdc_id": "DCDC170",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/67-006e/hwmon/hwmon*/loopb_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 5100,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE7_AVDD_TX_1.8_C",
            "dcdc_id": "DCDC171",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/67-0066/hwmon/hwmon*/loopa_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 10000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE7_AVDD_TX_1.2_C",
            "dcdc_id": "DCDC172",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/67-0066/hwmon/hwmon*/loopb_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 9000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE7_AVDD_TRX_0.75_C",
            "dcdc_id": "DCDC173",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/67-005e/hwmon/hwmon*/loopa_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 9500,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE_HCSL_PLL_VDD2.5_C",
            "dcdc_id": "DCDC174",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/59-0040/hwmon/hwmon*/curr2_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 2000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OE_VDD_1.8_C",
            "dcdc_id": "DCDC175",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/59-0040/hwmon/hwmon*/curr3_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 550,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "RLML_VDD V3.3_C",
            "dcdc_id": "DCDC176",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/71-0068/hwmon/hwmon*/loopa_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 30000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "RLMH_VDD V3.3_C",
            "dcdc_id": "DCDC177",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/71-0068/hwmon/hwmon*/loopb_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 30000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "SMB_CLK_VDD3.3_C",
            "dcdc_id": "DCDC178",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/59-0040/hwmon/hwmon*/curr1_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 1100,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "SMB_CLK2_VDD1.8_C",
            "dcdc_id": "DCDC179",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/56-0041/hwmon/hwmon*/curr3_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 1100,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "SMB_CLK_VDD1.8_C",
            "dcdc_id": "DCDC180",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/56-0041/hwmon/hwmon*/curr2_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 1100,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "SMB_FPGA_VDD1.8_C",
            "dcdc_id": "DCDC181",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/62-0040/hwmon/hwmon*/curr3_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 550,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "SMB_VDD_5.0_C",
            "dcdc_id": "DCDC182",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/60-0040/hwmon/hwmon*/curr2_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 3300,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "SMB_FPGA_V1.0_C",
            "dcdc_id": "DCDC183",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/62-0040/hwmon/hwmon*/curr2_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 1188,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "SMB_FPGA_V1.2_C",
            "dcdc_id": "DCDC184",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/57-0040/hwmon/hwmon*/curr1_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 550,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "SMB_VDD_3.3_C",
            "dcdc_id": "DCDC185",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/63-0040/hwmon/hwmon*/curr1_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 1550,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "SMB_CPLD_VDD_1.8_C",
            "dcdc_id": "DCDC186",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/63-0040/hwmon/hwmon*/curr2_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 330,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MCB_FPGA_VDD1.0_C",
            "dcdc_id": "DCDC187",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/12-0040/hwmon/hwmon*/curr1_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 1188,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MCB_FPGA_VDD1.8_C",
            "dcdc_id": "DCDC188",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/12-0040/hwmon/hwmon*/curr2_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 550,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MCB_FPGA_VDD1.2_C",
            "dcdc_id": "DCDC189",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/12-0040/hwmon/hwmon*/curr3_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 550,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MCB_CPLD_VDD3.3_C",
            "dcdc_id": "DCDC190",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/12-0041/hwmon/hwmon*/curr1_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 550,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MCB_VDD3.3_C",
            "dcdc_id": "DCDC191",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/12-0041/hwmon/hwmon*/curr2_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 550,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MCB_FPGA_VDD3.3_C",
            "dcdc_id": "DCDC192",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/12-0041/hwmon/hwmon*/curr3_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 550,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MCB_LEDL_VDD3.3_C",
            "dcdc_id": "DCDC210",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/91-0042/hwmon/hwmon*/curr1_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 1200,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "MCB_LEDR_VDD3.3_C",
            "dcdc_id": "DCDC211",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/91-0042/hwmon/hwmon*/curr2_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 1200,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OCM_XDPE_VCCIN_C",
            "dcdc_id": "DCDC193",
            "Min": -3100,
            "value": {
                "loc": "/sys/bus/i2c/devices/5-0070/hwmon/hwmon*/loopa_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 147000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OCM_XDPE_P1V8_C",
            "dcdc_id": "DCDC194",
            "Min": -3100,
            "value": {
                "loc": "/sys/bus/i2c/devices/5-0070/hwmon/hwmon*/loopb_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 2300,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OCM_XDPE_P1V05_C",
            "dcdc_id": "DCDC195",
            "Min": -3100,
            "value": {
                "loc": "/sys/bus/i2c/devices/5-006e/hwmon/hwmon*/loopa_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 14300,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OCM_XDPE_VNN_PCH_C",
            "dcdc_id": "DCDC196",
            "Min": -3100,
            "value": {
                "loc": "/sys/bus/i2c/devices/5-006e/hwmon/hwmon*/loopb_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 7400,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OCM_XDPE_VNN_NAC_C",
            "dcdc_id": "DCDC197",
            "Min": -3100,
            "value": {
                "loc": "/sys/bus/i2c/devices/5-0068/hwmon/hwmon*/loopa_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 22000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OCM_XDPE_VCC_ANA_C",
            "dcdc_id": "DCDC198",
            "Min": -3100,
            "value": {
                "loc": "/sys/bus/i2c/devices/5-0068/hwmon/hwmon*/loopb_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 2210,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "OCM_XDPE_P1V2_VDDQ_C",
            "dcdc_id": "DCDC199",
            "Min": -3100,
            "value": {
                "loc": "/sys/bus/i2c/devices/5-005e/hwmon/hwmon*/loopa_iout",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 19000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "FAN1_VDD12_C",
            "dcdc_id": "DCDC200",
            "Min": -3100,
            "value": {
                "loc": "/sys/bus/i2c/devices/88-0042/hwmon/hwmon*/curr1_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 10000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "FAN2_VDD12_C",
            "dcdc_id": "DCDC201",
            "Min": -3100,
            "value": {
                "loc": "/sys/bus/i2c/devices/89-0042/hwmon/hwmon*/curr2_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 10000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "FAN3_VDD12_C",
            "dcdc_id": "DCDC202",
            "Min": -3100,
            "value": {
                "loc": "/sys/bus/i2c/devices/88-0042/hwmon/hwmon*/curr2_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 10000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "FAN4_VDD12_C",
            "dcdc_id": "DCDC203",
            "Min": -3100,
            "value": {
                "loc": "/sys/bus/i2c/devices/89-0042/hwmon/hwmon*/curr3_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 10000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "FAN5_VDD12_C",
            "dcdc_id": "DCDC204",
            "Min": -3100,
            "value": {
                "loc": "/sys/bus/i2c/devices/88-0042/hwmon/hwmon*/curr3_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 10000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "FAN6_VDD12_C",
            "dcdc_id": "DCDC205",
            "Min": -3100,
            "value": {
                "loc": "/sys/bus/i2c/devices/90-0042/hwmon/hwmon*/curr1_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 10000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "FAN7_VDD12_C",
            "dcdc_id": "DCDC206",
            "Min": -3100,
            "value": {
                "loc": "/sys/bus/i2c/devices/89-0042/hwmon/hwmon*/curr1_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 10000,
            "format": "float(float(%s)/1000)",
        },

        {
            "name": "FAN8_VDD12_C",
            "dcdc_id": "DCDC207",
            "Min": -3100,
            "value": {
                "loc": "/sys/bus/i2c/devices/90-0042/hwmon/hwmon*/curr2_input",
                "way": "sysfs",
            },
            "read_times": 3,
            "Unit": "A",
            "Max": 10000,
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
        "ver": '3.0',
        "port_index_start": 1,
        "port_num": 128,
        "log_level": 2,
        "eeprom_retry_times": 5,
        "eeprom_retry_break_sec": 0.2,
        "presence_val_is_present": 0,
        "eeprom_path": "/sys/bus/i2c/devices/i2c-%d/%d-0050/eeprom",
        "eeprom_path_key": [24] * 16 + [25] * 16 + [26] * 16 + [27] * 16 + [28] * 16 + [29] * 16 + [30] * 16 + [31] * 16
    }
}
