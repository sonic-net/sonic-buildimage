#!/usr/bin/python
import os
import commands
import sys, getopt
import logging
import time

DEBUG = False
SLOT_STATUS_CURRENT_FILE = '/sys/class/hwmon/hwmon2/device/ESC600_Module/module_insert'
INSERT_CARD_MODEL_FILE = '/sys/bus/i2c/devices/{0}-0032/model'
# SLOT_STATUS_CURRENT_FILE = 'module_insert'
# INSERT_CARD_MODEL_FILE = 'slot_model'
PATH_73_BUS_BASE = 1
PATH_77_BUS_BASE = 9
PATH_71_BUS_BASE = 33

channel_path_73 = '/sys/bus/i2c/devices/i2c-{0}/new_device'


def my_log(txt):
    if DEBUG == True:
        print("[ROY]" + txt)
    return


'''
def do_cmd(cmd, show):
    my_log(cmd)
'''


def do_cmd(cmd, show):
    logging.info('Run :' + cmd)
    status, output = commands.getstatusoutput(cmd)
    my_log(cmd + " with result:" + str(status))
    my_log("      output:" + output)
    if status:
        logging.info('Failed :' + cmd)
        if show:
            print('Failed :' + cmd)
    return status, output


def get_attr_value(attr_path):
    retval = 'ERR'
    if not os.path.isfile(attr_path):
        return retval

    try:
        with open(attr_path, 'r') as fd:
            retval = fd.read()
    except Exception as error:
        logging.error("Unable to open ", attr_path, " file !")

    retval = retval.rstrip('\r\n')
    fd.close()
    return retval


def bmc_is_exist():
    bmc_filePath = '/sys/class/hwmon/hwmon2/device/ESC600_SYS/bmc_present'
    if os.path.exists(bmc_filePath):
        value = get_attr_value(bmc_filePath)
        if value.find('not') < 0:
            return True
        else:
            return False
    else:
        return False


def remove_devices_73():
    for slot_id in range(0,8):
        if os.path.isfile('/sys/bus/i2c/devices/{0}-0032/name'.format(slot_id+PATH_73_BUS_BASE)):
            cmd = 'echo 0x32 >/sys/bus/i2c/devices/i2c-{0}/delete_device'.format(slot_id+PATH_73_BUS_BASE)
            do_cmd(cmd,0)
        if os.path.isfile('/sys/bus/i2c/devices/{0}-0071/name'.format(slot_id + PATH_73_BUS_BASE)):
            cmd = 'echo 0x71 >/sys/bus/i2c/devices/i2c-{0}/delete_device'.format(slot_id+PATH_73_BUS_BASE)
            do_cmd(cmd, 0)
        if os.path.isfile('/sys/bus/i2c/devices/{0}-0072/name'.format(slot_id + PATH_73_BUS_BASE)):
            cmd = 'echo 0x72 >/sys/bus/i2c/devices/i2c-{0}/delete_device'.format(slot_id+PATH_73_BUS_BASE)
            do_cmd(cmd, 0)


def remove_devices_77():
    for slot_id in range(0, 8):
        if os.path.isfile('/sys/bus/i2c/devices/{0}-004c/name'.format(slot_id + PATH_77_BUS_BASE)):
            cmd = 'echo  0x4c >/sys/bus/i2c/devices/i2c-%d/delete_device' % (slot_id+PATH_77_BUS_BASE)
            do_cmd(cmd, 0)
        if os.path.isfile('/sys/bus/i2c/devices/{0}-0011/name'.format(slot_id + PATH_77_BUS_BASE)):
            cmd = 'echo  0x11 >/sys/bus/i2c/devices/i2c-%d/delete_device' % (slot_id+PATH_77_BUS_BASE)
            do_cmd(cmd, 0)
        if os.path.isfile('/sys/bus/i2c/devices/{0}-0012/name'.format(slot_id + PATH_77_BUS_BASE)):
            cmd = 'echo  0x12 >/sys/bus/i2c/devices/i2c-%d/delete_device' % (slot_id+PATH_77_BUS_BASE)
            do_cmd(cmd, 0)
        if os.path.isfile('/sys/bus/i2c/devices/{0}-0013/name'.format(slot_id + PATH_77_BUS_BASE)):
            cmd = 'echo  0x13 >/sys/bus/i2c/devices/i2c-%d/delete_device' % (slot_id+PATH_77_BUS_BASE)
            do_cmd(cmd, 0)


def sfp_module_100g(slot_id, bus_no):
    cmd = 'echo pca9548 0x71 > %s' % channel_path_73.format(PATH_73_BUS_BASE+slot_id)
    do_cmd(cmd, 0)
    cmd = 'echo pca9548 0x72 > %s' % channel_path_73.format(PATH_73_BUS_BASE+slot_id)
    do_cmd(cmd, 0)
    for i in range(bus_no,bus_no+16):
        cmd = 'echo optoe1 0x50 >/sys/bus/i2c/devices/i2c-%d/new_device' % i
        do_cmd(cmd,0)
        # fd.write("/sys/bus/i2c/devices/{0}-0050/eeprom\n".format(i))
    return bus_no+16


