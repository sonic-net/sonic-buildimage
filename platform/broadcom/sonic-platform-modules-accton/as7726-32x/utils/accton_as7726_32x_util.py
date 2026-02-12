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
usage: accton_as7726_32x_util.py [-h] [-d] [-f] {install,clean,threshold} ...

AS7726-32X Platform Utility

optional arguments:
  -h, --help            show this help message and exit
  -d, --debug           run with debug mode
  -f, --force           ignore error during installation or clean

Utility Command:
  {install,clean,threshold}
    install             : install drivers and generate related sysfs nodes
    clean               : uninstall drivers and remove related sysfs nodes
    threshold           : modify thermal threshold
"""
import subprocess
import sys
import logging
import re
import time
import argparse
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
    global THRESHOLD_RANGE_LOW, THRESHOLD_RANGE_HIGH

    util_parser = argparse.ArgumentParser(description="AS7726-32X Platform Utility")
    util_parser.add_argument("-d", "--debug", dest='debug', action='store_true', default=False,
                             help="run with debug mode")
    util_parser.add_argument("-f", "--force", dest='force', action='store_true', default=False,
                             help="ignore error during installation or clean")
    subcommand = util_parser.add_subparsers(dest='cmd', title='Utility Command', required=True)
    subcommand.add_parser('install', help=': install drivers and generate related sysfs nodes')
    subcommand.add_parser('clean', help=': uninstall drivers and remove related sysfs nodes')
    threshold_parser = subcommand.add_parser('threshold', help=': modify thermal threshold')
    threshold_parser.add_argument("-l", dest='list', action='store_true', default=False,
                                  help="list avaliable thermal")
    threshold_parser.add_argument("-t", dest='thermal', type=str, metavar='THERMAL_NAME',
                                  help="thermal name, ex: -t 'Temp sensor 1'")
    threshold_parser.add_argument("-ht", dest='high_threshold', type=restricted_float,
                                  metavar='THRESHOLD_VALUE',
                                  help="high threshold: %.1f ~ %.1f" % (THRESHOLD_RANGE_LOW, THRESHOLD_RANGE_HIGH))
    threshold_parser.add_argument("-hct", dest='high_crit_threshold', type=restricted_float,
                                  metavar='THRESHOLD_VALUE',
                                  help="high critical threshold : %.1f ~ %.1f" % (THRESHOLD_RANGE_LOW, THRESHOLD_RANGE_HIGH))
    args = util_parser.parse_args()

    if DEBUG == True:
        print(args)
        print((len(sys.argv)))

    DEBUG = args.debug
    if DEBUG:
        logging.basicConfig(level=logging.INFO)
    FORCE = 1 if args.force else 0

    if args.cmd == 'install':
        do_install()
    elif args.cmd == 'clean':
        do_uninstall()
    elif args.cmd == 'threshold':
        do_threshold()

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

THRESHOLD_RANGE_LOW = 30.0
THRESHOLD_RANGE_HIGH = 110.0
# Code to initialize chassis object
init_chassis_code = \
    "import sonic_platform.platform\n"\
    "platform = sonic_platform.platform.Platform()\n"\
    "chassis = platform.get_chassis()\n\n"

# Looking for thermal
looking_for_thermal_code = \
    "thermal = None\n"\
    "all_thermals = chassis.get_all_thermals()\n"\
    "for psu in chassis.get_all_psus():\n"\
    "    all_thermals += psu.get_all_thermals()\n"\
    "for tmp in all_thermals:\n"\
    "    if '{}' == tmp.get_name():\n"\
    "        thermal = tmp\n"\
    "        break\n"\
    "if thermal == None:\n"\
    "    print('{} not found!')\n"\
    "    exit(1)\n\n"

def avaliable_thermals():
    global init_chassis_code

    get_all_thermal_name_code = \
        "thermal_list = []\n"\
        "all_thermals = chassis.get_all_thermals()\n"\
        "for psu in chassis.get_all_psus():\n"\
        "    all_thermals += psu.get_all_thermals()\n"\
        "for tmp in all_thermals:\n"\
        "    thermal_list.append(tmp.get_name())\n"\
        "print(str(thermal_list)[1:-1])\n"

    all_code = "{}{}".format(init_chassis_code, get_all_thermal_name_code)

    status, output = getstatusoutput_noshell(["docker", "exec", "pmon", "python3", "-c", all_code])
    if status != 0:
        return ""
    return output

def restricted_float(x):
    global THRESHOLD_RANGE_LOW, THRESHOLD_RANGE_HIGH

    try:
        x = float(x)
    except ValueError:
        raise argparse.ArgumentTypeError("%r not a floating-point literal" % (x,))

    if x < THRESHOLD_RANGE_LOW or x > THRESHOLD_RANGE_HIGH:
        raise argparse.ArgumentTypeError("%r not in range [%.1f ~ %.1f]" %
                                         (x, THRESHOLD_RANGE_LOW, THRESHOLD_RANGE_HIGH))

    return x

def get_high_threshold(name):
    global init_chassis_code, looking_for_thermal_code

    get_high_threshold_code = \
        "try:\n"\
        "    print(thermal.get_high_threshold())\n"\
        "    exit(0)\n"\
        "except NotImplementedError:\n"\
        "    print('Not implement the get_high_threshold method!')\n"\
        "    exit(1)"

    all_code = "{}{}{}".format(init_chassis_code, looking_for_thermal_code.format(name, name),
                               get_high_threshold_code)

    status, output = getstatusoutput_noshell(["docker", "exec", "pmon", "python3", "-c", all_code])
    if status == 1:
        return None

    return float(output)

def get_high_crit_threshold(name):
    global init_chassis_code, looking_for_thermal_code

    get_high_crit_threshold_code = \
        "try:\n"\
        "    print(thermal.get_high_critical_threshold())\n"\
        "    exit(0)\n"\
        "except NotImplementedError:\n"\
        "    print('Not implement the get_high_critical_threshold method!')\n"\
        "    exit(1)"

    all_code = "{}{}{}".format(init_chassis_code, looking_for_thermal_code.format(name, name),
                               get_high_crit_threshold_code)

    status, output = getstatusoutput_noshell(["docker", "exec", "pmon", "python3", "-c", all_code])
    if status == 1:
        return None

    return float(output)

def do_threshold():
    global args, init_chassis_code, looking_for_thermal_code

    if args.list:
        print("Thermals: " + avaliable_thermals())
        return

    if args.thermal is None:
        print("The following arguments are required: -t")
        return

    set_threshold_code = ""
    if args.high_threshold is not None:
        if args.high_crit_threshold is not None and \
            args.high_threshold >= args.high_crit_threshold:
           print("Invalid Threshold!(High threshold can not be more than " \
                 "or equal to high critical threshold.)")
           exit(1)

        high_crit = get_high_crit_threshold(args.thermal)
        if high_crit is not None and \
           args.high_threshold >= high_crit:
           print("Invalid Threshold!(High threshold can not be more than " \
                 "or equal to high critical threshold.)")
           exit(1)

        set_threshold_code += \
            "try:\n"\
            "    if thermal.set_high_threshold({}) is False:\n"\
            "        print('{}: set_high_threshold failure!')\n"\
            "        exit(1)\n"\
            "except NotImplementedError:\n"\
            "    print('Not implement the set_high_threshold method!')\n"\
            "print('Apply the new high threshold successfully.')\n"\
            "\n".format(args.high_threshold, args.thermal)

    if args.high_crit_threshold is not None:
        high = get_high_threshold(args.thermal)
        if high is not None and \
            args.high_crit_threshold <= high:
            print("Invalid Threshold!(High critical threshold can not " \
                  "be less than or equal to high threshold.)")
            exit(1)

        set_threshold_code += \
            "try:\n"\
            "    if thermal.set_high_critical_threshold({}) is False:\n"\
            "        print('{}: set_high_critical_threshold failure!')\n"\
            "        exit(1)\n"\
            "except NotImplementedError:\n"\
            "    print('Not implement the set_high_critical_threshold method!')\n"\
            "print('Apply the new high critical threshold successfully.')\n"\
            "\n".format(args.high_crit_threshold, args.thermal)

    if set_threshold_code == "":
        return

    all_code = "{}{}{}".format(init_chassis_code, looking_for_thermal_code.format(args.thermal, args.thermal), set_threshold_code)

    status, output = getstatusoutput_noshell(["docker", "exec", "pmon", "python3", "-c", all_code])
    print(output)

if __name__ == "__main__":
    main()
