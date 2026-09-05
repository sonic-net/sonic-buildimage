#!/usr/bin/env python
#
# Name: qsfp.py, version: 1.0
#
# Description: Module contains the definitions of SONiC platform APIs 
#

try:
    import os
    import sys
    import time
    import subprocess
    from sonic_platform_base.sfp_base import SfpBase
    from sonic_platform_base.sonic_sfp.sff8436 import sff8436Dom
    from sonic_platform_base.sonic_sfp.sff8436 import sff8436InterfaceId
    from sonic_platform_base.sonic_sfp.sfputilhelper import SfpUtilHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

INFO_OFFSET = 128
DOM_OFFSET  = 0

XCVR_INTFACE_BULK_OFFSET     = 0
XCVR_INTFACE_BULK_WIDTH_QSFP = 20
XCVR_HW_REV_WIDTH_QSFP       = 2
XCVR_CABLE_LENGTH_WIDTH_QSFP = 5
XCVR_VENDOR_NAME_OFFSET      = 20
XCVR_VENDOR_NAME_WIDTH       = 16
XCVR_VENDOR_OUI_OFFSET       = 37
XCVR_VENDOR_OUI_WIDTH        = 3
XCVR_VENDOR_PN_OFFSET        = 40
XCVR_VENDOR_PN_WIDTH         = 16
XCVR_HW_REV_OFFSET           = 56
XCVR_HW_REV_WIDTH_OSFP       = 2
XCVR_VENDOR_SN_OFFSET        = 68
XCVR_VENDOR_SN_WIDTH         = 16
XCVR_VENDOR_DATE_OFFSET      = 84
XCVR_VENDOR_DATE_WIDTH       = 8
XCVR_DOM_CAPABILITY_OFFSET   = 92
XCVR_DOM_CAPABILITY_WIDTH    = 1

# Offset for values in QSFP eeprom
QSFP_DOM_REV_OFFSET                 = 1
QSFP_DOM_REV_WIDTH                  = 1
QSFP_TEMPE_OFFSET                   = 22
QSFP_TEMPE_WIDTH                    = 2
QSFP_VOLT_OFFSET                    = 26
QSFP_VOLT_WIDTH                     = 2
QSFP_CHANNL_MON_OFFSET              = 34
QSFP_CHANNL_MON_WIDTH               = 16
QSFP_CHANNL_MON_WITH_TX_POWER_WIDTH = 24
QSFP_CONTROL_OFFSET                 = 86
QSFP_CONTROL_WIDTH                  = 8
QSFP_CHANNL_RX_LOS_STATUS_OFFSET    = 3
QSFP_CHANNL_RX_LOS_STATUS_WIDTH     = 1
QSFP_CHANNL_TX_FAULT_STATUS_OFFSET  = 4
QSFP_CHANNL_TX_FAULT_STATUS_WIDTH   = 1
QSFP_POWEROVERRIDE_OFFSET           = 93
QSFP_POWEROVERRIDE_WIDTH            = 1
QSFP_MODULE_THRESHOLD_OFFSET        = 128
QSFP_MODULE_THRESHOLD_WIDTH         = 24
QSFP_CHANNEL_THRESHOLD_OFFSET       = 176
QSFP_CHANNEL_THRESHOLD_WIDTH        = 16

