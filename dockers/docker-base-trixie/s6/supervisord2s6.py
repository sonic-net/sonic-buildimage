#!/usr/bin/env python3
"""supervisord -> s6-overlay converter and drop-in replacement.

Two modes:
  * shim mode (default, installed as /usr/local/bin/supervisord): when running as
    PID 1, parse the
    already-rendered supervisord config chain + critical_processes, generate the
    s6-rc source tree under /etc/s6-overlay, then exec /init. This makes s6 the
    only supervisor. When not PID 1 (e.g. --pid=host) exec supervisord.real.
  * offline mode (--emit-only): generate the tree from explicit --config /
    --critical into --out / --bundle-out, without exec'ing. Used to verify the
    converter against each container's captured config before building.

The converter reproduces what supervisord + supervisord-dependent-startup would
actually do:
  * only programs that would actually start are converted: autostart=true OR
    dependent_startup=true (so e.g. FRR 'sharpd', which has neither, is skipped);
  * commands get supervisord's %-expansion first (%% -> %, %(ENV_X)s,
    %(program_name)s, %(group_name)s) - SONiC templates write dhcrelay's
    "-a %%h:%%p %%P" for exactly this reason - then are shlex-split (no
    shell), exactly like supervisord's exec, and re-emitted as execline argv
    so globs like Ethernet[0-9]* stay literal;
  * dependent_startup_wait_for=X:running|exited -> a dependencies.d/X edge;
    dangling edges (target not converted) are pruned so s6-rc-compile succeeds;
  * a program waited on as X:exited is a oneshot; X:running is a longrun; a
    critical program is always a longrun; otherwise autorestart=true/unexpected
    or a non-zero startsecs -> longrun, startsecs=0 -> oneshot (a daemon must
    never become a oneshot or s6-rc bring-up would block forever);
  * user=<u> -> s6-setuidgid <u>;
  * startsecs/startretries (supervisord defaults 1 / 3): every longrun gets a `supervisor-meta`
    file for the supervisorctl shim; a non-critical longrun with startsecs > 0 also stamps its
    spawn time in `run` and counts too-quick exits in `finish` (BACKOFF with n s waits, FATAL
    and left down after more than startretries in a row; see finish_script);
  * each critical longrun gets a finish script that halts the container so
    systemd restarts it (mirrors supervisor-proc-exit-listener); critical names
    with no matching program (e.g. radv's radvd) are ignored, as service_checker
    itself tolerates;
  * like the listener, that halt is gated on CONFIG_DB FEATURE|<container> auto_restart: the
    container name comes from the (dropped) supervisor-proc-exit-listener eventlistener's
    --container-name, and the finish skips the halt only when auto_restart is `disabled`
    (see critical_finish_script for the semantics and the fail-safe deviation).
"""
import os
import re
import sys
import glob
import shlex
import shutil

HALT_CMD = "/run/s6/basedir/bin/halt"
# supervisorctl stop/restart (shim) drops a marker here first so the critical-process
# finish script can tell a requested stop from a crash (supervisord reports requested
# stops as STOPPED, which supervisor-proc-exit-listener ignores).
EXPECTED_DIR = "/run/s6-expected"
# Per-service runtime state (tmpfs): `started` (/proc/uptime at the last spawn, written by the
# run script), `backoff` (consecutive too-quick exits), `state` (backoff|fatal|exited|stopped,
# written by finish / the supervisorctl shim; keep in sync with supervisorctl.SUPER_DIR) and
# `stop-request` (supervisorctl is stopping the service).
SUPER_DIR = "/run/s6-super"
DEFAULT_BASE_CONF = "/etc/supervisor/supervisord.conf"
DEFAULT_CONFD_GLOB = "/etc/supervisor/conf.d/*.conf"
DEFAULT_CRITICAL = "/etc/supervisor/critical_processes"
S6_RC_D = "/etc/s6-overlay/s6-rc.d"
# The "user" bundle is the one s6-overlay v3 actually starts; it must live
# inside s6-rc.d itself (not a separate, unused directory) for s6-rc-compile
# to pick it up.
S6_BUNDLE_D = os.path.join(S6_RC_D, "user")

# Marker file dropped into every service directory this converter creates, so
# a later run can tell its own output apart from s6-overlay-owned service
# definitions and only clean up the former (see generate()).
MANAGED_MARKER = ".supervisord2s6-managed"


def _truthy(v):
    return str(v).strip().lower() in ("true", "1", "yes", "on")


_FALSY = ("false", "0", "no", "off")


def _autorestart(v):
    """supervisord's autorestart= -> "true" | "false" | "unexpected".

    A missing key is "unexpected" (supervisord's default). supervisord's boolean words
    are true/yes/on/1 and false/no/off/0 (any case); "unexpected" is the third value.
    Anything else is a config error in supervisord; we keep the program running with
    the default policy and warn.
    """
    if v is None or not str(v).strip():
        return "unexpected"
    w = str(v).strip().lower()
    if w == "unexpected":
        return w
    if w in ("true", "1", "yes", "on"):
        return "true"
    if w in _FALSY:
        return "false"
    _warn("autorestart=%r is not a supervisord value; using the default 'unexpected'" % str(v))
    return "unexpected"


