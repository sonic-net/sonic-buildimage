#!/usr/bin/python
# -*- coding: UTF-8 -*-
from platform_common import *
PCA9548START  = -1
PCA9548BUSEND = -2

PLATFORM_CARDID      = 0x00004065
PLATFORM_PRODUCTNAME = "M2-W6010-48GT4X-FA"
PLATFORM_PART_NUMBER    = "01016994"
PLATFORM_LABEL_REVISION = "R01"
PLATFORM_ONIE_VERSION   = "2018.05-rc1"
PLATFORM_MAC_SIZE       = 3
PLATFORM_MANUF_NAME     = "Micas"
PLATFORM_MANUF_COUNTRY  = "USA"
PLATFORM_VENDOR_NAME    = "Micas"
PLATFORM_DIAG_VERSION   = "0.1.0.15"
PLATFORM_SERVICE_TAG    = "www.micasnetworks.com"

LOCAL_LED_CONTROL = {
    "CLOSE":{},
    "OPEN":{}
}

MACLED_PARAMS = []

# start system modules
STARTMODULE  = {
                "i2ccheck":0,
                "fancontrol":0,
                "avscontrol":0,
                "avscontrol_restful":0,
                "sfptempmodule":0,
                "sfptempmodule_interval":3,
                "macledreset": 0,
                "macledreset_interval": 5,
                "macledset_param":MACLED_PARAMS,
                }

FRULISTS = [
            {"name":"mmceeprom","bus":5,"loc":0x50, "E2PRODUCT":'2', "E2TYPE":'5' , "CANRESET":'1'},
            {"name":"cpueeprom","bus":5,"loc":0x57,"E2PRODUCT":'2', "E2TYPE":'4', "CANRESET":'1' },
        ]

# eeprom  = "1-0056/eeprom"
E2_LOC = {"bus":1, "devno":0x56}
E2_PROTECT = {}


CPLDVERSIONS = [
        {"bus":2, "devno":0x0d, "name":"CPU_CPLD"},
		{"bus":3, "devno":0x30, "name":"MAC_BOARD_CPLD_1"},
]

FIRMWARE_TOOLS = {"cpld":  [{"channel":"0","cmd":"firmware_upgrade %s cpld %s cpld", "successtips":"CPLD Upgrade succeeded!"}
                           ],
              }

# drivers list
DRIVERLISTS = [
        {"name":"i2c_dev", "delay":0},
        {"name":"i2c_algo_bit","delay":0},
        {"name":"spi-bitbang", "delay":0},
        {"name":"i2c_mux", "delay":0},
        {"name":"rtcpcf85063", "delay":0},
        {"name":"i2c_mux_pca954x", "delay":0}, # force_deselect_on_exit=1
        {"name":"platform_common dfd_my_type=0x4065", "delay":0},
        {"name":"wb_cpld", "delay":0},
        {"name":"wb_at24", "delay":0},
        {"name":"optoe", "delay":0},
]

DEVICE = [
        {"name":"rtcpcf85063","bus":1,"loc":0x51 },
        {"name":"wb_24c02","bus":1,"loc":0x56 },
        {"name":"wb_cpld","bus":3,"loc":0x30 },
        {"name":"wb_24c02","bus":5,"loc":0x50 },
        {"name":"wb_24c02","bus":5,"loc":0x57 },
]

INIT_PARAM = [
    {"loc":"3-0030/tx_write_protect","value": "59","delay":1},
    {"loc":"3-0030/tx_disable","value": "00"},
    {"loc":"3-0030/tx_write_protect","value": "4e"},
]

INIT_COMMAND = [
    "hwclock -s",
]

