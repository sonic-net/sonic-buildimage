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

import os
import subprocess
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
    if DEBUG == True:
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
    if DEBUG == True:
        print("[IX8C-56X]" + txt)
    return

def exec_cmd(cmd, show):
    logging.info('Run :' + cmd)
    status, output = subprocess.getstatusoutput(cmd)
    show_log (cmd +"with result:" + str(status))
    show_log ("      output:" + output)
    if status:
        logging.info('Failed :' + cmd)
        if show:
            print('Failed :' + cmd)
    return status, output

pca954x_bus_addr =[
'0-0072',
'0-0077',
'5-0073',
'6-0073',
'7-0073',
'8-0073',
'9-0073',
'10-0073',
'11-0073'
]

instantiate =[
#export pca9698 for qsfp present
'echo 34 > /sys/class/gpio/export',
'echo in > /sys/class/gpio/gpio34/direction',
'echo 38 > /sys/class/gpio/export',
'echo in > /sys/class/gpio/gpio38/direction',
'echo 42 > /sys/class/gpio/export',
'echo in > /sys/class/gpio/gpio42/direction',
'echo 46 > /sys/class/gpio/export',
'echo in > /sys/class/gpio/gpio46/direction',
'echo 50 > /sys/class/gpio/export',
'echo in > /sys/class/gpio/gpio50/direction',
'echo 54 > /sys/class/gpio/export',
'echo in > /sys/class/gpio/gpio54/direction',
'echo 58 > /sys/class/gpio/export',
'echo in > /sys/class/gpio/gpio58/direction',
'echo 62 > /sys/class/gpio/export',
'echo in > /sys/class/gpio/gpio62/direction',
#export pca9698 for qsfp lpmode
'echo 35 > /sys/class/gpio/export',
'echo out > /sys/class/gpio/gpio35/direction',
'echo 0 >/sys/class/gpio/gpio35/value',
'echo 39 > /sys/class/gpio/export',
'echo out > /sys/class/gpio/gpio39/direction',
'echo 0 >/sys/class/gpio/gpio39/value',
'echo 43 > /sys/class/gpio/export',
'echo out > /sys/class/gpio/gpio43/direction',
'echo 0 >/sys/class/gpio/gpio43/value',
'echo 47 > /sys/class/gpio/export',
'echo out > /sys/class/gpio/gpio47/direction',
'echo 0 >/sys/class/gpio/gpio47/value',
'echo 51 > /sys/class/gpio/export',
'echo out > /sys/class/gpio/gpio51/direction',
'echo 0 >/sys/class/gpio/gpio51/value',
'echo 55 > /sys/class/gpio/export',
'echo out > /sys/class/gpio/gpio55/direction',
'echo 0 >/sys/class/gpio/gpio55/value',
'echo 59 > /sys/class/gpio/export',
'echo out > /sys/class/gpio/gpio59/direction',
'echo 0 >/sys/class/gpio/gpio59/value',
'echo 63 > /sys/class/gpio/export',
'echo out > /sys/class/gpio/gpio63/direction',
'echo 0 >/sys/class/gpio/gpio63/value',
#Enable front-ports LED decoding
'echo 1 > /sys/class/cpld-led/CPLDLED-1/led_decode',
'echo 1 > /sys/class/cpld-led/CPLDLED-2/led_decode',
#SFP28 Module TxEnable
'echo 0 > /sys/class/cpld-sfp28/port-1/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-2/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-3/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-4/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-5/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-6/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-7/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-8/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-9/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-10/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-11/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-12/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-13/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-14/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-15/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-16/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-17/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-18/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-19/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-20/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-21/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-22/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-23/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-24/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-25/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-26/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-27/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-28/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-29/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-30/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-31/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-32/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-33/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-34/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-35/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-36/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-37/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-38/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-39/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-40/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-41/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-42/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-43/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-44/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-45/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-46/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-47/tx_dis',
'echo 0 > /sys/class/cpld-sfp28/port-48/tx_dis'
]

drivers =[
'lpc_ich',
'i2c-i801',
'i2c-dev',
'i2c-mux-pca954x',
'gpio-pca953x',
'optoe',
'qci_cpld_sfp28',
'qci_cpld_led',
'qci_platform_ix8c',
'quanta_hwmon_ipmi',
'ipmi_devintf'
]

