import binascii
import fcntl
import json
import logging
import math
import os
import sys
import threading
import time
import traceback
from array import array

RLMA_PAGE_BASE_OFFSET = 0x0
RLMB_PAGE_BASE_OFFSET = 0x4

RLM_IDENTIFIER_PAGE = 0xb0
RLM_IDENTIFIER_OFFSET = 0x80
RLM_IDENTIFIER_WIDTH = 1

RLM_REVERSION_PAGE = 0xb0
RLM_REVERSION_OFFSET = 0x81
RLM_REVERSION_WIDTH = 1

RLM_LASER_GRID_AND_COUNT_PAGE = 0xb0
RLM_LASER_GRID_AND_COUNT_OFFSET = 0x82
RLM_LASER_GRID_AND_COUNT_WIDTH = 1

RLM_MODULE_STATE_AND_INTERRUPT_PAGE = 0xb0
RLM_MODULE_STATE_AND_INTERRUPT_OFFSET = 0x83
RLM_MODULE_STATE_AND_INTERRUPT_WIDTH = 1

RLM_MODULE_LOW_POWER_CONTROL_PAGE = 0xb0
RLM_MODULE_LOW_POWER_CONTROL_OFFSET = 0x84
RLM_MODULE_LOW_POWER_CONTROL_WIDTH = 1

RLM_LASER_DISABLE_CONTROL_PAGE = 0xb0
RLM_LASER_DISABLE_CONTROL_OFFSET = 0x85
RLM_LASER_DISABLE_CONTROL_WIDTH = 2

RLM_LASER_ACTIVE_STATUS_PAGE = 0xb0
RLM_LASER_ACTIVE_STATUS_OFFSET = 0x87
RLM_LASER_ACTIVE_STATUS_WIDTH = 2

RLM_VCC_AND_TEMPERATURE_WARNING_ALARM_PAGE = 0xb0
RLM_VCC_AND_TEMPERATURE_WARNING_ALARM_OFFSET = 0x89
RLM_VCC_AND_TEMPERATURE_WARNING_ALARM_WIDTH = 1

RLM_LASER_BIAS_WARNING_PAGE = 0xb0
RLM_LASER_BIAS_WARNING_OFFSET = 0x8A
RLM_LASER_BIAS_WARNING_WIDTH = 2

RLM_LASER_BIAS_ALARM_PAGE = 0xb0
RLM_LASER_BIAS_ALARM_OFFSET = 0x8C
RLM_LASER_BIAS_ALARM_WIDTH = 2

RLM_MODULE_TEMPERATURE_MONITOR_PAGE = 0xb0
RLM_MODULE_TEMPERATURE_MONITOR_OFFSET = 0x96
RLM_MODULE_TEMPERATURE_MONITOR_WIDTH = 2

RLM_MODULE_SUPPLY_VOLTAGE_MONITOR_PAGE = 0xb0
RLM_MODULE_SUPPLY_VOLTAGE_MONITOR_OFFSET = 0x98
RLM_MODULE_SUPPLY_VOLTAGE_MONITOR_WIDTH = 2

RLM_MONITOR_VALUES_PAGE = 0xb0
RLM_MONITOR_VALUES_OFFSET = 0x9A
RLM_MONITOR_VALUES_WIDTH = 81

RLM_TEC_CURRENT_MONITOR_PAGE = 0xb0
RLM_TEC_CURRENT_MONITOR_OFFSET = 0xfa
RLM_TEC_CURRENT_MONITOR_WIDTH = 2

RLM_VENDOR_INFOMATION_PAGE = 0xb1
RLM_VENDOR_INFOMATION_OFFSET = 0x81
RLM_VENDOR_INFOMATION_WIDTH = 122

RLM_MAXIMUM_POWER_CONSUMPTION_PAGE = 0xb1
RLM_MAXIMUM_POWER_CONSUMPTION_OFFSET = 0xc8
RLM_MAXIMUM_POWER_CONSUMPTION_WIDTH = 1

RLM_LASER_POWER_MODE_CONTROL_PAGE = 0xb2
RLM_LASER_POWER_MODE_CONTROL_OFFSET = 0x80
RLM_LASER_POWER_MODE_CONTROL_WIDTH = 1

RLM_DOM_THRESHOLD_PAGE = 0xb2
RLM_DOM_THRESHOLD_OFFSET = 0xA2
RLM_DOM_THRESHOLD_WIDTH = 28

RLM_B1_CEHCK_SUM_PAGE = 0xb1
RLM_B1_CEHCK_SUM_OFFSET = 0xfb
RLM_B1_CEHCK_SUM_WIDTH = 1

class CPOException(Exception):
    def __init__(self, detailed_error_message):
        super(CPOException, self).__init__(detailed_error_message)



