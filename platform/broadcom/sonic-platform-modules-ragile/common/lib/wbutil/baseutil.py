#!/usr/bin/env python3
import os


def get_machine_info():
    if not os.path.isfile('/host/machine.conf'):
        return None
    machine_vars = {}
    with open('/host/machine.conf') as machine_file:
        for line in machine_file:
            tokens = line.split('=')
            if len(tokens) < 2:
                continue
            machine_vars[tokens[0]] = tokens[1].strip()
    return machine_vars


def get_platform_info(machine_info):
    if machine_info is not None:
        if 'onie_platform' in machine_info:
            return machine_info['onie_platform']
        if 'aboot_platform' in machine_info:
            return machine_info['aboot_platform']
    return None


def get_board_id(machine_info):
    if machine_info is not None:
        if 'onie_board_id' in machine_info:
            return machine_info['onie_board_id'].lower()
    return "NA"


def get_onie_machine(machine_info):
    if machine_info is not None:
        if 'onie_machine' in machine_info:
            return machine_info['onie_machine']
    return None
