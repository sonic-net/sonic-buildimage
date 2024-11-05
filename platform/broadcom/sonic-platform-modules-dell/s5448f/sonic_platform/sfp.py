#!/usr/bin/env python3
"""
#############################################################################
# DELLEMC S5448F
#
# Module contains an implementation of SONiC Platform Base API and
# provides the platform information
#
#############################################################################
"""

try:
    import sys
    import time
    import syslog
    from sonic_platform_base.sfp_standard import SfpStandard
    from .sfp_helper import SfphelperStandard
    from sonic_platform_base.sonic_sfp.qsfp_dd import qsfp_dd_InterfaceId
    from sonic_platform_base.sonic_sfp.qsfp_dd import qsfp_dd_Dom

except ImportError as err:
    raise ImportError(str(err) + "- required module not found")

BASE_OFFSET = 16388
RESET_BASE_OFFSET = 16384
INDEX_1 = 1
INDEX_2 = 3
INDEX_3 = 47
HEX_10 = 16
HEX_20 = 32
PORT_START = 1
PORT_6 = 6
PORT_54 = 54
PORT_END = 58
QSFP_DD_DOM_CAPABILITY_OFFSET = 2
QSFP_DD_DOM_CAPABILITY_WIDTH = 1
SFP_PORT_START = 5
SFP_PORT_END = 52
QSFP_QSA28_OUI_OFFSET = 64
QSFP_QSA28_OUI_WIDTH = 3

