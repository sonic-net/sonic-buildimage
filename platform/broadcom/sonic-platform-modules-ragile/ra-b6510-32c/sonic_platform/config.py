# -*- coding: utf-8 -*-

PSU_FAN_AIRFLOW = {
    "CSU550AP-3-300": "F2B",
    "CSU550AP-3-500": "F2B",
    "DPS-550AB-39 A": "F2B",
    "DPS-1300AB-6 S": "F2B",
    "FSP1200-20ERM": "F2B",
    "CSU800AP-3-300": "F2B",
    "CSU550AP-3-501": "B2F",
    "DPS-550AB-40 A": "B2F",
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

    PSU_FAN_SPEED_MIN = 5220
    PSU_FAN_SPEED_MAX = 17400

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

    FAN_SPEED_MAX = 24000
    FAN_SPEED_MIN = 7200


class DecodeFormat:
    TEXT = 0
    DECIMAL = 1
    ONE_BIT_HEX = 2
    HUNDREDTH = 3
    THOUSANDTH = 4
    MILLIONTH = 5
    AND = 6
    JOIN = 7
    FRU = 8
    HEX = 9


class DecodeMethod:
    SYSFS = 0
    I2C = 1
    I2C_WORD = 2
    DEVMEM = 3
    SDK = 4
    IO = 5
    FRU = 6


class FRU:
    SN = 0
    VERSION = 1
    PART_NAME = 2
    PRODUCT_NAME = 3
    MANUFACTURER = 4


class Description:
    CPLD = "Used for managing IO modules, SFP+ modules and system LEDs"
    BIOS = "Performs initialization of hardware components during booting"
    FPGA = "Platform management controller for on-board temperature monitoring, in-chassis power, Fan and LED control"


FAN_LED_COLORS = {
    "green": 0b0100,
    "red": 0b0010,
    "amber": 0b0110,
}


DEVICE_CONF = {
    "eeprom": {"bus": 1, "loc": "0056"},
    "components": [
        {
            "name": "CPLD1 (MAC Board A)",
            "firmware_version": {
                "bus": 8,
                "addr": 0x30,
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
                "bus": 8,
                "addr": 0x31,
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
                "addr": 0x0d,
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
                "loc": "/sys/bus/i2c/devices/3-004b/hwmon/*/temp1_max",
                "format": DecodeFormat.THOUSANDTH,
            },
            "low": None,
            "crit_low": None,
            "crit_high": None,
            "temperature": {
                "loc": "/sys/bus/i2c/devices/3-004b/hwmon/*/temp1_input",
                "format": DecodeFormat.THOUSANDTH,
            },
        },
        {
            "name": "OUTLET TEMP",
            "high": {
                "loc": "/sys/bus/i2c/devices/3-004c/hwmon/*/temp1_max",
                "format": DecodeFormat.THOUSANDTH,
            },
            "low": None,
            "crit_low": None,
            "crit_high": None,
            "temperature": {
                "loc": "/sys/bus/i2c/devices/3-004c/hwmon/*/temp1_input",
                "format": DecodeFormat.THOUSANDTH,
            },
        },
        {
            "name": "BOARD TEMP",
            "high": {
                "loc": "/sys/bus/i2c/devices/3-0049/hwmon/*/temp1_max",
                "format": DecodeFormat.THOUSANDTH,
            },
            "low": None,
            "crit_low": None,
            "crit_high": None,
            "temperature": {
                "loc": "/sys/bus/i2c/devices/3-0049/hwmon/*/temp1_input",
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
            "e2loc": {"bus": 16, "addr": 0x50, "way": "i2c", "size": "256"},
            "present": {
                "loc": "/sys/bus/i2c/devices/2-000d/fan_present",
                "format": DecodeFormat.ONE_BIT_HEX,
                "bit": 0,
            },
            "status": {
                "loc": "/sys/bus/i2c/devices/2-000d/fan_status",
                "format": DecodeFormat.ONE_BIT_HEX,
                "bit": 0,
            },
            "hw_version": {
                "loc": "/sys/bus/i2c/devices/16-0050/eeprom",
                "format": DecodeFormat.FRU,
                "fru_key": FRU.VERSION
            },
            "sn": {
                "loc": "/sys/bus/i2c/devices/16-0050/eeprom",
                "format": DecodeFormat.FRU,
                "fru_key": FRU.SN
            },
            "led": {
                "loc": "/sys/bus/i2c/devices/2-000d/fan1_led",
                "format": DecodeFormat.AND,
                "mask": 0b1111,
            },
            "led_colors": FAN_LED_COLORS,
            "rotors": [
                {
                    "speed_getter": {
                        "loc": "/sys/bus/i2c/devices/2-000d/fan1_1_real_speed"
                    },
                    "speed_setter": {
                        "loc": "/sys/bus/i2c/devices/2-000d/fan1_speed_set",
                        "format": DecodeFormat.HEX
                    },
                    "speed_max": Threshold.FAN_SPEED_MAX,
                    "slope": 236.51,
                    "intercept": 82.571,
                },
                {
                    "speed_getter": {
                        "loc": "/sys/bus/i2c/devices/2-000d/fan1_2_real_speed"
                    },
                    "speed_setter": {
                        "loc": "/sys/bus/i2c/devices/2-000d/fan1_speed_set",
                        "format": DecodeFormat.HEX
                    },
                    "speed_max": Threshold.FAN_SPEED_MAX,
                    "slope": 236.51,
                    "intercept": 82.571,
                }
            ],
            "tolerance": 20,
            "threshold": 30,
            "target_default": 0,
        },
        {
            "name": "fan2",
            "e2loc": {"bus": 17, "addr": 0x50, "way": "i2c", "size": "256"},
            "present": {
                "loc": "/sys/bus/i2c/devices/2-000d/fan_present",
                "format": DecodeFormat.ONE_BIT_HEX,
                "bit": 1,
            },
            "status": {
                "loc": "/sys/bus/i2c/devices/2-000d/fan_status",
                "format": DecodeFormat.ONE_BIT_HEX,
                "bit": 1,
            },
            "hw_version": {
                "loc": "/sys/bus/i2c/devices/17-0050/eeprom",
                "format": DecodeFormat.FRU,
                "fru_key": FRU.VERSION
            },
            "sn": {
                "loc": "/sys/bus/i2c/devices/17-0050/eeprom",
                "format": DecodeFormat.FRU,
                "fru_key": FRU.SN
            },
            "led": {
                "loc": "/sys/bus/i2c/devices/2-000d/fan2_led",
                "format": DecodeFormat.AND,
                "mask": 0b1111,
            },
            "led_colors": FAN_LED_COLORS,
            "rotors": [
                {
                    "speed_getter": {
                        "loc": "/sys/bus/i2c/devices/2-000d/fan2_1_real_speed"
                    },
                    "speed_setter": {
                        "loc": "/sys/bus/i2c/devices/2-000d/fan2_speed_set",
                        "format": DecodeFormat.HEX
                    },
                    "speed_max": Threshold.FAN_SPEED_MAX,
                    "slope": 236.51,
                    "intercept": 82.571,
                },
                {
                    "speed_getter": {
                        "loc": "/sys/bus/i2c/devices/2-000d/fan2_2_real_speed"
                    },
                    "speed_setter": {
                        "loc": "/sys/bus/i2c/devices/2-000d/fan2_speed_set",
                        "format": DecodeFormat.HEX,
                    },
                    "speed_max": Threshold.FAN_SPEED_MAX,
                    "slope": 236.51,
                    "intercept": 82.571,
                }
            ],
            "tolerance": 20,
            "threshold": 30,
            "target_default": 0,
        },
        {
            "name": "fan3",
            "e2loc": {"bus": 18, "addr": 0x50, "way": "i2c", "size": "256"},
            "present": {
                "loc": "/sys/bus/i2c/devices/2-000d/fan_present",
                "format": DecodeFormat.ONE_BIT_HEX,
                "bit": 2,
            },
            "status": {
                "loc": "/sys/bus/i2c/devices/2-000d/fan_status",
                "format": DecodeFormat.ONE_BIT_HEX,
                "bit": 2,
            },
            "hw_version": {
                "loc": "/sys/bus/i2c/devices/18-0050/eeprom",
                "format": DecodeFormat.FRU,
                "fru_key": FRU.VERSION
            },
            "sn": {
                "loc": "/sys/bus/i2c/devices/18-0050/eeprom",
                "format": DecodeFormat.FRU,
                "fru_key": FRU.SN
            },
            "led": {
                "loc": "/sys/bus/i2c/devices/2-000d/fan3_led",
                "format": DecodeFormat.AND,
                "mask": 0b1111,
            },
            "led_colors": FAN_LED_COLORS,
            "rotors": [
                {
                    "speed_getter": {
                        "loc": "/sys/bus/i2c/devices/2-000d/fan3_1_real_speed"
                    },
                    "speed_setter": {
                        "loc": "/sys/bus/i2c/devices/2-000d/fan3_speed_set",
                        "format": DecodeFormat.HEX
                    },
                    "speed_max": Threshold.FAN_SPEED_MAX,
                    "slope": 236.51,
                    "intercept": 82.571,
                },
                {
                    "speed_getter": {
                        "loc": "/sys/bus/i2c/devices/2-000d/fan3_2_real_speed"
                    },
                    "speed_setter": {
                        "loc": "/sys/bus/i2c/devices/2-000d/fan3_speed_set",
                        "format": DecodeFormat.HEX
                    },
                    "speed_max": Threshold.FAN_SPEED_MAX,
                    "slope": 236.51,
                    "intercept": 82.571,
                }
            ],
            "tolerance": 20,
            "threshold": 30,
            "target_default": 0,
        },
        {
            "name": "fan4",
            "e2loc": {"bus": 19, "addr": 0x50, "way": "i2c", "size": "256"},
            "present": {
                "loc": "/sys/bus/i2c/devices/2-000d/fan_present",
                "format": DecodeFormat.ONE_BIT_HEX,
                "bit": 3,
            },
            "status": {
                "loc": "/sys/bus/i2c/devices/2-000d/fan_status",
                "format": DecodeFormat.ONE_BIT_HEX,
                "bit": 3,
            },
            "hw_version": {
                "loc": "/sys/bus/i2c/devices/19-0050/eeprom",
                "format": DecodeFormat.FRU,
                "fru_key": FRU.VERSION
            },
            "sn": {
                "loc": "/sys/bus/i2c/devices/19-0050/eeprom",
                "format": DecodeFormat.FRU,
                "fru_key": FRU.SN
            },
            "led": {
                "loc": "/sys/bus/i2c/devices/2-000d/fan4_led",
                "format": DecodeFormat.AND,
                "mask": 0b1111,
            },
            "led_colors": FAN_LED_COLORS,
            "rotors": [
                {
                    "speed_getter": {
                        "loc": "/sys/bus/i2c/devices/2-000d/fan4_1_real_speed"
                    },
                    "speed_setter": {
                        "loc": "/sys/bus/i2c/devices/2-000d/fan4_speed_set",
                        "format": DecodeFormat.HEX
                    },
                    "speed_max": Threshold.FAN_SPEED_MAX,
                    "slope": 236.51,
                    "intercept": 82.571,
                },
                {
                    "speed_getter": {
                        "loc": "/sys/bus/i2c/devices/2-000d/fan4_2_real_speed"
                    },
                    "speed_setter": {
                        "loc": "/sys/bus/i2c/devices/2-000d/fan4_speed_set",
                        "format": DecodeFormat.HEX
                    },
                    "speed_max": Threshold.FAN_SPEED_MAX,
                    "slope": 236.51,
                    "intercept": 82.571,
                }
            ],
            "tolerance": 20,
            "threshold": 30,
            "target_default": 0,
        },
        {
            "name": "fan5",
            "e2loc": {"bus": 20, "addr": 0x50, "way": "i2c", "size": "256"},
            "present": {
                "loc": "/sys/bus/i2c/devices/2-000d/fan_present",
                "format": DecodeFormat.ONE_BIT_HEX,
                "bit": 4,
            },
            "status": {
                "loc": "/sys/bus/i2c/devices/2-000d/fan_status",
                "format": DecodeFormat.ONE_BIT_HEX,
                "bit": 4,
            },
            "hw_version": {
                "loc": "/sys/bus/i2c/devices/20-0050/eeprom",
                "format": DecodeFormat.FRU,
                "fru_key": FRU.VERSION
            },
            "sn": {
                "loc": "/sys/bus/i2c/devices/20-0050/eeprom",
                "format": DecodeFormat.FRU,
                "fru_key": FRU.SN
            },
            "led": {
                "loc": "/sys/bus/i2c/devices/2-000d/fan5_led",
                "format": DecodeFormat.AND,
                "mask": 0b1111,
            },
            "led_colors": FAN_LED_COLORS,
            "rotors": [
                {
                    "speed_getter": {
                        "loc": "/sys/bus/i2c/devices/2-000d/fan5_1_real_speed"
                    },
                    "speed_setter": {
                        "loc": "/sys/bus/i2c/devices/2-000d/fan5_speed_set",
                        "format": DecodeFormat.HEX
                    },
                    "speed_max": Threshold.FAN_SPEED_MAX,
                    "slope": 236.51,
                    "intercept": 82.571,
                },
                {
                    "speed_getter": {
                        "loc": "/sys/bus/i2c/devices/2-000d/fan5_2_real_speed"
                    },
                    "speed_setter": {
                        "loc": "/sys/bus/i2c/devices/2-000d/fan5_speed_set",
                        "format": DecodeFormat.HEX
                    },
                    "speed_max": Threshold.FAN_SPEED_MAX,
                    "slope": 236.51,
                    "intercept": 82.571,
                }
            ],
            "tolerance": 20,
            "threshold": 30,
            "target_default": 0,
        },
    ],
    "psus": [
        {
            "name": "psu1",
            "present": {
                "addr": 0x951,
                "format": DecodeFormat.ONE_BIT_HEX,
                "bit": 0,
                "way": DecodeMethod.IO
            },
            "status": {
                "addr": 0x951,
                "format": DecodeFormat.ONE_BIT_HEX,
                "bit": 1,
                "way": DecodeMethod.IO
            },
            "sn": {
                "loc": "/sys/bus/i2c/devices/24-0050/eeprom",
                "format": DecodeFormat.FRU,
                "fru_key": FRU.SN
            },
            "in_current": {
                "loc": "/sys/bus/i2c/devices/24-0058/hwmon/*/curr1_input",
                "format": DecodeFormat.THOUSANDTH,
            },
            "in_voltage": {
                "loc": "/sys/bus/i2c/devices/24-0058/hwmon/*/in1_input",
                "format": DecodeFormat.THOUSANDTH,
            },
            "out_voltage": {
                "loc": "/sys/bus/i2c/devices/24-0058/hwmon/*/in2_input",
                "format": DecodeFormat.THOUSANDTH,
            },
            "out_current": {
                "loc": "/sys/bus/i2c/devices/24-0058/hwmon/*/curr2_input",
                "format": DecodeFormat.THOUSANDTH,
            },
            "temperature": {
                "loc": "/sys/bus/i2c/devices/24-0058/hwmon/*/temp1_input",
                "format": DecodeFormat.THOUSANDTH,
            },
            "hw_version": {
                "loc": "/sys/bus/i2c/devices/24-0050/eeprom",
                "format": DecodeFormat.FRU,
                "fru_key": FRU.VERSION
            },
            # "psu_type": {
            #     "loc": "/sys/bus/i2c/devices/24-0050/eeprom",
            #     "format": DecodeFormat.FRU,
            #     "fru_key": FRU.SN
            # },
            "fans": [
                {
                    "name": "psu_fan1",
                    "present": {
                        "loc": "/sys/bus/i2c/devices/24-0058/hwmon/*/fan1_fault",
                    },
                    "status": {
                        "addr": 0x951,
                        "format": DecodeFormat.ONE_BIT_HEX,
                        "bit": 1,
                        "way": DecodeMethod.IO
                    },
                    "rotors": [
                        {
                            "speed_getter": {
                                "loc": "/sys/bus/i2c/devices/24-0058/hwmon/*/fan1_input"
                            },
                            "speed_setter": {
                                "bus": 24,
                                "addr": 0x58,
                                "offset": 0x3b,
                                "size": 1,
                                "way": DecodeMethod.I2C,
                                "format": DecodeFormat.HEX,
                            },
                            "speed_max": Threshold.PSU_FAN_SPEED_MAX,
                        }
                    ],
                    "tolerance": 20,
                    "threshold_low": 1900,
                }
            ],
            "in_power": {
                "loc": "/sys/bus/i2c/devices/24-0058/hwmon/*/power1_input",
                "format": DecodeFormat.MILLIONTH,
            },
            "out_power": {
                "loc": "/sys/bus/i2c/devices/24-0058/hwmon/*/power2_input",
                "format": DecodeFormat.MILLIONTH,
            },
        },
        {
            "name": "psu2",
            "present": {
                "addr": 0x951,
                "format": DecodeFormat.ONE_BIT_HEX,
                "bit": 4,
                "way": DecodeMethod.IO
            },
            "status": {
                "addr": 0x951,
                "format": DecodeFormat.ONE_BIT_HEX,
                "bit": 5,
                "way": DecodeMethod.IO
            },
            "sn": {
                "loc": "/sys/bus/i2c/devices/25-0050/eeprom",
                "format": DecodeFormat.FRU,
                "fru_key": FRU.SN
            },
            "in_current": {
                "loc": "/sys/bus/i2c/devices/25-0058/hwmon/*/curr1_input",
                "format": DecodeFormat.THOUSANDTH,
            },
            "in_voltage": {
                "loc": "/sys/bus/i2c/devices/25-0058/hwmon/*/in1_input",
                "format": DecodeFormat.THOUSANDTH,
            },
            "out_voltage": {
                "loc": "/sys/bus/i2c/devices/25-0058/hwmon/*/in2_input",
                "format": DecodeFormat.THOUSANDTH,
            },
            "out_current": {
                "loc": "/sys/bus/i2c/devices/25-0058/hwmon/*/curr2_input",
                "format": DecodeFormat.THOUSANDTH,
            },
            "temperature": {
                "loc": "/sys/bus/i2c/devices/25-0058/hwmon/*/temp1_input",
                "format": DecodeFormat.THOUSANDTH,
            },
            "hw_version": {
                "loc": "/sys/bus/i2c/devices/25-0050/eeprom",
                "format": DecodeFormat.FRU,
                "fru_key": FRU.VERSION
            },
            # "psu_type": {"loc": "/sys/bus/i2c/devices/8-0053/psu_type"},
            "fans": [
                {
                    "name": "psu_fan1",
                    "present": {
                        "loc": "/sys/bus/i2c/devices/25-0058/hwmon/*/fan1_fault",
                    },
                    "status": {
                        "addr": 0x951,
                        "format": DecodeFormat.ONE_BIT_HEX,
                        "bit": 5,
                        "way": DecodeMethod.IO
                    },
                    "rotors": [
                        {
                            "speed_getter": {
                                "loc": "/sys/bus/i2c/devices/25-0058/hwmon/*/fan1_input"
                            },
                            "speed_setter": {
                                "bus": 25,
                                "addr": 0x58,
                                "offset": 0x3b,
                                "size": 1,
                                "way": DecodeMethod.I2C,
                                "format": DecodeFormat.HEX,
                            },
                            "speed_max": Threshold.PSU_FAN_SPEED_MAX,
                        }
                    ],
                    "tolerance": 20,
                    "threshold_low": 1900,
                }
            ],
            "in_power": {
                "loc": "/sys/bus/i2c/devices/25-0058/hwmon/*/power1_input",
                "format": DecodeFormat.MILLIONTH,
            },
            "out_power": {
                "loc": "/sys/bus/i2c/devices/25-0058/hwmon/*/power2_input",
                "format": DecodeFormat.MILLIONTH,
            },
        },
    ],
}
