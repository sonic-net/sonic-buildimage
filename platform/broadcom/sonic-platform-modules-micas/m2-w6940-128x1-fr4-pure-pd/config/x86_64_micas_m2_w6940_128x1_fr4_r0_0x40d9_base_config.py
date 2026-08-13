#!/usr/bin/python
# -*- coding: UTF-8 -*-

DRIVERLISTS = [
    {"name": "wb_i2c_i801", "delay": 1, "removable": 0},
    {"name": "i2c_dev", "delay": 0, "removable": 0},
    {"name": "wb_i2c_algo_bit", "delay": 0},
    {"name": "wb_i2c_gpio", "delay": 0},
    {"name": "i2c_mux", "delay": 0, "removable": 0},
    {"name": "wb_i2c_gpio_device gpio_sda=181 gpio_scl=180 gpio_chip_name=INTC3001:00 bus_num=1", "delay": 0},
    {"name": "platform_common dfd_my_type=0x40d9", "delay": 0},
    {"name": "wb_logic_dev_common", "delay":0},
    {"name": "wb_fpga_pcie", "delay": 0},
    {"name": "wb_pcie_dev", "delay": 0},
    {"name": "wb_pcie_dev_device", "delay": 0},
    {"name": "wb_i2c_ocores", "delay": 0},
    {"name": "wb_i2c_ocores_device", "delay": 0},
    {"name": "wb_i2c_mux_pca9641", "delay": 0},
    {"name": "wb_i2c_mux_pca954x", "delay": 0},
    {"name": "wb_i2c_mux_pca954x_device_b3", "delay": 1},
    {"name": "wb_pmbus_core", "delay": 0},
    {"name": "wb_csu550", "delay": 0},
]

DEVICE = [
    {"name": "wb_fsp1200", "bus": 10, "loc": 0x58},
]

INIT_PARAM = []

FINISH_PARAM = []

SECONDARY_SUBVERSION_DECODE = {"ECD26020050":"DC",
                            "default":"AC"}

SUBVERSION_CONFIG = {"get_value" : {"gettype":'io', 'io_addr':"0x907"}}

SECONDARY_SUBVERSION_CONFIG ={"get_value" : {"loc": [
                                "10-0058/hwmon/hwmon*/mfr_model",
                                "10-0058/mfr_model",
                                "10-0058/device_name",
                                "10-0058/name",
                                "10-0058/hwmon/hwmon*/device_name",
                                "10-0058/hwmon/hwmon*/name",
                            ],"gettype": "sysfs"},
                            "decode_value":SECONDARY_SUBVERSION_DECODE, "default":"AC"}
