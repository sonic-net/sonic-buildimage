#!/usr/bin/env python3
"""Service-enablement effects of the host packages' maintainer scripts.

Emulates, for a fresh `dpkg --configure` of every package (control tars in
install order), the parts of the postinst that touch service enablement:

* deb-systemd-helper enable / update-state / mask / unmask (dh_installsystemd,
  dh_systemd_enable and hand-written copies of those snippets): symlinks below
  /etc/systemd/{system,user} plus the state in
  /var/lib/systemd/deb-systemd[-user]-helper-{enabled,masked}/ (init-system-helpers
  1.65 semantics: link closure from WantedBy/RequiredBy/UpheldBy/Alias/Also,
  "<unit>.dsh-also" files in closure order, one empty marker per link).
* systemctl enable / mask (offline, as in a chroot): symlinks only.
* update-rc.d defaults / defaults-disabled / enable / disable / remove: the
  image has no insserv, so update-rc.d's built-in fallback applies — S01/K01
  links in /etc/rc<lvl>.d from the LSB Default-Start/Default-Stop header.
* extra effects of hand-written postinst code (--extra JSON), e.g. screen's
  masked screen-cleanup.service.

The postinst is evaluated by a small line-based interpreter: if/elif/else/fi
nesting with the conditions of a first configure ($1=configure, $2 empty,
DPKG_ROOT empty, no running systemd); conditions affecting service state must be decidable or generation fails. Shell function bodies are skipped. Unit files and init
scripts are read from the package payloads (--payload tars).
"""

import argparse
import fnmatch
import io
import json
import re
import shlex
import sys
import tarfile

UNIT_DIRS = ("etc/systemd/system", "usr/lib/systemd/system")
DSH_ENABLED = "var/lib/systemd/deb-systemd-helper-enabled"
DSH_MASKED = "var/lib/systemd/deb-systemd-helper-masked"
MERGED = ("bin", "sbin", "lib", "lib32", "lib64", "libx32")


def norm(path):
    path = path[2:] if path.startswith("./") else path.lstrip("/")
    path = path.rstrip("/")
    head = path.split("/", 1)[0]
    if head in MERGED:
        path = "usr/" + path
    return path


def wanted(path):
    """Unit files, init scripts and rc links: the inputs of the emulation."""
    d, _, base = path.rpartition("/")
    return bool(base) and (d in ("usr/lib/systemd/system", "etc/systemd/system", "usr/lib/systemd/user", "etc/systemd/user", "etc/init.d")
                           or re.match(r"^etc/rc[0-6S]\.d$", d) is not None)


class Fs:
    """The relevant part of the image filesystem: units, init scripts, links."""

    def __init__(self):
        self.files = {}  # path -> bytes
        self.links = {}  # path -> target
        self.dirs = set()
        self.regular = set()
        self.modes = {}

    def load(self, tar_path):
        with tarfile.open(tar_path, "r:*") as tf:
            for m in tf:
                p = norm(m.name)
                if not p or "/.wh." in "/" + p:
                    continue
                if m.isdir():
                    self.dirs.add(p)
                elif m.isfile():
                    self.regular.add(p)
                    self.modes[p] = m.mode
                    self.links.pop(p, None)
                elif m.issym():
                    self.regular.discard(p)
                    self.modes.pop(p, None)
                    if m.name.removeprefix("./").rstrip("/") in ("bin", "sbin", "lib", "lib64") and norm(m.linkname) == p:
                        continue
                    self.links[p] = m.linkname
                if not wanted(p):
                    continue
                if m.isfile():
                    self.files[p] = tf.extractfile(m).read()
                    self.links.pop(p, None)
                elif m.issym():
                    self.links[p] = m.linkname
                    self.files.pop(p, None)

    def resolve(self, path, depth=0):
        if depth > 16:
            return None
        if path in self.regular:
            return path
        if path in self.links:
            tgt = self.links[path]
            if tgt.startswith("/"):
                return self.resolve(norm(tgt), depth + 1)
            return self.resolve(norm(path.rsplit("/", 1)[0] + "/" + tgt), depth + 1)
        return None

    def is_file(self, path):  # test -f (follows links)
        return self.resolve(path) is not None

    def is_link(self, path):
        return path in self.links

    def exists(self, path):
        return path in self.regular or path in self.links or path in self.dirs

    def read(self, path):
        real = self.resolve(path)
        return self.files[real].decode("utf-8", "replace") if real else None


