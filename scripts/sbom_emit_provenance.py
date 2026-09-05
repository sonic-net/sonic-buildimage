#!/usr/bin/env python3
"""
sbom_emit_provenance.py — emit an unsigned SLSA v1.0 provenance
attestation alongside the SBOM.

Output: `target/<artifact>.intoto.json` — sibling of whatever
installer this run produced (.bin / .swi / .img.gz / etc.).

The attestation follows in-toto Statement v1
(https://in-toto.io/Statement/v1) wrapping a SLSA Provenance v1
predicate (https://slsa.dev/provenance/v1) and answers the question:
"how was this binary built?"

It is intentionally **unsigned**. Release engineering signs both the
SBOM and the .intoto.json at publish time with whatever tool their
policy requires (cosign, openssl, etc.). The build slave never sees
production signing keys. See README.sbom.md "Attestation and signing"
for the recommended workflow.

The output is reproducible: two byte-identical builds produce
byte-identical .intoto.json documents, as long as SOURCE_DATE_EPOCH
is set or no timestamps are populated.
"""

import argparse
import datetime
import hashlib
import json
import os
import re
import subprocess
import sys
from typing import Optional


def warn(msg: str) -> None:
    sys.stderr.write(f"[sbom_emit_provenance.py] WARNING: {msg}\n")


def info(msg: str) -> None:
    if os.environ.get("SBOM_DEBUG", "n") == "y":
        sys.stderr.write(f"[sbom_emit_provenance.py] {msg}\n")


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------


def run(cmd: list, cwd: Optional[str] = None, timeout: int = 60) -> Optional[str]:
    try:
        r = subprocess.run(
            cmd, cwd=cwd, capture_output=True, text=True,
            timeout=timeout, check=False,
        )
        if r.returncode == 0:
            return r.stdout.strip()
    except Exception:
        pass
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


def reproducible_timestamp() -> Optional[str]:
    """Return SOURCE_DATE_EPOCH as ISO8601 if set; else None.

    None means "this attestation will not be byte-identical across
    independent builds because we don't have a reproducible build
    timestamp". We omit timestamps entirely in that case.
    """
    epoch = os.environ.get("SOURCE_DATE_EPOCH")
    if not epoch:
        return None
    try:
        return datetime.datetime.fromtimestamp(
            int(epoch), tz=datetime.timezone.utc
        ).strftime("%Y-%m-%dT%H:%M:%SZ")
    except Exception:
        return None


# ---------------------------------------------------------------------------
# Slave image identity (the "builder" in SLSA terms)
# ---------------------------------------------------------------------------


def slave_image_digest(image: str) -> Optional[dict]:
    """Return {repo, tag, sha256_digest} for a locally-loaded image.

    Falls back to None if docker is unavailable or the image isn't
    loaded (which is normal when this script runs in a fresh slave
    after collect_docker_version_files has tagged things differently).
    """
    out = run(["docker", "image", "inspect", "--format",
               "{{.Id}}\t{{.RepoDigests}}", image])
    if not out:
        return None
    fields = out.split("\t", 1)
    iid = fields[0] if fields else ""
    # iid is like "sha256:abc..."; strip the prefix to match SLSA digest map.
    digest = iid.split(":", 1)[1] if iid.startswith("sha256:") else iid
    return {
        "name": image,
        "digest": {"sha256": digest} if digest else {},
    }


def find_slave_images() -> list:
    """Locate the slave docker images used in this build.

    Sources: target/versions/build/build-sonic-slave-*/ directory names
    point at the slave kind (bookworm / trixie). We use them to derive
    the image refs and ask docker for their digests.
    """
    target_path = os.environ.get("TARGET_PATH", "target")
    build_dir = os.path.join(target_path, "versions", "build")
    out: list = []
    if not os.path.isdir(build_dir):
        return out
    for name in sorted(os.listdir(build_dir)):
        if not name.startswith("build-sonic-slave-"):
            continue
        slave_kind = name[len("build-"):]  # e.g. "sonic-slave-bookworm"
        # Find the loaded image by querying docker for any tag matching.
        # If multiple, prefer the longest tag (per-user variant).
        list_out = run(["docker", "images",
                        "--format", "{{.Repository}}:{{.Tag}}\t{{.ID}}",
                        slave_kind])
        if not list_out:
            out.append({"name": slave_kind, "digest": {}})
            continue
        # Pick the tag whose repo == slave_kind (skip -bhouse user variants
        # if both exist, by sorting and taking the bare repo first).
        candidates = []
        for line in list_out.splitlines():
            repo_tag, _, image_id = line.partition("\t")
            repo = repo_tag.partition(":")[0]
            if repo == slave_kind and image_id:
                candidates.append((repo_tag, image_id))
        if not candidates:
            out.append({"name": slave_kind, "digest": {}})
            continue
        repo_tag, image_id = candidates[0]
        digest = image_id.split(":", 1)[1] if image_id.startswith("sha256:") else image_id
        out.append({
            "name": f"pkg:docker/{repo_tag}",
            "digest": {"sha256": digest} if digest else {},
        })
    return out


# ---------------------------------------------------------------------------
# Repo identity
# ---------------------------------------------------------------------------


def git_commit() -> Optional[str]:
    return run(["git", "rev-parse", "HEAD"])


def git_remote_url() -> Optional[str]:
    url = run(["git", "config", "--get", "remote.origin.url"])
    if not url:
        return None
    # Normalize SSH form to https URI for SLSA.
    m = re.match(r"git@([^:]+):(.+?)(?:\.git)?$", url)
    if m:
        return f"https://{m.group(1)}/{m.group(2)}"
    return url.rstrip("/").removesuffix(".git")


