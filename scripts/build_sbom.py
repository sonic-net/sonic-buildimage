#!/usr/bin/env python3
"""
build_sbom.py — SBOM aggregator for SONiC builds.

Invoked between build_debian.sh and build_image.sh (from slave.mk) once
the host rootfs and all containers are assembled. Produces:

    target/<artifact>.cdx.json   (CycloneDX 1.6, sibling of the installer)
                                 — <artifact> is sonic-<machine>.bin
                                   for ONIE installers, .swi for Arista
                                   aboot, .img.gz for VS/VPP

Inputs (env vars from slave.mk):

    ENABLE_SBOM                must be 'y'; otherwise this is a no-op.
    SBOM_SCAN_TOOL             syft (default) | trivy
    SBOM_FORMAT                cyclonedx (default) | spdx | both
    TARGET_PATH                build output dir (default: 'target')
    TARGET_MACHINE             from onie-image.conf — names the SBOM file
    CONFIGURED_ARCH            amd64 | arm64 | armhf
    CONFIGURED_PLATFORM        broadcom | mellanox | vs | ...
    SONIC_VERSION_CONTROL_COMPONENTS   active pin policy (recorded in metadata)
    SBOM_INSTALLER_DOCKERS     space-separated list of docker .gz filenames
                               that actually ship in this installer.
    SBOM_INSTALLER_DEBS        space-separated list of .deb filenames
                               installed into the host rootfs.
    SBOM_INSTALLER_WHEELS      space-separated list of .whl filenames
                               installed into the host rootfs.

Algorithm:

    1. Walk per-artifact recipe-emit fragments (<artifact>.cdx.json
       next to each .deb / .whl / .gz). These are authoritative for
       SONiC-built artifacts.
    2. For each in-scope scope (host rootfs + each in-scope container),
       read the post-versions/ manifest written by sonic-build-hooks.
       Add observation components for any (name, version) not already
       covered by a recipe fragment.
    3. Optionally run the configured scanner (syft / trivy) as a wide
       net to catch transitive deps and language-ecosystem items the
       observation pass missed.
    4. Dedupe by (purl, arch): when multiple sources name the same
       component, recipe-emit wins (it has pedigree + patches data).
    5. Annotate top-level metadata with the build context.
    6. Emit one CycloneDX 1.6 document as the sibling of the .bin.

Failure mode: when ENABLE_SBOM=y the user has opted into SBOM emission
and a quietly-incomplete SBOM is worse than a build failure. The
aggregator validates that core inputs exist (host rootfs, declared
installer dockers, scanner binary) and exits non-zero if any are
missing. Set SBOM_STRICT=n to downgrade these to warnings for
debugging or one-off partial emits. Soft optional features
(SPDX conversion, provenance emit, license resolution) continue to
warn-and-continue.
"""

import datetime
import hashlib
import json
import os
import re
import subprocess
import sys
from typing import Any, Optional


def warn(msg: str) -> None:
    sys.stderr.write(f"[build_sbom.py] WARNING: {msg}\n")


def info(msg: str) -> None:
    sys.stderr.write(f"[build_sbom.py] {msg}\n")


def error(msg: str) -> None:
    sys.stderr.write(f"[build_sbom.py] ERROR: {msg}\n")


class SbomInputMissing(Exception):
    """Raised in strict mode when a required SBOM input is missing.

    The build_sbom recipe in slave.mk treats this as fatal — the user
    opted into SBOM generation via ENABLE_SBOM=y and we can't honor
    that opt-in if a core data source (host rootfs, installer docker,
    scanner binary) is absent.
    """


def check_required_inputs(
    target_path: str,
    target_machine: str,
    installer_dockers: list,
    scan_tool: str,
) -> None:
    """Validate that everything we declared we'd consume is actually
    present on disk. Raises SbomInputMissing on the first failure in
    strict mode; logs a warning and returns in lenient mode.

    Strict by default when ENABLE_SBOM=y; opt out with SBOM_STRICT=n
    for debugging or one-off partial SBOM emits.
    """
    strict = os.environ.get("SBOM_STRICT", "y").lower() == "y"
    problems: list[str] = []

    # 1. Host rootfs (sibling of target/, populated by build_debian.sh).
    #    Without it the SBOM is missing grub/kernel/host-utility/docker
    #    daemon visibility — the largest CVE surface on the .bin.
    fsroot = os.path.join(
        os.path.dirname(os.path.abspath(target_path)),
        f"fsroot-{target_machine}",
    )
    if not os.path.isdir(fsroot):
        problems.append(
            f"host rootfs not found at {fsroot}; cannot scan "
            f"host-installed packages (grub, kernel, docker daemon, "
            f"etc.). The build_sbom hook must run after build_debian.sh "
            f"and before fsroot cleanup."
        )

    # 2. Every declared installer docker must exist as a .gz in target/.
    #    These ship in the .bin; missing means a broken build that
    #    we should not pretend to inventory.
    for docker in installer_dockers:
        gz_path = os.path.join(target_path, docker)
        if not os.path.isfile(gz_path):
            problems.append(
                f"installer docker {docker} declared in "
                f"SBOM_INSTALLER_DOCKERS but missing at {gz_path}"
            )

    # 3. Scanner binary. Without it the SBOM loses CPE-tagged
    #    components and grype can't perform NVD matching downstream.
    if scan_tool and scan_tool not in ("none", "off", "skip"):
        scanner_bin = install_scanner(scan_tool)
        if not scanner_bin or not os.path.isfile(scanner_bin):
            problems.append(
                f"scanner '{scan_tool}' could not be installed via "
                f"scripts/install_sbom_tool.sh; without it the SBOM "
                f"loses host-rootfs and container-scan coverage"
            )

    if not problems:
        return

    msg = "SBOM input validation failed:\n  - " + "\n  - ".join(problems)
    if strict:
        error(msg)
        error("Set SBOM_STRICT=n to continue with a partial SBOM "
              "(not recommended).")
        raise SbomInputMissing(msg)
    else:
        warn(msg)
        warn("SBOM_STRICT=n; continuing with a partial SBOM.")


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------


def split_env_list(name: str) -> list:
    return [x.strip() for x in os.environ.get(name, "").split() if x.strip()]


def now_iso() -> str:
    epoch = os.environ.get("SOURCE_DATE_EPOCH")
    if epoch:
        try:
            return datetime.datetime.fromtimestamp(
                int(epoch), tz=datetime.timezone.utc
            ).strftime("%Y-%m-%dT%H:%M:%SZ")
        except Exception:
            pass
    return datetime.datetime.now(datetime.timezone.utc).strftime(
        "%Y-%m-%dT%H:%M:%SZ"
    )


def load_json(path: str) -> Optional[dict]:
    try:
        with open(path) as f:
            return json.load(f)
    except Exception:
        return None