class QSfp(SfpBase):

    def __init__(self, index):
        self.__index = index

        self.__platform = "x86_64-inventec_d6356-r0"
        self.__hwsku    = "INVENTEC-D6356"

        self.__port_to_i2c_mapping = {
                                      0:22,  1:23,  2:24,  3:25,  4:26,  5:27,  6:28,  7:29,
                                      8:30,  9:31,  10:32, 11:33, 12:34, 13:35, 14:36, 15:37,
                                      16:38, 17:39, 18:40, 19:41, 20:42, 21:43, 22:44, 23:45,
                                      24:46, 25:47, 26:48, 27:49, 28:50, 29:51, 30:52, 31:53,
                                      32:54, 33:55, 34:56, 35:57, 36:58, 37:59, 38:60, 39:61,
                                      40:62, 41:63, 42:64, 43:65, 44:66, 45:67, 46:68, 47:69,
                                      48:14, 49:15, 50:16, 51:17, 52:18, 53:19, 54:20, 55:21
                                    }
        self.__port_end = len(self.__port_to_i2c_mapping) - 1;

        self.__presence_attr = None
        self.__eeprom_path = None
        if self.__index in range(0, self.__port_end + 1):
            self.__presence_attr = "/sys/class/swps/port{}/present".format(self.__index)
            self.__lpmode_attr   = "/sys/class/swps/port{}/lpmod".format(self.__index)
            self.__reset_attr    = "/sys/class/swps/port{}/reset".format(self.__index)
            self.__eeprom_path   = "/sys/class/i2c-adapter/i2c-{0}/{0}-0050/eeprom".format(self.__port_to_i2c_mapping[self.__index])

        #print(self.__eeprom_path)

        SfpBase.__init__(self)

    def __get_attr_value(self, attr_path):

        retval = 'ERR'
        if (not os.path.isfile(attr_path)):
            return retval

        try:
            with open(attr_path, 'r') as fd:
                retval = fd.read()
        except Exception as error:
            logging.error("Unable to open ", attr_path, " file !")

        retval = retval.rstrip(' \t\n\r')
        return retval

    def __is_host(self):
        return subprocess.call(["docker"]) == 0

    def __get_path_to_port_config_file(self):
        host_platform_root_path = '/usr/share/sonic/device'
        docker_hwsku_path = '/usr/share/sonic/hwsku'

        host_platform_path = "/".join([host_platform_root_path, self.__platform])
        hwsku_path = "/".join([host_platform_path, self.__hwsku]) if self.__is_host() else docker_hwsku_path

        return "/".join([hwsku_path, "port_config.ini"])

    def __read_eeprom_specific_bytes(self, offset, num_bytes):
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
                eeprom_raw[n] = hex(ord(raw[n]))[2:].zfill(2)
        except:
            pass
        finally:
            if sysfsfile_eeprom:
                sysfsfile_eeprom.close()

        return eeprom_raw

    def __convert_string_to_num(self, value_str):
        if "-inf" in value_str:
            return 'N/A'
        elif "Unknown" in value_str:
            return 'N/A'
        elif 'dBm' in value_str:
            t_str = value_str.rstrip('dBm')
            return float(t_str)
        elif 'mA' in value_str:
            t_str = value_str.rstrip('mA')
            return float(t_str)
        elif 'C' in value_str:
            t_str = value_str.rstrip('C')
            return float(t_str)
        elif 'Volts' in value_str:
            t_str = value_str.rstrip('Volts')
            return float(t_str)
        else:
            return 'N/A'

