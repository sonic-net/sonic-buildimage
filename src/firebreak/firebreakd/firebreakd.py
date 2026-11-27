#!/usr/bin/env python3
"""Orchestrate FIREBREAK: Redis ACL users, unix proxies, remount, BPF attach.

Reconciliation:
  - CONFIG_DB collector declarations are filtered through host approvals.
  - Explicit Redis permissions never grow automatically with configuration.
  - Docker events and periodic verification repair installed policy drift.

Remount: prefer image-baked docker_image_ctl.j2 REDIS_MNT. Runtime .sh patch
is legacy-only when the ctl script still has the stock redis mount line.
"""

from __future__ import annotations

import ipaddress
import copy
import functools
import uuid
import json
import os
import signal
import subprocess
import sys
import syslog
import threading
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from policy import validate_profile, acl_rules, approved_collectors, fingerprint

BASE = Path("/opt/firebreak")
ETC = Path("/etc/sonic/firebreak")
RUN = Path("/var/run/firebreak")
CONFIG_DIR = ETC
REDIS_SOCK = "/var/run/redis/redis.sock"

# Poll CONFIG_DB without trusting it as a source of security authorization.
_WATCH_POLL_S = float(os.environ.get("FIREBREAK_WATCH_POLL", "5.0"))
_VERIFY_INTERVAL = float(os.environ.get("FIREBREAK_VERIFY_INTERVAL", "15.0"))
_reconcile_lock = threading.RLock()
_reload_requested = threading.Event()
_managed = {}
_pending = {}  # Initial profiles awaiting successful ACL installation; never watched.
_acl_fingerprints = {}
_egress_receipts = {}
_rejected_collectors = {}
_policy_ids = {}


def serialized(fn):
    @functools.wraps(fn)
    def wrapped(*args, **kwargs):
        with _reconcile_lock:
            return fn(*args, **kwargs)
    return wrapped


def log(msg: str, level=syslog.LOG_INFO) -> None:
    print(msg, flush=True)
    syslog.syslog(level, "FIREBREAK " + msg)


def run(cmd, check=True, **kw):
    log("exec: " + " ".join(cmd))
    return subprocess.run(cmd, check=check, text=True, capture_output=True, **kw)


def redis_cli(sock: str, *args: str) -> str:
    # -e also fails on Redis protocol errors, not only transport failures.
    # Never propagate argv containing ACL passwords into logs/exceptions.
    try:
        p = subprocess.run(["redis-cli", "-e", "-s", sock, "--raw", *args],
                           text=True, capture_output=True, timeout=10)
    except (OSError, subprocess.TimeoutExpired) as exc:
        raise RuntimeError("Redis transport failed") from None
    if p.returncode:
        raise RuntimeError("Redis command rejected: " + (args[0] if args else "unknown"))
    return p.stdout.strip()


def acl_fingerprint(sock, user):
    data = json.loads(redis_cli(sock, '--json', '-3', 'ACL', 'GETUSER', user))
    if not isinstance(data, dict) or 'on' not in data.get('flags', []):
        raise RuntimeError('Redis ACL user is missing or disabled')
    return fingerprint(data)


def ensure_secret(path: Path) -> str:
    path.parent.mkdir(parents=True, exist_ok=True)
    if not path.exists():
        fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(fd, 'w') as output:
            output.write(os.urandom(16).hex())
    os.chmod(path, 0o600)
    return path.read_text(encoding="utf-8").strip()


def apply_acl(cfg: dict, secret: str) -> None:
    validate_profile(cfg)
    user = cfg['redis_user']
    staged = []
    # Let the running Redis version normalize and validate every rule before
    # changing any real profile user. The temporary identity has identical
    # permissions/credential and is always removed before committing.
    for inst in cfg['instances'].values():
        sock = inst['unix_socket']
        rules = ['reset', 'on', '>' + secret] + acl_rules(cfg, inst)
        temporary = '__firebreak_validate_' + uuid.uuid4().hex
        try:
            if redis_cli(sock, 'ACL', 'SETUSER', temporary, *rules) != 'OK':
                raise RuntimeError('Redis refused staged ACL')
            expected = acl_fingerprint(sock, temporary)
            staged.append((sock, rules, expected))
        finally:
            redis_cli(sock, 'ACL', 'DELUSER', temporary)
    observed = {}
    for sock, rules, expected in staged:
        if redis_cli(sock, 'ACL', 'SETUSER', user, *rules) != 'OK':
            raise RuntimeError('Redis refused profile ACL')
        if acl_fingerprint(sock, user) != expected:
            raise RuntimeError('Installed ACL differs from compiled policy')
        observed[sock] = expected
    _acl_fingerprints[cfg['container']] = observed
    log('Applied explicit ACL policy for ' + user)


def prepare_private_dir(cfg: dict) -> Path:
    container = cfg["container"]
    priv = RUN / container
    priv.mkdir(parents=True, exist_ok=True)
    src_db = Path("/var/run/redis/sonic-db")
    dst_db = priv / "sonic-db"
    dst_db.mkdir(parents=True, exist_ok=True)
    if src_db.is_dir():
        for item in src_db.iterdir():
            subprocess.check_call(["cp", "-a", str(item), str(dst_db / item.name)])
    return priv


