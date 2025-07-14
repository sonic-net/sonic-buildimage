#!/usr/bin/env python
#
# Name: qsfp.py, version: 1.0
#
# Description: Module contains the definitions of SONiC platform APIs
#

import os
#import time
#from ctypes import create_string_buffer

try:
    from sonic_platform_base.sfp_base import SfpBase
    #    from sonic_platform_base.sonic_sfp.sfputilhelper import SfpUtilHelper
    from .helper import APIHelper
    from sonic_platform.qsfp_8436 import QSfp_SFF8436
    from sonic_platform.qsfp_cmis import QSfp_CMIS
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")


XCVR_TYPE_OFFSET = 0
XCVR_TYPE_WIDTH = 1

SFP_TYPE_CODE_LIST = [
    '03' # SFP/SFP+/SFP28
]
QSFP_TYPE_CODE_LIST = [
    '0d', # QSFP+ or later
    '11' # QSFP28 or later
]
QSFP_DD_TYPE_CODE_LIST = [
    '18', # QSFP-DD Double Density 8X Pluggable Transceiver
    '1b' # QSFP-DD Double Density 8X Pluggable Transceiver
]
CMIS_TYPE_CODE_LIST = [
    '18', # QSFP-DD Double Density 8X Pluggable Transceiver
    '1b'  # DSFP Dual Small Form Factor Pluggable Transceiver
]


SFP_TYPE = "SFP"
QSFP_TYPE = "QSFP"
OSFP_TYPE = "OSFP"
QSFP_DD_TYPE = "QSFP_DD"

TRANSCEIVER_PATH = '/sys/switch/transceiver/'

