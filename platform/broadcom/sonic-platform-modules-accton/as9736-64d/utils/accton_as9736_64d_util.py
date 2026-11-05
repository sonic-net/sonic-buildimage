#!/usr/bin/env python
#
# Copyright (C) 2016 Accton Networks, Inc.
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
"""

import subprocess
import getopt
import sys
import logging
import time
import os

PROJECT_NAME = 'as9736_64d'
version = '0.0.1'
verbose = False
DEBUG = False
args = []

i2c_prefix = '/sys/bus/i2c/devices/'
xcvr_set_no_reset = 'echo 0 > /sys/bus/platform/devices/as9736_64d_fpga/module_reset_all'

mknod = [
    # i2c-mux
    'echo pca9548 0x72 > /sys/bus/i2c/devices/i2c-0/new_device',
    'echo pca9548 0x71 > /sys/bus/i2c/devices/i2c-3/new_device',
    'echo pca9548 0x71 > /sys/bus/i2c/devices/i2c-5/new_device',
    'echo pca9548 0x76 > /sys/bus/i2c/devices/i2c-9/new_device',
    'echo pca9548 0x70 > /sys/bus/i2c/devices/i2c-10/new_device',
    'echo pca9548 0x70 > /sys/bus/i2c/devices/i2c-11/new_device',
    'echo pca9548 0x70 > /sys/bus/i2c/devices/i2c-12/new_device',
    'echo pca9548 0x70 > /sys/bus/i2c/devices/i2c-18/new_device',
    'echo pca9548 0x70 > /sys/bus/i2c/devices/i2c-19/new_device',

    # CPLD
    'echo as9736_64d_cpld_cpu 0x60 > /sys/bus/i2c/devices/i2c-6/new_device',
    # PDB-L
    'echo as9736_64d_cpld_pdb 0x60 > /sys/bus/i2c/devices/i2c-36/new_device',
    # PDB-R
    'echo as9736_64d_cpld_pdb 0x60 > /sys/bus/i2c/devices/i2c-44/new_device',
    'echo as9736_64d_cpld_scm 0x35 > /sys/bus/i2c/devices/i2c-51/new_device',

    # FAN
    'echo as9736_64d_fan 0x33 > /sys/bus/i2c/devices/i2c-25/new_device',

    # LM75
    'echo lm75 0x48 > /sys/bus/i2c/devices/i2c-2/new_device',
    'echo lm75 0x49 > /sys/bus/i2c/devices/i2c-2/new_device',
    'echo lm75 0x4c > /sys/bus/i2c/devices/i2c-14/new_device',
    'echo lm75 0x48 > /sys/bus/i2c/devices/i2c-27/new_device',
    'echo lm75 0x49 > /sys/bus/i2c/devices/i2c-27/new_device',
    'echo lm75 0x48 > /sys/bus/i2c/devices/i2c-34/new_device',
    'echo lm75 0x49 > /sys/bus/i2c/devices/i2c-42/new_device',
    'echo lm75 0x48 > /sys/bus/i2c/devices/i2c-57/new_device',
    'echo lm75 0x4c > /sys/bus/i2c/devices/i2c-58/new_device',
    'echo lm75 0x4c > /sys/bus/i2c/devices/i2c-65/new_device',
    'echo lm75 0x4d > /sys/bus/i2c/devices/i2c-66/new_device',

    # PSU-1
    'echo delta_dps2400 0x59 > /sys/bus/i2c/devices/i2c-41/new_device',
    'echo as9736_64d_psu1 0x51 > /sys/bus/i2c/devices/i2c-41/new_device',

    # PSU-2
    'echo delta_dps2400 0x58 > /sys/bus/i2c/devices/i2c-33/new_device',
    'echo as9736_64d_psu2 0x50 > /sys/bus/i2c/devices/i2c-33/new_device',

    # EERPOM
    'echo 24c256 0x51 > /sys/bus/i2c/devices/i2c-20/new_device',
]

FORCE = 0

def main():
    global DEBUG
    global args
    global FORCE

    if len(sys.argv) < 2:
        show_help()

    options, args = getopt.getopt(sys.argv[1:], 'hdf', ['help',
                                                        'debug',
                                                        'force',
                                                        ])
    if DEBUG:
        print(options)
        print(args)
        print(len(sys.argv))

    for opt, arg in options:
        if opt in ('-h', '--help'):
            show_help()
        elif opt in ('-d', '--debug'):
            DEBUG = True
            logging.basicConfig(level=logging.INFO)
        elif opt in ('-f', '--force'):
            FORCE = 1
        else:
            logging.info('no option')
    for arg in args:
        if arg == 'install':
            do_install()
            do_mux_setting("fp")
        elif arg == 'clean':
            do_uninstall()
        elif arg == 'api':
            do_sonic_platform_install()
        elif arg == 'api_clean':
           do_sonic_platform_clean()
        else:
            show_help()

    return 0

def do_mux_setting(select_mode):
    print("LDB 10G MUX setting select: mac to "+ select_mode)
    status, output = log_os_system("ldb_mux_setting.sh " + select_mode, 0)

    if status:
        print('Error: Failed to do MUX setting.')
        return status

def show_help():
    print(__doc__ % {'scriptName': sys.argv[0].split("/")[-1]})
    sys.exit(0)


def my_log(txt):
    if DEBUG:
        print("[ACCTON DBG]: " + txt)
    return

def log_os_system(cmd, show):
    logging.info('Run :' + cmd)
    output = ""
    status, output = subprocess.getstatusoutput(cmd)
    my_log(cmd + "with result:" + str(status))
    my_log("cmd:" + cmd)
    my_log("      output:" + output)
    if status:
        logging.info('Failed :' + cmd)
        if show:
            print(('Failed :' + cmd))
    return status, output

def driver_check():
    ret, lsmod = log_os_system("ls /sys/module/*accton*", 0)
    logging.info('mods:' + lsmod)
    if ret:
        return False

kos = [
    'depmod',
    'modprobe i2c_i801',
    'modprobe i2c_ismt',
    'modprobe i2c_dev',
    'modprobe i2c_mux_pca954x',
    'modprobe accton_as9736_64d_cpld',
    'modprobe accton_as9736_64d_fan',
    'modprobe accton_i2c_psu',
    'modprobe accton_as9736_64d_psu',
    'modprobe accton_as9736_64d_fpga',
    'modprobe optoe',
    'modprobe tmp421',
    'modprobe lm75']

def driver_install():
    global FORCE

    for i in range(0, len(kos)):
        status, output = log_os_system(kos[i], 1)

        if status:
            if FORCE == 0:
                return status
    return 0

def driver_uninstall():
    global FORCE
    for i in range(0, len(kos)):
        rm = kos[-(i + 1)].replace("modprobe", "modprobe -rq")
        lst = rm.split(" ")

        if len(lst) > 3:
            del(lst[3])
        rm = " ".join(lst)
        status, output = log_os_system(rm, 1)
        if status:
            if FORCE == 0:
                return status
    return 0

def device_install():
    global FORCE

    for i in range(0, len(mknod)):
        # for pca954x need times to built new i2c buses
        if mknod[i].find('pca954') != -1:
            time.sleep(2)

        status, output = log_os_system(mknod[i], 1)
        if status:
            print(output)
            if FORCE == 0:
                return status

    # set all pca954x idle_disconnect
    cmd = 'echo -2 | tee /sys/bus/i2c/drivers/pca954x/*-00*/idle_state'
    status, output = log_os_system(cmd, 1)
    if status:
        print(output)
        if FORCE == 0:
            return status

    return

def device_uninstall():
    global FORCE

    nodelist = mknod

    for i in range(len(nodelist)):
        target = nodelist[-(i + 1)]
        temp = target.split()
        del temp[1]
        temp[-1] = temp[-1].replace('new_device', 'delete_device')
        status, output = log_os_system(" ".join(temp), 1)
        if status:
            print(output)
            if FORCE == 0:
                return status

    return

def system_ready():
    if driver_check() == False:
        return False
    if not device_exist():
        return False
    return True

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
            print(('{} is not found'.format(PLATFORM_API2_WHL_FILE_PY3)))
    else:
        print(('{} has installed'.format(PLATFORM_API2_WHL_FILE_PY3)))

    return

def do_sonic_platform_clean():
    status, output = log_os_system("pip3 show sonic-platform > /dev/null 2>&1", 0)
    if status:
        print(('{} does not install, not need to uninstall'.format(PLATFORM_API2_WHL_FILE_PY3)))

    else:
        status, output = log_os_system("pip3 uninstall sonic-platform -y", 0)
        if status:
            print(('Error: Failed to uninstall {}'.format(PLATFORM_API2_WHL_FILE_PY3)))
            return status
        else:
            print(('{} is uninstalled'.format(PLATFORM_API2_WHL_FILE_PY3)))

    return

def do_install():
    if driver_check() == False:
        print("No driver, installing....")
        status = driver_install()
        if status:
            if FORCE == 0:
                return status
    else:
        print(PROJECT_NAME.upper() + " drivers detected....")
    if not device_exist():
        print("No device, installing....")
        status = device_install()
        if status:
            if FORCE == 0:
                return status
    else:
        print(PROJECT_NAME.upper() + " devices detected....")

    do_sonic_platform_install()
    # Init QSFP release reset mode:
    # (FPGA default change to reset mode after v.12)
    print("Release reset mode")
    status, output = log_os_system(xcvr_set_no_reset, 1)
    if status:
        if FORCE == 0:
            return status

    return

def do_uninstall():
    if not device_exist():
        print(PROJECT_NAME.upper() + " has no device installed....")
    else:
        print("Removing device....")
        status = device_uninstall()
        if status:
            if FORCE == 0:
                return status

    if driver_check() == False:
        print(PROJECT_NAME.upper() + " has no driver installed....")
    else:
        print("Removing installed driver....")
        status = driver_uninstall()
        if status:
            if FORCE == 0:
                return status

    do_sonic_platform_clean()

    return

def device_exist():
    ret1, log = log_os_system("ls " + i2c_prefix + "*0072", 0)
    ret2, log = log_os_system("ls " + i2c_prefix + "i2c-2", 0)
    return not(ret1 or ret2)

if __name__ == "__main__":
    main()