class State:
    def __init__(self, fs):
        self.fs = fs
        self.created_links = {}  # path -> target (ordered by creation)
        self.removed = set()
        self.files = {}  # path -> bytes (state files)
        self.mkdirs = set()
        self.log = []
        self.variables = {}
        self.instance = "system"

    @property
    def enabled_dir(self):
        return "var/lib/systemd/deb-systemd-" + ("user-" if self.instance == "user" else "") + "helper-enabled"

    @property
    def masked_dir(self):
        return "var/lib/systemd/deb-systemd-" + ("user-" if self.instance == "user" else "") + "helper-masked"

    def helper_call(self, action, unit, user=False):
        previous = self.instance
        self.instance = "user" if user else "system"
        try:
            return getattr(self, "dsh_" + action.replace("-", "_"))(unit)
        finally:
            self.instance = previous

    # -- filesystem primitives -------------------------------------------
    def make_path(self, d):
        parts = d.split("/")
        for i in range(1, len(parts) + 1):
            self.mkdirs.add("/".join(parts[:i]))

    def symlink(self, path, target):
        if self.fs.exists(path) or path in self.files:
            return False
        self.make_path(path.rsplit("/", 1)[0])
        self.fs.links[path] = target
        self.created_links[path] = target
        self.removed.discard(path)
        return True

    def unlink(self, path):
        if path in self.fs.links:
            del self.fs.links[path]
            self.created_links.pop(path, None)
            self.removed.add(path)
        if path in self.files:
            del self.files[path]

    def write(self, path, data):
        self.make_path(path.rsplit("/", 1)[0])
        self.files[path] = data

    # -- unit lookup -----------------------------------------------------
    def find_unit(self, unit):
        """deb-systemd-helper find_unit: /etc, /lib, /usr/lib (test -f)."""
        if self.fs.is_file(("etc/systemd/" + self.instance + "/") + unit):
            return ("/etc/systemd/" + self.instance + "/") + unit
        if self.fs.is_file(("usr/lib/systemd/" + self.instance + "/") + unit):
            return ("/lib/systemd/" + self.instance + "/") + unit
        return None

    def link_closure(self, unit, service_path, seen=None):
        """deb-systemd-helper get_link_closure: [(link, target)] in order."""
        seen = set() if seen is None else seen
        if unit in seen:
            return []
        seen.add(unit)
        text = self.fs.read(norm(service_path))
        if text is None:
            return []
        links, wants = [], []
        base = service_path.rsplit("/", 1)[1]
        for line in text.splitlines():
            m = re.match(r"^\s*(WantedBy|RequiredBy|UpheldBy)=(.+)$", line, re.I)
            if m:
                suffix = {"wantedby": ".wants", "requiredby": ".requires", "upheldby": ".upholds"}[m.group(1).lower()]
                for v in shlex.split(m.group(2)):
                    wants.append("etc/systemd/%s/%s%s/" % (self.instance, v, suffix))
            m = re.match(r"^\s*Also=(.+)$", line, re.I)
            if m:
                for v in shlex.split(m.group(1)):
                    p = self.find_unit(v)
                    if p:
                        links += self.link_closure(v, p, seen)
            m = re.match(r"^\s*Alias=(.+)$", line, re.I)
            if m:
                for v in shlex.split(m.group(1)):
                    if v != base:
                        links.append((("etc/systemd/" + self.instance + "/") + v, service_path))
        for w in wants:
            links.append((w + base, service_path))
        return links

    # -- deb-systemd-helper ----------------------------------------------
    def dsh_state(self, unit):
        return "%s/%s.dsh-also" % (self.enabled_dir, unit)

    def dsh_record(self, unit, link):
        st = self.dsh_state(unit)
        lines = self.files.get(st, b"").decode().splitlines()
        if link not in lines:
            lines.append(link)
        self.write(st, "".join(l + "\n" for l in lines).encode())

    def dsh_enable(self, unit):
        path = self.find_unit(unit)
        if not path:
            self.log.append("dsh enable %s: unit not found" % unit)
            return
        for link, target in self.link_closure(unit, path):
            self.dsh_record(unit, "/" + link)
            marker = self.enabled_dir + "/" + link[len(("etc/systemd/" + self.instance + "/")):]
            if marker in self.files:
                continue
            if not self.fs.is_link(link):
                self.symlink(link, target)
            self.write(marker, b"")
        self.log.append("dsh enable %s" % unit)

    def dsh_update_state(self, unit):
        path = self.find_unit(unit)
        if not path:
            self.log.append("dsh update-state %s: unit not found" % unit)
            return
        links = ["/" + l for l, _ in self.link_closure(unit, path)]
        st = self.dsh_state(unit)
        old = self.files.get(st, b"").decode().splitlines()
        for l in old:
            if l not in links:
                self.unlink(l.lstrip("/"))
                self.files.pop(self.enabled_dir + "/" + l[len(("/etc/systemd/" + self.instance + "/")):], None)
        self.write(st, "".join(l + "\n" for l in links).encode())
        self.log.append("dsh update-state %s" % unit)

    def dsh_was_enabled(self, unit):
        path = self.find_unit(unit)
        if not path:
            return False
        links = [l for l, _ in self.link_closure(unit, path)]
        if not links:
            return False
        if all(self.fs.is_link(l) for l in links):
            return True
        return self.dsh_state(unit) not in self.files

    def dsh_debian_installed(self, unit):
        return self.dsh_state(unit) in self.files

    def dsh_mask(self, unit):
        link = ("etc/systemd/" + self.instance + "/") + unit
        if not self.fs.exists(link):
            self.symlink(link, "/dev/null")
            self.write(self.masked_dir + "/" + unit, b"")
        self.log.append("dsh mask %s" % unit)

    def dsh_unmask(self, unit):
        marker = self.masked_dir + "/" + unit
        if marker in self.files:
            self.unlink(("etc/systemd/" + self.instance + "/") + unit)
            del self.files[marker]

    # -- systemctl (offline) ---------------------------------------------
    def systemctl_enable(self, unit):
        inst = None
        m = re.match(r"^(.+)@(.+)(\.[a-z]+)$", unit)
        name = unit
        if m:
            name, inst = m.group(1) + "@" + m.group(3), m.group(2)
        for d in UNIT_DIRS:
            if self.fs.is_file(d + "/" + name):
                path = "/" + ("lib/systemd/system" if d.startswith("usr/") else d) + "/" + name
                break
        else:
            self.log.append("systemctl enable %s: unit not found" % unit)
            return
        for link, target in self.link_closure(name, path):
            if inst and link.endswith("/" + name):
                link = link[: -len(name)] + unit
            self.symlink(link, target)
        self.log.append("systemctl enable %s" % unit)

    def systemctl_mask(self, unit):
        self.symlink(("etc/systemd/" + self.instance + "/") + unit, "/dev/null")
        self.log.append("systemctl mask %s" % unit)

    # -- update-rc.d (no insserv: built-in S01/K01 fallback) ---------------
    def rc_links(self, name):
        return sorted(p for p in self.fs.links
                      if re.match(r"^etc/rc[0-6S]\.d/[SK]\d\d" + re.escape(name) + "$", p))

    def lsb(self, name):
        text = self.fs.read("etc/init.d/" + name) or ""
        start, stop = [], []
        for line in text.splitlines():
            m = re.match(r"^#\s*Default-(Start|Stop):\s*(.*)$", line)
            if m:
                (start if m.group(1) == "Start" else stop)[:] = m.group(2).split()
        return start, stop

    def update_rc(self, name, action, args):
        if action == "remove":
            for p in self.rc_links(name):
                self.unlink(p)
        elif action in ("defaults", "defaults-disabled"):
            if not self.fs.is_file("etc/init.d/" + name):
                return
            if self.rc_links(name):
                return
            start, stop = self.lsb(name)
            k = "K" if action == "defaults-disabled" else "S"
            for lvl in start:
                self.symlink("etc/rc%s.d/%s01%s" % (lvl, k, name), "../init.d/" + name)
            for lvl in stop:
                self.symlink("etc/rc%s.d/K01%s" % (lvl, name), "../init.d/" + name)
        elif action in ("enable", "disable"):
            start, _ = self.lsb(name)
            levels = [a for a in args if re.match(r"^[0-6S]$", a)] or start
            src, dst = ("K", "S") if action == "enable" else ("S", "K")
            for p in self.rc_links(name):
                lvl = p[6]
                base = p.rsplit("/", 1)[1]
                if lvl in levels and base[0] == src:
                    tgt = self.fs.links[p]
                    self.unlink(p)
                    self.symlink("etc/rc%s.d/%s01%s" % (lvl, dst, name), tgt)
        else:
            self.log.append("update-rc.d %s %s: ignored" % (name, action))
            return
        self.log.append("update-rc.d %s %s" % (name, action))