def start_proxy(cfg: dict, secret_file: Path) -> list[subprocess.Popen]:
    procs = []
    user = cfg["redis_user"]
    container = cfg["container"]
    tcp_port = cfg.get("redis_tcp_proxy_port", 0)
    if tcp_port:
        tcp_port = int(tcp_port)
        if not (1024 <= tcp_port <= 65535):
            raise ValueError("redis_tcp_proxy_port %d not in 1024-65535" % tcp_port)
    priv = prepare_private_dir(cfg)
    proxy = str(BASE / "redis_proxy" / "redis_proxy.py")
    stop_proxies(container)
    # Clean up any legacy bind mounts from a previous native-auth run so
    # the proxy socket paths are available for binding.
    for sock in priv.glob("*.sock"):
        subprocess.run(["umount", str(sock)], check=False, capture_output=True)
    _proxies[container] = procs
    for inst in cfg.get("instances", {}).values():
        listen = str(priv / Path(inst["unix_socket"]).name)
        if os.path.exists(listen):
            try:
                os.unlink(listen)
            except OSError:
                pass
        cmd = [
            sys.executable,
            proxy,
            "--listen",
            listen,
            "--upstream",
            inst["unix_socket"],
            "--user",
            user,
            "--secret-file",
            str(secret_file),
        ]
        if tcp_port:
            cmd.extend(["--tcp-port", str(tcp_port)])
        proc = subprocess.Popen(cmd, start_new_session=True)
        procs.append(proc)
        for _ in range(50):
            if os.path.exists(listen):
                break
            time.sleep(0.1)
        else:
            raise RuntimeError("proxy socket not created: " + listen)
        log("proxy up %s -> %s pid=%s tcp=%s" % (listen, inst["unix_socket"], proc.pid, tcp_port or "none"))
    _proxies[container] = procs
    _children.extend(procs)
    return procs


def stop_proxies(container: str) -> None:
    """Terminate redis_proxy processes for one container."""
    for proc in _proxies.pop(container, []):
        try:
            proc.terminate()
        except OSError:
            pass
        try:
            proc.wait(timeout=3)
        except Exception:
            try:
                proc.kill()
            except OSError:
                pass
        if proc in _children:
            _children.remove(proc)
    priv = RUN / container
    for sock in priv.glob("*.sock"):
        # A Direct AUTH socket is a bind of the shared Redis server socket.
        # Never kill its users or unlink an active mount during proxy cleanup.
        if os.path.ismount(sock):
            continue
        try:
            sock.unlink()
        except OSError:
            pass


def restore_ctl_script(cfg: dict) -> None:
    """Point ctl REDIS_MNT back at stock /var/run/redis (undo firebreak remount)."""
    script = cfg.get("ctl_script")
    if not script or not os.path.isfile(script):
        return
    container = cfg["container"]
    priv = str(RUN / container)
    text = Path(script).read_text(encoding="utf-8", errors="replace")
    # j2-baked or legacy-patched forms
    patterns = [
        'REDIS_MNT="-v %s:/var/run/redis:rw"' % priv,
        'REDIS_MNT="-v /var/run/firebreak/%s:/var/run/redis:rw"' % container,
    ]
    stock = 'REDIS_MNT="-v /var/run/redis$DEV:/var/run/redis:rw"'
    new = text
    for pat in patterns:
        if pat in new:
            new = new.replace(pat, stock, 1)
    if new != text:
        Path(script).write_text(new, encoding="utf-8")
        log("restored stock REDIS_MNT in %s" % script)
    else:
        log("no firebreak REDIS_MNT to restore in %s" % script, syslog.LOG_WARNING)


def ctl_has_j2_remount(cfg: dict) -> bool:
    """True when docker_image_ctl.j2 (or prior patch) already points at firebreak."""
    script = cfg.get("ctl_script")
    if not script or not os.path.isfile(script):
        return False
    priv = str(RUN / cfg["container"])
    text = Path(script).read_text(encoding="utf-8", errors="replace")
    return priv in text or ("/var/run/firebreak/%s" % cfg["container"]) in text


def patch_ctl_script(cfg: dict) -> None:
    """Legacy: rewrite stock REDIS_MNT on images without j2 bake.

    Prefer docker_image_ctl.j2. Set FIREBREAK_LEGACY_CTL_PATCH=0 to never write.
    """
    if os.environ.get("FIREBREAK_LEGACY_CTL_PATCH", "1") == "0":
        log("legacy ctl patch disabled (FIREBREAK_LEGACY_CTL_PATCH=0)")
        return
    if ctl_has_j2_remount(cfg):
        log("ctl %s already j2-baked/firebreak remount; skip patch" % cfg.get("ctl_script"))
        return
    script = cfg.get("ctl_script")
    if not script or not os.path.isfile(script):
        log("no ctl script %s" % script, syslog.LOG_WARNING)
        return
    container = cfg["container"]
    priv = str(RUN / container)
    text = Path(script).read_text(encoding="utf-8", errors="replace")
    needle = 'REDIS_MNT="-v /var/run/redis$DEV:/var/run/redis:rw"'
    repl = 'REDIS_MNT="-v %s:/var/run/redis:rw"' % priv
    if needle in text and repl not in text:
        Path(script).write_text(text.replace(needle, repl, 1), encoding="utf-8")
        log("legacy patched %s REDIS_MNT -> %s (image not j2-baked)" % (script, priv))
    else:
        log("REDIS_MNT pattern not found in %s" % script, syslog.LOG_WARNING)


