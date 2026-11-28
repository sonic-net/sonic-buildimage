"""Maintainer-script effects: CA certificates, alternatives, ucf, debconf, misc.

Replays, against the assembled package image, what the configure-time
maintainer scripts of the host packages leave in the rootfs (they were not run
by the package layers):

* ca-certificates postinst + update-ca-certificates --fresh:
  /etc/ca-certificates.conf, /etc/ssl/certs/{*.pem, <hash>.N, ca-certificates.crt}
  (subject hashes = OpenSSL X509_NAME_hash, re-implemented below).
* update-alternatives --install (alternatives_data.py): /etc/alternatives/*,
  /var/lib/dpkg/alternatives/*, master/slave links.
* ucf/ucfr (sysstat, openssh-server, chrony, kdump-tools): the installed
  config files, /var/lib/ucf/{hashfile,registry}[.0-4], /var/lib/ucf/cache/*.
* debconf: /var/cache/debconf/{config,templates,passwords}.dat[-old] from the
  installed templates (/var/lib/dpkg/info/*.templates) and debconf_answers.json.
* update-shells, dh_apparmor local includes, fontconfig-config conf.d links,
  shadow's /etc/.pwd.lock, libpam-modules' opasswd, python3.13's marker,
  xfonts-utils' excluded-aliases, kexec-tools' /etc/default/kexec.

Output: one deterministic tar (sorted, mtime 0 unless the effect copies a
packaged file with cp -p, numeric owners).
"""
import base64
import hashlib
import io
import json
import re
import sys
import subprocess
import tarfile
from pathlib import Path

from alternatives_data import CALLS
from authentication import configure_authentication, pam_selection
from contextlib import ExitStack
from merge_layers import Dir, Tree, apply_layer
from tar_filesystem import TarFilesystem

TOP = {"bin": "usr/bin", "sbin": "usr/sbin", "lib": "usr/lib", "lib64": "usr/lib64"}


def norm(path):
    p = path.lstrip("/")
    head, _, rest = p.partition("/")
    if head in TOP:
        return TOP[head] + ("/" + rest if rest else "")
    return p


class Image:
    def __init__(self, spec):
        self.stack = ExitStack()
        self.tree = Tree()
        for path in spec["layers"]:
            archive = self.stack.enter_context(tarfile.open(path, "r:*"))
            apply_layer(self.tree, archive, path)
        self.fs = TarFilesystem(self.tree)
        self.arch = spec["architecture"]
        self.pam_removed = spec["pam_removed"]
        self.openssl = spec["openssl"]
        self.openssl_config = spec["openssl_config"]
        self.files = {}
        def walk(directory, prefix=""):
            for name, node in directory.children.items():
                path = prefix + name
                if isinstance(node, Dir):
                    walk(node, path + "/")
                elif path.startswith(("usr/share/ca-certificates/", "usr/share/debianutils/", "usr/share/pam-configs/", "var/lib/dpkg/info/", "etc/")):
                    member = self._member(node, path)
                    content = self.fs.read(path) if member.isfile() else None
                    self.files[path] = member, content
        walk(self.tree.root)
        for package, path in spec.get("controls", {}).items():
            archive = self.stack.enter_context(tarfile.open(path, "r:*"))
            control = archive.extractfile(next(member for member in archive.getmembers() if member.name.removeprefix("./") == "control")).read().decode()
            fields = dict(line.split(":", 1) for line in control.splitlines() if line and not line[0].isspace() and ":" in line)
            fields = {name: value.strip() for name, value in fields.items()}
            if fields["Package"] != package.split(":")[0]:
                raise ValueError("package control identity differs: " + package)
            if fields.get("Multi-Arch") == "same":
                package = fields["Package"] + ":" + fields["Architecture"]
            for member in archive:
                if member.isfile():
                    name = "var/lib/dpkg/info/" + package + "." + member.name.removeprefix("./")
                    self.files[name] = member, archive.extractfile(member).read()

    @staticmethod
    def _member(node, path):
        member = tarfile.TarInfo(path)
        member.mode, member.uid, member.gid, member.mtime = node.meta.mode, node.meta.uid, node.meta.gid, node.meta.mtime
        member.type = node.type
        member.linkname = node.linkname
        return member

    def entry(self, path):
        key = norm(path)
        if key in self.files and self.files[key][0].isfile():
            return self.files[key]
        try:
            node = self.fs.node(path)
        except FileNotFoundError:
            return None
        if isinstance(node, Dir):
            return None
        return self._member(node, path), self.fs.read(path) if node.src is not None else None

    def exists(self, path):
        if norm(path) in self.files:
            return True
        try:
            self.fs.node(path)
            return True
        except FileNotFoundError:
            return False

    def read(self, path):
        entry = self.entry(path)
        if entry is None or entry[1] is None:
            raise KeyError("missing image file content: " + path)
        return entry[1]

    def member(self, path):
        entry = self.entry(path)
        if entry is None:
            raise KeyError("missing image file: " + path)
        return entry[0]