# ---------------------------------------------------------------------------
# postinst interpreter

class Unknown(Exception):
    pass


def eval_atom(atom, st):
    a = atom.strip()
    neg = False
    if a.startswith("! "):
        neg, a = True, a[2:].strip()
    v = eval_atom_raw(a, st)
    return (not v) if neg else v


def eval_atom_raw(a, st):
    s = a.replace("${DPKG_ROOT:-}", "").replace("$DPKG_ROOT", "")
    m = re.fullmatch(r'\[\s+"\$\(echo \$(\w+)\)"\s+!=\s+"\$\1"\s+\]', s)
    if m and m.group(1) in st.variables:
        pattern = norm(st.variables[m.group(1)])
        return any(fnmatch.fnmatchcase(path, pattern) for path in st.fs.files.keys() | st.fs.links.keys())
    s = re.sub(r'\$(\w+)|\$\{(\w+)\}', lambda match: st.variables.get(match[1] or match[2], match[0]), s)
    m = re.fullmatch(r'(?:\[|test)\s+(!\s+)?-(e|f|x|L|d)\s+["\']?(/[^\s"\']+)["\']?\s*\]?', s)
    if m:
        path = norm(m[3])
        kind = m[2]
        value = st.fs.exists(path) if kind == 'e' else st.fs.is_file(path) if kind == 'f' else \
            path in st.fs.links if kind == 'L' else path in st.fs.dirs or path in st.mkdirs if kind == 'd' else \
            bool(st.fs.modes.get(st.fs.resolve(path), 0) & 0o111)
        return not value if m[1] else value
    m = re.match(r'^(?:\[|test)\s+"?\$(1|action)"?\s+(!?=)\s+["\']?([\w-]+)["\']?\s*\]?$', s)
    if m:
        return (m.group(3) == "configure") == (m.group(2) == "=")
    m = re.match(r'^(?:\[|test)\s+-(z|n)\s+"(\$2|)"\s*\]?$', s)
    if m:
        return m.group(1) == "z"
    m = re.match(r'^\[\s+"\$2"\s+(!?=)\s+""\s+\]$', s)
    if m:
        return m.group(1) == "="
    if re.match(r'^\[\s+-d\s+"?/run/systemd/system"?\s+\]$', s):
        return False
    m = re.match(r'^\[\s+-(x|f|e)\s+"?(/etc/init\.d/[^"\s]+)"?\s+\]$', s)
    if m:
        return st.fs.is_file(norm(m.group(2)))
    m = re.match(r"^deb-systemd-helper\s+((?:--(?:quiet|user|system)\s+)*)was-enabled\s+'?([^'\s]+)'?", s)
    if m:
        return st.helper_call("was-enabled", m.group(2), "--user" in m.group(1).split())
    m = re.match(r"^deb-systemd-helper\s+debian-installed\s+'?([^'\s]+)'?", s)
    if m:
        return st.dsh_debian_installed(m.group(1))
    m = re.match(r'^dpkg\s+--compare-versions\s+"\$2"\s+(lt|le|lt-nl|le-nl|gt|ge|eq|gt-nl|ge-nl)\s', s)
    if m:  # empty "$2" (first install) is the earliest version, or the latest with -nl
        return m.group(1) in ("lt", "le", "gt-nl", "ge-nl")
    m = re.match(r'^\[\s+-[Le]\s+"?/etc/rc[0-6S]\.d/', s)
    if m:
        return st.fs.exists(norm(s.split()[2].strip('"')))
    raise Unknown(a)


