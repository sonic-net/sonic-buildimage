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

import os
import subprocess
import getopt
import sys
import logging
import time

PROJECT_NAME = 'as5812_54x'
version = '0.2.0'
verbose = False
DEBUG = False
args = []
ALL_DEVICE = {}
DEVICE_NO = {'led':5, 'fan1':1, 'fan2':1,'fan3':1,'fan4':1,'fan5':1,'thermal':3, 'psu':2, 'sfp':54}


i2c_prefix = '/sys/bus/i2c/devices/'
'''
i2c_bus = {'thermal': ['61-0048','62-0049', '63-004a'] ,
           'psu': ['57-0050','58-0053'],
           'sfp': ['-0050']}
'''

QSFP_START = 48

sfp_map =  [2, 3, 4, 5, 6, 7, 8, 9, 10,
            11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
            21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
            31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
            41, 42, 43, 44, 45, 46, 47, 48, 49,
            50, 52, 54, 51, 53, 55]

port_cpld_path = [ "/sys/bus/i2c/devices/0-0061/"
                  ,'/sys/bus/i2c/devices/0-0062/']

mknod =[
'echo as5812_54x_cpld1 0x60 > /sys/bus/i2c/devices/i2c-0/new_device',
'echo as5812_54x_cpld2 0x61 > /sys/bus/i2c/devices/i2c-0/new_device',
'echo as5812_54x_cpld3 0x62 > /sys/bus/i2c/devices/i2c-0/new_device',
'echo pca9548 0x70 > /sys/bus/i2c/devices/i2c-1/new_device',

# PSU-1
'echo as5812_54x_psu1 0x38 > /sys/bus/i2c/devices/i2c-57/new_device',
'echo cpr_4011_4mxx  0x3c > /sys/bus/i2c/devices/i2c-57/new_device',
'echo as5812_54x_psu1 0x50 > /sys/bus/i2c/devices/i2c-57/new_device',

# PSU-2
'echo as5812_54x_psu2 0x3b > /sys/bus/i2c/devices/i2c-58/new_device',
'echo cpr_4011_4mxx 0x3f > /sys/bus/i2c/devices/i2c-58/new_device',
'echo as5812_54x_psu2 0x50 > /sys/bus/i2c/devices/i2c-58/new_device',

'echo lm75 0x48 > /sys/bus/i2c/devices/i2c-61/new_device',
'echo lm75 0x49 > /sys/bus/i2c/devices/i2c-62/new_device',
'echo lm75 0x4a > /sys/bus/i2c/devices/i2c-63/new_device',

#EERPOM
'echo 24c02 0x57 > /sys/bus/i2c/devices/i2c-1/new_device',
]

mknod2 =[
'echo as5812_54x_cpld1 0x60 > /sys/bus/i2c/devices/i2c-1/new_device',
'echo as5812_54x_cpld2 0x61 > /sys/bus/i2c/devices/i2c-1/new_device',
'echo as5812_54x_cpld3 0x62 > /sys/bus/i2c/devices/i2c-1/new_device',
'echo pca9548 0x70 > /sys/bus/i2c/devices/i2c-0/new_device',

# PSU-1
'echo as5812_54x_psu1 0x38 > /sys/bus/i2c/devices/i2c-57/new_device',
'echo cpr_4011_4mxx  0x3c > /sys/bus/i2c/devices/i2c-57/new_device',
'echo as5812_54x_psu1 0x50 > /sys/bus/i2c/devices/i2c-57/new_device',

# PSU-2
'echo as5812_54x_psu2 0x3b > /sys/bus/i2c/devices/i2c-58/new_device',
'echo cpr_4011_4mxx 0x3f > /sys/bus/i2c/devices/i2c-58/new_device',
'echo as5812_54x_psu2 0x50 > /sys/bus/i2c/devices/i2c-58/new_device',

'echo lm75 0x48 > /sys/bus/i2c/devices/i2c-61/new_device',
'echo lm75 0x49 > /sys/bus/i2c/devices/i2c-62/new_device',
'echo lm75 0x4a > /sys/bus/i2c/devices/i2c-63/new_device',

#EERPOM
'echo 24c02 0x57 > /sys/bus/i2c/devices/i2c-0/new_device',
]

FORCE = 0
LED_MODE_OFF = 0
LED_MODE_AMBER = 2  # Default value for LOC LED
LED_LOC_PATH = "/sys/class/leds/accton_as5812_54x_led::loc/brightness"
logging.basicConfig(filename= PROJECT_NAME+'.log', filemode='w',level=logging.DEBUG)
logging.basicConfig(level=logging.INFO)


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
        elif arg == 'api':
           do_sonic_platform_install()
        elif arg == 'api_clean':   
           do_sonic_platform_clean()  
        else:
            show_help()


    return 0

def show_help():
    print(__doc__ % {'scriptName' : sys.argv[0].split("/")[-1]})
    sys.exit(0)

def my_log(txt):
    if DEBUG == True:
        print("[ACCTON DBG]: "+txt)
    return

def log_os_system(cmd, show):
    logging.info('Run :'+cmd)
    status = 1
    output = ""
    status, output = subprocess.getstatusoutput(cmd)
    my_log (cmd +"with result:" + str(status))
    my_log ("cmd:" + cmd)
    my_log ("      output:"+output)
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



