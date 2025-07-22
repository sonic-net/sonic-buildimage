#!/usr/bin/python3
# -*- coding: UTF-8 -*-

# demo config
# @name: from logic_dev eg: /dev/cpld0 name cpld0
# @offset: config in list will read from cache file, can use "~","-","_"
# @lens: bin lens, default 256

# result: cpld0 will generate two file : [/tmp/.cpld0_cache /tmp/.cpld0_make]

GENERATE_MASK_CONFIG = [
        {"name": "cpld0", "offset": ["1", '3-4', '5-8', '0x90-0xa0', '0xb0_0xc0', '0xd0~0xd5']},
        {"name": "cpld6", "offset": ["1", '3-4', '5-8', '0x90-0xa0', '0xb0_0xc0', '0xd0~0xd5']},
        {"name": "fpga0", "offset": ["1", '3-4', '5-8', '0x90-0xa0', '0xb0_0xc0', '0xd0~0xd5'], 'lens': 0x20000},
    ]

GENERATE_I2C_DEV_CONFIG = [
        {"name": "optoe1", "bus": 98, "addr": 0x50, "offset": ["1-4"], 'lens': 0x8080},
    ]
