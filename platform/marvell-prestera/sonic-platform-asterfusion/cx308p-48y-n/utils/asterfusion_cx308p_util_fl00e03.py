#!/usr/bin/env python
#
# Copyright (C) 2019 Asterfusion Networks, Inc.


"""
Usage: %(scriptName)s [options] command

options:
    -h | --help     : this help message
    -d | --debug    : run with debug mode
    -f | --force    : ignore error during installation or clean 
command:
    install     : install drivers and generate related sysfs nodes
    clean       : uninstall drivers and remove related sysfs nodes
    show        : show all systen status
"""

import os
import subprocess
import sys, getopt
import logging
import re
import time
import json

PROJECT_NAME = 'cx308p_48y_n'
verbose = False
DEBUG = False
FORCE = 0

PLATFORM_INSTALL_INFO_FILE="/etc/sonic/platform_install.json"

# default is 'i2c-1', we will choose the correct one from 'i2c-0' and 'i2c-1'.
DEFAULT_BASE_BUS = 'i2c-1'
BASE_BUS = 'i2c-1'

I2C_BASE_BUS = {
    'i2c-1':{
        'path':'/sys/bus/i2c/devices/i2c-1',
        'status':'INSTALLED'
    },
    'i2c-0':{
        'path':'/sys/bus/i2c/devices/i2c-0',
        'status':'INSTALLED'
    }
}

switch_install_order = [
'PCA9548_0x70',
'PCA9548_0x71',
'PCA9548_0x72',
'PCA9548_0x73',
'PCA9548_0x74',
'PCA9548_0x75',
'PCA9548_0x76'
]

I2C_SWITCH_LIST = {
    'PCA9548_0x70': {
        'parent':'base',
        'driver':'pca9548',
        'i2caddr': '0x70',
        'path': ' ',
        'bus_map': [0,0,0,0,0,0,0,0],
        'status':'NOTINST'
    },
    'PCA9548_0x71': {
        'parent':'base',
        'driver':'pca9548',
        'i2caddr': '0x71',
        'path': ' ',
        'bus_map': [0,0,0,0,0,0,0,0],
        'status':'NOTINST'
    },
    'PCA9548_0x72': {
        'parent':'base',
        'driver':'pca9548',
        'i2caddr': '0x72',
        'path': ' ',
        'bus_map': [0,0,0,0,0,0,0,0],
        'status':'NOTINST'
    },
    'PCA9548_0x73': {
        'parent':'base',
        'driver':'pca9548',
        'i2caddr': '0x73',
        'path': ' ',
        'bus_map': [0,0,0,0,0,0,0,0],
        'status':'NOTINST'
    },
    'PCA9548_0x74': {
        'parent':'base',
        'driver':'pca9548',
        'i2caddr': '0x74',
        'path': ' ',
        'bus_map': [0,0,0,0,0,0,0,0],
        'status':'NOTINST'
    },
    'PCA9548_0x75': {
        'parent':'base',
        'driver':'pca9548',
        'i2caddr': '0x75',
        'path': ' ',
        'bus_map': [0,0,0,0,0,0,0,0],
        'status':'NOTINST'
    },
    'PCA9548_0x76': {
        'parent':'base',
        'driver':'pca9548',
        'i2caddr': '0x76',
        'path': ' ',
        'bus_map': [0,0,0,0,0,0,0,0],
        'status':'NOTINST'
    }
}

SFP_GROUPS = {
    'SFP-G01' :{
        'number': 8,
        'parent':'PCA9548_0x70',
        'channels':[0,1,2,3,4,5,6,7],
        'driver':'optoe2',
        'i2caddr': '0x50',
        'paths': [],
        'status':'NOTINST'
    },
    'SFP-G02' :{
        'number': 8,
        'parent':'PCA9548_0x71',
        'channels':[0,1,2,3,4,5,6,7],
        'driver':'optoe2',
        'i2caddr': '0x50',
        'paths': [],
        'status':'NOTINST'
    },
    'SFP-G03' :{
        'number': 8,
        'parent':'PCA9548_0x72',
        'channels':[0,1,2,3,4,5,6,7],
        'driver':'optoe2',
        'i2caddr': '0x50',
        'paths': [],
        'status':'NOTINST'
    },
    'SFP-G04' :{
        'number': 8,
        'parent':'PCA9548_0x73',
        'channels':[0,1,2,3,4,5,6,7],
        'driver':'optoe2',
        'i2caddr': '0x50',
        'paths': [],
        'status':'NOTINST'
    },
    'SFP-G05' :{
        'number': 8,
        'parent':'PCA9548_0x74',
        'channels':[0,1,2,3,4,5,6,7],
        'driver':'optoe2',
        'i2caddr': '0x50',
        'paths': [],
        'status':'NOTINST'
    },
    'SFP-G06' :{
        'number': 8,
        'parent':'PCA9548_0x75',
        'channels':[0,1,2,3,4,5,6,7],
        'driver':'optoe2',
        'i2caddr': '0x50',
        'paths': [],
        'status':'NOTINST'
    },
    'SFP-G07' :{
        'number': 8,
        'parent':'PCA9548_0x76',
        'channels':[0,1,2,3,4,5,6,7],
        'driver':'optoe1',
        'i2caddr': '0x50',
        'paths': [],
        'status':'NOTINST'
    }
}


