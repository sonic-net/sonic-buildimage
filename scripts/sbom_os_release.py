#!/usr/bin/env python3
"""
sbom_os_release.py — read which Debian release something was built for or
installed onto.

Shared by sbom_fragment.py, which records the release of the build
container a .deb was built in, and build_sbom.py, which reads the release
of the host rootfs and of each container. Two parsers would drift on
quoting and comments, and the release ends up in `distro=` on the purls
both scripts write, where a disagreement makes one package two.
"""

from typing import Optional


def parse(text: str) -> Optional[tuple]:
    """Parse os-release content into ``(ID, VERSION_ID)``.

    Returns None when no ``ID`` field is present.
    """
    kv: dict = {}
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        k, v = line.split("=", 1)
        kv[k] = v.strip().strip('"').strip("'")
    did = kv.get("ID")
    if not did:
        return None
    return (did, kv.get("VERSION_ID", ""))


def read(*paths: str) -> Optional[tuple]:
    """Return ``(ID, VERSION_ID)`` from the first readable os-release."""
    for path in paths:
        try:
            with open(path) as f:
                got = parse(f.read())
        except OSError:
            continue
        if got:
            return got
    return None
