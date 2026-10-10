#!/usr/bin/env python_nos
import logging
import json
import os
import time
import click
from platform_util import read_sysfs, setup_logger, read_s3ip_sysfs, BSP_COMMON_LOG_DIR, AliasedGroup, CONTEXT_SETTINGS
from public.platform_common_config import S3IP_SYSFS_NAME, SFF_TEMP_STORE_FILE

DEBUG_FILE = "/etc/.sff_temp_debug"
LOG_FILE = BSP_COMMON_LOG_DIR + "sff_temp_debug.log"
logger = setup_logger(LOG_FILE)

def debug_init():
    if os.path.exists(DEBUG_FILE):
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

def log_debug(s):
    logger.debug(s)

def log_error(s):
    logger.error(s)

def log_info(s):
    logger.info(s)

SFF_NUM_SYSFS  =  f"/sys/{S3IP_SYSFS_NAME}/transceiver/number"
SFF_TEMP_SYSFS = f"/sys/{S3IP_SYSFS_NAME}/transceiver/eth%d/temp"
SFF_TEMP_TMP_FILE = "/tmp/.sff_temp_tmp"
SFF_MAX_TEMP_VAL_STORE_FILE = "/tmp/highest_sff_temp"
SFF_MAX_TEMP_VAL_TMP_FILE = "/tmp/.sff_max_temp_val_tmp"
ERR_TEMP_VALUE = 999

class SffTemp(object):
    def __init__(self):
        self.polling_time = 5
        self.sff_number = 0

    def get_sff_number(self):
        ret, val = read_s3ip_sysfs(SFF_NUM_SYSFS)
        if ret is True:
            self.sff_number = int(val)
            log_debug("get sff number: %s" % self.sff_number)
        else:
            self.sff_number = 0
            log_error("get sff number fail, reason: %s" % val)

    def store_sff_temp(self):
        sff_temp_dict = {
            "max": {
                "temp": -ERR_TEMP_VALUE,
                "index": 0
            },
            "min": {
                "temp": -ERR_TEMP_VALUE,
                "index": 0
            }
        }

        for i in range(1, self.sff_number + 1):
            sff_temp_sysfs = SFF_TEMP_SYSFS % i
            ret, val = read_s3ip_sysfs(sff_temp_sysfs)
            if ret is True:
                if sff_temp_dict["max"]["temp"] < int(val):
                    sff_temp_dict["max"]["temp"] = int(val)
                    sff_temp_dict["max"]["index"] = i
                    log_debug("get sff%d max temp: %s" % (i, val))
                if sff_temp_dict["min"]["temp"] > int(val):
                    sff_temp_dict["min"]["temp"] = int(val)
                    sff_temp_dict["min"]["index"] = i
                    log_debug("get sff%d min temp: %s" % (i, val))
            else:
                log_debug("get sff%d temp fail, reason: %s" % (i, val))

        try:
            with open(SFF_TEMP_TMP_FILE, 'w', encoding='utf-8') as f:
                json.dump(sff_temp_dict, f, ensure_ascii=False, indent=4)
            os.rename(SFF_TEMP_TMP_FILE, SFF_TEMP_STORE_FILE)

            with open(SFF_MAX_TEMP_VAL_TMP_FILE, 'w', encoding='utf-8') as fd:
                fd.write(str(sff_temp_dict["max"]["temp"]))
            os.rename(SFF_MAX_TEMP_VAL_TMP_FILE, SFF_MAX_TEMP_VAL_STORE_FILE)
        except Exception as e:
            log_debug("write file fail, reason: %s" % e)

    def get_sff_max_temp(self):
        try:
            with open(SFF_TEMP_STORE_FILE, 'r', encoding='utf-8') as f:
                data_dict = json.load(f)
                if data_dict["max"]["temp"] != -ERR_TEMP_VALUE and data_dict["max"]["index"] != 0:
                    return True, data_dict["max"]
                else:
                    return False, "err sff max temp value: %s" % data_dict["max"]
        except Exception as e:
            msg = "get_sff_max_temp fail, reason: %s" % e
            return False, msg

    def run(self):
        while True:
            debug_init()
            if self.sff_number <= 0:
                self.get_sff_number()
            self.store_sff_temp()
            time.sleep(self.polling_time)

@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''device operator'''
    debug_init()

@main.command()
def start():
    '''start sff temp polling'''
    log_info("sff_temp_polling start")
    sff_temp = SffTemp()
    sff_temp.run()


@main.command()
def stop():
    '''stop sff temp polling'''
    log_info("sff_temp_polling stop")

@main.command()
def get_sff_max_temp():
    '''get_sff_max_temp'''
    sff_temp = SffTemp()
    ret, info = sff_temp.get_sff_max_temp()
    print(ret)
    print(info)

if __name__ == '__main__':
    main()