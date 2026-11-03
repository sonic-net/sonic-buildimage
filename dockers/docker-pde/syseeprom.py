#!/usr/bin/env python3

import contextlib
import importlib.util
import io
import sys

from sonic_platform_base.sonic_eeprom.eeprom_tlvinfo import TlvInfoDecoder

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
        spec = importlib.util.spec_from_file_location(module_name, module_file)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
    except IOError as e:
        assert False, ("Failed to load platform module '%s': %s" % (module_name, str(e)))

    try:
        platform_util_class = getattr(module, class_name)
        # board class of eeprom requires 4 paramerters, need special treatment here.
        platform_util = platform_util_class('','','','')
    except AttributeError as e:
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

# Platform API 2.0: the chassis returns the TLV fields keyed by "0x<code>".
# Returns (True, value or None) when the platform provides the API, else (False, None).
def get_tlv_from_platform_api(code):
    try:
        # Keep platform init messages out of the value printed on stdout. The import is the
        # API 2.0 check, so it stays here: a platform without sonic_platform falls back.
        with contextlib.redirect_stdout(io.StringIO()):
            from sonic_platform.platform import Platform
            info = Platform().get_chassis().get_system_eeprom_info()
    except Exception:
        return (False, None)
    return (True, (info or {}).get("0x%02X" % code))

def main():
    c = TlvInfoDecoder._TLV_CODE_PLATFORM_NAME
    if len(sys.argv) > 1:
        c = int(sys.argv[1], 0)

    available, value = get_tlv_from_platform_api(c)
    if available:
        print(value if value is not None else "Unknown")
        return

    # Platform API 1.0 plugins
    e = init_platform_eeprom()
    v, t = platform_eeprom.get_tlv_field(e, c)
    if v:
        value = t[2]
        print(value.decode('ascii', 'replace') if isinstance(value, (bytes, bytearray)) else value)
    else:
        print("Unknown")

if __name__ == '__main__':
    main()