un_drivers =[
'lpc_ich',
'i2c-i801',
'i2c-dev',
'i2c-mux-pca954x',
'gpio-pca953x',
'optoe',
'qci_cpld_sfp28',
'qci_cpld_led',
'qci_platform_ix8c',
'quanta_hwmon_ipmi',
'ipmi_devintf'
]

def system_install():
    global FORCE

    #setup driver dependency
    exec_cmd("depmod -a ", 1)
    #install drivers
    for i in range(0,len(drivers)):
        status, output = exec_cmd("modprobe " + drivers[i], 1)
        if status:
            print(output)
            #retry quanta_hwmon_ipmi in case it init failed due to ipmi_msghandler init uncompleted
            if drivers[i] == 'quanta_hwmon_ipmi':
                for _ in range(0, 3):
                    time.sleep(3)
                    ret, out = exec_cmd("modprobe quanta_hwmon_ipmi ", 1)
                    if ret == 0:
                        break
                if ret and FORCE == 0:
                    return ret
            elif FORCE == 0:
                return status

    #reload ethernet drivers in correct order
    exec_cmd("rmmod ixgbe ", 1)
    exec_cmd("rmmod igb ", 1)
    exec_cmd("modprobe igb ", 1)
    exec_cmd("modprobe ixgbe ", 1)

    # set pca954x idle_state as -2: MUX_IDLE_DISCONNECT
    for i in range(0,len(pca954x_bus_addr)):
        exec_cmd("echo -2 > /sys/bus/i2c/drivers/pca954x/{}/idle_state".format(pca954x_bus_addr[i]), 1)

    #turn on module power
    exec_cmd("echo 21 > /sys/class/gpio/export ", 1)
    exec_cmd("echo high > /sys/class/gpio/gpio21/direction ", 1)

    time.sleep(1)
    # qsfp reset gpio
    for qsfp_reset in [32, 36, 40, 44, 48, 52, 56, 60]:
        exec_cmd("echo "+str(qsfp_reset)+" > /sys/class/gpio/export", 1)
        exec_cmd("echo high > /sys/class/gpio/gpio"+str(qsfp_reset)+"/direction", 1)

    # Reset fron-ports LED CPLD
    exec_cmd("echo 73 > /sys/class/gpio/export ", 1)
    status, output = exec_cmd("cat /sys/class/gpio/gpio73/value", 1)
    if output != '1':
        exec_cmd("echo out > /sys/class/gpio/gpio73/direction ", 1)
        exec_cmd("echo 0 >/sys/class/gpio/gpio73/value", 1)
        exec_cmd("echo 1 >/sys/class/gpio/gpio73/value", 1)

    #instantiate devices
    for i in range(0, len(instantiate)):
        status, output = exec_cmd(instantiate[i], 1)
        if status:
            print(output)
            if FORCE == 0:
                return status

    #QSFP for 1~56 port
    for port_number in range(1, 57):
        bus_number = port_number + 12
        os.system("echo %d >/sys/bus/i2c/devices/%d-0050/port_name" % (port_number, bus_number))

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
        status, output = exec_cmd("pip3 install  /usr/share/sonic/device/x86_64-quanta_ix8c_bwde-r0/sonic_platform-1.0-py3-none-any.whl",1)
        if status:
            print(output)
            if FORCE == 0:
                return status
    else:
        print(" ix8c driver already installed....")
    return

def uninstall():
    global FORCE
    #uninstall drivers
    for i in range(len(un_drivers) - 1, -1, -1):
        status, output = exec_cmd("rmmod " + un_drivers[i], 1)
    if status:
        print(output)
        if FORCE == 0:
            return status

    status, output = exec_cmd("pip3 uninstall  sonic-platform -y ",1)
    if status:
        print(output)
        if FORCE == 0:
            return status

    return

def device_found():
    ret1, log1 = exec_cmd("cat /proc/modules | grep ix8c > /tmp/chkdriver.log", 0)
    ret2, log2 = exec_cmd("cat /tmp/chkdriver.log | grep ix8c", 0)

    if ret1 == 0 and len(log2) > 0:
        return True
    else:
        return False

if __name__ == "__main__":
    main()
