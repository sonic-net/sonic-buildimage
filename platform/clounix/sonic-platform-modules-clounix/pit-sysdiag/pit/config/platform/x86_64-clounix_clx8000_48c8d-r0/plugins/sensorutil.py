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

    def get_sensor_node_info(self, sensor_node, sensor_node_type, sensor_node_name, sensor_factor = None, dcdc_info_dict = None):
        self.dcdc_sensor_node = sensor_node
        self.dcdc_sensor_type = sensor_node_type
        self.dcdc_sensor_node_name = sensor_node_name

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

    def get_platform_temp(self, key):
        cmd = " ".join([CMD_SHOW_PLATFORM, "temperature"])
        status,output = run_command(cmd)
        temperature_output_lines = output.splitlines()
        self.check_show_platform_temperature(temperature_output_lines)

        regex_int = re.compile(r'\s*(.*)\s+([\d.]+)\s+([\d.]+)\s+([-\d.]+)\s+([\d.]+)\s+([-\d.]+)\s+.*')
        temperature_dict = {}
        for line in temperature_output_lines[2:]:
            if(line.find(key) >= 0):
                t1 = regex_int.match(line)
                if t1:
                    sensor = t1.group(1).strip()
                    temperature = t1.group(2).strip()
                    high_th = t1.group(3).strip()
                    crit_high_th = t1.group(4).strip()
                    temperature_dict["Value"] = temperature
                    temperature_dict["LowThd"] = "0.001"
                    temperature_dict["HighThd"] = high_th
                    temperature_dict["Unit"] = "C"
                    temperature_dict["Description"] = key
                    return temperature_dict

        return None

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
        sensor_p1v8_dcdc_voltage = {}
        sensor_p0v8_dcdc_voltage = {}
        sensor_p1v0_dcdc_voltage = {}
        sensor_p3v3_sfp_dcdc_voltage = {}
        # sensor_p12v_main_dcdc_voltage = {}
        for key,values in value.items():
            if key == "P1V8_AVDD":
                sensor_p1v8_dcdc_voltage["Value"] = values[1]
                sensor_p1v8_dcdc_voltage["LowThd"] = "1250"
                sensor_p1v8_dcdc_voltage["HighThd"] = "3000"
                sensor_p1v8_dcdc_voltage["Unit"] = "mV"
                sensor_p1v8_dcdc_voltage["Description"] = "P1V8_AVDD Voltage"
                print("%s: Current %s(mA), Voltage %s(mV), Power %s(W)" %(key, values[0], values[1], "N/A"))
                continue
                
            if key == "P0V8_AVDD":
                sensor_p0v8_dcdc_voltage["Value"] = int(values[1])/1000
                sensor_p0v8_dcdc_voltage["LowThd"] = "573"
                sensor_p0v8_dcdc_voltage["HighThd"] = "1375"
                sensor_p0v8_dcdc_voltage["Unit"] = "mV"
                sensor_p0v8_dcdc_voltage["Description"] = "P0V8_AVDD Voltage"
                       
            if key == "P1V0_AVDD":
                sensor_p1v0_dcdc_voltage["Value"] = int(values[1])/1000
                sensor_p1v0_dcdc_voltage["LowThd"] = "573"
                sensor_p1v0_dcdc_voltage["HighThd"] = "1375"
                sensor_p1v0_dcdc_voltage["Unit"] = "mV"
                sensor_p1v0_dcdc_voltage["Description"] = "P1V0_AVDD Voltage"
            
            if key == "P3V3_SFP":
                sensor_p3v3_sfp_dcdc_voltage["Value"] = int(values[1])/1000
                sensor_p3v3_sfp_dcdc_voltage["LowThd"] = "2500"
                sensor_p3v3_sfp_dcdc_voltage["HighThd"] = "6000"
                sensor_p3v3_sfp_dcdc_voltage["Unit"] = "mV"
                sensor_p3v3_sfp_dcdc_voltage["Description"] = "P3V3_SFP Voltage"
            
            # if key == "P12V_MAIN":
            #     sensor_p12v_main_dcdc_voltage["Value"] = int(values[1])/1000
            #     sensor_p12v_main_dcdc_voltage["LowThd"] = "2048"
            #     sensor_p12v_main_dcdc_voltage["HighThd"] = "2048"
            #     sensor_p12v_main_dcdc_voltage["Unit"] = "mV"
            #     sensor_p12v_main_dcdc_voltage["Description"] = "P12V_MAIN Voltage"

            print("%s: Current %s(mA), Voltage %s(mV), Power %s(W)" %(key, values[0],str(int(values[1])/1000), str(int(values[2])/1000000))) 

        sensor_dcdc["P1V8_VOLTAGE"] = sensor_p1v8_dcdc_voltage
        sensor_dcdc["P0V8_AVDD"] = sensor_p0v8_dcdc_voltage
        sensor_dcdc["P1V0_AVDD"] = sensor_p1v0_dcdc_voltage
        sensor_dcdc["P3V3_SFP"] = sensor_p3v3_sfp_dcdc_voltage
        # sensor_dcdc["P12V_MAIN"] = sensor_p12v_main_dcdc_voltage
        return sensor_dcdc

    def get_all(self):
        #cmd = " ".join([CMD_SHOW_PLATFORM, "temperature"])
        #status,output = run_command(cmd)
        #print("show platform temperature output:\n {}".format(output))

        #cmd = 'cat /sys_switch/temp_sensor/temp4/value'
        #status,output = run_command(cmd)
        #print("FAN_0x48 output: {}".format(output))
        #cmd = 'cat /sys_switch/temp_sensor/temp5/value'
        #status,output = run_command(cmd)
        #print("FAN_0x49 output: {}".format(output))

        #cmd = 'redis-cli -p 6382 -n 6 hget "Temp_4"'
        #status,output = run_command(cmd)
        #print("Redis FAN_0x48 output: {}".format(output))
        #cmd = 'redis-cli -p 6382 -n 6 hget "Temp_5"'
        #status,output = run_command(cmd)
        #print("Redis FAN_0x49 output: {}".format(output))

        cpu_temp = {}
        CPU_TEMP_KEY = "CORE_TEMP_1"
        cpu_temp = self.get_platform_temp(CPU_TEMP_KEY)

        fan_temp_48 = {}
        FAN_ADDRESS_48_KEY = "Temp_4"
        fan_temp_48 = self.get_platform_temp(FAN_ADDRESS_48_KEY)

        fan_temp_49 = {}
        FAN_ADDRESS_49_KEY = "Temp_5"
        fan_temp_49 = self.get_platform_temp(FAN_ADDRESS_49_KEY)

        psu1_ambient = {}
        PSU1_AMBIENT_KEY = "PSU1_TEMP1"
        psu1_ambient = self.get_platform_temp(PSU1_AMBIENT_KEY)

        psu1_pfc_hotspot = {}
        PSU1_PFC_HOTSPOT_KEY = "PSU1_TEMP3"
        psu1_pfc_hotspot = self.get_platform_temp(PSU1_PFC_HOTSPOT_KEY)

        psu1_sr_hotspot = {}
        PSU1_SR_HOTSPOT_KEY = "PSU1_TEMP2"
        psu1_sr_hotspot = self.get_platform_temp(PSU1_SR_HOTSPOT_KEY)

        psu2_ambient = {}
        PSU2_AMBIENT_KEY = "PSU2_TEMP1"
        psu2_ambient = self.get_platform_temp(PSU2_AMBIENT_KEY)

        psu2_pfc_hotspot = {}
        PSU2_PFC_HOTSPOT_KEY = "PSU2_TEMP3"
        psu2_pfc_hotspot = self.get_platform_temp(PSU2_PFC_HOTSPOT_KEY)

        psu2_sr_hotspot = {}
        PSU2_SR_HOTSPOT_KEY = "PSU2_TEMP2"
        psu2_sr_hotspot = self.get_platform_temp(PSU2_SR_HOTSPOT_KEY)

        temp75c_4a = {}
        TEMP75C_4a_KEY = "Temp3"
        temp75c_4a = self.get_platform_temp(TEMP75C_4a_KEY)

        temp75c_48 = {}
        TEMP75C_48_KEY = "Temp1"
        temp75c_48 = self.get_platform_temp(TEMP75C_48_KEY)

        temp75c_49 = {}
        TEMP75C_49_KEY = "Temp2"
        temp75c_49 = self.get_platform_temp(TEMP75C_49_KEY)
        
        sensor_all = {}
        sensor_cpu = {}
        sensor_cpu["Present"] = True
        if cpu_temp:
            sensor_cpu["CPU_TEMP"] = cpu_temp
        if fan_temp_48:
            sensor_cpu["FAN_TEMP_0x48"] = fan_temp_48
        if fan_temp_49:
            sensor_cpu["FAN_TEMP_0x49"] = fan_temp_49
        if psu1_ambient:
            sensor_cpu["PSU1_AMBIENT"] = psu1_ambient
        if psu1_pfc_hotspot:
            sensor_cpu["PSU1_PFC_HOTSPOT"] = psu1_pfc_hotspot
        if psu1_sr_hotspot:
            sensor_cpu["PSU1_SR_HOTSPOT"] = psu1_sr_hotspot
        if psu2_ambient:
            sensor_cpu["PSU2_AMBIENT"] = psu2_ambient
        if psu2_pfc_hotspot:
            sensor_cpu["PSU2_PFC_HOTSPOT"] = psu2_pfc_hotspot
        if psu2_sr_hotspot:
            sensor_cpu["PSU2_SR_HOTSPOT"] = psu2_sr_hotspot
        if temp75c_4a:
            sensor_cpu["TEMP75C_0x4a"] = temp75c_4a
        if temp75c_48:
            sensor_cpu["TEMP75C_0x48"] = temp75c_48
        if temp75c_49:
            sensor_cpu["TEMP75C_0x49"] = temp75c_49

        sensor_all["TEMP_SENSOR"] = sensor_cpu

        value = self.get_dcdc_value()
        sensor_all["DCDC_POWER"] = self.format_sensor_value(value)
  
        return sensor_all
