# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""``nh_voltage_margin`` -- margin a configured voltage rail high/low.

    nh_voltage_margin POS0V78_VDDC_D0 high
    nh_voltage_margin POS0V78_VDDC_D0 low
    nh_voltage_margin POS0V78_VDDC_D0 nominal      # restore
    nh_voltage_margin POS0V78_VDDC_D0 read         # read-only report
    nh_voltage_margin all read

Only rails declared in the platform's voltage_margin_config.json can be
margined; the per-rail absolute target and the [min, max] clamp come from that
file.
"""

import os
import sys

import click

from nexthop import voltage_margin as vm

LEVEL_READ = "read"
CHOICES = [vm.LEVEL_HIGH, vm.LEVEL_LOW, vm.LEVEL_NOMINAL, LEVEL_READ]


def _check_root():
    if os.getuid() != 0:
        click.secho("Root privileges required for this operation", fg="red")
        sys.exit(1)


@click.command(context_settings={"help_option_names": ["-h", "--help"]})
@click.argument("rail")
@click.argument("level", type=click.Choice(CHOICES))
@click.option("--target-mv", type=float, default=None, hidden=True)
@click.option("--offset-mv", type=float, default=None, hidden=True)
@click.option("--dry-run", is_flag=True,
              help="Print every transaction; execute nothing.")
@click.option("-v", "--verbose", is_flag=True,
              help="Echo every register access.")
def cli(rail, level, target_mv, offset_mv, dry_run, verbose):
    """Margin the RAIL to LEVEL (high, low, nominal, read).

    RAIL is a rail declared in voltage_margin_config.json, or 'all'.
    """
    if (target_mv is not None or offset_mv is not None) \
            and level in (vm.LEVEL_NOMINAL, LEVEL_READ):
        raise click.BadParameter(
            "--target-mv/--offset-mv only apply to 'high' or 'low'",
            param_hint="--target-mv/--offset-mv")
    if target_mv is not None and offset_mv is not None:
        raise click.BadParameter("--target-mv and --offset-mv are mutually exclusive",
                                 param_hint="--target-mv/--offset-mv")
    if not dry_run:
        _check_root()

    try:
        rails = vm.load_config()
    except vm.VoltageMarginError as e:
        click.secho("error: %s" % e, fg="red")
        sys.exit(1)

    if rail == "all":
        names = sorted(rails)
    else:
        # Validate against the allowlist up front for a clean error.
        try:
            vm.get_rail(rails, rail)
        except vm.VoltageMarginError as e:
            click.secho("error: %s" % e, fg="red")
            sys.exit(1)
        names = [rail]

    failures = 0
    for name in names:
        try:
            cfg = vm.get_rail(rails, name)
            if level == LEVEL_READ:
                vm.read_rail(name, cfg, dry_run=dry_run, verbose=verbose)
            else:
                vm.set_rail(name, cfg, level, target_mv=target_mv,
                            offset_mv=offset_mv, dry_run=dry_run, verbose=verbose)
        except vm.VoltageMarginError as e:
            click.secho("  ! %s: %s" % (name, e), fg="red")
            failures += 1

    sys.exit(1 if failures else 0)


if __name__ == "__main__":
    cli()
