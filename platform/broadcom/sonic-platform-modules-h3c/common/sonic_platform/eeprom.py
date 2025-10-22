#!/usr/bin/env python
"""
Version: 1.0

H3C B40X0
Module contains an implementation of SONiC Platform Base API and
provides the 'Syseeprom' information which are available in the platform
"""

try:
    from sonic_platform_base.sonic_eeprom.eeprom_tlvinfo import TlvInfoDecoder
    from vendor_sonic_platform.devcfg import Devcfg
except ImportError as error:
    raise ImportError(str(error) + "- required module not found")


class Eeprom(TlvInfoDecoder):
    """Platform-specific EEPROM class"""

    def __init__(self):
        super(Eeprom, self).__init__(Devcfg.EEPROM_DATA_DIR, 0, '', True)
