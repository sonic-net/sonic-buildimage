import sys
import os
from tabulate import tabulate
from test_case import TestCaseCommon
from function import load_platform_util_module, exec_cmd
from errcode import E
from pddf_check import pddf_rule

class SENSORTC(TestCaseCommon):
    __PLATFORM_SPECIFIC_MODULE_NAME = "sensorutil"
    __PLATFORM_SPECIFIC_CLASS_NAME = "SensorUtil"
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "sensor_tc"
        TestCaseCommon.__init__(self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file)
        try:
            self.pddf_rule = pddf_rule(logger)
            self.temp_sensor_num = self.pddf_rule.get_temp_sensor_num()
            self.volt_sensor_num = self.pddf_rule.get_volt_sensor_num()
            self.curr_sensor_num = self.pddf_rule.get_curr_sensor_num()
        except Exception as e:
            self.logger.log_err(str(e), True)
            sys.exit(1)

        self.sensor_util = None
        self.sensor_node = None
        self.sensor_type = None
        self.sensor_node_name = None
        self.sensor_factor = None
        self.svid_info_dict = None
        try:
            if self.platform_cfg_json and "sensor_info" in self.platform_cfg_json.keys():
                self.sensor_info_dict = self.platform_cfg_json["sensor_info"]
                self.sensor_node = self.sensor_info_dict["dcdc_sensor_node"]
                self.sensor_type = self.sensor_info_dict["dcdc_sensor_type"]
                self.sensor_node_name = self.sensor_info_dict["dcdc_sensor_node_name"]
                self.sensor_factor = self.sensor_info_dict["dcdc_sensor_factor"] if "dcdc_sensor_factor" in self.sensor_info_dict else None
            
            if self.platform_cfg_json and "svid_info" in self.platform_cfg_json.keys(): 
                self.svid_info_dict = self.platform_cfg_json["svid_info"]
        except Exception as e:
            self.logger.log_err(str(e), True)

        sensor_module = load_platform_util_module(self.__PLATFORM_SPECIFIC_MODULE_NAME)
        try:
            platform_util_class = getattr(sensor_module, self.__PLATFORM_SPECIFIC_CLASS_NAME)
            self.sensor_util = platform_util_class()
        except AttributeError as e:
            self.logger.log_err(str(e), True)
            sys.exit(1)
 
    def load_sensor_info(self):
        sensor_dict = {}
        self.sensor_util.get_sensor_node_info(self.sensor_node, self.sensor_type, self.sensor_node_name, self.sensor_factor, self.svid_info_dict)
        if self.sensor_util:
            sensor_dict = self.sensor_util.get_all()
        return sensor_dict
    
    def sensor_verify(self, sensor_dict, also_print_console=False):
        ret = E.OK
        fail_cnt = 0
        pass_cnt = 0
        elec_header = ["Sensor", 'InputName', 'Status', 'Curr', 'Voltage', 'Power']
        header = ["Sensor", 'InputName', 'Status', 'Value', 'LowThd', 'HighThd']
        elec_status_table = []
        status_table = []
        if not sensor_dict:
            fail_cnt += 1
            self.fail_reason.append("get sensors failed!")
            result = E.OK if fail_cnt == 0  else E.EFAIL
            return [pass_cnt, fail_cnt, result]

        self.logger.log_info( "[SENSORS VALUE CHECK]:", also_print_console)
        try:
            for sensor_name, sensor_obj in sensor_dict.items():
                if sensor_name == 'Number':
                    continue
                
                si_names = [k for k in sensor_obj.keys() if (('Present' != k) and sensor_obj['Present'])]
                si_names.sort()
                for si_name in si_names:
                    si = sensor_obj[si_name]
                    if 'DCDC_POWER' in sensor_name:
                        curr = float(si.get('Curr'))*0.001
                        volt = float(si.get('Voltage'))*0.001
                        power = float(si.get('Power'))*0.000001
                        curr = round(curr, 2)
                        volt = round(volt, 2)
                        power = round(power, 2)
                        power_cacl = curr * volt
                        delta = abs(power - power_cacl)
                        status = 'OK'
                        if delta > power*0.1:
                            status = 'NOT_OK'
                            self.fail_reason.append("{} out of range; curr:{}(A)  volt:{}(V) power:{}(W)".format(si_name, curr, volt, power))
                        elec_status_table.append([sensor_name, si_name, status, (str(curr)+'A'), (str(volt)+'V'), (str(power)+'W')])
                    elif 'TEMP_SENSOR' in sensor_name:
                        sval = si.get('Value')
                        slow = si.get('LowThd')
                        shigh = si.get("HighThd")
                        sunit = si.get('Unit')
                        sdesc = si.get('Description')
                        fault = False
                    
                        try:
                            sval = float(sval)
                        except:
                            sval = 0.0
                            fault = True

                        try:
                            slow = float(slow)
                        except:
                            slow = 0.0
                            fault = True

                        try:
                            shigh = float(shigh)
                        except:
                            shigh = 0.0
                            fault = True
                    
                        if sunit == None:
                            sunit = '#'
                            fault = True
                    
                        status = 'NOT_OK'
                        if fault == False and sval >= slow and sval <= shigh:
                            status = 'OK'
                        else:
                            ret = E.ESSR7003
                            self.fail_reason.append("{} out of threshold; current Value:{}  LowThd:{} HighThd:{}".format(si_name,sval,slow,shigh))
                        status_table.append([sensor_name, si_name, status, (str(sval)+sunit), (str(slow)+sunit), (str(shigh)+sunit)])
        except Exception as e:
            self.fail_reason.append(str(e))
            ret = E.ESSR7002
        
        if len(status_table) > 0:
            status_table.sort()
            self.logger.log_info(tabulate(status_table, header, tablefmt="simple"), also_print_console)
            self.logger.log_info('', also_print_console)
            self.logger.log_info(tabulate(elec_status_table, elec_header, tablefmt="simple"), also_print_console)
        
        if ret != E.OK:
            fail_cnt += 1
            self.logger.log_err("FAIL!", also_print_console)
        else:
            pass_cnt += 1
            self.logger.log_info("PASS.", also_print_console)
        
        result = E.OK if fail_cnt == 0  else E.EFAIL
        return [pass_cnt, fail_cnt, result]

    def verify_node(self):
        ret_list = [0, 0, E.OK]
        ret = self.pddf_rule.check_node('vol')
        ret_list = self.pddf_rule.update_ret_val(ret_list, ret)

        ret = self.pddf_rule.check_node('curr')
        ret_list = self.pddf_rule.update_ret_val(ret_list, ret)

        ret = self.pddf_rule.check_node('temp')
        ret_list = self.pddf_rule.update_ret_val(ret_list, ret)

        sensor_dict = self.load_sensor_info()
        ret = self.sensor_verify(sensor_dict, True)
        ret_list = self.pddf_rule.update_ret_val(ret_list, ret)
        return ret_list

    def run_test(self, *argv):
        ret = self.verify_node()
        if ret[2] != E.OK:
            self.fail_reason.append("some node not pass test, plz see the log")
        return ret