class Sfp(SfphelperStandard):
    """ DELLEMC Platform-specific Sfp class """
    BASE_RES_PATH = "/sys/bus/pci/devices/0000:04:00.0/resource0"

    def __init__(self, index, sfp_type, eeprom_path):
        SfphelperStandard.__init__(self, index, sfp_type, eeprom_path)
        self.qsfp_dd_info = qsfp_dd_InterfaceId()
        self.qsfp_dd_dom_info = qsfp_dd_Dom()
        self.qsfp_dd_app2_list = False

    @property
    def port_type(self):
        """ Returns native port type """
        if self.index >= SFP_PORT_START and self.index <= SFP_PORT_END:
            port_type = SfpStandard.PORT_TYPE_SFPDD
        else:
            port_type = SfpStandard.PORT_TYPE_QSFPDD
        return port_type

    def get_intl_state(self):
        """ Sets the intL (interrupt; active low) pin of this SFP """
        intl_state = True
        try:
            if self.sfp_type == 'QSFP_DD':
                # Port offset starts with 0x4004
                if self.index in range(PORT_START,PORT_6):
                    port_offset = BASE_OFFSET + ((self.index-INDEX_1) * HEX_10)
                elif self.index in range(PORT_6,PORT_54):
                    port_offset = BASE_OFFSET + ((self.index-INDEX_2) * HEX_20)
                elif self.index in range(PORT_54,PORT_END+1):
                    port_offset = BASE_OFFSET + ((self.index+INDEX_3) * HEX_10)
                else:
                    sys.stderr.write("Port index {} out of range (1-{})\n".format(self.index,PORT_END))

                status = self.pci_get_value(self.BASE_RES_PATH, port_offset)
                reg_value = int(status)

                # Mask off 4th bit for intL
                mask = (1 << 4)

                intl_state = (reg_value & mask)
        except  ValueError:
            pass
        return intl_state

    def _get_presence(self):
        # Check for invalid self.index
        mask = {'QSFP' : (1 << 4), 'SFP' : (1 << 0), 'QSFP_DD' : (1 << 4)}
        # Port offset starts with 0x4004
        if self.index in range(PORT_START,PORT_6):
            port_offset = BASE_OFFSET + ((self.index-INDEX_1) * HEX_10)
        elif self.index in range(PORT_6,PORT_54):
            port_offset = BASE_OFFSET + ((self.index-INDEX_2) * HEX_20)
        elif self.index in range(PORT_54,PORT_END+1):
            port_offset = BASE_OFFSET + ((self.index+INDEX_3) * HEX_10)
        else:
            sys.stderr.write("Port index {} out of range (1-{})\n".format(self.index,PORT_END))

        status = self.pci_get_value(self.BASE_RES_PATH, port_offset)
        reg_value = int(status)
        # ModPrsL is active low
        if (self.sfp_type in mask) and (reg_value & mask[self.sfp_type] == 0):
            return True
        return False

    def set_lpmode(self, lpmode):
        """
        Sets the lpmode(low power mode) of this SFP
        """
        try:
            if self.index in range(PORT_START,PORT_6):
                port_offset = RESET_BASE_OFFSET + ((self.index-INDEX_1) * HEX_10)
            elif self.index in range(PORT_6,PORT_54):
                port_offset = RESET_BASE_OFFSET + ((self.index-INDEX_2) * HEX_20)
            elif self.index in range(PORT_54,PORT_END+1):
                port_offset = RESET_BASE_OFFSET + ((self.index+INDEX_3) * HEX_10)
            else:
                syslog.syslog(syslog.LOG_ERR,"Port index {} out of range (1-{})\n".format(self.index,PORT_END))
                return False

            status = self.pci_get_value(self.BASE_RES_PATH, port_offset)
            reg_value = int(status)
            # Mask off 6th bit for lowpower mode
            mask = 1 << 6
            # LPMode is active high; set or clear the bit accordingly
            if lpmode is True:
                reg_value = reg_value | mask
            else:
                reg_value = reg_value & ~mask

            # Convert our register value back to a hex string and write back
            status = self.pci_set_value(self.BASE_RES_PATH, reg_value, port_offset)
        except ValueError as error:
            syslog.syslog(syslog.LOG_ERR, "Error: SFP lpmode set failed: %s" % str(error))
            return False

        return True

    def get_max_port_power(self):
        """ Retrieves the maximum power allowed on the port in watts ***
        This method of fetching power values is not ideal.
        TODO: enhance by placing power limits in config file
        ***
        """
        return self.max_port_power


    def get_qsa_adapter_type(self):
        """
        Reset the SFP and read the EEPROM of adaptor to identify
        the type of the QSA adapter.
        """
        type = 'N/A'
        try:
            if self.sfp_type == 'QSFP' or self.sfp_type == 'QSFP_DD':
                # Port offset starts with 0x4000
                if self.index in range(PORT_START,PORT_6):
                    port_offset = RESET_BASE_OFFSET + ((self.index-INDEX_1) * HEX_10)
                elif self.index in range(PORT_6,PORT_54):
                    port_offset = RESET_BASE_OFFSET + ((self.index-INDEX_2) * HEX_20)
                elif self.index in range(PORT_54,PORT_END+1):
                    port_offset = RESET_BASE_OFFSET + ((self.index+INDEX_3) * HEX_10)
                else:
                    sys.stderr.write("Port index {} out of range (1-{})\n".format(self.index,PORT_END))

                status = self.pci_get_value(self.BASE_RES_PATH, port_offset)
                reg_value = int(status)

                # Mask off 4th bit for reset
                mask = 1 << 4

                # ResetL is active low
                reg_value = reg_value & ~mask

                # Convert our register value back to a hex string and write back
                status = self.pci_set_value(self.BASE_RES_PATH, reg_value,
                port_offset)

                # Sleep 1 millisecond to allow it to settle
                time.sleep(1/1000)

                eeprom_raw = self._read_eeprom_bytes(self.sfp_eeprom_path,QSFP_QSA28_OUI_OFFSET,QSFP_QSA28_OUI_WIDTH)
                if eeprom_raw is not None:
                    QSFP_QSA28_OUI_VAL = ['00', '02', 'c9']
                    if(eeprom_raw == QSFP_QSA28_OUI_VAL):
                        type = 'QSA28'
                    else:
                        type = 'QSA'
                reg_value = reg_value | mask

                # Convert our register value back to a hex string and write back
                status = self.pci_set_value(self.BASE_RES_PATH, reg_value, port_offset)

                # Sleep 1 millisecond to allow it to settle
                time.sleep(1/1000)
        except ValueError as error:
            syslog.syslog(syslog.LOG_ERR, "Error: Get QSA adapter type failed: %s" % str(error))
            return type
        return type

    def hard_tx_disable(self, tx_disable):
        """
        Disable SFP TX for all channels by pulling up hard TX_DISABLE pin

        Args:
        tx_disable : A Boolean, True to enable tx_disable mode, False to disable
        tx_disable mode.

        Returns:
        A boolean, True if tx_disable is set successfully, False if not
        """
        ret = False
        if self.sfp_type == 'SFP':
            try:
                if self.index in range(PORT_START,PORT_6):
                    port_offset = RESET_BASE_OFFSET + ((self.index-INDEX_1) * HEX_10)
                elif self.index in range(PORT_6,PORT_54):
                    port_offset = RESET_BASE_OFFSET + ((self.index-INDEX_2) * HEX_20)
                elif self.index in range(PORT_54,PORT_END+1):
                    port_offset = RESET_BASE_OFFSET + ((self.index+INDEX_3) * HEX_10)
                else:
                    return False

                status = self.pci_get_value(self.BASE_RES_PATH, port_offset)
                reg_value = int(status)
                mask = 1
                reg_value = (reg_value | mask) if tx_disable else (reg_value & ~mask)
                self.pci_set_value(self.BASE_RES_PATH, reg_value, port_offset)
                syslog.syslog(syslog.LOG_INFO, "Port%d TX Disable :%d %x" % (self.index, tx_disable, port_offset))
                ret = True
            except ValueError:
                ret = False
        return ret

    def set_max_port_power(self, max_port_power):
        """
        Sets the max port power limit
        Args:
            max_port_power : Power value in watts
        Returns:
            None
        """
        self.max_port_power = max_port_power

    def set_port_warn_thresh_power(self, warn_thresh_power):
        """
        Sets port power warning threshold value
        Args:
            warn_thresh_power : Power value in watts
        Returns:
            None
        """
        self.warn_thresh_power = warn_thresh_power

    def set_port_alarm_thresh_power(self, alarm_thresh_power):
        """
        Sets port power alarm threshold value
        Args:
            alarm_thresh_power : Power value in watts
        Returns:
            None
        """
        self.alarm_thresh_power = alarm_thresh_power

    def get_port_warn_thresh_power(self):
        """
        Gets per port warning threshold value mentioned in the platform specific
        policy file
        Args:
            None
        Returns:
            Power threshold value in watts
        """
        return self.warn_thresh_power

    def get_port_alarm_thresh_power(self):
        """
        Gets per port alarm threshold value mentioned in the platform specific
        policy file
        Args:
            None
        Returns:
            Power threshold value in watts
        """
        return self.alarm_thresh_power

    def get_media_power_enable(self):
        """
        Gets the High Power optics status
        Args:
            None
        Returns:
            A boolean, True if high power optics is enabled, False if not
        """
        return self.media_power_enable

    def set_media_power_enable(self, enable):
        """
        Enable/Disable High Power optics feature for a port
        Args:
            enablee : A boolean, True to enable or disable high power optics on
                      a port
        Returns:
            None
        """
        self.media_power_enable = enable