class Out:
    def __init__(self, image):
        self.image = image
        self.items = {}

    def _dirs(self, path):
        parts = path.split("/")[:-1]
        for i in range(1, len(parts) + 1):
            d = "/".join(parts[:i])
            if d not in self.items and not self.image.exists(d):
                self.items[d] = dict(type=tarfile.DIRTYPE, mode=0o755, uid=0, gid=0, mtime=0)

    def file(self, path, data, mode=0o644, uid=0, gid=0, mtime=0):
        p = norm(path)
        self._dirs(p)
        self.items[p] = dict(type=tarfile.REGTYPE, data=data, mode=mode, uid=uid, gid=gid, mtime=mtime)

    def link(self, path, target):
        p = norm(path)
        self._dirs(p)
        self.items[p] = dict(type=tarfile.SYMTYPE, linkname=target, mode=0o777, uid=0, gid=0, mtime=0)

    def write(self, out):
        with tarfile.open(out, "w", format=tarfile.PAX_FORMAT, encoding="utf-8") as tar:
            for p in sorted(self.items):
                it = self.items[p]
                ti = tarfile.TarInfo(p)
                ti.type = it["type"]
                ti.mode = it["mode"]
                ti.uid, ti.gid = it["uid"], it["gid"]
                ti.uname = ti.gname = ""
                ti.mtime = it["mtime"]
                if it["type"] == tarfile.SYMTYPE:
                    ti.linkname = it["linkname"]
                    tar.addfile(ti)
                elif it["type"] == tarfile.REGTYPE:
                    ti.size = len(it["data"])
                    tar.addfile(ti, io.BytesIO(it["data"]))
                else:
                    tar.addfile(ti)


def heredoc(script, start):
    """Body of the first here-document whose command line matches `start`."""
    lines = script.split("\n")
    for i, line in enumerate(lines):
        if re.search(start, line):
            term = re.search(r"<<\s*-?\s*['\"]?(\w+)", line).group(1)
            body = []
            for l in lines[i + 1:]:
                if l.strip() == term:
                    return "\n".join(body) + "\n"
                body.append(l)
    raise ValueError("here-document not found: " + start)


# --------------------------------------------------------------------------
# OpenSSL X509_NAME_hash (canonical encoding, crypto/x509/x_name.c)

def subject_hash(cert_der, executable, config):
    result = subprocess.run([executable, "x509", "-inform", "DER", "-subject_hash", "-noout"],
                            input=cert_der, capture_output=True, check=True,
                            env={"PATH": "", "OPENSSL_CONF": config})
    value = result.stdout.decode().strip()
    if not re.fullmatch("[0-9a-f]{8}", value):
        raise ValueError("OpenSSL emitted an invalid certificate subject hash")
    return value


def pem_certs(data):
    return [base64.b64decode(b"".join(m.group(1).split()))
            for m in re.finditer(rb"-----BEGIN CERTIFICATE-----(.*?)-----END CERTIFICATE-----", data, re.S)]


