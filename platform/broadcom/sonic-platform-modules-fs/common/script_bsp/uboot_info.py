#!/usr/bin/env python3
# -*- coding: UTF-8 -*-

import click
from pathlib import Path
import os
import subprocess
from platform_util import log_to_file, get_value
from platform_config import UBOOT_INFO_CONF

# Constants
LOG_DIRECTORY = '/var/log/bsp_tech'
# Ensure log directory exists
os.makedirs(LOG_DIRECTORY, exist_ok=True)
LOG_FILE_PATH = os.path.join(LOG_DIRECTORY, 'uboot_info_debug.log')
LOG_WRITE_SIZE = 1 * 1024 * 1024  # 1 MB

UBOOT_ROLE_LIST = [
    ["current", "master"],
    ["current", "slave"],
    ["other", "master"],
    ["other", "slave"]
]

CONTEXT_SETTINGS = dict(help_option_names=['-h', '--help'])

class AliasedGroup(click.Group):
    def get_command(self, ctx, cmd_name):
        rv = super().get_command(ctx, cmd_name)
        if rv is not None:
            return rv
        matches = [x for x in self.list_commands(ctx) if x.startswith(cmd_name)]
        if not matches:
            return None
        elif len(matches) == 1:
            return super().get_command(ctx, matches[0])
        ctx.fail(f'Too many matches: {", ".join(sorted(matches))}')

def log_message(message):
    """Print the message and log it to the log file."""
    log_to_file(message, LOG_FILE_PATH, LOG_WRITE_SIZE)

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
        log_message(f"Create file failed: {e}")
        return False

class UbootInfo:
    def __init__(self, role, target, active):
        self.app = "show_version"
        self.role = role
        self.target = target
        self.destination = self._build_destination(active)

    def _build_destination(self, active):
        """Build the destination path based on the role and target."""
        base_path = "/tmp/version/uboot/"
        if active == "master":
            if self.role == "current":
                base_path += "hw_master/" 
            else:
                base_path += "hw_slave/"
        else:
            if self.role == "current":
                base_path += "hw_slave/" 
            else:
                base_path += "hw_master/"

        if self.target == "master":
            base_path += "sw_master"
        else:
            base_path += "sw_slave"

        return base_path

    def create_version(self):
        """Create the UBOOT version file."""
        if not check_and_create_file(self.destination):
            log_message(f"Create file: {self.destination} failed.")
            return False
        try:
            with open(self.destination, 'w') as outfile:
                subprocess.run([self.app, self.role, self.target], stdout=outfile, check=True)
            log_message(f"Create uboot({self.role}, {self.target}) version success.")
            return True
        except subprocess.CalledProcessError as e:
            log_message(f"Command failed: {e}")
            return False
        except Exception as e:
            log_message(str(e))
            return False

    def destroy_version(self):
        """Destroy the UBOOT version file."""
        file_path = Path(self.destination)
        try:
            file_path.unlink()
            log_message(f"Destroy uboot({self.role}, {self.target}) version success.")
            return True
        except FileNotFoundError:
            log_message("File does not exist.")
            return True
        except Exception as e:
            log_message(str(e))
            return False

    def __str__(self):
        return f"role: {self.role}, target: {self.target}, dest: {self.destination}, app: {self.app}."

def uboot_info_get_active_role():
    """Get the active role of UBOOT."""
    active_role_conf = UBOOT_INFO_CONF
    try:
        for item in active_role_conf:
            conf = item.get("value")
            ret, val = get_value(conf)
            if not ret:
                log_message(f"Get active uboot from {conf} failed.")
                continue
            log_message(f"value is {val:#x}, match is {conf.get('okval'):#x}.")
            if (val & conf.get('mask')) == conf.get("okval"):
                log_message(f"Current role is {item.get('active')}.")
                return item.get("active")
        log_message("Cannot get current active role.")
    except Exception as e:
        log_message(str(e))
    return None

@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    pass

@main.command()
def start():
    """Create uboot info."""
    log_message("Create uboot info.")
    active_role = uboot_info_get_active_role()
    if active_role is None:
        log_message("Cannot get current active role.")
        return

    for role, target in UBOOT_ROLE_LIST:
        uboot = UbootInfo(role, target, active_role)
        log_message(str(uboot))
        uboot.create_version()

@main.command()
def stop():
    """Destroy uboot info."""
    for role, target in UBOOT_ROLE_LIST:
        uboot = UbootInfo(role, target, "master")
        log_message(str(uboot))
        uboot.destroy_version()

if __name__ == '__main__':
    main()