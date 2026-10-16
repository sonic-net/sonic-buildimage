"""show firebreak — SONiC CLI for FIREBREAK host containment status.

Surfaces systemd unit state, per-container Lock1/Lock2 status from STATE_DB,
profile files, Redis ACL identity, egress allow-list, scraped collectors, and
proxy sockets. Enforcement remains on the host (firebreak.service); this
command is read-only display.
"""

from __future__ import annotations

import json
import subprocess
import time
from pathlib import Path

import click
from natsort import natsorted
from tabulate import tabulate
import utilities_common.cli as clicommon

try:
    from swsscommon.swsscommon import SonicV2Connector
except ImportError:  # pragma: no cover
    SonicV2Connector = None

CONFIG_DIR = Path("/etc/sonic/firebreak")
RUN_DIR = Path("/var/run/firebreak")
REDIS_SOCK = "/var/run/redis/redis.sock"


def _run(cmd, timeout=5):
    try:
        p = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=timeout,
            check=False,
        )
        return p.returncode, (p.stdout or "").strip(), (p.stderr or "").strip()
    except (OSError, subprocess.TimeoutExpired) as exc:
        return 1, "", str(exc)


def _systemctl_active(unit: str) -> str:
    _, out, _ = _run(["systemctl", "is-active", unit])
    return out or "unknown"


def _systemctl_enabled(unit: str) -> str:
    _, out, _ = _run(["systemctl", "is-enabled", unit])
    return out or "unknown"


def _docker_status(name: str) -> str:
    rc, out, _ = _run(
        ["docker", "inspect", "-f", "{{.State.Status}}", name],
        timeout=8,
    )
    if rc != 0:
        return "absent"
    return out or "unknown"


def _docker_redis_mount(name: str) -> str:
    rc, out, _ = _run(
        [
            "docker",
            "inspect",
            "--format",
            '{{range .Mounts}}{{if eq .Destination "/var/run/redis"}}{{.Source}}{{end}}{{end}}',
            name,
        ],
        timeout=8,
    )
    if rc != 0:
        return "-"
    return out or "-"


def _proxy_sock(name: str) -> str:
    sock = RUN_DIR / name / "redis.sock"
    return "up" if sock.exists() else "missing"


def _load_profiles() -> list:
    profiles = []
    if not CONFIG_DIR.is_dir():
        return profiles
    for path in sorted(CONFIG_DIR.glob("*.json")):
        if str(path).endswith(".disabled"):
            continue
        try:
            cfg = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError):
            continue
        cfg["_file"] = path.name
        profiles.append(cfg)
    return profiles


def _state_db():
    if SonicV2Connector is None:
        return None
    try:
        db = SonicV2Connector(use_unix_socket_path=True)
        db.connect(db.STATE_DB, False)
        return db
    except Exception:
        return None


def _status_fields(db, container: str) -> dict:
    if db is None:
        return {}
    key = "FIREBREAK_STATUS|%s" % container
    try:
        return db.get_all(db.STATE_DB, key) or {}
    except Exception:
        return {}


def _redis_cli(*args: str) -> str:
    cmd = ["redis-cli", "-s", REDIS_SOCK, "--raw", *args]
    rc, out, err = _run(cmd, timeout=8)
    if rc != 0:
        return err or out
    return out


def _collector_ips() -> list:
    """Mirror firebreakd collector_ips_from_config (IPv4 host part)."""
    ips = []
    seen = set()
    keys = _redis_cli("-n", "4", "KEYS", "*TELEMETRY*")
    keys += "\n" + _redis_cli("-n", "4", "KEYS", "*OTEL*")
    for line in keys.splitlines():
        line = line.strip()
        if not line or line.startswith("ERR"):
            continue
        raw = _redis_cli("-n", "4", "HGETALL", line)
        parts = raw.split("\n")
        for i in range(0, len(parts) - 1, 2):
            k, v = parts[i], parts[i + 1]
            if "endpoint" in k.lower() or "host" in k.lower() or k.lower().endswith("_ip"):
                host = v.split(":")[0].strip()
                if host and host[0].isdigit() and host not in seen:
                    seen.add(host)
                    ips.append((line, k, v, host))
    return ips


@click.group(cls=clicommon.AliasedGroup, name="firebreak", invoke_without_command=True)
@click.pass_context
def firebreak(ctx):
    """Show FIREBREAK host containment status (Lock1 Redis ACL + Lock2 egress)"""
    if ctx.invoked_subcommand is None:
        ctx.invoke(status)


