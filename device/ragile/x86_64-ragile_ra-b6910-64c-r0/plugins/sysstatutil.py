import os.path
import sys
sys.path.append('/usr/share/sonic/platform/plugins')
import pddfparse
import json


class SYSStatusUtil():
    """Platform-specific SYSStatus class"""

    def __init__(self):
        global pddf_obj
        global plugin_data
        with open(os.path.join(os.path.dirname(os.path.realpath(__file__)) + '/../pddf/pd-plugin.json')) as pd:
            plugin_data = json.load(pd)

        pddf_obj = pddfparse.PddfParse()

    def get_board_info(self):
        device = "SYSSTATUS"
        node = pddf_obj.get_path(device, "board_info")
        if node is None:
            return False
        try:
            with open(node, 'r') as f:
                status = f.read()
                print("board_info : %s" % status)
        except IOError:
            return False

    def get_cpld_versio(self):
        device = "SYSSTATUS"
        node = pddf_obj.get_path(device, "cpld1_version")
        if node is None:
            return False
        try:
            with open(node, 'r') as f:
                status = f.read()
                print("cpld1_version : %s" % status)
        except IOError:
            return False

    def get_power_module_status(self):
        device = "SYSSTATUS"
        node = pddf_obj.get_path(device, "power_module_status")
        if node is None:
            return False
        try:
            with open(node, 'r') as f:
                status = f.read()
                print("power_module_status : %s" % status)
        except IOError:
            return False

    def get_system_reset_status(self):
        device = "SYSSTATUS"
        for i in range(1, 8):
            node = pddf_obj.get_path(device, "system_reset"+str(i))
            if node is None:
                return False
            try:
                with open(node, 'r') as f:
                    status = f.read()
                    print("system_reset%s : %s" % (i, status))
            except IOError:
                print("system_reset%s not supported" % i)

    def get_misc_status(self):
        device = "SYSSTATUS"
        for i in range(1, 3):
            node = pddf_obj.get_path(device, "misc"+str(i))
            if node is None:
                return False
            try:
                with open(node, 'r') as f:
                    status = f.read()
                    print("misc%s : %s" % (i, status))
            except IOError:
                print("system_reset%s not supported" % i)

    def dump_sysfs(self):
        return pddf_obj.cli_dump_dsysfs('sys-status')
