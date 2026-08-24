#!/usr/bin/env python3
"""
sbom_parse_lockfiles.py — extract transitive package inventories from
language-ecosystem lockfiles harvested by the build hooks.

Inputs:
    --lockfiles TARBALL   A lockfiles.tar.gz emitted by per-container
                          collect_version_files. Repeatable.
    --output FILE         Write CycloneDX components[] (JSON array).

The aggregator (scripts/build_sbom.py) consumes the JSON array and
merges the components into the final SBOM with recipe-emit-wins dedupe.

Supported formats:

  Cargo.lock   (Rust)   TOML, [[package]] blocks with name/version/source/checksum
  go.sum       (Go)     '<module> <version> <h1:base64-sha256>' per line
  package-lock.json     (npm)
  pnpm-lock.yaml        (pnpm)
  yarn.lock             (yarn)

Notes on accuracy:

  - Cargo.lock contains the resolved-set (every crate that could satisfy
    the dep graph under any feature combination), which is a superset of
    the crates actually compiled into the binary. SBOM/SCA convention is
    to treat Cargo.lock as the authoritative inventory; precise per-binary
    coverage requires `cargo-auditable build`. See README.sbom.md "Known
    limitations".
  - go.sum is also a superset (it includes modules considered during
    resolution, including dropped versions). When syft can read a Go
    binary's embedded BuildInfo, that's more precise — but parsing go.sum
    ensures we have crate-level hashes that BuildInfo lacks.
"""

import argparse
import base64
import binascii
import json
import os
import re
import sys
import tarfile
from typing import Optional


def warn(msg: str) -> None:
    sys.stderr.write(f"[sbom_parse_lockfiles.py] WARNING: {msg}\n")


# ----------------------------------------------------------------------------
# Cargo.lock parser
# ----------------------------------------------------------------------------


def parse_cargo_lock(text: str, scope: str) -> list:
    """Returns CycloneDX component dicts for every [[package]] entry."""
    components = []
    # Split on '[[package]]' boundaries. Skip the leading non-package preamble.
    blocks = re.split(r"^\[\[package\]\]\s*$", text, flags=re.M)[1:]
    for block in blocks:
        # Stop at the next top-level [...] section header.
        block = re.split(r"^\[[^[]", block, flags=re.M)[0]
        fields = {}
        for line in block.splitlines():
            m = re.match(r'^\s*(\w+)\s*=\s*"?([^"]*?)"?\s*$', line)
            if m:
                fields[m.group(1)] = m.group(2)
        name = fields.get("name")
        version = fields.get("version")
        if not name or not version:
            continue
        comp = {
            "bom-ref": f"pkg:cargo/{name}@{version}",
            "type": "library",
            "name": name,
            "version": version,
            "purl": f"pkg:cargo/{name}@{version}",
            "properties": [
                {"name": "sonic:fragment_kind", "value": "lockfile"},
                {"name": "sonic:lockfile_format", "value": "Cargo.lock"},
                {"name": "sonic:scope", "value": scope},
            ],
        }
        checksum = fields.get("checksum")
        if checksum and re.fullmatch(r"[0-9a-f]{64}", checksum):
            comp["hashes"] = [{"alg": "SHA-256", "content": checksum}]
        source = fields.get("source") or ""
        if source.startswith("registry+"):
            comp["externalReferences"] = [{
                "type": "distribution",
                "url": source[len("registry+"):],
            }]
        elif source.startswith("git+"):
            comp["externalReferences"] = [{
                "type": "vcs",
                "url": source[len("git+"):],
            }]
        components.append(comp)
    return components


# ----------------------------------------------------------------------------
# go.sum parser
# ----------------------------------------------------------------------------


_GOSUM_LINE_RE = re.compile(
    r"^(?P<mod>\S+)\s+(?P<ver>\S+?)(?P<gomod>/go\.mod)?\s+(?P<hash>h1:\S+)$"
)


def _h1_to_sha256_hex(h1: str) -> Optional[str]:
    """Convert 'h1:base64(sha256)=' to lowercase-hex SHA-256."""
    if not h1.startswith("h1:"):
        return None
    try:
        raw = base64.standard_b64decode(h1[3:])
        if len(raw) != 32:
            return None
        return binascii.hexlify(raw).decode("ascii")
    except Exception:
        return None


def parse_go_sum(text: str, scope: str) -> list:
    """Returns CycloneDX components for each Go module in go.sum.

    go.sum has two lines per module:
      <mod> <ver> h1:<base64-sha256-of-module-content>
      <mod> <ver>/go.mod h1:<base64-sha256-of-go.mod-only>
    We use only the first kind (the module-content hash) for the SBOM,
    since that's the closest analog to a 'distribution hash'.
    """
    seen = set()
    components = []
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        m = _GOSUM_LINE_RE.match(line)
        if not m or m.group("gomod"):
            continue
        mod, ver = m.group("mod"), m.group("ver")
        key = (mod, ver)
        if key in seen:
            continue
        seen.add(key)
        purl = f"pkg:golang/{mod}@{ver}"
        comp = {
            "bom-ref": purl,
            "type": "library",
            "name": mod,
            "version": ver,
            "purl": purl,
            "properties": [
                {"name": "sonic:fragment_kind", "value": "lockfile"},
                {"name": "sonic:lockfile_format", "value": "go.sum"},
                {"name": "sonic:scope", "value": scope},
            ],
        }
        sha = _h1_to_sha256_hex(m.group("hash"))
        if sha:
            comp["hashes"] = [{"alg": "SHA-256", "content": sha}]
        components.append(comp)
    return components


