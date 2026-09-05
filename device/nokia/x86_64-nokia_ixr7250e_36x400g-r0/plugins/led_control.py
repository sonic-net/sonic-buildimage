#
# Name: led_control.py, version: 1.0
#
# Description: Module contains the Nokia specific LED control
# class object creation.
#
# Copyright (c) 2019, Nokia
# All rights reserved.
#

try:
    from platform_ndk import nokia_led_mgmt

except ImportError as e:
    raise ImportError("%s - required module not found" % e)

LedControl = nokia_led_mgmt.getLedControl()
