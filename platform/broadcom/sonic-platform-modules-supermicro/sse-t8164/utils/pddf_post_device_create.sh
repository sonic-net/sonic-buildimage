#!/bin/bash

echo 1 > /sys/class/CPLD1/cpld_device/sfp_ven || true
echo 0xff > /sys/class/CPLD1/cpld_device/osfp_efuse_enable_1 || true
echo 0xff > /sys/class/CPLD1/cpld_device/osfp_efuse_enable_2 || true
