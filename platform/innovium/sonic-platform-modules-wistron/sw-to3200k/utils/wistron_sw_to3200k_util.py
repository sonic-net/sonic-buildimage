#!/usr/bin/env python

"""
Usage: %(scriptName)s [options] command object

options:
    -h | --help     : this help message
    -d | --debug    : run with debug mode
    -f | --force    : ignore error during installation or clean
command:
    install     : install drivers and generate related sysfs nodes
    clean       : uninstall drivers and remove related sysfs nodes
    show        : show all systen status
    sff         : dump SFP eeprom
"""

import subprocess
import sys, getopt
import logging
import re
import time

PROJECT_NAME = 'sw_to3200k'
version = '0.0.1'
verbose = False
DEBUG = False
args = []
ALL_DEVICE = {}
DEVICE_NO = {'led':5, 'fan':7, 'thermal':7, 'psu':2, 'qsfp':32}

led_prefix ='/sys/devices/platform/wistron_sw_to3200k_led/leds/wistron_'+PROJECT_NAME+'_led::'
hwmon_types = {'led': ['diag','fan','loc','psu1','psu2']}
hwmon_nodes = {'led': ['brightness']}
hwmon_prefix ={'led': led_prefix}

i2c_prefix = '/sys/bus/i2c/devices/'

i2c_bus = {'fan': ['0-0066'],
           'thermal': ['0-0049','0-004a','0-004b','0-004c','0-004d','0-004e','0-004f'],
           'psu': ['0-0051','0-0052'],
           'qsfp': ['0-0061']}

i2c_nodes = {'fan': ['front_speed_rpm', 'rear_speed_rpm'],
             'thermal': ['temp1_input'],
             'psu': ['psu_present', 'psu_power_good'],
             'qsfp': ['module_present_']}

sfp_map = [10, 11, 12, 13, 14, 15, 16, 17,
           18, 19, 20, 21, 22, 23, 24, 25,
           26, 27, 28, 29, 30, 31, 32, 33,
           34, 35, 36, 37, 38, 39, 40, 41]

mknod =[
'echo sw_to3200k_fpga 0x60 > /sys/bus/i2c/devices/i2c-0/new_device',
'echo sw_to3200k_cpld1 0x61 > /sys/bus/i2c/devices/i2c-0/new_device',
'echo sw_to3200k_cpld2 0x62 > /sys/bus/i2c/devices/i2c-0/new_device',
# FAN
'echo sw_to3200k_fan 0x66 > /sys/bus/i2c/devices/i2c-0/new_device',
# Thermal
'echo sw_to3200k_thermal 0x49 > /sys/bus/i2c/devices/i2c-0/new_device',
'echo sw_to3200k_thermal 0x4a > /sys/bus/i2c/devices/i2c-0/new_device',
'echo sw_to3200k_thermal 0x4b > /sys/bus/i2c/devices/i2c-0/new_device',
'echo sw_to3200k_thermal 0x4c > /sys/bus/i2c/devices/i2c-0/new_device',
'echo sw_to3200k_thermal 0x4d > /sys/bus/i2c/devices/i2c-0/new_device',
'echo sw_to3200k_thermal 0x4e > /sys/bus/i2c/devices/i2c-0/new_device',
'echo sw_to3200k_thermal 0x4f > /sys/bus/i2c/devices/i2c-0/new_device',
# PSU-1 eeprom 0x51
'echo sw_to3200k_psu1 0x51 > /sys/bus/i2c/devices/i2c-0/new_device',
'echo acbel_fshxxx    0x59 > /sys/bus/i2c/devices/i2c-0/new_device',
# PSU-2 eeprom 0x52
'echo sw_to3200k_psu2 0x52 > /sys/bus/i2c/devices/i2c-0/new_device',
'echo acbel_fshxxx    0x5a > /sys/bus/i2c/devices/i2c-0/new_device',
# EEPROM
'echo 24c02 0x56 > /sys/bus/i2c/devices/i2c-0/new_device',
]

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

    logging.basicConfig(level=logging.INFO)

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
        elif arg == 'show':
           device_traversal()
        elif arg == 'sff':
            if len(args)!=2:
                show_eeprom_help()
            elif int(args[1]) > DEVICE_NO['qsfp'] -1:
                show_eeprom_help()
            else:
                show_eeprom(args[1])
            return
        else:
            show_help()


    return 0

