#!/usr/bin/env python3
"""Install automatic one-job runner registration on an already prepared host."""

import argparse
import json
import os
from pathlib import Path
import pwd
import re
import subprocess
import tempfile

TOOL_ROOT = Path("/opt/sonic-runner-tools/runner")
UNIT_ROOT = Path("/etc/systemd/system")
CONFIG = Path("/etc/sonic-vs-runner-manager.json")
SERVICE = "sonic-vs-runner-manager.service"
TIMER = "sonic-vs-runner-manager.timer"
HEADER = "# AUTO-GENERATED. DO NOT EDIT MANUALLY.\n# Generator: tools/ci/runner/install_manager.py\n"
HELPERS = ("manager.py", "rearm.py", "preflight.py", "apparmor_gs.py")


def service_text():
    return HEADER + """[Unit]
Description=Register the next one-job SONiC VS runner
Wants=network-online.target
After=network-online.target docker.service sonic-vs-workspace-permissions.service
Requires=docker.service sonic-vs-workspace-permissions.service
RequiresMountsFor=/data/sonic-runner
ConditionPathExists=/etc/sonic-vs-runner-manager.json

[Service]
Type=oneshot
User=root
WorkingDirectory=/opt/sonic-runner-tools/runner
ExecStart=/usr/bin/python3 -B /opt/sonic-runner-tools/runner/manager.py --config /etc/sonic-vs-runner-manager.json
TimeoutStartSec=15min
UMask=0022
StateDirectory=sonic-vs-runner-manager
StateDirectoryMode=0700
StandardOutput=journal
StandardError=journal
"""


def timer_text():
    return HEADER + """[Unit]
Description=Keep one SONiC VS runner available

[Timer]
OnBootSec=30s
OnUnitInactiveSec=60s
AccuracySec=5s
Unit=sonic-vs-runner-manager.service

[Install]
WantedBy=timers.target
"""


def configuration(args):
    if not re.fullmatch(r"[a-z_][a-z0-9_-]*", args.operator):
        raise ValueError("operator must be a local account name")
    if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9-]*", args.github_account):
        raise ValueError("github-account must be a GitHub login")
    if not re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+", args.repo) or ".." in args.repo:
        raise ValueError("repo must be owner/name")
    if args.pr is not None and args.pr < 1:
        raise ValueError("pr must be positive")
    return {
        "_generated": {
            "notice": "AUTO-GENERATED. DO NOT EDIT MANUALLY.",
            "generator": "tools/ci/runner/install_manager.py",
        },
        "operator": args.operator,
        "github_account": args.github_account,
        "repo": args.repo,
        "pr": args.pr,
    }


def write_owned(path, content, mode):
    """Publish a complete root-owned file without following an existing symlink."""
    descriptor, temporary = tempfile.mkstemp(prefix="." + path.name + ".", dir=path.parent)
    try:
        with os.fdopen(descriptor, "wb") as stream:
            stream.write(content)
            os.fchmod(stream.fileno(), mode)
            os.fchown(stream.fileno(), 0, 0)
        os.replace(temporary, path)
    finally:
        Path(temporary).unlink(missing_ok=True)


def install(config, *, enable=True):
    if os.geteuid() != 0:
        raise RuntimeError("Install with sudo; the manager needs systemd and runner-account access")
    account = pwd.getpwnam(config["operator"])
    if account.pw_uid == 0 or account.pw_name == "sonic-runner":
        raise RuntimeError("Use the normal GitHub-authenticated operator, not root or sonic-runner")
    state = subprocess.check_output(
        ["systemctl", "show", "sonic-vs-runner.service", "--property=LoadState", "--value"],
        text=True,
    ).strip()
    if state != "loaded":
        raise RuntimeError("Prepare this host with bootstrap.sh first")

    # Validate the existing operator login before enabling unattended registration.
    # No credential is placed in the generated configuration or runner account.
    from manager import OperatorGitHub
    OperatorGitHub(config).verify_identity()

    source = Path(__file__).resolve().parent
    contents = {name: (source / name).read_bytes() for name in HELPERS}
    TOOL_ROOT.mkdir(mode=0o755, parents=True, exist_ok=True)
    if TOOL_ROOT.is_symlink() or TOOL_ROOT.stat().st_uid != 0 or TOOL_ROOT.stat().st_mode & 0o022:
        raise RuntimeError("The installed helper directory must be root-owned and not group/world writable")
    # Stop only installed manager units during an update. The job service stays
    # independent, and the first installation has no manager units to stop.
    for unit in (TIMER, SERVICE):
        existing = subprocess.run(
            ["systemctl", "show", unit, "--property=LoadState", "--value"],
            text=True, capture_output=True,
        )
        if existing.stdout.strip() == "not-found":
            continue
        if existing.returncode or existing.stdout.strip() != "loaded":
            raise RuntimeError("Cannot inspect installed manager unit: " + unit)
        subprocess.run(["systemctl", "stop", unit], check=True)
    for name, data in contents.items():
        write_owned(TOOL_ROOT / name, data, 0o755)
    write_owned(CONFIG, (json.dumps(config, indent=2) + "\n").encode(), 0o600)
    write_owned(UNIT_ROOT / SERVICE, service_text().encode(), 0o644)
    write_owned(UNIT_ROOT / TIMER, timer_text().encode(), 0o644)
    subprocess.run(["systemctl", "daemon-reload"], check=True)
    if enable:
        subprocess.run(["systemctl", "enable", "--now", TIMER], check=True)
    else:
        subprocess.run(["systemctl", "disable", TIMER], check=True)
    print("Installed " + TIMER + (" and enabled automatic registration" if enable else " (not enabled)"))
    print("Routing: " + config["repo"] + " / " + ("PR " + str(config["pr"]) if config["pr"] else "master"))
    print("Inspect: sudo journalctl -u " + SERVICE + " -n 50 --no-pager")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--operator", required=True, help="Local user with an existing noninteractive gh login")
    parser.add_argument("--github-account", default="securely1g")
    parser.add_argument("--repo", default="securely1g/sonic-buildimage")
    target = parser.add_mutually_exclusive_group(required=True)
    target.add_argument("--master", action="store_true")
    target.add_argument("--pr", type=int)
    parser.add_argument("--dry-run", action="store_true", help="Print configuration and units without access checks or changes")
    parser.add_argument("--no-enable", action="store_true", help="Install files but leave the timer stopped")
    args = parser.parse_args()
    config = configuration(args)
    if args.dry_run:
        print(json.dumps(config, indent=2))
        print(service_text())
        print(timer_text())
        return
    install(config, enable=not args.no_enable)


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, RuntimeError, subprocess.CalledProcessError) as error:
        raise SystemExit("Manager installation failed: " + str(error))
