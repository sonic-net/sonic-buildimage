#
# Arista eeprom processing for SONiC
# Uses the arista driver library to obtain the TlvInfoDecoder
#

try:
    import arista.utils.sonic_eeprom as arista_eeprom
except ImportError as e:
    raise ImportError("%s - required module not found" % e)

board = arista_eeprom.getTlvInfoDecoder()