# ---------------------------------------------------------------------------
# SBOM linkage
# ---------------------------------------------------------------------------


def load_sbom_metadata(cdx_path: str) -> dict:
    """Pull bom-ref and serialNumber out of the SBOM for cross-reference."""
    try:
        with open(cdx_path) as f:
            doc = json.load(f)
    except Exception as e:
        warn(f"could not read {cdx_path}: {e}")
        return {}
    meta = doc.get("metadata", {})
    return {
        "bom_ref": meta.get("component", {}).get("bom-ref"),
        "serial_number": doc.get("serialNumber"),
        "spec_version": doc.get("specVersion"),
        "version": doc.get("version"),
    }


# ---------------------------------------------------------------------------
# Top-level provenance construction
# ---------------------------------------------------------------------------


_EXTERNAL_PARAM_VARS = [
    "CONFIGURED_PLATFORM",
    "CONFIGURED_ARCH",
    "TARGET_BOOTLOADER",
    "BLDENV",
    "ENABLE_SBOM",
    "SBOM_FORMAT",
    "SBOM_SCAN_TOOL",
    "SBOM_INCLUDE_LICENSES",
    "INCLUDE_KUBERNETES",
    "INCLUDE_PTF",
    "INCLUDE_PDE",
    "INCLUDE_MGMT_FRAMEWORK",
    "INCLUDE_DHCP_RELAY",
    "INCLUDE_DHCP_SERVER",
    "INCLUDE_MACSEC",
    "INCLUDE_NAT",
    "INCLUDE_SFLOW",
    "INCLUDE_FIPS",
    "INCLUDE_EXTERNAL_PATCHES",
    "SONIC_VERSION_CONTROL_COMPONENTS",
    "BUILD_PUBLIC_URL",
    "BUILD_SNAPSHOT_URL",
    "MIRROR_URLS",
    "MIRROR_SECURITY_URLS",
    "MIRROR_SNAPSHOT",
]


def build_provenance(
    bin_path: str, cdx_path: Optional[str],
) -> dict:
    bin_digest = file_sha256(bin_path)
    subject = [{
        "name": os.path.basename(bin_path),
        "digest": {"sha256": bin_digest} if bin_digest else {},
    }]

    external_params = {
        k: os.environ.get(k, "")
        for k in _EXTERNAL_PARAM_VARS
        if os.environ.get(k)
    }
    # The target identity is itself a build input.
    external_params["target"] = os.path.basename(bin_path)

    resolved_deps = []
    commit = git_commit()
    remote = git_remote_url()
    if remote and commit:
        resolved_deps.append({
            "uri": f"git+{remote}",
            "digest": {"gitCommit": commit},
        })
    # Cross-reference the SBOM as a resolved dependency.
    if cdx_path and os.path.isfile(cdx_path):
        cdx_sha = file_sha256(cdx_path)
        sbom_meta = load_sbom_metadata(cdx_path)
        ref = {
            "uri": "file://" + os.path.relpath(cdx_path),
            "digest": {"sha256": cdx_sha} if cdx_sha else {},
        }
        if sbom_meta.get("serial_number"):
            ref["name"] = sbom_meta["serial_number"]
        resolved_deps.append(ref)

    # Slave docker images = builder identity.
    slaves = find_slave_images()
    builder: dict = {
        "id": "https://github.com/sonic-net/sonic-buildimage/blob/master/Makefile",
    }
    if slaves:
        builder["builderDependencies"] = slaves

    metadata: dict = {
        "invocationId": os.environ.get(
            "BUILD_NUMBER",
            os.environ.get("BUILD_TIMESTAMP", "local-build"),
        ),
    }
    ts = reproducible_timestamp()
    if ts:
        # SLSA spec uses startedOn and finishedOn; we set both to the
        # same epoch-derived value for reproducibility.
        metadata["startedOn"] = ts
        metadata["finishedOn"] = ts

    predicate = {
        "buildDefinition": {
            "buildType": "https://github.com/sonic-net/sonic-buildimage/build/v1",
            "externalParameters": external_params,
            "internalParameters": {
                "host_arch": run(["dpkg", "--print-architecture"]) or "",
            },
            "resolvedDependencies": resolved_deps,
        },
        "runDetails": {
            "builder": builder,
            "metadata": metadata,
        },
    }

    statement = {
        "_type": "https://in-toto.io/Statement/v1",
        "subject": subject,
        "predicateType": "https://slsa.dev/provenance/v1",
        "predicate": predicate,
    }
    return statement


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--bin", required=True,
                    help="Path to the installer image (e.g. target/sonic-broadcom.bin)")
    ap.add_argument("--sbom",
                    help="Path to the sibling CycloneDX SBOM "
                         "(default: <bin>.cdx.json)")
    ap.add_argument("--output",
                    help="Where to write the .intoto.json "
                         "(default: <bin>.intoto.json)")
    args = ap.parse_args()

    if not os.path.isfile(args.bin):
        warn(f"bin not found: {args.bin}")
        return 0

    cdx_path = args.sbom or (args.bin + ".cdx.json")
    out_path = args.output or (args.bin + ".intoto.json")

    statement = build_provenance(args.bin, cdx_path=cdx_path)

    try:
        with open(out_path, "w") as f:
            json.dump(statement, f, indent=2, sort_keys=True)
        info(f"wrote {out_path}")
    except Exception as e:
        warn(f"could not write {out_path}: {e}")
        return 0
    return 0


if __name__ == "__main__":
    sys.exit(main())
