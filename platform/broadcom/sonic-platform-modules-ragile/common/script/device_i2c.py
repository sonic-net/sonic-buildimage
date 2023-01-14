#!/usr/bin/env python3
# -*- coding: UTF-8 -*-

import click
import os
import time
import subprocess
from ragileconfig import *
from ragileutil import rgpciwr, rgi2cset, io_wr
from sonic_py_common.general import getstatusoutput_noshell, getstatusoutput_noshell_pipe

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

def log_os_system(cmd):
    u'''execute shell command'''
    status, output = getstatusoutput_noshell(cmd)
    if status:
        print(output)
    return  status, output

def write_sysfs_value(reg_name, value):
    u'''write sysfs file'''
    mb_reg_file = "/sys/bus/i2c/devices/" + reg_name
    if (not os.path.isfile(mb_reg_file)):
        print(mb_reg_file,  'not found !')
        return False
    try:
        with open(mb_reg_file, 'w') as fd:
            fd.write(value)
    except Exception as error:
        return False
    return True

def check_driver():
    u'''whether there is driver start with rg'''
    status, output = getstatusoutput_noshell_pipe(["lsmod"], ["grep", "rg"], ["wc", "-l"])
    #System execution error
    if status:
        return False
    if output.isdigit() and int(output) > 0:
        return True
    else:
        return False

def get_pid(name):
    ret = []
    for dirname in os.listdir('/proc'):
        if dirname == 'curproc':
            continue
        try:
            with open('/proc/{}/cmdline'.format(dirname), mode='r') as fd:
                content = fd.read()
        except Exception:
            continue
        if name in content:
            ret.append(dirname)
    return ret

def start_avs_ctrl():
    cmd = ["avscontrol.py", "start"]
    rets = get_pid("avscontrol.py")
    if len(rets) == 0:
        subprocess.Popen(cmd)

def start_fan_ctrl():
    if STARTMODULE['fancontrol'] == 1:
        cmd = ["fancontrol.py", "start"]
        rets = get_pid("fancontrol.py")
        if len(rets) == 0:
            subprocess.Popen(cmd)

def starthal_fanctrl():
    if STARTMODULE.get('hal_fanctrl',0) == 1:
        cmd = ["hal_fanctrl.py", "start"]
        rets = get_pid("hal_fanctrl.py")
        if len(rets) == 0:
            subprocess.Popen(cmd)

def starthal_ledctrl():
    if STARTMODULE.get('hal_ledctrl',0) == 1:
        cmd = ["hal_ledctrl.py", "start"]
        rets = get_pid("hal_ledctrl.py")
        if len(rets) == 0:
            subprocess.Popen(cmd)

def start_slot_monitor():
    if STARTMODULE.get('slot_monitor',0) == 1:
        cmd = ["slot_monitor.py", "start"]
        rets = get_pid("slot_monitor.py")
        if len(rets) == 0:
            subprocess.Popen(cmd)

def stop_fan_ctrl():
    u'''disable fan timer service'''
    if STARTMODULE['fancontrol'] == 1:
        rets = get_pid("fancontrol.py")  #
        for ret in rets:
            cmd = ["kill", ret]
            subprocess.call(cmd)
        return True

def stophal_ledctrl():
    if STARTMODULE.get('hal_ledctrl',0) == 1:
        rets = get_pid("hal_ledctrl.py")
        for ret in rets:
            cmd = ["kill", ret]
            subprocess.call(cmd)
        return True

def stop_slot_monitor():
    u'''disable slot timer service'''
    if STARTMODULE.get('slot_monitor',0) == 1:
        rets = get_pid("slot_monitor.py")  #
        for ret in rets:
            cmd = ["kill", ret]
            subprocess.call(cmd)
        return True

def rm_dev(bus, loc):
    devpath = "/sys/bus/i2c/devices/%d-%04x"%(bus, loc)
    if os.path.exists(devpath):
        with open("/sys/bus/i2c/devices/i2c-%d/delete_device" % bus, 'w') as f:
            f.write('0x%02x\n' % loc)

def add_dev(name, bus, loc):
    if name == "lm75":
        time.sleep(0.1)
    pdevpath = "/sys/bus/i2c/devices/i2c-%d/" % (bus)
    for i in range(1, 100):#wait for mother-bus generation，maximum wait time is 10s
        if os.path.exists(pdevpath) is True:
            break
        time.sleep(0.1)
        if i % 10 == 0:
            click.echo("%%DEVICE_I2C-INIT: %s not found, wait 0.1 second ! i %d " % (pdevpath,i))

    devpath = "/sys/bus/i2c/devices/%d-%04x"%(bus, loc)
    if os.path.exists(devpath) is False:
        with open("/sys/bus/i2c/devices/i2c-%d/new_device" % bus, 'w') as f:
            f.write('%s 0x%02x\n' % (name, loc))

def removedevs():
    devs = GLOBALCONFIG["DEVS"]
    for index in range(len(devs)-1, -1, -1 ):
        rm_dev(devs[index]["bus"] , devs[index]["loc"])

def adddevs():
    devs = GLOBALCONFIG["DEVS"]
    for dev in range(0, devs.__len__()):
        add_dev(devs[dev]["name"], devs[dev]["bus"] , devs[dev]["loc"])

