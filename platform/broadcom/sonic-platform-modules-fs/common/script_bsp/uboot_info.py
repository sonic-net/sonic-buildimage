#!/usr/bin/env python3
# -*- coding: UTF-8 -*-

import click
from pathlib import Path
import subprocess
import logging
import os
from platform_util import log_to_file

log_directory = '/var/log/bsp_tech'
os.makedirs(log_directory, exist_ok=True)
log_file_path = os.path.join(log_directory, 'uboot_info_debug.log')

LOG_WRITE_SIZE = 1 * 1024 * 1024 # 1 MB

uboot_role_list = [
    ["current", "master"],
    ["current", "slave"],
    ["other", "master"],
    ["other", "slave"]
]

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

def log_message(message):
    """Print the message and log it to the log file."""
    log_to_file(message, log_file_path, LOG_WRITE_SIZE)

def check_and_create_file(file_path):
    """
    Check if the file exists; if not, create the file, including all intermediate directories.

    :param file_path: The path of the file to check and create
    :return: Returns True if the file exists or create success; otherwise, returns False
    """
    path = Path(file_path)
    if path.exists():
        return True
    try:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.touch()
        return True
    except Exception as e:
        log_message(f"create file fail: {e}")
        return False

class UbootInfo():
    def __init__(self, role, target):
        self.app = "show_version"
        self.destination = "/tmp/version/uboot/"
        self.role = role
        self.target = target

        if self.role == "current":
            self.destination += "0/"
        else:
            self.destination += "1/"

        if self.target == "master":
            self.destination += "master"
        else:
            self.destination += "slave"

    def create_version(self):
        try:
            ret = check_and_create_file(self.destination)
            if ret is False:
                success_message = f"create file: {self.destination} fail."
                log_message(success_message)
                return False
            with open(self.destination, 'w') as outfile:
                subprocess.run([self.app, self.role, self.target], stdout=outfile, check=True)
            success_message = f"create uboot({self.role}, {self.target}) version success."
            log_message(success_message)
            return True
        except subprocess.CalledProcessError as e:
            log_message(f"Command failed: {e}")
            return False
        except Exception as e:
            log_message(str(e))
            return False
    
    def destroy_version(self):
        try:
            file = Path(self.destination)
            file.unlink()
            success_message = f"destroy uboot({self.role}, {self.target}) version success."
            log_message(success_message)
            return True
        except FileNotFoundError:
            log_message("File does not exist.")
            return True
        except Exception as e:
            log_message(str(e))
            return False
        
    def to_string(self):
        return f"role: {self.role}, target: {self.target}, dest: {self.destination}, app: {self.app}."

@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    pass

@main.command()
def start():
    '''create uboot info'''
    log_message("Create uboot info.")
    for item in uboot_role_list:
        uboot = UbootInfo(item[0], item[1])
        log_message(uboot.to_string())
        uboot.create_version()

@main.command()
def stop():
    '''destroy uboot info'''
    for item in uboot_role_list:
        uboot = UbootInfo(item[0], item[1])
        log_message(uboot.to_string())
        uboot.destroy_version()

# uboot info operation
if __name__ == '__main__':
    main()