def unit_skippable(name: str, cfg: dict) -> str | None:
    """Return skip reason if this profile should not be enforced, else None."""
    script = cfg.get("ctl_script") or ("/usr/bin/%s.sh" % name)
    if not os.path.isfile(script):
        return "ctl script missing (%s)" % script
    p = subprocess.run(
        ["systemctl", "is-enabled", name],
        capture_output=True,
        text=True,
        check=False,
    )
    state = (p.stdout or p.stderr or "").strip().lower()
    if "masked" in state:
        return "systemd unit masked"
    # Unit file completely absent
    cat = subprocess.run(
        ["systemctl", "cat", name],
        capture_output=True,
        text=True,
        check=False,
    )
    if cat.returncode != 0 and "No files found" in (cat.stderr or ""):
        return "systemd unit not installed"
    return None


def recreate_container(cfg: dict, force=False) -> None:
    name = cfg["container"]
    priv = str(RUN / name)
    src = subprocess.run(
        [
            "docker",
            "inspect",
            "--format",
            '{{range .Mounts}}{{if eq .Destination "/var/run/redis"}}{{.Source}}{{end}}{{end}}',
            name,
        ],
        capture_output=True,
        text=True,
    )
    if not force and priv in src.stdout and "true" in subprocess.run(
        ["docker", "inspect", "-f", "{{.State.Running}}", name],
        capture_output=True,
        text=True,
    ).stdout:
        log("container %s already remounted; skip recreate" % name)
        return
    run(["systemctl", "stop", name], check=False)
    run(["docker", "rm", "-f", name], check=False)
    run(["systemctl", "reset-failed", name], check=False)
    patch_ctl_script(cfg)
    p = run(["systemctl", "start", name], check=False)
    if p.returncode != 0:
        err = "%s %s" % (p.stdout, p.stderr)
        log("systemctl start failed: %s" % err, syslog.LOG_ERR)
        if "masked" in err.lower():
            raise RuntimeError("unit masked: " + name)
        raise RuntimeError("failed to start " + name)
    for _ in range(60):
        s = subprocess.run(
            ["docker", "inspect", "-f", "{{.State.Running}}", name],
            capture_output=True,
            text=True,
        )
        if s.stdout.strip() == "true":
            break
        time.sleep(1)
    else:
        raise RuntimeError("container not running: " + name)
    log("container %s recreated with firebreak redis mount" % name)


def publish_status(container: str, status: str, **fields: str) -> None:
    if status != 'protected':
        fields.setdefault('lock1', 'unverified')
        fields.setdefault('lock2', 'unverified')
    try:
        cmd = [
            "redis-cli", "-e",
            "-s",
            REDIS_SOCK,
            "-n",
            "6",
            "HSET",
            "FIREBREAK_STATUS|%s" % container,
            "status",
            status,
        ]
        for k, v in fields.items():
            cmd.extend([k, v])
        subprocess.check_call(cmd, timeout=10)
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired) as exc:
        log("STATE_DB status write failed: %s" % exc, syslog.LOG_WARNING)


def _ipv4_host_from_endpoint(value: str) -> str | None:
    """Return canonical IPv4 host from 'A.B.C.D' or 'A.B.C.D:port'; else None.

    Uses ipaddress so leading-zero / noncanonical octets are rejected (e.g.
    010.0.0.1), matching what socket.inet_aton later uses in connect4_loader.
    """
    host = (value or "").split(":")[0].strip()
    if not host:
        return None
    try:
        addr = ipaddress.IPv4Address(host)
    except ipaddress.AddressValueError:
        return None
    return str(addr)


def collector_ips_from_config() -> list[str]:
    """Scrape CONFIG_DB TELEMETRY/OTEL hashes for collector IPv4 addresses.

    Looks at field names containing endpoint/host or ending in _ip. Values may
    be A.B.C.D or A.B.C.D:port (port ignored for connect4). Hostnames skipped.
    """
    ips: list[str] = []
    seen: set[str] = set()
    try:
        keys = redis_cli(REDIS_SOCK, "-n", "4", "KEYS", "*TELEMETRY*")
        keys += "\n" + redis_cli(REDIS_SOCK, "-n", "4", "KEYS", "*OTEL*")
        for line in keys.splitlines():
            line = line.strip()
            if not line:
                continue
            raw = redis_cli(REDIS_SOCK, "-n", "4", "HGETALL", line)
            parts = raw.split("\n")
            for i in range(0, len(parts) - 1, 2):
                k, v = parts[i], parts[i + 1]
                kl = k.lower()
                if "endpoint" in kl or "host" in kl or kl.endswith("_ip"):
                    host = _ipv4_host_from_endpoint(v)
                    if host and host not in seen:
                        seen.add(host)
                        ips.append(host)
    except (subprocess.CalledProcessError, RuntimeError):
        # A partial scrape must never masquerade as a complete observation.
        raise CollectorUnavailable('CONFIG_DB collector observation failed') from None
    return ips


