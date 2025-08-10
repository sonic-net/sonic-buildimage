#!/usr/bin/env python

"""
Arista 7050CX3-32S eeprom plugin
Uses the arista driver library to obtain the TlvInfoDecoder
"""

try:
    import arista.utils.sonic_eeprom as arista_eeprom
except ImportError, e:
    raise ImportError (str(e) + "- required module not found")

board = arista_eeprom.getTlvInfoDecoder()