def file_sha256(path: str) -> Optional[str]:
    try:
        h = hashlib.sha256()
        with open(path, "rb") as f:
            for chunk in iter(lambda: f.read(65536), b""):
                h.update(chunk)
        return h.hexdigest()
    except Exception:
        return None


def run(cmd: list, timeout: int = 600) -> tuple:
    """Returns (returncode, stdout, stderr)."""
    try:
        r = subprocess.run(
            cmd, capture_output=True, text=True, timeout=timeout, check=False
        )
        return r.returncode, r.stdout, r.stderr
    except subprocess.TimeoutExpired:
        return 124, "", "timeout"
    except Exception as e:
        return 1, "", str(e)


# ---------------------------------------------------------------------------
# Recipe-emit fragment collection
# ---------------------------------------------------------------------------


class FragmentIndex:
    """Walks target/ for <artifact>.cdx.json sidecars."""

    def __init__(self, target_path: str):
        self.target_path = target_path
        self.fragments: dict = {}   # filename → fragment-component
        self.all: list = []
        self._load()

    def _load(self):
        if not os.path.isdir(self.target_path):
            return
        for root, _, files in os.walk(self.target_path):
            # Skip the sbom-tools cache and per-scope tmp dirs we created.
            if "sbom-tools" in root or "sbom-tmp" in root:
                continue
            for fn in files:
                if fn.endswith(".cdx.json"):
                    # Only consume sidecar fragments — skip the final
                    # aggregate output if it has already been written
                    # in a prior run. The aggregate is named after the
                    # installer artifact (sonic-<machine>.bin /.swi
                    # / .img.gz), so match the 'sonic-' prefix plus
                    # any of the known installer extensions.
                    if fn.startswith("sonic-") and (
                        ".bin.cdx.json" in fn
                        or ".swi.cdx.json" in fn
                        or ".img.gz.cdx.json" in fn
                    ):
                        continue
                    doc = load_json(os.path.join(root, fn))
                    if not doc:
                        continue
                    meta_props = {
                        p.get("name"): p.get("value")
                        for p in doc.get("metadata", {}).get("properties", [])
                    }
                    if meta_props.get("sonic:fragment_kind") != "recipe-emit":
                        continue
                    for comp in doc.get("components", []):
                        # Index by the artifact filename so we can match
                        # against the installer's in-scope lists.
                        artifact_filename = None
                        for prop in comp.get("properties", []):
                            if prop.get("name") == "sonic:artifact_filename":
                                artifact_filename = prop.get("value")
                                break
                        if artifact_filename:
                            self.fragments[artifact_filename] = comp
                        self.all.append(comp)

    def for_filename(self, name: str) -> Optional[dict]:
        return self.fragments.get(name)


# ---------------------------------------------------------------------------
# Observation: post-versions/ manifests
# ---------------------------------------------------------------------------


_PKG_VER_RE = re.compile(r"^([^=]+)==(.+)$")


def parse_versions_file(path: str) -> list:
    """Reads a versions-deb-* or versions-py3-* manifest into [(name, ver)]."""
    out = []
    try:
        with open(path) as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith("#"):
                    continue
                m = _PKG_VER_RE.match(line)
                if m:
                    out.append((m.group(1).strip(), m.group(2).strip()))
    except Exception as e:
        warn(f"could not read {path}: {e}")
    return out


def find_post_versions(target_path: str, scope: str, kind: str,
                       arch: str) -> list:
    """Locate post-versions/versions-<kind>-*-<arch> for a scope.

    scope is e.g. 'host-image' or 'dockers/docker-fpm-frr'.
    kind is 'deb' or 'py3'.
    """
    base = os.path.join(target_path, "versions", scope, "post-versions")
    if not os.path.isdir(base):
        return []
    matches = []
    for fn in os.listdir(base):
        if fn.startswith(f"versions-{kind}-") and fn.endswith(f"-{arch}"):
            matches.append(os.path.join(base, fn))
    return sorted(matches)


def find_copyright_tarballs(target_path: str) -> list:
    """Find every per-scope copyrights.tar.gz under target/versions/."""
    return _find_tarballs(target_path, "copyrights.tar.gz")


def find_lockfile_tarballs(target_path: str) -> list:
    """Find every per-scope lockfiles.tar.gz under target/versions/."""
    return _find_tarballs(target_path, "lockfiles.tar.gz")


def _find_tarballs(target_path: str, name: str) -> list:
    base = os.path.join(target_path, "versions")
    if not os.path.isdir(base):
        return []
    out = []
    for root, _, files in os.walk(base):
        for fn in files:
            if fn == name:
                out.append(os.path.join(root, fn))
    return sorted(out)


def parse_lockfiles_for_scope(target_path: str, scope: str) -> list:
    """Parse only the lockfiles under a single scope dir
    (e.g. 'dockers/docker-ptf' or 'host-image'). Used by the
    per-container SBOM emit path so a container's sidecar SBOM only
    contains its own transitive lockfile deps."""
    tarball = os.path.join(
        target_path, "versions", scope, "post-versions", "lockfiles.tar.gz",
    )
    if not os.path.isfile(tarball):
        return []
    out_json = os.path.join(
        target_path,
        f"sbom-lockfile-components-{scope.replace('/', '-')}.json",
    )
    script = os.path.join(os.path.dirname(__file__),
                          "sbom_parse_lockfiles.py")
    rc, _, err = run(
        ["python3", script, "--output", out_json, "--lockfiles", tarball],
        timeout=300,
    )
    if rc != 0:
        warn(f"lockfile parser failed for scope {scope} "
             f"(rc={rc}): {err.strip()[:200]}")
        return []
    try:
        with open(out_json) as f:
            data = json.load(f)
        return data.get("components", [])
    except Exception as e:
        warn(f"could not read scoped lockfile parser output: {e}")
        return []


def parse_lockfiles(target_path: str) -> list:
    """Run scripts/sbom_parse_lockfiles.py over every harvested
    lockfiles.tar.gz; return the list of CycloneDX components."""
    tarballs = find_lockfile_tarballs(target_path)
    if not tarballs:
        return []
    out_json = os.path.join(target_path, "sbom-lockfile-components.json")
    script = os.path.join(os.path.dirname(__file__),
                          "sbom_parse_lockfiles.py")
    cmd = ["python3", script, "--output", out_json]
    for t in tarballs:
        cmd.extend(["--lockfiles", t])
    rc, _, err = run(cmd, timeout=600)
    if rc != 0:
        warn(f"lockfile parser failed (rc={rc}): {err.strip()[:200]}")
        return []
    try:
        with open(out_json) as f:
            data = json.load(f)
        return data.get("components", [])
    except Exception as e:
        warn(f"could not read lockfile parser output: {e}")
        return []


def _license_cache_dir() -> str:
    """Cache directory for license resolver output. Sibling to the
    scanner cache. Lives under target/ so `make reset` invalidates it
    automatically."""
    target_path = os.environ.get("TARGET_PATH", "target")
    d = os.path.join(target_path, "sbom-tools", "license-cache")
    try:
        os.makedirs(d, exist_ok=True)
    except OSError:
        pass
    return d