##############################################
# Device methods
##############################################

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        name = None

        sfputil_helper = SfpUtilHelper()
        sfputil_helper.read_porttab_mappings(self.__get_path_to_port_config_file())
        name = sfputil_helper.logical[self.__index] or "Unknown"
        return name

    def get_presence(self):
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        presence = False
        attr_path = self.__presence_attr

        attr_rv = self.__get_attr_value(attr_path)
        if (attr_rv != 'ERR'):
            if (int(attr_rv) == 0):
                presence = True

        return presence

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
        vendor_rev                 |1*255VCHAR     |vendor revision of SFP
        serial                     |1*255VCHAR     |serial number of the SFP
        manufacturer               |1*255VCHAR     |SFP vendor name
        model                      |1*255VCHAR     |SFP model name
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

        transceiver_info_dict_keys = ['type',                      'vendor_rev',
                                      'serial',                    'manufacturer',
                                      'model',                     'connector',
                                      'encoding',                  'ext_identifier',
                                      'ext_rateselect_compliance', 'cable_type',
                                      'cable_length',              'nominal_bit_rate',
                                      'specification_compliance',  'vendor_date',
                                      'vendor_oui']

        qsfp_cable_length_tup = ('Length(km)',    'Length OM3(2m)',
                                 'Length OM2(m)', 'Length OM1(m)',  'Length Cable Assembly(m)')

        qsfp_compliance_code_tup = ('10/40G Ethernet Compliance Code',                  'SONET Compliance codes',
                                    'SAS/SATA compliance codes',                        'Gigabit Ethernet Compliant codes',
                                    'Fibre Channel link length/Transmitter Technology', 'Fibre Channel transmission media',
                                    'Fibre Channel Speed')

        sfpi_obj = sff8436InterfaceId()
        if not self.get_presence() or not sfpi_obj:
            return {}

        offset = INFO_OFFSET

        sfp_interface_bulk_raw  = self.__read_eeprom_specific_bytes((offset + XCVR_INTFACE_BULK_OFFSET), XCVR_INTFACE_BULK_WIDTH_QSFP)
        sfp_interface_bulk_data = sfpi_obj.parse_sfp_info_bulk(sfp_interface_bulk_raw, 0)

        sfp_vendor_name_raw  = self.__read_eeprom_specific_bytes((offset + XCVR_VENDOR_NAME_OFFSET), XCVR_VENDOR_NAME_WIDTH)
        sfp_vendor_name_data = sfpi_obj.parse_vendor_name(sfp_vendor_name_raw, 0)

        sfp_vendor_pn_raw  = self.__read_eeprom_specific_bytes((offset + XCVR_VENDOR_PN_OFFSET), XCVR_VENDOR_PN_WIDTH)
        sfp_vendor_pn_data = sfpi_obj.parse_vendor_pn(sfp_vendor_pn_raw, 0)

        sfp_vendor_rev_raw  = self.__read_eeprom_specific_bytes((offset + XCVR_HW_REV_OFFSET), XCVR_HW_REV_WIDTH_QSFP)
        sfp_vendor_rev_data = sfpi_obj.parse_vendor_rev(sfp_vendor_rev_raw, 0)

        sfp_vendor_sn_raw  = self.__read_eeprom_specific_bytes((offset + XCVR_VENDOR_SN_OFFSET), XCVR_VENDOR_SN_WIDTH)
        sfp_vendor_sn_data = sfpi_obj.parse_vendor_sn(sfp_vendor_sn_raw, 0)

        sfp_vendor_oui_raw = self.__read_eeprom_specific_bytes((offset + XCVR_VENDOR_OUI_OFFSET), XCVR_VENDOR_OUI_WIDTH)
        if sfp_vendor_oui_raw is not None:
            sfp_vendor_oui_data = sfpi_obj.parse_vendor_oui(sfp_vendor_oui_raw, 0)

        sfp_vendor_date_raw  = self.__read_eeprom_specific_bytes((offset + XCVR_VENDOR_DATE_OFFSET), XCVR_VENDOR_DATE_WIDTH)
        sfp_vendor_date_data = sfpi_obj.parse_vendor_date(sfp_vendor_date_raw, 0)

        transceiver_info_dict = dict.fromkeys(transceiver_info_dict_keys, 'N/A')

        if sfp_interface_bulk_data:
            transceiver_info_dict['type']                      = sfp_interface_bulk_data['data']['type']['value']
            transceiver_info_dict['connector']                 = sfp_interface_bulk_data['data']['Connector']['value']
            transceiver_info_dict['encoding']                  = sfp_interface_bulk_data['data']['EncodingCodes']['value']
            transceiver_info_dict['ext_identifier']            = sfp_interface_bulk_data['data']['Extended Identifier']['value']
            transceiver_info_dict['ext_rateselect_compliance'] = sfp_interface_bulk_data['data']['RateIdentifier']['value']
            transceiver_info_dict['type_abbrv_name']           = sfp_interface_bulk_data['data']['type_abbrv_name']['value']
            transceiver_info_dict['nominal_bit_rate']          = str(sfp_interface_bulk_data['data']['Nominal Bit Rate(100Mbs)']['value'])

        transceiver_info_dict['manufacturer'] = sfp_vendor_name_data['data']['Vendor Name']['value'] if sfp_vendor_name_data else 'N/A'
        transceiver_info_dict['model']        = sfp_vendor_pn_data['data']['Vendor PN']['value'] if sfp_vendor_pn_data else 'N/A'
        transceiver_info_dict['vendor_rev'] = sfp_vendor_rev_data['data']['Vendor Rev']['value'] if sfp_vendor_rev_data else 'N/A'
        transceiver_info_dict['serial']       = sfp_vendor_sn_data['data']['Vendor SN']['value'] if sfp_vendor_sn_data else 'N/A'
        transceiver_info_dict['vendor_oui']   = sfp_vendor_oui_data['data']['Vendor OUI']['value'] if sfp_vendor_oui_data else 'N/A'
        transceiver_info_dict['vendor_date']  = sfp_vendor_date_data['data']['VendorDataCode(YYYY-MM-DD Lot)']['value'] if sfp_vendor_date_data else 'N/A'

        transceiver_info_dict['cable_type']   = "Unknown"
        transceiver_info_dict['cable_length'] = "Unknown"
        for key in qsfp_cable_length_tup:
            if key in sfp_interface_bulk_data['data']:
                transceiver_info_dict['cable_type']   = key
                transceiver_info_dict['cable_length'] = str(sfp_interface_bulk_data['data'][key]['value'])

        compliance_code_dict  = dict()
        for key in qsfp_compliance_code_tup:
            if key in sfp_interface_bulk_data['data']['Specification compliance']['value']:
                compliance_code_dict[key] = sfp_interface_bulk_data['data']['Specification compliance']['value'][key]['value']

        transceiver_info_dict['specification_compliance'] = str(compliance_code_dict)

        return transceiver_info_dict

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
        transceiver_dom_info_dict_keys = ['rx_los',       'tx_fault',
                                          'reset_status', 'power_lpmode',
                                          'tx_disable',   'tx_disable_channel',
                                          'temperature',  'voltage',
                                          'rx1power',     'rx2power',
                                          'rx3power',     'rx4power', 
                                          'tx1bias',      'tx2bias', 
                                          'tx3bias',      'tx4bias', 
                                          'tx1power',     'tx2power',
                                          'tx3power',     'tx4power']

        sfpd_obj = sff8436Dom()
        sfpi_obj = sff8436InterfaceId()

        if not self.get_presence() or not sfpi_obj or not sfpd_obj:
            return {}

        transceiver_dom_info_dict = dict.fromkeys(transceiver_dom_info_dict_keys, 'N/A')
        offset      = DOM_OFFSET
        offset_xcvr = INFO_OFFSET

        # QSFP capability byte parse, through this byte can know whether it support tx_power or not.
        # TODO: in the future when decided to migrate to support SFF-8636 instead of SFF-8436,
        # need to add more code for determining the capability and version compliance
        # in SFF-8636 dom capability definitions evolving with the versions.
        qsfp_dom_capability_raw = self.__read_eeprom_specific_bytes((offset_xcvr + XCVR_DOM_CAPABILITY_OFFSET), XCVR_DOM_CAPABILITY_WIDTH)
        if qsfp_dom_capability_raw is not None:
            qspf_dom_capability_data = sfpi_obj.parse_dom_capability(qsfp_dom_capability_raw, 0)
        else:
            return None

        dom_temperature_raw = self.__read_eeprom_specific_bytes((offset + QSFP_TEMPE_OFFSET), QSFP_TEMPE_WIDTH)
        if dom_temperature_raw is not None:
            dom_temperature_data = sfpd_obj.parse_temperature(dom_temperature_raw, 0)
            transceiver_dom_info_dict['temperature'] = dom_temperature_data['data']['Temperature']['value']

        dom_voltage_raw = self.__read_eeprom_specific_bytes((offset + QSFP_VOLT_OFFSET), QSFP_VOLT_WIDTH)
        if dom_voltage_raw is not None:
            dom_voltage_data = sfpd_obj.parse_voltage(dom_voltage_raw, 0)
            transceiver_dom_info_dict['voltage'] = dom_voltage_data['data']['Vcc']['value']

        qsfp_dom_rev_raw = self.__read_eeprom_specific_bytes((offset + QSFP_DOM_REV_OFFSET), QSFP_DOM_REV_WIDTH)
        if qsfp_dom_rev_raw is not None:
            qsfp_dom_rev_data = sfpd_obj.parse_sfp_dom_rev(qsfp_dom_rev_raw, 0)
            qsfp_dom_rev = qsfp_dom_rev_data['data']['dom_rev']['value']

        # The tx_power monitoring is only available on QSFP which compliant with SFF-8636
        # and claimed that it support tx_power with one indicator bit.
        dom_channel_monitor_data = {}
        dom_channel_monitor_raw = None
        qsfp_tx_power_support = qspf_dom_capability_data['data']['Tx_power_support']['value']
        if (qsfp_dom_rev[0:8] != 'SFF-8636' or (qsfp_dom_rev[0:8] == 'SFF-8636' and qsfp_tx_power_support != 'on')):
            dom_channel_monitor_raw = self.__read_eeprom_specific_bytes((offset + QSFP_CHANNL_MON_OFFSET), QSFP_CHANNL_MON_WIDTH)
            if dom_channel_monitor_raw is not None:
                dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params(dom_channel_monitor_raw, 0)

        else:
            dom_channel_monitor_raw = self.__read_eeprom_specific_bytes((offset + QSFP_CHANNL_MON_OFFSET), QSFP_CHANNL_MON_WITH_TX_POWER_WIDTH)
            if dom_channel_monitor_raw is not None:
                dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params_with_tx_power(dom_channel_monitor_raw, 0)
                transceiver_dom_info_dict['tx1power'] = dom_channel_monitor_data['data']['TX1Power']['value']
                transceiver_dom_info_dict['tx2power'] = dom_channel_monitor_data['data']['TX2Power']['value']
                transceiver_dom_info_dict['tx3power'] = dom_channel_monitor_data['data']['TX3Power']['value']
                transceiver_dom_info_dict['tx4power'] = dom_channel_monitor_data['data']['TX4Power']['value']

        if dom_channel_monitor_raw:
            transceiver_dom_info_dict['rx1power'] = dom_channel_monitor_data['data']['RX1Power']['value']
            transceiver_dom_info_dict['rx2power'] = dom_channel_monitor_data['data']['RX2Power']['value']
            transceiver_dom_info_dict['rx3power'] = dom_channel_monitor_data['data']['RX3Power']['value']
            transceiver_dom_info_dict['rx4power'] = dom_channel_monitor_data['data']['RX4Power']['value']
            transceiver_dom_info_dict['tx1bias']  = dom_channel_monitor_data['data']['TX1Bias']['value']
            transceiver_dom_info_dict['tx2bias']  = dom_channel_monitor_data['data']['TX2Bias']['value']
            transceiver_dom_info_dict['tx3bias']  = dom_channel_monitor_data['data']['TX3Bias']['value']
            transceiver_dom_info_dict['tx4bias']  = dom_channel_monitor_data['data']['TX4Bias']['value']

        for key in transceiver_dom_info_dict:
            transceiver_dom_info_dict[key] = self.__convert_string_to_num(transceiver_dom_info_dict[key])

        transceiver_dom_info_dict['rx_los']             = self.get_rx_los()
        transceiver_dom_info_dict['tx_fault']           = self.get_tx_fault()
        transceiver_dom_info_dict['reset_status']       = self.get_reset_status()
        transceiver_dom_info_dict['tx_disable']         = self.get_tx_disable()
        transceiver_dom_info_dict['tx_disable_channel'] = self.get_tx_disable_channel()
        transceiver_dom_info_dict['lp_mode']            = self.get_lpmode()


        return transceiver_dom_info_dict

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
        transceiver_dom_threshold_info_dict_keys = ['temphighalarm',    'temphighwarning',    'templowalarm',    'templowwarning',
                                                    'vcchighalarm',     'vcchighwarning',     'vcclowalarm',     'vcclowwarning',
                                                    'rxpowerhighalarm', 'rxpowerhighwarning', 'rxpowerlowalarm', 'rxpowerlowwarning',
                                                    'txpowerhighalarm', 'txpowerhighwarning', 'txpowerlowalarm', 'txpowerlowwarning',
                                                    'txbiashighalarm',  'txbiashighwarning',  'txbiaslowalarm',  'txbiaslowwarning']

        sfpd_obj = sff8436Dom()
        if not self.get_presence() or not sfpd_obj:
            return {}

        transceiver_dom_threshold_dict = dict.fromkeys(transceiver_dom_threshold_info_dict_keys, 'N/A')
        offset = DOM_OFFSET

        dom_module_threshold_raw = self.__read_eeprom_specific_bytes((offset + QSFP_MODULE_THRESHOLD_OFFSET), QSFP_MODULE_THRESHOLD_WIDTH)
        if dom_module_threshold_raw:
            module_threshold_values = sfpd_obj.parse_module_threshold_values(dom_module_threshold_raw, 0)
            module_threshold_data = module_threshold_values.get('data')
            if module_threshold_data:
                transceiver_dom_threshold_dict['temphighalarm']   = module_threshold_data['TempHighAlarm']['value']
                transceiver_dom_threshold_dict['templowalarm']    = module_threshold_data['TempLowAlarm']['value']
                transceiver_dom_threshold_dict['temphighwarning'] = module_threshold_data['TempHighWarning']['value']
                transceiver_dom_threshold_dict['templowwarning']  = module_threshold_data['TempLowWarning']['value']
                transceiver_dom_threshold_dict['vcchighalarm']    = module_threshold_data['VccHighAlarm']['value']
                transceiver_dom_threshold_dict['vcclowalarm']     = module_threshold_data['VccLowAlarm']['value']
                transceiver_dom_threshold_dict['vcchighwarning']  = module_threshold_data['VccHighWarning']['value']
                transceiver_dom_threshold_dict['vcclowwarning']   = module_threshold_data['VccLowWarning']['value']

        dom_channel_thres_raw = self.__read_eeprom_specific_bytes((offset + QSFP_CHANNEL_THRESHOLD_OFFSET), QSFP_CHANNEL_THRESHOLD_WIDTH)
        channel_threshold_values = sfpd_obj.parse_channel_threshold_values(dom_channel_thres_raw, 0)
        channel_threshold_data = channel_threshold_values.get('data')
        if channel_threshold_data:
            transceiver_dom_threshold_dict['rxpowerhighalarm']   = channel_threshold_data['RxPowerHighAlarm']['value']
            transceiver_dom_threshold_dict['rxpowerlowalarm']    = channel_threshold_data['RxPowerLowAlarm']['value']
            transceiver_dom_threshold_dict['rxpowerhighwarning'] = channel_threshold_data['RxPowerHighWarning']['value']
            transceiver_dom_threshold_dict['rxpowerlowwarning']  = channel_threshold_data['RxPowerLowWarning']['value']
            transceiver_dom_threshold_dict['txpowerhighalarm']   = "0.0dBm"
            transceiver_dom_threshold_dict['txpowerlowalarm']    = "0.0dBm"
            transceiver_dom_threshold_dict['txpowerhighwarning'] = "0.0dBm"
            transceiver_dom_threshold_dict['txpowerlowwarning']  = "0.0dBm"
            transceiver_dom_threshold_dict['txbiashighalarm']    = channel_threshold_data['TxBiasHighAlarm']['value']
            transceiver_dom_threshold_dict['txbiaslowalarm']     = channel_threshold_data['TxBiasLowAlarm']['value']
            transceiver_dom_threshold_dict['txbiashighwarning']  = channel_threshold_data['TxBiasHighWarning']['value']
            transceiver_dom_threshold_dict['txbiaslowwarning']   = channel_threshold_data['TxBiasLowWarning']['value']

        for key in transceiver_dom_threshold_dict:
            transceiver_dom_threshold_dict[key] = self.__convert_string_to_num(transceiver_dom_threshold_dict[key])

        return transceiver_dom_threshold_dict

    def get_reset_status(self):
        """
        Retrieves the reset status of SFP

        Returns:
            A Boolean, True if reset enabled, False if disabled
        """
        reset_status = False
        attr_path = self.__reset_attr

        attr_rv = self.__get_attr_value(attr_path)
        if (attr_rv != 'ERR'):
            if (int(attr_rv) == 0):
                reset_status = True

        return reset_status

    def get_rx_los(self):
        """
        Retrieves the RX LOS (lost-of-signal) status of SFP

        Returns:
            A Boolean, True if SFP has RX LOS, False if not.
            Note : RX LOS status is latched until a call to get_rx_los or a reset.
        """
        rx_los = False
        rx_los_list = []

        dom_channel_monitor_raw = self.__read_eeprom_specific_bytes(QSFP_CHANNL_RX_LOS_STATUS_OFFSET, QSFP_CHANNL_RX_LOS_STATUS_WIDTH)
        if dom_channel_monitor_raw is not None:
            rx_los_data = int(dom_channel_monitor_raw[0], 16)
            rx_los_list.append(rx_los_data & 0x01 != 0)
            rx_los_list.append(rx_los_data & 0x02 != 0)
            rx_los_list.append(rx_los_data & 0x04 != 0)
            rx_los_list.append(rx_los_data & 0x08 != 0)
            rx_los = rx_los_list[0] and rx_los_list[1] and rx_los_list[2] and rx_los_list[3]
        return rx_los

    def get_tx_fault(self):
        """
        Retrieves the TX fault status of SFP

        Returns:
            A Boolean, True if SFP has TX fault, False if not
            Note : TX fault status is lached until a call to get_tx_fault or a reset.
        """
        tx_fault = False
        tx_fault_list = []

        dom_channel_monitor_raw = self.__read_eeprom_specific_bytes(QSFP_CHANNL_TX_FAULT_STATUS_OFFSET, QSFP_CHANNL_TX_FAULT_STATUS_WIDTH)
        if dom_channel_monitor_raw is not None:
            tx_fault_data = int(dom_channel_monitor_raw[0], 16)
            tx_fault_list.append(tx_fault_data & 0x01 != 0)
            tx_fault_list.append(tx_fault_data & 0x02 != 0)
            tx_fault_list.append(tx_fault_data & 0x04 != 0)
            tx_fault_list.append(tx_fault_data & 0x08 != 0)
            tx_fault = tx_fault_list[0] and tx_fault_list[1] and tx_fault_list[2] and tx_fault_list[3]

        return tx_fault

    def get_tx_disable(self):
        """
        Retrieves the tx_disable status of this SFP

        Returns:
            A Boolean, True if tx_disable is enabled, False if disabled
        """
        tx_disable = False
        tx_disable_list = []

        sfpd_obj = sff8436Dom()
        if sfpd_obj is None:
            return tx_disable

        dom_control_raw = self.__read_eeprom_specific_bytes(QSFP_CONTROL_OFFSET, QSFP_CONTROL_WIDTH)
        if dom_control_raw is not None:
            dom_control_data = sfpd_obj.parse_control_bytes(dom_control_raw, 0)
            tx_disable_list.append('On' == dom_control_data['data']['TX1Disable']['value'])
            tx_disable_list.append('On' == dom_control_data['data']['TX2Disable']['value'])
            tx_disable_list.append('On' == dom_control_data['data']['TX3Disable']['value'])
            tx_disable_list.append('On' == dom_control_data['data']['TX4Disable']['value'])
            tx_disable = tx_disable_list[0] and tx_disable_list[1] and tx_disable_list[2] and tx_disable_list[3]

        return tx_disable

    def get_tx_disable_channel(self):
        """
        Retrieves the TX disabled channels in this SFP

        Returns:
            A hex of 4 bits (bit 0 to bit 3 as channel 0 to channel 3) to represent
            TX channels which have been disabled in this SFP.
            As an example, a returned value of 0x5 indicates that channel 0 
            and channel 2 have been disabled.
        """
        tx_disable_channel = 0
        tx_disable_list = []

        sfpd_obj = sff8436Dom()
        if sfpd_obj is None:
            return tx_disable_channel

        dom_control_raw = self.__read_eeprom_specific_bytes(QSFP_CONTROL_OFFSET, QSFP_CONTROL_WIDTH)
        if dom_control_raw is not None:
            dom_control_data = sfpd_obj.parse_control_bytes(dom_control_raw, 0)
            tx_disable_list.append('On' == dom_control_data['data']['TX1Disable']['value'])
            tx_disable_list.append('On' == dom_control_data['data']['TX2Disable']['value'])
            tx_disable_list.append('On' == dom_control_data['data']['TX3Disable']['value'])
            tx_disable_list.append('On' == dom_control_data['data']['TX4Disable']['value'])

        for i in range(len(tx_disable_list)):
            if tx_disable_list[i]:
                tx_disable_channel |= 1 << i

        return tx_disable_channel

    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this SFP

        Returns:
            A Boolean, True if lpmode is enabled, False if disabled
        """
        lpmode = False
        attr_path = self.__lpmode_attr

        attr_rv = self.__get_attr_value(attr_path)
        if (attr_rv != 'ERR'):
            if (int(attr_rv) == 1):
                lpmode = True

        return lpmode

    def get_power_override(self):
        """
        Retrieves the power-override status of this SFP

        Returns:
            A Boolean, True if power-override is enabled, False if disabled
        """
        power_override = False

        sfpd_obj = sff8436Dom()
        if sfpd_obj is None:
            return power_override

        dom_control_raw = self.__read_eeprom_specific_bytes(QSFP_CONTROL_OFFSET, QSFP_CONTROL_WIDTH)
        if dom_control_raw is not None:
            dom_control_data = sfpd_obj.parse_control_bytes(dom_control_raw, 0)
            power_override = ('On' == dom_control_data['data']['PowerOverride']['value'])

        return power_override

    def get_temperature(self):
        """
        Retrieves the temperature of this SFP

        Returns:
            An integer number of current temperature in Celsius
        """
        transceiver_dom_info_dict = self.get_transceiver_bulk_status()
        return transceiver_dom_info_dict.get("temperature", "N/A")


    def get_voltage(self):
        """
        Retrieves the supply voltage of this SFP

        Returns:
            An integer number of supply voltage in mV
        """
        transceiver_dom_info_dict = self.get_transceiver_bulk_status()
        return transceiver_dom_info_dict.get("voltage", "N/A")

    def get_tx_bias(self):
        """
        Retrieves the TX bias current of this SFP

        Returns:
            A list of four integer numbers, representing TX bias in mA
            for channel 0 to channel 4.
            Ex. ['110.09', '111.12', '108.21', '112.09']
        """
        transceiver_dom_info_dict = self.get_transceiver_bulk_status()
        tx1_bs = transceiver_dom_info_dict.get("tx1bias", "N/A")
        tx2_bs = transceiver_dom_info_dict.get("tx2bias", "N/A")
        tx3_bs = transceiver_dom_info_dict.get("tx3bias", "N/A")
        tx4_bs = transceiver_dom_info_dict.get("tx4bias", "N/A")
        return [tx1_bs, tx2_bs, tx3_bs, tx4_bs] if transceiver_dom_info_dict else []

    def get_rx_power(self):
        """
        Retrieves the received optical power for this SFP

        Returns:
            A list of four integer numbers, representing received optical
            power in mW for channel 0 to channel 4.
            Ex. ['1.77', '1.71', '1.68', '1.70']
        """
        transceiver_dom_info_dict = self.get_transceiver_bulk_status()
        rx1_pw = transceiver_dom_info_dict.get("rx1power", "N/A")
        rx2_pw = transceiver_dom_info_dict.get("rx2power", "N/A")
        rx3_pw = transceiver_dom_info_dict.get("rx3power", "N/A")
        rx4_pw = transceiver_dom_info_dict.get("rx4power", "N/A")
        return [rx1_pw, rx2_pw, rx3_pw, rx4_pw] if transceiver_dom_info_dict else []

    def get_tx_power(self):
        """
        Retrieves the TX power of this SFP

        Returns:
            A list of four integer numbers, representing TX power in mW
            for channel 0 to channel 4.
            Ex. ['1.86', '1.86', '1.86', '1.86']
        """
        transceiver_dom_info_dict = self.get_transceiver_bulk_status()
        tx1_pw = transceiver_dom_info_dict.get("tx1power", "N/A")
        tx2_pw = transceiver_dom_info_dict.get("tx2power", "N/A")
        tx3_pw = transceiver_dom_info_dict.get("tx3power", "N/A")
        tx4_pw = transceiver_dom_info_dict.get("tx4power", "N/A")
        return [tx1_pw, tx2_pw, tx3_pw, tx4_pw]

    def reset(self):
        """
        Reset SFP and return all user module settings to their default srate.

        Returns:
            A boolean, True if successful, False if not
        """

        try:
            reg_file = open(self.__reset_attr, "r+")
        except IOError as e:
            print "Error: unable to open file: %s" % str(e)
            return False

        reg_value = 0
        reg_file.write(hex(reg_value))
        reg_file.close()

        # Sleep 2 second to allow it to settle
        time.sleep(2)

        try:
            reg_file = open(self.__reset_attr, "r+")
        except IOError as e:
            print "Error: unable to open file: %s" % str(e)
            return False

        reg_value = 1
        reg_file.write(hex(reg_value))
        reg_file.close()

        return True

    def tx_disable(self, tx_disable):
        """
        Disable SFP TX for all channels

        Args:
            tx_disable : A Boolean, True to enable tx_disable mode, False to disable
                         tx_disable mode.

        Returns:
            A boolean, True if tx_disable is set successfully, False if not
        """
        sysfs_eeprom_path = self.__eeprom_path
        sysfsfile_eeprom  = None
        try:
            tx_disable_ctl = 0xf if tx_disable else 0x0
            buffer = create_string_buffer(1)
            buffer[0] = chr(tx_disable_ctl)
            # Write to eeprom
            sysfsfile_eeprom = open(sysfs_eeprom_path, "r+b")
            sysfsfile_eeprom.seek(QSFP_CONTROL_OFFSET)
            sysfsfile_eeprom.write(buffer[0])
        except IOError as e:
            print "Error: unable to open file: %s" % str(e)
            return False
        finally:
            if sysfsfile_eeprom is not None:
                sysfsfile_eeprom.close()
                time.sleep(0.01)
        return True

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
        sysfs_eeprom_path = self.__eeprom_path
        sysfsfile_eeprom  = None
        try:
            channel_state = self.get_tx_disable_channel()
            tx_enable_mask  = [0xe, 0xd, 0xb, 0x7]
            tx_disable_mask = [0x1, 0x3, 0x7, 0xf]
            tx_disable_ctl = channel_state | tx_disable_mask[channel] if disable else channel_state & tx_enable_mask[channel]
            buffer = create_string_buffer(1)
            buffer[0] = chr(tx_disable_ctl)
            # Write to eeprom
            sysfsfile_eeprom = open(sysfs_eeprom_path, "r+b")
            sysfsfile_eeprom.seek(QSFP_CONTROL_OFFSET)
            sysfsfile_eeprom.write(buffer[0])
        except IOError as e:
            print "Error: unable to open file: %s" % str(e)
            return False
        finally:
            if sysfsfile_eeprom is not None:
                sysfsfile_eeprom.close()
                time.sleep(0.01)
        return True
    
    def set_lpmode(self, lpmode):
        """
        Sets the lpmode (low power mode) of SFP

        Args:
            lpmode: A Boolean, True to enable lpmode, False to disable it
            Note  : lpmode can be overridden by set_power_override

        Returns:
            A boolean, True if lpmode is set successfully, False if not
        """
        try:
            reg_file = open(self.__lpmode_attr, "r+")
        except IOError as e:
            print "Error: unable to open file: %s" % str(e)
            return False

        reg_value = int(reg_file.readline().rstrip())

        if lpmode is True:
            reg_value = 1
        else:
            reg_value = 0

        reg_file.write(hex(reg_value))
        reg_file.close()

        return True

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
        sysfs_eeprom_path = self.__eeprom_path
        sysfsfile_eeprom  = None
        try:
            power_override_bit = 0
            if power_override:
                power_override_bit |= 1 << 0

            power_set_bit = 0
            if power_set:
                power_set_bit |= 1 << 1

            buffer = create_string_buffer(1)
            buffer[0] = chr(power_override_bit | power_set_bit)
            # Write to eeprom
            sysfsfile_eeprom = open(sysfs_eeprom_path, "r+b")
            sysfsfile_eeprom.seek(QSFP_POWEROVERRIDE_OFFSET)
            sysfsfile_eeprom.write(buffer[0])
        except IOError as e:
            print "Error: unable to open file: %s" % str(e)
            return False
        finally:
            if sysfsfile_eeprom is not None:
                sysfsfile_eeprom.close()
                time.sleep(0.01)
        return True

