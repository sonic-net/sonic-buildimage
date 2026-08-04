#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
import time
import syslog
import traceback
import operator
import click
from platform_config import SLOT_MONITOR_PARAM, MONITOR_DEV_STATUS_DECODE
from platform_util import io_rd, io_wr, wbi2cget, wbi2cset


SLOTMONITORDEBUG = 0

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


def slotwarninglog(s):
    # s = s.decode('utf-8').encode('gb2312')
    syslog.openlog("SLOTMONITOR", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_WARNING, s)


def slotcriticallog(s):
    # s = s.decode('utf-8').encode('gb2312')
    syslog.openlog("SLOTMONITOR", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_CRIT, s)


def sloterror(s):
    # s = s.decode('utf-8').encode('gb2312')
    syslog.openlog("SLOTMONITOR", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_ERR, s)


def slotinfo(s):
    # s = s.decode('utf-8').encode('gb2312')
    syslog.openlog("SLOTMONITOR", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_INFO, s)


def slotdebuglog(s):
    # s = s.decode('utf-8').encode('gb2312')
    if SLOTMONITORDEBUG == 1:
        syslog.openlog("SLOTMONITOR", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_DEBUG, s)


class SlotMonitor():
    def __init__(self):
        self.preSlotStatus = []

    def checkslot(self, ret):
        slots_conf = SLOT_MONITOR_PARAM.get('slots', None)
        slotpresent = MONITOR_DEV_STATUS_DECODE.get('slotpresent', None)

        if slots_conf is None or slotpresent is None:
            return False
        for item_slot in slots_conf:
            totalerr = 0
            try:
                ret_t = {}
                ret_t["id"] = item_slot.get('name')
                ret_t["status"] = ""
                presentattr = item_slot.get('present')
                gettype = presentattr.get('gettype')
                presentbit = presentattr.get('presentbit')
                if gettype == "io":
                    io_addr = presentattr.get('io_addr')
                    val = io_rd(io_addr)
                    if val is not None:
                        retval = val
                    else:
                        totalerr -= 1
                        sloterror(" %s  %s" % (item_slot.get('name'), "lpc read failed"))
                else:
                    bus = presentattr.get('bus')
                    loc = presentattr.get('loc')
                    offset = presentattr.get('offset')
                    ind, val = wbi2cget(bus, loc, offset)
                    if ind is True:
                        retval = val
                    else:
                        totalerr -= 1
                        sloterror(" %s  %s" % (item_slot.get('name'), "i2c read failed"))
                if totalerr < 0:
                    ret_t["status"] = "NOT OK"
                    ret.append(ret_t)
                    continue
                val_t = (int(retval, 16) & (1 << presentbit)) >> presentbit
                slotdebuglog("%s present:%s" % (item_slot.get('name'), slotpresent.get(val_t)))
                if val_t != slotpresent.get('okval'):
                    ret_t["status"] = "ABSENT"
                else:
                    ret_t["status"] = "PRESENT"
            except Exception as e:
                ret_t["status"] = "NOT OK"
                totalerr -= 1
                sloterror("checkslot error")
                sloterror(str(e))
            ret.append(ret_t)
        return True

    def dealslotplugin(self, name):
        slotdebuglog("enter dealslotplugin %s" % name)
        # wait for slot stable
        time.sleep(5)
        slots_conf = SLOT_MONITOR_PARAM.get('slots', None)
        if slots_conf is None:
            return False
        for item_slot in slots_conf:
            try:
                slotdebuglog("name %s, item_slot.get('name') %s" % (name, item_slot.get('name')))
                if name == item_slot.get('name'):
                    actattr = item_slot.get('act')
                    for item_act in actattr:
                        gettype = item_act.get('gettype')
                        if gettype == "io":
                            io_addr = item_act.get('io_addr')
                            value = item_act.get('value')
                            mask = item_act.get('mask')
                            val = io_rd(io_addr)
                            if val is None:
                                sloterror(" %s  %s" % (name, "lpc read failed"))
                                continue
                            set_val = (int(val, 16) & mask) | value
                            ret = io_wr(io_addr, set_val)
                            if ret is not True:
                                sloterror(" %s %s" % (name, "lpc write failed"))
                                continue
                            slotdebuglog("io set io_addr:0x%x value:0x%x success" % (io_addr, set_val))
                        elif gettype == "i2c":
                            bus = item_act.get('bus')
                            loc = item_act.get('loc')
                            offset = item_act.get('offset')
                            value = item_act.get('value')
                            ret, log = wbi2cset(bus, loc, offset, value)
                            if ret is not True:
                                sloterror(" %s %s %s" % (name, "i2c write failed", log))
                                continue
                            slotdebuglog(
                                "i2c set bus:%d loc:0x%x offset:0x%x value:0x%x success" %
                                (bus, loc, offset, value))
                        else:
                            sloterror("gettype error")
                    break
            except Exception as e:
                sloterror("dealslotplugin failed")
                sloterror(str(e))
                return False
        return True

    def updateSlotStatus(self):
        '''
        Only two status: PRESENT and ABSENT
        '''
        curSlotStatus = []
        self.checkslot(curSlotStatus)
        slotdebuglog('curSlotStatus: {}\n preSlotStatus: {}'.format(curSlotStatus, self.preSlotStatus))
        if operator.eq(self.preSlotStatus, curSlotStatus) is False:
            if len(self.preSlotStatus) == 0:
                # first time
                for i, item in enumerate(curSlotStatus):
                    if item['status'] == 'PRESENT':
                        slotdebuglog('SLOT_PLUG_IN: %s' % (item['id']))
                    elif item['status'] == 'ABSENT':
                        slotdebuglog('SLOT_ABSENT: %s' % (item['id']))
                    else:
                        slotdebuglog('SLOT_FAILED: %s status %s not support yet' % (item['id'], item['status']))
                    self.preSlotStatus.append(item)
            else:
                for i, item in enumerate(curSlotStatus):
                    if item['status'] == self.preSlotStatus[i]['status']:
                        continue
                    if item['status'] == 'PRESENT' and self.preSlotStatus[i]['status'] == 'ABSENT':
                        self.dealslotplugin(item['id'])
                        slotinfo('SLOT_PLUG_IN: %s' % (item['id']))
                    elif item['status'] == 'ABSENT' and self.preSlotStatus[i]['status'] == 'PRESENT':
                        slotwarninglog('SLOT_PLUG_OUT: %s' % (item['id']))
                    else:
                        slotwarninglog('SLOT_PLUG_OUT: %s status change from %s to %s not support' %
                                       (item['id'], self.preSlotStatus[i]['status'], item['status']))
                    self.preSlotStatus.remove(self.preSlotStatus[i])
                    self.preSlotStatus.insert(i, item)

    def slotmonitor(self):
        self.updateSlotStatus()
        return 0


def doSlotMonitor(slotMonitor):
    slotMonitor.slotmonitor()


def run(interval, slotMonitor):
    # slotMonitor.devattrinit()
    while True:
        try:
            doSlotMonitor(slotMonitor)
        except Exception as e:
            traceback.print_exc()
            sloterror(str(e))
        time.sleep(interval)


@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''slot monitor operator'''


@main.command()
def start():
    '''start slot monitor'''
    slotinfo("slot_monitor start")
    slotMonitor = SlotMonitor()
    interval = SLOT_MONITOR_PARAM.get('polling_time', 1)
    run(interval, slotMonitor)


@main.command()
def stop():
    '''stop slot monitor '''
    slotinfo("stop")


# device_i2c operation
if __name__ == '__main__':
    main()