def _license_cache_key(tarballs: list) -> str:
    """SHA-256 over the sorted SHA-256s of every copyrights.tar.gz
    input. The resolver's output is a pure function of the input
    tarballs' content, so this is the right cache key — content
    drift in any tarball forces a re-resolve."""
    h = hashlib.sha256()
    for t in sorted(tarballs):
        sha = file_sha256(t)
        if sha:
            h.update(sha.encode())
    return h.hexdigest()


def resolve_licenses(target_path: str) -> dict:
    """Returns { pkg_name: spdx_expression }.

    Runs scripts/sbom_resolve_licenses.py against every copyrights.tar.gz
    found under target/. The resolver does the heavy lifting (DEP-5
    parsing, licensecheck fallback, SPDX mapping).

    Output is cached under target/sbom-tools/license-cache/<sha>.json
    keyed by a hash of the input tarballs' content. The 3 per-variant
    aggregator invocations (broadcom / broadcom-dnx / broadcom-legacy-th)
    share the same input copyrights tarballs — they're harvested
    per-container by collect_version_files, and most containers are
    identical across variants — so without caching the resolver was
    running ~3x and producing identical output each time. Cache lives
    under target/ so `make reset` invalidates naturally.
    """
    tarballs = find_copyright_tarballs(target_path)
    if not tarballs:
        return {}

    cache_key = _license_cache_key(tarballs)
    cache_file = os.path.join(_license_cache_dir(), f"{cache_key}.json")
    if os.path.isfile(cache_file):
        try:
            with open(cache_file) as f:
                data = json.load(f)
            return data.get("resolved", {})
        except Exception:
            pass

    out_json = os.path.join(target_path, "sbom-licenses.json")
    script = os.path.join(os.path.dirname(__file__),
                          "sbom_resolve_licenses.py")
    cmd = ["python3", script, "--output", out_json]
    for t in tarballs:
        cmd.extend(["--copyrights", t])
    rc, _, err = run(cmd, timeout=600)
    if rc != 0:
        warn(f"license resolver failed (rc={rc}): {err.strip()[:200]}")
        return {}
    try:
        with open(out_json) as f:
            data = json.load(f)
    except Exception as e:
        warn(f"could not read resolver output: {e}")
        return {}

    # Populate the cache for subsequent per-variant aggregator runs.
    if data.get("resolved"):
        try:
            tmp = cache_file + ".tmp"
            with open(tmp, "w") as f:
                json.dump(data, f)
            os.replace(tmp, cache_file)
        except Exception as e:
            warn(f"could not write license cache {cache_file}: {e}")

    return data.get("resolved", {})


def apply_licenses(components: list, license_map: dict) -> tuple:
    """Attach licenses[] to components that lack one.
    Returns (with_license_count, noassertion_count)."""
    resolved = 0
    noassertion = 0
    for c in components:
        if c.get("licenses"):
            resolved += 1
            continue
        name = (c.get("name") or "").lower()
        if not name:
            continue
        spdx = license_map.get(name)
        if not spdx:
            # Debian binary packages often have a source-package name that
            # carries the copyright. Don't have that here; just leave it
            # NOASSERTION.
            noassertion += 1
            continue
        if spdx == "NOASSERTION":
            c["licenses"] = [{"license": {"id": "NOASSERTION"}}]
            noassertion += 1
        else:
            c["licenses"] = [{"expression": spdx}]
            resolved += 1
    return resolved, noassertion


def observation_components_for_scope(
    target_path: str, scope: str, arch: str, supplier: str,
) -> list:
    """Emit observation-only components for everything in post-versions/."""
    components = []
    seen: set = set()

    for vfile in find_post_versions(target_path, scope, "deb", arch):
        for name, ver in parse_versions_file(vfile):
            key = (name, ver, arch)
            if key in seen:
                continue
            seen.add(key)
            comp: dict[str, Any] = {
                "bom-ref": f"pkg:deb/debian/{name}@{ver}?arch={arch}",
                "type": "library",
                "name": name,
                "version": ver,
                "purl": f"pkg:deb/debian/{name}@{ver}?arch={arch}",
                "supplier": {"name": supplier},
                "properties": [
                    {"name": "sonic:fragment_kind", "value": "observation"},
                    {"name": "sonic:scope", "value": scope},
                    {"name": "sonic:arch", "value": arch},
                ],
            }
            components.append(comp)

    for vfile in find_post_versions(target_path, scope, "py3", arch):
        for name, ver in parse_versions_file(vfile):
            norm = name.replace("_", "-").lower()
            key = ("pypi", norm, ver)
            if key in seen:
                continue
            seen.add(key)
            comp = {
                "bom-ref": f"pkg:pypi/{norm}@{ver}",
                "type": "library",
                "name": norm,
                "version": ver,
                "purl": f"pkg:pypi/{norm}@{ver}",
                "supplier": {"name": "PyPI"},
                "properties": [
                    {"name": "sonic:fragment_kind", "value": "observation"},
                    {"name": "sonic:scope", "value": scope},
                ],
            }
            components.append(comp)

    return components


# ---------------------------------------------------------------------------
# Scanner pass (syft / trivy)
# ---------------------------------------------------------------------------


def install_scanner(tool: str) -> Optional[str]:
    """Call scripts/install_sbom_tool.sh; return the path to the binary."""
    script = os.path.join(os.path.dirname(__file__), "install_sbom_tool.sh")
    rc, out, err = run([script, tool], timeout=300)
    if rc != 0:
        warn(f"install_sbom_tool.sh {tool} failed (rc={rc}): {err.strip()}")
        return None
    return out.strip() or None


def _scanner_cache_dir() -> str:
    """Cache directory for scanner outputs, sibling to the scanner binary.

    Lives under target/ so `make reset` (which wipes target/) invalidates
    the cache automatically — no stale entries across resets.
    """
    target_path = os.environ.get("TARGET_PATH", "target")
    d = os.path.join(target_path, "sbom-tools", "syft-cache")
    try:
        os.makedirs(d, exist_ok=True)
    except OSError:
        pass
    return d


def _scanner_cache_lookup(
    tool: str, fs_path: str,
) -> tuple:
    """Return (sha256, cached_components | None) for a file-based scan.

    Returns (None, None) if SHA-256 cannot be computed (e.g. file
    disappeared between exists-check and hash). A non-None sha with
    None components indicates a cache miss that the caller can fill
    via _scanner_cache_store after running the scanner.
    """
    sha = file_sha256(fs_path)
    if not sha:
        return None, None
    cache_file = os.path.join(_scanner_cache_dir(), f"{tool}-{sha}.json")
    if os.path.isfile(cache_file):
        try:
            with open(cache_file) as f:
                return sha, json.load(f)
        except Exception:
            pass
    return sha, None


