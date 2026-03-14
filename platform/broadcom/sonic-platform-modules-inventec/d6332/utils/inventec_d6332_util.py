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
import syslog
import time
from sonic_py_common.general import getstatusoutput_noshell_pipe

DEBUG = False
args = []
FORCE = 0
FAN_VPD_CHANNEL= 1
FAN_VPD_ADDR_BASE=0x52
FAN_NUM=5
RETRY_LIMIT = 5
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
        print "[D6332]"+txt
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

def link_dir(prefix,dst):
    retry=0
    ret=False
    while(ret==False and retry<RETRY_LIMIT):
        ret=os.path.isdir(prefix)
        if ret==True:
            break
        time.sleep(0.5)
        retry+=1

    if ret==True:
        dirs=os.listdir(prefix)
        ret=False
        for i in dirs:
            if i.startswith('hwmon'):
                src=prefix+i
                os.symlink(src,dst)
                ret=True
                break
        if ret==False:
            syslog.syslog(syslog.LOG_ERR, "Can't find proper dir to link under %s" % prefix)
    else:
        syslog.syslog(syslog.LOG_ERR,"Path %s is not a dir" % prefix)

_path_prefix_list=[
    "/sys/bus/i2c/devices/i2c-pmbus-1/hwmon/",
    "/sys/bus/i2c/devices/i2c-pmbus-2/hwmon/",
    "/sys/devices/platform/coretemp.0/hwmon/",
    "/sys/bus/i2c/devices/i2c-tmp75-1/hwmon/",
    "/sys/bus/i2c/devices/i2c-tmp75-2/hwmon/",
    "/sys/bus/i2c/devices/i2c-tmp75-3/hwmon/",
    "/sys/bus/i2c/devices/i2c-tmp75-4/hwmon/"
]

_path_dst_list=[
    "/usr/share/sonic/device/x86_64-inventec_d6332-r0/psu1",
    "/usr/share/sonic/device/x86_64-inventec_d6332-r0/psu2",
    "/usr/share/sonic/device/x86_64-inventec_d6332-r0/coretemp",
    "/usr/share/sonic/device/x86_64-inventec_d6332-r0/board_thermal_1",
    "/usr/share/sonic/device/x86_64-inventec_d6332-r0/board_thermal_2",
    "/usr/share/sonic/device/x86_64-inventec_d6332-r0/board_thermal_3",
    "/usr/share/sonic/device/x86_64-inventec_d6332-r0/board_thermal_4",
]

instantiate = [
'echo inv_eeprom 0x55 > /sys/bus/i2c/devices/i2c-0/new_device'
#'echo inv_cpld 0x33 > /sys/bus/i2c/devices/i2c-0/i2c-2/new_device',
#'echo inv_cpld 0x77 > /sys/bus/i2c/devices/i2c-0/i2c-2/new_device'
]


drivers =[
#kernel-dirvers
'gpio_ich',
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
'lm75',
'inv_platform',
#'monitor',
'swps']


# Modify for fast-reboot
def system_install(boot_option):
    global FORCE

    #remove default drivers to avoid modprobe order conflicts
    status, output = exec_cmd("rmmod i2c_ismt ", 1)
    if status:
       print output
       if FORCE == 0:
          return status

    status, output = exec_cmd("rmmod i2c-i801 ", 1)
    if status:
       print output
       if FORCE == 0:
          return status

    status, output = exec_cmd("rmmod gpio_ich ", 1)
    if status:
       print output
       if FORCE == 0:
          return status

    status, output = exec_cmd("rmmod lpc_ich ", 1)
    if status:
       print output
       if FORCE == 0:
          return status

    #insert extra module
    #status, output = exec_cmd("insmod /lib/modules/4.9.0-9-2-amd64/kernel/drivers/gpio/gpio-ich.ko gpiobase=0",1)

    #install drivers
    ''' boot_option: 0 - normal, 1 - fast-reboot'''
    for i in range(0,len(drivers)):
       if drivers[i] == "swps":
           if boot_option == 1:
               status, output = exec_cmd("modprobe swps io_no_init=1", 1)
           else:
               status, output = exec_cmd("modprobe "+drivers[i], 1)
       else:
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
    for addr_offset in range (0,FAN_NUM):
        addr=FAN_VPD_ADDR_BASE+addr_offset
        cmd1 = ["i2cdetect", "-y", str(FAN_VPD_CHANNEL), str(addr), str(addr)]
        cmd2 = ["grep", f'{addr:x}']
        result, _ = getstatusoutput_noshell_pipe(cmd1, cmd2)
        if( result==0 ):
            cmd="echo inv_eeprom "+str(addr)+" > /sys/bus/i2c/devices/i2c-"+FAN_VPD_CHANNEL
            status, output = exec_cmd(cmd,1)
            if status:
               print output
               if FORCE == 0:                
                  return status
#
# INV_FIX-4037
# It replaces the original sff8436 driver with the optoe driver
#
    #optoe map to i2c-bus\
    for i in range(12,20):
        cmd="echo optoe1 0x50 > /sys/bus/i2c/devices/i2c-0/i2c-4/i2c-"+str(i)+"/new_device"
        status, output =exec_cmd(cmd,1)
        if status:
            print output
            if FORCE == 0:
                return status
    for i in range(20,28):
        status, output =exec_cmd("echo optoe1 0x50 > /sys/bus/i2c/devices/i2c-0/i2c-5/i2c-"+str(i)+"/new_device", 1)
        if status:
            print output
            if FORCE == 0:
                return status
    for i in range(28,36):
        status, output =exec_cmd("echo optoe1 0x50 > /sys/bus/i2c/devices/i2c-0/i2c-6/i2c-"+str(i)+"/new_device", 1)
        if status:
            print output
            if FORCE == 0:
                return status
    for i in range(36,44):
        status, output =exec_cmd("echo optoe1 0x50 > /sys/bus/i2c/devices/i2c-0/i2c-7/i2c-"+str(i)+"/new_device", 1)
        if status:
            print output
            if FORCE == 0:
                return status
                
    #make softlink for device info
    for i in range(0,len(_path_prefix_list)):
        if( os.path.islink(_path_dst_list[i]) ):
            os.unlink(_path_dst_list[i])
            syslog.syslog(syslog.LOG_WARNING, "Path %s exists, remove before link again" % _path_dst_list[i] )
        link_dir(_path_prefix_list[i],_path_dst_list[i])

    return


def system_ready():
    if not device_found():
        return False
    return True

def install(boot_option=0):
    ''' boot_option: 0 - normal, 1 - fast-reboot '''
    if not device_found():
        print "No device, installing...."
        status = system_install(boot_option)
        if status:
            if FORCE == 0:
                return status
    else:
        print "D6332 devices detected...."
    return

def uninstall():
    global FORCE
    #uninstall drivers
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


