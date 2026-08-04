#!/usr/bin/env python

#############################################################################
# Centec V682-48Y8C
#
# Platform and model specific eeprom subclass, inherits from the base class,
# and provides the followings:
# - the eeprom format definition
# - specific encoder/decoder if there is special need
#############################################################################

try:
    import os
    from sonic_eeprom import eeprom_tlvinfo
    from sonic_py_common import device_info
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

USR_SHARE_SONIC_PATH = "/usr/share/sonic"
HOST_DEVICE_PATH = USR_SHARE_SONIC_PATH + "/device"
CONTAINER_PLATFORM_PATH = USR_SHARE_SONIC_PATH + "/platform"

class board(eeprom_tlvinfo.TlvInfoDecoder):

    def __init__(self, name, path, cpld_root, ro):
        if os.path.isdir(CONTAINER_PLATFORM_PATH):
            platform_path = CONTAINER_PLATFORM_PATH
        else:
            platform = device_info.get_platform()
            if platform is None:
                raise
            platform_path = os.path.join(HOST_DEVICE_PATH, platform)

        self.eeprom_path = platform_path + '/eeprom_file'
        super(board, self).__init__(self.eeprom_path, 0, '', True)