def eval_cond(cond, st):
    cond = cond.strip()
    cond = re.sub(r";\s*then\s*$", "", cond)
    cond = re.sub(r"\s+then\s*$", "", cond)
    # left-to-right && / || (same precedence, as in sh)
    tokens = re.split(r"\s+(&&|\|\|)\s+", cond)
    val = eval_atom(tokens[0], st)
    i = 1
    while i < len(tokens):
        op, atom = tokens[i], tokens[i + 1]
        if op == "&&" and val:
            val = eval_atom(atom, st)
        elif op == "||" and not val:
            val = eval_atom(atom, st)
        i += 2
    return val


def logical_lines(text):
    buf = ""
    for line in text.splitlines():
        stripped = line.split(" #", 1)[0] if not line.lstrip().startswith("#") else ""
        if stripped.rstrip().endswith("\\"):
            buf += stripped.rstrip()[:-1] + " "
            continue
        yield (buf + stripped).strip()
        buf = ""
    if buf:
        yield buf.strip()


RELEVANT = re.compile(r"^(deb-systemd-helper\s+(?:--(?:quiet|user|system)\s+)*(enable|update-state|mask)|systemctl\s.*\b(enable|mask)\b|update-rc\.d\s|ln\s+-sf?\s+/dev/null)")


def run_postinst(pkg, text, st, unknown):
    stack = []  # [active_parent, branch_taken, current]
    st.variables = {}
    in_func = 0
    in_case = 0

    def active():
        return all(e[2] for e in stack)

    for line in logical_lines(text):
        if not line:
            continue
        if in_func < 0:  # "name()" with the "{" on the next line
            in_func = line.count("{") - line.count("}")
            continue
        if in_func:
            in_func += line.count("{") - line.count("}")
            continue
        if re.match(r"^[\w-]+\s*\(\)\s*\{?$", line) or re.match(r"^function\s+\w+", line):
            in_func = line.count("{") - line.count("}") or -1
            continue
        if re.match(r"^case\s", line):
            in_case += 1
            continue
        if line == "esac":
            in_case -= 1
            continue
        m = re.match(r"^if\s+(.*)$", line)
        if m:
            parent = active()
            val = False
            und = None
            if parent:
                try:
                    val = eval_cond(m.group(1), st)
                except Unknown as e:
                    und = str(e)
            stack.append([parent, val, val, und])
            if re.search(r";\s*then\s+.*;\s*fi$", line):
                stack.pop()  # one-line if: body ignored
            continue
        m = re.match(r"^elif\s+(.*)$", line)
        if m and stack:
            top = stack[-1]
            val = False
            if top[0] and not top[1]:
                try:
                    val = eval_cond(m.group(1), st)
                except Unknown as e:
                    top[3] = str(e)
            top[1] = top[1] or val
            top[2] = val
            continue
        if line == "else" and stack:
            top = stack[-1]
            top[2] = not top[1]
            continue
        if re.match(r"^fi\b", line) and stack:
            stack.pop()
            continue
        if any(entry[3] for entry in stack) and RELEVANT.match(line):
            raise ValueError("undecided service condition for " + pkg + ": " + line)
        if not active():
            undecided = [e[3] for e in stack if e[3]]
            if undecided and RELEVANT.match(line):
                unknown.append("%s: skipped under undecided %s: %s" % (pkg, undecided, line))
            continue
        assignment = re.fullmatch(r"(\w+)=(?:'([^']*)'|\"([^\"]*)\"|([^\s;]+))", line)
        if assignment:
            value = next(value for value in assignment.groups()[1:] if value is not None)
            st.variables[assignment[1]] = re.sub(r'\$(\w+)|\$\{(\w+)\}', lambda match: st.variables.get(match[1] or match[2], match[0]), value)
            continue
        execute(pkg, line, st, in_case)


