#!/usr/bin/env python3
"""Tests for docker_archive_to_oci_layout.py.
"""

import hashlib
import io
import json
import subprocess
import sys
import tarfile
import tempfile
from pathlib import Path

SCRIPT = Path(__file__).with_name("docker_archive_to_oci_layout.py")


def _add_bytes(tar: tarfile.TarFile, name: str, data: bytes) -> None:
    info = tarfile.TarInfo(name)
    info.size = len(data)
    tar.addfile(info, io.BytesIO(data))


def make_layer_tar(files: dict[str, str]) -> bytes:
    """An uncompressed layer tar -- its sha256 is the layer's diff_id."""
    buf = io.BytesIO()
    with tarfile.open(fileobj=buf, mode="w") as tar:
        for name, content in files.items():
            _add_bytes(tar, name, content.encode())
    return buf.getvalue()


def build_legacy_archive(path: Path, config_bytes: bytes, layer_bytes: bytes) -> None:
    with tarfile.open(path, "w:gz") as tar:
        _add_bytes(tar, "config.json", config_bytes)
        _add_bytes(tar, "layer0/layer.tar", layer_bytes)
        manifest = [
            {
                "Config": "config.json",
                "RepoTags": ["test:latest"],
                "Layers": ["layer0/layer.tar"],
            }
        ]
        _add_bytes(tar, "manifest.json", json.dumps(manifest).encode())


def build_oci_archive(path: Path, layout_dir: Path) -> None:
    with tarfile.open(path, "w:gz") as tar:
        for child in sorted(layout_dir.rglob("*")):
            if child.is_file():
                tar.add(child, arcname=str(child.relative_to(layout_dir)))


def run_converter(src: Path, out: Path) -> None:
    subprocess.run(
        [sys.executable, str(SCRIPT), "--src", str(src), "--out", str(out)],
        check=True,
    )


def assert_valid_layout(out: Path, expect_layer_digest: str, expect_config_digest: str):
    assert json.loads((out / "oci-layout").read_text())["imageLayoutVersion"] == "1.0.0"

    index = json.loads((out / "index.json").read_text())
    md = index["manifests"][0]
    mdg = md["digest"].split(":")[1]
    mblob = (out / "blobs" / "sha256" / mdg).read_bytes()
    assert hashlib.sha256(mblob).hexdigest() == mdg, "manifest digest mismatch"
    assert len(mblob) == md["size"], "manifest size mismatch"
    assert md["platform"] == {"architecture": "amd64", "os": "linux"}, md.get("platform")

    manifest = json.loads(mblob)
    cdg = manifest["config"]["digest"].split(":")[1]
    assert cdg == expect_config_digest, f"config digest {cdg} != {expect_config_digest}"
    assert (out / "blobs" / "sha256" / cdg).exists(), "missing config blob"

    ldg = manifest["layers"][0]["digest"].split(":")[1]
    assert ldg == expect_layer_digest, f"layer digest {ldg} != {expect_layer_digest}"
    assert (out / "blobs" / "sha256" / ldg).exists(), "missing layer blob"

    config = json.loads((out / "blobs" / "sha256" / cdg).read_bytes())
    assert config["rootfs"]["diff_ids"] == [f"sha256:{ldg}"], config["rootfs"]["diff_ids"]


def test_legacy_path() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        workdir = Path(tmp)
        layer = make_layer_tar({"etc/hello": "world\n"})
        layer_digest = hashlib.sha256(layer).hexdigest()
        config_bytes = json.dumps(
            {
                "architecture": "amd64",
                "os": "linux",
                "rootfs": {"type": "layers", "diff_ids": [f"sha256:{layer_digest}"]},
            }
        ).encode()
        config_digest = hashlib.sha256(config_bytes).hexdigest()

        archive = workdir / "legacy.tar.gz"
        build_legacy_archive(archive, config_bytes, layer)
        out = workdir / "legacy-out"
        run_converter(archive, out)

        assert_valid_layout(out, layer_digest, config_digest)
        # Config must be reused byte-for-byte (no rewrite).
        assert (out / "blobs" / "sha256" / config_digest).read_bytes() == config_bytes
    print("PASS: legacy docker-archive -> synthesized OCI layout")


def build_oci_layout_dir(layout: Path) -> None:
    """A minimal hand-built OCI layout -- enough to trigger the verbatim path.

    Built from scratch (not from a prior conversion) so this test does not
    depend on the legacy test or its output.
    """
    blobs = layout / "blobs" / "sha256"
    blobs.mkdir(parents=True)
    blob = b'{"hello":"oci"}'
    digest = hashlib.sha256(blob).hexdigest()
    (blobs / digest).write_bytes(blob)
    (layout / "oci-layout").write_text(json.dumps({"imageLayoutVersion": "1.0.0"}))
    (layout / "index.json").write_text(
        json.dumps(
            {
                "schemaVersion": 2,
                "mediaType": "application/vnd.oci.image.index.v1+json",
                "manifests": [{"digest": f"sha256:{digest}", "size": len(blob)}],
            }
        )
    )


def test_oci_verbatim_path() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        workdir = Path(tmp)
        layout = workdir / "layout"
        build_oci_layout_dir(layout)
        archive = workdir / "oci.tar.gz"
        build_oci_archive(archive, layout)
        out = workdir / "oci-out"
        run_converter(archive, out)

        for original in sorted(layout.rglob("*")):
            if original.is_file():
                copied = out / original.relative_to(layout)
                assert copied.read_bytes() == original.read_bytes(), copied
    print("PASS: already-OCI archive -> copied verbatim")


def test_malformed_manifest_rejected() -> None:
    """A manifest.json missing 'Layers' must fail loudly, not silently."""
    with tempfile.TemporaryDirectory() as tmp:
        workdir = Path(tmp)
        archive = workdir / "bad.tar.gz"
        with tarfile.open(archive, "w:gz") as tar:
            _add_bytes(tar, "config.json", b"{}")
            bad_manifest = [{"Config": "config.json", "RepoTags": ["x:latest"]}]
            _add_bytes(tar, "manifest.json", json.dumps(bad_manifest).encode())

        result = subprocess.run(
            [sys.executable, str(SCRIPT), "--src", str(archive), "--out", workdir / "bad-out"],
            capture_output=True,
            text=True,
        )
        assert result.returncode != 0, "converter accepted a manifest with no 'Layers'"
        assert "Layers" in result.stderr, result.stderr
    print("PASS: malformed manifest.json rejected")


def main() -> None:
    # Each test is self-contained (own temp dir, own fixtures) and may run in
    # any order, independently.
    test_legacy_path()
    test_oci_verbatim_path()
    test_malformed_manifest_rejected()
    print("All tests passed.")


if __name__ == "__main__":
    main()
