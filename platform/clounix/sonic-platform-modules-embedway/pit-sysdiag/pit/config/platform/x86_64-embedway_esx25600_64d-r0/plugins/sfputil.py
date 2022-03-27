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
    def parse_output_port_list(self):
        res = {}
        index = 0
        status, output = run_command(cmd_sfp_presence)
        sfp_presence = output.splitlines()
        output_lines = sfp_presence[2:]
        for line in output_lines:
            fields = line.split()
            res[index] = fields[0]
            index = index + 1
        return res

    # 获取sfp端口列表
    @property
    def sfp_port_list(self):
        '''
        SFP Ports list
        @return LIST [1, 2, 3, ...]
        '''
        # port index 1-basee
        SFP_PORT_INDEX = [idx for idx in range(65, 65)]
        SFP_PORT_LIST = {}
        parsed_presence = self.parse_output_port_list()
    
        for index in SFP_PORT_INDEX:
            SFP_PORT_LIST.update({index: parsed_presence[index - 1]})
        return SFP_PORT_LIST

    # 获取qsfp端口列表
    @property 
    def qsfp_ports_list(self):
        '''
        QSFP Ports list
        @return LIST [1, 2, 3, ...]
        '''
        # port index 1-basee
        QSFP_PORT_INDEX = [idx for idx in range(1, 64)]
        QSFP_PORT_LIST = {}
        parsed_presence = self.parse_output_port_list()

        for index in QSFP_PORT_INDEX:
            QSFP_PORT_LIST.update({index: parsed_presence[index - 1]})
        return QSFP_PORT_LIST

    # 获取光模块presence
    def get_presence(self, index):
        path = TRANSCEIVER_DEVICES_PATH + '/eth' + str(index)
        present_path = "".join([path, "/present"])

        try:
            with open(present_path,"rb") as f:
                present = f.read()
                if 'NA' in present or len(present) == 0:
                    return False
                return False if int(present) == 0 else True
        except:
            return False

    # 获取光模块qsfp interrupt
    def get_interrupt(self, index):
        path = TRANSCEIVER_DEVICES_PATH + '/eth' + str(index)
        intr_path = "".join([path, "/interrupt"])

        try:
            with open(intr_path,"rb") as f:
                intr = f.read()
                if 'NA' in intr or len(intr) == 0:
                    return False
                return False if int(intr) == 0 else True
        except:
            return False

    # 获取光模块qsfp low power mode
    def get_low_power_mode(self, index):
        path = TRANSCEIVER_DEVICES_PATH + '/eth' + str(index)
        lpmode_path = "".join([path, "/low_power_mode"])

        try:
            with open(lpmode_path,"rb") as f:
                lpmode = f.read()
                if 'NA' in lpmode or len(lpmode) == 0:
                    return False
                return False if int(lpmode) == 0 else True
        # except ValueError:
        #     return False if int(lpmode, 16) == 0 else True
        except ValueError:
            value = re.search(br'0x([0-9a-fA-F]+)', lpmode).group(1).decode()
            return False if int(value, 16) == 0 else True
        except:
            return False

    def set_low_power_mode(self, index, enable):
        path = TRANSCEIVER_DEVICES_PATH + '/eth' + str(index)
        lpmode_path = "".join([path, "/low_power_mode"])
        try:
            if os.path.exists(lpmode_path):
                with open(lpmode_path, "w") as f:
                    f.write(str(enable))
                    return True
            else:
                return False
        except:
            print("Write low_power_mode failed")
            return False

    # 获取光模块qsfp reset
    def get_reset(self, index):
        path = TRANSCEIVER_DEVICES_PATH + '/eth' + str(index)
        reset_path = "".join([path, "/reset"])

        try:
            with open(reset_path,"rb") as f:
                reset = f.read()
                if 'NA' in reset or len(reset) == 0:
                    return False
                return False if int(reset) == 0 else True
        except:
            return False

    def set_reset(self, index, enable):
        path = TRANSCEIVER_DEVICES_PATH + '/eth' + str(index)
        reset_path = "".join([path, "/reset"])
        try:
            if os.path.exists(reset_path):
                with open(reset_path, "w") as f:
                    f.write(str(enable))
                    return True
            else:
                return False
        except:
            print("Write reset failed")
            return False

    # 获取光模块sfp tx fault
    def get_tx_fault(self, index):
        path = TRANSCEIVER_DEVICES_PATH + '/eth' + str(index)
        tx_fault_path = "".join([path, "/tx_fault"])

        try:
            with open(tx_fault_path,"rb") as f:
                tx_fault = f.read()
                if 'NA' in tx_fault or len(tx_fault) == 0:
                    return False
                return False if int(tx_fault) == 0 else True
        except:
            return False

    def set_tx_fault(self, index, enable):
        path = TRANSCEIVER_DEVICES_PATH + '/eth' + str(index)
        tx_fault_path = "".join([path, "/tx_fault"])
        try:
            if os.path.exists(tx_fault_path):
                with open(tx_fault_path, "w") as f:
                    f.write(str(enable))
                    return True
            else:
                return False
        except:
            print("Write tx_fault failed")
            return False
        
    # 获取光模块sfp rx los
    def get_rx_los(self, index):
        path = TRANSCEIVER_DEVICES_PATH + '/eth' + str(index)
        rx_los_path = "".join([path, "/rx_los"])

        try:
            with open(rx_los_path,"rb") as f:
                rx_los = f.read()
                if 'NA' in rx_los or len(rx_los) == 0:
                    return False
                return False if int(rx_los) == 0 else True
        except:
            return False

    def set_rx_los(self, index, enable):
        path = TRANSCEIVER_DEVICES_PATH + '/eth' + str(index)
        rx_los_path = "".join([path, "/rx_los"])
        try:
            if os.path.exists(rx_los_path):
                with open(rx_los_path, "w") as f:
                    f.write(str(enable))
                    return True
            else:
                return False
        except:
            print("Write rx_los failed")
            return False

    # 获取光模块sfp tx disable
    def get_tx_disable(self, index):
        path = TRANSCEIVER_DEVICES_PATH + '/eth' + str(index)
        tx_disable_path = "".join([path, "/tx_disable"])

        try:
            with open(tx_disable_path,"rb") as f:
                tx_disable = f.read()
                if 'NA' in tx_disable or len(tx_disable) == 0:
                    return False
                return False if int(tx_disable) == 0 else True
        except:
            return False

    def set_tx_disable(self, index, enable):
        path = TRANSCEIVER_DEVICES_PATH + '/eth' + str(index)
        tx_disable_path = "".join([path, "/tx_disable"])
        try:
            if os.path.exists(tx_disable_path):
                with open(tx_disable_path, "w") as f:
                    f.write(str(enable))
                    return True
            else:
                return False
        except:
            print("Write tx_disable failed")
            return False
    
    def get_power_en(self, index):
        path = TRANSCEIVER_DEVICES_PATH + '/eth' + str(index)
        power_en_path = "".join([path, "/power_on"])

        try:
            with open(power_en_path,"rb") as f:
                power_en = f.read()
                if 'NA' in power_en or len(power_en) == 0:
                    return False
                return False if int(power_en) == 0 else True
        except:
            return False

    def set_power_en(self, index, power_on_en):
        path = TRANSCEIVER_DEVICES_PATH + '/eth' + str(index)
        overwrite_en_path = "".join([path, "/overwrite_en"])
        power_en_path = "".join([path, "/power_on"])
        try:
            if os.path.exists(overwrite_en_path):
                with open(overwrite_en_path, "w") as f:
                    f.write(str(1))
            if os.path.exists(power_en_path):
                with open(power_en_path, "w") as f:
                    f.write(str(power_on_en))
                    return True
        except:
            print("Write power_on failed")
            return False

    def check_transceiver_eeprom(self, path):
        eeprom_path = "".join([path, "/eeprom"])
        try:
            with open(eeprom_path,"rb") as f:
                f.seek(0)
                data = f.read(256)
                return True
        except:
            return False

    def get_eeprom_raw(self, index):        
        path = TRANSCEIVER_DEVICES_PATH + '/eth' + str(index)
        print("PATH:",path)
        # test 5 rounds
        for i in range(0, 5):
            try:
                ret = self.check_transceiver_eeprom(path)
                if ret == False:
                    return None
                else:
                    return ret
            except:
                return None
