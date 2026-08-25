#!/usr/bin/env python3
"""supervisord -> s6-overlay converter and drop-in replacement.

Two modes:
  * shim mode (default, installed as /usr/local/bin/supervisord): parse the
    already-rendered supervisord config chain + critical_processes, generate the
    s6-rc source tree under /etc/s6-overlay, then exec /init. This makes s6 the
    only supervisor — there is intentionally no supervisord fallback.
  * offline mode (--emit-only): generate the tree from explicit --config /
    --critical into --out / --bundle-out, without exec'ing. Used to verify the
    converter against each container's captured config before building.

The converter reproduces what supervisord + supervisord-dependent-startup would
actually do:
  * only programs that would actually start are converted: autostart=true OR
    dependent_startup=true (so e.g. FRR 'sharpd', which has neither, is skipped);
  * commands are shlex-split (no shell), exactly like supervisord's exec, and
    re-emitted as execline argv so globs like Ethernet[0-9]* stay literal;
  * dependent_startup_wait_for=X:running|exited -> a dependencies.d/X edge;
    dangling edges (target not converted) are pruned so s6-rc-compile succeeds;
  * a program waited on as X:exited is a oneshot; X:running is a longrun; a
    critical program is always a longrun; otherwise autorestart=true/unexpected
    or a non-zero startsecs -> longrun, startsecs=0 -> oneshot (a daemon must
    never become a oneshot or s6-rc bring-up would block forever);
  * user=<u> -> s6-setuidgid <u>;
  * each critical longrun gets a finish script that halts the container so
    systemd restarts it (mirrors supervisor-proc-exit-listener); critical names
    with no matching program (e.g. radv's radvd) are ignored, as service_checker
    itself tolerates.
"""
import os
import sys
import glob
import stat
import shlex
import shutil

HALT_CMD = "/run/s6/basedir/bin/halt"
DEFAULT_BASE_CONF = "/etc/supervisor/supervisord.conf"
DEFAULT_CONFD_GLOB = "/etc/supervisor/conf.d/*.conf"
DEFAULT_CRITICAL = "/etc/supervisor/critical_processes"
S6_RC_D = "/etc/s6-overlay/s6-rc.d"
S6_BUNDLE_D = "/etc/s6-overlay/user-bundles.d"

IGNORE_SECTION_PREFIXES = ("supervisord", "unix_http_server", "supervisorctl",
                           "rpcinterface:", "include", "eventlistener:")


def _truthy(v):
    return str(v).strip().lower() in ("true", "1", "yes", "on")


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


def read_critical(path):
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
                    # group: is unused in this image; would need supervisorctl
                    # expansion, so we skip it here.
    except (FileNotFoundError, IsADirectoryError, PermissionError):
        pass
    return names


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


def _execline_quote(arg):
    return '"' + arg.replace("\\", "\\\\").replace('"', '\\"') + '"'


def _command_line(argv, user):
    parts = ["with-contenv"]
    if user:
        parts += ["s6-setuidgid", user]
    parts += [_execline_quote(a) for a in argv]
    return " ".join(parts)


def _write(path, content, mode=0o644):
    with open(path, "w") as f:
        f.write(content)
    os.chmod(path, mode)


def generate(progs, prog_order, deps, types, critical, outdir, bundledir):
    if os.path.isdir(outdir):
        shutil.rmtree(outdir)
    os.makedirs(outdir)
    for pname in prog_order:
        sd = os.path.join(outdir, pname)
        os.makedirs(os.path.join(sd, "dependencies.d"), exist_ok=True)
        t = types[pname]
        _write(os.path.join(sd, "type"), t + "\n")
        argv = shlex.split(progs[pname]["command"])
        user = progs[pname].get("user")
        cmd = _command_line(argv, user)
        if t == "longrun":
            _write(os.path.join(sd, "run"),
                   "#!/command/execlineb -P\n" + cmd + "\n", 0o755)
            autorestart = progs[pname].get("autorestart", "false").strip().lower()
            if pname in critical:
                # critical process died -> halt container so systemd restarts it
                # (mirrors supervisor-proc-exit-listener)
                _write(os.path.join(sd, "finish"),
                       "#!/command/with-contenv sh\n"
                       'logger -t s6-super "critical process ' + pname +
                       ' exited (code=$1 signal=$2); halting container for restart"\n'
                       "exec " + HALT_CMD + "\n", 0o755)
            elif autorestart in ("true", "unexpected"):
                pass  # let s6-supervise restart it (default longrun behaviour)
            else:
                # autorestart=false, non-critical: run once / do not restart.
                # finish exit code 125 tells s6-supervise to bring the service
                # down instead of restarting (no tight-loop), matching supervisord.
                _write(os.path.join(sd, "finish"),
                       "#!/bin/sh\nexit 125\n", 0o755)
        else:  # oneshot: single-line execline 'up', no shebang
            _write(os.path.join(sd, "up"), cmd + "\n")
        for tgt in deps.get(pname, []):
            if tgt in progs:  # prune dangling edges
                open(os.path.join(sd, "dependencies.d", tgt), "w").close()

    ud = os.path.join(bundledir, "user")
    if os.path.isdir(ud):
        shutil.rmtree(ud)
    os.makedirs(os.path.join(ud, "contents.d"))
    _write(os.path.join(ud, "type"), "bundle\n")
    for pname in prog_order:
        open(os.path.join(ud, "contents.d", pname), "w").close()
    return prog_order, types


def convert(config=None, critical_path=DEFAULT_CRITICAL, outdir=S6_RC_D,
            bundledir=S6_BUNDLE_D):
    files = resolve_config_files(config)
    sections, order = parse_ini(files)
    critical = read_critical(critical_path)
    progs, prog_order, deps, types = build_model(sections, order, critical)
    generate(progs, prog_order, deps, types, critical, outdir, bundledir)
    return progs, prog_order, deps, types, critical


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
        critical = read_critical(crit or DEFAULT_CRITICAL)
        progs, prog_order, deps, types = build_model(sections, order, critical)
        generate(progs, prog_order, deps, types, critical,
                 out or "./s6-rc.d", bundle or "./user-bundles.d")
        for p in prog_order:
            d = ",".join(t for t in deps.get(p, []) if t in progs) or "-"
            star = "*" if p in critical and types[p] == "longrun" else " "
            print("  %-18s %-8s deps=%-24s crit=%s" % (p, types[p], d, star.strip() or "no"))
        return 0

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