def _scanner_cache_store(tool: str, sha: str, components: list) -> None:
    """Persist scanner output keyed by file SHA-256. Only called for
    non-empty results — an empty list could be a genuine zero-component
    scan or a scanner failure that returned []; caching the latter would
    poison subsequent variants."""
    if not sha or not components:
        return
    cache_file = os.path.join(_scanner_cache_dir(), f"{tool}-{sha}.json")
    try:
        tmp = cache_file + ".tmp"
        with open(tmp, "w") as f:
            json.dump(components, f)
        os.replace(tmp, cache_file)
    except Exception as e:
        warn(f"could not write scanner cache {cache_file}: {e}")


def run_scanner(scanner_bin: str, tool: str, scan_target: str) -> list:
    """Run scanner against a target; return components[] from output.

    scan_target may carry a syft scheme prefix (e.g. 'oci-archive:').
    SONiC's docker .gz files are gzipped OCI archives, and syft's
    archive readers don't pipe through gzip — so for the oci-archive
    case we transparently decompress to a temp file first.

    File-based scans (oci-archive:, fs paths) are cached by SHA-256 of
    the input file. Across the 3 ASIC variants of a single build
    (broadcom, broadcom-dnx, broadcom-legacy-th), the same docker .gz
    files are scanned 3 times by the aggregator's per-variant
    invocations; the cache short-circuits the 2nd and 3rd hits. The
    dir: scheme (host rootfs) is not cached — fsroot-<machine>/ differs
    per variant and a directory-tree hash would be expensive.
    """
    scheme = ""
    fs_path = scan_target
    if ":" in scan_target:
        scheme, fs_path = scan_target.split(":", 1)
    if not os.path.exists(fs_path):
        return []

    # Cache lookup for file-based scans only. dir: scans are not
    # cacheable because (a) hashing a directory tree is expensive and
    # (b) fsroot-<machine>/ differs per variant — no reuse anyway.
    cache_sha = None
    if scheme != "dir" and os.path.isfile(fs_path):
        cache_sha, cached = _scanner_cache_lookup(tool, fs_path)
        if cached is not None:
            return cached

    # syft's oci-archive reader doesn't handle gzip-wrapped tar.
    # Stream-decompress to a temp file for the duration of the scan.
    tmp_path = None
    if (tool == "syft" and scheme == "oci-archive"
            and _is_gzip(fs_path)):
        import gzip
        import tempfile
        with tempfile.NamedTemporaryFile(suffix=".tar", delete=False) as tf:
            tmp_path = tf.name
            with gzip.open(fs_path, "rb") as gz:
                while True:
                    chunk = gz.read(8 * 1024 * 1024)
                    if not chunk:
                        break
                    tf.write(chunk)
        scan_target = f"{scheme}:{tmp_path}"

    try:
        if tool == "syft":
            cmd = [scanner_bin, scan_target, "-o", "cyclonedx-json", "-q"]
        elif tool == "trivy":
            cmd = [scanner_bin, "fs", "--format", "cyclonedx", "--quiet",
                   scan_target]
        else:
            return []
        result = _run_scanner_inner(cmd, tool, scan_target)
        if cache_sha and result:
            _scanner_cache_store(tool, cache_sha, result)
        return result
    finally:
        if tmp_path and os.path.exists(tmp_path):
            os.unlink(tmp_path)


def _is_gzip(path: str) -> bool:
    """Cheap gzip-magic-bytes sniff."""
    try:
        with open(path, "rb") as f:
            return f.read(2) == b"\x1f\x8b"
    except Exception:
        return False


def _run_scanner_inner(cmd: list, tool: str, scan_target: str) -> list:
    rc, out, err = run(cmd, timeout=900)
    if rc != 0:
        warn(f"{tool} scan of {scan_target} failed (rc={rc}): "
             f"{err.strip()[:200]}")
        return []
    try:
        doc = json.loads(out)
    except Exception as e:
        warn(f"could not parse {tool} output for {scan_target}: {e}")
        return []
    comps = doc.get("components") or []
    for c in comps:
        c.setdefault("properties", []).append(
            {"name": "sonic:fragment_kind", "value": "scanner"}
        )
        c["properties"].append(
            {"name": "sonic:scanner", "value": tool}
        )
    return comps


# ---------------------------------------------------------------------------
# Merge with recipe-emit-wins dedupe
# ---------------------------------------------------------------------------


def _component_arch(c: dict) -> str:
    for p in c.get("properties") or []:
        if p.get("name") == "sonic:arch":
            return p.get("value") or ""
    return ""


# Suffixes that get added downstream of the recipe's filename version
# but before dpkg records the actually-installed version. We strip
# these (and a leading epoch like '1:') when computing a normalized
# dedupe key so the recipe-emit fragment 'openssh-server 10.0p1-7'
# matches the observation 'openssh-server 1:10.0p1-7+fips'. The pattern
# preserves the upstream-version prefix while eating the
# debian-build-system noise.
_VERSION_SUFFIX_RE = re.compile(
    r"(?:\+(?:fips|sonic(?:\.\d+)?|b\d+(?:sonic\d*)?|deb\d+u\d+))+$"
)
_VERSION_EPOCH_RE = re.compile(r"^\d+:")


def _normalize_version(v: str) -> str:
    v = _VERSION_EPOCH_RE.sub("", v)
    # Strip the suffix chain iteratively (handles +sonic.0+b1).
    while True:
        m = _VERSION_SUFFIX_RE.search(v)
        if not m:
            break
        v = v[: m.start()]
    return v


def _dedupe_keys(c: dict) -> list:
    """All keys a component should match against during dedupe.

    Returns the explicit PURL/bom-ref plus two normalized
    (name, version, arch) tuples — one with raw version (catches exact
    matches) and one with epoch+suffix stripped (catches the case where
    recipe-emit uses the filename version `10.0p1-7` and the eventual
    installed deb is `1:10.0p1-7+fips`). The two-key approach means:
      - Exact version matches still dedupe (same as before).
      - Different upstream versions of the same package stay distinct
        (e.g. bash 5.2.15 in bookworm vs 5.2.37 in trixie).
      - Only the build-system suffix drift collapses.
    """
    keys = []
    purl = c.get("purl")
    if purl:
        keys.append(("purl", purl))
    bom_ref = c.get("bom-ref")
    if bom_ref and bom_ref != purl:
        keys.append(("bom-ref", bom_ref))
    name = (c.get("name") or "").lower()
    version = c.get("version") or ""
    arch = _component_arch(c)
    if name and version:
        keys.append(("nva", name, version, arch))
        # Always emit the normalized key, even when normalize is a no-op,
        # so that a recipe-emit component (whose filename version usually
        # IS the normalized form) shares a key with the observation
        # component (whose dpkg version carries the +fips/+sonic/epoch
        # noise). Without this, the two never see each other.
        norm = _normalize_version(version) or version
        keys.append(("nva-norm", name, norm, arch))
    return keys


