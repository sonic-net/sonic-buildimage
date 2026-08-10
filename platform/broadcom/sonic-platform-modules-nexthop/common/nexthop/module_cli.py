# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import os
import sys

import click
from tabulate import tabulate

from nexthop.module_reload_lib import (
    ModuleReloadError,
    get_declared_modules,
    get_loaded_modules,
    is_pddf_module,
    reload_all_modules,
    reload_module,
    unload_module,
)


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


@cli.command("unload")
@click.argument("module_name")
@click.option(
    "--recursive/--no-recursive",
    default=True,
    help="Unload dependent modules first (default). With --no-recursive, fails if the module is in use.",
)
@click.option(
    "--any", "allow_any", is_flag=True, default=False,
    help="Allow unloading a module that isn't a declared PDDF/custom module for this platform.",
)
def unload(module_name, recursive, allow_any):
    """Unload MODULE_NAME, unloading its dependents first if needed."""
    check_root_privileges()
    check_module_allowed(module_name, allow_any)
    click.secho(f"Unloading {module_name}:", fg="cyan")
    try:
        unload_module(module_name, recursive=recursive)
    except ModuleReloadError as e:
        click.secho(f"Failed to unload {module_name}: {e}", fg="red")
        sys.exit(1)
    click.secho(f"Successfully unloaded {module_name}", fg="green")


@cli.command("reload")
@click.argument("module_name")
@click.option(
    "--recreate-devices/--no-recreate-devices",
    default=None,
    help="Recreate PDDF sysfs device nodes afterwards. Defaults to on when MODULE_NAME is a "
    "PDDF-declared module, off otherwise.",
)
@click.option(
    "--any", "allow_any", is_flag=True, default=False,
    help="Allow reloading a module that isn't a declared PDDF/custom module for this platform.",
)
def reload(module_name, recreate_devices, allow_any):
    """Unload and reload MODULE_NAME (and any dependents), then re-modprobe them all.

    Runs `depmod -a` before reloading, so a rebuilt or replaced .ko file is picked up.
    """
    check_root_privileges()
    check_module_allowed(module_name, allow_any)
    click.secho(f"Reloading {module_name}:", fg="cyan")
    try:
        reload_module(module_name, recreate_devices=recreate_devices)
    except ModuleReloadError as e:
        click.secho(f"Failed to reload {module_name}: {e}", fg="red")
        sys.exit(1)
    click.secho(f"Successfully reloaded {module_name}", fg="green")


@cli.command("reload-all")
def reload_all():
    """Reload all PDDF/custom kernel modules and PDDF devices via `pddf_util.py clean && install`."""
    check_root_privileges()
    click.secho("Reloading all PDDF/custom kernel modules and devices:", fg="cyan")
    try:
        reload_all_modules()
    except ModuleReloadError as e:
        click.secho(f"Failed to reload all PDDF/custom kernel modules and devices: {e}", fg="red")
        sys.exit(1)
    click.secho("Successfully reloaded all PDDF/custom kernel modules and devices", fg="green")


if __name__ == "__main__":
    cli()
