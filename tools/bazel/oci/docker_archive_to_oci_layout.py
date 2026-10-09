#!/usr/bin/env python3
"""Convert a docker-archive tarball (the output of `docker save`) into an OCI
image layout directory usable as `oci_image(base = ...)` in rules_oci.

`docker save` emits the docker-archive format (`manifest.json` +
`<config>.json` + uncompressed layer tars), while rules_oci's `base`
attribute expects an OCI image *layout* (`oci-layout`, `index.json`,
`blobs/sha256/<digest>`). This bridges the two with no external tooling
(no skopeo/crane/rules_python) -- stdlib only.

Usage:
    docker_archive_to_oci_layout.py --src <docker-archive.tar[.gz]> --out <dir>
"""

import argparse
import hashlib
import json
import tarfile
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import IO, Any

_CHUNK = 1024 * 1024

# OCI media types (https://github.com/opencontainers/image-spec).
_MT_INDEX = "application/vnd.oci.image.index.v1+json"
_MT_MANIFEST = "application/vnd.oci.image.manifest.v1+json"
_MT_CONFIG = "application/vnd.oci.image.config.v1+json"
# `docker save` writes uncompressed layer tars, so the uncompressed-layer media
# type applies and each blob digest equals the config's matching diff_id.
_MT_LAYER = "application/vnd.oci.image.layer.v1.tar"


@dataclass
class Descriptor:
    """An OCI content descriptor (image-spec/descriptor.md).

    Only the fields this converter emits are modelled: the three required ones,
    plus the optional `platform` used on image-index manifest entries.
    """

    media_type: str
    digest: str  # full "sha256:<hex>" reference
    size: int
    platform: dict[str, str] | None = None

    @classmethod
    def for_blob(
        cls,
        media_type: str,
        sha256_hex: str,
        size: int,
        platform: dict[str, str] | None = None,
    ) -> "Descriptor":
        """Build a descriptor from a bare hex digest (adds the `sha256:` prefix)."""
        return cls(media_type, f"sha256:{sha256_hex}", size, platform)

    def to_json(self) -> dict[str, Any]:
        out: dict[str, Any] = {
            "mediaType": self.media_type,
            "digest": self.digest,
            "size": self.size,
        }
        if self.platform is not None:
            out["platform"] = self.platform
        return out


def is_oci_layout(tar: tarfile.TarFile) -> bool:
    """True if the archive is already an OCI layout (e.g. a containerd export)."""
    names = set(tar.getnames())
    return "oci-layout" in names and "index.json" in names


def make_path_validating_filter(out_dir: Path):
    """Build an `extractall` filter that rejects path-traversal members."""
    root = out_dir.resolve()

    def _within_root(path: Path) -> bool:
        path = path.resolve()
        return path == root or path.is_relative_to(root)

    def _filter(member: tarfile.TarInfo, dest_path: str) -> tarfile.TarInfo:
        if not _within_root(root / member.name):
            raise ValueError(f"unsafe path in archive: {member.name!r} escapes {root}")

        link_base = root # default for hardlinks
        if member.issym(): # symlink
            link_base = (root / member.name).parent

        if not _within_root(link_base / member.linkname):
            raise ValueError(
                f"unsafe link in archive: {member.name!r} -> {member.linkname!r}"
            )
        return member

    return _filter


def write_blob_from_stream(fileobj: IO[bytes], out_dir: Path) -> tuple[str, int]:
    """Stream `fileobj` to `blobs/sha256/<digest>`; return (digest, size).

    Hashes while writing to a temp file, then renames into place, so a layer is
    never held entirely in memory (layers can be hundreds of MB).
    """
    blobs = out_dir / "blobs" / "sha256"
    blobs.mkdir(parents=True, exist_ok=True)
    digest = hashlib.sha256()
    size = 0
    fd, tmp_name = tempfile.mkstemp(dir=blobs)
    tmp = Path(tmp_name)
    try:
        with open(fd, "wb") as out:
            while chunk := fileobj.read(_CHUNK):
                digest.update(chunk)
                size += len(chunk)
                out.write(chunk)
        tmp.replace(blobs / digest.hexdigest())
    except BaseException:
        tmp.unlink(missing_ok=True)
        raise
    return digest.hexdigest(), size


