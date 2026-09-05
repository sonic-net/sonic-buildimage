# Name: sfputil.py version: 1.0
#
# Description: Platform-specific SFP transceiver interface for Juniper QFX5200
#
# Copyright (c) 2020, Juniper Networks, Inc.
# All rights reserved.
#
# Notice and Disclaimer: This code is licensed to you under the GNU General
# Public License as published by the Free Software Foundation, version 3 or
# any later version. This code is not an official Juniper product. You can
# obtain a copy of the License at <https://www.gnu.org/licenses/>
#
# OSS License:
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
# Third-Party Code: This code may depend on other components under separate
# copyright notice and license terms.  Your use of the source code for those
# components is subject to the terms and conditions of the respective license
# as noted in the Third-Party source code file.

try:
    import time
    import os.path
    import glob
    import io
    from sonic_sfp.sfputilbase import SfpUtilBase
    from sonic_platform_base.sonic_sfp.sff8472 import sff8472Dom
    from sonic_platform_base.sonic_sfp.sff8472 import sff8472InterfaceId
    from sonic_platform_base.sonic_sfp.sff8436 import sff8436Dom
    from sonic_platform_base.sonic_sfp.sff8436 import sff8436InterfaceId
    from sonic_py_common.logger import Logger
    from ctypes import create_string_buffer

except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))

SYSLOG_IDENTIFIER = "sfputil"

# Global Logger class instance
logger = Logger(SYSLOG_IDENTIFIER)

qfx5200_qsfp_cable_length_tup = ('Length(km)', 'Length OM3(2m)',
                                 'Length OM2(m)', 'Length OM1(m)',
                                 'Length Cable Assembly(m)')

qfx5200_sfp_cable_length_tup = ('LengthSMFkm-UnitsOfKm', 'LengthSMF(UnitsOf100m)',
                                'Length50um(UnitsOf10m)', 'Length62.5um(UnitsOfm)',
                                'LengthCable(UnitsOfm)', 'LengthOM3(UnitsOf10m)')

qfx5200_sfp_compliance_code_tup = ('10GEthernetComplianceCode', 'InfinibandComplianceCode',
                                   'ESCONComplianceCodes', 'SONETComplianceCodes',
                                   'EthernetComplianceCodes', 'FibreChannelLinkLength',
                                   'FibreChannelTechnology', 'SFP+CableTechnology',
                                   'FibreChannelTransmissionMedia', 'FibreChannelSpeed')

qfx5200_qsfp_compliance_code_tup = ('10/40G Ethernet Compliance Code', 'SONET Compliance codes',
                                    'SAS/SATA compliance codes', 'Gigabit Ethernet Compliant codes',
                                    'Fibre Channel link length/Transmitter Technology',
                                    'Fibre Channel transmission media', 'Fibre Channel Speed')


GPIO_SLAVE0_PORT_START = 0
GPIO_SLAVE0_PORT_END = 15
GPIO_SLAVE1_PORT_START = 16
GPIO_SLAVE1_PORT_END = 31

GPIO_PORT_START = 0
GPIO_PORT_END = 31

GPIO_PRESENCE_OFFSET = 16
GPIO_LPMODE_OFFSET = 48
GPIO_RESET_OFFSET = 0

gpio_sfp_presence = {}
gpio_sfp_lpmode = {}
gpio_sfp_reset = {}

SFP_I2C_OFFSET = 14


# definitions of the offset and width for values in XCVR info eeprom
XCVR_INTFACE_BULK_OFFSET = 0
XCVR_INTFACE_BULK_WIDTH_QSFP = 20
XCVR_INTFACE_BULK_WIDTH_SFP = 21
XCVR_TYPE_WIDTH = 1
XCVR_VENDOR_NAME_OFFSET = 20
XCVR_VENDOR_NAME_WIDTH = 16
XCVR_VENDOR_OUI_OFFSET = 37
XCVR_VENDOR_OUI_WIDTH = 3
XCVR_VENDOR_PN_OFFSET = 40
XCVR_VENDOR_PN_WIDTH = 16
XCVR_HW_REV_OFFSET = 56
XCVR_HW_REV_WIDTH_OSFP = 2
XCVR_HW_REV_WIDTH_QSFP = 2
XCVR_HW_REV_WIDTH_SFP = 4
XCVR_VENDOR_SN_OFFSET = 68
XCVR_VENDOR_SN_WIDTH = 16
XCVR_VENDOR_DATE_OFFSET = 84
XCVR_VENDOR_DATE_WIDTH = 8
XCVR_DOM_CAPABILITY_OFFSET = 92
XCVR_DOM_CAPABILITY_WIDTH = 1


# definitions of the offset for values in OSFP info eeprom
OSFP_TYPE_OFFSET = 0
OSFP_VENDOR_NAME_OFFSET = 129
OSFP_VENDOR_PN_OFFSET = 148
OSFP_HW_REV_OFFSET = 164
OSFP_VENDOR_SN_OFFSET = 166

# definitions of the offset and width for values in DOM info eeprom
QSFP_DOM_REV_OFFSET = 1
QSFP_DOM_REV_WIDTH = 1
QSFP_TEMPE_OFFSET = 22
QSFP_TEMPE_WIDTH = 2
QSFP_VOLT_OFFSET = 26
QSFP_VOLT_WIDTH = 2
QSFP_CHANNL_MON_OFFSET = 34
QSFP_CHANNL_MON_WIDTH = 16
QSFP_CHANNL_MON_WITH_TX_POWER_WIDTH = 24
QSFP_MODULE_THRESHOLD_OFFSET = 128
QSFP_MODULE_THRESHOLD_WIDTH = 24
QSFP_CHANNL_THRESHOLD_OFFSET = 176
QSFP_CHANNL_THRESHOLD_WIDTH = 16


SFP_TEMPE_OFFSET = 96
SFP_TEMPE_WIDTH = 2
SFP_VOLT_OFFSET = 98
SFP_VOLT_WIDTH = 2
SFP_CHANNL_MON_OFFSET = 100
SFP_CHANNL_MON_WIDTH = 6