def _warn(msg):
    sys.stderr.write("supervisord2s6: warning: %s\n" % msg)
    sys.stderr.flush()


def parse_ini(paths):
    """Minimal, predictable INI reader. Whole-line values (no inline-comment
    stripping) so command= with ';' inside quotes survives. Later files/keys
    override earlier ones."""
    sections = {}
    order = []
    cur = None
    for p in paths:
        try:
            with open(p) as f:
                raw_lines = f.readlines()
        except (FileNotFoundError, IsADirectoryError, PermissionError):
            continue
        for raw in raw_lines:
            line = raw.rstrip("\n")
            s = line.strip()
            if not s or s.startswith(";") or s.startswith("#"):
                continue
            if s.startswith("[") and s.endswith("]"):
                cur = s[1:-1].strip()
                if cur not in sections:
                    sections[cur] = {}
                    order.append(cur)
                continue
            if cur is None or "=" not in line:
                continue
            k, v = line.split("=", 1)
            sections[cur][k.strip()] = v.strip()
    return sections, order


def resolve_config_files(explicit):
    """Build the ordered list of files to parse. Follow [include] files= from
    the base file and always fold in conf.d/*.conf (where SONiC renders the
    programs)."""
    files = []
    seen = set()

    def add(p):
        for g in sorted(glob.glob(p)) if any(c in p for c in "*?[") else [p]:
            if g not in seen and os.path.isfile(g):
                seen.add(g)
                files.append(g)

    if explicit:
        add(explicit)
    else:
        add(DEFAULT_BASE_CONF)
    # expand includes declared by whatever base files we have so far
    base_sections, _ = parse_ini(list(files))
    inc = base_sections.get("include", {}).get("files")
    if inc:
        for tok in inc.split():
            base = os.path.dirname(files[0]) if files else "/etc/supervisor"
            add(tok if os.path.isabs(tok) else os.path.join(base, tok))
    add(DEFAULT_CONFD_GLOB)
    return files


def parse_groups(sections, order):
    """Map program name -> group name for every [group:X] section (supervisord's
    programs=a,b,c grouping, e.g. dhcp-relay/dhcp-server-ipv4). Used both to
    expand group: entries in critical_processes and to reproduce supervisord's
    "group:program" status naming (see supervisorctl's _display_name)."""
    group_of = {}
    for name in order:
        if not name.startswith("group:"):
            continue
        gname = name.split(":", 1)[1].strip()
        for p in sections[name].get("programs", "").split(","):
            p = p.strip()
            if p:
                group_of[p] = gname
    return group_of


def read_critical(path, group_of=None):
    group_of = group_of or {}
    names = set()
    try:
        with open(path) as f:
            for line in f:
                line = line.strip()
                # tolerate a missing trailing newline gluing the last entry to
                # anything downstream (seen in teamd's critical_processes)
                for chunk in line.split():
                    if chunk.startswith("program:"):
                        names.add(chunk.split(":", 1)[1].strip())
                    elif chunk.startswith("group:"):
                        gname = chunk.split(":", 1)[1].strip()
                        names.update(p for p, g in group_of.items() if g == gname)
    except (FileNotFoundError, IsADirectoryError, PermissionError):
        pass
    return names


_EXPAND_RE = re.compile(r"%%|%\(([A-Za-z_][A-Za-z0-9_]*)\)s")


def expand_supervisor(s, names=None):
    """Reproduce supervisord's %-expansion of command=/environment= values in a
    single pass: '%%' -> '%', '%(ENV_X)s' -> our own environment (the parent
    process environment, as supervisord does), and any key in `names`
    (program_name, group_name). Unknown %(key)s are left untouched. Single pass
    so an expanded value is never re-expanded."""
    names = names or {}

    def repl(m):
        key = m.group(1)
        if key is None:  # '%%'
            return "%"
        if key.startswith("ENV_"):
            return os.environ.get(key[4:], "")
        return names.get(key, m.group(0))
    return _EXPAND_RE.sub(repl, s)


def parse_environment(raw, names=None):
    """Parse a supervisord `environment=A="1",B=2` directive into [(k, v), ...],
    applying supervisord's %-expansion (see expand_supervisor) to each value."""
    pairs = []
    for k, v in re.findall(r'''([^,=\s]+)=("[^"]*"|'[^']*'|[^,]*)''', raw):
        v = v.strip()
        if len(v) >= 2 and v[0] == v[-1] and v[0] in "\"'":
            v = v[1:-1]
        v = expand_supervisor(v, names)
        pairs.append((k.strip(), v))
    return pairs


