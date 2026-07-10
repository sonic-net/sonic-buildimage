#!/usr/bin/env python3
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
import re
import time
from sonic_py_common.general import getstatusoutput_noshell

PROJECT_NAME = 'as7726_32x'
version = '0.0.1'
verbose = False
DEBUG = False
args = []
ALL_DEVICE = {}

i2c_prefix = '/sys/bus/i2c/devices/'
'''
i2c_bus = {'fan': ['54-0066'],
           'thermal': ['54-004c', '55-0048','55-0049', '55-004a', '55-004b'] ,
           'psu': ['49-0050','50-0053'],
           'sfp': ['-0050']}
i2c_nodes = {'fan': ['present', 'front_speed_rpm', 'rear_speed_rpm'],
           'thermal': ['hwmon/hwmon*/temp1_input'] ,
           'psu': ['psu_present ', 'psu_power_good']    ,
           'sfp': ['module_present_ ', 'module_tx_disable_']}
'''
sfp_map = [21, 22, 23, 24, 26, 25, 28, 27,
             17, 18, 19, 20, 29, 30, 31, 32,
             33, 34, 35, 36, 45, 46, 47, 48,
             37, 38, 39, 40, 41, 42, 43, 44,
             15, 16]

mknod =[
'echo pca9548 0x77 > /sys/bus/i2c/devices/i2c-0/new_device',
'echo pca9548 0x76 > /sys/bus/i2c/devices/i2c-1/new_device',
'echo pca9548 0x72 > /sys/bus/i2c/devices/i2c-1/new_device',
'echo pca9548 0x73 > /sys/bus/i2c/devices/i2c-1/new_device',
'echo pca9548 0x74 > /sys/bus/i2c/devices/i2c-1/new_device',
'echo pca9548 0x75 > /sys/bus/i2c/devices/i2c-1/new_device',
'echo pca9548 0x71 > /sys/bus/i2c/devices/i2c-2/new_device',

'echo as7726_32x_cpld1 0x60 > /sys/bus/i2c/devices/i2c-11/new_device',
'echo as7726_32x_cpld2 0x62 > /sys/bus/i2c/devices/i2c-12/new_device',
'echo as7726_32x_cpld3 0x64 > /sys/bus/i2c/devices/i2c-13/new_device',

'echo as7726_32x_fan 0x66 > /sys/bus/i2c/devices/i2c-54/new_device',


'echo lm75 0x4c > /sys/bus/i2c/devices/i2c-54/new_device',
'echo lm75 0x48 > /sys/bus/i2c/devices/i2c-55/new_device',
'echo lm75 0x49 > /sys/bus/i2c/devices/i2c-55/new_device',
'echo lm75 0x4a > /sys/bus/i2c/devices/i2c-55/new_device',
'echo lm75 0x4b > /sys/bus/i2c/devices/i2c-55/new_device',


# PSU-1
'echo as7726_32x_psu1 0x53 > /sys/bus/i2c/devices/i2c-50/new_device',
'echo ym2651 0x5b > /sys/bus/i2c/devices/i2c-50/new_device',

# PSU-2
'echo as7726_32x_psu2 0x50> /sys/bus/i2c/devices/i2c-49/new_device',
'echo ym2651 0x58 > /sys/bus/i2c/devices/i2c-49/new_device',

#EERPOM
'echo 24c02 0x56 > /sys/bus/i2c/devices/i2c-0/new_device',
]



FORCE = 0
logging.basicConfig(filename= PROJECT_NAME+'.log', filemode='w',level=logging.DEBUG)
logging.basicConfig(level=logging.INFO)


if DEBUG == True:
    print((sys.argv[0]))
    print(('ARGV      :', sys.argv[1:]))


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
        print((len(sys.argv)))

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
    print(( __doc__ % {'scriptName' : sys.argv[0].split("/")[-1]}))
    sys.exit(0)

def dis_i2c_ir3570a(addr):
    cmd = ["i2cset", "-y", "0", "0x"+"%x"%addr, "0xE5", "0x01"]
    status, output = getstatusoutput_noshell(cmd)
    cmd = ["i2cset", "-y", "0", "0x"+"%x"%addr, "0x12", "0x02"]
    status, output = getstatusoutput_noshell(cmd)
    return status

def ir3570_check():
    cmd = ["i2cdump", "-y", "0", "0x42", "s", "0x9a"]
    try:
        status, output = getstatusoutput_noshell(cmd)
        lines = output.split('\n')
        hn = re.findall(r'\w+', lines[-1])
        version = int(hn[1], 16)
        if version == 0x24:  #only for ir3570a
            ret = dis_i2c_ir3570a(4)
        else:
            ret = 0
    except Exception as e:
        print(( "Error on ir3570_check() e:" + str(e)))
        return -1
    return ret


