import sys
from tabulate import tabulate
from test_case import TestCaseCommon
from errcode import E
from function import load_platform_util_module

# sfp test class
class SFPTC(TestCaseCommon):
    __PLATFORM_SPECIFIC_MODULE_NAME = "sfputil"
    __PLATFORM_SPECIFIC_CLASS_NAME = "SfpUtil"
    
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "sfp_tc"
        TestCaseCommon.__init__(self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file)
        
        self.sfp_util = None
        self.sfp_port_list = {}
        self.qsfp_port_list = {}

        sfp_module = load_platform_util_module(self.__PLATFORM_SPECIFIC_MODULE_NAME)
        try:
            platform_util_class = getattr(sfp_module, self.__PLATFORM_SPECIFIC_CLASS_NAME)
            self.sfp_util = platform_util_class()
        except AttributeError as e:
            self.logger.log_err(str(e), True)
            sys.exit(1)

        self.sfp_port_list = self.sfp_util.sfp_port_list
        self.qsfp_port_list = self.sfp_util.qsfp_ports_list


    def get_sfp_status(self, also_print_console=False):
        ret = E.OK
        sfp_header = ['PORT', 'PRESENT', 'TX_DIS', 'RX_LOS', 'TX_FAULT']
        status_tbl = []
        ports = self.sfp_port_list

        self.logger.log_info("[SFP PIN CHECK]:", also_print_console)
        for index, port in ports.items():
            present = self.sfp_util.get_presence(index)
            tx_dis = self.sfp_util.get_tx_disable(index)
            rx_los = self.sfp_util.get_rx_los(index)
            tx_fault = self.sfp_util.get_tx_fault(index)
            
            if not present:
                self.fail_reason.append("sfp {} absent".format(port))

            if tx_dis:
                ret = E.ESFP18004
                self.fail_reason.append("sfp {} tx_dis happened".format(port))
            
            if rx_los:
                ret = E.ESFP18003
                self.fail_reason.append("sfp {} rx_los happened".format(port))
            
            if tx_fault:
                ret = E.ESFP18002
                self.fail_reason.append("sfp {} tx_fault happened".format(port))
            
            status_tbl.append(["Port"+str(port), present, tx_dis, rx_los, tx_fault])
        
        if len(status_tbl) > 0:
            self.logger.log_info(tabulate(status_tbl, sfp_header, tablefmt="simple"), also_print_console)
        
        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        
        return ret
    
    
    def get_qsfp_status(self, also_print_console=False):
        ret = E.OK
        qsfp_header = ['PORT', 'PRESENT', 'LPMODE', 'RESET', 'INTL']
        status_tbl = []
        ports = self.qsfp_port_list
        self.logger.log_info("[QSFP PIN CHECK]:", also_print_console)
        for index, port in ports.items():
            present = self.sfp_util.get_presence(index)
            lpmode = self.sfp_util.get_low_power_mode(index)
            reset = self.sfp_util.get_reset(index)
            intl = self.sfp_util.get_interrupt(index)
            
            if not present:
                self.fail_reason.append("qsfp {} absent".format(port))

            if intl:
                ret = E.ESFP18014
                self.fail_reason.append("qsfp {} under interrupt".format(port))
            if reset:
                ret = E.ESFP18006
                self.fail_reason.append("qsfp {} under reset".format(port))
            if lpmode:
                ret = E.ESFP18005
                self.fail_reason.append("qsfp {} under lpmode".format(port))
            
            status_tbl.append(["Port"+str(port), present, lpmode, reset, intl])
        
        if len(status_tbl) > 0:
            self.logger.log_info(tabulate(status_tbl, qsfp_header, tablefmt="simple"), also_print_console)
        
        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        return ret
    
    
    def read_sff_eeprom(self, also_print_console=False):
        ret = E.OK
        ports = self.sfp_port_list.copy()
        ports.update(self.qsfp_port_list)

        self.logger.log_info("[SFF EEPROM READ CHECK]:", also_print_console)
        for index, port in ports.items():
            try:
                if not self.sfp_util.get_presence(index):
                    ret = E.ESFP18001
                    self.fail_reason.append("port{} absent".format(port))
                    self.logger.log_err("Port{}: Fail".format(port), also_print_console)
                    continue
                
                sff_eeprom = self.sfp_util.get_eeprom_raw(index)
                if sff_eeprom is None:
                    ret = E.ESFP18008
                    err = "read port{} sff eeprom failed".format(port)
                    self.fail_reason.append(err)
                    self.logger.log_err("Port{}: Fail".format(port), also_print_console)
                else:
                    self.logger.log_info("Port{}: Pass".format(port), also_print_console)
            except Exception as e:
                ret = E.ESFP18008
                self.fail_reason.append(str(e))
                self.logger.log_err("Port{}: Fail".format(port), also_print_console)
        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        return ret

    def sff_attr_read_write(self, also_print_console=False):
        ret = E.OK

        self.logger.log_info("[SFF ATTR NODES WRITE-READ CHECK]:", also_print_console)
        for index, port in self.sfp_port_list.items():
            if self.sfp_util.set_tx_disable(index, 1):
                tx_dis = self.sfp_util.get_tx_disable(index)
                if tx_dis != True:
                    ret = E.ESFP18007
                    self.fail_reason.append("sfp {} tx_dis set failed".format(port))
                self.sfp_util.set_tx_disable(index, 0)
        
            if self.sfp_util.set_rx_los(index, 1):
                rx_los = self.sfp_util.get_rx_los(index)
                if rx_los != True:
                    ret = E.ESFP18009
                    self.fail_reason.append("sfp {} rx_los set failed".format(port))
                self.sfp_util.set_rx_los(index, 0)

            if self.sfp_util.set_tx_fault(index, 1):
                tx_fault = self.sfp_util.get_tx_fault(index)
                if tx_fault != True:
                    ret = E.ESFP18010
                    self.fail_reason.append("sfp {} tx_fault set failed".format(port))
                self.sfp_util.set_tx_fault(index, 0)

        for index, port in self.qsfp_port_list.items():
            if self.sfp_util.set_low_power_mode(index, 1):
                lpmode = self.sfp_util.get_low_power_mode(index)
                if lpmode != True:
                    ret = E.ESFP18011
                    self.fail_reason.append("qsfp {} low power mode set failed".format(port))
                self.sfp_util.set_low_power_mode(index, 0)

            if self.sfp_util.set_reset(index, 1):
                reset = self.sfp_util.get_reset(index)
                if reset != True:
                    ret = E.ESFP18012
                    self.fail_reason.append("qsfp {} reset set failed".format(port))
                self.sfp_util.set_reset(index, 0)

            if self.sfp_util.set_power_en(index, 0):
                power_enable = self.sfp_util.get_power_en(index)
                if power_enable != False:
                    ret = E.ESFP18013
                    self.fail_reason.append("qsfp {} power enable set failed".format(port))                
                self.sfp_util.set_power_en(index, 1)

        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        return ret

    def run_test(self, *argv):
        fail_cnt = 0
        pass_cnt = 0
        ret = self.get_sfp_status(True)
        if ret != E.OK:
            fail_cnt += 1
        else:
            pass_cnt += 1
        
        ret = self.get_qsfp_status(True)
        if ret != E.OK:
            fail_cnt += 1
        else:
            pass_cnt += 1
        
        ret = self.read_sff_eeprom(True)
        if ret != E.OK:
            fail_cnt += 1
        else:
            pass_cnt += 1

        ret = self.sff_attr_read_write(True)
        if ret != E.OK:
            fail_cnt += 1
        else:
            pass_cnt += 1

        result = E.OK if fail_cnt == 0  else E.EFAIL
        return [pass_cnt, fail_cnt, result]
