from collections import OrderedDict
import datetime
import json
from time import sleep
import time
from test_case import TestCaseCommon
from errcode import E
from function import exec_cmd, run_command

TIME_DELAY = 300
LOOP_COUNT_SHUT_START = 100
LOOP_COUNT_READ_WRITE_EEPROM = 100
LOOP_COUNT_READ_WRITE_EEPROM_RETRY = 1
TEST_MODE_INIT_DEINIT = 'init_deinit'
TEST_MODE_READ_WRITE_EEPROM = 'read_write_eeprom'

class SFPSTRESSTC(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "sfp_stress_tc"
        TestCaseCommon.__init__(self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file)

        self.init_deinit_port_list = []
        self.read_write_eeprom_index_list = []
        self.port_to_index_map = {}

        self.result = {
            TEST_MODE_INIT_DEINIT: {},
            TEST_MODE_READ_WRITE_EEPROM: {}
        }
    
    def update_result(self, mode, port, _type, reason):
        self.result[mode][port][_type] += 1
        if _type == "fail":
            self.result[mode][port]['reason'] = reason

    def wait_sdk_admin_status_change(self,port, wait_status, timeout):
        end_time = time.time() + timeout
        while time.time() < end_time:
            output = exec_cmd("clx_shell port show cfg portlist={}".format(port[8:]))
            admin_status = [line.strip().split()[3] for line in output.split("\n") if line.startswith(port[8:])][0]
            if admin_status == wait_status:
                return True 
            time.sleep(2)
        return False

    def sfp_init_deinit_stress_test(self, also_print_console=False):
        count = 0

        self.logger.log_info("current time: {}".format(str(datetime.datetime.now())), also_print_console)

        output = exec_cmd("sfputil show presence")
        for index, line in enumerate(output.split("\n")):
            if "Present" in line:
                port = line.split(" ")[0]
                oper_status = exec_cmd("sonic-db-cli APPL_DB HGET PORT_TABLE:{} oper_status".format(port))
                if oper_status.strip() == 'down':
                    continue
                self.init_deinit_port_list.append(port)
                self.result[TEST_MODE_INIT_DEINIT].update({
                            port: {
                                'succ': 0, 
                                'fail': 0, 
                                'reason': ''
                            }})

        self.logger.log_info("test port num: {}".format(len(self.init_deinit_port_list)), also_print_console)

        for port in self.init_deinit_port_list:
            count = 0
            while count < LOOP_COUNT_SHUT_START:
                count += 1
                exec_cmd("config interface shutdown {}".format(port))
                if not self.wait_sdk_admin_status_change(port, "dis", TIME_DELAY):
                    self.update_result(TEST_MODE_INIT_DEINIT, port, "fail", "shutdown port timeout")
                    continue

                exec_cmd("config interface startup {}".format(port))
                if not self.wait_sdk_admin_status_change(port, "en", TIME_DELAY):
                    self.update_result(TEST_MODE_INIT_DEINIT, port, "fail", "startup port timeout")
                    continue
                
                succ_flag = False
                end_time = time.time() + TIME_DELAY
                while time.time() < end_time:
                    oper_status = exec_cmd("sonic-db-cli APPL_DB HGET PORT_TABLE:{} oper_status".format(port))
                    admin_status = exec_cmd("sonic-db-cli APPL_DB HGET PORT_TABLE:{} admin_status".format(port))

                    if oper_status.strip() == "up" and admin_status.strip() == "up":
                        self.update_result(TEST_MODE_INIT_DEINIT, port, "succ", "")
                        succ_flag = True
                        break
                    sleep(2)
            
                if not succ_flag:
                    self.update_result(TEST_MODE_INIT_DEINIT, port, "fail", "oper status is not up")
            
            self.logger.log_info("{}: succ {}, fail {}".format(
                port,
                self.result[TEST_MODE_INIT_DEINIT][port]['succ'],
                self.result[TEST_MODE_INIT_DEINIT][port]['fail']), also_print_console)
            
            if self.result[TEST_MODE_INIT_DEINIT][port]['succ'] != LOOP_COUNT_SHUT_START:
                self.fail_reason.append("{} : {}".format(port, self.result[TEST_MODE_INIT_DEINIT][port]['reason']))

        self.logger.log_info("current time: {}".format(str(datetime.datetime.now())), also_print_console)

    def read_eeprom(self, path, offset, num_bytes):
        try:
            with open(path, "rb", buffering=0) as f:
                f.seek(offset)
                return bytearray(f.read(num_bytes))
        except (OSError, IOError):
            return None
        
    def write_eeprom(self, path, offset, num_bytes, data):
        try:
            with open(path, "r+b", buffering=0) as f:
                f.seek(offset)
                f.write(bytearray(data[0:num_bytes]))
        except (OSError, IOError):
            return False
        return True

    def sfp_read_eeprom_stress_test(self, also_print_console=False):
        count = 0
        sfp_id_list = []
        
        self.result[TEST_MODE_READ_WRITE_EEPROM].update({
            'start_time': str(datetime.datetime.now()),
            'finish_time': str(datetime.datetime.now())
        })  

        sysfs_path = "/sys_switch/transceiver/eth{}/eeprom"

        with open("/sys_switch/transceiver/number", "r") as f:
            port_num = int(f.read().strip())
            if port_num <= 0 or port_num > 65:
                self.logger.log_err("port number is out of range", True)
                return
        
        self.logger.log_info("current time: {}".format(str(datetime.datetime.now())), also_print_console)

        for index in range(1, port_num + 1):
            eeprom_path = sysfs_path.format(index)
            data = self.read_eeprom(eeprom_path, 0, 1)
            if data:
                self.read_write_eeprom_index_list.append(index)
                sfp_id_list.append(data[0])
                self.result[TEST_MODE_READ_WRITE_EEPROM].update({
                    index: {
                        'succ': 0,
                        'fail': 0,
                        'reason': ''
                    }
                })
        
        read_page_list = [0x00, 0x01, 0x02, 0x10, 0x11, 0x13, 0x14]
        write_eeprom_offset = 0x10 * 128 + 213 #Lane-Specific Masks in CMIS
        write_eeprom_length = 20
        write_eeprom_data = bytearray([0xff] * write_eeprom_length)

        for index, port in enumerate(self.read_write_eeprom_index_list):
            count = 0
            eeprom_path = sysfs_path.format(port)
            while count < LOOP_COUNT_READ_WRITE_EEPROM:
                count += 1
                # read eeprom
                pass_flag = True
                loop_count = LOOP_COUNT_READ_WRITE_EEPROM_RETRY
                while loop_count > 0:
                    status, output = run_command("hexdump -C {}".format(eeprom_path))
                    if status != 0:
                        pass_flag = False
                        break
                    loop_count -= 1
                
                if not pass_flag:
                    self.update_result(TEST_MODE_READ_WRITE_EEPROM, port, "fail", "eth{} hexdump -C eeprom failed".format(port))
                    continue
                
                # write eeprom  
                if sfp_id_list[index] == 0x18 or sfp_id_list[index] == 0x19 or sfp_id_list[index] == 0x1b or sfp_id_list[index] == 0x1e:
                    pass
                else:
                    self.update_result(TEST_MODE_READ_WRITE_EEPROM, port, "succ", "")
                    continue

                pass_flag = True
                loop_count = LOOP_COUNT_READ_WRITE_EEPROM_RETRY
                original_eeprom_data = self.read_eeprom(eeprom_path, write_eeprom_offset, write_eeprom_length)
                if original_eeprom_data is None:
                    self.update_result(TEST_MODE_READ_WRITE_EEPROM, port, "fail", "read eeprom failed")
                    continue

                while loop_count > 0:
                    write_flag = self.write_eeprom(eeprom_path, write_eeprom_offset, write_eeprom_length, write_eeprom_data)
                    if not write_flag:
                        self.update_result(TEST_MODE_READ_WRITE_EEPROM, port, "fail", "write eeprom failed")
                        pass_flag = False
                        break
                    loop_count -= 1
                
                write_flag = self.write_eeprom(eeprom_path, write_eeprom_offset, write_eeprom_length, original_eeprom_data)
                if not write_flag:
                    self.update_result(TEST_MODE_READ_WRITE_EEPROM, port, "fail", "write eeprom failed")
                    continue

                if not pass_flag:
                    continue
                
                self.update_result(TEST_MODE_READ_WRITE_EEPROM, port, "succ", "")

            self.logger.log_info("eth{}: succ {}, fail {} {}".format(
                port,
                self.result[TEST_MODE_READ_WRITE_EEPROM][port]['succ'],
                self.result[TEST_MODE_READ_WRITE_EEPROM][port]['fail'],
                self.result[TEST_MODE_READ_WRITE_EEPROM][port]['reason']), also_print_console)
        
        self.logger.log_info("current time: {}".format(str(datetime.datetime.now())), also_print_console)

    def run_test(self, *argv):
        
        pass_cnt = 2
        fail_cnt = 0
        self.logger.log_info("[SFP INIT DEINIT STRESS TEST]:", True)
        self.sfp_init_deinit_stress_test(True)
        for port in self.init_deinit_port_list:
            if self.result[TEST_MODE_INIT_DEINIT][port]['succ'] != LOOP_COUNT_SHUT_START:
                fail_cnt += 1
                pass_cnt -= 1
                break
        
        self.logger.log_info("{}".format("PASS" if pass_cnt == 2 else "FAIL" ))

        self.logger.log_info("[SFP READ WRITE EEPROM STRESS TEST]:", True)
        self.sfp_read_eeprom_stress_test(True)
        for index in self.read_write_eeprom_index_list:
            if self.result[TEST_MODE_READ_WRITE_EEPROM][index]['succ'] != LOOP_COUNT_READ_WRITE_EEPROM:
                fail_cnt += 1
                pass_cnt -= 1
                break
        
        return [pass_cnt, fail_cnt, E.OK if fail_cnt == 0 else E.EFAIL]
        