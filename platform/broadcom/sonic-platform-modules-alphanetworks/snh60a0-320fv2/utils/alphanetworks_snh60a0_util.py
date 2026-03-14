#!/usr/bin/env python
#
# Copyright (C) 2018 Alphanetworks Technology Corporation.
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

import os
import commands
import sys, getopt
import logging
import re
import time
from collections import namedtuple




PROJECT_NAME = 'snh60a0-320fv2'
device_path = "x86_64-alphanetworks_snh60a0_320fv2-r0"
version = '0.1.0'
verbose = False
DEBUG = False
args = []
FORCE = 0


if DEBUG == True:
    print sys.argv[0]
    print 'ARGV      :', sys.argv[1:]   


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
           do_install()
        elif arg == 'clean':
           do_uninstall()
        else:
            show_help()
            
            
    return 0              
        
def show_help():
    print __doc__ % {'scriptName' : sys.argv[0].split("/")[-1]}
    sys.exit(0)

def show_log(txt):
    if DEBUG == True:
        print PROJECT_NAME.upper()+": "+txt
    return

def log_os_system(cmd, show):
    logging.info('Run :'+cmd)  
    status, output = commands.getstatusoutput(cmd)
    show_log (cmd +" with result: " + str(status))
    show_log ("      output:"+output)    
    if status:
        logging.info('Failed :'+cmd)
        if show:
            print('Failed :'+cmd)
    return  status, output
            
def driver_check():
    ret, lsmod = log_os_system("lsmod | grep alphanetworks", 0)
    logging.info('mods:'+lsmod)
    if len(lsmod) ==0:
        return False   
    return True



kos = [
'modprobe i2c_dev',
'modprobe i2c_mux_pca954x',
'modprobe optoe'  ,
'modprobe snh60a0_power_cpld'  ,
'modprobe snh60a0_system_cpld' ,
'modprobe snh60a0_onie_eeprom' ,
'modprobe alphanetworks_snh60a0_320fv2_sfp' ]

def driver_install():
    global FORCE
    status, output = log_os_system("depmod", 1)
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
                   
sfp_map =  [14,15,16,17]
mknod =[
'echo pca9548 0x70 > /sys/bus/i2c/devices/i2c-0/new_device',
'echo pca9545 0x71 > /sys/bus/i2c/devices/i2c-6/new_device',
'echo pca9548 0x72 > /sys/bus/i2c/devices/i2c-8/new_device',
'echo pca9545 0x71 > /sys/bus/i2c/devices/i2c-7/new_device',
'echo pca9548 0x73 > /sys/bus/i2c/devices/i2c-21/new_device',
'echo 24c02 0x51 > /sys/bus/i2c/devices/i2c-11/new_device',
'echo 24c02 0x51 > /sys/bus/i2c/devices/i2c-12/new_device',
'echo snh60a0_onie_eeprom 0x56 > /sys/bus/i2c/devices/i2c-0/new_device',
'echo snh60a0_power_cpld 0x5e > /sys/bus/i2c/devices/i2c-0/new_device',
'echo snh60a0_system_cpld 0x5f > /sys/bus/i2c/devices/i2c-9/new_device',
'echo lm75 0x4D > /sys/bus/i2c/devices/i2c-4/new_device',
'echo lm75 0x4C > /sys/bus/i2c/devices/i2c-5/new_device',
'echo lm75 0x4F > /sys/bus/i2c/devices/i2c-0/new_device' ]

mknod2 =[
'echo pca9548 0x70 > /sys/bus/i2c/devices/i2c-1/new_device',
'echo pca9545 0x71 > /sys/bus/i2c/devices/i2c-6/new_device',
'echo pca9548 0x72 > /sys/bus/i2c/devices/i2c-8/new_device',
'echo pca9545 0x71 > /sys/bus/i2c/devices/i2c-7/new_device',
'echo pca9548 0x73 > /sys/bus/i2c/devices/i2c-21/new_device',
'echo 24c02 0x51 > /sys/bus/i2c/devices/i2c-11/new_device',
'echo 24c02 0x51 > /sys/bus/i2c/devices/i2c-12/new_device',
'echo snh60a0_onie_eeprom 0x56 > /sys/bus/i2c/devices/i2c-1/new_device',
'echo snh60a0_power_cpld 0x5e > /sys/bus/i2c/devices/i2c-1/new_device',
'echo snh60a0_system_cpld 0x5f > /sys/bus/i2c/devices/i2c-9/new_device',
'echo lm75 0x4D > /sys/bus/i2c/devices/i2c-4/new_device',
'echo lm75 0x4C > /sys/bus/i2c/devices/i2c-5/new_device',
'echo lm75 0x4F > /sys/bus/i2c/devices/i2c-1/new_device' ]

       
       