def show_help():
    print(__doc__ % {'scriptName': sys.argv[0].split("/")[-1]})
    sys.exit(0)

def log_os_system(cmd, show):
    logging.info('Run :' + cmd)
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)
    output, err = proc.communicate()
    status = proc.returncode
    if type(output) == bytes:
        output = output.decode()
    logging.info(cmd + " with result:" + str(status))
    logging.info("      output:" + output)
    if status:
        logging.info('Failed :' + cmd)
        if show:
            print('Failed ({}):'.format(status) + cmd)
    return status, output


i2c_prefix = '/sys/bus/i2c/devices/'
def device_exist():
    ret1, log = log_os_system("ls " + i2c_prefix + "*0070", 0)
    return not ret1

def driver_exist():
    ''' If driver exists, return True, otherwise return False'''
    retry_interval = 3
    max_retry_times = 30
    retry_times = 1
    while retry_times < max_retry_times:
        rc, result = log_os_system("lsmod | grep asterfusion", 0)
        # lsmod may return error when kernel is installing module, so we retry
        if rc == 0 and len(result) != 0:
            # driver exist, grep return 0 if a line is selected
            return True
        elif rc == 1 and len(result) == 0:
            # driver not exist, grep return 1 if selects no lines
            return False
        retry_times += 1
        time.sleep(retry_interval)
    logging.warning("driver check warning: lsmod failed for {}times!".format(max_retry_times))
    # if lsmod failed for all times, we think drivers are inexistent
    return False

kos = [
    'depmod -a',
    'modprobe i2c_dev',     # linux i2c commands e.g. i2cdetect need i2c_dev to work properly
    'modprobe i2c_mux_pca954x force_deselect_on_exit=1',
    'modprobe optoe',
    'modprobe x86-64-asterfusion-cx308p-48y-n_fl00e03',
]

def driver_install():
    global FORCE
    for i in range(0, len(kos)):
        status, output = log_os_system(kos[i], 1)
        if status:
            if FORCE == 0:
                return status
    return 0

def driver_uninstall():
    global FORCE
    for i in range(0, len(kos)):
        rm = kos[-(i + 1)].replace("modprobe", "modprobe -rq")
        rm = rm.replace("insmod", "rmmod")
        rm = rm.replace("force_deselect_on_exit=1", "")
        status, output = log_os_system(rm, 1)
        if status:
            if FORCE == 0:
                return status
    return 0

def check_base_bus():
    global I2C_SWITCH_LIST
    global SFP_GROUPS
    global BASE_BUS
    # we start check with the first i2c switch to install which on base bus
    switch = I2C_SWITCH_LIST[switch_install_order[0]]
    for bbus in I2C_BASE_BUS.keys():
        install_path = I2C_BASE_BUS[bbus]['path']
        cmd = "echo {} {} > {}/new_device".format(switch['driver'], switch['i2caddr'], install_path)
        status, output = log_os_system(cmd, 1)
        time.sleep(1)
        cmd = "ls /sys/bus/i2c/devices/{}-00{}/channel-0".format(bbus[-1],switch['i2caddr'][-2:])
        result, output = log_os_system(cmd, 1)
        #uninstall 
        cmd = "echo {} > {}/delete_device".format(switch['i2caddr'], install_path)
        status, output = log_os_system(cmd, 1)
        if result == 0:
            BASE_BUS = bbus
            break

    logging.info('Base bus is {}'.format(BASE_BUS))

    #exchange all base bus
    for dev_name in I2C_SWITCH_LIST.keys():
        if I2C_SWITCH_LIST[dev_name]['parent'] == 'base':
            I2C_SWITCH_LIST[dev_name]['parent'] = BASE_BUS
    for dev_name in SFP_GROUPS.keys():
        if SFP_GROUPS[dev_name]['parent'] == 'base':
            SFP_GROUPS[dev_name]['parent'] = BASE_BUS

