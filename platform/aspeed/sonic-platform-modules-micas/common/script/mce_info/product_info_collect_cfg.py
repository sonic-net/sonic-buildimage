#!/usr/bin/env python_nos
# -*- coding: UTF-8 -*-


PRODUCT_REG_INFO_CFG = {
    "reg1_5" : {
        "reg_desc" : "VID",
        "reg_type" : "cpld",
        "reg_addr" : {
            "addr_range" : {
                "base" : 0x18, "len" : 0x08,
            },
        },
        "get_cmd" :{
            # Command to read two registers
        },
        "get_val" :[0x11, 0x16],
    },
}

PRODUCT_FUNC_INFO_CFG = {
    "func0" : {
        "func_desc" : "dump base cpld reg",
        "get_cmd" : {
            "cmd" : "dfd_debug sysfs_file_rd /dev/cpld1 0 256", "gettype": "cmd",
        },
        "get_val" : None
    },
    "func1" : {
        "func_desc" : "get cpu id",
        "get_cmd" : {
            # The command for reading the CPU ID
        },
        "get_val" : None
    },
}