def build_model(sections, order, critical):
    progs = {}
    prog_order = []
    for name in order:
        if not name.startswith("program:"):
            continue
        attrs = sections[name]
        pname = name.split(":", 1)[1].strip()
        if not (_truthy(attrs.get("autostart", "false")) or
                _truthy(attrs.get("dependent_startup", "false"))):
            continue  # never started by supervisord/dependent-startup
        for msg in unsupported_settings(pname, attrs):
            _warn(msg)
        progs[pname] = attrs
        prog_order.append(pname)

    exited_targets, deps = set(), {}
    for pname in prog_order:
        edges = []
        for tok in progs[pname].get("dependent_startup_wait_for", "").replace(",", " ").split():
            tgt, _, state = tok.partition(":")
            edges.append(tgt)
            if state == "exited":
                exited_targets.add(tgt)
        deps[pname] = edges

    # A program is a oneshot iff someone must wait for it to COMPLETE (X:exited);
    # such programs are genuine run-once steps (start.sh, swssconfig, zsocket...).
    # Everything else is a longrun. This never turns a daemon into a oneshot
    # (which would block s6-rc bring-up forever).
    types = {p: ("oneshot" if p in exited_targets else "longrun") for p in prog_order}
    return progs, prog_order, deps, types


SIGNALS = {"HUP": 1, "INT": 2, "QUIT": 3, "KILL": 9, "USR1": 10, "USR2": 12, "TERM": 15}


def stop_signal(name):
    """supervisord stopsignal (TERM, KILL, INT, ...) -> signal number for s6's down-signal."""
    n = str(name).strip().upper()
    if n.startswith("SIG"):
        n = n[3:]
    if n.isdigit():
        return int(n)
    return SIGNALS.get(n, 15)


def stop_wait_ms(secs):
    """supervisord stopwaitsecs -> s6 timeout-kill milliseconds (default 10 s, like supervisord)."""
    try:
        return max(1, int(float(str(secs).strip()) * 1000))
    except ValueError:
        return 10000


# Keys with no s6 mapping that change what supervisord would do if they were set; none is
# used by SONiC's rendered configs, so a hit is a regression worth a visible warning.
WARNED_KEYS = {"numprocs", "process_name", "redirect_stderr"}


def unsupported_settings(pname, attrs):
    """-> list of warning strings for settings the converter cannot honour for one program."""
    out = []
    n = attrs.get("numprocs")
    if n is not None and str(n).strip() not in ("", "1"):
        out.append("program:%s sets numprocs=%s; only one instance is started" % (pname, n))
    pn = attrs.get("process_name")
    if pn is not None and pn.strip() not in ("", "%(program_name)s"):
        out.append("program:%s sets process_name=%s; the s6 service keeps the program name" % (pname, pn))
    rs = attrs.get("redirect_stderr")
    if rs is not None and _truthy(rs):
        out.append("program:%s sets redirect_stderr=%s; stderr is only folded into stdout "
                   "when stdout_syslog is on" % (pname, rs))
    return out


# supervisord program keys the converter maps to s6 (anything else is ignored; see IGNORED_KEYS).
HANDLED_KEYS = {"command", "autostart", "autorestart", "exitcodes", "user", "environment",
                "dependent_startup", "dependent_startup_wait_for", "stdout_syslog",
                "stopsignal", "stopwaitsecs", "startsecs", "startretries"}
# Known-ignored keys, deliberately: s6 has no equivalent, or it is irrelevant to the program.
#   priority          ordering comes from the dependent_startup edges
#   stdout_logfile/stderr_logfile  KNOWN DEVIATION: log files are never written. SONiC renders
#                     NONE and uses stdout_syslog (handled); a real path would be silently
#                     dropped (output goes to syslog if stdout_syslog is on, else the console)
#   stderr_syslog     stderr is always folded into stdout when stdout_syslog is on
#   stdout_capture_maxbytes  no capture mode
IGNORED_KEYS = {"priority", "stdout_logfile", "stderr_logfile",
                "stderr_syslog", "stdout_capture_maxbytes"}


def _execline_quote(arg):
    return '"' + arg.replace("\\", "\\\\").replace('"', '\\"') + '"'

# supervisord defaults for a program that does not set the key
DEFAULT_STARTSECS = 1
DEFAULT_STARTRETRIES = 3


def _int(v, default):
    """Tolerant non-negative integer (supervisord ints; '1.0'/junk/negative -> default)."""
    try:
        n = int(float(str(v).strip()))
    except (TypeError, ValueError):
        return default
    return n if n >= 0 else default


def exit_codes(raw):
    """supervisord exitcodes= (default "0") -> list of code strings."""
    codes = [c.strip() for c in str(raw).replace(",", " ").split() if c.strip()]
    return codes or ["0"]


