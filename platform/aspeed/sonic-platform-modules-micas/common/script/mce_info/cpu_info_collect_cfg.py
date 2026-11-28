#!/usr/bin/env python_nos
# -*- coding: UTF-8 -*-

class RegType:
    PCIE = 0x01
    MSR  = 0x02

class RegWidth:
    WIDTH_1B = 0x01
    WIDTH_2B = 0x02
    WIDTH_4B = 0x04
    WIDTH_8B = 0x08


CPU_REG_INFO_CFG0 = {
    "reg0": {
        "reg_desc": "VID",
        "reg_type": RegType.PCIE,
        "reg_addr": {
            "busnum": 0x01, "devnum": 0x00, "func": 0x00,
            "width": RegWidth.WIDTH_4B,
            "addr_range": 0x10,
        },
        "get_cmd": {
            "cmd0": "xxx",
        },
        "get_val": [0x11],
        "ok_val": [0x00],

        # Parse the bit field information of the entire register. If certain bits indicate an error, extract the error-related parsing information.
        "reg_bit_domain_parse_info" : [
            {
                "parse_cfg0": {"mask": 0x03, "val": 0x01, "parse_info": "bit0_1_val == 1 analytical information"},
                "parse_cfg1": {"mask": 0x03, "val": 0x02, "parse_info": "bit0_1_val == 2 analytical information"},
                "parse_cfg2": {"mask": 0x03, "val": 0x03, "parse_info": "bit0_1_val == 3 analytical information"},
                "parse_cfg3": {"mask": 0x10, "val": 0x10, "parse_info": "bit4_val == 1 analytical information"},
                "parse_cfg4": {"mask": 0x80, "val": 0x00, "parse_info": "bit7_val == 0 analytical information"},
            },
        ],
        
        # The bit field expansion of the entire register. That is, when a certain bit of the overall status register indicates an error, which register groups' information should be read.
        "is_leaf_node" : False,
        "reg_bit_domain_spread": {
            # When the value of bit0-1 is 1, the information of the register to be read
            "val1_regs_info": {
                "mask": 0x03, "val": 0x01,
                "regs_arr": {
                    "reg0": {
                        "reg_desc": "xxx",
                        "reg_type": RegType.PCIE,
                        "reg_addr": {
                            "busnum": 0x03, "devnum": 0x00, "func": 0x00,
                            "width": RegWidth.WIDTH_4B,
                            "addr_range": 0x14,
                        },
                        "get_cmd": {
                            # Command for reading a single register
                        },
                        "get_val": 0x11,

                        # The bit field parsing information of the entire register
                        "reg_bit_domain_parse_info" : [
                            {
                                "parse_cfg0": {"mask": 0x03, "val": 0x01, "parse_info": "bit0_1_val == 1 analytical information"},
                                "parse_cfg1": {"mask": 0x10, "val": 0x10, "parse_info": "bit4_val == 1 analytical information"},
                                "parse_cfg2": {"mask": 0x80, "val": 0x00, "parse_info": "bit7_val == 0 analytical information"},
                            },
                        ],
                    },
                    "reg1": {
                        "reg_desc": "xxx",
                        "reg_type": RegType.PCIE,
                        "reg_addr": {
                            "busnum": 0x03, "devnum": 0x00, "func": 0x00,
                            "width": RegWidth.WIDTH_4B,
                            "addr_range": 0x20,
                        },
                        "get_cmd": {
                            # Command for reading a single register
                        },
                        "get_val": 0x11,

                        # The bit field parsing information of the entire register
                        "reg_bit_domain_parse_info" : [
                            {
                                "parse_cfg0": {"mask": 0x03, "val": 0x01, "parse_info": "bit0_1_val == 1 analytical information"},
                                "parse_cfg1": {"mask": 0x10, "val": 0x10, "parse_info": "bit4_val == 1 analytical information"},
                                "parse_cfg2": {"mask": 0x80, "val": 0x00, "parse_info": "bit7_val == 0 analytical information"},
                            },
                        ],
                    },
                },
            },

            # When the value of bit0-1 is 2, the information of the register to be read
            "val2_regs_info": {
                "mask": 0x03, "val": 0x02,
                "regs_arr": {
                    "reg0-1": {
                        "reg_desc": "xxx",
                        "reg_type": RegType.PCIE,
                        "reg_addr": {
                            "busnum": 0x02, "devnum": 0x00, "func": 0x00,
                            "width": RegWidth.WIDTH_4B,
                            "addr_range": {
                                "base": 0x10, "len": 0x8,
                            },
                        },
                        "get_cmd": {
                            # Command for reading two registers
                        },
                        "get_val": [0x11, 0x22],

                        "reg_bit_domain_parse_info": [
                            # The bit field parsing information of reg0
                            {
                                "parse_cfg0": {"val": 1, "parse_info": "bit0_1_val == 0'b00 analytical information"},
                                "parse_cfg1": {"val": 2, "parse_info": "bit0_1_val == 0'b01 analytical information"},
                                "parse_cfg2": {"val": 3, "parse_info": "bit0_1_val == 0'b11 analytical information"},
                            },
                            # The bit field parsing information of reg1
                            {
                                "parse_cfg0": {"val": 1, "parse_info": "bit0_1_val == 0'b00 analytical information"},
                                "parse_cfg1": {"val": 2, "parse_info": "bit0_1_val == 0'b01 analytical information"},
                                "parse_cfg2": {"val": 3, "parse_info": "bit0_1_val == 0'b11 analytical information"},
                            },
                        ],
                    },
                },
            },
        },
    },

    "reg1_5": {
        "reg_desc": ["VID", "DID"],
        "reg_type": RegType.PCIE,
        "reg_addr": {
            "busnum": 0x02, "devnum": 0x00, "func": 0x00,
            "width": RegWidth.WIDTH_4B,
            "addr_range": {
                "base": 0x18, "len": 0x08,
            },
        },
        "get_cmd": {
            # Command to read two registers
        },
        "get_val": [0x11, 0x16],

        # The bit field parsing information of the entire register
        "reg_bit_domain_parse_info": [
            {
                "parse_cfg0": {"mask": 0x03, "val": 0x01, "parse_info": "bit0_1_val == 0'b00 analytical information"},
                "parse_cfg1": {"mask": 0x03, "val": 0x02, "parse_info": "bit0_1_val == 0'b01 analytical information"},
                "parse_cfg2": {"mask": 0x03, "val": 0x03, "parse_info": "bit0_1_val == 0'b11 analytical information"},
                "parse_cfg3": {"mask": 0x10, "val": 0x10, "parse_info": "bit4_val == 0'b1 analytical information"},
                "parse_cfg4": {"mask": 0x80, "val": 0x00, "parse_info": "bit7_val == 0'b0 analytical information"},
            },
            {
                "parse_cfg0": {"mask": 0x03, "val": 0x01, "parse_info": "bit0_1_val == 0'b00 analytical information"},
                "parse_cfg1": {"mask": 0x03, "val": 0x02, "parse_info": "bit0_1_val == 0'b01 analytical information"},
                "parse_cfg2": {"mask": 0x03, "val": 0x03, "parse_info": "bit0_1_val == 0'b11 analytical information"},
                "parse_cfg3": {"mask": 0x10, "val": 0x10, "parse_info": "bit4_val == 0'b1 analytical information"},
                "parse_cfg4": {"mask": 0x80, "val": 0x00, "parse_info": "bit7_val == 0'b0 analytical information"},
            },
        ],
    },
}

CPU_REG_INFO_CFG1 = {
}

CPU_REG_INFO_CFG2 = {
}


CPU_FUNC_INFO_CFG = {
    "func0" : {
        "func_desc" : "get cpu temp",
        "get_cmd" : {
            # Provide the command for reading the CPU temperature
        },
        "get_val" : None,
    },
    "func1" : {
        "func_desc" : "get cpu id",
        "get_cmd" : {
            # The command for reading the CPU ID
        },
        "get_val" : None,
    },
}


CPU_STATUS_PARAM = {
    # check_val, if the value is true, then "check_ok" indicates that the CPU has entered the power-off state.
    "power_off_status_cfg" : {"gettype": "i2c", "bus": 2, "loc": 0x37, "offset": 0x51, "mask": 0x01, "okval": 0x01},
    "reboot_status_cfg"    : {"gettype": "i2c", "bus": 2, "loc": 0x37, "offset": 0x51, "mask": 0x02, "okval": 0x02},
}






