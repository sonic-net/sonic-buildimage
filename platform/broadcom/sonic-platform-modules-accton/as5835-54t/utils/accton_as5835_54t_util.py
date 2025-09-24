#!/usr/bin/env python
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
"""

import subprocess
import getopt
import sys
import logging
import time

PROJECT_NAME = 'as5835_54t'
version = '0.1.0'
verbose = False
DEBUG = False
args = []
ALL_DEVICE = {}               
DEVICE_NO = {'led':5, 'fan':5,'thermal':4, 'psu':2, 'sfp':6}
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
        elif arg == 'clean':
           do_uninstall()
        else:
            show_help()
            
            
    return 0              
        
def show_help():
    print(__doc__ % {'scriptName' : sys.argv[0].split("/")[-1]})
    sys.exit(0)

def my_log(txt):
    if DEBUG == True:
        print("[Debug]"+txt)    
    return
    
def log_os_system(cmd, show):
    logging.info('Run :'+cmd)  
    status, output = subprocess.getstatusoutput(cmd)    
    my_log (cmd +"with result:" + str(status))
    my_log ("      output:"+output)    
    if status:
        logging.info('Failed :'+cmd)
        if show:
            print(('Failed :'+cmd))
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
'modprobe i2c_i801',
'modprobe i2c_ismt',
'modprobe i2c_mux_pca954x force_deselect_on_exit=1',
'modprobe accton_as5835_54t_cpld'  ,
'modprobe ym2651y'                  ,
'modprobe accton_as5835_54t_fan'     ,
'modprobe optoe'      ,
'modprobe accton_as5835_54t_leds'      ,
'modprobe accton_as5835_54t_psu' ]

def driver_install():
    global FORCE
    status, output = log_os_system("depmod", 1)
    for i in range(0,len(kos)):
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
i2c_bus = {'fan': ['3-0063']                 ,
           'thermal': ['18-004b','19-004c', '20-0049', '21-004a'] ,
           'psu': ['11-0050','12-0053'], 
           'sfp': ['-0050']}
i2c_nodes = {'fan': ['present', 'front_speed_rpm', 'rear_speed_rpm'] ,
           'thermal': ['hwmon/hwmon*/temp1_input'] ,
           'psu': ['psu_present ', 'psu_power_good']    ,
           'sfp': ['module_present_', 'module_tx_disable_']}
                   
sfp_map =  [28,29,26,30,31,27]

qsfp_start = 48

mknod =[                 
'echo pca9548 0x77 > /sys/bus/i2c/devices/i2c-1/new_device',
'echo pca9548 0x70 > /sys/bus/i2c/devices/i2c-2/new_device' ,
'echo pca9548 0x71 > /sys/bus/i2c/devices/i2c-2/new_device' ,
'echo pca9548 0x72 > /sys/bus/i2c/devices/i2c-2/new_device' ,
'echo 24c02 0x57 > /sys/bus/i2c/devices/i2c-1/new_device',

'echo as5835_54t_fan 0x63 > /sys/bus/i2c/devices/i2c-3/new_device',
'echo lm75 0x4b > /sys/bus/i2c/devices/i2c-18/new_device',
'echo lm75 0x4c > /sys/bus/i2c/devices/i2c-19/new_device',
'echo lm75 0x49 > /sys/bus/i2c/devices/i2c-20/new_device',
'echo lm75 0x4a > /sys/bus/i2c/devices/i2c-21/new_device',
'echo as5835_54t_psu1 0x50 > /sys/bus/i2c/devices/i2c-11/new_device',
'echo ym2401 0x58 > /sys/bus/i2c/devices/i2c-11/new_device',
'echo as5835_54t_psu2 0x53 > /sys/bus/i2c/devices/i2c-12/new_device',
'echo ym2401 0x5b > /sys/bus/i2c/devices/i2c-12/new_device',
'echo as5835_54t_cpld1 0x60 > /sys/bus/i2c/devices/i2c-3/new_device',
'echo as5835_54t_cpld2 0x61 > /sys/bus/i2c/devices/i2c-3/new_device',
'echo as5835_54t_cpld3 0x62 > /sys/bus/i2c/devices/i2c-3/new_device']

mknod2 =[                 
'echo pca9548 0x77 > /sys/bus/i2c/devices/i2c-0/new_device',
'echo pca9548 0x70 > /sys/bus/i2c/devices/i2c-2/new_device' ,
'echo pca9548 0x71 > /sys/bus/i2c/devices/i2c-2/new_device' ,
'echo pca9548 0x72 > /sys/bus/i2c/devices/i2c-2/new_device' ,
'echo 24c02 0x57 > /sys/bus/i2c/devices/i2c-0/new_device',

'echo as5835_54t_fan 0x63 > /sys/bus/i2c/devices/i2c-3/new_device',
'echo lm75 0x4b > /sys/bus/i2c/devices/i2c-18/new_device',
'echo lm75 0x4c > /sys/bus/i2c/devices/i2c-19/new_device',
'echo lm75 0x49 > /sys/bus/i2c/devices/i2c-20/new_device',
'echo lm75 0x4a > /sys/bus/i2c/devices/i2c-21/new_device',
'echo as5835_54t_psu1 0x50 > /sys/bus/i2c/devices/i2c-11/new_device',
'echo ym2401 0x58 > /sys/bus/i2c/devices/i2c-11/new_device',
'echo as5835_54t_psu2 0x53 > /sys/bus/i2c/devices/i2c-12/new_device',
'echo ym2401 0x5b > /sys/bus/i2c/devices/i2c-12/new_device',
'echo as5835_54t_cpld1 0x60 > /sys/bus/i2c/devices/i2c-3/new_device',
'echo as5835_54t_cpld2 0x61 > /sys/bus/i2c/devices/i2c-3/new_device',
'echo as5835_54t_cpld3 0x62 > /sys/bus/i2c/devices/i2c-3/new_device']
      
       
def i2c_order_check():    
    # i2c bus 0 and 1 might be installed in different order.
    # Here check if 0x77 is exist @ i2c-1
    tmp = "echo pca9548 0x77 > /sys/bus/i2c/devices/i2c-1/new_device"
    status, output = log_os_system(tmp, 0)
    if not device_exist():
        order = 1
    else:
        order = 0
    tmp = "echo 0x77 > /sys/bus/i2c/devices/i2c-1/delete_device"       
    status, output = log_os_system(tmp, 0)       
    return order
                     
def device_install():
    global FORCE
    
    order = i2c_order_check()
                
    # if 0x77 is not exist @i2c-1, use reversed bus order    
    if order:       
        for i in range(0,len(mknod2)):
            #for pca954x need times to built new i2c buses            
            if mknod2[i].find('pca954') != -1:
               time.sleep(1)         
               
            status, output = log_os_system(mknod2[i], 1)
            time.sleep(0.01)
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
    for i in range(49, 55): #Set qsfp port to normal state
        log_os_system("echo 0 > /sys/bus/i2c/devices/3-0062/module_reset_" + str(i), 1)   
    
    for i in range(0,len(sfp_map)):
        status, output =log_os_system("echo optoe1 0x50 > /sys/bus/i2c/devices/i2c-"+str(sfp_map[i])+"/new_device", 1)
        if status:
            print(output)
            if FORCE == 0:            
                return status
        path = "/sys/bus/i2c/devices/{0}-0050/port_name"
        status, output =log_os_system("echo port{0} > ".format(i+49)+path.format(sfp_map[i]), 1)
        if status:
            print(output)
            if FORCE == 0:            
                return status
    return 
    
def device_uninstall():
    global FORCE
    
    status, output =log_os_system("ls /sys/bus/i2c/devices/0-0077", 0)
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
        print(PROJECT_NAME.upper()+" drivers detected....")                      
    if not device_exist():
        print("No device, installing....")     
        status = device_install() 
        if status:
            if FORCE == 0:        
                return  status        
    else:
        print(PROJECT_NAME.upper()+" devices detected....")           
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
                    
    return       

def device_exist():
    ret1, log = log_os_system("ls "+i2c_prefix+"*0077", 0)
    ret2, log = log_os_system("ls "+i2c_prefix+"i2c-2", 0)
    return not(ret1 or ret2)

if __name__ == "__main__":
    main()
