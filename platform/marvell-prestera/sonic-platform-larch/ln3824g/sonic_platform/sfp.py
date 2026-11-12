import os
import sys
import subprocess

try:
    from sonic_platform_base.sfp_base import SfpBase
    from sonic_platform_base.sonic_sfp.sff8472 import sff8472InterfaceId
    from sonic_platform_base.sonic_sfp.sff8472 import sff8472Dom
    from sonic_py_common import logger
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

if sys.version_info[0] < 3:
    import commands as cmd
else:
    import subprocess as cmd

smbus_present = 1

try:
    import smbus
except ImportError as e:
    smbus_present = 0

XCVR_INTFACE_BULK_OFFSET = 0
XCVR_INTFACE_BULK_WIDTH_SFP = 21
XCVR_HW_REV_WIDTH_SFP = 4
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
XCVR_DOM_CAPABILITY_WIDTH = 1

# Offset for values in SFP eeprom
SFP_MODULE_ADDRA2_OFFSET = 256
SFP_TEMPE_OFFSET = 96
SFP_TEMPE_WIDTH = 2
SFP_VOLT_OFFSET = 98
SFP_VOLT_WIDTH = 2
SFP_CHANNL_MON_OFFSET = 100
SFP_CHANNL_MON_WIDTH = 6
SFP_MODULE_THRESHOLD_OFFSET = 0
SFP_MODULE_THRESHOLD_WIDTH = 40
SFP_CHANNL_THRESHOLD_OFFSET = 112
SFP_CHANNL_THRESHOLD_WIDTH = 2
SFP_STATUS_CONTROL_OFFSET = 110
SFP_STATUS_CONTROL_WIDTH = 1
SFP_TX_DISABLE_HARD_BIT = 7
SFP_TX_DISABLE_SOFT_BIT = 6
SFP_DOM_BULK_DATA_START = 96
SFP_DOM_BULK_DATA_SIZE = 10

sfp_cable_length_tup = ('LengthSMFkm-UnitsOfKm', 'LengthSMF(UnitsOf100m)',
                        'Length50um(UnitsOf10m)', 'Length62.5um(UnitsOfm)',
                        'LengthCable(UnitsOfm)', 'LengthOM3(UnitsOf10m)')

sfp_compliance_code_tup = ('10GEthernetComplianceCode', 'InfinibandComplianceCode',
                           'ESCONComplianceCodes', 'SONETComplianceCodes',
                           'EthernetComplianceCodes', 'FibreChannelLinkLength',
                           'FibreChannelTechnology', 'SFP+CableTechnology',
                           'FibreChannelTransmissionMedia', 'FibreChannelSpeed')
dom_info_dict_keys = [
    'rx_los', 'tx_fault', 'reset_status', 'lp_mode',
    'temperature', 'voltage',
    'rx1power', 'tx1bias', 'tx1power']

threshold_dict_keys = [
    'temphighalarm', 'temphighwarning',
    'templowalarm', 'templowwarning',
    'vcchighalarm', 'vcchighwarning',
    'vcclowalarm', 'vcclowwarning',
    'rxpowerhighalarm', 'rxpowerhighwarning',
    'rxpowerlowalarm', 'rxpowerlowwarning',
    'txpowerhighalarm', 'txpowerhighwarning',
    'txpowerlowalarm', 'txpowerlowwarning',
    'txbiashighalarm', 'txbiashighwarning',
    'txbiaslowalarm', 'txbiaslowwarning']

SFP_INFO_OFFSET = 0
SFP_DOM_OFFSET = 256

COPPER_TYPE = "COPPER"
SFP_TYPE = "SFP"

# SFP PORT numbers
SFP_PORT_START = 25
SFP_PORT_END = 30

MUX_ADDRESS = 0x73

SYSLOG_IDENTIFIER = "xcvrd"
sonic_logger = logger.Logger(SYSLOG_IDENTIFIER)

# map port numbers to their corresponding channel
# numbers of the MUX
PORT_TO_CHANNEL_MAP = {
    25: 0x1,
    26: 0x2,
    27: 0x4,
    28: 0x8,
    29: 0x10,
    30: 0x20
}

