#!/usr/bin/env python
import sys
import os
import time
import syslog
import click
import logging
from platform_config import get_config_param
from platform_util import exec_os_cmd, setup_logger, BSP_COMMON_LOG_DIR, AliasedGroup, CONTEXT_SETTINGS

'''
config demo
PLATFORM_MDIO_SWITCH_CONFIG = {
    "mdio_index": 1, 
    "reset": "phy",
    "name": "BCM53134O",   #same as the name in upgrade.py, used for eeprom erase/write
    "size": 128
}
'''
PLATFORM_MDIO_SWITCH_CONFIG = get_config_param("PLATFORM_MDIO_SWITCH_CONFIG", None)
DEBUG_FILE = "/etc/.mdio_util_debug_flag"
LOG_FILE = BSP_COMMON_LOG_DIR + "mdio_util_debug.log"
logger = setup_logger(LOG_FILE)

def mdio_util_debug(s):
    logger.debug(s)

def mdio_util_error(s):
    logger.error(s)

def mdio_util_info(s):
    logger.info(s)

def debug_init():
    if os.path.exists(DEBUG_FILE):
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''MDIO utility for OOB switch register access'''
    debug_init()
    if PLATFORM_MDIO_SWITCH_CONFIG is None:
        mdio_util_error("PLATFORM_MDIO_SWITCH_CONFIG not configured")
        click.echo("Error: PLATFORM_MDIO_SWITCH_CONFIG not configured", err=True)
        sys.exit(1)

@main.command()
@click.argument('addr', type=str)
@click.argument('register', type=str)
@click.argument('size_arg', required=False, type=str)
@click.option('--size', 'size_option', default=None, type=str, help='Read size compatibility option')
def read(addr, register, size_arg, size_option):
    '''Read register: mdio-util read <addr> <register> [size]'''
    _ = size_arg, size_option
    try:
        reg_base = int(register, 16) if register.startswith('0x') else int(register)
    except ValueError:
        mdio_util_error(f"Invalid register format: {register}")
        click.echo(f"Error: invalid register format: {register}", err=True)
        sys.exit(1)
    mdio_index = PLATFORM_MDIO_SWITCH_CONFIG.get("mdio_index", 1)
    reg = hex(reg_base)
    cmd = f"dfd_debug mdiodev_rd {mdio_index} {addr} {reg}"
    mdio_util_debug(f"Executing read command: {cmd}")
    status, output = exec_os_cmd(cmd)
    if status != 0:
        mdio_util_error(f"Read failed for register {reg}: {output}")
        click.echo(f"Error: read failed for register {reg}: {output}", err=True)
        sys.exit(1)
    mdio_util_debug("Read successful")
    click.echo(f"{reg}: {output.strip()}")

@main.command()
@click.argument('addr', type=str)
@click.argument('register', type=str)
@click.argument('value', type=str)
@click.argument('size_arg', required=False, type=str)
@click.option('--size', 'size_option', default=None, type=str, help='Write size compatibility option')
def write(addr, register, value, size_arg, size_option):
    '''Write register: mdio-util write <addr> <register> <value> [size]'''
    _ = size_arg, size_option
    try:
        reg_base = int(register, 16) if register.startswith('0x') else int(register)
    except ValueError:
        mdio_util_error(f"Invalid register format: {register}")
        click.echo(f"Error: invalid register format: {register}", err=True)
        sys.exit(1)
    mdio_index = PLATFORM_MDIO_SWITCH_CONFIG.get("mdio_index", 1)
    reg = hex(reg_base)
    cmd = f"dfd_debug mdiodev_wr {mdio_index} {addr} {reg} {value}"
    mdio_util_debug(f"Executing write command: {cmd}")
    status, output = exec_os_cmd(cmd)
    if status != 0:
        mdio_util_error(f"Write failed for register {reg}: {output}")
        click.echo(f"Error: write failed for register {reg}: {output}", err=True)
        sys.exit(1)
    mdio_util_debug("Write successful")
    click.echo("Write successful")

@main.command()
def reset():
    '''Reset OOB switch: mdio-util reset'''
    if not click.confirm('Are you sure you want to reset the OOB switch?'):
        mdio_util_debug("Reset cancelled by user")
        click.echo("Reset cancelled")
        return
    reset_name = PLATFORM_MDIO_SWITCH_CONFIG.get("reset")
    if not reset_name or not isinstance(reset_name, str):
        mdio_util_error("Reset name not configured")
        click.echo("Error: Reset name not configured", err=True)
        sys.exit(1)
    cmd = f"echo y | reboot_ctrl.py reset {reset_name}"
    mdio_util_debug(f"Executing reset command: {cmd}")
    status, output = exec_os_cmd(cmd)
    if status != 0:
        mdio_util_error(f"Reset failed: {output}")
        click.echo(f"Error: {output}", err=True)
        sys.exit(1)
    mdio_util_debug("Reset successful")
    click.echo("Reset successful")

@main.group()
def eeprom():
    '''EEPROM operations for OOB switch'''
    pass

@eeprom.command()
def erase():
    '''Erase EEPROM: mdio-util eeprom erase'''
    if not click.confirm('Are you sure you want to erase the EEPROM?'):
        mdio_util_debug("Erase cancelled by user")
        click.echo("Erase cancelled")
        return
    ee_name = PLATFORM_MDIO_SWITCH_CONFIG.get("name")
    if not ee_name:
        mdio_util_error("EEPROM name not configured in PLATFORM_MDIO_SWITCH_CONFIG")
        click.echo("Error: EEPROM name not configured", err=True)
        sys.exit(1)
    ee_size = PLATFORM_MDIO_SWITCH_CONFIG.get("size")
    if not ee_size:
        mdio_util_error("EEPROM size not configured in PLATFORM_MDIO_SWITCH_CONFIG")
        click.echo("Error: EEPROM size not configured", err=True)
        sys.exit(1)
    # Generate a temporary bin file filled with 0xFF
    temp_file = f"/tmp/eeprom_erase_{ee_name}.bin"
    try:
        with open(temp_file, 'wb') as f:
            f.write(b'\xFF' * ee_size)
        mdio_util_debug(f"Generated erase file: {temp_file} with size {ee_size} bytes")
    except Exception as e:
        mdio_util_error(f"Failed to generate erase file: {e}")
        click.echo(f"Error: Failed to generate erase file: {e}", err=True)
        sys.exit(1)
    
    try:
        cmd = f"upgrade.py cold {temp_file} -n {ee_name}"
        mdio_util_debug(f"Executing EEPROM erase via upgrade command: {cmd}")
        status, output = exec_os_cmd(cmd)
        if status == 0:
            mdio_util_debug(f"EEPROM erase succeeded: {output}")
            click.echo("EEPROM erase successful")
        else:
            mdio_util_error(f"EEPROM erase failed: {output}")
            click.echo(f"Error: {output}", err=True)
            sys.exit(1)
    finally:
        # Clean up temporary file
        if os.path.exists(temp_file):
            os.remove(temp_file)
            mdio_util_debug(f"Cleaned up temporary file: {temp_file}")

@eeprom.command()
@click.argument('file', required=False, type=click.Path(exists=True))
def write(file):
    '''Write EEPROM: mdio-util eeprom write [file]'''
    if not click.confirm('Are you sure you want to write to the EEPROM?'):
        mdio_util_debug("Write cancelled by user")
        click.echo("Write cancelled")
        return
    ee_name = PLATFORM_MDIO_SWITCH_CONFIG.get("name")
    if not ee_name:
        mdio_util_error("EEPROM name not configured in PLATFORM_MDIO_SWITCH_CONFIG")
        click.echo("Error: EEPROM name not configured", err=True)
        sys.exit(1)
    if not file:
        mdio_util_error("No file specified for EEPROM write")
        click.echo("Error: file must be provided for write", err=True)
        sys.exit(1)
    cmd = f"upgrade.py cold {file} -n {ee_name}"
    mdio_util_debug(f"Executing EEPROM write via upgrade command: {cmd}")
    status, output = exec_os_cmd(cmd)
    if status == 0:
        mdio_util_debug(f"EEPROM write succeeded: {output}")
        click.echo("EEPROM write successful")
    else:
        mdio_util_error(f"EEPROM write failed: {output}")
        click.echo(f"Error: {output}", err=True)
        sys.exit(1)

if __name__ == '__main__':
    main()