#!/usr/bin/env python
#
# Description: Module contains common APIs for fpga access
#
# Copyright (c) 2021, Juniper Networks, Inc.
# All rights reserved.
#
try:
    import os
    import glob
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

GPIO_SLAVE0_PORT_START = 0
GPIO_SLAVE0_PORT_END = 15
GPIO_SLAVE1_PORT_START = 16
GPIO_SLAVE1_PORT_END = 31

GPIO_PORT_START = 0
GPIO_PORT_END = 31

GPIO_PRESENCE_OFFSET = 16
GPIO_LPMODE_OFFSET = 48
GPIO_RESET_OFFSET = 0

gpio_sfp_presence = {}
gpio_sfp_lpmode = {}
gpio_sfp_reset = {}

SFP_I2C_OFFSET = 14

def gpio_create_file(gpio_pin):
    gpio_export_path = "/sys/class/gpio/export"
    gpio_pin_path = "/sys/class/gpio/gpio" + str(gpio_pin)
    if not os.path.exists(gpio_pin_path):
        try:
            gpio_export_file = open(gpio_export_path, 'w')
            gpio_export_file.write(str(gpio_pin))
            gpio_export_file.close()
        except IOError as e:
            print("Error: unable to open export file: %s" % str(e))
            return False
     
    return True

def gpio_sfp_port_init(gpio_base, port):
        presence_pin = gpio_base + GPIO_PRESENCE_OFFSET + (port % 16)
        if gpio_create_file(presence_pin):
            gpio_sfp_presence[port] = presence_pin
        reset_pin = gpio_base + GPIO_RESET_OFFSET + (port % 16)
        if gpio_create_file(reset_pin):
            gpio_sfp_reset[port] = reset_pin
        lpmode_pin = gpio_base + GPIO_LPMODE_OFFSET + (port % 16)
        if gpio_create_file(lpmode_pin):
            gpio_sfp_lpmode[port] = lpmode_pin


def gpio_sfp_slave_init(gpio_base_path, gpio_port_start, gpio_port_end):
    flist = glob.glob(gpio_base_path)
    if len(flist) == 1:
        try:
            fp = open(flist[0]+"/base")
            gpio_base = int(fp.readline().rstrip())
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return
        
        for port in range(gpio_port_start, gpio_port_end + 1):
            gpio_sfp_port_init(gpio_base, port)

def gpio_sfp_base_init():
    gpio_sfp_slave_init("/sys/bus/platform/drivers/gpioslave-tmc/gpioslave-tmc.21/gpio/gpio*",
                   GPIO_SLAVE0_PORT_START, GPIO_SLAVE0_PORT_END)
    gpio_sfp_slave_init("/sys/bus/platform/drivers/gpioslave-tmc/gpioslave-tmc.22/gpio/gpio*",
                   GPIO_SLAVE1_PORT_START, GPIO_SLAVE1_PORT_END)

def gpio_sfp_read(gpio_pin):
    gpio_pin_path = "/sys/class/gpio/gpio" + str(gpio_pin)
    value = 0

    try:
        reg_file = open(gpio_pin_path +"/value")
        value = int(reg_file.readline().rstrip())
    except IOError as e:
         print("error: unable to open file: %s" % str(e))
    
    return value

def gpio_sfp_write(gpio_pin, value):
    success = False
    gpio_pin_path = "/sys/class/gpio/gpio" + str(gpio_pin)

    try:
        gpio_file = open(gpio_pin_path +"/value", 'w')
        gpio_file.write(str(value))
        success = True
    except IOError as e:
         print("error: unable to open file: %s" % str(e))

    return success

def gpio_sfp_presence_get(port):
    if port not in list(gpio_sfp_presence.keys()):
        print("Port:" + str(port) +  " not in sfp dict")
        return 0

    gpio_pin = gpio_sfp_presence[port]
    return gpio_sfp_read(gpio_pin)

def gpio_sfp_lpmode_get(port):
    if port not in list(gpio_sfp_lpmode.keys()):
        return 0

    gpio_pin = gpio_sfp_lpmode[port]
    return gpio_sfp_read(gpio_pin)

def gpio_sfp_lpmode_set(port, value):
    if port not in list(gpio_sfp_lpmode.keys()):
        return False

    gpio_pin = gpio_sfp_lpmode[port]
    return gpio_sfp_write(gpio_pin, value)

def gpio_sfp_reset_set(port, value):
    if port not in list(gpio_sfp_reset.keys()):
        return False

    gpio_pin = gpio_sfp_reset[port]
    return gpio_sfp_write(gpio_pin, value)

def gpio_sfp_reset_get(port):
    if port not in list(gpio_sfp_reset.keys()):
        return False

    gpio_pin = gpio_sfp_reset[port]
    return gpio_sfp_read(gpio_pin)

def get_low_power_mode(self, port_num):
    reg_value = gpio_sfp_lpmode_get(port_num)
    if reg_value == 0:
        return True
    return False

def set_low_power_mode(self, port_num, lpmode):
    if lpmode == False:
        lpmode = 1
    else:
        lpmode = 0 
    status = gpio_sfp_lpmode_set(port_num, lpmode)
    return status

def reset(self, port_num):
    reset_val = 0
    status = gpio_sfp_reset_set(port_num, reset_val)
    return status
