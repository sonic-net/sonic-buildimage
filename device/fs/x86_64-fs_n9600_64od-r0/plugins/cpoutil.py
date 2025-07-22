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

class CpoException(Exception):
    def __init__(self, detailed_error_message):
        super(CpoException, self).__init__(detailed_error_message)

class cpoutilbase(object):

    def __init__(self) -> None:
        self.cpo_api = cpo_v1()

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
    
    def mutil_reset_oe(self, oe_list: list):
        return self.cpo_api.reset_oe(oe_list)

    def get_oe_list(self):
        return self.cpo_api.get_oe_list()

    def get_rlm_list_by_oe(self, oe):
        return self.cpo_api.get_rlm_list_by_oe(oe)

    def get_rlm_list(self):
        return self.cpo_api.get_rlm_list()

    def get_oe_by_rlm(self, rlm):
        return self.cpo_api.get_oe_by_rlm(rlm)

    def get_rlm_name(self, rlm):
        return "RLM{}".format(rlm)

    def get_port_index_list(self):
        return self.cpo_api.get_port_index_list()

    def get_port_index_list_by_oe(self, oe):
        return self.cpo_api.get_port_index_list_by_oe(oe)

    def get_oe_by_port_index(self, port_index):
        return self.cpo_api.get_oe_by_port_index(port_index)

    def get_rlm_presence(self, rlm):
        if rlm not in self.get_rlm_list():
            print("RLM index out of range!")
            return False
        return self.cpo_api.get_rlm_presence(rlm)

    def get_rlm_info_dict(self, rlm):
        if rlm not in self.get_rlm_list():
            return "RLM index out of range!"
        oe = self.cpo_api.get_oe_by_rlm(rlm)

        if self.cpo_api.get_rlm_list_by_oe(oe).index(rlm) == 0:
            RLM_PAGE_BASE_OFFSET = RLMA_PAGE_BASE_OFFSET
        else:
            RLM_PAGE_BASE_OFFSET = RLMB_PAGE_BASE_OFFSET

        rlm_info_dict = dict()
        rlm_api = cpo_v1()

        try:
            if not rlm_api._file_rw_lock(oe):
                return "EEPROM file lock failed!"

            rlm_vendor_infomation_raw = rlm_api.read_eeprom(oe, RLM_VENDOR_INFOMATION_PAGE + RLM_PAGE_BASE_OFFSET, RLM_VENDOR_INFOMATION_OFFSET, RLM_VENDOR_INFOMATION_WIDTH)
            rlm_b1_checksum_raw = rlm_api.read_eeprom(oe, RLM_B1_CEHCK_SUM_PAGE + RLM_PAGE_BASE_OFFSET, RLM_B1_CEHCK_SUM_OFFSET, RLM_B1_CEHCK_SUM_WIDTH)

            if not self.get_rlm_presence(rlm):
                return "RLM not presence"

            # b1_page_sum = sum([int(byte, 16) for byte in rlm_vendor_infomation_raw])&0xff
            # if b1_page_sum == 0 and int(rlm_b1_checksum_raw[0], 16) == 0:
            #     return "RLM not presence."
            # if b1_page_sum != int(rlm_b1_checksum_raw[0], 16):
            #     raise CPOException("page b1 byte 80 to byte 250 checksum failed, {} != {}".format(b1_page_sum, int(rlm_b1_checksum_raw[0], 16)))

            rlm_identifier_raw = rlm_api.read_eeprom(oe, RLM_IDENTIFIER_PAGE + RLM_PAGE_BASE_OFFSET, RLM_IDENTIFIER_OFFSET, RLM_IDENTIFIER_WIDTH)
            if rlm_identifier_raw is not None:
                rlm_identifier_data = rlm_api.parse_identifier(rlm_identifier_raw, 0)

            rlm_reversion_raw = rlm_api.read_eeprom(oe, RLM_REVERSION_PAGE + RLM_PAGE_BASE_OFFSET, RLM_REVERSION_OFFSET, RLM_REVERSION_WIDTH)
            if rlm_reversion_raw is not None:
                rlm_reversion_data = rlm_api.parse_reversion(rlm_reversion_raw, 0)

            rlm_laser_grid_and_count_raw = rlm_api.read_eeprom(oe, RLM_LASER_GRID_AND_COUNT_PAGE + RLM_PAGE_BASE_OFFSET, RLM_LASER_GRID_AND_COUNT_OFFSET, RLM_LASER_GRID_AND_COUNT_WIDTH)
            if rlm_laser_grid_and_count_raw is not None:
                rlm_laser_grid_and_count_data = rlm_api.parse_laser_grid_and_count(rlm_laser_grid_and_count_raw, 0)

            rlm_module_state_and_interrupt_raw = rlm_api.read_eeprom(oe, RLM_MODULE_STATE_AND_INTERRUPT_PAGE + RLM_PAGE_BASE_OFFSET, RLM_MODULE_STATE_AND_INTERRUPT_OFFSET, RLM_MODULE_STATE_AND_INTERRUPT_WIDTH)
            if rlm_module_state_and_interrupt_raw is not None:
                rlm_module_state_and_interrupt_data = rlm_api.parse_module_state_and_interrupt(rlm_module_state_and_interrupt_raw, 0)

            rlm_module_low_power_control_raw = rlm_api.read_eeprom(oe, RLM_MODULE_LOW_POWER_CONTROL_PAGE + RLM_PAGE_BASE_OFFSET, RLM_MODULE_LOW_POWER_CONTROL_OFFSET, RLM_MODULE_LOW_POWER_CONTROL_WIDTH)
            if rlm_module_low_power_control_raw is not None:
                rlm_module_low_power_control_data = rlm_api.parse_module_low_power_control(rlm_module_low_power_control_raw, 0)

            rlm_laser_disable_control_raw = rlm_api.read_eeprom(oe, RLM_LASER_DISABLE_CONTROL_PAGE + RLM_PAGE_BASE_OFFSET, RLM_LASER_DISABLE_CONTROL_OFFSET, RLM_LASER_DISABLE_CONTROL_WIDTH)
            if rlm_laser_disable_control_raw is not None:
                rlm_laser_disable_control_data = rlm_api.parse_laser_disable_control(rlm_laser_disable_control_raw, 0)

            rlm_laser_active_status_raw = rlm_api.read_eeprom(oe, RLM_LASER_ACTIVE_STATUS_PAGE + RLM_PAGE_BASE_OFFSET, RLM_LASER_ACTIVE_STATUS_OFFSET, RLM_LASER_ACTIVE_STATUS_WIDTH)
            if rlm_laser_active_status_raw is not None:
                rlm_laser_active_status_data = rlm_api.parse_laser_active_status(rlm_laser_active_status_raw, 0)

            rlm_vcc_and_temperature_warning_alarm_raw = rlm_api.read_eeprom(oe, RLM_VCC_AND_TEMPERATURE_WARNING_ALARM_PAGE + RLM_PAGE_BASE_OFFSET, RLM_VCC_AND_TEMPERATURE_WARNING_ALARM_OFFSET, RLM_VCC_AND_TEMPERATURE_WARNING_ALARM_WIDTH)
            if rlm_vcc_and_temperature_warning_alarm_raw is not None:
                rlm_vcc_and_temperature_warning_alarm_data = rlm_api.parse_vcc_and_temperature_warning_alarm(rlm_vcc_and_temperature_warning_alarm_raw, 0)

            rlm_laser_bias_warning_raw = rlm_api.read_eeprom(oe, RLM_LASER_BIAS_WARNING_PAGE + RLM_PAGE_BASE_OFFSET, RLM_LASER_BIAS_WARNING_OFFSET, RLM_LASER_BIAS_WARNING_WIDTH)
            if rlm_laser_bias_warning_raw is not None:
                rlm_laser_bias_warning_data = rlm_api.parse_laser_bias_warning(rlm_laser_bias_warning_raw, 0)

            rlm_laser_bias_alarm_raw = rlm_api.read_eeprom(oe, RLM_LASER_BIAS_ALARM_PAGE + RLM_PAGE_BASE_OFFSET, RLM_LASER_BIAS_ALARM_OFFSET, RLM_LASER_BIAS_ALARM_WIDTH)
            if rlm_laser_bias_alarm_raw is not None:
                rlm_laser_bias_alarm_data = rlm_api.parse_laser_bias_alarm(rlm_laser_bias_alarm_raw, 0)

            rlm_module_temperature_monitor_raw = rlm_api.read_eeprom(oe, RLM_MODULE_TEMPERATURE_MONITOR_PAGE + RLM_PAGE_BASE_OFFSET, RLM_MODULE_TEMPERATURE_MONITOR_OFFSET, RLM_MODULE_TEMPERATURE_MONITOR_WIDTH)
            if rlm_module_temperature_monitor_raw is not None:
                rlm_module_temperature_monitor_data = rlm_api.parse_module_temperature_monitor(rlm_module_temperature_monitor_raw, 0)

            rlm_module_supply_voltage_monitor_raw = rlm_api.read_eeprom(oe, RLM_MODULE_SUPPLY_VOLTAGE_MONITOR_PAGE + RLM_PAGE_BASE_OFFSET, RLM_MODULE_SUPPLY_VOLTAGE_MONITOR_OFFSET, RLM_MODULE_SUPPLY_VOLTAGE_MONITOR_WIDTH)
            if rlm_module_supply_voltage_monitor_raw is not None:
                rlm_module_supply_voltage_monitor_data = rlm_api.parse_module_supply_voltage_monitor(rlm_module_supply_voltage_monitor_raw, 0)

            if rlm_vendor_infomation_raw is not None:
                rlm_vendor_infomation_data = rlm_api.parse_vendor_infomation(rlm_vendor_infomation_raw, 0)

            rlm_maximum_power_consumption_raw = rlm_api.read_eeprom(oe, RLM_MAXIMUM_POWER_CONSUMPTION_PAGE + RLM_PAGE_BASE_OFFSET, RLM_MAXIMUM_POWER_CONSUMPTION_OFFSET, RLM_MAXIMUM_POWER_CONSUMPTION_WIDTH)
            if rlm_maximum_power_consumption_raw is not None:
                rlm_maximum_power_consumption_data = rlm_api.parse_maximum_power_consumption(rlm_maximum_power_consumption_raw, 0)

            rlm_laser_power_mode_control_raw = rlm_api.read_eeprom(oe, RLM_LASER_POWER_MODE_CONTROL_PAGE + RLM_PAGE_BASE_OFFSET, RLM_LASER_POWER_MODE_CONTROL_OFFSET, RLM_LASER_POWER_MODE_CONTROL_WIDTH)
            if rlm_laser_power_mode_control_raw is not None:
                rlm_laser_power_mode_control_data = rlm_api.parse_laser_power_mode_control(rlm_laser_power_mode_control_raw, 0)

            rlm_dom_threshold_raw = rlm_api.read_eeprom(oe, RLM_DOM_THRESHOLD_PAGE + RLM_PAGE_BASE_OFFSET, RLM_DOM_THRESHOLD_OFFSET, RLM_DOM_THRESHOLD_WIDTH)
            if rlm_dom_threshold_raw is not None:
                rlm_dom_threshold_data = rlm_api.parse_dom_threshold(rlm_dom_threshold_raw, 0)

            rlm_monitor_values_raw = rlm_api.read_eeprom(oe, RLM_MONITOR_VALUES_PAGE + RLM_PAGE_BASE_OFFSET, RLM_MONITOR_VALUES_OFFSET, RLM_MONITOR_VALUES_WIDTH)
            if rlm_monitor_values_raw is not None:
                rlm_monitor_values_data = rlm_api.parse_monitor_values(rlm_monitor_values_raw, 0)

            rlm_tec_current_monitor_raw = rlm_api.read_eeprom(oe, RLM_TEC_CURRENT_MONITOR_PAGE + RLM_PAGE_BASE_OFFSET, RLM_TEC_CURRENT_MONITOR_OFFSET, RLM_TEC_CURRENT_MONITOR_WIDTH)
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
            rlm_info_dict['ThresholdValues'] = rlm_dom_threshold_data['data']['ThresholdValues']
            rlm_info_dict['Laser current monitor'] = rlm_monitor_values_data['data']['Laser current monitor']
            rlm_info_dict['Laser voltage monitor'] = rlm_monitor_values_data['data']['Laser voltage monitor']
            rlm_info_dict['Laser opitcal power monitor'] = rlm_monitor_values_data['data']['Laser opitcal power monitor']
            rlm_info_dict['TEC current monitor'] = rlm_tec_current_monitor_data['data']['TEC current monitor']
            
            if (rlm_info_dict != None) and (rlm_info_dict['Laser grid and count'] != None):
                laser_count = rlm_info_dict['Laser grid and count']['Laser count']
                rlm_info_dict = self.laser_count_implemented(rlm_info_dict, laser_count)

        except CpoException:
            rlm_info_dict = None
        except BaseException:
            rlm_info_dict = None
            print(traceback.format_exc())
        finally:
            rlm_api._file_rw_unlock(oe)

        if rlm_info_dict == None:
            return "RLM eeprom get failed!"

        return rlm_info_dict

    def get_rlm_dom_info_dict(self, rlm):
        return self.get_rlm_info_dict(rlm)

    def get_rlm_lpmode(self, rlm):
        oe = self.cpo_api.get_oe_by_rlm(rlm)

        if self.cpo_api.get_rlm_list_by_oe(oe).index(rlm) == 0:
            RLM_PAGE_BASE_OFFSET = RLMA_PAGE_BASE_OFFSET
        else:
            RLM_PAGE_BASE_OFFSET = RLMB_PAGE_BASE_OFFSET

        rlm_enable = 0x00
        try:
            if not self.cpo_api._file_rw_lock(oe):
                return False

            rlm_laser_power_mode_control_raw = self.cpo_api.read_eeprom(oe, RLM_LASER_POWER_MODE_CONTROL_PAGE + RLM_PAGE_BASE_OFFSET, RLM_LASER_POWER_MODE_CONTROL_OFFSET, RLM_LASER_POWER_MODE_CONTROL_WIDTH)
            if rlm_laser_power_mode_control_raw is not None:
                rlm_laser_power_mode_control_data = self.cpo_api.parse_laser_power_mode_control(rlm_laser_power_mode_control_raw, 0)

            rlm_enable = rlm_laser_power_mode_control_data['data']['Laser power mode control']

        except BaseException:
            print(traceback.format_exc())
        finally:
            self.cpo_api._file_rw_unlock(oe)
        # return True if rlm is half power mode
        return int(rlm_enable, 16) == 0x00

    def set_rlm_lpmode_mutil(self, rlm_list:list, lpmode):
        retval = False
        oe_set = set()
        oe_need_enable_set = set()

        for rlm in rlm_list:
            oe_set.add(self.get_oe_by_rlm(rlm))
        
        # FIXME: this will affect the other ports belong to this oe
        # for oe in oe_set:
        #     if self.get_oe_enable(oe):
        #         oe_need_enable_set.add(oe)

        try:
            # lock eeprom
            for oe in oe_set:
                if not self.cpo_api._file_rw_lock(oe):
                    raise CpoException("eeprom file lock failed!")
            # disable oe
            for oe in oe_need_enable_set:
                self._set_oe_enable(oe, False)
            # set rlm lpmode
            time.sleep(1)
            retval = self._set_rlm_lpmode_mutil(rlm_list, lpmode)
            time.sleep(1)
        except Exception:
            retval = False
            print(traceback.format_exc())
        finally:
            # enable oe
            for oe in oe_need_enable_set:
                self._set_oe_enable(oe, True)
            # unlock eeprom
            for oe in oe_set:
                self.cpo_api._file_rw_unlock(oe)
        return retval
    

    def _set_rlm_lpmode_mutil(self, rlm_list:list, lpmode):
        for rlm in rlm_list:
            oe = self.get_oe_by_rlm(rlm)
            if lpmode == True:
                if self.cpo_api.get_rlm_list_by_oe(oe).index(rlm) == 0:
                    # disable RLM from full power mode (AOP ~ 20dBm): write 0x00 to byte 0x80 on page 0xB2.
                    self.cpo_api.write_eeprom(oe=oe, page=0xB2, offset=0x80, num_bytes=1, write_buffer=bytearray([0x00]))
                else:
                    # disable RLM from full power mode (AOP ~ 20dBm): write 0x00 to byte 0x80 on page 0xB6.
                    self.cpo_api.write_eeprom(oe=oe, page=0xB6, offset=0x80, num_bytes=1, write_buffer=bytearray([0x00]))
            else:
                if self.cpo_api.get_rlm_list_by_oe(oe).index(rlm) == 0:
                    # Enable RLM to full power mode (AOP ~ 20dBm): write 0xFF to byte 0x80 on page 0xB2.
                    self.cpo_api.write_eeprom(oe=oe, page=0xB2, offset=0x80, num_bytes=1, write_buffer=bytearray([0xFF]))
                else:
                    # Enable RLM to full power mode (AOP ~ 20dBm): write 0xFF to byte 0x80 on page 0xB6.
                    self.cpo_api.write_eeprom(oe=oe, page=0xB6, offset=0x80, num_bytes=1, write_buffer=bytearray([0xFF]))
        return True
    
    def _set_oe_enable(self, oe, enable):
        if enable:
            # Exit low power mode: write 0x20 to byte 0x1A on page 0x00. enabled bit7, the broadcast bit.
            self.cpo_api.write_eeprom(oe=oe, page=0x00, offset=0x1A, num_bytes=1, write_buffer=bytearray([0xA0]))
            # Enable data path: write 0x00 to byte 0x80 on page 0x10.
            self.cpo_api.write_eeprom(oe=oe, page=0x10, offset=0x80, num_bytes=1, write_buffer=bytearray([0x00]))
            # Enable Tx: write 0x00 to byte 0x82 on page 0x10.
            self.cpo_api.write_eeprom(oe=oe, page=0x10, offset=0x82, num_bytes=1, write_buffer=bytearray([0x00]))
            # Exit low power mode: write 0x20 to byte 0x1A on page 0x00.
            self.cpo_api.write_eeprom(oe=oe, page=0x00, offset=0x1A, num_bytes=1, write_buffer=bytearray([0x20]))
        else:
            # enter low power mode: write 0x30 to byte 0x1A on page 0x00. enabled bit7, the broadcast bit.
            self.cpo_api.write_eeprom(oe=oe, page=0x00, offset=0x1A, num_bytes=1, write_buffer=bytearray([0xB0]))
            # disable Tx: write 0xff to byte 0x82 on page 0x10.
            self.cpo_api.write_eeprom(oe=oe, page=0x10, offset=0x82, num_bytes=1, write_buffer=bytearray([0xff]))
            # disable data path: write 0xff to byte 0x80 on page 0x10.
            self.cpo_api.write_eeprom(oe=oe, page=0x10, offset=0x80, num_bytes=1, write_buffer=bytearray([0xff]))
            # enter low power mode: write 0x30 to byte 0x1A on page 0x00.
            self.cpo_api.write_eeprom(oe=oe, page=0x00, offset=0x1A, num_bytes=1, write_buffer=bytearray([0x30]))

        return True

    def set_oe_enable(self, oe, enable):
        if oe not in self.get_oe_list():
            return False
        retval = True
        try:
            if not self.cpo_api._file_rw_lock(oe):
                raise CpoException("eeprom file lock failed!")
            retval = self._set_oe_enable(oe=oe, enable=enable)
        except:
            retval = False
        finally:
            self.cpo_api._file_rw_unlock(oe)
        return retval

    def _get_oe_enable(self, oe):
        raw_data = self.cpo_api.read_eeprom(oe=oe, page=0x00, offset=0x1A, num_bytes=1)[0]
        # return True if oe is enabled
        return (int(raw_data, 16) & 0x10) != 0x10

    def get_oe_enable(self, oe):
        if oe not in self.get_oe_list():
            print("OE index out of range!")
            return False
        enable = False
        try:
            if not self.cpo_api._file_rw_lock(oe):
                return False
            enable = self._get_oe_enable(oe)
        except BaseException:
            print(traceback.format_exc())
        finally:
            self.cpo_api._file_rw_unlock(oe)
        return enable

    def get_speed(self, port_index):
        retval = 0
        try:
            oe_index = self.cpo_api.get_oe_bank_by_port(port_index)[0]
            if not self.cpo_api._file_rw_lock(oe_index):
                raise CpoException("eeprom file lock failed!")
            retval = self.cpo_api.get_speed(port_index)
        except Exception as e:
            print(str(e))
            retval = 0
        finally:
            self.cpo_api._file_rw_unlock(oe_index)
        return retval

    def set_speed(self, port_index, speed):
        retval = True
        try:
            oe = self.cpo_api.get_oe_bank_by_port(port_index)[0]
            if not self.cpo_api._file_rw_lock(oe):
                raise CpoException("eeprom file lock failed!")
            self.set_oe_enable(oe, False)
            time.sleep(5)
            retval = self.cpo_api.set_speed(port_index, speed)
        except Exception as e:
            print(str(e))
            retval = False
        finally:
            self.set_oe_enable(oe, True)
            time.sleep(5)
            self.cpo_api._file_rw_unlock(oe)
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
        self.pidfile_dict = dict()
        self.page_dict = dict()
        self.bank_dict = dict()

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

    def _reset_page_bank_cache(self):
        self.bank_dict = dict()
        self.page_dict = dict()

    def _switch_bank(self, oe, bank):
        if self.bank_dict.get(oe, None) == bank:
            return True
        self.bank_dict[oe] = None
        
        bank_offset = 126 #0x7e
        num_bytes = 1
        self._write_eeprom(oe, bank_offset, num_bytes, bytearray([bank]))
        cur_bank = self._read_eeprom(oe, bank_offset, num_bytes)[0]
        if cur_bank == bank:
            self.bank_dict[oe] = cur_bank
            return True
        return False

    def _switch_page(self, oe, page):
        if self.page_dict.get(oe, None) == page:
            return True
        self.page_dict[oe] = None

        page_offset = 127 #0x7f
        num_bytes = 1
        self._write_eeprom(oe, page_offset, num_bytes, bytearray([page]))
        cur_page = self._read_eeprom(oe, page_offset, num_bytes)[0]
        if cur_page == page:
            self.page_dict[oe] = cur_page
            return True
        return False
    
    def read_fpga(self, path, offset, num_bytes):
        time.sleep(0.01)
        eeprom_raw = []
        for i in range(0, num_bytes):
            eeprom_raw.append("0x00")
        try:
            with open(path, mode='rb', buffering=0) as f:
                f.seek(offset)
                raw = f.read(num_bytes)
                raw = bytearray(raw)
                for n in range(0, num_bytes):
                    eeprom_raw[n] = hex(raw[n])[2:].zfill(2)
                return eeprom_raw
        except BaseException:
            print(traceback.format_exc())
            # self._sfplog(LOG_ERROR_LEVEL, traceback.format_exc())
            pass
        return None

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
        return False
    
    def _file_rw_lock(self, oe):
        pidfile = self.pidfile_dict.get(oe, None)
        if pidfile == None:
            pidfile = open(self.get_eeprom_path(oe), "r")
            self.pidfile_dict[oe] = pidfile
        if pidfile == None:
            return False
        # Retry 100 times to lock file
        for i in range(0, 100):
            try:
                fcntl.flock(pidfile, fcntl.LOCK_EX | fcntl.LOCK_NB)
                self._reset_page_bank_cache()
                return True
            except Exception:
                time.sleep(0.001)
                continue

        pidfile.close()
        self.pidfile_dict[oe] = None
        return False

    def _file_rw_unlock(self, oe):
        pidfile = self.pidfile_dict.get(oe, None)
        if pidfile == None:
            return True
        try:
            fcntl.flock(pidfile, fcntl.LOCK_UN)
            pidfile.close()
            self.pidfile_dict[oe] = None
            return True
        except Exception as e:
            print("file unlock err, msg:%s" % (str(e)))
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
        # Internally measured temperature: signed 2s complement in 1/256 degree Celsius increments
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
        # Internally measured 3.3 Volt input supply voltage, signed 16 bit value in 100 uv increments
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
        # Internally measured laser bias voltage monitor.  Unsigned integer in 10uV increments.
        # print('Laser%d voltage: %.03f mV' % (i, pageL_data[58 + i] / 100))
        return '%.03f mV' % (int(eeprom_data[offset], 16)/100)

    def calc_bias(self, eeprom_data, offset, size):
        # Internally measured laser bias current monitor.  Unsigned integer in 100uA increments.
        # print('Laser%d Current: %.03f mA' % (i, ((pageL_data[26 + 2 * i] * 256 + pageL_data[27 + 2 * i]) / 100)))
        return '%.03f mA' % ((int(eeprom_data[offset], 16) * 256 + int(eeprom_data[offset + 1], 16))/100)

    def calc_power(self, eeprom_data, offset, size):
        # Internally measured laser optical power monitor.  Unsigned integer in 10uW increments.
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

    rlm_dom_threshold_items = {
        'Temp High Alarm': {
            'offset': 0,
            'size': 2,
            'type': 'func',
            'decode': {
                'func': calc_temperature
            }
        },
        'Temp Low Alarm': {
            'offset': 2,
            'size': 2,
            'type': 'func',
            'decode': {
                'func': calc_temperature
            }
        },
        'Temp High Warning': {
            'offset': 4,
            'size': 2,
            'type': 'func',
            'decode': {
                'func': calc_temperature
            }
        },
        'Temp Low Warning': {
            'offset': 6,
            'size': 2,
            'type': 'func',
            'decode': {
                'func': calc_temperature
            }
        },
        'Vcc High Alarm': {
            'offset': 8,
            'size': 2,
            'type': 'func',
            'decode': {
                'func': calc_voltage
            }
        },
        'Vcc Low Alarm': {
            'offset': 10,
            'size': 2,
            'type': 'func',
            'decode': {
                'func': calc_voltage
            }
        }, 
        'Vcc High Warning': {
            'offset': 12,
            'size': 2,
            'type': 'func',
            'decode': {
                'func': calc_voltage
            }
        },
        'Vcc Low Warning': {
            'offset': 14,
            'size': 2,
            'type': 'func',
            'decode': {
                'func': calc_voltage
            }
        },
        'TX Power High Alarm': {
            'offset': 16,
            'size': 2,
            'type': 'func',
            'decode': {
                'func': calc_power
            }
        },
        'TX Power Low Alarm': {
            'offset': 18,
            'size': 2,
            'type': 'func',
            'decode': {
                'func': calc_power
            }
        },
        'TX Power High Warning': {
            'offset': 20,
            'size': 2,
            'type': 'func',
            'decode': {
                'func': calc_power
            }
        },
        'TX Power Low Warning': {
            'offset': 22,
            'size': 2,
            'type': 'func',
            'decode': {
                'func': calc_power
            }
        }, 
        'Tx Bias High Alarm': {
            'offset': 24,
            'size': 2,
            'type': 'func',
            'decode': {
                'func': calc_bias
            }
        },
        'Tx Bias High Warning': {
            'offset': 26,
            'size': 2,
            'type': 'func',
            'decode': {
                'func': calc_bias
            }
        }
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

    def parse_dom_threshold(self, sn_raw_data, start_pos):
        return self.get_data_pretty(self.parse(self.rlm_dom_threshold, sn_raw_data, start_pos))

    def parse_monitor_values(self, sn_raw_data, start_pos):
        return self.get_data_pretty(self.parse(self.rlm_monitor_values, sn_raw_data, start_pos))

    def parse_tec_current_monitor(self, sn_raw_data, start_pos):
        return self.get_data_pretty(self.parse(self.rlm_tec_current_monitor, sn_raw_data, start_pos))


class cpo_cust(cpobase):

    def __init__(self):
        super().__init__()

    def get_oe_list(self):
        return list(range(1, 8+1))

    def get_rlm_list_by_oe(self, oe):
        return [[1,2],[3,4],[5,6],[7,8],[9,10],[11,12],[13,14],[15,16]][oe-1]

    def get_rlm_list(self):
        return list(range(1, 16+1))

    def get_oe_by_rlm(self, rlm):
        for oe in self.get_oe_list():
            if rlm in self.get_rlm_list_by_oe(oe):
                return oe
        return -1

    def get_port_index_list(self):
        return list(range(1, 1+128))

    def get_port_index_list_by_oe(self, oe):
        start = (oe-1)*16 + 1
        end = oe*16
        return list(range(start, end+1))

    def get_oe_by_port_index(self, port_index):
        for oe in self.get_oe_list():
            if port_index in self.get_port_index_list_by_oe(oe):
                return oe
        raise CpoException("port_index out of range!")

    def write_eeprom_by_port_index(self, port_index, page, offset, num_bytes, write_buffer):
        oe = self.get_oe_bank_by_port(port_index)[0]
        bank = self.get_oe_bank_by_port(port_index)[1]
        self._switch_bank(oe, bank)
        return self.write_eeprom(oe, page, offset, num_bytes, write_buffer)
        
    def read_eeprom_by_port_index(self, port_index, page, offset, num_bytes):
        oe = self.get_oe_bank_by_port(port_index)[0]
        bank = self.get_oe_bank_by_port(port_index)[1]
        self._switch_bank(oe, bank)
        return self.read_eeprom(oe, page, offset, num_bytes)

    def get_eeprom_path(self, oe):
        oe_index = oe - 1
        index_list = list(range(24,32))
        if oe_index >= len(index_list):
            return None
        return "/sys/bus/i2c/devices/i2c-{}/{}-0050/eeprom".format(index_list[oe_index], index_list[oe_index])

    def get_oe_bank_by_port(self, port_index):
        if port_index not in self.get_port_index_list():
            raise CpoException("port_index out of range")
        oe = self.get_oe_by_port_index(port_index)
        oe_port_list = self.get_port_index_list_by_oe(oe)
        bank = oe_port_list.index(port_index) // 2
        lane = oe_port_list.index(port_index) %2
        return oe, bank, lane

    def get_speed(self, port_index):
        lane = self.get_oe_bank_by_port(port_index)[2]

        if lane == 0:
            apsel_contrl_value = int(self.read_eeprom_by_port_index(port_index, 0x11, 0xce, 1)[0], 16)
        else:
            apsel_contrl_value = int(self.read_eeprom_by_port_index(port_index, 0x11, 0xd2, 1)[0], 16)

        apsel_contrl_value = apsel_contrl_value & 0xF0

        if apsel_contrl_value == 0x10:
            return 400000
        elif apsel_contrl_value == 0x20:
            return 200000
        elif apsel_contrl_value == 0x30:
            return 100000
        else:
            raise CpoException("speed out of range, apsel_contrl_value: {}".format(apsel_contrl_value))

    def set_speed(self, port_index, speed):
        # port_index  oe_index  bank_index
        # print(self.get_oe_bank_by_port(port_index))
        oe = self.get_oe_bank_by_port(port_index)[0]
        bank = self.get_oe_bank_by_port(port_index)[1]
        lane = self.get_oe_bank_by_port(port_index)[2]

        # Step 1: Set byte 26 to 0x90. This will keep LPmode asserted and enabled bit7, the broadcast bit.
        # self.write_eeprom(oe=oe, page=0x00, offset=0x1A, num_bytes=1, write_buffer=bytearray([0x90]))
        # Step 1: Set byte 26 to 0x10. This will only keep LPmode asserted.
        self.write_eeprom(oe=oe, page=0x00, offset=0x1A, num_bytes=1, write_buffer=bytearray([0x10]))
        # Step 2: Set U10h byte 145-152 with values 0x10 0x10 0x10 0x10 0x18 0x18 0x18 0x18 or others accoding to speed
        if speed == 400000:
            apsel_contrl_value = 0x10
        elif speed == 200000:
            apsel_contrl_value = 0x20
        elif speed == 100000:
            apsel_contrl_value = 0x30
        else:
            raise CpoException("speed out of range")

        if lane == 1:
            apsel_contrl_value = apsel_contrl_value | 0x8
        
        if lane == 0:
            self.write_eeprom_by_port_index(port_index, 0x10, 0x91, 1, bytearray([apsel_contrl_value]))
            self.write_eeprom_by_port_index(port_index, 0x10, 0x92, 1, bytearray([apsel_contrl_value]))
            self.write_eeprom_by_port_index(port_index, 0x10, 0x93, 1, bytearray([apsel_contrl_value]))
            self.write_eeprom_by_port_index(port_index, 0x10, 0x94, 1, bytearray([apsel_contrl_value]))
        else:
            self.write_eeprom_by_port_index(port_index, 0x10, 0x95, 1, bytearray([apsel_contrl_value]))
            self.write_eeprom_by_port_index(port_index, 0x10, 0x96, 1, bytearray([apsel_contrl_value]))
            self.write_eeprom_by_port_index(port_index, 0x10, 0x97, 1, bytearray([apsel_contrl_value]))
            self.write_eeprom_by_port_index(port_index, 0x10, 0x98, 1, bytearray([apsel_contrl_value]))
        # Step 3: Apply_DataPathInit
        raw_value = int(self.read_eeprom_by_port_index(port_index, 0x10, 0x8F, 1)[0], 16)
        if lane == 0:
            self.write_eeprom_by_port_index(port_index, 0x10, 0x8F, 1, bytearray([raw_value | 0x0F]))
        else:
            self.write_eeprom_by_port_index(port_index, 0x10, 0x8F, 1, bytearray([raw_value | 0XF0]))
        # Step 4: Initialize the data path associated with host lane
        raw_value = int(self.read_eeprom_by_port_index(port_index, 0x10, 0x80, 1)[0], 16)
        if lane == 0:
            self.write_eeprom_by_port_index(port_index, 0x10, 0x80, 1, bytearray([raw_value & 0xF0]))
        else:
            self.write_eeprom_by_port_index(port_index, 0x10, 0x80, 1, bytearray([raw_value & 0X0F]))
        # Step 5: Tx output squelch permitted
        raw_value = int(self.read_eeprom_by_port_index(port_index, 0x10, 0x83, 1)[0], 16)
        if lane == 0:
            self.write_eeprom_by_port_index(port_index, 0x10, 0x83, 1, bytearray([raw_value & 0xF0]))
        else:
            self.write_eeprom_by_port_index(port_index, 0x10, 0x83, 1, bytearray([raw_value & 0X0F]))
        # Step 6: TODO Recover OE and RLM
        # Set byte 26 to 0x80
        self.write_eeprom(oe=oe, page=0x00, offset=0x1A, num_bytes=1, write_buffer=bytearray([0x80]))
        # # # Set UB2h byte 128 to 0xFF to bring RLM0 into full power mode
        # self.write_eeprom(oe=oe, page=0xB2, offset=0x80, num_bytes=1, write_buffer=bytearray([0xFF]))
        # # Set UB6h byte 128 to 0xFF to bring RLM1 into full power mode
        # self.write_eeprom(oe=oe, page=0xB6, offset=0x80, num_bytes=1, write_buffer=bytearray([0xFF]))

        return True

    def get_rlm_presence(self, rlm):
        presence = False
        oe = self.get_oe_by_rlm(rlm)
        if self.get_rlm_list(oe).index(rlm) == 0:
            RLM_PAGE_BASE_OFFSET = RLMA_PAGE_BASE_OFFSET
        else:
            RLM_PAGE_BASE_OFFSET = RLMB_PAGE_BASE_OFFSET
        try:
            if not self._file_rw_lock(oe):
                return False
            rlm_vendor_infomation_raw = self.read_eeprom(oe, RLM_VENDOR_INFOMATION_PAGE + RLM_PAGE_BASE_OFFSET, RLM_VENDOR_INFOMATION_OFFSET, RLM_VENDOR_INFOMATION_WIDTH)
            rlm_b1_checksum_raw = self.read_eeprom(oe, RLM_B1_CEHCK_SUM_PAGE + RLM_PAGE_BASE_OFFSET, RLM_B1_CEHCK_SUM_OFFSET, RLM_B1_CEHCK_SUM_WIDTH)
            b1_page_sum = sum([int(byte, 16) for byte in rlm_vendor_infomation_raw])&0xff
            if b1_page_sum != int(rlm_b1_checksum_raw[0], 16):
                raise CpoException("page b1 byte 80 to byte 250 checksum failed, {} != {}".format(b1_page_sum, int(rlm_b1_checksum_raw[0], 16)))
            if b1_page_sum != 0:
                presence = True
        except CpoException:
            pass
        except BaseException:
            print(traceback.format_exc())
        finally:
            self._file_rw_unlock(oe)
        return presence

    def reset_oe(self, oe_list: list):
        byte = 0xff
        for oe in oe_list:
            if oe not in self.get_oe_list():
                print("OE index out of range!")
                continue
            oe_index = oe - 1
            byte = byte & ~(0x01 << oe_index)
        result = os.system("i2cset -f -y 20 0x1e 0x72 0x%02X" % byte)
        if result != 0:
            return False
        time.sleep(3)
        result = os.system("i2cset -f -y 20 0x1e 0x72 0xff")
        return result == 0

class cpo_v1(cpo_cust):
    def __init__(self):
        super().__init__()
        self.presence_fpga_dict = {
            "/dev/fpga1": {
                "offset": {
                    "0x64": [None]*8 + list(range(8)) + [None]*8 + list(range(8,16))
                }
            }
        }

    def get_rlm_presence(self, rlm):
        presence = False
        if self.presence_fpga_dict == None:
            return presence
        rlm_index = rlm-1
        try:
            for dev, dev_value in self.presence_fpga_dict.items():
                for offset, offset_value in dev_value["offset"].items():
                    if rlm_index not in offset_value:
                        continue
                    fpga_data = self.read_fpga(dev, int(offset, 16), 4)
                    if fpga_data == None:
                        continue
                    rlm_bit_index = offset_value.index(rlm_index)
                    presence = (~int(fpga_data[rlm_bit_index // 8], 16)) & (0x1 << (rlm_bit_index % 8))
                    return presence
        except BaseException:
            print(traceback.format_exc())
        return presence


