#!/usr/bin/env python3
#
# Copyright (C) 2019 Alphanetworks Technology Corporation.
# Robin Chen <Robin_chen@Alphanetworks.com>
# This program is free software: you can redistribute it and/or modify 
# it under the terms of the GNU General Public License as published by 
# the Free Software Foundation, either version 3 of the License, or 
# any later version. 
#
# This program is distributed in the hope that it will be useful, 
# but WITHOUT ANY WARRANTY; without even the implied warranty of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
# GNU General Public License for more details. 
# see <http://www.gnu.org/licenses/>
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
import sys, getopt
import logging
import time

PROJECT_NAME = 'snj60d0-320f'
DRIVER_NAME = 'snj60d0_320f'
version = '0.1.0'
verbose = False
DEBUG = False
args = []
FORCE = 0

if DEBUG == True:
    print(sys.argv[0])
    print('ARGV      :', sys.argv[1:])


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
        elif arg == 'clean':
           do_uninstall()
        else:
            show_help()

    return 0

def show_help():
    print(__doc__ % {'scriptName' : sys.argv[0].split("/")[-1]})
    sys.exit(0)

def show_log(txt):
    if DEBUG == True:
        print(PROJECT_NAME.upper()+": "+txt)
    return

def log_os_system(cmd, show):
    logging.info('Run :'+cmd)  
    status, output = subprocess.getstatusoutput(cmd)    
    show_log(cmd +" with result: " + str(status))
    show_log("      output:"+output)
    if status:
        logging.info('Failed :'+cmd)
        if show:
            print('Failed :'+cmd)
    return  status, output

def driver_check():
    ret, lsmod = log_os_system("lsmod | grep " + DRIVER_NAME, 0)
    logging.info('mods:'+lsmod)
    if len(lsmod) ==0:
        return False   
    return True


kos = [
'modprobe i2c_ismt',
'modprobe i2c_i801',
'modprobe i2c_dev',
'modprobe i2c_mux_pca954x',
'modprobe optoe',
'modprobe yesm1300am',
'modprobe '+PROJECT_NAME+'_fpga'  ,
'modprobe '+PROJECT_NAME+'_onie_eeprom' ,
'modprobe '+PROJECT_NAME+'_i2c_mux_cpld' ]

def driver_install():
    global FORCE
    #remove default drivers to avoid modprobe order conflicts
    log_os_system("echo 'blacklist i2c-ismt' > /etc/modprobe.d/blacklist.conf", 1)
    time.sleep(1)
    log_os_system("modprobe -r i2c-ismt ", 1)
    log_os_system("modprobe -r i2c-i801 ", 1)
    #setup driver dependency
    status, output = log_os_system("depmod", 1)
    if status:
        if FORCE == 0:
            return status
    for i in range(0,len(kos)):
        if kos[i].find('pca954') != -1:
            status, output = log_os_system(kos[i]+ " force_deselect_on_exit=1", 1)
        else:
            status, output = log_os_system(kos[i], 1)
        if status:
            if FORCE == 0:
                return status
    return 0

def driver_uninstall():
    global FORCE
    for i in range(0,len(kos)):
        rm = kos[-(i+1)].replace("modprobe", "modprobe -rq")
        rm = rm.replace("insmod", "rmmod")
        status, output = log_os_system(rm, 1)
        if status:
            if FORCE == 0:
                return status
    return 0


i2c_prefix = '/sys/bus/i2c/devices/'

sfp_map = [ 22,23,24,25,26,27,28,29,
            30,31,32,33,34,35,36,37,
            38,39,40,41,42,43,44,45,
            46,47,48,49,50,51,52,53]

mknod =[
'echo snj60d0_onie_eeprom 0x56 > /sys/bus/i2c/devices/i2c-1/new_device',
'echo lm75 0x4F > /sys/bus/i2c/devices/i2c-1/new_device',
'echo pca9548 0x70 > /sys/bus/i2c/devices/i2c-1/new_device',
'echo snj60d0_fpga 0x5e > /sys/bus/i2c/devices/i2c-1/new_device',
'echo lm75 0x4D > /sys/bus/i2c/devices/i2c-4/new_device',
'echo lm75 0x4C > /sys/bus/i2c/devices/i2c-5/new_device',
'echo pca9545 0x71 > /sys/bus/i2c/devices/i2c-6/new_device',
'echo pca9548 0x72 > /sys/bus/i2c/devices/i2c-8/new_device',
'echo 24c02 0x50 > /sys/bus/i2c/devices/i2c-10/new_device',
'echo yesm1300am 0x58 > /sys/bus/i2c/devices/i2c-10/new_device',
'echo 24c02 0x51 > /sys/bus/i2c/devices/i2c-11/new_device',
'echo yesm1300am 0x59 > /sys/bus/i2c/devices/i2c-11/new_device',
'echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-12/new_device',
'echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-13/new_device',
'echo snj60d0_320f_cpld1 0x5f > /sys/bus/i2c/devices/i2c-14/new_device', 
'echo snj60d0_320f_cpld2 0x5f > /sys/bus/i2c/devices/i2c-15/new_device',
'echo snj60d0_320f_cpld3 0x5f > /sys/bus/i2c/devices/i2c-16/new_device',
'echo snj60d0_320f_cpld4 0x5f > /sys/bus/i2c/devices/i2c-17/new_device', ]