def gpio_create_file(gpio_pin):
    gpio_export_path = "/sys/class/gpio/export"
    gpio_pin_path = "/sys/class/gpio/gpio" + str(gpio_pin)
    if not os.path.exists(gpio_pin_path):
        try:
            gpio_export_file = open(gpio_export_path, 'w')
            gpio_export_file.write(str(gpio_pin))
            gpio_export_file.close()
        except IOError as e:
            print("Error: unable to open export file: %s" % str(e))
            return False

    return True


def gpio_sfp_port_init(gpio_base, port):
    presence_pin = gpio_base + GPIO_PRESENCE_OFFSET + (port % 16)
    if gpio_create_file(presence_pin):
        gpio_sfp_presence[port] = presence_pin
    reset_pin = gpio_base + GPIO_RESET_OFFSET + (port % 16)
    if gpio_create_file(reset_pin):
        gpio_sfp_reset[port] = reset_pin
    lpmode_pin = gpio_base + GPIO_LPMODE_OFFSET + (port % 16)
    if gpio_create_file(lpmode_pin):
        gpio_sfp_lpmode[port] = lpmode_pin


def gpio_sfp_slave_init(gpio_base_path, gpio_port_start, gpio_port_end):
    flist = glob.glob(gpio_base_path)
    if len(flist) == 1:
        try:
            fp = open(flist[0]+"/base")
            gpio_base = int(fp.readline().rstrip())
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return

        for port in range(gpio_port_start, gpio_port_end + 1):
            gpio_sfp_port_init(gpio_base, port)


def gpio_sfp_base_init():
    gpio_sfp_slave_init("/sys/bus/platform/drivers/gpioslave-tmc/gpioslave-tmc.21/gpio/gpio*",
                        GPIO_SLAVE0_PORT_START, GPIO_SLAVE0_PORT_END)
    gpio_sfp_slave_init("/sys/bus/platform/drivers/gpioslave-tmc/gpioslave-tmc.22/gpio/gpio*",
                        GPIO_SLAVE1_PORT_START, GPIO_SLAVE1_PORT_END)


def gpio_sfp_read(gpio_pin):
    gpio_pin_path = "/sys/class/gpio/gpio" + str(gpio_pin)
    value = 0

    try:
        reg_file = open(gpio_pin_path + "/value")
        value = int(reg_file.readline().rstrip())
    except IOError as e:
        print("error: unable to open file: %s" % str(e))

    return value


def gpio_sfp_write(gpio_pin, value):
    success = False
    gpio_pin_path = "/sys/class/gpio/gpio" + str(gpio_pin)

    try:
        gpio_file = open(gpio_pin_path + "/value", 'w')
        gpio_file.write(str(value))
        success = True
    except IOError as e:
        print("error: unable to open file: %s" % str(e))

    return success


def gpio_sfp_presence_get(port):
    if port not in list(gpio_sfp_presence.keys()):
        print("Port:" + str(port) + " not in sfp dict")
        return 0

    gpio_pin = gpio_sfp_presence[port]
    return gpio_sfp_read(gpio_pin)


def gpio_sfp_lpmode_get(port):
    if port not in list(gpio_sfp_lpmode.keys()):
        return 0

    gpio_pin = gpio_sfp_lpmode[port]
    return gpio_sfp_read(gpio_pin)


def gpio_sfp_lpmode_set(port, value):
    if port not in list(gpio_sfp_lpmode.keys()):
        return False

    gpio_pin = gpio_sfp_lpmode[port]
    return gpio_sfp_write(gpio_pin, value)


def gpio_sfp_reset_set(port, value):
    if port not in list(gpio_sfp_reset.keys()):
        return False

    gpio_pin = gpio_sfp_reset[port]
    return gpio_sfp_write(gpio_pin, value)


