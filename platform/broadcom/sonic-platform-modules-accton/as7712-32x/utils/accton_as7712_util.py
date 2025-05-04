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
import time

PROJECT_NAME = 'as7712_32x'
version = '0.1.0'
verbose = False
DEBUG = False
args = []
ALL_DEVICE = {}

i2c_prefix = '/sys/bus/i2c/devices/'
'''
i2c_bus = {'fan': ['2-0066']                 ,
           'thermal': ['3-0048','3-0049', '3-004a', '3-004b'] ,
           'psu': ['10-0050','11-0053'], 
           'sfp': ['-0050']}
i2c_nodes = {'fan': ['present', 'front_speed_rpm', 'rear_speed_rpm'] ,
           'thermal': ['hwmon/hwmon*/temp1_input'] ,
           'psu': ['psu_present ', 'psu_power_good']    ,
           'sfp': ['module_present', 'sfp_tx_disable_all']}
'''
                  
sfp_map =  [22,23,24,25,27,26,29,28,
                 18,19,20,21,30,31,32,33,
                 34,35,36,37,46,47,48,49,
                 38,39,40,41,42,43,44,45]
mknod =[                 
'echo pca9548 0x76 > /sys/bus/i2c/devices/i2c-0/new_device',
'echo pca9548 0x71 > /sys/bus/i2c/devices/i2c-1/new_device' ,
'echo pca9548 0x72 > /sys/bus/i2c/devices/i2c-1/new_device' ,
'echo pca9548 0x73 > /sys/bus/i2c/devices/i2c-1/new_device' ,
'echo pca9548 0x74 > /sys/bus/i2c/devices/i2c-1/new_device',
'echo pca9548 0x75 > /sys/bus/i2c/devices/i2c-1/new_device',
'echo 24c02 0x57 > /sys/bus/i2c/devices/i2c-1/new_device',

'echo as7712_32x_fan 0x66 > /sys/bus/i2c/devices/i2c-2/new_device ',
'echo lm75 0x48 > /sys/bus/i2c/devices/i2c-3/new_device',
'echo lm75 0x49 > /sys/bus/i2c/devices/i2c-3/new_device',
'echo lm75 0x4a > /sys/bus/i2c/devices/i2c-3/new_device',
'echo lm75 0x4b > /sys/bus/i2c/devices/i2c-3/new_device',
'echo as7712_32x_psu1 0x53 > /sys/bus/i2c/devices/i2c-11/new_device',
'echo ym2651 0x5b > /sys/bus/i2c/devices/i2c-11/new_device',
'echo as7712_32x_psu2 0x50 > /sys/bus/i2c/devices/i2c-10/new_device',
'echo ym2651 0x58 > /sys/bus/i2c/devices/i2c-10/new_device',
'echo cpld_as7712 0x60 > /sys/bus/i2c/devices/i2c-4/new_device',
'echo cpld_plain 0x62 > /sys/bus/i2c/devices/i2c-5/new_device',
'echo cpld_plain 0x64 > /sys/bus/i2c/devices/i2c-6/new_device']

mknod2 =[                 
'echo pca9548 0x76 > /sys/bus/i2c/devices/i2c-1/new_device',
'echo pca9548 0x71 > /sys/bus/i2c/devices/i2c-0/new_device' ,
'echo pca9548 0x72 > /sys/bus/i2c/devices/i2c-0/new_device' ,
'echo pca9548 0X73 > /sys/bus/i2c/devices/i2c-0/new_device' ,
'echo pca9548 0x74 > /sys/bus/i2c/devices/i2c-0/new_device',
'echo pca9548 0x75 > /sys/bus/i2c/devices/i2c-0/new_device',
'echo 24c02 0x57 > /sys/bus/i2c/devices/i2c-0/new_device',

'echo as7712_32x_fan 0x66 > /sys/bus/i2c/devices/i2c-2/new_device ',
'echo lm75 0x48 > /sys/bus/i2c/devices/i2c-3/new_device',
'echo lm75 0x49 > /sys/bus/i2c/devices/i2c-3/new_device',
'echo lm75 0x4a > /sys/bus/i2c/devices/i2c-3/new_device',
'echo lm75 0x4b > /sys/bus/i2c/devices/i2c-3/new_device',
'echo as7712_32x_psu1 0x53 > /sys/bus/i2c/devices/i2c-11/new_device',
'echo ym2651 0x5b > /sys/bus/i2c/devices/i2c-11/new_device',
'echo as7712_32x_psu2 0x50 > /sys/bus/i2c/devices/i2c-10/new_device',
'echo ym2651 0x58 > /sys/bus/i2c/devices/i2c-10/new_device',
'echo cpld_as7712 0x60 > /sys/bus/i2c/devices/i2c-4/new_device',
'echo cpld_plain 0x62 > /sys/bus/i2c/devices/i2c-5/new_device',
'echo cpld_plain 0x64 > /sys/bus/i2c/devices/i2c-6/new_device']

FORCE = 0
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
        else:
            show_help()
            
    return 0              
        
def show_help():
    print(__doc__ % {'scriptName' : sys.argv[0].split("/")[-1]})
    sys.exit(0)

        
            
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
'modprobe i2c_mux_pca954x',
'modprobe accton_i2c_cpld'  ,
'modprobe ym2651y'                  ,
'modprobe accton_as7712_32x_fan'     ,
'modprobe optoe'      ,
'modprobe leds-accton_as7712_32x'      ,
'modprobe accton_as7712_32x_psu' ]

def driver_install():
    global FORCE
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


       
       
def i2c_order_check():    
    # i2c bus 0 and 1 might be installed in different order.
    # Here check if 0x76 is exist @ i2c-0
    tmp = "echo pca9548 0x76 > /sys/bus/i2c/devices/i2c-0/new_device"
    status, output = log_os_system(tmp, 0)
    if not device_exist():
        order = 1
    else:
        order = 0
    tmp = "echo 0x76 > /sys/bus/i2c/devices/i2c-0/delete_device"       
    status, output = log_os_system(tmp, 0)       
    return order
                     
def device_install():
    global FORCE
    
    order = i2c_order_check()
                
    # if 0x76 is not exist @i2c-0, use reversed bus order    
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
    for i in range(0,len(sfp_map)):
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
    ret1, log = log_os_system("ls "+i2c_prefix+"*0076", 0)
    ret2, log = log_os_system("ls "+i2c_prefix+"i2c-2", 0)
    return not(ret1 or ret2)

if __name__ == "__main__":
    main()