class CollectorUnavailable(RuntimeError):
    """A complete collector observation could not be obtained."""


def resolve_egress_allow(cfg: dict, *, retries: int = 3, delay_sec: float = 1.0) -> list[str]:
    """Build the IPv4 allow-list: static JSON first, then CONFIG_DB scrape.

    When egress_from_config_db is set, scrape *before* BPF attach so a configured
    OTLP collector is not locked out by an empty deny-by-default map. Retries
    cover early-boot races where database/CONFIG_DB is not ready yet.
    """
    static = list(cfg.get("egress_allow_ipv4") or [])
    allow: list[str] = []
    seen: set[str] = set()

    def _add(ip: str) -> None:
        canon = _ipv4_host_from_endpoint(ip)
        if canon and canon not in seen:
            seen.add(canon)
            allow.append(canon)

    for ip in static:
        _add(ip)

    scraped: list[str] = []
    if cfg.get("egress_from_config_db"):
        attempts = max(1, retries)
        for attempt in range(attempts):
            try:
                scraped = collector_ips_from_config()
            except RuntimeError:
                if attempt + 1 == attempts:
                    raise CollectorUnavailable('CONFIG_DB collector observation failed after retries') from None
                log('Collector observation failed; retrying', syslog.LOG_WARNING)
                time.sleep(delay_sec)
                continue
            if scraped or attempt + 1 == attempts:
                break
            log(
                "egress scrape empty for %s (attempt %s/%s); retrying" % (
                    cfg.get("container"),
                    attempt + 1,
                    attempts,
                )
            )
            time.sleep(delay_sec)
        approved, rejected = approved_collectors(cfg, scraped)
        _rejected_collectors[cfg['container']] = rejected
        if rejected:
            log('Rejected unapproved collectors for %s: %s' % (cfg['container'], rejected), syslog.LOG_WARNING)
        for ip in approved:
            _add(ip)
        log(
            "egress allow %s: static=%s scraped=%s -> %s"
            % (cfg.get("container"), static, scraped, allow)
        )
    else:
        _rejected_collectors[cfg['container']] = []
        log("egress allow %s: static=%s (no CONFIG_DB scrape)" % (cfg.get("container"), allow))

    if not allow and cfg.get("egress_deny_default", True):
        log(
            "egress allow %s empty — deny-by-default; legitimate OTLP/traps will EPERM until destinations are declared"
            % cfg.get("container"),
            syslog.LOG_WARNING,
        )
    return allow


def attach_egress(cfg: dict) -> list[str]:
    loader = str(BASE / 'bpf' / 'connect4_loader.py')
    observation_error = None
    try:
        uniq = resolve_egress_allow(cfg)
    except CollectorUnavailable as exc:
        receipt = _egress_receipts.get(cfg['container'])
        if receipt:
            actual = {}
            try:
                current = run([sys.executable, loader, '--container', cfg['container'], '--inspect'],
                              check=False, timeout=10)
                if not current.returncode:
                    actual = json.loads(current.stdout)
            except (OSError, subprocess.TimeoutExpired, ValueError):
                pass
            if all(actual.get(k) == receipt.get(k) for k in ('mode', 'cgroup', 'cgroup_id', 'program_ids')):
                raise  # Keep the last observed attachment during an outage.
        # A new/recreated cgroup still needs a restrictive filter. Only static
        # host approvals are available; do not report this as verified protection.
        uniq = list(dict.fromkeys(cfg.get('egress_allow_ipv4', [])))
        observation_error = exc
    cmd = [sys.executable, loader, '--container', cfg['container']]
    for ip in uniq:
        cmd.extend(['--allow-ip', ip])
    tcp_port = cfg.get("redis_tcp_proxy_port", 0)
    if tcp_port:
        cmd.extend(['--redis-redirect-port', str(tcp_port)])
    p = run(cmd, check=False, timeout=20)
    if p.returncode:
        raise RuntimeError('BPF installation failed: ' + p.stderr[-500:])
    receipt = json.loads(p.stdout)
    if receipt.get('mode') != 'bpf' or len(receipt.get('program_ids', [])) != 1:
        raise RuntimeError('BPF loader did not return a verified attachment')
    receipt['allow'] = uniq
    _egress_receipts[cfg['container']] = receipt
    if observation_error:
        raise observation_error
    return uniq


def report_failure(name, exc):
    # No ACL argv or credentials are included in errors.
    reason = str(exc)[:500]
    publish_status(name, 'degraded' if name in _protected else 'error',
                   reason=reason, checked_at=str(int(time.time())))
    log('Protection verification failed for %s: %s' % (name, reason), syslog.LOG_ERR)