# ----------------------------------------------------------------------------
# package-lock.json (npm v2/v3 format) parser
# ----------------------------------------------------------------------------


def parse_package_lock_json(text: str, scope: str) -> list:
    """Parses npm v2/v3 package-lock.json. Older v1 format also supported."""
    components = []
    try:
        doc = json.loads(text)
    except Exception as e:
        warn(f"could not parse package-lock.json: {e}")
        return components
    seen = set()
    # v2/v3: 'packages' dict, keyed by relative path; root is "".
    for path, info in (doc.get("packages") or {}).items():
        if not path or info.get("link"):
            continue
        name = info.get("name") or path.split("node_modules/")[-1]
        version = info.get("version")
        if not name or not version:
            continue
        key = (name, version)
        if key in seen:
            continue
        seen.add(key)
        purl = f"pkg:npm/{name}@{version}"
        comp = {
            "bom-ref": purl,
            "type": "library",
            "name": name,
            "version": version,
            "purl": purl,
            "properties": [
                {"name": "sonic:fragment_kind", "value": "lockfile"},
                {"name": "sonic:lockfile_format", "value": "package-lock.json"},
                {"name": "sonic:scope", "value": scope},
            ],
        }
        integrity = info.get("integrity") or ""
        # integrity is e.g. "sha512-...base64..."
        if integrity.startswith(("sha256-", "sha384-", "sha512-")):
            alg, _, b64 = integrity.partition("-")
            try:
                raw = base64.standard_b64decode(b64)
                comp["hashes"] = [{
                    "alg": alg.upper().replace("SHA", "SHA-"),
                    "content": binascii.hexlify(raw).decode("ascii"),
                }]
            except Exception:
                pass
        resolved = info.get("resolved")
        if resolved:
            comp["externalReferences"] = [{
                "type": "distribution", "url": resolved,
            }]
        components.append(comp)
    # v1: top-level 'dependencies' map (recursive).
    if not components and "dependencies" in doc:
        def walk(deps, prefix=""):
            for name, info in deps.items():
                ver = info.get("version") or ""
                if (name, ver) in seen:
                    continue
                seen.add((name, ver))
                purl = f"pkg:npm/{name}@{ver}"
                components.append({
                    "bom-ref": purl,
                    "type": "library",
                    "name": name,
                    "version": ver,
                    "purl": purl,
                    "properties": [
                        {"name": "sonic:fragment_kind", "value": "lockfile"},
                        {"name": "sonic:lockfile_format",
                         "value": "package-lock.json"},
                        {"name": "sonic:scope", "value": scope},
                    ],
                })
                if "dependencies" in info:
                    walk(info["dependencies"], prefix + name + "/")
        walk(doc.get("dependencies", {}))
    return components


# ----------------------------------------------------------------------------
# pnpm-lock.yaml (rudimentary parser — extracts /pkg@version keys)
# ----------------------------------------------------------------------------


_PNPM_PKG_RE = re.compile(r"^\s+/([^@:\s]+(?:/[^@:\s]+)*)@([^:\s]+):\s*$")


def parse_pnpm_lock_yaml(text: str, scope: str) -> list:
    """Best-effort pnpm-lock.yaml parser without yaml dependency. pnpm
    lockfiles encode package identity as a key like '/foo@1.2.3:' under
    the 'packages:' map. Hash data is omitted in this minimal parser —
    if you need it, swap in a real YAML parser."""
    components = []
    seen = set()
    in_packages = False
    for line in text.splitlines():
        if not in_packages:
            if line.startswith("packages:"):
                in_packages = True
            continue
        if line and not line.startswith(" "):
            break
        m = _PNPM_PKG_RE.match(line)
        if not m:
            continue
        name = m.group(1)
        version = m.group(2)
        if (name, version) in seen:
            continue
        seen.add((name, version))
        purl = f"pkg:npm/{name}@{version}"
        components.append({
            "bom-ref": purl,
            "type": "library",
            "name": name,
            "version": version,
            "purl": purl,
            "properties": [
                {"name": "sonic:fragment_kind", "value": "lockfile"},
                {"name": "sonic:lockfile_format", "value": "pnpm-lock.yaml"},
                {"name": "sonic:scope", "value": scope},
            ],
        })
    return components


# ----------------------------------------------------------------------------
# yarn.lock parser (classic v1 format)
# ----------------------------------------------------------------------------