# Names that identify kernel-module packages whose runtime depends on
# the Linux kernel binary. Used to build the CycloneDX dependencies[]
# graph so consumers can trace ABI-incompatible-kernel risks.
_KERNEL_MODULE_PATTERNS = [
    re.compile(r"^opennsl-modules"),                # Broadcom XGS / DNX
    re.compile(r"^sx-kernel"),                      # Mellanox SX
    re.compile(r"^ionic-modules"),                  # AMD/Pensando ionic
    re.compile(r"^mrvlteralynx"),                   # Marvell Teralynx
    re.compile(r"^.*-dkms$"),                       # DKMS module debs (Bluefield etc.)
    re.compile(r"^sonic-platform-modules-"),        # vendor platform-modules
    re.compile(r"^platform-modules-"),              # micas-style
    re.compile(r"^saibcm-modules"),                 # Broadcom SAI kernel piece
    re.compile(r"^.*-kernel-modules$"),
]


def _is_kernel_module(name: str) -> bool:
    return any(p.match(name or "") for p in _KERNEL_MODULE_PATTERNS)


def _is_kernel_image(name: str) -> bool:
    """Match the primary linux-image fragment. Exclude debug/headers/kbuild."""
    n = name or ""
    if not n.startswith("linux-image-"):
        return False
    # Reject -dbg and -dbgsym variants; the primary kernel is what
    # modules link against at runtime.
    if n.endswith("-dbg") or n.endswith("-dbgsym") or "-unsigned-dbg" in n:
        return False
    return True


def build_dependency_graph(components: list) -> list:
    """Return a CycloneDX dependencies[] array recording the edges
    we can derive from recipe-emit metadata.

    Three edge classes are emitted into a single unscoped graph
    (CycloneDX 1.6 dependencies[] doesn't distinguish build-time vs
    runtime; analytics that need the split should read the
    sonic:build_depends / sonic:runtime_depends properties off the
    components themselves):

      1. Kernel-module -> kernel-image. Out-of-tree modules (Broadcom
         OPENNSL, Mellanox SX, etc.) are built against a specific
         kernel ABI; recording the edge lets consumers reason about
         kernel-ABI-compatible upgrade paths and CVE blast radius.

      2. SONiC-built .deb -> declared build/runtime deps. Read from
         sonic:build_depends / sonic:runtime_depends string properties
         that sbom_fragment.py copies out of the recipe's $(pkg)_DEPENDS
         and $(pkg)_RDEPENDS makefile variables. The property strings
         are space-separated .deb filenames that we resolve back to the
         sibling fragment's bom-ref via sonic:artifact_filename. Filenames
         that don't resolve (almost always upstream packages from
         Debian for which we have no recipe-emit fragment) are recorded
         as a sonic:unresolved_deps property on the component for audit;
         they don't appear in the graph.

      3. SONiC-built .deb -> per-binary language deps shipped inside.
         sbom_fragment.py emits each Rust crate / Go module / Python
         dist-info entry as a recipe-emit-{rust,go,python} component
         carrying sonic:source_deb=<deb filename>; we reverse that
         into an edge from the .deb's bom-ref to the language-dep's
         bom-ref so a consumer can walk swss_*.deb -> tokio@1.x or
         sonic-gnmi_*.deb -> github.com/openconfig/gnmi@v0.10 without
         parsing properties.
    """
    # filename -> bom-ref lookup over the merged component set. The
    # same resolution path is used by all three edge classes that need
    # to refer to a sibling component by its on-disk artifact name.
    filename_to_ref: dict = {}
    for c in components:
        ref = c.get("bom-ref")
        if not ref:
            continue
        for prop in c.get("properties", []) or []:
            if prop.get("name") == "sonic:artifact_filename":
                fn = prop.get("value")
                if fn:
                    filename_to_ref[fn] = ref
                break

    # Accumulate as ref -> set(dependsOn refs) so multiple edge
    # classes that converge on the same source component merge into
    # a single CycloneDX dependencies[] entry per ref.
    edges: dict = {}

    # (1) kernel-module -> kernel-image
    kernel_refs = [
        c["bom-ref"] for c in components
        if _is_kernel_image(c.get("name")) and c.get("bom-ref")
    ]
    if kernel_refs:
        kernel_set = set(kernel_refs)
        for c in components:
            if not _is_kernel_module(c.get("name") or ""):
                continue
            ref = c.get("bom-ref")
            if ref:
                edges.setdefault(ref, set()).update(kernel_set)

    # (2) declared build/runtime deps (resolved to sibling fragments)
    for c in components:
        ref = c.get("bom-ref")
        if not ref:
            continue
        build_dep_str = ""
        runtime_dep_str = ""
        for prop in c.get("properties", []) or []:
            n = prop.get("name")
            if n == "sonic:build_depends":
                build_dep_str = prop.get("value", "") or ""
            elif n == "sonic:runtime_depends":
                runtime_dep_str = prop.get("value", "") or ""
        if not (build_dep_str or runtime_dep_str):
            continue
        dep_filenames = set(build_dep_str.split()) | set(runtime_dep_str.split())
        resolved: set = set()
        unresolved: set = set()
        for fn in dep_filenames:
            if not fn:
                continue
            tgt = filename_to_ref.get(fn)
            if tgt and tgt != ref:
                resolved.add(tgt)
            elif not tgt:
                unresolved.add(fn)
        if resolved:
            edges.setdefault(ref, set()).update(resolved)
        if unresolved:
            props = c.setdefault("properties", [])
            existing = None
            for p in props:
                if p.get("name") == "sonic:unresolved_deps":
                    existing = p
                    break
            joined = " ".join(sorted(unresolved))
            if existing is not None:
                merged = sorted(set(
                    (existing.get("value", "") or "").split()
                ) | unresolved)
                existing["value"] = " ".join(merged)
            else:
                props.append({
                    "name": "sonic:unresolved_deps",
                    "value": joined,
                })

    # (3) per-binary language deps (recipe-emit-{rust,go,python}) ->
    # their owning .deb. sbom_fragment.py attaches sonic:source_deb to
    # every such component; we look up the .deb's bom-ref and reverse
    # the attribution into a dependsOn edge.
    for c in components:
        crate_ref = c.get("bom-ref")
        if not crate_ref:
            continue
        source_deb = None
        is_lang_dep = False
        for prop in c.get("properties", []) or []:
            n = prop.get("name")
            v = prop.get("value", "")
            if n == "sonic:source_deb":
                source_deb = v
            elif n == "sonic:fragment_kind" and v.startswith("recipe-emit-") and v != "recipe-emit":
                is_lang_dep = True
        if not (is_lang_dep and source_deb):
            continue
        deb_ref = filename_to_ref.get(source_deb)
        if deb_ref and deb_ref != crate_ref:
            edges.setdefault(deb_ref, set()).add(crate_ref)

    deps = [
        {"ref": ref, "dependsOn": sorted(targets)}
        for ref, targets in edges.items()
        if targets
    ]
    return deps