@serialized
def verify_protection(cfg):
    name = cfg['container']
    if _policy_ids.get(name) != fingerprint(cfg):
        raise RuntimeError('Desired profile has not been installed completely')
    result = run(['docker', 'inspect', name], check=False, timeout=10)
    if result.returncode:
        raise RuntimeError('Container is missing')
    item = json.loads(result.stdout)[0]
    if not item['State']['Running']:
        raise RuntimeError('Container is not running')
    mounts = {m['Destination']: m for m in item['Mounts']}
    if mounts.get('/var/run/redis', {}).get('Source') != str(RUN / name):
        raise RuntimeError('Private Redis mount is missing')
    if cfg.get('required_processes'):
        processes = run(['docker', 'exec', name, 'supervisorctl', 'status'], check=False, timeout=10)
        states = {parts[0]: parts[1] for line in processes.stdout.splitlines()
                  if len(parts := line.split()) >= 2}
        missing = [p for p in cfg['required_processes'] if states.get(p) != 'RUNNING']
        if missing:
            failed = [p for p in missing if states.get(p) in ('FATAL', 'EXITED')]
            if failed:
                raise RuntimeError('Required processes failed: ' + ', '.join(failed))
            raise RuntimeError('Required processes not running: ' + ', '.join(missing))
    expected = _acl_fingerprints.get(name, {})
    if not expected:
        raise RuntimeError('No installed ACL receipt')
    for sock, value in expected.items():
        if acl_fingerprint(sock, cfg['redis_user']) != value:
            raise RuntimeError('Redis ACL drift detected')
    children = _proxies.get(name, [])
    if not children or any(p.poll() is not None for p in children):
        raise RuntimeError('Redis proxy is not running')
    for inst in cfg['instances'].values():
        reply = run(['docker', 'exec', name, 'redis-cli', '-e', '-s', inst['unix_socket'],
                     'ACL', 'WHOAMI'], check=False, timeout=10)
        if reply.returncode or reply.stdout.strip() != cfg['redis_user']:
            raise RuntimeError('Proxy connection identity differs')
    receipt = _egress_receipts.get(name)
    if not receipt:
        raise RuntimeError('No installed BPF receipt')
    result = run([sys.executable, str(BASE / 'bpf' / 'connect4_loader.py'),
                  '--container', name, '--inspect'], check=False, timeout=10)
    if result.returncode:
        raise RuntimeError('Cannot inspect attached BPF program')
    actual = json.loads(result.stdout)
    if any(actual.get(k) != receipt.get(k) for k in ('mode', 'cgroup', 'cgroup_id', 'program_ids')):
        raise RuntimeError('BPF attachment or container cgroup changed')
    if receipt['allow'] != resolve_egress_allow(cfg, retries=1):
        raise RuntimeError('Installed destinations differ from approved configuration')
    now = str(int(time.time()))
    publish_status(name, 'protected', reason='', checked_at=now, verified_at=now,
                   policy_id=_policy_ids[name], container_id=item['Id'],
                   lock1='verified', lock2='verified', backend=actual['mode'],
                   verification_ttl=str(int(max(60, _VERIFY_INTERVAL * 3))),
                   egress_allow=','.join(receipt['allow']),
                   rejected_collectors=','.join(_rejected_collectors.get(name, [])))
    return True


@serialized
def reconcile_health():
    # Bounded polling catches missed Docker events, ACL changes and removed
    # attachments. Repair is serialized with enable/disable/reload.
    for cfg in list({**_pending, **_managed}.values()):
        name = cfg['container']
        skipped = unit_skippable(name, cfg)
        if skipped:
            publish_status(name, 'skipped', reason=skipped)
            continue
        try:
            verify_protection(cfg)
        except Exception as exc:
            report_failure(name, exc)
            try:
                if 'Native connector' in str(exc) or 'Required processes failed' in str(exc):
                    protect(cfg, force_recreate=True)
                else:
                    protect(cfg)
            except Exception as repair_exc:
                report_failure(name, repair_exc)


def load_configs() -> list[dict]:
    # Validate the entire desired set before a reload changes enforcement.
    cfgs = []
    for path in sorted(CONFIG_DIR.glob('*.json')):
        info = path.stat()
        if info.st_uid != 0 or info.st_mode & 0o022:
            raise ValueError('Policy must be root-owned and not group/world writable: ' + str(path))
        cfgs.append(validate_profile(json.loads(path.read_text())))
    names = [cfg['container'] for cfg in cfgs]
    users = [cfg['redis_user'] for cfg in cfgs]
    if len(set(names)) != len(names) or len(set(users)) != len(users):
        raise ValueError('Profile container and Redis user names must be unique')
    return cfgs


_children: list[subprocess.Popen] = []
_proxies: dict[str, list[subprocess.Popen]] = {}  # container -> proxy procs
_secrets: dict[str, str] = {}  # container -> secret
_protected: set[str] = set()  # containers that completed protect()
_watch_names: list[str] = []  # mutable; docker watcher reads this
_watch_cfgs: list[dict] = []  # mutable; CONFIG_DB watcher reads this
_stop = threading.Event()


def _sync_watchers() -> None:
    """Keep watcher target lists aligned with currently protected profiles."""
    cfgs = list(_managed.values())
    _watch_cfgs[:] = cfgs
    _watch_names[:] = [c["container"] for c in cfgs]


