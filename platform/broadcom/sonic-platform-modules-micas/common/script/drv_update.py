#!/usr/bin/env python
#
# Copyright (C) 2024 Micas Networks Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import syslog
import os
import shutil
from platform_config import DRVIER_UPDATE_CONF
from platform_util import exec_os_cmd


DRV_UPDATE_DEBUG_FILE = "/etc/.drv_update_debug_flag"

DRVUPDATEERROR = 1
DRVUPDATEDEBUG = 2
debuglevel = 0


def drv_update_debug(s):
    if DRVUPDATEDEBUG & debuglevel:
        syslog.openlog("DRV_UPDATE", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_INFO, s)


def drv_update_error(s):
    if DRVUPDATEERROR & debuglevel:
        syslog.openlog("DRV_UPDATE", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_ERR, s)


def drv_update_info(s):
    syslog.openlog("DRV_UPDATE", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_LOCAL1 | syslog.LOG_NOTICE, s)

def debug_init():
    global debuglevel
    try:
        with open(DRV_UPDATE_DEBUG_FILE, "r") as fd:
            value = fd.read()
        debuglevel = int(value)
    except Exception:
        debuglevel = 0


def get_driver_md5sum(drv_path):
    status, output = exec_os_cmd("md5sum %s" % drv_path)
    if status or len(output) == 0:
        return False, output
    drv_md5 = output.strip().split(" ")[0]
    return True, drv_md5


def do_driver_replace(src_file, target_file):
    # Backup target file
    src_file_dir = os.path.dirname(src_file)
    target_file_name = os.path.basename(target_file)
    drv_update_debug("src_file: %s, src_file_dir: %s" % (src_file, src_file_dir))
    drv_update_debug("target_file: %s, target_file_name: %s" % (target_file, target_file_name))
    try:
        shutil.copyfile(target_file, "%s/%s.bak" % (src_file_dir, target_file_name))
        shutil.copyfile(src_file, target_file)
        return True
    except Exception as e:
        drv_update_error("do_driver_replace error, msg: %s" % str(e))
        return False


def doDrvUpdate():
    reboot_flag = DRVIER_UPDATE_CONF.get("reboot_flag", 0)
    drv_list = DRVIER_UPDATE_CONF.get("drv_list", [])
    err_cnt = 0
    update_initramfs_flag = 0
    # get kernel version
    status, output = exec_os_cmd("uname -r")
    if status or len(output) == 0:
        drv_update_error("Failed to get kernel version, status: %s, log: %s" % (status, output))
        return
    kversion = output.strip()
    drv_update_debug("kernel version: %s" % kversion)
    for item in drv_list:
        try:
            source_drv = item.get("source")
            target_drv = item.get("target")
            judge_flag = item.get("judge_flag")
            if source_drv is None or target_drv is None or judge_flag is None:
                drv_update_error("driver update config error, source_drv: %s, target_drv: %s, judge_file: %s" % (source_drv, target_drv, judge_flag))
                err_cnt += 1
                continue
            drv_update_debug("source_drv: %s, target_drv: %s, judge_flag: %s" % (source_drv, target_drv, judge_flag))

            # Check if the current driver is expected
            if os.path.exists(judge_flag):
                drv_update_debug("The current driver is expected, do nothing")
                continue

            # get source driver file path
            source_drv_path = "/lib/modules/%s/%s" % (kversion, source_drv)
            drv_update_debug("source driver: %s, file path: %s" % (source_drv, source_drv_path))

            # get target driver file path
            target_drv_path = "/lib/modules/%s/%s" % (kversion, target_drv)
            drv_update_debug("target driver: %s, file path: %s" % (target_drv, target_drv_path))

            # get source driver md5sum
            status, source_drv_md5 = get_driver_md5sum(source_drv_path)
            if status is False:
                msg = "get %s md5sum failed msg: %s" % (source_drv_path, source_drv_md5)
                drv_update_error(msg)
                err_cnt += 1
                continue
            drv_update_debug("source driver file path: %s, md5sum: %s" % (source_drv_path, source_drv_md5))

            # get target driver md5sum
            status, target_drv_md5 = get_driver_md5sum(target_drv_path)
            if status is False:
                msg = "get %s md5sum failed msg: %s" % (target_drv_path, target_drv_md5)
                drv_update_error(msg)
                err_cnt += 1
                continue
            drv_update_debug("target driver file path: %s, md5sum: %s" % (target_drv_path, target_drv_md5))

            if source_drv_md5 != target_drv_md5:
                drv_update_debug("source_drv_md5 not equal to target_drv_md5, try to use source driver replace target driver")
                status = do_driver_replace(source_drv_path, target_drv_path)
                if status is False:
                    err_cnt += 1
                    continue
            else:
                drv_update_debug("source_drv_md5 equal to target_drv_md5, do nothing")

            drv_update_debug("Driver replacement completed, set update_initramfs_flag")
            update_initramfs_flag = 1

        except Exception as e:
            err_cnt += 1
            drv_update_error(str(e))

    if update_initramfs_flag == 1:
        drv_update_debug("starting to update initramfs")
        exec_os_cmd("update-initramfs -u")
        drv_update_debug("update initramfs finish")

    exec_os_cmd("sync")
    if update_initramfs_flag == 1 and err_cnt == 0 and reboot_flag == 1:
        reboot_log = "%DRV_UPDATE-5-REBOOT: Update initramfs is completed, restarting the system to take effect."
        reboot_log_cmd = "echo '%s' > /dev/ttyS0" % reboot_log
        exec_os_cmd(reboot_log_cmd)
        drv_update_info(reboot_log)
        exec_os_cmd("/sbin/reboot")
    return

if __name__ == '__main__':
    debug_init()
    doDrvUpdate()