class QSfp(SfpBase):

    port_start = 0
    port_end = 55

    dom_supported = True
    dom_temp_supported = True
    dom_volt_supported = True
    dom_rx_power_supported = True
    dom_tx_power_supported = True
    dom_tx_disable_supported = True
    calibration = 1

    def __init__(self, index, qsfp_conf):
        self.__index = index
        self.__conf = qsfp_conf
        self.__api_helper = APIHelper()

        self.__attr_path_prefix = '/sys/switch/transceiver/eth{}/'.format(self.__index+1)

        self.__presence_attr = None
        self.__eeprom_path = None

        if self.__index in range(0, self.port_end + 1):
            self.__eeprom_path = TRANSCEIVER_PATH+'eth{}/'.format(self.__index+1)+'eeprom'
            self.__presence_attr = TRANSCEIVER_PATH+'eth{}/'.format(self.__index+1)+'present'
            self.__reset_attr = TRANSCEIVER_PATH+'eth{}/'.format(self.__index+1)+'reset'
            self.__lpmode_attr = TRANSCEIVER_PATH+'eth{}/'.format(self.__index+1)+'lpmode'
        SfpBase.__init__(self)
        self._detect_sfp_type(QSFP_DD_TYPE)
        self.info_dict_keys = ['type', 'hardware_rev', 'serial', 'manufacturer',
            'model', 'connector', 'encoding', 'ext_identifier',
            'ext_rateselect_compliance', 'cable_type', 'cable_length',
            'nominal_bit_rate', 'specification_compliance', 'vendor_date', 'vendor_oui', 'application_advertisement']
        self.qsfp_8436 = QSfp_SFF8436(index,qsfp_conf)
        self.qsfp_cmis = QSfp_CMIS(index,qsfp_conf)
        self.is_cmis = False

    def _detect_sfp_type(self, sfp_type):
        sfp_type = QSFP_DD_TYPE
        self.is_cmis = False

        if not self.get_presence():
            self.sfp_type = sfp_type
            return

        eeprom_raw = []

        eeprom_raw = self._read_eeprom_specific_bytes(
            XCVR_TYPE_OFFSET, XCVR_TYPE_WIDTH)
        if eeprom_raw:
            if eeprom_raw[0] in SFP_TYPE_CODE_LIST:
                self.sfp_type = SFP_TYPE
            elif eeprom_raw[0] in QSFP_TYPE_CODE_LIST:
                #self.sfp_type = QSFP_TYPE
                self.sfp_type = QSFP_DD_TYPE #xcvrd.py  unable to set SI settings when sfp_type is QSFP 
            elif eeprom_raw[0] in QSFP_DD_TYPE_CODE_LIST:
                self.sfp_type = QSFP_DD_TYPE
            else:
                self.sfp_type = sfp_type
            if eeprom_raw[0] in CMIS_TYPE_CODE_LIST :
               self.is_cmis = True 
        else:
            self.sfp_type = sfp_type


    def __is_host(self):
        return os.system("docker > /dev/null 2>&1") == 0

    def _read_eeprom_specific_bytes(self, offset, num_bytes):
        sysfsfile_eeprom = None
        eeprom_raw = []

        for i in range(0, num_bytes):
            eeprom_raw.append("0x00")

        sysfs_eeprom_path = self.__eeprom_path
        try:
            sysfsfile_eeprom = open(sysfs_eeprom_path, mode="rb", buffering=0)
            sysfsfile_eeprom.seek(offset)
            raw = sysfsfile_eeprom.read(num_bytes)
            raw_len = len(raw)
            for n in range(0, raw_len):
                eeprom_raw[n] = hex(raw[n])[2:].zfill(2)
        #except BaseException:
        #    pass
        finally:
            if sysfsfile_eeprom:
                sysfsfile_eeprom.close()

        return eeprom_raw

    def read_eeprom(self, offset, num_bytes):
        sysfsfile_eeprom = None
        eeprom_read_bytes = []
        eeprom_raw = []
        raw = []

        for i in range(0, num_bytes):
            eeprom_raw.append("0x00")

        sysfs_eeprom_path = self.__eeprom_path
        try:
            sysfsfile_eeprom = open(sysfs_eeprom_path, mode="rb", buffering=0)
            sysfsfile_eeprom.seek(offset)
            raw = sysfsfile_eeprom.read(num_bytes)
            raw_len = len(raw)
            eeprom_read_bytes = bytearray(raw)
            for n in range(0, raw_len):
                eeprom_raw[n] = hex(raw[n])[2:].zfill(2)
        except Exception:
            pass
        finally:
            if sysfsfile_eeprom:
                sysfsfile_eeprom.close()

        return eeprom_read_bytes

    def write_eeprom(self, offset, num_bytes, write_buffer):
        try:
            with open(self.__eeprom_path, mode='r+b', buffering=0) as f:
                f.seek(offset)
                f.write(write_buffer[0:num_bytes])
                f.close()
        except (OSError, IOError):
            return False
        return True

##############################################
# Device methods
##############################################

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return self.__conf[self.__index]['name']

    def get_presence(self):
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        presence = False

        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'present')
        if (attr_rv != None):
            attr_rv = int(attr_rv, 16)
            if ((attr_rv & 0x1)  == 1):
                presence = True
        return presence

    def get_all_presence(self):
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """

        presence_bit = 0
        attr_rv = self.__api_helper.read_one_line_file(TRANSCEIVER_PATH + 'present')
        if (attr_rv != None):
            presence_bit = int(attr_rv, 16)
        return presence_bit

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        transceiver_info_dict = self.get_transceiver_info()
        return transceiver_info_dict.get("model", "N/A")

    def get_serial(self):
        """
        Retrieves the serial number of the device

        Returns:
            string: Serial number of device
        """
        transceiver_info_dict = self.get_transceiver_info()
        return transceiver_info_dict.get("serial", "N/A")

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self.get_presence() and not self.get_reset_status()

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return self.__index

    def is_replaceable(self):
        """
        Indicate whether Chassis is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True