class Sfp(SfpBase):
    """Platform-specific Sfp class"""

    PORT_EEPROM_PATH = '/sys/bus/i2c/devices/0-0050/eeprom'

    _tx_disable_bmp = 0b00111111

    def __init__(self, index, sfp_type):
        SfpBase.__init__(self)

        self.port_num = index # 1-based physical port number
        self.sfp_type = sfp_type
        self.dom_supported = False
        self.dom_temp_supported = False
        self.dom_volt_supported = False
        self.dom_rx_power_supported = False
        self.dom_tx_power_supported = False
        self.calibration = 0

        if self.sfp_type == SFP_TYPE:
            self.tx_disable(False)

    def __get_channel_num(self, port_num):
        if SFP_PORT_START <= port_num <= SFP_PORT_END:
            return PORT_TO_CHANNEL_MAP.get(port_num, 0x0)

    def _convert_string_to_num(self, value_str):
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

    def _dom_capability_detect(self):
        if not self.get_presence():
            self.dom_supported = False
            self.dom_temp_supported = False
            self.dom_volt_supported = False
            self.dom_rx_power_supported = False
            self.dom_tx_power_supported = False
            self.calibration = 0
            return

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

            if len(sfp_dom_capability_raw) >= 2:
                self.dom_tx_disable_supported = (int(sfp_dom_capability_raw[1], 16) & 0x40 != 0)
            else:
                self.dom_tx_disable_supported = False

    def __read_eeprom_specific_bytes_raw(self, offset, num_bytes):
        if not os.path.exists(self.PORT_EEPROM_PATH):
            sonic_logger.log_error("path %s is not exist" % self.PORT_EEPROM_PATH)
            return None

        sysfsfile_eeprom = None

        try:
            sysfsfile_eeprom = open(self.PORT_EEPROM_PATH, mode="rb", buffering=0)
            sysfsfile_eeprom.seek(offset)
            raw = sysfsfile_eeprom.read(num_bytes)
        except Exception as e:
            pass
        finally:
            if sysfsfile_eeprom:
                sysfsfile_eeprom.close()
        return raw

    def __read_eeprom_specific_bytes(self, offset, num_bytes):

        raw = self.__read_eeprom_specific_bytes_raw(offset, num_bytes)
        if raw is None:
            return None

        eeprom_raw = []
        for i in range(0, num_bytes):
            eeprom_raw.append("0x00")

        for n in range(0, num_bytes):
            eeprom_raw[n] = hex(raw[n])[2:].zfill(2)

        return eeprom_raw