def checksignaldriver(name):
    status, output = getstatusoutput_noshell_pipe(["lsmod"], ["grep", name], ["wc", "-l"])
    #System execution error
    if status:
        return False
    if output.isdigit() and int(output) > 0:
        return True
    else:
        return False

def adddriver(name, delay):
    cmd = ["modprobe", name]
    if delay != 0:
        time.sleep(delay)
    if not checksignaldriver(name):
        getstatusoutput_noshell(cmd)

def removedriver(name, delay):
    realname = name.lstrip().split(" ")[0];
    cmd = ["rmmod", "-f", realname]
    if checksignaldriver(realname):
        getstatusoutput_noshell(cmd)

def removedrivers():
    u'''remove all drivers'''
    if GLOBALCONFIG is None:
        click.echo("%%DEVICE_I2C-INIT: load global config failed.")
        return
    drivers = GLOBALCONFIG.get("DRIVERLISTS", None)
    if drivers is None:
        click.echo("%%DEVICE_I2C-INIT: load driver list failed.")
        return
    for index in range(len(drivers)-1, -1, -1 ):
        delay = 0
        name = ""
        if type(drivers[index]) == dict and "delay" in drivers[index]:
            name = drivers[index].get("name")
            delay = drivers[index]["delay"]
        else:
            name = drivers[index]
        removedriver(name, delay)

def adddrivers():
    u'''add drivers'''
    if GLOBALCONFIG is None:
        click.echo("%%DEVICE_I2C-INIT: load global config failed.")
        return
    drivers = GLOBALCONFIG.get("DRIVERLISTS", None)
    if drivers is None:
        click.echo("%%DEVICE_I2C-INIT: load driver list failed.")
        return
    for index in range(0 ,len(drivers)):
        delay = 0
        name = ""
        if type(drivers[index]) == dict and "delay" in drivers[index]:
            name = drivers[index].get("name")
            delay = drivers[index]["delay"]
        else:
            name = drivers[index]
        adddriver(name, delay)

def otherinit():
    for index in GLOBALINITPARAM:
        index_type = index.get("type", None)
        if index_type == "io":
            ret = io_wr(index.get("offset"), index.get("val"))
        else:
            ret, _ = rgi2cset(
                index.get("bus"),
                index.get("devaddr"),
                index.get("offset"),
                index.get("val")
            )
        if not ret:
            click.echo("%%DEVICE_I2C-INIT: init param %s failed." % index.get("name"))

    for index in GLOBALINITCOMMAND:
        log_os_system(index)

def unload_driver():
    u'''remove devices and drivers'''
    stop_fan_ctrl()  # disable fan-control service
    removedevs()    # remove other devices
    removedrivers() # remove drivers

def reload_driver():
    u'''reload devices and drivers'''
    removedevs()    # remove other devices
    removedrivers() # remove drivers
    time.sleep(1)
    adddrivers()
    adddevs()


def i2c_check(bus,retrytime = 6):
    try:
        i2cpath = "/sys/bus/i2c/devices/" + bus
        while retrytime and not os.path.exists(i2cpath):
            click.echo("%%DEVICE_I2C-HA: i2c bus abnormal, last bus %s is not exist." % i2cpath)
            reload_driver()
            retrytime -= 1
            time.sleep(1)
    except Exception as e:
        click.echo("%%DEVICE_I2C-HA: %s" % str(e))
    return

def set_mac_leds(data):
    '''write pci register'''
    pcibus = MAC_LED_RESET.get("pcibus")
    slot = MAC_LED_RESET.get("slot")
    fn = MAC_LED_RESET.get("fn")
    bar = MAC_LED_RESET.get("bar")
    offset = MAC_LED_RESET.get("offset")
    val = MAC_LED_RESET.get(data, None)
    if val is None:
        click.echo("%%DEVICE_I2C-INIT: set_mac_leds wrong input")
        return
    rgpciwr(pcibus, slot, fn, bar, offset, val)

def load_driver():
    u'''load devices and drivers'''
    adddrivers()
    adddevs()
    if STARTMODULE.get("i2ccheck",0) == 1: #i2c HA
        busend = i2ccheck_params.get("busend")
        retrytime = i2ccheck_params.get("retrytime")
        i2c_check(busend,retrytime)
    start_fan_ctrl() # enable fan
    starthal_fanctrl() # enable fan control
    starthal_ledctrl() # enable LED control
    if STARTMODULE['avscontrol'] == 1:
        start_avs_ctrl() # avs voltage-adjustment
    start_slot_monitor() # slot insertion and removal initialization monitor
    otherinit();    # other initialization, QSFP initialization
    if STARTMODULE.get("macledreset", 0) == 1:
        set_mac_leds("reset")

@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''device operator'''
    pass


@main.command()
def start():
    '''load device '''
    if check_driver():
        unload_driver()
    load_driver()

@main.command()
def stop():
    '''stop device '''
    unload_driver()

@main.command()
def restart():
    '''restart device'''
    unload_driver()
    load_driver()

if __name__ == '__main__':
    u'''device_i2c operation'''
    main()