def meta_text(attrs, critical):
    """Contents of the per-service `supervisor-meta` file the supervisorctl shim reads
    (copied into /run/service/<svc>/ by s6-rc-compile like `supervisor-name`)."""
    return ("startsecs=%d\nstartretries=%d\nstopwaitsecs=%d\nautorestart=%s\nexitcodes=%s\ncritical=%d\n" % (
        _int(attrs.get("startsecs"), DEFAULT_STARTSECS),
        _int(attrs.get("startretries"), DEFAULT_STARTRETRIES),
        _int(attrs.get("stopwaitsecs"), 10),
        _autorestart(attrs.get("autorestart")),
        ",".join(exit_codes(attrs.get("exitcodes", "0"))),
        1 if critical else 0))


def start_stamp(name):
    """execline prefix for `run`: record /proc/uptime at every spawn, so finish can tell a
    too-quick exit (monotonic; the wall clock can step and is not trusted)."""
    sh = ("mkdir -p %s/%s; read u _ < /proc/uptime; echo $u > %s/%s/started"
          % (SUPER_DIR, name, SUPER_DIR, name))
    return "foreground { /bin/sh -c %s } " % _execline_quote(sh)


_FINISH_ACCOUNTING = """#!/bin/sh
# $1 exit code (256 = killed by a signal)  $2 signal  $3 service  $4 pgid
# supervisord startsecs/startretries: an exit within startsecs of the spawn is "too quick"
# (BACKOFF, retried after n s), more than startretries of them in a row is FATAL (left down).
N=@NAME@; D=@SUPER@/$N; SS=@SS@; RT=@RT@; CODES="@CODES@"
mkdir -p "$D"
if [ -e @EXPECTED@/$N ] || [ -e "$D/stop-request" ]; then     # requested by supervisorctl: not a crash
    rm -f @EXPECTED@/$N "$D/backoff"
    echo stopped > "$D/state"
    exit 0
fi
cs() { _u=$1; _f=${_u#*.}; _f=${_f#0}; echo $(( ${_u%.*} * 100 + ${_f:-0} )); }
t0=0; [ -r "$D/started" ] && read t0 _ < "$D/started"
read u _ < /proc/uptime
run=$(( $(cs "$u") - $(cs "$t0") ))
if [ "$run" -ge 0 ] && [ "$run" -lt $((SS * 100)) ]; then
    n=0; [ -r "$D/backoff" ] && read n < "$D/backoff"
    n=$((n + 1)); echo $n > "$D/backoff"
    if [ "$n" -gt "$RT" ]; then
        echo fatal > "$D/state"
        logger -t s6-super "$N exited too quickly $n times (startretries=$RT); giving up (FATAL)"
        exit 125
    fi
    echo backoff > "$D/state"
    # supervisord waits n s before retry n; s6 itself adds 1 s, so wait n-1 s (stop interrupts it)
    i=0
    while [ "$i" -lt $(( (n - 1) * 10 )) ] && [ ! -e "$D/stop-request" ] && [ ! -e @EXPECTED@/$N ]; do
        sleep 0.1; i=$((i + 1))
    done
    exit 0
fi
rm -f "$D/backoff"
echo exited > "$D/state"
@TAIL@"""

_FINISH_TAIL = {
    "true": "exit 0\n",                      # restart unconditionally
    "false": "exit 125\n",                   # run once: leave it down
    "unexpected": 'for c in $CODES; do [ "$1" = "$c" ] && exit 125; done\nexit 0\n',
}


def finish_script(name, startsecs, startretries, mode, codes):
    """Finish script of a NON-critical longrun, or None for "no finish" (restart always).

    With startsecs == 0 supervisord never counts an exit as too quick, so the plain
    autorestart script is emitted (unchanged from before startsecs support). With
    startsecs > 0 the retry accounting wraps the same autorestart decision.
    """
    if startsecs <= 0:
        if mode == "unexpected":
            # exit codes listed in exitcodes (default "0") are expected: exit 125 tells
            # s6-supervise to leave the service down instead of restarting it.
            cond = " || ".join('[ "$1" = "%s" ]' % c for c in codes)
            return ("#!/bin/sh\nif " + cond + "; then\n    exit 125\nfi\nexit 0\n")
        if mode == "true":
            return None           # let s6-supervise restart it unconditionally
        # autorestart=false: run once. finish exit 125 brings the service down (no tight loop).
        return "#!/bin/sh\nexit 125\n"
    txt = _FINISH_ACCOUNTING
    for k, v in (("@NAME@", shlex.quote(name)), ("@SUPER@", SUPER_DIR), ("@EXPECTED@", EXPECTED_DIR),
                 ("@SS@", str(startsecs)), ("@RT@", str(startretries)), ("@CODES@", " ".join(codes)),
                 ("@TAIL@", _FINISH_TAIL[mode])):
        txt = txt.replace(k, v)
    return txt


