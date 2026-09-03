#!/usr/bin/python

###############################################################################
#
## PSU utility.
#
## Copyright (C) Alibaba, INC.
#
################################################################################

import requests
import re
import json

try:
    from sonic_psu.psu_base import PsuBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class PsuUtil(PsuBase):
    BMC_REQ_BASE_URI = "http://240.1.1.1:8080/api"

    def __init__(self):
        PsuBase.__init__(self)
        self.psu_info_uri = "/".join([self.BMC_REQ_BASE_URI, "psu/info"])
        self.psu_num_uri = "/".join([self.BMC_REQ_BASE_URI, "psu/number"])

    def _get_psu_info(self, tmout=120):
        resp = requests.get(self.psu_info_uri, timeout=tmout)
        if not resp:
            return False

        psu_json = resp.json()
        if not psu_json or not "data" in psu_json:
            return False

        self.psu_info = psu_json["data"]

        return True

    def get_num_psus(self, tmout=120):
        """
        Retrieves the number of PSUs available on the device
        :return: An integer, the number of PSUs available on the device
        """
        resp = requests.get(self.psu_num_uri, timeout=tmout)
        if not resp:
            return -1

        psu_nr_json = resp.json()
        if not psu_nr_json or "data" not in psu_nr_json:
            return -1

        try:
            nr_psu = psu_nr_json["data"]["Number"]
        except Exception as e:
            nr_psu = -1

        return nr_psu

    def get_psu_status(self, index):
        """
        Retrieves the oprational status of power supply unit (PSU) defined
                by 1-based index <index>
        :param index: An integer, 1-based index of the PSU of which to query status
        :return: Boolean, True if PSU is operating properly, False if PSU is faulty
        """

        # init data
        psu_key = "PSU" + str(index)
        psu_status_key = "Power Status"
        psu_power_status = False

        try:
            # Request and validate sensor's information
            self.fru_status_list, self.psu_info_list = self.request_data()

            # Get PSU power status.
            for fru_status in self.fru_status_list:
                is_psu = fru_status.get(psu_key)
                psu_status = str(fru_status.get(psu_status_key)).strip()

                if is_psu is not None and psu_status == "OK":
                    psu_power_status = True

        except:
            print("Error: Unable to access PSU power status")
            return False

        return psu_power_status

    def get_psu_presence(self, index):
        """
        Retrieves the presence status of power supply unit (PSU) defined
                by 1-based index <index>
        :param index: An integer, 1-based index of the PSU of which to query status
        :return: Boolean, True if PSU is plugged, False if not
        """

        # Init data
        psu_key = "PSU" + str(index)
        psu_presence_key = "Present"
        psu_presence_status = False

        try:
            # Request and validate sensor's information.
            self.fru_status_list, self.psu_info_list = self.request_data()

            # Get PSU present status.
            for fru_status in self.fru_status_list:
                is_psu = fru_status.get(psu_key)
                psu_status = str(fru_status.get(psu_presence_key)).strip()

                if is_psu is not None and psu_status == "Present":
                    psu_presence_status = True

        except:
            print("Error: Unable to access PSU presence status")
            return False

        return psu_presence_status

    def get_psu_sn(self, index):
        """
        Get the serial number of the psu,

        :param index: An integer, 1-based index of the PSU.
        :return: Serial number
        """
        serial_number = "N/A"
        psu_key = "PSU" + str(index) + " FRU"
        psu_sn_key = "Serial Number"

        try:
            # Request and validate sensor's information.
            self.fru_status_list, self.psu_info_list = self.request_data()

            # Get PSU fru info.
            for psu_fru in self.psu_info_list:
                psu_sn = str(psu_fru.get(psu_sn_key)).strip()
                if psu_fru.get(psu_key) is not None:
                    serial_number = psu_sn if psu_sn.strip() != "" else "N/A"
                    break

        except:
            return "N/A"

        return serial_number

    def get_psu_pn(self, index):
        """
        Get the product name of the psu

        :param index: An integer, 1-based index of the PSU.
        :return: Product name
        """
        product_name = "N/A"
        psu_key = "PSU" + str(index) + " FRU"
        psu_pn_key = "Product Name"

        try:
            # Request and validate sensor's information
            self.fru_status_list, self.psu_info_list = self.request_data()

            # Get PSU fru info.
            for psu_fru in self.psu_info_list:
                psu_pn = str(psu_fru.get(psu_pn_key)).strip()
                if psu_fru.get(psu_key) is not None:
                    product_name = psu_pn if psu_pn.strip() != "" else "N/A"
                    break

        except:
            return "N/A"

        return product_name

    def get_all(self):
        """
            Number: mandatory, max number of PSU, integer
            PSU1, PSU2, ...: mandatory, PSU name, string
            Present: mandatory for each PSU, present status, boolean, True for present, False for NOT present
            PowerStatus: conditional, if PRESENT is True, power status of PSU,boolean, True for powered, False for NOT powered
            PN, conditional, if PRESENT is True, PN of the PSU, string
            SN, conditional, if PRESENT is True, SN of the PSU, string
        """
