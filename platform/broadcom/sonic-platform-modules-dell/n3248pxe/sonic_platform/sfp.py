#!/usr/bin/env python

#############################################################################
# DELLEMC N3248PXE
#
# Module contains an implementation of SONiC Platform Base API and
# provides the platform information
#
#############################################################################

try:
    import os
    import struct
    import mmap
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase

except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class Sfp(SfpOptoeBase):
    """
    DELLEMC Platform-specific Sfp class
    """

    def __init__(self, index, sfp_type, eeprom_path):
        SfpOptoeBase.__init__(self)
        self.sfp_type = sfp_type
        self.index = index
        self.eeprom_path = eeprom_path

    def get_eeprom_path(self):
        return self.eeprom_path

    def get_name(self):
        return "SFP/SFP+/SFP28"

    def pci_mem_read(self, mm, offset):
        mm.seek(offset)
        read_data_stream = mm.read(4)
        reg_val = struct.unpack('I', read_data_stream)
        mem_val = str(reg_val)[1:-2]
        # print "reg_val read:%x"%reg_val
        return mem_val

    def pci_mem_write(self, mm, offset, data):
        mm.seek(offset)
        # print "data to write:%x"%data
        mm.write(struct.pack('I', data))

    def pci_set_value(self, resource, val, offset):
        fd = os.open(resource, os.O_RDWR)
        mm = mmap.mmap(fd, 0)
        val = self.pci_mem_write(mm, offset, val)
        mm.close()
        os.close(fd)
        return val

    def pci_get_value(self, resource, offset):
        fd = os.open(resource, os.O_RDWR)
        mm = mmap.mmap(fd, 0)
        val = self.pci_mem_read(mm, offset)
        mm.close()
        os.close(fd)
        return val

    def _get_cpld_register(self, reg):
        reg_file = '/sys/devices/platform/dell-n3248pxe-cpld.0/' + reg
        try:
            rv = open(reg_file, 'r').read()
        except IOError : return 'ERR'
        return rv.strip('\r\n').lstrip(' ')

    def get_presence(self):
        """
        Retrieves the presence of the sfp
        Returns : True if sfp is present and false if it is absent
        """
        # Check for invalid port_num
        presence = False
        if not (self.index >= SFP_PORT_START and self.index <= SFP_PORT_END): return presence
        bit_mask = 1 << (self.index - SFP_PORT_START)
        try:
            sfp_mod_prs = self._get_cpld_register('sfp_modprs')
            if sfp_mod_prs == 'ERR' : return presence
            presence =  ((int(sfp_mod_prs, 16) & bit_mask) == 0)
        except Exception:
            pass
        return presence

    def get_reset_status(self):
        """
        Retrives the reset status of SFP
        """
        reset_status = False
        return reset_status

    def get_rx_los(self):
        """
        Retrieves the RX LOS (lost-of-signal) status of SFP
        """
        rx_los = False
        if not (self.index >= SFP_PORT_START and self.index <= SFP_PORT_END): return rx_los
        bit_mask = 1 << (self.index - SFP_PORT_START)
        try:
            sfp_rxlos = self._get_cpld_register('sfp_rxlos')
            if sfp_rxlos == 'ERR' : return rx_los
            rx_los =  ((int(sfp_rxlos, 16) & bit_mask) != 0)
        except Exception:
            pass
        return rx_los

    def get_tx_fault(self):
        """
        Retrieves the TX fault status of SFP
        """
        tx_fault = False
        if not (self.index >= SFP_PORT_START and self.index <= SFP_PORT_END): return tx_fault
        bit_mask = 1 << (self.index - SFP_PORT_START)
        try:
            sfp_txfault = self._get_cpld_register('sfp_txfault')
            if sfp_txfault == 'ERR' : return tx_fault
            tx_fault =  ((int(sfp_txfault, 16) & bit_mask) != 0)
        except Exception:
            pass
        return tx_fault

    def get_tx_disable(self):
        """
        Retrieves the tx_disable status of this SFP
        """
        tx_disable = False
        if not (self.index >= SFP_PORT_START and self.index <= SFP_PORT_END): return tx_disable
        bit_mask = 1 << (self.index - SFP_PORT_START)
        try:
            sfp_txdisable = self._get_cpld_register('sfp_txdis')
            if sfp_txdisable == 'ERR' : return tx_disable
            tx_disable =  ((int(sfp_txdisable, 16) & bit_mask) != 0)
        except Exception:
            pass
        return tx_disable

    def get_tx_disable_channel(self):
        """
        Retrieves the TX disabled channels in this SFP
        """
        tx_disable_channel = 0
        try:
            if (self.sfp_type == 'QSFP'):
                tx_disable_data = self._get_eeprom_data('tx_disable')
                for tx_disable_id in ('Tx1Disable', 'Tx2Disable', 'Tx3Disable', 'Tx4Disable'):
                    tx_disable_channel <<= 1
                    tx_disable_channel |= (tx_disable_data['data']['Tx1Disable']['value'] is 'On')
        except (TypeError, ValueError):
            return 'N/A'
        return tx_disable_channel

    def get_lpmode(self):
        """
        Retrieves the lpmode(low power mode) of this SFP
        """
        lpmode_state = False
        return lpmode_state

    def get_power_override(self):
        """
        Retrieves the power-override status of this SFP
        """
        power_override_state = False

        try:
            if (self.sfp_type == 'QSFP'):
                power_override_data = self._get_eeprom_data('power_override')
                power_override = power_override_data['data']['PowerOverRide']['value']
                power_override_state = (power_override is 'On')
        except (TypeError, ValueError): pass
        return power_override_state

    def get_temperature(self):
        """
        Retrieves the temperature of this SFP
        """
        try :
            temperature_data = self._get_eeprom_data('Temperature')
            temperature = temperature_data['data']['Temperature']['value']
        except (TypeError, ValueError):
            return 'N/A'
        return temperature

    def get_voltage(self):
        """
        Retrieves the supply voltage of this SFP
        """
        try:
            voltage_data = self._get_eeprom_data('Voltage')
            voltage = voltage_data['data']['Vcc']['value']
        except (TypeError, ValueError):
            return 'N/A'
        return voltage

    def get_tx_bias(self):
        """
        Retrieves the TX bias current of this SFP
        """
        tx_bias_list = []
        try:
            tx_bias_data = self._get_eeprom_data('ChannelMonitor')
            if (self.sfp_type == 'QSFP'):
                for tx_bias_id in ('TX1Bias', 'TX2Bias', 'TX3Bias', 'TX4Bias') :
                    tx_bias = tx_bias_data['data'][tx_bias_id]['value']
                    tx_bias_list.append(tx_bias)
            else:
                tx1_bias = tx_bias_data['data']['TXBias']['value']
                tx_bias_list =  [tx1_bias, "N/A", "N/A", "N/A"]
        except (TypeError, ValueError):
            return None
        return tx_bias_list

    def get_rx_power(self):
        """
        Retrieves the received optical power for this SFP
        """
        rx_power_list = []
        try:
            rx_power_data = self._get_eeprom_data('ChannelMonitor')
            if (self.sfp_type == 'QSFP'):
                for rx_power_id in ('RX1Power', 'RX2Power', 'RX3Power', 'RX4Power'):
                    rx_power = rx_power_data['data'][rx_power_id]['value']
                    rx_power_list.append(rx_power)
            else:
                rx1_pw = rx_power_data['data']['RXPower']['value']
                rx_power_list =  [rx1_pw, "N/A", "N/A", "N/A"]
        except (TypeError, ValueError):
            return None
        return rx_power_list

    def get_tx_power(self):
        """
        Retrieves the TX power of this SFP
        """
        tx_power_list = []
        try:
            if(self.sfp_type == 'QSFP'):
                # QSFP capability byte parse, through this byte can know whether it support tx_power or not.
                # TODO: in the future when decided to migrate to support SFF-8636 instead of SFF-8436,
                # need to add more code for determining the capability and version compliance
                # in SFF-8636 dom capability definitions evolving with the versions.
                qspf_dom_capability_data = self._get_eeprom_data('dom_capability')
                qsfp_dom_rev_data = self._get_eeprom_data('dom_rev')
                qsfp_dom_rev = qsfp_dom_rev_data['data']['dom_rev']['value']
                qsfp_tx_power_support = qspf_dom_capability_data['data']['Tx_power_support']['value']

                # The tx_power monitoring is only available on QSFP which compliant with SFF-8636
                # and claimed that it support tx_power with one indicator bit.
                if (qsfp_dom_rev[0:8] != 'SFF-8636' or (qsfp_dom_rev[0:8] == 'SFF-8636' and qsfp_tx_power_support != 'on')):
                    return None
                channel_monitor_data = self._get_eeprom_data('ChannelMonitor_TxPower')
                for tx_power_id in ('TX1Power', 'TX2Power', 'TX3Power', 'TX4Power'):
                    tx_pw = channel_monitor_data['data'][tx_power_id]['value']
                    tx_power_list.append(tx_pw)
            else:
                channel_monitor_data = self._get_eeprom_data('ChannelMonitor')
                tx1_pw = channel_monitor_data['data']['TXPower']['value']
                tx_power_list = [tx1_pw, 'N/A', 'N/A', 'N/A']
        except (TypeError, ValueError):
            return None
        return tx_power_list

    def reset(self):
        """
        Reset the SFP and returns all user settings to their default state
        """
        return True

    def set_lpmode(self, lpmode):
        """
        Sets the lpmode(low power mode) of this SFP
        """
        return True

    def get_status(self):
        """
        Retrieves the operational status of the device
        """
        reset = self.get_reset_status()
        return (not reset)

    def get_max_port_power(self):
        """
        Retrieves the maximumum power allowed on the port in watts
        """
        return 2.5
