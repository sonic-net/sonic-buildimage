# -*- coding: utf-8 -*-

PSU_FAN_AIRFLOW = {
    "CSU550AP-3-300": "F2B",
    "AS-40FAN-01-F-RJ": "F2B",
    "CSU550AP-3-500": "F2B",
    "DPS-550AB-39 A": "F2B",
    "DPS-1300AB-6 S": "F2B",
    "FSP1200-20ERM": "F2B",
    "CSU800AP-3-300": "F2B",
    "CSU550AP-3-501": "B2F",
    "DPS-550AB-40 A": "B2F",
}

FAN_AIRFLOW = {
    "AS-80FAN-01-F-RJ": "F2B",
    "AS-40FAN-01-F-RJ": "F2B",
    "AS-80FAN-01-R-RJ": "B2F",
    "AS-40FAN-01-R-RJ": "B2F",
}

psutypedecode = {
    0x00: "N/A",
    0x01: "AC",
    0x02: "DC",
}


class Unit:
    Temperature = "C"
    Voltage = "V"
    Current = "A"
    Power = "W"
    Speed = "RPM"


class Threshold:
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

    PSU_OUTPUT_POWER_MIN = 10 * 1000
    PSU_OUTPUT_POWER_MAX = 1300 * 1000

    PSU_INPUT_POWER_MIN = 10 * 1000
    PSU_INPUT_POWER_MAX = 1444 * 1000

    PSU_OUTPUT_CURRENT_MIN = 2 * 1000
    PSU_OUTPUT_CURRENT_MAX = 107 * 1000

    PSU_INPUT_CURRENT_MIN = 0.2 * 1000
    PSU_INPUT_CURRENT_MAX = 7 * 1000

    FAN_SPEED_MAX = 23000
    FAN_SPEED_MIN = 500


class DecodeFormat:
    TEXT = 0
    DECIMAL = 1
    ONE_BIT_HEX = 2
    HUNDREDTH = 3
    THOUSANDTH = 4
    MILLIONTH = 5
    AND = 6
    JOIN = 7


class DecodeMethod:
    SYSFS = 0
    I2C = 1
    I2C_WORD = 2
    DEVMEM = 3
    SDK = 4


class Description:
    CPLD = "Used for managing IO modules, SFP+ modules and system LEDs"
    BIOS = "Performs initialization of hardware components during booting"
    FPGA = "Platform management controller for on-board temperature monitoring, in-chassis power, Fan and LED control"


FAN_LED_COLORS = {
    "green": 0b1001,
    "red": 0b1010,
    "amber": 0b0011,
}