def execute(pkg, line, st, in_case):
    line = re.sub(r"\$\{DPKG_ROOT:\+[^}]*\}", "", line)
    if RELEVANT.match(line):
        line = re.sub(r'\$(\w+)|\$\{(\w+)\}', lambda match: st.variables.get(match[1] or match[2], match[0]), line)
    cmd = re.split(r"\s*(?:>|2>|\|\||&&|;)", line)[0]
    try:
        words = shlex.split(cmd)
    except ValueError:
        return
    if not words:
        return
    if in_case and words[0] in ("deb-systemd-helper", "systemctl", "update-rc.d"):
        st.log.append("%s: in case branch, ignored: %s" % (pkg, line))
        return
    if words[0] == "deb-systemd-helper":
        args = [w for w in words[1:] if not w.startswith("--")]
        if not args:
            return
        action, units = args[0], args[1:]
        fn = {"enable": st.dsh_enable, "update-state": st.dsh_update_state,
              "mask": st.dsh_mask, "unmask": st.dsh_unmask}.get(action)
        if fn:
            for u in units:
                st.helper_call(action, u, "--user" in words)
        else:
            st.log.append("%s: ignored %s" % (pkg, line))
    elif words[0] == "systemctl":
        args = [w for w in words[1:] if not w.startswith("-")]
        if args and args[0] == "enable":
            for u in args[1:]:
                st.systemctl_enable(u)
        elif args and args[0] == "mask":
            for u in args[1:]:
                st.systemctl_mask(u)
        elif args and args[0] not in ("daemon-reload", "daemon-reexec", "is-enabled", "is-active",
                                      "start", "stop", "restart", "try-restart", "reload"):
            st.log.append("%s: ignored %s" % (pkg, line))
    elif words[0] == "update-rc.d":
        args = [w for w in words[1:] if w != "-f"]
        if len(args) >= 2:
            st.update_rc(args[0], args[1], args[2:])
    elif words[0] == "ln" and len(words) == 4 and words[1] in ("-s", "-sf", "-fs") \
            and words[2] == "/dev/null" and words[3].startswith("/") and "$" not in words[3]:
        # Hand-written mask, e.g. ntpsec: ln -sf /dev/null /etc/systemd/system/ntpsec.timer
        p = norm(words[3])
        if words[1] != "-s" and p in st.fs.links:
            st.unlink(p)
        st.symlink(p, "/dev/null")
        st.log.append("%s: mask link %s" % (pkg, p))


