#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os
import time
import syslog
import click
import logging
from platform_util import setup_logger, BSP_COMMON_LOG_DIR
from platform_config import *

SFF_TYPE_UNKNOWN = "UNKNOWN"
DEBUG_FILE = "/etc/.sff_polling_debug_flag"
LOG_FILE = BSP_COMMON_LOG_DIR + "sff_polling_debug.log"
logger = setup_logger(LOG_FILE)

def debug_init():
    if os.path.exists(DEBUG_FILE):
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

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


def sff_polling_debug(s):
    logger.debug(s)


def sff_polling_error(s):
    logger.error(s)

def dev_file_write(path, offset, buf):
    msg = ""
    value = ""
    fd = -1

    if not os.path.exists(path):
        msg = path + " not found !"
        return False, msg

    sff_polling_debug("dev_file_write path:%s, offset:0x%x, value: %s" % (path, offset, buf))

    try:
        for item in buf:
            value += chr(item)
        fd = os.open(path, os.O_WRONLY)
        os.lseek(fd, offset, os.SEEK_SET)
        ret = os.write(fd, value)
    except Exception as e:
        msg = str(e)
        return False, msg
    finally:
        if fd > 0:
            os.close(fd)

    return True, ret


def dev_file_read(path, offset, len):
    value = []
    msg = ""
    ret = ""
    fd = -1

    if not os.path.exists(path):
        msg = path + " not found !"
        return False, msg

    try:
        fd = os.open(path, os.O_RDONLY)
        os.lseek(fd, offset, os.SEEK_SET)
        ret = os.read(fd, len)
        for item in ret:
            value.append(ord(item))
    except Exception as e:
        msg = str(e)
        return False, msg
    finally:
        if fd > 0:
            os.close(fd)

    sff_polling_debug("dev_file_read path:%s, offset:0x%x, len:%d, value: %s" % (path, offset, len, value))
    return True, value


class SffPolling(object):

    def __init__(self, config, index, sffid):
        self.__sff_id = sffid  # Module id 1 ~ Total number of modules
        self.__logic_dev_loc = config["logic_dev_loc"]   # polling path of the logical device
        self.__reg_base_addr = config["base_addr"] + (index * config["range"])  # The base address of each module's polling configuration
        self.__sff_type_conf = config["sff_type"]        # The path to obtain the module type
        self.__pre_sff_type = SFF_TYPE_UNKNOWN           # The module type is initialized to UNKNOWN

    @property
    def sff_id(self):
        return self.__sff_id

    @property
    def logic_dev_loc(self):
        return self.__logic_dev_loc

    @property
    def reg_base_addr(self):
        return self.__reg_base_addr

    @property
    def sff_type_conf(self):
        return self.__sff_type_conf

    @property
    def pre_sff_type(self):
        return self.__pre_sff_type

    @pre_sff_type.setter
    def pre_sff_type(self, value):
        self.__pre_sff_type = value

    @property
    def sff_type(self):
        # Read sysfs get module type, read failed or module type is not supported, return "UNKNOWN"
        sff_type_loc = self.sff_type_conf["loc"] % self.sff_id
        mask = self.sff_type_conf.get("mask", 0xff)

        if not os.path.exists(sff_type_loc):
            sff_polling_error("sff%d type loc: %s not found" % (self.sff_id, sff_type_loc))
            return SFF_TYPE_UNKNOWN
        try:
            with open(sff_type_loc, "r") as fd:
                value = fd.read()
            sff_type_value = int(value, 16) & mask
            sff_type = self.sff_type_conf.get(sff_type_value, SFF_TYPE_UNKNOWN)
            sff_polling_debug(
                "sff%d type loc:%s, value:%s, type:%s" %
                (self.sff_id, sff_type_loc, sff_type_value, sff_type))
            return sff_type
        except Exception as e:
            sff_polling_error("sff%d get type error: %s" % (self.sff_id, str(e)))
        return SFF_TYPE_UNKNOWN

    def set_sff_polling_reg(self, conf_item):
        # Set polling parameters for each module
        reg_desc = conf_item["reg_desc"]
        offset = self.reg_base_addr + conf_item["offset"]
        value = conf_item["value"]
        status, msg = dev_file_write(self.logic_dev_loc, offset, value)
        if status is False:
            sff_polling_error("sff%d set %s failed, loc:%s, offset:0x%x, value:%s, err msg:%s"
                              % (self.sff_id, reg_desc, self.logic_dev_loc, offset, value, msg))
            return False
        sff_polling_debug("sff%d set %s success, loc:%s, offset:0x%x, value:%s"
                          % (self.sff_id, reg_desc, self.logic_dev_loc, offset, value))
        return True

    def check_sff_polling_enable(self):
        # Read polling_cfg_en to determine whether to start polling
        polling_cfg_en = SFF_POLLING_CONF["polling_cfg_en"]
        offset = self.reg_base_addr + polling_cfg_en["offset"]
        mask = polling_cfg_en["mask"]
        polling_cfg_en_val = polling_cfg_en["enable"]
        status, value = dev_file_read(self.logic_dev_loc, offset, 1)
        if status is False:
            sff_polling_error(
                "sff%d get polling_cfg_en status failed, loc:%s, offset:0x%x" %
                (self.sff_id, self.logic_dev_loc, offset))
            return -1
        if (value[0] & mask) == polling_cfg_en_val:
            sff_polling_debug("sff%d already start polling" % self.sff_id)
            return True
        sff_polling_debug("sff%d not start polling" % self.sff_id)
        return False

    def start_sff_polling(self):
        try:
            # Read module type
            sff_type = self.sff_type
            if sff_type == SFF_TYPE_UNKNOWN:
                sff_polling_debug("sff%d type unknown do nothing" % self.sff_id)
                return
            # Determine whether to start polling
            polling_en_status = self.check_sff_polling_enable()
            # The module type has not changed and polling has been started
            if sff_type == self.pre_sff_type and polling_en_status is True:
                sff_polling_debug(
                    "sff%d type %s not change, and already start polling, do nothing" %
                    (self.sff_id, sff_type))
                return
            # If the module type changes or polling is not enabled, reconfigure it
            sff_polling_debug(
                "sff%d type change from %s to %s, start to set polling config" %
                (self.sff_id, self.pre_sff_type, sff_type))
            sff_conf_list = SFF_POLLING_CONF[sff_type]
            for item in sff_conf_list:
                status = self.set_sff_polling_reg(item)
                if status is False:
                    sff_polling_error("sff%d start polling failed." % self.sff_id)
                    return
            sff_polling_debug("sff%d start polling success." % self.sff_id)
            self.pre_sff_type = sff_type  # Update module type
        except Exception as e:
            sff_polling_error("sff%d start polling error, msg: %s" % (self.sff_id, str(e)))
        return


def run(interval, sff_list):
    while True:
        debug_init()
        for obj in sff_list:
            obj.start_sff_polling()
        time.sleep(interval)


@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    pass


@main.command()
def start():
    '''start sff polling process'''

    sff_list = []
    for item in SFF_POLLING_CONF["device"]:
        index = 0
        start_port = item["start_port"]
        end_port = item["end_port"]
        for sff_id in range(start_port, end_port + 1):
            obj = SffPolling(item, index, sff_id)
            sff_list.append(obj)
            index += 1

    interval = SFF_POLLING_CONF.get("polling", 3)
    run(interval, sff_list)


if __name__ == '__main__':
    debug_init()
    main()
