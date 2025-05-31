#!/opt/xplt/venv/bin/python3
# Copyright (C) 2016 Accton Networks, Inc.
# Copyright (C) 2024 Xsightlabs Ltd.
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
import getopt
import shlex
import glob
import sys
import logging
import re
import time
import os

PROJECT_NAME = 'es9618xx'
version = '0.1.0'
verbose = False
DEBUG = False
ARGS = []
ALL_DEVICE = {}
DEVICE_NO = {
    'led': 5,
    'fan': 6,
    'thermal': 8,
    'psu': 2,
    'sfp': 16,
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
    use "{} sfp 1-16 {{0|1}}" to set sfp# module_lpmode
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
    print('    use "' + cmd + ' 1-16" to dump sfp# eeprom')
    sys.exit(0)


def log_dbg(txt):
    if DEBUG:
        print('[DBG] ' + txt)


def run_command(cmd_str, use_sudo=True):
    cmd_list = shlex.split(cmd_str)
    if use_sudo:
        cmd_list.insert(0, 'sudo')

    logging.info('Run :' + cmd_str)
    try:
        process = subprocess.run(
            cmd_list,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True
        )
        stdout = process.stdout.strip()
        retcode = process.returncode
    except Exception as e:
        log_dbg(f"Error running command: {e}")
        return (1, str(e))

    log_dbg(cmd_str + ' with result: ' + str(retcode))
    log_dbg('      output: ' + stdout)

    if retcode != 0:
        logging.info('Failed :' + cmd_str)

    return (retcode, stdout)


def echo_to_file(value, filepath, use_sudo=True):
    # Use 'tee' as a safe way to write via subprocess with sudo
    try:
        cmd_list = ['sudo', 'tee', filepath] if use_sudo else ['tee', filepath]
        process = subprocess.run(
            cmd_list,
            input=value,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True
        )
        if process.returncode != 0:
            raise Exception(process.stdout.strip())
        return (0, '')

    except Exception as e:
        log_dbg(f"Failed to write '{value}' to '{filepath}' with error: {str(e)}")
        return (1, str(e))


def list_es9618xx_modules():
    matches = glob.glob('/sys/module/*es9618xx*')
    if matches:
        return (0, '\n'.join(matches))
    else:
        return (1, '')


def grep_file_for_string(filepath, pattern):
    try:
        with open(filepath, 'r') as f:
            for line in f:
                if pattern in line:
                    return (0, line.strip())
        return (1, '')
    except Exception as e:
        return (1, str(e))


def driver_check():
    ret, out = list_es9618xx_modules()
    if ret:
        logging.info("Modules: None found")
        return False
    else:
        logging.info(f"Modules: {out}")
        return True


def read_file_contents(filepath):
    try:
        with open(filepath, 'r') as f:
            contents = f.read()
        return 0, contents
    except Exception as e:
        return (1, str(e))

kos = [
    'modprobe i2c-dev',
    'modprobe i2c_mux_pca954x',
    'modprobe dps850',
    'modprobe optoe',
    'modprobe x86-64-es9618xx-cpld',
    'modprobe x86-64-es9618xx-fan',
    'modprobe x86-64-es9618xx-leds',
    'modprobe x86-64-es9618xx-psu',
    'modprobe at24',
    'modprobe lm75',
    'modprobe tmp401'
    ]


def driver_install():
    global FORCE
    run_command('depmod')
    for i in range(0, len(kos)):
        ret = run_command(kos[i])
        if ret[0] and FORCE == 0:
            return ret[0]
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
        ret = run_command(rm)
        if ret[0] and FORCE == 0:
            return ret[0]
    return 0


led_prefix = '/sys/class/leds/' + PROJECT_NAME + '_led::'
hwmon_types = {'led': ['diag', 'fan', 'loc', 'psu1', 'psu2']}
leds_dict = {'led_loc': 'loc', 'led_diag': 'diag', 'led_fan': 'fan', 'led_psu1': 'psu1', 'led_psu2': 'psu2'}
hwmon_nodes = {'led': ['brightness']}
hwmon_prefix = {'led': led_prefix}

i2c_prefix = '/sys/bus/i2c/devices/'
i2c_bus = {
    'fan': ['10-0066'],
    'thermal': ['11-0048', '11-0049', '11-004a', '11-004b', '11-004c', '11-004d', '11-004e', '11-004f'],
    'psu': ['3-0051', '2-0050'],
    'sfp': ['-0050'],
    }
i2c_nodes = {
    'fan': ['present', 'front_speed_rpm', 'rear_speed_rpm'],
    'thermal': ['temp1_input'],
    'psu': ['psu_present ', 'psu_power_good'],
    'sfp': ['module_present', 'module_lpmode'],
    }

mknod_entries = [
    ("pca9548", "0x72", "1"),
    ("pca9548", "0x73", "1"),
    ("pca9548", "0x74", "1"),
    ("pca9548", "0x75", "1"),
    ("pca9548", "0x76", "1"),
    ("es9618xx_cpld1", "0x68", "1"),
    ("es9618xx_cpld2", "0x61", "13"),
    ("es9618xx_psu1", "0x51", "3"),
    ("es9618xx_psu2", "0x50", "2"),
    ("es9618xx_fan", "0x66", "10"),
    ("dps850", "0x58", "2"),
    ("dps850", "0x59", "3"),
    ("lm75", "0x48", "11"),
    ("lm75", "0x49", "11"),
    ("lm75", "0x4a", "11"),
    ("lm75", "0x4b", "11"),
    ("lm75", "0x4c", "11"),
    ("lm75", "0x4d", "11"),
    ("lm75", "0x4e", "11"),
    ("lm75", "0x4f", "11"),
    ("tmp431", "0x4c", "8"),
    ("24c32", "0x50", "0"),
]

sfp_map = list(range(18, 34))


def device_install():
    global FORCE

    for dev, addr, bus in mknod_entries:
        path = f"/sys/bus/i2c/devices/i2c-{bus}/new_device"
        ret, msg = echo_to_file(f"{dev} {addr}", path)
        if ret and FORCE == 0:
            return ret

        if dev == "pca9548":
            # Allow pca954x to build new i2c buses
            time.sleep(0.3)
            # Configure pca954x to disconnect on idle
            path = f"/sys/bus/i2c/devices/{bus}-00{addr[2:]}/idle_state"
            ret, msg = echo_to_file("-2", path)
            if ret and FORCE == 0:
                return ret

    for count, bus in enumerate(sfp_map, start=1):
        path = f"/sys/bus/i2c/devices/i2c-{bus}/new_device"
        ret, msg = echo_to_file("optoe3 0x50", path)
        if ret and FORCE == 0:
            return ret

        path = f"/sys/bus/i2c/devices/{bus}-0050/port_name"
        ret, msg = echo_to_file(f"port{count}", path)
        if ret and FORCE == 0:
            return ret


def device_uninstall():
    global FORCE

    for i in sfp_map:
        path = f"/sys/bus/i2c/devices/i2c-{i}/delete_device"
        value = "0x50"
        ret, msg = echo_to_file(value, path)
        if ret and FORCE == 0:
            return ret

    for dev, addr, bus in reversed(mknod_entries):
        path = f"/sys/bus/i2c/devices/i2c-{bus}/delete_device"
        value = f"{addr}"
        ret, msg = echo_to_file(value, path)
        if ret and FORCE == 0:
            return ret


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
    device_path = "{}{}{}{}{}".format(PLATFORM_ROOT_PATH, '/x86_64-', 'accton_', PROJECT_NAME, '-r0')
    SONIC_PLATFORM_BSP_WHL_PKG_PY3 = "/".join([device_path, PLATFORM_API2_WHL_FILE_PY3])
    SONIC_PLATFORM_BSP_WHL_PKG_PY2 = "/".join([device_path, PLATFORM_API2_WHL_FILE_PY2])

    #Check API2.0 on py whl file
    status, output = run_command("pip3 show sonic-platform")
    if status:
        if os.path.exists(SONIC_PLATFORM_BSP_WHL_PKG_PY3):
            status, output = run_command("pip3 install "+ SONIC_PLATFORM_BSP_WHL_PKG_PY3)
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
    status, output = run_command("pip2 show sonic-platform")
    if status:
        if os.path.exists(SONIC_PLATFORM_BSP_WHL_PKG_PY2):
            status, output = run_command("pip2 install "+ SONIC_PLATFORM_BSP_WHL_PKG_PY2)
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
    status, output = run_command("pip3 show sonic-platform")
    if status:
        print('{} does not install, not need to uninstall'.format(PLATFORM_API2_WHL_FILE_PY3))

    else:
        status, output = run_command("pip3 uninstall sonic-platform -y")
        if status:
            print('Error: Failed to uninstall {}'.format(PLATFORM_API2_WHL_FILE_PY3))
            return status
        else:
            print('{} is uninstalled'.format(PLATFORM_API2_WHL_FILE_PY3))

    status, output = run_command("pip2 show sonic-platform")
    if status:
        print('{} does not install, not need to uninstall'.format(PLATFORM_API2_WHL_FILE_PY2))

    else:
        status, output = run_command("pip2 uninstall sonic-platform -y")
        if status:
            print('Error: Failed to uninstall {}'.format(PLATFORM_API2_WHL_FILE_PY2))
            return status
        else:
            print('{} is uninstalled'.format(PLATFORM_API2_WHL_FILE_PY2))

    return


def set_i2c_bus_indexing_order():
    fname = os.path.basename(__file__)
    try:
        path = "/sys/bus/i2c/devices/i2c-{}/name"
        patt = "I801"
        res0, i2c_0 = grep_file_for_string(path.format(0), patt)
        res1, i2c_1 = grep_file_for_string(path.format(1), patt)
        if int(res0) + int(res1) > 1:
            raise SystemExit(f'{fname}: Cannot find i2c master device!')
        if int(res0) == 0:
            path = "/dev/kmsg"
            value = f"echo {fname}: Swapp i2c buses"
            ret0, log = echo_to_file(value, path)
            ret1, log = run_command("rmmod i2c_ismt")
            ret2, log = run_command("rmmod i2c_i801")
            ret3, log = run_command("modprobe i2c_ismt")
            ret4, log = run_command("modprobe i2c_i801")
            if ret1 or ret2 or ret3 or ret4:
                return False
        return True

    except Exception as err:
        print(f'{fname}: {err=}, {type(err)=}')
        raise


def do_install():
    print('Checking system....')

    if not set_i2c_bus_indexing_order():
        sys.exit(1)

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

    return 0


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

    return 0


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
                        log_dbg(node + ': ' + path)
                        ALL_DEVICE[key][node].append(path)
                elif 'sfp' == key:
                    for k in range(0, DEVICE_NO[key]):
                        node = key + str(k + 1)
                        fmt = i2c_prefix + "13-0061/{}_{}"
                        path = fmt.format(nodes[j], k + 1)
                        log_dbg(node + ': ' + path)
                        ALL_DEVICE[key][node].append(path)
                elif 'thermal' == key:
                    node = key + str(i + 1)
                    fmt = i2c_prefix + "{}/hwmon/hwmon{}/{}"
                    path = fmt.format(buses[i], int(buses[0][6:]) + i, nodes[j])
                    log_dbg(node + ': ' + path)
                    ALL_DEVICE[key][node].append(path)
                else:
                    node = key + str(i + 1)
                    path = i2c_prefix + buses[i] + '/' + nodes[j]
                    log_dbg(node + ': ' + path)
                    ALL_DEVICE[key][node].append(path)

    for key in hwmon_types:
        itypes = hwmon_types[key]
        nodes = hwmon_nodes[key]
        for i in range(0, len(itypes)):
            for j in range(0, len(nodes)):
                node = key + '_' + itypes[i]
                path = hwmon_prefix[key] + itypes[i] + '/' + nodes[j]
                log_dbg(node + ': ' + path)
                ALL_DEVICE[key][key + str(i + 1)].append(path)

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

    node = f"{i2c_prefix}{sfp_map[int(index) - 1]}{i2c_bus['sfp'][0]}/eeprom"
    # Check if got hexdump command in current environment
    ret, out = run_command('which hexdump')
    ret, out2 = run_command('which busybox hexdump')
    if out:
        hex_cmd = ["hexdump", "-C"]
    elif out2:
        hex_cmd = ["busybox", "hexdump", "-C"]
    else:
        logging.info(out)
        print(out)
        return 1

    print(node + ':')

    # Read the binary contents of the EEPROM file
    with open(node, "rb") as f:
        data = f.read(256)

    # Run hexdump -C using subprocess, pass binary data through stdin
    process = subprocess.run(
        hex_cmd,
        input=data,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )
    retcode = process.returncode

    if retcode == 0:
        print(process.stdout.decode())
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

        for i in range(0, len(ALL_DEVICE['led'])):
            for k in ALL_DEVICE['led']['led' + str(i + 1)]:
                ret, log = echo_to_file(args[1], k)
                if ret:
                    return ret

    elif args[0] in leds_dict:
        ret = set_specific_led(leds_dict[args[0]], args[1])
        if ret:
            return ret

    elif args[0] == 'fan':
        if int(args[1]) > 100:
            show_set_help()
            return

        node = ALL_DEVICE['fan']['fan1'][0]
        node = node.replace(node.split('/')[-1],
                            'fan_duty_cycle_percentage')
        ret, log = read_file_contents(node)
        if ret == 0:
            print('Previous fan duty: ' + log.strip() + '%')
        ret, log = echo_to_file(args[1], node)
        if ret == 0:
            print('Current fan duty: ' + args[1] + '%')
        return ret
    elif args[0] == 'sfp':
        if len(args) < 2 or int(args[1]) == 0 or int(args[2]) > 1:
            show_set_help()
            return

        for j in ALL_DEVICE[args[0]][args[0] + str(args[1])]:
            if j.find('module_lpmode') != -1:
                ret, log = echo_to_file(args[2], j)
                if ret:
                    return ret
    return

def set_specific_led(led, mode):
    for i in range(0, len(ALL_DEVICE['led'])):
        for k in ALL_DEVICE['led']['led' + str(i + 1)]:
            if led in k:
                ret, log = echo_to_file(mode, k)
                return ret

# Extract digits from string
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
                (ret, log) = run_command('cat ' + k, 0)
                func = k.split('/')[-1].strip()
                func = re.sub(j + '_', '', func, 1)
                func = re.sub(i.lower() + '_', '', func, 1)
                if ret == 0:
                    print(func + '=' + log + ' ',)
                else:
                    print(func + '=' + 'X' + ' ',)
            print()
        print()
    return

def device_exist():
    return os.path.exists("{}1-0072".format(i2c_prefix))

if __name__ == '__main__':
    main()
