#############################################################################
# Nokia
#
#############################################################################

import subprocess

try:
    from sonic_platform_base.sfp_base import SfpBase
    from sonic_platform_base.sonic_sfp.sff8472 import sff8472InterfaceId
    from sonic_platform_base.sonic_sfp.sff8472 import sff8472Dom
    from sonic_platform_base.sonic_sfp.sfputilhelper import SfpUtilHelper
    from sonic_py_common import logger
    from sonic_py_common.general import getstatusoutput_noshell
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

smbus_present = 1

try:
    import smbus
except ImportError as e:
    smbus_present = 0


INFO_OFFSET = 128
DOM_OFFSET = 0

# definitions of the offset and width for values in XCVR info eeprom
XCVR_INTFACE_BULK_OFFSET = 0

XCVR_INTFACE_BULK_WIDTH_SFP = 21
XCVR_TYPE_OFFSET = 0
XCVR_TYPE_WIDTH = 1
XCVR_EXT_TYPE_OFFSET = 1
XCVR_EXT_TYPE_WIDTH = 1
XCVR_CONNECTOR_OFFSET = 2
XCVR_CONNECTOR_WIDTH = 1
XCVR_COMPLIANCE_CODE_OFFSET = 3
XCVR_COMPLIANCE_CODE_WIDTH = 8
XCVR_ENCODING_OFFSET = 11
XCVR_ENCODING_WIDTH = 1
XCVR_NBR_OFFSET = 12
XCVR_NBR_WIDTH = 1
XCVR_EXT_RATE_SEL_OFFSET = 13
XCVR_EXT_RATE_SEL_WIDTH = 1
XCVR_CABLE_LENGTH_OFFSET = 14

XCVR_CABLE_LENGTH_WIDTH_SFP = 6
XCVR_VENDOR_NAME_OFFSET = 20
XCVR_VENDOR_NAME_WIDTH = 16
XCVR_VENDOR_OUI_OFFSET = 37
XCVR_VENDOR_OUI_WIDTH = 3
XCVR_VENDOR_PN_OFFSET = 40
XCVR_VENDOR_PN_WIDTH = 16
XCVR_HW_REV_OFFSET = 56

XCVR_HW_REV_WIDTH_SFP = 4
XCVR_VENDOR_SN_OFFSET = 68
XCVR_VENDOR_SN_WIDTH = 16
XCVR_VENDOR_DATE_OFFSET = 84
XCVR_VENDOR_DATE_WIDTH = 8
XCVR_DOM_CAPABILITY_OFFSET = 92
XCVR_DOM_CAPABILITY_WIDTH = 2
XCVR_INTERFACE_DATA_START = 0
XCVR_INTERFACE_DATA_SIZE = 92

SFP_DOM_BULK_DATA_START = 96
SFP_DOM_BULK_DATA_SIZE = 10

SFP_MODULE_ADDRA2_OFFSET = 256
SFP_MODULE_THRESHOLD_OFFSET = 0
SFP_MODULE_THRESHOLD_WIDTH = 56
SFP_CHANNL_THRESHOLD_OFFSET = 112
SFP_CHANNL_THRESHOLD_WIDTH = 2

SFP_TEMPE_OFFSET = 96
SFP_TEMPE_WIDTH = 2
SFP_VOLT_OFFSET = 98
SFP_VOLT_WIDTH = 2
SFP_CHANNL_MON_OFFSET = 100
SFP_CHANNL_MON_WIDTH = 6
SFP_CHANNL_STATUS_OFFSET = 110
SFP_CHANNL_STATUS_WIDTH = 1

sfp_cable_length_tup = ('LengthSMFkm-UnitsOfKm', 'LengthSMF(UnitsOf100m)',
                        'Length50um(UnitsOf10m)', 'Length62.5um(UnitsOfm)',
                        'LengthCable(UnitsOfm)', 'LengthOM3(UnitsOf10m)')

sfp_compliance_code_tup = ('10GEthernetComplianceCode', 'InfinibandComplianceCode',
                           'ESCONComplianceCodes', 'SONETComplianceCodes',
                           'EthernetComplianceCodes', 'FibreChannelLinkLength',
                           'FibreChannelTechnology', 'SFP+CableTechnology',
                           'FibreChannelTransmissionMedia', 'FibreChannelSpeed')

