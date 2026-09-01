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
import uuid
from typing import Any, Optional

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import sbom_purl  # noqa: E402  (needs the path set above)


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


def serial_number_for(doc: dict) -> str:
    """A stable identifier for this BOM document, derived from it.

    The point of a serial number is to let anything produced against
    this BOM — a vulnerability report, a VEX statement, another BOM that
    includes it — say which document it came from, rather than pointing
    at a filename that means nothing once the file has moved.

    CycloneDX says a BOM SHOULD get a fresh serial number on every
    generation. We deliberately do not: README.sbom.md guarantees that
    two byte-identical builds of the same source produce byte-identical
    SBOMs, which is why SOURCE_DATE_EPOCH is threaded all the way into
    the container. A random identifier would quietly retire that
    guarantee. Deriving it from the content keeps both properties —
    different documents get different serial numbers, and the same
    document gets the same one every time.
    """
    payload = {k: v for k, v in doc.items() if k != "serialNumber"}
    digest = hashlib.sha256(
        json.dumps(payload, sort_keys=True, separators=(",", ":")).encode()
    ).hexdigest()
    return uuid.uuid5(uuid.NAMESPACE_URL, digest).urn


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


def run(cmd: list, timeout: int = 600, env: Optional[dict] = None) -> tuple:
    """Returns (returncode, stdout, stderr).

    env, when given, is overlaid on the current environment rather than
    replacing it — a scanner still needs PATH, HOME and its own cache
    variables to work.
    """
    try:
        r = subprocess.run(
            cmd, capture_output=True, text=True, timeout=timeout, check=False,
            env={**os.environ, **env} if env else None,
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
    distro: Optional[tuple] = None,
) -> list:
    """Emit observation-only components for everything in post-versions/.

    When ``distro`` is a ``(id, version_id)`` tuple (e.g. ``("ubuntu",
    "24.04")``), deb PURLs are namespaced to that distro and carry a
    ``distro=`` qualifier so grype selects the right OS advisory feed.
    When ``None`` the legacy ``pkg:deb/debian/`` form is preserved.
    """
    components = []
    seen: set = set()

    if distro and distro[0]:
        deb_ns = distro[0]
        deb_distro = f"{distro[0]}-{distro[1]}" if distro[1] else ""
        deb_supplier = distro[0].capitalize()
    else:
        deb_ns = "debian"
        deb_distro = ""
        deb_supplier = supplier

    for vfile in find_post_versions(target_path, scope, "deb", arch):
        for name, ver in parse_versions_file(vfile):
            key = (name, ver, arch)
            if key in seen:
                continue
            seen.add(key)
            deb_purl = sbom_purl.build(
                "deb", name, ver, namespace=deb_ns,
                qualifiers={"arch": arch, "distro": deb_distro},
            )
            comp: dict[str, Any] = {
                "bom-ref": deb_purl,
                "type": "library",
                "name": name,
                "version": ver,
                "purl": deb_purl,
                "supplier": {"name": deb_supplier},
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
            py_purl = sbom_purl.build("pypi", norm, ver)
            comp = {
                "bom-ref": py_purl,
                "type": "library",
                "name": norm,
                "version": ver,
                "purl": py_purl,
                "supplier": {"name": "PyPI"},
                "properties": [
                    {"name": "sonic:fragment_kind", "value": "observation"},
                    {"name": "sonic:scope", "value": scope},
                ],
            }
            components.append(comp)

    return components


# ---------------------------------------------------------------------------
# Validation
# ---------------------------------------------------------------------------


def validate_document(path: str) -> None:
    """Check the document we just wrote against the CycloneDX schema.

    Nothing did this before, and the consequence was not theoretical: every
    SBOM this script has ever produced was rejected by
    `cyclonedx validate --input-version v1_6`, because one unschema'd field on
    a pedigree patch invalidates the whole file. It went unnoticed for as long
    as it did precisely because the tools we happen to use are lenient — syft,
    grype and trivy all read it happily — so the only thing that would ever
    have said so is the check that was missing.

    A warning rather than a failure by default: this runs at the end of a build
    that has already taken hours, and refusing to finish over a document
    somebody can still read is the wrong trade. `SBOM_STRICT=1` makes it fatal,
    which is what CI should set — the point of writing the reason down is that
    the next person does not have to rediscover it.
    """
    cli = install_scanner("cyclonedx-cli")
    if not cli:
        warn("cyclonedx-cli is unavailable, so the document was not validated")
        return
    # --fail-on-errors, or this is decorative. cyclonedx-cli prints
    # "BOM is not valid." and **exits 0** without it, so a check written the
    # obvious way passes on a document the same command just rejected. That is
    # the same shape as the bug it is here to catch: a green result that means
    # "nothing failed" rather than "it was checked".
    rc, out, err = run(
        [cli, "validate", "--input-file", path,
         "--input-version", "v1_6", "--fail-on-errors"],
        timeout=300,
    )
    if rc == 0:
        info("SBOM validates against CycloneDX 1.6")
        return
    detail = (err or out or "").strip()
    message = (
        f"the SBOM does not validate against CycloneDX 1.6: "
        f"{detail[:2000]}"
    )
    if os.environ.get("SBOM_STRICT") == "1":
        error(message)
        raise SystemExit(1)
    warn(message)
    warn("set SBOM_STRICT=1 to make this fail the build")


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


# Bumped whenever a change alters what a scan returns for unchanged
# input. The cache is keyed by the SHA-256 of the scanned file, so
# without this an entry written before the file components were
# dropped would be replayed forever — the input did not change, only
# our reading of it.
SCANNER_CACHE_VERSION = "v2"


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
    cache_file = os.path.join(
        _scanner_cache_dir(), f"{tool}-{SCANNER_CACHE_VERSION}-{sha}.json")
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
    cache_file = os.path.join(
        _scanner_cache_dir(), f"{tool}-{SCANNER_CACHE_VERSION}-{sha}.json")
    try:
        tmp = cache_file + ".tmp"
        with open(tmp, "w") as f:
            json.dump(components, f)
        os.replace(tmp, cache_file)
    except Exception as e:
        warn(f"could not write scanner cache {cache_file}: {e}")


def run_scanner(scanner_bin: str, tool: str, scan_target: str,
                scope: str = "") -> list:
    """Run scanner against a target; return components[] from output.

    ``scope`` records where the scan looked — "host-image" or
    "dockers/<name>" — and is stamped onto every component that comes
    back. syft is invoked once per container and once for the host
    rootfs, so it already knows which filesystem a package came from;
    without stamping it here the results are appended to one flat list
    and that knowledge is lost, which is what left the containment
    graph empty. Stamping happens after the cache, never before: the
    cache is keyed by the digest of the scanned file, and the same
    archive can be reached under more than one name.

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
            return _scoped(cached, scope)

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
        scanner_env = None
        if tool == "syft":
            cmd = [scanner_bin, scan_target, "-o", "cyclonedx-json", "-q"]
            # syft defaults to file.metadata.selection=owned-by-package,
            # which emitted a `type: file` component — a path and two
            # digests — for every file on the image. That was 57,332 of
            # the 65,870 components in a broadcom .bin SBOM, 24.8 MB of
            # its 54 MB, and none of it was usable: a file component has
            # no purl, no version and no CPE, so no vulnerability feed
            # can match it, and the CycloneDX encoder drops syft's
            # file-ownership relationships, so nothing said which
            # package a file belonged to either.
            #
            # Turning the cataloger off rather than filtering afterwards
            # also skips hashing every file on the image, which is the
            # expensive part of the scan.
            scanner_env = {"SYFT_FILE_METADATA_SELECTION": "none"}
        elif tool == "trivy":
            cmd = [scanner_bin, "fs", "--format", "cyclonedx", "--quiet",
                   scan_target]
        else:
            return []
        result = _run_scanner_inner(cmd, tool, scan_target, scanner_env)
        if cache_sha and result:
            _scanner_cache_store(tool, cache_sha, result)
        return _scoped(result, scope)
    finally:
        if tmp_path and os.path.exists(tmp_path):
            os.unlink(tmp_path)


def _scoped(comps: list, scope: str) -> list:
    """Stamp sonic:scope onto every component in a scan result."""
    if scope:
        for c in comps:
            _add_scope(c, scope)
    return comps


def _is_gzip(path: str) -> bool:
    """Cheap gzip-magic-bytes sniff."""
    try:
        with open(path, "rb") as f:
            return f.read(2) == b"\x1f\x8b"
    except Exception:
        return False


def _run_scanner_inner(cmd: list, tool: str, scan_target: str,
                       env: Optional[dict] = None) -> list:
    rc, out, err = run(cmd, timeout=900, env=env)
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
    # Belt and braces for the file components the syft config above
    # already suppresses: trivy is a supported scanner too and has its
    # own defaults, and a future scanner release could change its mind.
    # A `type: file` component cannot carry a finding, so there is no
    # arrangement under which we want one.
    comps = [c for c in comps if c.get("type") != "file"]
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


def _purl_type(purl) -> str:
    """The ecosystem a package URL names — `deb`, `cargo`, `npm`.

    Empty for a component with no package URL, which groups those
    together; they are matched on name and version as before.
    """
    if not purl or not purl.startswith("pkg:"):
        return ""
    return purl[len("pkg:"):].split("/", 1)[0].lower()


def _dedupe_keys(c: dict) -> list:
    """All keys a component should match against during dedupe.

    Returns the explicit PURL/bom-ref plus two normalized
    (name, version) tuples — one with raw version (catches exact
    matches) and one with epoch+suffix stripped (catches the case where
    recipe-emit uses the filename version `10.0p1-7` and the eventual
    installed deb is `1:10.0p1-7+fips`). The two-key approach means:
      - Exact version matches still dedupe (same as before).
      - Different upstream versions of the same package stay distinct
        (e.g. bash 5.2.15 in bookworm vs 5.2.37 in trixie).
      - Only the build-system suffix drift collapses.

    Architecture is deliberately not part of the key. It used to be,
    read from the `sonic:arch` property, and it silently defeated the
    whole (name, version) key: only recipe-emit and observation
    fragments carry that property, so every syft component compared as
    architecture "" and never matched the recipe fragment describing
    the same .deb. That is what left one package in the SBOM twice
    under two package URLs.

    Restoring it is not a matter of reading the architecture from
    somewhere else, because no producer here knows it. The recipe takes
    it from the .deb's filename, which says `amd64` for a
    `symcrypt-openssl` whose control file says `all`; the observation
    stamps CONFIGURED_ARCH on everything, which says `amd64` for
    `Architecture: all` packages like ifupdown2 and initramfs-tools;
    only syft reads dpkg. Three guesses that disagree cannot be a
    component of identity.

    Nor is it needed for one. A CycloneDX document produced here
    describes a single image built for a single target architecture,
    and dpkg will not install one name at one version twice within it.
    If SONiC ever emits a multi-arch SBOM, the prerequisite is all
    three producers reading dpkg's own `Architecture:` field — not
    reinstating a key two of them fill in by guessing.
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
    # The ecosystem is part of identity. A Rust crate and the Debian package
    # built from it share a name and a version and are not the same component:
    # they are described by different producers, resolve against different
    # advisory feeds, and merging them silently drops one identity. Two SONiC
    # programs did exactly that once architecture left the key — the crate and
    # the .deb had nothing else keeping them apart. It does not weaken the
    # dedupe this key exists for, where both records are `deb` and differ only
    # in namespace.
    ecosystem = _purl_type(purl)
    if name and version:
        keys.append(("nv", ecosystem, name, version))
        # Always emit the normalized key, even when normalize is a no-op,
        # so that a recipe-emit component (whose filename version usually
        # IS the normalized form) shares a key with the observation
        # component (whose dpkg version carries the +fips/+sonic/epoch
        # noise). Without this, the two never see each other.
        norm = _normalize_version(version) or version
        keys.append(("nv-norm", ecosystem, name, norm))
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


def container_name(docker_filename: str) -> str:
    """'docker-fpm-frr[-dbg].gz' -> 'docker-fpm-frr'.

    The container component, its sonic:scope label and the scanner
    invocation all have to agree on this, and they each used to spell
    it out separately.
    """
    return docker_filename.replace(".gz", "").replace("-dbg", "")


def _property(c: dict, name: str) -> str:
    """One property off a component, or empty."""
    for prop in c.get("properties", []) or []:
        if prop.get("name") == name:
            return prop.get("value") or ""
    return ""


def _scope_values(c: dict) -> set:
    """Every scope a component was observed in.

    Multi-valued because containment genuinely is: libc is in every
    container, and recording only the first one seen would make the
    other twenty look as though they did not ship it.
    """
    for p in c.get("properties") or []:
        if p.get("name") == "sonic:scope":
            return set((p.get("value") or "").split())
    return set()


def _add_scope(c: dict, scope: str) -> None:
    """Record that a component was observed in ``scope``.

    Space-separated, matching sonic:build_depends and
    sonic:unresolved_deps. No scope value contains a space.
    """
    if not scope:
        return
    props = c.setdefault("properties", [])
    for p in props:
        if p.get("name") == "sonic:scope":
            vals = set((p.get("value") or "").split())
            vals.add(scope)
            p["value"] = " ".join(sorted(vals))
            return
    props.append({"name": "sonic:scope", "value": scope})


# The harvested lockfile paths are rooted at the source tree, and the source
# trees recipes record are not.
#
# `sonic:lockfile` says `sonic/src/sonic-gnmi/go.sum` because that is where the
# path sits inside the harvest tarball; `sonic:src_path` says `src/sonic-gnmi`,
# relative to the repository. Comparing them directly matched **0 of 952**
# lockfile dependencies — the whole attribution was inert while looking
# entirely reasonable, since "nothing matched" and "nothing to match" produce
# the same empty result and neither says anything.
#
# The other 658 paths are `usr/...`: lockfiles shipped *inside* an installed
# package rather than built here, which have no source tree in this repository
# and are left alone.
_SOURCE_TREE_ROOT = "sonic/"


def _lockfile_repo_path(found_in: str) -> str:
    """A harvested lockfile path, relative to the repository."""
    found_in = (found_in or "").strip("/")
    if found_in.startswith(_SOURCE_TREE_ROOT):
        return found_in[len(_SOURCE_TREE_ROOT):]
    return found_in


# A lockfile harvested from the build slave, for something outside the SONiC
# source tree, describes the toolchain rather than the image.
#
# `versions/build/log-*/lockfiles.tar.gz` is collected from the container that
# *compiles* SONiC, and it holds two unrelated things. Paths under `sonic/` are
# our own source trees, and their dependencies really are linked into the
# binaries we ship. Paths under anything else — `usr/share/go-1.19/src/go.sum`,
# a vscode extension's `package-lock.json` inside a ruby gem — belong to the
# compiler and its friends, which are not installed in the image at all: a
# real broadcom build ships no golang package, and no shipped scope's harvest
# contains a single `usr/` path.
#
# Left in `components` they are asserted to be image contents, and CycloneDX
# reads a component with no `scope` as `required`. On a real image that is 658
# components carrying 489 vulnerability matches about a compiler nobody runs.
#
# `scope: "excluded"` does not fix it: **measured against grype 0.112.0 and
# 0.118.0, the same component reports the same 20 matches whether it is marked
# excluded, optional, required or nothing at all.** The marking is correct for
# a human and inert for the scanner.
#
# So they move to `formulation`, which is the section CycloneDX 1.5 added for
# exactly this — how the thing was built, as against what it contains. Nothing
# is discarded: a build-chain compromise is a real question (xz-utils was
# introduced through a build system, not through source), and it stays
# answerable from the same document. Measured the same way: in `formulation`,
# grype reports **0**, and the document validates as CycloneDX 1.6.
def split_build_tooling(components: list) -> tuple:
    """Return (what the image contains, what built it)."""
    contained, tooling = [], []
    for c in components:
        found_in = (_property(c, "sonic:lockfile") or "").strip("/")
        if found_in and not found_in.startswith(_SOURCE_TREE_ROOT):
            tooling.append(c)
        else:
            contained.append(c)
    return contained, tooling


def build_dependency_graph(components: list, root_ref: str = "",
                           root_contains_all: bool = False,
                           installed: Optional[set] = None) -> list:
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

      4. Containment: the image -> the containers it installs, and each
         container -> the packages inside it. Without this the document
         had no root at all: nothing descended from the image component,
         the container components appeared in no edge, and 7,569 of
         8,538 packages sat with no edge in either direction. A consumer
         could see that a package was vulnerable but not which container
         shipped it, which is the first thing anyone triaging asks.

         Placement comes from sonic:scope, which every observation and
         scanner component now carries. A component we cannot place is
         left unrooted rather than attached to the image on the grounds
         that it must be somewhere — that would report a containment
         nobody observed.

    ``installed`` names the containers the image actually installs.
    Every container fragment in target/ reaches this function —
    slave.mk emits one for each docker it saves, including the test
    containers that ship in no .bin — so without it the image claims
    to contain whatever else happened to be built alongside it.

    ``root_contains_all`` is for the per-container documents, where
    every component is in the one container by construction.
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

    # (5) a dependency read out of a lockfile belongs to whatever was built
    # from the source tree that lockfile sits in.
    #
    # A Go module is not something an image depends on. It is compiled into a
    # program, and the program is what the go.sum sits beside — which is the
    # first question anybody asks of one of these, and the graph could not
    # answer it. Measured on a real image: of 3,011 dependencies read from
    # lockfiles, 1,760 were attributed to a repository or a container and
    # **1,251 were not** — 350 hung off the image, asserting the image depends
    # on a Go module, and 901 appeared in no edge at all.
    #
    # Matched by path: the lockfile's own path against the source tree each
    # recipe recorded building from. Where the two do not line up nothing is
    # emitted — the dependency stays unrooted rather than being attached to
    # something on the grounds that it must belong somewhere, which is the
    # rule the containment pass already holds to.
    lockfile_owner: list = []
    for c in components:
        ref = c.get("bom-ref")
        src = _property(c, "sonic:src_path")
        if ref and src:
            lockfile_owner.append((src.strip("/"), ref))
    # Longest source path first, so a nested tree wins over the tree above it.
    lockfile_owner.sort(key=lambda pair: len(pair[0]), reverse=True)

    placed_from_lockfile = 0
    for c in components:
        ref = c.get("bom-ref")
        found_in = _lockfile_repo_path(_property(c, "sonic:lockfile"))
        if not ref or not found_in:
            continue
        for src, owner in lockfile_owner:
            if owner == ref:
                continue
            if src and (found_in + "/").startswith(src + "/"):
                edges.setdefault(owner, set()).add(ref)
                placed_from_lockfile += 1
                break
    if placed_from_lockfile:
        info(f"Lockfile dependencies attributed to what was built beside "
             f"them: {placed_from_lockfile}")

    # (4) containment: image -> containers -> packages
    if root_ref:
        container_ref_for: dict = {}
        for c in components:
            if c.get("type") == "container" and c.get("bom-ref") and c.get("name"):
                if installed is not None and c["name"] not in installed:
                    continue
                container_ref_for[c["name"]] = c["bom-ref"]

        for cref in container_ref_for.values():
            if cref != root_ref:
                edges.setdefault(root_ref, set()).add(cref)

        for c in components:
            ref = c.get("bom-ref")
            if not ref or ref == root_ref or c.get("type") == "container":
                continue
            if root_contains_all:
                edges.setdefault(root_ref, set()).add(ref)
                continue
            # A dependency read out of a lockfile is not something the image
            # contains. The scope says which filesystem the lockfile was
            # harvested from, not that the image depends on a Go module, and
            # treating the two the same is what put 350 of them directly under
            # the image. Class (5) attributes these where it can; where it
            # cannot they stay unrooted, which is honest.
            from_lockfile = _property(c, "sonic:lockfile") != ""
            for scope in _scope_values(c):
                if scope == "host-image":
                    if not from_lockfile:
                        edges.setdefault(root_ref, set()).add(ref)
                elif scope.startswith("dockers/"):
                    cref = container_ref_for.get(scope[len("dockers/"):])
                    if cref and cref != ref:
                        edges.setdefault(cref, set()).add(ref)

    deps = [
        {"ref": ref, "dependsOn": sorted(targets)}
        for ref, targets in edges.items()
        if targets
    ]
    return deps


def check_dependency_graph(deps: list, components: list, root_ref: str,
                           installed: Optional[set] = None) -> list:
    """Problems with a finished dependencies[] graph, as readable lines.

    Nothing downstream validates this. grype reads components[] and
    ignores the graph entirely, so a document that says the image
    contains a container it never shipped is published, attested and
    consumed without a single build going red. These are the three
    ways the graph can lie that are checkable from the document alone.
    """
    problems = []
    known = {c["bom-ref"] for c in components if c.get("bom-ref")}
    if root_ref:
        known.add(root_ref)
    container_names = {
        c["bom-ref"]: c.get("name")
        for c in components
        if c.get("type") == "container" and c.get("bom-ref")
    }

    for d in deps:
        ref = d.get("ref")
        if ref not in known:
            problems.append(f"{ref} depends on things but is not in the document")
        for tgt in d.get("dependsOn", []):
            if tgt not in known:
                problems.append(f"{ref} -> {tgt}, which is not in the document")
            if tgt == ref:
                problems.append(f"{ref} depends on itself")
            if (installed is not None and ref == root_ref
                    and tgt in container_names
                    and container_names[tgt] not in installed):
                problems.append(
                    f"image claims to contain container "
                    f"{container_names[tgt]}, which it does not install"
                )
    return problems


def merge_components(*sources: list) -> list:
    """Dedupe by (purl) and (name, version). Sources are passed in
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
                winner = out[winner_idx]
                _promote_cpe(winner, c)
                _promote_scope(winner, c)
                _promote_provenance(winner, c)
                for k in _promote_version(winner, c):
                    seen.setdefault(k, winner_idx)
                # Promotion can rewrite the winner's identifier, and a
                # later source may spell the package that new way.
                for k in _promote_qualifiers(winner, c):
                    seen.setdefault(k, winner_idx)
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


# What a deduped-out record can tell us that the winner cannot.
#
# Only syft read a real filesystem, so only syft knows these. The winner is
# the recipe-emit fragment, which knows what was *built* and not what it was
# installed as.
_PROMOTED_QUALIFIERS = (
    # Which Debian release the package was installed on. Grype selects an OS
    # advisory feed with it.
    "distro",
    # Which Debian *source* package a binary package came from — libssl3 from
    # openssl, apt-utils from apt. Debian advisories are published against the
    # source package, so without this a binary package cannot be matched to
    # the advisory that covers it. 535 packages in a real image carry one.
    "upstream",
)


def _promote_qualifiers(winner: dict, loser: dict) -> list:
    """Carry a deduped-out record's purl qualifiers onto the winner.

    Returns the dedupe keys the winner newly answers to, so the caller
    can register them.

    The recipe-emit fragment wins the dedupe because it knows what was
    built, but it names the package `pkg:deb/sonic/openssl` — a SONiC
    rebuild is not the Debian package, and saying so is the point of
    the namespace. What it cannot know is anything about the installed
    system: which Debian release this went onto, or which source package
    Debian built it from.

    Before these two records merged they both survived into the
    document, so those qualifiers reached grype on the syft copy. Now
    that they merge, dropping the loser wholesale takes them with it —
    which is the same reasoning that already moves the CPE, applied to
    every qualifier only the filesystem knows rather than to `distro`
    alone. Promoting just `distro` was the first version of this, and it
    silently cost every one of those 535 packages its source-package
    name.
    """
    purl = winner.get("purl")
    if not purl:
        return []
    have = sbom_purl.qualifiers_of(purl)
    want = sbom_purl.qualifiers_of(loser.get("purl") or "")
    promoted = purl
    for key in _PROMOTED_QUALIFIERS:
        if want.get(key) and not have.get(key):
            promoted = sbom_purl.with_qualifier(promoted, key, want[key])
    if promoted == purl:
        return []
    if winner.get("bom-ref") == purl:
        winner["bom-ref"] = promoted
    winner["purl"] = promoted
    return [("purl", promoted)]


# Fields a deduped-out record can state that the winner leaves empty.
#
# Who shipped a package is one of the minimum elements an SBOM is expected to
# carry, and the recipe-emit fragment does not know it — only a reader of the
# installed package does. 577 components lost it when these records began
# merging.
_PROMOTED_FIELDS = (
    "publisher",
    "supplier",
    "licenses",
    "externalReferences",
    "hashes",
    # What a package was forked from, and what its carried patches fix. 35
    # packages lost it — bash, frr, flashrom among them — which are precisely
    # the SONiC-patched ones this document exists to describe.
    "pedigree",
)


def _promote_provenance(winner: dict, loser: dict) -> None:
    """Carry fields the winner leaves empty across the dedupe."""
    for field in _PROMOTED_FIELDS:
        value = loser.get(field)
        if value and not winner.get(field):
            winner[field] = value


def _promote_version(winner: dict, loser: dict) -> list:
    """State the version that was installed, not the one built.

    Returns the dedupe keys the winner newly answers to.

    These two records merged because their versions normalize to the
    same thing, which is the case `_normalize_version` exists for: the
    recipe knows `openssh-server 10.0p1-7` from a filename, and dpkg
    installed `1:10.0p1-7+fips`. While both records survived into the
    document that difference cost nothing, because the installed
    spelling was still in there somewhere. Once they merge, the
    recipe-emit winner's filename version is the only version stated,
    and the document says the image carries stock openssh where it
    carries the FIPS rebuild — and drops the Debian epoch for eight
    other packages, which is part of the version and what a version
    comparison orders on first.

    So the more specific spelling wins: an epoch beats none, and a build
    suffix beats none. Both are refinements of the same version rather
    than a different one — that is precisely why the two records were
    allowed to merge — so taking the longer spelling cannot pick a
    different package.

    Only the version moves. Everything else about the winner is the
    recipe's, which is the point of it winning.
    """
    ours, theirs = winner.get("version"), loser.get("version")
    if not ours or not theirs or ours == theirs:
        return []
    # Only where normalization is what merged them. An exact purl match
    # means these are the same spelling already.
    if _normalize_version(ours) != _normalize_version(theirs):
        return []

    def specificity(v):
        return (
            bool(_VERSION_EPOCH_RE.match(v)),
            bool(_VERSION_SUFFIX_RE.search(v)),
            len(v),
        )

    if specificity(theirs) <= specificity(ours):
        return []
    winner["version"] = theirs
    purl = winner.get("purl")
    if not purl:
        return []
    promoted = sbom_purl.with_version(purl, theirs)
    if promoted == purl:
        return []
    if winner.get("bom-ref") == purl:
        winner["bom-ref"] = promoted
    winner["purl"] = promoted
    return [("purl", promoted)]


def _promote_scope(winner: dict, loser: dict) -> None:
    """Carry a deduped-out record's scopes onto the winner.

    A recipe-emit fragment describes what was built and outranks a
    scanner observation of the same package, but only the observation
    knows where it ended up. Dropping the loser wholesale would discard
    exactly the placement the containment graph is built from.
    """
    for scope in _scope_values(loser):
        _add_scope(winner, scope)


# ---------------------------------------------------------------------------
# Container distro detection (needed for deb/OS-package vuln matching)
# ---------------------------------------------------------------------------


def _parse_os_release(text: str) -> Optional[tuple]:
    """Parse /etc/os-release content into ``(ID, VERSION_ID)``.

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


def detect_container_distro(gz_path: str) -> Optional[tuple]:
    """Return the container's ``(distro_id, version_id)`` by reading
    ``/etc/os-release`` from the image archive, e.g. ``("ubuntu", "24.04")``.

    The env var ``SBOM_CONTAINER_DISTRO`` (``id-version``, e.g.
    ``ubuntu-24.04``) overrides detection. Returns None when the distro
    can't be determined; callers then skip emitting distro metadata and
    the SBOM keeps its legacy (distro-less) shape.
    """
    override = os.environ.get("SBOM_CONTAINER_DISTRO", "").strip()
    if override:
        # Accept "id" or "id-version"; split on the LAST '-' so distro
        # ids that themselves contain hyphens (e.g. "cbl-mariner-2.0")
        # keep their id intact. Ignore a value with an empty id.
        if "-" in override:
            oid, over = override.rsplit("-", 1)
        else:
            oid, over = override, ""
        if oid:
            return (oid, over)
        warn(f"ignoring malformed SBOM_CONTAINER_DISTRO={override!r}")

    import shutil
    import tarfile
    import tempfile

    def _search(tf) -> Optional[tuple]:
        for member in tf:
            # /etc/os-release is commonly a symlink to /usr/lib/os-release,
            # so don't require a regular file (symlinks report isfile()==
            # False) and match both paths. extractfile() follows in-tar
            # symlinks and returns the target content.
            nm = member.name.lstrip("./").rstrip("/")
            if nm.endswith("etc/os-release") or nm.endswith("usr/lib/os-release"):
                ex = tf.extractfile(member)
                if ex is not None:
                    got = _parse_os_release(
                        ex.read().decode("utf-8", "replace")
                    )
                    if got:
                        return got
        return None

    try:
        with tarfile.open(gz_path, "r:*") as outer:
            for m in outer:
                nm = m.name.lstrip("./").rstrip("/")
                # os-release directly present in a flattened rootfs archive
                # (may be a symlink to usr/lib/os-release)
                if nm.endswith("etc/os-release") or nm.endswith(
                    "usr/lib/os-release"
                ):
                    ex = outer.extractfile(m)
                    if ex is not None:
                        got = _parse_os_release(
                            ex.read().decode("utf-8", "replace")
                        )
                        if got:
                            return got
                    continue
                if not m.isfile():
                    continue
                # nested image layers (docker-archive: */layer.tar or
                # *.tar; OCI: blobs/sha256/*) — peek inside for os-release
                looks_like_layer = (
                    nm.endswith(".tar")
                    or nm.endswith("layer.tar")
                    or "blobs/sha256" in nm
                    or nm.endswith(".tar.gz")
                )
                if not looks_like_layer:
                    continue
                ex = outer.extractfile(m)
                if ex is None:
                    continue
                # Spill the layer to a temp file, then parse with random
                # access. Opening a nested tar directly off the gzip outer
                # stream (mode="r|*") silently finds no members, and
                # buffering whole layers in memory risks OOM on large
                # images, so stream to disk (bounded memory) and seek.
                tmp = tempfile.NamedTemporaryFile(
                    prefix="sbom-distro-", suffix=".tar",
                    dir=os.path.dirname(os.path.abspath(gz_path)) or None,
                    delete=False,
                )
                try:
                    shutil.copyfileobj(ex, tmp, 1024 * 1024)
                    tmp.close()
                    with tarfile.open(tmp.name, "r:*") as inner:
                        got = _search(inner)
                except tarfile.TarError:
                    got = None
                finally:
                    try:
                        os.unlink(tmp.name)
                    except OSError:
                        pass
                if got:
                    return got
    except Exception as e:  # defensive: never fail the build on this
        warn(f"container distro detection failed for {gz_path}: {e}")
    return None


def operating_system_component(distro_id: str, version_id: str) -> dict:
    """Build a CycloneDX ``operating-system`` component carrying the
    distro. grype keys OS-package (deb/rpm/apk) matching off the
    ``syft:distro:*`` properties, so without such a component every deb
    package is silently skipped.
    """
    ver = version_id or "unknown"
    props = [{"name": "syft:distro:id", "value": distro_id}]
    if version_id:
        props.append(
            {"name": "syft:distro:versionID", "value": version_id}
        )
    return {
        "bom-ref": f"os:{distro_id}-{ver}",
        "type": "operating-system",
        "name": distro_id,
        "version": ver,
        "properties": props,
    }


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

    cname = container_name(container_filename)
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

    # Detect the container's OS/distro so grype can vuln-match deb/OS
    # packages. Without a distro the SBOM carries no OS context and
    # grype silently skips every deb component, scanning only language
    # packages. See README.sbom.md "Vulnerability scanning".
    distro = detect_container_distro(gz_path)
    if distro:
        info(f"Container distro: {distro[0]} {distro[1]}")
    else:
        warn("container distro undetected; deb/OS packages will not be "
             "vuln-matched by grype for this container SBOM")

    # Observation: only this container's scope.
    scope = f"dockers/{cname}"
    obs = observation_components_for_scope(
        target_path, scope, arch, "Debian", distro=distro,
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
                scanner_bin, scan_tool, target_spec, scope=scope,
            )
    info(f"Scanner cross-check: {len(scanner_components)} components")

    all_components = merge_components(
        recipe_components,
        [container_comp],
        obs,
        lockfile_components,
        scanner_components,
    )

    # Emit an operating-system component so grype can identify the
    # distro and match deb/OS packages against the right advisory feed.
    if distro:
        all_components.append(
            operating_system_component(distro[0], distro[1])
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

    # What built this is not what it contains. Split before the graph is
    # built, so no edge points at something the document no longer lists.
    all_components, build_tooling = split_build_tooling(all_components)
    if build_tooling:
        info(f"Build environment: {len(build_tooling)} components moved to "
             f"formulation; they are not in the image")

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

    if build_tooling:
        # What compiled it, kept and clearly labelled rather than asserted to
        # be inside it. Nothing is discarded: a build-chain compromise is a
        # real question and stays answerable from the same document.
        sbom["formulation"] = [{
            "bom-ref": "sonic:build-environment",
            "components": sorted(
                build_tooling, key=lambda c: c.get("bom-ref", "")
            ),
        }]

    # Everything in a per-container document is in that container by
    # construction, so the root contains all of it.
    container_root = container_comp.get("bom-ref", cname)
    deps = build_dependency_graph(
        all_components,
        root_ref=container_root,
        root_contains_all=True,
    )
    if deps:
        sbom["dependencies"] = sorted(deps, key=lambda d: d.get("ref", ""))
        for problem in check_dependency_graph(
            deps, all_components, container_root,
        ):
            warn(f"dependency graph: {problem}")

    # Derived from the finished document, so it must be set last.
    sbom["serialNumber"] = serial_number_for(sbom)

    try:
        with open(out_path, "w") as f:
            json.dump(sbom, f, indent=2, sort_keys=True)
        info(f"SBOM written: {out_path}")
        info(f"Component count: {len(all_components)}")
    except Exception as e:
        warn(f"could not write {out_path}: {e}")

    if os.path.exists(out_path):
        validate_document(out_path)

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
        cname = container_name(docker)

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
                    run_scanner(scanner_bin, scan_tool, target_spec,
                                scope="host-image")
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
                        run_scanner(scanner_bin, scan_tool, target_spec,
                                    scope=f"dockers/{container_name(docker)}")
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
    # What built this is not what it contains. Split before the graph is
    # built, so no edge points at something the document no longer lists.
    all_components, build_tooling = split_build_tooling(all_components)
    if build_tooling:
        info(f"Build environment: {len(build_tooling)} components moved to "
             f"formulation; they are not in the image")

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

    if build_tooling:
        # What compiled it, kept and clearly labelled rather than asserted to
        # be inside it. Nothing is discarded: a build-chain compromise is a
        # real question and stays answerable from the same document.
        sbom["formulation"] = [{
            "bom-ref": "sonic:build-environment",
            "components": sorted(
                build_tooling, key=lambda c: c.get("bom-ref", "")
            ),
        }]

    # Build the dependencies[] graph, rooted at the image component so
    # the document describes one tree rather than a pile of fragments.
    root_ref = f"sonic-{target_machine}"
    installed = {container_name(d) for d in installer_dockers}
    deps = build_dependency_graph(
        all_components, root_ref=root_ref, installed=installed,
    )
    if deps:
        sbom["dependencies"] = sorted(deps, key=lambda d: d.get("ref", ""))
        edge_count = sum(len(d.get("dependsOn", [])) for d in deps)
        placed = set()
        for d in deps:
            placed.update(d.get("dependsOn", []))
        unplaced = sum(
            1 for c in all_components
            if c.get("bom-ref") and c["bom-ref"] not in placed
        )
        info(f"Dependencies: {len(deps)} refs, {edge_count} edges; "
             f"{unplaced} components not placed under any parent")
        for problem in check_dependency_graph(
            deps, all_components, root_ref, installed,
        ):
            warn(f"dependency graph: {problem}")

    # Derived from the finished document, so it must be set last.
    sbom["serialNumber"] = serial_number_for(sbom)

    try:
        with open(out_path, "w") as f:
            json.dump(sbom, f, indent=2, sort_keys=True)
        info(f"SBOM written: {out_path}")
        info(f"Component count: {len(all_components)}")
    except Exception as e:
        warn(f"could not write {out_path}: {e}")
        return 0

    validate_document(out_path)

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
