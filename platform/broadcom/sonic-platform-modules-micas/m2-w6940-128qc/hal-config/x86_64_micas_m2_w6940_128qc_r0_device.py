#!/usr/bin/python3

psu_fan_airflow = {
    "intake": ['GW-CRPS2000DWA']
}

fanairflow = {
    "intake": ['FAN80-01-F'],
}

psu_display_name = {
    "PA2000I-F": ['GW-CRPS2000DWA'],
}

psutypedecode = {
    0x00: 'N/A',
    0x01: 'AC',
    0x02: 'DC',
}

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
    
    "thermal_config": {
        "number": 10,
    },
    
    "fan_config": {
        "num_fantrays": 5,
        "num_fans_pertray": 2,
    },

    "psu_config": {
        "number": 4
    },

    "cpld_config": {
        "number": 7
    },

    "bmc_config": {
        "number": 1
    },

    "fpga_config": {
        "number": 1,
        "fpgas": [
            {
                "name": "fpga0",
                "alias": "MAC FPGA",
                "type": "XC7A50T-2FGG484I",
                "firmware_version": {
                    "dev_path": "/dev/fpga0",
                    "offset": 0x00,
                    "len": 4
                },
                "board_version": {
                    "dev_path": "/dev/fpga0",
                    "offset": 0x04,
                    "len": 4
                }
            }
        ]
    },

    "bios_config": {
        "number": 1,
        "bios": [
            {
                "name": "bios0",
                "alias": "BIOS",
                "type": {
                    "cmd": "dmidecode -t 0 |grep Vendor |awk -F\": \"  \'{ print $2 }\'",
                    "gettype": "cmd"
                },
                "firmware_version": {
                    "cmd": "dmidecode -t 0 |grep Version |awk -F\": \"  \'{ print $2 }\'",
                    "gettype": "cmd"
                },
                "board_version": {
                    "cmd": "dmidecode -t 0 |grep Version |awk -F\": \"  \'{ print $2 }\'",
                    "gettype": "cmd"
                }
            }
        ]
    },

    "cpu": [
        {
            "name": "cpu",
            "reboot_cause_path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"
        }
    ],
    "sfps": {
        "ver": '1.0',
        "port_index_start": 0,
        "port_num": 128,
        "log_level": 2,
        "eeprom_retry_times": 5,
        "eeprom_retry_break_sec": 0.2,
        "presence_cpld": {
            "dev_id": {
                3: {
                    "offset": {
                        0x30: "1-4, 7-10",
                        0x31: "13-16, 19-22",
                        0x32: "25-28, 31-34",
                        0x33: "37-40, 43-46",
                        0x34: "49-52, 55-58",
                        0x35: "61-64",
                    },
                },
                4: {
                    "offset": {
                        0x30: "67-70, 73-76",
                        0x31: "79-82, 85-88",
                        0x32: "91-94, 97-100",
                        0x33: "103-106, 109-112",
                        0x34: "115-118, 121-124",
                        0x35: "125-128",
                    },
                },
                2: {
                    "offset": {
                        0x30: "5-6, 11-12, 17-18, 23-24",
                        0x31: "29-30, 35-36, 41-42, 47-48",
                        0x32: "53-54, 59-60, 65-66, 71-72",
                        0x33: "77-78, 83-84, 89-90, 95-96",
                        0x34: "101-102, 107-108, 113-114, 119-120",
                    },
                },
            },
        },
        "presence_val_is_present": 0,
        "eeprom_path": "/sys/bus/i2c/devices/i2c-%d/%d-0050/eeprom",
        "eeprom_path_key": list(range(25, 153)),
        "optoe_driver_path": "/sys/bus/i2c/devices/i2c-%d/%d-0050/dev_class",
        "optoe_driver_key": list(range(25, 153)),
        "reset_cpld": {
            "dev_id": {
                3: {
                    "offset": {
                        0x90: "1-4, 7-10",
                        0x91: "13-16, 19-22",
                        0x92: "25-28, 31-34",
                        0x93: "37-40, 43-46",
                        0x94: "49-52, 55-58",
                        0x95: "61-64",
                    },
                },
                4: {
                    "offset": {
                        0x90: "67-70, 73-76",
                        0x91: "79-82, 85-88",
                        0x92: "91-94, 97-100",
                        0x93: "103-106, 109-112",
                        0x94: "115-118, 121-124",
                        0x95: "125-128",
                    },
                },
                2: {
                    "offset": {
                        0x90: "5-6, 11-12, 17-18, 23-24",
                        0x91: "29-30, 35-36, 41-42, 47-48",
                        0x92: "53-54, 59-60, 65-66, 71-72",
                        0x93: "77-78, 83-84, 89-90, 95-96",
                        0x94: "101-102, 107-108, 113-114, 119-120",
                    },
                },
            },
        },
        "reset_val_is_reset": 0,
    }
}
