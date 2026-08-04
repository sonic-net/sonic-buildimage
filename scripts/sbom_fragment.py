#!/usr/bin/env python3
"""
sbom_fragment.py — emit a CycloneDX 1.6 fragment for a single build artifact.

Invoked from slave.mk recipes after each artifact is built. Reads context
from environment variables:

    ARTIFACT      Required. Path to the just-built .deb / .whl / .gz.
    RECIPE_TYPE   Required. One of:
                  DPKG_DEB, MAKE_DEB, DERIVED_DEB, EXTRA_DEB,
                  ONLINE_DEB, PYTHON_STDEB, PYTHON_WHEEL, DOCKER_IMAGE.
    SRC_PATH      Optional. Value of $($*_SRC_PATH). Used for submodule
                  + patch detection.
    URL           Optional. Value of $($*_URL) for ONLINE_DEB recipes.
    DEPENDS       Optional. Build dependencies.
    RDEPENDS      Optional. Runtime dependencies.
    MAIN_DEB      Optional. For DERIVED_DEB / EXTRA_DEB: filename of the
                  parent .deb that produced this artifact.

Output: <ARTIFACT>.cdx.json next to the artifact.

What this script emits per fragment:
    - Recipe-driven identity: PURL, name, version, arch from the
      artifact filename and recipe context.
    - Source provenance: git submodule URL + commit SHA when SRC_PATH
      is a submodule.
    - Patch enumeration: SHA-256 per patch file when a sibling
      <SRC_PATH>.patch/, <SRC_PATH>/patch/, or <SRC_PATH>/patches/ is
      found.
    - Aggregate patch-set SHA-1 over (series + *.patch), matching the
      scheme used internally by src/sonic-frr/Makefile.
    - Upstream ancestor (pedigree.ancestors[]) when detectable:
      - dget upstream resolved from versions-web for apt-source recipes
      - nested-submodule ancestor for sonic-net wrappers around upstream
      - direct submodule ancestor for non-sonic-net submodules
    - Vendor supplier identity for SONIC_ONLINE_DEBS, derived from URL
      pattern, plus a default EULA license marker. ARTIFACT_LICENSE
      env-var override wins when set.

License resolution itself (DEP-5 parsing, SPDX translation) is
performed later by the aggregator (build_sbom.py) against
copyrights.tar.gz tarballs harvested by the build hook; it is not
done here.

The script is fail-soft: any unexpected condition logs to stderr and
exits 0. Build failure must not be caused by SBOM emit issues.
"""

import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
from typing import Any, Optional


def warn(msg: str) -> None:
    sys.stderr.write(f"[sbom_fragment.py] WARNING: {msg}\n")


def info(msg: str) -> None:
    # Silent by default: per-artifact emission would log ~120 lines per build.
    # Set SBOM_DEBUG=y in the env to see normal progress messages.
    if os.environ.get("SBOM_DEBUG", "n") == "y":
        sys.stderr.write(f"[sbom_fragment.py] {msg}\n")


# ----------------------------------------------------------------------------
# Filename parsers
# ----------------------------------------------------------------------------

_DEB_RE = re.compile(r"^(?P<name>[^_]+)_(?P<version>[^_]+)_(?P<arch>[^.]+)\.deb$")
_WHL_RE = re.compile(
    r"^(?P<name>[^-]+)-(?P<version>[^-]+)-"
    r"(?P<pytag>[^-]+)-(?P<abitag>[^-]+)-(?P<plat>[^.]+)\.whl$"
)
_GZ_RE = re.compile(r"^(?P<name>.+?)(?P<dbg>-dbg)?\.gz$")


def parse_artifact_name(path: str) -> dict:
    """Best-effort decompose an artifact filename."""
    fn = os.path.basename(path)
    if fn.endswith(".deb"):
        m = _DEB_RE.match(fn)
        if m:
            return {
                "kind": "deb",
                "name": m.group("name"),
                "version": m.group("version"),
                "arch": m.group("arch"),
                "filename": fn,
            }
    if fn.endswith(".whl"):
        m = _WHL_RE.match(fn)
        if m:
            return {
                "kind": "wheel",
                "name": m.group("name").replace("_", "-").lower(),
                "version": m.group("version"),
                "arch": "any",
                "py_tag": m.group("pytag"),
                "abi_tag": m.group("abitag"),
                "platform_tag": m.group("plat"),
                "filename": fn,
            }
    if fn.endswith(".gz"):
        m = _GZ_RE.match(fn)
        if m:
            return {
                "kind": "docker",
                "name": m.group("name"),
                "version": os.environ.get("SONIC_IMAGE_VERSION", "0.0.0"),
                "arch": os.environ.get("CONFIGURED_ARCH", "amd64"),
                "filename": fn,
                "debug": bool(m.group("dbg")),
            }
    return {
        "kind": "unknown",
        "name": fn,
        "version": "0.0.0",
        "arch": os.environ.get("CONFIGURED_ARCH", "amd64"),
        "filename": fn,
    }


