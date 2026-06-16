#!/usr/bin/python3


psu_fan_airflow = {
    "intake": ['PA150II-F', 'PD150II-F'],
    "exhaust": ['PA150II-R', 'PD150II-R']
}

fanairflow = {
    "intake": ['M1LFAN I-F'],
    "exhaust": ['M1LFAN I-R']
}

psu_display_name = {
    "PA150II-F": ['PA150II-F'],
    "PA150II-R": ['PA150II-R'],
    "PD150II-F": ['PD150II-F'],
    "PD150II-R": ['PD150II-R']
}

psutypedecode = {
    "AC": ["PA150II-F", "PA150II-R"],
    "DC": ["PD150II-F", "PD150II-R"],
}


class Unit:
    Temperature = "C"
    Voltage = "V"
    Current = "A"
    Power = "W"
    Speed = "RPM"


class threshold:
    FAN_SPEED_MAX = 19800
    FAN_SPEED_MIN = 4860


class Description:
    CPLD = "Used for managing IO modules, SFP+ modules and system LEDs"
    BIOS = "Performs initialization of hardware components during booting"
    FPGA = "Platform management controller for on-board temperature monitoring, in-chassis power"


devices = {
    "onie_e2": [
        {
            "name": "ONIE_E2",
            "e2loc": {"loc": "/sys/bus/i2c/devices/2-0056/eeprom", "way": "sysfs"},
            "airflow": "exhaust"
        },
    ],
    "psus": [
        {
            "e2loc": {"loc": "/sys/bus/i2c/devices/7-0056/eeprom", "way": "sysfs"},
            "e2_type": "custfru",
            "present": {"loc": "/sys/wb_plat/psu/psu1/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "name": "PSU1",
            "psu_display_name": psu_display_name,
            "airflow": psu_fan_airflow,
            "psu_fan_tolerance": 40,
            "InputsType": {"gettype": "fru", 'psutypedecode': psutypedecode},
            "OutputsStatus": {"loc": "/sys/wb_plat/psu/psu1/output", "way": "sysfs", "mask": 0x01, "okval": 1},
            "Temperature": {
                "value": {"value": None, "way": "config"},
                "Unit": Unit.Temperature
            },
            "FanSpeed": {
                "value": {"value": None, "way": "config"},
                "Unit": Unit.Speed
            },
            "InputsVoltage": {
                'AC': {
                    "value": {"value": None, "way": "config"},
                    "Unit": Unit.Voltage

                },
                'DC': {
                    "value": {"value": None, "way": "config"},
                    "Unit": Unit.Voltage
                },
                'other': {
                    "value": {"value": None, "way": "config"},
                    "Unit": Unit.Voltage
                }
            },
            "InputsCurrent": {
                "value": {"value": None, "way": "config"},
                "Unit": Unit.Current
            },
            "InputsPower": {
                "value": {"value": None, "way": "config"},
                "Unit": Unit.Power
            },
            "OutputsVoltage": {
                "value": {"value": None, "way": "config"},
                "Unit": Unit.Voltage
            },
            "OutputsCurrent": {
                "value": {"value": None, "way": "config"},
                "Unit": Unit.Current
            },
            "OutputsPower": {
                "value": {"value": None, "way": "config"},
                "Unit": Unit.Power
            },
        },
        {
            "e2loc": {"loc": "/sys/bus/i2c/devices/7-0057/eeprom", "way": "sysfs"},
            "e2_type": "custfru",
            "present": {"loc": "/sys/wb_plat/psu/psu2/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "name": "PSU2",
            "psu_display_name": psu_display_name,
            "airflow": psu_fan_airflow,
            "psu_fan_tolerance": 40,
            "InputsType": {"gettype": "fru", 'psutypedecode': psutypedecode},
            "OutputsStatus": {"loc": "/sys/wb_plat/psu/psu2/output", "way": "sysfs", "mask": 0x01, "okval": 1},
            "Temperature": {
                "value": {"value": None, "way": "config"},
                "Unit": Unit.Temperature
            },
            "FanSpeed": {
                "value": {"value": None, "way": "config"},
                "Unit": Unit.Speed
            },
            "InputsVoltage": {
                'AC': {
                    "value": {"value": None, "way": "config"},
                    "Unit": Unit.Voltage

                },
                'DC': {
                    "value": {"value": None, "way": "config"},
                    "Unit": Unit.Voltage
                },
                'other': {
                    "value": {"value": None, "way": "config"},
                    "Unit": Unit.Voltage
                }
            },
            "InputsCurrent": {
                "value": {"value": None, "way": "config"},
                "Unit": Unit.Current
            },
            "InputsPower": {
                "value": {"value": None, "way": "config"},
                "Unit": Unit.Power
            },
            "OutputsVoltage": {
                "value": {"value": None, "way": "config"},
                "Unit": Unit.Voltage
            },
            "OutputsCurrent": {
                "value": {"value": None, "way": "config"},
                "Unit": Unit.Current
            },
            "OutputsPower": {
                "value": {"value": None, "way": "config"},
                "Unit": Unit.Power
            },
        }
    ],
    "temps": [
        {
            "name": "SWITCH_TEMP",
            "temp_id": "TEMP1",
            "api_name": "ASIC_TEMP",
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/18-0044/hwmon/hwmon*/temp99_input", "way": "sysfs"},
                "Min": 2000,
                "Low": 10000,
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
                "Min": 2000,
                "Low": 10000,
                "High": 85000,
                "Max": 91000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
        {
            "name": "INLET_TEMP",
            "temp_id": "TEMP3",
            "Temperature": {
                "value": [
                    {"loc": "/sys/bus/i2c/devices/6-0048/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                ],
                "Min": -10000,
                "Low": 0,
                "High": 55000,
                "Max": 60000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            },
            "fix_value": {
                "fix_type": "config",
                "addend": -7,
            }
        },
        {
            "name": "OUTLET_TEMP",
            "temp_id": "TEMP4",
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/6-0049/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": -10000,
                "Low": 0,
                "High": 70000,
                "Max": 75000,
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
            "led": {"loc": "/dev/cpld1", "offset": 0x50, "len": 1, "way": "devfile"},
            "led_attrs": {
                "off": 0x10, "red_flash": 0x11, "red": 0x12,
                "green_flash": 0x13, "green": 0x14, "amber_flash": 0x15,
                "amber": 0x16, "mask": 0x17
            },
        },
        {
            "name": "FRONT_PSU_LED",
            "led_type": "PSU_LED",
            "led": {"loc": "/dev/cpld1", "offset": 0x51, "len": 1, "way": "devfile"},
            "led_attrs": {
                "off": 0x10, "red_flash": 0x11, "red": 0x12,
                "green_flash": 0x13, "green": 0x14, "amber_flash": 0x15,
                "amber": 0x16, "mask": 0x17
            },
        },
        {
            "name": "FRONT_FAN_LED",
            "led_type": "FAN_LED",
            "led": {"loc": "/dev/cpld1", "offset": 0x52, "len": 1, "way": "devfile"},
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
            "e2loc": {'loc': '/sys/bus/i2c/devices/8-0053/eeprom', 'way': 'sysfs'},
            "present": {"loc": "/sys/wb_plat/fan/fan1/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FAN_SPEED_MAX,
            "led": {"loc": "/dev/cpld1", "offset": 0x92, "len": 1, "way": "devfile"},
            "led_attrs": {
                "off": 0x00, "red_flash": 0x01, "red": 0x02,
                "green_flash": 0x03, "green": 0x04, "amber_flash": 0x05,
                "amber": 0x06, "mask": 0x07
            },
            "PowerMax": 6.6,
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"loc": "/dev/cpld1", "offset": 0x90, "len": 1, "way": "devfile"},
                    "Running": {"loc": "/sys/wb_plat/fan/fan1/motor0/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/wb_plat/fan/fan1/motor0/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/wb_plat/fan/fan1/motor0/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
            },
        },
        {
            "name": "FAN2",
            "airflow": fanairflow,
            "e2loc": {'loc': '/sys/bus/i2c/devices/9-0053/eeprom', 'way': 'sysfs'},
            "present": {"loc": "/sys/wb_plat/fan/fan2/present", "way": "sysfs", "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FAN_SPEED_MAX,
            "led": {"loc": "/dev/cpld1", "offset": 0x93, "len": 1, "way": "devfile"},
            "led_attrs": {
                "off": 0x00, "red_flash": 0x01, "red": 0x02,
                "green_flash": 0x03, "green": 0x04, "amber_flash": 0x05,
                "amber": 0x06, "mask": 0x07
            },
            "PowerMax": 6.6,
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"loc": "/dev/cpld1", "offset": 0x91, "len": 1, "way": "devfile"},
                    "Running": {"loc": "/sys/wb_plat/fan/fan2/motor0/status", "way": "sysfs", "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"loc": "/sys/wb_plat/fan/fan2/motor0/status", "way": "sysfs", "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/sys/wb_plat/fan/fan2/motor0/speed", "way": "sysfs"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FAN_SPEED_MAX,
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
            "name": "PORT_CPLD",
            "cpld_id": "CPLD3",
            "VersionFile": {"loc": "/dev/cpld2", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for sff modules",
            "slot": 0,
            "warm": 0,
        },
        {
            "name": "BIOS",
            "cpld_id": "CPLD4",
            "VersionFile": {"cmd": "dmidecode -s bios-version", "way": "cmd"},
            "desc": "Performs initialization of hardware components during booting",
            "slot": 0,
            "type": "str",
            "warm": 0,
        }
    ],
    "dcdc": [
        {
            "name": "VDD_CORE_0.8V",
            "dcdc_id": "DCDC1",
            "Min": 784,
            "value": {
                "loc": "/sys/bus/i2c/devices/17-0058/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 893,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "PVCCP",
            "dcdc_id": "DCDC2",
            "Min": 468,
            "value": {
                "loc": "/sys/bus/i2c/devices/0-0068/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1364,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "PVNN",
            "dcdc_id": "DCDC3",
            "Min": 585,
            "value": {
                "loc": "/sys/bus/i2c/devices/0-0068/hwmon/hwmon*/in4_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1364,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "P1V05",
            "dcdc_id": "DCDC4",
            "Min": 945,
            "value": {
                "loc": "/sys/bus/i2c/devices/0-006e/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1155,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "VCCRAM",
            "dcdc_id": "DCDC5",
            "Min": 675,
            "value": {
                "loc": "/sys/bus/i2c/devices/0-006e/hwmon/hwmon*/in4_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1320,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "P1V2_VDDQ",
            "dcdc_id": "DCDC6",
            "Min": 1080,
            "value": {
                "loc": "/sys/bus/i2c/devices/0-005e/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1320,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "CPU_VDD1.8V",
            "dcdc_id": "DCDC7",
            "Min": 1620,
            "value": {
                "loc": "/sys/bus/i2c/devices/3-0040/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1980,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "P3V3_STBY",
            "dcdc_id": "DCDC8",
            "Min": 2970,
            "value": {
                "loc": "/sys/bus/i2c/devices/3-0040/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3630,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "P5V_AUX",
            "dcdc_id": "DCDC9",
            "Min": 4500,
            "value": {
                "loc": "/sys/bus/i2c/devices/3-0040/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 5500,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "VDD1.2V",
            "dcdc_id": "DCDC10",
            "Min": 1140,
            "value": {
                "loc": "/sys/bus/i2c/devices/3-0041/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1260,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "VDD1.8V",
            "dcdc_id": "DCDC11",
            "Min": 1710,
            "value": {
                "loc": "/sys/bus/i2c/devices/3-0041/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1890,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "VDD3.3V",
            "dcdc_id": "DCDC12",
            "Min": 3200,
            "value": {
                "loc": "/sys/bus/i2c/devices/3-0041/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3600,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "VDD1.0V",
            "dcdc_id": "DCDC13",
            "Min": 969,
            "value": {
                "loc": "/sys/bus/i2c/devices/3-0042/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1071,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "VDD3.3V_SFP",
            "dcdc_id": "DCDC14",
            "Min": 3200,
            "value": {
                "loc": "/sys/bus/i2c/devices/3-0042/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3600,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "VAN_0.8V",
            "dcdc_id": "DCDC15",
            "Min": 784,
            "value": {
                "loc": "/sys/bus/i2c/devices/3-0042/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 893,
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
        "port_index_start": 0,
        "port_num": 52,
        "log_level": 2,
        "eeprom_retry_times": 5,
        "eeprom_retry_break_sec": 0.2,
        "presence_cpld": {
            "dev_id": {
                5: {
                    "offset": {
                        0x30: "50,49,52,51",
                    },
                },
            },
        },
        "presence_val_is_present": 0,
        "eeprom_path": "/sys/bus/i2c/devices/i2c-%d/%d-0050/eeprom",
        "eeprom_path_key": [11]*48 + list(range(11, 15)),
        "optoe_driver_path": "/sys/bus/i2c/devices/i2c-%d/%d-0050/dev_class",
        "optoe_driver_key": list(range(11, 15)),
        "txdis_cpld": {
            "dev_id": {
                5: {
                    "offset": {
                        0x90: "50,49,52,51",
                    },
                },
            },
        },
        "txdisable_val_is_on": 1,
    }
}
