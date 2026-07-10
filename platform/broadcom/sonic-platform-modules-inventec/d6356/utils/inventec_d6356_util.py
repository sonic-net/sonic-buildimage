#!/usr/bin/env python
#
# Copyright (C) 2017 Inventec, Inc.
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
    install         : install drivers and generate related sysfs nodes
    clean           : uninstall drivers and remove related sysfs nodes
"""

import os
import commands
import sys, getopt
import logging
import re
import time
from collections import namedtuple

DEBUG = False
args = []
FORCE = 0
i2c_prefix = '/sys/bus/i2c/devices/'


if DEBUG == True:
    print sys.argv[0]
    print 'ARGV: ', sys.argv[1:]


def main():
    global DEBUG
    global args
    global FORCE


    if len(sys.argv)<2:
        show_help()

    options, args = getopt.getopt(sys.argv[1:], 'hdf', ['help',
                                                       'debug',
                                                       'force',
                                                          ])
    if DEBUG == True:
        print options
        print args
        print len(sys.argv)

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
    print __doc__ % {'scriptName' : sys.argv[0].split("/")[-1]}
    sys.exit(0)

def show_log(txt):
    if DEBUG == True:
        print "[D6356]"+txt
    return

def exec_cmd(cmd, show):
    logging.info('Run :'+cmd)
    status, output = commands.getstatusoutput(cmd)
    show_log (cmd +" with result:" + str(status))
    show_log ("      output:"+output)
    if status:
        logging.info('Failed :'+cmd)
        if show:
            print('Failed :'+cmd)
    return  status, output


instantiate = [
'echo inv_eeprom 0x55 > /sys/bus/i2c/devices/i2c-0/i2c-2/new_device'
#'echo inv_cpld 0x33 > /sys/bus/i2c/devices/i2c-0/i2c-2/new_device',
#'echo inv_cpld 0x77 > /sys/bus/i2c/devices/i2c-0/i2c-2/new_device'
]


drivers =[
#kernel-dirvers
'lpc_ich',
'i2c-i801',
'i2c-mux',
'i2c-mux-pca954x',
'i2c-mux-pca9541',
'i2c-dev',
'ucd9000',
#inv-modules
'inv_eeprom',
'inv_cpld',
'inv_platform',
#'monitor',
'swps']



def system_install():
    global FORCE

    #remove default drivers to avoid modprobe order conflicts
    status, output = exec_cmd("rmmod i2c_ismt ", 1)
    status, output = exec_cmd("rmmod i2c-i801 ", 1)
    status, output = exec_cmd("rmmod gpio_ich ", 1)
    status, output = exec_cmd("rmmod lpc_ich ", 1)

    #insert extra module
    status, output = exec_cmd("insmod /lib/modules/4.9.0-11-2-amd64/kernel/drivers/gpio/gpio-ich.ko gpiobase=0",1)

    #install drivers
    for i in range(0,len(drivers)):
       status, output = exec_cmd("modprobe "+drivers[i], 1)
    if status:
	   print output
	   if FORCE == 0:
	      return status

    #instantiate devices
    for i in range(0,len(instantiate)):
       #time.sleep(1)
       status, output = exec_cmd(instantiate[i], 1)
    if status:
	   print output
	   if FORCE == 0:                
	      return status
#
# INV_FIX-4037
# It replaces the original sff8436 driver with the optoe driver
#
    #optoe map to i2c-bus
    for i in range(14,22):
        status, output =exec_cmd("echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-1/i2c-5/i2c-6/i2c-"+str(i)+"/new_device", 1)
        if status:
            print output
            if FORCE == 0:
                return status
    for i in range(22,30):
        status, output =exec_cmd("echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-1/i2c-5/i2c-7/i2c-"+str(i)+"/new_device", 1)
        if status:
            print output
            if FORCE == 0:
                return status
    for i in range(30,38):
        status, output =exec_cmd("echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-1/i2c-5/i2c-8/i2c-"+str(i)+"/new_device", 1)
        if status:
            print output
            if FORCE == 0:
                return status
    for i in range(38,46):
        status, output =exec_cmd("echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-1/i2c-5/i2c-9/i2c-"+str(i)+"/new_device", 1)
        if status:
            print output
            if FORCE == 0:
                return status
    for i in range(46,54):
        status, output =exec_cmd("echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-1/i2c-5/i2c-10/i2c-"+str(i)+"/new_device", 1)
        if status:
            print output
            if FORCE == 0:
                return status
    for i in range(54,62):
        status, output =exec_cmd("echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-1/i2c-5/i2c-11/i2c-"+str(i)+"/new_device", 1)
        if status:
            print output
            if FORCE == 0:
                return status
    for i in range(62,70):
        status, output =exec_cmd("echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-1/i2c-5/i2c-12/i2c-"+str(i)+"/new_device", 1)
        if status:
            print output
            if FORCE == 0:
                return status
    return


def system_ready():
    if not device_found():
        return False
    return True

def install():
    if not device_found():
        print "No device, installing...."
        status = system_install()
        if status:
            if FORCE == 0:
                return status
    else:
        print "D6356 devices detected...."
    return

def uninstall():
    global FORCE
    #uninstall drivers
    exec_cmd("rmmod gpio_ich",1)
    for i in range(len(drivers)-1,-1,-1):
       status, output = exec_cmd("rmmod "+drivers[i], 1)
    if status:
	   print output
	   if FORCE == 0:
	      return status
    return

def device_found():
    ret1, log = exec_cmd("ls "+i2c_prefix+"*0072", 0)
    ret2, log = exec_cmd("ls "+i2c_prefix+"i2c-5", 0)
    return not(ret1 or ret2)

if __name__ == "__main__":
    main()


