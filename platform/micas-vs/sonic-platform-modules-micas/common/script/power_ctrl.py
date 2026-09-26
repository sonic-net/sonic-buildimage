#!/usr/bin/env python3
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
import click
import os
import fcntl
import time
import sys
from platform_util import get_value, set_value
from platform_config import POWER_CTRL_CONF


POWERCTLDEBUG = 0
OE_SUPPROT_OPERATE = ("status", "off", "on", "reset")
STATUS_OFF = "off"
STATUS_ON = "on"
STATUS_UNKNOWN = "unknown"
CLI_CONFIRM = True
POWER_CTRL_DEBUG_FILE = "/etc/.power_control_debug"
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


def debug_init():
    global POWERCTLDEBUG
    if os.path.exists(POWER_CTRL_DEBUG_FILE):
        POWERCTLDEBUG = 1
    else:
        POWERCTLDEBUG = 0


def powerctrldebug(s):
    # s = s.decode('utf-8').encode('gb2312')
    if POWERCTLDEBUG == 1:
        syslog.openlog("POWERCTRL", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_DEBUG, s)


def get_status_once(conf):
    val_conf = conf.get("val_conf", {})
    ret, val = get_value(val_conf)
    if ret is False:
        powerctrldebug("get_status_once failure, val_conf: %s, msg: %s" % (val_conf, val))
        return STATUS_UNKNOWN

    if isinstance(val, str):
        val_tmp = int(val, 16)
    else:
        val_tmp = val

    mask = val_conf.get("mask")
    if mask is not None:
        value = val_tmp & mask
    else:
        value = val_tmp

    powerctrldebug("val_conf: %s, value: %s, val_tmp: %s, mask: %s" % (val_conf, value, val_tmp, mask))
    off_val_list = conf.get("off", [])
    if value in off_val_list:
        powerctrldebug("value: %s, is in off_val_list: %s, status is off" % (value, off_val_list))
        return STATUS_OFF

    on_val_list = conf.get("on", [])
    if value in on_val_list:
        powerctrldebug("value: %s, is in on_val_list: %s, status is on" % (value, on_val_list))
        return STATUS_ON

    powerctrldebug("value: %s, not in off_val_list: %s, and on_val_list: %s" % (value, off_val_list, on_val_list))
    return STATUS_UNKNOWN


def get_status(conf):
    status_list = []
    for item in conf:
        status = get_status_once(item)
        break_status = item.get("break_status", [])
        if status in break_status:
            powerctrldebug("status: %s is in break status: %s, return" % (status, break_status))
            return status
        status_list.append(status)
    if len(set(status_list)) == 1: # All states are consistent
        powerctrldebug("All states are consistent, status_list: %s, return status: %s" % (status_list, status_list[0]))
        return status_list[0]
    # status list inconsistent
    powerctrldebug("The status in the status list is inconsistent, status_list: %s" % status_list)
    return STATUS_UNKNOWN


def do_power_operation(conf):
    for item in conf:
        ret, msg = set_value(item)
        if ret is False:
            powerctrldebug("set value failed, conf: %s, msg %s" % (item, msg))
            return ret, msg
        powerctrldebug("set value success, conf: %s" % item)
    return True, ""


def do_power_off(name, conf, cli_confirm):
    power_conf = conf.get("off")
    if power_conf is None:
        msg = ("power off config is none, can't do power off operation.")
        return False, msg

    if cli_confirm is True and not click.confirm("Are you sure you want to power off %s?" % name):
        click.echo('Aborted.')
        sys.exit(0)
    ret, msg = do_power_operation(power_conf)
    return ret, msg


def do_power_on(name, conf, cli_confirm):
    power_conf = conf.get("on")
    if power_conf is None:
        msg = ("power on config is none, can't do power on operation.")
        return False, msg

    if cli_confirm is True and not click.confirm("Are you sure you want to power on %s?" % name):
        click.echo('Aborted.')
        sys.exit(0)
    ret, msg = do_power_operation(power_conf)
    return ret, msg


def do_power_cycle(name, conf, cli_confirm):
    if cli_confirm is True and not click.confirm("Are you sure you want to power cycle %s?" % name):
        click.echo('Aborted.')
        sys.exit(0)

    power_conf = conf.get("cycle")
    if power_conf is not None:
        ret, msg = do_power_operation(power_conf)
        return ret, msg
    # power cycle config is none, try to power off then power on
    ret, msg = do_power_off(name, conf, False)
    if ret is False:
        return ret, msg
    ret, msg = do_power_on(name, conf, False)
    return ret, msg