def show_help():
    print(__doc__ % {'scriptName' : sys.argv[0].split("/")[-1]})
    sys.exit(0)

def show_eeprom_help():
    cmd =  sys.argv[0].split("/")[-1]+ " "  + args[0]
    print("    use \""+ cmd + " 1-32 \" to dump sfp# eeprom")
    sys.exit(0)

def my_log(txt):
    if DEBUG == True:
        print("[WISTRON DBG]: "+txt)
    return

def log_os_system(cmd, show):
    logging.info('Run :'+cmd)
    output = ""
    status, output = subprocess.getstatusoutput(cmd)
    my_log (cmd +"with result:" + str(status))
    my_log ("cmd:" + cmd)
    my_log ("      output:"+output)
    if status:
        logging.info('Failed :'+cmd)
        if show:
            print('Failed :'+cmd)
    return  status, output

def driver_inserted():
    ret, lsmod = log_os_system("lsmod | grep wistron", 0)
    logging.info('mods:'+lsmod)
    if len(lsmod) ==0:
        return False


kos = [
'modprobe ipmi_msghandler',
'modprobe ipmi_si',
'modprobe ipmi_devintf',
'modprobe i2c_dev',
'modprobe at24',
'modprobe wistron_i2c_psu',
'modprobe wistron_sw_to3200k_cpld',
'modprobe wistron_sw_to3200k_fan',
'modprobe wistron_sw_to3200k_leds',
'modprobe wistron_sw_to3200k_psu',
'modprobe wistron_sw_to3200k_oom',
'modprobe wistron_sw_to3200k_thermal']

def driver_install():
    global FORCE
    log_os_system("depmod", 1)
    for i in range(0,len(kos)):
        status, output = log_os_system(kos[i], 1)
        time.sleep(1)
        if status:
            if FORCE == 0:
                return status
    return 0

def driver_uninstall():
    global FORCE
    for i in range(0,len(kos)):
        rm = kos[-(i+1)].replace("modprobe", "modprobe -rq")
        lst = rm.split(" ")
        print("lst=%s"%lst)
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
        print("init i2c device instance")
        status, output = log_os_system(mknod[i], 1)
        if status:
            print(output)
            if FORCE == 0:
                return status

    for i in range(0,len(sfp_map)):
        status, output = log_os_system("echo sw_to3200k_oom 0x"+str(sfp_map[i])+ " > /sys/bus/i2c/devices/i2c-0/new_device", 1)
        if status:
            print(output)
            if FORCE == 0:
                return status

    for i in range(0,len(sfp_map)):
        status, output =log_os_system("echo port"+str(i+1)+" > /sys/bus/i2c/devices/0-00"+str(sfp_map[i])+"/port_name", 1)
        if status:
            print(output)
            if FORCE == 0:
                return status

    status, output = log_os_system("cp /usr/share/sonic/device/x86_64-wistron_sw_to3200k-r0/smartd.conf /etc/;systemctl restart smartd.service", 1)
    if status:
        print(output)
        if FORCE == 0:
            return status

    status, output = log_os_system("cp /usr/share/sonic/device/x86_64-wistron_sw_to3200k-r0/watchdog-control.service /usr/lib/systemd/system/", 1)
    if status:
        print(output)
        if FORCE == 0:
            return status
    return

def device_uninstall():
    global FORCE

    for i in range(0,len(sfp_map)):
        target = "echo 0x"+str(sfp_map[i])+ " > /sys/bus/i2c/devices/i2c-0/delete_device"
        print(target)
        status, output =log_os_system(target, 1)
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
        time.sleep(1)
        if status:
            if FORCE == 0:
                return  status
    else:
        print(PROJECT_NAME.upper()+" drivers detected....")

    if not device_exist():
        status = device_install()
        if status:
            if FORCE == 0:
                return  status
    else:
        print(PROJECT_NAME.upper()+" devices detected....")

    return

def do_uninstall():
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

    return

