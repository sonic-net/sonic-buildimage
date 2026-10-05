#!/usr/bin/python

###############################################################################
#
# # Sensor utility.
#
# # Copyright (C) Alibaba, INC.
#
################################################################################

import requests
import json
import operator

class SensorUtil():
    BMC_REQ_BASE_URI = "http://240.1.1.1:8080/api"

    def __init__(self):
        self.sensor_info_uri = "/".join([self.BMC_REQ_BASE_URI, "sensor/info"])
        self.sensor_info = None

    def _get_sensor_info(self, tmout = 120):
        resp = requests.get(self.sensor_info_uri, timeout = tmout)
        if not resp:
            return False

        sensor_json = resp.json()
        if not sensor_json or not "data" in sensor_json:
            return False

        self.sensor_info = sensor_json["data"]

        return True

    def get_sys_airflow(self):
        sys_air_flow = "Unknown"

        sys_pn = sys_pn_data[0][1]
        if "R1240-F0001" in sys_pn:
            sys_air_flow = "FTOB"
        elif"R1240-F0002" in sys_pn:
            sys_air_flow = "BTOF"

        return sys_air_flow

    def _get_type(self, unit_str):
        unit2type = {"C": "temperature", "A": "amp", "V": "voltage",
                     "RPM": "RPM", "W": "power"}

        if unit_str in unit2type:
            return unit2type[unit_str]
        return None

    def get_num_sensors(self):
        sensor_dict = self.get_all()
        sum = 0
        for sensor_name, sensor_obj in list(sensor_dict.items()):
            if operator.eq(sensor_name, 'Number') == True:
                continue
            sum += len(list(sensor_obj.keys()))
        return sum

    def get_all(self):
        sensor_info = {}

        if not self._get_sensor_info():
            return sensor_info

        sname_temp_cl = ["Cpu", "Inlet", "Switch"]
        sname_temp_rj = ["CPU_TEMP", "INLET_TEMP", "SWITCH_TEMP"]
        for si_name, si_val in list(self.sensor_info.items()):
            if si_name.startswith("PSU"):
                sname = si_name.split("_")[0]
                if si_name.find("Temperature") != -1:
                    new_si_name = "%s_TEMP" % sname
                    si_name = new_si_name
            elif si_name in sname_temp_cl:
                sname = "TEMPERATURE"
                new_si_name = "%s_temp" % si_name
                si_name = new_si_name.upper()
            elif si_name in sname_temp_rj:
                sname = "TEMPERATURE"
            elif si_name.startswith("CPU"):
                sname = "CPU"
            elif si_name.startswith("FAN"):
                sname = "FAN"
            elif si_name.startswith("Switch") or \
                 si_name.startswith("SWITCH") or \
                 si_name.find("_LC_") != -1:
                sname = "SWITCH"
            elif si_name.startswith("SYS") or \
                 si_name.startswith("Baseboard"):
                sname = "SYSTEM"
            else:
                sname = "OTHER"

            si_info = {}
            si_info["LowThd"] = si_val["Min"]
            si_info["HighThd"] = si_val["Max"]
            si_info["Value"] = si_val["Value"]
            type = self._get_type(si_val["Unit"])
            if not type:
                continue
            si_info["Type"] = type

            if sname not in sensor_info:
                sensor_info[sname] = {}

            sensor_info[sname][si_name] = si_info

        # j = json.dumps(self.sensor_info, sort_keys=True, indent=4, separators=(',', ': '))
        # print j

        return sensor_info
