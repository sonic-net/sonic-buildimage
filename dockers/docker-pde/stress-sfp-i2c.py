#!/usr/bin/env python

try:
    import sys
    import time
    import imp
    from natsort import natsorted
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))

HWSKU_PATH = "/usr/share/sonic/hwsku"
PLATFORM_PATH = "/usr/share/sonic/platform"

PLATFORM_SPECIFIC_MODULE_NAME = 'sfputil'
PLATFORM_SPECIFIC_CLASS_NAME = 'SfpUtil'

XCVR_EEPROM_TYPE_UNKNOWN = 0
XCVR_EEPROM_TYPE_SFP = 1
XCVR_EEPROM_TYPE_QSFP = 2
XCVR_EEPROM_TYPE_QSFPDD = 3
XCVR_EEPROM_TYPE_OSFP = XCVR_EEPROM_TYPE_QSFPDD

OSFP_TYPE_ID = "18"

# Global platform-specific sfputil class instance
platform_sfputil = None

# Global port dictionaries
port_dict = None

# Loads platform specific sfputil module from source
def load_platform_sfputil():
    global platform_sfputil

    if platform_sfputil is not None:
        return

    try:
        module_file = "/".join([PLATFORM_PATH, "plugins", PLATFORM_SPECIFIC_MODULE_NAME + ".py"])
        module = imp.load_source(PLATFORM_SPECIFIC_MODULE_NAME, module_file)
    except IOError, e:
        print("Failed to load platform module '%s': %s" % (PLATFORM_SPECIFIC_MODULE_NAME, str(e)), True)

    assert module is not None

    try:
        platform_sfputil_class = getattr(module, PLATFORM_SPECIFIC_CLASS_NAME)
        platform_sfputil = platform_sfputil_class()
    except AttributeError, e:
        print("Failed to instantiate '%s' class: %s" % (PLATFORM_SPECIFIC_CLASS_NAME, str(e)), True)

    assert platform_sfputil is not None
    return

# Loads platform port dictionaries
def load_platform_portdict():
    global port_dict

    if port_dict is not None:
        return

    port_dict = {}
    idx = 0
    file = open(HWSKU_PATH + "/port_config.ini", "r")
    line = file.readline()
    while line is not None and len(line) > 0:
        line = line.strip()
        if line.startswith("#"):
            line = file.readline()
            continue
        list = line.split()
        if len(list) >= 4:
            idx = int(list[3])
        port_dict[list[0]] = { "index": str(idx) }
        idx += 1
        line = file.readline()

    return port_dict

def get_sfp_eeprom_type(port, data):
    type = XCVR_EEPROM_TYPE_UNKNOWN

    if (port in platform_sfputil.osfp_ports) or (port in platform_sfputil.qsfp_ports):
        if data is None:
            return type

        if data[0] in OSFP_TYPE_ID:
            code = 0
            for i in range(128, 222):
                code += int(data[i], 16)
            if (code & 0xff) == int(data[222], 16):
                type = XCVR_EEPROM_TYPE_OSFP
            else:
                type = XCVR_EEPROM_TYPE_QSFP
        else:
            type = XCVR_EEPROM_TYPE_QSFP
    else:
        type = XCVR_EEPROM_TYPE_SFP

    return type

# Test for SFP EEPROM
def stress_sfp_i2c_one():
    load_platform_sfputil()
    load_platform_portdict()
    num_sfp = 0
    for intf in natsorted(port_dict.keys()):
        port = int(port_dict[intf]['index'])
        if not platform_sfputil._is_valid_port(port):
            continue
        if platform_sfputil.get_presence(port):
            num_sfp += 1

    assert num_sfp >= 2, "2 or more SFP modules should be attached for this test"

    for intf in natsorted(port_dict.keys()):
        port = int(port_dict[intf]['index'])
        if not platform_sfputil._is_valid_port(port):
            continue
        if not platform_sfputil.get_presence(port):
            continue

        data = platform_sfputil.get_eeprom_raw(port, 256)
        assert data is not None, "SFP{}: unable to read EEPROM".format(port)

        code = 0
        type = get_sfp_eeprom_type(port, data)
        if type == XCVR_EEPROM_TYPE_QSFPDD:
            for i in range(128, 222):
                code += int(data[i], 16)
            assert (code & 0xff) == int(data[222], 16), "{}: check code error".format(intf)
        elif type == XCVR_EEPROM_TYPE_QSFP:
            for i in range(128, 191):
                code += int(data[i], 16)
            assert (code & 0xff) == int(data[191], 16), "{}: check code error".format(intf)
        else:
            for i in range(0, 63):
                code += int(data[i], 16)
            assert (code & 0xff) == int(data[63], 16), "{}: check code error".format(intf)

def stress_sfp_i2c(sec=180):
    print("Initiating {} seconds SFP I2C stress test...".format(sec))
    timeout = time.time() + sec
    while timeout >= time.time():
        stress_sfp_i2c_one()
        sys.stdout.write("#")
        sys.stdout.flush()
    print("\nPASS")

if __name__ == '__main__':
    sec = 180
    if len(sys.argv) >= 2:
        sec = int(sys.argv[1])
    stress_sfp_i2c(sec)