# ----------------------------------------------------------------------------
# Submodule detection (best-effort, fail-soft)
# ----------------------------------------------------------------------------


def run(cmd: list, cwd: Optional[str] = None) -> Optional[str]:
    """Run a command and return stdout or None on any error."""
    try:
        r = subprocess.run(
            cmd, cwd=cwd, capture_output=True, text=True, timeout=30, check=False
        )
        if r.returncode == 0:
            return r.stdout.strip()
    except Exception:
        pass
    return None


def _find_gitmodules_for(abs_src: str) -> Optional[tuple]:
    """Walk parents looking for a .gitmodules; return (gm_path, rel_path)."""
    cur = os.path.dirname(abs_src)
    while cur and cur != "/":
        gm = os.path.join(cur, ".gitmodules")
        if os.path.isfile(gm):
            return gm, os.path.relpath(abs_src, cur)
        cur = os.path.dirname(cur)
    return None


def _lookup_submodule_url(gm: str, rel: str) -> Optional[str]:
    """Find the submodule.<x>.url whose path = rel inside the .gitmodules file."""
    url = run(["git", "config", "-f", gm, "--get",
               f"submodule.{rel}.url"])
    if url:
        return url
    # .gitmodules keys by section name (often != path); fall back to grep.
    try:
        with open(gm) as f:
            text = f.read()
        blocks = re.split(r"^\[submodule ", text, flags=re.M)
        for b in blocks[1:]:
            if re.search(rf"^\s*path\s*=\s*{re.escape(rel)}\s*$", b, re.M):
                m = re.search(r"^\s*url\s*=\s*(\S+)\s*$", b, re.M)
                if m:
                    return m.group(1)
    except Exception:
        pass
    return None


def detect_submodule(src_path: str) -> Optional[dict]:
    """If src_path is a git submodule, return {url, commit, describe}."""
    if not src_path or not os.path.isdir(src_path):
        return None
    abs_src = os.path.abspath(src_path)
    found = _find_gitmodules_for(abs_src)
    if not found:
        return None
    gm, rel = found
    url = _lookup_submodule_url(gm, rel)
    if not url:
        return None
    commit = run(["git", "rev-parse", "HEAD"], cwd=abs_src)
    describe = run(["git", "describe", "--tags", "--always"], cwd=abs_src)
    return {
        "url": url.rstrip(".git").rstrip("/"),
        "commit": commit,
        "describe": describe,
    }


def detect_nested_submodules(src_path: str) -> list:
    """Find submodule paths that live *inside* src_path.

    Used to detect the FRR pattern (src/sonic-frr → src/sonic-frr/frr →
    FRRouting/frr) where the outer dir is a sonic-net wrapper holding
    patches + Makefile and the inner dir is upstream.
    """
    if not src_path or not os.path.isdir(src_path):
        return []
    abs_src = os.path.abspath(src_path)
    found = _find_gitmodules_for(abs_src)
    if not found:
        return []
    gm, src_rel = found
    out = []
    try:
        with open(gm) as f:
            text = f.read()
        for block in re.split(r"^\[submodule ", text, flags=re.M)[1:]:
            m = re.search(r"^\s*path\s*=\s*(\S+)\s*$", block, re.M)
            if not m:
                continue
            path = m.group(1)
            # Is this path strictly under src_rel?
            if path == src_rel or not path.startswith(src_rel.rstrip("/") + "/"):
                continue
            u = re.search(r"^\s*url\s*=\s*(\S+)\s*$", block, re.M)
            if not u:
                continue
            url = u.group(1).rstrip(".git").rstrip("/")
            cur = os.path.dirname(gm)
            sub_abs = os.path.join(cur, path)
            commit = run(["git", "rev-parse", "HEAD"], cwd=sub_abs)
            describe = run(["git", "describe", "--tags", "--always"],
                           cwd=sub_abs)
            out.append({
                "path": path,
                "url": url,
                "commit": commit,
                "describe": describe,
            })
    except Exception as e:
        warn(f"nested-submodule scan failed in {src_path}: {e}")
    return out


# ----------------------------------------------------------------------------
# Upstream Debian source lookup (Pattern A2)
# ----------------------------------------------------------------------------

# Two sources of versions-web data:
#   1. files/build/versions/default/versions-web - committed pins.
#      Carries the "blessed" upstream URLs for things that have been
#      version-promoted (dget'd Debian sources are the main case here).
#   2. target/versions/.../versions-web - written by the wget/curl shim
#      during the current build. Carries everything the build actually
#      fetched, including things not yet promoted (e.g. the kernel
#      source descriptor, which the kernel Makefile wgets directly).
# We scan both so the lookup is correct on first-build before any
# promotion has happened.
_VERSIONS_WEB_SOURCES = [
    os.path.join("files", "build", "versions", "default", "versions-web"),
]

