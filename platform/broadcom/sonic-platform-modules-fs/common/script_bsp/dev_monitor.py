#!/usr/bin/env python3
import sys
import os
import time
import syslog
import traceback
import click
import logging
from platform_config import DEV_MONITOR_PARAM
from platform_util import get_value, set_value, setup_logger, BSP_COMMON_LOG_DIR, waitForDocker
from public.platform_common_config import SDKCHECK_PARAMS


CONTEXT_SETTINGS = {"help_option_names": ['-h', '--help']}


class AliasedGroup(click.Group):
    def get_command(self, ctx, cmd_name):
        rv = click.Group.get_command(self, ctx, cmd_name)
        if rv is not None:
            return rv
        matches = [x for x in self.list_commands(ctx)
                   if x.startswith(cmd_name)]
        if not matches:
            return None
        if len(matches) == 1:
            return click.Group.get_command(self, ctx, matches[0])
        ctx.fail('Too many matches: %s' % ', '.join(sorted(matches)))
        return None

DEBUG_FILE = "/etc/.devmonitor_debug_flag"
LOG_FILE = BSP_COMMON_LOG_DIR + "dev_monitor_debug.log"
logger = setup_logger(LOG_FILE)

def debug_init():
    if os.path.exists(DEBUG_FILE):
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

def deverror(s):
    # s = s.decode('utf-8').encode('gb2312')
    logger.error(s)

def devinfo(s):
    # s = s.decode('utf-8').encode('gb2312')
    logger.info(s)

def devdebuglog(s):
    logger.debug(s)