########  DEVICE BASE API  #########

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return "Ethernet{}".format(self.port_num - 1)

    def get_presence(self):
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        if self.sfp_type == COPPER_TYPE:
            return False

        if smbus_present == 0:  # if called from sfputil outside of pmon
            cmdstatus, sfpstatus = cmd.getstatusoutput('sudo i2cget -y 0 0x33 0x04')
            if cmdstatus:
                sonic_logger.log_warning("sfp cmdstatus i2c read failed")
                return False
            sfpstatus = int(sfpstatus, 16)
        else:
            bus = smbus.SMBus(0)
            DEVICE_ADDRESS = 0x33
            DEVICE_REG = 0x4
            try:
                sfpstatus = bus.read_byte_data(DEVICE_ADDRESS, DEVICE_REG)
            except Exception as e:
                sonic_logger.log_warning("Failed to retrieve SFP presence info")

        sfpstatus = sfpstatus & (1 << (self.port_num-SFP_PORT_START))
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

    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        transceiver_dom_info_dict = self.get_transceiver_info()
        return transceiver_dom_info_dict.get("vendor_rev", "N/A")

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self.get_presence()

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device. If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of entPhysicalContainedIn is '0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device or -1 if cannot determine the position
        """
        return self.port_num

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

########  SFP BASE API  #########

    def get_transceiver_info(self):
        """
        Retrieves transceiver info of this SFP

        Returns:
            A dict which contains following keys/values :
        ================================================================================
        keys                       |Value Format   |Information
        ---------------------------|---------------|----------------------------
        type                       |1*255VCHAR     |type of SFP
        type_abbrv_name            |1*255VCHAR     |type of SFP, abbreviated
        hardware_rev               |1*255VCHAR     |hardware version of SFP
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
        vendor_date                |1*255VCHAR     |vendor date
        vendor_oui                 |1*255VCHAR     |vendor OUI
        application_advertisement  |1*255VCHAR     |supported applications advertisement
        ================================================================================
        """
        DEVICE_ADDRESS = MUX_ADDRESS # i2c bridge address
        # check present status
        if self.port_num < SFP_PORT_START:
            return {}
        else:
            sfpi_obj = sff8472InterfaceId() #SFP
        if not self.get_presence() or not sfpi_obj:
            return {}

        channel = self.__get_channel_num(self.port_num)
        if smbus_present == 0:  # if called from sfputil outside of pmon
            cmdstatus, sfpstatus = cmd.getstatusoutput('sudo i2cset -y 0 0x73 %x' % channel)
            if cmdstatus:
                sonic_logger.log_warning("Failed to set channel %s into mux", channel)
                return False
        else:
            bus = smbus.SMBus(0)

        try:
            bus.write_byte(DEVICE_ADDRESS, channel)
        except:
            sonic_logger.log_warning("Failed to write channel number into mux")

        offset = SFP_INFO_OFFSET
        sfp_interface_bulk_raw = self.__read_eeprom_specific_bytes(
            (offset + XCVR_INTFACE_BULK_OFFSET), XCVR_INTFACE_BULK_WIDTH_SFP)

        sfp_interface_bulk_data = sfpi_obj.parse_sfp_info_bulk(
            sfp_interface_bulk_raw, 0)

        sfp_vendor_name_raw = self.__read_eeprom_specific_bytes(
            (offset + XCVR_VENDOR_NAME_OFFSET), XCVR_VENDOR_NAME_WIDTH)
        sfp_vendor_name_data = sfpi_obj.parse_vendor_name(
            sfp_vendor_name_raw, 0)

        sfp_vendor_pn_raw = self.__read_eeprom_specific_bytes(
            (offset + XCVR_VENDOR_PN_OFFSET), XCVR_VENDOR_PN_WIDTH)
        sfp_vendor_pn_data = sfpi_obj.parse_vendor_pn(
            sfp_vendor_pn_raw, 0)

        sfp_vendor_rev_raw = self.__read_eeprom_specific_bytes(
            (offset + XCVR_HW_REV_OFFSET), XCVR_HW_REV_WIDTH_SFP)

        sfp_vendor_rev_data = sfpi_obj.parse_vendor_rev(
            sfp_vendor_rev_raw, 0)

        sfp_vendor_sn_raw = self.__read_eeprom_specific_bytes(
            (offset + XCVR_VENDOR_SN_OFFSET), XCVR_VENDOR_SN_WIDTH)
        sfp_vendor_sn_data = sfpi_obj.parse_vendor_sn(
            sfp_vendor_sn_raw, 0)

        sfp_vendor_oui_raw = self.__read_eeprom_specific_bytes(
            (offset + XCVR_VENDOR_OUI_OFFSET), XCVR_VENDOR_OUI_WIDTH)
        if sfp_vendor_oui_raw is not None:
            sfp_vendor_oui_data = sfpi_obj.parse_vendor_oui(
                sfp_vendor_oui_raw, 0)

        sfp_vendor_date_raw = self.__read_eeprom_specific_bytes(
            (offset + XCVR_VENDOR_DATE_OFFSET), XCVR_VENDOR_DATE_WIDTH)
        sfp_vendor_date_data = sfpi_obj.parse_vendor_date(
            sfp_vendor_date_raw, 0)

        transceiver_info_dict = dict()
        compliance_code_dict = dict()

        if sfp_interface_bulk_data:
            transceiver_info_dict['type'] = sfp_interface_bulk_data['data']['type']['value']
            transceiver_info_dict['connector'] = sfp_interface_bulk_data['data']['Connector']['value']
            transceiver_info_dict['encoding'] = sfp_interface_bulk_data['data']['EncodingCodes']['value']
            transceiver_info_dict['ext_identifier'] = sfp_interface_bulk_data['data']['Extended Identifier']['value']
            transceiver_info_dict['ext_rateselect_compliance'] = sfp_interface_bulk_data['data']['RateIdentifier']['value']
            transceiver_info_dict['type_abbrv_name'] = sfp_interface_bulk_data['data']['type_abbrv_name']['value']

        transceiver_info_dict['application_advertisement'] = "N/A"
        transceiver_info_dict['manufacturer'] = sfp_vendor_name_data[
            'data']['Vendor Name']['value'] if sfp_vendor_name_data else 'N/A'
        transceiver_info_dict['model'] = sfp_vendor_pn_data['data']['Vendor PN']['value'] if sfp_vendor_pn_data else 'N/A'
        transceiver_info_dict['vendor_rev'] = sfp_vendor_rev_data['data']['Vendor Rev']['value'] if sfp_vendor_rev_data else 'N/A'
        transceiver_info_dict['serial'] = sfp_vendor_sn_data['data']['Vendor SN']['value'] if sfp_vendor_sn_data else 'N/A'
        transceiver_info_dict['vendor_oui'] = sfp_vendor_oui_data['data']['Vendor OUI']['value'] if sfp_vendor_oui_data else 'N/A'
        transceiver_info_dict['vendor_date'] = sfp_vendor_date_data[
            'data']['VendorDataCode(YYYY-MM-DD Lot)']['value'] if sfp_vendor_date_data else 'N/A'
        transceiver_info_dict['cable_type'] = "Unknown"
        transceiver_info_dict['cable_length'] = "Unknown"

        for key in sfp_cable_length_tup:
            if key in sfp_interface_bulk_data['data']:
                transceiver_info_dict['cable_type'] = key
                transceiver_info_dict['cable_length'] = str(
                    sfp_interface_bulk_data['data'][key]['value'])

        for key in sfp_compliance_code_tup:
            if key in sfp_interface_bulk_data['data']['Specification compliance']['value']:
                compliance_code_dict[key] = sfp_interface_bulk_data['data']['Specification compliance']['value'][key]['value']

        transceiver_info_dict['specification_compliance'] = str(
            compliance_code_dict)
        transceiver_info_dict['nominal_bit_rate'] = str(
            sfp_interface_bulk_data['data']['NominalSignallingRate(UnitsOf100Mbd)']['value'])

        self._dom_capability_detect()
        transceiver_info_dict['dom_capability'] = "{'sff8472_dom_support': 'yes'}" if self.dom_supported else "{'sff8472_dom_support': 'no'}"

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
        self._dom_capability_detect()

        transceiver_dom_info_dict = dict.fromkeys(dom_info_dict_keys, "N/A")
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

        transceiver_dom_info_dict['temperature'] = dom_temperature_data['data']['Temperature']['value']
        transceiver_dom_info_dict['voltage'] = dom_voltage_data['data']['Vcc']['value']
        transceiver_dom_info_dict['rx1power'] = dom_channel_monitor_data['data']['RXPower']['value']
        transceiver_dom_info_dict['tx1bias'] = dom_channel_monitor_data['data']['TXBias']['value']
        transceiver_dom_info_dict['tx1power'] = dom_channel_monitor_data['data']['TXPower']['value']
        transceiver_dom_info_dict['lp_mode'] = self.get_lpmode()
        transceiver_dom_info_dict['reset_status'] = self.get_reset_status()

        for key in transceiver_dom_info_dict:
            val = transceiver_dom_info_dict[key]
            transceiver_dom_info_dict[key] = self._convert_string_to_num(
                val) if type(val) is str else val

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
                                threshold_dict_keys, "N/A")
        offset = SFP_MODULE_ADDRA2_OFFSET

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

        for key in transceiver_dom_threshold_info_dict:
            transceiver_dom_threshold_info_dict[key] = self._convert_string_to_num(
                transceiver_dom_threshold_info_dict[key])

        return transceiver_dom_threshold_info_dict

    def get_reset_status(self):
        """
        Retrieves the reset status of SFP

        Returns:
            A Boolean, True if reset enabled, False if disabled
        """
        if self.sfp_type == COPPER_TYPE:
            return False

        if self.sfp_type == SFP_TYPE:
            return False

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
        if self.sfp_type == COPPER_TYPE:
            return False

        if smbus_present == 0:  # if called from sfputil outside of pmon
            cmdstatus, register = cmd.getstatusoutput('sudo i2cget -y 0 0x33 0x5')
            if cmdstatus:
                sonic_logger.log_warning("sfp cmdstatus i2c get failed %s" % register )
                return False
            register = int(register, 16)
        else:
            bus = smbus.SMBus(0)
            DEVICE_ADDRESS = 0x33
            DEVICE_REG = 0x5
            register = bus.read_byte_data(DEVICE_ADDRESS, DEVICE_REG)

        register = register & (1 << (self.port_num-SFP_PORT_START))
        if register == 0:
            return False

        return True

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
        raise NotImplementedError

    def get_tx_disable(self):
        """
        Retrieves the tx_disable status of this SFP

        Returns:
            A list of boolean values, representing the TX disable status
            of each available channel, value is True if SFP channel
            is TX disabled, False if not.
            E.g., for a tranceiver with four channels: [False, False, True, False]
        """
        tx_disable_list = []

        mask = 1 << (self.port_num - SFP_PORT_START)
        if self._tx_disable_bmp & mask:
            # If the bit is 1 (tx disabled), append True to the list
            tx_disable_list.append(True)
        else:
            # If the bit is 0 (tx enabled), append False to the list
            tx_disable_list.append(False)

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
        raise NotImplementedError

    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this SFP

        Returns:
            A Boolean, True if lpmode is enabled, False if disabled
        """
        return False

    def get_power_override(self):
        """
        Retrieves the power-override status of this SFP

        Returns:
            A Boolean, True if power-override is enabled, False if disabled
        """
        return False

    def get_temperature(self):
        """
        Retrieves the temperature of this SFP

        Returns:
            A float representing the current temperature in Celsius
        """
        transceiver_dom_info_dict = self.get_transceiver_bulk_status()
        return transceiver_dom_info_dict.get("temperature", "N/A")


    def get_voltage(self):
        """
        Retrieves the supply voltage of this SFP

        Returns:
            A float representing the supply voltage in mV
        """
        transceiver_dom_info_dict = self.get_transceiver_bulk_status()
        return transceiver_dom_info_dict.get("voltage", "N/A")

    def get_tx_bias(self):
        """
        Retrieves the TX bias current of all SFP channels

        Returns:
            A list of floats, representing TX bias in mA
            for each available channel
            E.g., for a tranceiver with four channels: ['110.09', '111.12', '108.21', '112.09']
        """
        transceiver_dom_info_dict = self.get_transceiver_bulk_status()
        tx1_bs = transceiver_dom_info_dict.get("tx1bias", "N/A")
        return [tx1_bs]

    def get_rx_power(self):
        """
        Retrieves the received optical power of all SFP channels

        Returns:
            A list of floats, representing received optical
            power in mW for each available channel
            E.g., for a tranceiver with four channels: ['1.77', '1.71', '1.68', '1.70']
        """
        transceiver_dom_info_dict = self.get_transceiver_bulk_status()
        rx1_pw = transceiver_dom_info_dict.get("rx1power", "N/A")
        return [rx1_pw]

    def get_tx_power(self):
        """
        Retrieves the TX power of all SFP channels

        Returns:
            A list of floats, representing TX power in mW
            for each available channel
            E.g., for a tranceiver with four channels: ['1.86', '1.86', '1.86', '1.86']
        """
        transceiver_dom_info_dict = self.get_transceiver_bulk_status()
        tx1_pw = transceiver_dom_info_dict.get("tx1power", "N/A")
        return [tx1_pw]

    def reset(self):
        """
        Reset SFP and return all user module settings to their default srate.

        Returns:
            A boolean, True if successful, False if not
        """
        # RJ45 and SFP ports not resettable
        return False

    def tx_disable(self, tx_disable):
        """
        Disable SFP TX for all channels

        Args:
            tx_disable : A Boolean, True to enable tx_disable mode, False to disable
                         tx_disable mode.

        Returns:
            A boolean, True if tx_disable is set successfully, False if not
        """
        mask = 1 << (self.port_num - SFP_PORT_START)

        if tx_disable == False:
            Sfp._tx_disable_bmp = Sfp._tx_disable_bmp & ~mask
        else:
            Sfp._tx_disable_bmp = Sfp._tx_disable_bmp | mask

        if smbus_present == 0:  # if called from sfputil outside of pmon
            cmdstatus, output = cmd.getstatusoutput('sudo i2cset -y -m 0x0f 0 0x33 0x0 %d' % Sfp._tx_disable_bmp)
            if cmdstatus:
                sonic_logger.log_warning("sfp cmdstatus i2c write failed %s" % output )
                return False
        else:
            bus = smbus.SMBus(0)
            DEVICE_ADDRESS = 0x33
            DEVICE_REG = 0x0
            bus.write_byte_data(DEVICE_ADDRESS, DEVICE_REG, Sfp._tx_disable_bmp)

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
        raise NotImplementedError

    def set_lpmode(self, lpmode):
        """
        Sets the lpmode (low power mode) of SFP

        Args:
            lpmode: A Boolean, True to enable lpmode, False to disable it
            Note  : lpmode can be overridden by set_power_override

        Returns:
            A boolean, True if lpmode is set successfully, False if not
        """
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
        raise NotImplementedError

    def read_eeprom(self, offset, num_bytes):
        """
        read eeprom specfic bytes beginning from a random offset with size as num_bytes

        Args:
             offset :
                     Integer, the offset from which the read transaction will start
             num_bytes:
                     Integer, the number of bytes to be read

        Returns:
            bytearray, if raw sequence of bytes are read correctly from the offset of size num_bytes
            None, if the read_eeprom fails
        """
        return bytearray(self.__read_eeprom_specific_bytes_raw(offset, num_bytes))

    def write_eeprom(self, offset, num_bytes, write_buffer):
        """
        write eeprom specfic bytes beginning from a random offset with size as num_bytes
        and write_buffer as the required bytes

        Args:
             offset :
                     Integer, the offset from which the read transaction will start
             num_bytes:
                     Integer, the number of bytes to be written
             write_buffer:
                     bytearray, raw bytes buffer which is to be written beginning at the offset

        Returns:
            a Boolean, true if the write succeeded and false if it did not succeed.
        """
        # Not supported at the moment
        return False

    def get_error_description(self):
        """
        Retrives the error descriptions of the SFP module

        Returns:
            String that represents the current error descriptions of vendor specific errors
            In case there are multiple errors, they should be joined by '|',
            like: "Bad EEPROM|Unsupported cable"
        """
        if not self.get_presence():
            return self.SFP_STATUS_UNPLUGGED
        return self.SFP_STATUS_OK