_DSC_URL_RE = re.compile(
    r"^(?P<url>https?://[^=]+?/(?P<pkg>[^/]+)/(?P=pkg)_(?P<ver>[^/]+?)\.dsc)==(?P<md5>[0-9a-f]+)$"
)
_ORIG_URL_RE = re.compile(
    r"^(?P<url>https?://[^=]+?/(?P<pkg>[^/]+)/(?P=pkg)_(?P<ver>[^.]+?)\.orig\.tar\.[^=]+)==(?P<md5>[0-9a-f]+)$"
)


def _collect_versions_web_files() -> list:
    """Return the committed pin file plus any build-time versions-web
    captures under target/versions/."""
    files = []
    for p in _VERSIONS_WEB_SOURCES:
        if os.path.isfile(p):
            files.append(p)
    target = os.environ.get("TARGET_PATH", "target")
    if os.path.isdir(target):
        for root, _, fns in os.walk(os.path.join(target, "versions")):
            for fn in fns:
                if fn == "versions-web":
                    files.append(os.path.join(root, fn))
    return files


def lookup_upstream_debian_source(src_path: str) -> Optional[dict]:
    """For Pattern A2 (dget/wget-based recipes), find the upstream Debian
    source descriptor in versions-web. Returns
        {name, version, dsc_url, dsc_md5, orig_url, orig_md5}
    or None.
    """
    if not src_path:
        return None
    src_basename = os.path.basename(src_path.rstrip("/"))
    if not src_basename:
        return None
    # Special case: sonic-linux-kernel builds Debian's `linux` source.
    lookup_keys = [src_basename]
    if src_basename == "sonic-linux-kernel":
        lookup_keys.append("linux")
    dsc_match = None
    orig_match = None
    for path in _collect_versions_web_files():
        try:
            with open(path) as f:
                for line in f:
                    line = line.strip()
                    if not line:
                        continue
                    m = _DSC_URL_RE.match(line)
                    if m and m.group("pkg") in lookup_keys:
                        dsc_match = m
                        continue
                    m = _ORIG_URL_RE.match(line)
                    if m and m.group("pkg") in lookup_keys:
                        orig_match = m
        except Exception as e:
            warn(f"versions-web scan of {path} failed: {e}")
            continue
        if dsc_match:
            break
    if not dsc_match:
        return None
    info = {
        "name": dsc_match.group("pkg"),
        "version": dsc_match.group("ver"),
        "dsc_url": dsc_match.group("url"),
        "dsc_md5": dsc_match.group("md5"),
    }
    if orig_match:
        info["orig_url"] = orig_match.group("url")
        info["orig_md5"] = orig_match.group("md5")
    return info


# ----------------------------------------------------------------------------
# Patch series detection
# ----------------------------------------------------------------------------


def find_patch_dir(src_path: str) -> Optional[str]:
    """Locate a patch series directory adjacent to or under src_path."""
    if not src_path:
        return None
    candidates = [
        src_path + ".patch",   # src/scapy.patch, src/sonic-swss.patch, ...
        os.path.join(src_path, "patch"),     # src/openssh/patch, src/sonic-frr/patch
        os.path.join(src_path, "patches"),   # src/bash/patches, src/sonic-mgmt-common/patches
    ]
    # Linux kernel uses a uniquely-named subdir.
    if os.path.basename(src_path.rstrip("/")) == "sonic-linux-kernel":
        candidates.insert(0, os.path.join(src_path, "patches-sonic"))
    for c in candidates:
        if os.path.isfile(os.path.join(c, "series")):
            return c
    return None


def enumerate_patches(patch_dir: str) -> list:
    """Read series file, hash each patch."""
    out = []
    series = os.path.join(patch_dir, "series")
    if not os.path.isfile(series):
        return out
    try:
        with open(series) as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith("#"):
                    continue
                # Series lines can carry options after the filename.
                fname = line.split()[0]
                pf = os.path.join(patch_dir, fname)
                if not os.path.isfile(pf):
                    continue
                h = hashlib.sha256()
                with open(pf, "rb") as pfh:
                    h.update(pfh.read())
                out.append({
                    "name": fname,
                    "path": os.path.relpath(pf),
                    "sha256": h.hexdigest(),
                })
    except Exception as e:
        warn(f"failed to read patch series in {patch_dir}: {e}")
    return out


def patchset_hash(patch_dir: str) -> Optional[str]:
    """Aggregate SHA-256 over series + *.patch contents (FRR-style)."""
    series = os.path.join(patch_dir, "series")
    if not os.path.isfile(series):
        return None
    h = hashlib.sha256()
    try:
        with open(series, "rb") as f:
            h.update(f.read())
        # Hash the patch files in series order.
        with open(series) as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith("#"):
                    continue
                fname = line.split()[0]
                pf = os.path.join(patch_dir, fname)
                if os.path.isfile(pf):
                    with open(pf, "rb") as pfh:
                        h.update(pfh.read())
        return h.hexdigest()
    except Exception:
        return None


# ----------------------------------------------------------------------------
# PURL construction
# ----------------------------------------------------------------------------


def is_sonic_net_url(url: Optional[str]) -> bool:
    if not url:
        return False
    return "github.com/sonic-net" in url or "github.com/Azure/sonic" in url


