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
import os
from sonic_py_common.general import getstatusoutput_noshell


PROJECT_NAME = 'as7326_56x'
version = '0.1.0'
verbose = False
DEBUG = False
args = []
ALL_DEVICE = {}
DEVICE_NO = {'led':5, 'fan':6,'thermal':4, 'psu':2, 'sfp':58}
FORCE = 0
#logging.basicConfig(filename= PROJECT_NAME+'.log', filemode='w',level=logging.DEBUG)
#logging.basicConfig(level=logging.INFO)


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
        elif arg == 'api':
           do_sonic_platform_install()
        elif arg == 'api_clean':
           do_sonic_platform_clean()
        elif arg == 'clean':
           do_uninstall()
        else:
            show_help()
    return 0

def show_help():
    print(__doc__ % {'scriptName' : sys.argv[0].split("/")[-1]})
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
        print("Error on ir3570_check() e:" + str(e))
        return -1
    return ret


def my_log(txt):
    if DEBUG == True:
        print("[ROY]"+txt)
    return

def log_os_system(cmd, show):
    logging.info('Run :'+cmd)
    status, output = subprocess.getstatusoutput(cmd)
    my_log (cmd +"with result:" + str(status))
    my_log ("      output:"+output)
    if status:
        logging.info('Failed :'+cmd)
        if show:
            print('Failed :'+cmd)
    return  status, output

def driver_check():
    ret, lsmod = log_os_system("ls /sys/module/*accton*", 0)
    logging.info('mods:'+lsmod)
    if ret :
        return False
    else :
        return True



kos = [
'modprobe i2c_dev',
'modprobe i2c_mux_pca954x',
'modprobe accton_i2c_cpld'  ,
'modprobe ym2651y'                  ,
'modprobe accton_as7326_56x_fan'     ,
'modprobe optoe'      ,
'modprobe accton_as7326_56x_leds'      ,
'modprobe accton_as7326_56x_psu' ]

def driver_install():
    global FORCE
    
    status, output = log_os_system('modprobe i2c_dev', 1)
    status, output = log_os_system("depmod", 1)
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
        rm = kos[-(i+1)].replace("modprobe", "modprobe -rq")
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

i2c_prefix = '/sys/bus/i2c/devices/'
'''
i2c_bus = {'fan': ['11-0066']                 ,
           'thermal': ['15-0048','15-0049', '15-004a', '15-004b'] ,
           'psu': ['17-0051','13-0053'],
           'sfp': ['-0050']}
i2c_nodes = {'fan': ['present', 'front_speed_rpm', 'rear_speed_rpm'] ,
           'thermal': ['hwmon/hwmon*/temp1_input'] ,
           'psu': ['psu_present ', 'psu_power_good']    ,
           'sfp': ['module_present_', 'module_tx_disable_']}
'''
sfp_map =  [
        42,41,44,43,47,45,46,50,
        48,49,52,51,53,56,55,54,
        58,57,60,59,61,63,62,64,
        66,68,65,67,69,71,72,70,
        74,73,76,75,77,79,78,80,
        81,82,84,85,83,87,88,86,    #port 41~48
        25,26,27,28,29,30,31,32,    #port 49~56 QSFP
        22,23]                      #port 57~58 SFP+ from CPU NIF.
qsfp_start = 48
qsfp_end   = 56

#For sideband signals of SFP/QSFP modules.
cpld_of_module = {'12-0062': list(range(0,30)),
		  '18-0060': list(range(30,58)) }


mknod =[
'echo pca9548 0x77 > /sys/bus/i2c/devices/i2c-0/new_device',
'echo pca9548 0x70 > /sys/bus/i2c/devices/i2c-1/new_device' ,
'echo pca9548 0x71 > /sys/bus/i2c/devices/i2c-1/new_device' ,
'echo pca9548 0x72 > /sys/bus/i2c/devices/i2c-24/new_device' ,
'echo pca9548 0x70 > /sys/bus/i2c/devices/i2c-2/new_device' ,
'echo pca9548 0x71 > /sys/bus/i2c/devices/i2c-33/new_device',
'echo pca9548 0x72 > /sys/bus/i2c/devices/i2c-34/new_device',
'echo pca9548 0x73 > /sys/bus/i2c/devices/i2c-35/new_device',
'echo pca9548 0x74 > /sys/bus/i2c/devices/i2c-36/new_device',
'echo pca9548 0x75 > /sys/bus/i2c/devices/i2c-37/new_device',
'echo pca9548 0x76 > /sys/bus/i2c/devices/i2c-38/new_device',

'echo as7326_56x_fan 0x66 > /sys/bus/i2c/devices/i2c-11/new_device ',
'echo lm75 0x48 > /sys/bus/i2c/devices/i2c-15/new_device',
'echo lm75 0x49 > /sys/bus/i2c/devices/i2c-15/new_device',
'echo lm75 0x4a > /sys/bus/i2c/devices/i2c-15/new_device',
'echo lm75 0x4b > /sys/bus/i2c/devices/i2c-15/new_device',
'echo as7326_56x_psu1 0x51 > /sys/bus/i2c/devices/i2c-17/new_device',
'echo ym2651 0x59 > /sys/bus/i2c/devices/i2c-17/new_device',
'echo as7326_56x_psu2 0x53 > /sys/bus/i2c/devices/i2c-13/new_device',
'echo ym2651 0x5b > /sys/bus/i2c/devices/i2c-13/new_device',
'echo as7326_56x_cpld1 0x60 > /sys/bus/i2c/devices/i2c-18/new_device',
'echo as7326_56x_cpld2 0x62 > /sys/bus/i2c/devices/i2c-12/new_device',
'echo as7326_56x_cpld3 0x64 > /sys/bus/i2c/devices/i2c-19/new_device']