def ca_certificates(img, out, report):
    script = img.read("/var/lib/dpkg/info/ca-certificates.postinst").decode()
    header = heredoc(script, r"cat > /etc/ca-certificates.conf <<")
    base = "usr/share/ca-certificates/"
    available = sorted((p[len(base):] for p, (m, _) in img.files.items()
                        if p.startswith(base) and p.endswith(".crt") and m.isfile()),
                       key=lambda s: s.encode())
    # trust_new_crts=yes on a fresh install: every available certificate enabled.
    out.file("/etc/ca-certificates.conf", (header + "".join(c + "\n" for c in available)).encode())
    bundle = b""
    pems = {}
    for crt in available:
        name = re.sub(r",", "_", re.sub(r"[()]", "=", crt.rsplit("/", 1)[-1][:-len(".crt")].replace(" ", "_"))) + ".pem"
        out.link("/etc/ssl/certs/" + name, "/" + base + crt)
        data = img.read("/" + base + crt)
        pems[name] = data
        bundle += data if data.endswith(b"\n") or not data else data + b"\n"
    out.file("/etc/ssl/certs/ca-certificates.crt", bundle)
    # openssl rehash: sorted file names, one link per distinct certificate.
    buckets = {}
    for name in sorted(pems, key=lambda s: s.encode()):
        certs = pem_certs(pems[name])
        if len(certs) != 1:
            raise ValueError("expected one certificate in " + name)
        h = subject_hash(certs[0], img.openssl, img.openssl_config)
        fp = hashlib.sha1(certs[0]).digest()
        bucket = buckets.setdefault(h, [])
        if fp in [f for f, _ in bucket]:
            continue
        bucket.append((fp, name))
        out.link("/etc/ssl/certs/%s.%d" % (h, len(bucket) - 1), name)
    report["ca_certificates"] = {"certs": len(available), "hash_links": sum(len(b) for b in buckets.values())}
    return available


# --------------------------------------------------------------------------
# update-alternatives

def alternatives(img, out, report):
    groups = {}
    for pkg in sorted(CALLS):
        for _, link, name, path, prio, slaves in CALLS[pkg]:
            if not img.exists(path):
                continue
            g = groups.setdefault(name, {"link": link, "slaves": {}, "choices": {}})
            for slink, sname, _ in slaves:
                g["slaves"].setdefault(sname, slink)
            g["choices"][path] = (prio, {sname: spath for _, sname, spath in slaves})
    for name, g in sorted(groups.items()):
        best = sorted(g["choices"].items(), key=lambda kv: (-kv[1][0], kv[0]))[0]
        out.link("/etc/alternatives/" + name, best[0])
        out.link(g["link"], "/etc/alternatives/" + name)
        for sname, slink in g["slaves"].items():
            spath = best[1][1].get(sname)
            if spath and img.exists(spath):
                out.link("/etc/alternatives/" + sname, spath)
                out.link(slink, "/etc/alternatives/" + sname)
        snames = sorted(g["slaves"], key=lambda s: s.encode())
        state = "auto\n%s\n" % g["link"]
        for s in snames:
            state += "%s\n%s\n" % (s, g["slaves"][s])
        state += "\n"
        for path in sorted(g["choices"], key=lambda s: s.encode()):
            prio, smap = g["choices"][path]
            state += "%s\n%d\n" % (path, prio)
            for s in snames:
                state += smap.get(s, "") + "\n"
        state += "\n"
        out.file("/var/lib/dpkg/alternatives/" + name, state.encode())
    report["alternatives"] = sorted(groups)


# --------------------------------------------------------------------------
# ucf / ucfr

