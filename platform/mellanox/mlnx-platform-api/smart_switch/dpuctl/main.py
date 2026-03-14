#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2024-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

"""Click Implemenetation for dpuctl related commands"""
from multiprocessing import Process
from tabulate import tabulate
try:
    import click
    from sonic_platform.dpuctlplat import DpuCtlPlat
except ImportError as e:
    raise ImportError(str(e) + '- required module not found') from e


def call_dpu_reset(obj, force):
    """Function to call object specific Reset for each dpu"""
    try:
        obj.dpu_reboot(force)
    except Exception as error:
        print(f"An error occurred: {type(error).__name__} - {error}")


def call_dpu_power_on(obj, force):
    """Function to call object specific power on for each dpu"""
    try:
        obj.dpu_power_on(force)
    except Exception as error:
        print(f"An error occurred: {type(error).__name__} - {error}")


def call_dpu_power_off(obj, force):
    """Function to call object specific power off for each dpu"""
    try:
        obj.dpu_power_off(force)
    except Exception as error:
        print(f"An error occurred: {type(error).__name__} - {error}")


def call_dpu_status_update(obj):
    """Function to call object specific status update for each dpu"""
    try:
        obj.dpu_status_update()
    except Exception as error:
        print(f"An error occurred: {type(error).__name__} - {error}")


def validate_return_dpus(all_dpus, dpu_names, dpuctl_list):
    """Function to validate list of dpus provided by User"""
    if (((not all_dpus) and (dpu_names is None)) or (all_dpus and (dpu_names is not None))):
        raise AssertionError("Invalid Arguments provided!"
                             "Please provide either dpu_names or -all option")

    if all_dpus:
        return dpuctl_list
    dpu_names_l = dpu_names.split(',')
    dpu_names_l = [dpu_name.strip() for dpu_name in dpu_names_l]
    for provided_dpu in dpu_names_l:
        if provided_dpu not in dpuctl_list:
            raise AssertionError("Invalid Arguments provided!"
                                 f"{provided_dpu} does not exist!")
    return dpu_names_l


def execute_function_call(ctx,
                          all_dpus,
                          force,
                          dpu_names,
                          function_to_call,
                          verbose=None):
    """Function to fork multiple child process for each DPU
       and call required function"""
    try:
        dpuctl_dict = ctx.obj['dpuctl_dict']
        selected_dpus = validate_return_dpus(all_dpus, dpu_names, dpuctl_dict.keys())
        selected_dpus = list(set(selected_dpus))
        proc_list = []
        for dpu_name, dpu_obj in dpuctl_dict.items():
            if verbose:
                dpu_obj.verbosity = True
            if dpu_name in selected_dpus:
                if function_to_call == "PW_ON":
                    proc = Process(target=call_dpu_power_on, args=(dpu_obj, force))
                elif function_to_call == "PW_OFF":
                    proc = Process(target=call_dpu_power_off,
                                   args=(dpu_obj, force))
                elif function_to_call == "RST":
                    proc = Process(target=call_dpu_reset, args=(dpu_obj, force))
                proc_list.append(proc)
        for proc in proc_list:
            proc.start()
        for proc in proc_list:
            proc.join()
    except Exception as error:
        print(f"An error occurred: {type(error).__name__} - {error}")


@click.group()
@click.pass_context
def dpuctl(ctx=None):
    """SONiC command line - 'dpuctl' Wrapper command:
        Smart Switch DPU reset flow commands"""
    # Hardcoded HW-mgmt names
    try:
        existing_dpu_list=['dpu0', 'dpu1', 'dpu2', 'dpu3']
        # dpu0 in Platform.json = dpu1 in HW-mgmt
        dpuctl_dict = {}
        for dpu_name in existing_dpu_list:
            dpu_obj = DpuCtlPlat(dpu_name)
            dpu_obj.setup_logger(use_print=True)
            dpuctl_dict[dpu_name] = dpu_obj
        context = {
            "dpuctl_dict": dpuctl_dict,
        }
        ctx.obj = context
    except Exception as error:
        print(f"An error occurred: {type(error).__name__} - {error}")
        ctx.exit()


@dpuctl.command(name='dpu-reset')
@click.option('--force', '-f', is_flag=True, default=False, help='Perform force Reboot - Turned off by default')
@click.option('--all', '-a', 'all_dpus', is_flag=True, default=False, help='Execute for all DPUs')
@click.argument('dpu_names', metavar='<dpu_names>', required=False)
@click.option('--verbose', '-v', 'verbose', is_flag=True, default=False, help='Print debug messages')
@click.pass_context
def dpuctl_reset(ctx, force, all_dpus, verbose, dpu_names=None):
    """Reboot individual or all DPUs"""
    execute_function_call(ctx, all_dpus, force, dpu_names, "RST", verbose)


@dpuctl.command(name='dpu-power-on')
@click.option('--force', '-f', is_flag=True, default=False, help='Perform force power on - Turned off by default')
@click.option('--all', '-a', 'all_dpus', is_flag=True, default=False, help='Execute on all DPUs')
@click.option('--verbose', '-v', 'verbose', is_flag=True, default=False, help='Print debug messages')
@click.argument('dpu_names', metavar='<dpu_names>', required=False)
@click.pass_context
def dpuctl_power_on(ctx, force, all_dpus, verbose, dpu_names=None):
    """Power On individual or all DPUs"""
    execute_function_call(ctx, all_dpus, force, dpu_names, "PW_ON", verbose)


@dpuctl.command(name='dpu-power-off')
@click.option('--force', '-f', is_flag=True, default=False, help='Perform force power off Turned of by default')
@click.option('--all', '-a', 'all_dpus', is_flag=True, default=False, help='Execute on all DPUs')
@click.option('--verbose', '-v', 'verbose', is_flag=True, default=False, help='Print debug messages')
@click.argument('dpu_names', metavar='<dpu_names>', required=False)
@click.pass_context
def dpuctl_power_off(ctx, force, all_dpus, verbose, dpu_names=None):
    """Power Off individual or all DPUs"""
    execute_function_call(ctx, all_dpus, force, dpu_names, "PW_OFF", verbose)


@dpuctl.command(name='dpu-status')
@click.argument('dpu_names', metavar='<dpu_names>', required=False)
@click.pass_context
def dpuctl_get_status(ctx, all_dpus=False, dpu_names=None):
    """Obtain current status of the DPUs"""
    try:
        if not dpu_names:
            all_dpus = True
        dpuctl_dict = ctx.obj['dpuctl_dict']
        selected_dpus = validate_return_dpus(all_dpus, dpu_names, dpuctl_dict.keys())
        selected_dpus = list(set(selected_dpus))
        status_list = []
        for dpu_name, dpu_obj in dpuctl_dict.items():
            if dpu_name in selected_dpus:
                call_dpu_status_update(dpu_obj)
                dpu_status_list = [dpu_name,
                                   dpu_obj.dpu_ready_indication,
                                   dpu_obj.dpu_shtdn_ready_indication,
                                   dpu_obj.boot_prog_indication,
                                   dpu_obj.dpu_force_pwr_indication]
                status_list.append(dpu_status_list)
        header = ['DPU', 'dpu ready', 'dpu shutdown ready', 'boot progress', 'Force Power Required']
        click.echo(tabulate(status_list, header))
    except Exception as error:
        print(f"An error occurred: {type(error).__name__} - {error}")


if __name__ == '__main__':
    dpuctl()