@firebreak.command("status")
def status():
    """Show daemon + per-container protection status"""
    click.echo("FIREBREAK daemon")
    click.echo("  systemd unit:   firebreak.service")
    daemon_state = _systemctl_active("firebreak")
    click.echo("  active:         %s" % daemon_state)
    click.echo("  enabled:        %s" % _systemctl_enabled("firebreak"))
    click.echo("  profiles dir:   %s" % CONFIG_DIR)
    click.echo("  private run:    %s" % RUN_DIR)
    click.echo("")

    profiles = _load_profiles()
    db = _state_db()
    header = [
        "Container",
        "Profile",
        "Status",
        "Docker",
        "Proxy",
        "Egress allow",
        "CFG_DB scrape",
        "Verified age",
    ]
    body = []
    for cfg in profiles:
        name = cfg.get("container", "?")
        st = _status_fields(db, name)
        state = st.get('status', '-')
        age = '-'
        try:
            elapsed = max(0, int(time.time()) - int(st['verified_at']))
            age = str(elapsed) + 's'
            if state == 'protected' and elapsed > int(st.get('verification_ttl', '60')):
                state = 'stale'
        except (KeyError, ValueError, TypeError):
            if state == 'protected':
                state = 'unverified'
        if daemon_state != 'active' and state == 'protected':
            state = 'stale'
        body.append(
            [
                name,
                cfg.get("_file", ""),
                state,
                _docker_status(name),
                _proxy_sock(name),
                st.get("egress_allow", "") or "(deny-all)",
                "yes" if cfg.get("egress_from_config_db") else "no",
                age,
            ]
        )
    if not body:
        click.echo("No profiles in %s" % CONFIG_DIR)
        return
    click.echo(tabulate(body, header, tablefmt="simple"))
    for cfg in profiles:
        st = _status_fields(db, cfg['container'])
        if st.get('reason'):
            click.echo('  %s: %s' % (cfg['container'], st['reason']))
        if st.get('rejected_collectors'):
            click.echo('  %s: unapproved collectors ignored: %s' % (cfg['container'], st['rejected_collectors']))
    click.echo("")
    click.echo("Verified = observed ACL, configured client identity, mount and BPF attachment; not complete containment.")


@firebreak.command("profiles")
def profiles_cmd():
    """List installed FIREBREAK JSON profiles"""
    profiles = _load_profiles()
    if not profiles:
        click.echo("No profiles in %s" % CONFIG_DIR)
        return
    header = ["File", "Container", "Redis user", "Egress CFG_DB", "Static allow"]
    body = []
    for cfg in profiles:
        body.append(
            [
                cfg.get("_file"),
                cfg.get("container"),
                cfg.get("redis_user"),
                "yes" if cfg.get("egress_from_config_db") else "no",
                ",".join(cfg.get("egress_allow_ipv4") or []) or "-",
            ]
        )
    click.echo(tabulate(body, header, tablefmt="simple"))


@firebreak.command("acl")
@click.argument("container", required=False)
def acl(container):
    """Show Redis ACL user rules for a protected container (or all)"""
    profiles = _load_profiles()
    if container:
        profiles = [c for c in profiles if c.get("container") == container]
        if not profiles:
            click.echo("No FIREBREAK profile for container %s" % container)
            raise SystemExit(1)
    for cfg in profiles:
        user = cfg.get("redis_user")
        name = cfg.get("container")
        if not user:
            click.echo("=== %s (no redis_user in profile) ===\n" % name)
            continue
        click.echo("=== %s (redis_user=%s) ===" % (name, user))
        dry = _redis_cli(
            "ACL",
            "DRYRUN",
            user,
            "HSET",
            "TACPLUS_SERVER|evil",
            "auth_type",
            "pap",
        )
        click.echo("DRYRUN HSET TACPLUS_SERVER|evil: %s" % (dry or "(empty)"))
        listing = _redis_cli("ACL", "LIST")
        matched = [ln for ln in listing.splitlines() if ln.startswith("user %s " % user)]
        if matched:
            click.echo(matched[0])
        else:
            click.echo("(ACL user %s not found — is firebreak.service active?)" % user)
        click.echo("")


@firebreak.command("egress")
@click.argument("container", required=False)
def egress(container):
    """Show Lock2 egress allow-list from STATE_DB (CONFIG_DB watcher updates this)"""
    profiles = _load_profiles()
    if container:
        profiles = [c for c in profiles if c.get("container") == container]
        if not profiles:
            click.echo("No FIREBREAK profile for container %s" % container)
            raise SystemExit(1)
    db = _state_db()
    header = ["Container", "Status", "Egress allow (IPv4)", "CFG_DB scrape"]
    body = []
    for cfg in natsorted(profiles, key=lambda c: c.get("container", "")):
        name = cfg.get("container")
        st = _status_fields(db, name)
        body.append(
            [
                name,
                st.get("status", "-"),
                st.get("egress_allow", "") or "(deny-all / empty)",
                "yes" if cfg.get("egress_from_config_db") else "no",
            ]
        )
    click.echo(tabulate(body, header, tablefmt="simple"))


@firebreak.command("collectors")
def collectors():
    """Show collector/endpoint IPs scraped from CONFIG_DB TELEMETRY/OTEL keys"""
    rows = _collector_ips()
    if not rows:
        click.echo("No TELEMETRY/OTEL collector endpoints found in CONFIG_DB")
        click.echo("(watcher feeds these into BPF for profiles with egress_from_config_db)")
        return
    header = ["CONFIG_DB key", "Field", "Value", "Allow IP"]
    body = [[k, f, v, ip] for (k, f, v, ip) in rows]
    click.echo(tabulate(body, header, tablefmt="simple"))
    click.echo("")
    click.echo("IPs mirrored to Lock2 for: " + ", ".join(
        c.get("container") for c in _load_profiles() if c.get("egress_from_config_db")
    ))


@firebreak.command("sockets")
def sockets():
    """Show private Redis directory, auth mode, and container remount source"""
    profiles = _load_profiles()
    if not profiles:
        click.echo("No profiles in %s" % CONFIG_DIR)
        return
    header = ["Container", "Proxy path", "Proxy", "Docker /var/run/redis mount"]
    body = []
    for cfg in profiles:
        name = cfg.get("container")
        path = str(RUN_DIR / name / "redis.sock")
        body.append(
            [
                name,
                path,
                _proxy_sock(name),
                _docker_redis_mount(name),
            ]
        )
    click.echo(tabulate(body, header, tablefmt="simple"))
    click.echo("")
    click.echo("Expect mount source under /var/run/firebreak/<container> when protected.")
