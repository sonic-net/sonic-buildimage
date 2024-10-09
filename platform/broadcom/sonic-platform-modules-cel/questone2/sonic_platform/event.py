#!/usr/bin/env python

#############################################################################
# Celestica
#
#############################################################################

import os

from .helper import APIHelper


PLATFORM_PATH = "/sys/devices/platform/"
SWITCH_BRD_PLATFORM = "questone2"
POLL_INTERVAL = 1


class SfpEvent:
    ''' Listen to insert/remove sfp events '''

    PORT_INFO_DIR = 'SFF'
    PATH_INT_SYSFS = "{0}/{port_name}/{type_prefix}_isr_flags"
    PATH_INTMASK_SYSFS = "{0}/{port_name}/{type_prefix}_isr_mask"
    PATH_PRS_SYSFS = "{0}/{port_name}/{prs_file_name}"
    PRESENT_EN = 0x10

    SFP_PORT_START = 1
    SFP_PORT_END = 48
    QSFP_PORT_START = 49
    QSFP_PORT_END = 56

    def __init__(self, sfp_list):
        self.num_sfp = len(sfp_list)
        self._api_helper = APIHelper()
        self.__initialize_interrupts()

    def __initialize_interrupts(self):
        # Initial Interrupt MASK for QSFP, SFP

        sfp_info_obj = {}
        port_info_path = os.path.join(
            PLATFORM_PATH, SWITCH_BRD_PLATFORM, self.PORT_INFO_DIR)

        for index in range(self.num_sfp):
            port_num = index + 1
            if port_num in range(self.SFP_PORT_START, self.SFP_PORT_END+1):
                port_name = "SFP{}".format(
                    str(port_num - self.SFP_PORT_START + 1))
                port_type = "sfp"
                sysfs_prs_file = "{}_modabs".format(port_type)
            elif port_num in range(self.QSFP_PORT_START, self.QSFP_PORT_END+1):
                port_name = "QSFP{}".format(
                    str(port_num - self.QSFP_PORT_START + 1))
                port_type = "qsfp"
                sysfs_prs_file = "{}_modprs".format(port_type)

            sfp_info_obj[index] = {}
            sfp_info_obj[index]['intmask_sysfs'] = self.PATH_INTMASK_SYSFS.format(
                port_info_path,
                port_name=port_name,
                type_prefix=port_type)

            sfp_info_obj[index]['int_sysfs'] = self.PATH_INT_SYSFS.format(
                port_info_path,
                port_name=port_name,
                type_prefix=port_type)

            sfp_info_obj[index]['prs_sysfs'] = self.PATH_PRS_SYSFS.format(
                port_info_path,
                port_name=port_name,
                prs_file_name=sysfs_prs_file)

            self._api_helper.write_file(
                sfp_info_obj[index]["intmask_sysfs"], hex(self.PRESENT_EN))

        self.sfp_info_obj = sfp_info_obj

    def __is_port_device_present(self, port_idx):
        prs_path = self.sfp_info_obj[port_idx]["prs_sysfs"]
        is_present = 1 - int(self._api_helper.read_txt_file(prs_path))
        return is_present

    def update_port_event_object(self, interrup_devices, port_dict):
        for port_idx in interrup_devices:
            port_dict[port_idx] = str(self.__is_port_device_present(port_idx))
        return port_dict

    def check_all_port_interrupt_event(self):
        interrupt_devices = {}
        for i in range(self.num_sfp):
            int_sysfs = self.sfp_info_obj[i]["int_sysfs"]
            interrupt_flags = self._api_helper.read_txt_file(int_sysfs)
            if interrupt_flags != '0x00':
                interrupt_devices[i] = 1
        return interrupt_devices


