#!/usr/bin/env python_nos
import os
import time
import click
import logging
import shutil
from logging.handlers import RotatingFileHandler
from platform_util import AliasedGroup, CONTEXT_SETTINGS, exec_os_cmd, read_s3ip_sysfs
from platform_config import BSP_COMMON_LOG_DIR, get_config_param
from public.platform_common_config import S3IP_SYSFS_NAME

PSU_LOG_FILE = BSP_COMMON_LOG_DIR + "dfx_psu_blackbox_record.log"
PSU_BLACKBOX_DIR = "/proc/psu/"
PSU_SYSFS_DIR = f"/sys/{S3IP_SYSFS_NAME}/psu"
STATUS_FILE = "hw_status"
BLACKBOX_PATH_FILE = "blackbox_path"
DFX_INFO_FILE = "dfx_info"
DEBUG_FILE = "/etc/.dfx_blackbox_record_debug_flag"

def logger_init(type):
    global blackbox_logger
    if not os.path.exists(BSP_COMMON_LOG_DIR):
        os.system("mkdir -p %s" % BSP_COMMON_LOG_DIR)
        os.system("sync")

    # Define the backup log file path with a timestamp
    backup_file = os.path.join(BSP_COMMON_LOG_DIR, f"{PSU_LOG_FILE}.last_startup")
    # Backup the existing log file if it exists
    if os.path.exists(PSU_LOG_FILE):
        shutil.move(PSU_LOG_FILE, backup_file)

    blackbox_logger = logging.getLogger(type)
    blackbox_logger.setLevel(logging.DEBUG)
    blackbox_handler = RotatingFileHandler(filename=PSU_LOG_FILE, maxBytes=1 * 1024 * 1024, backupCount=1)
    blackbox_handler.setFormatter(logging.Formatter('%(asctime)s, %(filename)s, %(levelname)s, %(message)s'))
    blackbox_handler.setLevel(logging.DEBUG)
    blackbox_logger.addHandler(blackbox_handler)

def dfx_blackbox_record_error(s, should_print=False):
    if should_print:
        print(s)
    else:
        blackbox_logger.error(s)

def debug_init():
    if os.path.exists(DEBUG_FILE):
        blackbox_logger.setLevel(logging.DEBUG)
    else:
        blackbox_logger.setLevel(logging.INFO)