# supervisor-proc-exit-listener(-rs) is the stock eventlistener that halts a SONiC container
# when a critical process dies; its --container-name is the CONFIG_DB FEATURE key it consults.
LISTENER_PROG = "supervisor-proc-exit-listener"
# FEATURE keys are plain feature names; anything else is not put into a shell script.
_CONTAINER_NAME_RE = re.compile(r"^[A-Za-z0-9][A-Za-z0-9_.-]*$")
# Upper bound of the on-box auto_restart query (`timeout -k 1 N sonic-db-cli ...`, so N + 1 s at
# worst). It must stay well below the finish script's timeout-finish: s6-supervise SIGKILLs a
# finish that overruns and then simply respawns the service, i.e. a hung query would silently
# skip the halt the fail-safe promises.
AUTORESTART_QUERY_TIMEOUT_S = 2
CRITICAL_TIMEOUT_FINISH_MS = 10000


def listener_options(sections):
    """supervisor-proc-exit-listener's options -> {"container": name, "unix_socket": bool}, or None.

    Looks at every [eventlistener:*] whose command runs the listener (the Rust
    `supervisor-proc-exit-listener-rs` or the Python `supervisor-proc-exit-listener`) and reads
    `--container-name X` / `-c X` (also `=X` / `-cX`) and `-s` / `--use-unix-socket-path`. The
    converter drops these listeners (s6 replaces them); this keeps what the critical finish needs to
    mirror them. None when there is no listener, no container name, or an implausible name
    (a warning is printed for the last case).
    """
    for sec, attrs in sections.items():
        if not sec.startswith("eventlistener:"):
            continue
        try:
            toks = shlex.split(attrs.get("command", ""))
        except ValueError:
            continue
        idx = [i for i, t in enumerate(toks) if os.path.basename(t).startswith(LISTENER_PROG)]
        if not idx:
            continue
        args, name, unix = toks[idx[0] + 1:], None, False
        j = 0
        while j < len(args):
            a = args[j]
            if a in ("-c", "--container-name") and j + 1 < len(args):
                name, j = args[j + 1], j + 1
            elif a.startswith("--container-name="):
                name = a.split("=", 1)[1]
            elif a.startswith("-c") and not a.startswith("--") and len(a) > 2:
                name = a[2:]
            elif a in ("-s", "--use-unix-socket-path"):
                unix = True
            j += 1
        if not name:
            continue
        if not _CONTAINER_NAME_RE.match(name):
            _warn("[%s] --container-name %r is not a plausible FEATURE key; "
                  "critical processes always halt the container" % (sec, name))
            continue
        return {"container": name, "unix_socket": unix}
    return None


def critical_finish_script(name, listener, mode, codes):
    """Finish script of a CRITICAL longrun: halt the container (systemd restarts it) when it exits.

    An exit that supervisorctl requested (marker in EXPECTED_DIR) never halts.

    With a known container (`listener`, see listener_options) the halt is gated like stock's
    supervisor-proc-exit-listener, which only terminates supervisord when CONFIG_DB
    FEATURE|<container> auto_restart is `enabled` (sonic-mgmt tests disable it and then kill/stop
    critical processes). The finish asks `sonic-db-cli CONFIG_DB HGET FEATURE|<container> auto_restart`
    once (this rare exit path only; at most AUTORESTART_QUERY_TIMEOUT_S + 1 s):
      * `disabled`  -> log one line, do NOT halt, and treat the exit like any other program's:
                       the program's own autorestart policy decides (`false`, every critical
                       program SONiC ships bar sysmgr's rebootbackend: stays down, exit 125;
                       `true`: s6 respawns it; `unexpected`: down iff the exit code is in
                       exitcodes), which is what supervisord does while the listener only alerts;
      * anything else (`enabled`, an unset field, an unreadable/empty/odd answer, query error or
        timeout, sonic-db-cli missing) -> halt.
    Deliberate deviation from stock: stock halts only on exactly `enabled` and treats an unreachable
    CONFIG_DB (or a missing FEATURE row) as "not enabled" (no halt); here every doubt halts, because
    a container that cannot reach its database is already broken and a silent dead critical process
    is the worse outcome. exitcodes are NOT consulted: a critical process that exits 0 on its own
    still halts (stock treats exit codes listed in exitcodes as expected and does not).
    Without a listener the container name is unknown and the finish always halts (as before).
    """
    marker = EXPECTED_DIR + "/" + name
    head = ("#!/command/with-contenv sh\n"
            "if [ -e " + marker + " ]; then\n"
            "    rm -f " + marker + "\n"
            "    exit 0\n"
            "fi\n")
    what = "critical process " + name + " exited (code=$1 signal=$2)"
    if not listener:
        return (head + 'logger -t s6-super "' + what + '; halting container for restart"\n'
                "exec " + HALT_CMD + "\n")
    container = listener["container"]
    key = shlex.quote("FEATURE|" + container)
    tail = _FINISH_TAIL[mode].replace("$CODES", " ".join(codes))
    return (head +
            "# auto_restart gate (stock supervisor-proc-exit-listener parity; empty/error -> halt)\n"
            "v=$(timeout -k 1 " + str(AUTORESTART_QUERY_TIMEOUT_S) + " sonic-db-cli" +
            (" -s" if listener.get("unix_socket") else "") +
            " CONFIG_DB HGET " + key + " auto_restart 2>/dev/null) || v=\n"
            'if [ "$v" = "disabled" ]; then\n'
            '    logger -t s6-super "' + what + "; auto_restart=disabled for " + container +
            ', not halting container"\n'
            "    mkdir -p " + SUPER_DIR + "/" + name + "; echo exited > " + SUPER_DIR + "/" + name + "/state\n" +
            "".join("    " + ln + "\n" for ln in tail.splitlines()) +
            "fi\n"
            'logger -t s6-super "' + what + '; halting container for restart (auto_restart=${v:-unknown})"\n'
            "exec " + HALT_CMD + "\n")



