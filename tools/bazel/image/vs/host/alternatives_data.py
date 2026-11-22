"""update-alternatives --install calls of the host maintainer scripts.

Transcribed from the configure-time postinst of each package (dpkg configure
order of the Make image); the generator replays them like dpkg's
update-alternatives. A call whose master path is absent from the image is
skipped (dpkg would refuse it; the postinst guards make it unreachable).
Entries: (package, link, name, path, priority, [(slave_link, slave_name, slave_path), ...]).
"""

_MAN_LANGS = ["da", "de", "fr", "it", "ja", "pl", "ru", "tr"]


def _vim(name, man):
    slaves = []
    if man:
        slaves.append(("/usr/share/man/man1/%s.1.gz" % name, "%s.1.gz" % name, "/usr/share/man/man1/vim.1.gz"))
        for lang in _MAN_LANGS:
            slaves.append(("/usr/share/man/%s/man1/%s.1.gz" % (lang, name), "%s.%s.1.gz" % (name, lang),
                           "/usr/share/man/%s/man1/vim.1.gz" % lang))
    return ("vim", "/usr/bin/" + name, name, "/usr/bin/vim.basic", 30, slaves)


def _xz():
    slaves = [("/usr/share/man/man1/lzma.1.gz", "lzma.1.gz", "/usr/share/man/man1/xz.1.gz"),
              ("/usr/bin/unlzma", "unlzma", "/usr/bin/unxz"),
              ("/usr/share/man/man1/unlzma.1.gz", "unlzma.1.gz", "/usr/share/man/man1/unxz.1.gz")]
    for util in ["cat", "more", "less", "diff", "cmp", "grep", "egrep", "fgrep"]:
        slaves.append(("/usr/bin/lz" + util, "lz" + util, "/usr/bin/xz" + util))
        slaves.append(("/usr/share/man/man1/lz%s.1.gz" % util, "lz%s.1.gz" % util, "/usr/share/man/man1/xz%s.1.gz" % util))
    return ("xz-utils", "/usr/bin/lzma", "lzma", "/usr/bin/xz", 20, slaves)


def _xtables(pkg, name, variant, prio):
    return (pkg, "/usr/sbin/" + name, name, "/usr/sbin/%s-%s" % (name, variant), prio, [
        ("/usr/sbin/%s-restore" % name, name + "-restore", "/usr/sbin/%s-%s-restore" % (name, variant)),
        ("/usr/sbin/%s-save" % name, name + "-save", "/usr/sbin/%s-%s-save" % (name, variant)),
    ])


def _traceroute():
    calls = [("traceroute", "/usr/bin/traceroute", "traceroute", "/usr/bin/traceroute.db", 100, [
        ("/usr/sbin/traceroute", "traceroute.sbin", "/usr/bin/traceroute.db"),
        ("/usr/share/man/man1/traceroute.1.gz", "traceroute.1.gz", "/usr/share/man/man1/traceroute.db.1.gz"),
    ])]
    for alt in ["traceroute6", "lft", "traceproto"]:
        calls.append(("traceroute", "/usr/bin/" + alt, alt, "/usr/bin/%s.db" % alt, 100, [
            ("/usr/share/man/man1/%s.1.gz" % alt, alt + ".1.gz", "/usr/share/man/man1/%s.db.1.gz" % alt)]))
    calls.append(("traceroute", "/usr/sbin/tcptraceroute", "tcptraceroute", "/usr/sbin/tcptraceroute.db", 100, [
        ("/usr/share/man/man8/tcptraceroute.8.gz", "tcptraceroute.8.gz", "/usr/share/man/man8/tcptraceroute.db.8.gz")]))
    return calls


_WHICH_LANGS = ["de", "es", "fr", "it", "ja", "pl", "sl"]