class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    PORT_START = 0
    PORT_END = 31
    PORTS_IN_BLOCK = 32
    QSFP_PORT_START = 0
    QSFP_PORT_END = 31

    cmd = '/var/run/sfppresence'

    _port_to_eeprom_mapping = {}
    port_to_i2cbus_mapping = {
        0: 14,
        1: 15,
        2: 16,
        3: 17,
        4: 18,
        5: 19,
        6: 20,
        7: 21,
        8: 22,
        9: 23,
        10: 24,
        11: 25,
        12: 26,
        13: 27,
        14: 28,
        15: 29,
        16: 30,
        17: 31,
        18: 32,
        19: 33,
        20: 34,
        21: 35,
        22: 36,
        23: 37,
        24: 38,
        25: 39,
        26: 40,
        27: 41,
        28: 42,
        29: 43,
        30: 44,
        31: 45
    }

    port_ctle_settings = {
        0: 119,  # 0x77
        1: 102,  # 0x66
        2: 102,  # 0x66
        3: 102,  # 0x66
        4: 102,  # 0x66
        5: 85,   # 0x55
        6: 102,  # 0x66
        7: 85,   # 0x55
        8: 85,   # 0x55
        9: 85,   # 0x55
        10: 119,  # 0x77
        11: 119,  # 0x77
        12: 102,  # 0x66
        13: 85,  # 0x55
        14: 68,  # 0x44
        15: 51,  # 0x33
        16: 68,  # 0x44
        17: 85,  # 0x55
        18: 102,  # 0x66
        19: 102,  # 0x66
        20: 119,  # 0X77
        21: 102,  # 0x66
        22: 85,  # 0X55
        23: 85,  # 0X55
        24: 85,  # 0x55
        25: 85,  # 0x55
        26: 102,  # 0x66
        27: 85,  # 0x55
        28: 102,  # 0x66
        29: 102,  # 0x66
        30: 119,  # 0x77
        31: 119  # 0x77
    }

    optics_list_100g = {
        "AFBR-89CDDZ-JU1",
        "AFBR-89CDDZ-JU2",
        "FTLC9551REPM-J1",
        "FCBN425QE1C30-J1",
        "FCBN425QE1C10-J1",
        "FTLC9551REPM-J3",
        "LUX42604CO",
        "LUX42604BO",
        "EOLQ-161HG-10-LJ1",
        "FTLC1151RDPL-J1",
        "TR-FC13R-NJC",
        "TR-FC13L-NJC",
        "TR-FC13R-NJ3",
        "SPQ-CE-LR-CDFB-J2",
        "1K1QAC",
        "SPQCELRCDFAJ2"
    }

    optics_list_40g = {
        "FTL4C1QE1C-J1"
    }

    def clear_bit(self, data, pos):
        return (data & (~(1 << pos)))

    def check_bit(self, data, pos):
        return (data & (1 << pos))

    def set_bit(self, data, pos):
        return (data | (1 << pos))

    def is_100g_optics(self, part_num):
        ret = part_num in self.optics_list_100g
        return ret

    def is_40g_optics(self, part_num):
        ret = part_num in self.optics_list_40g
        return ret

    def is_optics(self, part_num):
        if self.is_100g_optics(part_num):
            return True
        elif self.is_40g_optics(part_num):
            return True
        else:
            return False

    def process_TxCTLE(self, eeprom, port_num, part_num):
        if self.is_100g_optics(part_num):
            logger.log_debug(" QFX5200: TxCTLE port {} and SFP PN NUM {} ".format(port_num, part_num))
            # Accessing page 3 of optics
            regval = 3
            buffer = create_string_buffer(1)
            buffer[0] = chr(regval)
            eeprom.seek(127)
            eeprom.write(buffer[0])

            regval = self.port_ctle_settings[port_num]
            logger.log_debug("QFX5200: TxCTLE port {}, SFP PN NUM {}, regval {} ".format(port_num, part_num, regval))

            eeprom.seek(234)
            buffer[0] = chr(regval)
            eeprom.write(buffer[0])
            eeprom.seek(235)
            eeprom.write(buffer[0])
            # Moving back the optics page to 0
            regval = 0x0
            buffer[0] = chr(regval)
            eeprom.seek(127)
            eeprom.write(buffer[0])
        else:
            pass

    def is_highpower_optics(self, data):
        return (self.check_bit(data, 0) | self.check_bit(data, 1))

    def is_cdr_present(self, data):
        return (self.check_bit(data, 2) | self.check_bit(data, 3))

    @property
    def port_start(self):
        return self.PORT_START

    @property
    def port_end(self):
        return self.PORT_END

    @property
    def qsfp_port_start(self):
        return self.QSFP_PORT_START

    @property
    def qsfp_port_end(self):
        return self.QSFP_PORT_END

    @property
    def qsfp_ports(self):
        return list(range(0, self.PORTS_IN_BLOCK + 1))

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

    def __init__(self):
        gpio_sfp_base_init()
        eeprom_path = "/sys/class/i2c-adapter/i2c-{0}/{0}-0050/eeprom"
        for x in range(0, self.port_end + 1):
            self.port_to_eeprom_mapping[x] = eeprom_path.format(self.port_to_i2cbus_mapping[x])

        logger.log_debug("QFX5200: SfpUtil __init__")
        if os.path.isfile(self.cmd):
            logger.log_debug("QFX5200: SfpUtil removing sfppresence file")
            os.remove(self.cmd)

        SfpUtilBase.__init__(self)

    def get_presence(self, port_num):
        reg_value = gpio_sfp_presence_get(port_num)
        if reg_value == 1:
            return True

        return False

    def get_low_power_mode(self, port_num):
        reg_value = gpio_sfp_lpmode_get(port_num)
        if reg_value == 0:
            return True

        return False

    def set_low_power_mode(self, port_num, lpmode):

        if lpmode == False:
            lpmode = 1
        else:
            lpmode = 0

        status = gpio_sfp_lpmode_set(port_num, lpmode)
        return status

    def reset(self, port_num):
        reset_val = 0
        status = gpio_sfp_reset_set(port_num, reset_val)
        return status

    #  Writing to a file from a list
    def write_to_file(self, file_name, from_list):
        try:
            fp1 = open(file_name, 'w')
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        for i in from_list:
            fp1.write(i)
            fp1.write('\n')

        fp1.close()
        return True

    # Reading from a file to a list

    def read_from_file(self, file_name):
        try:
            fp = open(file_name, 'r')
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        to_list = fp.readlines()
        to_list = [x.rstrip() for x in to_list]
        fp.close()
        return to_list

    def sfp_detect(self):
        port = 0
        ret_dict = {}
        defl_dict = {}
        current_sfp_values = [0] * 32
        previous_sfp_values = [0] * 32

        logger.log_debug("QFX5200: sfp_detect Start")
        if not os.path.isfile(self.cmd):
            logger.log_debug("QFX5200: sfppresence file not created")
        else:
            if (self.read_from_file(self.cmd) == False):
                logger.log_debug("QFX5200: sfp_detect not able to open sfppresence file")
                return False, defl_dict
            else:
                previous_sfp_values = self.read_from_file(self.cmd)
                logger.log_debug("QFX5200: sfp_detect sfppresence file present, previous_sfp_values populated")

        # Read the current values from sysfs
        for port in range(GPIO_PORT_START, GPIO_PORT_END + 1):
            sfp_present = gpio_sfp_presence_get(port)
            current_sfp_values[port] = str(sfp_present)
            ret_dict.update({port: current_sfp_values[port]})

            prev_value = str(previous_sfp_values[port])

            current_value = current_sfp_values[port]
            if (prev_value != current_value):
                ret_dict.update({port: current_sfp_values[port]})
                value = str(current_sfp_values[port])
                if (value):
                    offset = 128
                    sfpi_obj = sff8436InterfaceId()
                    if sfpi_obj is None:
                        return None

                    file_path = self._get_port_eeprom_path(port, self.IDENTITY_EEPROM_ADDR)
                    if not self._sfp_eeprom_present(file_path, 0):
                        print("Error, file not exist %s" % file_path)
                        return None

                    try:
                        sysfsfile_eeprom = open(file_path, mode="rb", buffering=0)
                    except IOError:
                        print("Error: reading sysfs file %s" % file_path)
                        return None

                    sfp_vendor_pn_raw = self._read_eeprom_specific_bytes(
                        sysfsfile_eeprom, (offset + XCVR_VENDOR_PN_OFFSET), XCVR_VENDOR_PN_WIDTH)
                    if sfp_vendor_pn_raw is not None:
                        sfp_vendor_pn_data = sfpi_obj.parse_vendor_pn(sfp_vendor_pn_raw, 0)
                    else:
                        return None

                    try:
                        sysfsfile_eeprom.close()
                    except IOError:
                        print("Error: closing sysfs file %s" % file_path)
                        return None

                    sfp_pn_num = str(sfp_vendor_pn_data['data']['Vendor PN']['value'])
                    if self.is_optics(sfp_pn_num):
                        logger.log_debug(
                            "QFX5200: Port {} is connected with Optics with Part Number {}".format(port, sfp_pn_num))
                        eeprom1 = open(self._port_to_eeprom_mapping[port],  "r+b")
                        self.process_TxCTLE(eeprom1, port, sfp_pn_num)
                        eeprom1.close
                # End of if(value)
            # End of current_sfp_values != previous_sfp_vlalues
        # End of for loop

        if(self.write_to_file(self.cmd, current_sfp_values) == True):
            logger.log_debug("QFX5200: sfp_detect, current sfp values written successfully to sfppresence file")
            logger.log_debug("QFX5200: sfp_detect ret_dict is {}".format(ret_dict))
            return True, ret_dict
        else:
            logger.log_debug("QFX5200: sfp_detect, current sfp values not written to sfppresence file")
            logger.log_debug("QFX5200: sfp_detect ret_dict is {}".format(def1_dict))
            return False, defl_dict

    # Read out SFP type, vendor name, PN, REV, SN from eeprom.
    def get_transceiver_info_dict(self, port_num):
        transceiver_info_dict = {}
        compliance_code_dict = {}
        logger.log_debug("QFX5200: get_transceiver_info_dict Start")
        # ToDo: OSFP tranceiver info parsing not fully supported.
        # in inf8628.py lack of some memory map definition
        # will be implemented when the inf8628 memory map ready
        if port_num in self.osfp_ports:
            offset = 0
            vendor_rev_width = XCVR_HW_REV_WIDTH_OSFP

            sfpi_obj = inf8628InterfaceId()
            if sfpi_obj is None:
                print("Error: sfp_object open failed")
                return None

            file_path = self._get_port_eeprom_path(port_num, self.IDENTITY_EEPROM_ADDR)
            if not self._sfp_eeprom_present(file_path, 0):
                print("Error, file not exist %s" % file_path)
                return None

            try:
                sysfsfile_eeprom = open(file_path, mode="rb", buffering=0)
            except IOError:
                print("Error: reading sysfs file %s" % file_path)
                return None

            sfp_type_raw = self._read_eeprom_specific_bytes(
                sysfsfile_eeprom, (offset + OSFP_TYPE_OFFSET), XCVR_TYPE_WIDTH)
            if sfp_type_raw is not None:
                sfp_type_data = sfpi_obj.parse_sfp_type(sfp_type_raw, 0)
            else:
                return None

            sfp_vendor_name_raw = self._read_eeprom_specific_bytes(
                sysfsfile_eeprom, (offset + OSFP_VENDOR_NAME_OFFSET), XCVR_VENDOR_NAME_WIDTH)
            if sfp_vendor_name_raw is not None:
                sfp_vendor_name_data = sfpi_obj.parse_vendor_name(sfp_vendor_name_raw, 0)
            else:
                return None

            sfp_vendor_pn_raw = self._read_eeprom_specific_bytes(
                sysfsfile_eeprom, (offset + OSFP_VENDOR_PN_OFFSET), XCVR_VENDOR_PN_WIDTH)
            if sfp_vendor_pn_raw is not None:
                sfp_vendor_pn_data = sfpi_obj.parse_vendor_pn(sfp_vendor_pn_raw, 0)
            else:
                return None

            sfp_vendor_rev_raw = self._read_eeprom_specific_bytes(
                sysfsfile_eeprom, (offset + OSFP_HW_REV_OFFSET), vendor_rev_width)
            if sfp_vendor_rev_raw is not None:
                sfp_vendor_rev_data = sfpi_obj.parse_vendor_rev(sfp_vendor_rev_raw, 0)
            else:
                return None

            sfp_vendor_sn_raw = self._read_eeprom_specific_bytes(
                sysfsfile_eeprom, (offset + OSFP_VENDOR_SN_OFFSET), XCVR_VENDOR_SN_WIDTH)
            if sfp_vendor_sn_raw is not None:
                sfp_vendor_sn_data = sfpi_obj.parse_vendor_sn(sfp_vendor_sn_raw, 0)
            else:
                return None

            try:
                sysfsfile_eeprom.close()
            except IOError:
                print("Error: closing sysfs file %s" % file_path)
                return None

            transceiver_info_dict['type'] = sfp_type_data['data']['type']['value']
            transceiver_info_dict['type_abbrv_name'] = sfp_type_data['data']['type_abbrv_name']['value']
            transceiver_info_dict['manufacturer'] = sfp_vendor_name_data['data']['Vendor Name']['value']
            transceiver_info_dict['model'] = sfp_vendor_pn_data['data']['Vendor PN']['value']
            transceiver_info_dict['vendor_rev'] = sfp_vendor_rev_data['data']['Vendor Rev']['value']
            transceiver_info_dict['serial'] = sfp_vendor_sn_data['data']['Vendor SN']['value']
            # Below part is added to avoid fail the xcvrd, shall be implemented later
            transceiver_info_dict['vendor_oui'] = 'N/A'
            transceiver_info_dict['vendor_date'] = 'N/A'
            transceiver_info_dict['connector'] = 'N/A'
            transceiver_info_dict['encoding'] = 'N/A'
            transceiver_info_dict['ext_identifier'] = 'N/A'
            transceiver_info_dict['ext_rateselect_compliance'] = 'N/A'
            transceiver_info_dict['cable_type'] = 'N/A'
            transceiver_info_dict['cable_length'] = 'N/A'
            transceiver_info_dict['specification_compliance'] = 'N/A'
            transceiver_info_dict['nominal_bit_rate'] = 'N/A'

        else:
            if port_num in self.qsfp_ports:
                offset = 128
                vendor_rev_width = XCVR_HW_REV_WIDTH_QSFP
                interface_info_bulk_width = XCVR_INTFACE_BULK_WIDTH_QSFP
                sfp_type = 'QSFP'

                sfpi_obj = sff8436InterfaceId()
                if sfpi_obj is None:
                    print("Error: sfp_object open failed")
                    return None

            else:
                offset = 0
                vendor_rev_width = XCVR_HW_REV_WIDTH_SFP
                interface_info_bulk_width = XCVR_INTFACE_BULK_WIDTH_SFP
                sfp_type = 'SFP'

                sfpi_obj = sff8472InterfaceId()
                if sfpi_obj is None:
                    print("Error: sfp_object open failed")
                    return None

            file_path = self._get_port_eeprom_path(port_num, self.IDENTITY_EEPROM_ADDR)
            if not self._sfp_eeprom_present(file_path, 0):
                print("Error, file not exist %s" % file_path)
                return None

            try:
                sysfsfile_eeprom = open(file_path, mode="rb", buffering=0)
            except IOError:
                print("Error: reading sysfs file %s" % file_path)
                return None

            sfp_interface_bulk_raw = self._read_eeprom_specific_bytes(
                sysfsfile_eeprom, (offset + XCVR_INTFACE_BULK_OFFSET), interface_info_bulk_width)
            if sfp_interface_bulk_raw is not None:
                sfp_interface_bulk_data = sfpi_obj.parse_sfp_info_bulk(sfp_interface_bulk_raw, 0)
            else:
                return None

            sfp_vendor_name_raw = self._read_eeprom_specific_bytes(
                sysfsfile_eeprom, (offset + XCVR_VENDOR_NAME_OFFSET), XCVR_VENDOR_NAME_WIDTH)
            if sfp_vendor_name_raw is not None:
                sfp_vendor_name_data = sfpi_obj.parse_vendor_name(sfp_vendor_name_raw, 0)
            else:
                return None

            sfp_vendor_pn_raw = self._read_eeprom_specific_bytes(
                sysfsfile_eeprom, (offset + XCVR_VENDOR_PN_OFFSET), XCVR_VENDOR_PN_WIDTH)
            if sfp_vendor_pn_raw is not None:
                sfp_vendor_pn_data = sfpi_obj.parse_vendor_pn(sfp_vendor_pn_raw, 0)
            else:
                return None

            sfp_vendor_rev_raw = self._read_eeprom_specific_bytes(
                sysfsfile_eeprom, (offset + XCVR_HW_REV_OFFSET), vendor_rev_width)
            if sfp_vendor_rev_raw is not None:
                sfp_vendor_rev_data = sfpi_obj.parse_vendor_rev(sfp_vendor_rev_raw, 0)
            else:
                return None

            sfp_vendor_sn_raw = self._read_eeprom_specific_bytes(
                sysfsfile_eeprom, (offset + XCVR_VENDOR_SN_OFFSET), XCVR_VENDOR_SN_WIDTH)
            if sfp_vendor_sn_raw is not None:
                sfp_vendor_sn_data = sfpi_obj.parse_vendor_sn(sfp_vendor_sn_raw, 0)
            else:
                return None

            sfp_vendor_oui_raw = self._read_eeprom_specific_bytes(
                sysfsfile_eeprom, (offset + XCVR_VENDOR_OUI_OFFSET), XCVR_VENDOR_OUI_WIDTH)
            if sfp_vendor_oui_raw is not None:
                sfp_vendor_oui_data = sfpi_obj.parse_vendor_oui(sfp_vendor_oui_raw, 0)
            else:
                return None

            sfp_vendor_date_raw = self._read_eeprom_specific_bytes(
                sysfsfile_eeprom, (offset + XCVR_VENDOR_DATE_OFFSET), XCVR_VENDOR_DATE_WIDTH)
            if sfp_vendor_date_raw is not None:
                sfp_vendor_date_data = sfpi_obj.parse_vendor_date(sfp_vendor_date_raw, 0)
            else:
                return None

            try:
                sysfsfile_eeprom.close()
            except IOError:
                print("Error: closing sysfs file %s" % file_path)
                return None

            transceiver_info_dict['type'] = sfp_interface_bulk_data['data']['type']['value']
            transceiver_info_dict['type_abbrv_name'] = sfp_interface_bulk_data['data']['type_abbrv_name']['value']
            transceiver_info_dict['manufacturer'] = sfp_vendor_name_data['data']['Vendor Name']['value']
            transceiver_info_dict['model'] = sfp_vendor_pn_data['data']['Vendor PN']['value']
            transceiver_info_dict['vendor_rev'] = sfp_vendor_rev_data['data']['Vendor Rev']['value']
            transceiver_info_dict['serial'] = sfp_vendor_sn_data['data']['Vendor SN']['value']
            transceiver_info_dict['vendor_oui'] = sfp_vendor_oui_data['data']['Vendor OUI']['value']
            transceiver_info_dict['vendor_date'] = sfp_vendor_date_data[
                'data']['VendorDataCode(YYYY-MM-DD Lot)']['value']
            transceiver_info_dict['connector'] = sfp_interface_bulk_data['data']['Connector']['value']
            transceiver_info_dict['encoding'] = sfp_interface_bulk_data['data']['EncodingCodes']['value']
            transceiver_info_dict['ext_identifier'] = sfp_interface_bulk_data['data']['Extended Identifier']['value']
            transceiver_info_dict['ext_rateselect_compliance'] = sfp_interface_bulk_data['data']['RateIdentifier']['value']
            if sfp_type == 'QSFP':
                for key in qfx5200_qsfp_cable_length_tup:
                    if key in sfp_interface_bulk_data['data']:
                        transceiver_info_dict['cable_type'] = key
                        transceiver_info_dict['cable_length'] = str(sfp_interface_bulk_data['data'][key]['value'])
                        break
                    else:
                        transceiver_info_dict['cable_type'] = key
                        transceiver_info_dict['cable_length'] = 'N/A'

                for key in qfx5200_qsfp_compliance_code_tup:
                    if key in sfp_interface_bulk_data['data']['Specification compliance']['value']:
                        compliance_code_dict[key] = sfp_interface_bulk_data['data']['Specification compliance']['value'][key]['value']
                transceiver_info_dict['specification_compliance'] = str(compliance_code_dict)

                if 'Nominal Bit Rate(100Mbs)' in sfp_interface_bulk_data['data']:
                    transceiver_info_dict['nominal_bit_rate'] = str(
                        sfp_interface_bulk_data['data']['Nominal Bit Rate(100Mbs)']['value'])
                else:
                    transceiver_info_dict['nominal_bit_rate'] = 'N/A'
            else:
                for key in qfx5200_sfp_cable_length_tup:
                    if key in sfp_interface_bulk_data['data']:
                        transceiver_info_dict['cable_type'] = key
                        transceiver_info_dict['cable_length'] = str(sfp_interface_bulk_data['data'][key]['value'])
                    else:
                        transceiver_info_dict['cable_type'] = key
                        transceiver_info_dict['cable_length'] = 'N/A'

                for key in qfx5200_sfp_compliance_code_tup:
                    if key in sfp_interface_bulk_data['data']['Specification compliance']['value']:
                        compliance_code_dict[key] = sfp_interface_bulk_data['data']['Specification compliance']['value'][key]['value']
                transceiver_info_dict['specification_compliance'] = str(compliance_code_dict)

                if 'NominalSignallingRate(UnitsOf100Mbd)' in sfp_interface_bulk_data['data']:
                    transceiver_info_dict['nominal_bit_rate'] = str(
                        sfp_interface_bulk_data['data']['NominalSignallingRate(UnitsOf100Mbd)']['value'])
                else:
                    transceiver_info_dict['nominal_bit_rate'] = 'N/A'

        logger.log_debug("QFX5200: get_transceiver_info_dict End")
        return transceiver_info_dict

    def get_transceiver_dom_info_dict(self, port_num):
        transceiver_dom_info_dict = {}

        logger.log_debug("QFX5200: get_transceiver_dom_info_dict Start")
        if port_num in self.osfp_ports:
            # Below part is added to avoid fail xcvrd, shall be implemented later
            transceiver_dom_info_dict['temperature'] = 'N/A'
            transceiver_dom_info_dict['voltage'] = 'N/A'
            transceiver_dom_info_dict['rx1power'] = 'N/A'
            transceiver_dom_info_dict['rx2power'] = 'N/A'
            transceiver_dom_info_dict['rx3power'] = 'N/A'
            transceiver_dom_info_dict['rx4power'] = 'N/A'
            transceiver_dom_info_dict['tx1bias'] = 'N/A'
            transceiver_dom_info_dict['tx2bias'] = 'N/A'
            transceiver_dom_info_dict['tx3bias'] = 'N/A'
            transceiver_dom_info_dict['tx4bias'] = 'N/A'
            transceiver_dom_info_dict['tx1power'] = 'N/A'
            transceiver_dom_info_dict['tx2power'] = 'N/A'
            transceiver_dom_info_dict['tx3power'] = 'N/A'
            transceiver_dom_info_dict['tx4power'] = 'N/A'

        elif port_num in self.qsfp_ports:
            offset = 0
            offset_xcvr = 128
            file_path = self._get_port_eeprom_path(port_num, self.IDENTITY_EEPROM_ADDR)
            if not self._sfp_eeprom_present(file_path, 0):
                return None

            try:
                sysfsfile_eeprom = open(file_path, mode="rb", buffering=0)
            except IOError:
                print("Error: reading sysfs file %s" % file_path)
                return None

            sfpd_obj = sff8436Dom()
            if sfpd_obj is None:
                return None

            sfpi_obj = sff8436InterfaceId()
            if sfpi_obj is None:
                return None

            # QSFP capability byte parse, through this byte can know whether it support tx_power or not.
            # TODO: in the future when decided to migrate to support SFF-8636 instead of SFF-8436,
            # need to add more code for determining the capability and version compliance
            # in SFF-8636 dom capability definitions evolving with the versions.
            qsfp_dom_capability_raw = self._read_eeprom_specific_bytes(
                sysfsfile_eeprom, (offset_xcvr + XCVR_DOM_CAPABILITY_OFFSET), XCVR_DOM_CAPABILITY_WIDTH)
            if qsfp_dom_capability_raw is not None:
                qspf_dom_capability_data = sfpi_obj.parse_dom_capability(qsfp_dom_capability_raw, 0)
            else:
                return None

            dom_temperature_raw = self._read_eeprom_specific_bytes(
                sysfsfile_eeprom, (offset + QSFP_TEMPE_OFFSET), QSFP_TEMPE_WIDTH)
            if dom_temperature_raw is not None:
                dom_temperature_data = sfpd_obj.parse_temperature(dom_temperature_raw, 0)
            else:
                return None

            dom_voltage_raw = self._read_eeprom_specific_bytes(
                sysfsfile_eeprom, (offset + QSFP_VOLT_OFFSET), QSFP_VOLT_WIDTH)
            if dom_voltage_raw is not None:
                dom_voltage_data = sfpd_obj.parse_voltage(dom_voltage_raw, 0)
            else:
                return None

            qsfp_dom_rev_raw = self._read_eeprom_specific_bytes(
                sysfsfile_eeprom, (offset + QSFP_DOM_REV_OFFSET), QSFP_DOM_REV_WIDTH)
            if qsfp_dom_rev_raw is not None:
                qsfp_dom_rev_data = sfpd_obj.parse_sfp_dom_rev(qsfp_dom_rev_raw, 0)
            else:
                return None

            transceiver_dom_info_dict['temperature'] = dom_temperature_data['data']['Temperature']['value']
            transceiver_dom_info_dict['voltage'] = dom_voltage_data['data']['Vcc']['value']

            # The tx_power monitoring is only available on QSFP which compliant with SFF-8636
            # and claimed that it support tx_power with one indicator bit.
            dom_channel_monitor_data = {}
            qsfp_dom_rev = qsfp_dom_rev_data['data']['dom_rev']['value']
            qsfp_tx_power_support = qspf_dom_capability_data['data']['Tx_power_support']['value']
            if (qsfp_dom_rev[0:8] != 'SFF-8636' or (qsfp_dom_rev[0:8] == 'SFF-8636' and qsfp_tx_power_support != 'on')):
                dom_channel_monitor_raw = self._read_eeprom_specific_bytes(
                    sysfsfile_eeprom, (offset + QSFP_CHANNL_MON_OFFSET), QSFP_CHANNL_MON_WIDTH)
                if dom_channel_monitor_raw is not None:
                    dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params(dom_channel_monitor_raw, 0)
                else:
                    return None

                transceiver_dom_info_dict['tx1power'] = 'N/A'
                transceiver_dom_info_dict['tx2power'] = 'N/A'
                transceiver_dom_info_dict['tx3power'] = 'N/A'
                transceiver_dom_info_dict['tx4power'] = 'N/A'
            else:
                dom_channel_monitor_raw = self._read_eeprom_specific_bytes(
                    sysfsfile_eeprom, (offset + QSFP_CHANNL_MON_OFFSET), QSFP_CHANNL_MON_WITH_TX_POWER_WIDTH)
                if dom_channel_monitor_raw is not None:
                    dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params_with_tx_power(
                        dom_channel_monitor_raw, 0)
                else:
                    return None

                transceiver_dom_info_dict['tx1power'] = dom_channel_monitor_data['data']['TX1Power']['value']
                transceiver_dom_info_dict['tx2power'] = dom_channel_monitor_data['data']['TX2Power']['value']
                transceiver_dom_info_dict['tx3power'] = dom_channel_monitor_data['data']['TX3Power']['value']
                transceiver_dom_info_dict['tx4power'] = dom_channel_monitor_data['data']['TX4Power']['value']

            try:
                sysfsfile_eeprom.close()
            except IOError:
                print("Error: closing sysfs file %s" % file_path)
                return None

            transceiver_dom_info_dict['temperature'] = dom_temperature_data['data']['Temperature']['value']
            transceiver_dom_info_dict['voltage'] = dom_voltage_data['data']['Vcc']['value']
            transceiver_dom_info_dict['rx1power'] = dom_channel_monitor_data['data']['RX1Power']['value']
            transceiver_dom_info_dict['rx2power'] = dom_channel_monitor_data['data']['RX2Power']['value']
            transceiver_dom_info_dict['rx3power'] = dom_channel_monitor_data['data']['RX3Power']['value']
            transceiver_dom_info_dict['rx4power'] = dom_channel_monitor_data['data']['RX4Power']['value']
            transceiver_dom_info_dict['tx1bias'] = dom_channel_monitor_data['data']['TX1Bias']['value']
            transceiver_dom_info_dict['tx2bias'] = dom_channel_monitor_data['data']['TX2Bias']['value']
            transceiver_dom_info_dict['tx3bias'] = dom_channel_monitor_data['data']['TX3Bias']['value']
            transceiver_dom_info_dict['tx4bias'] = dom_channel_monitor_data['data']['TX4Bias']['value']

        else:
            offset = 256
            file_path = self._get_port_eeprom_path(port_num, self.DOM_EEPROM_ADDR)
            if not self._sfp_eeprom_present(file_path, 0):
                return None

            try:
                sysfsfile_eeprom = open(file_path, mode="rb", buffering=0)
            except IOError:
                print("Error: reading sysfs file %s" % file_path)
                return None

            sfpd_obj = sff8472Dom()
            if sfpd_obj is None:
                return None

            dom_temperature_raw = self._read_eeprom_specific_bytes(
                sysfsfile_eeprom, (offset + SFP_TEMPE_OFFSET), SFP_TEMPE_WIDTH)
            if dom_temperature_raw is not None:
                dom_temperature_data = sfpd_obj.parse_temperature(dom_temperature_raw, 0)
            else:
                return None

            dom_voltage_raw = self._read_eeprom_specific_bytes(
                sysfsfile_eeprom, (offset + SFP_VOLT_OFFSET), SFP_VOLT_WIDTH)
            if dom_voltage_raw is not None:
                dom_voltage_data = sfpd_obj.parse_voltage(dom_voltage_raw, 0)
            else:
                return None

            dom_channel_monitor_raw = self._read_eeprom_specific_bytes(
                sysfsfile_eeprom, (offset + SFP_CHANNL_MON_OFFSET), SFP_CHANNL_MON_WIDTH)
            if dom_channel_monitor_raw is not None:
                dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params(dom_channel_monitor_raw, 0)
            else:
                return None

            try:
                sysfsfile_eeprom.close()
            except IOError:
                print("Error: closing sysfs file %s" % file_path)
                return None

            transceiver_dom_info_dict['temperature'] = dom_temperature_data['data']['Temperature']['value']
            transceiver_dom_info_dict['voltage'] = dom_voltage_data['data']['Vcc']['value']
            transceiver_dom_info_dict['rx1power'] = dom_channel_monitor_data['data']['RXPower']['value']
            transceiver_dom_info_dict['rx2power'] = 'N/A'
            transceiver_dom_info_dict['rx3power'] = 'N/A'
            transceiver_dom_info_dict['rx4power'] = 'N/A'
            transceiver_dom_info_dict['tx1bias'] = dom_channel_monitor_data['data']['TXBias']['value']
            transceiver_dom_info_dict['tx2bias'] = 'N/A'
            transceiver_dom_info_dict['tx3bias'] = 'N/A'
            transceiver_dom_info_dict['tx4bias'] = 'N/A'
            transceiver_dom_info_dict['tx1power'] = dom_channel_monitor_data['data']['TXPower']['value']
            transceiver_dom_info_dict['tx2power'] = 'N/A'
            transceiver_dom_info_dict['tx3power'] = 'N/A'
            transceiver_dom_info_dict['tx4power'] = 'N/A'

        logger.log_debug("QFX5200: get_transceiver_dom_info_dict End")
        return transceiver_dom_info_dict

    def get_transceiver_dom_threshold_info_dict(self, port_num):
        transceiver_dom_threshold_info_dict = {}
        dom_info_dict_keys = ['temphighalarm',    'temphighwarning',
                              'templowalarm',     'templowwarning',
                              'vcchighalarm',     'vcchighwarning',
                              'vcclowalarm',      'vcclowwarning',
                              'rxpowerhighalarm', 'rxpowerhighwarning',
                              'rxpowerlowalarm',  'rxpowerlowwarning',
                              'txpowerhighalarm', 'txpowerhighwarning',
                              'txpowerlowalarm',  'txpowerlowwarning',
                              'txbiashighalarm',  'txbiashighwarning',
                              'txbiaslowalarm',   'txbiaslowwarning'
                              ]
        transceiver_dom_threshold_info_dict = dict.fromkeys(dom_info_dict_keys, 'N/A')

        logger.log_debug("QFX5200: get_transceiver_dom_threshold_info_dict Start")
        if port_num in self.qsfp_ports:
            file_path = self._get_port_eeprom_path(port_num, self.IDENTITY_EEPROM_ADDR)
            if not self._sfp_eeprom_present(file_path, 0):
                return None

            try:
                sysfsfile_eeprom = io.open(file_path, mode="rb", buffering=0)
            except IOError:
                print("Error: reading sysfs file %s" % file_path)
                return None

            sfpd_obj = sff8436Dom()
            if sfpd_obj is None:
                return transceiver_dom_threshold_info_dict

            # Dom Threshold data starts from offset 384
            # Revert offset back to 0 once data is retrieved
            offset = 384
            dom_module_threshold_raw = self._read_eeprom_specific_bytes(
                sysfsfile_eeprom,
                (offset + QSFP_MODULE_THRESHOLD_OFFSET),
                QSFP_MODULE_THRESHOLD_WIDTH)
            if dom_module_threshold_raw is not None:
                dom_module_threshold_data = sfpd_obj.parse_module_threshold_values(dom_module_threshold_raw, 0)
            else:
                return transceiver_dom_threshold_info_dict

            dom_channel_threshold_raw = self._read_eeprom_specific_bytes(
                sysfsfile_eeprom,
                (offset + QSFP_CHANNL_THRESHOLD_OFFSET),
                QSFP_CHANNL_THRESHOLD_WIDTH)
            if dom_channel_threshold_raw is not None:
                dom_channel_threshold_data = sfpd_obj.parse_channel_threshold_values(dom_channel_threshold_raw, 0)
            else:
                return transceiver_dom_threshold_info_dict

            try:
                sysfsfile_eeprom.close()
            except IOError:
                print("Error: closing sysfs file %s" % file_path)
                return None

            # Threshold Data
            transceiver_dom_threshold_info_dict['temphighalarm'] = dom_module_threshold_data['data']['TempHighAlarm']['value']
            transceiver_dom_threshold_info_dict['temphighwarning'] = dom_module_threshold_data['data']['TempHighWarning']['value']
            transceiver_dom_threshold_info_dict['templowalarm'] = dom_module_threshold_data['data']['TempLowAlarm']['value']
            transceiver_dom_threshold_info_dict['templowwarning'] = dom_module_threshold_data['data']['TempLowWarning']['value']
            transceiver_dom_threshold_info_dict['vcchighalarm'] = dom_module_threshold_data['data']['VccHighAlarm']['value']
            transceiver_dom_threshold_info_dict['vcchighwarning'] = dom_module_threshold_data['data']['VccHighWarning']['value']
            transceiver_dom_threshold_info_dict['vcclowalarm'] = dom_module_threshold_data['data']['VccLowAlarm']['value']
            transceiver_dom_threshold_info_dict['vcclowwarning'] = dom_module_threshold_data['data']['VccLowWarning']['value']
            transceiver_dom_threshold_info_dict['rxpowerhighalarm'] = dom_channel_threshold_data['data']['RxPowerHighAlarm']['value']
            transceiver_dom_threshold_info_dict['rxpowerhighwarning'] = dom_channel_threshold_data['data']['RxPowerHighWarning']['value']
            transceiver_dom_threshold_info_dict['rxpowerlowalarm'] = dom_channel_threshold_data['data']['RxPowerLowAlarm']['value']
            transceiver_dom_threshold_info_dict['rxpowerlowwarning'] = dom_channel_threshold_data['data']['RxPowerLowWarning']['value']
            transceiver_dom_threshold_info_dict['txbiashighalarm'] = dom_channel_threshold_data['data']['TxBiasHighAlarm']['value']
            transceiver_dom_threshold_info_dict['txbiashighwarning'] = dom_channel_threshold_data['data']['TxBiasHighWarning']['value']
            transceiver_dom_threshold_info_dict['txbiaslowalarm'] = dom_channel_threshold_data['data']['TxBiasLowAlarm']['value']
            transceiver_dom_threshold_info_dict['txbiaslowwarning'] = dom_channel_threshold_data['data']['TxBiasLowWarning']['value']

        else:
            offset = 256
            file_path = self._get_port_eeprom_path(port_num, self.DOM_EEPROM_ADDR)
            if not self._sfp_eeprom_present(file_path, 0):
                return None

            try:
                sysfsfile_eeprom = io.open(file_path, "rb", 0)
            except IOError:
                print("Error: reading sysfs file %s" % file_path)
                return None

            sfpd_obj = sff8472Dom(None, 1)
            if sfpd_obj is None:
                return transceiver_dom_threshold_info_dict

            dom_module_threshold_raw = self._read_eeprom_specific_bytes(sysfsfile_eeprom,
                                                                        (offset + SFP_MODULE_THRESHOLD_OFFSET), SFP_MODULE_THRESHOLD_WIDTH)

            if dom_module_threshold_raw is not None:
                dom_module_threshold_data = sfpd_obj.parse_alarm_warning_threshold(dom_module_threshold_raw, 0)
            else:
                return transceiver_dom_threshold_info_dict

            try:
                sysfsfile_eeprom.close()
            except IOError:
                print("Error: closing sysfs file %s" % file_path)
                return None

            # Threshold Data
            transceiver_dom_threshold_info_dict['temphighalarm'] = dom_module_threshold_data['data']['TempHighAlarm']['value']
            transceiver_dom_threshold_info_dict['templowalarm'] = dom_module_threshold_data['data']['TempLowAlarm']['value']
            transceiver_dom_threshold_info_dict['temphighwarning'] = dom_module_threshold_data['data']['TempHighWarning']['value']
            transceiver_dom_threshold_info_dict['templowwarning'] = dom_module_threshold_data['data']['TempLowWarning']['value']
            transceiver_dom_threshold_info_dict['vcchighalarm'] = dom_module_threshold_data['data']['VoltageHighAlarm']['value']
            transceiver_dom_threshold_info_dict['vcclowalarm'] = dom_module_threshold_data['data']['VoltageLowAlarm']['value']
            transceiver_dom_threshold_info_dict['vcchighwarning'] = dom_module_threshold_data['data']['VoltageHighWarning']['value']
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

        logger.log_debug("QFX5200: get_transceiver_dom_threshold_info_dict End")
        return transceiver_dom_threshold_info_dict

    def get_transceiver_change_event(self, timeout=2000):
        time.sleep(3)
        return self.sfp_detect()
