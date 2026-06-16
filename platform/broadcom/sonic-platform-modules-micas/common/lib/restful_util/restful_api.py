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
import syslog

import requests


class RestfulApiClient():
    Debug_file = "/tmp/restful_api_debug"
    BmcBaseUrl = 'http://240.1.1.2:8080'
    NetworkUrl = '/api/v1.0/network'
    PowerUrl = '/api/v1.0/power'
    HostnameUrl = '/api/v1.0/hostname'
    EventsUrl = '/api/v1.0/events'
    SensorsUrl = '/api/v1.0/sys_switch/sensors'
    TempSensorsUrl = '/api/v1.0/sys_switch/temp_sensor'
    VolSensorsUrl = '/api/v1.0/sys_switch/vol_sensor'
    CurSensorsUrl = '/api/v1.0/sys_switch/cur_sensor'
    SyseepromUrl = '/api/v1.0/syseeprom'
    FansUrl = '/api/v1.0/sys_switch/fans'
    FanUrl = '/api/v1.0/sys_switch/fan/'
    PsusUrl = '/api/v1.0/sys_switch/psus'
    PsuUrl = '/api/v1.0/sys_switch/psu/'
    LEDsUrl = '/api/v1.0/sys_switch/leds'
    FirmwaresUrl = '/api/v1.0/sys_switch/firmwares'
    WatchdogsUrl = '/api/v1.0/watchdog-lambda-os'
    UsersUrl = '/api/v1.0/users'
    TimeUrl = '/api/v1.0/time'
    TimezoneUrl = '/api/v1.0/timezone'
    NtpUrl = '/api/v1.0/ntp'

    def restful_api_error_log(self, msg):
        syslog.openlog("restful_api")
        syslog.syslog(syslog.LOG_ERR, msg)
        syslog.closelog()

    def restful_api_debug_log(self, msg):
        if os.path.exists(self.Debug_file):
            syslog.openlog("restful_api")
            syslog.syslog(syslog.LOG_DEBUG, msg)
            syslog.closelog()

    def get_request(self, url, time_out=(30, 30)):
        try:
            full_url = self.BmcBaseUrl + url
            self.restful_api_debug_log("GET: %s" % full_url)
            response = requests.get(full_url, timeout=time_out)
            self.restful_api_debug_log("RET: %s" % str(response.json()))
            return response.json()
        except Exception as e:
            self.restful_api_error_log(str(e))
            return None

    def post_request(self, url, data, time_out=(30, 30)):
        try:
            full_url = self.BmcBaseUrl + url
            self.restful_api_debug_log("POST: %s -d %s" % (full_url, str(data)))
            response = requests.post(full_url, json=data, timeout=time_out)
            if response.status_code == 200:
                self.restful_api_debug_log("RET: %s" % str(response.json()))
                return response.json()
            else:
                self.restful_api_error_log("Post request failed. Status code {}".format(response.status_code))
                return None
        except Exception as e:
            self.restful_api_error_log(str(e))
            return None
'''
if __name__ == '__main__':
    client = RestfulApiClient()

    time_data = client.get_request(client.TimeUrl)
    print("Current time:", time_data)

    fans = client.get_request(client.FansUrl)
    print("Current fans:", fans)

    new_time = "2023-08-31 14:41:28 +0800"
    response = client.post_request(client.TimeUrl, {"time": new_time})
    print("Time set successfully:", response)

    cmd = "bmc reset cold"
    response = client.post_request(client.PowerUrl, {"cmd": cmd})
    print("Time set successfully:", response)

    #sensors = client.get_request(client.SensorsUrl)
    #print("sensors:", sensors)
'''