def control_info(path):
    with tarfile.open(path, "r:*") as tf:
        names = {norm(m.name): m for m in tf if m.isfile()}
        ctl = tf.extractfile(names["control"]).read().decode()
        pkg = re.search(r"^Package:\s*(\S+)", ctl, re.M).group(1)
        post = names.get("postinst")
        text = tf.extractfile(post).read().decode("utf-8", "replace") if post else ""
    return pkg, text


def apply_extra(pkg, extra, st):
    for e in extra.get(pkg, []):
        p = e["path"]
        if e["type"] == "symlink":
            if not st.fs.exists(p):
                st.symlink(p, e["target"])
        elif e["type"] == "file":
            st.write(p, e["content"].encode())
        st.log.append("%s: extra %s %s" % (pkg, e["type"], p))


def write_tar(out, st, mtime):
    entries = {}
    for d in st.mkdirs:
        if d.startswith(("etc/systemd/system/", "etc/systemd/user/", "etc/rc", DSH_ENABLED, DSH_MASKED, "var/lib/systemd/deb-systemd-user-helper-")) and not st.fs.exists(d):
            entries[d] = ("dir", None)
    for p, t in st.created_links.items():
        entries[p] = ("sym", t)
    for p, data in st.files.items():
        entries[p] = ("file", data)
    with tarfile.open(out, "w", format=tarfile.GNU_FORMAT) as tf:
        for p in sorted(entries):
            kind, val = entries[p]
            ti = tarfile.TarInfo(p)
            ti.uid = ti.gid = 0
            ti.uname = ti.gname = "root"
            ti.mtime = mtime
            if kind == "dir":
                ti.type, ti.mode = tarfile.DIRTYPE, 0o755
                tf.addfile(ti)
            elif kind == "sym":
                ti.type, ti.mode, ti.linkname = tarfile.SYMTYPE, 0o777, val
                tf.addfile(ti)
            else:
                ti.mode, ti.size = 0o644, len(val)
                tf.addfile(ti, io.BytesIO(val))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", required=True)
    ap.add_argument("--payload", action="append", default=[], help="rootfs payload tar (layer or deb data)")
    ap.add_argument("--control", action="append", default=[], help="deb control tar, install order")
    ap.add_argument("--extra", help="JSON {package: [effect]} for hand-written postinst code")
    ap.add_argument("--log")
    ap.add_argument("--mtime", type=int, default=0)
    a = ap.parse_args()

    fs = Fs()
    for p in a.payload:
        fs.load(p)
    # Links shipped by the payloads are the starting state; directories implied.
    for p in list(fs.files) + list(fs.links):
        parts = p.split("/")
        for i in range(1, len(parts)):
            fs.dirs.add("/".join(parts[:i]))
    st = State(fs)
    extra = json.load(open(a.extra)) if a.extra else {}
    unknown = []
    seen = set()
    for c in a.control:
        pkg, text = control_info(c)
        if pkg in seen:
            continue
        seen.add(pkg)
        if text:
            run_postinst(pkg, text, st, unknown)
        apply_extra(pkg, extra, st)
    # A unit marker file implies its directory exists in the state tree.
    for p in st.files:
        st.make_path(p.rsplit("/", 1)[0])
    if unknown:
        raise ValueError("undecided service effects: " + "; ".join(unknown))
    write_tar(a.out, st, a.mtime)
    if a.log:
        with open(a.log, "w") as f:
            f.write("\n".join(st.log) + "\n")
            f.write("".join("UNDECIDED %s\n" % u for u in sorted(set(unknown))))


if __name__ == "__main__":
    main()
