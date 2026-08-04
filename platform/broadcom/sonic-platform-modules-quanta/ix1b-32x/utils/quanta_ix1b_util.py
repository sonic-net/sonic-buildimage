#!/usr/bin/env python
#
# Copyright (C) 2018 Quanta Computer Inc.
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

import commands
import sys, getopt
import logging
import time

DEBUG = False
args = []
FORCE = 0
i2c_prefix = '/sys/bus/i2c/devices/'

if DEBUG == True:
    print(sys.argv[0])
    print('ARGV      :', sys.argv[1:])

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
    if DEBUG is True:
        print(options)
        print(args)
        print(len(sys.argv))

    for opt, arg in options:
        if opt in ('-h', '--help'):
            show_help()
        elif opt in ('-d', '--debug'):
            DEBUG = True
            logging.basicConfig(level = logging.INFO)
        elif opt in ('-f', '--force'):
            FORCE = 1
        else:
            logging.info('no option')

    for arg in args:
        if arg == 'install':
            install()
        elif arg == 'clean':
            uninstall()
        else:
            show_help()

    return 0

def show_help():
    print(__doc__ % {'scriptName' : sys.argv[0].split("/")[-1]})
    sys.exit(0)

def show_log(txt):
    if DEBUG is True:
        print("[IX1B-32X]" + txt)

    return

def exec_cmd(cmd, show):
    logging.info('Run :' + cmd)
    status, output = commands.getstatusoutput(cmd)
    show_log (cmd + "with result:" + str(status))
    show_log ("      output:" + output)
    if status:
        logging.info('Failed :' + cmd)
        if show:
            print('Failed :' + cmd)

    return status, output

instantiate = [
#turn on module power
'echo 53 > /sys/class/gpio/export',
'echo out > /sys/class/gpio/gpio53/direction',
'echo 1 > /sys/class/gpio/gpio53/value',
#PSU1 present
'echo 16 > /sys/class/gpio/export',
'echo in > /sys/class/gpio/gpio16/direction',
#PSU1 power good signal
'echo 17 > /sys/class/gpio/export',
'echo in > /sys/class/gpio/gpio17/direction',
#PSU2 present
'echo 19 > /sys/class/gpio/export',
'echo in > /sys/class/gpio/gpio19/direction',
#PSU2 power good signal
'echo 20 > /sys/class/gpio/export',
'echo in > /sys/class/gpio/gpio20/direction',
#FAN 1-4 present
'echo 36 > /sys/class/gpio/export',
'echo in > /sys/class/gpio/gpio36/direction',
'echo 37 > /sys/class/gpio/export',
'echo in > /sys/class/gpio/gpio37/direction',
'echo 38 > /sys/class/gpio/export',
'echo in > /sys/class/gpio/gpio38/direction',
'echo 39 > /sys/class/gpio/export',
'echo in > /sys/class/gpio/gpio39/direction',
#turn on 100G led by default
'i2cset -y 0x13 0x38 0x00 0xff',
'i2cset -y 0x13 0x38 0x01 0xff',
'i2cset -y 0x13 0x39 0x00 0xff',
'i2cset -y 0x13 0x39 0x01 0xff'
]

drivers =[
'lpc_ich',
'i2c-i801',
'i2c-dev',
'i2c-mux-pca954x',
'gpio-pca953x',
'qci_pmbus',
'leds-gpio',
'optoe',
'qci_cpld_qsfp28',
'qci_platform_ix1b'
]

def system_install():
    global FORCE

    time.sleep(3)
    #remove default drivers to avoid modprobe order conflicts
    status, output = exec_cmd("rmmod i2c_ismt ", 1)
    status, output = exec_cmd("rmmod i2c-i801 ", 1)
    #setup driver dependency
    status, output = exec_cmd("depmod -a ", 1)

    #install drivers
    for i in range(0, len(drivers)):
        status, output = exec_cmd("modprobe " + drivers[i], 1)

    if status:
        print(output)
        if FORCE == 0:
            return status

    #instantiate devices
    for i in range(0, len(instantiate)):
        status, output = exec_cmd(instantiate[i], 1)

    if status:
        print(output)
        if FORCE == 0:
            return status

    #Mount Quanta hardware monitor driver
    status, output = exec_cmd("modprobe qci_hwmon_ix1b", 1)

    #QSFP for 1~32 port
    for port_number in range(1, 33):
        bus_number = port_number + 31
        file = "/sys/bus/i2c/devices/%d-0050/port_name" % bus_number
        with open(file, 'w') as f:
            f.write(str(port_number) + '\n')

    #Set system LED to green
    status, output = exec_cmd("echo 1 > /sys/class/leds/sysled_green/brightness", 1)

    return

def system_ready():
    if not device_found():
        return False

    return True

def install():
    if not device_found():
        print("No device, installing....")
        status = system_install()

        if status:
            if FORCE == 0:
                return status
    else:
        print(" ix1b driver already installed....")

    return

def uninstall():
    global FORCE

    #uninstall drivers
    for i in range(len(drivers) - 1, -1, -1):
        status, output = exec_cmd("rmmod " + drivers[i], 1)

    if status:
        print(output)
        if FORCE == 0:
            return status

    return

def device_found():
    ret1, log = exec_cmd("ls " + i2c_prefix + "i2c-0", 0)

    return ret1

if __name__ == "__main__":
    main()



