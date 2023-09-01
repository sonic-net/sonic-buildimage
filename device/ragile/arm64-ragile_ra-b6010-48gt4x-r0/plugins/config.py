#!/usr/bin/python
# -*- coding: UTF-8 -*-
RAGILE_CARDID      = 0x00004099
RAGILE_PRODUCTNAME = "RA-B6010-48GT4X"
RAGILE_PART_NUMBER    = "RJ000001"
RAGILE_LABEL_REVISION = "R01"
RAGILE_ONIE_VERSION   = "2018.02"
RAGILE_MAC_SIZE       = 3
RAGILE_MANUF_NAME     = "Ragile"
RAGILE_MANUF_COUNTRY  = "CHN"
RAGILE_VENDOR_NAME    = "Ragile"
RAGILE_DIAG_VERSION   = "0.1.0.15"
RAGILE_SERVICE_TAG    = "www.ragile.com"

CPUEEPROMS = {"name":"cpueeprom","bus":5,"loc":0x57,"E2PRODUCT":'2', "E2TYPE":'4'}

# rg_eeprom  = "1-0056/eeprom"
E2_LOC = {"bus":1, "devno":0x56}
E2_PROTECT = {}
FAN_PROTECT = {"bus":1, "devno":0x0d, "addr":0x19, "open":0x00, "close":0xff}


