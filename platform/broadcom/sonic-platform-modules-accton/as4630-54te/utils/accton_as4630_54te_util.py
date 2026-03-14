#!/usr/bin/env python
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
    show        : show all systen status
    sff         : dump SFP eeprom
    set         : change board setting with fan|led|sfp
"""

import subprocess
import getopt
import sys
import logging
import re
import time
import os

PROJECT_NAME = 'as4630_54te'
version = '0.0.1'
verbose = False
DEBUG = False
args = []
ALL_DEVICE = {}
DEVICE_NO = {
    'led': 5,
    'fan1': 1,
    'fan2': 1,
    'fan3': 1,
    'fan4': 1,
    'fan5': 1,
    'thermal': 3,
    'psu': 2,
    'sfp': 54}


led_prefix = '/sys/devices/platform/as4630_54te_led/leds/accton_' + \
    PROJECT_NAME + '_led::'
fan_prefix = '/sys/devices/platform/as4630_54te_'
hwmon_types = {'led': ['diag', 'fan', 'loc', 'psu1', 'psu2'],
               'fan1': ['fan'],
               'fan2': ['fan'],
               'fan3': ['fan'],
               'fan4': ['fan'],
               'fan5': ['fan'],
               }
hwmon_nodes = {
    'led': ['brightness'],
    'fan1': [
        'fan_duty_cycle_percentage',
        'fan1_fault',
        'fan1_speed_rpm',
        'fan1_direction',
        'fanr1_fault',
        'fanr1_speed_rpm'],
    'fan2': [
        'fan_duty_cycle_percentage',
        'fan2_fault',
        'fan2_speed_rpm',
        'fan2_direction',
        'fanr2_fault',
        'fanr2_speed_rpm'],
    'fan3': [
        'fan_duty_cycle_percentage',
        'fan3_fault',
        'fan3_speed_rpm',
        'fan3_direction',
        'fanr3_fault',
        'fanr3_speed_rpm'],
    'fan4': [
        'fan4_duty_cycle_percentage',
        'fan4_fault',
        'fan4_speed_rpm',
        'fan4_direction',
        'fanr4_fault',
        'fanr4_speed_rpm'],
    'fan5': [
        'fan_duty_cycle_percentage',
        'fan5_fault',
        'fan5_speed_rpm',
        'fan5_direction',
        'fanr5_fault',
        'fanr5_speed_rpm'],
}
hwmon_prefix = {'led': led_prefix,
                'fan1': fan_prefix,
                'fan2': fan_prefix,
                'fan3': fan_prefix,
                'fan4': fan_prefix,
                'fan5': fan_prefix,
                }

i2c_prefix = '/sys/bus/i2c/devices/'
i2c_bus = {'fan': ['54-0066'],
           'thermal': ['54-004c', '55-0048', '55-0049', '55-004a', '55-004b'],
           'psu': ['49-0050', '50-0053'],
           'sfp': ['-0050']}
i2c_nodes = {'fan': ['present', 'front_speed_rpm', 'rear_speed_rpm'],
             'thermal': ['hwmon/hwmon*/temp1_input'],
             'psu': ['psu_present ', 'psu_power_good'],
             'sfp': ['module_present_ ', 'module_tx_disable_']}

sfp_map = [18, 19, 20, 21, 22, 23]

mknod = [
    'echo pca9548 0x77 > /sys/bus/i2c/devices/i2c-1/new_device',
    'echo pca9548 0x71 > /sys/bus/i2c/devices/i2c-2/new_device',
    'echo pca9548 0x70 > /sys/bus/i2c/devices/i2c-3/new_device',

    'echo as4630_54te_cpld 0x60 > /sys/bus/i2c/devices/i2c-3/new_device',

    'echo lm77 0x48 > /sys/bus/i2c/devices/i2c-14/new_device',
    'echo lm75 0x4a > /sys/bus/i2c/devices/i2c-25/new_device',
    'echo lm75 0x4b > /sys/bus/i2c/devices/i2c-24/new_device',


    # PSU-1
    'echo as4630_54te_psu1 0x50 > /sys/bus/i2c/devices/i2c-10/new_device',
    'echo ype1200am 0x58 > /sys/bus/i2c/devices/i2c-10/new_device',

    # PSU-2
    'echo as4630_54te_psu2 0x51> /sys/bus/i2c/devices/i2c-11/new_device',
    'echo ype1200am 0x59 > /sys/bus/i2c/devices/i2c-11/new_device',

    # EERPOM
    'echo 24c02 0x57 > /sys/bus/i2c/devices/i2c-1/new_device',
]

# Disable CPLD debug mode
cpld_set = [
    'i2cset -y -f 3 0x60 0x2a 0xff',
    'i2cset -y -f 3 0x60 0x2b 0xff',
    'i2cset -y -f 3 0x60 0x86 0x89'
]

FORCE = 0
#logging.basicConfig(filename= PROJECT_NAME+'.log', filemode='w',level=logging.DEBUG)
# logging.basicConfig(level=logging.INFO)


def main():
    global DEBUG
    global args
    global FORCE

    if DEBUG:
        print((sys.argv[0]))
        print(('ARGV      : %s' % sys.argv[1:]))

    if len(sys.argv) < 2:
        show_help()

    options, args = getopt.getopt(sys.argv[1:], 'hdf', ['help',
                                                        'debug',
                                                        'force',
                                                        ])
    if DEBUG:
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
            print("TEST")
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
        elif arg == 'show':
            device_traversal()
        elif arg == 'sff':
            if len(args) != 2:
                show_eeprom_help()
            elif int(args[1]) == 0 or int(args[1]) > DEVICE_NO['sfp']:
                show_eeprom_help()
            else:
                show_eeprom(args[1])
            return
        elif arg == 'set':
            if len(args) < 3:
                show_set_help()
            else:
                set_device(args[1:])
            return
        else:
            show_help()

    return 0


def show_help():
    print(__doc__ % {'scriptName': sys.argv[0].split("/")[-1]})
    sys.exit(0)


def show_set_help():
    cmd = sys.argv[0].split("/")[-1] + " " + args[0]
    print(cmd + " [led|sfp|fan]")
    print("    use \"" + cmd + " led 0-4 \"  to set led color")
    print("    use \"" + cmd + " fan 0-100\" to set fan duty percetage")
    print("    use \"" + cmd + " sfp 1-32 {0|1}\" to set sfp# tx_disable")
    sys.exit(0)


def show_eeprom_help():
    cmd = sys.argv[0].split("/")[-1] + " " + args[0]
    print("    use \"" + cmd + " 1-32 \" to dump sfp# eeprom")
    sys.exit(0)


def my_log(txt):
    if DEBUG:
        print("[ACCTON DBG]: " + txt)
    return


def log_os_system(cmd, show):
    logging.info('Run :' + cmd)
    output = ""
    status, output = subprocess.getstatusoutput(cmd)
    my_log(cmd + "with result:" + str(status))
    my_log("cmd:" + cmd)
    my_log("      output:" + output)
    if status:
        logging.info('Failed :' + cmd)
        if show:
            print(('Failed :' + cmd))
    return status, output


def driver_inserted():
    ret, lsmod = log_os_system("ls /sys/module/*accton*", 0)
    logging.info('mods:' + lsmod)
    if ret:
        return False

#'modprobe cpr_4011_4mxx',


kos = [
    'depmod -ae',
    'modprobe i2c_dev',
    'modprobe i2c_mux_pca954x',
    'modprobe ym2651y',
    'modprobe x86_64_accton_as4630_54te_cpld',
    'modprobe x86_64_accton_as4630_54te_leds',
    'modprobe x86_64_accton_as4630_54te_psu',
    'modprobe optoe']


def driver_install():
    global FORCE

    my_log("rmmond i2cismt")
    log_os_system("rmmod i2c_ismt", 1)
    log_os_system("rmmod i2c_i801", 1)
    log_os_system("modprobe i2c-i801", 1)
    time.sleep(1)
    log_os_system("modprobe i2c-ismt", 1)

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
        lst = rm.split(" ")
        print("lst=%s" % lst)
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

    for i in range(0, len(mknod)):
        # for pca954x need times to built new i2c buses
        if mknod[i].find('pca954') != -1:
            time.sleep(2)

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

    print("Check SFP")
    for i in range(0, len(sfp_map)):
        if(i < 4):
            opt = 'optoe2'
        else:
            opt = 'optoe1'
        status, output = log_os_system("echo " +
                                       str(opt) +
                                       " 0x50 > /sys/bus/i2c/devices/i2c-" +
                                       str(sfp_map[i]) +
                                       "/new_device", 1)
        if status:
            print(output)
            if FORCE == 0:
                return status

        status, output = log_os_system("echo port" +
                                       str(i +
                                           49) +
                                       " > /sys/bus/i2c/devices/" +
                                       str(sfp_map[i]) +
                                       "-0050/port_name", 1)
        if status:
            print(output)
            if FORCE == 0:
                return status

    return


def device_uninstall():
    global FORCE

    for i in range(0, len(sfp_map)):
        target = "/sys/bus/i2c/devices/i2c-" + \
            str(sfp_map[i]) + "/delete_device"
        status, output = log_os_system("echo 0x50 > " + target, 1)
        if status:
            print(output)
            if FORCE == 0:
                return status

    nodelist = mknod

    for i in range(len(nodelist)):
        target = nodelist[-(i + 1)]
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
            print(('{} is not found'.format(PLATFORM_API2_WHL_FILE_PY3)))
    else:
        print(('{} has installed'.format(PLATFORM_API2_WHL_FILE_PY3)))

    return

def do_sonic_platform_clean():
    status, output = log_os_system("pip3 show sonic-platform > /dev/null 2>&1", 0)
    if status:
        print(('{} does not install, not need to uninstall'.format(PLATFORM_API2_WHL_FILE_PY3)))

    else:
        status, output = log_os_system("pip3 uninstall sonic-platform -y", 0)
        if status:
            print(('Error: Failed to uninstall {}'.format(PLATFORM_API2_WHL_FILE_PY3)))
            return status
        else:
            print(('{} is uninstalled'.format(PLATFORM_API2_WHL_FILE_PY3)))

    return


def do_install():
    if driver_inserted() == False:
        status = driver_install()
        if status:
            if FORCE == 0:
                return status
    else:
        print(PROJECT_NAME.upper() + " drivers detected....")
    if not device_exist():
        status = device_install()
        if status:
            if FORCE == 0:
                return status
    else:
        print(PROJECT_NAME.upper() + " devices detected....")

    for i in range(len(cpld_set)):
        status, output = log_os_system(cpld_set[i], 1)
        if status:
            if FORCE == 0:
                return status

    do_sonic_platform_install()

    return


def do_uninstall():
    if not device_exist():
        print(PROJECT_NAME.upper() + " has no device installed....")
    else:
        print("Removing device....")
        status = device_uninstall()
        if status:
            if FORCE == 0:
                return status

    if driver_inserted() == False:
        print(PROJECT_NAME.upper() + " has no driver installed....")
    else:
        print("Removing installed driver....")
        status = driver_uninstall()
        if status:
            if FORCE == 0:
                return status

    do_sonic_platform_clean()

    return


def devices_info():
    global DEVICE_NO
    global ALL_DEVICE
    global i2c_bus, hwmon_types
    for key in DEVICE_NO:
        ALL_DEVICE[key] = {}
        for i in range(0, DEVICE_NO[key]):
            ALL_DEVICE[key][key + str(i + 1)] = []

    for key in i2c_bus:
        buses = i2c_bus[key]
        nodes = i2c_nodes[key]
        for i in range(0, len(buses)):
            for j in range(0, len(nodes)):
                if 'fan' == key:
                    for k in range(0, DEVICE_NO[key]):
                        node = key + str(k + 1)
                        path = i2c_prefix + \
                            buses[i] + "/fan" + str(k + 1) + "_" + nodes[j]
                        my_log(node + ": " + path)
                        ALL_DEVICE[key][node].append(path)
                elif 'sfp' == key:
                    for k in range(0, DEVICE_NO[key]):
                        node = key + str(k + 1)
                        path = i2c_prefix + \
                            str(sfp_map[k]) + buses[i] + "/" + nodes[j]
                        my_log(node + ": " + path)
                        ALL_DEVICE[key][node].append(path)
                else:
                    node = key + str(i + 1)
                    path = i2c_prefix + buses[i] + "/" + nodes[j]
                    my_log(node + ": " + path)
                    ALL_DEVICE[key][node].append(path)

    for key in hwmon_types:
        itypes = hwmon_types[key]
        nodes = hwmon_nodes[key]
        for i in range(0, len(itypes)):
            for j in range(0, len(nodes)):
                node = key + "_" + itypes[i]
                path = hwmon_prefix[key] + itypes[i] + "/" + nodes[j]
                my_log(node + ": " + path)
                ALL_DEVICE[key][key + str(i + 1)].append(path)

    # show dict all in the order
    if DEBUG:
        for i in sorted(ALL_DEVICE.keys()):
            print((i + ": "))
            for j in sorted(ALL_DEVICE[i].keys()):
                print(("   " + j))
                for k in (ALL_DEVICE[i][j]):
                    print(("   " + "   " + k))
    return


def show_eeprom(index):
    if system_ready() == False:
        print("System's not ready.")
        print("Please install first!")
        return

    if len(ALL_DEVICE) == 0:
        devices_info()
    node = ALL_DEVICE['sfp']['sfp' + str(index)][0]
    node = node.replace(node.split("/")[-1], 'eeprom')
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
    print("node=%s" % node)
    print(node + ":")
    ret, log = log_os_system("cat " + node + "| " + hex_cmd + " -C", 1)
    if ret == 0:
        print(log)
    else:
        print("**********device no found**********")
    return


def set_device(args):
    global DEVICE_NO
    global ALL_DEVICE
    if system_ready() == False:
        print("System's not ready.")
        print("Please install first!")
        return

    if len(ALL_DEVICE) == 0:
        devices_info()

    if args[0] == 'led':
        if int(args[1]) > 4:
            show_set_help()
            return
        #print  ALL_DEVICE['led']
        for i in range(0, len(ALL_DEVICE['led'])):
            for k in (ALL_DEVICE['led']['led' + str(i + 1)]):
                ret, log = log_os_system("echo " + args[1] + " >" + k, 1)
                if ret:
                    return ret
    elif args[0] == 'fan':
        if int(args[1]) > 100:
            show_set_help()
            return
        #print  ALL_DEVICE['fan']
        # fan1~6 is all fine, all fan share same setting
        node = ALL_DEVICE['fan1']['fan11'][0]
        node = node.replace(node.split("/")[-1], 'fan1_duty_cycle_percentage')
        ret, log = log_os_system("cat " + node, 1)
        if ret == 0:
            print(("Previous fan duty: " + log.strip() + "%"))
        ret, log = log_os_system("echo " + args[1] + " >" + node, 1)
        if ret == 0:
            print(("Current fan duty: " + args[1] + "%"))
        return ret
    elif args[0] == 'sfp':
        if int(args[1]) > DEVICE_NO[args[0]] or int(args[1]) == 0:
            show_set_help()
            return
        if len(args) < 2:
            show_set_help()
            return

        if int(args[2]) > 1:
            show_set_help()
            return

        #print  ALL_DEVICE[args[0]]
        for i in range(0, len(ALL_DEVICE[args[0]])):
            for j in ALL_DEVICE[args[0]][args[0] + str(args[1])]:
                if j.find('tx_disable') != -1:
                    ret, log = log_os_system("echo " + args[2] + " >" + j, 1)
                    if ret:
                        return ret

    return

# get digits inside a string.
# Ex: 31 for "sfp31"


def get_value(input):
    digit = re.findall(r'\d+', input)
    return int(digit[0])


def device_traversal():
    if system_ready() == False:
        print("System's not ready.")
        print("Please install first!")
        return

    if len(ALL_DEVICE) == 0:
        devices_info()
    for i in sorted(ALL_DEVICE.keys()):
        print("============================================")
        print((i.upper() + ": "))
        print("============================================")

        for j in sorted(list(ALL_DEVICE[i].keys()), key=get_value):
            print("   " + j + ":", end=' ')
            for k in (ALL_DEVICE[i][j]):
                ret, log = log_os_system("cat " + k, 0)
                func = k.split("/")[-1].strip()
                func = re.sub(j + '_', '', func, 1)
                func = re.sub(i.lower() + '_', '', func, 1)
                if ret == 0:
                    print(func + "=" + log + " ", end=' ')
                else:
                    print(func + "=" + "X" + " ", end=' ')
            print()
            print("----------------------------------------------------------------")

        print()
    return


def device_exist():
    ret1, log = log_os_system("ls " + i2c_prefix + "*0077", 0)
    ret2, log = log_os_system("ls " + i2c_prefix + "i2c-2", 0)
    return not(ret1 or ret2)


if __name__ == "__main__":
    main()