port_led_disable= 'echo {value} > /sys/bus/i2c/devices/1-005e/port_led_disable'

cpld_port_led_enable=[
'echo {value} > /sys/bus/i2c/devices/14-005f/cpld_port_led_enable_1',
'echo {value} > /sys/bus/i2c/devices/15-005f/cpld_port_led_enable_2',
'echo {value} > /sys/bus/i2c/devices/16-005f/cpld_port_led_enable_3',
'echo {value} > /sys/bus/i2c/devices/17-005f/cpld_port_led_enable_4', ]

def device_install():
    global FORCE 
    
    for i in range(0,len(mknod)):
        #for pca954x need times to built new i2c buses            
        if mknod[i].find('pca954') != -1:
            time.sleep(1)         
               
        if mknod[i].find('lm75') != -1:
            time.sleep(1)
        
        status, output = log_os_system(mknod[i], 1)
        if status:
            print(output)
            if FORCE == 0:
                return status

    for port in range(len(sfp_map)):
        cmd = 'echo optoe3 0x50 > /sys/bus/i2c/devices/i2c-{}/new_device'.format(sfp_map[port])
        status, output =log_os_system(cmd, 1)
        if status:
            print(output)
            if FORCE == 0:
                return status

    cmd = "echo 0 > /sys/bus/i2c/devices/1-005e/sys_reset1"
    status, output =log_os_system(cmd, 1)
    if status:
        print(output)
        if FORCE == 0:
            return status

    cmd = "echo 4 > /sys/bus/i2c/devices/1-005e/sys_reset2"
    status, output =log_os_system(cmd, 1)
    if status:
        print(output)
        if FORCE == 0:
            return status

    # Front port LED enable
    cmd = port_led_disable.format(value=0)
    status, output =log_os_system(cmd, 1)
    if status:
        print(output)
        if FORCE == 0:
            return status

    for i in range(len(cpld_port_led_enable)):
        cmd = cpld_port_led_enable[i].format(value=1)
        status, output =log_os_system(cmd, 1)
        if status:
            print(output)
            if FORCE == 0:
                return status

    return 
    
def device_uninstall():
    global FORCE

    # Front port LED disable
    cmd = port_led_disable.format(value=1)
    status, output =log_os_system(cmd, 1)
    if status:
        print(output)
        if FORCE == 0:
            return status

    for i in range(len(cpld_port_led_enable)):
        cmd = cpld_port_led_enable[i].format(value=0)
        status, output =log_os_system(cmd, 1)
        if status:
            print(output)
            if FORCE == 0:
                return status

    for port in range(len(sfp_map)):
        cmd = 'echo 0x50 > /sys/bus/i2c/devices/i2c-{}/delete_device'.format(sfp_map[port])
        status, output =log_os_system(cmd, 1)
        if status:
            print(output)
            if FORCE == 0:
                return status

    nodelist = mknod
    for i in range(len(nodelist)):
        target = nodelist[-(i+1)]
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

def do_install():
    print("Checking system....")
    if driver_check() == False:
        print("No driver, installing....")
        status = driver_install()
        if status:
            if FORCE == 0:
                return  status
    else:
        print(PROJECT_NAME.upper() + " drivers detected....")
    if not device_exist():
        print("No device, installing....")
        status = device_install()
        if status:
            if FORCE == 0:
                return  status
    else:
        print(PROJECT_NAME.upper() + " devices detected....")
    return

def do_uninstall():
    print("Checking system....")
    if not device_exist():
        print(PROJECT_NAME.upper() + " has no device installed....")
    else:
        print("Removing device....")
        status = device_uninstall() 
        if status:
            if FORCE == 0:
                return  status  

    if driver_check()== False :
        print(PROJECT_NAME.upper() + " has no driver installed....")
    else:
        print("Removing installed driver....")
        status = driver_uninstall()
        if status:
            if FORCE == 0:
                return  status

    return

def device_exist():
    ret1, log = log_os_system("ls " + i2c_prefix + "*0070", 0)
    ret2, log = log_os_system("ls " + i2c_prefix + "i2c-2", 0)
    return not(ret1 or ret2)

if __name__ == "__main__":
    main()