def merge_components(*sources: list) -> list:
    """Dedupe by (purl) and (name, version, arch). Sources are passed in
    PRIORITY order; first occurrence wins for the base record. But for
    components dropped by dedupe, we *promote* their CPE list onto the
    winner — recipe-emit fragments carry rich SONiC provenance but no
    CPE, while syft (lower priority) produces CPEs that grype needs for
    NVD-based CVE matching when distro detection isn't available.
    """
    seen: dict = {}    # dedupe key -> winner index in `out`
    out: list = []
    for src in sources:
        for c in src:
            keys = _dedupe_keys(c)
            if not keys:
                continue
            winner_idx = next((seen[k] for k in keys if k in seen), None)
            if winner_idx is not None:
                _promote_cpe(out[winner_idx], c)
                continue
            for k in keys:
                seen[k] = len(out)
            out.append(c)
    return out


def _promote_cpe(winner: dict, loser: dict) -> None:
    """Move CPE data from a deduped-out record onto the winner.

    Grype's NVD matcher relies on the `cpe` field; recipe-emit
    fragments don't produce one, but syft does. Without this
    promotion, every recipe-built Debian/sonic package loses the CPE
    that would have let grype match it against the Debian/NVD CVE
    feeds.
    """
    cpe = loser.get("cpe")
    if cpe and not winner.get("cpe"):
        winner["cpe"] = cpe
    cpes = loser.get("cpes")
    if cpes and not winner.get("cpes"):
        winner["cpes"] = cpes


# ---------------------------------------------------------------------------
# Top-level orchestration
# ---------------------------------------------------------------------------


def _container_main(container_filename: str) -> int:
    """Produce a sidecar SBOM for a single container archive.

    Used for test containers (docker-ptf, docker-sonic-mgmt) that are
    out-of-scope for the .bin SBOM but still need their own
    vulnerability/provenance surface. Also used for in-scope
    containers, where it complements the merged .bin SBOM with a
    container-level view useful for diffing and per-container
    security scanning.

    Output: target/<container_filename>.sbom.cdx.json
    """
    target_path = os.environ.get("TARGET_PATH", "target")
    arch = os.environ.get(
        "CONFIGURED_ARCH",
        subprocess.run(
            ["dpkg", "--print-architecture"],
            capture_output=True, text=True, check=False,
        ).stdout.strip() or "amd64",
    )
    platform = os.environ.get("CONFIGURED_PLATFORM", "")
    scan_tool = os.environ.get("SBOM_SCAN_TOOL", "syft")
    vcc = os.environ.get("SONIC_VERSION_CONTROL_COMPONENTS", "")

    gz_path = os.path.join(target_path, container_filename)
    if not os.path.isfile(gz_path):
        info(f"container archive not found: {gz_path}; skipping.")
        return 0

    cname = container_filename.replace(".gz", "").replace("-dbg", "")
    out_path = os.path.join(
        target_path, f"{container_filename}.sbom.cdx.json"
    )
    info(f"Building per-container SBOM for {container_filename}")

    # Recipe-emit fragment for this container (if any).
    fragments = FragmentIndex(target_path)
    frag = fragments.for_filename(container_filename)
    recipe_components = [frag] if frag else []

    # Synthesize a container-typed parent component if no recipe
    # fragment exists (out-of-scope test containers go this path).
    if frag:
        container_comp = frag
    else:
        version = os.environ.get("SONIC_IMAGE_VERSION", "0.0.0")
        container_comp = {
            "bom-ref": f"pkg:oci/{cname}@{version}?arch={arch}",
            "type": "container",
            "name": cname,
            "version": version,
            "purl": f"pkg:oci/{cname}@{version}?arch={arch}",
            "properties": [
                {"name": "sonic:fragment_kind", "value": "observation"},
                {"name": "sonic:arch", "value": arch},
            ],
        }
        sha = file_sha256(gz_path)
        if sha:
            container_comp["hashes"] = [
                {"alg": "SHA-256", "content": sha}
            ]

    # Observation: only this container's scope.
    scope = f"dockers/{cname}"
    obs = observation_components_for_scope(
        target_path, scope, arch, "Debian",
    )
    info(f"Container observation: {len(obs)} components")

    # Lockfile-derived: only this container's scope.
    lockfile_components = parse_lockfiles_for_scope(target_path, scope)
    info(f"Lockfile-derived: {len(lockfile_components)} components")

    # Scanner: this single .gz. SONiC's docker .gz files are OCI
    # archives (not docker-archive tarballs), so syft needs the
    # 'oci-archive:' scheme.
    scanner_components: list = []
    if scan_tool and scan_tool not in ("none", "off", "skip"):
        scanner_bin = install_scanner(scan_tool)
        if scanner_bin:
            if scan_tool == "syft":
                target_spec = f"oci-archive:{gz_path}"
            else:
                target_spec = gz_path
            scanner_components = run_scanner(
                scanner_bin, scan_tool, target_spec,
            )
    info(f"Scanner cross-check: {len(scanner_components)} components")

    all_components = merge_components(
        recipe_components,
        [container_comp],
        obs,
        lockfile_components,
        scanner_components,
    )
    info(f"Final merged: {len(all_components)} unique components")

    # License resolution (per-scope copyrights tarball + fallback to
    # the full set if the per-scope tarball is missing).
    if os.environ.get("SBOM_INCLUDE_LICENSES", "y") == "y":
        license_map = resolve_licenses(target_path)
        if license_map:
            resolved, noassertion = apply_licenses(
                all_components, license_map,
            )
            total = len(all_components)
            pct = (100.0 * resolved / total) if total else 0.0
            info(f"License resolution: {resolved}/{total} resolved "
                 f"({pct:.1f}%); {noassertion} NOASSERTION")

    sbom: dict[str, Any] = {
        "bomFormat": "CycloneDX",
        "specVersion": "1.6",
        "version": 1,
        "metadata": {
            "timestamp": now_iso(),
            "tools": [{
                "vendor": "SONiC",
                "name": "build_sbom.py",
                "version": "1.0",
            }],
            "component": {
                "type": "container",
                "bom-ref": container_comp.get("bom-ref", cname),
                "name": cname,
                "version": container_comp.get("version", "0.0.0"),
            },
            "properties": [
                {"name": "sonic:platform", "value": platform},
                {"name": "sonic:arch", "value": arch},
                {"name": "sonic:scope_kind",
                 "value": "single-container"},
                {"name": "sonic:container", "value": cname},
                {"name": "sonic:version_control_components",
                 "value": vcc},
                {"name": "sonic:scan_tool", "value": scan_tool},
            ],
        },
        "components": sorted(
            all_components, key=lambda c: c.get("bom-ref", "")
        ),
    }

    deps = build_dependency_graph(all_components)
    if deps:
        sbom["dependencies"] = sorted(deps, key=lambda d: d.get("ref", ""))

    try:
        with open(out_path, "w") as f:
            json.dump(sbom, f, indent=2, sort_keys=True)
        info(f"SBOM written: {out_path}")
        info(f"Component count: {len(all_components)}")
    except Exception as e:
        warn(f"could not write {out_path}: {e}")

    # SPDX conversion (optional)
    sbom_format = os.environ.get("SBOM_FORMAT", "cyclonedx").lower()
    if sbom_format in ("spdx", "both"):
        # Reuse emit_spdx but with the per-container output path; the
        # function derives the spdx path from target_machine, so we
        # call cyclonedx-cli directly here for the per-container case.
        spdx_path = out_path.removesuffix(".cdx.json") + ".spdx.json"
        _convert_to_spdx(out_path, spdx_path)

    return 0


