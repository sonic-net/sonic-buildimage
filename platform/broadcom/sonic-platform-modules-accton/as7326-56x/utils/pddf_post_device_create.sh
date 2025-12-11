#!/bin/bash


enable_powr1014a_device()
{
    echo powr1014 0x25 >  /sys/bus/i2c/devices/i2c-21/new_device
}
enable_powr1014a_device
