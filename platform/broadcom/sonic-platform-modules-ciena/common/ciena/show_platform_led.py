#!/usr/bin/env python3
# Copyright 2026 Ciena Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
SONiC CLI plugin: ``show platform led``

Adds a ``led`` subcommand to ``show platform`` that reads all front-panel
system LEDs on the Ciena platform directly from sysfs.

The plugin is auto-loaded by the SONiC show CLI plugin framework when
placed in /usr/local/lib/python3.11/dist-packages/show/plugins/.
"""

import os
import click
from tabulate import tabulate

_LED_SYSFS = "/sys/class/leds"


def _read(path):
    """Read a sysfs file, return stripped content or None."""
    try:
        with open(path, "r") as f:
            return f.read().strip()
    except OSError:
        return None


def _led_state(sysfs_name):
    """Return (brightness, blink) tuple for a LED sysfs node."""
    base = os.path.join(_LED_SYSFS, sysfs_name)
    br = _read(os.path.join(base, "brightness"))
    blk = _read(os.path.join(base, "blink"))
    return br, blk


def _color_str(brightness, blink):
    """Format brightness+blink into a human-readable state string."""
    if brightness is None:
        return "N/A"
    if brightness == "0":
        return "off"
    state = "on"
    if blink == "1":
        state = "blinking"
    return state


def _get_combined_status():
    """Derive the combined system status (what system-health shows).

    STATUS green + ALARM yellow work as a pair:
      green on        -> green / green (blinking)
      yellow on       -> amber / amber (blinking)
      both off        -> off
    """
    grn_br, grn_blk = _led_state("front:green:status")
    ylw_br, ylw_blk = _led_state("front:yellow:alarm")
    if grn_br == "1":
        return "green (blinking)" if grn_blk == "1" else "green"
    if ylw_br == "1":
        return "amber (blinking)" if ylw_blk == "1" else "amber"
    return "off"


def _get_sync_state():
    """Read the multi-colour SYNC LED (green / red / yellow nodes)."""
    for sysfs_name, color in [("front:green:sync", "green"),
                               ("front:red:sync", "red"),
                               ("front:yellow:sync", "yellow")]:
        br, blk = _led_state(sysfs_name)
        if br == "1":
            return "{} (blinking)".format(color) if blk == "1" else color
    return "off"


# Table of LEDs to display:
#   (name, description, callable returning state string)
_SYSTEM_LEDS = [
    ("STATUS", "Front panel status (green)",
     lambda: _color_str(*_led_state("front:green:status"))),
    ("ALARM", "Front panel alarm (yellow)",
     lambda: _color_str(*_led_state("front:yellow:alarm"))),
    ("SYNC", "Front panel sync (multi-color)",
     _get_sync_state),
    ("PSA", "PSU-A power OK (hw-driven)",
     lambda: _color_str(_led_state("psa:green:ok")[0], None)),
    ("PSB", "PSU-B power OK (hw-driven)",
     lambda: _color_str(_led_state("psb:green:ok")[0], None)),
]


@click.command()
def led():
    """Show front-panel system LED status"""
    rows = []
    for name, description, state_fn in _SYSTEM_LEDS:
        try:
            state = state_fn()
        except Exception:
            state = "N/A"
        rows.append([name, description, state])

    click.echo(tabulate(
        rows,
        headers=["LED", "Description", "State"],
        tablefmt="simple",
    ))
    click.echo("")
    click.echo("System status LED (combined): {}".format(_get_combined_status()))


def register(cli):
    """Register the 'led' command under 'show platform'."""
    if "platform" in cli.commands:
        cli.commands["platform"].add_command(led)
