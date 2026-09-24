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

PROJECT_NAME = 'as7535_28xb'
DEBUG = False
ARGS = []
FORCE = 0

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
            if new_i2c_device(driver, addr, bus_number) != True:
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
            if new_i2c_device(driver, 0x50, bus_number) != True:
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

ipmi_ko = [
    'modprobe ipmi_msghandler',
    'modprobe ipmi_ssif',
    'modprobe ipmi_si',
    'modprobe ipmi_devintf']

ATTEMPTS = 5
INTERVAL = 3

def init_ipmi_dev_intf():
    attempts = ATTEMPTS
    interval = INTERVAL

    while attempts:
        for i in range(0, len(ipmi_ko)):
            subprocess.getstatusoutput(ipmi_ko[i])

        if os.path.exists('/dev/ipmi0') or os.path.exists('/dev/ipmidev/0'):
            return (0, (ATTEMPTS - attempts) * interval)

        for i in reversed(range(0, len(ipmi_ko))):
            rm = ipmi_ko[i].replace("modprobe", "modprobe -rq")
            subprocess.getstatusoutput(rm)

        attempts -= 1
        time.sleep(interval)

    return (1, ATTEMPTS * interval)

def init_ipmi_oem_cmd():
    attempts = ATTEMPTS
    interval = INTERVAL

    while attempts:
        status, output = subprocess.getstatusoutput('ipmitool raw 0x34 0x95')
        if status:
            attempts -= 1
            time.sleep(interval)
            continue

        return (0, (ATTEMPTS - attempts) * interval)

    return (1, ATTEMPTS * interval)

def init_ipmi():
    attempts = ATTEMPTS
    interval = 60

    while attempts:
        attempts -= 1

        (status, elapsed_dev) = init_ipmi_dev_intf()
        if status:
            time.sleep(interval - elapsed_dev)
            continue

        (status, elapsed_oem) = init_ipmi_oem_cmd()
        if status:
            time.sleep(interval - elapsed_dev - elapsed_oem)
            continue

        print('IPMI dev interface is ready.')
        return 0

    print('Failed to initialize IPMI dev interface')
    return 1

kos = [
    'depmod -ae',
    'modprobe i2c_dev',
    'modprobe i2c_mux_pca954x force_deselect_on_exit=1',
    'modprobe x86-64-accton-as7535-28xb-cpld',
    'modprobe x86-64-accton-as7535-28xb-fan',
    'modprobe x86-64-accton-as7535-28xb-thermal',
    'modprobe x86-64-accton-as7535-28xb-leds',
    'modprobe x86-64-accton-as7535-28xb-psu',
    'modprobe x86-64-accton-as7535-28xb-sys',
    'modprobe optoe']

def driver_install():
    global FORCE

    status = init_ipmi()
    if status:
        if FORCE == 0:
            return status

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
    # multiplexer (PCA9548)
    ('pca9548', 0x77, 0), # i2c 1-8
    ('pca9548', 0x76, 1), # i2c 9-16
    ('pca9548', 0x73, 9), # i2c 17-24
    # multiplexer for QSFP ports
    ('pca9548', 0x74, 17), # i2c 25-32
    ('pca9548', 0x74, 18), # i2c 33-40
    ('pca9548', 0x74, 19), # i2c 41-48

    # CPLD
    ('as7535_28xb_cpld', 0x61, 12)]

qsfp_bus_list = [23, 21, 24, 22]

sfp_bus_list = [
    25, 26, 27, 28, 29, 30, 31, 32, 33, 34,
    35, 36, 37, 38, 39, 40, 41, 42, 43, 44,
    45, 46, 47, 48]

def device_install():
    global FORCE

    if new_i2c_devices(i2c_device_list) is not True:
        return 1

    for i in range(1, 5):
        path = '/sys/bus/i2c/devices/12-0061/module_reset_'+str(i)
        if write_txt_file(path, '0') is not True:
            return 1

    if new_sfp_devices(qsfp_bus_list, 'optoe1') is not True:
        return 1

    if new_sfp_devices(sfp_bus_list, 'optoe2') is not True:
        return 1

    port_bus_list = qsfp_bus_list + sfp_bus_list
    for p in range(len(port_bus_list)):
        path = "/sys/bus/i2c/devices/{0}-0050/port_name".format(port_bus_list[p])
        if write_txt_file(path, 'port{0}'.format(p+1)) is not True:
            return 1

    return

def device_uninstall():
    global FORCE

    if delete_sfp_devices(qsfp_bus_list+sfp_bus_list) is not True:
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
    ret1 = log_os_system('ls ' + i2c_prefix + '*0077', 0)
    ret2 = log_os_system('ls ' + i2c_prefix + 'i2c-2', 0)
    return not (ret1[0] or ret2[0])

if __name__ == '__main__':
    main()
