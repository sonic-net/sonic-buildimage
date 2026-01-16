# -*- coding:utf-8
from pit_util_common import run_command
import re

CMD_SHOW_PLATFORM = "show platform"

class BaseUtil(object):
    def __init__(self):
        pass
    def get_cpu_mac(self):
        cmd = "ifconfig eth0|grep -w ether|awk '{print $2}'"
        status, out = run_command(cmd)
        if status != 0 or len(out) <= 0:
            self.fail_reason.append("get cpu mac error.")
            return "N/A"
        else:
            return out

    def get_base_mac(self):
        cmd = " ".join([CMD_SHOW_PLATFORM, "syseeprom"])
        regex_int = re.compile(r'([\S\s]+)(0x[A-F0-9]+)\s+([\d]+)\s+([\S\s]*)')
        status, out = run_command(cmd)
        parsed_syseeprom = {}
        if status != 0 or len(out) <= 0:
            self.fail_reason.append("get base mac error.")
            return "N/A"
        else:
            for line in out.splitlines():
                if line.find("Base MAC Address") >= 0:
                    t1 = regex_int.match(line)
                    if t1:
                        base_mac = t1.group(4).strip()
                        return base_mac
