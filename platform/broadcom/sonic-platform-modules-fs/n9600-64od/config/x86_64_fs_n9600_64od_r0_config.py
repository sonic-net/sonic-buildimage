#!/usr/bin/python3
# -*- coding: UTF-8 -*-
from platform_common import *

STARTMODULE = {
    "hal_fanctrl": 1,
    "hal_ledctrl": 1,
    "avscontrol": 1,
    "tty_console": 1,
    "dev_monitor": 1,
    "pmon_syslog": 1,
    "sff_temp_polling": 1,
    "reboot_cause": 1,
    "product_name": 1,
    "generate_airflow": 1,
}

DEV_MONITOR_PARAM = {
    "polling_time": 10,
    "psus": [
        {
            "name": "psu1",
            "present": {"gettype": "io", "io_addr": 0x958, "presentbit": 2, "okval": 0},
             "device": [
                {"id": "psu1pmbus", "name": "wb_fsp1200", "bus": 42, "loc": 0x58, "attr": "hwmon"},
                {"id": "psu1frue2", "name": "wb_24c02", "bus": 42, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "psu2",
            "present": {"gettype": "io", "io_addr": 0x958, "presentbit": 6, "okval": 0},
            "device": [
                {"id": "psu2pmbus", "name": "wb_fsp1200", "bus": 43, "loc": 0x58, "attr": "hwmon"},
                {"id": "psu2frue2", "name": "wb_24c02", "bus": 43, "loc": 0x50, "attr": "eeprom"},
            ],
        },
    ],
    "fans": [
        {
            "name": "fan1",
            "present": {"gettype": "devfile", "path": "/dev/cpld10", "offset": 0x5b, "read_len":1, "presentbit": 0, "okval": 0},
            "device": [
                {"id": "fan1frue2", "name": "wb_24c64", "bus": 52, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan2",
            "present": {"gettype": "devfile", "path": "/dev/cpld10", "offset": 0x5b, "read_len":1, "presentbit": 1, "okval": 0},
            "device": [
                {"id": "fan2frue2", "name": "wb_24c64", "bus": 53, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan3",
            "present": {"gettype": "devfile", "path": "/dev/cpld10", "offset": 0x5b, "read_len":1, "presentbit": 2, "okval": 0},
            "device": [
                {"id": "fan3frue2", "name": "wb_24c64", "bus": 54, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan4",
            "present": {"gettype": "devfile", "path": "/dev/cpld10", "offset": 0x5b, "read_len":1, "presentbit": 3, "okval": 0},
            "device": [
                {"id": "fan4frue2", "name": "wb_24c64", "bus": 55, "loc": 0x50, "attr": "eeprom"},
            ],
        },
    ],
    "others": [
        {
            "name": "eeprom",
            "device": [
                {"id": "eeprom_1", "name": "wb_24c02", "bus": 1, "loc": 0x56, "attr": "eeprom"},
            ],
        },
        {
            "name": "lm75",
            "device": [
                {"id": "lm75_1", "name": "wb_lm75", "bus": 51, "loc": 0x4b, "attr": "hwmon"},
                {"id": "lm75_2", "name": "wb_lm75", "bus": 56, "loc": 0x4e, "attr": "hwmon"},
                {"id": "lm75_3", "name": "wb_lm75", "bus": 58, "loc": 0x4b, "attr": "hwmon"},
                {"id": "lm75_4", "name": "wb_lm75", "bus": 75, "loc": 0x4b, "attr": "hwmon"},
                {"id": "lm75_5", "name": "wb_lm75", "bus": 76, "loc": 0x4f, "attr": "hwmon"},
            ],
        },
        {
            "name":"ct7318",
            "device":[
                {"id":"ct7318_1", "name":"ct7318","bus":77, "loc":0x4c, "attr":"hwmon"},
                {"id":"ct7318_2", "name":"ct7318","bus":78, "loc":0x4c, "attr":"hwmon"},
            ],
        },
        {
            "name": "ucd90160",
            "device": [
                {"id": "wb_ucd90160_1", "name": "wb_ucd90160", "bus": 68, "loc": 0x5b, "attr": "hwmon"},
                {"id": "wb_ucd90160_2", "name": "wb_ucd90160", "bus": 69, "loc": 0x5f, "attr": "hwmon"},
                {"id": "wb_ucd90160_3", "name": "wb_ucd90160", "bus": 82, "loc": 0x5b, "attr": "hwmon"},
                {"id": "wb_ucd90160_4", "name": "wb_ucd90160", "bus": 83, "loc": 0x5b, "attr": "hwmon"},
            ],
        },
        {
            "name": "xdpe12284",
            "device": [
                {"id": "xdpe12284_1", "name": "wb_xdpe12284", "bus": 90, "loc": 0x70, "attr": "hwmon"},
                {"id": "xdpe12284_2", "name": "wb_xdpe12284", "bus": 91, "loc": 0x70, "attr": "hwmon"},
                {"id": "xdpe12284_3", "name": "wb_xdpe12284", "bus": 92, "loc": 0x70, "attr": "hwmon"},
                {"id": "xdpe12284_4", "name": "wb_xdpe12284", "bus": 93, "loc": 0x70, "attr": "hwmon"},
                {"id": "xdpe12284_5", "name": "wb_xdpe12284", "bus": 94, "loc": 0x70, "attr": "hwmon"},
                {"id": "xdpe12284_6", "name": "wb_xdpe12284", "bus": 95, "loc": 0x70, "attr": "hwmon"},
                {"id": "xdpe12284_7", "name": "wb_xdpe12284", "bus": 96, "loc": 0x70, "attr": "hwmon"},
                {"id": "xdpe12284_8", "name": "wb_xdpe12284", "bus": 97, "loc": 0x70, "attr": "hwmon"},
                {"id": "xdpe12284_9", "name": "wb_xdpe12284", "bus": 69, "loc": 0x70, "attr": "hwmon"},
                {"id": "xdpe12284_10", "name": "wb_xdpe12284", "bus": 69, "loc": 0x6e, "attr": "hwmon"},
                {"id": "xdpe12284_11", "name": "wb_xdpe12284", "bus": 69, "loc": 0x5e, "attr": "hwmon"},
                {"id": "xdpe12284_12", "name": "wb_xdpe12284", "bus": 69, "loc": 0x68, "attr": "hwmon"},
            ],
        },
    ],
}

MANUINFO_CONF = {
    "version2": 1,

    "BIOS" : {
        "source" : "bios",
    },

    "ONIE" : {
        "source" : "onie",
        "print" : ["Build Date","Version"]
    },

    "CPU" : {
        "source" : "processor",
    },

    "SSD" : {
        "source" : "ssd",
    },

    "CPLD": {
        "class" : 1,
        "source" : "s3ip",
        "type" : "cpld",
    },

    "PSU": {
        "class" : 1,
        "source" : "s3ip",
        "type" : "psu",
    },

    "FAN": {
        "class" : 1,
        "source" : "s3ip",
        "type" : "fan",
    },

    "NIC" : {
        "Device Model" : {"gettype": "direct_config", "value": "NA"},
        "Vendor" : {"gettype": "direct_config", "value": "INTEL"},
        "Firmware Version" : {"cmd": "ethtool -i eth0 |grep firmware-version | awk -F': ' '{print $2}'", "gettype": "cmd"},
    },

    "FPGA": {
        "class" : 1,
        "source" : "s3ip",
        "type" : "fpga",
    },

    "OTHERS" : {
        "child" : {
            "CPU-BMC-SWITCH" : {
                "Device Model" : {"gettype": "direct_config", "value": "BCM53134O"},
                "Vendor" : {"gettype": "direct_config", "value": "Broadcom"},
                "Hardware Version" : {
                    "gettype": "func", 
                    "funcname": "get_and_generate_mgmt_version",
                    "params": {
                        "before": [
                            # OE high
                            {"gettype": "cmd", "cmd": "echo 10051 > /sys/class/gpio/export"},
                            {"gettype": "cmd", "cmd": "echo high > /sys/class/gpio/gpio10051/direction"},
                            # SEL1 high
                            {"gettype": "cmd", "cmd": "echo 10052 > /sys/class/gpio/export"},
                            {"gettype": "cmd", "cmd": "echo high > /sys/class/gpio/gpio10052/direction"},
                            #enable 53134 update
                            {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x3d, "value": 0x00},
                            {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x45, "value": 0x01},
                            {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x46, "value": 0x06},
                            {"gettype": "cmd", "cmd": "modprobe wb_spi_gpio"},
                            {"gettype": "cmd", "cmd": "modprobe wb_spi_gpio_device sck=55  mosi=54 miso=52 cs=53 bus=0 gpio_chip_name=INTC3001:00"},
                            {"gettype": "cmd", "cmd": "modprobe wb_spi_93xx46"},
                        ],
                        "get_version": "md5sum /sys/bus/spi/devices/spi0.0/eeprom | awk '{print $1}'",
                        "after": [
                            {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio10052/value"},
                            {"gettype": "cmd", "cmd": "echo 10052 > /sys/class/gpio/unexport"},
                            {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio10051/value"},
                            {"gettype": "cmd", "cmd": "echo 10051 > /sys/class/gpio/unexport"},
                        ],
                        "finally": [
                            {"gettype": "cmd", "cmd": "rmmod wb_spi_93xx46"},
                            {"gettype": "cmd", "cmd": "rmmod wb_spi_gpio_device"},
                            {"gettype": "cmd", "cmd": "rmmod wb_spi_gpio"},
                            {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x46, "value": 0x00},
                            {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x45, "value": 0x00},
                            {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x3d, "value": 0x01},
                        ],
                    },
                },
            },
        },
    },
}

PMON_SYSLOG_STATUS = {
    "polling_time": 3,
    "sffs": {
        "present": {"path": ["/sys/s3ip/transceiver/*/present"], "ABSENT": 0},
        "nochangedmsgflag": 0,
        "nochangedmsgtime": 60,
        "noprintfirsttimeflag": 1,
        "alias": {
            "sff1": "Ethernet1",
            "sff2": "Ethernet2",
            "sff3": "Ethernet3",
            "sff4": "Ethernet4",
            "sff5": "Ethernet5",
            "sff6": "Ethernet6",
            "sff7": "Ethernet7",
            "sff8": "Ethernet8",
            "sff9": "Ethernet9",
            "sff10": "Ethernet10",
            "sff11": "Ethernet11",
            "sff12": "Ethernet12",
            "sff13": "Ethernet13",
            "sff14": "Ethernet14",
            "sff15": "Ethernet15",
            "sff16": "Ethernet16",
            "sff17": "Ethernet17",
            "sff18": "Ethernet18",
            "sff19": "Ethernet19",
            "sff20": "Ethernet20",
            "sff21": "Ethernet21",
            "sff22": "Ethernet22",
            "sff23": "Ethernet23",
            "sff24": "Ethernet24",
            "sff25": "Ethernet25",
            "sff26": "Ethernet26",
            "sff27": "Ethernet27",
            "sff28": "Ethernet28",
            "sff29": "Ethernet29",
            "sff30": "Ethernet30",
            "sff31": "Ethernet31",
            "sff32": "Ethernet32",
            "sff33": "Ethernet33",
            "sff34": "Ethernet34",
            "sff35": "Ethernet35",
            "sff36": "Ethernet36",
            "sff37": "Ethernet37",
            "sff38": "Ethernet38",
            "sff39": "Ethernet39",
            "sff40": "Ethernet40",
            "sff41": "Ethernet41",
            "sff42": "Ethernet42",
            "sff43": "Ethernet43",
            "sff44": "Ethernet44",
            "sff45": "Ethernet45",
            "sff46": "Ethernet46",
            "sff47": "Ethernet47",
            "sff48": "Ethernet48",
            "sff49": "Ethernet49",
            "sff50": "Ethernet50",
            "sff51": "Ethernet51",
            "sff52": "Ethernet52",
            "sff53": "Ethernet53",
            "sff54": "Ethernet54",
            "sff55": "Ethernet55",
            "sff56": "Ethernet56",
            "sff57": "Ethernet57",
            "sff58": "Ethernet58",
            "sff59": "Ethernet59",
            "sff60": "Ethernet60",
            "sff61": "Ethernet61",
            "sff62": "Ethernet62",
            "sff63": "Ethernet63",
            "sff64": "Ethernet64",
            "sff65": "Ethernet65",
            "sff66": "Ethernet66",
        }
    },
    "fans": {
        "present": {"path": ["/sys/s3ip/fan/*/present"], "ABSENT": 0},
        "status": [
            {"path": "/sys/s3ip/fan/%s/status", 'okval': 1},
        ],
        "nochangedmsgflag": 1,
        "nochangedmsgtime": 60,
        "noprintfirsttimeflag": 0,
        "alias": {
            "fan1": "FAN1",
            "fan2": "FAN2",
            "fan3": "FAN3",
            "fan4": "FAN4"
        }
    },
    "psus": {
        "present": {"path": ["/sys/s3ip/psu/*/present"], "ABSENT": 0},
        "status": [
            {"path": "/sys/s3ip/psu/%s/out_status", "okval":1},
        ],
        "nochangedmsgflag": 1,
        "nochangedmsgtime": 60,
        "noprintfirsttimeflag": 0,
        "alias": {
            "psu1": "PSU1",
            "psu2": "PSU2"
        }
    }
}

REBOOT_CTRL_PARAM = {
    "cpu": {"path":"/dev/cpld1", "offset":0x17, "rst_val":0xfd, "rst_delay":0, "gettype":"devfile"},
    "mac": [
        {"gettype": "cmd", "cmd": "setpci -s 14:02.0 0x50.W=0x0050", "rst_delay":0.1},
        {"path":"/dev/cpld6", "offset":0x16, "rst_val":0x00, "rst_delay":1, "gettype": "devfile"},
        {"path":"/dev/cpld6", "offset":0x16, "rst_val":0x01, "rst_delay":0, "gettype": "devfile"},
        {"gettype": "cmd", "cmd": "setpci -s 14:02.0 0x50.W=0x0060", "rst_delay":0.1},
    ],
    "phy": {"path":"/dev/cpld1", "offset":0x18, "rst_val":0x1e, "rst_delay":0, "gettype":"devfile"},
    "power": [
        {"bus": 42, "loc": 0x58, "offset": 0x02, "rst_val": 0x48, "rst_delay":0.1, "gettype": "i2c"},
        {"bus": 42, "loc": 0x58, "offset": 0x01, "rst_val": 0x40, "rst_delay":0.1, "gettype": "i2c"},
        {"bus": 43, "loc": 0x58, "offset": 0x02, "rst_val": 0x48, "rst_delay":0.1, "gettype": "i2c"},
        {"bus": 43, "loc": 0x58, "offset": 0x01, "rst_val": 0x40, "rst_delay":0.1, "gettype": "i2c"},
    ],

}

REBOOT_CAUSE_PARA = {
    "reboot_cause_list": [
        {
            "name": "cold_reboot",
            "monitor_point": {"gettype":"devfile", "path":"/dev/cpld1", "offset":0x1d, "read_len":1, "okval":0x09},
            "record": [
                {"record_type": "file", "mode": "cover", "log": "Power Loss, ", "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
                {"record_type": "file", "mode": "add", "log": "Power Loss, ", "path": "/etc/sonic/.reboot/.history-reboot-cause.txt"}
            ]
        },
        {
            "name": "wdt_reboot",
            "monitor_point": {"gettype":"devfile", "path":"/dev/cpld1", "offset":0x1d, "read_len":1, "okval":0x05},
            "record": [
                {"record_type": "file", "mode": "cover", "log": "Watchdog, ", "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
                {"record_type": "file", "mode": "add", "log": "Watchdog, ", "path": "/etc/sonic/.reboot/.history-reboot-cause.txt"}
            ],
        },
        {
            "name": "bmc_reboot",
            "monitor_point": {"gettype":"devfile", "path":"/dev/cpld1", "offset":0x1d, "read_len":1, "okval":0x06},
            "record": [
                {"record_type": "file", "mode": "cover", "log": "BMC reboot, ", "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
                {"record_type": "file", "mode": "add", "log": "BMC reboot, ", "path": "/etc/sonic/.reboot/.history-reboot-cause.txt"}
            ],
        },
        {
            "name": "cpu_reboot",
            "monitor_point": {"gettype":"devfile", "path":"/dev/cpld1", "offset":0x1d, "read_len":1, "okval":[0x03, 0x04]},
            "record": [
                {"record_type":"file", "mode":"cover", "log":"CPU reboot, ", "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
                {"record_type": "file", "mode": "add", "log": "CPU reboot, ", "path": "/etc/sonic/.reboot/.history-reboot-cause.txt"}
            ],
        },
        {
            "name": "bmc_powerdown",
            "monitor_point": {"gettype":"devfile", "path":"/dev/cpld1", "offset":0x1d, "read_len":1, "okval":[0x02, 0x07, 0x0a]},
            "record": [
                {"record_type": "file", "mode": "cover", "log": "BMC powerdown, ", "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
                {"record_type": "file", "mode": "add", "log": "BMC powerdown, ", "path": "/etc/sonic/.reboot/.history-reboot-cause.txt"}
            ],
        },
        {
            "name": "otp_switch_reboot",
            "monitor_point": {"gettype": "file_exist", "judge_file": "/etc/.otp_switch_reboot_flag", "okval": True},
            "record": [
                {"record_type": "file", "mode": "cover", "log": "Thermal Overload: ASIC, ", "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
                {"record_type": "file", "mode": "add", "log": "Thermal Overload: ASIC, ", "path": "/etc/sonic/.reboot/.history-reboot-cause.txt"}
            ],
            "finish_operation": [
                {"gettype": "cmd", "cmd": "rm -rf /etc/.otp_switch_reboot_flag"},
            ]
        },
        {
            "name": "otp_other_reboot",
            "monitor_point": {"gettype": "file_exist", "judge_file": "/etc/.otp_other_reboot_flag", "okval": True},
            "record": [
                {"record_type": "file", "mode": "cover", "log": "Thermal Overload: Other, ", "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
                {"record_type": "file", "mode": "add", "log": "Thermal Overload: Other, ", "path": "/etc/sonic/.reboot/.history-reboot-cause.txt"}
            ],
            "finish_operation": [
                {"gettype": "cmd", "cmd": "rm -rf /etc/.otp_other_reboot_flag"},
            ]
        },
    ],
    "other_reboot_cause_record": [
        {"record_type": "file", "mode": "cover", "log": "Other, ", "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
        {"record_type": "file", "mode": "add", "log": "Other, ", "path": "/etc/sonic/.reboot/.history-reboot-cause.txt"}
    ],
}

##################### MAC Voltage adjust####################################
MAC_DEFAULT_PARAM = [
    {
        "name": "mac_core",              # AVS name
        "type": 0,                       # 1: used default value, if rov value not in range. 0: do nothing, if rov value not in range
        "default": 0x82,                 # default value, if rov value not in range
        "rov_source": 0,                 # 0: get rov value from cpld, 1: get rov value from SDK
        "cpld_avs": {"path": "/dev/cpld6", "offset": 0x30, "read_len": 1, "gettype": "devfile"},
        "set_avs": {
            "loc": "/sys/bus/i2c/devices/84-0040/avs0_vout_command", "gettype": "sysfs", "formula": None},
        "mac_avs_param": {
            0x92: 0xBF4,
            0x90: 0xC29,
            0x8e: 0xC56,
            0x8c: 0xC8B,
            0x8a: 0xCBD,
            0x88: 0xCEA,
            0x86: 0xD14,
            0x84: 0xD44,
            0x82: 0xD71
        }
    }
]

DRIVERLISTS = [
    {"name": "wb_i2c_i801", "delay": 1},
    {"name": "i2c_dev", "delay": 0},
    {"name": "wb_i2c_algo_bit", "delay": 0},
    {"name": "wb_i2c_gpio", "delay": 0},
    {"name": "i2c_mux", "delay": 0},
    {"name": "wb_i2c_gpio_device gpio_sda=181 gpio_scl=180 gpio_chip_name=INTC3001:00 bus_num=1", "delay": 0},
    {"name": "platform_common dfd_my_type=0x40e9", "delay": 0},
    {"name": "wb_logic_dev_common", "delay":0},
    {"name": "wb_fpga_pcie", "delay": 0},
    {"name": "wb_pcie_dev", "delay": 0},
    {"name": "wb_pcie_dev_device", "delay": 0},
    {"name": "wb_io_dev", "delay": 0},
    {"name": "wb_io_dev_device", "delay": 0},
    {"name": "wb_indirect_dev", "delay": 0},
    {"name": "wb_indirect_dev_device", "delay": 0},
    {"name": "wb_i2c_dev", "delay": 0},
    {"name": "wb_spi_dev", "delay": 0},
    {"name": "wb_fpga_i2c_bus_drv", "delay": 0},
    {"name": "wb_fpga_i2c_bus_device", "delay": 0},
    {"name": "wb_i2c_mux_pca9641", "delay": 0},
    {"name": "wb_i2c_mux_pca954x", "delay": 0},
    {"name": "wb_i2c_mux_pca954x_device", "delay": 0},
    {"name": "wb_fpga_pca954x_drv", "delay": 0},
    {"name": "wb_fpga_pca954x_device", "delay": 0},
    {"name": "wb_i2c_dev_device", "delay": 0},
    {"name": "mdio_bitbang", "delay": 0},
    {"name": "mdio_gpio", "delay": 0},
    {"name": "wb_mdio_gpio_device gpio_mdc=69 gpio_mdio=70 gpio_chip_name=INTC3001:00", "delay": 0},
    {"name": "wb_wdt", "delay": 0},
    {"name": "wb_wdt_device", "delay": 0},
    {"name": "wb_eeprom_93xx46", "delay": 0},
    {"name": "wb_lm75", "delay": 0},
    {"name": "wb_tmp401", "delay": 0},
    {"name": "ct7148", "delay": 0},
    {"name": "wb_rc32312", "delay": 0},
    {"name": "wb_optoe", "delay": 0},
    {"name": "wb_at24", "delay": 0},
    {"name": "wb_pmbus_core", "delay": 0},
    {"name": "wb_csu550", "delay": 0},
    {"name": "wb_ina3221", "delay": 0},
    {"name": "wb_tps53622", "delay": 0},
    {"name": "wb_ucd9000", "delay": 0},
#    {"name": "wb_ucd9081", "delay": 0}, 
    {"name": "wb_xdpe12284", "delay": 0},
    {"name": "wb_xdpe132g5c_pmbus", "delay":0},
    {"name": "wb_xdpe132g5c", "delay": 0},
    {"name": "firmware_driver_cpld", "delay": 0},
    {"name": "firmware_driver_ispvme", "delay": 0},
    {"name": "firmware_driver_sysfs", "delay": 0},
    {"name": "wb_firmware_upgrade_device", "delay": 0},
    {"name": "hw_test", "delay": 0},

    {"name": "s3ip_sysfs", "delay": 0},
    {"name": "wb_switch_driver", "delay": 0},
    {"name": "syseeprom_device_driver", "delay": 0},
    {"name": "fan_device_driver", "delay": 0},
    {"name": "cpld_device_driver", "delay": 0},
    {"name": "sysled_device_driver", "delay": 0},
    {"name": "psu_device_driver", "delay": 0},
    {"name": "transceiver_device_driver", "delay": 0},
    {"name": "temp_sensor_device_driver", "delay": 0},
    {"name": "vol_sensor_device_driver", "delay": 0},
    {"name": "curr_sensor_device_driver", "delay": 0},
    {"name": "fpga_device_driver", "delay": 0},
    {"name": "watchdog_device_driver", "delay": 0},
    {"name": "wb_spd", "delay": 0},
]

DEVICE = [
    {"name": "wb_24c02", "bus": 1, "loc": 0x56},
    {"name": "wb_24c02", "bus": 57, "loc": 0x57},
    {"name": "wb_24c02", "bus": 61, "loc": 0x57},
    {"name": "wb_24c02", "bus": 66, "loc": 0x57},
    # fan
    {"name": "wb_24c64", "bus": 52, "loc": 0x50},
    {"name": "wb_24c64", "bus": 53, "loc": 0x50},
    {"name": "wb_24c64", "bus": 54, "loc": 0x50},
    {"name": "wb_24c64", "bus": 55, "loc": 0x50},
    # psu
    {"name": "wb_24c02", "bus": 42, "loc": 0x50},
    {"name": "wb_fsp1200", "bus": 42, "loc": 0x58},
    {"name": "wb_24c02", "bus": 43, "loc": 0x50},
    {"name": "wb_fsp1200", "bus": 43, "loc": 0x58},
    # temp
    {"name": "wb_lm75", "bus": 51, "loc": 0x4b},
    {"name": "wb_lm75", "bus": 56, "loc": 0x4e},
    {"name": "wb_lm75", "bus": 58, "loc": 0x4b},
    {"name": "wb_lm75", "bus": 75, "loc": 0x4b},
    {"name": "wb_lm75", "bus": 76, "loc": 0x4f},
    {"name": "ct7318", "bus": 77, "loc": 0x4c},
    {"name": "ct7318", "bus": 78, "loc": 0x4c},
    #dcdc
    {"name": "wb_ucd90160", "bus": 68, "loc": 0x5b},
    {"name": "wb_ucd90160", "bus": 69, "loc": 0x5f},
    {"name": "wb_xdpe12284", "bus": 69, "loc": 0x70},
    {"name": "wb_xdpe12284", "bus": 69, "loc": 0x6e},
    {"name": "wb_xdpe12284", "bus": 69, "loc": 0x5e},
    {"name": "wb_xdpe12284", "bus": 69, "loc": 0x68},
#    {"name": "wb_ucd9081", "bus": 72, "loc": 0x68},
    {"name": "wb_ucd90160", "bus": 82, "loc": 0x5b},
    {"name": "wb_ucd90160", "bus": 83, "loc": 0x5b},
    {"name": "wb_xdpe12284", "bus": 90, "loc": 0x70},
    {"name": "wb_xdpe12284", "bus": 91, "loc": 0x70},
    {"name": "wb_xdpe12284", "bus": 92, "loc": 0x70},
    {"name": "wb_xdpe12284", "bus": 93, "loc": 0x70},
    {"name": "wb_xdpe12284", "bus": 94, "loc": 0x70},
    {"name": "wb_xdpe12284", "bus": 95, "loc": 0x70},
    {"name": "wb_xdpe12284", "bus": 96, "loc": 0x70},
    {"name": "wb_xdpe12284", "bus": 97, "loc": 0x70},
    #avs
    {"name": "wb_xdpe132g5c_pmbus", "bus": 84, "loc": 0x40},
    {"name": "wb_xdpe132g5c", "bus": 84, "loc": 0x10},
    {"name": "wb_xdpe132g5c_pmbus", "bus": 85, "loc": 0x4d},
    {"name": "wb_xdpe132g5c", "bus": 85, "loc": 0x1d},
    {"name": "wb_xdpe132g5c_pmbus", "bus": 86, "loc": 0x4d},
    {"name": "wb_xdpe132g5c", "bus": 86, "loc": 0x1d},
    {"name": "wb_rc32312", "bus":102, "loc": 0x09},
]

OPTOE = [
    {"name": "wb_optoe2", "startbus": 59, "endbus": 60},
    {"name": "wb_optoe3", "startbus": 106, "endbus": 169},
]


INIT_PARAM = []

INIT_COMMAND_PRE = []

INIT_COMMAND = [
    # open X86 BMC Serial port
    "dfd_debug sysfs_data_wr /dev/cpld1 0x41 0x01",
    # enable stream light
    "dfd_debug sysfs_data_wr /dev/cpld6 0xef 0x01",
    "dfd_debug sysfs_data_wr /dev/cpld7 0xef 0x01",
    "dfd_debug sysfs_data_wr /dev/cpld8 0x80 0xff",
    "dfd_debug sysfs_data_wr /dev/cpld8 0x81 0xff",
    # KR power_on
    "dfd_debug sysfs_data_wr /dev/cpld9 0x80 0x03",
    # KR tx-disable enable
    "dfd_debug sysfs_data_wr /dev/cpld9 0x58 0x00",
    ]

UPGRADE_SUMMARY = {
    "devtype": 0x40e9,

    "slot0": {
        "subtype": 0,
        "VME": {
            "chain1": {
                "name": "BASE_CPLD",
                "is_support_warm_upg": 1,
            },
            "chain2": {
                "name": "MAC_CPLDAB",
                "is_support_warm_upg": 1,
            },
            "chain3": {
                "name": "MAC_CPLDC",
                "is_support_warm_upg": 1,
            },
            "chain4": {
                "name": "FAN_CPLD",
                "is_support_warm_upg": 1,
            },
            "chain5": {
                "name": "MGMT_CPLD",
                "is_support_warm_upg": 1,
            },
            "chain6": {
                "name": "CPU_CPLD",
                "is_support_warm_upg": 1,
            },
        },

        "SPI-LOGIC-DEV": {
            "chain1": {
                "name": "MAC_FPGA",
                "is_support_warm_upg": 1,
            }
        },

        "SYSFS": {
            "chain2": {
                "name": "BCM53134",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x3d, "value": 0x00, "delay": 0.1},
                    {"cmd": "modprobe wb_spi_gpio", "gettype": "cmd"},
                    {"cmd": "modprobe wb_spi_gpio_device sck=55  mosi=54 miso=52 cs=53 bus=0 gpio_chip_name=INTC3001:00", "gettype": "cmd"},
                    {"cmd": "modprobe wb_spi_93xx46", "gettype": "cmd", "delay": 0.1},
                ],
                "finish_cmd": [
                    {"cmd": "rmmod wb_spi_93xx46", "gettype": "cmd"},
                    {"cmd": "rmmod wb_spi_gpio_device", "gettype": "cmd"},
                    {"cmd": "rmmod wb_spi_gpio", "gettype": "cmd", "delay": 0.1},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x3d, "value": 0x01, "delay": 0.1},
                ],
            },
        },

        "MTD": {
            "chain4": {
                "name": "BIOS",
                "filesizecheck": 20480,  # bios check file size, Unit: K
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"cmd": "modprobe mtd", "gettype": "cmd"},
                    {"cmd": "modprobe spi_nor", "gettype": "cmd"},
                    {"cmd": "modprobe ofpart", "gettype": "cmd"},
                    {"cmd": "modprobe intel_spi writeable=1", "gettype": "cmd"},
                    {"cmd": "modprobe intel_spi_pci", "gettype": "cmd"},
                ],
                "finish_cmd": [
                    {"cmd": "rmmod intel_spi_pci", "gettype": "cmd"},
                    {"cmd": "rmmod intel_spi", "gettype": "cmd"},
                    {"cmd": "rmmod ofpart", "gettype": "cmd"},
                    {"cmd": "rmmod spi_nor", "gettype": "cmd"},
                    {"cmd": "rmmod mtd", "gettype": "cmd"},
                ],
            },
        },

        "TEST": {
            "fpga": [
                {"chain": 1, "file": "/etc/.upgrade_test/fpga_test_header.bin", "display_name": "MAC_FPGA"},
            ],
            "cpld": [
                {"chain": 1, "file": "/etc/.upgrade_test/cpld_test_0_1_header.vme", "display_name": "BASE_CPLD"},
                {"chain": 2, "file": "/etc/.upgrade_test/cpld_test_0_2_header.vme", "display_name": "MAC_CPLDAB"},
                {"chain": 3, "file": "/etc/.upgrade_test/cpld_test_0_3_header.vme", "display_name": "MAC_CPLDC"},
                {"chain": 4, "file": "/etc/.upgrade_test/cpld_test_0_4_header.vme", "display_name": "FAN_CPLD"},
                {"chain": 5, "file": "/etc/.upgrade_test/cpld_test_0_5_header.vme", "display_name": "MGMT_CPLD"},
                {"chain": 6, "file": "/etc/.upgrade_test/cpld_test_0_6_header.vme", "display_name": "CPU_CPLD"},
            ],
        },
    },
}

PLATFORM_E2_CONF = {
    "fan": [
        {"name": "fan1", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/52-0050/eeprom"},
        {"name": "fan2", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/53-0050/eeprom"},
        {"name": "fan3", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/54-0050/eeprom"},
        {"name": "fan4", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/55-0050/eeprom"},
    ],
    "psu": [
        {"name": "psu1", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/42-0050/eeprom"},
        {"name": "psu2", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/43-0050/eeprom"},
    ],
    "syseeprom": [
        {"name": "syseeprom", "e2_type": "onie_tlv", "e2_path": "/sys/bus/i2c/devices/1-0056/eeprom"},
    ],
}

AIR_FLOW_CONF = {
    "psu_fan_airflow": {
        "intake": ['PA3000I-F', 'CRPS3000CL', 'ECDL3000123'],
        "exhaust": ['PA3000I-R', 'CRPS3000CLR']
    },

    "fanairflow": {
        "intake": ['FAN80-02-F'],
        "exhaust": ['FAN80-02-R']
    },

    "fans": [
        {
            "name": "FAN1",
            "e2_type": "fru",
            "e2_path": "/sys/bus/i2c/devices/52-0050/eeprom",
            "area": "productInfoArea",
            "field": "productName",
            "decode": "fanairflow"
        },
        {
            "name": "FAN2",
            "e2_type": "fru",
            "e2_path": "/sys/bus/i2c/devices/53-0050/eeprom",
            "area": "productInfoArea",
            "field": "productName",
            "decode": "fanairflow"
        },
        {
            "name": "FAN3",
            "e2_type": "fru",
            "e2_path": "/sys/bus/i2c/devices/54-0050/eeprom",
            "area": "productInfoArea",
            "field": "productName",
            "decode": "fanairflow"
        },
        {
            "name": "FAN4",
            "e2_type": "fru",
            "e2_path": "/sys/bus/i2c/devices/55-0050/eeprom",
            "area": "productInfoArea",
            "field": "productName",
            "decode": "fanairflow"
        }
    ],

    "psus": [
        {
            "name": "PSU1",
            "e2_type": "fru",
            "e2_path": "/sys/bus/i2c/devices/42-0050/eeprom",
            "area": "productInfoArea",
            "field": "productPartModelName",
            "decode": "psu_fan_airflow"
        },
        {
            "name": "PSU2",
            "e2_type": "fru",
            "e2_path": "/sys/bus/i2c/devices/43-0050/eeprom",
            "area": "productInfoArea",
            "field": "productPartModelName",
            "decode": "psu_fan_airflow"
        }
    ]
}
