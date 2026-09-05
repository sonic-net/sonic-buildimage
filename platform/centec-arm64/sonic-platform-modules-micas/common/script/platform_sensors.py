#!/usr/bin/python3
#   * onboard  sensors
#
import os
import sys
from sonic_platform.redfish_api import Redfish_Api

def get_machine_info():
    if not os.path.isfile('/host/machine.conf'):
        return None
    machine_vars = {}
    with open('/host/machine.conf') as machine_file:
        for line in machine_file:
            tokens = line.split('=')
            if len(tokens) < 2:
                continue
            machine_vars[tokens[0]] = tokens[1].strip()
    return machine_vars

def get_platform_info(machine_info):
    if machine_info is not None:
        if 'onie_platform' in machine_info:
            return machine_info['onie_platform']
        elif 'aboot_platform' in machine_info:
            return machine_info['aboot_platform']
    return None

PLATFORM_ROOT_PATH = '/usr/share/sonic/device'
platform = None

def get_platform_name():
    global platform
    platform = get_platform_info(get_machine_info())
    return platform

val = get_platform_name()
sys.path.append("/".join([PLATFORM_ROOT_PATH, platform]))

def print_console(str):
    print(str)

def print_platform():
    platform_info = get_platform_name()
    print_console(platform_info)
    print_console('Adapter: Platform Management Controller')
    print_console("")

def get_sensor():
    sensor = Redfish_Api().get_thresholdSensors()
    ctrl = sensor["Sensors"]
    list_sensor =[]
    for item in ctrl:
        name = item.get("@odata.id").split("/",9)[9]
        now = item.get("Reading")
        min = item.get("Thresholds").get("LowerFatal").get("Reading")
        max = item.get("Thresholds").get("UpperFatal").get("Reading")
        unit = item.get("ReadingUnits")
        if unit == "Amps":
            unit = "A"
            if min == (-1000):
                min = (min/1000)
        elif unit == "Volts":
            unit = "V"
        tmp = {}
        tmp["name"]= name
        tmp["now"] = ("%.3f" % now)
        tmp["min"] = ("%.3f" % min)
        tmp["max"] = ("%.3f" % max)
        tmp["unit"] = unit
        list_sensor.append(tmp)
    return list_sensor

def print_boarddcdc():
    val_ret = get_sensor()
    print_info_str = ""
    toptile = "Onboard Sensors:"
    errformat = "    {id:<26} : {errmsg}"
    formatstr = "    {name:<26} : {now:<6} {unit:<1}  (Min = {min:<6} {unit:<1} , Max = {max:<6} {unit:<1} )"

    if len(val_ret) != 0:
        print_info_str += toptile + '\n'
        for item in val_ret:
            realformat = formatstr if item.get('errcode', 0) == 0 else errformat
            print_info_str += realformat.format(**item) + '\n'
        print_console(print_info_str)

def getsensors():
    print_platform()
    print_boarddcdc()

if __name__ == "__main__":
    getsensors()