def _convert_to_spdx(cdx_path: str, spdx_path: str) -> None:
    """Shared SPDX conversion helper. Used by both bin and container modes."""
    script = os.path.join(os.path.dirname(__file__), "install_sbom_tool.sh")
    rc, out, err = run([script, "cyclonedx-cli"], timeout=300)
    if rc != 0:
        warn(f"could not install cyclonedx-cli (rc={rc}): {err.strip()[:200]}")
        return
    binary = out.strip()
    if not binary or not os.path.isfile(binary):
        warn(f"cyclonedx-cli binary not found at {binary!r}")
        return
    rc, _, err = run(
        [binary, "convert",
         "--input-file", cdx_path,
         "--output-file", spdx_path,
         "--output-format", "spdxjson"],
        timeout=300,
    )
    if rc != 0:
        warn(f"cyclonedx-cli convert failed (rc={rc}): {err.strip()[:200]}")
        return
    info(f"SPDX written: {spdx_path}")


def main() -> int:
    if os.environ.get("ENABLE_SBOM", "n") != "y":
        info("ENABLE_SBOM=n; skipping.")
        return 0

    import argparse
    ap = argparse.ArgumentParser(description="SONiC SBOM aggregator")
    ap.add_argument(
        "--container", metavar="FILENAME",
        help="Emit a single-container SBOM (e.g. 'docker-ptf.gz'). "
             "Output goes to target/<filename>.sbom.cdx.json. In this "
             "mode, host rootfs observation and the .bin scope filter "
             "are bypassed; only the named container's data is "
             "aggregated. Used to produce sidecar SBOMs for test "
             "containers (docker-ptf, docker-sonic-mgmt) that are "
             "out-of-scope for the .bin SBOM but still need their own "
             "vulnerability surface documented.",
    )
    args = ap.parse_args()

    if args.container:
        return _container_main(args.container)

    target_path = os.environ.get("TARGET_PATH", "target")
    target_machine = os.environ.get("TARGET_MACHINE", "generic")
    arch = os.environ.get("CONFIGURED_ARCH",
                          subprocess.run(
                              ["dpkg", "--print-architecture"],
                              capture_output=True, text=True, check=False,
                          ).stdout.strip() or "amd64")
    platform = os.environ.get("CONFIGURED_PLATFORM", "")
    scan_tool = os.environ.get("SBOM_SCAN_TOOL", "syft")
    vcc = os.environ.get("SONIC_VERSION_CONTROL_COMPONENTS", "")

    installer_dockers = split_env_list("SBOM_INSTALLER_DOCKERS")

    # Strict-mode validation: fail loudly when the user enabled SBOM
    # generation but a critical input is missing. Better to break the
    # build than ship a quietly-incomplete SBOM (e.g. host rootfs
    # missing → no grub/kernel/docker-daemon visibility).
    try:
        check_required_inputs(
            target_path, target_machine, installer_dockers, scan_tool,
        )
    except SbomInputMissing:
        return 1

    # Derive output filenames from the actual artifact basename
    # (e.g. 'sonic-broadcom.bin', 'sonic-aboot-broadcom.swi',
    # 'sonic-vs.img.gz') so siblings line up regardless of installer
    # format. Falls back to '<machine>.bin' for any caller that
    # didn't plumb SBOM_TARGET_ARTIFACT through.
    artifact_basename = os.environ.get(
        "SBOM_TARGET_ARTIFACT", f"sonic-{target_machine}.bin"
    )
    out_path = os.path.join(target_path, f"{artifact_basename}.cdx.json")
    info(f"Building SBOM for {artifact_basename} "
         f"(platform={platform}, arch={arch})")

    fragments = FragmentIndex(target_path)
    info(f"Loaded {len(fragments.all)} recipe-emit fragments from {target_path}/")

    # ---- Recipe-emit components (authoritative for SONiC-built things) ----
    recipe_components = list(fragments.all)

    # ---- Observation components for host rootfs ----
    obs_host = observation_components_for_scope(
        target_path, "host-image", arch, "Debian",
    )
    info(f"Host rootfs observation: {len(obs_host)} components")

    # ---- Per-container observation + container identity ----
    container_components: list = []
    obs_containers: list = []
    for docker in installer_dockers:
        # The docker filename in the installer list may be e.g.
        # 'docker-fpm-frr.gz' or 'docker-fpm-frr-dbg.gz'.
        gz_path = os.path.join(target_path, docker)
        cname = docker.replace(".gz", "").replace("-dbg", "")

        # Build a container-typed parent component (use recipe fragment
        # if present, otherwise synthesize).
        frag = fragments.for_filename(docker)
        if frag:
            container_comp = frag
        else:
            container_comp = {
                "bom-ref": f"pkg:oci/{cname}@{os.environ.get('SONIC_IMAGE_VERSION', '0.0.0')}?arch={arch}",
                "type": "container",
                "name": cname,
                "version": os.environ.get("SONIC_IMAGE_VERSION", "0.0.0"),
                "properties": [
                    {"name": "sonic:fragment_kind", "value": "observation"},
                    {"name": "sonic:arch", "value": arch},
                ],
            }
        if os.path.isfile(gz_path):
            sha = file_sha256(gz_path)
            if sha:
                container_comp.setdefault("hashes", []).append(
                    {"alg": "SHA-256", "content": sha}
                )
        container_components.append(container_comp)

        # In-container observation set.
        scope = f"dockers/{cname}"
        obs_containers.extend(observation_components_for_scope(
            target_path, scope, arch, "Debian",
        ))

    info(f"Container observation: {len(obs_containers)} components "
         f"across {len(installer_dockers)} containers")

    # ---- Lockfile-derived components (Rust/Go/npm transitive deps) ----
    lockfile_components = parse_lockfiles(target_path)
    info(f"Lockfile-derived: {len(lockfile_components)} components")

    # ---- Optional scanner cross-check ----
    scanner_components: list = []
    if scan_tool and scan_tool not in ("none", "off", "skip"):
        scanner_bin = install_scanner(scan_tool)
        if scanner_bin:
            info(f"Running {scan_tool} from {scanner_bin} as cross-check")
            # Scan the host rootfs. SONiC stages this as fsroot-<machine>/
            # at the repo root (a sibling of target/), populated by
            # build_debian.sh and packed into a squashfs by build_image.sh.
            # We scan the directory tree directly — syft's `dir:` source
            # picks up Debian packages, embedded Go modules (for docker
            # daemon binaries etc.), grub stage binaries, and so on.
            # This is the source-of-truth scan for everything that ships
            # outside docker containers (kernel/grub/host utilities/the
            # docker daemon itself).
            fsroot = os.path.join(
                os.path.dirname(os.path.abspath(target_path)),
                f"fsroot-{target_machine}",
            )
            if os.path.isdir(fsroot):
                if scan_tool == "syft":
                    target_spec = f"dir:{fsroot}"
                else:
                    target_spec = fsroot
                info(f"Scanning host rootfs: {fsroot}")
                scanner_components.extend(
                    run_scanner(scanner_bin, scan_tool, target_spec)
                )
            else:
                info(f"host rootfs not found at {fsroot}; "
                     f"skipping host-rootfs scan")
            # Scan each in-scope container archive. SONiC's docker .gz
            # files are OCI archives (not docker-archive tarballs), so
            # syft needs the 'oci-archive:' scheme.
            for docker in installer_dockers:
                gz_path = os.path.join(target_path, docker)
                if os.path.isfile(gz_path):
                    if scan_tool == "syft":
                        target_spec = f"oci-archive:{gz_path}"
                    else:
                        target_spec = gz_path
                    scanner_components.extend(
                        run_scanner(scanner_bin, scan_tool, target_spec)
                    )

    info(f"Scanner cross-check: {len(scanner_components)} components")

    # ---- Merge (recipe-emit wins, then observation, then lockfile,
    #              then scanner) ----
    # Lockfile sits between observation and scanner: it gives more
    # precise identity than scanner's binary detection (it has crate
    # hashes from the lockfile), but observation/recipe-emit win when
    # they describe the same component because they carry richer
    # SONiC-specific provenance.
    all_components = merge_components(
        recipe_components,
        container_components,
        obs_host,
        obs_containers,
        lockfile_components,
        scanner_components,
    )

    info(f"Final merged: {len(all_components)} unique components")

    # ---- License resolution ----
    if os.environ.get("SBOM_INCLUDE_LICENSES", "y") == "y":
        license_map = resolve_licenses(target_path)
        if license_map:
            resolved, noassertion = apply_licenses(all_components, license_map)
            total = len(all_components)
            pct = (100.0 * resolved / total) if total else 0.0
            info(f"License resolution: {resolved}/{total} "
                 f"resolved ({pct:.1f}%); {noassertion} NOASSERTION")
        else:
            info("License resolution: no copyrights tarballs found")

    # ---- Build the final BOM ----
    sbom: dict[str, Any] = {
        "bomFormat": "CycloneDX",
        "specVersion": "1.6",
        "version": 1,
        "metadata": {
            "timestamp": now_iso(),
            "tools": [{
                "vendor": "SONiC",
                "name": "build_sbom.py",
                "version": "1.0",
            }],
            "component": {
                "type": "operating-system",
                "bom-ref": f"sonic-{target_machine}",
                "name": f"sonic-{target_machine}",
                "version": os.environ.get("SONIC_IMAGE_VERSION", "0.0.0"),
            },
            "properties": [
                {"name": "sonic:platform", "value": platform},
                {"name": "sonic:arch", "value": arch},
                {"name": "sonic:target_machine", "value": target_machine},
                {"name": "sonic:scan_tool", "value": scan_tool},
                {"name": "sonic:version_control_components", "value": vcc},
                {"name": "sonic:recipe_fragments_loaded",
                 "value": str(len(fragments.all))},
                {"name": "sonic:installer_dockers",
                 "value": " ".join(installer_dockers)},
            ],
        },
        "components": sorted(
            all_components, key=lambda c: c.get("bom-ref", "")
        ),
    }

    # Build the dependencies[] graph: kernel modules -> kernel image.
    deps = build_dependency_graph(all_components)
    if deps:
        sbom["dependencies"] = sorted(deps, key=lambda d: d.get("ref", ""))
        info(f"Dependencies: {len(deps)} kernel-module edges")

    try:
        with open(out_path, "w") as f:
            json.dump(sbom, f, indent=2, sort_keys=True)
        info(f"SBOM written: {out_path}")
        info(f"Component count: {len(all_components)}")
    except Exception as e:
        warn(f"could not write {out_path}: {e}")
        return 0

    # ---- SPDX export ----
    sbom_format = os.environ.get("SBOM_FORMAT", "cyclonedx").lower()
    if sbom_format in ("spdx", "both"):
        emit_spdx(out_path, target_path, artifact_basename)

    # ---- SLSA / in-toto provenance ----
    emit_provenance(out_path, target_path, artifact_basename)

    return 0


