import sys
from tabulate import tabulate
from test_case import TestCaseCommon
from function import load_platform_util_module
from errcode import E
import traceback


class SENSORTC(TestCaseCommon):
    __PLATFORM_SPECIFIC_MODULE_NAME = "sensorutil"
    __PLATFORM_SPECIFIC_CLASS_NAME = "SensorUtil"
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "sensor_tc"
        self.sensor_util = None
        TestCaseCommon.__init__(self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file)

        sensor_module = load_platform_util_module(self.__PLATFORM_SPECIFIC_MODULE_NAME)
        try:
            platform_util_class = getattr(sensor_module, self.__PLATFORM_SPECIFIC_CLASS_NAME)
            self.sensor_util = platform_util_class()
        except AttributeError as e:
            self.logger.log_err(str(e), True)
            sys.exit(1)

    def load_sensor_info(self):
        sensor_dict = {}
        if self.sensor_util:
            sensor_dict = self.sensor_util.get_all()
        return sensor_dict

    def sensor_verify(self, sensor_dict):
        self.logger.log_info("sensor verify start")
        if not sensor_dict:
            self.log_reason("get sensors failed!")
            return E.EFAIL

        ret = E.OK
        header = ["Sensor", 'InputName', 'Status', 'Value', 'LowThd', 'HighThd']
        status_table = []

        try:
            for sensor_name, sensor_obj in sensor_dict.items():
                if sensor_name == 'Number':
                    continue

                if not isinstance(sensor_obj, dict):
                    continue

                si_names = [k for k in sensor_obj.keys()]
                si_names.sort()
                for si_name in si_names:
                    si = sensor_obj[si_name]
                    sval = si.get('Value')
                    slow = si.get('LowThd')
                    shigh = si.get("HighThd")
                    sunit = si.get('Unit')
                    sdesc = si.get('Description')
                    fault = False

                    if not sunit:
                        sunit = ""
                        stype = si.get('Type')
                        if stype:
                            type2unit = {"amp": "A", "voltage": "V", "power": "W",
                                         "temperature": "C", "RPM": "RPM"}
                            if stype in type2unit:
                                sunit = type2unit[stype]

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

                    status = 'NOT_OK'
                    if fault == False and sval >= slow and sval <= shigh:
                        status = 'OK'
                    else:
                        ret = E.ESSR7003
                        self.log_reason("{} out of threshold".format(si_name))

                    status_table.append([sensor_name, si_name, status, "{} {}".format(sval, sunit), \
                                         "{} {}".format(slow, sunit), "{} {}".format(shigh, sunit)])
        except Exception as e:
            reason = "sensor verify got exception: {}".format(str(e))
            self.fail_reason.append(str(e))
            self.logger.log_err(traceback.format_exc())
            ret = E.ESSR7002

        if len(status_table) > 0:
            status_table.sort()
            self.logger.log_info(tabulate(status_table, header, tablefmt="simple"))

        if ret != E.OK:
            self.logger.log_err("sensor verify done, FAILED.")
        else:
            self.logger.log_info("sensor verify done, PASS.")

        return ret

    def run_test(self, *argv):
        try:
            sensor_dict = self.load_sensor_info()
            ret = self.sensor_verify(sensor_dict)
        except Exception as e:
            reason = "load_sensor/sensor_verify got exception: {}".format(str(e))
            self.log_reason(reason)

        return ret
