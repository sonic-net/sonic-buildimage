# -*- coding:utf-8
from pit_util_common import run_command
from test_case import TestCaseCommon
from errcode import E


MGMT_INTERFACE = "eth0"
CMD_ETHTOOL = "ethtool"

def get_interface_speed():
    cmd = "cat /sys/class/net/eth0/speed"
    status, out = run_command(cmd)
    if status != 0 or len(out) <= 0:
        self.fail_reason.append("get interface link speed error.")
        return "-1"
    else:
        return out.strip()

def get_interface_link_status():
    cmd = "cat /sys/class/net/eth0/carrier"
    status, out = run_command(cmd)
    if status != 0 or len(out) <= 0:
        self.fail_reason.append("get interface link status error.")
        return "-1"
    else:
        if out.strip() == "1":
            return True
        else:
            return False

def get_interface_duplex():
    cmd = "cat /sys/class/net/eth0/duplex"
    status, out = run_command(cmd)
    if status != 0 or len(out) <= 0:
        self.fail_reason.append("get interface duplex error.")
        return "-1"
    else:
        return out.strip()       

def get_interface_speed():
    cmd = "cat /sys/class/net/eth0/speed"
    status, out = run_command(cmd)
    if status != 0 or len(out) <= 0:
        self.fail_reason.append("get interface link speed error.")
        return "-1"
    else:
        return out.strip()

class MgmtPortUtil(object):
    def __init__(self):
        pass

    def get_all(self):
        """
                Retrieves the management port running status
                @return: DICT
        """
        mgmt_info = {}
        mgmt_info["Speed"] = get_interface_speed()
        mgmt_info["Duplex"] = get_interface_duplex()
        mgmt_info["Link"] = get_interface_link_status()
        return mgmt_info