#{
#    "Number": 4,
#    "PSU1": {
#        "AirFlow": "N/A",
#        "FanSpeed": {
#            "Max": 30000,
#            "Min": 1000,
#            "Unit": "RPM",
#            "Value": -99999
#        },
#        "Inputs": {
#            "Current": {
#                "HighAlarm": 7.0,
#                "LowAlarm": 0.0,
#                "Unit": "A",
#                "Value": -99999.0
#            },
#            "Power": {
#                "HighAlarm": 1220.0,
#                "LowAlarm": -1,
#                "Unit": "W",
#                "Value": -99999.0
#            },
#            "Status": false,
#            "Type": "Unknown",
#            "Voltage": {
#                "HighAlarm": 264.0,
#                "LowAlarm": 90.0,
#                "Unit": "V",
#                "Value": -99999.0
#            }
#        },
#        "Outputs": {
#            "Current": {
#                "HighAlarm": 90.0,
#                "LowAlarm": 0.0,
#                "Unit": "A",
#                "Value": -99999.0
#            },
#            "Power": {
#                "HighAlarm": 1100.0,
#                "LowAlarm": -1,
#                "Unit": "W",
#                "Value": -99999.0
#            },
#            "Status": false,
#            "Type": "Unknown",
#            "Voltage": {
#                "HighAlarm": 13.2,
#                "LowAlarm": 10.8,
#                "Unit": "V",
#                "Value": -99999.0
#            }
#        },
#        "PN": "",
#        "SN": "JHZD1849000585",
#        "Temperature": {
#            "Max": 70.0,
#            "Min": 60.0,
#            "Unit": "C",
#            "Value": -99999.0
#        }
#    }
#}
        psu_info = {}
        if not self._get_psu_info():
            return psu_info

        #j = json.dumps(self.psu_info, sort_keys=True, indent=4, separators=(',', ': '))
        #print j

        if "Number" not in self.psu_info:
            return psu_info

        psu_nr = self.psu_info["Number"]
        psu_info["Number"] = psu_nr
        for idx in range(1, psu_nr+1):
            psu_name = "PSU%d" % idx
            if not psu_name in self.psu_info:
                print(("%s not in self.psu_info" % psu_name))
                continue

            pi = self.psu_info[psu_name]
            pinfo = {}
            try:
                pinfo["Present"] = True if pi["Present"] == "yes" else False
                pinfo["AirFlow"] = pi["AirFlow"]
                pinfo["PN"] = pi["PN"]
                pinfo["SN"] = pi["SN"]
            except Exception as e:
                print(("%s not in self.psu_info, exception 1" % psu_name))
                continue

            if "Inputs" in pi:
                pii = pi["Inputs"]
                if "Status" in pii:
                    try:
                        pinfo["InputStatus"] = pii["Status"]
                        pinfo["InputType"] = pii["Type"]
                    except Exception as e:
                        pinfo["InputStatus"] = False
                        pinfo["InputType"] = "N/A"

            if "Outputs" in pi:
                pio = pi["Outputs"]
                if "Status" in pio:
                    try:
                        pinfo["OutputStatus"] = pio["Status"]
                    except Exception as e:
                        pinfo["OutputStatus"] = False

            if "FanSpeed" in pi:
                pif = pi["FanSpeed"]
                try:
                    pinfo["Speed"] = pif["Value"]
                    pinfo["LowThd"] = pif["Min"]
                    pinfo["HighThd"] = pif["Max"]
                except Exception as e:
                    pinfo["Speed"] = 0
                    pinfo["LowThd"] = 0
                    pinfo["HighThd"] = 0

            psu_info[psu_name] = pinfo

        #j = json.dumps(psu_info, sort_keys=True, indent=4, separators=(',', ': '))
        #print j

        return psu_info

