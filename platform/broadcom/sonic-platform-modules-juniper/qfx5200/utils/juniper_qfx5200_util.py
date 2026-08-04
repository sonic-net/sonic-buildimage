#!/usr/bin/env python
#
# Description: This file contains the Juniper QFX5200 Platform Initialization routines
#
# Copyright (c) 2020, Juniper Networks, Inc.
# All rights reserved.
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

import os
import commands
import sys
import logging
import time
import subprocess
from sonic_py_common.general import getstatusoutput_noshell

PROJECT_NAME = 'QFX5200-32C'
verbose = False
DEBUG = False
FORCE = 0

if DEBUG == True:
    print(sys.argv[0])
    print('ARGV      :', sys.argv[1:])   

i2c_prefix = '/sys/bus/i2c/devices/'

kos = [
'modprobe i2c-mux',
'modprobe mfd-core',
'modprobe tmp401',
'modprobe jnx-tmc-core',
'modprobe leds-jnx-tmc',
'modprobe jnx-refpga-tmc',
'modprobe i2c-tmc',
'modprobe gpio-tmc',
'modprobe jnx-tmc-psu',
'modprobe jnx-psu-monitor',
'modprobe jnx-refpga-lpcm',
'modprobe adt7470'
]

mknod =[   
'echo tmp435  0x48 > /sys/bus/i2c/devices/i2c-5/new_device',
'echo tmp435  0x49 > /sys/bus/i2c/devices/i2c-5/new_device',
'echo tmp435  0x4A > /sys/bus/i2c/devices/i2c-5/new_device',
'echo tmp435  0x4B > /sys/bus/i2c/devices/i2c-5/new_device',
'echo tmp435  0x48 > /sys/bus/i2c/devices/i2c-6/new_device',
'echo tmp435  0x49 > /sys/bus/i2c/devices/i2c-6/new_device',
'echo tmp435  0x4A > /sys/bus/i2c/devices/i2c-6/new_device',
'echo tmp435  0x4B > /sys/bus/i2c/devices/i2c-6/new_device',
'echo tmp435  0x48 > /sys/bus/i2c/devices/i2c-7/new_device',
'echo tmp435  0x49 > /sys/bus/i2c/devices/i2c-7/new_device',
'echo adt7470 0x2C > /sys/bus/i2c/devices/i2c-7/new_device',
'echo adt7470 0x2E > /sys/bus/i2c/devices/i2c-7/new_device',
'echo adt7470 0x2F > /sys/bus/i2c/devices/i2c-7/new_device',
'echo jpsu    0x58 > /sys/bus/i2c/devices/i2c-3/new_device',
'echo jpsu    0x58 > /sys/bus/i2c/devices/i2c-4/new_device',
] 

def my_log(txt):
    if DEBUG == True:
        print(txt)
    return
    
def log_os_system(cmd, show):
    logging.info('Run :'+cmd)  
    status, output = commands.getstatusoutput(cmd)    
    my_log (cmd +"with result:" + str(status))
    my_log ("      output:"+output)    
    if status:
        logging.info('Failed :'+cmd)
        if show:
            print('Failed :'+cmd)
    return  status, output

def write_file(text, file):
    with open(file, 'w') as f:
        f.write(text + '\n')
            
def driver_install():
    global FORCE
    log_os_system("depmod", 1)
    for i in range(0,len(kos)):
        status, output = log_os_system(kos[i], 1)
        time.sleep(2)
        if status:
            if FORCE == 0:        
                return status              
    return 0

def device_exist():
    ret1, log = log_os_system("ls "+i2c_prefix+"5-0049", 0)
    ret2, log = log_os_system("ls "+i2c_prefix+"7-002c", 0)

    return not(ret1 or ret2)

def device_install():
    global FORCE
    for i in range(0,len(mknod)):
        status, output = log_os_system(mknod[i], 1)
        if status:
            print(output)
            if FORCE == 0:
                return status

def do_install():
    status = driver_install()
    if status:
        if FORCE == 0:        
            return  status
    
    if not device_exist():
        logging.info('No device, installing....')     
        status = device_install() 
        if status:
            if FORCE == 0:        
                return  status        
    else:
        print(PROJECT_NAME.upper()+" devices detected....")
    return
    
