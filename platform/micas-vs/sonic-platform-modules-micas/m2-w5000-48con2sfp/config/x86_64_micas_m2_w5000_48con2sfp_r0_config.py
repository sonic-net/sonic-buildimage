#!/usr/bin/python
# -*- coding: UTF-8 -*-
from platform_common import *

STARTMODULE = {
    "hal_fanctrl": 1,
    "hal_ledctrl": 0,
    "avscontrol": 0,
    "dev_monitor": 1,
    "reboot_cause": 0,
    "pmon_syslog": 0,
    "sff_temp_polling": 0,
    "generate_airflow": 0,
    "set_eth_mac": 0,
    "drv_update": 0,
}

MANUINFO_CONF = {
    "bios": {
        "key": "BIOS",
        "head": True,
        "next": "onie"
    },
    "bios_vendor": {
        "parent": "bios",
        "key": "Vendor",
        "cmd": "dmidecode -t 0 |grep Vendor",
        "pattern": r".*Vendor",
        "separator": ":",
        "arrt_index": 1,
    },
    "bios_version": {
        "parent": "bios",
        "key": "Version",
        "cmd": "dmidecode -t 0 |grep Version",
        "pattern": r".*Version",
        "separator": ":",
        "arrt_index": 2,
    },
    "bios_date": {
        "parent": "bios",
        "key": "Release Date",
        "cmd": "dmidecode -t 0 |grep Release",
        "pattern": r".*Release Date",
        "separator": ":",
        "arrt_index": 3,
    },
    "onie": {
        "key": "ONIE",
        "next": "cpu"
    },
    "onie_date": {
        "parent": "onie",
        "key": "Build Date",
        "file": "/host/machine.conf",
        "pattern": r"^onie_build_date",
        "separator": "=",
        "arrt_index": 1,
    },
    "onie_version": {
        "parent": "onie",
        "key": "Version",
        "file": "/host/machine.conf",
        "pattern": r"^onie_version",
        "separator": "=",
        "arrt_index": 2,
    },

    "cpu": {
        "key": "CPU",
        "next": "cpld"
    },
    "cpu_vendor": {
        "parent": "cpu",
        "key": "Vendor",
        "cmd": "dmidecode --type processor |grep Manufacturer",
        "pattern": r".*Manufacturer",
        "separator": ":",
        "arrt_index": 1,
    },
    "cpu_model": {
        "parent": "cpu",
        "key": "Device Model",
        "cmd": "dmidecode --type processor | grep Version",
        "pattern": r".*Version",
        "separator": ":",
        "arrt_index": 2,
    },
    "cpu_core": {
        "parent": "cpu",
        "key": "Core Count",
        "cmd": "dmidecode --type processor | grep \"Core Count\"",
        "pattern": r".*Core Count",
        "separator": ":",
        "arrt_index": 3,
    },
    "cpu_thread": {
        "parent": "cpu",
        "key": "Thread Count",
        "cmd": "dmidecode --type processor | grep \"Thread Count\"",
        "pattern": r".*Thread Count",
        "separator": ":",
        "arrt_index": 4,
    },
    "cpld": {
        "key": "CPLD",
        "next": "fpga"
    },

    "cpld1": {
        "key": "CPLD1",
        "parent": "cpld",
        "arrt_index": 1,
    },
    "cpld1_model": {
        "key": "Device Model",
        "parent": "cpld1",
        "config": "LCMXO3LF-2100C-5BG256C",
        "arrt_index": 1,
    },
    "cpld1_vender": {
        "key": "Vendor",
        "parent": "cpld1",
        "config": "LATTICE",
        "arrt_index": 2,
    },
    "cpld1_desc": {
        "key": "Description",
        "parent": "cpld1",
        "config": "CPU_CPLD",
        "arrt_index": 3,
    },
    "cpld1_version": {
        "key": "Firmware Version",
        "parent": "cpld1",
        "devfile": {
            "loc": "/dev/cpld0",
            "offset":0,
            "len":4,
            "bit_width":1
        },
        "arrt_index": 4,
    },

    "cpld2": {
        "key": "CPLD2",
        "parent": "cpld",
        "arrt_index": 2,
    },
    "cpld2_model": {
        "key": "Device Model",
        "parent": "cpld2",
        "config": "LCMXO3LF-6900C-5BG256C",
        "arrt_index": 1,
    },
    "cpld2_vender": {
        "key": "Vendor",
        "parent": "cpld2",
        "config": "LATTICE",
        "arrt_index": 2,
    },
    "cpld2_desc": {
        "key": "Description",
        "parent": "cpld2",
        "config": "CTRL_CPLD",
        "arrt_index": 3,
    },
    "cpld2_version": {
        "key": "Firmware Version",
        "parent": "cpld2",
        "devfile": {
            "loc": "/dev/cpld1",
            "offset":0,
            "len":4,
            "bit_width":1
        },
        "arrt_index": 4,
    },

    "cpld3": {
        "key": "CPLD3",
        "parent": "cpld",
        "arrt_index": 3,
    },
    "cpld3_model": {
        "key": "Device Model",
        "parent": "cpld3",
        "config": "LCMXO3LF-2100C-5BG256C",
        "arrt_index": 1,
    },
    "cpld3_vender": {
        "key": "Vendor",
        "parent": "cpld3",
        "config": "LATTICE",
        "arrt_index": 2,
    },
    "cpld3_desc": {
        "key": "Description",
        "parent": "cpld3",
        "config": "PORT_CPLD",
        "arrt_index": 3,
    },
    "cpld3_version": {
        "key": "Firmware Version",
        "parent": "cpld3",
        "cmd": "cat /sys/bus/i2c/devices/5-0030/cpld_version | sed 's/ //g'",
        "arrt_index": 4,
    },

    "fpga": {
        "key": "FPGA",
    },

    "fpga1": {
        "key": "FPGA1",
        "parent": "fpga",
        "arrt_index": 1,
    },
    "fpga1_model": {
        "parent": "fpga1",
        "config": "XC7A50T-2FGG484I",
        "key": "Device Model",
        "arrt_index": 1,
    },
    "fpga1_vender": {
        "parent": "fpga1",
        "config": "XILINX",
        "key": "Vendor",
        "arrt_index": 2,
    },
    "fpga1_desc": {
        "key": "Description",
        "parent": "fpga1",
        "config": "NA",
        "arrt_index": 3,
    },
    "fpga1_hw_version": {
        "parent": "fpga1",
        "config": "NA",
        "key": "Hardware Version",
        "arrt_index": 4,
    },
    "fpga1_fw_version": {
        "parent": "fpga1",
        "devfile": {
            "loc": "/dev/fpga0",
            "offset": 0,
            "len": 4,
            "bit_width": 4
        },
        "key": "Firmware Version",
        "arrt_index": 5,
    },
    "fpga1_date": {
        "parent": "fpga1",
        "devfile": {
            "loc": "/dev/fpga0",
            "offset":4,
            "len":4,
            "bit_width":4
        },
        "key": "Build Date",
        "arrt_index": 6,
    },

}