def i2c_order_check():    
    # i2c bus 0 and 1 might be installed in different order.
    # Here check if 0x76 is exist @ i2c-0

    status, output = log_os_system("i2cdetect -l | grep I801 | grep i2c-0", 0)
    if  not output:
        order = 1
    else:
        order = 0

    if not device_exist():
        #order = 1
        tmp = "sed -i 's/0-/1-/g' /usr/share/sonic/device/"+device_path+"/fancontrol"
        status, output = log_os_system(tmp, 0)
        tmp = "sed -i 's/0-/1-/g' /usr/share/sonic/device/"+device_path+"/plugins/led_control.py"
        status, output = log_os_system(tmp, 0)
    else:
        #order = 0
        tmp = "sed -i 's/1-/0-/g' /usr/share/sonic/device/"+device_path+"/fancontrol"
        status, output = log_os_system(tmp, 0)
        tmp = "sed -i 's/1-/0-/g' /usr/share/sonic/device/"+device_path+"/plugins/led_control.py"
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
               
            if mknod2[i].find('lm75') != -1:
               time.sleep(1)
         
            status, output = log_os_system(mknod2[i], 1)
            if status:
                print output
                if FORCE == 0:
                    return status  
    else:
        for i in range(0,len(mknod)):
            #for pca954x need times to built new i2c buses            
            if mknod[i].find('pca954') != -1:
               time.sleep(1)         
               
            if mknod[i].find('lm75') != -1:
               time.sleep(1)
         
            status, output = log_os_system(mknod[i], 1)
            if status:
                print output
                if FORCE == 0:                
                    return status  

    status, output =log_os_system("echo 0 > /sys/bus/i2c/devices/9-005f/sys_reset1", 1)
    if status:
        print output
        if FORCE == 0:
            return status

    for i in range(0, 32):
        index = i / 8
        port = i % 8
        reg_sfp = 0
        if port == 0:
            reg_sfp = 1

        if reg_sfp == 1:
            status, output =log_os_system("echo sfpcpld"+str(i+1)+" 0x5f > /sys/bus/i2c/devices/i2c-"+str(sfp_map[index])+"/new_device", 1)
            if status:
                print output
                if FORCE == 0:
                    return status
            status, output =log_os_system("echo sfpcpld"+str(i+1)+" 0x50 > /sys/bus/i2c/devices/i2c-"+str(sfp_map[index])+"/new_device", 1)
            if status:
                print output
                if FORCE == 0:
                    return status

    return 
    
def device_uninstall():
    global FORCE
    
    status, output = log_os_system("i2cdetect -l | grep I801 | grep i2c-0", 0)

    if not output:
        I2C_ORDER=1
    else:
        I2C_ORDER=0                    

    for i in range(0, 32):
        index = i / 8
        port = i % 8
        reg_sfp = 0
        if port == 0:
            reg_sfp = 1

        if reg_sfp == 1:
            target = "/sys/bus/i2c/devices/i2c-"+str(sfp_map[index])+"/delete_device"
            status, output =log_os_system("echo 0x5f > "+ target, 1)
            if status:
                print output
                if FORCE == 0:            
                    return status
            target = "/sys/bus/i2c/devices/i2c-"+str(sfp_map[index])+"/delete_device"
            status, output =log_os_system("echo 0x50 > "+ target, 1)
            if status:
                print output
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
            print output
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
    tmp = "find /var/lib/docker/overlay -iname fancontrol | grep usr/sbin/fancontrol | xargs cat | sed '429d' | sed '428a if \[ $? -ne 1 \]' | sed '425d' | sed '424a return' > /tmp/tmp_fancontrol"
    status, output = log_os_system(tmp, 1)
    tmp = "fancontrol_tmp=`find /var/lib/docker/overlay -iname fancontrol | grep usr/sbin/fancontrol` ; cp /tmp/tmp_fancontrol $fancontrol_tmp"
    status, output = log_os_system(tmp, 1)
    print "Checking system...."
    if driver_check() == False:
        print "No driver, installing...."    
        status = driver_install()
        if status:
            if FORCE == 0:        
                return  status
    else:
        print PROJECT_NAME.upper()+" drivers detected...."                      
    if not device_exist():
        print "No device, installing...."     
        status = device_install() 
        if status:
            if FORCE == 0:        
                return  status        
    else:
        print PROJECT_NAME.upper()+" devices detected...."           
    return
    
def do_uninstall():
    print "Checking system...."
    if not device_exist():
        print PROJECT_NAME.upper() +" has no device installed...."         
    else:
        print "Removing device...."     
        status = device_uninstall() 
        if status:
            if FORCE == 0:            
                return  status  
                
    if driver_check()== False :
        print PROJECT_NAME.upper() +" has no driver installed...."
    else:
        print "Removing installed driver...."
        status = driver_uninstall()
        if status:
            if FORCE == 0:        
                return  status                          
                    
    return       

def device_exist():
    ret1, log = log_os_system("ls "+i2c_prefix+"*0070", 0)
    ret2, log = log_os_system("ls "+i2c_prefix+"i2c-2", 0)
    return not(ret1 or ret2)

if __name__ == "__main__":
    main()
