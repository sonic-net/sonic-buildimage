import os
import re
import sys
from unittest.mock import MagicMock, patch

import pytest

from . import common_test

sys.path.append("ctrmgr")
import ctrmgr_iptables


PROXY_FILE="http_proxy.conf"

test_data = {
    "1": {
        "ip": "10.10.20.20",
        "port": "3128",
        "pre_rules": [
            "DNAT tcp -- 20.20.0.0/0 172.16.1.1 tcp dpt:8080 to:100.127.20.21:8080",
            "DNAT tcp -- 0.0.0.0/0 172.16.1.1 tcp dpt:3128 to:11.11.11.11:8080",
            "DNAT tcp -- 0.0.0.0/0 172.16.1.1 tcp dpt:3128 to:11.11.11.11:8080",
            "DNAT tcp -- 0.0.0.0/0 172.16.1.1 tcp dpt:3128 to:11.11.11.11:8088",
            "DNAT tcp -- 0.0.0.0/0 172.16.1.1 tcp dpt:3129 to:11.11.11.11:8088"
        ],
        "post_rules": [
            "DNAT tcp -- 20.20.0.0/0 172.16.1.1 tcp dpt:8080 to:100.127.20.21:8080",
            "DNAT tcp -- 0.0.0.0/0 172.16.1.1 tcp dpt:3129 to:11.11.11.11:8088",
            "DNAT tcp -- 0.0.0.0/0 172.16.1.1 tcp dpt:3128 to:10.10.20.20:3128"
            ],
        "ret": "10.10.20.20:3128"
    },
    "2": {
        "ip": "",
        "port": "",
        "pre_rules": [
            "DNAT tcp -- 20.20.0.0/0 172.16.1.1 tcp dpt:8080 to:100.127.20.21:8080",
            "DNAT tcp -- 0.0.0.0/0 172.16.1.1 tcp dpt:3128 to:11.11.11.11:8080",
            "DNAT tcp -- 0.0.0.0/0 172.16.1.1 tcp dpt:3128 to:11.11.11.11:8080",
            "DNAT tcp -- 0.0.0.0/0 172.16.1.1 tcp dpt:3128 to:11.11.11.11:8088"
        ],
        "post_rules": [
            "DNAT tcp -- 20.20.0.0/0 172.16.1.1 tcp dpt:8080 to:100.127.20.21:8080"
        ],
        "ret": ""
    },
    "3": {
        "ip": "www.google.com",
        "port": "3128",
        "pre_rules": [
            "DNAT tcp -- 20.20.0.0/0 172.16.1.1 tcp dpt:8080 to:100.127.20.21:8080",
            "DNAT tcp -- 0.0.0.0/0 172.16.1.1 tcp dpt:3128 to:11.11.11.11:8080"
        ],
        "post_rules": [
            "DNAT tcp -- 20.20.0.0/0 172.16.1.1 tcp dpt:8080 to:100.127.20.21:8080",
            "DNAT tcp -- 0.0.0.0/0 172.16.1.1 tcp dpt:3128 to:.*3128"
        ]
    },
    "4": {
        "ip": "www.google.comx",
        "port": "3128",
        "conf_file": "no_proxy.conf",
        "pre_rules": [
            "DNAT tcp -- 20.20.0.0/0 172.16.1.1 tcp dpt:8080 to:100.127.20.21:8080",
            "DNAT tcp -- 0.0.0.0/0 172.16.1.1 tcp dpt:3128 to:11.11.11.11:8080"
        ],
        "post_rules": [
            "DNAT tcp -- 20.20.0.0/0 172.16.1.1 tcp dpt:8080 to:100.127.20.21:8080",
            "DNAT tcp -- 0.0.0.0/0 172.16.1.1 tcp dpt:3128 to:11.11.11.11:8080"
        ],
        "ret": ""
    }
}


current_tc = None
current_rules = None

class proc:
    returncode = 0
    stdout = None
    stderr = None

    def __init__(self, ret, stdout, stderr):
        self.returncode = ret
        self.stdout = bytearray(stdout, 'utf-8')
        self.stderr = bytearray(stderr, 'utf-8')
        print("out={} err={}".format(stdout, stderr))


def mock_subproc_run(cmd, shell, capture_output):
    cmd_prefix = "sudo iptables -t nat "
    list_cmd = "{}-n -L OUTPUT ".format(cmd_prefix)
    del_cmd = "{}-D OUTPUT ".format(cmd_prefix)
    ins_cmd = "{}-A OUTPUT -p tcp -d ".format(cmd_prefix)

    assert shell
    
    print("cmd={}".format(cmd))
    if cmd.startswith(list_cmd):
        num = int(cmd[len(list_cmd):])
        out = current_rules[num] if len(current_rules) > num else ""
        return proc(0, out, "")

    if cmd.startswith(del_cmd):
        num = int(cmd[len(del_cmd):])
        if num >= len(current_rules):
            print("delete num={} is greater than len={}".format(num, len(current_rules)))
            print("current_rules = {}".format(current_rules))
            assert False
        del current_rules[num]
        return proc(0, "", "")

    if cmd.startswith(ins_cmd):
        l = cmd.split()
        assert len(l) == 16
        rule = "DNAT tcp -- 0.0.0.0/0 {} tcp dpt:{} to:{}".format(l[9], l[11], l[-1])
        current_rules.append(rule)
        return proc(0, "", "")

    print("unknown cmd: {}".format(cmd))
    return None


def match_rules(pattern_list, str_list):
    if len(pattern_list) != len(str_list):
        print("pattern len {} != given {}".format(
            len(pattern_list), len(str_list)))
        return False

    for i in range(len(pattern_list)):
        if not re.match(pattern_list[i], str_list[i]):
            print("{}: {} != {}".format(i, pattern_list[i], str_list[i]))
            return False
    return True


class TestIPTableUpdate(object):

    @patch("ctrmgr_iptables.subprocess.run")
    def test_table(self, mock_proc):
        global current_rules, current_tc

        mock_proc.side_effect = mock_subproc_run
        for i, tc in test_data.items():
            print("----- Test: {} Start ------------------".format(i))
            current_tc = tc
            current_rules = tc["pre_rules"].copy()

            ctrmgr_iptables.DST_IP = ""
            ctrmgr_iptables.DST_PORT = ""
            ctrmgr_iptables.DST_FILE = os.path.join(
                    os.path.dirname(os.path.realpath(__file__)),
                    tc.get("conf_file", PROXY_FILE))
            ret = ctrmgr_iptables.iptable_proxy_rule_upd(tc["ip"], tc["port"])
            if "ret" in tc:
                assert ret == tc["ret"]
            if not match_rules(tc["post_rules"], current_rules):
                print("current_rules={}".format(current_rules))
                print("post_rules={}".format(tc["post_rules"]))
                assert False
            print("----- Test: {} End   ------------------".format(i))