def emit_provenance(cdx_path: str, target_path: str,
                    artifact_basename: str) -> None:
    """Invoke scripts/sbom_emit_provenance.py to produce a sibling
    .intoto.json document. Failure is logged but does not break the
    build. artifact_basename is the actual installer filename
    (.bin / .swi / .img.gz / etc.)."""
    artifact_path = os.path.join(target_path, artifact_basename)
    if not os.path.isfile(artifact_path):
        info(f"no installer artifact at {artifact_path}; "
             f"skipping provenance emit")
        return
    script = os.path.join(os.path.dirname(__file__),
                          "sbom_emit_provenance.py")
    rc, _, err = run(
        ["python3", script, "--bin", artifact_path, "--sbom", cdx_path],
        timeout=120,
    )
    if rc != 0:
        warn(f"provenance emit failed (rc={rc}): {err.strip()[:200]}")
        return
    info(f"Provenance written: {artifact_path}.intoto.json")


def emit_spdx(cdx_path: str, target_path: str,
              artifact_basename: str) -> None:
    """Convert the CycloneDX SBOM to SPDX via cyclonedx-cli."""
    script = os.path.join(os.path.dirname(__file__), "install_sbom_tool.sh")
    rc, out, err = run([script, "cyclonedx-cli"], timeout=300)
    if rc != 0:
        warn(f"could not install cyclonedx-cli (rc={rc}): {err.strip()[:200]}")
        return
    binary = out.strip()
    if not binary or not os.path.isfile(binary):
        warn(f"cyclonedx-cli binary not found at {binary!r}")
        return

    spdx_path = os.path.join(target_path, f"{artifact_basename}.spdx.json")
    rc, _, err = run(
        [binary, "convert",
         "--input-file", cdx_path,
         "--output-file", spdx_path,
         "--output-format", "spdxjson"],
        timeout=300,
    )
    if rc != 0:
        warn(f"cyclonedx-cli convert failed (rc={rc}): {err.strip()[:200]}")
        return
    info(f"SPDX written: {spdx_path}")


if __name__ == "__main__":
    sys.exit(main())