class FanEvent:
    ''' Listen to insert/remove fan events '''

    FAN_INSERT_STATE = '1'
    FAN_REMOVE_STATE = '0'

    def __init__(self, fan_list):
        self.fan_list = fan_list

    def get_fan_state(self):
        fan_dict = {}
        for idx, fan in enumerate(self.fan_list):
            fan_dict[idx] = self.FAN_INSERT_STATE if fan.get_presence(
            ) else self.FAN_REMOVE_STATE
        return fan_dict

    def check_fan_status(self, cur_fan_dict, fan_dict):
        for idx, fan in enumerate(self.fan_list):
            presence = fan.get_presence()
            if(presence and cur_fan_dict[idx] == self.FAN_REMOVE_STATE):
                fan_dict[idx] = self.FAN_INSERT_STATE
            elif(not presence and cur_fan_dict[idx] == self.FAN_INSERT_STATE):
                fan_dict[idx] = self.FAN_REMOVE_STATE
        return fan_dict


class VoltageEvent:
    ''' Listen to abnormal voltage events '''

    VOLTAGE_PATH = "ocores-i2c-polling/i2c-71/71-0035/iio:device0"
    VOLTAGE_SCALE_SYSFS = "in_voltage_scale"
    VOLTAGE_RAW_SYSFS = "in_voltage{}_raw"
    VOLTAGE_CONFIG = {
        0: {
            'name': 'XP3R3V_FD',
            'max': 3470,
            'min': 3140
        },
        1: {
            'name': 'XP3R3V',
            'max': 3470,
            'min': 3140
        },
        2: {
            'name': 'XP1R82V',
            'max': 1950,
            'min': 1470
        },
        3: {
            'name': 'XP1R05V',
            'max': 1068,
            'min': 1032
        },
        4: {
            'name': 'XP1R7V',
            'max': 1785,
            'min': 1615
        },
        5: {
            'name': 'XP1R2V',
            'max': 1260,
            'min': 1140
        },
        6: {
            'name': 'XP1R3V',
            'max': 1352,
            'min': 1248
        },
        7: {
            'name': 'XP1R5V',
            'max': 1580,
            'min': 1430
        },
        8: {
            'name': 'XP2R5V',
            'max': 2750,
            'min': 2375
        },
        9: {
            'name': 'XP0R6V_VTT',
            'max': 632,
            'min': 568
        },
    }
    VOLTAGE_NORMAL_EVENT = '0'
    VOLTAGE_ABNORMAL_EVENT = '1'

    def __init__(self):
        self.voltage_path = os.path.join(
            PLATFORM_PATH, SWITCH_BRD_PLATFORM, self.VOLTAGE_PATH)
        self.voltage_idx = [0, 1, 8]
        self._api_helper = APIHelper()
        self.voltage_scale = self._get_voltage_scale()

    def _get_voltage_scale(self):
        voltage_scale_path = os.path.join(
            self.voltage_path, self.VOLTAGE_SCALE_SYSFS)
        get_voltage_scale = self._api_helper.read_txt_file(voltage_scale_path)
        if get_voltage_scale:
            return float(get_voltage_scale)
        else:
            return 0

    def get_voltage_state(self):
        voltage_mv_dict = {}
        for idx in range(0, len(self.VOLTAGE_CONFIG)):
            voltage_raw_path = os.path.join(
                self.voltage_path, self.VOLTAGE_RAW_SYSFS.format(idx))
            in_voltageX_raw = self._api_helper.read_txt_file(voltage_raw_path)

            voltage_mV = float(in_voltageX_raw) * self.voltage_scale
            if idx in self.voltage_idx:
                voltage_mV *= 2

            v_name = self.VOLTAGE_CONFIG[idx]['name']
            max_v = self.VOLTAGE_CONFIG[idx]['max']
            min_v = self.VOLTAGE_CONFIG[idx]['min']

            voltage_mv_dict[v_name] = self.VOLTAGE_NORMAL_EVENT
            if voltage_mV > max_v or voltage_mV < min_v:
                voltage_mv_dict[v_name] = self.VOLTAGE_ABNORMAL_EVENT
        return voltage_mv_dict

    def check_voltage_status(self, cur_voltage_dict, int_voltage):
        voltage_dict = self.get_voltage_state()
        for v_name in voltage_dict:
            if voltage_dict[v_name] != cur_voltage_dict[v_name]:
                int_voltage[v_name] = voltage_dict[v_name]
        return int_voltage
