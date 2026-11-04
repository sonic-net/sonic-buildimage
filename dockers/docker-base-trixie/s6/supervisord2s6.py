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
    itself tolerates.
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


def generate(progs, prog_order, deps, types, critical, outdir, bundledir, group_of=None):
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
                # (mirrors supervisor-proc-exit-listener)
                _write(os.path.join(sd, "finish"),
                       "#!/command/with-contenv sh\n"
                       "if [ -e " + EXPECTED_DIR + "/" + pname + " ]; then\n"
                       "    rm -f " + EXPECTED_DIR + "/" + pname + "\n"
                       "    exit 0\n"
                       "fi\n"
                       'logger -t s6-super "critical process ' + pname +
                       ' exited (code=$1 signal=$2); halting container for restart"\n'
                       "exec " + HALT_CMD + "\n", 0o755)
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
    generate(progs, prog_order, deps, types, critical, outdir, bundledir, group_of)
    return progs, prog_order, deps, types, critical


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
                 out, bundle or os.path.join(out, "user"), group_of)
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