def my_log(txt):
    if DEBUG == True:
        print(("[ACCTON DBG]: ",txt))
    return

def log_os_system(cmd, show):
    logging.info('Run :'+cmd)
    status = 1
    output = ""
    status, output = subprocess.getstatusoutput(cmd)
    my_log (cmd +" with result:" + str(status))
    #my_log ("cmd:" + cmd)
    #my_log ("      output:"+output)
    if status:
        logging.info('Failed :'+cmd)
        if show:
            print(('Failed :'+cmd))
    return  status, output

def driver_inserted():
    ret, lsmod = log_os_system("ls /sys/module/*accton*", 0)
    logging.info('mods:'+lsmod)
    if ret :
        return False
    else :
        return True

def cpld_reset_mac():
    ret, lsmod = log_os_system("i2cset -y 0 0x77 0x1", 0)
    ret, lsmod = log_os_system("i2cset -y 0 0x76 0x4", 0)
    ret, lsmod = log_os_system("i2cset -y 0 0x60 0x8 0x77", 0)
    time.sleep(1)
    ret, lsmod = log_os_system("i2cset -y 0 0x60 0x8 0xf7", 0)
    return True



#'modprobe cpr_4011_4mxx',

kos = [
'depmod -ae',
'modprobe i2c_dev',
'modprobe i2c_mux_pca954x force_deselect_on_exit=1',
'modprobe ym2651y',
'modprobe accton_as7726_32x_cpld',
'modprobe accton_as7726_32x_fan',
'modprobe accton_as7726_32x_leds',
'modprobe accton_as7726_32x_psu',
'modprobe optoe']

def driver_install():
    global FORCE
    for i in range(0,len(kos)):
        status, output = log_os_system(kos[i], 1)
        if status:
            if FORCE == 0:
                return status

    print("Done driver_install")
    
    #status=cpld_reset_mac()
    return 0

def driver_uninstall():
    global FORCE
    for i in range(0,len(kos)):
        rm = kos[-(i+1)].replace("modprobe", "modprobe -rq")        
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

    for i in range(0,len(mknod)):
        #for pca954x need times to built new i2c buses
        if mknod[i].find('pca954') != -1:
            time.sleep(2)
        
        status, output = log_os_system(mknod[i], 1)        
        if status:
            print(output)
            if FORCE == 0:
                return status

    for i in range(0,len(sfp_map)):
        status, output =log_os_system("echo optoe1 0x50 > /sys/bus/i2c/devices/i2c-"+str(sfp_map[i])+"/new_device", 1)
        if status:
            print(output)
            if FORCE == 0:
                return status
        status, output =log_os_system("echo port"+str(i)+" > /sys/bus/i2c/devices/"+str(sfp_map[i])+"-0050/port_name", 1)
        if status:
            print(output)
            if FORCE == 0:
                return status

    print("Done device_install")

    return

def device_uninstall():
    global FORCE

    status, output =log_os_system("ls /sys/bus/i2c/devices/0-0070", 0)
    if status==0:
        I2C_ORDER=1
    else:
    	I2C_ORDER=0

    for i in range(0,len(sfp_map)):
        target = "/sys/bus/i2c/devices/i2c-"+str(sfp_map[i])+"/delete_device"
        status, output =log_os_system("echo 0x50 > "+ target, 1)
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
    if driver_inserted() == False:        
        return False
    if not device_exist():
        print("not device_exist()")
        return False
    return True

def do_install():
    if driver_inserted() == False:
        status = driver_install()
        if status:
            if FORCE == 0:
                return  status
    else:
        print((PROJECT_NAME.upper()+" drivers detected...."))

    ir3570_check()

    if not device_exist():
        status = device_install()
        if status:
            if FORCE == 0:
                return  status
    else:
        print((PROJECT_NAME.upper()+" devices detected...."))
    return

def do_uninstall():
    if not device_exist():
        print((PROJECT_NAME.upper()+" has no device installed...."))
    else:
        print("Removing device....")
        status = device_uninstall()
        if status:
            if FORCE == 0:
                return  status

    if driver_inserted()== False :
        print((PROJECT_NAME.upper()+" has no driver installed...."))
    else:
        print("Removing installed driver....")
        status = driver_uninstall()
        if status:
            if FORCE == 0:
                return  status

    return

def device_exist():
    ret1, log = log_os_system("ls "+i2c_prefix+"*0077", 0)
    ret2, log = log_os_system("ls "+i2c_prefix+"i2c-2", 0)
    return not(ret1 or ret2)

if __name__ == "__main__":
    main()