# Pattern C — vendor binary suppliers. Used to label SONIC_ONLINE_DEBS
# (Broadcom/Mellanox/Marvell/NVIDIA/Pensando/Arista SDKs) so the SBOM
# records who owns the license terms. The match is a substring against
# the lowercased URL; first hit wins.
_VENDOR_URL_PATTERNS = [
    ("broadcom", "Broadcom Inc."),
    ("sai-broadcom", "Broadcom Inc."),
    ("libsaibcm", "Broadcom Inc."),
    ("mellanox", "NVIDIA / Mellanox"),
    ("nvidia", "NVIDIA"),
    ("bluefield", "NVIDIA / Mellanox"),
    ("doca", "NVIDIA"),
    ("marvell", "Marvell Technology"),
    ("teralynx", "Marvell Technology"),
    ("prestera", "Marvell Technology"),
    ("pensando", "AMD / Pensando"),
    ("aristanetworks", "Arista Networks"),
    ("aristanetwork", "Arista Networks"),
    ("phy-credo", "Credo"),
    ("p4lang", "P4 Language Consortium"),
    ("openconfig", "OpenConfig"),
]


def _vendor_supplier_from_url(url: Optional[str]) -> Optional[str]:
    if not url:
        return None
    u = url.lower()
    for pat, supplier in _VENDOR_URL_PATTERNS:
        if pat in u:
            return supplier
    return None


def build_purl(meta: dict, submodule: Optional[dict]) -> str:
    """Construct a PURL appropriate to the artifact + source provenance."""
    if meta["kind"] == "deb":
        # Heuristic: when source is a sonic-net submodule, use the github
        # PURL as primary identity. Otherwise it's an apt-style deb.
        if submodule and is_sonic_net_url(submodule.get("url")):
            repo = submodule["url"].rsplit("/", 1)[-1]
            sha = (submodule.get("commit") or "")[:12]
            return f"pkg:github/sonic-net/{repo}@{sha}"
        # Default: deb with sonic namespace (locally rebuilt).
        return f"pkg:deb/sonic/{meta['name']}@{meta['version']}?arch={meta['arch']}"
    if meta["kind"] == "wheel":
        return f"pkg:pypi/{meta['name']}@{meta['version']}"
    if meta["kind"] == "docker":
        return f"pkg:oci/{meta['name']}@{meta['version']}?arch={meta['arch']}"
    return f"pkg:generic/{meta['name']}@{meta['version']}"


# ----------------------------------------------------------------------------
# Ancestor (pedigree.ancestors[]) construction
# ----------------------------------------------------------------------------


def _github_purl_for_url(url: str, ref: Optional[str]) -> str:
    """Derive a pkg:github/<org>/<repo>@<ref> PURL from a GitHub URL."""
    m = re.search(r"github\.com[:/]([^/]+)/([^/]+?)(?:\.git)?/?$", url)
    if m:
        return f"pkg:github/{m.group(1)}/{m.group(2)}@{ref or 'unknown'}"
    # Generic fallback for non-GitHub URLs.
    return f"pkg:generic/{url}@{ref or 'unknown'}"


def ancestor_from_submodule(sm: dict) -> dict:
    """Build a pedigree.ancestor entry from a non-sonic-net submodule."""
    ref = sm.get("describe") or (sm.get("commit") or "")[:12]
    purl = _github_purl_for_url(sm["url"], ref)
    return {
        "type": "library",
        "name": sm["url"].rsplit("/", 1)[-1],
        "version": ref,
        "purl": purl,
        "externalReferences": [{
            "type": "vcs",
            "url": sm["url"],
            "comment": f"submodule commit {sm.get('commit', '')}",
        }],
    }


def ancestor_from_debian_source(info: dict) -> dict:
    """Build a pedigree.ancestor entry from a versions-web dget lookup."""
    ext_refs = [{
        "type": "distribution",
        "url": info["dsc_url"],
        "hashes": [{"alg": "MD5", "content": info["dsc_md5"]}],
    }]
    if info.get("orig_url"):
        ext_refs.append({
            "type": "distribution",
            "url": info["orig_url"],
            "hashes": [{"alg": "MD5", "content": info["orig_md5"]}],
            "comment": "original upstream tarball",
        })
    return {
        "type": "library",
        "name": info["name"],
        "version": info["version"],
        "purl": f"pkg:deb/debian/{info['name']}@{info['version']}",
        "externalReferences": ext_refs,
    }


# ----------------------------------------------------------------------------
# Fragment construction
# ----------------------------------------------------------------------------


def component_type(kind: str) -> str:
    if kind == "docker":
        return "container"
    if kind == "wheel":
        return "library"
    return "library"


def split_csv(value: str) -> list:
    return [x.strip() for x in (value or "").split() if x.strip()]