class Blackbox_Record:
    def __init__(self, interval=60):
        logger_init("blackbox_record")
        self.interval = interval
        self.psu_nodes = [n for n in os.listdir(PSU_SYSFS_DIR) if n.startswith("psu")]
        self.status_cache = {}       # {psu: last_status}
        self.blackbox_path_cache = {} # {psu: blackbox_path}

    def get_status(self, status_file):
        ret, val = read_s3ip_sysfs(status_file)
        if ret is True:
            blackbox_logger.debug(f"get status_file {status_file} val {val}")
            return val
        else:
            return None


    def get_blackbox_path(self, psu, should_print=False):
        blackbox_path_file = os.path.join(PSU_SYSFS_DIR, psu, BLACKBOX_PATH_FILE)
        if not os.path.exists(blackbox_path_file):
            dfx_blackbox_record_error(f"Blackbox path file does not exist: {blackbox_path_file}", should_print=should_print)
            return None

        ret, val = read_s3ip_sysfs(blackbox_path_file)
        if ret is True:
            blackbox_logger.debug(f"get blackbox_path_file {blackbox_path_file} val {val}")
            if not val.startswith(PSU_BLACKBOX_DIR):
                dfx_blackbox_record_error(f"blackbox_path content format error: {val} (from {blackbox_path_file})", should_print=should_print)
                return None
            path_dir = os.path.dirname(val)
            new_path = os.path.join(path_dir, DFX_INFO_FILE)
            return new_path
        else:
            return None


    def read_blackbox_files(self, blackbox_path, psu, scene="[UPDATE]"):
        cmd = f"cat {blackbox_path}"
        try:
            ret, output = exec_os_cmd(cmd)
            if ret == 0:
                blackbox_logger.info(f"{scene}[{psu}] blackbox_info:\n{output}")
            else:
                blackbox_logger.error(f"{scene}[{psu}] Failed to read {psu} blackbox_info ({blackbox_path}): {output}")
        except Exception as e:
            blackbox_logger.error(f"{scene}[{psu}] Exception while reading {psu} blackbox_info ({blackbox_path}): {str(e)}")

    def read_blackbox_files_print(self, blackbox_path, psu):
        cmd = f"cat {blackbox_path}"
        try:
            ret, output = exec_os_cmd(cmd)
            if ret == 0:
                print(f"[{psu}] blackbox_info:\n{output}")
            else:
                print(f"[{psu}] Failed to read {psu} blackbox_info ({blackbox_path}): {output}")
        except Exception as e:
            print(f"[{psu}] Exception while reading {psu} blackbox_info ({blackbox_path}): {str(e)}")

    def init_record(self):
        for psu in self.psu_nodes:
            status_file = os.path.join(PSU_SYSFS_DIR, psu, STATUS_FILE)
            blackbox_path = self.get_blackbox_path(psu)
            if blackbox_path is None or not os.path.exists(blackbox_path):
                blackbox_logger.error(f"Blackbox path not found or does not exist for {psu}: {blackbox_path}")
            else:
                self.read_blackbox_files(blackbox_path, psu, scene="[INIT]")
            self.blackbox_path_cache[psu] = blackbox_path

            if not os.path.exists(status_file):
                blackbox_logger.error(f"Status file does not exist: {status_file}")
                continue
            status = self.get_status(status_file)
            if status is None:
                continue

            self.status_cache[psu] = status

    def print_black_box_one(self, psu):
        blackbox_path = self.get_blackbox_path(psu, should_print=True)
        if blackbox_path is None:
            print(f"Blackbox path not found for {psu}.")
            return
        
        if not os.path.exists(blackbox_path):
            print(f"Blackbox path not exist for {psu}: {blackbox_path}")
            return
        self.read_blackbox_files_print(blackbox_path, psu)

    def print_black_box(self, index):
        if index == 'all':
            sorted_psu_nodes = sorted(self.psu_nodes)
            for psu in sorted_psu_nodes:
                self.print_black_box_one(psu)
            return

        psu = index
        if psu in self.psu_nodes:
            self.print_black_box_one(psu)
        else:
            print(f"not found {psu}, pleas use 1 to {len(self.psu_nodes)}")

    def poll_status_and_record(self):
        while True:
            debug_init()
            blackbox_logger.debug("start poll")
            current_nodes = [n for n in os.listdir(PSU_SYSFS_DIR) if n.startswith("psu")]
            for psu in current_nodes:
                status_file = os.path.join(PSU_SYSFS_DIR, psu, STATUS_FILE)
                if not os.path.exists(status_file):
                    blackbox_logger.error(f"Status file does not exist (polling): {status_file}")
                    continue
                status = self.get_status(status_file)
                if status is None:
                    continue
                last_status = self.status_cache.get(psu)
                if status != last_status:
                    blackbox_logger.info(f"[UPDATE][{psu}] status_fr_pmbus changed: {last_status} -> {status}")
                    blackbox_path = self.blackbox_path_cache.get(psu)
                    if not blackbox_path:
                        blackbox_path = self.get_blackbox_path(psu)
                        if blackbox_path and os.path.exists(blackbox_path):
                            self.blackbox_path_cache[psu] = blackbox_path

                    if blackbox_path and os.path.exists(blackbox_path):
                        self.read_blackbox_files(blackbox_path, psu)
                    else:
                        blackbox_logger.error(f"[UPDATE][{psu}] Blackbox path not available/cannot read: {blackbox_path}")
                    self.status_cache[psu] = status
            blackbox_logger.debug("end poll")
            time.sleep(self.interval)

    def print_psu_nodes(self):
        if not self.psu_nodes:
            print("No PSU nodes found.")
        else:
            print("Available PSU nodes:")
            for psu in sorted(self.psu_nodes):
                print(psu)

    def print_psu_blackbox(self, index):
        if index == 'all':
            self.print_black_box(index)
            return

        if not index.isdigit():
            print("param error, %s is not digital or 'all'" % index)
            return

        psu_name = f"psu{index}"
        self.print_black_box(psu_name)
        
    def run(self):
        debug_init()
        try:
            self.init_record()
            config = get_config_param("DFX_PSU_BLACKBOX_RECORD", {})
            if not config:
                blackbox_logger.debug("DFX_PSU_BLACKBOX_RECORD config is None; exiting.")
                return

            polling_value = config.get("psu_status_polling", 0)
            if polling_value == 0:
                blackbox_logger.debug("Polling value is 0; exiting.")
                return

            polling_time = config.get("psu_status_polling_time", 60)
            self.interval = polling_time

            time.sleep(self.interval)
            self.poll_status_and_record()
        except Exception as e:
            blackbox_logger.error(f"An error: {e}")


@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''device operator'''

@main.command()
def start():
    '''start blackbox record'''
    blackbox_record = Blackbox_Record()
    blackbox_record.run()

@main.command(name='blackbox_get')
@click.argument('value')
def blackbox_get(value):
    '''start blackbox record'''
    blackbox_record = Blackbox_Record()
    blackbox_record.print_psu_blackbox(value)

if __name__ == '__main__':
    main()