mknod2 =[
]

#EERPOM
eeprom_mknod =[
'echo 24c04 0x56 > /sys/bus/i2c/devices/i2c-0/new_device',
'echo 24c02 0x57 > /sys/bus/i2c/devices/i2c-0/new_device'
]


def i2c_order_check():
    # This project has only 1 i2c bus.
    return 0

def eeprom_check():
    cmd = ["i2cget", "-y", "-f", "0", "0x56"]
    status, output = getstatusoutput_noshell(cmd)
    return status

def device_install():
    global FORCE

    order = i2c_order_check()

    # if 0x70 is not exist @i2c-1, use reversed bus order
    if order:
        for i in range(0,len(mknod2)):
            #for pca954x need times to built new i2c buses
            if mknod2[i].find('pca954') != -1:
               time.sleep(1)

            status, output = log_os_system(mknod2[i], 1)
            if status:
                print(output)
                if FORCE == 0:
                    return status
    else:
        for i in range(0,len(mknod)):
            #for pca954x need times to built new i2c buses
            if mknod[i].find('pca954') != -1:
               time.sleep(1)

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
    
    # initiate IDPROM
    # Close 0x77 mux to make sure if the I2C address of IDPROM is 0x56 or 0x57
    log_os_system("i2cset -f -y 0 0x77 0 ", 1)
    ret=eeprom_check()
    if ret==0:
        log_os_system(eeprom_mknod[0], 1) #old board, 0x56 eeprom
        time.sleep(0.2)
        exists = os.path.isfile('/sys/bus/i2c/devices/0-0056/eeprom')
        if (exists is False):
            subprocess.call('echo 0x56 > /sys/bus/i2c/devices/i2c-0/delete_device', shell=True)
            log_os_system(eeprom_mknod[1], 1)
    else:
        log_os_system(eeprom_mknod[1], 1) #new board, 0x57 eeprom                
                    
                    
    for i in range(0,len(sfp_map)):
        if i < qsfp_start or i >= qsfp_end:
            status, output =log_os_system("echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-"+str(sfp_map[i])+"/new_device", 1)
        else:
            status, output =log_os_system("echo optoe1 0x50 > /sys/bus/i2c/devices/i2c-"+str(sfp_map[i])+"/new_device", 1)
        if status:
            print(output)
            if FORCE == 0:
                return status
    print("Done device_install")
    return

def device_uninstall():
    global FORCE

    status, output =log_os_system("ls /sys/bus/i2c/devices/1-0076", 0)
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

    if I2C_ORDER==0:
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

    #Deal with for del 0x56 or 0x57 sysfs device    
    exists = os.path.isfile('/sys/bus/i2c/devices/0-0056/eeprom')
        
    if (exists is True):
        target = eeprom_mknod[0] #0x56
    else:
        target = eeprom_mknod[1] #0x57
    
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

def do_install():
    print("Checking system....")
    if driver_check() == False:
        print("No driver, installing....")
        status = driver_install()
        if status:
            if FORCE == 0:
                return  status
    else:
        print(PROJECT_NAME.upper()+" drivers detected....")

    ir3570_check()

    if not device_exist():
        print("No device, installing....")
        status = device_install()
        if status:
            if FORCE == 0:
                return  status
    else:
        print(PROJECT_NAME.upper()+" devices detected....")

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

    if driver_check()== False :
        print(PROJECT_NAME.upper() +" has no driver installed....")
    else:
        print("Removing installed driver....")
        status = driver_uninstall()
        if status:
            if FORCE == 0:
                return  status

    do_sonic_platform_clean()

    return

def device_exist():
    ret1, log = log_os_system("ls "+i2c_prefix+"*0070", 0)
    ret2, log = log_os_system("ls "+i2c_prefix+"i2c-2", 0)
    return not(ret1 or ret2)

if __name__ == "__main__":
    main()