# Package -> its update-alternatives --install calls, in script order.
CALLS = {
    "mawk": [("mawk", "/usr/bin/awk", "awk", "/usr/bin/mawk", 5, [
        ("/usr/share/man/man1/awk.1.gz", "awk.1.gz", "/usr/share/man/man1/mawk.1.gz"),
        ("/usr/bin/nawk", "nawk", "/usr/bin/mawk"),
        ("/usr/share/man/man1/nawk.1.gz", "nawk.1.gz", "/usr/share/man/man1/mawk.1.gz")])],
    "debianutils": [("debianutils", "/usr/bin/which", "which", "/usr/bin/which.debianutils", 0,
                     [("/usr/share/man/man1/which.1.gz", "which.1.gz", "/usr/share/man/man1/which.debianutils.1.gz")] +
                     [("/usr/share/man/%s/man1/which.1.gz" % l, "which.%s1.gz" % l,
                       "/usr/share/man/%s/man1/which.debianutils.1.gz" % l) for l in _WHICH_LANGS])],
    "bash": [("bash", "/usr/share/man/man7/builtins.7.gz", "builtins.7.gz", "/usr/share/man/man7/bash-builtins.7.gz", 10, [])],
    "util-linux": [("util-linux", "/usr/bin/pager", "pager", "/bin/more", 50, [
        ("/usr/share/man/man1/pager.1.gz", "pager.1.gz", "/usr/share/man/man1/more.1.gz")])],
    "tar": [("tar", "/usr/sbin/rmt", "rmt", "/usr/sbin/rmt-tar", 50, [
        ("/usr/share/man/man8/rmt.8.gz", "rmt.8.gz", "/usr/share/man/man8/rmt-tar.8.gz")])],
    "xz-utils": [_xz()],
    "cpio": [("cpio", "/bin/mt", "mt", "/bin/mt-gnu", 10, [
        ("/usr/share/man/man1/mt.1.gz", "mt.1.gz", "/usr/share/man/man1/mt-gnu.1.gz")])],
    "less": [("less", "/usr/bin/pager", "pager", "/usr/bin/less", 77, [
        ("/usr/share/man/man1/pager.1.gz", "pager.1.gz", "/usr/share/man/man1/less.1.gz")])],
    "vim": [_vim("editor", True), _vim("ex", True), _vim("rview", False), _vim("rvim", False),
            _vim("vi", True), _vim("view", True), _vim("vim", False), _vim("vimdiff", False)],
    "iptables": [_xtables("iptables", "iptables", "legacy", 10), _xtables("iptables", "ip6tables", "legacy", 10),
                 _xtables("iptables", "iptables", "nft", 20), _xtables("iptables", "ip6tables", "nft", 20),
                 _xtables("iptables", "arptables", "nft", 20), _xtables("iptables", "ebtables", "nft", 20)],
    "arptables": [_xtables("arptables", "arptables", "legacy", 10)],
    "ebtables": [_xtables("ebtables", "ebtables", "legacy", 10)],
    "tnftp": [("tnftp", "/usr/bin/ftp", "ftp", "/usr/bin/tnftp", 120, [
        ("/usr/share/man/man1/ftp.1.gz", "ftp.1.gz", "/usr/share/man/man1/tnftp.1.gz")])],
    "traceroute": _traceroute(),
    "ndisc6": [("ndisc6", "/usr/bin/traceroute6", "traceroute6", "/usr/bin/rltraceroute6", 110, [
        ("/usr/share/man/man8/traceroute6.8.gz", "traceroute6.8.gz", "/usr/share/man/man8/rltraceroute6.8.gz")])],
    "tcptraceroute": [("tcptraceroute", "/usr/bin/tcptraceroute", "tcptraceroute", "/usr/bin/tcptraceroute.mt", 110, [
        ("/usr/share/man/man1/tcptraceroute.1.gz", "tcptraceroute.1.gz", "/usr/share/man/man1/tcptraceroute.mt.1.gz")])],
    "sysstat": [("sysstat", "/usr/bin/sar", "sar", "/usr/bin/sar.sysstat", 0, [
        ("/usr/share/man/man1/sar.1.gz", "sar.1.gz", "/usr/share/man/man1/sar.sysstat.1.gz")])],
    "wireless-regdb": [
        ("wireless-regdb", "/lib/firmware/regulatory.db", "regulatory.db", "/lib/firmware/regulatory.db-debian", 100, [
            ("/lib/firmware/regulatory.db.p7s", "regulatory.db.p7s", "/lib/firmware/regulatory.db.p7s-debian")]),
        ("wireless-regdb", "/lib/firmware/regulatory.db", "regulatory.db", "/lib/firmware/regulatory.db-upstream", 50, [
            ("/lib/firmware/regulatory.db.p7s", "regulatory.db.p7s", "/lib/firmware/regulatory.db.p7s-upstream")]),
    ],
    # procps: check_alternatives() only for *.procps binaries (none in bookworm's procps).
    "procps": [("procps", "/usr/bin/w", "w", "/usr/bin/w.procps", 50, [
        ("/usr/share/man/man1/w.1.gz", "w.1.gz", "/usr/share/man/man1/w.procps.1.gz")])],
}
