#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import sys
import os
import syslog
import click
from platform_util import exec_os_cmd


IPMITOOL_CMD = "ipmitool raw 0x32 0x04"  # All products are the same command

PLATFORM_IPMI_DEBUG_FILE = "/etc/.platform_ipmi_debug_flag"
UPGRADEDEBUG = 1
debuglevel = 0


def debug_init():
    global debuglevel
    if os.path.exists(PLATFORM_IPMI_DEBUG_FILE):
        debuglevel = debuglevel | UPGRADEDEBUG
    else:
        debuglevel = debuglevel & ~(UPGRADEDEBUG)


def ipmidebuglog(s):
    # s = s.decode('utf-8').encode('gb2312')
    if UPGRADEDEBUG & debuglevel:
        syslog.openlog("PLATFORM_IPMI", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_DEBUG, s)


def ipmierror(s):
    # s = s.decode('utf-8').encode('gb2312')
    syslog.openlog("PLATFORM_IPMI", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_ERR, s)


@click.command()
@click.argument('cmd', required=True)
def platform_ipmi_main(cmd):
    '''Send command to BMC through ipmi'''
    try:
        # Convert string command to ASCII
        user_cmd = ""
        for ch in cmd:
            user_cmd += " " + str(ord(ch))

        final_cmd = IPMITOOL_CMD + user_cmd
        ipmidebuglog("final cmd:%s" % final_cmd)

        # exec ipmitool cmd
        status, output = exec_os_cmd(final_cmd)
        if status:
            ipmierror("exec ipmitool_cmd:%s user_cmd:%s failed" % (IPMITOOL_CMD, cmd))
            ipmierror("failed log: %s" % output)
            return False, "exec final_cmd failed"

        # the data read by ipmitool is hex value, needs transformation
        data_list = output.replace("\n", "").strip(' ').split(' ')
        ipmidebuglog("data_list: %s" % data_list)
        result = ""
        for data in data_list:
            result += chr(int(data, 16))

        # 'result' string include ret and log, separated by ,
        result_list = result.split(',', 2)
        if len(result_list) != 2:
            log = "split failed. len(result) != 2. result:%s" % result
            ipmierror(log)
            return False, log
        if int(result_list[0]) != 0:
            ipmierror("finally analy ipmitool_cmd:%s user_cmd:%s exec failed" % (IPMITOOL_CMD, cmd))
            ipmierror("failed return log: %s" % result_list[1])
            print(result_list[1])
            return False, result_list[1]

        ipmidebuglog("finally exec ipmitool_cmd:%s user_cmd:%s success" % (IPMITOOL_CMD, cmd))
        print(result_list[1])
        return True, result_list[1]

    except Exception as e:
        log = "An exception occurred, exception log:%s" % str(e)
        ipmierror(log)
        return False, log


if __name__ == '__main__':
    debug_init()
    ret, msg = platform_ipmi_main()
    if ret is False:
        sys.exit(1)
    sys.exit(0)
