from tabulate import tabulate
from test_case import TestCaseCommon
from errcode import E
from function import run_command
import sys

GLOBAL_VALUE = 0xA5
GLOBAL_VALUE_CLEAR = 0x5A

class I2CTC(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "i2c_tc" # this param specified the case config dirictory
        TestCaseCommon.__init__(self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file)
        self.i2c_devices = None   # default
        """ case_config.json example
        "i2c":{
            "PORT_CPLD" : {
                "bus" : 2,
                "address" : "0x30",
                "register" : "0x0F",
                "flag" : "rw"
            },
            "FCB_CPLD" : {
                "bus" : 2,
                "address" : "0x31",
                "register" : "0x0F",
                "flag" : "rw"
            }
        },
        """
        try:
            if self.platform_cfg_json and 'i2c' in self.platform_cfg_json.keys():
                self.i2c_devices = self.platform_cfg_json['i2c']
        except Exception as e:
            self.logger.log_err(str(e), True)
    
    
    def i2c_device_rw_test(self, also_print_console=False):
        '''
            I2C device read/write test
        '''
        if self.i2c_devices is None:
            self.fail_reason.append("{}: i2c devices config null".format(self.module_name))
            return E.EEXIST
        
        ret = E.OK
        header = ['Device', 'Bus', 'Address', 'Status']
        status_tbl = []
        
        self.logger.log_info( "[I2C DEVICES R/W TEST]:", also_print_console)
        devices = self.i2c_devices.keys()
        for device in devices:
            dev_detail = self.i2c_devices.get(device)
            bus = dev_detail["bus"]
            addr = dev_detail["address"]
            reg = dev_detail["register"]
            flag = dev_detail["flag"]
            status = "OK"
                        
            if flag == 'rw':
                # write i2c register
                cmd = "i2cset -f -y {0} {1} {2} {3}".format(bus, addr, reg, GLOBAL_VALUE)
                code,out = run_command(cmd)
                # read i2c register
                cmd = "i2cget -f -y {0} {1} {2}".format(bus, addr, reg)
                code, out = run_command(cmd)
                # valigate value
                if code or int(out, 16) != GLOBAL_VALUE:
                        ret = E.EIIC8003
                        err = "[{}] {} r/w test failed!".format(self.module_name, device)
                        self.fail_reason.append(err)
                        status = "Not_OK"
                
                # write i2c register, clear last writed value
                cmd = "i2cset -f -y {0} {1} {2} {3}".format(bus, addr, reg, GLOBAL_VALUE_CLEAR)
                code,out = run_command(cmd)
                # read i2c register
                cmd = "i2cget -f -y {0} {1} {2}".format(bus, addr, reg)
                code, out = run_command(cmd)
                # valigate value
                if code or int(out, 16) != GLOBAL_VALUE_CLEAR:
                        ret = E.EIIC8003
                        err = "[{}] {} r/w test failed!".format(self.module_name, device)
                        self.fail_reason.append(err)
                        status = "Not_OK"
            elif flag == 'ro':
                cmd = "i2cget -f -y {0} {1} {2}".format(bus, addr, reg)
                code, out = run_command(cmd)
                if code:
                    ret = E.EIIC8002
                    err = "[{}] {} r/w test failed!".format(self.module_name, device)
                    self.fail_reason.append(err)
                    status = "Not_OK"
            
            status_tbl.append([device, bus, addr, status])
        if len(status_tbl) > 0:
            self.logger.log_info(tabulate(status_tbl, header, tablefmt="simple"), also_print_console)
        
        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        
        return ret
    
    
    def run_test(self, *argv):
        """
            test case main process
        """
        #fix
        #loop = sys.argv[0]
        loop = 1
        pass_cnt = 0
        fail_cnt = 0
        for i in range(1, loop + 1):
            ret = self.i2c_device_rw_test(True)
            if ret != E.OK:
                self.logger.log_err("i2c r/w loop {} failed!".format(i), True)
                fail_cnt += 1
                break
            else:
                pass_cnt += 1
        
        result = E.OK if fail_cnt == 0  else E.EFAIL
        return [pass_cnt, fail_cnt, result]