##############################################
# SFP methods
##############################################
    def get_transceiver_info(self):
        """
        Retrieves transceiver info of this SFP

        Returns:
            A dict which contains following keys/values :
        ========================================================================
        keys                       |Value Format   |Information
        ---------------------------|---------------|----------------------------
        type                       |1*255VCHAR     |type of SFP
        hardware_rev                |1*255VCHAR     |hardware version of SFP
        serial                  |1*255VCHAR     |serial number of the SFP
        manufacturer            |1*255VCHAR     |SFP vendor name
        model                  |1*255VCHAR     |SFP model name
        connector                  |1*255VCHAR     |connector information
        encoding                   |1*255VCHAR     |encoding information
        ext_identifier             |1*255VCHAR     |extend identifier
        ext_rateselect_compliance  |1*255VCHAR     |extended rateSelect compliance
        cable_length               |INT            |cable length in m
        mominal_bit_rate           |INT            |nominal bit rate by 100Mbs
        specification_compliance   |1*255VCHAR     |specification compliance
        vendor_date                |1*255VCHAR     |vendor date
        vendor_oui                 |1*255VCHAR     |vendor OUI
        ========================================================================
        """
        self._detect_sfp_type(self.sfp_type)
        if self.is_cmis == True:
            return self.qsfp_cmis.get_transceiver_info()
        else:
            return self.qsfp_8436.get_transceiver_info()
        

    def get_transceiver_bulk_status(self):
        """
        Retrieves transceiver bulk status of this SFP

        Returns:
            A dict which contains following keys/values :
        ========================================================================
        keys                       |Value Format   |Information
        ---------------------------|---------------|----------------------------
        rx_los                     |BOOLEAN        |RX loss-of-signal status, True if has RX los, False if not.
        tx_fault                   |BOOLEAN        |TX fault status, True if has TX fault, False if not.
        reset_status               |BOOLEAN        |reset status, True if SFP in reset, False if not.
        lp_mode                    |BOOLEAN        |low power mode status, True in lp mode, False if not.
        tx_disable                 |BOOLEAN        |TX disable status, True TX disabled, False if not.
        tx_disabled_channel        |HEX            |disabled TX channels in hex, bits 0 to 3 represent channel 0
                                   |               |to channel 3.
        temperature                |INT            |module temperature in Celsius
        voltage                    |INT            |supply voltage in mV
        tx<n>bias                  |INT            |TX Bias Current in mA, n is the channel number,
                                   |               |for example, tx2bias stands for tx bias of channel 2.
        rx<n>power                 |INT            |received optical power in mW, n is the channel number,
                                   |               |for example, rx2power stands for rx power of channel 2.
        tx<n>power                 |INT            |TX output power in mW, n is the channel number,
                                   |               |for example, tx2power stands for tx power of channel 2.
        ========================================================================
        """
        if self.is_cmis == True:
            return self.qsfp_cmis.get_transceiver_bulk_status()
        else:
            return self.qsfp_8436.get_transceiver_bulk_status()
        
    def get_transceiver_threshold_info(self):
        """
        Retrieves transceiver threshold info of this SFP

        Returns:
            A dict which contains following keys/values :
        ========================================================================
        keys                       |Value Format   |Information
        ---------------------------|---------------|----------------------------
        temphighalarm              |FLOAT          |High Alarm Threshold value of temperature in Celsius.
        templowalarm               |FLOAT          |Low Alarm Threshold value of temperature in Celsius.
        temphighwarning            |FLOAT          |High Warning Threshold value of temperature in Celsius.
        templowwarning             |FLOAT          |Low Warning Threshold value of temperature in Celsius.
        vcchighalarm               |FLOAT          |High Alarm Threshold value of supply voltage in mV.
        vcclowalarm                |FLOAT          |Low Alarm Threshold value of supply voltage in mV.
        vcchighwarning             |FLOAT          |High Warning Threshold value of supply voltage in mV.
        vcclowwarning              |FLOAT          |Low Warning Threshold value of supply voltage in mV.
        rxpowerhighalarm           |FLOAT          |High Alarm Threshold value of received power in dBm.
        rxpowerlowalarm            |FLOAT          |Low Alarm Threshold value of received power in dBm.
        rxpowerhighwarning         |FLOAT          |High Warning Threshold value of received power in dBm.
        rxpowerlowwarning          |FLOAT          |Low Warning Threshold value of received power in dBm.
        txpowerhighalarm           |FLOAT          |High Alarm Threshold value of transmit power in dBm.
        txpowerlowalarm            |FLOAT          |Low Alarm Threshold value of transmit power in dBm.
        txpowerhighwarning         |FLOAT          |High Warning Threshold value of transmit power in dBm.
        txpowerlowwarning          |FLOAT          |Low Warning Threshold value of transmit power in dBm.
        txbiashighalarm            |FLOAT          |High Alarm Threshold value of tx Bias Current in mA.
        txbiaslowalarm             |FLOAT          |Low Alarm Threshold value of tx Bias Current in mA.
        txbiashighwarning          |FLOAT          |High Warning Threshold value of tx Bias Current in mA.
        txbiaslowwarning           |FLOAT          |Low Warning Threshold value of tx Bias Current in mA.
        ========================================================================
        """
        if self.is_cmis == True:
            return self.qsfp_cmis.get_transceiver_threshold_info()
        else:
            return self.qsfp_8436.get_transceiver_threshold_info()
    def get_reset_status(self):
        """
        Retrieves the reset status of SFP

        Returns:
            A Boolean, True if reset enabled, False if disabled
        """
        reset_status = False

        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'reset')
        if (attr_rv != None):
            attr_rv = int(attr_rv, 16)
            if ((attr_rv & 0x1)  == 1):
                reset_status = True
        return reset_status

    def get_rx_los(self):
        """
        Retrieves the RX LOS (loss-of-signal) status of SFP

        Returns:
            A list of boolean values, representing the RX LOS status
            of each available channel, value is True if SFP channel
            has RX LOS, False if not.
            E.g., for a tranceiver with four channels: [False, False, True, False]
            Note : RX LOS status is latched until a call to get_rx_los or a reset.
        """
        if self.is_cmis == True:
            return self.qsfp_cmis.get_rx_los()
        else:
            return self.qsfp_8436.get_rx_los()

    def get_tx_fault(self):
        """
        Retrieves the TX fault status of SFP

        Returns:
            A list of boolean values, representing the TX fault status
            of each available channel, value is True if SFP channel
            has TX fault, False if not.
            E.g., for a tranceiver with four channels: [False, False, True, False]
            Note : TX fault status is lached until a call to get_tx_fault or a reset.
        """
        if self.is_cmis == True:
            return self.qsfp_cmis.get_tx_fault()
        else:
            return self.qsfp_8436.get_tx_fault()

    def get_tx_disable(self):
        """
        Retrieves the tx_disable status of this SFP

        Returns:
            A Boolean, True if tx_disable is enabled, False if disabled
        """
        if self.is_cmis == True:
            return self.qsfp_cmis.get_tx_disable()
        else:
            return self.qsfp_8436.get_tx_disable()

    def get_tx_disable_channel(self):
        """
        Retrieves the TX disabled channels in this SFP

        Returns:
            A hex of 4 bits (bit 0 to bit 3 as channel 0 to channel 3) to represent
            TX channels which have been disabled in this SFP.
            As an example, a returned value of 0x5 indicates that channel 0
            and channel 2 have been disabled.
        """
        if self.is_cmis == True:
            return self.qsfp_cmis.get_tx_disable_channel()
        else:
            return self.qsfp_8436.get_tx_disable_channel()

    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this SFP

        Returns:
            A Boolean, True if lpmode is enabled, False if disabled
        """
        if self.is_cmis == True:
            return self.qsfp_cmis.get_lpmode()
        else:
            return self.qsfp_8436.get_lpmode()

    def get_power_override(self):
        """
        Retrieves the power-override status of this SFP

        Returns:
            A Boolean, True if power-override is enabled, False if disabled
        """
        return None

    def get_temperature(self):
        """
        Retrieves the temperature of this SFP

        Returns:
            An integer number of current temperature in Celsius
        """
        if self.is_cmis == True:
            return self.qsfp_cmis.get_temperature()
        else:
            return self.qsfp_8436.get_temperature()

    def get_voltage(self):
        """
        Retrieves the supply voltage of this SFP

        Returns:
            An integer number of supply voltage in mV
        """
        if self.is_cmis == True:
            return self.qsfp_cmis.get_voltage()
        else:
            return self.qsfp_8436.get_voltage()


    def get_tx_bias(self):
        """
        Retrieves the TX bias current of this SFP

        Returns:
            A list of four integer numbers, representing TX bias in mA
            for channel 0 to channel 4.
            Ex. ['110.09', '111.12', '108.21', '112.09']
        """
        if self.is_cmis == True:
            return self.qsfp_cmis.get_tx_bias()
        else:
            return self.qsfp_8436.get_tx_bias()

    def get_rx_power(self):
        """
        Retrieves the received optical power for this SFP

        Returns:
            A list of four integer numbers, representing received optical
            power in mW for channel 0 to channel 4.
            Ex. ['1.77', '1.71', '1.68', '1.70']
        """
        if self.is_cmis == True:
            return self.qsfp_cmis.get_rx_power()
        else:
            return self.qsfp_8436.get_rx_power()

    def get_tx_power(self):
        """
        Retrieves the TX power of this SFP

        Returns:
            A list of four integer numbers, representing TX power in mW
            for channel 0 to channel 4.
            Ex. ['1.86', '1.86', '1.86', '1.86']
        """
        if self.is_cmis == True:
            return self.qsfp_cmis.get_tx_power()
        else:
            return self.qsfp_8436.get_tx_power()

    def reset(self):
        """
        Reset SFP and return all user module settings to their default srate.

        Returns:
            A boolean, True if successful, False if not
        """
        if self.is_cmis == True:
            return self.qsfp_cmis.reset()
        else:
            return self.qsfp_8436.reset()

    def tx_disable(self, tx_disable):
        """
        Disable SFP TX for all channels

        Args:
            tx_disable : A Boolean, True to enable tx_disable mode, False to disable
                         tx_disable mode.

        Returns:
            A boolean, True if tx_disable is set successfully, False if not
        """
        if self.is_cmis == True:
            return self.qsfp_cmis.tx_disable()
        else:
            return self.qsfp_8436.tx_disable()


    def tx_disable_channel(self, channel, disable):
        """
        Sets the tx_disable for specified SFP channels

        Args:
            channel : A hex of 4 bits (bit 0 to bit 3) which represent channel 0 to 3,
                      e.g. 0x5 for channel 0 and channel 2.
            disable : A boolean, True to disable TX channels specified in channel,
                      False to enable

        Returns:
            A boolean, True if successful, False if not
        """
        if self.is_cmis == True:
            return self.qsfp_cmis.tx_disable_channel()
        else:
            return self.qsfp_8436.tx_disable_channel()

    def set_lpmode(self, lpmode):
        """
        Sets the lpmode (low power mode) of SFP

        Args:
            lpmode: A Boolean, True to enable lpmode, False to disable it
            Note  : lpmode can be overridden by set_power_override

        Returns:
            A boolean, True if lpmode is set successfully, False if not
        """
        if self.is_cmis == True:
            return self.qsfp_cmis.set_lpmode()
        else:
            return self.qsfp_8436.set_lpmode()

    def set_power_override(self, power_override, power_set):
        """
        Sets SFP power level using power_override and power_set

        Args:
            power_override :
                    A Boolean, True to override set_lpmode and use power_set
                    to control SFP power, False to disable SFP power control
                    through power_override/power_set and use set_lpmode
                    to control SFP power.
            power_set :
                    Only valid when power_override is True.
                    A Boolean, True to set SFP to low power mode, False to set
                    SFP to high power mode.

        Returns:
            A boolean, True if power-override and power_set are set successfully,
            False if not
        """
        return True