def do_power_reset(name, conf, cli_confirm):
    power_conf = conf.get("reset")
    if power_conf is None:
        msg = ("power reset config is none, can't do power reset operation.")
        return False, msg

    if cli_confirm is True and not click.confirm("Are you sure you want to power on %s?" % name):
        click.echo('Aborted.')
        sys.exit(0)

    ret, msg = do_power_operation(power_conf)
    return ret, msg


def do_operation(conf, command):
    name = conf.get("name")

    # First get the current status before any operation
    curr_status = None
    status_conf = conf.get("status")
    if status_conf is None:
        powerctrldebug("%s status config is None" % name)
    else:
        curr_status = get_status(status_conf)
        powerctrldebug("%s get_status %s" % (name, curr_status))

    # get status command
    if command == "status":
        if curr_status is None:
            print("Can't get %s %s config" % (name, command))
        else:
            print("Power status for %s: %s" % (name, curr_status))
        return

    # power off command
    if command == "off":
        if curr_status == "off":
            print("%s is already powered off..." % name)
            return
        ret, msg = do_power_off(name, conf, CLI_CONFIRM)
        if ret is False:
            print("%s powered off failure, msg: %s" % (name, msg))
        else:
            print("%s powered off successfully" % name)
        return

    # power on command
    if command == "on":
        if curr_status == "on":
            print("%s is already powered on..." % name)
            return
        ret, msg =  do_power_on(name, conf, CLI_CONFIRM)
        if ret is False:
            print("%s powered on failure, msg: %s" % (name, msg))
        else:
            print("%s powered on successfully" % name)
        return

    # power cycle command
    if command == "cycle":
        if curr_status == "off":
            # current status is off, do powered on to powered cycle
            powerctrldebug("%s current status is off, try to do powered on" % name)
            ret, msg = do_power_on(name, conf, CLI_CONFIRM)
        else:
            # current status is not off, do powered cycle
            powerctrldebug("%s current status is not off, try to do powered cycle" % name)
            ret, msg = do_power_cycle(name, conf, CLI_CONFIRM)

        if ret is False:
            print("%s powered cycle failure, msg: %s" % (name, msg))
        else:
            print("%s powered cycle successfully" % name)
        return

    # power reset command
    if curr_status == "off":
        print("%s is currently powered off. Please power it on before performing the reset operation." % name)
        return
    ret, msg = do_power_reset(name, conf, CLI_CONFIRM)
    if ret is False:
        print("%s powered reset failure, msg: %s" % (name, msg))
    else:
        print("%s powered reset successfully" % name)
    return


def do_oe_power_ctrl(oe_index, command):
    oe_power_conf = POWER_CTRL_CONF.get("oe", [])
    oe_num = len(oe_power_conf)
    if oe_num == 0:
        print("OE power control config is none, don't support oe power control.")
        return

    if oe_index < 0 or oe_index >= oe_num:
        print("Invalid oe index: %d, oe index must [0~%d]" % (oe_index, oe_num -1))
        return

    if command not in OE_SUPPROT_OPERATE:
        print("Unsupported operation command: %s" % command)
        return

    oe_power_conf_item = oe_power_conf[oe_index]

    do_operation(oe_power_conf_item, command)
    return


pidfile = 0
def ApplicationInstance():
    global pidfile
    pidfile = open(os.path.realpath(__file__), "r")
    try:
        fcntl.flock(pidfile, fcntl.LOCK_EX | fcntl.LOCK_NB)
        return True
    except Exception:
        return False


# Single-process execution only
def cli_ready_check():
    start_time = time.time()
    while True:
        ret = ApplicationInstance()
        if ret is True:
            break

        if time.time() - start_time < 0:
            start_time = time.time()

        if time.time() - start_time > 10:
            print("Please wait for the power_ctrl command to complete before performing further operations")
            sys.exit(1)
        time.sleep(0.1)
    return


@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
@click.help_option('-h', '--help', help='show help info')
@click.option('-y', '--yes', is_flag=True, help='Automatically confirm and skip the prompt.', hidden=True)
def main(yes):
    '''power control script'''
    global CLI_CONFIRM
    if yes is True:
        CLI_CONFIRM = False


# oe power control
@main.command()
@click.argument('oe_index', required=True, type=int)
@click.argument('command', required=True)
def oe(oe_index, command):
    '''OE_INDEX: start from 0, COMMAND: off, on, reset, status'''
    do_oe_power_ctrl(oe_index, command)


if __name__ == '__main__':
    if os.geteuid() != 0:
        print("Root privileges are required for this operation")
        sys.exit(1)
    cli_ready_check()
    debug_init()
    main()
