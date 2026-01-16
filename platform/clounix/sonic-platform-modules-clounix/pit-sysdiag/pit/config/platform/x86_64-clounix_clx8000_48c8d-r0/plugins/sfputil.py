# -*- coding:utf-8
from pit_util_common import run_command
from test_case import TestCaseCommon
from errcode import E
import re
import json
import os

cmd_sfp_presence = "sfputil show presence"
cmd_sfp_eeprom = "sfputil show eeprom -d"
cmd_sft_show_lpmode = "sfputil show lpmode"
TRANSCEIVER_DEVICES_PATH = "/sys_switch/transceiver"

class SfpUtil(object):
    def parse_output_port_list(self, output_lines):
        res = {}
        index = 0
        for line in output_lines:
            fields = line.split()
            res[index] = fields[0]
            index = index + 1
        return res

    def parse_output(self, output_lines):
        """
        @summary: For parsing command output. The output lines should have format 'key value'.
        @param output_lines: Command output lines
        @return: Returns result in a dictionary
        """
        res = {}
        for line in output_lines:
            fields = line.split()
            if len(fields) != 2:
                continue
            res[fields[0]] = fields[1]
        return res

    def parse_output_lpmode(self, output_lines):
        res = {}
        for line in output_lines:
            fields = line.split()
            if fields[1] == 'On':
                res[fields[0]] = fields[1]
        return res

    # 获取sfp端口列表
    @property
    def sfp_port_list(self):
        '''
        SFP Ports list
        @return LIST [1, 2, 3, ...]
        '''
        SFP_PORT_LIST = []
        status, output = run_command(cmd_sfp_presence)
        sfp_presence = output.splitlines()
        parsed_presence = self.parse_output_port_list(sfp_presence[2:])
    
        SFP_PORT_LIST = list(parsed_presence.values())
        return SFP_PORT_LIST[0:48]

    # 获取qsfp端口列表
    @property 
    def qsfp_ports(self):
        '''
        QSFP Ports list
        @return LIST [1, 2, 3, ...]
        '''
        QSFP_PORT_LIST = []
        status, output = run_command(cmd_sfp_presence)
        sfp_presence = output.splitlines()
        parsed_presence = self.parse_output_port_list(sfp_presence[2:])
    
        QSFP_PORT_LIST = list(parsed_presence.values())
        return QSFP_PORT_LIST[48:57]

    # 获取光模块presence
    def get_presence(self, port_index):
        status, output = run_command(cmd_sfp_presence)
        sfp_presence = output.splitlines()
        parsed_presence = self.parse_output(sfp_presence[2:])
        if port_index in parsed_presence:
            return True
        else:
            return False

    # 获取光模块qsfp interrupt
    def get_interrupt(self, port_index):
        return False

    # 获取光模块qsfp low power mode
    def get_low_power_mode(self, port_index):
        status, output = run_command(cmd_sft_show_lpmode)
        sfp_lpmode = output.splitlines()
        parsed_lpmode = self.parse_output_lpmode(sfp_lpmode[2:])
        if port_index in parsed_lpmode:
            return True
        else:
            return False

    def set_low_power_mode(self, port_index, enable):
        return True

    # 获取光模块qsfp reset
    def get_reset(self, port_index):
        return False

    def reset(self, port_index):
        return True

    # 获取光模块sfp tx fault
    def get_tx_fault(self, port_index):
        return False

    # 获取光模块sfp rx los
    def get_rx_los(self, port_index):
        return False

    # 获取光模块sfp tx disable
    def get_tx_disable(self, port_index):
        return False

    def set_tx_disable(self, port_index, enable):
        return True
    
    def check_transceiver_eeprom(self, path):
        eeprom_path = "".join([path, "/eeprom"])
        try:
            with open(eeprom_path,"rb") as f:
                data = f.read(1)
                f.close()
                return True
        except:
            return False

    def get_eeprom_raw(self, port_index):
        t1 =  re.match(r"^Ethernet(\d+)", port_index)
        if t1:
            index = int(t1.group(1).strip())
            if index < 200:
                index = index / 4 + 1
            else:
                index = (index / 4) - (index - 192) / 8 + 1
            
            path = TRANSCEIVER_DEVICES_PATH + '/eth' + str(index)
            print("PATH:",path)
            ret = self.check_transceiver_eeprom(path)
            if ret == False:
                return None
            else:
                return ret
        else:
            return None