# ---------------------------------------------------------------------------
# Per-.deb language dependency attribution
#
# For each SONiC-built .deb we extract once with `dpkg-deb -x` and run
# three harvesters against the unpacked tree:
#
#   Rust    cargo-auditable embeds the resolved Cargo dep graph as a
#           zlib-compressed JSON blob in the `.dep-v0` ELF section.
#           rust-audit-info reads it back out.
#   Go      `go build` embeds module info via runtime/debug.BuildInfo
#           (the `.go.buildinfo` ELF section). `go version -m <bin>`
#           prints it as line-oriented text.
#   Python  Wheel-style installs land in `*.dist-info/` directories
#           alongside the importable package. We walk for METADATA
#           files and parse them as RFC 822.
#
# Each emitted component carries:
#       sonic:fragment_kind  = recipe-emit-{rust,go,python}
#       sonic:source_deb     = <the .deb filename that shipped them>
#
# build_sbom.py:build_dependency_graph() reverses sonic:source_deb into
# a dependsOn edge from the .deb's bom-ref to each language-dep
# component, so consumers can walk swss_*.deb -> tokio@1.x or
# sonic-gnmi_*.deb -> github.com/openconfig/gnmi@v0.10 directly.
#
# Scope is .deb-level (not per-binary): if two binaries inside the same
# .deb both link tokio, we emit one tokio component. CVE-blast-radius
# triage operates at the artifact level for our use case.
# ---------------------------------------------------------------------------

# Per-binary tool-execution timeout. rust-audit-info and `go version -m`
# are fast (tens of ms each) on well-formed inputs but could hang on
# corrupt or extremely large ELFs. Don't let one weird .deb wedge the
# whole build.
_LANG_TOOL_TIMEOUT_SECS = 30

# Sentinel env vars so the "tool not available" warnings only fire once
# per build, instead of once per .deb fragment.
_RUST_TOOL_LOGGED = "_SBOM_RUST_AUDIT_INFO_MISSING_LOGGED"
_GO_TOOL_LOGGED = "_SBOM_GO_TOOL_MISSING_LOGGED"

_PYPI_NORM_RE = re.compile(r"[-_.]+")


def _is_elf(path: str) -> bool:
    """Cheap 4-byte magic check for ELF files. Skips symlinks and
    non-files (named pipes, sockets) silently."""
    try:
        if not os.path.isfile(path) or os.path.islink(path):
            return False
        with open(path, "rb") as f:
            return f.read(4) == b"\x7fELF"
    except OSError:
        return False


def _pypi_normalize(name: str) -> str:
    """PEP 503 name normalization: lowercase, runs of -_. -> single -."""
    return _PYPI_NORM_RE.sub("-", name).lower()


def _rust_components_from_elf(
    elf_path: str, audit_info_bin: str, deb_filename: str,
) -> list:
    """Run rust-audit-info on one ELF; return CycloneDX components for
    every crate it found. Silent skip on any failure — most binaries
    are non-Rust or built without cargo-auditable.
    """
    try:
        out = subprocess.run(
            [audit_info_bin, elf_path],
            capture_output=True, text=True, check=False,
            timeout=_LANG_TOOL_TIMEOUT_SECS,
        )
    except subprocess.TimeoutExpired:
        warn(f"rust-audit-info timed out on {elf_path}")
        return []
    if out.returncode != 0 or not out.stdout.strip():
        return []
    try:
        data = json.loads(out.stdout)
    except json.JSONDecodeError:
        return []
    pkgs = data.get("packages") or []
    components = []
    for pkg in pkgs:
        name = pkg.get("name")
        version = pkg.get("version")
        if not (name and version):
            continue
        source = pkg.get("source") or "local"
        purl = f"pkg:cargo/{name}@{version}"
        comp = {
            "bom-ref": purl,
            "type": "library",
            "name": name,
            "version": version,
            "purl": purl,
            "properties": [
                {"name": "sonic:fragment_kind", "value": "recipe-emit-rust"},
                {"name": "sonic:source_deb", "value": deb_filename},
                {"name": "sonic:source", "value": source},
            ],
        }
        kind = pkg.get("kind")
        if kind:
            comp["properties"].append(
                {"name": "sonic:cargo_kind", "value": str(kind)}
            )
        components.append(comp)
    return components


