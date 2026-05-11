#!/usr/bin/env python3
"""
DELLEMC

 This file contains helper routines to load SFP EEPROM drivers during bootup
 for all platforms.
  1) Currently Z9432 and Z9332 platforms will be using this script to load
     media drivers by reading SFP type. By Default, QSFP_DD "optoe3" driver
     is loaded when no SFP is present
  2) In Future, for other platforms/newer SFPs (For e.g. SFP56-DD) these
     routines can be extended to add SFP Types in media_xlate & media_driver
     dictionary
  Refer https://github.com/opencomputeproject/oom/blob/master/optoe/optoe.txt
  for more details about optoe driver
"""
try:
    import sys
    import syslog
    import subprocess
    import errno
    from contextlib import closing
    from smbus2 import SMBus

except ImportError as error:
    raise ImportError(str(error) + "- required module not found")

def get_media_type(sfp_type):
    """
    Translates SFP Type to Transceiver Form Factor
    """
    media_xlate = {
        # SFP/SFP+/SFP28 and later
        0x03 : 'SFP',
        # QSFP/QSFP+/QSFP28 and later
        0x0c : 'QSFP', 0x0d : 'QSFP', 0x11 : 'QSFP',
        # QSFP_DD
        0x18 : 'QSFP_DD',
        # SFP_DD
        0x1a : 'SFP_DD',
        # OSFP
        0x19 : 'OSFP'
    }

    media_type = media_xlate[sfp_type] if sfp_type in media_xlate else 'None'
    return media_type

def get_sfp_type_from_raw_eeprom(i2c_bus):
    """
    Takes I2Bus number as input and return SFP Type from the EEPROM
    """
    address = 0x50
    offset = 0
    sfp_type = 'None'

    try:
        with closing(SMBus(i2c_bus)) as bus:
            sfp_type = bus.read_byte_data(address, offset, force=True)
    except OSError as error:
        if error.args[0] != errno.EIO and error.args[0] != errno.ETIMEDOUT and  error.args[0] != errno.EPERM:
            syslog.syslog(syslog.LOG_ERR,
                          "Error : SFP Type get failed for I2C Bus %s" % i2c_bus)
    return sfp_type

def load_media_driver(i2c_bus, media_type, native_type):
    """
    For All non Native SFP, load their respective drivers based on their type.
    By default, select the Native driver based on the platform
    """

    sfp_path = "/sys/bus/i2c/devices/i2c-{0}/new_device".format(i2c_bus)
    # Add new SFP Form Factors & Drivers in this dictionary & update media_xlate table
    # as well
    media_driver = {'SFP-CU' : 'copper', 'SFP' : 'optoe2', 'QSFP' : 'optoe1',
            'QSFP_DD' : 'optoe3', 'SFP_DD' : 'optoe3', 'OSFP' : "optoe3"}

    if media_type == 'None':
        #Currently Z9432 and Z9332 platforms will be using this script to load drivers
        #so using QSFP_DD 400G Native driver "optoe3" by default
        driver_type = native_type
    else:
        driver_type = media_driver[media_type]

    #Create new device with inserted SFP media driver
    driver_install_cmd = "{0} 0x50".format(driver_type)
    try:
        with open(sfp_path, 'w') as file_obj:
            file_obj.write(driver_install_cmd)
    except IOError as error:
        syslog.syslog(syslog.LOG_ERR,
                      "Error : Driver load failed for I2C bus %s Errno %d" %
                      (i2c_bus, error.errno))
    return True

def load_non_native_drivers(index, native_type):
    """
    Takes I2Bus number as input and load driver based on SFP type presence
    """
    sfp_type = get_sfp_type_from_raw_eeprom(index)
    media_type = get_media_type(sfp_type)
    load_media_driver(index, media_type, native_type)

if __name__ == "__main__":
    idx_range = sys.argv[1].split(':')
    for idx in range(int(idx_range[0]), int(idx_range[1])+1):
        load_non_native_drivers(idx, sys.argv[2])