def main():

    hwmon_input_node_mapping = ['2c','2e','2f']
    PWM1FREQ_PATH = '/sys/bus/i2c/devices/7-00{0}/hwmon/{1}/pwm1_freq'
    NUMSENSORS_PATH = '/sys/bus/i2c/devices/7-00{0}/hwmon/{1}/num_temp_sensors'
    HWMONINPUT_PATH = '/sys/bus/i2c/devices/7-00{0}/hwmon/'
    PWMINPUT_NUM = 3
    hwmon_input_path_mapping = {}
    pwm_input_path_mapping = {}
    numsensors_input_path_mapping = {}
    

    # Enabling REFPGA	
    EnableREFFGACmd = ['busybox', 'devmem', '0xFED50011', '8', '0x53'] 
    try:
        subprocess.call(EnableREFFGACmd)
    except OSError:
        print('Error: Execution of "%s" failed', EnableREFFGACmd)
        return False

    time.sleep(2)
    
    # Create CPU Board EEPROM device	
    CreateEEPROMdeviceCmd = '24c02 0x51'
    file = '/sys/bus/i2c/devices/i2c-0/new_device'
    try:
        write_file(CreateEEPROMdeviceCmd, file)
    except OSError:
        print('Error: Execution of "%s" failed', CreateEEPROMdeviceCmd)
        return False

    time.sleep(1)

    #Retrieve the Base MAC Address from EEPROM	
    status, macAddress = getstatusoutput_noshell(["decode-syseeprom", "-m", "0x24"])
    if status:
        print('Error: Could not retrieve BASE MAC Address from EEPROM')
        return False

    #Make eth0 interface down	
    status, eth0Down = getstatusoutput_noshell(["ifconfig", "eth0", "down"])
    if status:
        print('Error: Could not make eth0 interface down')
        return False

    #Assign BASE MAC ADDRESS retieved from CPU board EEPROM to eth0 interface	
    mac_address_prog = ["ifconfig", "eth0", "hw", "ether", str(macAddress)]

    status, MACAddressProg = getstatusoutput_noshell(mac_address_prog)
    if status:
        print('Error: Could not set up "macAddress" for eth0 interface')
        return False

    #Make eth0 interface up	
    status, eth0UP = getstatusoutput_noshell(["ifconfig", "eth0", "up"])
    if status:
        print('Error: Could not make eth0 interface up')
        return False

    # Juniper QFX5200 platform drivers install
    do_install()
    time.sleep(2)

    # Juniper SFP Intialization	
    JuniperSFPInitCmd = ['python', '/usr/share/sonic/device/x86_64-juniper_qfx5200-r0/plugins/qfx5200_sfp_init.py']
    try:
        subprocess.call(JuniperSFPInitCmd)
    except OSError:
        print('Error: Execution of "%s" failed', JuniperSFPInitCmd)
        return False

    time.sleep(1)
    # Invoking the script which retrieves the data from CPU Board and Main Board EEPROM and storing in file	
    EEPROMDataCmd = ['python', '/usr/share/sonic/device/x86_64-juniper_qfx5200-r0/plugins/qfx5200_eeprom_data.py']
    try:
        subprocess.call(EEPROMDataCmd)
    except OSError:
        print('Error: Execution of "%s" failed', EEPROMDataCmd)
        return False

    for x in range(PWMINPUT_NUM):
         hwmon_input_path_mapping[x] = HWMONINPUT_PATH.format(hwmon_input_node_mapping[x])

	 hwmon_path = os.listdir(hwmon_input_path_mapping[x])
	 hwmon_dir = ''
	 for hwmon_name in hwmon_path:
	     hwmon_dir = hwmon_name
	    
	 pwm_input_path_mapping[x] = PWM1FREQ_PATH.format(
		                                 hwmon_input_node_mapping[x],
				                 hwmon_dir)
         device_path = pwm_input_path_mapping[x]
         time.sleep(1)
         cmd = "22500"
         write_file(cmd, device_path)

	 numsensors_input_path_mapping[x] = NUMSENSORS_PATH.format(
		                                 hwmon_input_node_mapping[x],
				                 hwmon_dir)
         numsensors_path = numsensors_input_path_mapping[x]
         time.sleep(1)
         cmd = "0"
         write_file(cmd, numsensors_path)

    return True              
        
if __name__ == "__main__":
    main()