def _go_components_from_elf(
    elf_path: str, go_bin: str, deb_filename: str,
) -> list:
    """Run `go version -m <elf>` and parse the embedded module graph.

    Format (each module line begins with a leading tab):

        <path>: go1.21.5
        \\tpath\\t<main module path>
        \\tmod\\t<main module>\\t(devel)
        \\tdep\\t<name>\\t<version>\\t<h1-hash>
        \\t=>\\t<name>\\t<version>\\t<h1-hash>    (replacement of previous dep)
        \\tbuild\\t...

    We only care about `dep` and `=>` lines. A `=>` immediately
    following a `dep` line means the dep was replaced; we keep the
    replacement (what actually shipped).
    """
    try:
        out = subprocess.run(
            [go_bin, "version", "-m", elf_path],
            capture_output=True, text=True, check=False,
            timeout=_LANG_TOOL_TIMEOUT_SECS,
        )
    except subprocess.TimeoutExpired:
        warn(f"go version -m timed out on {elf_path}")
        return []
    if out.returncode != 0 or not out.stdout:
        return []
    # `go version -m` prints "<path>: <ver>" for Go binaries; for
    # non-Go ELFs it prints "<path>: go: ..." or returns rc=1. Either
    # way, no module lines follow.
    deps: list = []   # ordered list of (name, version)
    for line in out.stdout.splitlines():
        parts = line.split("\t")
        # Module lines look like ['', '<kind>', '<name>', '<version>', ...]
        if len(parts) < 4 or parts[0] != "":
            continue
        kind = parts[1]
        name = parts[2]
        version = parts[3]
        if not (name and version):
            continue
        if kind == "dep":
            deps.append((name, version))
        elif kind == "=>" and deps:
            # Replacement directive — overwrite the immediately
            # preceding dep so we record what actually shipped.
            deps[-1] = (name, version)
    components = []
    for name, version in deps:
        purl = f"pkg:golang/{name}@{version}"
        components.append({
            "bom-ref": purl,
            "type": "library",
            "name": name,
            "version": version,
            "purl": purl,
            "properties": [
                {"name": "sonic:fragment_kind", "value": "recipe-emit-go"},
                {"name": "sonic:source_deb", "value": deb_filename},
            ],
        })
    return components


def _python_components_from_dist_info(
    tmpdir: str, deb_filename: str,
) -> list:
    """Walk an extracted .deb tree for *.dist-info/METADATA files and
    parse Name + Version out of each. Wheels installed inside a .deb
    (either directly bundled, or staged via pip in the recipe) show up
    here. Older .egg-info directories with PKG-INFO are not parsed;
    dist-info is the modern format and what pip emits since 2020.
    """
    components = []
    seen: set = set()  # (normalized_name, version)
    for root, _, _ in os.walk(tmpdir):
        if not root.endswith(".dist-info"):
            continue
        meta_path = os.path.join(root, "METADATA")
        if not os.path.isfile(meta_path):
            continue
        name = None
        version = None
        try:
            with open(meta_path, errors="replace") as f:
                for line in f:
                    # Headers stop at the first blank line (RFC 822).
                    if not line.strip():
                        break
                    # Continuation lines start with whitespace; ignore
                    # them — Name and Version are always single-line.
                    if line[0] in (" ", "\t"):
                        continue
                    if line.startswith("Name:"):
                        name = line.split(":", 1)[1].strip()
                    elif line.startswith("Version:"):
                        version = line.split(":", 1)[1].strip()
                    if name and version:
                        break
        except OSError:
            continue
        if not (name and version):
            continue
        norm = _pypi_normalize(name)
        key = (norm, version)
        if key in seen:
            continue
        seen.add(key)
        purl = f"pkg:pypi/{norm}@{version}"
        components.append({
            "bom-ref": purl,
            "type": "library",
            "name": name,
            "version": version,
            "purl": purl,
            "properties": [
                {"name": "sonic:fragment_kind", "value": "recipe-emit-python"},
                {"name": "sonic:source_deb", "value": deb_filename},
            ],
        })
    return components


def extract_lang_deps_from_deb(deb_path: str, deb_filename: str) -> list:
    """One-shot .deb extraction + Rust + Go + Python harvesters.

    Returns a flat list of CycloneDX components. Each component is
    deduped at .deb scope (one entry per (purl) per .deb regardless
    of how many binaries it appeared in).

    Returns [] when:
      - the .deb doesn't exist or extraction fails
      - the .deb contains no Rust/Go/Python deps to attribute
    """
    if not os.path.exists(deb_path):
        return []

    # Tool availability: warn once per build (via process-env sentinel)
    # if a tool is missing, then skip its harvester. The other two
    # languages still run.
    audit_info = shutil.which("rust-audit-info")
    if not audit_info and not os.environ.get(_RUST_TOOL_LOGGED):
        warn(
            "rust-audit-info not in PATH; .deb-introspected Rust "
            "crate attribution disabled (this enrichment requires "
            "rust-audit-info, installed in slave Dockerfiles)"
        )
        os.environ[_RUST_TOOL_LOGGED] = "1"
    go_bin = shutil.which("go")
    if not go_bin and not os.environ.get(_GO_TOOL_LOGGED):
        warn(
            "go not in PATH; .deb-introspected Go module attribution "
            "disabled (golang-go is normally installed in every slave "
            "image alongside the C/C++ toolchain)"
        )
        os.environ[_GO_TOOL_LOGGED] = "1"

    by_purl: dict = {}   # purl -> component (dedup at .deb scope)
    counts = {"rust": 0, "go": 0, "python": 0, "elfs": 0}

    try:
        with tempfile.TemporaryDirectory(prefix="sbom-lang-deb-") as tmpdir:
            extract = subprocess.run(
                ["dpkg-deb", "-x", deb_path, tmpdir],
                capture_output=True, text=True, check=False,
            )
            if extract.returncode != 0:
                warn(
                    f"dpkg-deb -x failed for {deb_path}: "
                    f"{(extract.stderr or '').strip()[:200]}"
                )
                return []
            # Single ELF walk — both Rust + Go harvesters share it.
            for root, _, files in os.walk(tmpdir):
                for fn in files:
                    path = os.path.join(root, fn)
                    if not _is_elf(path):
                        continue
                    counts["elfs"] += 1
                    if audit_info:
                        for comp in _rust_components_from_elf(
                            path, audit_info, deb_filename,
                        ):
                            if comp["purl"] not in by_purl:
                                by_purl[comp["purl"]] = comp
                                counts["rust"] += 1
                    if go_bin:
                        for comp in _go_components_from_elf(
                            path, go_bin, deb_filename,
                        ):
                            if comp["purl"] not in by_purl:
                                by_purl[comp["purl"]] = comp
                                counts["go"] += 1
            # Python dist-info walk: independent of ELF walk; runs
            # against the same tmpdir.
            for comp in _python_components_from_dist_info(
                tmpdir, deb_filename,
            ):
                if comp["purl"] not in by_purl:
                    by_purl[comp["purl"]] = comp
                    counts["python"] += 1
    except OSError as e:
        warn(f"language-dep attribution failed for {deb_path}: {e}")
        return []

    if not by_purl:
        return []

    info(
        f"lang-attribution: {deb_filename} -> "
        f"{counts['rust']} crates, {counts['go']} go modules, "
        f"{counts['python']} python dists ({counts['elfs']} ELFs scanned)"
    )
    # Sort for stable output ordering across builds.
    return [by_purl[p] for p in sorted(by_purl)]


