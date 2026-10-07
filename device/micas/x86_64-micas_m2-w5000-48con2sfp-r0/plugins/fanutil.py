#!/usr/bin/python

import requests
import re
import json
import traceback
import time


class FanUtil():
    BMC_REQ_BASE_URI = "http://240.1.1.1:8080/api"
    ROTOR_PER_FAN = 1

    def __init__(self):
        self.fan_info_uri = "/".join([self.BMC_REQ_BASE_URI, "fan/info"])
        self.fan_num_uri = "/".join([self.BMC_REQ_BASE_URI, "fan/number"])

    def _get_fan_info(self, tmout=120):
        count = 3
        conerr = False
        while (count > 0):
            count = count - 1
            try:
                resp = requests.get(self.fan_info_uri, timeout=tmout)
                if not resp:
                    return False
            except requests.exceptions.ConnectionError as reason:
                if "Connection aborted" in "%s" % (reason):
                    time.sleep(2)
                else:
                    conerr = True
                    break
            else:
                break
        if count == 0 or conerr:
            traceback.print_exc()
            return False

        fan_json = resp.json()
        if not fan_json or not "data" in fan_json:
            return False

        self.fan_info = fan_json["data"]

        return True

    def get_num_fans(self, tmout=120):
        """
        Get total fan number

        @return number of fan, -1 for failure
        """
        count = 3
        conerr = False
        while (count > 0):
            count = count - 1
            try:
                resp = requests.get(self.fan_num_uri, timeout=tmout)
                if not resp:
                    return -1
            except requests.exceptions.ConnectionError as reason:
                if "Connection aborted" in "%s" % (reason):
                    time.sleep(2)
                else:
                    conerr = True
                    break
            else:
                break

        if count == 0 or conerr:
            traceback.print_exc()
            return -1

        fan_nr_json = resp.json()
        if not fan_nr_json or "data" not in fan_nr_json:
            return -1

        try:
            nr_fan = fan_nr_json["data"]["Number"]
        except Exception as e:
            nr_fan = -1

        return nr_fan

    def get_all(self):
        """
            Get all information of system FANs, returns JSON objects in python 'DICT'.
            Number, mandatory, max number of FAN, integer
            FAN1_1, FAN1_2, ... mandatory, FAN name, string
            Present, mandatory for each FAN, present status, boolean, True for present, False for NOT present, read directly from h/w
            Running, conditional, if PRESENT is True, running status of the FAN, True for running, False for stopped, read directly from h/w
            Speed, conditional, if PRESENT is True, real FAN speed, float, read directly from h/w
            LowThd, conditional, if PRESENT is True, lower bound of FAN speed, float, read from h/w
            HighThd, conditional, if PRESENT is True, upper bound of FAN speed, float, read from h/w
            PN, conditional, if PRESENT is True, PN of the FAN, string
            SN, conditional, if PRESENT is True, SN of the FAN, string)
        """
        fan_info = {}
        if not self._get_fan_info():
            return fan_info

        if "Number" not in self.fan_info:
            return fan_info

        fan_nr = self.fan_info["Number"]
        for fan_idx in range(1, fan_nr+1):
            fan_name = "FAN%d" % fan_idx
            if not fan_name in self.fan_info:
                print("%s not in self.fan_info" % fan_name)
                continue
            fi = self.fan_info[fan_name]
            if "Rotors" not in fi:
                print("Rotors not in fi")
                continue
            rotor_nr = fi["Rotors"]
            for ridx in range(1, rotor_nr+1):
                sub_name = "%s_%d" % (fan_name, ridx)
                rname = "Rotor%d" % ridx
                sub_fan_info = {}
                if rname not in fi:
                    print("%s not in fi" % rname)
                    continue
                try:
                    sub_fan_info["Present"] = True if fi["Present"] == "yes" else False
                    sub_fan_info["Running"] = fi[rname]["Running"]
                    sub_fan_info["Speed"] = fi[rname]["Speed"]
                    sub_fan_info["LowThd"] = fi[rname]["SpeedMin"]
                    sub_fan_info["HighThd"] = fi[rname]["SpeedMax"]
                    sub_fan_info["PN"] = fi["PN"]
                    sub_fan_info["SN"] = fi["SN"]
                    sub_fan_info["AirFlow"] = fi["AirFlow"]
                    if (fi[rname]["HwAlarm"] == "no") and \
                        (sub_fan_info["Speed"] != None and sub_fan_info["LowThd"] != None and sub_fan_info["Speed"] >= sub_fan_info["LowThd"]) and \
                        (sub_fan_info["Speed"] != None and sub_fan_info["HighThd"] != None and sub_fan_info["Speed"] <= sub_fan_info["HighThd"]):
                        sub_fan_info["Status"] = True
                    else:
                        sub_fan_info["Status"] = False

                    fan_info[sub_name] = sub_fan_info
                except Exception as e:
                    print("GOT EXCEPTON: %s" % str(e))
                    continue
        fan_info["Number"] = fan_nr * self.ROTOR_PER_FAN

        #j = json.dumps(fan_info, sort_keys=True, indent=4, separators=(',', ': '))
        #print j

        return fan_info

