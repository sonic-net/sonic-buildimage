#!/usr/bin/env python3

import glob
try:
    import click
except ImportError as error:
    pass

import sys
library_path = '/usr/local/bin'
sys.path.append(library_path)
from platform_util import set_value, exec_os_cmd

def write_sysfs_value(reg_name, value):
    mb_reg_file = "/sys/bus/i2c/devices/" + reg_name
    locations = glob.glob(mb_reg_file)
    if len(locations) == 0:
        print("%s not found" % mb_reg_file)
        return False
    sysfs_loc = locations[0]
    try:
        with open(sysfs_loc, 'w') as fd:
            fd.write(value)
    except Exception:
        return False
    return True

def platform_process_other_init(global_init_param, global_init_command):
    # Subsequent products disable the GLOBALINITPARAM configuration
    for index in global_init_param:
        if isinstance(index, dict):
            loc = index.get("loc", None)
            value = index.get("value", None)
            if loc and value:
                write_sysfs_value(loc, value)
            else:
                click.echo("%%WB_PLATFORM_PROCESS: failed to initialize the parameter, loc or value is None, config %s" % index)
        else:
            click.echo("%%WB_PLATFORM_PROCESS: failed to initialize the parameter, the config %s is not a dict type" % index)

    for index in global_init_command:
        if isinstance(index, dict):
            set_value(index)
        else:
            exec_os_cmd(index)

def platform_process_other_init_pre(global_init_param_pre, global_init_command_pre):
    # Subsequent products disable the GLOBALINITPARAM configuration
    for index in global_init_param_pre:
        if isinstance(index, dict):
            loc = index.get("loc", None)
            value = index.get("value", None)
            if loc and value:
                write_sysfs_value(loc, value)
            else:
                click.echo("%%WB_PLATFORM_PROCESS: failed to initialize the parameter, loc or value is None, config %s" % index)
        else:
            click.echo("%%WB_PLATFORM_PROCESS: failed to initialize the parameter, the config %s is not a dict type" % index)

    for index in global_init_command_pre:
        if isinstance(index, dict):
            set_value(index)
        else:
            exec_os_cmd(index)

