#!/usr/bin/python3

import os
import sys
import importlib.machinery


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
        if 'aboot_platform' in machine_info:
            return machine_info['aboot_platform']
    return None


PLATFORM_ROOT_PATH = '/usr/share/sonic/device'
PLATFORM_SPECIFIC_MODULE_NAME = 'monitor'
PLATFORM_SPECIFIC_CLASS_NAME = 'status'
platform_status_class = None
platform = None


def get_platform_name():
    global platform
    platform = get_platform_info(get_machine_info())
    return platform


val = get_platform_name()
sys.path.append("/".join([PLATFORM_ROOT_PATH, platform]))

# Loads platform specific sfputil module from source


def load_platform_monitor():
    global platform_status_class
    platform_name = get_platform_info(get_machine_info())
    platform_path = "/".join([PLATFORM_ROOT_PATH, platform_name])
    try:
        module_file = "/".join([platform_path, PLATFORM_SPECIFIC_MODULE_NAME + ".py"])
        module = importlib.machinery.SourceFileLoader(PLATFORM_SPECIFIC_MODULE_NAME, module_file).load_module()
    except IOError:
        return -1
    try:
        platform_status_class = getattr(module, PLATFORM_SPECIFIC_CLASS_NAME)
    except AttributeError:
        return -2
    return 0


def printerr(msg):
    print("\033[0;31m%s\033[0m" % msg)


def print_console(msg):
    print(msg)


val_t = load_platform_monitor()
if val_t != 0:
    raise Exception("load monitor.py error")


def print_platform():
    platform_info = get_platform_name()
    print_console(platform_info)
    print_console("")


def print_cputemp_sensors():
    val_ret = get_call_value_by_function("getcputemp")
    print_info_str = ""
    toptile = "Onboard coretemp Sensors:"
    formatstr = "    {name:<20} : {temp} C (high = {max} C , crit = {crit} C )"

    if len(val_ret) != 0:
        print_info_str += toptile + '\n'
        for item in val_ret:
            print_info_str += formatstr.format(**item) + '\n'
    print_console(print_info_str)


def print_boardtemp():
    val_ret = get_call_value_by_function("getTemp")
    print_info_str = ""
    toptile = "Onboard Temperature Sensors:"
    errformat = "    {id:<20} : {errmsg}"
    formatstr = "    {id:<20} : {temp1_input} C (high = {temp1_max} C, hyst = {temp1_max_hyst} C)"

    if len(val_ret) != 0:
        print_info_str += toptile + '\n'
        for item in val_ret:
            realformat = formatstr if item.get('errcode', 0) == 0 else errformat
            print_info_str += realformat.format(**item) + '\n'
    print_console(print_info_str)


def print_mactemp_sensors():
    val_ret = get_call_value_by_function("getmactemp")
    print_info_str = ""
    toptile = "Onboard MAC Temperature Sensors:"
    errformat = "    {id:<20} : {errmsg}"
    formatstr = "    {id:<20} : {temp_input} C"

    if len(val_ret) != 0:
        print_info_str += toptile + '\n'
        for item in val_ret:
            realformat = formatstr if item.get('errcode', 0) == 0 else errformat
            print_info_str += realformat.format(**item) + '\n'
        print_console(print_info_str)


def print_macpower_sensors():
    val_ret = get_call_value_by_function("getmacpower")
    print_info_str = ""
    toptile = "Onboard MAC Power Sensors:"
    errformat = "    {id:<20} : {errmsg}"
    formatstr = "    {id:<20} : {power_input} W"

    if len(val_ret) != 0:
        print_info_str += toptile + '\n'
        for item in val_ret:
            realformat = formatstr if item.get('errcode', 0) == 0 else errformat
            print_info_str += realformat.format(**item) + '\n'
        print_console(print_info_str)


