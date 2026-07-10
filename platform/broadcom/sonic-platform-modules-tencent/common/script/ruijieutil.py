#!/usr/bin/python3
# -------------------------------------------------------------------------
#
# Author:      sonic_rd
#
# Created:     02/07/2018
# Copyright:   2001-2022 Ruijie Network. All rights reserved.
# -------------------------------------------------------------------------
import sys
import os
import re
import time
from ruijieconfig import STARTMODULE
from platform_util import rj_os_system


def getSdkReg(reg):
    try:
        cmd = "bcmcmd -t 1 'getr %s ' < /dev/null" % reg
        ret, result = rj_os_system(cmd)
        result_t = result.strip().replace("\r", "").replace("\n", "")
        if ret != 0 or "Error:" in result_t:
            return False, result
        patt = r"%s.(.*):(.*)>drivshell" % reg
        rt = re.findall(patt, result_t, re.S)
        test = re.findall("=(.*)", rt[0][0])[0]
    except Exception as e:
        return False, 'getsdk register error'
    return True, test


def waitForDhcp(timeout):
    time_cnt = 0
    while True:
        try:
            ret, status = rj_os_system("systemctl status dhcp_relay.service")
            if (ret == 0 and "running" in status)  or "SUCCESS" in status:
                break
            else:
                sys.stdout.write(".")
                sys.stdout.flush()
                time_cnt = time_cnt + 1
                if time_cnt > timeout:
                    raise Exception("waitForDhcp timeout")
                time.sleep(1)
        except Exception as e:
            return False
    return True

def waitForSdk(sdk_fpath ,timeout):
    time_cnt = 0
    while True:
        try:
            if os.path.exists(sdk_fpath):
                break
            else:
                #sys.stdout.write(".")
                #sys.stdout.flush()
                time_cnt = time_cnt + 1
                if time_cnt > timeout:
                    raise Exception("waitForSdk timeout")
                time.sleep(1)
        except Exception as e:
            return False
    return True

def waitForDocker(need_restart=False,timeout=180):
    sdkcheck_params = STARTMODULE.get("sdkcheck",{})
    if sdkcheck_params.get("checktype") == "file":
        sdk_fpath = sdkcheck_params.get("sdk_fpath")
        return waitForSdk(sdk_fpath,timeout)
    return waitForDhcp(timeout)
