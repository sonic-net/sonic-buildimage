#!/usr/bin/env python3
#
# Copyright (C) 2019 Accton Networks, Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""
Usage: %(scriptName)s [options] command object

options:
    -h | --help     : this help message
    -d | --debug    : run with debug mode
    -f | --force    : ignore error during installation or clean
command:
    install     : install drivers and generate related sysfs nodes
    clean       : uninstall drivers and remove related sysfs nodes
    show        : show all systen status
    sff         : dump SFP eeprom
    set         : change board setting with fan|led|sfp
"""

import subprocess
import getopt
import sys
import logging
import time
import os

PROJECT_NAME = 'as4625_54p'
DEBUG = False
ARGS = []
FORCE = 0
LED_MODE_OFF = 0
LED_MODE_GREEN = 1  # Default value for LOC LED
LED_LOC_PATH = "/sys/class/leds/as4625_led::loc/brightness"

def main():
    global DEBUG
    global ARGS
    global FORCE

    if len(sys.argv) < 2:
        return 0

    (options, ARGS) = getopt.getopt(sys.argv[1:], 'hdf',
                                ['help','debug', 'force'])
    if DEBUG == True:
        print(options)
        print(ARGS)
        print(len(sys.argv))

    for arg in ARGS:
        if arg == 'install':
            do_install()
        elif arg == 'clean':
            do_uninstall()
        elif arg == 'api':
           do_sonic_platform_install()
        elif arg == 'api_clean':
           do_sonic_platform_clean()
    return 0

def log_os_system(cmd, show):
    logging.info('Run :' + cmd)
    status, output = subprocess.getstatusoutput(cmd)
    if status:
        logging.info('Failed :' + cmd)
        if show:
            print('Failed :' + cmd)
    return (status, output)

def driver_check():
    ret, lsmod = log_os_system("ls /sys/module/*accton*", 0)
    logging.info('mods:'+lsmod)
    if ret :
        return False
    else :
        return True

def new_device(driver, addr, bus, devdir):
    if not os.path.exists(os.path.join(bus, devdir)):
        try:
            with open("%s/new_device" % bus, "w") as f:
                f.write("%s 0x%x\n" % (driver, addr))
                f.flush()
        except Exception as e:
            print("Unexpected error initialize device %s:0x%x:%s: %s" % (driver, addr, bus, e))
            return None

        if not os.path.exists(os.path.join(bus, devdir)):
            return False
        elif driver.find('pca954') != -1:
            if not os.path.exists(os.path.join(bus, devdir, "channel-0")):
                delete_device(addr, bus, devdir)
                return False
            elif os.path.exists(os.path.join(bus, devdir, "idle_state")):
                try:
                    with open(os.path.join(bus, devdir, "idle_state"), "w") as f:
                        f.write("-2\n")
                except Exception as e:
                    return False
    else:
        print("Device %s:%x:%s already exists." % (driver, addr, bus))
        return False

    return True

def delete_device(addr, bus, devdir):
    if os.path.exists(os.path.join(bus, devdir)):
        try:
            with open("%s/delete_device" % bus, "w") as f:
                f.write("%s\n" % addr)
                f.flush()
        except Exception as e:
            print("Unexpected error delete device 0x%x:%s: %s" % (addr, bus, e))
            return None
    else:
        print("Device %x:%s does not exist." % (addr, bus))
        return True

    return True

def new_i2c_device(driver, addr, bus_number):
    bus = '/sys/bus/i2c/devices/i2c-%d' % bus_number
    devdir = "%d-%4.4x" % (bus_number, addr)
    return new_device(driver, addr, bus, devdir)

def delete_i2c_device(addr, bus_number):
    bus = '/sys/bus/i2c/devices/i2c-%d' % bus_number
    devdir = "%d-%4.4x" % (bus_number, addr)
    return delete_device(addr, bus, devdir)

def new_i2c_devices(new_device_list):
    for (driver, addr, bus_number) in new_device_list:
        for i in range(0, 30):
            if new_i2c_device(driver, addr, bus_number) is not True:
                time.sleep(1)
                continue
            else:
                break
        else:
            return False

    return True

def new_sfp_devices(sfp_bus_list, driver):
    for bus_number in sfp_bus_list:
        for i in range(0, 30):
            if new_i2c_device(driver, 0x50, bus_number) is not True:
                time.sleep(1)
                continue
            else:
                break
        else:
            return False

    return True

def delete_i2c_devices(delete_device_list):
    ret = True

    for (driver, addr, bus_number) in delete_device_list:
        if delete_i2c_device(addr, bus_number) is not True:
            ret = False
        continue

    return ret

def delete_sfp_devices(sfp_bus_list):
    ret = True

    for bus_number in sfp_bus_list:
        if delete_i2c_device(0x50, bus_number) is not True:
            ret = False
        continue

    return ret

kos = [
    'depmod -ae',
    'modprobe i2c_dev',
    'modprobe i2c_i801',
    'modprobe i2c_ismt',
    'modprobe i2c_mux_pca954x',
    'modprobe x86-64-accton-as4625-54p-cpld',
    'modprobe x86-64-accton-as4625-54p-fan',
    'modprobe x86-64-accton-as4625-54p-leds',
    'modprobe x86-64-accton-as4625-54p-psu',
    'modprobe optoe',
    'modprobe ym2651y']

def driver_install():
    global FORCE

    for i in range(0,len(kos)):
        status, output = log_os_system(kos[i], 1)
        if status:
            if FORCE == 0:
                return status
    return 0

def driver_uninstall():
    global FORCE
    for i in reversed(range(0,len(kos))):
        rm = kos[i].replace("modprobe", "modprobe -rq")
        rm = rm.replace("insmod", "rmmod")
        lst = rm.split(" ")
        if len(lst) > 3:
            del(lst[3])
        rm = " ".join(lst)
        status, output = log_os_system(rm, 1)
        if status:
            if FORCE == 0:
                return status
    return 0

def write_txt_file(file_path, value):
    try:
        with open(file_path, 'w') as fd:
            fd.write(value)
    except IOError as e:
        print("Error write value to file: %s" % str(e))
        return False
    return True

i2c_prefix = '/sys/bus/i2c/devices/'

i2c_device_list = [
    # initialize multiplexer (PCA9548)
    ('pca9548', 0x70, 1),
    ('pca9548', 0x71, 1),

    # initialize CPLD
    ('as4625_cpld1', 0x64, 0),

    # initiate PSU-1 AC Power
    ('as4625_54p_psu1', 0x50, 8),
    ('umec_up1k21r', 0x58, 8),

    # initiate PSU-2 AC Power
    ('as4625_54p_psu2', 0x51, 9),
    ('umec_up1k21r', 0x59, 9),

    # inititate LM75
    ('lm75', 0x4A, 3),
    ('lm75', 0x4B, 3),
    ('lm75', 0x4D, 3),
    ('lm75', 0x4E, 3),
    ('lm75', 0x4F, 3),

    # initiate IDPROM
    ('24c02', 0x51, 7)
]

sfp_bus_list = [ 10, 11, 12, 13, 14, 15 ]

def device_install():
    global FORCE

    if new_i2c_devices(i2c_device_list) is not True:
        return 1

    if new_sfp_devices(sfp_bus_list, 'optoe2') is not True:
        return 1

    for p in range(len(sfp_bus_list)):
        path = "/sys/bus/i2c/devices/{0}-0050/port_name".format(sfp_bus_list[p])
        if write_txt_file(path, 'port{0}'.format(p+49)) is not True:
            return 1

    return

def device_uninstall():
    global FORCE

    if delete_sfp_devices(sfp_bus_list) is not True:
        return 1

    if delete_i2c_devices(i2c_device_list[::-1]) is not True:
        return 1

    return

PLATFORM_ROOT_PATH = '/usr/share/sonic/device'
PLATFORM_API2_WHL_FILE_PY3 ='sonic_platform-1.0-py3-none-any.whl'
def do_sonic_platform_install():
    device_path = "{}{}{}{}".format(PLATFORM_ROOT_PATH, '/x86_64-accton_', PROJECT_NAME, '-r0')
    SONIC_PLATFORM_BSP_WHL_PKG_PY3 = "/".join([device_path, PLATFORM_API2_WHL_FILE_PY3])

    #Check API2.0 on py whl file
    status, output = log_os_system("pip3 show sonic-platform > /dev/null 2>&1", 0)
    if status:
        if os.path.exists(SONIC_PLATFORM_BSP_WHL_PKG_PY3):
            status, output = log_os_system("pip3 install "+ SONIC_PLATFORM_BSP_WHL_PKG_PY3, 1)
            if status:
                print("Error: Failed to install {}".format(PLATFORM_API2_WHL_FILE_PY3))
                return status
            else:
                print("Successfully installed {} package".format(PLATFORM_API2_WHL_FILE_PY3))
        else:
            print('{} is not found'.format(PLATFORM_API2_WHL_FILE_PY3))
    else:
        print('{} has installed'.format(PLATFORM_API2_WHL_FILE_PY3))

def do_sonic_platform_clean():
    status, output = log_os_system("pip3 show sonic-platform > /dev/null 2>&1", 0)
    if status:
        print('{} does not install, not need to uninstall'.format(PLATFORM_API2_WHL_FILE_PY3))

    else:
        status, output = log_os_system("pip3 uninstall sonic-platform -y", 0)
        if status:
            print('Error: Failed to uninstall {}'.format(PLATFORM_API2_WHL_FILE_PY3))
            return status
        else:
            print('{} is uninstalled'.format(PLATFORM_API2_WHL_FILE_PY3))

    return

def set_loc_led(color):
    global FORCE

    if os.path.exists(LED_LOC_PATH):
        cmd = 'echo {} > {}'.format(color, LED_LOC_PATH)
        try:
            status, output = log_os_system(cmd, 1)
            if status:
                print(output)
                if FORCE == 0:
                    return status
        except Exception as e:
            print({}.format(e))
    else:
        print('{} does not exist.'.format(LED_LOC_PATH))

    return

def do_install():
    print('Checking system....')
    if driver_check() is False:
        print('No driver, installing....')
        status = driver_install()
        if status:
            if FORCE == 0:
                return status
    else:
        print(PROJECT_NAME.upper() + ' drivers detected....')
    if not device_exist():
        print('No device, installing....')
        status = device_install()
        if status:
            if FORCE == 0:
                return status
    else:
        print(PROJECT_NAME.upper() + ' devices detected....')

    set_loc_led(LED_MODE_OFF)

    do_sonic_platform_install()

    return

def do_uninstall():
    print('Checking system....')
    if not device_exist():
        print(PROJECT_NAME.upper() + ' has no device installed....')
    else:
        print('Removing device....')
        status = device_uninstall()
        if status and FORCE == 0:
            return status

    if driver_check() is False:
        print(PROJECT_NAME.upper() + ' has no driver installed....')
    else:
        print('Removing installed driver....')
        status = driver_uninstall()
        if status and FORCE == 0:
            return status

    do_sonic_platform_clean()

    return None

def device_exist():
    ret1 = log_os_system('ls ' + i2c_prefix + '*0070', 0)
    ret2 = log_os_system('ls ' + i2c_prefix + 'i2c-2', 0)
    return not (ret1[0] or ret2[0])

if __name__ == '__main__':
    main()
