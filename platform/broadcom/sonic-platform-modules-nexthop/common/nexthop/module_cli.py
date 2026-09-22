# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import os
import sys

import click
from tabulate import tabulate

from nexthop.module_reload_lib import (
    MAX_WATCHDOG_SECONDS,
    ModuleReloadError,
    RELOAD_WATCHDOG_SECONDS,
    get_declared_modules,
    get_loaded_modules,
    is_pddf_module,
    reload_all_modules,
    reload_module,
)


def watchdog_seconds_option(command):
    return click.option(
        "--watchdog-seconds",
        type=click.IntRange(1, MAX_WATCHDOG_SECONDS),
        default=RELOAD_WATCHDOG_SECONDS,
        show_default=True,
        help="Seconds the FPGA watchdog stays armed while the reload runs; a "
        "reload stuck longer than this resets the switch. Raise it when "
        "root-causing a wedged teardown.",
    )(command)


def check_root_privileges():
    if os.getuid() != 0:
        click.secho("Root privileges required for this operation", fg="red")
        sys.exit(1)


def check_module_allowed(module_name, allow_any):
    if allow_any or is_pddf_module(module_name):
        return
    click.secho(
        f"'{module_name}' is not a PDDF/custom kernel module declared for this platform. "
        "Pass --any to operate on it anyway.",
        fg="red",
    )
    sys.exit(1)


@click.group()
def cli():
    pass


@cli.command("list")
def list_modules():
    """List PDDF/custom kernel modules declared for this platform and whether they're loaded."""
    pddf_kos, custom_kos = get_declared_modules()
    if not pddf_kos and not custom_kos:
        click.secho("No PDDF/custom kernel modules declared for this platform", fg="red")
        return

    loaded_modules = get_loaded_modules()

    rows = []
    for category, modules in (("pddf", pddf_kos), ("custom", custom_kos)):
        for module in modules:
            name = module.replace("-", "_")
            loaded = name in loaded_modules
            dependents = loaded_modules.get(name, [])
            rows.append([
                module,
                category,
                "yes" if loaded else "no",
                ", ".join(dependents) if dependents else "-",
            ])

    header = ["Module", "Category", "Loaded", "Used by"]
    click.echo(tabulate(rows, header, tablefmt="simple"))


@cli.command("reload")
@click.argument("module_name")
@click.option(
    "--any", "allow_any", is_flag=True, default=False,
    help="Allow reloading a module that isn't a declared PDDF/custom module for this platform.",
)
@watchdog_seconds_option
def reload(module_name, allow_any, watchdog_seconds):
    """Unload and reload MODULE_NAME (and any dependents), then re-modprobe them all.

    A PDDF-declared module goes through the full `pddf_util.py` teardown and
    reinstall; unloading one with its device nodes still in place panics the kernel,
    so there is no way to ask for the targeted path.

    Runs `depmod -a` before reloading, so a rebuilt or replaced .ko file is picked up.
    """
    check_root_privileges()
    check_module_allowed(module_name, allow_any)
    click.secho(f"Reloading {module_name}:", fg="cyan")
    try:
        reload_module(module_name, watchdog_seconds=watchdog_seconds)
    except ModuleReloadError as e:
        click.secho(f"Failed to reload {module_name}: {e}", fg="red")
        sys.exit(1)
    click.secho(f"Successfully reloaded {module_name}", fg="green")


@cli.command("reload-all")
@watchdog_seconds_option
def reload_all(watchdog_seconds):
    """Reload all PDDF/custom kernel modules and PDDF devices.

    Runs `pddf_util.py --force clean`, unloads whatever it leaves behind in
    dependency order, then `pddf_util.py install`. The FPGA watchdog stays armed
    with punching paused for the duration (see --watchdog-seconds), and kernel
    messages are kept off the console while it runs, so an operator watching the
    console sees nothing until it finishes; `dmesg` still has everything.

    Unlike `reload MODULE_NAME`, this does not run `depmod -a`, so a newly added
    .ko file is not picked up.
    """
    check_root_privileges()
    click.secho("Reloading all PDDF/custom kernel modules and devices:", fg="cyan")
    try:
        reload_all_modules(watchdog_seconds=watchdog_seconds)
    except ModuleReloadError as e:
        click.secho(f"Failed to reload all PDDF/custom kernel modules and devices: {e}", fg="red")
        sys.exit(1)
    click.secho("Successfully reloaded all PDDF/custom kernel modules and devices", fg="green")


if __name__ == "__main__":
    cli()