# drivers list
DRIVERLISTS = [
        {"name":"r8169", "delay":0, "reload": 0},
        {"name":"igb", "delay":0, "reload": 0},
        {"name":"i2c_dev", "delay":0},
        {"name":"i2c_algo_bit","delay":0},
        {"name":"i2c_mux", "delay":0},
        {"name":"platform_common dfd_my_type=0x4141", "delay": 0},
        {"name":"fpga_i2c_ocores", "delay":0},
        {"name":"fpga_pcie_uart", "delay":0},
        {"name":"fpga_uart_ocores", "delay":0},
        {"name":"wb_i2c_mux_pca954x", "delay": 0},
        {"name":"wb_i2c_mux_pca954x_device", "delay": 0},
        {"name":"wb_pcie_dev", "delay": 0},
        {"name":"wb_pcie_dev_device", "delay": 0},
        {"name":"wb_i2c_dev", "delay": 0},
        {"name":"wb_i2c_dev_device", "delay": 0},
        {"name":"i2c_gpio", "delay":0},
        {"name":"at24", "delay":0},
        {"name": "pmbus_core", "delay": 0},
        {"name": "isl68137", "delay": 0},
        {"name":"lm75", "delay": 0},
        {"name":"ucd9000", "delay": 0},
        {"name":"ina3221", "delay": 0},
        {"name":"mc_cpld", "delay":0},
        {"name":"wb_amd_xgbe", "delay":0},
        {"name":"optoe", "delay":0},
        # {"name":"wb_isl68137", "delay":0},
        # {"name": "firmware_driver_cpld", "delay": 0},
        # {"name": "firmware_driver_ispvme", "delay": 0},
        # {"name": "firmware_driver_sysfs", "delay": 0},
        # {"name": "wb_firmware_upgrade_device", "delay": 0},
        {"name": "wb_i2c_dev_device", "delay": 0},
]

