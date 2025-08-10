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



PROJECT_NAME = 'as7816_64x'
version = '0.1.0'
verbose = False
DEBUG = False
args = []
ALL_DEVICE = {}               

FORCE = 0
#logging.basicConfig(filename= PROJECT_NAME+'.log', filemode='w',level=logging.DEBUG)
#logging.basicConfig(level=logging.INFO)


if DEBUG == True:
    print(sys.argv[0])
    print('ARGV      :', sys.argv[1:]   )


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
    print( __doc__ % {'scriptName' : sys.argv[0].split("/")[-1]})
    sys.exit(0)

   
def dis_i2c_ir3570a(addr):
    cmd = "i2cset -y 0 0x%x 0xE5 0x01" % addr
    status, output = subprocess.getstatusoutput(cmd)
    cmd = "i2cset -y 0 0x%x 0x12 0x02" % addr
    status, output = subprocess.getstatusoutput(cmd)
    return status

def ir3570_check():
    cmd = "i2cdump -y 0 0x42 s 0x9a"
    try:
        status, output = subprocess.getstatusoutput(cmd)
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
'modprobe i2c_mux_pca954x force_deselect_on_exit=1',
'modprobe optoe',
'modprobe accton_i2c_cpld'  ,
'modprobe ym2651y'                  ,
'modprobe x86-64-accton-as7816-64x-fan'     ,
#'modprobe x86-64-accton-as7816-64x-sfp'      ,
'modprobe x86-64-accton-as7816-64x-leds'      ,
'modprobe x86-64-accton-as7816-64x-psu' ]

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


i2c_prefix = '/sys/bus/i2c/devices/'
'''
i2c_bus = {'fan': ['17-0068']                 ,
           'thermal': ['18-0048','18-0049', '18-004a' , '18-004b', '17-004d', '17-004e'] ,
           'psu': ['10-0053','9-0050'], 
           'sfp': ['-0050']}
i2c_nodes = {'fan': ['present', 'front_speed_rpm', 'rear_speed_rpm'] ,
           'thermal': ['hwmon/hwmon*/temp1_input'] ,
           'psu': ['psu_present', 'psu_power_good']    ,
           'sfp': ['module_present']}
'''                  

sfp_map =  [37,38,39,40,42,41,44,43,33,34,35,36,45,46,47,48,49,50,51,52,
           61,62,63,64,53,54,55,56,57,58,59,60,69,70,71,72,77,78,79,80,65,
	   66,67,68,73,74,75,76,85,86,87,88,31,32,29,30,81,82,83,84,25,26,
           27,28]


mknod =[   
'echo pca9548  0x77 > /sys/bus/i2c/devices/i2c-0/new_device',
'echo pca9548  0x71 > /sys/bus/i2c/devices/i2c-1/new_device',
'echo pca9548  0x76 > /sys/bus/i2c/devices/i2c-1/new_device',
'echo pca9548  0x73 > /sys/bus/i2c/devices/i2c-1/new_device',
'echo pca9548  0x70 > /sys/bus/i2c/devices/i2c-2/new_device',
'echo pca9548  0x71 > /sys/bus/i2c/devices/i2c-2/new_device',
'echo pca9548  0x72 > /sys/bus/i2c/devices/i2c-2/new_device',
'echo pca9548  0x73 > /sys/bus/i2c/devices/i2c-2/new_device',
'echo pca9548  0x74 > /sys/bus/i2c/devices/i2c-2/new_device',
'echo pca9548  0x75 > /sys/bus/i2c/devices/i2c-2/new_device',
'echo pca9548  0x76 > /sys/bus/i2c/devices/i2c-2/new_device',
'echo 24c02  0x56 > /sys/bus/i2c/devices/i2c-0/new_device',
'echo as7816_64x_psu1  0x53 > /sys/bus/i2c/devices/i2c-10/new_device',
'echo ym2851  0x5b > /sys/bus/i2c/devices/i2c-10/new_device',
'echo as7816_64x_psu2  0x50 > /sys/bus/i2c/devices/i2c-9/new_device',
'echo ym2851  0x58 > /sys/bus/i2c/devices/i2c-9/new_device',
'echo as7816_64x_fan  0x68 > /sys/bus/i2c/devices/i2c-17/new_device',
'echo lm75  0x48 > /sys/bus/i2c/devices/i2c-18/new_device',
'echo lm75  0x49 > /sys/bus/i2c/devices/i2c-18/new_device',
'echo lm75  0x4a > /sys/bus/i2c/devices/i2c-18/new_device',
'echo lm75  0x4b > /sys/bus/i2c/devices/i2c-18/new_device',
'echo lm75  0x4d > /sys/bus/i2c/devices/i2c-17/new_device',
'echo lm75  0x4e > /sys/bus/i2c/devices/i2c-17/new_device',
'echo cpld_as7816  0x60 > /sys/bus/i2c/devices/i2c-19/new_device',
'echo cpld_plain  0x62 > /sys/bus/i2c/devices/i2c-20/new_device',
'echo cpld_plain  0x64 > /sys/bus/i2c/devices/i2c-21/new_device',
'echo cpld_plain  0x66 > /sys/bus/i2c/devices/i2c-22/new_device']
       
def i2c_order_check():    
    return 0
                     
def device_install():
    global FORCE
    
    for i in range(0,len(mknod)):
        #for pca954x need times to built new i2c buses            
        if mknod[i].find('pca954') != -1:
           time.sleep(1)         
           
        status, output = log_os_system(mknod[i], 1)
        if status:
            print(output)
            if FORCE == 0:                
                return status  

    for i in range(0,len(sfp_map)):
        path = "/sys/bus/i2c/devices/i2c-"+str(sfp_map[i])+"/new_device"
        status, output =log_os_system("echo optoe1 0x50 > " + path, 1)
        if status:
            print(output)
            if FORCE == 0:            
                return status                                  
     
    print("Done device_install")
     
    return 
    
def device_uninstall():
    global FORCE
    
    status, output =log_os_system("ls /sys/bus/i2c/devices/1-0076", 0)
    
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
    ret1, log = log_os_system("ls "+i2c_prefix+"*0076", 0)
    ret2, log = log_os_system("ls "+i2c_prefix+"i2c-2", 0)
    return not(ret1 or ret2)

if __name__ == "__main__":
    main()
