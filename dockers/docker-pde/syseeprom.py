#!/usr/bin/python

import sys
import imp

PLATFORM_SPECIFIC_MODULE_NAME = "eeprom"
PLATFORM_SPECIFIC_CLASS_NAME = "board"

platform_eeprom = None
platform_eeprom_data = None

# Returns path to platform and hwsku
def get_path_to_platform_and_hwsku():
    platform_path = '/usr/share/sonic/platform'
    hwsku_path = '/usr/share/sonic/hwsku'
    return (platform_path, hwsku_path)

# Loads platform specific psuutil module from source
def load_platform_util(module_name, class_name):
    platform_util = None

    # Get path to platform and hwsku
    (platform_path, hwsku_path) = get_path_to_platform_and_hwsku()

    try:
        module_file = "/".join([platform_path, "plugins", module_name + ".py"])
        module = imp.load_source(module_name, module_file)
    except IOError, e:
        assert False, ("Failed to load platform module '%s': %s" % (module_name, str(e)))

    try:
        platform_util_class = getattr(module, class_name)
        # board class of eeprom requires 4 paramerters, need special treatment here.
        platform_util = platform_util_class('','','','')
    except AttributeError, e:
        assert False, ("Failed to instantiate '%s' class: %s" % (class_name, str(e)))

    return platform_util

def init_platform_eeprom():
    global platform_eeprom
    global platform_eeprom_data

    if platform_eeprom is None:
        platform_eeprom = load_platform_util(PLATFORM_SPECIFIC_MODULE_NAME, \
                                             PLATFORM_SPECIFIC_CLASS_NAME)
    if platform_eeprom_data is None:
        platform_eeprom_data = platform_eeprom.read_eeprom()

    return platform_eeprom_data

def main():
    e = init_platform_eeprom()
    c = platform_eeprom._TLV_CODE_PLATFORM_NAME
    if len(sys.argv) > 1:
        c = int(sys.argv[1], 0)
    v, t = platform_eeprom.get_tlv_field(e, c)
    if v:
        print(t[2])
    else:
        print("Unknown")

if __name__ == '__main__':
    main()

