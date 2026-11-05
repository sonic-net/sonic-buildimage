from function import run_command
from test_case import TestCaseCommon
from errcode import E
import traceback


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

    def _memory_info_check_by_dmidecode(self):
        ret = E.OK
        pn_list = []
        vendor_list = []

        status, out = run_command("dmidecode -t 17")
        if status != 0 or len(out) <= 0:
            self.fail_reason.append("dmidecode exec failed.")
            ret = E.EMEM4001
        else:
            for item in out.splitlines():
                self.logger.log_info(item)
                if ":" in item:
                    key = item.split(":")[0].strip()
                    value = item.split(":")[1].strip()
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

    def _arm_memory_ecc_check(self):
        return E.OK

    def _x86_memory_ecc_check(self):
        status, out = run_command("edac-util -v")
        self.logger.log_info(out)
        if status:
            self.fail_reason.append("memort ecc occured")
            return E.EMEM4003
        else:
            return E.OK

    def test_memory_info(self):
        self.logger.log_info("test memory info start")

        if self.arch in X86_ARCH_LIST:
            ret = self._memory_info_check_by_dmidecode()
        elif self.arch in ARM_ARCH_LIST:
            ret = E.OK

        if ret != E.OK:
            self.logger.log_err("test memory info done, FAILED.")
        else:
            self.logger.log_err("test memory info done, PASS.")

        return ret

    def test_memory_capacity(self):
        self.logger.log_info("test memory capacity start")
        ret = E.OK
        status, out = run_command("free -t")
        self.logger.log_info(out)
        if status:
            self.log_reason("exec failed, cmd: free -t")
            ret = E.EIO
        else:
            for line in out.splitlines():
                if line.find("Total") >= 0:
                    free_mem = line.split()[3]
                    if int(free_mem) < self.free_mem_size:
                        self.log_reason("free memory less than {}kB".format(self.free_mem_size))
                        ret = E.EMEM4004
        
        if ret != E.OK:
            self.logger.log_err("test memory capacity done, FAILED.")
        else:
            self.logger.log_err("test memory capacity done, PASS.")
        
        return ret

    def test_memory_stress(self):
        self.logger.log_info("test memory stress start")
        
        ret = E.OK
        status, out = run_command("memtester 1M 1")
        self.logger.log_info(out)
        if status:
            reason = "exec failed, cmd: memtester 1M 1"
            self.log_reason(reason)
            ret = E.EMEM4002
        
        if ret != E.OK:
            self.logger.log_err("test memory stress done, FAILED!")
        else:
            self.logger.log_err("test memory stress done, PASS!")

        return ret

    def test_memory_ecc(self):
        self.logger.log_info("test memory ecc start")

        if self.arch in X86_ARCH_LIST:
            ret = self._x86_memory_ecc_check()
        elif self.arch in ARM_ARCH_LIST:
            ret = self._arm_memory_ecc_check()

        if ret != E.OK:
            self.logger.log_err("test memory stress done, FAILED.")
        else:
            self.logger.log_err("test memory stress done, PASS.")

        return ret

    def run_test(self, *argv):
        final_ret = E.OK

        try:
            ret = self.test_memory_info()
            if ret != E.OK:
                final_ret = ret
        except Exception as e:
            reason = "test memory info exception {}, FAILED".format(str(e))
            self.log_reason(reason)
            self.logger.log_err(traceback.format_exc())
            final_ret = E.EFAIL

        try:
            ret = self.test_memory_capacity()
            if ret != E.OK:
                final_ret = ret
        except Exception as e:
            reason = "test memory capacity exception {}, FAILED".format(str(e))
            self.log_reason(reason)
            self.logger.log_err(traceback.format_exc())
            final_ret = E.EFAIL

        try:
            ret = self.test_memory_stress()
            if ret != E.OK:
                final_ret = ret
        except Exception as e:
            reason = "test memory by memtester exception {}, FAILED".format(str(e))
            self.log_reason(reason)
            self.logger.log_err(traceback.format_exc())
            final_ret = E.EFAIL

        try:
            ret = self.test_memory_ecc()
            if ret != E.OK:
                final_ret = ret
        except Exception as e:
            reason = "test memory ecc exception {}, FAILED".format(str(e))
            self.log_reason(reason)
            self.logger.log_err(traceback.format_exc())
            final_ret = E.EFAIL

        return final_ret
