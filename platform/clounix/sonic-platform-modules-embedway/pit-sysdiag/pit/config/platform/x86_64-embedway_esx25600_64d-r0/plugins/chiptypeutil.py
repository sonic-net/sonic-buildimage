# -*- coding:utf-8
from pit_util_common import run_command
from test_case import TestCaseCommon
from errcode import E
import re
import json
import os


class ChipTypeUtil(object):
    def __init__(self):
        self.s3ip_path = "/sys_switch"

    def get_board_type(self):
        board_list = {
                "0x1023":"ESX25600-64D-1VB",
        }
        cmd = "cat /sys/kernel/pddf/devices/sysstatus/sysstatus_data/fpga_board_version"
        status, output = run_command(cmd)
        board_code = output.strip()
        for key, value in board_list.items():
            if key == board_code:
                return value
        return "N/A"
    
    def get_cpld_type(self):
        cpld_type = {}
        status, output = run_command("cat /sys_switch/cpld/number")
        print("platform have %s cpld(s)" % output)
        for i in range(1, int(output)+1):
            cmd = "cat "+ self.s3ip_path + "/cpld/cpld{}/type".format(i)
            status, output = run_command(cmd)
            cpld_type["CPLD{}".format(i)] = output

        return cpld_type

    def get_fpga_type(self):
        cmd = "cat "+ self.s3ip_path + "/fpga/fpga1/type"
        status, output = run_command(cmd)
        fpga_running_type = output.strip()

        return fpga_running_type

