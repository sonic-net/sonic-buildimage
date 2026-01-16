# -*- coding:utf-8
from cProfile import run
from pit_util_common import run_command
from test_case import TestCaseCommon
from errcode import E
import re
import json

CMD_SHOW_PLATFORM = "show platform"

class SensorUtil(object):
    def __init__(self):
        self.dcdc_sensor_node_name = None
        self.dcdc_sensor_type = None
        self.dcdc_sensor_node = None
        self.dcdc_sensor_factor = None
        self.dcdc_info_dict = None

    def get_sensor_node_info(self, sensor_node, sensor_node_type, sensor_node_name, sensor_factor = None, dcdc_info_dict = None):
        self.dcdc_sensor_node = sensor_node
        self.dcdc_sensor_type = sensor_node_type
        self.dcdc_sensor_node_name = sensor_node_name
        self.dcdc_sensor_factor = sensor_factor
        self.dcdc_info_dict = dcdc_info_dict

    # 获取设备支持的sensor数量
    def get_num_sensors(self):
        """
        Retrieves the number of Sensors supported on the device
        """
        SENSOR_NUM = 1
        return SENSOR_NUM
    def get_field_range(self, second_line):
        """
        @summary: Utility function to help get field range from a simple tabulate output line.
        Simple tabulate output looks like:

        Head1   Head2       H3 H4
        -----  ------  ------- --
        V1      V2       V3 V4

        @return: Returned a list of field range. E.g. [(0,4), (6, 10)] means there are two fields for
        each line, the first field is between position 0 and position 4, the second field is between
        position 6 and position 10.
        """
        field_ranges = []
        begin = 0
        while 1:
            end = second_line.find(' ', begin)
            if end == -1:
                field_ranges.append((begin, len(second_line)))
                break

            field_ranges.append((begin, end))
            begin = second_line.find('-', end)
            if begin == -1:
                break

        return field_ranges

    def verify_show_platform_temperature_output(self,raw_output_lines):
        """
        @summary: Verify output of `show platform temperature`. Expected output is
                "Thermal Not detected" or a table of thermal status data with 8 columns.
        """
        NUM_EXPECTED_COLS = 8

        if len(raw_output_lines) <= 0:
            print("There must be at least one line of output")

        if len(raw_output_lines) == 1:
            print( "Unexpected thermal status output")
        else:
            if len(raw_output_lines) <= 2:
                print("There must be at least two lines of output if any thermal is detected")

            second_line = raw_output_lines[1]
        
            field_ranges = self.get_field_range(second_line)
            if len(field_ranges) != NUM_EXPECTED_COLS:
                print("Output should consist of {} columns".format(NUM_EXPECTED_COLS))
    
    def check_show_platform_temperature(self,temperature_output_lines):
        """
        @summary: Verify output of `show platform temperature`
        """
        cmd = " ".join([CMD_SHOW_PLATFORM, "temperature"])

        #print("Verifying output of '{}'".format(cmd))
        self.verify_show_platform_temperature_output(temperature_output_lines)

    def get_platform_temp(self):
        sensor_cpu = {}
        sensor_cpu["Present"] = True

        cmd = " ".join([CMD_SHOW_PLATFORM, "temperature"])
        status,output = run_command(cmd)
        temperature_output_lines = output.splitlines()
        self.check_show_platform_temperature(temperature_output_lines)

        regex_int = re.compile(r'\s*(.*)\s+([\d.]+)\s+([\d.]+)\s+([-\d.]+)\s+([\d.]+)\s+([-\d.]+)\s+.*')
        for line in temperature_output_lines[2:]:
            temperature_dict = {}
            t1 = regex_int.match(line)
            if t1:
                sensor = t1.group(1).strip()
                temperature = t1.group(2).strip()
                high_th = t1.group(3).strip()
                crit_high_th = t1.group(4).strip()
                # filter out the invalid temper sensors
                if float(temperature) <= 0:
                    continue
                temperature_dict["Value"] = temperature
                temperature_dict["LowThd"] = "0.001"
                temperature_dict["HighThd"] = high_th
                temperature_dict["Unit"] = "C"
                temperature_dict["Description"] = sensor
                sensor_cpu[sensor] = temperature_dict

        return sensor_cpu

    def get_dcdc_sensor_value(self, sensor_type):
        sensor_value = []
        for node in self.dcdc_sensor_node_name[sensor_type]:
            cmd = " ".join(["cat", self.dcdc_sensor_node[sensor_type]+node])
            if(node == "NA"):
                output = "0"
            else:
               status, output = run_command(cmd)
            sensor_value.append(output)

        return sensor_value
    
    def get_dcdc_value(self):
        dcdc_sensor_value = {}
        for type in self.dcdc_sensor_type:
            dcdc_sensor_value[type] = self.get_dcdc_sensor_value(type)
        return dcdc_sensor_value
    
    def format_sensor_value(self,value):
        sensor_dcdc = {}
        sensor_dcdc["Present"] = True
        
        for key,values in value.items():                        
            sensor_dcdc_voltage = {}
            sensor_dcdc_item = {}            
            
            vol_factor = self.dcdc_sensor_factor[key][1] if (self.dcdc_sensor_factor and key in self.dcdc_sensor_factor) else 1
            sensor_dcdc_voltage["Value"] = int(values[1]) / 1000 * vol_factor
            sensor_dcdc_voltage["LowThd"] = self.dcdc_info_dict[key]["low_voltage"] if key in self.dcdc_info_dict else int(values[1])
            sensor_dcdc_voltage["HighThd"] = self.dcdc_info_dict[key]["high_voltage"] if key in self.dcdc_info_dict else int(values[1])
            sensor_dcdc_voltage["Unit"] = "mV"
            sensor_dcdc_voltage["Description"] = "%s Voltage" % key
            sensor_dcdc_item = {key: sensor_dcdc_voltage}
            sensor_dcdc.update(sensor_dcdc_item)
            print("%s: Current %s(mA), Voltage %s(mV), Power %s(W)" %(key, values[0],str(int(values[1])), str(int(values[2])/1000000))) 
            
        return sensor_dcdc

    def get_all(self):
        sensor_all = {}
    
        sensor_all["TEMP_SENSOR"] = self.get_platform_temp()

        value = self.get_dcdc_value()
        sensor_all["DCDC_POWER"] = self.format_sensor_value(value)
  
        return sensor_all   