def validate_live_identity(cfg: dict) -> None:
    """Require a completed disable before changing an installed client identity.

    ACL/egress rules may reload in place. Identity and connection changes need
    stock recreation and old-user revocation, which the disable path provides.
    """
    previous = _managed.get(cfg['container'])
    if previous is None:
        return

    def identity(profile):
        return {
            'redis_user': profile['redis_user'],
            'secret_file': profile['secret_file'],
            'use_proxy': True,
            'ctl_script': profile.get('ctl_script'),
            'instances': {name: {key: inst.get(key) for key in ('unix_socket', 'tcp_port', 'select')}
                          for name, inst in profile['instances'].items()},
        }

    if identity(previous) != identity(cfg):
        raise ValueError('Disable %s successfully before changing its Redis identity, backend or instances' % cfg['container'])


@serialized
def protect(cfg: dict, force_recreate=False) -> bool:
    """Protect one container. Returns False if skipped (masked/missing)."""
    validate_profile(cfg)
    name = cfg["container"]
    validate_live_identity(cfg)
    if name not in _managed:
        _pending[name] = copy.deepcopy(cfg)
    skip = unit_skippable(name, cfg)
    if skip:
        log("skip %s: %s" % (name, skip), syslog.LOG_WARNING)
        publish_status(name, "skipped", reason=skip)
        return False
    secret_file = Path(cfg["secret_file"])
    secret = ensure_secret(secret_file)
    if not _ensure_keyspace_notifications():
        raise RuntimeError('Cannot configure host keyspace notifications')
    apply_acl(cfg, secret)
    # Redis staging must succeed before a candidate becomes visible to watchers
    # or replaces the last accepted profile and its credential.
    _managed[name] = copy.deepcopy(cfg)
    _pending.pop(name, None)
    _secrets[name] = secret
    publish_status(name, 'enabling', reason='', verified_at='', policy_id=fingerprint(cfg))
    # Always use the host proxy: it forces AUTH on every connection and keeps
    # 127.0.0.1 out of the BPF allow list so TCP 6379 is blocked.
    start_proxy(cfg, secret_file)
    try:
        recreate_container(cfg, force=force_recreate)
    except RuntimeError as exc:
        msg = str(exc)
        if "masked" in msg.lower() or "failed to start" in msg.lower():
            log("skip %s after start failure: %s" % (name, msg), syslog.LOG_WARNING)
            stop_proxies(name)
            publish_status(name, "skipped", reason=msg)
            return False
        raise
    attach_egress(cfg)
    _policy_ids[name] = fingerprint(cfg)
    _protected.add(name)
    _sync_watchers()
    verify_protection(cfg)
    log("protected %s" % name)
    return True


def del_acl_user(cfg: dict, *, strict=False) -> None:
    """Remove the named Redis ACL user (best-effort) so stock default is unambiguous."""
    user = cfg.get("redis_user") or cfg.get("container")
    if not user:
        return
    instances = cfg.get("instances") or {}
    socks: list[str]
    if instances:
        socks = [inst.get("unix_socket") or REDIS_SOCK for inst in instances.values()]
    else:
        # Minimal/fallback profiles (e.g. unprotect-all without JSON) have no instances.
        socks = [REDIS_SOCK]
    for sock in socks:
        try:
            out = redis_cli(sock, "ACL", "DELUSER", user)
            log("ACL DELUSER %s on %s: %s" % (user, sock, out))
        except (RuntimeError, subprocess.CalledProcessError) as exc:
            if strict:
                raise RuntimeError('Cannot revoke Redis identity during disable') from None
            log("ACL DELUSER %s failed: %s" % (user, exc), syslog.LOG_WARNING)


@serialized
def unprotect(cfg: dict, reason: str = "disabled") -> None:
    """Stop proxy, restore stock Redis mount, recreate container, clear status."""
    name = cfg["container"]
    log("unprotect %s (%s)" % (name, reason))
    stop_proxies(name)
    # Retain the accepted identity if revocation fails, so a later enable cannot
    # silently switch identities while the old ACL and clients are still live.
    try:
        del_acl_user(cfg, strict=True)
    except RuntimeError as exc:
        report_failure(name, exc)
        raise
    _secrets.pop(name, None)
    _protected.discard(name)
    _managed.pop(name, None)
    _pending.pop(name, None)
    _acl_fingerprints.pop(name, None)
    _egress_receipts.pop(name, None)
    _policy_ids.pop(name, None)
    restore_ctl_script(cfg)
    try:
        # Force recreate onto stock mount even if currently on firebreak path.
        run(["systemctl", "stop", name], check=False)
        run(["docker", "rm", "-f", name], check=False)
        run(["systemctl", "reset-failed", name], check=False)
        p = run(["systemctl", "start", name], check=False)
        if p.returncode != 0:
            log("unprotect start failed %s: %s %s" % (name, p.stdout, p.stderr), syslog.LOG_ERR)
            publish_status(name, "disabled", reason="restore start failed")
        else:
            publish_status(name, "disabled", reason=reason)
            log("unprotected %s — stock redis mount restored" % name)
    except Exception as exc:
        log("unprotect failed %s: %s" % (name, exc), syslog.LOG_ERR)
        publish_status(name, "error", reason=str(exc))
    _sync_watchers()


