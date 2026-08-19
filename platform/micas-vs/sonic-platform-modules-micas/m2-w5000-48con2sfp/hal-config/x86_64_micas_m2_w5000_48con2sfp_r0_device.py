#!/usr/bin/python3


class Unit:
    Temperature = "C"
    Voltage = "V"
    Current = "A"
    Power = "W"
    Speed = "RPM"


class threshold:
    FAN_SPEED_MAX = 20000
    FAN_SPEED_MIN = 2000


class Description:
    CPLD = "Used for managing IO modules, SFP+ modules and system LEDs"
    BIOS = "Performs initialization of hardware components during booting"
    FPGA = "Platform management controller for on-board temperature monitoring, in-chassis power"


devices = {
    "onie_e2": [
        {
            "name": "ONIE_E2",
            "e2loc": {"loc": "/sys/bus/i2c/devices/3-0050/eeprom", "way": "sysfs"},
            "airflow": "intake"
        },
    ],
    "temps": [
        {
            "name": "CPU_TEMP",
            "temp_id": "TEMP1",
            "Temperature": {
                "value": {"loc": "/sys/devices/pci0000:00/0000:00:18.3/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": -30000,
                "Low": 0,
                "High": 90000,
                "Max": 95000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
        {
            "name": "INLET_TEMP",
            "temp_id": "TEMP2",
            "Temperature": {
                "value": [
                    {"loc": "/sys/bus/i2c/devices/7-0048/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                ],
                "Min": -40000,
                "Low": 0,
                "High": 50000,
                "Max": 55000,
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
            "temp_id": "TEMP3",
            "Temperature": {
                "value": {"loc": "/sys/bus/i2c/devices/8-0049/hwmon/hwmon*/temp1_input", "way": "sysfs"},
                "Min": -30000,
                "Low": 0,
                "High": 80000,
                "Max": 85000,
                "Unit": Unit.Temperature,
                "format": "float(float(%s)/1000)"
            }
        },
    ],
    "leds": [
        {
            "name": "FRONT_SYS_LED",
            "led_type": "SYS_LED",
            "led": {"bus": 4, "addr": 0x0d, "offset": 0x70, "way": "i2c"},
            "led_attrs": {
                "off": 0x00,
                "green_flash": 0x05, "green": 0x01, "amber_flash": 0x06,
                "amber": 0x02, "mask": 0x07
            },
        },
    ],
    "fans": [
        {
            "name": "FAN1",
            "airflow": "intake",
            "present": {"way": "config", "value": 1, "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FAN_SPEED_MAX,
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"loc": "/dev/cpld1", "offset": 0x80, "len": 1, "way": "devfile"},
                    "Running": {"way": "config", "value": 1, "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"way": "config", "value": 1, "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/dev/cpld1", "offset": 0x84, "len": 2, "way": "devfile"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
            },
        },
        {
            "name": "FAN2",
            "airflow": "intake",
            "present": {"way": "config", "value": 1, "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FAN_SPEED_MAX,
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"loc": "/dev/cpld1", "offset": 0x81, "len": 1, "way": "devfile"},
                    "Running": {"way": "config", "value": 1, "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"way": "config", "value": 1, "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/dev/cpld1", "offset": 0x86, "len": 2, "way": "devfile"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
            },
        },
        {
            "name": "FAN3",
            "airflow": "intake",
            "present": {"way": "config", "value": 1, "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FAN_SPEED_MAX,
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"loc": "/dev/cpld1", "offset": 0x82, "len": 1, "way": "devfile"},
                    "Running": {"way": "config", "value": 1, "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"way": "config", "value": 1, "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/dev/cpld1", "offset": 0x88, "len": 2, "way": "devfile"},
                        "Min": threshold.FAN_SPEED_MIN,
                        "Max": threshold.FAN_SPEED_MAX,
                        "Unit": Unit.Speed,
                    },
                },
            },
        },
        {
            "name": "FAN4",
            "airflow": "intake",
            "present": {"way": "config", "value": 1, "mask": 0x01, "okval": 1},
            "SpeedMin": threshold.FAN_SPEED_MIN,
            "SpeedMax": threshold.FAN_SPEED_MAX,
            "Rotor": {
                "Rotor1_config": {
                    "name": "Rotor1",
                    "Set_speed": {"loc": "/dev/cpld1", "offset": 0x83, "len": 1, "way": "devfile"},
                    "Running": {"way": "config", "value": 1, "mask": 0x01, "is_runing": 1},
                    "HwAlarm": {"way": "config", "value": 1, "mask": 0x01, "no_alarm": 1},
                    "SpeedMin": threshold.FAN_SPEED_MIN,
                    "SpeedMax": threshold.FAN_SPEED_MAX,
                    "Speed": {
                        "value": {"loc": "/dev/cpld1", "offset": 0x8a, "len": 2, "way": "devfile"},
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
            "VersionFile": {"cmd": "cat /sys/bus/i2c/devices/5-0030/cpld_version | sed 's/ //g'", "way": "cmd"},
            "desc": "Used for sff modules",
            "slot": 0,
            "type": "str",
            "warm": 0,
        },
        {
            "name": "FPGA",
            "cpld_id": "CPLD4",
            "VersionFile": {"loc": "/dev/fpga0", "offset": 0, "len": 4, "way": "devfile_ascii"},
            "desc": "Used for base functions",
            "slot": 0,
            "format": "little_endian",
            "warm": 1,
        },
        {
            "name": "BIOS",
            "cpld_id": "CPLD5",
            "VersionFile": {"cmd": "dmidecode -s bios-version", "way": "cmd"},
            "desc": "Performs initialization of hardware components during booting",
            "slot": 0,
            "type": "str",
            "warm": 0,
        }
    ],
    "dcdc": [
        {
            "name": "CTL_VDD3.3_V",
            "dcdc_id": "DCDC1",
            "Min": 3135,
            "value": {
                "loc": "/sys/bus/i2c/devices/9-0040/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3465,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "VDD3.3_V",
            "dcdc_id": "DCDC2",
            "Min": 3135,
            "value": {
                "loc": "/sys/bus/i2c/devices/9-0040/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3465,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SFP_VDD3.3_V",
            "dcdc_id": "DCDC3",
            "Min": 3135,
            "value": {
                "loc": "/sys/bus/i2c/devices/9-0040/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3465,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "PSU1_Output_V",
            "dcdc_id": "DCDC4",
            "Min": 11400,
            "value": {
                "loc": "/sys/bus/i2c/devices/10-0041/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 12600,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "PSU2_Output_V",
            "dcdc_id": "DCDC5",
            "Min": 11400,
            "value": {
                "loc": "/sys/bus/i2c/devices/10-0041/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 12600,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "VDD1.8_V",
            "dcdc_id": "DCDC6",
            "Min": 1710,
            "value": {
                "loc": "/sys/bus/i2c/devices/10-0041/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1890,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "VDD12V_F",
            "dcdc_id": "DCDC7",
            "Min": 11400,
            "value": {
                "loc": "/sys/bus/i2c/devices/0-005f/hwmon/hwmon*/in1_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 12600,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "VCC_P5V0_DDR",
            "dcdc_id": "DCDC8",
            "Min": 4500,
            "value": {
                "loc": "/sys/bus/i2c/devices/0-005f/hwmon/hwmon*/in2_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 5500,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "VDD_P1V1_V3K_MEM_S3",
            "dcdc_id": "DCDC9",
            "Min": 1067,
            "value": {
                "loc": "/sys/bus/i2c/devices/0-005f/hwmon/hwmon*/in3_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1133,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "VDD_P0V75_V3K_MISC_S5",
            "dcdc_id": "DCDC10",
            "Min": 723,
            "value": {
                "loc": "/sys/bus/i2c/devices/0-005f/hwmon/hwmon*/in4_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 773,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "VDD_P3V3_V3K_S5",
            "dcdc_id": "DCDC11",
            "Min": 3135,
            "value": {
                "loc": "/sys/bus/i2c/devices/0-005f/hwmon/hwmon*/in5_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3465,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "VDD_P1V8_V3K_S5",
            "dcdc_id": "DCDC12",
            "Min": 1710,
            "value": {
                "loc": "/sys/bus/i2c/devices/0-005f/hwmon/hwmon*/in6_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1890,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "VDD_P0V78_V3K_MEM_S0",
            "dcdc_id": "DCDC13",
            "Min": 741,
            "value": {
                "loc": "/sys/bus/i2c/devices/0-005f/hwmon/hwmon*/in11_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 819,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "VDD_P0V75_V3K_MISC_S0",
            "dcdc_id": "DCDC14",
            "Min": 723,
            "value": {
                "loc": "/sys/bus/i2c/devices/0-005f/hwmon/hwmon*/in12_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 773,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "VDD_P3V3_V3K_S0",
            "dcdc_id": "DCDC15",
            "Min": 3135,
            "value": {
                "loc": "/sys/bus/i2c/devices/0-005f/hwmon/hwmon*/in13_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 3465,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "VDD_P1V8_V3K_S0",
            "dcdc_id": "DCDC16",
            "Min": 1710,
            "value": {
                "loc": "/sys/bus/i2c/devices/0-005f/hwmon/hwmon*/in14_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1890,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "VDD_VR_P1V0_V3K_CORE",
            "dcdc_id": "DCDC17",
            "Min": 900,
            "value": {
                "loc": "/sys/bus/i2c/devices/0-005f/hwmon/hwmon*/in15_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "V",
            "Max": 1200,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "CTL_VDD3.3_C",
            "dcdc_id": "DCDC18",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/9-0040/hwmon/hwmon*/curr1_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "A",
            "Max": 1400,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "VDD3.3_C",
            "dcdc_id": "DCDC19",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/9-0040/hwmon/hwmon*/curr2_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "A",
            "Max": 4000,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "SFP_VDD3.3_C",
            "dcdc_id": "DCDC20",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/9-0040/hwmon/hwmon*/curr3_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "A",
            "Max": 1300,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "PSU1_Output_C",
            "dcdc_id": "DCDC21",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/10-0041/hwmon/hwmon*/curr1_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "A",
            "Max": 6130,
            "format": "float(float(%s)/1000)",
        },
        {
            "name": "PSU2_Output_C",
            "dcdc_id": "DCDC22",
            "Min": -1000,
            "value": {
                "loc": "/sys/bus/i2c/devices/10-0041/hwmon/hwmon*/curr2_input",
                "way": "sysfs",
            },
            "read_times": 5,
            "Unit": "A",
            "Max": 6130,
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
    }
}