# Backwards-compatibility alias for any caller still importing the
# Rust-specific name. Kept as a thin wrapper that filters to Rust
# components only.
def extract_rust_deps_from_deb(deb_path: str, deb_filename: str) -> list:
    """Deprecated: use extract_lang_deps_from_deb() instead.
    Returned for backward compatibility; filters to recipe-emit-rust.
    """
    return [
        c for c in extract_lang_deps_from_deb(deb_path, deb_filename)
        if any(p.get("name") == "sonic:fragment_kind"
               and p.get("value") == "recipe-emit-rust"
               for p in c.get("properties", []) or [])
    ]


def build_fragment(artifact: str, recipe_type: str) -> dict:
    meta = parse_artifact_name(artifact)
    src_path = os.environ.get("SRC_PATH", "")
    submodule = detect_submodule(src_path) if src_path else None
    patch_dir = find_patch_dir(src_path) if src_path else None
    patches = enumerate_patches(patch_dir) if patch_dir else []
    ps_hash = patchset_hash(patch_dir) if patch_dir else None

    bom_ref = build_purl(meta, submodule)

    component: dict[str, Any] = {
        "bom-ref": bom_ref,
        "type": component_type(meta["kind"]),
        "name": meta["name"],
        "version": meta["version"],
        "purl": bom_ref,
        "properties": [
            {"name": "sonic:fragment_kind", "value": "recipe-emit"},
            {"name": "sonic:recipe_type", "value": recipe_type},
            {"name": "sonic:artifact_filename", "value": meta["filename"]},
        ],
    }

    if meta["kind"] == "deb":
        component["properties"].append(
            {"name": "sonic:arch", "value": meta["arch"]}
        )

    # Honor an explicit per-recipe license override
    # ($(ARTIFACT)_LICENSE in the recipe → ARTIFACT_LICENSE env var here).
    # Useful for SONiC-native debs/wheels that don't ship a debian/copyright
    # — the DEP-5 resolver can't infer their license from /usr/share/doc.
    artifact_license = os.environ.get("ARTIFACT_LICENSE", "").strip()
    if artifact_license:
        component["licenses"] = [{"expression": artifact_license}]

    if submodule:
        component["externalReferences"] = [{
            "type": "vcs",
            "url": submodule["url"],
            "comment": f"submodule pinned to {submodule.get('commit', '')}",
        }]
        component["properties"].append(
            {"name": "sonic:submodule_commit",
             "value": submodule.get("commit", "")}
        )

    online_url = os.environ.get("URL", "")
    if recipe_type == "ONLINE_DEB" and online_url:
        component.setdefault("externalReferences", []).append({
            "type": "distribution",
            "url": online_url,
        })
        # Compute SHA-256 of the just-fetched artifact so we have an
        # authoritative hash regardless of whether _SKIP_VERSION was set.
        try:
            h = hashlib.sha256()
            with open(artifact, "rb") as f:
                for chunk in iter(lambda: f.read(65536), b""):
                    h.update(chunk)
            component["hashes"] = [{"alg": "SHA-256", "content": h.hexdigest()}]
        except Exception as e:
            warn(f"could not hash {artifact}: {e}")

        # Pattern C: derive vendor supplier from URL pattern; mark the
        # license category as proprietary by default (overridable via
        # ARTIFACT_LICENSE if a vendor publishes under a known SPDX
        # license).
        supplier = _vendor_supplier_from_url(online_url)
        if supplier:
            component["supplier"] = {"name": supplier}
            component["properties"].append(
                {"name": "sonic:supplier_source", "value": "vendor_url"}
            )
            # Don't override an explicit ARTIFACT_LICENSE if one was set.
            if not component.get("licenses"):
                component["licenses"] = [
                    {"license": {"name": f"{supplier} proprietary (EULA)"}}
                ]

    # ---- Ancestor resolution (Pattern A2/A3/A4) ----
    ancestors: list = []

    # Pattern A4: direct non-sonic-net submodule. The submodule IS the
    # upstream. Emit it as ancestor so the component records both the
    # SONiC-rebuilt identity (primary PURL) and the upstream source.
    if submodule and not is_sonic_net_url(submodule.get("url")):
        ancestors.append(ancestor_from_submodule(submodule))

    # Pattern A3: sonic-net wrapper with a nested non-sonic-net
    # submodule (FRR / sonic-pins / gnoi / wpa-supplicant pattern).
    # The nested submodule is the true upstream.
    if src_path:
        for nested in detect_nested_submodules(src_path):
            if not is_sonic_net_url(nested.get("url")):
                ancestors.append(ancestor_from_submodule(nested))

    # Pattern A2: dget-based recipe. Look up the upstream Debian source
    # descriptor in versions-web. Only consider this if SRC_PATH is NOT
    # itself a submodule (which would mean Pattern A1/A4).
    if src_path and not submodule:
        upstream = lookup_upstream_debian_source(src_path)
        if upstream:
            ancestors.append(ancestor_from_debian_source(upstream))

    if patches or ancestors:
        pedigree: dict[str, Any] = {}
        if ancestors:
            pedigree["ancestors"] = ancestors
        if patches:
            pedigree["patches"] = [
                {
                    "type": "unofficial",
                    "diff": {
                        "url": f"file://{p['path']}",
                        "hashes": [{"alg": "SHA-256", "content": p["sha256"]}],
                    },
                }
                for p in patches
            ]
        if ps_hash:
            pedigree["notes"] = f"patch-set sha256: {ps_hash}"
        component["pedigree"] = pedigree

    main_deb = os.environ.get("MAIN_DEB", "")
    if recipe_type in ("DERIVED_DEB", "EXTRA_DEB") and main_deb:
        component["properties"].append(
            {"name": "sonic:parent_artifact", "value": main_deb}
        )

    depends = split_csv(os.environ.get("DEPENDS", ""))
    rdepends = split_csv(os.environ.get("RDEPENDS", ""))
    if depends:
        component["properties"].append(
            {"name": "sonic:build_depends", "value": " ".join(depends)}
        )
    if rdepends:
        component["properties"].append(
            {"name": "sonic:runtime_depends", "value": " ".join(rdepends)}
        )

    # Language-dep attribution: extract per-binary Rust crates / Go
    # modules + per-dist-info Python packages from any .deb fragment.
    # Non-.deb recipes return []. Timing instrumentation lets the
    # build log show real per-deb cost (Phase E); gated to only log
    # when something was found or when the operation took noticeably
    # long, so we don't spam ~120 zero-line entries per build.
    lang_components: list = []
    if meta["kind"] == "deb":
        t0 = time.monotonic()
        lang_components = extract_lang_deps_from_deb(
            artifact, meta["filename"],
        )
        dt = time.monotonic() - t0
        if lang_components or dt > 1.0:
            info(
                f"lang-deps timing: {meta['filename']} took {dt:.2f}s "
                f"({len(lang_components)} components)"
            )

    fragment = {
        "bomFormat": "CycloneDX",
        "specVersion": "1.6",
        "version": 1,
        "metadata": {
            "properties": [
                {"name": "sonic:fragment_kind", "value": "recipe-emit"},
                {"name": "sonic:recipe_type", "value": recipe_type},
                {"name": "sonic:src_path", "value": src_path},
                {"name": "sonic:platform",
                 "value": os.environ.get("CONFIGURED_PLATFORM", "")},
                {"name": "sonic:arch",
                 "value": os.environ.get("CONFIGURED_ARCH", "")},
            ],
        },
        "components": [component, *lang_components],
    }
    return fragment


# ----------------------------------------------------------------------------
# Entry point
# ----------------------------------------------------------------------------


def main() -> int:
    if os.environ.get("ENABLE_SBOM", "n") != "y":
        return 0

    artifact = os.environ.get("ARTIFACT", "")
    recipe_type = os.environ.get("RECIPE_TYPE", "")
    if not artifact or not recipe_type:
        warn("ARTIFACT and RECIPE_TYPE env vars are required")
        return 0  # Fail-soft

    if not os.path.exists(artifact):
        warn(f"artifact does not exist: {artifact}")
        return 0

    try:
        fragment = build_fragment(artifact, recipe_type)
    except Exception as e:
        warn(f"fragment construction failed for {artifact}: {e}")
        return 0

    out_path = f"{artifact}.cdx.json"
    try:
        with open(out_path, "w") as f:
            json.dump(fragment, f, indent=2, sort_keys=True)
        info(f"wrote {out_path}")
    except Exception as e:
        warn(f"could not write {out_path}: {e}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