def load_disabled_names() -> set[str]:
    """Containers whose profile was renamed to *.json.disabled."""
    names: set[str] = set()
    if not CONFIG_DIR.is_dir():
        return names
    for path in CONFIG_DIR.glob("*.json.disabled"):
        try:
            cfg = json.loads(path.read_text(encoding="utf-8"))
            if cfg.get("container"):
                names.add(cfg["container"])
        except json.JSONDecodeError:
            names.add(path.name.replace(".json.disabled", ""))
    return names


@serialized
def reload_all(reason: str = "SIGHUP") -> None:
    """Re-read profiles: unprotect disabled, protect newly eligible, refresh locks."""
    log("reload (%s)" % reason)
    active_cfgs = load_configs()
    for cfg in active_cfgs:
        validate_live_identity(cfg)
    active_names = {c["container"] for c in active_cfgs}
    disabled = load_disabled_names()
    for name in list(_pending):
        if name not in active_names or name in disabled:
            _pending.pop(name, None)

    # Drop protection for profiles moved to *.disabled or removed.
    for name in list(_managed):
        if name not in active_names or name in disabled:
            cfg = _managed[name]
            # Disk contents may already contain a rejected identity change.
            # Revoke the identity actually installed by this daemon.
            unprotect(cfg, reason="profile disabled or removed")

    # Protect newly eligible profiles (e.g. unmasked unit + profile present).
    for cfg in active_cfgs:
        name = cfg["container"]
        if name in _protected and _managed.get(name) == cfg:
            continue
        if unit_skippable(name, cfg) is None:
            try:
                protect(cfg)
            except Exception as exc:
                report_failure(name, exc)

    cfgs = [c for c in _managed.values() if c["container"] in _protected]
    _sync_watchers()
    if not cfgs:
        log("reload: no protected containers; nothing to refresh")
        return
    refresh_lock1(cfgs, reason)
    refresh_lock2(cfgs, reason)
    for cfg in cfgs:
        try:
            verify_protection(cfg)
        except Exception as exc:
            report_failure(cfg['container'], exc)


@serialized
def refresh_lock2(cfgs: list[dict], reason: str) -> None:
    targets = [c for c in cfgs if c.get("egress_from_config_db") and _managed.get(c['container']) == c
               and _policy_ids.get(c['container']) == fingerprint(c)]
    if not targets:
        return
    log("Lock2 refresh (%s)" % reason)
    for cfg in targets:
        try:
            attach_egress(cfg)
            verify_protection(cfg)
        except Exception as exc:
            report_failure(cfg['container'], exc)


@serialized
def refresh_lock1(cfgs: list[dict], reason: str) -> None:
    log("Lock1 ACL refresh (%s)" % reason)
    for cfg in cfgs:
        if _managed.get(cfg['container']) != cfg:
            continue
        secret = _secrets.get(cfg["container"])
        if not secret:
            secret_file = Path(cfg["secret_file"])
            if secret_file.is_file():
                secret = secret_file.read_text(encoding="utf-8").strip()
        if not secret:
            continue
        try:
            apply_acl(cfg, secret)
            verify_protection(cfg)
        except Exception as exc:
            report_failure(cfg['container'], exc)


def _ensure_keyspace_notifications() -> bool:
    """Enable keyspace events on CONFIG_DB redis if possible. Returns True if usable."""
    try:
        cur = json.loads(redis_cli(REDIS_SOCK, '--json', '-3', 'CONFIG', 'GET', 'notify-keyspace-events'))
        val = cur['notify-keyspace-events']
        need = set("KEA")  # keyspace, keyevent-ish: keys + hash + generic
        have = set(val or "")
        if need <= have:
            return True
        new = "".join(sorted(have | need))
        redis_cli(REDIS_SOCK, "CONFIG", "SET", "notify-keyspace-events", new)
        log("enabled notify-keyspace-events=%s" % new)
        return True
    except (subprocess.CalledProcessError, RuntimeError, ValueError, KeyError, TypeError) as exc:
        log("keyspace notifications unavailable: %s" % exc, syslog.LOG_WARNING)
        return False


def watch_docker_restarts() -> None:
    proc = subprocess.Popen(
        ["docker", "events", "--filter", "event=start", "--format", "{{.Actor.Attributes.name}}"],
        stdout=subprocess.PIPE,
        text=True,
    )
    assert proc.stdout
    for line in proc.stdout:
        if _stop.is_set():
            break
        name = line.strip()
        if name in _watch_names and name in _protected:
            log("docker start %s — reattach egress" % name)
            for cfg in list(_watch_cfgs):
                if cfg["container"] == name:
                    try:
                        with _reconcile_lock:
                            if _managed.get(name) != cfg:
                                continue
                            attach_egress(cfg)
                            verify_protection(cfg)
                    except Exception as exc:
                        report_failure(name, exc)