def get_next_bus_num():
    num_list = []
    device_list = os.listdir("/sys/bus/i2c/devices")
    for x in device_list:
        t = re.match(r'i2c-(\d+)', x)
        if t:
            num_list.append(int(t.group(1)))
    logging.info('next_bus_id is {}'.format(max(num_list)+1))
    return max(num_list)+1

def install_i2c_switch():
    
    for switch_name in switch_install_order:
        next_bus_id = get_next_bus_num()
        switch = I2C_SWITCH_LIST[switch_name]
        if switch['parent'] in I2C_BASE_BUS:
            install_path = I2C_BASE_BUS[switch['parent']]['path']

        cmd = "echo {} {} > {}/new_device".format(switch['driver'], switch['i2caddr'], install_path)
        status, output = log_os_system(cmd, 1)
        if status != 0:
            switch['status'] = 'FAILED'
            continue
        
        if switch['parent'] in I2C_BASE_BUS:
            switch['path'] = "/sys/bus/i2c/devices/{}-00{}".format(switch['parent'][-1],switch['i2caddr'][-2:])
        
        # add delay to make sure the root switch for sfp is installed completely,
        # so we can start the installation of next switch

        #Check if bus are actually created
        for busid in range(next_bus_id,next_bus_id+8):
            if not os.path.exists("/sys/bus/i2c/devices/i2c-{}".format(busid)):
                print("Fail to create bus when install {}".format(switch_name))
                switch['status'] = 'FAILED'
                break
        else:
            # exit loop normally; not breakout
            switch['bus_map'] = list(range(next_bus_id,next_bus_id+8))
            switch['status'] = 'INSTALLED'

def uninstall_i2c_switch():
    for switch_name in reversed(switch_install_order):
        switch = I2C_SWITCH_LIST[switch_name]
        if switch['parent'] in I2C_BASE_BUS:
            uninst_path = I2C_BASE_BUS[switch['parent']]['path']
        else:
            uninst_path = I2C_SWITCH_LIST[switch['parent']]['path']
        
        # switch is not installed, skip this switch
        if switch['status'] != 'INSTALLED':
            continue
        
        # if 'parent_ch' in switch:
        #    uninst_path = uninst_path+"/channel-{}".format(switch['parent_ch'])
            
        cmd = "echo {} > {}/delete_device".format(switch['i2caddr'], uninst_path)
        status, output = log_os_system(cmd, 1)

def remove_install_status():
    if os.path.exists(PLATFORM_INSTALL_INFO_FILE):
        os.remove(PLATFORM_INSTALL_INFO_FILE)

def restore_install_status():
    output = []
    output.append(I2C_SWITCH_LIST)
    output.append(SFP_GROUPS)
    jsondata = json.dumps(output)
    with open(PLATFORM_INSTALL_INFO_FILE,'w') as fd:
        fd.write(jsondata)

def install_sfp():
    for sfp_group_name in SFP_GROUPS.keys():
        sfp_group = SFP_GROUPS[sfp_group_name]
        if sfp_group['parent'] in I2C_BASE_BUS:
            install_path = I2C_BASE_BUS[sfp_group['parent']]['path']
        else:
            install_path = I2C_SWITCH_LIST[sfp_group['parent']]['path']
        
        # parent switch is not installed, skip this sfp group
        if I2C_SWITCH_LIST[sfp_group['parent']]['status'] != 'INSTALLED':
            sfp_group['paths'] = ['n/a']*sfp_group['number']
            continue
        
        for n in range(0,sfp_group['number']):
            sfp_install_path = install_path+"/channel-{}".format(sfp_group['channels'][n])
            cmd = "echo {} {} > {}/new_device".format(sfp_group['driver'], sfp_group['i2caddr'], sfp_install_path)
            status, output = log_os_system(cmd, 1)
            if status != 0:
                sfp_group['status'] = 'FAILED'
                sfp_group['paths'].append("n/a")
                continue
                
            if sfp_group['parent'] in I2C_BASE_BUS:
                sfp_group['paths'].append("/sys/bus/i2c/devices/{}-00{}".format(sfp_group['parent'][-1],sfp_group['i2caddr'][-2:]))
            else:
                sfp_group['paths'].append("/sys/bus/i2c/devices/{}-00{}".format(I2C_SWITCH_LIST[sfp_group['parent']]['bus_map'][sfp_group['channels'][n]],sfp_group['i2caddr'][-2:]))
        
        # if all sfps in a group are success
        if len(sfp_group['paths']) == sfp_group['number']:
            sfp_group['status'] = "INSTALLED"