COPPER_TYPE = "COPPER"
SFP_TYPE = "SFP"

# SFP PORT numbers
SFP_PORT_START = 49
SFP_PORT_END = 52

SYSLOG_IDENTIFIER = "xcvrd"
sonic_logger = logger.Logger(SYSLOG_IDENTIFIER)


class Sfp(SfpBase):
    """Platform-specific Sfp class"""
    """
    Nokia platform-specific Sfp class
    """

    # Paths
    PLATFORM_ROOT_PATH = "/usr/share/sonic/device"
    PMON_HWSKU_PATH = "/usr/share/sonic/hwsku"
    HOST_CHK_CMD = ["docker"]

    PLATFORM = "armhf-nokia_ixs7215_52x-r0"
    HWSKU = "Nokia-7215"

    port_to_i2c_mapping = 0

    def __init__(self, index, sfp_type, eeprom_path, port_i2c_map):
        SfpBase.__init__(self)

        self.index = index
        self.port_num = index
        self.sfp_type = sfp_type
        self.eeprom_path = eeprom_path
        self.port_to_i2c_mapping = port_i2c_map
        self.port_name = sfp_type + str(index)
        self.port_to_eeprom_mapping = {}

        self.port_to_eeprom_mapping[index] = eeprom_path

        self.info_dict_keys = ['type', 'vendor_rev', 'serial', 'manufacturer',
                               'model', 'connector', 'encoding', 'ext_identifier',
                               'ext_rateselect_compliance', 'cable_type', 'cable_length',
                               'nominal_bit_rate', 'specification_compliance',
                               'type_abbrv_name', 'vendor_date', 'vendor_oui']

        self.dom_dict_keys = ['rx_los', 'tx_fault', 'reset_status', 'power_lpmode',
                              'tx_disable', 'tx_disable_channel', 'temperature',
                              'voltage', 'rx1power', 'rx2power', 'rx3power',
                              'rx4power', 'tx1bias', 'tx2bias', 'tx3bias', 'tx4bias',
                              'tx1power', 'tx2power', 'tx3power', 'tx4power']

        self.threshold_dict_keys = ['temphighalarm', 'temphighwarning', 'templowalarm',
                                    'templowwarning', 'vcchighalarm', 'vcchighwarning',
                                    'vcclowalarm', 'vcclowwarning', 'rxpowerhighalarm',
                                    'rxpowerhighwarning', 'rxpowerlowalarm', 'rxpowerlowwarning',
                                    'txpowerhighalarm', 'txpowerhighwarning', 'txpowerlowalarm',
                                    'txpowerlowwarning', 'txbiashighalarm', 'txbiashighwarning',
                                    'txbiaslowalarm', 'txbiaslowwarning']

        self.dom_supported = False
        self.dom_temp_supported = False
        self.dom_volt_supported = False
        self.dom_rx_power_supported = False
        self.dom_tx_power_supported = False
        self.calibration = 0

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

    def __is_host(self):
        return subprocess.call(self.HOST_CHK_CMD) == 0

    def __get_path_to_port_config_file(self):
        platform_path = "/".join([self.PLATFORM_ROOT_PATH, self.PLATFORM])
        hwsku_path = "/".join([platform_path, self.HWSKU]
                              ) if self.__is_host() else self.PMON_HWSKU_PATH
        return "/".join([hwsku_path, "port_config.ini"])

    def __read_eeprom_specific_bytes(self, offset, num_bytes):
        sysfsfile_eeprom = None

        eeprom_raw = []
        for i in range(0, num_bytes):
            eeprom_raw.append("0x00")

        sysfs_sfp_i2c_client_eeprom_path = self.port_to_eeprom_mapping[self.port_num]

        try:
            sysfsfile_eeprom = open(
                sysfs_sfp_i2c_client_eeprom_path, mode="rb", buffering=0)
            sysfsfile_eeprom.seek(offset)
            raw = sysfsfile_eeprom.read(num_bytes)
            for n in range(0, num_bytes):
                eeprom_raw[n] = hex(raw[n])[2:].zfill(2)
        except Exception as e:
            pass
        finally:
            if sysfsfile_eeprom:
                sysfsfile_eeprom.close()
        return eeprom_raw

    def _dom_capability_detect(self):
        if self.sfp_type == "SFP":
            sfpi_obj = sff8472InterfaceId()
            if sfpi_obj is None:
                return None
            sfp_dom_capability_raw = self.__read_eeprom_specific_bytes(
                XCVR_DOM_CAPABILITY_OFFSET, XCVR_DOM_CAPABILITY_WIDTH)
            if sfp_dom_capability_raw is not None:
                sfp_dom_capability = int(sfp_dom_capability_raw[0], 16)
                self.dom_supported = (sfp_dom_capability & 0x40 != 0)
                if self.dom_supported:
                    self.dom_temp_supported = True
                    self.dom_volt_supported = True
                    self.dom_rx_power_supported = True
                    self.dom_tx_power_supported = True
                    if sfp_dom_capability & 0x20 != 0:
                        self.calibration = 1
                    elif sfp_dom_capability & 0x10 != 0:
                        self.calibration = 2
                    else:
                        self.calibration = 0
                else:
                    self.dom_temp_supported = False
                    self.dom_volt_supported = False
                    self.dom_rx_power_supported = False
                    self.dom_tx_power_supported = False
                    self.calibration = 0
                self.dom_tx_disable_supported = (
                    int(sfp_dom_capability_raw[1], 16) & 0x40 != 0)
        else:
            self.dom_supported = False
            self.dom_temp_supported = False
            self.dom_volt_supported = False
            self.dom_rx_power_supported = False
            self.dom_tx_power_supported = False

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
        nominal_bit_rate           |INT            |nominal bit rate by 100Mbs
        specification_compliance   |1*255VCHAR     |specification compliance
        type_abbrv_name            |1*255VCHAR     |type of SFP (abbreviated)
        vendor_date                |1*255VCHAR     |vendor date
        vendor_oui                 |1*255VCHAR     |vendor OUI
        application_advertisement  |1*255VCHAR     |supported applications advertisement
        ========================================================================
         """

        if self.sfp_type == COPPER_TYPE:
            return None

        compliance_code_dict = {}
        transceiver_info_dict = dict.fromkeys(self.info_dict_keys, 'N/A')

        if not self.get_presence():
            return transceiver_info_dict

        if self.sfp_type == SFP_TYPE:
            offset = 0
            vendor_rev_width = XCVR_HW_REV_WIDTH_SFP
            interface_info_bulk_width = XCVR_INTFACE_BULK_WIDTH_SFP

            sfpi_obj = sff8472InterfaceId()
            if sfpi_obj is None:
                print("Error: sfp_object open failed")
                return None

            sfp_interface_bulk_raw = self.__read_eeprom_specific_bytes(
                offset + XCVR_INTERFACE_DATA_START, XCVR_INTERFACE_DATA_SIZE)
            if sfp_interface_bulk_raw is None:
                return None

            start = XCVR_INTFACE_BULK_OFFSET - XCVR_INTERFACE_DATA_START
            end = start + interface_info_bulk_width
            sfp_interface_bulk_data = sfpi_obj.parse_sfp_info_bulk(
                sfp_interface_bulk_raw[start: end], 0)

            start = XCVR_VENDOR_NAME_OFFSET - XCVR_INTERFACE_DATA_START
            end = start + XCVR_VENDOR_NAME_WIDTH
            sfp_vendor_name_data = sfpi_obj.parse_vendor_name(
                sfp_interface_bulk_raw[start: end], 0)

            start = XCVR_VENDOR_PN_OFFSET - XCVR_INTERFACE_DATA_START
            end = start + XCVR_VENDOR_PN_WIDTH
            sfp_vendor_pn_data = sfpi_obj.parse_vendor_pn(
                sfp_interface_bulk_raw[start: end], 0)

            start = XCVR_HW_REV_OFFSET - XCVR_INTERFACE_DATA_START
            end = start + vendor_rev_width
            sfp_vendor_rev_data = sfpi_obj.parse_vendor_rev(
                sfp_interface_bulk_raw[start: end], 0)

            start = XCVR_VENDOR_SN_OFFSET - XCVR_INTERFACE_DATA_START
            end = start + XCVR_VENDOR_SN_WIDTH
            sfp_vendor_sn_data = sfpi_obj.parse_vendor_sn(
                sfp_interface_bulk_raw[start: end], 0)

            start = XCVR_VENDOR_OUI_OFFSET - XCVR_INTERFACE_DATA_START
            end = start + XCVR_VENDOR_OUI_WIDTH
            sfp_vendor_oui_data = sfpi_obj.parse_vendor_oui(
                sfp_interface_bulk_raw[start: end], 0)

            start = XCVR_VENDOR_DATE_OFFSET - XCVR_INTERFACE_DATA_START
            end = start + XCVR_VENDOR_DATE_WIDTH
            sfp_vendor_date_data = sfpi_obj.parse_vendor_date(
                sfp_interface_bulk_raw[start: end], 0)

            transceiver_info_dict['type'] = sfp_interface_bulk_data \
                ['data']['type']['value']
            transceiver_info_dict['manufacturer'] = sfp_vendor_name_data \
                ['data']['Vendor Name']['value']
            transceiver_info_dict['model'] = sfp_vendor_pn_data \
                ['data']['Vendor PN']['value']
            transceiver_info_dict['vendor_rev'] = sfp_vendor_rev_data \
                ['data']['Vendor Rev']['value']
            transceiver_info_dict['serial'] = sfp_vendor_sn_data \
                ['data']['Vendor SN']['value']
            transceiver_info_dict['vendor_oui'] = sfp_vendor_oui_data \
                ['data']['Vendor OUI']['value']
            transceiver_info_dict['vendor_date'] = sfp_vendor_date_data \
                ['data']['VendorDataCode(YYYY-MM-DD Lot)']['value']
            transceiver_info_dict['connector'] = sfp_interface_bulk_data \
                ['data']['Connector']['value']
            transceiver_info_dict['encoding'] = sfp_interface_bulk_data \
                ['data']['EncodingCodes']['value']
            transceiver_info_dict['ext_identifier'] = sfp_interface_bulk_data \
                ['data']['Extended Identifier']['value']
            transceiver_info_dict['ext_rateselect_compliance'] = sfp_interface_bulk_data \
                ['data']['RateIdentifier']['value']
            transceiver_info_dict['type_abbrv_name'] = sfp_interface_bulk_data \
                ['data']['type_abbrv_name']['value']

            for key in sfp_cable_length_tup:
                if key in sfp_interface_bulk_data['data']:
                    transceiver_info_dict['cable_type'] = key
                    transceiver_info_dict['cable_length'] = \
                        str(sfp_interface_bulk_data['data'][key]['value'])

            for key in sfp_compliance_code_tup:
                if key in sfp_interface_bulk_data['data']['Specification compliance']['value']:
                    compliance_code_dict[key] = sfp_interface_bulk_data \
                        ['data']['Specification compliance']['value'][key]['value']

            transceiver_info_dict['specification_compliance'] = \
                str(compliance_code_dict)
            transceiver_info_dict['nominal_bit_rate'] = \
                str(sfp_interface_bulk_data['data']['NominalSignallingRate(UnitsOf100Mbd)']['value'])
            transceiver_info_dict['application_advertisement'] = 'N/A'

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

        transceiver_dom_info_dict = dict.fromkeys(self.dom_dict_keys, 'N/A')

        if self.sfp_type == COPPER_TYPE:
            return transceiver_dom_info_dict

        if self.sfp_type == SFP_TYPE:

            self._dom_capability_detect()
            if not self.dom_supported:
                return transceiver_dom_info_dict

            offset = 256
            sfpd_obj = sff8472Dom()
            if sfpd_obj is None:
                return transceiver_dom_info_dict
            sfpd_obj._calibration_type = self.calibration

            dom_data_raw = self.__read_eeprom_specific_bytes(
                (offset + SFP_DOM_BULK_DATA_START), SFP_DOM_BULK_DATA_SIZE)

            start = SFP_TEMPE_OFFSET - SFP_DOM_BULK_DATA_START
            end = start + SFP_TEMPE_WIDTH
            dom_temperature_data = sfpd_obj.parse_temperature(
                dom_data_raw[start: end], 0)

            start = SFP_VOLT_OFFSET - SFP_DOM_BULK_DATA_START
            end = start + SFP_VOLT_WIDTH
            dom_voltage_data = sfpd_obj.parse_voltage(
                dom_data_raw[start: end], 0)

            start = SFP_CHANNL_MON_OFFSET - SFP_DOM_BULK_DATA_START
            end = start + SFP_CHANNL_MON_WIDTH
            dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params(
                dom_data_raw[start: end], 0)

            transceiver_dom_info_dict['temperature'] = self.__convert_string_to_num(
                dom_temperature_data['data']['Temperature']['value'])
            transceiver_dom_info_dict['voltage'] = self.__convert_string_to_num(
                dom_voltage_data['data']['Vcc']['value'])
            transceiver_dom_info_dict['rx1power'] = self.__convert_string_to_num(
                dom_channel_monitor_data['data']['RXPower']['value'])
            transceiver_dom_info_dict['tx1bias'] = self.__convert_string_to_num(
                dom_channel_monitor_data['data']['TXBias']['value'])
            transceiver_dom_info_dict['tx1power'] = self.__convert_string_to_num(
                dom_channel_monitor_data['data']['TXPower']['value'])

        transceiver_dom_info_dict['rx_los'] = self.get_rx_los()
        transceiver_dom_info_dict['tx_fault'] = self.get_tx_fault()
        transceiver_dom_info_dict['reset_status'] = self.get_reset_status()
        transceiver_dom_info_dict['lp_mode'] = self.get_lpmode()

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
        transceiver_dom_threshold_info_dict = dict.fromkeys(
            self.threshold_dict_keys, 'N/A')

        if self.sfp_type == COPPER_TYPE:
            return transceiver_dom_threshold_info_dict

        if self.sfp_type == SFP_TYPE:

            offset = SFP_MODULE_ADDRA2_OFFSET

            self._dom_capability_detect()
            if not self.dom_supported:
                return transceiver_dom_threshold_info_dict

            sfpd_obj = sff8472Dom(None, self.calibration)
            if sfpd_obj is None:
                return transceiver_dom_threshold_info_dict

            dom_module_threshold_raw = self.__read_eeprom_specific_bytes((offset + SFP_MODULE_THRESHOLD_OFFSET),
                                                                         SFP_MODULE_THRESHOLD_WIDTH)
            if dom_module_threshold_raw is not None:
                dom_module_threshold_data = sfpd_obj.parse_alarm_warning_threshold(
                    dom_module_threshold_raw, 0)
            else:
                return transceiver_dom_threshold_info_dict

            # Threshold Data
            transceiver_dom_threshold_info_dict['temphighalarm'] = dom_module_threshold_data['data']['TempHighAlarm']['value']
            transceiver_dom_threshold_info_dict['templowalarm'] = dom_module_threshold_data['data']['TempLowAlarm']['value']
            transceiver_dom_threshold_info_dict['temphighwarning'] = dom_module_threshold_data['data']['TempHighWarning']['value']
            transceiver_dom_threshold_info_dict['templowwarning'] = dom_module_threshold_data['data']['TempLowWarning']['value']
            transceiver_dom_threshold_info_dict['vcchighalarm'] = dom_module_threshold_data['data']['VoltageHighAlarm']['value']
            transceiver_dom_threshold_info_dict['vcclowalarm'] = dom_module_threshold_data['data']['VoltageLowAlarm']['value']
            transceiver_dom_threshold_info_dict['vcchighwarning'] = dom_module_threshold_data[
                'data']['VoltageHighWarning']['value']
            transceiver_dom_threshold_info_dict['vcclowwarning'] = dom_module_threshold_data['data']['VoltageLowWarning']['value']
            transceiver_dom_threshold_info_dict['txbiashighalarm'] = dom_module_threshold_data['data']['BiasHighAlarm']['value']
            transceiver_dom_threshold_info_dict['txbiaslowalarm'] = dom_module_threshold_data['data']['BiasLowAlarm']['value']
            transceiver_dom_threshold_info_dict['txbiashighwarning'] = dom_module_threshold_data['data']['BiasHighWarning']['value']
            transceiver_dom_threshold_info_dict['txbiaslowwarning'] = dom_module_threshold_data['data']['BiasLowWarning']['value']
            transceiver_dom_threshold_info_dict['txpowerhighalarm'] = dom_module_threshold_data['data']['TXPowerHighAlarm']['value']
            transceiver_dom_threshold_info_dict['txpowerlowalarm'] = dom_module_threshold_data['data']['TXPowerLowAlarm']['value']
            transceiver_dom_threshold_info_dict['txpowerhighwarning'] = dom_module_threshold_data['data']['TXPowerHighWarning']['value']
            transceiver_dom_threshold_info_dict['txpowerlowwarning'] = dom_module_threshold_data['data']['TXPowerLowWarning']['value']
            transceiver_dom_threshold_info_dict['rxpowerhighalarm'] = dom_module_threshold_data['data']['RXPowerHighAlarm']['value']
            transceiver_dom_threshold_info_dict['rxpowerlowalarm'] = dom_module_threshold_data['data']['RXPowerLowAlarm']['value']
            transceiver_dom_threshold_info_dict['rxpowerhighwarning'] = dom_module_threshold_data['data']['RXPowerHighWarning']['value']
            transceiver_dom_threshold_info_dict['rxpowerlowwarning'] = dom_module_threshold_data['data']['RXPowerLowWarning']['value']

        return transceiver_dom_threshold_info_dict

    def get_reset_status(self):
        """
        Retrieves the reset status of SFP
        Returns:
            A Boolean, True if reset enabled, False if disabled
        """
        self._dom_capability_detect()
        if not self.dom_supported:
            return False
        if self.sfp_type == COPPER_TYPE:
            return False

        if self.sfp_type == SFP_TYPE:
            offset = 0

            dom_channel_monitor_raw = self.__read_eeprom_specific_bytes(
                (offset + SFP_CHANNL_STATUS_OFFSET), SFP_CHANNL_STATUS_WIDTH)

            if dom_channel_monitor_raw is not None:
                return False
            else:
                return True

    def get_rx_los(self):
        """
        Retrieves the RX LOS (lost-of-signal) status of SFP
        Returns:
            A Boolean, True if SFP has RX LOS, False if not.
        """
        if self.sfp_type == COPPER_TYPE:
            return None
        if not self.dom_supported:
            return None
        rx_los_list = []

        offset = 256
        dom_channel_monitor_raw = self.__read_eeprom_specific_bytes((offset + SFP_CHANNL_STATUS_OFFSET), SFP_CHANNL_STATUS_WIDTH)
        if dom_channel_monitor_raw is not None:
            rx_los_data = int(dom_channel_monitor_raw[0], 16)
            rx_los_list.append(rx_los_data & 0x02 != 0)
        else:
            return None

        return rx_los_list

    def get_tx_fault(self):
        """
        Retrieves the TX fault status of SFP
        Returns:
            A Boolean, True if SFP has TX fault, False if not
            Note : TX fault status is lached until a call to get_tx_fault or a reset.
        """
        if self.sfp_type == COPPER_TYPE:
            return None
        if not self.dom_supported:
            return None
        tx_fault_list = []

        offset = 256
        dom_channel_monitor_raw = self.__read_eeprom_specific_bytes((offset + SFP_CHANNL_STATUS_OFFSET), SFP_CHANNL_STATUS_WIDTH)
        if dom_channel_monitor_raw is not None:
            tx_fault_data = int(dom_channel_monitor_raw[0], 16)
            tx_fault_list.append(tx_fault_data & 0x04 != 0)
        else:
            return None
        return tx_fault_list


    def get_tx_disable(self):
        """
        Retrieves the tx_disable status of this SFP
        Returns:
            A Boolean, True if tx_disable is enabled, False if disabled
        """

        if self.sfp_type == COPPER_TYPE:
            return None
        if not self.dom_supported:
            return None

        tx_disable_list = []
        offset = 256
        dom_channel_monitor_raw = self.__read_eeprom_specific_bytes((offset + SFP_CHANNL_STATUS_OFFSET), SFP_CHANNL_STATUS_WIDTH)
        if dom_channel_monitor_raw is not None:
            tx_disable_data = int(dom_channel_monitor_raw[0], 16)
            tx_disable_list.append(tx_disable_data & 0xC0 != 0)
        else:
            return None

        return tx_disable_list

    def get_tx_disable_channel(self):
        """
        Retrieves the TX disabled channels in this SFP
        Returns:
            A hex of 4 bits (bit 0 to bit 3 as channel 0 to channel 3) to represent
            TX channels which have been disabled in this SFP.
            As an example, a returned value of 0x5 indicates that channel 0
            and channel 2 have been disabled.
        """
        tx_disable_list = self.get_tx_disable()
        if tx_disable_list is None:
            return 0
        tx_disabled = 0
        for i in range(len(tx_disable_list)):
            if tx_disable_list[i]:
                tx_disabled |= 1 << i
        return tx_disabled

    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this SFP
        Returns:
            A Boolean, True if lpmode is enabled, False if disabled
        """
        if self.sfp_type == COPPER_TYPE:
            return False
        if self.sfp_type == SFP_TYPE:
            return False

    def get_power_override(self):
        """
        Retrieves the power-override status of this SFP
        Returns:
            A Boolean, True if power-override is enabled, False if disabled
        """
        if self.sfp_type == COPPER_TYPE:
            return False
        if self.sfp_type == SFP_TYPE:
            return False

    def get_temperature(self):
        """
        Retrieves the temperature of this SFP
        Returns:
            An integer number of current temperature in Celsius
        """
        if self.sfp_type == COPPER_TYPE:
            return None

        transceiver_bulk_status = self.get_transceiver_bulk_status()
        return transceiver_bulk_status.get("temperature", "N/A")

    def get_voltage(self):
        """
        Retrieves the supply voltage of this SFP
        Returns:
            An integer number of supply voltage in mV
        """
        if self.sfp_type == COPPER_TYPE:
            return None

        transceiver_bulk_status = self.get_transceiver_bulk_status()
        return transceiver_bulk_status.get("voltage", "N/A")

    def get_tx_bias(self):
        """
        Retrieves the TX bias current of this SFP
        Returns:

        """
        if self.sfp_type == COPPER_TYPE:
            return None

        tx_bias_list = []
        transceiver_bulk_status = self.get_transceiver_bulk_status()
        tx_bias_list.append(transceiver_bulk_status.get("tx1bias", "N/A"))

        return tx_bias_list


    def get_rx_power(self):
        """
        Retrieves the received optical power for this SFP
        Returns:
            A list of four integer numbers, representing received optical
            power in mW for channel 0 to channel 4.
            Ex. ['1.77', '1.71', '1.68', '1.70']
        """
        rx_power_list = []
        if self.sfp_type == COPPER_TYPE:
            return None

        if self.sfp_type == SFP_TYPE:

            offset = 256

            sfpd_obj = sff8472Dom()
            if sfpd_obj is None:
                return None

            self._dom_capability_detect()
            if self.dom_supported:
                sfpd_obj._calibration_type = self.calibration

                dom_channel_monitor_raw = self.__read_eeprom_specific_bytes(
                    (offset + SFP_CHANNL_MON_OFFSET), SFP_CHANNL_MON_WIDTH)
                if dom_channel_monitor_raw is not None:
                    dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params(
                        dom_channel_monitor_raw, 0)
                    rx_power_list.append(self.__convert_string_to_num(
                        dom_channel_monitor_data['data']['RXPower']['value']))
                else:
                    return None
            else:
                return None

        return rx_power_list

    def get_tx_power(self):
        """
        Retrieves the TX power of this SFP
        Returns:
            A list of four integer numbers, representing TX power in mW
            for channel 0 to channel 4.
            Ex. ['1.86', '1.86', '1.86', '1.86']
        """
        tx_power_list = []

        if self.sfp_type == COPPER_TYPE:
            return None

        if self.sfp_type == SFP_TYPE:

            offset = 256
            sfpd_obj = sff8472Dom()
            if sfpd_obj is None:
                return None

            self._dom_capability_detect()
            if self.dom_supported:
                sfpd_obj._calibration_type = self.calibration

                dom_channel_monitor_raw = self.__read_eeprom_specific_bytes(
                    (offset + SFP_CHANNL_MON_OFFSET), SFP_CHANNL_MON_WIDTH)
                if dom_channel_monitor_raw is not None:
                    dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params(
                        dom_channel_monitor_raw, 0)
                    tx_power_list.append(self.__convert_string_to_num(
                        dom_channel_monitor_data['data']['TXPower']['value']))
                else:
                    return None
            else:
                return None
        return tx_power_list

    def reset(self):
        """
        Reset SFP.
        Returns:
            A boolean, True if successful, False if not
        """
        # RJ45 and SFP ports not resettable
        return False

    def tx_disable(self, tx_disable):
        """
        Disable SFP TX
        Args:
            tx_disable : A Boolean, True to enable tx_disable mode, False to disable
                         tx_disable mode.
        Returns:
            A boolean, True if tx_disable is set successfully, False if not
        """
        if self.sfp_type == COPPER_TYPE:
            return False

        if smbus_present == 0:  # if called from sfputil outside of pmon
            cmdstatus, register = getstatusoutput_noshell(['sudo', 'i2cget', '-y', '0', '0x41', '0x5'])
            if cmdstatus:
                sonic_logger.log_warning("sfp cmdstatus i2c get failed %s" % register )
                return False
            register = int(register, 16)
        else:
            bus = smbus.SMBus(0)
            DEVICE_ADDRESS = 0x41
            DEVICE_REG = 0x5
            register = bus.read_byte_data(DEVICE_ADDRESS, DEVICE_REG)

        pos = [1, 2, 4, 8]
        mask = pos[self.index-SFP_PORT_START]
        if tx_disable is True:
            setbits = register | mask
        else:
            setbits = register & ~mask

        if smbus_present == 0:  # if called from sfputil outside of pmon
            cmdstatus, output = getstatusoutput_noshell(['sudo', 'i2cset', '-y', '-m', '0x0f', '0', '0x41', '0x5', str(setbits)])
            if cmdstatus:
                sonic_logger.log_warning("sfp cmdstatus i2c write failed %s" % output )
                return False
        else:
            bus = smbus.SMBus(0)
            DEVICE_ADDRESS = 0x41
            DEVICE_REG = 0x5
            bus.write_byte_data(DEVICE_ADDRESS, DEVICE_REG, setbits)

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

        return NotImplementedError

    def set_lpmode(self, lpmode):
        """
        Sets the lpmode (low power mode) of SFP
        Args:
            lpmode: A Boolean, True to enable lpmode, False to disable it
            Note  : lpmode can be overridden by set_power_override
        Returns:
            A boolean, True if lpmode is set successfully, False if not
        """
        if self.sfp_type == COPPER_TYPE:
            return False
        if self.sfp_type == SFP_TYPE:
            return False

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

        return NotImplementedError

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        sfputil_helper = SfpUtilHelper()
        sfputil_helper.read_porttab_mappings(
            self.__get_path_to_port_config_file())
        name = sfputil_helper.logical[self.index-1] or "Unknown"
        return name

    def get_presence(self):
        """
        Retrieves the presence
        Returns:
            bool: True if is present, False if not
        """
        if self.sfp_type == COPPER_TYPE:
            return False

        if smbus_present == 0:  # if called from sfputil outside of pmon
            cmdstatus, sfpstatus = getstatusoutput_noshell(['sudo', 'i2cget', '-y', '0', '0x41', '0x3'])
            sfpstatus = int(sfpstatus, 16)
        else:
            bus = smbus.SMBus(0)
            DEVICE_ADDRESS = 0x41
            DEVICE_REG = 0x3
            sfpstatus = bus.read_byte_data(DEVICE_ADDRESS, DEVICE_REG)

        pos = [1, 2, 4, 8]
        bit_pos = pos[self.index-SFP_PORT_START]
        sfpstatus = sfpstatus & (bit_pos)

        if sfpstatus == 0:
            return True

        return False

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        transceiver_dom_info_dict = self.get_transceiver_info()
        return transceiver_dom_info_dict.get("model", "N/A")

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        transceiver_dom_info_dict = self.get_transceiver_info()
        return transceiver_dom_info_dict.get("serial", "N/A")

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self.get_presence()

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """

        if self.sfp_type == "SFP":
            return True
        else:
            return False

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device
        Returns:
            integer: The 1-based relative physical position in parent device
        """
        return self.index
    
    def get_error_description(self):
        """
        Retrives the error descriptions of the SFP module

        Returns:
            String that represents the current error descriptions of vendor specific errors
            In case there are multiple errors, they should be joined by '|',
            like: "Bad EEPROM|Unsupported cable"
        """
        
        if not self.get_presence():
            error_description = self.SFP_STATUS_UNPLUGGED
        else:
            error_description = self.SFP_STATUS_OK

        return error_description