class cpoutilbase(object):

    def __init__(self) -> None:
        self.rlm_api = cpo_v1()
        self.oe_list = list(range(8))

    def get_dict_str_format(self, indict, indent):
        output = ''
        if indict != None:
            for key, value in indict.items():
                if type(value) == dict:
                    output += "{}{}:\n{}".format(indent, key, self.get_dict_str_format(value, indent + (' ' * 8)))
                else:
                    output += "{}{}: {}\n".format(indent, key, value)
        return output

    def laser_count_implemented(self, indict, laser_count):
        if type(indict) != dict:
            return indict

        outdict = dict()
        for key, value in indict.items():
            flag = True
            if type(key) == str:
                for i in range(laser_count+1, 16):
                    laser_name = "Laser{:02d}".format(i)
                    if key.find(laser_name) >= 0:
                        flag = False
                        break
            if flag:
                outdict[key] = self.laser_count_implemented(value, laser_count)
        return outdict
    
    def mutil_reset_oe(self, oe_index_list: list):
        byte = 0xff
        for oe_index in oe_index_list:
            if oe_index not in self.get_oe_index_list():
                print("OE index out of range!")
                continue
            byte = byte & ~(0x01 << oe_index)

        result = os.system("i2cset -f -y 20 0x1e 0x72 0x%02X" % byte)
        return result == 0

    def get_oe_index_list(self):
        return list(range(8))

    def get_rlm_index_list(self, oe_index):
        return [0,1]

    def get_rlm_presence(self, oe_index, rlm_index):
        if oe_index not in self.get_oe_index_list():
            print("OE index out of range!")
            return False
        if rlm_index not in self.get_rlm_index_list(oe_index):
            print("RLM index out of range!")
            return False

        if rlm_index == 0:
            RLM_PAGE_BASE_OFFSET = RLMA_PAGE_BASE_OFFSET
        elif rlm_index == 1:
            RLM_PAGE_BASE_OFFSET = RLMB_PAGE_BASE_OFFSET

        presence = False
        rlm_api = cpo_v1()
        try:
            rlm_api._file_rw_lock(oe_index)

            rlm_vendor_infomation_raw = rlm_api.read_eeprom(oe_index, RLM_VENDOR_INFOMATION_PAGE + RLM_PAGE_BASE_OFFSET, RLM_VENDOR_INFOMATION_OFFSET, RLM_VENDOR_INFOMATION_WIDTH)
            rlm_b1_checksum_raw = rlm_api.read_eeprom(oe_index, RLM_B1_CEHCK_SUM_PAGE + RLM_PAGE_BASE_OFFSET, RLM_B1_CEHCK_SUM_OFFSET, RLM_B1_CEHCK_SUM_WIDTH)

            b1_page_sum = sum([int(byte, 16) for byte in rlm_vendor_infomation_raw])&0xff

            if b1_page_sum != int(rlm_b1_checksum_raw[0], 16):
                raise CPOException("page b1 byte 80 to byte 250 checksum failed, {} != {}".format(b1_page_sum, int(rlm_b1_checksum_raw[0], 16)))
            if b1_page_sum != 0:
                presence = True

        except CPOException:
            pass
        except BaseException:
            print(traceback.format_exc())
        finally:
            rlm_api._file_rw_unlock(oe_index)

        return presence

    def get_rlm_enable(self, oe_index, rlm_index):
        if oe_index not in self.get_oe_index_list():
            print("OE index out of range!")
            return False
        if rlm_index not in self.get_rlm_index_list(oe_index):
            print("RLM index out of range!")
            return False

        if rlm_index == 0:
            RLM_PAGE_BASE_OFFSET = RLMA_PAGE_BASE_OFFSET
        elif rlm_index == 1:
            RLM_PAGE_BASE_OFFSET = RLMB_PAGE_BASE_OFFSET

        rlm_info_dict = dict()
        rlm_enable = 0x00
        rlm_api = cpo_v1()
        try:
            rlm_api._file_rw_lock(oe_index)

            rlm_laser_power_mode_control_raw = rlm_api.read_eeprom(oe_index, RLM_LASER_POWER_MODE_CONTROL_PAGE + RLM_PAGE_BASE_OFFSET, RLM_LASER_POWER_MODE_CONTROL_OFFSET, RLM_LASER_POWER_MODE_CONTROL_WIDTH)
            if rlm_laser_power_mode_control_raw is not None:
                rlm_laser_power_mode_control_data = rlm_api.parse_laser_power_mode_control(rlm_laser_power_mode_control_raw, 0)

            if rlm_info_dict != None:
                rlm_enable = rlm_laser_power_mode_control_data['data']['Laser power mode control']

        except BaseException:
            print(traceback.format_exc())
        finally:
            rlm_api._file_rw_unlock(oe_index)

        return int(rlm_enable, 16) == 0xff

    def get_rlm_info_dict(self, oe_index, rlm_index):
        if oe_index not in self.get_oe_index_list():
            return "OE index out of range!"
        if rlm_index not in self.get_rlm_index_list(oe_index):
            return "RLM index out of range!"

        if rlm_index == 0:
            RLM_PAGE_BASE_OFFSET = RLMA_PAGE_BASE_OFFSET
        elif rlm_index == 1:
            RLM_PAGE_BASE_OFFSET = RLMB_PAGE_BASE_OFFSET

        rlm_info_dict = dict()
        rlm_api = cpo_v1()

        try:
            rlm_api._file_rw_lock(oe_index)

            rlm_vendor_infomation_raw = rlm_api.read_eeprom(oe_index, RLM_VENDOR_INFOMATION_PAGE + RLM_PAGE_BASE_OFFSET, RLM_VENDOR_INFOMATION_OFFSET, RLM_VENDOR_INFOMATION_WIDTH)
            rlm_b1_checksum_raw = rlm_api.read_eeprom(oe_index, RLM_B1_CEHCK_SUM_PAGE + RLM_PAGE_BASE_OFFSET, RLM_B1_CEHCK_SUM_OFFSET, RLM_B1_CEHCK_SUM_WIDTH)

            b1_page_sum = sum([int(byte, 16) for byte in rlm_vendor_infomation_raw])&0xff
            if b1_page_sum == 0 and int(rlm_b1_checksum_raw[0], 16) == 0:
                return "RLM not presence."
            if b1_page_sum != int(rlm_b1_checksum_raw[0], 16):
                raise CPOException("page b1 byte 80 to byte 250 checksum failed, {} != {}".format(b1_page_sum, int(rlm_b1_checksum_raw[0], 16)))

            rlm_identifier_raw = rlm_api.read_eeprom(oe_index, RLM_IDENTIFIER_PAGE + RLM_PAGE_BASE_OFFSET, RLM_IDENTIFIER_OFFSET, RLM_IDENTIFIER_WIDTH)
            if rlm_identifier_raw is not None:
                rlm_identifier_data = rlm_api.parse_identifier(rlm_identifier_raw, 0)

            rlm_reversion_raw = rlm_api.read_eeprom(oe_index, RLM_REVERSION_PAGE + RLM_PAGE_BASE_OFFSET, RLM_REVERSION_OFFSET, RLM_REVERSION_WIDTH)
            if rlm_reversion_raw is not None:
                rlm_reversion_data = rlm_api.parse_reversion(rlm_reversion_raw, 0)

            rlm_laser_grid_and_count_raw = rlm_api.read_eeprom(oe_index, RLM_LASER_GRID_AND_COUNT_PAGE + RLM_PAGE_BASE_OFFSET, RLM_LASER_GRID_AND_COUNT_OFFSET, RLM_LASER_GRID_AND_COUNT_WIDTH)
            if rlm_laser_grid_and_count_raw is not None:
                rlm_laser_grid_and_count_data = rlm_api.parse_laser_grid_and_count(rlm_laser_grid_and_count_raw, 0)

            rlm_module_state_and_interrupt_raw = rlm_api.read_eeprom(oe_index, RLM_MODULE_STATE_AND_INTERRUPT_PAGE + RLM_PAGE_BASE_OFFSET, RLM_MODULE_STATE_AND_INTERRUPT_OFFSET, RLM_MODULE_STATE_AND_INTERRUPT_WIDTH)
            if rlm_module_state_and_interrupt_raw is not None:
                rlm_module_state_and_interrupt_data = rlm_api.parse_module_state_and_interrupt(rlm_module_state_and_interrupt_raw, 0)

            rlm_module_low_power_control_raw = rlm_api.read_eeprom(oe_index, RLM_MODULE_LOW_POWER_CONTROL_PAGE + RLM_PAGE_BASE_OFFSET, RLM_MODULE_LOW_POWER_CONTROL_OFFSET, RLM_MODULE_LOW_POWER_CONTROL_WIDTH)
            if rlm_module_low_power_control_raw is not None:
                rlm_module_low_power_control_data = rlm_api.parse_module_low_power_control(rlm_module_low_power_control_raw, 0)

            rlm_laser_disable_control_raw = rlm_api.read_eeprom(oe_index, RLM_LASER_DISABLE_CONTROL_PAGE + RLM_PAGE_BASE_OFFSET, RLM_LASER_DISABLE_CONTROL_OFFSET, RLM_LASER_DISABLE_CONTROL_WIDTH)
            if rlm_laser_disable_control_raw is not None:
                rlm_laser_disable_control_data = rlm_api.parse_laser_disable_control(rlm_laser_disable_control_raw, 0)

            rlm_laser_active_status_raw = rlm_api.read_eeprom(oe_index, RLM_LASER_ACTIVE_STATUS_PAGE + RLM_PAGE_BASE_OFFSET, RLM_LASER_ACTIVE_STATUS_OFFSET, RLM_LASER_ACTIVE_STATUS_WIDTH)
            if rlm_laser_active_status_raw is not None:
                rlm_laser_active_status_data = rlm_api.parse_laser_active_status(rlm_laser_active_status_raw, 0)

            rlm_vcc_and_temperature_warning_alarm_raw = rlm_api.read_eeprom(oe_index, RLM_VCC_AND_TEMPERATURE_WARNING_ALARM_PAGE + RLM_PAGE_BASE_OFFSET, RLM_VCC_AND_TEMPERATURE_WARNING_ALARM_OFFSET, RLM_VCC_AND_TEMPERATURE_WARNING_ALARM_WIDTH)
            if rlm_vcc_and_temperature_warning_alarm_raw is not None:
                rlm_vcc_and_temperature_warning_alarm_data = rlm_api.parse_vcc_and_temperature_warning_alarm(rlm_vcc_and_temperature_warning_alarm_raw, 0)

            rlm_laser_bias_warning_raw = rlm_api.read_eeprom(oe_index, RLM_LASER_BIAS_WARNING_PAGE + RLM_PAGE_BASE_OFFSET, RLM_LASER_BIAS_WARNING_OFFSET, RLM_LASER_BIAS_WARNING_WIDTH)
            if rlm_laser_bias_warning_raw is not None:
                rlm_laser_bias_warning_data = rlm_api.parse_laser_bias_warning(rlm_laser_bias_warning_raw, 0)

            rlm_laser_bias_alarm_raw = rlm_api.read_eeprom(oe_index, RLM_LASER_BIAS_ALARM_PAGE + RLM_PAGE_BASE_OFFSET, RLM_LASER_BIAS_ALARM_OFFSET, RLM_LASER_BIAS_ALARM_WIDTH)
            if rlm_laser_bias_alarm_raw is not None:
                rlm_laser_bias_alarm_data = rlm_api.parse_laser_bias_alarm(rlm_laser_bias_alarm_raw, 0)

            rlm_module_temperature_monitor_raw = rlm_api.read_eeprom(oe_index, RLM_MODULE_TEMPERATURE_MONITOR_PAGE + RLM_PAGE_BASE_OFFSET, RLM_MODULE_TEMPERATURE_MONITOR_OFFSET, RLM_MODULE_TEMPERATURE_MONITOR_WIDTH)
            if rlm_module_temperature_monitor_raw is not None:
                rlm_module_temperature_monitor_data = rlm_api.parse_module_temperature_monitor(rlm_module_temperature_monitor_raw, 0)

            rlm_module_supply_voltage_monitor_raw = rlm_api.read_eeprom(oe_index, RLM_MODULE_SUPPLY_VOLTAGE_MONITOR_PAGE + RLM_PAGE_BASE_OFFSET, RLM_MODULE_SUPPLY_VOLTAGE_MONITOR_OFFSET, RLM_MODULE_SUPPLY_VOLTAGE_MONITOR_WIDTH)
            if rlm_module_supply_voltage_monitor_raw is not None:
                rlm_module_supply_voltage_monitor_data = rlm_api.parse_module_supply_voltage_monitor(rlm_module_supply_voltage_monitor_raw, 0)

            if rlm_vendor_infomation_raw is not None:
                rlm_vendor_infomation_data = rlm_api.parse_vendor_infomation(rlm_vendor_infomation_raw, 0)

            rlm_maximum_power_consumption_raw = rlm_api.read_eeprom(oe_index, RLM_MAXIMUM_POWER_CONSUMPTION_PAGE + RLM_PAGE_BASE_OFFSET, RLM_MAXIMUM_POWER_CONSUMPTION_OFFSET, RLM_MAXIMUM_POWER_CONSUMPTION_WIDTH)
            if rlm_maximum_power_consumption_raw is not None:
                rlm_maximum_power_consumption_data = rlm_api.parse_maximum_power_consumption(rlm_maximum_power_consumption_raw, 0)

            rlm_laser_power_mode_control_raw = rlm_api.read_eeprom(oe_index, RLM_LASER_POWER_MODE_CONTROL_PAGE + RLM_PAGE_BASE_OFFSET, RLM_LASER_POWER_MODE_CONTROL_OFFSET, RLM_LASER_POWER_MODE_CONTROL_WIDTH)
            if rlm_laser_power_mode_control_raw is not None:
                rlm_laser_power_mode_control_data = rlm_api.parse_laser_power_mode_control(rlm_laser_power_mode_control_raw, 0)

            rlm_dom_threshold_raw = rlm_api.read_eeprom(oe_index, RLM_DOM_THRESHOLD_PAGE + RLM_PAGE_BASE_OFFSET, RLM_DOM_THRESHOLD_OFFSET, RLM_DOM_THRESHOLD_WIDTH)
            if rlm_dom_threshold_raw is not None:
                rlm_dom_threshold_data = rlm_api.parse_dom_threshold(rlm_dom_threshold_raw, 0)

            rlm_monitor_values_raw = rlm_api.read_eeprom(oe_index, RLM_MONITOR_VALUES_PAGE + RLM_PAGE_BASE_OFFSET, RLM_MONITOR_VALUES_OFFSET, RLM_MONITOR_VALUES_WIDTH)
            if rlm_monitor_values_raw is not None:
                rlm_monitor_values_data = rlm_api.parse_monitor_values(rlm_monitor_values_raw, 0)

            rlm_tec_current_monitor_raw = rlm_api.read_eeprom(oe_index, RLM_TEC_CURRENT_MONITOR_PAGE + RLM_PAGE_BASE_OFFSET, RLM_TEC_CURRENT_MONITOR_OFFSET, RLM_TEC_CURRENT_MONITOR_WIDTH)
            if rlm_tec_current_monitor_raw is not None:
                rlm_tec_current_monitor_data = rlm_api.parse_tec_current_monitor(rlm_tec_current_monitor_raw, 0)

            rlm_info_dict['Identifier'] = rlm_identifier_data['data']['Identifier']
            rlm_info_dict['Reversion'] = rlm_reversion_data['data']['Reversion']
            rlm_info_dict['Laser grid and count'] = rlm_laser_grid_and_count_data['data']['Laser grid and count']
            rlm_info_dict['Module state & Interrupt'] = rlm_module_state_and_interrupt_data['data']['Module state & Interrupt']
            rlm_info_dict['Module low power control'] = rlm_module_low_power_control_data['data']['Module low power control']
            rlm_info_dict['Laser disable control'] = rlm_laser_disable_control_data['data']['Laser disable control']
            rlm_info_dict['Laser active status'] = rlm_laser_active_status_data['data']['Laser active status']
            rlm_info_dict['Vcc 3.3V & temperature low/high warning/alarm'] = rlm_vcc_and_temperature_warning_alarm_data['data']['Vcc 3.3V & Temperature low/high warning/alarm']
            rlm_info_dict['Laser bias warning'] = rlm_laser_bias_warning_data['data']['Laser bias warning']
            rlm_info_dict['Laser bias alarm'] = rlm_laser_bias_alarm_data['data']['Laser bias alarm']
            rlm_info_dict['Module temperature monitor'] = rlm_module_temperature_monitor_data['data']['Module Temperature Monitor']
            rlm_info_dict['Module supply voltage monitor'] = rlm_module_supply_voltage_monitor_data['data']['Module supply voltage Monitor']
            rlm_info_dict['Vendor infomation'] = rlm_vendor_infomation_data['data']['Vendor infomation']
            rlm_info_dict['Maximum power consumption'] = rlm_maximum_power_consumption_data['data']['Maximum power consumption']
            rlm_info_dict['Laser power mode control'] = rlm_laser_power_mode_control_data['data']['Laser power mode control']
            # rlm_info_dict['ThresholdValues'] = rlm_dom_threshold_data['data']['ThresholdValues']
            rlm_info_dict['Laser current monitor'] = rlm_monitor_values_data['data']['Laser current monitor']
            rlm_info_dict['Laser voltage monitor'] = rlm_monitor_values_data['data']['Laser voltage monitor']
            rlm_info_dict['Laser opitcal power monitor'] = rlm_monitor_values_data['data']['Laser opitcal power monitor']
            rlm_info_dict['TEC current monitor'] = rlm_tec_current_monitor_data['data']['TEC current monitor']
            
            if (rlm_info_dict != None) and (rlm_info_dict['Laser grid and count'] != None):
                laser_count = rlm_info_dict['Laser grid and count']['Laser count']
                rlm_info_dict = self.laser_count_implemented(rlm_info_dict, laser_count)

        except CPOException:
            rlm_info_dict = None
        except BaseException:
            rlm_info_dict = None
            print(traceback.format_exc())
        finally:
            rlm_api._file_rw_unlock(oe_index)

        if rlm_info_dict == None:
            return "RLM eeprom get failed!"

        return rlm_info_dict

    def _set_rlm_enable(self, oe_index, rlm_index):
        if rlm_index not in [0,1]:
            return False
        if rlm_index == 0:
            bank_list = [0,1,2,3]
        else:
            bank_list = [4,5,6,7]

        # Exit low power mode: write 0x20 to byte 0x1A on page 0x00.
        self.rlm_api.write_eeprom(oe=oe_index, page=0x00, offset=0x1A, num_bytes=1, write_buffer=bytearray([0x20]))

        for bank in bank_list:
            # switch bank
            self.rlm_api.write_eeprom(oe=oe_index, page=0x00, offset=0x7e, num_bytes=1, write_buffer=bytearray([bank]))
            # Enable data path: write 0x00 to byte 0x80 on page 0x10.
            self.rlm_api.write_eeprom(oe=oe_index, page=0x10, offset=0x80, num_bytes=1, write_buffer=bytearray([0x00]))
            # Enable Tx: write 0x00 to byte 0x82 on page 0x10.
            self.rlm_api.write_eeprom(oe=oe_index, page=0x10, offset=0x82, num_bytes=1, write_buffer=bytearray([0x00]))
            # Enable RLM to full power mode (AOP ~ 20dBm): write 0xFF to byte 0x80 on page 0xB2.
            if bank < 4:
                self.rlm_api.write_eeprom(oe=oe_index, page=0xB2, offset=0x80, num_bytes=1, write_buffer=bytearray([0xFF]))
            else:
                self.rlm_api.write_eeprom(oe=oe_index, page=0xB6, offset=0x80, num_bytes=1, write_buffer=bytearray([0xFF]))

    def _set_rlm_disable(self, oe_index, rlm_index):
        if rlm_index not in [0,1]:
            return False
        if rlm_index == 0:
            bank_list = [0,1,2,3]
        else:
            bank_list = [4,5,6,7]

        for bank in bank_list:
            # switch bank
            self.rlm_api.write_eeprom(oe=oe_index, page=0x00, offset=0x7e, num_bytes=1, write_buffer=bytearray([bank]))
            if bank < 4:
                # disable RLM to full power mode (AOP ~ 20dBm): write 0x00 to byte 0x80 on page 0xB2.
                self.rlm_api.write_eeprom(oe=oe_index, page=0xB2, offset=0x80, num_bytes=1, write_buffer=bytearray([0x00]))
            else:
                # disable RLM to full power mode (AOP ~ 20dBm): write 0x00 to byte 0x80 on page 0xB2.
                self.rlm_api.write_eeprom(oe=oe_index, page=0xB6, offset=0x80, num_bytes=1, write_buffer=bytearray([0x00]))
            # # disable Tx: write 0xff to byte 0x82 on page 0x10.
            # self.rlm_api.write_eeprom(oe=oe_index, page=0x10, offset=0x82, num_bytes=1, write_buffer=bytearray([0xff]))
            # # disable data path: write 0xff to byte 0x80 on page 0x10.
            # self.rlm_api.write_eeprom(oe=oe_index, page=0x10, offset=0x80, num_bytes=1, write_buffer=bytearray([0xff]))
            # # enter low power mode: write 0x30 to byte 0x1A on page 0x00.
            # self.rlm_api.write_eeprom(oe=oe_index, page=0x00, offset=0x1A, num_bytes=1, write_buffer=bytearray([0x30]))

    def set_rlm_enable(self, oe_index, rlm_index, enable):
        retval = True
        try:
            self.rlm_api._file_rw_lock(oe_index)
            if enable:
                self._set_rlm_enable(oe_index, rlm_index)
            else:
                self._set_rlm_disable(oe_index, rlm_index)
        except:
            print("error!")
            retval = False
        finally:
            self.rlm_api._file_rw_unlock(oe_index)
        return retval

