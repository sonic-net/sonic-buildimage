from function import run_command
from test_case import TestCaseCommon
from errcode import E

X86_ARCH_LIST = ["x86", "x86_64", "amd", "amd64"]
ARM_ARCH_LIST = ["arm", "arm64"]

# memory test class
class MEMORYTC(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "memory_tc"
        TestCaseCommon.__init__(self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file)
        self.arch = "x86"            # default arch
        self.memory_bom_list = None  # default conf
        self.free_mem_size = 100     # free memory size in kB. if free mem is less than free_mem_size, fail.
        try:
            if self.platform_cfg_json and 'memory_bom' in self.platform_cfg_json.keys():
                self.memory_bom_list = self.platform_cfg_json['memory_bom']
            if self.platform_cfg_json and 'arch' in self.platform_cfg_json.keys():
                self.arch = self.platform_cfg_json['arch']
            if self.platform_cfg_json and 'memory_free_size' in self.platform_cfg_json.keys():
                self.free_mem_size = int(self.platform_cfg_json['memory_free_size'])
        except Exception as e:
            self.logger.log_err(str(e), True)


    def _memory_info_check_by_dmidecode(self, also_print_console=True):
        ret = E.OK
        pn_list = []
        vendor_list = []
        
        status, out = run_command("dmidecode -t 17")
        if status != 0 or len(out) <= 0:
            self.fail_reason.append("dmidecode exec failed.")
            ret = E.EMEM4001
        else:
            for item in out.splitlines():
                self.logger.log_info(item, also_print_console)
                if ":" in item:
                    key = item.split(":")[0].strip()
                    value = item.split(":")[1].strip()
                    #self.logger.log_info("%-30s: %s" % (key, value), True)
                    if key == 'Part Number' and value != 'NO DIMM':
                        pn_list.append(value)
                    if key == 'Manufacturer' and value != 'NO DIMM':
                        vendor_list.append(value)
            
            # memory bom check
            if self.memory_bom_list:
                memory_matched = False
                for memory_bom in self.memory_bom_list:
                    if memory_bom["manufacturer"] in vendor_list and memory_bom["pn"] in pn_list:
                        memory_matched = True
                        break
                if not memory_matched:
                    ret = E.EMEM4001
                    self.fail_reason.append("memory not matched")
        
        return ret
    
    
    def _arm_memory_ecc_check(self, also_print_console=True):
        return E.OK
    
    
    def _x86_memory_ecc_check(self, also_print_console=True):
        status, out = run_command("edac-util -v")
        self.logger.log_info(out, also_print_console)
        if status:
            self.fail_reason.append("memort ecc occured")
            return E.EMEM4003
        else:
            return E.OK

    
    def memory_info_check(self, also_print_console=True):
        self.logger.log_info("[CHECK MEMORY INFO]:", also_print_console)

        if self.arch in X86_ARCH_LIST:
            ret = self._memory_info_check_by_dmidecode()
        elif self.arch in ARM_ARCH_LIST:
            ret = E.OK
        
        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
            
        return ret
    
    
    def test_memory_capacity(self, also_print_console=True):
        self.logger.log_info("[CHECK MEMORY CAPACITY]:", also_print_console)
        ret = E.OK
        status, out = run_command("free -t")
        self.logger.log_info(out, also_print_console)
        if status:
            self.fail_reason.append("free exec failed")
            ret = E.EIO
        else:
            for line in out.splitlines():
                if line.find("Total") >= 0:
                    free_mem = line.split()[3]
                    if int(free_mem) < self.free_mem_size:
                        self.fail_reason.append("free memory less than {}kB".format(self.free_mem_size))
                        ret = E.EMEM4004
        
        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        
        return ret


    def test_memory_by_memtester(self, also_print_console=True):
        self.logger.log_info("[MEMORY MEMTESTER]:", True)
        
        status, out = run_command("memtester 1M 1")
        self.logger.log_info(out, also_print_console)
        if status:
            self.fail_reason.append("memtester failed")
            ret = E.EMEM4002
        else:
            ret = E.OK
        
        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        return ret


    def memory_ecc_check(self, also_print_console=True):
        self.logger.log_info("[CHECK MEMORY ECC]:", True)
        
        if self.arch in X86_ARCH_LIST:
            ret = self._x86_memory_ecc_check()
        elif self.arch in ARM_ARCH_LIST:
            ret = self._arm_memory_ecc_check()
        
        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        
        return ret


    def run_test(self, *argv):
        pass_cnt = 0
        fail_cnt = 0
        ret = self.memory_info_check()
        if ret != E.OK:
            fail_cnt += 1
        else:
            pass_cnt += 1
        ret = self.test_memory_capacity()
        if ret != E.OK:
            fail_cnt += 1
        else:
            pass_cnt += 1
        ret = self.test_memory_by_memtester()
        if ret != E.OK:
            fail_cnt += 1
        else:
            pass_cnt += 1
        ret = self.memory_ecc_check()
        if ret != E.OK:
            fail_cnt += 1
        else:
            pass_cnt += 1

        result = E.OK if fail_cnt == 0  else E.EFAIL
        return [pass_cnt, fail_cnt, result]
