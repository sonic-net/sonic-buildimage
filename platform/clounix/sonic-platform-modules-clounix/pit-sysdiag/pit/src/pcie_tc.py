import os
import re
from tabulate import tabulate
from test_case import TestCaseCommon
from errcode import *
from function import run_command
import sys

GLOBAL_VALUE1 = '0x5A'
GLOBAL_VALUE2 = '0xA5'

# pcie bus and devices test class
class PCIETC(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "pcie_tc"
        TestCaseCommon.__init__(self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file)
        self.pcie_devices = None         # default
        self.fpga_devices = None         # default
        """ case_config.json example
        "pcie":{
            "I210": {
                "width":"x1",
                "speed": "2.5GT/s",
                "num":1
                },
            "b870":{
                "width":"x4",
                "speed":"8GT/s",
                "num":1
                },
            "X552":{
                "width":"x1",
                "speed":"2.5GT/s",
                "num":2
                }
        },
        "fpga":{
            "FPGA":{
                "path":"/sys/bus/platform/devices/switchboard_fpga/FPGA",
                "register":"scratch"
            }
        },
        """
        try:
            if self.platform_cfg_json and 'pcie' in self.platform_cfg_json.keys():
                self.pcie_devices = self.platform_cfg_json['pcie']
            if self.platform_cfg_json and 'fpga' in self.platform_cfg_json.keys():
                self.fpga_devices = self.platform_cfg_json['fpga']
        except Exception as e:
            self.logger.log_err(str(e), True)
    
    
    def _get_bus_dev_func(self, device):
        bus_dev_func = []
        ret, out = run_command("lspci | grep {}".format(device))
        for line in out.splitlines():
            if re.search("[0-9a-f]{2}\:[0-9a-f]{2}\.[0-9a-f]{1}", line):
                bus_dev_func.append(re.findall("[0-9a-f]{2}\:[0-9a-f]{2}\.[0-9a-f]{1}", line)[0])
            else:
                bus_dev_func.append(None)
        
        return bus_dev_func
    
    
    def _get_device_conf(self, busid):
        ret, output = run_command("lspci -s {} -vvv | grep -i LnkSta | grep Speed".format(busid))
        if ret or output == '':
            return '', ''
        else:
            speed = output.strip(" \t\n").split(',')[0].split('Speed')[1].strip()
            width = output.strip(" \t\n").split(',')[1].split('Width')[1].strip()
            print(speed, width)
            return speed, width
    
    
    def check_pcie_device(self, also_print_console=False):
        self.logger.log_info( "[PCIE DEVICE CHECK]:", also_print_console)
        header = ['Device', 'Bus', 'Width', 'Speed']
        ret = E.OK
        status_tbl = []
        
        if self.pcie_devices:
            for device in self.pcie_devices.keys():
                conf_width = self.pcie_devices[device]["width"]
                conf_speed = self.pcie_devices[device]["speed"]
                dev_num = int(self.pcie_devices[device]["num"])
                
                busid_list = self._get_bus_dev_func(device)
                if not busid_list:
                    self.fail_reason.append("{} not found".format(device))
                    ret = E.EPCI9001
                    status_tbl.append([device, 'None', 'None', 'None'])
                else:
                    if len(busid_list) != dev_num:
                        self.logger.log_err("{} number expect {}, real {}".format(device, dev_num, len(busid_list)))
                        self.fail_reason.append("{} number mismatch".format(device))
                        ret = E.EPCI9001
                    
                    for busid in busid_list:
                        speed, width = self._get_device_conf(busid)
                        if conf_width not in width:
                            self.fail_reason.append("{} width not matched".format(device))
                            ret = E.EPCI9005
                        if conf_speed not in speed:
                            self.fail_reason.append("{} speed not matched".format(device))
                            ret = E.EPCI9004
                        status_tbl.append([device, busid, width, speed])
        
        else:
            ret = E.EEXIST
            self.fail_reason.append("pcie config NULL")
        
        if len(status_tbl) > 0:
            self.logger.log_info(tabulate(status_tbl, header, tablefmt="simple"), also_print_console)
        
        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        
        return ret
    
    
    def fpga_device_rw_test(self, also_print_console=False):
        self.logger.log_info( "[FPGA RW TEST]:", also_print_console)
        ret = E.OK
        
        if self.fpga_devices:
            for device in self.fpga_devices.keys():
                try:
                    device_conf = self.fpga_devices.get(device)
                    driver_path = device_conf['path']
                    reg_file = os.path.join(driver_path, device_conf['register'])
                    
                    with open(reg_file, 'w') as f:
                        f.write(GLOBAL_VALUE1)
                    with open(reg_file, 'r') as f:
                        value = f.read().strip()
                    self.logger.log_info("{} write {}, read {}".format(device, GLOBAL_VALUE1, value), also_print_console)
                    
                    if int(value, 16) != int(GLOBAL_VALUE1, 16):
                        ret = E.EPCI9003
                        self.fail_reason.append("{} rw fail".format(device))
                    
                    with open(reg_file, 'w') as f:
                        f.write(GLOBAL_VALUE2)
                    with open(reg_file, 'r') as f:
                        value = f.read().strip()
                    self.logger.log_info("{} write {}, read {}".format(device, GLOBAL_VALUE2, value), also_print_console)
                    
                    if int(value, 16) != int(GLOBAL_VALUE2, 16):
                        ret = E.EPCI9003
                        self.fail_reason.append("{} rw fail".format(device))
                    
                except Exception as e:
                    self.logger.log_err(str(e), True)
                    self.fail_reason.append(str(e))
                    ret = E.EPCI9002
                        
        else:
            ret = E.EEXIST
            self.fail_reason.append("fpga config NULL")
        
        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        
        return ret
    
    
    def run_test(self, *argv):
        fail_cnt = 0
        pass_cnt = 0
        ret = self.check_pcie_device(True)
        if ret != E.OK:
            fail_cnt += 1
        else:
            pass_cnt += 1

        
        # TODO Need FPGA
        # loop = sys.argv[0]
        # loop = 1
        # for i in range(1, loop + 1):
        #     ret = self.fpga_device_rw_test(True)
        #     if ret != E.OK:
        #         self.logger.log_err("pcie r/w loop {} failed!".format(i), True)
        #         return ret

        result = E.OK if fail_cnt == 0  else E.EFAIL
        return [pass_cnt, fail_cnt, result]
