#!/usr/bin/env python_nos

COLLECT_MODULES = [
    "i2c",
    "disk",
    "nic"
]

RUNNINGDATA_TYPE = "runningdata"
COMPONENT_TYPE = "component"

COLLECT_TYPE_MAP = {
    RUNNINGDATA_TYPE: [
        "s3ip",
        "i2c",
        "temp",
        "service",
        "pci"
    ],
    COMPONENT_TYPE: [
        "nic",
        "logic_dev",
        "psu",
        "vr",
        "disk",
        "eeprom"
    ]
}

I2C_COLLECT = {}

DISK_COLLECT = {}

NIC_COLLECT = {}