def ucf(img, out, answers, report):
    def src(path):
        m = img.member(path)
        return img.read(path), m.mode & 0o7777, m.mtime

    calls = []  # (package, destination, content, dest mode, dest gid, cache mode, cache mtime)
    if img.exists("/var/lib/dpkg/info/sysstat.postinst"):
        sysstat = img.read("/var/lib/dpkg/info/sysstat.postinst").decode()
        enabled = answers["sysstat/enable"]
        body = heredoc(sysstat, r'cat > "\$def_file" <<').replace("$ENABLED", enabled).encode()
        calls.append(("sysstat", "/etc/default/sysstat", body, 0o644, 0, 0o600, 0))
    if img.exists("/var/lib/dpkg/info/openssh-server.postinst"):
        data, mode, mtime = src("/usr/share/openssh/sshd_config")
        if answers["openssh-server/permit-root-login"] != "true":
            data = re.sub(rb"(?m)^#*PermitRootLogin .*", b"PermitRootLogin yes", data)
        if answers["openssh-server/password-authentication"] != "true":
            data = re.sub(rb"(?m)^#PasswordAuthentication .*", b"PasswordAuthentication no", data)
        calls.append(("openssh-server", "/etc/ssh/sshd_config", data, mode, 0, mode, mtime))
    if img.exists("/var/lib/dpkg/info/chrony.postinst"):
        for f in ["chrony.conf", "chrony.keys"]:
            data, mode, mtime = src("/usr/share/chrony/" + f)
            # chrony.keys must be readable by chronyd (group _chrony, gid fixed by
            # the image's account layout: build_debian/pi-accounts users).
            gid = int(next(line.split(":")[2] for line in img.read("/etc/group").decode().splitlines() if line.startswith("_chrony:"))) if f == "chrony.keys" else 0
            calls.append(("chrony", "/etc/chrony/" + f, data, mode, gid, mode, mtime))
    # kdump-tools is in the image only where the extension installs it (grub targets).
    if img.exists("/var/lib/dpkg/info/kdump-tools.postinst"):
        data, mode, mtime = src("/usr/share/kdump-tools/kdump-tools.conf")
        new = "1" if answers["kdump-tools/use_kdump"] == "true" else "0"
        if not re.search(rb"(?m)^ *USE_KDUMP=" + new.encode() + rb"$", data):
            if not re.search(rb"(?m)^ *USE_KDUMP=", data):
                data += b"USE_KDUMP=\n"
            data = re.sub(rb"(?m)^ *USE_KDUMP=.*", b"USE_KDUMP=" + new.encode(), data)
        calls.append(("kdump-tools", "/etc/default/kdump-tools", data, mode, 0, mode, mtime))

    hashes, registry = [], []
    hist_h, hist_r = [""], [""]  # ucf postinst: touch hashfile registry
    for pkg, dest, data, mode, gid, cmode, cmtime in calls:
        out.file(dest, data, mode=mode, gid=gid)
        out.file("/var/lib/ucf/cache/" + dest.replace("/", ":"), data, mode=cmode, mtime=cmtime)
        hashes.append("%s  %s\n" % (hashlib.md5(data).hexdigest(), dest))
        registry.append("%s \t %s\n" % (pkg, dest))
        hist_h.append("".join(hashes))
        hist_r.append("".join(registry))
    # Each ucf/ucfr run rotates its database (savelog: .0 newest ... .4 oldest).
    for base, hist in (("hashfile", hist_h), ("registry", hist_r)):
        out.file("/var/lib/ucf/" + base, hist[-1].encode())
        for i, content in enumerate(reversed(hist[:-1][-5:])):
            out.file("/var/lib/ucf/%s.%d" % (base, i), content.encode())
    report["ucf"] = [c[1] for c in calls]


# --------------------------------------------------------------------------
# debconf

def parse_templates(text):
    """debconf Template->load: fields lower-cased; Description split."""
    out = []
    for stanza in re.split(r"\n\s*\n", text.strip("\n")):
        fields = {}
        key = None
        ext = {}
        for line in stanza.split("\n"):
            if line.startswith("#") or not line:
                continue
            if line[0] in " \t":
                bit = line[1:]
                if key is None:
                    raise ValueError("continuation without field")
                cur = ext.get(key, "")
                if bit.strip() == ".":
                    cur += "\n\n"
                elif bit[:1].isspace():
                    cur = cur + ("\n" if cur and not cur.endswith("\n") else "") + bit.rstrip() + "\n"
                else:
                    cur = cur + (" " if cur and not cur.endswith(("\n", " ")) else "") + bit.strip()
                ext[key] = cur
                continue
            k, _, v = line.partition(":")
            key = k.strip().lower()
            fields[key] = v.strip()
        for k, v in ext.items():
            v = re.sub(r"\n+$", "", v)
            if k.startswith("description"):
                fields["extended_" + k] = v
            else:
                fields[k] = fields[k] + ("\n" if fields[k] else "") + v if fields.get(k) else v
        out.append(fields)
    return out


def esc(v):
    # debconf's File/822 driver escapes only newlines; backslashes are kept verbatim.
    return v.replace("\n", "\\n")


