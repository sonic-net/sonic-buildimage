#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
import click
import time
import syslog
import traceback
from ruijieutil import waitForDocker, STARTMODULE, AVSUTIL
try:
    from rest.rest import BMCMessage
except ImportError:
    pass

CONTEXT_SETTINGS = dict(help_option_names=['-h', '--help'])

class AliasedGroup(click.Group):
    def get_command(self, ctx, cmd_name):
        rv = click.Group.get_command(self, ctx, cmd_name)
        if rv is not None:
            return rv
        matches = [x for x in self.list_commands(ctx)
                   if x.startswith(cmd_name)]
        if not matches:
            return None
        elif len(matches) == 1:
            return click.Group.get_command(self, ctx, matches[0])
        ctx.fail('Too many matches: %s' % ', '.join(sorted(matches)))
    
def avswarninglog(s):
    s = s.decode('utf-8').encode('gb2312')
    syslog.openlog("AVSCONTROL",syslog.LOG_PID)
    syslog.syslog(syslog.LOG_WARNING,s)
    
def avscriticallog(s):
    s = s.decode('utf-8').encode('gb2312')
    syslog.openlog("AVSCONTROL",syslog.LOG_PID)
    syslog.syslog(syslog.LOG_CRIT,s)
    
def avserror(s):
    s = s.decode('utf-8').encode('gb2312')
    syslog.openlog("AVSCONTROL",syslog.LOG_PID)
    syslog.syslog(syslog.LOG_ERR,s)
    
def avsinfo(s):
    syslog.openlog("AVSCONTROL",syslog.LOG_PID)
    syslog.syslog(syslog.LOG_INFO,s)

def doAvsCtrol():
    index = 0
    url = "/xyz/openbmc_project/hostchannel/attr/MacRov"
    while True:
        if "avscontrol_restful" in STARTMODULE and STARTMODULE['avscontrol_restful'] == 1:
            try:
                #for alibmc rest.py has define get_macrov_value function
                get_macrov_value = getattr(BMCMessage(), "get_macrov_value", None)
                if callable(get_macrov_value):
                    macrov_value = int(get_macrov_value())
                else:
                    macrov_value = int(BMCMessage().getBmcValue(url))
                if  macrov_value >= 0:
                    break
            except Exception as e:
                time.sleep(2)
                continue
        else:
            if AVSUTIL.mac_adj():
                break
 
        index += 1
        if index >= 10:
            avserror("%%DEV_MONITOR-AVS: MAC Voltage adjust failed.")
            exit(-1)
    avsinfo("%%AVSCONTROL success")
    exit(0)
    
def run(interval):
    while True:
        try:
            if waitForDocker(timeout = 0) == True:
                time.sleep(10)  # w10s
                doAvsCtrol()
            time.sleep(interval)
        except Exception as e:
            traceback.print_exc()
            print(e)

@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''device operator'''
    pass
    
@main.command()
def start():
    '''start AVS control'''
    avsinfo("%%AVSCONTROL start")
    interval = 5
    run(interval)

##device_i2c operation
if __name__ == '__main__':
    main()