########  CUSTOM SFP API  #########

    def get_transceiver_pm(self):
        '''
        xcvr api is not implemented
        '''
        return None

        """
        Below part is added to avoid failing xcvrd
        """

    def get_transceiver_status(self):
        raise NotImplementedError

    def get_transceiver_info_firmware_versions(self):
        raise NotImplementedError
    
    def get_transceiver_threshold_info(self):
        return None

    """
    The methods below are part of the transceiver API that xcvrd calls but that
    SfpBase does not define - only SfpOptoeBase does. Without them the call
    raises AttributeError instead of NotImplementedError, which xcvrd does not
    catch, so its DOM thread dies and the daemon restarts in a loop. Raising
    NotImplementedError is the contract xcvrd expects: it then falls back to an
    empty result for this platform.
    """

    def get_transceiver_dom_flags(self):
        raise NotImplementedError

    def get_transceiver_dom_real_value(self):
        raise NotImplementedError

    def get_transceiver_status_flags(self):
        raise NotImplementedError

    def get_transceiver_vdm_flags(self):
        raise NotImplementedError

    def get_transceiver_vdm_real_value_basic(self):
        raise NotImplementedError

    def get_transceiver_vdm_real_value_statistic(self):
        raise NotImplementedError

    def get_transceiver_vdm_thresholds(self):
        raise NotImplementedError

    def is_transceiver_vdm_supported(self):
        raise NotImplementedError

    def is_vdm_statistic_supported(self):
        raise NotImplementedError