def debconf(img, out, spec, ca_enabled, report):
    templates = {}  # name -> fields
    owners = {}  # question -> set(packages)
    info = "var/lib/dpkg/info/"
    for p in sorted(img.files):
        if p.startswith(info) and p.endswith(".templates"):
            pkg = p[len(info):-len(".templates")]
            for t in parse_templates(img.files[p][1].decode("utf-8")):
                name = t.pop("template")
                templates[name] = t
                owners.setdefault(name, set()).add(pkg)
    # Owner names as registered by apt's dpkg-preconfigure (may lack the arch
    # qualifier) and/or by the postinst's own db_* calls.
    owner_map = {k.replace("@ARCH", img.arch): [x.replace("@ARCH", img.arch) for x in v]
                 for k, v in spec["owners"].items()}
    for q in owners:
        new = set()
        for pkg in owners[q]:
            new.update(owner_map.get(pkg, [pkg]))
        owners[q] = new

    def computed(kind):
        if kind.startswith("@computed:pam_"):
            profiles, seen, enabled = pam_selection(img, img.pam_removed)
            if kind == "@computed:pam_enabled":
                return ", ".join(enabled)
            if kind == "@computed:pam_names":
                return ", ".join(seen)
            if kind == "@computed:pam_descriptions":
                return ", ".join(profiles[name]["Name"] for name in seen)
            raise ValueError("unknown PAM answer: " + kind)
        if kind == "@computed:ca_enabled":
            return ", ".join(ca_enabled)
        if kind == "@computed:locales_supported":
            # locales.config: PROVIDED_LOCALES literal | sort -u (LC_ALL=C), joined ", ".
            cfg = img.read("/var/lib/dpkg/info/locales.config").decode()
            lit = re.search(r'^PROVIDED_LOCALES="([^"]*)"', cfg, re.M).group(1)
            lines = sorted({l.rstrip() for l in lit.split("\n") if l.strip()}, key=lambda s: s.encode())
            return ", ".join(lines)
        return kind

    def item(name, fields, owner_list, flags=(), variables=None):
        s = "Name: %s\n" % name
        for k in sorted(fields, key=lambda s: s.encode()):
            s += "%s: %s\n" % (k[0].upper() + k[1:], esc(fields[k]))
        if owner_list:
            s += "Owners: %s\n" % ", ".join(sorted(owner_list, key=lambda s: s.encode()))
        if flags:
            s += "Flags: %s\n" % ", ".join(sorted(flags))
        if variables:
            s += "Variables:\n"
            for k in sorted(variables, key=lambda s: s.encode()):
                s += " %s = %s\n" % (k, esc(variables[k]))
        return s + "\n"

    tdat = "".join(item(n, templates[n], [n]) for n in sorted(templates, key=lambda s: s.encode()))
    cdat, pdat = "", ""
    answers = {}
    for n in sorted(templates, key=lambda s: s.encode()):
        a = spec["questions"].get(n, {})
        fields = {"template": n}
        if "value" in a:
            v = a["value"]
            v = templates[n].get("default", "") if v == "default" else computed(v)
            fields["value"] = v
            answers[n] = v
        variables = {k: computed(v) for k, v in a.get("variables", {}).items()} or None
        rec = item(n, fields, owners[n], a.get("flags", ()), variables)
        if templates[n].get("type") == "password":
            pdat += rec
        else:
            cdat += rec
    for name, data, mode in (("config.dat", cdat, 0o644), ("templates.dat", tdat, 0o644), ("passwords.dat", pdat, 0o600)):
        out.file("/var/cache/debconf/" + name, data.encode(), mode=mode)
        if name != "passwords.dat":
            # The last two saves of the build were identical (no change after the final package).
            out.file("/var/cache/debconf/%s-old" % name, data.encode(), mode=mode)
    report["debconf"] = {"templates": len(templates), "unknown_answers": sorted(set(spec["questions"]) - set(templates))}
    for n, a in spec["questions"].items():
        if n not in answers and n in templates and "value" in a:
            answers[n] = a["value"]
    return answers


# --------------------------------------------------------------------------
# misc small effects