def _command_line(argv, user, env_pairs=None, syslog_tag=None):
    parts = ["with-contenv"]
    if syslog_tag:
        # supervisord's stdout_syslog/stderr_syslog=true: pipe the program's output
        # (stderr folded into stdout) to syslog tagged with the program name.
        # `pipeline -w` keeps the daemon as the parent process (s6 supervises the
        # daemon pid) and runs logger as the reader child.
        parts += ["pipeline", "-w", "{", "logger", "-t", _execline_quote(syslog_tag), "}",
                  "fdmove", "-c", "2", "1"]
    if env_pairs:
        parts += ["s6-env"] + [_execline_quote("%s=%s" % (k, v)) for k, v in env_pairs]
    if user:
        parts += ["s6-setuidgid", user]
    parts += [_execline_quote(a) for a in argv]
    return " ".join(parts)


def _write(path, content, mode=0o644):
    with open(path, "w") as f:
        f.write(content)
    os.chmod(path, mode)


def generate(progs, prog_order, deps, types, critical, outdir, bundledir, group_of=None, listener=None):
    """Write the s6-rc source tree. `listener` = listener_options(sections) (None: no auto_restart gate)."""
    group_of = group_of or {}
    os.makedirs(outdir, exist_ok=True)
    # Only remove service directories this converter created on a previous
    # run (tracked via MANAGED_MARKER); leave every s6-overlay-owned service
    # definition under s6-rc.d alone.
    for entry in sorted(os.listdir(outdir)):
        path = os.path.join(outdir, entry)
        if os.path.isdir(path) and os.path.exists(os.path.join(path, MANAGED_MARKER)):
            shutil.rmtree(path)

    for pname in prog_order:
        sd = os.path.join(outdir, pname)
        os.makedirs(os.path.join(sd, "dependencies.d"), exist_ok=True)
        t = types[pname]
        _write(os.path.join(sd, "type"), t + "\n")
        _write(os.path.join(sd, MANAGED_MARKER), "")
        # supervisorctl status prints "group:program" for grouped processes
        # (service_checker.py's group: critical-process expansion and
        # docker-dhcp-relay's start.sh both rely on this exact format); record
        # it since the s6 service dir itself must stay named after the bare
        # program for dependencies.d/ edges to resolve.
        display_name = "%s:%s" % (group_of[pname], pname) if pname in group_of else pname
        _write(os.path.join(sd, "supervisor-name"), display_name + "\n")
        names = {"program_name": pname}
        if pname in group_of:
            names["group_name"] = group_of[pname]
        argv = shlex.split(expand_supervisor(progs[pname]["command"], names))
        user = progs[pname].get("user")
        env_pairs = parse_environment(progs[pname].get("environment", ""), names)
        syslog_tag = pname if _truthy(progs[pname].get("stdout_syslog", "false")) else None
        cmd = _command_line(argv, user, env_pairs, syslog_tag)
        if t == "longrun":
            # supervisord stopsignal/stopwaitsecs (defaults TERM / 10 s then SIGKILL)
            _write(os.path.join(sd, "down-signal"),
                   str(stop_signal(progs[pname].get("stopsignal", "TERM"))) + "\n")
            _write(os.path.join(sd, "timeout-kill"),
                   str(stop_wait_ms(progs[pname].get("stopwaitsecs", "10"))) + "\n")
            autorestart = _autorestart(progs[pname].get("autorestart"))
            startsecs = _int(progs[pname].get("startsecs"), DEFAULT_STARTSECS)
            startretries = _int(progs[pname].get("startretries"), DEFAULT_STARTRETRIES)
            # retry accounting for non-critical programs only: a critical process that dies
            # halts the container whatever its age, so there is nothing to count.
            accounting = pname not in critical and startsecs > 0
            _write(os.path.join(sd, "run"),
                   "#!/command/execlineb -P\n" + (start_stamp(pname) if accounting else "") + cmd + "\n",
                   0o755)
            _write(os.path.join(sd, "supervisor-meta"), meta_text(progs[pname], pname in critical))
            if accounting and startretries >= 5:
                # the finish script sleeps up to (startretries - 1) s between retries; s6 kills a
                # finish that runs longer than timeout-finish (default 5 s)
                _write(os.path.join(sd, "timeout-finish"), "%d\n" % ((startretries + 2) * 1000))
            if pname in critical:
                # critical process died -> halt container so systemd restarts it
                # (mirrors supervisor-proc-exit-listener, incl. its auto_restart gate)
                _write(os.path.join(sd, "finish"),
                       critical_finish_script(pname, listener, autorestart,
                                              exit_codes(progs[pname].get("exitcodes", "0"))), 0o755)
                if listener:
                    # the gate's query (bounded to AUTORESTART_QUERY_TIMEOUT_S + 1 s) must never
                    # run into s6's default 5 s finish timeout, which would skip the halt
                    _write(os.path.join(sd, "timeout-finish"), "%d\n" % CRITICAL_TIMEOUT_FINISH_MS)
            else:
                fin = finish_script(pname, startsecs, startretries, autorestart,
                                    exit_codes(progs[pname].get("exitcodes", "0")))
                if fin is not None:
                    _write(os.path.join(sd, "finish"), fin, 0o755)
        else:  # oneshot: single-line execline 'up', no shebang
            _write(os.path.join(sd, "up"), cmd + "\n", 0o755)
        for tgt in deps.get(pname, []):
            if tgt in progs:  # prune dangling edges
                open(os.path.join(sd, "dependencies.d", tgt), "w").close()

    # The "user" bundle directory itself may be owned by s6-overlay; keep it
    # and only replace its contents.d entries with the services we manage.
    ud = bundledir
    os.makedirs(ud, exist_ok=True)
    cd = os.path.join(ud, "contents.d")
    if os.path.isdir(cd):
        shutil.rmtree(cd)
    os.makedirs(cd)
    _write(os.path.join(ud, "type"), "bundle\n")
    for pname in prog_order:
        open(os.path.join(cd, pname), "w").close()
    return prog_order, types


