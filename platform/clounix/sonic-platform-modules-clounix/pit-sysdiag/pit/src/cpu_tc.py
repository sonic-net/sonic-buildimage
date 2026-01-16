from function import run_command
from test_case import TestCaseCommon
from errcode import E


# CPU test class
class CPUTC(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "cpu_tc"
        TestCaseCommon.__init__(self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file)
        self.cpu_info_dict = None
        try:
            if self.platform_cfg_json and 'cpu_info' in self.platform_cfg_json.keys():
                self.cpu_info_dict = self.platform_cfg_json['cpu_info']
        except Exception as e:
            self.logger.log_err(str(e), True)

    def test_cpu_info(self, also_print_console=False):
        ret = E.OK
        self.logger.log_info("check_cpu_info start", also_print_console)

        cmd = "lscpu | head -n25"
        status, log = run_command(cmd)
        if status != 0 or len(log) <= 0:
            reason = "Failed, get cpu info failed, command {}, status {}, log {}".format( \
                     cmd, status, log)
            self.log_reason(reason)
            ret = E.ECPU3005
        else:
            lines = log.splitlines()
            expected_cpu_model = self.cpu_info_dict.get('model_name')
            expected_bogomips = self.cpu_info_dict.get('BogoMIPS')
            expected_cpu_num = self.cpu_info_dict.get('CPU(s)')
            expected_cpu_mhz = self.cpu_info_dict.get('CPU MHz')
            for line in lines:
                cols = line.split(":")
                if len(cols) < 2:
                    continue

                if expected_cpu_model and cols[0].strip() == "Model name":
                    if cols[1].strip() != expected_cpu_model:
                        reason = "Failed, CPU model name {}(expected {})".format( \
                                 cols[0], expected_cpu_model)
                        self.fail_reason.append(reason)
                        ret = E.ECPU3001

                if expected_bogomips and cols[0].strip() == 'BogoMIPS':
                    read_bogomips = float(cols[1].strip())
                    conf_bogomips = float(expected_bogomips)
                    if read_bogomips <= (conf_bogomips * 0.99) or \
                        read_bogomips >= conf_bogomips * 1.01:
                        reason = "Failed, BogoMIPS {}(expected {})".format( \
                                 cols[0], expected_bogomips)
                        self.fail_reason.append(reason)
                        ret = E.ECPU3001

                if expected_cpu_num and cols[0].strip() == 'CPU(s)':
                    if cols[1].strip() != self.cpu_info_dict.get('CPU(s)'):
                        reason = "Failed, CPU number {}(expected {})".format( \
                                 cols[0], expected_cpu_num)
                        self.fail_reason.append(reason)
                        ret = E.ECPU3001

                if expected_cpu_mhz and cols[0].strip() == 'CPU MHz':
                    read_cpu_mhz = float(cols[1].strip())
                    conf_cpu_mhz = float(expected_cpu_mhz)
                    if read_cpu_mhz <= (conf_cpu_mhz * 0.99) or \
                        read_cpu_mhz >= (conf_cpu_mhz * 1.01):
                        reason = "Failed, CPU MHz {}(expected {})".format( \
                                 cols[0], expected_cpu_mhz)
                        self.fail_reason.append(reason)
                        ret = E.ECPU3001

        if ret != E.OK:
            self.logger.log_err("test_cpu_info done, FAILED", also_print_console)
        else:
            self.logger.log_info("test_cpu_info done, PASS", also_print_console)

        return ret

    def run_test(self, *argv):
        pass_cnt = 0
        fail_cnt = 0
        ret = self.test_cpu_info(True)
        if ret != E.OK:
            fail_cnt += 1
        else:
            pass_cnt += 1

        result = E.OK if fail_cnt == 0  else E.EFAIL
        return [pass_cnt, fail_cnt, result]
