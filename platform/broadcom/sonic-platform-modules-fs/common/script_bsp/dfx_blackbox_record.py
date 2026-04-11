#!/usr/bin/env python3
import os
import click
import logging
import shutil
from logging.handlers import RotatingFileHandler
from platform_util import exec_os_cmd
from platform_config import BSP_COMMON_LOG_DIR, get_config_param

CONTEXT_SETTINGS = {"help_option_names": ['-h', '--help']}
PSU_LOG_FILE = BSP_COMMON_LOG_DIR + "dfx_psu_blackbox_record.log"
PSU_BLACKBOX_DIR = "/proc/psu/"

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

class Blackbox_Record:
    def __init__(self):
        logger_init("blackbox_record")

    def psu_blackbox_record(self):
        for device in get_config_param("DFX_PSU_BLACKBOX_RECORD", []):
            name = device["name"]
            loc = device["loc"]
            path = os.path.join(PSU_BLACKBOX_DIR, loc)

            # Check if the directory exists
            if not os.path.exists(path):
                blackbox_logger.error(f"Directory does not exist: {path}\n")
                continue

            # Read the blackbox_info file
            cmd = f"cat {path}/blackbox_info"
            try:
                ret, output = exec_os_cmd(cmd)
                if ret == 0:
                    blackbox_logger.info(f"{name} blackbox_info:\n{output}")
                else:
                    blackbox_logger.error(f"Failed to read {name} blackbox_info: {output}")
            except Exception as e:
                blackbox_logger.error(f"Exception while reading {name} blackbox_info: {str(e)}")

            # Read the blackbox file
            cmd = f"cat {path}/blackbox"
            try:
                ret, output = exec_os_cmd(cmd)
                if ret == 0:
                    blackbox_logger.info(f"{name} blackbox raw data:\n{output}")
                else:
                    blackbox_logger.error(f"Failed to read {name} blackbox raw data: {output}")
            except Exception as e:
                blackbox_logger.error(f"Exception while reading {name} blackbox raw data: {str(e)}")

    
    def run(self):
        self.psu_blackbox_record()

    
@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''device operator'''

@main.command()
def start():
    '''start blackbox record'''
    blackbox_record = Blackbox_Record()
    blackbox_record.run()

if __name__ == '__main__':
    main()