def write_blob_from_bytes(data: bytes, out_dir: Path) -> tuple[str, int]:
    """Write `data` to `blobs/sha256/<digest>`; return (digest, size)."""
    blobs = out_dir / "blobs" / "sha256"
    blobs.mkdir(parents=True, exist_ok=True)
    digest = hashlib.sha256(data).hexdigest()
    (blobs / digest).write_bytes(data)
    return digest, len(data)


def convert_docker_archive(tar: tarfile.TarFile, out_dir: Path) -> None:
    """Synthesize an OCI layout from a legacy `manifest.json`-based archive.

    Relies on layers being uncompressed tars (always true for `docker save`):
    a layer's sha256 then equals the config's `diff_id`, so the config blob is
    reused byte-for-byte and only the OCI manifest/index are newly written.
    """
    docker_manifest = json.load(tar.extractfile("manifest.json"))
    if not isinstance(docker_manifest, list) or not docker_manifest:
        raise ValueError("manifest.json must be a non-empty list")
    entry = docker_manifest[0]
    for field in ("Config", "Layers"):
        if field not in entry:
            raise ValueError(f"manifest.json entry is missing {field!r}")
    if not isinstance(entry["Layers"], list) or not entry["Layers"]:
        raise ValueError("manifest.json 'Layers' must be a non-empty list")

    # The config is reused verbatim; its diff_ids already match our layer digests.
    config_bytes = tar.extractfile(entry["Config"]).read()
    config_digest, config_size = write_blob_from_bytes(config_bytes, out_dir)
    config = json.loads(config_bytes)

    # Layers, kept in manifest order (which matches the config's diff_id order).
    layers: list[Descriptor] = []
    for layer_name in entry["Layers"]:
        digest, size = write_blob_from_stream(tar.extractfile(layer_name), out_dir)
        layers.append(Descriptor.for_blob(_MT_LAYER, digest, size))

    manifest = {
        "schemaVersion": 2,
        "mediaType": _MT_MANIFEST,
        "config": Descriptor.for_blob(_MT_CONFIG, config_digest, config_size).to_json(),
        "layers": [layer.to_json() for layer in layers],
    }
    manifest_bytes = json.dumps(manifest, separators=(",", ":"), sort_keys=True).encode()
    manifest_digest, manifest_size = write_blob_from_bytes(manifest_bytes, out_dir)
    manifest_descriptor: Descriptor = Descriptor.for_blob(
        _MT_MANIFEST,
        manifest_digest,
        manifest_size,
        platform={
            "architecture": config.get("architecture", "amd64"),
            "os": config.get("os", "linux"),
        },
    )

    index = {
        "schemaVersion": 2,
        "mediaType": _MT_INDEX,
        "manifests": [manifest_descriptor.to_json()],
    }
    (out_dir / "index.json").write_text(json.dumps(index, separators=(",", ":")))
    (out_dir / "oci-layout").write_text(json.dumps({"imageLayoutVersion": "1.0.0"}))


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--src", required=True, type=Path, help="docker-archive .tar[.gz]"
    )
    parser.add_argument(
        "--out", required=True, type=Path, help="output OCI layout directory"
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    # The extractall below is guarded by make_path_validating_filter, which
    # rejects any member/link path that escapes args.out.
    #
    # nosemgrep: trailofbits.python.tarfile-extractall-traversal.tarfile-extractall-traversal
    with tarfile.open(args.src, "r:*") as tar:
        if is_oci_layout(tar):
            tar.extractall(args.out, filter=make_path_validating_filter(args.out))
        else:
            convert_docker_archive(tar, args.out)


if __name__ == "__main__":
    main()