def watch_config_db() -> None:
    """Bounded, restartable observation; Redis outages do not kill the watcher.

    ACLs are explicit and no longer depend on discovered CONFIG_DB prefixes.
    Desired profile changes still require an operator reload.
    """
    last_ips = None
    while not _stop.wait(_WATCH_POLL_S):
        try:
            ips = tuple(sorted(collector_ips_from_config()))
            if ips != last_ips:
                refresh_lock2(list(_watch_cfgs), 'collector configuration changed')
                last_ips = ips
        except Exception as exc:
            log('Collector observation failed; retaining installed policy: ' + str(exc), syslog.LOG_WARNING)


def load_all_profiles_for_unprotect() -> list[dict]:
    """Active + disabled profiles (for restore-stock / --unprotect-all)."""
    cfgs = list(load_configs())
    if not CONFIG_DIR.is_dir():
        return cfgs
    have = {c["container"] for c in cfgs}
    for path in sorted(CONFIG_DIR.glob("*.json.disabled")):
        try:
            cfg = json.loads(path.read_text(encoding="utf-8"))
        except json.JSONDecodeError:
            continue
        name = cfg.get("container") or path.name.replace(".json.disabled", "")
        if name not in have:
            cfg.setdefault("container", name)
            cfgs.append(cfg)
            have.add(name)
    return cfgs


def unprotect_all(reason: str = "restore-stock") -> int:
    """One-shot: restore every known profile to stock."""
    cfgs = load_all_profiles_for_unprotect()
    if not cfgs:
        # Still try common Tier-A names if profiles were wiped.
        for name in ("dhcp_relay", "snmp", "otel"):
            unprotect(
                {"container": name, "ctl_script": "/usr/bin/%s.sh" % name, "redis_user": name},
                reason=reason,
            )
        return 0
    for cfg in cfgs:
        try:
            unprotect(cfg, reason=reason)
        except Exception as exc:
            log("unprotect_all %s: %s" % (cfg.get("container"), exc), syslog.LOG_ERR)
    return 0


def main() -> int:
    syslog.openlog("firebreakd")
    ETC.mkdir(parents=True, exist_ok=True)
    RUN.mkdir(parents=True, exist_ok=True)
    argv = sys.argv[1:]
    if argv and argv[0] in ("--unprotect-all", "unprotect-all"):
        return unprotect_all("cli unprotect-all")

    only = argv
    cfgs = load_configs()
    if only:
        cfgs = [c for c in cfgs if c["container"] in only]
    if not cfgs:
        log("no configs in %s" % CONFIG_DIR, syslog.LOG_ERR)
        return 1

    def _cfg_for(name: str) -> dict:
        for c in load_configs():
            if c.get("container") == name:
                return c
        dis = CONFIG_DIR / ("%s.json.disabled" % name)
        if dis.is_file():
            try:
                return json.loads(dis.read_text(encoding="utf-8"))
            except json.JSONDecodeError:
                pass
        return {"container": name, "ctl_script": "/usr/bin/%s.sh" % name}

    def _stop_handler(signum, frame):
        _stop.set()
        # Restore stock mounts so containers are not left on dead proxy socks.
        for name, cfg in list({**_pending, **_managed}.items()):
            try:
                unprotect(cfg, reason="daemon stop")
            except Exception as exc:
                log("stop unprotect %s: %s" % (name, exc), syslog.LOG_ERR)
        for p in list(_children):
            try:
                p.terminate()
            except OSError:
                pass
        sys.exit(0)

    def _hup_handler(signum, frame):
        _reload_requested.set()

    signal.signal(signal.SIGTERM, _stop_handler)
    signal.signal(signal.SIGINT, _stop_handler)
    signal.signal(signal.SIGHUP, _hup_handler)

    active: list[dict] = []
    for cfg in cfgs:
        try:
            if protect(cfg):
                active.append(cfg)
        except Exception as exc:
            report_failure(cfg['container'], exc)
    if not active:
        log("no containers protected (all skipped); watching for CONFIG_DB/FEATURE changes", syslog.LOG_WARNING)

    if os.environ.get("FIREBREAK_NO_WATCH"):
        log("FIREBREAK_NO_WATCH set; exiting after protect")
        return 0 if len(active) == len(cfgs) else 1

    _sync_watchers()
    t_docker = threading.Thread(target=watch_docker_restarts, name="fb-docker", daemon=True)
    t_cfg = threading.Thread(target=watch_config_db, name="fb-configdb", daemon=True)
    t_docker.start()
    t_cfg.start()
    log(
        "watchers running: docker-restart + CONFIG_DB (Lock2/Lock1); protected=%s"
        % sorted(_protected)
    )
    next_verify = time.monotonic() + _VERIFY_INTERVAL
    while not _stop.wait(1):
        if _reload_requested.is_set():
            _reload_requested.clear()
            try:
                reload_all('SIGHUP')
            except Exception as exc:
                log('Reload rejected; existing policies retained: ' + str(exc), syslog.LOG_ERR)
        if time.monotonic() >= next_verify:
            reconcile_health()
            next_verify = time.monotonic() + _VERIFY_INTERVAL
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