kos = [
'depmod -ae',
'modprobe i2c_dev',
'modprobe i2c_mux_pca954x force_deselect_on_exit=1',
'modprobe i2c_i801',
'modprobe i2c_ismt',
'modprobe optoe',
'modprobe i2c-mux-accton_as5812_54x_cpld',
'modprobe cpr_4011_4mxx',
'modprobe ym2651y',
'modprobe accton_as5812_54x_fan',
'modprobe leds-accton_as5812_54x',
'modprobe accton_as5812_54x_psu']

def driver_install():
    global FORCE
    for i in range(0,len(kos)):
        status, output = log_os_system(kos[i], 1)
        if status:
            if FORCE == 0:
                return status

    print("Done driver_install")

    return 0

def driver_uninstall():
    global FORCE
    for i in range(0,len(kos)):
        #remove parameter if any
        rm = kos[-(i+1)]
        lst = rm.split(" ")
        if len(lst) > 2:
            del(lst[2:])
        rm = " ".join(lst)

        #Change to removing commands
        rm = rm.replace("modprobe", "modprobe -rq")
        rm = rm.replace("insmod", "rmmod")
        status, output = log_os_system(rm, 1)
        if status:
            if FORCE == 0:
                return status
    return 0



def i2c_order_check():
    # i2c bus 0 and 1 might be installed in different order.
    # Here check if 0x70 is exist @ i2c-0
    tmp = "i2cget -y -f 0 0x70"
    ret = log_os_system(tmp, 0)
    if not ret[0]:
        order = 1
    else:
        order = 0
    m =  "[%s]Detected I2C_BUS_ORDER:%d" % (os.path.basename(__file__), order)
    my_log(m)
    return order

def get_i2c_order():
    return i2c_order_check()

def device_install():
    global FORCE

    order = get_i2c_order()
    # if 0x76 is not exist @i2c-0, use reversed bus order
    if order:
        for i in range(0,len(mknod2)):
            #for pca954x need times to built new i2c buses
            if mknod2[i].find('pca954') != -1:
               time.sleep(2)

            status, output = log_os_system(mknod2[i], 1)
            if status:
                print(output)
                if FORCE == 0:
                    return status
    else:
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
        if i < QSFP_START:
            status, output =log_os_system("echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-"+str(sfp_map[i])+"/new_device", 1)
        else:
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

    for i in range(0,len(sfp_map)):
        target = "/sys/bus/i2c/devices/i2c-"+str(sfp_map[i])+"/delete_device"
        status, output =log_os_system("echo 0x50 > "+ target, 1)
        if status:
            print(output)
            if FORCE == 0:
                return status

    order = get_i2c_order()
    if order == 0:
        nodelist = mknod
    else:
        nodelist = mknod2

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
            print('{} is not found'.format(PLATFORM_API2_WHL_FILE_PY3))
    else:        
        print('{} has installed'.format(PLATFORM_API2_WHL_FILE_PY3))
     
    return 
     
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
    print("Checking system....")
    if driver_inserted() == False:
        print("No driver, installing....")
        status = driver_install()
        if status:
            if FORCE == 0:
                return  status
    else:
        print(PROJECT_NAME.upper()+" drivers detected....")
    if not device_exist():
        print("No device, installing....")
        status = device_install()
        if status:
            if FORCE == 0:
                return  status
    else:
        print(PROJECT_NAME.upper()+" devices detected....")

    set_loc_led(LED_MODE_OFF)

    do_sonic_platform_install()

    return

def do_uninstall():
    print("Checking system....")
    if not device_exist():
        print(PROJECT_NAME.upper() +" has no device installed....")
    else:
        print("Removing device....")
        status = device_uninstall()
        if status:
            if FORCE == 0:
                return  status

    if driver_inserted()== False :
        print(PROJECT_NAME.upper() +" has no driver installed....")
    else:
        print("Removing installed driver....")
        status = driver_uninstall()
        if status:
            if FORCE == 0:
                return  status
    do_sonic_platform_clean()

    return

def get_cpld_path(index):
    order = get_i2c_order()
    if order !=0 :
        return port_cpld_path[index].replace("0-", "1-")
    else:
        return port_cpld_path[index]

def cpld_path_of_port(port_index):
    if port_index < 1 and port_index > DEVICE_NO['sfp']:
        return None
    if port_index < 25:
        return get_cpld_path(0)
    else:
        return get_cpld_path(1)

def get_path_sfp_tx_dis(port_index):
    cpld_p = cpld_path_of_port(port_index)
    if cpld_p is None:
        return False, ''
    else:
        dev = cpld_p+"module_tx_disable_"+str(port_index)
        return True, dev

def get_path_sfp_presence(port_index):
    cpld_p = cpld_path_of_port(port_index)
    if cpld_p is None:
        return False, ''
    else:
        dev = cpld_p+"module_present_"+str(port_index)
        return True, dev

def device_exist():
    ret1, log = log_os_system("ls "+i2c_prefix+"*0070", 0)
    ret2, log = log_os_system("ls "+i2c_prefix+"i2c-2", 0)
    return not(ret1 or ret2)

if __name__ == "__main__":
    main()