def convert(config=None, critical_path=DEFAULT_CRITICAL, outdir=S6_RC_D,
            bundledir=S6_BUNDLE_D):
    files = resolve_config_files(config)
    sections, order = parse_ini(files)
    group_of = parse_groups(sections, order)
    critical = read_critical(critical_path, group_of)
    progs, prog_order, deps, types = build_model(sections, order, critical)
    generate(progs, prog_order, deps, types, critical, outdir, bundledir, group_of,
             listener=listener_options(sections))
    try:
        write_loaded_manifest(sections, order, group_of, config)
    except OSError as e:  # only `supervisorctl reread/update` needs it; never block the boot
        _warn("cannot record the loaded program set (%s); supervisorctl reread/update disabled" % e)
    return progs, prog_order, deps, types, critical


# ---------------------------------------------------------------------------
# Runtime support for `supervisorctl reread` / `update` (programs added after boot).
#
# convert() records which [program:x] sections were loaded (and a digest of each) in
# LOADED_MANIFEST; the supervisorctl shim diffs the config files against it (reread) and adds
# the new programs to the live s6 scan directory (update) with the same generate() code the
# boot-time conversion uses.  Changed/removed programs cannot be applied live.
# ---------------------------------------------------------------------------
import hashlib
import json
import tempfile

# Loaded-program manifest.  Lives next to the generated s6-rc source tree (written before /init
# starts, so it must not be under /run); rewritten on every container start.
LOADED_MANIFEST = "/etc/s6-overlay/supervisord2s6.loaded.json"
# Live servicedirs of programs added at runtime; linked into the scan directory (/run/service)
# by supervisorctl update.  Not part of the s6-rc compiled database.
DYN_DIR = "/run/s6-dyn"


def program_digest(attrs):
    """Stable digest of one [program:x] section (any key change shows up as 'changed')."""
    return hashlib.sha1(json.dumps(sorted(attrs.items())).encode()).hexdigest()


def loaded_programs(sections, order, group_of):
    """-> {program: {"group": <group name or None>, "sha": digest}} for EVERY [program:x] section,
    including those build_model skips (autostart=false) - supervisord has them loaded too."""
    out = {}
    for name in order:
        if name.startswith("program:"):
            p = name.split(":", 1)[1].strip()
            out[p] = {"group": group_of.get(p), "sha": program_digest(sections[name])}
    return out


def save_loaded_manifest(data, path=None):
    path = path or LOADED_MANIFEST
    os.makedirs(os.path.dirname(path), exist_ok=True)
    tmp = path + ".tmp"
    with open(tmp, "w") as f:
        json.dump(data, f, sort_keys=True)
    os.replace(tmp, path)


def write_loaded_manifest(sections, order, group_of, config, path=None):
    save_loaded_manifest({"config": config, "programs": loaded_programs(sections, order, group_of)}, path)


def read_loaded_manifest(path=None):
    """-> manifest dict, or None when missing/unreadable (container not converted by this version)."""
    try:
        with open(path or LOADED_MANIFEST) as f:
            data = json.load(f)
    except (OSError, ValueError):
        return None
    return data if isinstance(data.get("programs"), dict) else None


