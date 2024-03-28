#!/usr/bin/env python3
import sys
import os
import time
import syslog
import traceback
import click
from platform_config import DEV_MONITOR_PARAM
from platform_util import io_rd, wbi2cget


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


DEVMONITOR_DEBUG_FILE = "/etc/.devmonitor_debug_flag"

debuglevel = 0


def debug_init():
    global debuglevel
    if os.path.exists(DEVMONITOR_DEBUG_FILE):
        debuglevel = 1
    else:
        debuglevel = 0


def devwarninglog(s):
    # s = s.decode('utf-8').encode('gb2312')
    syslog.openlog("DEVMONITOR", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_WARNING, s)


def devcriticallog(s):
    # s = s.decode('utf-8').encode('gb2312')
    syslog.openlog("DEVMONITOR", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_CRIT, s)


def deverror(s):
    # s = s.decode('utf-8').encode('gb2312')
    syslog.openlog("DEVMONITOR", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_ERR, s)


def devinfo(s):
    # s = s.decode('utf-8').encode('gb2312')
    syslog.openlog("DEVMONITOR", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_INFO, s)


def devdebuglog(s):
    # s = s.decode('utf-8').encode('gb2312')
    if debuglevel == 1:
        syslog.openlog("DEVMONITOR", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_DEBUG, s)


class DevMonitor():

    def getpresentstatus(self, param):
        try:
            ret = {}
            ret["status"] = ''
            gettype = param.get('gettype')
            presentbit = param.get('presentbit')
            okval = param.get('okval')
            if gettype == "io":
                io_addr = param.get('io_addr')
                val = io_rd(io_addr)
                if val is None:
                    ret["status"] = "NOT OK"
                    return ret
                retval = val
            else:
                bus = param.get('bus')
                loc = param.get('loc')
                offset = param.get('offset')
                ind, val = wbi2cget(bus, loc, offset)
                if ind is not True:
                    ret["status"] = "NOT OK"
                    return ret
                retval = val
            val_t = (int(retval, 16) & (1 << presentbit)) >> presentbit
            if val_t != okval:
                ret["status"] = "ABSENT"
            else:
                ret["status"] = "PRESENT"
        except Exception as e:
            ret["status"] = "NOT OK"
            deverror("getpresentstatus error")
            deverror(str(e))
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
            deverror(str(e))
        return False

    def monitor(self, ret):
        totalerr = 0
        for item in ret:
            try:
                name = item.get('name')
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
                                self.removeDev(bus, loc)
                                time.sleep(0.1)
                                self.addDev(devname, bus, loc)
                        else:
                            self.removeDev(bus, loc)
                            time.sleep(0.1)
                            self.addDev(devname, bus, loc)
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
                deverror(str(e))
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


def doDevMonitor(devMonitor):
    ret_t = 0
    ret_t += devMonitor.psusmonitor()
    ret_t += devMonitor.fansmonitor()
    ret_t += devMonitor.slotsmonitor()
    ret_t += devMonitor.othersmonitor()
    return ret_t


def run(interval, devMonitor):
    # devMonitor.devattrinit()
    while True:
        try:
            debug_init()
            ret = doDevMonitor(devMonitor)
        except Exception as e:
            traceback.print_exc()
            deverror(str(e))
            ret = -1
        if ret == 0:
            time.sleep(5)
            devinfo("dev_monitor finished!")
            sys.exit(0)
        time.sleep(interval)


@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''device operator'''


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