def parse_yarn_lock(text: str, scope: str) -> list:
    """Parses yarn.lock v1 classic format. v2+ (berry) uses YAML which
    we skip here — most projects still use v1."""
    components = []
    seen = set()
    cur_name = None
    cur_version = None
    cur_integrity = None
    cur_resolved = None

    def flush():
        nonlocal cur_name, cur_version, cur_integrity, cur_resolved
        if cur_name and cur_version and (cur_name, cur_version) not in seen:
            seen.add((cur_name, cur_version))
            purl = f"pkg:npm/{cur_name}@{cur_version}"
            comp = {
                "bom-ref": purl,
                "type": "library",
                "name": cur_name,
                "version": cur_version,
                "purl": purl,
                "properties": [
                    {"name": "sonic:fragment_kind", "value": "lockfile"},
                    {"name": "sonic:lockfile_format", "value": "yarn.lock"},
                    {"name": "sonic:scope", "value": scope},
                ],
            }
            if cur_integrity and cur_integrity.startswith(
                    ("sha256-", "sha384-", "sha512-")):
                alg, _, b64 = cur_integrity.partition("-")
                try:
                    raw = base64.standard_b64decode(b64)
                    comp["hashes"] = [{
                        "alg": alg.upper().replace("SHA", "SHA-"),
                        "content": binascii.hexlify(raw).decode("ascii"),
                    }]
                except Exception:
                    pass
            if cur_resolved:
                comp["externalReferences"] = [{
                    "type": "distribution", "url": cur_resolved,
                }]
            components.append(comp)
        cur_name = cur_version = cur_integrity = cur_resolved = None

    for raw in text.splitlines():
        line = raw.rstrip()
        if not line or line.startswith("#"):
            flush()
            continue
        if not line.startswith(" "):
            # entry-header line, e.g. '"foo@^1.0", "foo@~1.0.1":'
            flush()
            m = re.search(r'^"?([^@\s"]+(?:/[^@\s"]+)?)@', line)
            if m:
                cur_name = m.group(1)
        else:
            s = line.strip()
            m = re.match(r'version\s+"([^"]+)"', s)
            if m:
                cur_version = m.group(1)
            m = re.match(r'integrity\s+([^\s]+)', s)
            if m:
                cur_integrity = m.group(1)
            m = re.match(r'resolved\s+"([^"]+)"', s)
            if m:
                cur_resolved = m.group(1)
    flush()
    return components


# ----------------------------------------------------------------------------
# Tarball walker
# ----------------------------------------------------------------------------


_LOCKFILE_HANDLERS = {
    "Cargo.lock": parse_cargo_lock,
    "go.sum": parse_go_sum,
    "package-lock.json": parse_package_lock_json,
    "pnpm-lock.yaml": parse_pnpm_lock_yaml,
    "yarn.lock": parse_yarn_lock,
}


def scope_from_tarball_path(path: str) -> str:
    """Derive a human-readable scope label from the tarball path.
    target/versions/dockers/docker-fpm-frr/post-versions/lockfiles.tar.gz
        -> 'dockers/docker-fpm-frr'
    target/versions/host-image/post-versions/lockfiles.tar.gz
        -> 'host-image'
    """
    m = re.search(r"versions/(.+?)/post-versions/", path)
    return m.group(1) if m else path


def parse_tarball(tarball: str, components_out: list, stats: dict) -> None:
    if not os.path.isfile(tarball):
        return
    scope = scope_from_tarball_path(tarball)
    try:
        tf = tarfile.open(tarball, "r:*")
    except Exception as e:
        warn(f"could not open {tarball}: {e}")
        return
    with tf:
        for member in tf:
            if not member.isfile():
                continue
            basename = os.path.basename(member.name)
            handler = _LOCKFILE_HANDLERS.get(basename)
            if not handler:
                continue
            try:
                f = tf.extractfile(member)
                if f is None:
                    continue
                text = f.read().decode("utf-8", errors="replace")
            except Exception:
                continue
            try:
                comps = handler(text, scope)
            except Exception as e:
                warn(f"parser {handler.__name__} failed on "
                     f"{member.name} in {tarball}: {e}")
                continue
            if comps:
                components_out.extend(comps)
                stats.setdefault(basename, 0)
                stats[basename] += len(comps)


# ----------------------------------------------------------------------------
# Entry point
# ----------------------------------------------------------------------------


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--lockfiles", action="append", default=[],
                    help="Path to a lockfiles.tar.gz; repeatable.")
    ap.add_argument("--output", required=True,
                    help="Where to write the components[] JSON array.")
    args = ap.parse_args()

    all_components: list = []
    stats: dict = {}
    for tarball in args.lockfiles:
        parse_tarball(tarball, all_components, stats)

    try:
        with open(args.output, "w") as f:
            json.dump({
                "components": all_components,
                "stats": stats,
            }, f, indent=2)
    except Exception as e:
        sys.stderr.write(
            f"[sbom_parse_lockfiles.py] could not write {args.output}: {e}\n"
        )
        return 1

    total = len(all_components)
    bits = " ".join(f"{k}={v}" for k, v in sorted(stats.items()))
    sys.stderr.write(
        f"[sbom_parse_lockfiles.py] parsed {total} components "
        f"({bits or 'no lockfiles found'})\n"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
