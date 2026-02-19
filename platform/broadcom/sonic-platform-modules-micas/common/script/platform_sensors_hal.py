#!/usr/bin/env python3
#
# Copyright (C) 2024 Micas Networks Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import os
import sys
from plat_hal.interface import interface, getplatform_name
from collections import OrderedDict

class Platoform_sensor_hal(object):

    # status showed
    __STATUS_OK = "OK"
    __STATUS_ABSENT = "ABSENT"
    __STATUS_NOT_OK = "NOT OK"
    __STATUS_FAILED = "GET FAILED"

    def __init__(self):
        self.int_case = interface()

    def print_console(self, msg):
        print(msg)

    def print_platform(self):
        platform_info = getplatform_name()
        self.print_console(platform_info)
        self.print_console("")

    def print_boardtemp(self):
        try:
            '''
            eg: cpu temp, mac temp and others
            Onboard Temperature Sensors:
            BASE_air_inlet       : 17.0 C (high = 80.0 C)
            MCU_air_inlet0       : 16.5 C (high = 80.0 C)
            MCU_air_inlet1       : 16.5 C (high = 80.0 C)
            '''
            info_dict = self.int_case.get_temp_info_s3ip()
    
            monitor_sensor = []
            for sensor_key, sensor_info in info_dict.items():
                monitor_one_sensor_dict = OrderedDict()
                monitor_one_sensor_dict['id'] = sensor_key
                try:
                    monitor_one_sensor_dict['temp1_input'] = float(sensor_info["Value"]) / 1000
                    monitor_one_sensor_dict['temp1_max'] = float(sensor_info["Max"]) / 1000
                except Exception:
                    monitor_one_sensor_dict["status"] = self.__STATUS_FAILED
                # monitor_one_sensor_dict['temp1_max_hyst'] = sensor_info["High"]
                monitor_sensor.append(monitor_one_sensor_dict)
    
            print_info_str = ""
            toptile = "Onboard Temperature Sensors:"
            errformat = "    {id:<25} : {status}"
            # formatstr = "    {id:<20} : {temp1_input} C (high = {temp1_max} C, hyst = {temp1_max_hyst} C)"
            formatstr = "    {id:<25} : {temp1_input} C (high = {temp1_max} C)"
    
            if len(monitor_sensor) != 0:
                print_info_str += toptile + '\n'
                for item in monitor_sensor:
                    realformat = formatstr if item.get('status', self.__STATUS_OK) == self.__STATUS_OK else errformat
                    print_info_str += realformat.format(**item) + '\n'
                self.print_console(print_info_str)
        except Exception:
            pass

    def print_fan_sensor(self):
        try:
            '''
            eg:
            Onboard fan Sensors:
            fan1 :
                fan_type  :FAN18K8086-F
                sn        :0000000000000
                hw_version:00
                Speed     :
                    speed_front :12077 RPM
                    speed_rear  :10231 RPM
                status    :OK
            '''
            fans = self.int_case.get_fans()
            fan_dict = self.int_case.get_fan_info_all()
            monitor_fans = []
            for fan in fans:
                monitor_one_fan_dict = OrderedDict()
                monitor_one_fan_dict["id"] = fan.name
                present = fan_dict.get(fan.name).get("Present")
                if present == "no":
                    monitor_one_fan_dict["status"] = self.__STATUS_ABSENT
                else:
                    monitor_one_fan_dict["fan_type"] = fan_dict.get(fan.name).get("DisplayName")
                    monitor_one_fan_dict["sn"] = fan_dict.get(fan.name).get("SN")
                    monitor_one_fan_dict["hw_version"] = fan_dict.get(fan.name).get("HW")
        
                    all_rotors_ok = True
                    rotor_speeds = {}
                    for rotor in fan.rotor_list:
                        rotor_info = fan_dict.get(fan.name).get(rotor.name)
                        rotor_speeds[rotor.name] = rotor_info.get("Speed")
                        running = rotor_info.get("Running")
                        hw_alarm = rotor_info.get("HwAlarm")
                        if running != "yes" or hw_alarm != "no":
                            all_rotors_ok = False
                    monitor_one_fan_dict.update(rotor_speeds)
                    monitor_one_fan_dict["status"] = self.__STATUS_OK if all_rotors_ok else self.__STATUS_NOT_OK
                monitor_one_fan_dict["rotor_num"] = len(fan.rotor_list)
                monitor_fans.append(monitor_one_fan_dict)
    
            print_info_str = ""
            toptile = "Onboard fan Sensors:"
            errformat = "    {id} : {status}\n"  # "    {id:<20} : {status}"
            fan_signle_rotor_format = "    {id} : \n"  \
                "        fan_type  : {fan_type}\n"  \
                "        sn        : {sn}\n"  \
                "        hw_version: {hw_version}\n"  \
                "        Speed     : {Speed} RPM\n"     \
                "        status    : {status} \n"
            fan_double_rotor_format = "    {id} : \n"  \
                "        fan_type  : {fan_type}\n"  \
                "        sn        : {sn}\n"  \
                "        hw_version: {hw_version}\n"  \
                "        Speed     :\n"     \
                "            speed_front : {Rotor1:<5} RPM\n"     \
                "            speed_rear  : {Rotor2:<5} RPM\n"     \
                "        status    : {status} \n"
    
            if len(monitor_fans) != 0:
                print_info_str += toptile + '\n'
                for item in monitor_fans:
                    if item.get("rotor_num", 1) == 2:
                        realformat = fan_double_rotor_format if item.get('status', self.__STATUS_OK) == self.__STATUS_OK else errformat
                    else:
                        realformat = fan_signle_rotor_format if item.get('status', self.__STATUS_OK) == self.__STATUS_OK else errformat
                    print_info_str += realformat.format(**item)
                self.print_console(print_info_str)
        except Exception:
            pass

    def print_psu_sensor(self):
        try:
            '''
            Onboard Power Supply Unit Sensors:
            psu1 :
                type       :PSA2000CRPS-F
                sn         :R693A3D100003
                in_current :1.8 A
                in_voltage :237.8 V
                out_current:30.1 A
                out_voltage:12.2 V
                temp       :22.5 C
                fan_speed  :26656 RPM
                in_power   :413.0 W
                out_power  :366.5 W
            '''
            psus = self.int_case.get_psus()
            psu_dict = self.int_case.get_psu_info_all()
            monitor_psus = []
            for psu in psus:
                monitor_one_psu_dict = OrderedDict()
                monitor_one_psu_dict["id"] = psu.name
                present = psu_dict.get(psu.name).get("Present")
                if present == "no":
                    monitor_one_psu_dict["status"] = self.__STATUS_ABSENT
                else:
                    monitor_one_psu_dict["type"] = psu_dict.get(psu.name).get("PN")
                    monitor_one_psu_dict["sn"] = psu_dict.get(psu.name).get("SN")
                    monitor_one_psu_dict["in_current"] = psu_dict.get(psu.name).get("Inputs").get("Current").get("Value")
                    monitor_one_psu_dict["in_voltage"] = psu_dict.get(psu.name).get("Inputs").get("Voltage").get("Value")
                    monitor_one_psu_dict["out_current"] = psu_dict.get(psu.name).get("Outputs").get("Current").get("Value")
                    monitor_one_psu_dict["out_voltage"] = psu_dict.get(psu.name).get("Outputs").get("Voltage").get("Value")
                    monitor_one_psu_dict["temp"] = psu_dict.get(psu.name).get("Temperature").get("Value")
                    monitor_one_psu_dict["fan_speed"] = psu_dict.get(psu.name).get("FanSpeed").get("Value")
                    monitor_one_psu_dict["in_power"] = psu_dict.get(psu.name).get("Inputs").get("Power").get("Value")
                    monitor_one_psu_dict["out_power"] = psu_dict.get(psu.name).get("Outputs").get("Power").get("Value")
                monitor_psus.append(monitor_one_psu_dict)

            print_info_str = ""
            toptile = "Onboard Power Supply Unit Sensors:"
            errformat = "    {id} : {status}\n"  # "    {id:<20} : {status}"
            psuformat = "    {id} : \n"  \
                        "        type       : {type}\n"  \
                        "        sn         : {sn}\n"  \
                        "        in_current : {in_current} A\n"  \
                        "        in_voltage : {in_voltage} V\n"     \
                        "        out_current: {out_current} A\n"     \
                        "        out_voltage: {out_voltage} V\n"     \
                        "        temp       : {temp} C        \n"     \
                        "        fan_speed  : {fan_speed} RPM\n"     \
                        "        in_power   : {in_power} W\n"        \
                        "        out_power  : {out_power} W\n"

            if len(monitor_psus) != 0:
                print_info_str += toptile + '\r\n'
                for item in monitor_psus:
                    realformat = psuformat if item.get('status', self.__STATUS_OK) == self.__STATUS_OK else errformat
                    print_info_str += realformat.format(**item)
                self.print_console(print_info_str)
        except Exception:
            pass

    def print_boarddcdc(self):
        try:
            '''
            eg:
            Onboard DCDC Sensors:
            CPU_VCCIN                        : 12.187 V (Min = 1.500  V, Max = 2.000  V)
            CPU_P1V8                         : 1.780  V (Min = 1.710  V, Max = 1.890  V)
            CPU_P1V05                        : 12.250 V (Min = 1.000  V, Max = 1.120  V)
            CPU_VNN_PCH                      : 1.060  V (Min = 0.600  V, Max = 1.200  V)
            CPU_P1V2_VDDQ                    : 12.125 V (Min = 1.140  V, Max = 1.260  V)
            CPU_VNN_NAC                      : 12.187 V (Min = 0.600  V, Max = 1.200  V)
            CPU_VCC_ANA                      : 0.845  V (Min = 0.950  V, Max = 1.050  V)
            CPU_P1V05                        : 1.057  V (Min = 0.990  V, Max = 1.130  V)
            '''

            dcdc_dict = self.int_case.get_dcdc_all_info()
            monitor_sensor = []
            for sensor_key, sensor_info in dcdc_dict.items():
                monitor_one_sensor_dict = OrderedDict()
                monitor_one_sensor_dict['id'] = sensor_key
                try:
                    monitor_one_sensor_dict['dcdc_input'] = float(sensor_info["Value"]) / 1000
                    monitor_one_sensor_dict['dcdc_unit'] = sensor_info["Unit"]
                    monitor_one_sensor_dict['dcdc_min'] = float(sensor_info["Min"]) / 1000
                    monitor_one_sensor_dict['dcdc_unit'] = sensor_info["Unit"]
                    monitor_one_sensor_dict['dcdc_max'] = float(sensor_info["Max"]) / 1000
                    monitor_one_sensor_dict['dcdc_unit'] = sensor_info["Unit"]
                    monitor_one_sensor_dict["status"] = sensor_info["Status"]
                except Exception:
                    monitor_one_sensor_dict["status"] = self.__STATUS_FAILED
                monitor_sensor.append(monitor_one_sensor_dict)

            print_info_str = ""
            toptile = "Onboard DCDC Sensors:"
            errformat = "    {id:<26} : {errmsg}"
            ok_formatstr = "    {id:<26} : {dcdc_input:<6} {dcdc_unit:<1} (Min = {dcdc_min:<6} {dcdc_unit:<1}, Max = {dcdc_max:<6} {dcdc_unit:<1})"
            nok_formatstr = "    {id:<26} : {dcdc_input:<6} {dcdc_unit:<1} (Min = {dcdc_min:<6} {dcdc_unit:<1}, Max = {dcdc_max:<6} {dcdc_unit:<1}) ({status:<6})"

            if len(monitor_sensor) != 0:
                print_info_str += toptile + '\n'
                for item in monitor_sensor:
                    if item.get("status", self.__STATUS_OK) == self.__STATUS_OK:
                        realformat = ok_formatstr
                    elif item.get("status", self.__STATUS_OK) == self.__STATUS_NOT_OK:
                        realformat = nok_formatstr
                    else:
                        realformat = errformat
                    print_info_str += realformat.format(**item) + '\n'
                self.print_console(print_info_str)
        except Exception:
            pass

    def getsensors(self):
        self.print_platform()
        self.print_boardtemp()
        # self.print_macpower_sensors()
        self.print_fan_sensor()
        self.print_psu_sensor()
        # self.print_slot_sensor()
        self.print_boarddcdc()
    
    def get_sensor_print_src(self):
        return self.int_case.get_sensor_print_src()

if __name__ == "__main__":
    platform_sensor_hal = Platoform_sensor_hal()
    platform_sensor_hal.getsensors()
