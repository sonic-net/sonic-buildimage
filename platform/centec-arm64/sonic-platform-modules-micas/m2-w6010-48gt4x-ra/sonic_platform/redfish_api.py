#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import shlex
import json
import os
import ssl
import subprocess
import syslog

class Redfish_Api():
    BmcBaseUrl = 'http://240.1.1.1:8080'
    ThermalUrl = '/redfish/v1/Chassis/1/Thermal'
    PowerUrl = '/redfish/v1/Chassis/1/Power'
    ThresholdSensorsUrl = '/redfish/v1/Chassis/1/ThresholdSensors'
    FanSpeedUrl = '/redfish/v1/Chassis/1/Thermal/Actions/Oem/OpenBmc/Fan.SetSpeed'
    BoardsUrl = '/redfish/v1/Chassis/1/Boards/'
    BoardLedUrl = "/redfish/v1/Chassis/1/Boards/{}/Actions/Oem/OpenBmc/Boards.SetLED"

    # Maximum time in seconds that you allow the connection to the server to take.
    connect_timeout = 30
    # Maximum  time  in seconds that you allow the whole operation to take
    operation_timeout = 300

    default_prefix='/redfish/v1/'
    session = None
    __DEBUG__ = "N"
    __DUMP_RESP__ = "N"
    RST_STATUS = "status"
    RST_SUCCESS = "OK"
    refish_logger = None

    def redfish_log_debug(self, msg):
        if (self.__DEBUG__ == "Y"):
            syslog.openlog("redfis_api")
            syslog.syslog(syslog.LOG_DEBUG, msg)
            syslog.closelog()

    def redfish_log_error(self, msg):
        syslog.openlog("redfish_api")
        syslog.syslog(syslog.LOG_ERR, msg)
        syslog.closelog()

    def __init__(self):
        pass

    def get_full_url(self, url):
        return self.BmcBaseUrl + url

    def _exec_cmd(self, cmd):
        self.redfish_log_debug("Cmd: %s" % cmd)
        p = subprocess.Popen(shlex.split(cmd), shell=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        p.wait()
        self.redfish_log_debug("Cmd return: %d" % p.returncode)
        str_stdout = p.stdout.read().decode('utf-8')
        str_stderr = p.stderr.read().decode('utf-8')
        self.redfish_log_debug("Cmd stdout: %s" % str_stdout)
        if p.returncode !=0:
            self.redfish_log_error("Cmd: %s, failed! error msg:%s" % (cmd, str_stderr))
            return None
        else:
            try:
                ret = json.loads(str_stdout)
                return ret
            except Exception as e:
                self.redfish_log_error("Cmd: %s, failed! stdout msg:%s" % (cmd, str_stdout))
                return None

    def _redfish_get(self, url):
        self.redfish_log_debug("Get info from %s." % url)
        result = None
        try:
            cmd_get="curl --connect-timeout %d -m %d -X GET %s" % (self.connect_timeout, self.operation_timeout, self.get_full_url(url))
            result = self._exec_cmd(cmd_get)
            return result
        except Exception as e:
            self.redfish_log_error("error_message: %s" % e)
            return None

    def _redfish_post(self, url, playload):
        self.redfish_log_debug("post url: %s" % url)
        self.redfish_log_debug("Playload: %s" % playload)

        playload_json = json.dumps(playload)
        result = False
        try:
            cmd_post="curl --connect-timeout %d -m %d -X POST %s -d \'%s\'" % (self.connect_timeout, self.operation_timeout, self.get_full_url(url), playload_json)
            ret_msg = self._exec_cmd(cmd_post)
            if ret_msg == None:
                return False
            elif ret_msg["success"] == False:
                self.redfish_log_error("Url: '%s', Playload: '%s', Bmc return failed, error_message: %s" % (url, playload_json, ret_msg["Message"]))
                result = False
            else:
                result = True
            return result
        except Exception as e:
            self.redfish_log_error("error_message: %s" % e)
            return False

    def get_thermal(self):
        """Get thermal info
        :returns: class 'redfish.rest.v1.RestResponse' or None when failed
        """
        return self._redfish_get(self.ThermalUrl)

    def get_power(self):
        """Get power info
        :returns: class 'redfish.rest.v1.RestResponse' or None when failed
        """
        return self._redfish_get(self.PowerUrl)

    def get_thresholdSensors(self):
        """Get thresholdSensors info
        :returns: class 'redfish.rest.v1.RestResponse' or None when failed
        """
        return self._redfish_get(self.ThresholdSensorsUrl)

    def post_odata(self, odata_id, playload):
        """post odata info
        :params odata_id: the specified odata_id path
        :type odata_id: string
        :playload: info to post
        :type: dictionary
        :returns: True or False
        """
        if odata_id is None or playload is None:
            print("post failed: odata_id or playload is None")
            return False
        return self._redfish_post(odata_id, playload)

    def get_odata(self, odata_id):
        """Get odata info
        :params odata_id: the specified odata_id path
        :type odata_id: string
        :returns: class 'redfish.rest.v1.RestResponse' or None when failed
        """
        if odata_id is None:
            print("Get odata_id failed: odata_id is None")
            return None
        return self._redfish_get(odata_id)

    def post_fanSpeed(self, playload):
        """post odata info
        :playload: info to post
        :type: dictionary
        :returns: True or False
        """
        if playload is None:
            print("post failed: playload is None")
            return False
        return self._redfish_post(self.FanSpeedUrl, playload)

    def get_board(self, board_name="indicatorboard"):
        """Get board info
        :board_name: name of board, default is "indicatorboard"
        :type: string
        :returns: class'redfish.rest.v1.RestResponse' or None when failed
        """
        if board_name is None :
            print("get failed: board_name is None")
            return None
        return self._redfish_get(self.BoardsUrl + board_name)

    def post_boardLed(self, playload, board_name="indicatorboard"):
        """post boardLed info
        :board_name: name of board, default is "indicatorboard"
        :type: string
        :playload: info to post
        :type: dictionary
        :returns: True or False
        """
        if board_name is None or playload is None:
            print("post failed: playload is None")
            return False
        return self._redfish_post(self.BoardLedUrl.format(board_name), playload)

    '''  not supported currently
    def post_thermal(self, playload):
        """post thermal info
        :playload: info to post
        :type: dictionary
        :returns: True or False
        """
        if playload is None:
            print("post_thermal failed: playload is None")
            return None
        return self._redfish_post(self.ThermalUrl, playload)

    def post_power(self, playload):
        """post power info
        :playload: info to post
        :type: dictionary
        :returns: True or False
        """
        if playload is None:
            print("post_power failed: playload is None")
            return None
        return self._redfish_post(self.PowerUrl, playload)

    def post_thresholdSensors(self, playload):
        """post thresholdSensors info
        :playload: info to post
        :type: dictionary
        :returns: True or False
        """
        if playload is None:
            print("post_thresholdSensors failed: playload is None")
            return None
        return self._redfish_post(self.ThresholdSensorsUrl, playload)

    def get_fanSpeed(self):
        """Get board led info
        :returns: class'redfish.rest.v1.RestResponse' or None when failed
        """
        return self._redfish_get(self.FanSpeedUrl)

    def post_board(self, playload, board_name="indicatorboard"):
        """post board info
        :board_name: name of board, default is "indicatorboard"
        :type: string
        :playload: info to post
        :type: dictionary
        :returns: True or False
        """
        if board_name is None or playload is None:
            print("post failed: playload is None")
            return False
        return self._redfish_post(self.BoardsUrl + board_name, playload)

    def get_boardLed(self, board_name="indicatorboard"):
        """Get boardLed info
        :board_name: name of board, default is "indicatorboard"
        :type: string
        :returns: class'redfish.rest.v1.RestResponse' or None when failed
        """
        if board_name is None :
            print("get failed: board_name is None")
            return None
        return self._redfish_get(self.BoardsUrl % board_name)

    '''

'''
if __name__ == '__main__':
    redfish = Redfish_Api()

    ### get
    # boards
    ret = redfish.get_board()
    if ret is None:
        print("get failed: board")
    else:
        print("get succeeded, board:%s" % ret)

    ret = redfish.get_thresholdSensors()
    if ret is None:
        print("get failed: threshold")
    else:
        print("get succeeded, threshold:%s" % ret)

    ret = redfish.get_power()
    if ret is None:
        print("get failed: power")
    else:
        print("get succeeded, power:%s" % ret)

    ret = redfish.get_thermal()
    if ret is None:
        print("get failed:thermal")
    else:
        print("get succeeded,thermal:%s" % ret)

    # get playload
    resp = redfish.get_thresholdSensors()
    if (resp != None):
        print(resp["@odata.id"])
        print(resp["@odata.type"])
        print(resp["Id"])
        print(resp["Name"])
    else:
        print("Failed: get_thresholdSensors")

    ### post
    # fanSpeed
    playload = {}
    playload["FanName"] = 'Fan0'
    playload["FanSpeedLevelPercents"] = "70"
    print("post fanSpeed:%s" % redfish.post_fanSpeed(playload))

    #{"LEDs": [{"IndicatorLEDColor": "green","LEDType": "sys"},{"IndicatorLEDColor": "off","LEDType": "pwr"},{"IndicatorLEDColor": "green","LEDType": "fan"}]}
    playload = {}
    led = {}
    led1 = {}
    led_list = []
    led["IndicatorLEDColor"] = "green"
    led["LEDType"] = "sys"
    led1["IndicatorLEDColor"] = "off"
    led1["LEDType"] = "pwr"
    led_list.append(led)
    led_list.append(led1)
    playload["LEDs"] = led_list
    # boardsLed
    print("post boardLed:%s" % redfish.post_boardLed(playload))
'''