class DevMonitor():

    def getpresentstatus(self, param):
        try:
            ret = {}
            ret["status"] = ''

            ret_t, val = get_value(param)
            if ret_t is False:
                ret["status"] = "NOT OK"
                devdebuglog("get present status failed, param: %s, log: %s" % (param, val))
                return ret

            presentbit = param.get('presentbit', 0)
            okval = param.get('okval')
            val_t = (val & (1 << presentbit)) >> presentbit
            if val_t != okval:
                ret["status"] = "ABSENT"
            else:
                ret["status"] = "PRESENT"
        except Exception as e:
            ret["status"] = "NOT OK"
            deverror("getpresentstatus error, msg: %s" % (traceback.format_exc()))
        return ret

    def removeDev(self, bus, loc):
        cmd = "echo  0x%02x > /sys/bus/i2c/devices/i2c-%d/delete_device" % (loc, bus)
        devpath = "/sys/bus/i2c/devices/%d-%04x" % (bus, loc)
        if os.path.exists(devpath):
            os.system(cmd)

    def addDev(self, name, bus, loc):
        if name == "lm75":
            time.sleep(0.1)
        cmd = "echo  %s 0x%02x > /sys/bus/i2c/devices/i2c-%d/new_device" % (name, loc, bus)
        devpath = "/sys/bus/i2c/devices/%d-%04x" % (bus, loc)
        if os.path.exists(devpath) is False:
            os.system(cmd)

    def checkattr(self, bus, loc, attr):
        try:
            attrpath = "/sys/bus/i2c/devices/%d-%04x/%s" % (bus, loc, attr)
            if os.path.exists(attrpath):
                return True
        except Exception as e:
            deverror("checkattr error")
            deverror(str(traceback.format_exc()))
        return False

    def register_bind_dev_again(self, device_name, driver_name, driver_type):
        action = {"gettype": "cmd", "cmd" : "echo %s > /sys/bus/%s/drivers/%s/bind"}
        action["cmd"] = action["cmd"] % (device_name, driver_type, driver_name)

        ret, msg = set_value(action)
        if ret is False:
            devdebuglog("%s register_bind_dev_again fail, reason: %s" % (device_name, msg))
        else:
            devdebuglog("%s register_bind_dev_again  success" % device_name)

        return ret

    def do_bind_dev_pre_check(self, device_name, driver_name, driver_type):
        bind_dev_pre_check_cmd = {
            "cmd": "echo %s > /sys/bus/%s/drivers/%s/unbind",
            "gettype": "cmd",
            "pre_check": {
                "gettype": "file_exist",
                "judge_file": "/sys/bus/%s/drivers/%s/%s",
                "okval": True
            }
        }
        bind_dev_pre_check_cmd["cmd"] = bind_dev_pre_check_cmd["cmd"] % (device_name, driver_type, driver_name)
        bind_dev_pre_check_cmd["pre_check"]["judge_file"] = bind_dev_pre_check_cmd["pre_check"]["judge_file"]  % (driver_type, driver_name, device_name)

        ret, msg = set_value(bind_dev_pre_check_cmd)
        if ret is False:
            devdebuglog("%s do_bind_dev_pre_check fail, reason: %s" % (device_name, msg))
        else:
            devdebuglog("%s do_bind_dev_pre_check  success" % device_name)

        return ret

    def register_device_again(self, device_name, bus, loc, driver_name, driver_type):
        if driver_type:
            ret = self.do_bind_dev_pre_check(device_name, driver_name, driver_type)
            if ret is False:
                return

            self.register_bind_dev_again(device_name, driver_name, driver_type)
        else:
            self.removeDev(bus, loc)
            time.sleep(0.1)
            self.addDev(device_name, bus, loc)

    def monitor(self, ret):
        totalerr = 0
        for item in ret:
            try:
                name = item.get('name')
                driver_type = item.get('driver_type')
                itemattr = '%sattr' % name
                val_t = getattr(DevMonitor, itemattr, None)
                if val_t == 'OK':
                    continue
                present = item.get('present', None)
                devices = item.get('device')
                err_t = 0
                for item_dev in devices:
                    item_devattr = '%s' % (item_dev['id'])
                    val_t = getattr(DevMonitor, item_devattr, None)
                    if val_t == 'OK':
                        continue
                    devname = item_dev.get('name')
                    bus = item_dev.get('bus')
                    loc = item_dev.get('loc')
                    attr = item_dev.get('attr')
                    if self.checkattr(bus, loc, attr) is False:
                        err_t -= 1
                        setattr(DevMonitor, item_devattr, 'NOT OK')
                        if present is not None:
                            presentstatus = self.getpresentstatus(present)
                            devdebuglog("%s present status:%s" % (name, presentstatus.get('status')))
                            if presentstatus.get('status') == 'PRESENT':
                                self.register_device_again(devname, bus, loc, name, driver_type)
                        else:
                            self.register_device_again(devname, bus, loc, name, driver_type)
                    else:
                        setattr(DevMonitor, item_devattr, 'OK')
                    val_t = getattr(DevMonitor, item_devattr, None)
                    devdebuglog("%s status %s" % (item_devattr, val_t))
                if err_t == 0:
                    setattr(DevMonitor, itemattr, 'OK')
                else:
                    totalerr -= 1
                    setattr(DevMonitor, itemattr, 'NOT OK')
                val_t = getattr(DevMonitor, itemattr, None)
                devdebuglog("%s status %s" % (itemattr, val_t))
            except Exception as e:
                totalerr -= 1
                deverror("monitor error")
                deverror(traceback.format_exc())
        return totalerr

    def psusmonitor(self):
        psus_conf = DEV_MONITOR_PARAM.get('psus')
        if psus_conf is None:
            return 0
        psusattr = 'psusattr'
        val_t = getattr(DevMonitor, psusattr, None)
        if val_t == 'OK':
            return 0
        ret = self.monitor(psus_conf)
        if ret == 0:
            setattr(DevMonitor, psusattr, 'OK')
        else:
            setattr(DevMonitor, psusattr, 'NOT OK')
        val_t = getattr(DevMonitor, psusattr, None)
        devdebuglog("psusattr:value:%s" % (val_t))
        return ret

    def fansmonitor(self):
        fans_conf = DEV_MONITOR_PARAM.get('fans')
        if fans_conf is None:
            return 0
        fansattr = 'fansattr'
        val_t = getattr(DevMonitor, fansattr, None)
        if val_t == 'OK':
            return 0
        ret = self.monitor(fans_conf)
        if ret == 0:
            setattr(DevMonitor, fansattr, 'OK')
        else:
            setattr(DevMonitor, fansattr, 'NOT OK')
        val_t = getattr(DevMonitor, fansattr, None)
        devdebuglog("fansattr:value:%s" % (val_t))
        return ret

    def slotsmonitor(self):
        slots_conf = DEV_MONITOR_PARAM.get('slots')
        if slots_conf is None:
            return 0
        slotsattr = 'slotsattr'
        val_t = getattr(DevMonitor, slotsattr, None)
        if val_t == 'OK':
            return 0
        ret = self.monitor(slots_conf)
        if ret == 0:
            setattr(DevMonitor, slotsattr, 'OK')
        else:
            setattr(DevMonitor, slotsattr, 'NOT OK')
        val_t = getattr(DevMonitor, slotsattr, None)
        devdebuglog("slotsattr:value:%s" % (val_t))
        return ret

    def binddevsmonitor(self):
        binddevs_conf = DEV_MONITOR_PARAM.get('binddevs')
        if binddevs_conf is None:
            return 0
        binddevsattr = 'binddevsattr'
        val_t = getattr(DevMonitor, binddevsattr, None)
        if val_t == 'OK':
            return 0
        ret = self.monitor(binddevs_conf)
        if ret == 0:
            setattr(DevMonitor, binddevsattr, 'OK')
        else:
            setattr(DevMonitor, binddevsattr, 'NOT OK')
        val_t = getattr(DevMonitor, binddevsattr, None)
        devdebuglog("binddevsattr:value:%s" % (val_t))
        return ret

    def othersmonitor(self):
        others_conf = DEV_MONITOR_PARAM.get('others')
        if others_conf is None:
            return 0
        othersattr = 'othersattr'
        val_t = getattr(DevMonitor, othersattr, None)
        if val_t == 'OK':
            return 0
        ret = self.monitor(others_conf)
        if ret == 0:
            setattr(DevMonitor, othersattr, 'OK')
        else:
            setattr(DevMonitor, othersattr, 'NOT OK')
        val_t = getattr(DevMonitor, othersattr, None)
        devdebuglog("othersattr:value:%s" % (val_t))
        return ret

    def sdkmonitor(self):
        sdk_conf = DEV_MONITOR_PARAM.get('sdk')
        ret = 0
        if sdk_conf is None:
            return ret
        sdkattr = 'sdkattr'
        val_t = getattr(DevMonitor, sdkattr, None)
        if val_t == 'OK':
            return ret
        if waitForDocker(SDKCHECK_PARAMS, timeout=0) == True:
            setattr(DevMonitor, sdkattr, 'OK')
            acts = sdk_conf.get("act", [])
            for item in acts:
                res, log = set_value(item)
                if not res:
                    setattr(DevMonitor, sdkattr, 'NOT OK')
                    devdebuglog("deal sdk monitor items error:%s" % log)
        else:
            ret = -1
            setattr(DevMonitor, sdkattr, 'NOT OK')
        val_t = getattr(DevMonitor, sdkattr, None)
        devdebuglog("sdkattr:value:%s" % (val_t))
        return ret

def doDevMonitor(devMonitor):
    ret_t = 0
    ret_t += devMonitor.psusmonitor()
    ret_t += devMonitor.fansmonitor()
    ret_t += devMonitor.slotsmonitor()
    ret_t += devMonitor.othersmonitor()
    ret_t += devMonitor.binddevsmonitor()
    ret_t += devMonitor.sdkmonitor()
    return ret_t


def run(interval, devMonitor):
    # devMonitor.devattrinit()
    while True:
        try:
            debug_init()
            ret = doDevMonitor(devMonitor)
        except Exception as e:
            traceback.print_exc()
            deverror(traceback.format_exc())
            ret = -1
        if ret == 0:
            time.sleep(5)
            devinfo("dev_monitor finished!")
            sys.exit(0)
        time.sleep(interval)


@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''device operator'''
    debug_init()

@main.command()
def start():
    '''start device monitor'''
    devinfo("dev_monitor start")
    devMonitor = DevMonitor()
    interval = DEV_MONITOR_PARAM.get('polling_time', 10)
    run(interval, devMonitor)


@main.command()
def stop():
    '''stop device monitor '''
    devinfo("stop")


# device_i2c operation
if __name__ == '__main__':
    main()
