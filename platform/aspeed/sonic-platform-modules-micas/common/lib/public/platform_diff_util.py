#!/usr/bin/env python_nos

import sys
library_path = '/usr/local/bin'
sys.path.append(library_path)
from platform_util import set_value, exec_os_cmd

def platform_process_other_init(global_init_param, global_init_command):
    # Subsequent products disable the GLOBALINITPARAM configuration
    for index in global_init_param:
        set_value(index)

    for index in global_init_command:
        if isinstance(index, dict):
            set_value(index)
        else:
            exec_os_cmd(index)

def platform_process_other_init_pre(global_init_param_pre, global_init_command_pre):
    # Subsequent products disable the GLOBALINITPARAM configuration
    for index in global_init_param_pre:
        set_value(index)
    for index in global_init_command_pre:
        if isinstance(index, dict):
            set_value(index)
        else:
            exec_os_cmd(index)

