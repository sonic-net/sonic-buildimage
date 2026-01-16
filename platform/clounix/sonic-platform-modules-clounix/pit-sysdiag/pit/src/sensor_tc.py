import sys
from tabulate import tabulate
from test_case import TestCaseCommon
from function import load_platform_util_module
from errcode import E

class SENSORTC(TestCaseCommon):
    __PLATFORM_SPECIFIC_MODULE_NAME = "sensorutil"
    __PLATFORM_SPECIFIC_CLASS_NAME = "SensorUtil"
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "sensor_tc"
        self.sensor_util = None
        self.sensor_node = None
        self.sensor_type = None
        self.sensor_node_name = None
        self.sensor_factor = None
        self.svid_info_dict = None
        TestCaseCommon.__init__(self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file)
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
        header = ["Sensor", 'InputName', 'Status', 'Value', 'LowThd', 'HighThd']
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
        
        if ret != E.OK:
            fail_cnt += 1
            self.logger.log_err("FAIL!", also_print_console)
        else:
            pass_cnt += 1
            self.logger.log_info("PASS.", also_print_console)
        
        result = E.OK if fail_cnt == 0  else E.EFAIL
        return [pass_cnt, fail_cnt, result]

    def run_test(self, *argv):
        sensor_dict = self.load_sensor_info()
        return self.sensor_verify(sensor_dict, True)