DEVICE = [
        {"name":"24c02","bus":3,"loc":0x56 },
        {"name":"24c02","bus":3,"loc":0x57 },
        {"name":"24c02","bus":3,"loc":0x50 },
        {"name":"mc_cpld","bus":5,"loc":0x30 },

        {"name": "tmp1075", "bus": 0, "loc": 0x4a},
        {"name": "lm75", "bus": 7, "loc": 0x48},
        {"name": "lm75", "bus": 8, "loc": 0x49},        

        {"name": "ucd90160", "bus": 0, "loc": 0x5f},

        {"name": "raa228228", "bus": 0, "loc": 0x72},

        {"name": "ina3221", "bus": 9, "loc": 0x40},
        {"name": "ina3221", "bus": 10, "loc": 0x41},
]

INIT_PARAM = [
    {"loc": "5-0030/sfp_txdis0", "value": "0xde"},
    {"loc": "5-0030/sfp_txdis1", "value": "0xde"},  
]

INIT_COMMAND = [
    "balance_affinity.sh",
    "dialout_group.sh",
    "console_init_cfg.py",
]

REBOOT_CAUSE_PARA = {
    "reboot_cause_list": [
        {
            "name": "cold_reboot",
            "monitor_point": {"gettype":"devfile", "path":"/dev/cpld1", "offset":0x38, "read_len":1, "okval":0x01},
            "record": [
                {"record_type": "file", "mode": "cover", "log": "Power Loss, ", "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
                {"record_type": "file", "mode": "add", "log": "Power Loss, ", "path": "/etc/sonic/.reboot/.history-reboot-cause.txt"}
            ]
        },
        {
            "name": "wdt_reboot",
            "monitor_point": {"gettype":"devfile", "path":"/dev/cpld1", "offset":0x38, "read_len":1, "okval":0x06},
            "record": [
                {"record_type": "file", "mode": "cover", "log": "Watchdog, ", "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
                {"record_type": "file", "mode": "add", "log": "Watchdog, ", "path": "/etc/sonic/.reboot/.history-reboot-cause.txt"}
            ],
        },
        {
            "name": "cpu_reboot",
            "monitor_point": {"gettype":"devfile", "path":"/dev/cpld1", "offset":0x38, "read_len":1, "okval":[0x03, 0x04, 0x05]},
            "record": [
                {"record_type":"file", "mode":"cover", "log":"CPU reboot, ", "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
                {"record_type": "file", "mode": "add", "log": "CPU reboot, ", "path": "/etc/sonic/.reboot/.history-reboot-cause.txt"}
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

UPGRADE_SUMMARY = {
    "devtype": 0x4141,

    "slot0": {
        "subtype": 0,
        "VME": {
            "chain1": {
                "name": "CPU_CPLD",
                "is_support_warm_upg": 0,
            },
            "chain2": {
                "name": "CTRL_CPLD",
                "is_support_warm_upg": 0,
            },
            "chain3": {
                "name": "PORT_CPLD",
                "is_support_warm_upg": 0,
            },
        },

        "SPI-LOGIC-DEV": {
            "chain2": {
                "name": "FPGA",
                "is_support_warm_upg": 0,
            },
        },

        "TEST": {
            "cpld": [
                {"chain": 1, "file": "/etc/.upgrade_test/VerifyID_LinkCheck_LCMXO3LF_6900C_header.vme", "display_name": "CPU_CPLD"},
                {"chain": 2, "file": "/etc/.upgrade_test/W5000_48CON2SFP_CTL_CPLD_VERIFYID_20260128_header.vme", "display_name": "CTRL_CPLD"},
                {"chain": 3, "file": "/etc/.upgrade_test/W5000_48CON2SFP_PORT_CPLD_VERIFYID_20260128_header.vme", "display_name": "PORT_CPLD"},
            ],
            "fpga": [
                {"chain": 2, "file": "/etc/.upgrade_test/w5000_48con2sfp_top_ver4401_260127update_header.bin", "display_name": "FPGA"},
            ],
        },
    },
}

PLATFORM_E2_CONF = {
    "syseeprom": [
        {"name": "syseeprom", "e2_type": "onie_tlv", "e2_path": "/sys/bus/i2c/devices/3-0050/eeprom"},
    ],
}