def config_groups(programs):
    """-> {group name: {program: digest}}; a program outside any [group:x] is its own group
    (supervisord names process groups, and `reread`/`update` speak in group names)."""
    out = {}
    for p, e in programs.items():
        out.setdefault(e.get("group") or p, {})[p] = e.get("sha")
    return out


def diff_loaded(old, new):
    """supervisord's diff_to_active over two {program: entry} maps -> (added, changed, removed)
    sorted lists of group names."""
    og, ng = config_groups(old), config_groups(new)
    return (sorted(set(ng) - set(og)),
            sorted(g for g in set(og) & set(ng) if og[g] != ng[g]),
            sorted(set(og) - set(ng)))


def current_config(manifest):
    """Re-read the config files the container was started with -> (sections, order, group_of)."""
    sections, order = parse_ini(resolve_config_files(manifest.get("config")))
    return sections, order, parse_groups(sections, order)


def build_runtime_servicedir(pname, attrs, group_of, dest, autostart=True):
    """Create the live servicedir `dest` of a program added after boot.

    The files come from generate() itself (single-program call into a scratch tree, always a
    non-critical longrun: supervisord only reads critical_processes at start, so a program added
    later cannot halt the container), minus the s6-rc-source-only entries (type, dependencies.d,
    managed marker).  A program with autostart=false gets a ./down file, so s6-supervise
    leaves it stopped until `supervisorctl start`.  dest appears atomically (rename)."""
    parent = os.path.dirname(dest)
    os.makedirs(parent, exist_ok=True)
    scratch = tempfile.mkdtemp(prefix=".gen-", dir=parent)
    try:
        generate({pname: attrs}, [pname], {}, {pname: "longrun"}, set(),
                 scratch, os.path.join(scratch, "user"), group_of)
        sd = os.path.join(scratch, pname)
        for n in ("type", MANAGED_MARKER):
            os.remove(os.path.join(sd, n))
        shutil.rmtree(os.path.join(sd, "dependencies.d"))
        if not autostart:
            _write(os.path.join(sd, "down"), "")
        if os.path.lexists(dest):  # stale leftover of a previous run of this container
            shutil.rmtree(dest)
        os.rename(sd, dest)
    finally:
        shutil.rmtree(scratch, ignore_errors=True)


REAL_SUPERVISORD = ("/usr/local/bin/supervisord.real", "/usr/bin/supervisord.real")


def exec_real(argv):
    """Replace this process with the real supervisord (kept as *.real)."""
    for real in REAL_SUPERVISORD:
        if os.path.exists(real):
            os.execv(real, [real] + list(argv))
    sys.stderr.write("supervisord2s6: supervisord.real not found\n")
    return 127


def main(argv):
    # offline emit mode for verification
    if "--emit-only" in argv:
        cfg = crit = out = bundle = None
        for i in range(len(argv)):
            if argv[i] == "--config":
                cfg = argv[i + 1]
            elif argv[i] == "--critical":
                crit = argv[i + 1]
            elif argv[i] == "--out":
                out = argv[i + 1]
            elif argv[i] == "--bundle-out":
                bundle = argv[i + 1]
        # in emit mode we bypass include/conf.d discovery and use the file given
        files = [cfg] if cfg else resolve_config_files(None)
        sections, order = parse_ini(files)
        group_of = parse_groups(sections, order)
        critical = read_critical(crit or DEFAULT_CRITICAL, group_of)
        progs, prog_order, deps, types = build_model(sections, order, critical)
        out = out or "./s6-rc.d"
        generate(progs, prog_order, deps, types, critical,
                 out, bundle or os.path.join(out, "user"), group_of,
                 listener=listener_options(sections))
        for p in prog_order:
            d = ",".join(t for t in deps.get(p, []) if t in progs) or "-"
            star = "*" if p in critical and types[p] == "longrun" else " "
            print("  %-18s %-8s deps=%-24s crit=%s" % (p, types[p], d, star.strip() or "no"))
        return 0

    # s6-overlay's /init only works as PID 1 (s6-linux-init falls back to
    # telinit otherwise), e.g. never under `docker run --pid=host`. Anywhere
    # else, behave exactly like the original supervisord.
    if os.getpid() != 1:
        return exec_real(argv)

    # shim mode: parse supervisord's -c if present, generate, exec /init
    cfg = None
    for i, a in enumerate(argv):
        if a == "-c" and i + 1 < len(argv):
            cfg = argv[i + 1]
        elif a.startswith("-c") and len(a) > 2:
            cfg = a[2:]
    try:
        convert(config=cfg)
    except Exception as e:  # never leave the container without an init
        sys.stderr.write("supervisord2s6: conversion failed: %r\n" % e)
        sys.stderr.flush()
        raise
    os.execv("/init", ["/init"])


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
