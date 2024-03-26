#!/usr/bin/env python
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
    show        : show all system status
    sff         : dump SFP eeprom
    set         : change board setting with fan|led|sfp
"""

import subprocess
import shlex
import getopt
import sys
import logging
import re
import time
import os

PROJECT_NAME = 'es9632xx'
version = '0.1.0'
verbose = False
DEBUG = False
ARGS = []
ALL_DEVICE = {}
DEVICE_NO = {
    'led': 5,
    'fan': 6,
    'thermal': 7,
    'psu': 2,
    'sfp': 32,
    }
FORCE = 0

# logging.basicConfig(filename= PROJECT_NAME+'.log', filemode='w',level=logging.DEBUG)
# logging.basicConfig(level=logging.INFO)

if DEBUG == True:
    print(sys.argv[0])
    print('ARGV      :', sys.argv[1:])


def main():
    global DEBUG
    global ARGS
    global FORCE

    if len(sys.argv) < 2:
        show_help()

    (options, ARGS) = getopt.getopt(sys.argv[1:], 'hdf',
                                ['help','debug', 'force'])
    if DEBUG == True:
        print(options)
        print(ARGS)
        print(len(sys.argv))

    for (opt, arg) in options:
        if opt in ('-h', '--help'):
            show_help()
        elif opt in ('-d', '--debug'):
            DEBUG = True
            logging.basicConfig(level=logging.INFO)
        elif opt in ('-f', '--force'):
            FORCE = 1
        else:
            logging.info('no option')
    for arg in ARGS:
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
            if len(ARGS) != 2:
                show_eeprom_help()
            elif int(ARGS[1]) == 0 or int(ARGS[1]) > DEVICE_NO['sfp']:
                show_eeprom_help()
            else:
                show_eeprom(ARGS[1])
            return
        elif arg == 'set':
            if len(ARGS) < 3:
                show_set_help()
            else:
                set_device(ARGS[1:])
            return
        else:
            show_help()
    return 0


def show_help():
    print(__doc__ % {'scriptName': sys.argv[0].split('/')[-1]})
    sys.exit(0)


def show_set_help():
    cmd = sys.argv[0].split('/')[-1] + ' ' + ARGS[0]
    set_help = """{} [led|led_loc|led_diag|led_fan|led_psu1|led_psu2|sfp|fan]
    use "{} fan 0-100" to set all fans duty percetage
    use "{} sfp 1-48 {{0|1}}" to set sfp# tx_disable
    use "{} led 0-4 " to set all leds to same color
    led colors:
        0 = off
        1 = green
        2 = green blinking
        3 = amber
        4 = amber blinking
    use "{} led_xxx {{0|1|2|3|4}} to set specific led color
    led:
        led_loc     : set location led      {{0|3|4}}
        led_diag    : set diagnostic led    {{0|1|2|3}}
        led_fan     : set fan led           {{0|1|3}}
        led_psu1    : set psu1 led          {{0|1|3}}
        led_psu2    : set psu2 led          {{0|1|3}}""".format(cmd, cmd, cmd, cmd, cmd)
    print(set_help)
    sys.exit(0)

def show_eeprom_help():
    cmd = sys.argv[0].split('/')[-1] + ' ' + ARGS[0]
    print('    use "' + cmd + ' 1-54 " to dump sfp# eeprom')
    sys.exit(0)


def my_log(txt):
    if DEBUG == True:
        print('[DBG]' + txt)
    return
def echo_to_file(cmd, show):
    logging.info('Run :' + cmd)
    split_strings = cmd.split(">")
    split_strings = [s.strip() for s in split_strings]
    echo_cmd = split_strings[0]
    file_path = split_strings[1]
    my_log('Run :' + echo_cmd + ' > ' + file_path)
    with open(file_path, 'w') as f:
        process = subprocess.run(shlex.split(echo_cmd), stdout=f, shell=False)

    my_log(cmd + ' with result:' + str(process.returncode))
    my_log('      output:' + str(process.stdout))
    if process.returncode:
        logging.info('Failed :' + cmd)
        if show:
            print('Failed :' + cmd)
    return (process.returncode, process.stdout)

def log_os_system(cmd, show):
    logging.info('Run :' + cmd)
    process = subprocess.run(shlex.split(cmd), shell=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                             universal_newlines=True)
    my_log(cmd + ' with result:' + str(process.returncode))
    my_log('      output:' + str(process.stdout))
    if process.returncode:
        logging.info('Failed :' + cmd)
        if show:
            print('Failed :' + cmd)
    return (process.returncode, process.stdout)


def driver_check():
    ret, lsmod = log_os_system("ls /sys/module/es9632*", 0)
    logging.info('mods:'+lsmod)
    if ret :
        return False
    else :
        return True


kos = [
    'modprobe i2c_dev',
    'modprobe i2c_mux_pca954x',
    'modprobe dps850',
    'modprobe optoe',
    'modprobe es9632-cpld',
    'modprobe es9632-fan',
    'modprobe es9632-leds',
    'modprobe es9632-psu',
    'modprobe es9632-sys',
    'modprobe es9632-sfp',
    'modprobe at24'
    ]


def driver_install():
    global FORCE
    log_os_system('/usr/sbin/depmod', 1)
    for i in range(0, len(kos)):
        ret = log_os_system(kos[i], 1)
        if ret[0] and FORCE == 0:
            return status
    return 0


def driver_uninstall():
    global FORCE
    for i in range(0, len(kos)):
        rm = kos[-(i + 1)].replace('modprobe', 'modprobe -rq')
        rm = rm.replace('insmod', 'rmmod')
        lst = rm.split(' ')
        if len(lst) > 3:
            del lst[3]
        rm = ' '.join(lst)
        ret = log_os_system(rm, 1)
        if ret[0] and FORCE == 0:
            return ret[0]
    return 0


led_prefix = '/sys/class/leds/' + PROJECT_NAME[:-2] + '_led::'
hwmon_types = {'led': ['diag', 'fan', 'loc', 'psu1', 'psu2']}
leds_dict = {'led_loc': 'loc', 'led_diag': 'diag', 'led_fan': 'fan', 'led_psu1': 'psu1', 'led_psu2': 'psu2'}
hwmon_nodes = {'led': ['brightness']}
hwmon_prefix = {'led': led_prefix}

i2c_prefix = '/sys/bus/i2c/devices/'
i2c_bus = {
    'fan': ['17-0066'],
    'thermal': ['18-0048', '18-0049', '18-004a', '18-004b', '18-004d', '18-004e', '18-004f'],
    'psu': ['10-0051', '9-0050'],
    'sfp': ['-0050'],
    }
i2c_nodes = {
    'fan': ['present', 'front_speed_rpm', 'rear_speed_rpm'],
    'thermal': ['hwmon/hwmon*/temp1_input'],
    'psu': ['psu_present ', 'psu_power_good'],
    'sfp': ['module_present', 'module_tx_disable'],
    }

sfp_map = [25, 26, 27, 28, 29, 30, 31, 32, 33, 34,
           35, 36, 37, 38, 39, 40, 41, 42, 43, 44,
           45, 46, 47, 48, 49, 50, 51, 52, 53, 54,
           55, 56]

mknod = [
    'echo pca9548 0x77 > /sys/bus/i2c/devices/i2c-0/new_device',
    'echo pca9548 0x72 > /sys/bus/i2c/devices/i2c-1/new_device',
    'echo pca9548 0x76 > /sys/bus/i2c/devices/i2c-1/new_device',
    'echo pca9548 0x72 > /sys/bus/i2c/devices/i2c-2/new_device',
    'echo pca9548 0x73 > /sys/bus/i2c/devices/i2c-2/new_device',
    'echo pca9548 0x74 > /sys/bus/i2c/devices/i2c-2/new_device',
    'echo pca9548 0x75 > /sys/bus/i2c/devices/i2c-2/new_device',
    'echo pca9548 0x76 > /sys/bus/i2c/devices/i2c-2/new_device',
    'echo es9632_cpld1 0x68 > /sys/bus/i2c/devices/i2c-1/new_device',
    'echo es9632_cpld2 0x61 > /sys/bus/i2c/devices/i2c-20/new_device',
    'echo es9632_cpld3 0x62 > /sys/bus/i2c/devices/i2c-21/new_device',
    'echo es9632_fan 0x66 > /sys/bus/i2c/devices/i2c-17/new_device',
    'echo es9632_psu1 0x51 > /sys/bus/i2c/devices/i2c-10/new_device',
    'echo es9632_psu2 0x50 > /sys/bus/i2c/devices/i2c-9/new_device',
    'echo dps850 0x59 > /sys/bus/i2c/devices/i2c-10/new_device',
    'echo dps850 0x58 > /sys/bus/i2c/devices/i2c-9/new_device',
    'echo lm75 0x48 > /sys/bus/i2c/devices/i2c-18/new_device',
    'echo lm75 0x49 > /sys/bus/i2c/devices/i2c-18/new_device',
    'echo lm75 0x4a > /sys/bus/i2c/devices/i2c-18/new_device',
    'echo lm75 0x4b > /sys/bus/i2c/devices/i2c-18/new_device',
    'echo lm75 0x4d > /sys/bus/i2c/devices/i2c-18/new_device',
    'echo lm75 0x4e > /sys/bus/i2c/devices/i2c-18/new_device',
    'echo lm75 0x4f > /sys/bus/i2c/devices/i2c-18/new_device',
    'echo es9632_sys 0x55 > /sys/bus/i2c/devices/i2c-23/new_device',
    'echo 24c02 0x57 > /sys/bus/i2c/devices/i2c-0/new_device']

def device_install():
    global FORCE

    for i in range(0, len(mknod)):
        (status, output) = echo_to_file(mknod[i], 1)
        if status:
            print(output)
            if FORCE == 0:
                return status

        if mknod[i].find('pca954') != -1:
            # Allow pca954x to build new i2c buses
            time.sleep(0.3)
            # Configure pca954x to disconnect on idle
            temp = mknod[i].split(os.sep)
            node = temp[len(temp)-2].split('-')[1] + \
                    '-00' + re.split('x| ', temp[0])[3]
            (status, output) = \
                echo_to_file('echo -2 > /sys/bus/i2c/devices/'
                             + node + '/idle_state', 1)
            if status:
                print(output)
                if FORCE == 0:
                    return status

    for i in range(0, len(sfp_map)):
        (status, output) = \
            echo_to_file('echo optoe3 0x50 > /sys/bus/i2c/devices/i2c-'
                            + str(sfp_map[i]) + '/new_device', 1)
        if status:
            print(output)
            if FORCE == 0:
                return status

        num = i + 1
        (status, output) = \
            echo_to_file('echo port' + str(num) + ' > /sys/bus/i2c/devices/'
                            + str(sfp_map[i]) + '-0050/port_name', 1)
        if status:
            print(output)
            if FORCE == 0:
                return status
    return

def device_uninstall():
    global FORCE

    for i in range(0, len(sfp_map)):
        target = '/sys/bus/i2c/devices/i2c-' + str(sfp_map[i]) \
            + '/delete_device'
        (status, output) = echo_to_file('echo 0x50 > ' + target, 1)
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
        (status, output) = echo_to_file(' '.join(temp), 1)
        if status:
            print(output)
            if FORCE == 0:
                return status

    return


def system_ready():
    if driver_check() is False:
        return False
    if not device_exist():
        return False
    return True

PLATFORM_ROOT_PATH = '/usr/share/sonic/device'
PLATFORM_API2_WHL_FILE_PY2 ='sonic_platform-1.0-py2-none-any.whl'
PLATFORM_API2_WHL_FILE_PY3 ='sonic_platform-1.0-py3-none-any.whl'
def do_sonic_platform_install():
    device_path = "{}{}{}{}".format(PLATFORM_ROOT_PATH, '/x86_64-', PROJECT_NAME, '-r0')
    SONIC_PLATFORM_BSP_WHL_PKG_PY3 = "/".join([device_path, PLATFORM_API2_WHL_FILE_PY3])
    SONIC_PLATFORM_BSP_WHL_PKG_PY2 = "/".join([device_path, PLATFORM_API2_WHL_FILE_PY2])

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

    #Check API2.0 on py2.7 whl file
    status, output = log_os_system("pip3 show sonic-platform > /dev/null 2>&1", 0)
    if status:
        if os.path.exists(SONIC_PLATFORM_BSP_WHL_PKG_PY2):
            status, output = log_os_system("pip3 install "+ SONIC_PLATFORM_BSP_WHL_PKG_PY2, 1)
            if status:
                print("Error: Failed to install {}".format(PLATFORM_API2_WHL_FILE_PY2))
                return status
            else:
                print("Successfully installed {} package".format(PLATFORM_API2_WHL_FILE_PY2))
        else:
            print('{} is not found'.format(PLATFORM_API2_WHL_FILE_PY2))
    else:
        print('{} has installed'.format(PLATFORM_API2_WHL_FILE_PY2))
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

    status, output = log_os_system("pip3 show sonic-platform > /dev/null 2>&1", 0)
    if status:
        print('{} does not install, not need to uninstall'.format(PLATFORM_API2_WHL_FILE_PY2))

    else:
        status, output = log_os_system("pip3 uninstall sonic-platform -y", 0)
        if status:
            print('Error: Failed to uninstall {}'.format(PLATFORM_API2_WHL_FILE_PY2))
            return status
        else:
            print('{} is uninstalled'.format(PLATFORM_API2_WHL_FILE_PY2))

    return

def do_install():
    print('Checking system....')
    if driver_check() is False:
        print('No driver, installing....')
        status = driver_install()
        if status:
            if FORCE == 0:
                return status
    else:
        print(PROJECT_NAME.upper() + ' drivers detected....')
    if not device_exist():
        print('No device, installing....')
        status = device_install()
        if status:
            if FORCE == 0:
                return status
    else:
        print(PROJECT_NAME.upper() + ' devices detected....')
    do_sonic_platform_install()

    return


def do_uninstall():
    print('Checking system....')
    if not device_exist():
        print(PROJECT_NAME.upper() + ' has no device installed....')
    else:
        print('Removing device....')
        status = device_uninstall()
        if status and FORCE == 0:
            return status

    if driver_check() is False:
        print(PROJECT_NAME.upper() + ' has no driver installed....')
    else:
        print('Removing installed driver....')
        status = driver_uninstall()
        if status and FORCE == 0:
            return status

    do_sonic_platform_clean()

    return None


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
                        path = i2c_prefix + buses[i] + '/fan' + str(k
                                + 1) + '_' + nodes[j]
                        my_log(node + ': ' + path)
                        ALL_DEVICE[key][node].append(path)
                elif 'sfp' == key:
                    for k in range(0, DEVICE_NO[key]):
                        if k in range(24) or k in range(48, 52):
                            fmt = i2c_prefix + '5-0062/{0}_{1}'
                        else:
                            fmt = i2c_prefix + '6-0064/{0}_{1}'
                        node = key + str(k + 1)
                        path = fmt.format(nodes[j], k + 1)
                        my_log(node + ': ' + path)
                        ALL_DEVICE[key][node].append(path)
                else:
                    node = key + str(i + 1)
                    path = i2c_prefix + buses[i] + '/' + nodes[j]
                    my_log(node + ': ' + path)
                    ALL_DEVICE[key][node].append(path)

    for key in hwmon_types:
        itypes = hwmon_types[key]
        nodes = hwmon_nodes[key]
        for i in range(0, len(itypes)):
            for j in range(0, len(nodes)):
                node = key + '_' + itypes[i]
                path = hwmon_prefix[key] + itypes[i] + '/' + nodes[j]
                my_log(node + ': ' + path)
                ALL_DEVICE[key][key + str(i + 1)].append(path)

    # show dict all in the order
    if DEBUG == True:
        for i in sorted(ALL_DEVICE.keys()):
            print(i + ': ')
            for j in sorted(ALL_DEVICE[i].keys()):
                print('   ' + j)
                for k in ALL_DEVICE[i][j]:
                    print('   ' + '   ' + k)
    return


def show_eeprom(index):
    if system_ready() is False:
        print('Systems not ready.')
        print('Please install first!')
        return

    if len(ALL_DEVICE) == 0:
        devices_info()
    node = ALL_DEVICE['sfp']['sfp' + str(index)][0]
    node = node.replace(node.split('/')[-1], 'sfp_eeprom')

    # check if got hexdump command in current environment
    (ret, log) = log_os_system('which hexdump', 0)
    (ret, log2) = log_os_system('which busybox hexdump', 0)
    if log:
        hex_cmd = 'hexdump'
    elif log2:
        hex_cmd = ' busybox hexdump'
    else:
        log = 'Failed : no hexdump cmd!!'
        logging.info(log)
        print(log)
        return 1

    print(node + ':')
    (ret, log) = log_os_system('cat ' + node + '| ' + hex_cmd + ' -C',
                               1)
    if ret == 0:
        print(log)
    else:
        print( '**********device no found**********')
    return


def set_device(args):
    global DEVICE_NO
    global ALL_DEVICE
    if system_ready() is False:
        print('System is not ready.')
        print('Please install first!')
        return

    if not ALL_DEVICE:
        devices_info()

    if args[0] == 'led':
        if int(args[1]) > 4:
            show_set_help()
            return

        # print(ALL_DEVICE['led'])
        for i in range(0, len(ALL_DEVICE['led'])):
            for k in ALL_DEVICE['led']['led' + str(i + 1)]:
                ret = log_os_system('echo ' + args[1] + ' >' + k, 1)
                if ret[0]:
                    return ret[0]

    elif args[0] in leds_dict:
        ret = set_specific_led(leds_dict[args[0]], args[1])
        if ret:
            return ret

    elif args[0] == 'fan':
        if int(args[1]) > 100:
            show_set_help()
            return

        # print(ALL_DEVICE['fan'])
        # fan1~6 is all fine, all fan share same setting

        node = ALL_DEVICE['fan']['fan1'][0]
        node = node.replace(node.split('/')[-1],
                            'fan_duty_cycle_percentage')
        (ret, log) = log_os_system('cat ' + node, 1)
        if ret == 0:
            print('Previous fan duty: ' + log.strip() + '%')
        ret = log_os_system('echo ' + args[1] + ' >' + node, 1)
        if ret[0] == 0:
            print('Current fan duty: ' + args[1] + '%')
        return ret
    elif args[0] == 'sfp':
        if int(args[1]) > qsfp_start or int(args[1]) == 0:
            show_set_help()
            return
        if len(args) < 2:
            show_set_help()
            return

        if int(args[2]) > 1:
            show_set_help()
            return

        # print(ALL_DEVICE[args[0]])

        for i in range(len(ALL_DEVICE[args[0]])):
            for j in ALL_DEVICE[args[0]][args[0] + str(args[1])]:
                if j.find('tx_disable') != -1:
                    ret = log_os_system('echo ' + args[2] + ' >' + j,  1)
                    if ret[0]:
                        return ret[0]

    return

def set_specific_led(led, mode):
    for i in range(0, len(ALL_DEVICE['led'])):
        for k in ALL_DEVICE['led']['led' + str(i + 1)]:
            if led in k:
                ret = log_os_system('echo ' + mode + ' >' + k, 1)
                return ret[0]

# get digits inside a string.
# Ex: get 31 from "sfp31"
def get_value(i):
    digit = re.findall('\d+', i)
    return int(digit[0])


def device_traversal():
    if system_ready() is False:
        print("System is  not ready.")
        print('Please install first!')
        return

    if not ALL_DEVICE:
        devices_info()
    for i in sorted(ALL_DEVICE.keys()):
        print('============================================')
        print(i.upper() + ': ')
        print('============================================')
        for j in sorted(ALL_DEVICE[i].keys(), key=get_value):
            print('   ' + j + ':',)
            for k in ALL_DEVICE[i][j]:
                (ret, log) = log_os_system('cat ' + k, 0)
                func = k.split('/')[-1].strip()
                func = re.sub(j + '_', '', func, 1)
                func = re.sub(i.lower() + '_', '', func, 1)
                if ret == 0:
                    print(func + '=' + log + ' ',)
                else:
                    print(func + '=' + 'X' + ' ',)
            print()
            print('----------------------------------------------------------------')
        print()
    return


def device_exist():
    ret1 = log_os_system('ls ' + i2c_prefix + '*0077', 0)
    ret2 = log_os_system('ls ' + i2c_prefix + 'i2c-2', 0)
    return not (ret1[0] or ret2[0])


if __name__ == '__main__':
    main()
