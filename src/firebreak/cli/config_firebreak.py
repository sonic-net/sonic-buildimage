"""config firebreak — host-side enable/disable/reload (no CONFIG_DB policy).

Policy profiles stay in /etc/sonic/firebreak/*.json. Secrets stay host-local.
Collector IPs remain in feature TELEMETRY/OTEL tables; firebreakd watches them.

  config firebreak enable                 # start daemon (all active profiles)
  config firebreak disable                # stop daemon + restore stock mounts
  config firebreak enable <container>     # activate profile + protect one ctr
  config firebreak disable <container>    # *.json.disabled + unprotect one ctr
  config firebreak reload                 # SIGHUP: sync protect/unprotect + refresh
"""

from __future__ import annotations

import os
import subprocess
from pathlib import Path

import click
from utilities_common.cli import AbbreviationGroup

CONFIG_DIR = Path("/etc/sonic/firebreak")


def _run(cmd, check=False):
    click.echo("Running command: " + " ".join(cmd))
    p = subprocess.run(cmd, capture_output=True, text=True, check=False)
    if p.stdout:
        click.echo(p.stdout.strip())
    if p.stderr:
        click.echo(p.stderr.strip())
    if check and p.returncode != 0:
        raise SystemExit(p.returncode)
    return p.returncode


def _sighup() -> int:
    rc = _run(["systemctl", "reload", "firebreak.service"])
    if rc != 0:
        rc = _run(["systemctl", "kill", "-s", "HUP", "firebreak.service"])
    return rc


def _profile_paths(container: str) -> tuple[Path, Path]:
    active = CONFIG_DIR / ("%s.json" % container)
    disabled = CONFIG_DIR / ("%s.json.disabled" % container)
    return active, disabled


@click.group(cls=AbbreviationGroup, name="firebreak")
def firebreak():
    """Configure FIREBREAK host daemon (enable/disable/reload)"""
    pass


@firebreak.command("enable")
@click.argument("container", required=False)
def enable(container):
    """Enable daemon, or enable+protect one container profile"""
    if not container:
        _run(["systemctl", "enable", "firebreak.service"])
        _run(["systemctl", "start", "firebreak.service"], check=True)
        _run(["systemctl", "is-active", "firebreak.service"])
        return

    active, disabled = _profile_paths(container)
    if disabled.is_file() and not active.is_file():
        os.rename(disabled, active)
        click.echo("Activated profile %s" % active)
    elif not active.is_file():
        click.echo("No profile for %s in %s" % (container, CONFIG_DIR), err=True)
        raise SystemExit(1)

    # Ensure daemon is up, then SIGHUP so firebreakd protect()s the new profile.
    _run(["systemctl", "enable", "firebreak.service"])
    st = subprocess.run(
        ["systemctl", "is-active", "firebreak.service"],
        capture_output=True,
        text=True,
    )
    if st.stdout.strip() != "active":
        _run(["systemctl", "start", "firebreak.service"], check=True)
    else:
        if _sighup() != 0:
            click.echo("reload failed after enabling %s" % container, err=True)
            raise SystemExit(1)
    click.echo("FIREBREAK enable %s signaled (brief container recreate on first protect)." % container)


@firebreak.command("disable")
@click.argument("container", required=False)
def disable(container):
    """Disable daemon (restore mounts), or disable+unprotect one container"""
    if not container:
        # Daemon stop handler runs unprotect() for each protected container.
        _run(["systemctl", "stop", "firebreak.service"])
        _run(["systemctl", "disable", "firebreak.service"])
        click.echo(
            "FIREBREAK disabled. Protected containers were restored to stock Redis mounts."
        )
        return

    active, disabled = _profile_paths(container)
    if active.is_file():
        os.rename(active, disabled)
        click.echo("Disabled profile %s" % disabled)
    elif disabled.is_file():
        click.echo("Profile already disabled: %s" % disabled)
    else:
        click.echo("No profile for %s in %s" % (container, CONFIG_DIR), err=True)
        raise SystemExit(1)

    st = subprocess.run(
        ["systemctl", "is-active", "firebreak.service"],
        capture_output=True,
        text=True,
    )
    if st.stdout.strip() == "active":
        if _sighup() != 0:
            click.echo("reload failed after disabling %s" % container, err=True)
            raise SystemExit(1)
        click.echo(
            "FIREBREAK disable %s signaled (container recreate to restore stock mount)."
            % container
        )
    else:
        click.echo("Daemon inactive — profile disabled on disk only.")


@firebreak.command("reload")
def reload_cmd():
    """Sync profiles (protect new / unprotect disabled) + refresh Lock1/Lock2"""
    rc = _sighup()
    if rc != 0:
        click.echo("reload failed — is firebreak.service active?", err=True)
        raise SystemExit(1)
    click.echo("FIREBREAK reload signaled (protect/unprotect sync + Lock1/Lock2 refresh).")