DEVICE_CONF = {
    "eeprom": {"bus": 2, "loc": "0057"},
    "components": [
        {
            "name": "CPLD1 (MAC Board A)",
            "firmware_version": {
                "bus": 2,
                "addr": 0x33,
                "offset": 0,
                "size": 4,
                "way": DecodeMethod.I2C,
                "format": DecodeFormat.JOIN,
                "sep": "/",
            },
            "desc": Description.CPLD,
            "slot": 0,
        },
        {
            "name": "CPLD2 (MAC Board B)",
            "firmware_version": {
                "bus": 2,
                "addr": 0x35,
                "offset": 0,
                "size": 4,
                "way": DecodeMethod.I2C,
                "format": DecodeFormat.JOIN,
                "sep": "/",
            },
            "desc": Description.CPLD,
            "slot": 0,
        },
        {
            "name": "CPLD3 (CONNECT Board A)",
            "firmware_version": {
                "bus": 2,
                "addr": 0x37,
                "offset": 0,
                "size": 4,
                "way": DecodeMethod.I2C,
                "format": DecodeFormat.JOIN,
                "sep": "/",
            },
            "desc": Description.CPLD,
            "slot": 0,
        },
        {
            "name": "CPLD4 (CPU Board)",
            "firmware_version": {
                "bus": 0,
                "addr": 0x0D,
                "offset": 0,
                "size": 4,
                "way": DecodeMethod.I2C,
                "format": DecodeFormat.JOIN,
                "sep": "/",
            },
            "desc": Description.CPLD,
            "slot": 1,
        },
    ],
    "thermals": [
        {
            "name": "INLET TEMP",
            "high": {
                "loc": "/sys/bus/i2c/devices/2-0048/hwmon/*/temp1_max",
                "format": DecodeFormat.THOUSANDTH,
            },
            "low": None,
            "crit_low": None,
            "crit_high": None,
            "temperature": {
                "loc": "/sys/bus/i2c/devices/2-0048/hwmon/*/temp1_input",
                "format": DecodeFormat.THOUSANDTH,
            },
        },
        {
            "name": "OUTLET TEMP",
            "high": {
                "loc": "/sys/bus/i2c/devices/2-0049/hwmon/*/temp1_max",
                "format": DecodeFormat.THOUSANDTH,
            },
            "low": None,
            "crit_low": None,
            "crit_high": None,
            "temperature": {
                "loc": "/sys/bus/i2c/devices/2-0049/hwmon/*/temp1_input",
                "format": DecodeFormat.THOUSANDTH,
            },
        },
        {
            "name": "BOARD TEMP",
            "high": {
                "loc": "/sys/bus/i2c/devices/2-004a/hwmon/*/temp1_max",
                "format": DecodeFormat.THOUSANDTH,
            },
            "low": None,
            "crit_low": None,
            "crit_high": None,
            "temperature": {
                "loc": "/sys/bus/i2c/devices/2-004a/hwmon/*/temp1_input",
                "format": DecodeFormat.THOUSANDTH,
            },
        },
        {
            "name": "PHYSICAL ID 0",
            "high": {
                "loc": "/sys/class/hwmon/hwmon0/temp1_max",
                "format": DecodeFormat.THOUSANDTH,
            },
            "low": None,
            "crit_low": None,
            "crit_high": {
                "loc": "/sys/class/hwmon/hwmon0/temp1_crit",
                "format": DecodeFormat.THOUSANDTH,
            },
            "temperature": {
                "loc": "/sys/class/hwmon/hwmon0/temp1_input",
                "format": DecodeFormat.THOUSANDTH,
            },
        },
        {
            "name": "CPU CORE 0",
            "high": {
                "loc": "/sys/class/hwmon/hwmon0/temp2_max",
                "format": DecodeFormat.THOUSANDTH,
            },
            "low": None,
            "crit_low": None,
            "crit_high": {
                "loc": "/sys/class/hwmon/hwmon0/temp2_crit",
                "format": DecodeFormat.THOUSANDTH,
            },
            "temperature": {
                "loc": "/sys/class/hwmon/hwmon0/temp2_input",
                "format": DecodeFormat.THOUSANDTH,
            },
        },
        {
            "name": "CPU CORE 1",
            "high": {
                "loc": "/sys/class/hwmon/hwmon0/temp3_max",
                "format": DecodeFormat.THOUSANDTH,
            },
            "low": None,
            "crit_low": None,
            "crit_high": {
                "loc": "/sys/class/hwmon/hwmon0/temp3_crit",
                "format": DecodeFormat.THOUSANDTH,
            },
            "temperature": {
                "loc": "/sys/class/hwmon/hwmon0/temp3_input",
                "format": DecodeFormat.THOUSANDTH,
            },
        },
        {
            "name": "CPU CORE 2",
            "high": {
                "loc": "/sys/class/hwmon/hwmon0/temp4_max",
                "format": DecodeFormat.THOUSANDTH,
            },
            "low": None,
            "crit_low": None,
            "crit_high": {
                "loc": "/sys/class/hwmon/hwmon0/temp4_crit",
                "format": DecodeFormat.THOUSANDTH,
            },
            "temperature": {
                "loc": "/sys/class/hwmon/hwmon0/temp4_input",
                "format": DecodeFormat.THOUSANDTH,
            },
        },
        {
            "name": "CPU CORE 3",
            "high": {
                "loc": "/sys/class/hwmon/hwmon0/temp5_max",
                "format": DecodeFormat.THOUSANDTH,
            },
            "low": None,
            "crit_low": None,
            "crit_high": {
                "loc": "/sys/class/hwmon/hwmon0/temp5_crit",
                "format": DecodeFormat.THOUSANDTH,
            },
            "temperature": {
                "loc": "/sys/class/hwmon/hwmon0/temp5_input",
                "format": DecodeFormat.THOUSANDTH,
            },
        },
    ],
    "fans": [
        {
            "name": "fan1",
            "e2loc": {"bus": 3, "addr": 0x53, "way": "i2c", "size": "256"},
            "present": {
                "loc": "/sys/bus/i2c/devices/2-0037/fan_present",
                "format": DecodeFormat.ONE_BIT_HEX,
                "bit": 0,
            },
            "status": {
                "loc": "/sys/bus/i2c/devices/2-0037/fan_status",
                "format": DecodeFormat.ONE_BIT_HEX,
                "bit": 0,
            },
            "hw_version": {"loc": "/sys/bus/i2c/devices/3-0053/fan_hw_version"},
            "sn": {"loc": "/sys/bus/i2c/devices/3-0053/fan_sn"},
            "led": {
                "loc": "/sys/bus/i2c/devices/0-0032/fan0_led",
                "format": DecodeFormat.AND,
                "mask": 0b1011,
            },
            "led_colors": FAN_LED_COLORS,
            "rotors": [
                {
                    "speed_getter": {
                        "loc": "/sys/bus/i2c/devices/2-0037/hwmon/*/fan1_input"
                    },
                    "speed_setter": {
                        "loc": "/sys/bus/i2c/devices/0-0032/fan_speed_set"
                    },
                    "speed_max": Threshold.FAN_SPEED_MAX,
                }
            ],
        },
        {
            "name": "fan2",
            "e2loc": {"bus": 4, "addr": 0x53, "way": "i2c", "size": "256"},
            "present": {
                "loc": "/sys/bus/i2c/devices/2-0037/fan_present",
                "format": DecodeFormat.ONE_BIT_HEX,
                "bit": 1,
            },
            "status": {
                "loc": "/sys/bus/i2c/devices/2-0037/fan_status",
                "format": DecodeFormat.ONE_BIT_HEX,
                "bit": 1,
            },
            "hw_version": {"loc": "/sys/bus/i2c/devices/4-0053/fan_hw_version"},
            "sn": {"loc": "/sys/bus/i2c/devices/4-0053/fan_sn"},
            "led": {
                "loc": "/sys/bus/i2c/devices/0-0032/fan1_led",
                "format": DecodeFormat.AND,
                "mask": 0b1011,
            },
            "led_colors": FAN_LED_COLORS,
            "rotors": [
                {
                    "speed_getter": {
                        "loc": "/sys/bus/i2c/devices/2-0037/hwmon/*/fan2_input"
                    },
                    "speed_setter": {
                        "loc": "/sys/bus/i2c/devices/0-0032/fan_speed_set"
                    },
                    "speed_max": Threshold.FAN_SPEED_MAX,
                }
            ],
        },
        {
            "name": "fan3",
            "e2loc": {"bus": 3, "addr": 0x53, "way": "i2c", "size": "256"},
            "present": {
                "loc": "/sys/bus/i2c/devices/2-0037/fan_present",
                "format": DecodeFormat.ONE_BIT_HEX,
                "bit": 2,
            },
            "status": {
                "loc": "/sys/bus/i2c/devices/2-0037/fan_status",
                "format": DecodeFormat.ONE_BIT_HEX,
                "bit": 2,
            },
            "hw_version": {"loc": "/sys/bus/i2c/devices/5-0053/fan_hw_version"},
            "sn": {"loc": "/sys/bus/i2c/devices/5-0053/fan_sn"},
            "led": {
                "loc": "/sys/bus/i2c/devices/0-0032/fan2_led",
                "format": DecodeFormat.AND,
                "mask": 0b1011,
            },
            "led_colors": FAN_LED_COLORS,
            "rotors": [
                {
                    "speed_getter": {
                        "loc": "/sys/bus/i2c/devices/2-0037/hwmon/*/fan3_input"
                    },
                    "speed_setter": {
                        "loc": "/sys/bus/i2c/devices/0-0032/fan_speed_set"
                    },
                    "speed_max": Threshold.FAN_SPEED_MAX,
                }
            ],
        },
        {
            "name": "fan4",
            "e2loc": {"bus": 3, "addr": 0x53, "way": "i2c", "size": "256"},
            "present": {
                "loc": "/sys/bus/i2c/devices/2-0037/fan_present",
                "format": DecodeFormat.ONE_BIT_HEX,
                "bit": 3,
            },
            "status": {
                "loc": "/sys/bus/i2c/devices/2-0037/fan_status",
                "format": DecodeFormat.ONE_BIT_HEX,
                "bit": 3,
            },
            "hw_version": {"loc": "/sys/bus/i2c/devices/6-0053/fan_hw_version"},
            "sn": {"loc": "/sys/bus/i2c/devices/6-0053/fan_sn"},
            "led": {
                "loc": "/sys/bus/i2c/devices/0-0032/fan3_led",
                "format": DecodeFormat.AND,
                "mask": 0b1011,
            },
            "led_colors": FAN_LED_COLORS,
            "rotors": [
                {
                    "speed_getter": {
                        "loc": "/sys/bus/i2c/devices/2-0037/hwmon/*/fan4_input"
                    },
                    "speed_setter": {
                        "loc": "/sys/bus/i2c/devices/0-0032/fan_speed_set"
                    },
                    "speed_max": Threshold.FAN_SPEED_MAX,
                }
            ],
        },
    ],
    "psus": [
        {
            "name": "psu1",
            "present": {
                "loc": "/sys/bus/i2c/devices/2-0037/psu_status",
                "format": DecodeFormat.ONE_BIT_HEX,
                "bit": 0,
            },
            "status": {
                "loc": "/sys/bus/i2c/devices/2-0037/psu_status",
                "format": DecodeFormat.ONE_BIT_HEX,
                "bit": 1,
            },
            "sn": {"loc": "/sys/bus/i2c/devices/7-0050/psu_sn"},
            "in_current": {
                "loc": "/sys/bus/i2c/devices/7-0058/hwmon/*/curr1_input",
                "format": DecodeFormat.THOUSANDTH,
            },
            "in_voltage": {
                "loc": "/sys/bus/i2c/devices/7-0058/hwmon/*/in1_input",
                "format": DecodeFormat.THOUSANDTH,
            },
            "out_voltage": {
                "loc": "/sys/bus/i2c/devices/7-0058/hwmon/*/in2_input",
                "format": DecodeFormat.THOUSANDTH,
            },
            "out_current": {
                "loc": "/sys/bus/i2c/devices/7-0058/hwmon/*/curr2_input",
                "format": DecodeFormat.THOUSANDTH,
            },
            "temperature": {
                "loc": "/sys/bus/i2c/devices/7-0058/hwmon/*/temp1_input",
                "format": DecodeFormat.THOUSANDTH,
            },
            "hw_version": {"loc": "/sys/bus/i2c/devices/7-0050/psu_hw"},
            "psu_type": {"loc": "/sys/bus/i2c/devices/7-0050/psu_type"},
            "fans": [
                {
                    "name": "psu_fan1",
                    "present": {
                        "loc": "/sys/bus/i2c/devices/7-0058/hwmon/*/fan1_fault",
                    },
                    "status": {
                        "loc": "/sys/bus/i2c/devices/2-0037/psu_status",
                        "format": DecodeFormat.ONE_BIT_HEX,
                        "bit": 1,
                    },
                    "rotors": [
                        {
                            "speed_getter": {
                                "loc": "/sys/bus/i2c/devices/7-0058/hwmon/*/fan1_input"
                            },
                            "speed_max": Threshold.PSU_FAN_SPEED_MAX,
                        }
                    ],
                }
            ],
            "in_power": {
                "loc": "/sys/bus/i2c/devices/7-0058/hwmon/*/power1_input",
                "format": DecodeFormat.MILLIONTH,
            },
            "out_power": {
                "loc": "/sys/bus/i2c/devices/7-0058/hwmon/*/power2_input",
                "format": DecodeFormat.MILLIONTH,
            },
        },
        {
            "name": "psu2",
            "present": {
                "loc": "/sys/bus/i2c/devices/2-0037/psu_status",
                "format": DecodeFormat.ONE_BIT_HEX,
                "bit": 4,
            },
            "status": {
                "loc": "/sys/bus/i2c/devices/2-0037/psu_status",
                "format": DecodeFormat.ONE_BIT_HEX,
                "bit": 5,
            },
            "sn": {"loc": "/sys/bus/i2c/devices/8-0053/psu_sn"},
            "in_current": {
                "loc": "/sys/bus/i2c/devices/8-005b/hwmon/*/curr1_input",
                "format": DecodeFormat.THOUSANDTH,
            },
            "in_voltage": {
                "loc": "/sys/bus/i2c/devices/8-005b/hwmon/*/in1_input",
                "format": DecodeFormat.THOUSANDTH,
            },
            "out_voltage": {
                "loc": "/sys/bus/i2c/devices/8-005b/hwmon/*/in2_input",
                "format": DecodeFormat.THOUSANDTH,
            },
            "out_current": {
                "loc": "/sys/bus/i2c/devices/8-005b/hwmon/*/curr2_input",
                "format": DecodeFormat.THOUSANDTH,
            },
            "temperature": {
                "loc": "/sys/bus/i2c/devices/8-005b/hwmon/*/temp1_input",
                "format": DecodeFormat.THOUSANDTH,
            },
            "hw_version": {"loc": "/sys/bus/i2c/devices/8-0053/psu_hw"},
            "psu_type": {"loc": "/sys/bus/i2c/devices/8-0053/psu_type"},
            "fans": [
                {
                    "name": "psu_fan1",
                    "present": {
                        "loc": "/sys/bus/i2c/devices/8-005b/hwmon/*/fan1_fault",
                    },
                    "status": {
                        "loc": "/sys/bus/i2c/devices/2-0037/psu_status",
                        "format": DecodeFormat.ONE_BIT_HEX,
                        "bit": 5,
                    },
                    "rotors": [
                        {
                            "speed_getter": {
                                "loc": "/sys/bus/i2c/devices/8-005b/hwmon/*/fan1_input"
                            },
                            "speed_max": Threshold.PSU_FAN_SPEED_MAX,
                        }
                    ],
                }
            ],
            "in_power": {
                "loc": "/sys/bus/i2c/devices/8-005b/hwmon/*/power1_input",
                "format": DecodeFormat.MILLIONTH,
            },
            "out_power": {
                "loc": "/sys/bus/i2c/devices/8-005b/hwmon/*/power2_input",
                "format": DecodeFormat.MILLIONTH,
            },
        },
    ],
}