def print_fan_sensor():
    val_ret = get_call_value_by_function("checkFan")
    print_info_str = ""
    toptile = "Onboard fan Sensors:"
    errformat = "    {id} : {errmsg}\n"  # "    {id:<20} : {errmsg}"
    fan_signle_rotor_format = "    {id} : \n"  \
        "        fan_type  :{fan_type}\n"  \
        "        sn        :{sn}\n"  \
        "        hw_version:{hw_version}\n"  \
        "        Speed     :{Speed} RPM\n"     \
        "        status    :{errmsg} \n"
    fan_double_rotor_format = "    {id} : \n"  \
        "        fan_type  :{fan_type}\n"  \
        "        sn        :{sn}\n"  \
        "        hw_version:{hw_version}\n"  \
        "        Speed     :\n"     \
        "            speed_front :{rotor1_speed:<5} RPM\n"     \
        "            speed_rear  :{rotor2_speed:<5} RPM\n"     \
        "        status    :{errmsg} \n"

    if len(val_ret) != 0:
        print_info_str += toptile + '\n'
        for item in val_ret:
            if item.get('Speed', None) is None:
                realformat = fan_double_rotor_format if item.get('errcode', 0) == 0 else errformat
            else:
                realformat = fan_signle_rotor_format if item.get('errcode', 0) == 0 else errformat
            print_info_str += realformat.format(**item)
    print_console(print_info_str)


def print_psu_sensor():
    val_ret = get_call_value_by_function("getPsu")
    print_info_str = ""
    toptile = "Onboard Power Supply Unit Sensors:"
    errformat = "    {id} : {errmsg}\n"  # "    {id:<20} : {errmsg}"
    psuformat = "    {id} : \n"  \
                "        type       :{type1}\n"  \
                "        sn         :{sn}\n"  \
                "        in_current :{in_current} A\n"  \
                "        in_voltage :{in_voltage} V\n"     \
                "        out_current:{out_current} A\n"     \
                "        out_voltage:{out_voltage} V\n"     \
                "        temp       :{temp} C        \n"     \
                "        fan_speed  :{fan_speed} RPM\n"     \
                "        in_power   :{in_power} W\n"        \
                "        out_power  :{out_power} W\n"

    if len(val_ret) != 0:
        print_info_str += toptile + '\r\n'
        for item in val_ret:
            realformat = psuformat if item.get('errcode', 0) == 0 else errformat
            print_info_str += realformat.format(**item)
    print_console(print_info_str)


def print_slot_sensor():
    val_ret = get_call_value_by_function("checkSlot")
    print_info_str = ""
    toptile = "Onboard slot Sensors:"
    errformat = "    {id} : {errmsg}\n"  # "    {id:<20} : {errmsg}"
    psuformat = "    {id} : \n"  \
                "        slot_type  :{slot_type}\n"  \
                "        sn         :{sn}\n"  \
                "        hw_version :{hw_version} \n"  \
                "        status     :{errmsg}\n"

    if len(val_ret) != 0:
        print_info_str += toptile + '\r\n'
        for item in val_ret:
            realformat = psuformat if item.get('errcode', 0) == 0 else errformat
            print_info_str += realformat.format(**item)
        print_console(print_info_str)


def print_boarddcdc():
    val_ret = get_call_value_by_function("getDcdc")
    print_info_str = ""
    toptile = "Onboard DCDC Sensors:"
    errformat = "    {id:<26} : {errmsg}"
    formatstr = "    {id:<26} : {dcdc_input:<6} {dcdc_unit:<1} (Min = {dcdc_min:<6} {dcdc_unit:<1}, Max = {dcdc_max:<6} {dcdc_unit:<1})"

    if len(val_ret) != 0:
        print_info_str += toptile + '\n'
        for item in val_ret:
            realformat = formatstr if item.get('errcode', 0) == 0 else errformat
            print_info_str += realformat.format(**item) + '\n'
        print_console(print_info_str)


def get_call_value_by_function(function_name):
    valtemp = []
    if hasattr(platform_status_class, function_name):
        test2_func = getattr(platform_status_class, function_name)
        test2_func(valtemp)
    return valtemp


def getsensors():
    print_platform()
    print_cputemp_sensors()
    print_boardtemp()
    print_mactemp_sensors()
    print_macpower_sensors()
    print_fan_sensor()
    print_psu_sensor()
    print_slot_sensor()
    print_boarddcdc()


if __name__ == "__main__":
    getsensors()
