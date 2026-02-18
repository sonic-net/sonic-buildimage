#!/usr/bin/python
# -*- coding: UTF-8 -*-

import os
import imp
import time
import syslog
import subprocess


SYSLOG_IDENTIFIER = "NETCARD_STATUS"

def log_info(msg):
    try:
        syslog.openlog(SYSLOG_IDENTIFIER)
        syslog.syslog(syslog.LOG_INFO, msg)
        syslog.closelog()
    except Exception as e:
        msg = traceback.format_exc()
        print("run NETCARD_STATUS error:%s"%msg)
        syslog.closelog()

def log_os_system(cmd):
    log_info ('         Run :'+ cmd)
    status, output = subprocess.getstatusoutput(cmd)
    log_info (" with result :" + str(status))
    log_info ("      output :" + output)
    return  status, output

def main():
    log_os_system("ifconfig usb0 up")
    cmd = "grep ac51-48c2g /host/machine.conf"
    status, output = subprocess.getstatusoutput(cmd)
    if status:
        eth_list = ["eth0","lo"]
    else:
        eth_list = ["eth0", "eth1","eth2","eth3","eth4","lo"]

    for f in eth_list:
        times = 0
        retry_times = 60
        while times < retry_times:
            cmd = "ifconfig %s" % f
            ret, log = log_os_system(cmd)
            if ret != 0:
                log_info("set %s fail,ret:%d,log:%s" %(cmd,ret,log))
                time.sleep(1)
                times = times + 1
                continue
            # If all mac addresses are 0, one mac address is added by default
            if "00:00:00:00:00:00" in log:
                cmd = "ifconfig %s hw ether 00:10:94:01:12:34" % f
                ret_mac, log_mac = log_os_system(cmd)
                if ret_mac != 0:
                    log_info("set  %s mac address fail,ret:%d,log:%s" %(f,ret_mac,log_mac))
                    time.sleep(1)
                    times = times + 1
                    continue
            if "UP" in log:
                break
            cmd = "ifconfig %s up" % f
            ret, log = log_os_system(cmd)
            time.sleep(1)
            times = times + 1
        if times == retry_times:
            log_info("devcie %s disable,ret:%d,log:%s" %(f,ret,log))

if __name__ == '__main__':
    main()