type_of_module_state_and_interrupt = {
    '00': "High power mode & Interrupt event occurred",
    '01': "High power mode & Interrupt event cleared",
    '02': "Low power mode & Interrupt event occurred",
    '03': "Low power mode & Interrupt event cleared"
}

type_of_module_low_power_control = {
    '00': "High power mode",
    '01': "Low power mode"
}

type_of_identifier = {
    '18': "QSFP-DD"
}

class cpobase(object):

    def __init__(self) -> None:
        self.version = 0.1
        self.eeprom_retry_times = 5
        self.eeprom_retry_break_sec = 0.2
        self.pidfile = None
        self.page = None

    def test_bit(self, n, bitpos):
        try:
            mask = 1 << bitpos
            if (n & mask) == 0:
                return 0
            else:
                return 1
        except Exception:
            return -1

    def twos_comp(self, num, bits):
        try:
            if ((num & (1 << (bits - 1))) != 0):
                num = num - (1 << bits)
            return num
        except Exception:
            return 0

    def mw_to_dbm(self, mW):
        if mW == 0:
            return float("-inf")
        elif mW < 0:
            return float("NaN")
        return 10. * math.log10(mW)

    def power_in_dbm_str(self, mW):
        return "%.4f%s" % (self.mw_to_dbm(mW), "dBm")

    # Convert Date to String
    def convert_date_to_string(self, eeprom_data, offset, size):
        try:
            year_offset  = 0
            month_offset = 2
            day_offset   = 4
            lot_offset   = 6

            date = self.convert_hex_to_string(eeprom_data, offset, offset + size)
            retval = "20"+ date[year_offset:month_offset] + "-" + \
                    date[month_offset:day_offset] + "-" + \
                    date[day_offset:lot_offset] + " " + \
                    date[lot_offset:size]
        except Exception as err:
            retval = str(err)
        return retval

    # Convert Hex to String
    def convert_hex_to_string(self, arr, start, end):
        try:
            ret_str = ''
            for n in range(start, end):
                ret_str += arr[n]
            return binascii.unhexlify(ret_str).decode("utf-8", "ignore").strip()
        except Exception as err:
            return str(err)

    def parse_rlm_element(self, eeprom_data, eeprom_ele, start_pos):
        value  = None
        offset = eeprom_ele.get('offset') + start_pos
        size   = eeprom_ele.get('size')
        type   = eeprom_ele.get('type')
        decode = eeprom_ele.get('decode')
        bitmask = eeprom_ele.get('bitmask', None)
        bitvalue_map = eeprom_ele.get('bitvalue_map', None)

        if type == 'enum':
            # Get the matched value
            if bitmask != None:
                key_value_int = int(eeprom_data[offset], 16) & bitmask
                key_value_str = str(hex(key_value_int)[2:].zfill(2))
            else:
                key_value_str = str(eeprom_data[offset])
            value = decode.get(key_value_str, 'Unknown')

        elif type == 'bitmap':
            # Get the 'on' bitname
            bitvalue_dict = {}
            for bitname, bitinfo in sorted(decode.items()):
                bitinfo_offset = bitinfo.get('offset') + start_pos
                bitinfo_pos = bitinfo.get('bit')
                bitinfo_value = bitinfo.get('value')
                data = int(eeprom_data[bitinfo_offset], 16)
                bit_value = self.test_bit(data, bitinfo_pos)
                if bitinfo_value != None:
                    if bit_value == bitinfo_value:
                        value = bitname
                        break
                elif bit_value == 1:
                    value = bitname
                    break

        elif type == 'bitvalue':
            # Get the value of the bit
            bitpos = eeprom_ele.get('bit')
            data = int(eeprom_data[offset], 16)
            bitval = self.test_bit(data, bitpos)
            if bitvalue_map != None:
                value = bitvalue_map[bitval]
            else:
                value = ['Off', 'On'][bitval]

        elif type == 'func':
            # Call the decode func to get the value
            value = decode['func'](self, eeprom_data,
                         offset, size)

        elif type == 'str':
            value = self.convert_hex_to_string(eeprom_data, offset,
                              offset + size)

        elif type == 'int':
            value = int(eeprom_data[offset], 16)
            if bitmask != None:
                value = value & bitmask

        elif type == 'date':
            value = self.convert_date_to_string(eeprom_data, offset,
                              size)

        elif type == 'hex':
            if size == 1:
                value = "0x{}".format(eeprom_data[offset])
            elif size > 1:
                value = '-'.join(eeprom_data[offset:offset+size])

        return value

    # Recursively parses sff data into dictionary
    def parse_rlm(self, eeprom_map, eeprom_data, start_pos):
        outdict = {}
        for name, meta_data in sorted(eeprom_map.items()):
            type = meta_data.get('type')

            # Initialize output value
            value_dict = {}
            value_dict['outtype'] = meta_data.get('outtype')
            value_dict['short_name'] = meta_data.get('short_name')

            if type != 'nested':
                data = self.parse_rlm_element(eeprom_data,
                                  meta_data, start_pos)
            else:
                nested_map = meta_data.get('decode')
                data = self.parse_rlm(nested_map,
                             eeprom_data, start_pos)

            if data != None:
                value_dict['value'] = data
                outdict[name] = value_dict

        return outdict

    def parse(self, eeprom_map, eeprom_data, start_pos):
        """ Example Return format:
        {'version': '1.0', 'data': {'Length50um(UnitsOf10m)':
        {'outtype': None, 'value': 8, 'short_name': None},
        'TransceiverCodes': {'outtype': None, 'value':
        {'10GEthernetComplianceCode': {'outtype': None, 'value':
        '10G Base-SR', 'short_name': None}}, 'short_name': None},
        'ExtIdentOfTypeOfTransceiver': {'outtype': None, 'value':
        'GBIC/SFP func defined by two-wire interface ID', 'short_name':
         None}, 'Length62.5um(UnitsOfm)': {'outtype': None,"""

        outdict = {}
        return_dict = {}

        outdict = self.parse_rlm(eeprom_map, eeprom_data, start_pos)

        return_dict['version'] = self.version
        return_dict['data'] = outdict

        return return_dict

    # Returns sff parsed data in a pretty dictionary format
    def get_data_pretty_dict(self, indict):
        outdict = {}

        for elem, elem_val in sorted(indict.items()):
            value = elem_val.get('value')
            if type(value) == dict:
                outdict[elem] = self.get_data_pretty_dict(value)
            else:
                outdict[elem] = value

        return outdict

    def get_data_pretty(self, indata):
        """Example Return format:
        {'version': '1.0', 'data': {'Length50um(UnitsOf10m)': 8,
        'TransceiverCodes': {'10GEthernetComplianceCode':
        '10G Base-SR'}, 'ExtIdentOfTypeOfTransceiver': 'GBIC/SFP func
        defined by two-wire interface ID', 'Length62.5um(UnitsOfm)': 3,
         'VendorPN': 'FTLX8571D3BNL', 'RateIdentifier': 'Unspecified',
         'NominalSignallingRate(UnitsOf100Mbd)': 103, 'VendorOUI': ..}}
        {'version': '1.0', 'data': {'AwThresholds':
        {'TXPowerLowWarning': '-5.0004 dBm', 'TempHighWarning':
        '88.0000C', 'RXPowerHighAlarm': '0.0000 dBm',
        'TXPowerHighAlarm': '-0.7998 dBm', 'RXPowerLowAlarm':
        '-20.0000 dBm', 'RXPowerHighWarning': '-1.0002 dBm',
        'VoltageLowAlarm': '2.9000Volts'"""

        return_dict = {}

        return_dict['version'] = indata.get('version')
        return_dict['data'] = self.get_data_pretty_dict(indata.get(
                                'data'))
        return return_dict

    def get_eeprom_path(self, oe):
        raise NotImplementedError

    def read_eeprom(self, oe, page, offset, num_bytes):
        # temp solution for CPO byte0 bug, remove me when it fixed
        if offset == 0:
            self._switch_page(oe, page=0)

        if offset >= 128:
            self._switch_page(oe, page)

        # for other page, need to convert flat_mem offset to single page offset
        return self._read_eeprom(oe, offset, num_bytes)

    def write_eeprom(self, oe, page, offset, num_bytes, write_buffer):
        # temp solution for CPO byte0 bug, remove me when it fixed
        if offset == 0:
            self._switch_page(oe, page=0)

        if offset >= 128:
            self._switch_page(oe, page)

        # for other page, need to convert flat_mem offset to single page offset
        return self._write_eeprom(oe, offset, num_bytes, write_buffer)

    def _switch_page(self, oe, page):
        if page == self.page:
            return True
        page_offset = 127 #0x7f
        num_bytes = 1
        self._write_eeprom(oe, page_offset, num_bytes, bytearray([page]))
        cur_page = self._read_eeprom(oe, page_offset, num_bytes)[0]
        if cur_page is None:
            # self._sfplog(LOG_ERROR_LEVEL, "CPO read PAGE ERROR!")
            return False
        if cur_page == page:
            self.page = cur_page
            return True
        return False

    def _read_eeprom(self, oe, offset, num_bytes):
        eeprom_raw = []
        for i in range(0, num_bytes):
            eeprom_raw.append("0x00")

        try:
            for i in range(self.eeprom_retry_times):
                with open(self.get_eeprom_path(oe), mode='rb', buffering=0) as f:
                    f.seek(offset)
                    raw = f.read(num_bytes)
                    # temporary solution for a sonic202111 bug
                    raw = bytearray(raw)

                    for n in range(0, num_bytes):
                        eeprom_raw[n] = hex(raw[n])[2:].zfill(2)

                    return eeprom_raw

        except BaseException:
            print(traceback.format_exc())
            # self._sfplog(LOG_ERROR_LEVEL, traceback.format_exc())
            pass
        return None

    def _write_eeprom(self, oe, offset, num_bytes, write_buffer):
        for i in range(self.eeprom_retry_times):
            try:
                with open(self.get_eeprom_path(oe), mode='r+b', buffering=0) as f:
                    f.seek(offset)
                    f.write(write_buffer[0:num_bytes])
                return True
            except BaseException:
                print(traceback.format_exc())
                time.sleep(self.eeprom_retry_break_sec)
                pass
        return False

    def _file_rw_lock(self, oe):
        self.pidfile = open(self.get_eeprom_path(oe), "r")
        file_lock_flag = False
        # Retry 100 times to lock file
        for i in range(0, 100):
            try:
                fcntl.flock(self.pidfile, fcntl.LOCK_EX | fcntl.LOCK_NB)
                file_lock_flag = True
                # self._sfplog(LOG_DEBUG_LEVEL, "file lock success")
                return True
            except Exception:
                time.sleep(0.001)
                continue

        if file_lock_flag == False:
            if self.pidfile is not None:
                self.pidfile.close()
                self.pidfile = None
            return False

    def _file_rw_unlock(self, oe):
        try:
            if self.pidfile is not None:
                fcntl.flock(self.pidfile, fcntl.LOCK_UN)
                self.pidfile.close()
                self.pidfile = None
                # self._sfplog(LOG_DEBUG_LEVEL, "file unlock success")
            else:
                # self._sfplog(LOG_DEBUG_LEVEL, "pidfile is invalid, do nothing")
                pass
            return True
        except Exception as e:
            print("file unlock err, msg:%s" % (str(e)))
            # self._sfplog(LOG_ERROR_LEVEL, "file unlock err, msg:%s" % (str(e)))
            return False

    def calc_reversion(self, eeprom_data, offset, size):
        # Management interface revision; the upper nibble is the whole number part and the lower nibble is the decimal part.
        # Example: 01h indicates version 0.1, 21h indicates version 2.1.
        int_data = int(eeprom_data[0], 16)
        msb = (int_data & 0xF0) >> 4
        lsb = int_data & 0x0F
        return ("{}.{}".format(int(msb), int(lsb)))

    def calc_maximum_power_consumption(self, eeprom_data, offset, size):
        # Maximum worst case Module power consumption represented in increaments of 0.25w/bit.
        # Example: 12W = 12/0.25 = 48 -->0x30
        try:
            power_num = int(eeprom_data[offset], 16)
            retval = '%.2f' %(power_num/4) + 'W'
        except Exception as err:
            retval = str(err)
        return retval

    def calc_temperature(self, eeprom_data, offset, size):
        # Internally measured temperature: signed 2’s complement in 1/256 degree Celsius increments
        try:
            cal_type = 1

            msb = int(eeprom_data[offset], 16)
            lsb = int(eeprom_data[offset + 1], 16)

            result = (msb << 8) | (lsb & 0xff)
            result = self.twos_comp(result, 16)

            if cal_type == 1:
                # Internal calibration
                result = float(result / 256.0)
                retval = '%.4f' %result + 'C'
            else:
                retval = 'Unknown'
        except Exception as err:
            retval = str(err)

        return retval

    def calc_voltage(self, eeprom_data, offset, size):
        # Internally measured 3.3 Volt input supply voltage, signed 16 bit value in 100 μV increments
        try:
            cal_type = 1
            msb = int(eeprom_data[offset], 16)
            lsb = int(eeprom_data[offset + 1], 16)
            result = (msb << 8) | (lsb & 0xff)
            if cal_type == 1:
                # Internal Calibration
                result = float(result * 0.0001)
                #print(indent, name, ' : %.4f' %result, 'Volts')
                retval = '%.4f' %result + 'Volts'
            else:
                #print(indent, name, ' : Unknown')
                retval = 'Unknown'
        except Exception as err:
            retval = str(err)
        return retval

    def calc_voltage_type_2(self, eeprom_data, offset, size):
        # Internally measured laser bias voltage monitor.  Unsigned integer in 10μV increments.
        # print('Laser%d voltage: %.03f mV' % (i, pageL_data[58 + i] / 100))
        return '%.03f mV' % (int(eeprom_data[offset], 16)/100)

    def calc_bias(self, eeprom_data, offset, size):
        # Internally measured laser bias current monitor.  Unsigned integer in 100μA increments.
        # print('Laser%d Current: %.03f mA' % (i, ((pageL_data[26 + 2 * i] * 256 + pageL_data[27 + 2 * i]) / 100)))
        return '%.03f mA' % ((int(eeprom_data[offset], 16) * 256 + int(eeprom_data[offset + 1], 16))/100)
        
    def calc_power(self, eeprom_data, offset, size):
        # Internally measured laser optical power monitor.  Unsigned integer in 10μW increments.
        # print('Laser%d power: %.03f mW %.03f dBm' % (i, PmW, Pdb))
        PmW = (int(eeprom_data[offset], 16) * 256 + int(eeprom_data[offset + 1], 16)) / 100
        if PmW > 0:
            Pdb = 10 * math.log(PmW, 10)
        else:
            Pdb = -40
        return '%.03f mW %.03f dBm' % (PmW, Pdb)

    rlm_identifier = {
        'Identifier': {
            'offset':0,
            'size':1,
            'type': 'enum',
            'decode': type_of_identifier
        }
    }

    rlm_reversion = {
        'Reversion': {
            'offset':0,
            'size':1,
            'type': 'func',
            'decode': {
                'func': calc_reversion
            }
        }
    }

    type_of_laser_grid = {
        '00': "CWDM4",
        '10': "DR4"
    }

    laser_grid_and_count_items = {
        'Laser grid': {
            'offset': 0,
            'size': 1,
            'type': 'enum',
            'bitmask': 0x10,
            'decode': type_of_laser_grid
        },
        # Number of lases implemented in module. Count start at 0.
        'Laser count': {
            'offset': 0,
            'size': 1,
            'type': 'int',
            'bitmask': 0x0F
        }
    }

    rlm_laser_grid_and_count = {
        'Laser grid and count': {
            'offset': 0,
            'size': 1,
            'type': 'nested',
            'decode': laser_grid_and_count_items
        }
    }

    rlm_module_state_and_interrupt = {
        'Module state & Interrupt': {
            'offset': 0,
            'size': 1,
            'type': 'enum',
            'decode': type_of_module_state_and_interrupt
        }
    }

    rlm_module_low_power_control = {
        'Module low power control': {
            'offset': 0,
            'size': 1,
            'type': 'enum',
            'decode': type_of_module_low_power_control
        }
    }

    def laser_mask_gen(key, bitvalue_map=None):
        output = dict()
        for offset in range(2):
            for bit in range(8):
                output["Laser{:02d} {}".format(bit+offset*8, key)] = {
                    "offset": offset,
                    "bit": bit,
                    "type": 'bitvalue',
                    "bitvalue_map": bitvalue_map
                }
        return output

    laser_active_status_masks = laser_mask_gen("active status", ["Inactive", "Active"])
    laser_disable_control_masks = laser_mask_gen("disable control", ["Enable", "Disable"])

    vcc_and_temperature_warning_alarm_masks = {
        'Latched high temperature alarm flag':
             {'offset':0,
              'bit': 0,
              'type': 'bitvalue'},
        'Latched low temperature alarm flag':
             {'offset':0,
              'bit': 1,
              'type': 'bitvalue'},
        'Latched high temperaturer warning flag':
             {'offset':0,
              'bit': 2,
              'type': 'bitvalue'},
        'Latched low temperature warning flag':
             {'offset':0,
              'bit': 3,
              'type': 'bitvalue'},
        'Latched high Vcc alarm flag':
             {'offset':0,
              'bit': 4,
              'type': 'bitvalue'},
        'Latched low Vcc alarm flag':
             {'offset':0,
              'bit': 5,
              'type': 'bitvalue'},
        'Latched high Vcc warning flag':
             {'offset':0,
              'bit': 6,
              'type': 'bitvalue'},
        'Latched low Vcc warning flag':
             {'offset':0,
              'bit': 7,
              'type': 'bitvalue'},
    }

    rlm_laser_disable_control = {
        'Laser disable control': {
            'offset': 0,
            'size': 2,
            'type': 'nested',
            'decode': laser_disable_control_masks
        }
    }

    rlm_laser_active_status = {
        'Laser active status': {
            'offset': 0,
            'size': 2,
            'type': 'nested',
            'decode': laser_active_status_masks
        }
    }

    rlm_vcc_and_temperature_warning_alarm = {
        "Vcc 3.3V & Temperature low/high warning/alarm": {
            'offset': 0,
            'size': 1,
            'type': 'nested',
            'decode': vcc_and_temperature_warning_alarm_masks
        }
    }

    laser_bias_warning_masks = laser_mask_gen("bias warning")
    laser_bias_alarm_masks = laser_mask_gen("bias alarm")

    rlm_laser_bias_warning = {
        "Laser bias warning": {
            'offset': 0,
            'size': 2,
            'type': 'nested',
            'decode': laser_bias_warning_masks
        }
    }

    rlm_laser_bias_alarm = {
        "Laser bias alarm": {
            'offset': 0,
            'size': 2,
            'type': 'nested',
            'decode': laser_bias_alarm_masks
        }
    }

    rlm_module_temperature_monitor = {
        'Module Temperature Monitor': {
            'offset': 0,
            'size': 2,
            'type': 'func',
            'decode': {
                'func': calc_temperature
            }
        }
    }

    rlm_module_supply_voltage_monitor = {
        'Module supply voltage Monitor': {
            'offset': 0,
            'size': 2,
            'type': 'func',
            'decode': {
                'func': calc_voltage
            }
        }
    }

    rlm_vendor_infomation_items = {
        'Vendor Name': {
            'offset': 0,
            'size': 16,
            'type': 'str'
        },
        'Vendor OUI': {
            'offset': 16,
            'size': 3,
            'type': 'hex'
        },
        'Vendor PN': {
            'offset': 19,
            'size': 16,
            'type': 'str'
        },
        'Vendor Rev': {
            'offset': 35,
            'size': 2,
            'type': 'str'
        },
        'Vendor SN': {
            'offset': 37,
            'size': 16,
            'type': 'str'
        },
        'Vendor Date Code(YYYY-MM-DD Lot)': {
            'offset': 53,
            'size': 8,
            'type': 'date'
        }   
    }

    # TODO: CLEI Code

    rlm_vendor_infomation = {
        'Vendor infomation': {
            'offset': 0,
            'size': 61,
            'type': 'nested',
            'decode': rlm_vendor_infomation_items
        }
    }

    rlm_maximum_power_consumption = {
        'Maximum power consumption': {
            'offset': 0,
            'size': 1,
            'type': 'func',
            'decode': {
                'func': calc_maximum_power_consumption
            }
        }
    }

    rlm_laser_power_mode_control = {
        'Laser power mode control': {
            'offset': 0,
            'size': 1,
            'type': 'hex'
        }
    }

    # TODO
    rlm_dom_threshold_items = {
        # 'Temp High Alarm': {
        #     'offset': 0,
        #     'size': 2,
        #     'type': 'func',
        #     'decode': {
        #         'func': calc_temperature
        #     }
        # },
        # 'Temp Low Alarm': {
        #     'offset': 2,
        #     'size': 2,
        #     'type': 'func',
        #     'decode': {
        #         'func': calc_temperature
        #     }
        # },
        # 'Temp High Warning': {
        #     'offset': 4,
        #     'size': 2,
        #     'type': 'func',
        #     'decode': {
        #         'func': calc_temperature
        #     }
        # },
        # 'Temp Low Warning': {
        #     'offset': 6,
        #     'size': 2,
        #     'type': 'func',
        #     'decode': {
        #         'func': calc_temperature
        #     }
        # },
        # 'Vcc High Alarm': {
        #     'offset': 8,
        #     'size': 2,
        #     'type': 'func',
        #     'decode': {
        #         'func': calc_voltage
        #     }
        # },
        # 'Vcc Low Alarm': {
        #     'offset': 10,
        #     'size': 2,
        #     'type': 'func',
        #     'decode': {
        #         'func': calc_voltage
        #     }
        # }, 
        # 'Vcc High Warning': {
        #     'offset': 12,
        #     'size': 2,
        #     'type': 'func',
        #     'decode': {
        #         'func': calc_voltage
        #     }
        # },
        # 'Vcc Low Warning': {
        #     'offset': 14,
        #     'size': 2,
        #     'type': 'func',
        #     'decode': {
        #         'func': calc_voltage
        #     }
        # },
        # 'TX Power High Alarm': {
        #     'offset': 16,
        #     'size': 2,
        #     'type': 'str'
        # },
        # 'TX Power Low Alarm': {
        #     'offset': 18,
        #     'size': 2,
        #     'type': 'str'
        # },
        # 'TX Power High Warning': {
        #     'offset': 20,
        #     'size': 2,
        #     'type': 'str'
        # },
        # 'TX Power Low Warning': {
        #     'offset': 22,
        #     'size': 2,
        #     'type': 'date'
        # }, 
        # 'Tx Bias High Alarm': {
        #     'offset': 24,
        #     'size': 2,
        #     'type': 'str'
        # },
        # 'Tx Bias High Warning': {
        #     'offset': 26,
        #     'size': 2,
        #     'type': 'str'
        # }
    }

    rlm_dom_threshold = {
        'ThresholdValues': {
            'offset': 0,
            'size': 28,
            'type': 'nested',
            'decode': rlm_dom_threshold_items
        }
    }

    def laser_items_gen(key, func, offset, size):
        output = dict()
        laser_size = int(size/16)
        for laser_index in range(16):
            output["Laser{:02d} {}".format(laser_index, key)] = {
                "offset": offset + laser_size * laser_index,
                "size": laser_size,
                "type": 'func',
                'decode': {
                    'func': func
                }
            }
        return output

    rlm_monitor_values = {
        'Laser current monitor': {
            'offset': 0,
            'size': 32,
            'type': 'nested',
            'decode': laser_items_gen('current monitor', calc_bias, 0, 32)
        },
        'Laser voltage monitor': {
            'offset': 32,
            'size': 16,
            'type': 'nested',
            'decode': laser_items_gen('voltage monitor', calc_voltage_type_2, 32, 16)
        },
        'Laser opitcal power monitor': {
            'offset': 49,
            'size': 32,
            'type': 'nested',
            'decode': laser_items_gen('opitcal power monitor', calc_power, 49, 32)
        }
    }

    def calc_tec_current_monitor(self, eeprom_data, offset, size):
        # TEC current monitor signed 2's complement number in 1/32767% increments
        # +32767 is max TEC current (100%) - Max heating
        # -32767 is min TEC current (100%) - Max cooling
        try:
            cal_type = 1
            msb = int(eeprom_data[offset], 16)
            lsb = int(eeprom_data[offset + 1], 16)
            result = (msb << 8) | (lsb & 0xff)
            result = self.twos_comp(result, 16)
            if cal_type == 1:
                result = float(result / 32767)*100
                retval = '%.2f' %result + '%'
            else:
                #print(indent, name, ' : Unknown')
                retval = 'Unknown'
        except Exception as err:
            retval = str(err)
        return retval

    rlm_tec_current_monitor = {
        'TEC current monitor': {
            'offset': 0,
            'size': 2,
            'type': 'func',
            'decode': {
                'func': calc_tec_current_monitor
            }
        }
    }

    def parse_identifier(self, sn_raw_data, start_pos):
        return self.get_data_pretty(self.parse(self.rlm_identifier, sn_raw_data, start_pos))

    def parse_reversion(self, sn_raw_data, start_pos):
        return self.get_data_pretty(self.parse(self.rlm_reversion, sn_raw_data, start_pos))

    def parse_laser_grid_and_count(self, sn_raw_data, start_pos):
        return self.get_data_pretty(self.parse(self.rlm_laser_grid_and_count, sn_raw_data, start_pos))

    def parse_module_state_and_interrupt(self, sn_raw_data, start_pos):
        return self.get_data_pretty(self.parse(self.rlm_module_state_and_interrupt, sn_raw_data, start_pos))

    def parse_module_low_power_control(self, sn_raw_data, start_pos):
        return self.get_data_pretty(self.parse(self.rlm_module_low_power_control, sn_raw_data, start_pos))

    def parse_laser_disable_control(self, sn_raw_data, start_pos):
        return self.get_data_pretty(self.parse(self.rlm_laser_disable_control, sn_raw_data, start_pos))

    def parse_laser_active_status(self, sn_raw_data, start_pos):
        return self.get_data_pretty(self.parse(self.rlm_laser_active_status, sn_raw_data, start_pos))

    def parse_vcc_and_temperature_warning_alarm(self, sn_raw_data, start_pos):
        return self.get_data_pretty(self.parse(self.rlm_vcc_and_temperature_warning_alarm, sn_raw_data, start_pos))

    def parse_laser_bias_warning(self, sn_raw_data, start_pos):
        return self.get_data_pretty(self.parse(self.rlm_laser_bias_warning, sn_raw_data, start_pos))

    def parse_laser_bias_alarm(self, sn_raw_data, start_pos):
        return self.get_data_pretty(self.parse(self.rlm_laser_bias_alarm, sn_raw_data, start_pos))

    def parse_module_temperature_monitor(self, sn_raw_data, start_pos):
        return self.get_data_pretty(self.parse(self.rlm_module_temperature_monitor, sn_raw_data, start_pos))

    def parse_module_supply_voltage_monitor(self, sn_raw_data, start_pos):
        return self.get_data_pretty(self.parse(self.rlm_module_supply_voltage_monitor, sn_raw_data, start_pos))

    def parse_vendor_infomation(self, sn_raw_data, start_pos):
        return self.get_data_pretty(self.parse(self.rlm_vendor_infomation, sn_raw_data, start_pos))

    def parse_maximum_power_consumption(self, sn_raw_data, start_pos):
        return self.get_data_pretty(self.parse(self.rlm_maximum_power_consumption, sn_raw_data, start_pos))

    def parse_laser_power_mode_control(self, sn_raw_data, start_pos):
        return self.get_data_pretty(self.parse(self.rlm_laser_power_mode_control, sn_raw_data, start_pos))

    # TODO
    def parse_dom_threshold(self, sn_raw_data, start_pos):
        return self.get_data_pretty(self.parse(self.rlm_dom_threshold, sn_raw_data, start_pos))

    def parse_monitor_values(self, sn_raw_data, start_pos):
        return self.get_data_pretty(self.parse(self.rlm_monitor_values, sn_raw_data, start_pos))

    def parse_tec_current_monitor(self, sn_raw_data, start_pos):
        return self.get_data_pretty(self.parse(self.rlm_tec_current_monitor, sn_raw_data, start_pos))


class cpo_v1(cpobase):

    def get_eeprom_path(self, oe):
        index_list = list(range(24,32))
        if oe >= len(index_list):
            return None
        return "/sys/bus/i2c/devices/i2c-{}/{}-0050/eeprom".format(index_list[oe], index_list[oe])