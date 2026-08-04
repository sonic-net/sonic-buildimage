# Ingrasys S8810-32Q Platform Driver for SONiC

Copyright (C) 2016 Ingrasys, Inc.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.


## Licensing terms

The Licensing terms of the files within this project is split 2 parts.
The Linux kernel is released under the GNU General Public License version 2.
All other code is released under the GNU General Public License version 3.
Please see the LICENSE file for copies of both licenses.

## Contents of this package

 - service/
 > Service config files for platform initialization and monitoring
 - utils/
 > Scripts useful during platform bringup and sysfs function
 - conf/
 > Platform configure files.

## Kernel modules and drivers

The driver for interacting with Ingrasys S8810-32Q is contained in the I2C 
kernel module and initialization script. The initialization script loads 
the modules in the correct order. It has been built and tested against
the Linux 3.16 kernel. 

The initialization script will modprobe the needed modules, navigate to the 
module's device directory in sysfs, and write configuration data to 
the kernel module.

### IGB

This is OOB Ports on front panel for management plane.

The IGB module must be loaded first on Ingrasys S8810-32Q platform.

### I2C i801

The I2C i801 on Ingrasys S8810-32Q can be found in
`/sys/bus/i2c/devices/i2c-0/`

This is I2C bus for Clock Gen, DIMM channel and digital potentiometers. 

The i801 module must be loaded second on Ingrasys S8810-32Q.

### I2C iSMT

The I2C iSMT module on S8810-32Q can be found in
`/sys/bus/i2c/devices/i2c-1/`

This is I2C bus for CPLD, HWM, power controller and I2C Switches.

The i801 module must be loaded third on Ingrasys S8810-32Q.

### I2C PCA9548
The PCA9548 module on S8810-32Q can be found in
`/sys/bus/i2c/devices/i2c-2/` , `/sys/bus/i2c/devices/i2c-3/`, 
`/sys/bus/i2c/devices/i2c-4/`, `/sys/bus/i2c/devices/i2c-5/`,
`/sys/bus/i2c/devices/i2c-6/`, `/sys/bus/i2c/devices/i2c-7/`,
`/sys/bus/i2c/devices/i2c-8/`, `/sys/bus/i2c/devices/i2c-9/`.

The pca9548 module for zQSFP module get/set functions, PSU information, 
fan status and EEPROM.

## Hardware components

The hardware components are initialized in the init script on S8810-32Q. 
The following describes manual initialization as well as interaction.
The examples below are just for Ingrasys S8810-32Q platform.

### Hardware initialization

When the sonic-platform-ingrasys-s8810-32q package is installed on the S8810-32Q,
it is automatically initialized. If you want to manual initialization, the 
utility command usage as follows:
```
    i2c_utils.sh i2c_init
```

### EEPROM

The EEPROM is including the board SKU, model name, vendor name, serial number, 
and other information can be accessed with the specific eeprom kernel module.
After using `modprobe eeprom_mb` to detect the eeprom, it will show up in sysfs.

The hexdump utility can be used to decode the raw output of the EEPROM. 
For example,
```
    bash# echo "mb_eeprom 0x54" > /sys/bus/i2c/devices/i2c-9/new_device
    bash# cat /sys/bus/i2c/drivers/mb_eeprom/9-0054/eeprom | hexdump -C
```

### Front panel LEDs

LEDs can be setup on/off by using i2c utility `/usr/sbin/i2c_utils.sh`.
Utility function command usage as follows:

```
Status LED:
    i2c_utils.sh i2c_sys_led green|amber on|off

Fan status LED:
    i2c_utils.sh i2c_fan_led green|amber on|off

PSU1 status LED:
    i2c_utils.sh i2c_psu1_led green|amber on|off

PSU2 status LED:
    i2c_utils.sh i2c_psu2_led green|amber on|off

```
QSFP Module Port LEDs control by ASIC library.


### Fan speed

Fan speed are controlled by the w83795 kernel module. 
It can be found in `/sys/class/hwmon/hwmon1/device/`.
If they were compiled as modules, you will need to modprobe w83795 for
their sysfs entries to show up. Each fan has an `fan<N>_input` file 
for reading the fan speed. And `pwm1` setting fan1 to fan4, 
`pwm2` setting fan5 to fan8.

There is docker-platform-monitor container installed fancontrol package that can
automatic control platform fan speed.


### Temperature sensors

Temperature sensors are controlled by the w83795 kernel
module. It can be found in `/sys/class/hwmon/hwmon1/device/`. 
If they were compiled as modules, then you will need to modprobe w83795 for
their sysfs entries to show up.
`temp1_input` is front MAC temperature sensor. `temp2_input` is rear MAC 
temperature sensor. 

There is docker-platform-monitor container installed lm-sensors package that can
monitor platform temperature. And `sensors` command can show each 
temperature sensors status.

### Power supplies

Power supplies status and its EEPROM info can be used i2c utility 
`/usr/sbin/i2c_utils.sh` to get.
The usage as follows:
```
PSU EEPROM:
    i2c_utils.sh i2c_psu_eeprom_get
    hexdump -C psu0.rom
    hexdump -C psu1.rom

PSU Status:
    i2c_utils.sh i2c_psu_status
```

### QSFPs
QSFP modules are managed by the pca9548 kernel driver.
The i2c utility `/usr/sbin/i2c_utils.sh` can be used to get status and
module EEPROM informations.
The usage as follows:
```
QSFP EEPROM:
    i2c_utils.sh i2c_qsfp_eeprom_get [1-32]

QSFP Insert Event:
    i2c_utils.sh i2c_qsfp_eeprom_get [1-32]
    0 => not insert
    1 => inserted
```

