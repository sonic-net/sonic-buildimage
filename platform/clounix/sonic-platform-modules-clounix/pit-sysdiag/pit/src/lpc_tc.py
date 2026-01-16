import os
from tabulate import tabulate
from test_case import TestCaseCommon
from errcode import E
from function import run_command

GLOBAL_VALUE1 = 0x5A
GLOBAL_VALUE2 = 0xA5

# LPC bus and devices test class
class LPCTC(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "lpc_tc"
        TestCaseCommon.__init__(self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file)
        self.lpc_devices = None       # default
        """ case_config.json example
        "lpc":{
            "CTRL_CPLD":{
                "path":"/sys/bus/platform/devices/ctrl_cpld",
                "register":"sw_rw_test",
                "flag":"rw"
            }
        },
        """
        try:
            if self.platform_cfg_json and 'lpc' in self.platform_cfg_json.keys():
                self.lpc_devices = self.platform_cfg_json['lpc']
        except Exception as e:
            self.logger.log_err(str(e))

        
    def lpc_device_rw_test(self, also_print_console=False):
        self.logger.log_info( "[LPC DEVICE RW TEST]:", also_print_console)
        ret = E.OK
        
        if self.lpc_devices:
            for device in self.lpc_devices.keys():
                try:
                    device_conf = self.lpc_devices.get(device)
                    reg_file = os.path.join(device_conf['path'], device_conf['register'])

                    if device_conf['flag'] == "rw":
                        with open(reg_file, 'w') as f:
                            f.write(str(GLOBAL_VALUE1))
                        with open(reg_file, 'r') as f:
                            value = f.read()
                        self.logger.log_info("{} write {}, read {}".format(device, GLOBAL_VALUE1, value), also_print_console)
                        
                        if int(value) != GLOBAL_VALUE1:
                            ret = E.ELPC10003
                            self.fail_reason.append("{} rw fail".format(device))
                        
                        with open(reg_file, 'w') as f:
                            f.write(str(GLOBAL_VALUE2))
                        with open(reg_file, 'r') as f:
                            value = f.read()
                        self.logger.log_info("{} write {}, read {}".format(device, GLOBAL_VALUE2, value), also_print_console)
                        
                        if int(value) != GLOBAL_VALUE2:
                            ret = E.ELPC10003
                            self.fail_reason.append("{} rw fail".format(device))
                    
                    elif device_conf['flag'] == "ro":
                        with open(reg_file, 'r') as f:
                            value = f.read()
                        self.logger.log_info("{} read {}={}".format(device, device_conf['register'], value), also_print_console)
                    else:
                        pass

                except Exception as e:
                    self.logger.log_err(str(e))
                    self.fail_reason.append(str(e))
                    ret = E.ELPC10001
        
        else:
            ret = E.EEXIST
            self.fail_reason.append("lpc config NULL")
                
        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        
        return ret
    
    
    def run_test(self, *argv):
        loop = argv[0]
        for i in range(1, loop + 1):
            ret = self.lpc_device_rw_test(True)
            if ret != E.OK:
                self.logger.log_err("lpc r/w loop {} failed!".format(i), True)
                break
        return ret