def sfp_module_400g(slot_id, bus_no):
    cmd = 'echo pca9548 0x71 > %s' % channel_path_73.format(PATH_73_BUS_BASE+slot_id)
    do_cmd(cmd, 0)
    for i in range(bus_no,bus_no+4):
        cmd = 'echo optoe1 0x50 >/sys/bus/i2c/devices/i2c-%d/new_device' % i
        do_cmd(cmd,0)
        # fd.write("/sys/bus/i2c/devices/{0}-0050/eeprom\n".format(i))
    return bus_no+8

def install_slot_73_cpld_driver(slot_id):
    # install 0x32 cpld driver first, we need it to read the card model(100G or 400G)
    cmd = 'echo phy_cpld640 0x32 > %s' % channel_path_73.format(PATH_73_BUS_BASE+slot_id)
    do_cmd(cmd, 0)
    time.sleep(0.1)

def install_slot_73_sfp(slot_id, bus_no):
    count= 0
    timeout_count = 50
    path = INSERT_CARD_MODEL_FILE.format(PATH_73_BUS_BASE+slot_id)
    # path = INSERT_CARD_MODEL_FILE
    while True:
        if os.path.exists(path):
            break
        count += 1
        if count > timeout_count:
            print("detect {} timeout".format(path))
            sys.exit()
        time.sleep(0.2)

    with open(path, 'r') as fd:
        text_lines = fd.readlines()
        for line in text_lines:
            if "100G" in line:
                bus_no = sfp_module_100g(slot_id, bus_no)
            if "400G" in line:
                bus_no = sfp_module_400g(slot_id, bus_no)
    return bus_no


def sensors_on_card(slot_id, is_100G):
    cmd = 'echo g781 0x4c >/sys/bus/i2c/devices/i2c-%d/new_device' % (slot_id+PATH_77_BUS_BASE)
    do_cmd(cmd, 0)
    cmd = 'echo tps40425 0x11 >/sys/bus/i2c/devices/i2c-%d/new_device' % (slot_id+PATH_77_BUS_BASE)
    do_cmd(cmd, 0)
    if is_100G:
        cmd = 'echo tps40425 0x12 >/sys/bus/i2c/devices/i2c-%d/new_device' % (slot_id+PATH_77_BUS_BASE)
        do_cmd(cmd, 0)
    cmd = 'echo tps40425 0x13 >/sys/bus/i2c/devices/i2c-%d/new_device' % (slot_id+PATH_77_BUS_BASE)
    do_cmd(cmd, 0)


def install_slot_77(slot_id):
    # Path already generate by install_73(73 must first install)
    path = INSERT_CARD_MODEL_FILE.format(PATH_73_BUS_BASE+slot_id)
    # path = INSERT_CARD_MODEL_FILE
    with open(path, 'r') as fd:
        text_lines = fd.readlines()
        for line in text_lines:
            if "100G" in line:
                sensors_on_card(slot_id, True)
            if "400G" in line:
                sensors_on_card(slot_id, False)


def install_card_devices(bmc_exist):
    global PATH_71_BUS_BASE
    # adjust bus base
    if bmc_exist:
        PATH_71_BUS_BASE = 9
    bus_no = PATH_71_BUS_BASE
    with open(SLOT_STATUS_CURRENT_FILE, 'r') as file_stream:
        text_lines = file_stream.readlines()
        for line in text_lines:
            # got index from 1 we need from 0 ==> int(filter(str.isdigit, line))-1
            # install cpld driver for all slot
            install_slot_73_cpld_driver(int(filter(str.isdigit, line))-1)
       
        time.sleep(0.5) # add delay to wait cpld sysfile ready
        for line in text_lines:
            if "is present" in line:
                # install only the present one
                # install_slot_73_cpld_driver(int(filter(str.isdigit, line))-1)
                
                bus_no = install_slot_73_sfp(int(filter(str.isdigit, line))-1, bus_no)
                if not bmc_exist:
                    install_slot_77(int(filter(str.isdigit, line))-1)


def remove_card_devices(bmc_exist):
    remove_devices_73()
    if not bmc_exist:
        remove_devices_77()


def main():
    global DEBUG
    global args
    options, args = getopt.getopt(sys.argv[1:], 'd', ['debug'])
    bmc_exist = bmc_is_exist()
    for opt, arg in options:
        if opt in ('-d', '--debug'):
            DEBUG = True
            logging.basicConfig(level=logging.INFO)
        else:
            logging.info('no option')
    for arg in args:
        if arg == 'install':
            # remove devices first
            remove_card_devices(bmc_exist)
            install_card_devices(bmc_exist)
        elif arg == 'clean':
            remove_card_devices(bmc_exist)
        else:
            return


if __name__ == "__main__":
    main()