def uninstall_sfp():
    for sfp_group_name in SFP_GROUPS.keys():
        sfp_group = SFP_GROUPS[sfp_group_name]
        if sfp_group['parent'] in I2C_BASE_BUS:
            uninst_path = I2C_BASE_BUS[sfp_group['parent']]['path']
        else:
            uninst_path = I2C_SWITCH_LIST[sfp_group['parent']]['path']
        
        # sfp is not installed, skip this sfp group
        if sfp_group['status'] != 'INSTALLED':
            continue
            
        for n in range(0,sfp_group['number']):
            sfp_uninst_path = uninst_path+"/channel-{}".format(sfp_group['channels'][n])
            cmd = "echo {} > {}/delete_device".format(sfp_group['i2caddr'], sfp_uninst_path)
            status, output = log_os_system(cmd, 1)

def device_install():
    remove_install_status()
    check_base_bus()
    install_i2c_switch()
    # add delay to make sure all switch is installed completely,
    # so we can start install other slave device safely.
    time.sleep(1)
    install_sfp()
    restore_install_status()

def device_uninstall():
    global SFP_GROUPS
    global I2C_SWITCH_LIST
    try:
        with open(PLATFORM_INSTALL_INFO_FILE) as fd:
            install_info = json.load(fd)
            SFP_GROUPS = install_info[1]
            I2C_SWITCH_LIST = install_info[0]
            uninstall_sfp()
            uninstall_i2c_switch()
        remove_install_status()
    except IOError as e:
        print(e)
        print("Platform install information file is not exist, please do install first")

def do_install():
    print("Checking system....")
    if driver_exist() == False:
        print("No driver, installing....")
        status = driver_install()
        if status:
            if FORCE == 0:
                return status
    else:
        print(PROJECT_NAME.upper() + " drivers detected....")

    if not device_exist():
        print("No device, installing....")
        status = device_install()
        if status:
            if FORCE == 0:
                return status
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
                return status

    if driver_exist() == False:
        print(PROJECT_NAME.upper() + " has no driver installed....")
    else:
        print("Removing installed driver....")
        status = driver_uninstall()
        if status:
            if FORCE == 0:
                return status
    return

def devices_info():
    bus_list = []
    with open(PLATFORM_INSTALL_INFO_FILE) as fd:
        install_info = json.load(fd)
        for device_name in install_info[0].keys():
            device = install_info[0][device_name]
            print("{} :".format(device_name))
            if device['parent'] in I2C_BASE_BUS:
                print("  On Bus: {}".format(device['parent']))
            else:
                print("  On Bus: i2c-{}".format(install_info[0][device['parent']]['bus_map']))
            print("  i2c Address: {}".format(device['i2caddr']))
            print("  status: {}".format(device['status']))
            if device['status'] == 'INSTALLED':
                print("  install path: {}".format(device['path']))
                if device.get('hwmon_path'):
                    print("  hwmon_path: {}".format(device['hwmon_path']))
            print(' ')
        
        for sfp_group_name in install_info[1].keys():
            bus_list = []
            sfp_group = install_info[1][sfp_group_name]
            print("{} :".format(sfp_group_name))
            print("  sfp number: {}".format(sfp_group['number']))
            for n in range(0,sfp_group['number']):
                bus_list.append("i2c-{}".format(install_info[0][sfp_group['parent']]['bus_map'][sfp_group['channels'][n]]))
            print("  On Bus: {}".format(bus_list)) 
            print("  status: {}".format(sfp_group['status']))
            print("  install path: {}".format(', '.join(sfp_group['paths']))) 
            print(' ')


def main():
    global DEBUG
    global args
    global FORCE

    if DEBUG == True:
        print(sys.argv[0])
        print('ARGV      :', sys.argv[1:])

    if len(sys.argv) < 2:
        show_help()

    options, args = getopt.getopt(sys.argv[1:], 'hdf', ['help', 'debug', 'force'])
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
        elif arg == 'show':
            devices_info()
        else:
            show_help()

    return 0

if __name__ == "__main__":
    main()