def devices_info():
    global DEVICE_NO
    global ALL_DEVICE
    global i2c_bus, hwmon_types
    for key in DEVICE_NO:
        ALL_DEVICE[key]= {}
        for i in range(0,DEVICE_NO[key]):
            ALL_DEVICE[key][key+str(i+1)] = []

    for key in i2c_bus:
        buses = i2c_bus[key]
        nodes = i2c_nodes[key]
        for i in range(0,len(buses)):
            for j in range(0,len(nodes)):
                if 'fan' == key:
                    for k in range(0,DEVICE_NO[key]):
                        node = key+str(k+1)
                        path = i2c_prefix+ buses[i]+"/fan"+str(k+1)+"_"+ nodes[j]
                        my_log(node+": "+ path)
                        ALL_DEVICE[key][node].append(path)
                elif 'qsfp' == key:
                    for k in range(0,DEVICE_NO[key]):
                        node = key+str(k+1)
                        if k < 16:
                            path = i2c_prefix+"0-0061/"+ nodes[j]+str(k+1)
                        else:
                            path = i2c_prefix+"0-0062/"+ nodes[j]+str(k+1)
                        my_log(node+": "+ path)
                        ALL_DEVICE[key][node].append(path)
                else:
                    node = key+str(i+1)
                    path = i2c_prefix+ buses[i]+"/"+ nodes[j]
                    my_log(node+": "+ path)
                    ALL_DEVICE[key][node].append(path)

    for key in hwmon_types:
        itypes = hwmon_types[key]
        nodes = hwmon_nodes[key]
        for i in range(0,len(itypes)):
            for j in range(0,len(nodes)):
                node = key+"_"+itypes[i]
                path = hwmon_prefix[key]+ itypes[i]+"/"+ nodes[j]
                my_log(node+": "+ path)
                ALL_DEVICE[key][ key+str(i+1)].append(path)

    #show dict all in the order
    if DEBUG == True:
        for i in sorted(ALL_DEVICE.keys()):
            print(i+": ")
            for j in sorted(ALL_DEVICE[i].keys()):
                print("   "+j)
                for k in (ALL_DEVICE[i][j]):
                    print("   "+"   "+k)
    return

def show_eeprom(index):
    if system_ready()==False:
        print("System's not ready.")
        print("Please install first!")
        return

    node= "/sys/bus/i2c/devices/0-00"+str(sfp_map[int(index)-1])+"/eeprom1"
    # check if got hexdump command in current environment
    ret, log = log_os_system("which hexdump", 0)
    ret, log2 = log_os_system("which busybox hexdump", 0)
    if len(log):
        hex_cmd = 'hexdump'
    elif len(log2):
        hex_cmd = ' busybox hexdump'
    else:
        log = 'Failed : no hexdump cmd!!'
        logging.info(log)
        print(log)
        return 1

    print("node=%s"%node)
    ret, log = log_os_system(hex_cmd+" -C " + node, 1)
    if ret==0:
        print(log)
    else:
        print("**********device no found**********")
    return

#get digits inside a string.
#Ex: 31 for "sfp31"
def get_value(input):
    digit = re.findall('\d+', input)
    return int(digit[0])

def device_traversal():
    if system_ready()==False:
        print("System's not ready.")
        print("Please install first!")
        return

    if len(ALL_DEVICE)==0:
        devices_info()
    for i in sorted(ALL_DEVICE.keys()):
        print("============================================")
        print(i.upper()+": ")
        print("============================================")

        for j in sorted(ALL_DEVICE[i].keys(), key=get_value):
            print("   "+j+":",)
            for k in (ALL_DEVICE[i][j]):
                ret, log = log_os_system("cat "+k, 0)
                func = k.split("/")[-1].strip()
                func = re.sub(j+'_','',func,1)
                func = re.sub(i.lower()+'_','',func,1)
                if ret==0:
                    print(func+"="+log+" ",)
                else:
                    print(func+"="+"X"+" ",)
            print()
            print("----------------------------------------------------------------")


        print()
    return

def device_exist():
    ret1, log = log_os_system("ls "+i2c_prefix+"*0060", 0)
    ret2, log = log_os_system("ls "+i2c_prefix+"i2c-0", 0)
    return not(ret1 or ret2)

if __name__ == "__main__":
    main()