def misc(img, out, answers, report):
    # Source initramfs-tools-core postinst creates this before Make appends VS modules.
    if img.exists("/var/lib/dpkg/info/initramfs-tools-core.postinst") and not img.exists("/etc/initramfs-tools/modules"):
        modules = img.read("/usr/share/initramfs-tools/modules")
        out.file("/etc/initramfs-tools/modules", b"".join(line for line in modules.splitlines(keepends=True) if not line.startswith(b"###")))
        report["initramfs_modules_fresh"] = True
    # debianutils postinst: update-shells (fresh: /etc/shells from the template).
    pkg_shells = []
    srcs = ["/usr/share/debianutils/shells"] + sorted(
        "/" + p for p in img.files if p.startswith("usr/share/debianutils/shells.d/"))
    for f in srcs:
        for line in img.read(f).decode().splitlines():
            line = line.split("#", 1)[0]
            if not line:
                continue
            pkg_shells.append(line)
            d, b = line.rsplit("/", 1)
            real = "/" + norm(d) if norm(d) != d.lstrip("/") else d
            if real + "/" + b != line:
                pkg_shells.append(real + "/" + b)
    etc, seen = [], set()
    for line in img.read("/usr/share/debianutils/shells").decode().splitlines():
        shell = line.split("#", 1)[0]
        if not shell or shell not in seen:
            etc.append(line)
            seen.add(shell)
    for s in pkg_shells:
        if s not in seen:
            etc.append(s)
            seen.add(s)
    out.file("/etc/shells", ("\n".join(etc) + "\n").encode())
    out.file("/var/lib/shells.state", "".join(s + "\n" for s in pkg_shells).encode())
    # shadow tools (useradd/groupadd via adduser) leave the lckpwdf lock file.
    out.file("/etc/.pwd.lock", b"", mode=0o600)
    # libpam-modules postinst: touch /etc/security/opasswd; chmod 600.
    if img.exists("/var/lib/dpkg/info/libpam-modules:%s.postinst" % img.arch):
        out.file("/etc/security/opasswd", b"", mode=0o600)
    # python3.13 postinst: marker of a configured interpreter.
    if img.exists("/var/lib/dpkg/info/python3.13.postinst"):
        out.file("/var/lib/python/python3.13_installed", b"")
    # xfonts-utils postinst.
    if img.exists("/var/lib/dpkg/info/xfonts-utils.postinst"):
        script = img.read("/var/lib/dpkg/info/xfonts-utils.postinst").decode()
        line = re.search(r"echo '([^']*)' > \$EXCLUDE", script).group(1)
        out.file("/var/lib/xfonts/excluded-aliases", (line + "\n").encode())
    # dh_apparmor: empty local include per profile when absent.
    local = []
    for p in sorted(img.files):
        if p.startswith("var/lib/dpkg/info/") and p.endswith(".postinst"):
            script = img.files[p][1].decode("utf-8", "replace")
            pairs = re.findall(r'(?<!LOCAL_)APP_PROFILE="([^"]+)"\s*\n\s*if \[ -f "\$APP_PROFILE" \]; then.*?LOCAL_APP_PROFILE="([^"]+)"', script, re.S)
            for prof, target in pairs:
                if img.exists(prof) and not img.exists(target):
                    out.file(target, b"")
                    local.append(target)
    # fontconfig-config postinst (debconf hinting/bitmaps answers).
    if img.exists("/var/lib/dpkg/info/fontconfig-config.postinst"):
        avail = "/usr/share/fontconfig/conf.avail/"
        style = {"hintnone": "10-hinting-none.conf", "hintslight": "10-hinting-slight.conf",
                 "hintmedium": "10-hinting-medium.conf", "hintfull": "10-hinting-full.conf"}
        f = style[answers["fontconfig/hinting_style"]]
        out.link("/etc/fonts/conf.d/" + f, avail + f)
        if answers["fontconfig/enable_bitmaps"] != "true":
            out.link("/etc/fonts/conf.d/70-no-bitmaps.conf", avail + "70-no-bitmaps.conf")
    # ifupdown2 fresh configure: only create the default when no interfaces exists.
    if img.exists("/var/lib/dpkg/info/ifupdown2.postinst"):
        if not img.exists("/etc/network/interfaces"):
            out.file("/etc/network/interfaces", b"# interfaces(5) file used by ifup(8) and ifdown(8)\nauto lo\niface lo inet loopback\n")
        if img.exists("/etc/udev/rules.d"):
            for rule in ("80-networking.rules", "60-bridge-network-interface.rules"):
                destination = "/etc/udev/rules.d/" + rule
                if img.exists("/lib/udev/rules.d/" + rule) and not img.exists(destination):
                    out.link(destination, "/dev/null")
    # Monit postinst creates a root:adm log only on first configuration.
    if img.exists("/var/lib/dpkg/info/monit.postinst"):
        if not img.exists("/var/log/monit.log"):
            adm_gid = int(next(line.split(":")[2] for line in img.read("/etc/group").decode().splitlines() if line.startswith("adm:")))
            out.file("/var/log/monit.log", b"", mode=0o640, gid=adm_gid)
        if img.exists("/etc/aliases"):
            aliases = img.read("/etc/aliases")
            if not re.search(rb"(?im)^monit[ \t]*:", aliases):
                out.file("/etc/aliases", aliases + b"monit: root\n", mode=img.member("/etc/aliases").mode)
    # bash-tacplus postinst binds the single installed source-owned plugin.
    if img.exists("/var/lib/dpkg/info/bash-tacplus.postinst"):
        plugins = img.fs.glob("usr/lib/*/security/bash_tacplus.so") + img.fs.glob("usr/lib/security/bash_tacplus.so")
        if len(plugins) != 1:
            raise ValueError("expected one installed bash_tacplus plugin: " + str(plugins))
        previous = img.read("/etc/bash_plugins.conf") if img.exists("/etc/bash_plugins.conf") else b""
        retained = b"".join(line for line in previous.splitlines(keepends=True) if not re.search(rb"plugin=.*bash_tacplus\.so", line))
        out.file("/etc/bash_plugins.conf", retained + ("plugin=" + plugins[0] + "\n").encode())
    # screen.postinst creates the tmpfiles override from the installed ELF mode.
    if img.exists("/var/lib/dpkg/info/screen.postinst"):
        mode = img.member("/usr/bin/screen").mode & 0o7777
        run_mode = {0o4755: 0o755, 0o755: 0o1777}.get(mode, 0o775)
        utmp_gid = int(next(line.split(":")[2] for line in img.read("/etc/group").decode().splitlines() if line.startswith("utmp:")))
        out.items["run/screen"] = dict(type=tarfile.DIRTYPE, mode=run_mode, uid=0, gid=utmp_gid, mtime=0)
        override = "/etc/tmpfiles.d/screen-cleanup.conf"
        if mode in (0o4755, 0o755) and not img.exists(override):
            out.file(override, ("# This file is generated by /var/lib/dpkg/info/screen.postinst upon package configuration\n"
                                "d /run/screen %04o root utmp\n" % run_mode).encode())
    report["misc"] = {"shells": pkg_shells, "apparmor_local": local}
    # kexec-tools postinst: generate /etc/default/kexec from its here-document.
    # No /etc/kernel-img.conf, so the link_in_boot seds are no-ops; then
    # update_param rewrites KEY= lines whose value differs from the debconf
    # answer. Trixie removed LOAD_KEXEC; preserve the installed template.
    if img.exists("/var/lib/dpkg/info/kexec-tools.postinst") and not img.exists("/etc/default/kexec"):
        if img.exists("/etc/kernel-img.conf"):
            raise ValueError("kexec-tools: /etc/kernel-img.conf present; link_in_boot edit not modelled")
        script = img.read("/var/lib/dpkg/info/kexec-tools.postinst").decode()
        body = heredoc(script, r"cat >\$\{INITCONFFILE\} <<")
        want = (("USE_GRUB_CONFIG", "false"),)  # no /boot/grub at configure time
        for k, v in want:
            if not re.search(r"^ *%s=" % k, body, re.M):
                raise ValueError("kexec-tools: %s missing from template (not modelled)" % k)
            body = re.sub(r"^ *%s=.*$" % k, lambda m: "%s=%s" % (k, v), body, flags=re.M)
        out.file("/etc/default/kexec", body.encode())
        report["misc"]["kexec"] = True


def main():
    spec_path, answers_path, out_path, report_path = sys.argv[1:]
    spec = json.load(open(spec_path))
    img = Image(spec)
    out = Out(img)
    report = {"architecture": img.arch}
    ca = ca_certificates(img, out, report)
    alternatives(img, out, report)
    answers = debconf(img, out, json.load(open(answers_path)), ca, report)
    ucf(img, out, answers, report)
    misc(img, out, answers, report)
    configure_authentication(img, out, spec["pam_removed"], report)
    out.write(out_path)
    if spec.get("ca_bundle"):
        Path(spec["ca_bundle"]).write_bytes(out.items["etc/ssl/certs/ca-certificates.crt"]["data"])
    img.stack.close()
    report["entries"] = len(out.items)
    with open(report_path, "w") as f:
        json.dump(report, f, indent=1, sort_keys=True)


if __name__ == "__main__":
    main()
