#!/usr/bin/python
# -*- coding: UTF-8 -*-
from ragilecommon import *
PCA9548START  = -1
PCA9548BUSEND = -2

RAGILE_CARDID      = 0x00004065
RAGILE_PRODUCTNAME = "RA-B6010-48GT4X-R"
RAGILE_PART_NUMBER    = "01017247"
RAGILE_LABEL_REVISION = "R01"
RAGILE_ONIE_VERSION   = "2018.05-rc1"
RAGILE_MAC_SIZE       = 3
RAGILE_MANUF_NAME     = "Ragile"
RAGILE_MANUF_COUNTRY  = "USA"
RAGILE_VENDOR_NAME    = "Ragile"
RAGILE_DIAG_VERSION   = "0.1.0.15"
RAGILE_SERVICE_TAG    = "www.ragilenetworks.com"

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

# rg_eeprom  = "1-0056/eeprom"
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
        {"name":"ragile_common dfd_my_type=0x4065", "delay":0},
        {"name":"firmware_driver", "delay":0},
        {"name":"rg_cpld", "delay":0},
        {"name":"rg_at24", "delay":0},
        #{"name":"spi-gpio", "delay":0},
        #{"name":"rg_spi_gpio", "delay":0},
        #{"name":"tpm_tis_core", "delay":0},
        #{"name":"tpm_tis_spi", "delay":0},
        {"name":"optoe", "delay":0},
]

DEVICE = [
        {"name":"rtcpcf85063","bus":1,"loc":0x51 },
        {"name":"rg_24c02","bus":1,"loc":0x56 },
        {"name":"rg_cpld","bus":3,"loc":0x30 },
        {"name":"rg_24c02","bus":5,"loc":0x50 },
        {"name":"rg_24c02","bus":5,"loc":0x57 },
]

INIT_PARAM = [
    {"loc":"3-0030/tx_write_protect","value": "59","delay":1},
    {"loc":"3-0030/tx_disable","value": "00"},
    {"loc":"3-0030/tx_write_protect","value": "4e"},
]

INIT_COMMAND = [
    "hwclock -s",
]

